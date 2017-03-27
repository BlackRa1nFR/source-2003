//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "appframework/IAppSystem.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class CUtlBuffer;
typedef void * FileHandle_t;
typedef int FileFindHandle_t;


//-----------------------------------------------------------------------------
// Enums used by the interface
//-----------------------------------------------------------------------------

enum FileSystemSeek_t
{
	FILESYSTEM_SEEK_HEAD = 0,
	FILESYSTEM_SEEK_CURRENT,
	FILESYSTEM_SEEK_TAIL,
};

enum
{
	FILESYSTEM_INVALID_FIND_HANDLE = -1
};

enum FileWarningLevel_t
{
	// A problem!
	FILESYSTEM_WARNING = -1,

	// Don't print anything
	FILESYSTEM_WARNING_QUIET = 0,

	// On shutdown, report names of files left unclosed
	FILESYSTEM_WARNING_REPORTUNCLOSED,

	// Report number of times a file was opened, closed
	FILESYSTEM_WARNING_REPORTUSAGE,

	// Report all open/close events to console ( !slow! )
	FILESYSTEM_WARNING_REPORTALLACCESSES,

	// Report all open/close/read events to the console ( !slower! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_READ,

	// Report all open/close/read/write events to the console ( !slower! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE,


};

enum SearchPathAdd_t
{
	PATH_ADD_TO_HEAD,	// First path searched
	PATH_ADD_TO_TAIL,	// Last path searched
};


#define FILESYSTEM_INVALID_HANDLE	( FileHandle_t )0

//-----------------------------------------------------------------------------
// Structures used by the interface
//-----------------------------------------------------------------------------

struct FileSystemStatistics
{
	size_t nReads,		
		   nWrites,		
		   nBytesRead,
		   nBytesWritten,
		   nSeeks;
};

//-----------------------------------------------------------------------------
// Main file system interface
//-----------------------------------------------------------------------------

// This is the minimal interface that can be implemented to provide access to
// a named set of files.
#define BASEFILESYSTEM_INTERFACE_VERSION		"VBaseFileSystem003"

class IBaseFileSystem
{
public:
	// if pathID is NULL, all paths will be searched for the file
	virtual FileHandle_t	Open( const char *pFileName, const char *pOptions, const char *pathID = 0 ) = 0;
	virtual void			Close( FileHandle_t file ) = 0;

	virtual int				Read( void* pOutput, int size, FileHandle_t file ) = 0;
	virtual int				Write( void const* pInput, int size, FileHandle_t file ) = 0;

	virtual void			Seek( FileHandle_t file, int pos, FileSystemSeek_t seekType ) = 0;
	virtual unsigned int	Tell( FileHandle_t file ) = 0;
	virtual unsigned int	Size( FileHandle_t file ) = 0;
	virtual unsigned int	Size( const char *pFileName, const char *pPathID = 0 ) = 0;

	virtual long			GetFileTime( const char *pFileName, const char *pPathID = 0 ) = 0;

	virtual void			Flush( FileHandle_t file ) = 0;
	virtual bool			Precache( const char *pFileName, const char *pPathID ) = 0;

	virtual bool			FileExists( const char *pFileName, const char *pPathID = 0 ) = 0;
	virtual bool			IsFileWritable( char const *pFileName, const char *pPathID = 0 ) = 0;
};



#define FILESYSTEM_INTERFACE_VERSION			"VFileSystem009"

class IFileSystem : public IBaseFileSystem, public IAppSystem
{
public:
	// Remove all search paths (including write path?)
	virtual void			RemoveAllSearchPaths( void ) = 0;

	// Add paths in priority order (mod dir, game dir, ....)
	// If one or more .pak files are in the specified directory, then they are
	//  added after the file system path
	// If the path is the relative path to a .bsp file, then any previous .bsp file 
	//  override is cleared and the current .bsp is searched for an embedded PAK file
	//  and this file becomes the highest priority search path ( i.e., it's looked at first
	//   even before the mod's file system path ).
	virtual void			AddSearchPath( const char *pPath, const char *pathID, SearchPathAdd_t addType = PATH_ADD_TO_TAIL ) = 0;
	virtual bool			RemoveSearchPath( const char *pPath, const char *pathID = 0 ) = 0;

