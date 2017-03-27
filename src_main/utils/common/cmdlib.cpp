// -----------------------
// cmdlib.c
// -----------------------

#include <windows.h>
#include "cmdlib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "vstdlib/strtools.h"
#include <conio.h>
#include "utlvector.h"
#include "filesystem_helpers.h"
#include "utllinkedlist.h"


#if defined( MPI )

	#include "vmpi.h"
	#include "vmpi_filesystem.h"
	#include "messbuf.h"
	#include "vmpi_dispatch.h"
	#include "vmpi_tools_shared.h"

#else
	
	#include "filesystem_tools.h"

#endif


#if defined( _WIN32 ) || defined( WIN32 )
#include <direct.h>
#endif


// set these before calling CheckParm
int myargc;
char **myargv;

char		com_token[1024];
qboolean	com_eof;

qboolean		archive;
char			archivedir[1024];

FileHandle_t g_pLogFile = 0;

CUtlLinkedList<CleanupFn, unsigned short> g_CleanupFunctions;
CUtlLinkedList<SpewHookFn, unsigned short> g_ExtraSpewHooks;

bool g_bStopOnExit = false;
void (*g_ExtraSpewHook)(const char*) = NULL;


#if defined( _WIN32 ) || defined( WIN32 )



void CmdLib_FPrintf( FileHandle_t hFile, const char *pFormat, ... )
{
	static CUtlVector<char> buf;
	if ( buf.Count() == 0 )
		buf.SetCount( 1024 );

	va_list marker;
	va_start( marker, pFormat );
	
	while ( 1 )
	{
		int ret = _vsnprintf( buf.Base(), buf.Count(), pFormat, marker );
		if ( ret >= 0 )
		{
			// Write the string.
			g_pFileSystem->Write( buf.Base(), ret, hFile );
			
			// Write a null terminator.
			char cNull = 0;
			g_pFileSystem->Write( &cNull, 1, hFile );
			
			break;
		}
		else
		{
			// Make the buffer larger.
			int newSize = buf.Count() * 2;
			buf.SetCount( newSize );
			if ( buf.Count() != newSize )
			{
				Error( "CmdLib_FPrintf: can't allocate space for text." );
			}
		}
	}

	va_end( marker );
}


char* CmdLib_FGets( char *pOut, int outSize, FileHandle_t hFile )
{
	for ( int iCur=0; iCur < (outSize-1); iCur++ )
	{
		char c;
		if ( !g_pFileSystem->Read( &c, 1, hFile ) )
		{
			if ( iCur == 0 )
				return NULL;
			else
				break;
		}

		pOut[iCur] = c;
		if ( c == '\n' )
			break;

		if ( c == EOF )
		{
			if ( iCur == 0 )
				return NULL;
			else
				break;
		}
	}

	pOut[iCur] = 0;
	return pOut;
}


#include <wincon.h>


// This pauses before exiting if they use -StopOnExit. Useful for debugging.
class CExitStopper
{
public:
	~CExitStopper()
	{
		if ( g_bStopOnExit )
		{
			Warning( "\nPress any key to quit.\n" );
			getch();
		}
	}
} g_ExitStopper;




static unsigned short g_InitialColor = 0xFFFF;
static unsigned short g_LastColor = 0xFFFF;
static unsigned short g_BadColor = 0xFFFF;
static WORD g_BackgroundFlags = 0xFFFF;

static void GetInitialColors( )
{
	// Get the old background attributes.
	CONSOLE_SCREEN_BUFFER_INFO oldInfo;
	GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ), &oldInfo );
	g_InitialColor = g_LastColor = oldInfo.wAttributes & (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY);
	g_BackgroundFlags = oldInfo.wAttributes & (BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE|BACKGROUND_INTENSITY);

	g_BadColor = 0;
	if (g_BackgroundFlags & BACKGROUND_RED)
		g_BadColor |= FOREGROUND_RED;
	if (g_BackgroundFlags & BACKGROUND_GREEN)
		g_BadColor |= FOREGROUND_GREEN;
	if (g_BackgroundFlags & BACKGROUND_BLUE)
		g_BadColor |= FOREGROUND_BLUE;
	if (g_BackgroundFlags & BACKGROUND_INTENSITY)
		g_BadColor |= FOREGROUND_INTENSITY;
}

static WORD SetConsoleTextColor( int red, int green, int blue, int intensity )
{
	WORD ret = g_LastColor;
	
	g_LastColor = 0;
	if( red )	g_LastColor |= FOREGROUND_RED;
	if( green ) g_LastColor |= FOREGROUND_GREEN;
	if( blue )  g_LastColor |= FOREGROUND_BLUE;
	if( intensity ) g_LastColor |= FOREGROUND_INTENSITY;

	// Just use the initial color if there's a match...
	if (g_LastColor == g_BadColor)
		g_LastColor = g_InitialColor;

	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), g_LastColor | g_BackgroundFlags );
	return ret;
}


