//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "voice_wavefile.h"
#include "FileSystem.h"

#include "../../FileSystem_Engine.h"

static unsigned long ReadDWord(FileHandle_t fp) 
{
	unsigned long ret;  
	g_pFileSystem->Read(&ret, 4, fp); 
	return ret;
}

static unsigned short ReadWord(FileHandle_t fp) 
{
	unsigned short ret; 
	g_pFileSystem->Read(&ret, 2, fp);
	return ret;
}

static void WriteDWord(FileHandle_t fp, unsigned long val) 
{
	g_pFileSystem->Write(&val, 4, fp);
}

static void WriteWord(FileHandle_t fp, unsigned short val) 
{
	g_pFileSystem->Write(&val, 2, fp);
}



bool ReadWaveFile(
	const char *pFilename,
	char *&pData,
	int &nDataBytes,
	int &wBitsPerSample,
	int &nChannels,
	int &nSamplesPerSec)
{
	FileHandle_t fp = g_pFileSystem->Open(pFilename, "rb");
	if(!fp)
		return false;

	g_pFileSystem->Seek(fp, 22, FILESYSTEM_SEEK_HEAD);
	
	nChannels = ReadWord(fp);
	nSamplesPerSec = ReadDWord(fp);

	g_pFileSystem->Seek(fp, 34, FILESYSTEM_SEEK_HEAD);
	wBitsPerSample = ReadWord(fp);

	g_pFileSystem->Seek(fp, 40, FILESYSTEM_SEEK_HEAD);
	nDataBytes = ReadDWord(fp);
	ReadDWord(fp);
	pData = new char[nDataBytes];
	if(!pData)
	{
		g_pFileSystem->Close(fp);
		return false;
	}
	g_pFileSystem->Read(pData, nDataBytes, fp);
	return true;
}

bool WriteWaveFile(
	const char *pFilename, 
	const char *pData, 
	int nBytes, 
	int wBitsPerSample, 
	int nChannels, 
	int nSamplesPerSec)
{
	FileHandle_t fp = g_pFileSystem->Open(pFilename, "wb");
	if(!fp)
		return false;

	// Write the RIFF chunk.
	g_pFileSystem->Write("RIFF", 4, fp);
	WriteDWord(fp, 0);
	g_pFileSystem->Write("WAVE", 4, fp);
	

	// Write the FORMAT chunk.
	g_pFileSystem->Write("fmt ", 4, fp);
	
	WriteDWord(fp, 0x10);
	WriteWord(fp, 1);	// WAVE_FORMAT_PCM
	WriteWord(fp, (unsigned short)nChannels);	
	WriteDWord(fp, (unsigned long)nSamplesPerSec);
	WriteDWord(fp, (unsigned long)((wBitsPerSample / 8) * nChannels * nSamplesPerSec));
	WriteWord(fp, (unsigned short)((wBitsPerSample / 8) * nChannels));
	WriteWord(fp, (unsigned long)wBitsPerSample);

	// Write the DATA chunk.
	g_pFileSystem->Write("data", 4, fp);
	WriteDWord(fp, (unsigned long)nBytes);
	g_pFileSystem->Write(pData, nBytes, fp);


	// Go back and write the length of the riff file.
	unsigned long dwVal = g_pFileSystem->Tell(fp) - 8;
	g_pFileSystem->Seek(fp, 4, FILESYSTEM_SEEK_HEAD);
	WriteDWord(fp, dwVal);

	g_pFileSystem->Close(fp);
	return true;
}


