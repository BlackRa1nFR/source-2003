//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Create an output wave stream.  Used to record audio for in-engine movies 
//
// $NoKeywords: $
//=============================================================================

#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 1)
#include <windows.h>
#include <mmsystem.h>
#pragma warning(pop)

#include "riff.h"
#include "sound_private.h"
#include "snd_wave_temp.h"
#include "filesystem.h"

#include "common.h"

extern IFileSystem *g_pFileSystem;

// Create a wave file
void WaveCreateTmpFile( const char *filename, int rate, int bits, int channels )
{
	char tmpfilename[256];
	COM_StripExtension( filename, tmpfilename, sizeof( tmpfilename ) );
	COM_DefaultExtension( tmpfilename, ".WAV", sizeof( tmpfilename ) );

	FileHandle_t file;
	file = g_pFileSystem->Open( tmpfilename, "wb" );
	int chunkid, chunksize;
	chunkid = MAKEID('R','I','F','F');
	chunksize = 0;
	g_pFileSystem->Write( &chunkid, sizeof(int), file );
	g_pFileSystem->Write( &chunksize, sizeof(int), file );

	chunkid = RIFF_WAVE;
	g_pFileSystem->Write( &chunkid, sizeof(int), file );

		// create a 16-bit stereo output file
	PCMWAVEFORMAT fmt;
	fmt.wf.wFormatTag = WAVE_FORMAT_PCM;
	fmt.wf.nChannels = channels;
	fmt.wf.nSamplesPerSec = rate;
	fmt.wf.nAvgBytesPerSec = rate * bits * channels / 8;
	fmt.wf.nBlockAlign = 4;
	fmt.wBitsPerSample = bits;

	chunkid = WAVE_FMT;
	chunksize = sizeof(fmt);

	g_pFileSystem->Write( &chunkid, sizeof(int), file );
	g_pFileSystem->Write( &chunksize, sizeof(int), file );
	g_pFileSystem->Write( &fmt, chunksize, file );

	chunkid = WAVE_DATA;
	chunksize = 0;
	g_pFileSystem->Write( &chunkid, sizeof(int), file );
	g_pFileSystem->Write( &chunksize, sizeof(int), file );
	g_pFileSystem->Close( file );
}

void WaveAppendTmpFile( const char *filename, void *buffer, int bufferSize )
{
	char tmpfilename[256];
	COM_StripExtension( filename, tmpfilename, sizeof( tmpfilename ) );
	COM_DefaultExtension( tmpfilename, ".WAV", sizeof( tmpfilename ) );

	FileHandle_t file;
	file = g_pFileSystem->Open( tmpfilename, "r+b" );
	g_pFileSystem->Seek( file, 0, FILESYSTEM_SEEK_TAIL );
	g_pFileSystem->Write( buffer, bufferSize, file );
	g_pFileSystem->Close( file );
}

void WaveFixupTmpFile( const char *filename )
{
	char tmpfilename[256];
	COM_StripExtension( filename, tmpfilename, sizeof( tmpfilename ) );
	COM_DefaultExtension( tmpfilename, ".WAV", sizeof( tmpfilename ) );

	FileHandle_t file;
	file = g_pFileSystem->Open( tmpfilename, "r+b" );
	
	// file size goes in RIFF chunk
	int size = g_pFileSystem->Size( file ) - 8;
	// offset to data chunk
	int headerSize = (sizeof(int)*5 + sizeof(PCMWAVEFORMAT));
	// size of data chunk
	int dataSize = size - headerSize;


	g_pFileSystem->Seek( file, 4, FILESYSTEM_SEEK_HEAD );
	g_pFileSystem->Write( &size, 4, file );
	// skip the header and the 4-byte chunk tag and write the size
	g_pFileSystem->Seek( file, headerSize+4, FILESYSTEM_SEEK_HEAD );
	g_pFileSystem->Write( &dataSize, 4, file );
	g_pFileSystem->Close( file );
}