static void RestoreConsoleTextColor( WORD color )
{
	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), color | g_BackgroundFlags );
	g_LastColor = color;
}


#if defined( CMDLIB_NODBGLIB )

// This can go away when everything is in bin.
void Error( char const *pMsg, ... )
{
	va_list marker;
	va_start( marker, pMsg );
	vprintf( pMsg, marker );
	va_end( marker );

	exit( -1 );
}

#else

CRITICAL_SECTION g_SpewCS;
bool g_bSpewCSInitted = false;
bool g_bSuppressPrintfOutput = false;

SpewRetval_t CmdLib_SpewOutputFunc( SpewType_t type, char const *pMsg )
{
	// Hopefully two threads won't call this simultaneously right at the start!
	if ( !g_bSpewCSInitted )
	{
		InitializeCriticalSection( &g_SpewCS );
		g_bSpewCSInitted = true;
	}

	WORD old;
	SpewRetval_t retVal;
	
	EnterCriticalSection( &g_SpewCS );
	{
		if (( type == SPEW_MESSAGE ) || (type == SPEW_LOG ))
		{
			old = SetConsoleTextColor( 1, 1, 1, 0 );
			retVal = SPEW_CONTINUE;
		}
		else if( type == SPEW_WARNING )
		{
			old = SetConsoleTextColor( 1, 1, 0, 1 );
			retVal = SPEW_CONTINUE;
		}
		else if( type == SPEW_ASSERT )
		{
			old = SetConsoleTextColor( 1, 0, 0, 1 );
			retVal = SPEW_DEBUGGER;

#ifdef MPI
			// VMPI workers don't want to bring up dialogs and suchlike.			
			if ( g_bUseMPI && !g_bMPIMaster )
			{
				VMPI_HandleCrash( pMsg, true );
				exit( 0 );
			}
#endif
		}
		else if( type == SPEW_ERROR )
		{
			old = SetConsoleTextColor( 1, 0, 0, 1 );
			retVal = SPEW_ABORT; // doesn't matter.. we exit below so we can return an errorlevel (which dbg.dll doesn't do).
		}
		else
		{
			old = SetConsoleTextColor( 1, 1, 1, 1 );
			retVal = SPEW_CONTINUE;
		}

		if ( !g_bSuppressPrintfOutput || type == SPEW_ERROR )
			printf( "%s", pMsg );

		OutputDebugString( pMsg );
		
		if ( type == SPEW_ERROR )
		{
			printf( "\n" );
			OutputDebugString( "\n" );
		}

		if( g_pLogFile )
		{
			CmdLib_FPrintf( g_pLogFile, "%s", pMsg );
			g_pFileSystem->Flush( g_pLogFile );
		}

		// Dispatch to other spew hooks.
		FOR_EACH_LL( g_ExtraSpewHooks, i )
			g_ExtraSpewHooks[i]( pMsg );

		RestoreConsoleTextColor( old );
	}
	LeaveCriticalSection( &g_SpewCS );

	if ( type == SPEW_ERROR )
	{
		CmdLib_Exit( 1 );
	}

	return retVal;
}


void InstallSpewFunction()
{
	setvbuf( stdout, NULL, _IONBF, 0 );
	setvbuf( stderr, NULL, _IONBF, 0 );

	SpewOutputFunc( CmdLib_SpewOutputFunc );
	GetInitialColors();
}


void InstallExtraSpewHook( SpewHookFn pFn )
{
	g_ExtraSpewHooks.AddToTail( pFn );
}


void CmdLib_AllocError( unsigned long size )
{
	Error( "Error trying to allocate %d bytes.\n", size );
}


void InstallAllocationFunctions()
{
	Plat_SetAllocErrorFn( CmdLib_AllocError );
}


void SetSpewFunctionLogFile( char const *pFilename )
{
	Assert( (!g_pLogFile) );
	g_pLogFile = g_pFileSystem->Open( pFilename, "a" );

	Assert( g_pLogFile );
	if (!g_pLogFile)
		Error("Can't create LogFile:\"%s\"\n", pFilename );

	CmdLib_FPrintf( g_pLogFile, "\n\n\n" );
}


void CloseSpewFunctionLogFile()
{
	if ( g_pFileSystem && g_pLogFile )
	{
		g_pFileSystem->Close( g_pLogFile );
		g_pLogFile = FILESYSTEM_INVALID_HANDLE;
	}
}


void CmdLib_AtCleanup( CleanupFn pFn )
{
	g_CleanupFunctions.AddToTail( pFn );
}


void CmdLib_Cleanup()
{
	CloseSpewFunctionLogFile();

	CmdLib_TermFileSystem();

#if defined( MPI )
	// Unfortunately, when you call exit(), even if you have things registered with atexit(),
	// threads go into a seemingly undefined state where GetExitCodeThread gives STILL_ACTIVE
	// and WaitForSingleObject will stall forever on the thread. Because of this, we must cleanup
	// everything that uses threads before exiting.
	VMPI_Finalize();
#endif

	FOR_EACH_LL( g_CleanupFunctions, i )
		g_CleanupFunctions[i]();
}


