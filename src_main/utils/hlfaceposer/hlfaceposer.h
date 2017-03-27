//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( HLFACEPOSER_H )
#define HLFACEPOSER_H
#ifdef _WIN32
#pragma once
#endif

#include <ctype.h>

#define CONSOLE_R 82
#define CONSOLE_G 173
#define CONSOLE_B 216

#define ERROR_R 255
#define ERROR_G 102
#define ERROR_B	0

#define FILE_R 0
#define FILE_G 63
#define FILE_B 200

#define MAX_FP_MODELS 16

#define SCRUBBER_HANDLE_WIDTH		40
#define SCRUBBER_HANDLE_HEIGHT		10

bool FacePoser_GetOverridesShowing( void );
char *FacePoser_MakeWindowsSlashes( char *pname );
char *va( const char *fmt, ... );
const char *GetGameDirectory( void );
void Con_Printf( const char *fmt, ... );
void Con_Overprintf( const char *fmt, ... );
void Con_ColorPrintf( int r, int g, int b, const char *fmt, ... );

bool FPFullpathFileExists( const char *filename );
void MakeFileWriteable( const char *filename );
void FPCopyFile( const char *source, const char *dest );
class mxWindow;
void FacePoser_MakeToolWindow( mxWindow *w, bool smallcaption );
void FacePoser_LoadWindowPositions( char const *name, bool& visible, int& x, int& y, int& w, int& h, bool& locked, bool& zoomed );
void FacePoser_SaveWindowPositions( char const *name, bool visible, int x, int y, int w, int h, bool locked, bool zoomed );
void FacePoser_AddWindowStyle( mxWindow *w, int addbits );
void FacePoser_AddWindowExStyle( mxWindow *w, int addbits );
void FacePoser_RemoveWindowStyle( mxWindow *w, int removebits );
void FacePoser_RemoveWindowExStyle( mxWindow *w, int removebits );
bool FacePoser_HasWindowStyle( mxWindow *w, int bits );
bool FacePoser_HasWindowExStyle( mxWindow *w, int bits );

void FacePoser_EnsurePhonemesLoaded( void );

int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSize);
int ConvertUnicodeToANSI(const wchar_t *unicode, char *ansi, int ansiBufferSize);

float FacePoser_SnapTime( float t );
char const *FacePoser_DescribeSnappedTime( float t );
int FacePoser_GetSceneFPS( void );
bool FacePoser_IsSnapping( void );

char const *FacePoser_TranslateSoundName( char const *soundname );

extern class IFileSystem *filesystem;
extern class ISceneTokenProcessor *tokenprocessor;

#endif // HLFACEPOSER_H
