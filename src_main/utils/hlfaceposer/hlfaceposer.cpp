#include "cbase.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "cmdlib.h"
#include "mx/mx.h"
#include "mxStatusWindow.h"
#include "FileSystem.h"
#include "StudioModel.h"
#include "ControlPanel.h"
#include "MDLViewer.h"
#include "mxExpressionTray.H"
#include "viewersettings.h"
#include "vstdlib/strtools.h"
#include "faceposer_models.h"
#include "expressions.h"
#include "choreoview.h"
#include "choreoscene.h"
#include "vstdlib/random.h"
#include "SoundEmitterSystemBase.h"

//-----------------------------------------------------------------------------
// Purpose: Takes a full path and determines if the file exists on the disk
// Input  : *filename - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool FPFullpathFileExists( const char *filename )
{
	// Should be a full path
	Assert( strchr( filename, ':' ) );

	struct _stat buf;
	int result = _stat( filename, &buf );
	if ( result != -1 )
		return true;

	return false;
}

// Utility functions mostly
char *FacePoser_MakeWindowsSlashes( char *pname )
{
	static char returnString[ 4096 ];
	strcpy( returnString, pname );
	pname = returnString;

	while ( *pname ) {
		if ( *pname == '/' )
			*pname = '\\';
		pname++;
	}

	return returnString;
}

const char *GetGameDirectory( void )
{
	static char gamedir[ 256 ];

	strcpy( gamedir, basegamedir );
	int len = strlen( gamedir );
	if ( len > 0 )
	{
		// Strip path separator
		if ( gamedir[ len - 1 ] == '/' ||
			 gamedir[ len - 1 ] == '\\' )
		{
			gamedir[ len - 1 ] = 0;
		}
	}

	return gamedir;
}

char *va( const char *fmt, ... )
{
	va_list args;
	static char output[4][1024];
	static int outbuffer = 0;

	outbuffer++;
	va_start( args, fmt );
	vprintf( fmt, args );
	vsprintf( output[ outbuffer & 3 ], fmt, args );
	return output[ outbuffer & 3 ];
}

void Con_Overprintf( const char *fmt, ... )
{
	va_list args;
	static char output[1024];

	va_start( args, fmt );
	vprintf( fmt, args );
	vsprintf( output, fmt, args );

	if ( !g_pStatusWindow )
	{
		return;
	}

	g_pStatusWindow->StatusPrint( CONSOLE_R, CONSOLE_G, CONSOLE_B, true, output );
}

void Con_Printf( const char *fmt, ... )
{
	va_list args;
	static char output[1024];

	va_start( args, fmt );
	vprintf( fmt, args );
	vsprintf( output, fmt, args );

	if ( !g_pStatusWindow )
	{
		return;
	}

	g_pStatusWindow->StatusPrint( CONSOLE_R, CONSOLE_G, CONSOLE_B, false, output );
}