void CmdLib_Exit( int exitCode )
{
	CmdLib_Cleanup();
	exit( -1 );
}	



#endif

#endif




/*
===================
ExpandWildcards

Mimic unix command line expansion
===================
*/
#define	MAX_EX_ARGC	1024
int		ex_argc;
char	*ex_argv[MAX_EX_ARGC];
#ifdef _WIN32
#include "io.h"
void ExpandWildcards (int *argc, char ***argv)
{
	struct _finddata_t fileinfo;
	int		handle;
	int		i;
	char	filename[1024];
	char	filebase[1024];
	char	*path;

	ex_argc = 0;
	for (i=0 ; i<*argc ; i++)
	{
		path = (*argv)[i];
		if ( path[0] == '-'
			|| ( !strstr(path, "*") && !strstr(path, "?") ) )
		{
			ex_argv[ex_argc++] = path;
			continue;
		}

		handle = _findfirst (path, &fileinfo);
		if (handle == -1)
			return;

		ExtractFilePath (path, filebase);

		do
		{
			sprintf (filename, "%s%s", filebase, fileinfo.name);
			ex_argv[ex_argc++] = copystring (filename);
		} while (_findnext( handle, &fileinfo ) != -1);

		_findclose (handle);
	}

	*argc = ex_argc;
	*argv = ex_argv;
}
#else
void ExpandWildcards (int *argc, char ***argv)
{
}
#endif


// only printf if in verbose mode
qboolean verbose = false;
void qprintf (char *format, ...)
{
	if (!verbose)
		return;

	va_list argptr;
	va_start (argptr,format);

	char str[2048];
	_vsnprintf( str, sizeof(str), format, argptr );

#if defined( CMDLIB_NODBGLIB )
	printf( "%s", str );
#else
	Msg( "%s", str );
#endif

	va_end (argptr);
}

// ---------------------------------------------------------------------------
//
// These are the base paths that everything will be referenced relative to (textures especially)
// All of these directories include the trailing slash
//
// ---------------------------------------------------------------------------

// This is the the path of the initial source file (relative to the cwd)
char		qdir[1024];

// This is the base engine + mod-specific game dir (e.g. "c:/tf2/mytfmod/")
char		gamedir[1024];	

// This is the base engine directory (c:\hl2).
char		basedir[1024];

// This is the base engine + base game directory (e.g. "c:/hl2/hl2/", or "c:/tf2/tf2/" )
char		basegamedir[1024];

// this is the override directory
char qproject[1024] = {0};





void COM_FixSlashes( char *pname )
{
	while ( *pname ) {
		if ( *pname == '\\' )
			*pname = '/';
		pname++;
	}
}


// strip off the last directory from dirName
bool StripLastDir( char *dirName )
{
	if( dirName[0] == 0 || !Q_stricmp( dirName, "./" ) || !Q_stricmp( dirName, ".\\" ) )
		return false;
	
	int len = Q_strlen( dirName );

	// skip trailing slash
	if ( dirName[len-1] == '/' )
		len--;

	while ( len > 0 )
	{
		if ( PATHSEPARATOR( dirName[len-1] ) )
		{
			dirName[len] = 0;
			COM_FixSlashes( dirName );
			return true;
		}
		len--;
	}

	// Allow it to return an empty string and true. This can happen if something like "tf2/" is passed in.
	// The correct behavior is to strip off the last directory ("tf2") and return true.
	if( len == 0 )
	{
		Q_strcpy( dirName, "./" );
		return true;
	}

	return true;
}


void AppendSlash( char *pStr, int strSize )
{
	int len;

	len = Q_strlen( pStr );

	if ( !PATHSEPARATOR(pStr[len-1]) )
	{
		if ( len+1 >= strSize )
		{
			Error( "AppendSlash( %s ): ran out of buffer space", pStr );
		}
		
		pStr[len] = '/';
		len++;
		pStr[len] = 0;
	}
}


// merge dir/file into a full path, adding a / if necessary
void MakePath( char *dest, int destLen, const char *dirName, const char *fileName )
{
	Q_strncpy( dest, dirName, destLen );

	AppendSlash( dest, destLen );

	Q_strncat( dest, fileName, destLen );
	COM_FixSlashes( dest );
}


// Find the parent dir that contains "engine.DLL" - identifies the base engine directory
qboolean FindBasePath( char *root )
{
	char fileName[1024], dirName[1024];

	strcpy( dirName, root );
	AppendSlash( dirName, sizeof( dirName ) );

	while ( true ) 
	{
		// check for engine.dll
		MakePath( fileName, sizeof( fileName ), dirName, "engine.dll" );
		if ( FileExists( fileName ) )
		{
			strcpy( root, dirName );
			return true;
		}
		if ( !StripLastDir( dirName ) )
			return false;
	} 
}


