//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEFILESYSTEM_H
#define BASEFILESYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#undef GetCurrentDirectory
#elif _LINUX
#include <unistd.h> // unlink
#include "linux_support.h"
#define HANDLE int
#define INVALID_HANDLE_VALUE -1

// undo the prepended "_" 's
#define _stat stat
#define _vsnprintf vsnprintf
#define _alloca alloca
#define _S_IFDIR S_IFDIR

#endif

#include <time.h>


#include "filesystem.h"
#include "utlvector.h"

#include <stdarg.h>

#include "utlrbtree.h"
#include "utlsymbol.h"
#include "bspfile.h"





#ifdef _WIN32
#define CORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR '/'
#elif _LINUX
#define CORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR '\\'
#endif

#ifdef	_WIN32
#define PATHSEPARATOR(c) ((c) == '\\' || (c) == '/')
#elif _LINUX
#define PATHSEPARATOR(c) ((c) == '/')
#endif	//_WIN32



class CBaseFileSystem : public IFileSystem
{
public:
	CBaseFileSystem();
	~CBaseFileSystem();

	// Methods of IAppSystem
	virtual bool			Connect( CreateInterfaceFn factory );
	virtual void			Disconnect();
	virtual void			*QueryInterface( const char *pInterfaceName );
	virtual InitReturnVal_t Init();
	virtual void			Shutdown();

	void					ParsePathID( const char* &pFilename, const char* &pPathID, char tempPathID[MAX_PATH] );

	// file handling
	virtual FileHandle_t		Open( const char *pFileName, const char *pOptions, const char *pathID );
	virtual void				Close( FileHandle_t );
	virtual void				Seek( FileHandle_t file, int pos, FileSystemSeek_t method );
	virtual unsigned int		Tell( FileHandle_t file );
	virtual unsigned int		Size( FileHandle_t file );
	virtual unsigned int		Size( const char *pFileName, const char *pPathID );

	virtual bool				IsOk( FileHandle_t file );
	virtual void				Flush( FileHandle_t file );
	virtual bool				Precache( const char *pFileName, const char *pPathID);
	virtual bool				EndOfFile( FileHandle_t file );
 
	virtual int					Read( void *pOutput, int size, FileHandle_t file );
	virtual int					Write( void const* pInput, int size, FileHandle_t file );
	virtual char				*ReadLine( char *pOutput, int maxChars, FileHandle_t file );
	virtual int					FPrintf( FileHandle_t file, char *pFormat, ... );

	// Gets the current working directory
	virtual bool				GetCurrentDirectory( char* pDirectory, int maxlen );

	// this isn't implementable on STEAM as is.
	virtual void				CreateDirHierarchy( const char *path, const char *pathID );

	// returns true if the file is a directory
	virtual bool				IsDirectory( const char *pFileName, const char *pathID );

	// path info
	virtual const char			*GetLocalPath( const char *pFileName, char *pLocalPath );
	virtual bool				FullPathToRelativePath( const char *pFullpath, char *pRelative );
	virtual int					GetLocalPathLen( const char *pFileName );

	// removes a file from disk
	virtual void				RemoveFile( char const* pRelativePath, const char *pathID );

	// Remove all search paths (including write path?)
	virtual void				RemoveAllSearchPaths( void );
	// STUFF FROM IFileSystem
	// Add paths in priority order (mod dir, game dir, ....)
	// Can also add pak files (errr, NOT YET!)
	virtual void				AddSearchPath( const char *pPath, const char *pathID, SearchPathAdd_t addType );
	virtual bool				RemoveSearchPath( const char *pPath, const char *pathID );

	virtual bool				FileExists( const char *pFileName, const char *pPathID = NULL );
	virtual long				GetFileTime( const char *pFileName, const char *pPathID = NULL );
	virtual bool				IsFileWritable( char const *pFileName, const char *pPathID = NULL );
	virtual void				FileTimeToString( char *pString, int maxChars, long fileTime );
	
	virtual const char			*FindFirst( const char *pWildCard, FileFindHandle_t *pHandle );
	virtual const char			*FindNext( FileFindHandle_t handle );
	virtual bool				FindIsDirectory( FileFindHandle_t handle );
	virtual void				FindClose( FileFindHandle_t handle );

	virtual void				PrintOpenedFiles( void );
	virtual void				SetWarningFunc( void (*pfnWarning)( const char *fmt, ... ) );
	virtual void				SetWarningLevel( FileWarningLevel_t level );
	virtual void				RenameFile( char const *pOldPath, char const *pNewPath, const char *pathID );

	virtual void				GetLocalCopy( const char *pFileName );

	// Returns the file system statistics retreived by the implementation.  Returns NULL if not supported.
	virtual const FileSystemStatistics *GetFilesystemStatistics();
	
	// Load dlls
	virtual CSysModule 			*LoadModule( const char *path );
	virtual void				UnloadModule( CSysModule *pModule );

protected:
	// IMPLEMENTATION DETAILS FOR CBaseFileSystem 
	struct FindData_t
	{
		WIN32_FIND_DATA findData;
		int currentSearchPathID;
		CUtlVector<char> wildCardString;
		HANDLE findHandle;
	};
	
	class CFileHandle
	{
	public:
		CFileHandle( void )
		{
			m_pFile = NULL;
			m_nStartOffset = 0;
			m_nLength = 0;
			m_nFileTime = 0;
			m_bPack = false;
		}

