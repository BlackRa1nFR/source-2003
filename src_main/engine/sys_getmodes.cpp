//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
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

#include "winquake.h"
#include "gl_matsysiface.h"
#include "vmodes.h"
#include "modes.h"
#include "ivideomode.h"
#include "igame.h"
#include "iengine.h"
#include "engine_launcher_api.h"
#include "dll_state.h"
#include "iregistry.h"
#include "common.h"
#include "vstdlib/icommandline.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void VID_UpdateWindowVars(void *prc, int x, int y);

extern viddef_t	vid;  		// global video state

//-----------------------------------------------------------------------------
// Purpose: Functionality shared by all video modes
//-----------------------------------------------------------------------------
class CVideoMode_Common : public IVideoMode
{
public:
						CVideoMode_Common( void );
	virtual 			~CVideoMode_Common( void );

public:
	virtual bool		Init( void *pvInstance );
	virtual void		Shutdown( void );

	virtual vmode_t		*AddMode( void );
	virtual vmode_t		*GetMode( int num );
	virtual int			GetModeCount( void );

	virtual bool		IsWindowedMode( void ) const;

	virtual bool		GetInitialized( void ) const;
	virtual void		SetInitialized( bool init );

	virtual void		UpdateWindowPosition( void );

	virtual void		FlipScreen();

	virtual void		RestoreVideo( void );
	virtual void		ReleaseVideo( void );

private:
	void				CenterEngineWindow(HWND hWndCenter, int width, int height);
	void				DrawLoadingText( HWND window, RECT *rect );

	virtual void		ReleaseFullScreen( void );
	virtual void		ChangeDisplaySettingsToFullscreen( void );

private:
	enum
	{
		MAX_MODE_LIST =	512
	};

	// Master mode list
	vmode_t				m_rgModeList[ MAX_MODE_LIST ];
	vmode_t				m_safeMode;
	int					m_nNumModes;
	int					m_nGameMode;
	bool				m_bInitialized;

protected:
	vmode_t				*GetCurrentMode( void );
	void				AdjustWindowForMode( void );

