// WaitAndRestart.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "vstdlib/strtools.h"


int main( int argc, char* argv[] )
{
	if ( argc < 3 )
	{
		printf( "WaitAndRestart <seconds to wait> command line...\n" );
		return 1;
	}

	DWORD timeToWait = (DWORD)atoi( argv[1] );
	printf( "\n\nWaiting for %d seconds to launch ' ", timeToWait );
	for ( int i=2; i < argc; i++ )
	{
		printf( "%s ", argv[i] );
	}
	printf( "'\n\nPress a key to cancel... " );

	
	DWORD startTime = GetTickCount();
	while ( GetTickCount() - startTime < (timeToWait*1000) )
	{
		if ( kbhit() )
			return 2;
		
		Sleep( 100 );
	}

	// Ok, launch it!
	char commandLine[1024] = {0};
	for ( i=2; i < argc; i++ )
	{
		Q_strncat( commandLine, "\"", sizeof( commandLine ) );
		Q_strncat( commandLine, argv[i], sizeof( commandLine ) );
		Q_strncat( commandLine, "\" ", sizeof( commandLine ) );
	}
	
	STARTUPINFO si;
	memset( &si, 0, sizeof( si ) );
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof( pi ) );

	if ( CreateProcess( 
		NULL, 
		commandLine, 
		NULL,						// security
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE,			// flags
		NULL,						// environment
		NULL,						// current directory
		&si,
		&pi ) )
	{
		printf( "Process started.\n" );
		CloseHandle( pi.hThread );	// We don't care what the process does.
		CloseHandle( pi.hProcess );
	}
	else
	{
		printf( "CreateProcess error!\n" );
	}

	return 0;
}

