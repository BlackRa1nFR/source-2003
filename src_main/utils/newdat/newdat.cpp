//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Makes .DAT files
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "basetypes.h"
#include "checksum_md5.h"

void Sys_Error( char *fmt, ... );
extern void Con_Printf( char *fmt, ... );
int COM_FOpenFile( const char *filename, FILE **file );

// So we can link CRC
int LittleLongFn( int l );
int (*LittleLong)(int l) = LittleLongFn;


void StripExtension( const char *in, char *out )
{
	// BUGBUG -- The one in common doesn't work with relative paths, so do this instead.
	const char *tail = in + strlen( in ) - 1;

	while ( tail > in && *tail != '.' )
		tail--;

	int len = tail - in;
	
	while (len)
	{
		*out++ = *in++;
		len--;
	}
	*out = 0;
}


// So we can link CRC
void Sys_Error( char *fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	vprintf( fmt, args );
	exit(0);
}

// So we can link CRC
int COM_FindFile( char *filename, FILE **file )
{
	if ( file )
	{
		*file = fopen( filename, "rb" );
		if ( *file )
			return 1;
	}
	return 0;
}

// So we can link CRC
int LittleLongFn( int l )
{
	return l;
}

// So we can link CRC
void Con_Printf( char *fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	vprintf( fmt, args );
}


// So we can link CRC
int COM_FOpenFile( const char *filename, FILE **file )
{
	return COM_FindFile( (char *)filename, file );
}

bool MD5_Hash_File(unsigned char digest[16], char *pszFileName, bool bUsefopen /* = FALSE */, bool bSeed /* = FALSE */, unsigned int seed[4] /* = NULL */)
{
	FILE *fp;
	byte chunk[1024];
	int nBytesRead;
	MD5Context_t ctx;
	
	int nSize;

	if (!bUsefopen)
	{
		nSize = COM_FindFile(pszFileName, &fp);
		if ( !fp || ( nSize == -1 ) )
			return false;
	}
	else
	{
		fp = fopen( pszFileName, "rb" );
		if ( !fp )
			return false;

		fseek ( fp, 0, SEEK_END );
		nSize = ftell ( fp );
		fseek ( fp, 0, SEEK_SET );

		if ( nSize <= 0 )
		{
			fclose ( fp );
			return false;
		}
	}

	memset(&ctx, 0, sizeof(MD5Context_t));

	MD5Init(&ctx);

	if (bSeed)
	{
		// Seed the hash with the seed value
		MD5Update( &ctx, (const unsigned char *)&seed[0], 16 );
	}

	// Now read in 1K chunks
	while (nSize > 0)
	{
		if (nSize > 1024)
			nBytesRead = fread(chunk, 1, 1024, fp);
		else
			nBytesRead = fread(chunk, 1, nSize, fp);

		// If any data was received, CRC it.
		if (nBytesRead > 0)
		{
			nSize -= nBytesRead;
			MD5Update(&ctx, chunk, nBytesRead);
		}

		// We we are end of file, break loop and return
		if ( feof( fp ) )
		{
			fclose( fp );
			fp = NULL;
			break;
		}
		// If there was a disk error, indicate failure.
		else if ( ferror(fp) )
		{
			if ( fp )
				fclose(fp);
			return FALSE;
		}
	}	

	if ( fp )
		fclose(fp);

	MD5Final(digest, &ctx);

	return TRUE;
}

//-----------------------------------------------------------------------------
// Purpose: newdat.exe - makes the .DAT signature for file / virus checking
// Input  : argc - std args
//			*argv[] - 
// Output : int 0 == success. 1 == failure
//-----------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	char out[512], datFile[512];
	unsigned char digest[16];

	if ( argc < 2 )
	{
		printf("USAGE: newdat <filename>\n" );
		return 1;
	}


	// Get the filename without the extension
	StripExtension( argv[1], out );
	sprintf( datFile, "%s.dat", out );

	// Build the MD5 hash for the .EXE file
	MD5_Hash_File( digest, argv[1], TRUE, FALSE, NULL );

	// Write the first 4 bytes of the MD5 hash as the signature ".dat" file
	FILE *fp = fopen( datFile, "wb" );
	if ( fp )
	{
		fwrite( digest, sizeof(int), 1, fp );
		fclose( fp );
		printf("Wrote %s\n", datFile );
		return 0;
	}
	else
		printf("Can't open %s\n", datFile );

	return 1;
}
