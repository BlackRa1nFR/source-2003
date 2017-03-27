// vmpi_service_install.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "vmpi_defs.h"
#include <io.h>
#include <process.h>
#include "iphelpers.h"
#include "vstdlib/strtools.h"


#define HLKM_WINDOWS_RUN_KEY		"Software\\Microsoft\\Windows\\CurrentVersion\\Run"

#define VMPI_SERVICE_VALUE_NAME		"VMPI Service"
#define VMPI_SERVICE_UI_VALUE_NAME	"VMPI Service UI"


#define Msg printf
#define Error Error2

void Error2( const char *pMsg, ... )
{
	char msg[512];
	va_list marker;
	
	va_start( marker, pMsg );
	_snprintf( msg, sizeof( msg ), pMsg, marker );
	msg[sizeof( msg ) - 1] = 0;

	va_end( marker );

	MessageBox( NULL, msg, "Error", MB_OK );
}


char* FindArg( int argc, char **argv, const char *pArgName, char *pDefaultValue="" )
{
	for ( int i=0; i < argc; i++ )
	{
		if ( stricmp( argv[i], pArgName ) == 0 )
		{
			if ( (i+1) >= argc )
				return pDefaultValue;
			else
				return argv[i+1];
		}
	}
	return NULL;
}


char* GetLastErrorString()
{
	static char err[2048];
	
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	strncpy( err, (char*)lpMsgBuf, sizeof( err ) );
	LocalFree( lpMsgBuf );

	err[ sizeof( err ) - 1 ] = 0;

	return err;
}


bool LaunchApp( char *pCommandLine, const char *pBaseDir )
{
	STARTUPINFO si;
	memset( &si, 0, sizeof( si ) );
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof( pi ) );

	return CreateProcess( NULL, pCommandLine, NULL,	NULL, FALSE, 0,	NULL, pBaseDir, &si, &pi ) != 0;
}		


bool StartAsWinApp()
{
	char filename[512], uiFilename[512];

	// Get the registry keys to tell it where to look for the exes.
	HKEY hKey = NULL;
	RegCreateKey( HKEY_LOCAL_MACHINE, HLKM_WINDOWS_RUN_KEY, &hKey );
	DWORD filenameLen = sizeof( filename );
	DWORD uiFilenameLen = sizeof( uiFilename );
	DWORD dummyType;

	if ( RegQueryValueEx( hKey, VMPI_SERVICE_VALUE_NAME, NULL, &dummyType, (LPBYTE)filename, &filenameLen ) == ERROR_SUCCESS &&
		 RegQueryValueEx( hKey, VMPI_SERVICE_UI_VALUE_NAME, NULL, &dummyType, (LPBYTE)uiFilename, &uiFilenameLen ) == ERROR_SUCCESS )
	{
		if ( LaunchApp( filename, "." ) && LaunchApp( uiFilename, "." ) )
		{
			Msg( "vmpi_service started as a Windows app.\n" );
			return true;
		}
		else
		{
			Error( "Error launching command lines:\n%s\n%s\n", filename, uiFilename );
			return false;
		}
	}
	else
	{
		Error( "Can't access registry keys:\n%s\\%s\n%s\\%s\nRun with -install first.", 
			HLKM_WINDOWS_RUN_KEY, VMPI_SERVICE_VALUE_NAME,
			HLKM_WINDOWS_RUN_KEY, VMPI_SERVICE_UI_VALUE_NAME );
		
		return false;
	}
}


bool StartVMPIService( SC_HANDLE hSCManager )
{
	bool bRet = true;

	// First, get rid of an old service with the same name.
	SC_HANDLE hMyService = OpenService( hSCManager, VMPI_SERVICE_NAME_INTERNAL, SERVICE_START );
	if ( hMyService )
	{
		if ( StartService( hMyService, NULL, NULL ) )
		{
			Msg( "Started!\n" );
		}
		else
		{
			Error( "Can't start the service.\n" );
			bRet = false;
		}
	}
	else
	{
		Error( "Can't open service: %s\n", VMPI_SERVICE_NAME_INTERNAL );
		bRet = false;
	}

	CloseServiceHandle( hMyService );
	return bRet;
}


