
// This program is meant to be left running on a machine.

// It sits in a while loop doing these steps:
// -	Read the vmapbuilder.cfg file, which has a list of .vmf files, their matching .bsp files,
//		and .vmf file dates telling the last time vmapbuilder built the file.
// -	Grab the top filename, move it to the bottom, and re-save the cfg file.
// -	Grab the .VMF file. If its date does not match the one from the .cfg file, then proceed,
//		otherwise, start back at step 1.
// -	Grab the HL2 or TF2 trees.
// -	Run vbsp, vrad, and vvis on the map.
// -	Checks out and checks in the .bsp.

// Note: the way the program reads and writes the .cfg file allows there to be any
// number of machines processing the list of maps, as long as they all are reading
// from the same vmapbuilder.cfg file. This can reduce turnaround time greatly.

#include "stdafx.h"
#include "utlvector.h"


#define MAX_FILENAME_LEN	256


HANDLE g_hOutputFile = 0;


// -------------------------------------------------------------------------------- //
// Config file reader.
// -------------------------------------------------------------------------------- //

class CConfigFile
{
public:

	class EMailAddress
	{
	public:
		char		m_EMailAddress[MAX_FILENAME_LEN];
	};

	class Entry
	{
	public:
		enum {MAX_EMAIL_ADDRESSES=128};

						Entry()
						{
							m_nEMailAddresses = 0;
							m_bFastVis = false;
						}

		EMailAddress	m_EMailAddresses[MAX_EMAIL_ADDRESSES];
		int				m_nEMailAddresses;

		char			m_Filename[MAX_FILENAME_LEN];
		char			m_VMFPath[MAX_FILENAME_LEN];
		long			m_VMFTime;	// Last time the VMF file was successfully reprocessed.
		bool			m_bFastVis;
	};


public:

	void			Term();	
	bool			Read( char const *pFilename );
	bool			Write( char const *pFilename );
	Entry*			FindEntryByFilename( char const *pFilename );



public:
	
	char			m_SSDatabase[MAX_FILENAME_LEN];			// \\jeeves\hl2vss
	char			m_SSResourcePath[MAX_FILENAME_LEN];		// $/HL2/Release/Dev
	char			m_SSBSPPath[MAX_FILENAME_LEN];			// $/HL2/Release/Dev/hl2/maps

	CUtlVector<Entry>	m_Entries;


private:

	// Fill in pDest with the next token. Return false if there is no next token.
	bool			GetNextToken( FILE *fp, char *pDest, int destLen );
	
	// Return true if the next token is equal to pTest.
	bool			VerifyNextToken( FILE *fp, char const *pTest );

	bool			IsWhitespace( char ch );
};


void CConfigFile::Term()
{	
	m_Entries.Purge();
	m_SSDatabase[0] = m_SSResourcePath[0] = m_SSBSPPath[0] = 0;	
}