// Returns true if the path specified is an absolute pathname.
bool IsAbsolutePath( const char *pDir )
{
	// Does it have a drive letter like "d:/test.txt"?
	if ( strstr( pDir, ":" ) )
	{
		return true;
	}
	else
	{
		// If it starts with a slash, it either begins at the root like "/hl2/bin/file.txt"
		// or it's a UNC path like "//machine/share/file.txt".
		if ( PATHSEPARATOR( pDir[0] ) )
		{
			return true;
		}
	}
	
	return false;
}


// This removes "./" and "../" from the pathname, sort of "normalizing" the path.
// pFilename should be a full pathname.
// Returns false if it tries to ".." past the root directory in the drive (in which case 
// it is an invalid path).
bool RemoveDotSlashes( char *pFilename )
{
	// Get rid of "./"'s
	char *pIn = pFilename;
	char *pOut = pFilename;
	while ( *pIn )
	{
		// The logic on the second line is preventing it from screwing up "../"
		if ( pIn[0] == '.' && PATHSEPARATOR( pIn[1] ) &&
			(pIn == pFilename || pIn[-1] != '.') )
		{
			pIn += 2;
		}
		else
		{
			*pOut = *pIn;
			++pIn;
			++pOut;
		}
	}
	*pOut = 0;


	// Each time we encounter a "..", back up until we've read the previous directory name,
	// then get rid of it.
	pIn = pFilename;
	while ( *pIn )
	{
		if ( pIn[0] == '.' && pIn[1] == '.' )
		{
			char *pEndOfDots = pIn + 2;
			char *pStart = pIn - 2;

			// Ok, now scan back for the path separator that starts the preceding directory.
			while ( 1 )
			{
				if ( pStart < pFilename )
					return false;

				if ( PATHSEPARATOR( *pStart ) )
					break;

				--pStart;
			}

			// Now slide the string down to get rid of the previous directory and the ".."
			memmove( pStart, pEndOfDots, strlen( pEndOfDots ) + 1 );

			// Start over.
			pIn = pFilename;
		}
		else
		{
			++pIn;
		}
	}
	
	return true;
}


static void StripTrailingSlash( char *ppath )
{
	int len;

	len = strlen( ppath );

	if ( len > 0 )
	{
		if ( (ppath[len-1] == '\\') || (ppath[len-1] == '/'))
			ppath[len-1] = 0;
	}
}

// This will search for "engine.DLL" and "server.DLL" and use the presence of those files
// to properly set up your working directories.
//-----------------------------------------------------------------------------
// Purpose: configures the strings
//			NOTE: These strings get MODIFIED.  THEY MUST BE SEPARATE AND WRITABLE
// Input  : *root - root directory (best guess at the dir containing the engine)
//			*subdir - subdir best guess for the root game dir
// Output : qboolean - true if successful
//-----------------------------------------------------------------------------
qboolean FindGamedir( const char *path )
{
	int len;
	char root[1024];
	char temp[1024];

	strcpy( gamedir, path );

	if ( !gamedir )
		return false;

	len = strlen( gamedir );
	if ( len < 2 )
		return false;

	// cleanup the path
	COM_FixSlashes( gamedir );
	strlwr( gamedir );
	AppendSlash( gamedir, sizeof( gamedir ) );

	// strip off the end
	strcpy( root, gamedir );
	if ( !StripLastDir( root ) )
		return false;

	// set base game dir to hl2 or tf2
	MakePath( temp, sizeof( temp ), root, "hl2/bin/server.dll" );
	if ( _access( temp, 0 ) != 0 )	// NOTE: it must not use the filesystem here because the filesystem search paths would mess it up.
	{
		MakePath( temp, sizeof( temp ), root, "tf2/bin/server.dll" );
		if ( _access( temp, 0 ) != 0 )
			return false;

		MakePath( basegamedir, sizeof( basegamedir ), root, "tf2/" );
	}
	else
	{
		MakePath( basegamedir, sizeof( basegamedir ), root, "hl2/" );
	}

	Q_strncpy( basedir, root, sizeof( basedir ) );


	// Make the paths into full paths.
	char *pDirs[4] = {basedir, basegamedir, gamedir, qdir};
	for ( int i=0; i < 4; i++ )
	{
		char *pDir = pDirs[i];
		if ( !IsAbsolutePath( pDir ) )
		{
			char tempDir[1024];
			Q_strncpy( tempDir, pDir, sizeof( tempDir ) );
			_getcwd( pDir, 1024 );
			StripTrailingSlash( pDir );
			Q_strncat( pDir, "\\", 1024 );
			Q_strncat( pDir, tempDir, 1024 );
			COM_FixSlashes( pDir );

			char originalName[1024];
			Q_strncpy( originalName, pDir, sizeof( originalName ) );
			
			// "Normalize" the path so VMPI's file system understands it.
			if ( !RemoveDotSlashes( pDir ) )
				Error( "RemoveDotSlashes( %s ) error\n", originalName );
		}
	}


#if defined( MPI )

	// Send this to the workers because they will wait for it in SetQdirFromPath.
	if ( g_bUseMPI && g_bMPIMaster )
	{
		SendQDirInfo();
	}

#else

	// Add search paths if we're not using MPI.
	if ( g_pFullFileSystem )
	{
		g_pFullFileSystem->RemoveSearchPath( NULL, "GAME" );
		g_pFullFileSystem->AddSearchPath( basegamedir, "GAME", PATH_ADD_TO_HEAD );
		g_pFullFileSystem->AddSearchPath( gamedir, "GAME", PATH_ADD_TO_HEAD );
	}

#endif

	return true;
}


