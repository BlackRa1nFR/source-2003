// fwatch.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int main(int argc, char* argv[])
{
	if( argc < 2 )
	{
		printf("fwatch <filename>\n");
		return -1;
	}

	SetPriorityClass( GetCurrentProcess(), IDLE_PRIORITY_CLASS );

	HANDLE hFile = INVALID_HANDLE_VALUE;
	while( hFile == INVALID_HANDLE_VALUE )
	{
		hFile = CreateFile(
			argv[1],
			GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL );

		Sleep( 50 );
	}

	while( 1 )
	{
		char buf[1];
		DWORD dwRead;
		if( ReadFile( hFile, buf, sizeof(buf), &dwRead, NULL ) && dwRead )
		{			
			printf( "%c", buf[0] );
		}
		else
		{
			Sleep(1);
		}
	}

	CloseHandle( hFile );
	return 0;
}

