
#pragma warning(disable:4115)
#pragma warning(disable:4100)

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <io.h>
 
#include "Steam.h"
#include "SteamLib.h"

#include <sys/stat.h>
#include <assert.h>

static TSteamError g_tLastError;
static TSteamError g_tLastErrorNoFile;
static FILE *g_pLastErrorFile = NULL;

int  STEAMprogressCounter;
int  STEAMtrackingProgress;
void (*AppProgressCallback)(void);
int  AppProgressCallbackFreq;


void   (*AppKeepAliveTicCallback)(char* scr_msg);



#define FORCE_KEEPALIVE_TIC_READ_SIZE 131072



// TODO: remove the following function, it's NOT Steam generic.
void FixSlashes( char *pszName )
{
	while ( *pszName ) {
		if ( *pszName == '/' )
			*pszName = '\\';
		pszName++;
	}
}


/*
**-------------------------------------------------------------------------
** DoEventTic()
**
**		If the app has told us to provide keep-alive type tics, do so here.
**
**-------------------------------------------------------------------------
*/
static void DoEventTic(BOOL bForce)
{
#define EVENTS_TO_TIC	25

	static long numEvents = 0;
	static char *msg = NULL;

	static BOOL bDoingTicEvent = FALSE;

	// Have we seen enough events to update the app with tics?
	// Also need to prevent recursion here as the game may read, etc while 
	// servicing the tic...
	numEvents++;
	if ( AppKeepAliveTicCallback && !bDoingTicEvent && (bForce || (numEvents % EVENTS_TO_TIC == 0)) )
	{
		// Prevent recursion...
		bDoingTicEvent = TRUE;

		// Send it back to the app.
		AppKeepAliveTicCallback(msg);

		bDoingTicEvent = FALSE;
	}
}

void CheckError(FILE *fp, TSteamError *steamError)
{
	if (fp)
	{
		if (steamError->eSteamError != eSteamErrorNone || g_tLastError.eSteamError != eSteamErrorNone)
		{
			g_pLastErrorFile = fp;
			g_tLastError = *steamError;
		}
	}
	else
	{
		// write to the NULL error checker
		if (steamError->eSteamError != eSteamErrorNone || g_tLastErrorNoFile.eSteamError != eSteamErrorNone)
		{
			g_tLastErrorNoFile = *steamError;
		}
	}
}

/*****************************************************************/
/*****************************************************************/
// CLIB wrappers for system functions such as fopen, fread, etc.
/*****************************************************************/
/*****************************************************************/

/**************/
//STEAM_Startup
//Initializes the Steam VFS
/**************/
int STEAM_Startup(void)
{
	TSteamError steamError;
	int result;
	SteamClearError(&steamError);
	result = SteamStartup( (STEAM_USING_FILESYSTEM | STEAM_USING_LOGGING ), &steamError );
//	result = SteamStartup( (STEAM_USING_FILESYSTEM | STEAM_USING_LOGGING | STEAM_USING_USERID | STEAM_USING_ACCOUNT), &steamError );
	CheckError(NULL, &steamError);
	return result;
}

/**************/
//STEAM_Mount
//Mounts the Steam VFS for a specific mount point
/**************/
int STEAM_Mount(const char *szMountPath)
{
	TSteamError steamError;
	int result;
	steamError.eSteamError = eSteamErrorNone;
	SteamClearError(&steamError);
//	result = SteamMountAppFilesystem( szMountPath , &steamError );
	result = SteamMountAppFilesystem( &steamError );
	CheckError(NULL, &steamError);
	return result;
}

/**************/
//STEAM_Unmount
//Unmounts the Steam VFS
/**************/
int STEAM_Unmount(void)
{
	TSteamError steamError;
	int result;
	steamError.eSteamError = eSteamErrorNone;
	SteamClearError(&steamError);
//	result = SteamUnmountFilesystem(&steamError);
	result = SteamUnmountAppFilesystem( &steamError );
	CheckError(NULL, &steamError);
	return result;
}