void SetQdirFromPath( const char *path )
{
#if defined( MPI )
	// If we're a worker, since we don't own the file system, let the master send this data.
	if ( g_bUseMPI && !g_bMPIMaster )
	{
		RecvQDirInfo();
		return;
	}
#endif

	char	temp[1024];
	char	*pProject = getenv("VPROJECT");

	strcpy( qdir, path );
	
	// qdir
	COM_FixSlashes( qdir );
	strlwr( qdir );
	StripFilename( qdir );
	if (qdir[0] != '\0')
	{
		AppendSlash( qdir, sizeof( qdir ) );
	}

	// qproject overrides all other checks.
	if ( strlen( qproject ) )
	{
		if ( FindGamedir( qproject ))
		{
			return;
		}
		Error("Can't setup path from :\"%s\"\n", qproject );
	}

	// use vproject if it exists
	if ( pProject )
	{
		if ( FindGamedir( pProject ))
		{
			return;
		}
		// Error("Can't setup path, use -proj or set VPROJECT to your game directory (e.g. D:/DEV/TF2) in your environment\n");
	}

	// try the path they sent
	strcpy( temp, path );
	do 
	{
		if ( FindGamedir( temp ))
		{
			return;
		}
	} while ( StripLastDir( temp ) );

	// use the cwd and hunt down the tree until we find something
	Q_getwd( temp );
	do
	{
		if ( FindGamedir( temp ))
		{
			return;
		}
	} while ( StripLastDir( temp ) );

	Error("Can't setup path, set VPROJECT to your game directory (e.g. D:/DEV/TF2) in your environment\n");
}


#if defined( MPI )
	
	// Share some functions out for vmpi.lib
	class CVMPIFileSystemHelpers : public IVMPIFileSystemHelpers
	{
	public:
		virtual const char*	GetBaseDir()
		{
			return basedir;
		}

		virtual void		COM_FixSlashes( char *pname )
		{
			::COM_FixSlashes( pname );
		}

		virtual bool		StripLastDir( char *dirName )
		{
			return ::StripLastDir( dirName );
		}

		virtual void		MakePath( char *dest, int destLen, const char *dirName, const char *fileName )
		{
			::MakePath( dest, destLen, dirName, fileName );
		}
	};
	static CVMPIFileSystemHelpers g_VMPIFileSystemHelpers;


	void CmdLib_InitFileSystem( const char *pBSPFilename, bool bUseEngineFileSystem )
	{
		Assert( !bUseEngineFileSystem );	// The VMPI file system doesn't support search paths.		
		g_pFileSystem = VMPI_FileSystem_Init( &g_VMPIFileSystemHelpers );
		SetQdirFromPath( pBSPFilename );
	}


	void CmdLib_TermFileSystem()
	{
		VMPI_FileSystem_Term();
	}


	CreateInterfaceFn CmdLib_GetFileSystemFactory()
	{
		return VMPI_FileSystem_GetFactory();
	}

#else // MPI

	#include "filesystem_tools.h"
	


	void CmdLib_InitFileSystem( const char *pBSPFilename, bool bUseEngineFileSystem )
	{
		FileSystem_Init( bUseEngineFileSystem );
		SetQdirFromPath( pBSPFilename );
	}


	void CmdLib_TermFileSystem()
	{
	}


	CreateInterfaceFn CmdLib_GetFileSystemFactory()
	{
		return FileSystem_GetFactory();
	}


#endif


char *ExpandArg (char *path)
{
	static char full[1024];

	if (path[0] != '/' && path[0] != '\\' && path[1] != ':')
	{
		Q_getwd (full);
		strcat (full, path);
	}
	else
		strcpy (full, path);
	return full;
}


char *ExpandPath (char *path)
{
	static char full[1024];
	if (path[0] == '/' || path[0] == '\\' || path[1] == ':')
		return path;
	sprintf (full, "%s%s", qdir, path);
	return full;
}



char *copystring(char *s)
{
	char	*b;
	b = (char *)malloc(strlen(s)+1);
	strcpy (b, s);
	return b;
}



/*
================
I_FloatTime
================
*/
double I_FloatTime (void)
{
	return Plat_FloatTime();
#if 0
// more precise, less portable
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);
	
	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000000.0;
	}
	
	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