	bool				m_bWindowed;

};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CVideoMode_Common::CVideoMode_Common( void )
{
	m_nNumModes			= 0;
	m_nGameMode			= 0;
	m_bWindowed			= false;
	m_bInitialized		= false;
	m_safeMode.width	= 640;
	m_safeMode.height	= 480;
	m_safeMode.bpp		= 16;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CVideoMode_Common::~CVideoMode_Common( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVideoMode_Common::GetInitialized( void ) const
{
	return m_bInitialized;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : init - 
//-----------------------------------------------------------------------------
void CVideoMode_Common::SetInitialized( bool init )
{
	m_bInitialized = init;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVideoMode_Common::IsWindowedMode( void ) const
{
	return m_bWindowed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : vmode_t
//-----------------------------------------------------------------------------
vmode_t *CVideoMode_Common::AddMode( void )
{
	if ( m_nNumModes >= MAX_MODE_LIST )
	{
		return NULL;
	}

	return &m_rgModeList[ m_nNumModes++ ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : num - 
//-----------------------------------------------------------------------------
vmode_t	*CVideoMode_Common::GetMode( int num )
{
	if ( num >= m_nNumModes || num < 0 )
	{
		return &m_safeMode;
	}

	return &m_rgModeList[ num ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CVideoMode_Common::GetModeCount( void )
{
	return m_nNumModes;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : modenum - 
// Output : vmode_t
//-----------------------------------------------------------------------------
vmode_t *CVideoMode_Common::GetCurrentMode( void )
{
	return GetMode( m_nGameMode );
}

//-----------------------------------------------------------------------------
// Purpose: Compares video modes so we can sort the list
// Input  : *arg1 - 
//			*arg2 - 
// Output : static int
//-----------------------------------------------------------------------------
static int VideoModeCompare( const void *arg1, const void *arg2 )
{
	vmode_t *m1, *m2;

	m1 = (vmode_t *)arg1;
	m2 = (vmode_t *)arg2;

	if ( m1->width < m2->width )
	{
		return -1;
	}

	if ( m1->width == m2->width )
	{
		if ( m1->height < m2->height )
		{
			return -1;
		}

		if ( m1->height > m2->height )
		{
			return 1;
		}

		return 0;
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CVideoMode_Common::Init( void *pvInstance )
{
	bool bret = true;

	int x = 640;
	int y = 480;

	x = registry->ReadInt( "ScreenWidth", 640 );
	y = registry->ReadInt( "ScreenHeight", 480 );

	// Default to 16 BPP Unless we're asked to change
	int		bitsperpixel = 16;

	bitsperpixel = registry->ReadInt( "ScreenBPP", 16 );

	if ( CommandLine()->FindParm ( "-16bpp") )
	{
		bitsperpixel = 16;
	}

	if ( CommandLine()->FindParm( "-24bpp" ) )
	{
		bitsperpixel = 24;
	}

	if ( CommandLine()->FindParm( "-32bpp" ) )
	{
		bitsperpixel = 32;
	}

	bool allowsmallmodes = false;
	if ( CommandLine()->FindParm( "-small" ) )
	{
		allowsmallmodes = true;
	}

	int adapter = 0; // materialSystemInterface->GetDisplayAdapterCount();

	m_nNumModes = materialSystemInterface->GetModeCount( adapter );
	if ( m_nNumModes > MAX_MODE_LIST )
	{
		Assert( 0 );
		m_nNumModes = MAX_MODE_LIST;
	}
	int lastW = -1;
	int lastH = -1;

	int slot = 0;

	for ( int i = 0; i < m_nNumModes; i++ )
	{
		MaterialVideoMode_t info;

		materialSystemInterface->GetModeInfo( adapter, i, info );

		if ( info.m_Width == lastW && info.m_Height == lastH )
			continue;

		if ( info.m_Width < 640 )
		{
			if ( !allowsmallmodes )
				continue;
		}

		lastW = info.m_Width;
		lastH = info.m_Height;

		m_rgModeList[ slot ].width = info.m_Width;
		m_rgModeList[ slot ].height = info.m_Height;
		m_rgModeList[ slot ].bpp = bitsperpixel;

		slot++;
	}

	m_nNumModes = slot;

	// Try and match mode based on w,h
	int iOK = -1;

	// Is the user trying to force a mode
	const char *pArg;
	pArg = CommandLine()->ParmValue( "-width" );
	if ( pArg )
	{
		x = atoi( pArg );
	}
	
	pArg = CommandLine()->ParmValue( "-w" );
	if ( pArg )
	{
		x = atoi( pArg );
	}

	pArg = CommandLine()->ParmValue( "-height" );
	if ( pArg )
	{
		y = atoi( pArg );
	}
	
	pArg = CommandLine()->ParmValue( "-h" );
	if ( pArg )
	{
		y = atoi( pArg );
	}

	Assert( m_nNumModes + 1 < MAX_MODE_LIST );
	// This allows you to have a window of any size.  Requires you to set both width and height for the window.
	if( m_bWindowed && 
		( m_nNumModes + 1 < MAX_MODE_LIST ) &&
		( CommandLine()->FindParm ( "-width" ) || CommandLine()->FindParm ( "-w" ) ) &&
		( CommandLine()->FindParm ( "-height" ) || CommandLine()->FindParm ( "-h" ) ) )
	{
		m_rgModeList[ m_nNumModes ].width = x;
		m_rgModeList[ m_nNumModes ].height = y;
		m_rgModeList[ m_nNumModes ].bpp = bitsperpixel;
		m_nNumModes++;
	}
	
	if ( m_nNumModes >= 2 )
	{
		// Sort modes
		qsort( (void *)&m_rgModeList[0], m_nNumModes, sizeof(vmode_t), VideoModeCompare );
	}

	for ( i = 0; i < m_nNumModes; i++)
	{
		// Match width first
		if ( m_rgModeList[i].width != x )
			continue;
		
		iOK = i;

		if ( m_rgModeList[i].height != y )
			continue;

		// Found a decent match
		break;
	}

	// No match, use mode 0
	if ( i >= m_nNumModes )
	{
		if ( iOK != -1 )
		{
			i = iOK;
		}
		else
		{
			i = 0;
		}
	}

	m_nGameMode = i;

	registry->WriteInt( "ScreenWidth", m_rgModeList[ i ].width );
	registry->WriteInt( "ScreenHeight", m_rgModeList[ i ].height );
	registry->WriteInt( "ScreenBPP",  m_rgModeList[ i ].bpp );

	// Fill in vid structure for the mode
	vid.bits		= m_rgModeList[ i ].bpp;
	vid.height		= m_rgModeList[ i ].height;
	vid.width		= m_rgModeList[ i ].width;

	bret = game->CreateGameWindow();
	
	AdjustWindowForMode();

	return bret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVideoMode_Common::UpdateWindowPosition( void )
{
	RECT	window_rect;
	int		x, y, w, h;

	// Get the window from the game ( right place for it? )
	game->GetWindowRect( &x, &y, &w, &h );

	window_rect.left = x;
	window_rect.right = x + w;
	window_rect.top = y;
	window_rect.bottom = y + h;

	int cx =  x + w / 2;
	int cy =  y + h / 2;

	// Tell the engine
	VID_UpdateWindowVars( &window_rect, cx, cy );
}

void CVideoMode_Common::ChangeDisplaySettingsToFullscreen( void )
{
}

void CVideoMode_Common::ReleaseFullScreen( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mode - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CVideoMode_Common::AdjustWindowForMode( void )
{
	vmode_t *mode = GetCurrentMode();
	assert( mode );

	RECT WindowRect;

	// Use Change Display Settings to go full screen
	ChangeDisplaySettingsToFullscreen();

	WindowRect.top		= 0;
	WindowRect.left		= 0;
	WindowRect.right	= mode->width;
	WindowRect.bottom	= mode->height;
	
	// Get window style
	DWORD style = GetWindowLong( game->GetMainWindow(), GWL_STYLE );
	// Compute rect needed for that size client area based on window style
	AdjustWindowRectEx(&WindowRect, style, FALSE, 0);
	// Move the window to 0, 0 and the new true size
	SetWindowPos (game->GetMainWindow(), NULL, 0, 0, WindowRect.right - WindowRect.left,
				 WindowRect.bottom - WindowRect.top, SWP_NOZORDER | SWP_NOREDRAW );
	// Now center
	CenterEngineWindow(game->GetMainWindow(),
				 WindowRect.right - WindowRect.left,
				 WindowRect.bottom - WindowRect.top );

	game->SetWindowSize( mode->width, mode->height );

	// Draw something on the window while loading
	DrawLoadingText( game->GetMainWindow(), &WindowRect );

	// Make sure we have updated window information
	UpdateWindowPosition();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVideoMode_Common::Shutdown( void )
{
	ReleaseFullScreen();

	if ( !GetInitialized() )
		return;

	SetInitialized( false );

	m_nGameMode = 0;

	// REMOVE INTERFACE
	videomode = NULL;
	delete this;
}

void CVideoMode_Common::FlipScreen( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hWndCenter - 
//			width - 
//			height - 
// Output : static void
//-----------------------------------------------------------------------------
void CVideoMode_Common::CenterEngineWindow(HWND hWndCenter, int width, int height)
{
    int     CenterX, CenterY;

	CenterX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	CenterY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	CenterX = (CenterX < 0) ? 0: CenterX;
	CenterY = (CenterY < 0) ? 0: CenterY;

	game->SetWindowXY( CenterX, CenterY );

	SetWindowPos (hWndCenter, NULL, CenterX, CenterY, 0, 0,
				  SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_DRAWFRAME);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : window - 
//			*rect - 
// Output : static void
//-----------------------------------------------------------------------------
void CVideoMode_Common::DrawLoadingText( HWND window, RECT *rect )
{
	HDC hdc = GetDC( window );
	FillRect( hdc, rect, (HBRUSH)GetStockObject( DKGRAY_BRUSH ) );

	HFONT fnt;
		fnt = CreateFont(
		-15					             // H
		, 0   					         // W
		, 0								 // Escapement
		, 0							     // Orient
		, FW_HEAVY	 					 // Wt.  (BOLD)
		, TRUE							 // Ital.
		, 0             				 // Underl.
		, 0                 			 // SO
		, ANSI_CHARSET		    		 // Charset
		, OUT_TT_PRECIS					 // Out prec.
		, CLIP_DEFAULT_PRECIS			 // Clip prec.
		, PROOF_QUALITY   			     // Qual.
		, VARIABLE_PITCH | FF_DONTCARE   // Pitch and Fam.
		, "Arial" );

	COLORREF clr = RGB( 200, 200, 200 );

	SetTextColor( hdc, clr );
	SetBkMode(hdc, TRANSPARENT);

	HFONT old = (HFONT)::SelectObject(hdc, fnt );
	DrawText( hdc, "Loading..." , -1, rect, 
		DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE );
	
	SelectObject( hdc, old);
	DeleteObject( fnt );
	ReleaseDC( window, hdc );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVideoMode_Common::RestoreVideo( void )
{
}

void CVideoMode_Common::ReleaseVideo( void )
{
}

class CVideoMode_MaterialSystem: public CVideoMode_Common
{
public:
	typedef CVideoMode_Common BaseClass;
	
	CVideoMode_MaterialSystem( bool windowed );

	virtual char const	*GetName() { return "gl"; };

	virtual bool		Init( void *pvInstance );

	virtual void		ReleaseVideo( void );
	virtual void		RestoreVideo( void );

private:
	virtual void		ReleaseFullScreen( void );
	virtual void		ChangeDisplaySettingsToFullscreen( void );
};

CVideoMode_MaterialSystem::CVideoMode_MaterialSystem( bool windowed )
{
	m_bWindowed = windowed;
}

bool CVideoMode_MaterialSystem::Init( void *pvInstance )
{
	bool bret = BaseClass::Init( pvInstance );

	SetInitialized( bret );

	return bret;
}

void CVideoMode_MaterialSystem::ReleaseVideo( void )
{
	if ( IsWindowedMode() )
		return;

	ReleaseFullScreen();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVideoMode_MaterialSystem::RestoreVideo( void )
{
	if ( IsWindowedMode() )
		return;

	ShowWindow(game->GetMainWindow(), SW_SHOWNORMAL);
	AdjustWindowForMode();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVideoMode_MaterialSystem::ReleaseFullScreen( void )
{
	if ( IsWindowedMode() )
		return;

	// Hide the main window
	ChangeDisplaySettings( NULL, 0 );
	ShowWindow(game->GetMainWindow(), SW_MINIMIZE);
}

//-----------------------------------------------------------------------------
// Purpose: Use Change Display Settings to go Full Screen
// Input  : *mode - 
// Output : static void
//-----------------------------------------------------------------------------
void CVideoMode_MaterialSystem::ChangeDisplaySettingsToFullscreen( void )
{
	if ( IsWindowedMode() )
		return;

	vmode_t *mode = GetCurrentMode();
	assert( mode );
	if ( !mode )
		return;


	DEVMODE dm;
	memset(&dm, 0, sizeof(dm));

	dm.dmSize		= sizeof( dm );
	dm.dmPelsWidth  = mode->width;
	dm.dmPelsHeight = mode->height;
	dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
	dm.dmBitsPerPel = mode->bpp;

	int freq = CommandLine()->ParmValue( "-freq", -1 );
	if ( freq >= 0 )
	{
		dm.dmDisplayFrequency = freq;
		dm.dmFields |= DM_DISPLAYFREQUENCY;
	}

	ChangeDisplaySettings( &dm, CDS_FULLSCREEN );
}

IVideoMode *videomode = ( IVideoMode * )NULL;

void VideoMode_Create()
{
	// Assume full screen
	bool windowed = false;

	// Get last setting
	windowed = registry->ReadInt( "ScreenWindowed", 0 ) ? true : false;

	// Check for windowed mode command line override
	if ( CommandLine()->FindParm( "-sw" ) || 
		 CommandLine()->FindParm( "-startwindowed" ) ||
		 CommandLine()->FindParm( "-windowed" ) ||
		 CommandLine()->FindParm( "-window" ) )
	{
		windowed = true;
	}
	// Check for fullscreen override
	else if ( CommandLine()->FindParm( "-full" ) ||
		CommandLine()->FindParm( "-fullscreen" ) )
	{
		windowed = false;
	}

	// Store actual mode back out to registry
	registry->WriteInt( "ScreenWindowed", windowed ? 1 : 0 );

	videomode = new CVideoMode_MaterialSystem( windowed );

	assert( videomode );
}

void VideoMode_SwitchMode( int windowed )
{
	registry->WriteInt( "ScreenWindowed", windowed );

}

void VideoMode_SetVideoMode( int width, int height, int bpp )
{
	registry->WriteInt( "ScreenWidth", width );
	registry->WriteInt( "ScreenHeight", height );
	registry->WriteInt( "ScreenBPP", bpp );

}

void VideoMode_GetVideoModes( struct vmode_s **liststart, int *count )
{
	*count = videomode->GetModeCount();
	*liststart = videomode->GetMode( 0 );
}

void VideoMode_GetCurrentVideoMode( int *wide, int *tall, int *bpp )
{
	vmode_t *mode = videomode->GetCurrentMode();
	if ( !mode )
		return;

	if ( wide )
	{
		*wide = mode->width;
	}
	if ( tall )
	{
		*tall = mode->height;
	}
	if ( bpp )
	{
		*bpp  = mode->bpp;
	}
}

void VideoMode_GetCurrentRenderer( char *name, int namelen, int *windowed )
{
	Q_strncpy( name, videomode->GetName(), namelen );
	if ( windowed )
	{
		*windowed = videomode->IsWindowedMode() ? 1 : 0;
	}
}

void VideoMode_ChangeFullscreenMode( bool fullscreen )
{
	registry->WriteInt( "ScreenWindowed", !fullscreen ? 1 : 0 );
}

