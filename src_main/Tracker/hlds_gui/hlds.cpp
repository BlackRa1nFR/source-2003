#include <windows.h> 
#include "hlds.h"
#include <VGUI_ISystem.h>
#include <VGUI_Controls.h>
#include "FileSystem.h"

CHLDSServer server;

#ifdef _WIN32
static const char *g_pszengine = "swds.dll";
#endif

// System Memory & Size
static unsigned char	*gpMemBase = NULL;
static int				giMemSize = 0x2000000;  // 32 Mb default heapsize

#ifdef _WIN32
// This is a big chunk of uninitialized memory that we reserve for loading blobs into.
// For more information, see LoadBlob.cpp in the engine
char g_rgchBlob[0x03800000];
#endif	// _WIN32

DWORD WINAPI ServerThreadFunc( LPVOID threadobject );

void Sys_Sleep( int msec )
{
	Sleep( msec );
}


int Sys_GetExecutableName( char *out )
{
	if ( !::GetModuleFileName( ( HINSTANCE )GetModuleHandle( NULL ), out, 256 ) )
	{
		return 0;
	}

	return 1;
}

/*
==============
Sys_ErrorMessage

Engine is erroring out, display error in message box
==============
*/
void Sys_ErrorMessage( int level, const char *msg )
{
	server.Sys_Printf(const_cast<char *>(msg));
//	MessageBox( NULL, msg, "Half-Life", MB_OK );
//	PostQuitMessage(0);	
}

/*
==============
UpdateStatus

Update status line at top of console if engine is running
==============
*/
void CHLDSServer::UpdateStatus( int force )
{
	static double tLast = 0.0;
	double	tCurrent;
	int		n, nMax;
	char	szMap[32];
	float	fps;

	if ( !engineAPI )
		return;

	tCurrent = (float)( vgui::system()->GetTimeMillis() / 1000.0f );

	engineAPI->UpdateStatus( &fps, &n, &nMax, szMap );

	if ( !force )
	{
		if ( ( tCurrent - tLast ) < 0.5f )
			return;
	}
	

	tLast = tCurrent;
	if(window) window->UpdateStatus(fps,szMap,nMax,n);

// do something with szPrompt here
}


/*
==============
Sys_Printf

Engine is printing to console
==============
*/
void CHLDSServer::Sys_Printf(char *fmt, ...)
{
	// Dump text to debugging console.
	va_list argptr;
	char szText[1024];

	va_start (argptr, fmt);
	vsnprintf (szText, sizeof(szText), fmt, argptr);
	va_end (argptr);

	// Get Current text and append it.
	if(window)	window->ConsoleText(szText);
}


void CHLDSServer::Start(const char *txt,const char *cvIn)
{
	strncpy(cmdline,txt,1024);
	if(cvIn) strncpy(cvars,cvIn,1024);
	m_hShutdown	= CreateEvent( NULL, TRUE, FALSE, NULL );
	m_hThread = CreateThread( NULL, 0, ServerThreadFunc, (void *)this, 0, &m_nThreadId );

}

void CHLDSServer::Stop()
{
//	SetEvent( m_hShutdown );

	if(GetEngineAPI())
	{
		GetEngineAPI()->AddConsoleText("quit\n");
	}
	engineAPI=NULL;
	window=NULL;

	// Check for shutdown event
	while( WAIT_OBJECT_0 != WaitForSingleObject( m_hShutdown, 100 ) )
	{
		Sleep( 10 );
	}
	CloseHandle( m_hThread );
	CloseHandle( m_hShutdown );



}



//-----------------------------------------------------------------------------
// Purpose: Return the directory where this .exe is running from
// Output : char
//-----------------------------------------------------------------------------
char *UTIL_GetBaseDir( void )
{
	static char	basedir[ MAX_PATH ];
	int j;
	char *pBuffer = NULL;

	basedir[ 0 ] = 0;

	if ( Sys_GetExecutableName( basedir ) )
	{

		pBuffer = strrchr( basedir,'\\' );

		if ( *pBuffer )
		{
			*(pBuffer+1) = '\0';
		}


		j = strlen( basedir );
		if (j > 0)
		{
			if ( ( basedir[ j-1 ] == '\\' ) || 
				 ( basedir[ j-1 ] == '/' ) )
			{
				basedir[ j-1 ] = 0;
			}
		}
	}

	return basedir;
}

#include "idedicatedexports.h"

class CDedicatedExports : public IDedicatedExports
{
public:
	void Sys_Printf( char *text )
	{
		server.Sys_Printf( text );
	}
};

EXPOSE_SINGLE_INTERFACE( CDedicatedExports, IDedicatedExports, VENGINE_DEDICATEDEXPORTS_API_VERSION );


//-----------------------------------------------------------------------------
// Purpose: the main loop for the dedicated server
// Output : value to return to shell
//-----------------------------------------------------------------------------
DWORD WINAPI ServerThreadFunc( LPVOID threadobject )
{
	int		iret = DLL_NORMAL;
	IDedicatedServerAPI *engineAPI;

	char cur[1024];
	_getcwd(cur,1024);

	CSysModule *engineModule = NULL;
	CreateInterfaceFn engineFactory = NULL;
	
	while(iret!=DLL_CLOSE )
	{
	
		//_chdir(UTIL_GetBaseDir());
		vgui::filesystem()->Unmount();
		
		engineModule = Sys_LoadModule( g_pszengine );
	
	
		if ( !engineModule )
		{
			goto cleanup;
		}

		engineFactory = Sys_GetFactory( engineModule );
		if ( engineFactory )
		{
			engineAPI = ( IDedicatedServerAPI * )engineFactory( VENGINE_HLDS_API_VERSION, NULL );
		}

		if ( !engineAPI )
		{
			goto cleanup;
		}
		server.SetEngineAPI(engineAPI);
	
		engineAPI->Init( UTIL_GetBaseDir(),server.GetCmdline(), Sys_GetFactoryThis() );

		vgui::filesystem()->Mount();

		if(strlen(server.GetCvars())>0)
		{
			engineAPI->AddConsoleText(server.GetCvars());
			server.ResetCvars();
		}


		while ( 1 )
		{
			// Try to allow other apps to get some CPU
			Sys_Sleep( 1 );
			
			// Check for shutdown event
			if ( !engineAPI->RunFrame() )
				break;
			server.UpdateStatus( 0 );
 
		}

		server.SetEngineAPI(NULL);
		server.SetInstance(NULL);
		iret = engineAPI->Shutdown();

		Sys_UnloadModule( engineModule );

	} // while(iret != DLL_CLOSE && gbAppHasBeenTerminated==FALSE )

cleanup:
	// this line is IMPORTANT!
	SetEvent(server.GetShutDownHandle()); // tell the main window we have shut down now 

	//return 0;
	ExitThread(0);

}