#endif
}

void Q_getwd (char *out)
{
#if defined( _WIN32 ) || defined( WIN32 )
	_getcwd (out, 256);
	strcat (out, "\\");
#else
	getwd (out);
	strcat (out, "/");
#endif
	COM_FixSlashes( out );
}


void Q_mkdir (char *path)
{
#if defined( _WIN32 ) || defined( WIN32 )
	if (_mkdir (path) != -1)
		return;
#else
	if (mkdir (path, 0777) != -1)
		return;
#endif
	if (errno != EEXIST)
		Error ("mkdir %s: %s",path, strerror(errno));
}

/*
============
FileTime

returns -1 if not present
============
*/
int	FileTime (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}



/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	return (char*)ParseFile( data, com_token, NULL );
}


/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/


/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
int CheckParm (char *check)
{
	int             i;

	for (i = 1;i<myargc;i++)
	{
		if ( !Q_strcasecmp(check, myargv[i]) )
			return i;
	}

	return 0;
}



/*
================
Q_filelength
================
*/
int Q_filelength (FileHandle_t f)
{
	return g_pFileSystem->Size( f );
}


FileHandle_t SafeOpenWrite (char *filename)
{
	FileHandle_t f = g_pFileSystem->Open(filename, "wb");

	if (!f)
	{
		//Error ("Error opening %s: %s",filename,strerror(errno));
		// BUGBUG: No way to get equivalent of errno from IFileSystem!
		Error ("Error opening %s! (Check for write enable)\n",filename);
	}

	return f;
}

#define MAX_CMDLIB_BASE_PATHS 10
static char g_pBasePaths[MAX_CMDLIB_BASE_PATHS][MAX_PATH];
static int g_NumBasePaths = 0;

void CmdLib_AddBasePath( const char *pPath )
{
//	printf( "CmdLib_AddBasePath( \"%s\" )\n", pPath );
	if( g_NumBasePaths < MAX_CMDLIB_BASE_PATHS )
	{
		Q_strncpy( g_pBasePaths[g_NumBasePaths], pPath, MAX_PATH );
		COM_FixSlashes( g_pBasePaths[g_NumBasePaths] );
		g_NumBasePaths++;
	}
	else
	{
		Assert( 0 );
	}
}

bool CmdLib_HasBasePath( const char *pFileName_, int &pathLength )
{
	char *pFileName = ( char * )_alloca( strlen( pFileName_ ) + 1 );
	strcpy( pFileName, pFileName_ );
	COM_FixSlashes( pFileName );
	pathLength = 0;
	int i;
	for( i = 0; i < g_NumBasePaths; i++ )
	{
		// see if we can rip the base off of the filename.
		if( Q_strncasecmp( g_pBasePaths[i], pFileName, strlen( g_pBasePaths[i] ) ) == 0 )
		{
			pathLength = strlen( g_pBasePaths[i] );
			return true;
		}
	}
	return false;
}

int CmdLib_GetNumBasePaths( void )
{
	return g_NumBasePaths;
}

const char *CmdLib_GetBasePath( int i )
{
	Assert( i >= 0 && i < g_NumBasePaths );
	return g_pBasePaths[i];
}

FileHandle_t SafeOpenRead( char *filename )
{
	int pathLength;
	FileHandle_t f = 0;
	if( CmdLib_HasBasePath( filename, pathLength ) )
	{
		filename = filename + pathLength;
		int i;
		for( i = 0; i < g_NumBasePaths; i++ )
		{
			char tmp[MAX_PATH];
			strcpy( tmp, g_pBasePaths[i] );
			strcat( tmp, filename );
			f = g_pFileSystem->Open( tmp, "rb" );
			if( f )
			{
				return f;
			}
		}
		Error ("Error opening %s: %s",filename,strerror(errno));
		return f;
	}
	else
	{
		f = g_pFileSystem->Open( filename, "rb" );
		if ( !f )
			Error ("Error opening %s: %s",filename,strerror(errno));

		return f;
	}
}

void SafeRead( FileHandle_t f, void *buffer, int count)
{
	if ( g_pFileSystem->Read (buffer, count, f) != (size_t)count)
		Error ("File read failure");
}


void SafeWrite ( FileHandle_t f, void *buffer, int count)
{
	if (g_pFileSystem->Write (buffer, count, f) != (size_t)count)
		Error ("File write failure");
}


/*
==============
FileExists
==============
*/
qboolean	FileExists (char *filename)
{
	FileHandle_t hFile = g_pFileSystem->Open( filename, "rb" );
	if ( hFile == FILESYSTEM_INVALID_HANDLE )
	{
		return false;
	}
	else
	{
		g_pFileSystem->Close( hFile );
		return true;
	}
}