	// Deletes a file (on the WritePath)
	virtual void			RemoveFile( char const* pRelativePath, const char *pathID ) = 0;

	// this isn't implementable on STEAM as is.
	virtual void			CreateDirHierarchy( const char *path, const char *pathID ) = 0;

	// File I/O and info
	virtual bool			IsDirectory( const char *pFileName, const char *pathID ) = 0;

	virtual void			FileTimeToString( char* pStrip, int maxCharsIncludingTerminator, long fileTime ) = 0;

	virtual bool			IsOk( FileHandle_t file ) = 0;

	virtual bool			EndOfFile( FileHandle_t file ) = 0;

	virtual char			*ReadLine( char *pOutput, int maxChars, FileHandle_t file ) = 0;
	virtual int				FPrintf( FileHandle_t file, char *pFormat, ... ) = 0;

	// FindFirst/FindNext
	virtual const char		*FindFirst( const char *pWildCard, FileFindHandle_t *pHandle ) = 0;
	virtual const char		*FindNext( FileFindHandle_t handle ) = 0;
	virtual bool			FindIsDirectory( FileFindHandle_t handle ) = 0;
	virtual void			FindClose( FileFindHandle_t handle ) = 0;

	virtual void			GetLocalCopy( const char *pFileName ) = 0;

	virtual CSysModule 		*LoadModule( const char *path ) = 0;

	// return string length of local path EXCLUDING NULL terminator.
	virtual int				GetLocalPathLen( const char *pFileName ) = 0;
	virtual const char		*GetLocalPath( const char *pFileName, char *pLocalPath ) = 0;

	// Returns true on success ( based on current list of search paths, otherwise false if 
	//  it can't be resolved )
	virtual bool			FullPathToRelativePath( const char *pFullpath, char *pRelative ) = 0;

	// Gets the current working directory
	virtual bool			GetCurrentDirectory( char* pDirectory, int maxlen ) = 0;

	// Dump to printf/OutputDebugString the list of files that have not been closed
	virtual void			PrintOpenedFiles( void ) = 0;

	virtual void			SetWarningFunc( void (*pfnWarning)( const char *fmt, ... ) ) = 0;
	virtual void			SetWarningLevel( FileWarningLevel_t level ) = 0;

	// Renames a file (on the WritePath)
	virtual void			RenameFile( char const *pOldPath, char const *pNewPath, const char *pathID ) = 0;

	// Returns the file system statistics retreived by the implementation.  Returns NULL if not supported.
	virtual const FileSystemStatistics *GetFilesystemStatistics() = 0;

	virtual void			LogLevelLoadStarted( const char *name ) = 0;
	virtual void			LogLevelLoadFinished( const char *name ) = 0;
	virtual int				ProgressCounter( void ) = 0;

	// HintResourceNeed() is not to be confused with resource precaching.
	virtual int				HintResourceNeed( const char *hintlist, int forgetEverything ) = 0;
	virtual int				BlockForResources( const char *hintlist ) = 0;

	virtual int				PauseResourcePreloading( void ) = 0;
	virtual int				ResumeResourcePreloading( void ) = 0;
	virtual void			TrackProgress( int enable ) = 0;
	virtual void			RegisterAppProgressCallback( void (*fpProgCallBack)(void), int freq ) = 0;
	virtual void			RegisterAppKeepAliveTicCallback( void (*fpKeepAliveTicCallBack)(char *scr_msg) ) = 0;
	virtual void			UpdateProgress( void ) = 0;
	virtual int				SetVBuf( FileHandle_t stream, char *buffer, int mode, long size ) = 0;
	virtual void			GetInterfaceVersion( char *p, int maxlen ) = 0;
	virtual bool			IsFileImmediatelyAvailable(const char *pFileName) = 0;
	virtual void			PrintSearchPaths( void ) = 0;

	virtual void			UnloadModule( CSysModule *pModule ) = 0;
};


#endif // FILESYSTEM_H
