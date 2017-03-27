//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FILESYSTEM_TOOLS_H
#define FILESYSTEM_TOOLS_H
#ifdef _WIN32
#pragma once
#endif

#include "filesystem.h"


// This class implements a thin wrapper over stdio's file functions. FileHandle_t's are FILEs.
class CBaseStdioFileSystem : public IBaseFileSystem
{
public:
	virtual FileHandle_t	Open( const char *pFileName, const char *pOptions, const char *pathID = 0 );
	virtual void			Close( FileHandle_t file );
	virtual void			Seek( FileHandle_t file, int pos, FileSystemSeek_t seekType );
	virtual unsigned int	Tell( FileHandle_t file );
	virtual unsigned int	Size( FileHandle_t file );
	virtual unsigned int	Size( const char *pFileName, const char *pPathID = 0 );
	virtual void			Flush( FileHandle_t file );
	virtual bool			Precache( const char *pFileName, const char *pPathID );

	virtual long			GetFileTime( const char *pFilename, const char *pPathID = 0 );
	virtual int				Read( void* pOutput, int size, FileHandle_t file );
	virtual int				Write( void const* pInput, int size, FileHandle_t file );
	virtual bool			FileExists( const char *pFileName, const char *pPathID = 0 );
	virtual bool			IsFileWritable( const char *pFileName, const char *pPathID = 0 );
};

// NOTE: If bUseEngineFileSystem is true, it loads filesystem_stdio.dll and sets up search paths.
//       This should only be used by tools that need to use search paths.
// 
// g_pFullFileSystem will be NULL unless bUseEngineFileSystem is true.
bool				FileSystem_Init( bool bUseEngineFileSystem = false );
void				FileSystem_Term();

CreateInterfaceFn	FileSystem_GetFactory( void );

extern IBaseFileSystem	*g_pFileSystem;
extern IFileSystem		*g_pFullFileSystem;	// NOTE: only set if bUseEngineFileSystem is true in FileSystem_Init.


#endif // FILESYSTEM_TOOLS_H