void DeleteIsInstalledKey()
{
	// Delete the IsInstalled key.
	HKEY hKey = NULL;
	RegCreateKey( HKEY_LOCAL_MACHINE, VMPI_SERVICE_KEY, &hKey );
	if ( hKey )
	{
		DWORD val = 0;
		RegSetValueEx( 
			hKey,
			"IsInstalled",
			0,
			REG_DWORD,
			(unsigned char*)&val,
			sizeof( val ) );
	}
}


void WriteIsInstalledKey()
{
	// Write that it's installed.
	HKEY hKey = NULL;
	RegCreateKey( HKEY_LOCAL_MACHINE, VMPI_SERVICE_KEY, &hKey );
	if ( hKey )
	{
		DWORD val = 1;
		RegSetValueEx( 
			hKey,
			"IsInstalled",
			0,
			REG_DWORD,
			(unsigned char*)&val,
			sizeof( val ) );
	}
}


bool DeleteServiceApp()
{
	// Delete the old registry keys.
	HKEY hKey = NULL;
	RegCreateKey( HKEY_LOCAL_MACHINE, HLKM_WINDOWS_RUN_KEY, &hKey );
	RegDeleteValue( hKey, VMPI_SERVICE_VALUE_NAME );
	RegDeleteValue( hKey, VMPI_SERVICE_UI_VALUE_NAME );

	// Send the 
	ISocket *pSocket = CreateIPSocket();
	if ( pSocket )
	{
		if ( pSocket->BindToAny( 0 ) )
		{
			char cPacket[4] =
			{
				VMPI_PROTOCOL_VERSION,
				VMPI_PASSWORD_OVERRIDE,	// (force it to accept this message).
				0,
				VMPI_STOP_SERVICE
			};
			
			CIPAddr addr( 127, 0, 0, 1, 0 );
			
			for ( int iPort=VMPI_SERVICE_PORT; iPort <= VMPI_LAST_SERVICE_PORT; iPort++ )
			{
				addr.port = iPort;
				pSocket->SendTo( &addr, cPacket, sizeof( cPacket ) );
			}
			
			// Give it a sec to get the message and shutdown in case we're restarting.
			Sleep( 2000 );
			
			
			// This is the overkill method. If it didn't shutdown gracefully, kill it.
			HMODULE hInst = LoadLibrary( "psapi.dll" );
			if ( hInst )
			{
				typedef BOOL (WINAPI *EnumProcessesFn)(DWORD *lpidProcess, DWORD cb, DWORD *cbNeeded);
				typedef BOOL (WINAPI *EnumProcessModulesFn)(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded );
				typedef DWORD (WINAPI *GetModuleBaseNameFn)( HANDLE hProcess, HMODULE hModule, LPTSTR lpBaseName, DWORD nSize );
				
				EnumProcessesFn EnumProcesses = (EnumProcessesFn)GetProcAddress( hInst, "EnumProcesses" );
				EnumProcessModulesFn EnumProcessModules = (EnumProcessModulesFn)GetProcAddress( hInst, "EnumProcessModules" );
				GetModuleBaseNameFn GetModuleBaseName = (GetModuleBaseNameFn)GetProcAddress( hInst, "GetModuleBaseNameA" );
				if ( EnumProcessModules && EnumProcesses )
				{				
					// Now just to make sure, kill the processes we're interested in.
					DWORD procIDs[1024];
					DWORD nBytes;
					if ( EnumProcesses( procIDs, sizeof( procIDs ), &nBytes ) )
					{
						DWORD nProcs = nBytes / sizeof( procIDs[0] );
						for ( DWORD i=0; i < nProcs; i++ )
						{
							HANDLE hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, procIDs[i] );
							if ( hProc )
							{
								HMODULE hModules[1024];
								if ( EnumProcessModules( hProc, hModules, sizeof( hModules ), &nBytes ) )
								{
									DWORD nModules = nBytes / sizeof( hModules[0] );
									for ( DWORD iModule=0; iModule < nModules; iModule++ )
									{
										char filename[512];
										if ( GetModuleBaseName( hProc, hModules[iModule], filename, sizeof( filename ) ) )
										{
											if ( Q_stristr( filename, "vmpi_service.exe" ) || Q_stristr( filename, "vmpi_service_ui.exe" ) )
											{
												TerminateProcess( hProc, 1 );
												CloseHandle( hProc );
												hProc = NULL;
												break;
											}
										}
									}
								}

								CloseHandle( hProc );
							}
						}
					}
				}

				FreeLibrary( hInst );
			}
		}

		pSocket->Release();
	}

	DeleteIsInstalledKey();
	return true;
}


