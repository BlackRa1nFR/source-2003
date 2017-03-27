//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Unit test program
//
// $NoKeywords: $
//=============================================================================

#include "unitlib/unitlib.h"
#include "tier0/dbg.h"
#include <stdio.h>
#include <windows.h>

#pragma warning (disable:4100)

SpewRetval_t UnitTestSpew( SpewType_t type, char const *pMsg )
{
	printf( "%s", pMsg );
	OutputDebugString( pMsg );
	return SPEW_CONTINUE;
}


/*
============
main
============
*/
int main (int argc, char **argv)
{
	printf( "Valve Software - unittest.exe (%s)\n", __DATE__ );

	// Install a special Spew handler that ignores all assertions and lets us
	// run for as long as possible
	SpewOutputFunc( UnitTestSpew );

	// Very simple... just iterate over all .DLLs in the current directory and call
	// LoadLibrary on them; check to see if they added any test cases. If
	// they didn't then unload them just as quick.

	// We may want to make this more sophisticated, giving it a search path,
	// or giving test DLLs special extensions, or statically linking the test DLLs
	// to this program.

	WIN32_FIND_DATA findFileData;
	HANDLE hFind= FindFirstFile("*.dll", &findFileData);

	int testCount = 0;
	while (hFind != INVALID_HANDLE_VALUE)
	{
		HMODULE hLib = LoadLibrary(findFileData.cFileName);
		int newTestCount = UnitTestCount();
		if (newTestCount == testCount)
		{
			FreeLibrary( hLib );
		}
		testCount = newTestCount;

		if (!FindNextFile( hFind, &findFileData ))
			break;
	}

	for ( int i = 0; i < testCount; ++i )
	{
		ITestCase* pTestCase = GetUnitTest(i);
		printf("Starting test %s....\n", pTestCase->GetName() );
		pTestCase->RunTest();
	}

	return 0;
}