/**************/
//STEAM_Shutdown
//Terminates the Steam VFS
/**************/
void STEAM_Shutdown(void)
{
	TSteamError steamError;
	steamError.eSteamError = eSteamErrorNone;
	SteamClearError(&steamError);
	SteamCleanup(&steamError);
	CheckError(NULL, &steamError);
}

/**************/
//STEAM_fopen
/**************/
FILE *STEAM_fopen(const char *filename, const char *options)
{
	TSteamError steamError;
	char wadName[_MAX_PATH];
	SteamHandle_t hndl;

	strcpy(wadName,filename);			
	FixSlashes(wadName);

#if 0
	strlwr(wadName);
	if (   strstr(wadName, "client.dll")
	   )
	   hndl = STEAM_INVALID_HANDLE; // convenient breakpoint spot...
#endif

	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	hndl = SteamOpenFile(wadName, options, &steamError);
	CheckError((FILE *)hndl, &steamError);

	return (FILE *)hndl;
}

/**************/
//STEAM_tmpfile
/**************/
FILE *STEAM_tmpfile(void)
{
	SteamHandle_t hndl;
	TSteamError steamError;
	hndl = SteamOpenTmpFile(&steamError);
	CheckError((FILE *)hndl, &steamError);
	return (FILE *)hndl;
}

/**************/
//STEAM_fread
/**************/
unsigned int STEAM_fread(void *buffer, unsigned int rdsize, unsigned int count,FILE *file)
{
	SteamHandle_t hndl = (SteamHandle_t)file;
	char* p = buffer;
	unsigned int totalSizeRead = 0;
	unsigned int bytesRead = 0;
	unsigned int amtLeftToRead = rdsize * count;
	unsigned int readBlockSize = 524288;
	TSteamError steamError;

#if 0
	if ( rdsize == 12 || count == 12 )
	   hndl = STEAM_INVALID_HANDLE; // convenient breakpoint spot...
#endif

	while ( amtLeftToRead )
	{
		unsigned int numBytesToRead;

		// Show progress...
		if ( STEAMtrackingProgress && (amtLeftToRead > FORCE_KEEPALIVE_TIC_READ_SIZE) )
		{
			DoEventTic(TRUE);
		}

		// Read the next block
		numBytesToRead = ( amtLeftToRead > readBlockSize ? readBlockSize : amtLeftToRead );
	 	bytesRead = SteamReadFile(p, 1, numBytesToRead, hndl, &steamError);

		// If we didn't read what we thought we should, we're done...
		if ( bytesRead == 0 || (bytesRead != numBytesToRead && steamError.eSteamError != eSteamErrorEOF) )
			break;

		// If we had an error (not EOF) we're done...
		if ( steamError.eSteamError != eSteamErrorNone && steamError.eSteamError != eSteamErrorEOF )
			break;

		// Update for next loop...	
		amtLeftToRead -= bytesRead; // what's left to read...
		p += bytesRead;				// where to put the next block of data...
		totalSizeRead += bytesRead; // how much we've read so far.
	}

	CheckError((FILE *)hndl, &steamError);
	return totalSizeRead / rdsize;

}

/**************/
//STEAM_fgetc
/**************/
int STEAM_fgetc(FILE *file)
{
	unsigned char c;

	if ( STEAM_fread(&c, 1, 1, file) != 1 )
		c = (unsigned char)EOF;

	return (int)c;
}

/**************/
//STEAM_fgets
/**************/
char *STEAM_fgets(char *string, int numCharToRead, FILE *stream)
{
	unsigned char c;
	int numCharRead = 0;
	
	// Read at most n chars from the file or until a newline
	*string = c = '\0';
	while ( (numCharRead < numCharToRead-1) && (c != '\n') )
	{
		// Read in the next char...
		if ( STEAM_fread(&c, 1, 1, stream) != 1 )
		{
			if ( g_tLastError.eSteamError != eSteamErrorEOF || numCharRead == 0 )
			{
				return NULL;	// If we hit an error, return NULL.
			}
			
			numCharRead = numCharToRead;	// Hit EOF, no more to read, all done...
		}

		else
		{
			*string++ = c;		// add the char to the string and point to the next pos
			*string = '\0';		// append NULL
			numCharRead++;		// count the char read
		}
	}
	return string; // Has a NULL termination...
}