/*
==============
LoadFile
==============
*/
int    LoadFile (char *filename, void **bufferptr)
{
	int    length;
	void    *buffer;

	FileHandle_t f = SafeOpenRead (filename);
	length = Q_filelength (f);
	buffer = malloc (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, buffer, length);
	g_pFileSystem->Close (f);

	*bufferptr = buffer;
	return length;
}



/*
==============
SaveFile
==============
*/
void    SaveFile (char *filename, void *buffer, int count)
{
	FileHandle_t f = SafeOpenWrite (filename);
	SafeWrite (f, buffer, count);
	g_pFileSystem->Close (f);
}



void DefaultExtension (char *path, char *extension)
{
	char    *src;
//
// if path doesnt have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (!PATHSEPARATOR(*src) && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}


void DefaultPath (char *path, char *basepath)
{
	char    temp[128];

	if ( PATHSEPARATOR(path[0]) )
		return;                   // absolute path location
	strcpy (temp,path);
	strcpy (path,basepath);
	strcat (path,temp);
}


void    StripFilename (char *path)
{
	int             length;

	length = strlen(path)-1;
	while (length > 0 && !PATHSEPARATOR(path[length]))
		length--;
	path[length] = 0;
}

void    StripExtension (char *path)
{
	int             length;

	COM_FixSlashes( path );

	length = strlen(path)-1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/')
			return;		// no extension
	}
	if (length)
		path[length] = 0;
}


/*
====================
Extract file parts
====================
*/
// FIXME: should include the slash, otherwise
// backing to an empty path will be wrong when appending a slash
void ExtractFilePath (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != '\\' && *(src-1) != '/')
		src--;

	memcpy (dest, path, src-path);
	dest[src-path] = 0;
}

void ExtractFileBase (const char *path, char *dest, int destSize)
{
	char *pDestStart = dest;

	if ( destSize <= 1 )
		return;

	const char *src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && !PATHSEPARATOR(*(src-1)))
		src--;

	while (*src && *src != '.')
	{
		*dest++ = *src++;
		if ( dest - pDestStart >= destSize-1 )
			break;
	}
	*dest = 0;
}

void ExtractFileExtension (const char *path, char *dest, int destSize)
{
	const char    *src;

	src = path + strlen(path) - 1;

//
// back up until a . or the start
//
	while (src != path && *(src-1) != '.' )
		src--;

	// check to see if the '.' is part of a pathname
	if (src == path || *src == '/' || *src == '\\' )
	{
		*dest = 0;	// no extension
		return;
	}

	Q_strncpy( dest, src, destSize );
}


/*
==============
ParseNum / ParseHex
==============
*/
int ParseHex (char *hex)
{
	char    *str;
	int    num;

	num = 0;
	str = hex;

	while (*str)
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str-'0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str-'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str-'A';
		else
			Error ("Bad hex number: %s",hex);
		str++;
	}

	return num;
}


int ParseNum (char *str)
{
	if (str[0] == '$')
		return ParseHex (str+1);
	if (str[0] == '0' && str[1] == 'x')
		return ParseHex (str+2);
	return atol (str);
}




//=======================================================


// FIXME: byte swap?

// this is a 16 bit, non-reflected CRC using the polynomial 0x1021
// and the initial and final xor values shown below...  in other words, the
// CCITT standard CRC used by XMODEM

#define CRC_INIT_VALUE	0xffff
#define CRC_XOR_VALUE	0x0000

