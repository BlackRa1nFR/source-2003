//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include "bsplib.h"
#include "cmdlib.h"
#include "vstdlib/icommandline.h"

int CopyVariableLump( int lump, void **dest, int size );

void Usage( void )
{
	fprintf( stderr, "usage: \n" );
	fprintf( stderr, "bspzip -extract bspfile blah.zip\n");
	fprintf( stderr, "bspzip -dir bspfile\n");
	fprintf( stderr, "bspzip -addfile bspfile relativepathname fullpathname newbspfile\n");
	fprintf( stderr, "bspzip -addlist bspfile listfile newbspfile\n");
	exit( -1 );
}

int main( int argc, char **argv )
{
	if (argc < 2 )
	{
		Usage();
	}
	
	CommandLine()->CreateCmdLine( argc, argv );
	CmdLib_InitFileSystem( argv[2] );

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
	if( ( stricmp( argv[1], "-extract" ) == 0 ) && argc == 4 )
	{
		char bspName[1024];
		strcpy( bspName, argv[2] );
		DefaultExtension (bspName, ".bsp");

		char zipName[1024];
		strcpy( zipName, argv[3] );
		DefaultExtension (zipName, ".zip");

		ExtractZipFileFromBSP( bspName, zipName );
	}
	else if( ( stricmp( argv[1], "-dir" ) == 0 ) && argc == 3 )
	{
		char bspName[1024];
		strcpy( bspName, argv[2] );
		DefaultExtension (bspName, ".bsp");
		LoadBSPFile (bspName);		
		PrintBSPPackDirectory();
	}
	else if( ( stricmp( argv[1], "-addfile" ) == 0 ) && argc == 6 )
	{
		char bspName[1024];
		strcpy( bspName, argv[2] );
		DefaultExtension (bspName, ".bsp");

		char relativeName[1024];
		strcpy( relativeName, argv[3] );

		char fullpathName[1024];
		strcpy( fullpathName, argv[4] );

		char newbspName[1024];
		strcpy( newbspName, argv[5] );
		DefaultExtension (newbspName, ".bsp");

		// read it in, add pack file, write it back out
		LoadBSPFile (bspName);		
		AddFileToPack( relativeName, fullpathName );
		WriteBSPFile(newbspName);
	}
	else if( ( stricmp( argv[1], "-addlist" ) == 0 ) && argc == 5 )
	{
		char bspName[1024];
		strcpy( bspName, argv[2] );
		DefaultExtension (bspName, ".bsp");

		char filelistName[1024];
		strcpy( filelistName, argv[3] );

		char newbspName[1024];
		strcpy( newbspName, argv[4] );
		DefaultExtension (newbspName, ".bsp");

		// read it in, add pack file, write it back out

		char relativeName[1024];
		char fullpathName[1024];
		FILE *fp = fopen(filelistName, "r");

		if ( fp )
		{
			printf("Opening bsp file: %s\n", bspName);
			LoadBSPFile (bspName);		

			while ( !feof(fp) )
			{
				if ( ( fgets( relativeName, 1024, fp ) != NULL ) &&	
					 ( fgets( fullpathName, 1024, fp ) != NULL ) )
				{
					relativeName[strlen(relativeName) - 1] = '\0';		//strip newline
					fullpathName[strlen(fullpathName) - 1] = '\0';
					printf("Adding file: %s\n", fullpathName);
					AddFileToPack( relativeName, fullpathName );
				}
				else if ( !feof( fp ) )
				{
					printf( "Error: Missing paired relative/full path names\n");
					fclose( fp );
					return( -1 );
				}
			}
			printf("Writing new bsp file: %s\n", newbspName);
			WriteBSPFile(newbspName);
			fclose( fp );
		}
	}
	else
	{
		Usage();
	}
	return 0;
}