/**************/
//STEAM_fputc
/**************/
int STEAM_fputc(int c, FILE *stream)
{
	unsigned char chr = (unsigned char)c;
	SteamHandle_t hndl = (SteamHandle_t)stream;
	int n;
	TSteamError steamError;

	n = SteamWriteFile(&chr, sizeof(chr), 1, hndl, &steamError);
	if ( n != 1 || steamError.eSteamError != eSteamErrorNone )
	{
		CheckError((FILE *)hndl, &steamError);
		return EOF;
	}
	return c;
}

/**************/
//STEAM_fputs
/**************/
int STEAM_fputs(const char *string, FILE *stream)
{
	SteamHandle_t hndl = (SteamHandle_t)stream;
	int len = strlen(string);
	int n;
	TSteamError steamError;

	n = SteamWriteFile((void*)string, sizeof(char), len, hndl, &steamError);
	if ( n != len || steamError.eSteamError != eSteamErrorNone )
	{
		CheckError((FILE *)hndl, &steamError);
		return EOF;
	}
	return 1;
}

/**************/
//STEAM_fwrite
/**************/
unsigned int STEAM_fwrite(void *buffer, unsigned int wrsize, unsigned int count,FILE *file)
{
	SteamHandle_t hndl = (SteamHandle_t)file;
	TSteamError steamError;
	int result;
	result = SteamWriteFile(buffer, wrsize, count, hndl, &steamError);
	CheckError((FILE *)hndl, &steamError);
	return result;
}

/**************/
//STEAM_fprintf
/**************/
int STEAM_fprintf( FILE *fp, const char *format, ... )
{
	int retval;
	va_list argptr;

	// Make sure world is sane...
	if ( !fp || !format )
		return -1;

	// Init variable argument list.
	va_start(argptr, format);

	// Pass it all on...
	retval = STEAM_vfprintf(fp, format, argptr);

	// Clean up...
	va_end(argptr);

	return retval;
}

				
/**************/
//STEAM_vfprintf
//
//  Yes...this sucks...but would rather malloc than try to guess what the
//  largest formatted string will be and reserve that on the stack each 
//  time...so we're first vfprintf'ing to the NULL device to determine
//  the length of the formatted string, and then mallocing a buffer to 
//  hold it for vsprintf.
//
/**************/
int STEAM_vfprintf( FILE *fp, const char *format, va_list argptr )
{
	int blen, plen;
	char *buf;
	FILE *nullDeviceFP;

	// Make sure world is sane...
	if ( !fp || !format )
		return -1;

	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	// Open the null device...used by vfprintf to determine the length of
	// the formatted string.
	nullDeviceFP = fopen("nul:", "w");
	if ( !nullDeviceFP )
		return -1;

	// Figure out how long the formatted string will be...dump formatted
	// string to the bit bucket.
	blen = vfprintf(nullDeviceFP, format, argptr);
	fclose(nullDeviceFP);
	if ( !blen )
	{
		return -1;
	}

	// Get buffer in which to build the formatted string.
	buf = (char *)malloc(blen+1);
	if ( !buf )
	{
		return -1;
	}

	// Build the formatted string.
	plen = vsprintf(buf, format, argptr);
	va_end(argptr);
	if ( plen != blen )
	{
		free(buf);
		return -1;
	}

	// Write out the formatted string.
	if ( plen != (int)STEAM_fwrite(buf, 1, plen, fp) )
	{
		free(buf);
		return -1;
	}

	free(buf);
	return plen;
}

				
/**************/
//STEAM_fclose
/**************/
int STEAM_fclose(FILE *file)
{
	SteamHandle_t hndl = (SteamHandle_t)file;
	TSteamError steamError;
	int result;
	result = SteamCloseFile(hndl, &steamError);
	CheckError((FILE *)hndl, &steamError);
	return result;
}