bool DeleteOldService( SC_HANDLE hSCManager )
{
	bool bRet = true;

	// First, get rid of an old service with the same name.
	SC_HANDLE hOldService = OpenService( hSCManager, VMPI_SERVICE_NAME_INTERNAL, SERVICE_STOP | DELETE );
	if ( hOldService )
	{
		// Stop the service.
		Msg( "Found the service already running.\n" );
		Msg( "Stopping service...\n" );
		SERVICE_STATUS status;
		ControlService( hOldService, SERVICE_CONTROL_STOP, &status );
		
		Msg( "Deleting service...\n" );
		if ( DeleteService( hOldService ) )
		{
			Msg( "Deleted old service.\n" );
		}
		else
		{
			Error( "Couldn't delete the old '%s' service!\n", VMPI_SERVICE_NAME );
			bRet = false;
		}

		CloseServiceHandle( hOldService );
	}

	
	if ( bRet )
		DeleteIsInstalledKey();

	return bRet;
}


bool SetupFilenames( const char *pBaseDir, char filename[512], char uiFilename[512] )
{
	_snprintf( filename, 512, "%s\\vmpi_service.exe", pBaseDir );
	_snprintf( uiFilename, 512, "%s\\vmpi_service_ui.exe", pBaseDir );

	if ( _access( filename, 0 ) || _access( uiFilename, 0 ) )
	{
		const char *pFilename = filename;
		const char *pUIFilename = uiFilename;

		Error( "Can't find %s or %s.\n", pFilename, pUIFilename );
		return false;
	}

	return true;
}


bool InstallAsWindowsApp( const char *pBaseDir )
{
	char filename[512], uiFilename[512];
	if ( !SetupFilenames( pBaseDir, filename, uiFilename ) )
		return false;

	char serviceCmdLine[512];
	_snprintf( serviceCmdLine, sizeof( serviceCmdLine ), "\"%s\" -console", filename );

	// Add registry keys to run these two when they logon.
	HKEY hUIKey = NULL;
	RegCreateKey( HKEY_LOCAL_MACHINE, HLKM_WINDOWS_RUN_KEY, &hUIKey );
	if ( !hUIKey || 
		RegSetValueEx( hUIKey, VMPI_SERVICE_VALUE_NAME, 0, REG_SZ, (unsigned char*)serviceCmdLine, strlen( serviceCmdLine ) + 1 ) != ERROR_SUCCESS ||
		RegSetValueEx( hUIKey, VMPI_SERVICE_UI_VALUE_NAME, 0, REG_SZ, (unsigned char*)uiFilename, strlen( uiFilename) + 1 ) != ERROR_SUCCESS )
	{
		Error( "Can't install registry key for %s\n", uiFilename );
		return false;
	}

	WriteIsInstalledKey();
	return true;
}