		FILE			*m_pFile;
		bool			m_bPack;
		int				m_nStartOffset;
		int				m_nLength;
		long			m_nFileTime;
	};

	enum
	{
		MAX_FILES_IN_PACK = 32768,
	};

	class CPackFileEntry
	{
	public:
		CUtlSymbol			m_Name;
		int					m_nPosition;
		int					m_nLength;
	};

	class CSearchPath 
	{

	public:
							CSearchPath( void );
							~CSearchPath( void );

		static bool PackFileLessFunc( CPackFileEntry const& src1, CPackFileEntry const& src2 );
		const char* GetPathString() const;
		const char* GetPathIDString() const;

		CUtlSymbol			m_Path;
		CUtlSymbol			m_PathID;

		bool				m_bIsMapPath;
		bool				m_bIsPackFile;
		long				m_lPackFileTime;
		CFileHandle			*m_hPackFile;
		int					m_nNumPackFiles;

		CUtlRBTree< CPackFileEntry, int > m_PackFiles;

		static CBaseFileSystem*	m_fs;
	};
	
	friend class CSearchPath;

	// Global list of pack file handles
	CUtlVector< FILE * > m_PackFileHandles;
	CUtlVector<FindData_t> m_FindData;
	CUtlVector< CSearchPath > m_SearchPaths;
	FILE *m_pLogFile;
	bool m_bOutputDebugString;

	// Statistics:
	FileSystemStatistics m_Stats;

protected:
	//----------------------------------------------------------------------------
	// Purpose: Functions implementing basic file system behavior.
	//----------------------------------------------------------------------------
	virtual FILE *FS_fopen( const char *filename, const char *options ) = 0;
	virtual void FS_fclose( FILE *fp ) = 0;
	virtual void FS_fseek( FILE *fp, long pos, int seekType ) = 0;
	virtual long FS_ftell( FILE *fp ) = 0;
	virtual int FS_feof( FILE *fp ) = 0;
	virtual size_t FS_fread( void *dest, size_t count, size_t size, FILE *fp ) = 0;
	virtual size_t FS_fwrite( const void *src, size_t count, size_t size, FILE *fp ) = 0;
	virtual size_t FS_vfprintf( FILE *fp, const char *fmt, va_list list ) = 0;
	virtual int FS_ferror( FILE *fp ) = 0;
	virtual int FS_fflush( FILE *fp ) = 0;
	virtual char *FS_fgets( char *dest, int destSize, FILE *fp ) = 0;
	virtual int FS_stat( const char *path, struct _stat *buf ) = 0;
	virtual HANDLE FS_FindFirstFile(char *findname, WIN32_FIND_DATA *dat) = 0;
	virtual bool FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA *dat) = 0;
	virtual bool FS_FindClose(HANDLE handle) = 0;

protected:
	//-----------------------------------------------------------------------------
	// Purpose: For tracking unclosed files
	// NOTE:  The symbol table could take up memory that we don't want to eat here.
	// In that case, we shouldn't store them in a table, or we should store them as locally allocates stings
	//  so we can control the size
	//-----------------------------------------------------------------------------
	class COpenedFile
	{
	public:
					COpenedFile( void );
					~COpenedFile( void );

					COpenedFile( const COpenedFile& src );

		bool operator==( const COpenedFile& src ) const;

		void		SetName( char const *name );
		char const	*GetName( void );

		FILE		*m_pFile;
		char		*m_pName;
	};

	//CUtlRBTree< COpenedFile, int > m_OpenedFiles;
	CUtlVector <COpenedFile> m_OpenedFiles;

	static bool OpenedFileLessFunc( COpenedFile const& src1, COpenedFile const& src2 );

	FileWarningLevel_t			m_fwLevel;
	void						(*m_pfnWarning)( const char *fmt, ... );

	FILE						*Trace_FOpen( const char *filename, const char *options );
	void						Trace_FClose( FILE *fp );
	void						Trace_FRead( int size, FILE* file );
	void						Trace_FWrite( int size, FILE* file );

	void						Trace_DumpUnclosedFiles( void );

	void						Warning( FileWarningLevel_t level, const char *fmt, ... );

	bool						FileInSearchPaths( const char *pSearchWildcard, const char *pFileName, 
								int minSearchPathID, int maxSearchPathID );
	bool						FindNextFileHelper( FindData_t *pFindData );

	void						RemoveAllMapSearchPaths( void );
	void						AddMapPackFile( const char *pPath, SearchPathAdd_t addType );
	void						AddPackFiles( const char *pPath, SearchPathAdd_t addType );
	bool						PreparePackFile( CSearchPath& packfile, int offsetofpackinmetafile, int filelen );
	void						PrintSearchPaths( void );

	FileHandle_t				FindFile( const CSearchPath *path, const char *pFileName, const char *pOptions );
	int							FastFindFile( const CSearchPath *path, const char *pFileName );

	const char					*GetWritePath(const char *pathID);

	// Computes a full write path
	void						ComputeFullWritePath( char* pDest, const char *pWritePathID, char const *pRelativePath );

	// Opens a file for read or write
	FileHandle_t OpenForRead( const char *pFileName, const char *pOptions, const char *pathID );
	FileHandle_t OpenForWrite( const char *pFileName, const char *pOptions, const char *pathID );
	CSearchPath *FindWritePath( const char *pathID );

};

// singleton accessor
CBaseFileSystem *BaseFileSystem();

#endif // BASEFILESYSTEM_H