/**************/
//STEAM_fseek
/**************/
int STEAM_fseek(FILE *file, long offset, int method)
{
	SteamHandle_t hndl = (SteamHandle_t)file;
	ESteamSeekMethod seekMethod = (ESteamSeekMethod)method;
	TSteamError steamError;
	int result;

	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	result = SteamSeekFile(hndl, offset, seekMethod, &steamError);
	CheckError((FILE *)hndl, &steamError);
	return result;
}

/**************/
//STEAM_ftell
/**************/
long STEAM_ftell(FILE *file)
{
	SteamHandle_t hndl = (SteamHandle_t)file;
	long steam_offset;
	TSteamError steamError;

	// Determine the real file offset (as opposed to the logical offset represented by
	// the fread buffer...)
	steam_offset = SteamTellFile(hndl, &steamError);
	if ( steamError.eSteamError != eSteamErrorNone )
	{
		CheckError((FILE *)hndl, &steamError);
		return -1L;
	}

	return steam_offset;
}

/**************/
//STEAM_feof
/**************/
int STEAM_feof( FILE *stream )
{
//	SteamHandle_t hndl = (SteamHandle_t)stream;
	long orig_pos;
	
	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	// Figure out where in the file we currently are...
	orig_pos = STEAM_ftell(stream);
	
	if ( stream == g_pLastErrorFile && g_tLastError.eSteamError == eSteamErrorEOF )
		return 1;

	if ( g_tLastError.eSteamError != eSteamErrorNone )
		return 0;

	// Jump to the end...
	if ( STEAM_fseek(stream, 0L, SEEK_END) != 0 )
		return 0;

	// If we were already at the end, return true
	if ( orig_pos == STEAM_ftell(stream) )
		return 1;

	// Otherwise, go back to the original spot and return false.
	STEAM_fseek(stream, orig_pos, SEEK_SET);
	return 0; 
}

/**************/
//STEAM_ferror
/**************/
int STEAM_ferror( FILE *stream )
{
	if (stream)
	{
		if (stream != g_pLastErrorFile)
		{
			// it's asking for an error for a previous file, return no error
			return 0;
		}

		return ( g_tLastError.eSteamError != eSteamErrorNone );
	}

	return g_tLastErrorNoFile.eSteamError != eSteamErrorNone;
}

/**************/
//STEAM_clearerr
/**************/
void STEAM_clearerr( FILE *stream )
{
	SteamClearError(&g_tLastError);
	return;
}

/**************/
//STEAM_ferror
/**************/
void STEAM_strerror( FILE *stream, char *p, int maxlen )
{
	if ( p && maxlen > 0 )
	{
		if ( stream == NULL )
			strncpy(p, g_tLastErrorNoFile.szDesc, maxlen);
		else
			strncpy(p, g_tLastError.szDesc, maxlen);
	}
	return;
}

/**************/
//STEAM_stat
/**************/
int STEAM_stat(const char *path, struct _stat *buf)
{
	TSteamElemInfo Info;
	int returnVal;
	struct _stat tmpBuf;
	TSteamError steamError;

	char tmpPath[_MAX_PATH];

	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	strcpy(tmpPath, path);			
	FixSlashes(tmpPath);

	memset(buf, 0, sizeof(struct _stat));

	returnVal= SteamStat(tmpPath, &Info, &steamError);

	if ( returnVal == 0 )
	{
		if (Info.bIsDir )
		{
			buf->st_mode |= _S_IFDIR;
			buf->st_size = 0;
		}
		else
		{
			buf->st_mode |= _S_IFREG;
			buf->st_size = Info.uSizeOrCount;
		}

		buf->st_atime = Info.lLastAccessTime;
		buf->st_mtime = Info.lLastModificationTime;
		buf->st_ctime = Info.lCreationTime;


		_stat(path, &tmpBuf);
	}

	CheckError(NULL, &steamError);
	return returnVal;
}

