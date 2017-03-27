//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseFileSystem.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CFileSystem_Stdio : public CBaseFileSystem
{
public:
	CFileSystem_Stdio();
	~CFileSystem_Stdio();

	// Higher level filesystem methods requiring specific behavior
	virtual void GetLocalCopy( const char *pFileName );
	virtual void LogLevelLoadStarted( const char *name );
	virtual void LogLevelLoadFinished( const char *name );
	virtual int	ProgressCounter( void );
	virtual int	HintResourceNeed( const char *hintlist, int forgetEverything );
	virtual int	BlockForResources( const char *hintlist );
	virtual int	PauseResourcePreloading( void );
	virtual int	ResumeResourcePreloading( void );
	virtual void TrackProgress( int enable );
	virtual void RegisterAppProgressCallback( void (*fpProgCallBack)(void), int freq );
	virtual void RegisterAppKeepAliveTicCallback( void (*fpKeepAliveTicCallBack)(char *scr_msg) );
	virtual void UpdateProgress( void );
	virtual int	SetVBuf( FileHandle_t stream, char *buffer, int mode, long size );
	virtual void GetInterfaceVersion( char *p, int maxlen );

protected:
	// implementation of CBaseFileSystem virtual functions
	virtual FILE *FS_fopen( const char *filename, const char *options );
	virtual void FS_fclose( FILE *fp );
	virtual void FS_fseek( FILE *fp, long pos, int seekType );
	virtual long FS_ftell( FILE *fp );
	virtual int FS_feof( FILE *fp );
	virtual size_t FS_fread( void *dest, size_t count, size_t size, FILE *fp );
	virtual size_t FS_fwrite( const void *src, size_t count, size_t size, FILE *fp );
	virtual size_t FS_vfprintf( FILE *fp, const char *fmt, va_list list );
	virtual int FS_ferror( FILE *fp );
	virtual int FS_fflush( FILE *fp );
	virtual char *FS_fgets( char *dest, int destSize, FILE *fp );
	virtual int FS_stat( const char *path, struct _stat *buf );
	virtual HANDLE FS_FindFirstFile(char *findname, WIN32_FIND_DATA *dat);
	virtual bool FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA *dat);
	virtual bool FS_FindClose(HANDLE handle);

	virtual bool IsFileImmediatelyAvailable(const char *pFileName);

private:
	bool m_bMounted;

};

//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------