void Con_ColorPrintf( int r, int g, int b, const char *fmt, ... )
{
	va_list args;
	static char output[1024];

	va_start( args, fmt );
	vprintf( fmt, args );
	vsprintf( output, fmt, args );

	if ( !g_pStatusWindow )
	{
		return;
	}

	g_pStatusWindow->StatusPrint( r, g, b, false, output );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void MakeFileWriteable( const char *filename )
{
	Assert( filesystem );
	char fullpath[ 512 ];
	if (filesystem->GetLocalPath( filename, fullpath ))
	{
		COM_FixSlashes( fullpath );
		SetFileAttributes( fullpath, FILE_ATTRIBUTE_NORMAL );
	}
}

void FPCopyFile( const char *source, const char *dest )
{
	Assert( filesystem );
	char fullpaths[ 512 ];
	char fullpathd[ 512 ];

	filesystem->GetLocalPath( source, fullpaths );
	//filesystem->GetLocalPath( dest, fullpathd );
	sprintf( fullpathd, "%s/%s", GetGameDirectory(), dest );

	COM_FixSlashes( fullpaths );
	COM_FixSlashes( fullpathd );

	CopyFile( fullpaths, fullpathd, FALSE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool FacePoser_GetOverridesShowing( void ) 
{
	if ( g_pExpressionTrayTool )
	{
		return g_pExpressionTrayTool->GetOverridesShowing();
	}
	return false;
}

bool FacePoser_HasWindowStyle( mxWindow *w, int bits )
{
	HWND wnd = (HWND)w->getHandle();
	DWORD style = GetWindowLong( wnd, GWL_STYLE );
	return ( style & bits ) ? true : false;
}

bool FacePoser_HasWindowExStyle( mxWindow *w, int bits )
{
	HWND wnd = (HWND)w->getHandle();
	DWORD style = GetWindowLong( wnd, GWL_EXSTYLE );
	return ( style & bits ) ? true : false;
}

void FacePoser_AddWindowStyle( mxWindow *w, int addbits )
{
	HWND wnd = (HWND)w->getHandle();
	DWORD style = GetWindowLong( wnd, GWL_STYLE );
	style |= addbits;
	SetWindowLong( wnd, GWL_STYLE, style );
}

void FacePoser_AddWindowExStyle( mxWindow *w, int addbits )
{
	HWND wnd = (HWND)w->getHandle();
	DWORD style = GetWindowLong( wnd, GWL_EXSTYLE );
	style |= addbits;
	SetWindowLong( wnd, GWL_EXSTYLE, style );
}

void FacePoser_RemoveWindowStyle( mxWindow *w, int removebits )
{
	HWND wnd = (HWND)w->getHandle();
	DWORD style = GetWindowLong( wnd, GWL_STYLE );
	style &= ~removebits;
	SetWindowLong( wnd, GWL_STYLE, style );
}

void FacePoser_RemoveWindowExStyle( mxWindow *w, int removebits )
{
	HWND wnd = (HWND)w->getHandle();
	DWORD style = GetWindowLong( wnd, GWL_EXSTYLE );
	style &= ~removebits;
	SetWindowLong( wnd, GWL_EXSTYLE, style );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *w - 
//-----------------------------------------------------------------------------
void FacePoser_MakeToolWindow( mxWindow *w, bool smallcaption )
{
	FacePoser_AddWindowStyle( w, WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );
	if ( smallcaption )
	{
		FacePoser_AddWindowExStyle( w, WS_EX_OVERLAPPEDWINDOW );
		FacePoser_AddWindowExStyle( w, WS_EX_TOOLWINDOW );
	}
}

bool LoadViewerSettingsInt( char const *keyname, int *value );
bool SaveViewerSettingsInt ( const char *keyname, int value );

void FacePoser_LoadWindowPositions( char const *name, bool& visible, int& x, int& y, int& w, int& h, bool& locked, bool& zoomed )
{
	char subkey[ 512 ];
	int v;

	Q_snprintf( subkey, sizeof( subkey ), "%s - visible", name );
	LoadViewerSettingsInt( subkey, &v );
	visible = v ? true : false;
	
	Q_snprintf( subkey, sizeof( subkey ), "%s - locked", name );
	LoadViewerSettingsInt( subkey, &v );
	locked = v ? true : false;

	Q_snprintf( subkey, sizeof( subkey ), "%s - zoomed", name );
	LoadViewerSettingsInt( subkey, &v );
	zoomed = v ? true : false;

	Q_snprintf( subkey, sizeof( subkey ), "%s - x", name );
	LoadViewerSettingsInt( subkey, &x );
	Q_snprintf( subkey, sizeof( subkey ), "%s - y", name );
	LoadViewerSettingsInt( subkey, &y );
	Q_snprintf( subkey, sizeof( subkey ), "%s - width", name );
	LoadViewerSettingsInt( subkey, &w );
	Q_snprintf( subkey, sizeof( subkey ), "%s - height", name );
	LoadViewerSettingsInt( subkey, &h );
}

void FacePoser_SaveWindowPositions( char const *name, bool visible, int x, int y, int w, int h, bool locked, bool zoomed )
{
	char subkey[ 512 ];
	Q_snprintf( subkey, sizeof( subkey ), "%s - visible", name );
	SaveViewerSettingsInt( subkey, visible );
	Q_snprintf( subkey, sizeof( subkey ), "%s - locked", name );
	SaveViewerSettingsInt( subkey, locked );
	Q_snprintf( subkey, sizeof( subkey ), "%s - x", name );
	SaveViewerSettingsInt( subkey, x );
	Q_snprintf( subkey, sizeof( subkey ), "%s - y", name );
	SaveViewerSettingsInt( subkey, y );
	Q_snprintf( subkey, sizeof( subkey ), "%s - width", name );
	SaveViewerSettingsInt( subkey, w );
	Q_snprintf( subkey, sizeof( subkey ), "%s - height", name );
	SaveViewerSettingsInt( subkey, h );
	Q_snprintf( subkey, sizeof( subkey ), "%s - zoomed", name );
	SaveViewerSettingsInt( subkey, zoomed );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FacePoser_EnsurePhonemesLoaded( void )
{
	if ( !expressions->FindClass( "phonemes" ) )
	{
		expressions->LoadClass( va( "%s/expressions/phonemes.txt", GetGameDirectory() ) );
		CExpClass *cl = expressions->FindClass( "phonemes" );
		if ( !cl )
		{
			Con_Printf( "FacePoser_EnsurePhonemesLoaded:  phonemes.txt missing!!!\n" );
		}
	}

	if ( !expressions->FindClass( "phonemes_strong" ) )
	{
		expressions->LoadClass( va( "%s/expressions/phonemes_strong.txt", GetGameDirectory() ) );
	}
	if ( !expressions->FindClass( "phonemes_weak" ) )
	{
		expressions->LoadClass( va( "%s/expressions/phonemes_weak.txt", GetGameDirectory() ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: converts an english string to unicode
//-----------------------------------------------------------------------------
int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSize)
{
	return ::MultiByteToWideChar(CP_ACP, 0, ansi, -1, unicode, unicodeBufferSize);
}

//-----------------------------------------------------------------------------
// Purpose: converts an unicode string to an english string
//-----------------------------------------------------------------------------
int ConvertUnicodeToANSI(const wchar_t *unicode, char *ansi, int ansiBufferSize)
{
	return ::WideCharToMultiByte(CP_ACP, 0, unicode, -1, ansi, ansiBufferSize, NULL, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: If FPS is set and "using grid", snap to proper fractional time value
// Input  : t - 
// Output : float
//-----------------------------------------------------------------------------
float FacePoser_SnapTime( float t )
{
	if ( !g_pChoreoView )
		return t;

	CChoreoScene *scene = g_pChoreoView->GetScene();
	if ( !scene )
		return t;

	return scene->SnapTime( t );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : char const
//-----------------------------------------------------------------------------
char const *FacePoser_DescribeSnappedTime( float t )
{
	static char desc[ 128 ];
	Q_snprintf( desc, sizeof( desc ), "%.3f", t );

	if ( !g_pChoreoView )
		return desc;

	CChoreoScene *scene = g_pChoreoView->GetScene();
	if ( !scene )
		return desc;

	t = scene->SnapTime( t );

	int fps = scene->GetSceneFPS();

	int ipart = (int)t;
	int fracpart = (int)( ( t - (float)ipart ) * (float)fps + 0.5f );

	int frame = ipart * fps + fracpart;

	if ( fracpart == 0 )
	{
		Q_snprintf( desc, sizeof( desc ), "frame %i (time %i s.)", frame, ipart );
	}
	else
	{
		Q_snprintf( desc, sizeof( desc ), "frame %i (time %i + %i/%i s.)", 
			frame, ipart,fracpart, fps );
	}

	return desc;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int FacePoser_GetSceneFPS( void )
{
	if ( !g_pChoreoView )
		return 1000;

	CChoreoScene *scene = g_pChoreoView->GetScene();
	if ( !scene )
		return 1000;

	return scene->GetSceneFPS();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool FacePoser_IsSnapping( void )
{
	if ( !g_pChoreoView )
		return false;

	CChoreoScene *scene = g_pChoreoView->GetScene();
	if ( !scene )
		return false;

	return scene->IsUsingFrameSnap();
}

char const *FacePoser_TranslateSoundName( char const *soundname )
{
	if ( Q_stristr( soundname, ".wav" ) )
		return soundname;

	return soundemitter->GetWavFileForSound( soundname );
}

static CUniformRandomStream g_Random;
IUniformRandomStream *random = &g_Random;

static CSoundEmitterSystemBase g_SoundEmitter;
CSoundEmitterSystemBase *soundemitter = &g_SoundEmitter;