/**************/
//STEAM_rewind
/**************/
void STEAM_rewind( FILE *stream )
{
	STEAM_fseek(stream, 0L, SEEK_SET);
	STEAM_clearerr(stream);
	return;
}

/**************/
//STEAM_fflush
/**************/
int STEAM_fflush( FILE *stream )
{
	TSteamError steamError;
	SteamHandle_t hndl = (SteamHandle_t)stream;
	int result = SteamFlushFile(hndl, &steamError);
	CheckError((FILE *)hndl, &steamError);
	return result;
}

/**************/
//STEAM_flushall
/**************/
int STEAM_flushall( void )
{
	return 0; // hum...what to do here...we're not dealing with _fcloseall()...
}

/**************/
//STEAM_setbuf
/**************/
void STEAM_setbuf( FILE *stream, char *buffer)
{
	TSteamError steamError;
	SteamHandle_t hndl = (SteamHandle_t)stream;

	if ( buffer == NULL )
		SteamSetvBuf(hndl, NULL, eSteamBufferMethodNBF, 0, &steamError);
	else
		SteamSetvBuf(hndl, buffer, eSteamBufferMethodFBF, BUFSIZ, &steamError);
	CheckError((FILE *)hndl, &steamError);
	return;
}

/**************/
//STEAM_setvbuf
/**************/
int STEAM_setvbuf( FILE *stream, char *buffer, int mode, size_t size)
{
	int retval = -1;
	SteamHandle_t hndl = (SteamHandle_t)stream;
	ESteamBufferMethod steamMode = eSteamBufferMethodFBF;
	TSteamError steamError;

	// Convert mode to Steam version...already set for _IOFBF.
	if ( mode == _IONBF )
		steamMode = eSteamBufferMethodNBF;

	// We don't support _IOLBF.
	if ( mode != _IOLBF )
	{
		retval = SteamSetvBuf(hndl, (void*)buffer, steamMode, size, &steamError);
		CheckError((FILE *)hndl, &steamError);
	}

	return retval;
}

/**************/
//STEAM_FindFirstFile
/**************/
HANDLE STEAM_FindFirstFile(char *pszMatchName, STEAMFindFilter filter, WIN32_FIND_DATA *findInfo)
{
	TSteamElemInfo steamFindInfo;
	HANDLE hResult = INVALID_HANDLE_VALUE;
	SteamHandle_t steamResult;
	char tmpMatchName[_MAX_PATH];
	TSteamError steamError;

	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	strcpy(tmpMatchName, pszMatchName);			
	FixSlashes(tmpMatchName);

	steamResult = SteamFindFirst(tmpMatchName, (ESteamFindFilter)filter, &steamFindInfo, &steamError);
	CheckError(NULL, &steamError);

	if ( steamResult == STEAM_INVALID_HANDLE )
	{
		hResult = INVALID_HANDLE_VALUE;
	}

	else
	{
		hResult = (HANDLE)steamResult;
		strcpy(findInfo->cFileName, steamFindInfo.cszName);
		
// NEED TO DEAL WITH THIS STUFF!!!  FORTUNATELY HALF-LIFE DOESN'T USE ANY OF IT
// AND ARCANUM USES _findfirst() etc.
//
//		findInfo->ftLastWriteTime = steamFindInfo.lLastModificationTime;
//		findInfo->ftCreationTime = steamFindInfo.lCreationTime;
//		findInfo->ftLastAccessTime = steamFindInfo.lLastAccessTime;
//		findInfo->nFileSizeHigh = ;
//		findInfo->nFileSizeLow = ;

		// Determine if the found object is a directory...
		if ( steamFindInfo.bIsDir )
			findInfo->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
		else
			findInfo->dwFileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;

		// Determine if the found object was local or remote.
		// ***NOTE*** we are hijacking the FILE_ATTRIBUTE_OFFLINE bit and using it in a different
		//            (but similar) manner than the WIN32 documentation indicates ***NOTE***
		if ( steamFindInfo.bIsLocal )
			findInfo->dwFileAttributes &= ~FILE_ATTRIBUTE_OFFLINE;
		else
			findInfo->dwFileAttributes |= FILE_ATTRIBUTE_OFFLINE;
	}
	
	return hResult;
}