bool CConfigFile::Read( char const *pFilename )
{
	Term();

	// Read in the whole file.
	FILE *fp = fopen( pFilename, "rt" );
	if( !fp )
		return false;

	int iCurPos = 0;
	if( !VerifyNextToken( fp, "ssdatabase" ) || 
		!GetNextToken( fp, m_SSDatabase, sizeof(m_SSDatabase) ) )
	{
		fclose( fp );
		return false;
	}

	if( !VerifyNextToken( fp, "resourcepath" )  || 
		!GetNextToken( fp, m_SSResourcePath, sizeof(m_SSResourcePath) ) )
	{
		fclose( fp );
		return false;
	}

	if( !VerifyNextToken( fp, "bsppath" ) ||
		!GetNextToken( fp, m_SSBSPPath, sizeof(m_SSBSPPath) ) )
	{
		fclose( fp );
		return false;
	}

	while( VerifyNextToken( fp, "file" ) )
	{
		CConfigFile::Entry entry;
		char timeStr[512];
		if( !GetNextToken( fp, entry.m_Filename, sizeof(entry.m_Filename) ) ||
			!GetNextToken( fp, entry.m_VMFPath, sizeof(entry.m_VMFPath) ) ||
			!GetNextToken( fp, timeStr, sizeof(timeStr) ) 
		)
		{
			fclose( fp );
			return false;
		}
		
		entry.m_VMFTime = atoi( timeStr );

		// Read the email addresses.
		while(1)
		{		 
			char token[1024];

			int curPos = ftell( fp );
			if( entry.m_nEMailAddresses < CConfigFile::Entry::MAX_EMAIL_ADDRESSES && 
				GetNextToken( fp, token, sizeof(token) ) )
			{
				if( stricmp( token, "-email" ) == 0 )
				{
					CConfigFile::EMailAddress *addr = &entry.m_EMailAddresses[entry.m_nEMailAddresses];
					if( !GetNextToken( fp, addr->m_EMailAddress, sizeof(addr->m_EMailAddress) ) )
					{
						delete addr;
						fclose( fp );
						return false;
					}

					++entry.m_nEMailAddresses;
					continue;
				}
				else if( stricmp( token, "-fast" ) == 0 )
				{
					entry.m_bFastVis = true;
					continue;
				}
			}
			
			// No more options..
			fseek( fp, curPos, SEEK_SET );
			break;
		}

		m_Entries.AddToTail( entry );
	}

	fclose( fp );
	return true;
}


bool CConfigFile::Write( char const *pFilename )
{
	FILE *fp = fopen( pFilename, "wt" );
	if( !fp )
		return false;

	fprintf( fp, "ssdatabase %s\n", m_SSDatabase );
	fprintf( fp, "resourcepath %s\n", m_SSResourcePath );
	fprintf( fp, "bsppath %s\n", m_SSBSPPath );

	for( int i=0; i < m_Entries.Size(); i++ )
	{
		CConfigFile::Entry *pEntry = &m_Entries[i];
		fprintf( fp, "file %s %s %d", pEntry->m_Filename, pEntry->m_VMFPath, pEntry->m_VMFTime );
		
		for( int e=0; e < pEntry->m_nEMailAddresses; e++ )
			fprintf( fp, " -email %s", pEntry->m_EMailAddresses[e].m_EMailAddress );
		
		if( pEntry->m_bFastVis )
			fprintf( fp, " -fast" );

		fprintf( fp, "\n" );
	}

	fclose( fp );
	return true;
}


CConfigFile::Entry* CConfigFile::FindEntryByFilename( char const *pFilename )
{
	for( int i=0; i < m_Entries.Size(); i++ )
	{
		if( stricmp( m_Entries[i].m_Filename, pFilename ) == 0 )
			return &m_Entries[i];
	}

	return NULL;
}


bool CConfigFile::GetNextToken( FILE *fp, char *pDest, int destLen )
{
	// Eat up whitespace..
	char ch = 0;
	while( IsWhitespace( ch = fgetc(fp) )  )
	{
		;
	}

	if( ch == EOF )
		return false;

	int iDestPos = 0;
	while( iDestPos < destLen )
	{
		pDest[iDestPos] = ch;
		++iDestPos;
		
		ch = fgetc( fp );
		if( IsWhitespace( ch ) || ch == EOF )
		{
			pDest[iDestPos] = 0;
			return true;
		}
	}

	return false;
}


bool CConfigFile::VerifyNextToken( FILE *fp, char const *pTest )
{
	char token[2048];
	if( !GetNextToken( fp, token, sizeof(token) ) )
		return false;

	return stricmp( token, pTest ) == 0;
}


bool CConfigFile::IsWhitespace( char ch )
{
	return ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t';
}


// -------------------------------------------------------------------------------- //
// Global code.
// -------------------------------------------------------------------------------- //

// Send an email to someone..
typedef ULONG (FAR PASCAL *MAPISendMailFn)(
  LHANDLE lhSession,         
  ULONG ulUIParam,           
  lpMapiMessage lpMessage,   
  FLAGS flFlags,             
  ULONG ulReserved           
); 