// This function registers the service with the service manager.
bool InstallService( SC_HANDLE hSCManager, const char *pBaseDir )
{
	char filename[512], uiFilename[512];
	if ( !SetupFilenames( pBaseDir, filename, uiFilename ) )
		return false;


	// Try a to reinstall the service for up to 5 seconds.
	Msg( "Creating new service...\n" );

	SC_HANDLE hMyService = NULL;
	DWORD startTime = GetTickCount();
	while ( GetTickCount() - startTime < 5000 )
	{
		// Now reinstall it.
		hMyService = CreateService(
			hSCManager,				// Service Control Manager database.
			VMPI_SERVICE_NAME_INTERNAL,		// Service name.
			VMPI_SERVICE_NAME,		// Display name.
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			SERVICE_AUTO_START,		// Start automatically on system bootup.
			SERVICE_ERROR_NORMAL,
			filename,				// Executable to register for the service.
			NULL,					// no load ordering group 
			NULL,					// no tag identifier 
			NULL,					// no dependencies 
			"valve\\VMPI",			// account
			"Crunchin!"				// password 
//			NULL,					// account
//			NULL					// password 
			);
	
		if ( hMyService )
			break;
		else
			Sleep( 300 );
	}

	if ( !hMyService )
	{
		Error( "CreateService failed (%s)!\n", GetLastErrorString() );
		CloseServiceHandle( hSCManager );
		return false;
	}


	// Now setup the UI executable to run when their system starts.
	HKEY hUIKey = NULL;
	RegCreateKey( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hUIKey );
	if ( !hUIKey || RegSetValueEx( hUIKey, "VMPI Service UI", 0, REG_SZ, (unsigned char*)uiFilename, strlen( uiFilename) + 1 ) != ERROR_SUCCESS )
	{
		Error( "Can't install registry key for %s\n", uiFilename );
		return false;
	}


	WriteIsInstalledKey();

	// Start the service.
	Msg( "Starting service...\n" );
	if ( StartService( hMyService, NULL, NULL ) )
	{
		Msg( "Started service!\n" );
		Msg( "Starting UI...\n" );

		if ( LaunchApp( uiFilename, pBaseDir ) )
		{
			Msg( "UI started!\n" );
		}
		else
		{
			Error( "Error starting UI\n%s\n", GetLastErrorString() );
		}
	}
	else
	{
		Error( "Error in StartService:\n%s\n", GetLastErrorString() );
	}

	CloseServiceHandle( hMyService );
	return true;
}


int PrintUsage()
{
	Error( "vmpi_service_install -install <dir with exes> -delete -start -WinApp\n" );
	return 1;
}


//int main(int argc, char* argv[])

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	int argc = __argc;
	char **argv = __argv;

	if ( argc <= 1 )
	{
		return PrintUsage();
	}


	SC_HANDLE hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if ( !hSCManager )
	{
		Msg( "OpenSCManager failed (%s)!\n", GetLastErrorString() );
		return false;
	}

	
	int ret = 0; // 0 = success


	bool bWinApp = (FindArg( argc, argv, "-WinApp" ) != 0);
	const char *pBaseDir = FindArg( argc, argv, "-install" );

	// Stop the current app if it's already running.
	if ( pBaseDir || FindArg( argc, argv, "-delete" ) )
	{
		// Delete both the service and the win app.
		ret = !DeleteServiceApp() || !DeleteOldService( hSCManager );
	}

	if ( pBaseDir )
	{
		if ( bWinApp )
		{
			ret = !InstallAsWindowsApp( pBaseDir );
		}
		else
		{
			ret = !InstallService( hSCManager, pBaseDir );
		}
	}
	
	
	// If still successful, then start the service if they want.
	if( ret == 0 && FindArg( argc, argv, "-start" ) )
	{
		if ( bWinApp )
		{
			ret = !StartAsWinApp();
		}
		else
		{
			ret = !StartVMPIService( hSCManager );
		}
	}


	CloseServiceHandle( hSCManager );
	return ret;
}