/**************/
//STEAM_FindNextFile
/**************/
int STEAM_FindNextFile(HANDLE dir, WIN32_FIND_DATA *findInfo)
{
	TSteamElemInfo steamFindInfo;
	SteamHandle_t hDir = (SteamHandle_t)dir;
	int result;
	TSteamError steamError;

	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	// STEAM return success per _findnext()...flip for FindNextFile()
	result = (SteamFindNext(hDir, &steamFindInfo, &steamError) == 0);
	CheckError(NULL, &steamError);

	if ( result )
	{
		dir = (HANDLE)hDir;
		strcpy(findInfo->cFileName, steamFindInfo.cszName);
		if ( steamFindInfo.bIsDir )
			findInfo->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
		else
			findInfo->dwFileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;
	}
	
	return result;
}

/**************/
//STEAM_FindClose
/**************/
int STEAM_FindClose(HANDLE dir)
{
	SteamHandle_t hDir = (SteamHandle_t)dir;
	int result;
	TSteamError steamError;
	
	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	// STEAM return success per _findclose()...flip for FindClose()
	result = (SteamFindClose(hDir, &steamError) == 0); 
	CheckError(NULL, &steamError);
	return result;
}

/**************/
//STEAM_findfirst
/**************/
long STEAM_findfirst(char *pszMatchName, STEAMFindFilter filter, struct _finddata_t *fileinfo )
{
	TSteamElemInfo steamFindInfo;
	long hResult = -1;
	SteamHandle_t steamResult;
	char tmpMatchName[_MAX_PATH];
	TSteamError steamError;

	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	strcpy(tmpMatchName, pszMatchName);			
	FixSlashes(tmpMatchName);

	steamResult = SteamFindFirst(tmpMatchName, (ESteamFindFilter)filter, &steamFindInfo, &steamError);
	CheckError(NULL, &steamError);

	if ( steamResult == STEAM_INVALID_HANDLE )
	{
		hResult = -1;
	}

	else
	{
		hResult = (long)steamResult;
		strcpy(fileinfo->name, steamFindInfo.cszName);

		fileinfo->size = steamFindInfo.uSizeOrCount;
		fileinfo->time_access = steamFindInfo.lLastAccessTime;
		fileinfo->time_create = steamFindInfo.lCreationTime;
		fileinfo->time_write = steamFindInfo.lLastModificationTime;
		
		// Determine if the found object is a directory...
		if ( steamFindInfo.bIsDir )
			fileinfo->attrib = _A_SUBDIR;
		else
			fileinfo->attrib = _A_NORMAL;

		// ??? Is there anything we can do with bIsLocal?
	}
	
	return hResult;
}

/**************/
//STEAM_findnext
/**************/
int STEAM_findnext(long dir, struct _finddata_t *fileinfo )
{
	TSteamElemInfo steamFindInfo;
	SteamHandle_t hDir = (SteamHandle_t)dir;
	int result;
	TSteamError steamError;

	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	result = SteamFindNext(hDir, &steamFindInfo, &steamError);
	CheckError(NULL, &steamError);

	if ( result == 0 )
	{
		dir = (long)hDir;
		strcpy(fileinfo->name, steamFindInfo.cszName);

		fileinfo->size = steamFindInfo.uSizeOrCount;
		fileinfo->time_access = steamFindInfo.lLastAccessTime;
		fileinfo->time_create = steamFindInfo.lCreationTime;
		fileinfo->time_write = steamFindInfo.lLastModificationTime;
		
		// Determine if the found object is a directory...
		if ( steamFindInfo.bIsDir )
			fileinfo->attrib = _A_SUBDIR;
		else
			fileinfo->attrib = _A_NORMAL;

		// ??? Is there anything we can do with bIsLocal?
	}
	
	return result;
}