bool SendMail( char *pTo, char *pSubject, char *pText )
{
	char const *pLib = "MAPI32.DLL";

	char to[1024];
	_snprintf( to, sizeof(to), "SMTP:%s", pTo );

	HINSTANCE hMAPI = LoadLibrary( pLib );
	if( !hMAPI )
		return false;

	MAPISendMailFn fn = (MAPISendMailFn)GetProcAddress( hMAPI, "MAPISendMail" );
	if( !fn )
	{
		FreeLibrary( hMAPI );
		return false;
	}

	MapiRecipDesc recip =
	{
		0,			// reserved
		MAPI_TO,	// who it goes to
		to,
		to,
		0,
		0
	};	 	

	MapiMessage note = {0,            // reserved, must be 0
						(char*)pSubject,     // subject
						(char*)pText,        // note text
						NULL,         // NULL = interpersonal message
						NULL,         // no date; MAPISendMail ignores it
						NULL,         // no conversation ID
						0L,           // no flags, MAPISendMail ignores it
						NULL,         // no originator, this is ignored too
						1,            // one recipient
						&recip,       // NULL recipient array
						0,            // no attachment
						NULL };		  // the attachment structure

	// Store this off..
	char workingDir[256];
	GetCurrentDirectory( sizeof(workingDir), workingDir );
 
	long bRet = fn( 
		NULL,			// implicit session.
		0,				// parent window.
		&note,			// the message.
		0,				// flags
		0				// reserved.
		) == SUCCESS_SUCCESS;

	// MAPISendMail screws with your working directory, so restore it here.
	SetCurrentDirectory( workingDir );

	FreeLibrary( hMAPI );
	return !!bRet;
}

char const* FindArg( char const *pName )
{
	for( int i=1; i < __argc; i++ )
		if( stricmp( pName, __argv[i] ) == 0 )
			return (i+1) < __argc ? __argv[i+1] : "";

	return NULL;
}

int PrintUsage()
{
	printf( "vmapbuilder <cfg filename> <-noget>\n" );
	return 1;
}


void AppPrint( char const *pMsg, ... )
{
	char msg[8192];
	va_list marker;

	va_start( marker, pMsg );
	_vsnprintf( msg, sizeof(msg), pMsg, marker );
	va_end( marker );

	printf( "%s", msg );

	// Write to the output file too...	
	DWORD dwNumBytesWritten = 0;
	WriteFile( g_hOutputFile, msg, strlen(msg)+1, &dwNumBytesWritten, NULL );
}


void BuildTimeDurationString( DWORD elapsed, char timeStr[256] )
{
	DWORD seconds = elapsed / 1000;
	
	DWORD minutes = seconds / 60;
	seconds -= minutes * 60;

	DWORD hours = minutes / 60;
	minutes -= hours * 60;

	_snprintf( timeStr, 256, "(%d hr, %d min, %d sec)", hours, minutes, seconds );
}


DWORD RunProcess( char *pCmdLine )
{
	STARTUPINFO si;
	memset( &si, 0, sizeof(si) );
	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdError = g_hOutputFile;
	si.hStdOutput = g_hOutputFile;
	si.hStdInput = GetStdHandle( STD_INPUT_HANDLE );

	PROCESS_INFORMATION pi;

	if( !CreateProcess( 
		NULL,
		pCmdLine,
		NULL,
		NULL,
		TRUE,		// bInheritHandles
		CREATE_NO_WINDOW | IDLE_PRIORITY_CLASS,
		NULL,
		NULL,
		&si,
		&pi ) )
	{
		return false;
	}

	// Wait for it to complete..
	DWORD dwExitCode = 1;
	WaitForSingleObject( pi.hProcess, INFINITE );
	GetExitCodeProcess( pi.hProcess, &dwExitCode );

	CloseHandle( pi.hThread );
	CloseHandle( pi.hProcess );

	return dwExitCode;
}