static unsigned short crctable[256] =
{
	0x0000,	0x1021,	0x2042,	0x3063,	0x4084,	0x50a5,	0x60c6,	0x70e7,
	0x8108,	0x9129,	0xa14a,	0xb16b,	0xc18c,	0xd1ad,	0xe1ce,	0xf1ef,
	0x1231,	0x0210,	0x3273,	0x2252,	0x52b5,	0x4294,	0x72f7,	0x62d6,
	0x9339,	0x8318,	0xb37b,	0xa35a,	0xd3bd,	0xc39c,	0xf3ff,	0xe3de,
	0x2462,	0x3443,	0x0420,	0x1401,	0x64e6,	0x74c7,	0x44a4,	0x5485,
	0xa56a,	0xb54b,	0x8528,	0x9509,	0xe5ee,	0xf5cf,	0xc5ac,	0xd58d,
	0x3653,	0x2672,	0x1611,	0x0630,	0x76d7,	0x66f6,	0x5695,	0x46b4,
	0xb75b,	0xa77a,	0x9719,	0x8738,	0xf7df,	0xe7fe,	0xd79d,	0xc7bc,
	0x48c4,	0x58e5,	0x6886,	0x78a7,	0x0840,	0x1861,	0x2802,	0x3823,
	0xc9cc,	0xd9ed,	0xe98e,	0xf9af,	0x8948,	0x9969,	0xa90a,	0xb92b,
	0x5af5,	0x4ad4,	0x7ab7,	0x6a96,	0x1a71,	0x0a50,	0x3a33,	0x2a12,
	0xdbfd,	0xcbdc,	0xfbbf,	0xeb9e,	0x9b79,	0x8b58,	0xbb3b,	0xab1a,
	0x6ca6,	0x7c87,	0x4ce4,	0x5cc5,	0x2c22,	0x3c03,	0x0c60,	0x1c41,
	0xedae,	0xfd8f,	0xcdec,	0xddcd,	0xad2a,	0xbd0b,	0x8d68,	0x9d49,
	0x7e97,	0x6eb6,	0x5ed5,	0x4ef4,	0x3e13,	0x2e32,	0x1e51,	0x0e70,
	0xff9f,	0xefbe,	0xdfdd,	0xcffc,	0xbf1b,	0xaf3a,	0x9f59,	0x8f78,
	0x9188,	0x81a9,	0xb1ca,	0xa1eb,	0xd10c,	0xc12d,	0xf14e,	0xe16f,
	0x1080,	0x00a1,	0x30c2,	0x20e3,	0x5004,	0x4025,	0x7046,	0x6067,
	0x83b9,	0x9398,	0xa3fb,	0xb3da,	0xc33d,	0xd31c,	0xe37f,	0xf35e,
	0x02b1,	0x1290,	0x22f3,	0x32d2,	0x4235,	0x5214,	0x6277,	0x7256,
	0xb5ea,	0xa5cb,	0x95a8,	0x8589,	0xf56e,	0xe54f,	0xd52c,	0xc50d,
	0x34e2,	0x24c3,	0x14a0,	0x0481,	0x7466,	0x6447,	0x5424,	0x4405,
	0xa7db,	0xb7fa,	0x8799,	0x97b8,	0xe75f,	0xf77e,	0xc71d,	0xd73c,
	0x26d3,	0x36f2,	0x0691,	0x16b0,	0x6657,	0x7676,	0x4615,	0x5634,
	0xd94c,	0xc96d,	0xf90e,	0xe92f,	0x99c8,	0x89e9,	0xb98a,	0xa9ab,
	0x5844,	0x4865,	0x7806,	0x6827,	0x18c0,	0x08e1,	0x3882,	0x28a3,
	0xcb7d,	0xdb5c,	0xeb3f,	0xfb1e,	0x8bf9,	0x9bd8,	0xabbb,	0xbb9a,
	0x4a75,	0x5a54,	0x6a37,	0x7a16,	0x0af1,	0x1ad0,	0x2ab3,	0x3a92,
	0xfd2e,	0xed0f,	0xdd6c,	0xcd4d,	0xbdaa,	0xad8b,	0x9de8,	0x8dc9,
	0x7c26,	0x6c07,	0x5c64,	0x4c45,	0x3ca2,	0x2c83,	0x1ce0,	0x0cc1,
	0xef1f,	0xff3e,	0xcf5d,	0xdf7c,	0xaf9b,	0xbfba,	0x8fd9,	0x9ff8,
	0x6e17,	0x7e36,	0x4e55,	0x5e74,	0x2e93,	0x3eb2,	0x0ed1,	0x1ef0
};

void CRC_Init(unsigned short *crcvalue)
{
	*crcvalue = CRC_INIT_VALUE;
}

void CRC_ProcessByte(unsigned short *crcvalue, byte data)
{
	*crcvalue = (*crcvalue << 8) ^ crctable[(*crcvalue >> 8) ^ data];
}

unsigned short CRC_Value(unsigned short crcvalue)
{
	return crcvalue ^ CRC_XOR_VALUE;
}
//=============================================================================

/*
============
CreatePath
============
*/
void	CreatePath (char *path)
{
	char	*ofs, c;

	if (path[1] == ':')
		path += 2;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		c = *ofs;
		if (c == '/' || c == '\\')
		{	// create the directory
			*ofs = 0;
			Q_mkdir (path);
			*ofs = c;
		}
	}
}


/*
============
QCopyFile

  Used to archive source files
============
*/
void QCopyFile (char *from, char *to)
{
	void	*buffer;
	int		length;

	length = LoadFile (from, &buffer);
	CreatePath (to);
	SaveFile (to, buffer, length);
	free (buffer);
}

//-----------------------------------------------------------------------------
// Purpose: Writes pFileName with a new extension (pExt) into pOut
// Input  : *pOut - output buffer
//			*pFileName - name of file with arbitrary/none extension: e.g. "FILENAME.TMP"
//			*pExt - extension name including "." e.g. ".OUT"
// Output: "FILENAME.OUT" written to pOut
//-----------------------------------------------------------------------------
void SetExtension( char *pOut, const char *pFileName, const char *pExt )
{
	char *pExtCopy;

	if ( pOut != pFileName )
	{
		strcpy( pOut, pFileName );
	}
	pExtCopy = strstr( pOut, "." );
	if ( !pExtCopy )
	{
		pExtCopy = pOut + strlen(pOut);
	}
	pExtCopy[0] = 0;
	strcpy( pExtCopy, pExt );
}




