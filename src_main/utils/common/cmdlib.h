//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef CMDLIB_H
#define CMDLIB_H

#ifdef _WIN32
#pragma once
#endif

// cmdlib.h

#include "basetypes.h"

// This can go away when everything is in bin.
#if defined( CMDLIB_NODBGLIB )
	void Error( char const *pMsg, ... );
#else
	#include "tier0/dbg.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include "filesystem.h"


#if defined( _WIN32 ) || defined( WIN32 )
#define PATHSEPARATOR(c) ((c) == '\\' || (c) == '/')
#else	//_WIN32
#define PATHSEPARATOR(c) ((c) == '/')
#endif	//_WIN32


// Tools should use this to fprintf data to files.
void CmdLib_FPrintf( FileHandle_t hFile, const char *pFormat, ... );
char* CmdLib_FGets( char *pOut, int outSize, FileHandle_t hFile );


// This can be set so Msg() sends output to hook functions (like the VMPI MySQL database),
// but doesn't actually printf the output.
extern bool g_bSuppressPrintfOutput;

extern IBaseFileSystem *g_pFileSystem;

// These use either the MPI file system or the stdio file system, depending on
// if MPI is defined or not.
//
// This also does what SetQdirFromPath used to do.
//
// NOTE: If bUseEngineFileSystem is true, it loads filesystem_stdio.dll and sets up search paths.
//       This should only be used by tools that need to use search paths.
// 
// g_pFullFileSystem will be NULL unless bUseEngineFileSystem is true.
void				CmdLib_InitFileSystem( const char *pFilename, bool bUseEngineFileSystem = false );
void				CmdLib_TermFileSystem();	// GracefulExit calls this.
CreateInterfaceFn	CmdLib_GetFileSystemFactory();


#ifdef _WIN32
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA

#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4305)     // truncate from double to float
#endif


// the dec offsetof macro doesnt work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)


// set these before calling CheckParm
extern int myargc;
extern char **myargv;

void Q_getwd (char *out);

int Q_filelength (FileHandle_t f);
int	FileTime (char *path);

void	Q_mkdir (char *path);

char *ExpandArg (char *path);	// expand relative to CWD
char *ExpandPath (char *path);	// expand relative to gamedir

char *ExpandPathAndArchive (char *path);


double I_FloatTime (void);

int		CheckParm (char *check);

FileHandle_t	SafeOpenWrite (char *filename);
FileHandle_t	SafeOpenRead (char *filename);
void			SafeRead( FileHandle_t f, void *buffer, int count);
void			SafeWrite( FileHandle_t f, void *buffer, int count);

int		LoadFile (char *filename, void **bufferptr);
void	SaveFile (char *filename, void *buffer, int count);
qboolean	FileExists (char *filename);

void 	DefaultExtension (char *path, char *extension);
void 	DefaultPath (char *path, char *basepath);
void 	StripFilename (char *path);
void 	StripExtension (char *path);

bool	StripLastDir( char *dirName );

void 	ExtractFilePath (char *path, char *dest);
void 	ExtractFileBase ( const char *path, char *dest, int destSize );
void	ExtractFileExtension ( const char *path, char *dest, int destSize );

int 	ParseNum (char *str);

// Do a printf in the specified color.
#define CP_ERROR	stderr, 1, 0, 0, 1		// default colors..
#define CP_WARNING	stderr, 1, 1, 0, 1		
#define CP_STARTUP	stdout, 0, 1, 1, 1		
#define CP_NOTIFY	stdout, 1, 1, 1, 1
void ColorPrintf( FILE *pFile, bool red, bool green, bool blue, bool intensity, char const *pFormat, ... );

// Initialize spew output.
void InstallSpewFunction();

// This registers an extra callback for spew output.
typedef void (*SpewHookFn)( const char * );
void InstallExtraSpewHook( SpewHookFn pFn );

// Install allocation hooks so we error out if an allocation can't happen.
void InstallAllocationFunctions();

// This shuts down mgrs that use threads gracefully. If you just call exit(), the threads can
// get in a state where you can't tell if they are shutdown or not, and it can stall forever.
typedef void (*CleanupFn)();
void CmdLib_AtCleanup( CleanupFn pFn );	// register a callback when Cleanup() is called.
void CmdLib_Cleanup();
void CmdLib_Exit( int exitCode );	// Use this to cleanup and call exit().

// Append all spew output to the specified file.
void SetSpewFunctionLogFile( char const *pFilename );

char *COM_Parse (char *data);
void COM_FixSlashes( char *pname );

extern	char		com_token[1024];
extern	qboolean	com_eof;

char *copystring(char *s);


void CRC_Init(unsigned short *crcvalue);
void CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short CRC_Value(unsigned short crcvalue);

void	CreatePath (char *path);
void	QCopyFile (char *from, char *to);
void SetExtension( char *pOut, const char *pFileName, const char *pExt );

extern	qboolean		archive;
extern	char			archivedir[1024];


extern	qboolean verbose;

void qprintf( char *format, ... );

void ExpandWildcards (int *argc, char ***argv);

// merge dir/file into a full path, adding a / if necessary
void MakePath( char *dest, int destLen, const char *dirName, const char *fileName );

void CmdLib_AddBasePath( const char *pBasePath );
bool CmdLib_HasBasePath( const char *pFileName, int &pathLength );
int CmdLib_GetNumBasePaths( void );
const char *CmdLib_GetBasePath( int i );

// This is the the path of the initial source file
extern char		qdir[1024];

// This is the base engine + mod-specific game dir (e.g. "d:/tf2/mytfmod/")
extern char		gamedir[1024];	

// This is the base engine directory (c:\hl2).
extern char		basedir[1024];

// This is the base engine + base game directory (e.g. "e:/hl2/hl2/", or "d:/tf2/tf2/" )
extern char		basegamedir[1024];

// this is the override directory
extern char qproject[1024];

extern bool g_bStopOnExit;

// for compression routines
typedef struct
{
	byte	*data;
	int		count;
} cblock_t;


#endif // CMDLIB_H