int main(int argc, char* argv[])
{
	char cmd[512];
	int ret;

	if( argc < 2 )
	{
		return PrintUsage();
	}


	// All output gets redirected to this file.
	SECURITY_ATTRIBUTES fileAttribs;
	fileAttribs.nLength = sizeof(fileAttribs);
	fileAttribs.lpSecurityDescriptor = NULL;
	fileAttribs.bInheritHandle = TRUE;

	g_hOutputFile = CreateFile( 
		"vmapbuilder.out", 
		GENERIC_WRITE,
		FILE_SHARE_READ,
		&fileAttribs,
		CREATE_ALWAYS,
		FILE_FLAG_WRITE_THROUGH|FILE_ATTRIBUTE_NORMAL,
		NULL );


	// Low priority..
	SetPriorityClass( GetCurrentProcess(), IDLE_PRIORITY_CLASS );

	bool bSave = !FindArg( "-nosave" );
	bool bNoVis = !!FindArg( "-novis" );
	bool bNoRad = !!FindArg( "-norad" );

	char const *pCFGFile = argv[1];
	while( 1 )
	{
		DWORD startTime = GetTickCount();

		CConfigFile file;
		if( file.Read( pCFGFile ) && file.m_Entries.Size() > 0 )
		{
			// Move the first entry in the file to the end.
			CConfigFile::Entry entry = file.m_Entries[0];
			file.m_Entries.Remove( 0 );
			file.m_Entries.AddToTail( entry );
			if( bSave )
			{
				file.Write( pCFGFile );
			}


			char ssCmd[256];
			_snprintf( ssCmd, sizeof(ssCmd), "%s\\win32\\ss.exe", file.m_SSDatabase );

			// Grab the SourceSafe tree.			  
			AppPrint( "\n\n-------------------------------------------------------------\n" );
			AppPrint( "Processing %s\n", entry.m_Filename );
			AppPrint( "-------------------------------------------------------------\n" );
			AppPrint( "Grabbing resources in %s\n", file.m_SSDatabase );
			
			sprintf( cmd, "ssdir=%s", file.m_SSDatabase );
			ret = _putenv( cmd );
			
			// Get the VMF.
			char vmfFilename[512];
			sprintf( vmfFilename, "%s/%s.vmf", entry.m_VMFPath, entry.m_Filename );
			sprintf( cmd, "%s get %s -I-", ssCmd, vmfFilename );
			ret = RunProcess( cmd );

			// Check the timestamp (don't reprocess if it's up-to-date).
			sprintf( vmfFilename, "%s.vmf", entry.m_Filename );
			struct _stat fileStat;
			ret = _stat( vmfFilename, &fileStat );
			long vmfTime = fileStat.st_mtime;
			if( vmfTime == entry.m_VMFTime )
			{
				AppPrint( "%s is up-to-date\n", vmfFilename );
			}
			else
			{
				char localBSPFilename[512];
				sprintf( localBSPFilename, "%s.bsp", entry.m_Filename );
				
				// Attrib the bsp if it exists.
				sprintf( cmd, "attrib -r %s", localBSPFilename );
				RunProcess( cmd );
				
				sprintf( cmd, "%s cp %s", ssCmd, file.m_SSResourcePath );
				ret = RunProcess( cmd );
				
				if( !FindArg( "-noget" ) )
				{
					sprintf( cmd, "%s get * -R -I-", ssCmd );
					ret = RunProcess( cmd );
				}

				// run each tool.
				char vbspCommand[256], vradCommand[256], vvisCommand[256];
				sprintf( vbspCommand, "bin\\vbsp -low %s", entry.m_Filename );
				
				if( entry.m_bFastVis )
					sprintf( vvisCommand, "bin\\vvis -fast -low %s", entry.m_Filename );
				else
					sprintf( vvisCommand, "bin\\vvis -low %s", entry.m_Filename );

				sprintf( vradCommand, "bin\\vrad -low %s", entry.m_Filename );
				if( !RunProcess( vbspCommand ) && 
					(bNoVis || !RunProcess( vvisCommand )) && 
					(bNoRad || !RunProcess( vradCommand )) )
				{
					// Check in the BSP file.
					char bspFilename[512];
					sprintf( bspFilename, "%s/%s.bsp", file.m_SSBSPPath, entry.m_Filename );

					// First, try to add it to SS because it may not exist yet.
					sprintf( cmd, "%s cp %s", ssCmd, file.m_SSBSPPath );
					RunProcess(cmd);
					sprintf( cmd, "%s add %s.bsp -I-", ssCmd, entry.m_Filename );
					ret = RunProcess(cmd);

					// Store off the BSP file temporarily..
					char tempFilename[512];
					sprintf( localBSPFilename, "%s.bsp", entry.m_Filename );
					
					sprintf( tempFilename, "%s.TEMP", entry.m_Filename );
					sprintf( cmd, "del /f %s", tempFilename );
					system( cmd );

					sprintf( cmd, "attrib -r %s", localBSPFilename );
					system( cmd );
					ret = MoveFile( localBSPFilename, tempFilename );
					if( ret )
					{
						char undoCmd[512];
						sprintf( undoCmd, "%s undocheckout %s -I-", ssCmd, bspFilename );

						sprintf( cmd, "%s checkout %s -I-", ssCmd, bspFilename );
						ret = RunProcess( cmd );
						if( !ret )
						{
							// Copy the new BSP file over.
							DeleteFile( localBSPFilename );
							ret = MoveFile( tempFilename, localBSPFilename );
							if( ret )
							{
								sprintf( cmd, "%s checkin %s -I-", ssCmd, bspFilename );
								ret = RunProcess( cmd );
								if( !ret )
								{
									while( !file.Read( pCFGFile ) )
									{
										Sleep( 300 );
									}

									if( bSave )
									{
										CConfigFile::Entry *pEntry = file.FindEntryByFilename( entry.m_Filename );
										if( pEntry )
										{
											pEntry->m_VMFTime = vmfTime;
											while( !file.Write( pCFGFile ) )
											{
												Sleep( 300 );
											}
										}
									}
									
									// Update the timestamp in the config file.
									AppPrint( "Completed '%s' successfully!\n", entry.m_Filename );

									// Send emails.
									char computerName[256] = {0};
									DWORD len = sizeof(computerName);
									GetComputerName( computerName, &len );

									DWORD elapsed = GetTickCount() - startTime;
									char timeStr[256];
									BuildTimeDurationString( elapsed, timeStr );

									char subject[1024];
									_snprintf( subject, sizeof(subject), "[vmapbuilder] completed '%s' on '%s' in %s", entry.m_Filename, computerName, timeStr );
									for( int e=0; e < entry.m_nEMailAddresses; e++ )
									{
										char *pAddr = entry.m_EMailAddresses[e].m_EMailAddress;
										if( !SendMail( pAddr, subject, subject ) )
										{
											AppPrint( "Unable to send confirmation email to %s\n", pAddr );
										}
									}
								}
								else
								{
									AppPrint( "ERROR: Can't checkin %s\n", bspFilename );
									RunProcess( undoCmd );
								}
							}
							else
							{
								AppPrint( "ERROR: Can't copy back the BSP file %s\n", localBSPFilename );
								RunProcess( undoCmd );
							}
						}
						else
						{
							AppPrint( "ERROR: Can't checkout %s\n", bspFilename );
						}
					}
					else
					{
						AppPrint( "ERROR: Can't create temporary file %s\n", tempFilename );
					}
				}
				else
				{
					AppPrint( "Command '%s' failed\n", cmd );
				}
			}
		}
		else
		{
			AppPrint( "Can't read maplist file: %s\n", pCFGFile );
		}

		// Sleep for a bit in case all maps are processed so we don't kill the network.
		Sleep( 2000 );
	}

	CloseHandle( g_hOutputFile );
	return 0;
}