/**************/
//STEAM_findclose
/**************/
int STEAM_findclose(long dir)
{
	SteamHandle_t hDir = (SteamHandle_t)dir;
	TSteamError steamError;
	int result;

	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	result = SteamFindClose(hDir, &steamError);
	CheckError(NULL, &steamError);
	return result;
}

/**************/
//STEAM_FileSize
/**************/
unsigned int STEAM_FileSize( FILE *file )
{
	SteamHandle_t hFile = (SteamHandle_t)file;
	TSteamError steamError;
	unsigned int result;

	if ( STEAMtrackingProgress )
	{
		DoEventTic(FALSE);
	}

	result = SteamSizeFile(hFile, &steamError);
	CheckError(file, &steamError);
	return result;
}


#include "string.h"

/*
===================================
STEAM_LoadLibrary

  Wrapper around LoadLibrary() to get
  a local copy of the DLL before the
  load is done.
===================================
*/
HINSTANCE STEAM_LoadLibrary( const char *dllName )
{
	if ( dllName == NULL )
		return NULL;
	STEAM_GetLocalCopy(dllName);
	return LoadLibrary(dllName);
}

/*
===================================
STEAM_GetLocalCopy

Get a local copy of a remote file so that future accesses to this file do not
go out to the server.
===================================
*/
void STEAM_GetLocalCopy( const char *fileName )
{
	char tmpFileName[_MAX_PATH];
	TSteamError steamError;

	if ( STEAMtrackingProgress )
	{
		DoEventTic(TRUE);
	}

	strcpy(tmpFileName, fileName);			
	FixSlashes(tmpFileName);

	SteamGetLocalFileCopy(tmpFileName, &steamError);
	CheckError(NULL, &steamError);
}


/*
===================================
STEAM_FileIsAvailLocal

Determine if the file is available locally.
===================================
*/
int STEAM_FileIsAvailLocal( const char *file )
{
	char tmpFileName[_MAX_PATH];
	TSteamError steamError;
	int result;

	if ( STEAMtrackingProgress )
	{
		DoEventTic(TRUE);
	}

	strcpy(tmpFileName, file);			
	FixSlashes(tmpFileName);

	result = SteamIsFileImmediatelyAvailable(tmpFileName, &steamError);
	CheckError(NULL, &steamError);
	return result;
}

//======================================================================================
// R E S O U R C E   H I N T / P R E L O A D E R   A P I
//======================================================================================

/*
===================================
STEAM_HintResourceNeed

VFS resource preload hint system.

  Input:

	-hintlist is a text list with the base names of the resource lists to preload.
	 A resource list is a text file with a .lst extension found in the 'reslist' subdir
	 of the HL root directory (peer of 'valve' subdir).  It contains a list of file name,
	 offset, length for every resource to be preloaded.

	-forgetEverything is TRUE if the hint system should completely stop working on any queued
	 resource lists.  FALSE will cause the hint system to return to the suspended list(s).

  Returns:

	-TRUE if successful, FALSE otherwise.
===================================
*/
int STEAM_HintResourceNeed( const char *hintlist, int forgetEverything )
{
	TSteamError steamError;
//	int result = SteamHintResourceNeed(hintlist, forgetEverything, &steamError);
	int result = SteamHintResourceNeed( "", hintlist, forgetEverything, &steamError );
	CheckError(NULL, &steamError);
	return result;
}

/*
===================================
STEAM_PauseResourcePreloading

  Suspend STEAM resource preloading
===================================
*/
int STEAM_PauseResourcePreloading(void)
{
	int val = TRUE;

#if STEAM_SYNCHRONIZED_PRELOADING
	{
		val = SteamPauseCachePreloading(&steamError);
		CheckError(NULL, &steamError);
	}
#endif

	return val;
}

/*
===================================
STEAM_ResumeResourcePreloading

  Resume STEAM resource preloading
===================================
*/
int STEAM_ResumeResourcePreloading(void)
{
	int val = TRUE;

#if STEAM_SYNCHRONIZED_PRELOADING
	{
		val = SteamResumeCachePreloading(&steamError);
		CheckError(NULL, &steamError);
	}
#endif

	return val;

}