CFileSystem_Stdio::CFileSystem_Stdio()
{
	m_bMounted = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFileSystem_Stdio::~CFileSystem_Stdio()
{
	Assert(!m_bMounted);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
FILE *CFileSystem_Stdio::FS_fopen( const char *filename, const char *options )
{
	FILE *tst = NULL;
	// stop newline characters at end of filename
	assert(!strchr(filename, '\n') && !strchr(filename, '\r'));
	tst=fopen(filename, options);
#if !defined _WIN32
	if(!tst && !strchr(options,'w') && !strchr(options,'+') ) // try opening the lower cased version
	{
		const char *file =findFileInDirCaseInsensitive(filename);
		tst = fopen( file, options );
	}
#endif
	//if(!tst)
	//	perror(filename);
	return tst;
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
void CFileSystem_Stdio::FS_fclose( FILE *fp )
{
	fclose(fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
void CFileSystem_Stdio::FS_fseek( FILE *fp, long pos, int seekType )
{
	fseek(fp, pos, seekType);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
long CFileSystem_Stdio::FS_ftell( FILE *fp )
{
	return ftell(fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
int CFileSystem_Stdio::FS_feof( FILE *fp )
{
	return feof(fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
size_t CFileSystem_Stdio::FS_fread( void *dest, size_t count, size_t size, FILE *fp )
{
	return fread(dest, count, size, fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
size_t CFileSystem_Stdio::FS_fwrite( const void *src, size_t count, size_t size, FILE *fp )
{
	return fwrite(src, count, size, fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
size_t CFileSystem_Stdio::FS_vfprintf( FILE *fp, const char *fmt, va_list list )
{
	return vfprintf(fp, fmt, list);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
int CFileSystem_Stdio::FS_ferror( FILE *fp )
{
	return ferror(fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
int CFileSystem_Stdio::FS_fflush( FILE *fp )
{
	return fflush(fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
char *CFileSystem_Stdio::FS_fgets( char *dest, int destSize, FILE *fp )
{
	return fgets(dest, destSize, fp);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
int CFileSystem_Stdio::FS_stat( const char *path, struct _stat *buf )
{
	int rt = _stat(path, buf);
#if !defined _WIN32
	if(rt==-1)
	{
		const char *file =findFileInDirCaseInsensitive(path);
		if(file)
			rt=_stat(file,buf);
	}	
#endif
	return rt;
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
HANDLE CFileSystem_Stdio::FS_FindFirstFile(char *findname, WIN32_FIND_DATA *dat)
{
	return ::FindFirstFile(findname, dat);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
bool CFileSystem_Stdio::FS_FindNextFile(HANDLE handle, WIN32_FIND_DATA *dat)
{
	return (::FindNextFile(handle, dat) != 0);
}

//-----------------------------------------------------------------------------
// Purpose: low-level filesystem wrapper
//-----------------------------------------------------------------------------
bool CFileSystem_Stdio::FS_FindClose(HANDLE handle)
{
	return (::FindClose(handle) != 0);
}

//-----------------------------------------------------------------------------
// Purpose: files are always immediately available on disk
//-----------------------------------------------------------------------------
bool CFileSystem_Stdio::IsFileImmediatelyAvailable(const char *pFileName)
{
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFileName - 
//-----------------------------------------------------------------------------
void CFileSystem_Stdio::GetLocalCopy( const char *pFileName )
{
	// do nothing. . everything is local.
}

void CFileSystem_Stdio::LogLevelLoadStarted( const char *name )
{
}

void CFileSystem_Stdio::LogLevelLoadFinished( const char *name )
{
}

int CFileSystem_Stdio::ProgressCounter( void )
{
	return 0;
}

int CFileSystem_Stdio::HintResourceNeed( const char *hintlist, int forgetEverything )
{
	return 0;
}

int CFileSystem_Stdio::BlockForResources( const char *hintlist )
{
	return 0;
}

int CFileSystem_Stdio::PauseResourcePreloading(void)
{
	return 0;
}

int CFileSystem_Stdio::ResumeResourcePreloading(void)
{
	return 0;
}

void CFileSystem_Stdio::TrackProgress( int enable )
{
}

void CFileSystem_Stdio::RegisterAppProgressCallback( void(*fpProgCallBack)(void), int freq )
{
}

void CFileSystem_Stdio::RegisterAppKeepAliveTicCallback(void(*fpKeepAliveTicCallBack)(char *scr_msg) )
{
}

void CFileSystem_Stdio::UpdateProgress( void )
{
}

int CFileSystem_Stdio::SetVBuf( FileHandle_t stream, char *buffer, int mode, long size )
{
	CFileHandle *fh = ( CFileHandle *)stream;
	if ( !fh )
	{
		Warning( (FileWarningLevel_t)-1, "FS:  Tried to SetVBuf NULL file handle!\n" );
		return 0;
	}

	return setvbuf( fh->m_pFile, buffer, mode, size );
}

void CFileSystem_Stdio::GetInterfaceVersion( char *p, int maxlen )
{
	strncpy( p, "Stdio", maxlen );
}



static CFileSystem_Stdio g_FileSystem_Stdio;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CFileSystem_Stdio, IFileSystem, FILESYSTEM_INTERFACE_VERSION, g_FileSystem_Stdio );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CFileSystem_Stdio, IBaseFileSystem, BASEFILESYSTEM_INTERFACE_VERSION, g_FileSystem_Stdio );