/*
===================================
STEAM_ForgetAllResourceHints

  Instruct the STEAM resource preload system to stop 
  working on any queued resource lists.
===================================
*/
int STEAM_ForgetAllResourceHints(void)
{
	int val = TRUE;

#if STEAM_SYNCHRONIZED_PRELOADING
	{
		val = SteamForgetAllHints(&steamError);
		CheckError(NULL, &steamError);
	}
#endif

	return val;
}

/*
===================================
STEAM_BlockForResources

  Instruct the STEAM resource preload system to get
  the indicated resources and block until all are
  obtained.

  ***NOTE*** THIS BLOCKS (DUH) SO DON'T USE WANTONLY
===================================
*/
int STEAM_BlockForResources( const char *hintlist )
{
	int val = TRUE;

#if STEAM_SYNCHRONIZED_PRELOADING
	{
//		val = SteamHintResourceNeed(hintlist, TRUE, &steamError); // Temporary implementation.
		val = SteamWaitForResources(hintlist, &steamError);
		CheckError(NULL, &steamError);
	}
#endif

	return val;
}

//======================================================================================
// P R O G R E S S   T R A C K I N G   A P I
//======================================================================================

/*
===================================
STEAM_TrackProgress

  Turn on/off progress tracking,
  i.e. for use with a progress bar.
===================================
*/
void STEAM_TrackProgress(int enable)
{
	STEAMtrackingProgress = enable;
	STEAMprogressCounter = -1;

	return;
}

/*
===================================
STEAM_RegisterAppProgressCallback

  Allow the app to register its callback
  function for progress indication.
===================================
*/
STEAM_RegisterAppProgressCallback( void(*fpProgCallBack)(void), int freq )
{
	AppProgressCallback = fpProgCallBack;
	AppProgressCallbackFreq = freq;
}

/*
===================================
STEAM_RegisterAppKeepAliveTicCallback

  Allow the app to register its callback
  function for keep alive indication.
===================================
*/
STEAM_RegisterAppKeepAliveTicCallback( void(*fpKeepAliveTicCallBack)(char *scr_msg) )
{
	AppKeepAliveTicCallback = fpKeepAliveTicCallBack;
}


/*
==============================
STEAM_UpdateProgress

  Advance progress...

==============================
*/
void STEAM_UpdateProgress( void )
{
	if ( !STEAMtrackingProgress )
		return;
	
	STEAMprogressCounter++;

	if ( AppProgressCallback && (STEAMprogressCounter % AppProgressCallbackFreq) == 0 )
	{
		AppProgressCallback();
	}

}

/*
===================================
STEAM_ProgressCounter

  Return the progress counter.

===================================
*/
int STEAM_ProgressCounter(void)
{
	return STEAMprogressCounter;
}

//======================================================================================
// L O G G I N G   A P I
//======================================================================================

/*
===========
STEAM_LogLevelLoadStarted

VFS logging...started loading a level.

*/
void STEAM_LogLevelLoadStarted( const char *name )
{
#if 0
	char wadName[256];
	int i;
	strcpy(wadName, name);
	strlwr(wadName);
	if ( strstr(wadName, "c0a0b")  )
	   i = 0; // convenient breakpoint spot...
#endif

	SteamLogResourceLoadStarted(name);
}

/*
===========
STEAM_LogLevelLoadFinished

VFS logging...finished loading a level.

*/
void STEAM_LogLevelLoadFinished( const char *name )
{
	SteamLogResourceLoadFinished(name);
	STEAM_TrackProgress(FALSE);
}

//======================================================================================
// V E R S I O N I N G   A P I
//======================================================================================

/*
===========
STEAM_GetInterfaceVersion

VFS version information

*/
void STEAM_GetInterfaceVersion( char *p, int maxlen )
{
	if ( p && maxlen > 0)
		SteamGetVersion(p, maxlen);

	return;
}

