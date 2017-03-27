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
#include "stdwin.h"
#include <stdlib.h>
#include <stdio.h>
#include "vstdlib/icommandline.h"
#include "launcher_util.h"
#include "vmodes.h"
#include "modes.h"
#include "ilauncherdirectdraw.h"
#include "ivideomode.h"
#include "igame.h"
#include "iengine.h"
#include "engine_launcher_api.h"
#include "dll_state.h"
#include "iregistry.h"
#include "tier0/vcrmode.h"


viddef_t	vid;  		// global video state

//-----------------------------------------------------------------------------
// Purpose: Handler video mode list, etc.
//-----------------------------------------------------------------------------
class CVideoMode : public IVideoMode
{
public:	
						CVideoMode( void );
	virtual 			~CVideoMode( void );

	void				Init( void );
	void				Shutdown( void );
	vmode_t				*AddMode( void );
	vmode_t				*GetCurrentMode( void );

	bool				IsWindowedMode( void ) const;
	void				ReleaseFullScreen( void ) const;

	bool				GetInitialized( void ) const;
	void				SetInitialized( bool init );

	void				UpdateWindowPosition( void );
	void				GetVid( struct viddef_s *pvid );
	bool				AdjustWindowForMode( vmode_t *mode );

	bool				GetInModeChange( void ) const;
	void				SetInModeChange( bool changing );

private:
 
private:
	enum
	{
		MAX_MODE_LIST =	32
	};

	// Master mode list
	vmode_t				m_rgModeList[ MAX_MODE_LIST ];
	int					m_nNumModes;
	int					m_nGameMode;
	bool				m_bWindowed;
	bool				m_bInitialized;
	bool				m_bInModeChange;
};

static CVideoMode g_VideoMode;
IVideoMode *videomode = ( IVideoMode * )&g_VideoMode;

// FIXME:  Remove vid.vidtype, DX8 is only supported VT
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CVideoMode::CVideoMode( void )
{
	m_nNumModes			= 0;
	m_nGameMode			= 0;
	m_bWindowed			= false;
	m_bInitialized		= false;
	m_bInModeChange		= false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CVideoMode::~CVideoMode( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVideoMode::GetInitialized( void ) const
{
	return m_bInitialized;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : init - 
//-----------------------------------------------------------------------------
void CVideoMode::SetInitialized( bool init )
{
	m_bInitialized = init;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVideoMode::IsWindowedMode( void ) const
{
	return m_bWindowed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : vmode_t
//-----------------------------------------------------------------------------
vmode_t *CVideoMode::AddMode( void )
{
	if ( m_nNumModes >= MAX_MODE_LIST )
	{
		return NULL;
	}

	return &m_rgModeList[ m_nNumModes++ ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : modenum - 
// Output : vmode_t
//-----------------------------------------------------------------------------
vmode_t *CVideoMode::GetCurrentMode( void )
{
	if ( m_nGameMode >= 0 )
	{
		return &m_rgModeList[ m_nGameMode ];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVideoMode::GetInModeChange( void ) const
{
	return m_bInModeChange;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : changing - 
//-----------------------------------------------------------------------------
void CVideoMode::SetInModeChange( bool changing )
{
	m_bInModeChange = changing;
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
void CVideoMode::Init( void )
{
	char *p;
	int x = 640;
	int y = 480;

	x = registry->ReadInt( "ScreenWidth", 640 );
	y = registry->ReadInt( "ScreenHeight", 480 );

	// Default to 32 BPP Unless we're asked to change
	int		bitsperpixel = 32;

	// Check for windowed mode
	if ( CommandLine()->CheckParm( "-sw" ) || 
		 CommandLine()->CheckParm( "-startwindowed" ) ||
		 CommandLine()->CheckParm( "-windowed" ) ||
		 CommandLine()->CheckParm( "-window" ) )
	{
		m_bWindowed = true;
	}

	if ( CommandLine()->CheckParm ( "-16bpp") )
	{
		bitsperpixel = 16;
	}

	if ( CommandLine()->CheckParm( "-24bpp" ) )
	{
		bitsperpixel = 24;
	}

	// Assume failure
	m_nNumModes = 0;

	// Populate mode list from DDraw
	if ( directdraw->Init() )
	{
		// Enumerate modes
		directdraw->EnumDisplayModes( bitsperpixel );

		// Kill DDraw right away
		directdraw->Shutdown();
	}

	if ( m_nNumModes >= 2 )
	{
		// Sort modes
		qsort( (void *)&m_rgModeList[0], m_nNumModes, sizeof(vmode_t), VideoModeCompare );
	}

	// Try and match mode based on w,h
	int iOK = -1;

	// Is the user trying to force a mode
	if ( CommandLine()->CheckParm ( "-width", &p ) && p )
	{
		x = atoi(p);
	}

	if ( CommandLine()->CheckParm ( "-w", &p ) && p )
	{
		x = atoi(p);
	}

	if ( CommandLine()->CheckParm ( "-height", &p ) && p )
	{
		y = atoi(p);
	}

	if ( CommandLine()->CheckParm ( "-h", &p ) && p )
	{
		y = atoi(p);
	}

#if 0
	if( m_bWindowed )
	{
		if( m_nNumModes < MAX_MODE_LIST )
		{
			m_rgModeList[m_nNumModes].bpp = 32;
			m_rgModeList[m_nNumModes].width = x;
			m_rgModeList[m_nNumModes].height = y;
			sprintf( m_rgModeList[m_nNumModes].modedesc, "%d x %d", x, y );
			m_nGameMode = m_nNumModes;
			m_nNumModes++;
		}
	}
	else
#endif
	{
		int i;
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
	}
	registry->WriteInt( "ScreenWidth", m_rgModeList[ m_nGameMode ].width );
	registry->WriteInt( "ScreenHeight", m_rgModeList[ m_nGameMode ].height );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVideoMode::UpdateWindowPosition( void )
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
	engine->VID_UpdateWindowVars( &window_rect, cx, cy );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pvid - 
//-----------------------------------------------------------------------------
void CVideoMode::GetVid( struct viddef_s *pvid )
{
	if ( pvid )
	{
		*pvid = vid;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVideoMode::ReleaseFullScreen( void ) const
{
	if ( IsWindowedMode() )
		return;

	// Hide the main window
	ChangeDisplaySettings( NULL, 0 );
	Sleep( 1000 );
}

//-----------------------------------------------------------------------------
// Purpose: Use Change Display Settings to go Full Screen
// Input  : *mode - 
// Output : static void
//-----------------------------------------------------------------------------
static void ChangleDisplaySettingsToFullscreen( vmode_t *mode )
{
	DEVMODE dm;
	memset(&dm, 0, sizeof(dm));

	dm.dmSize		= sizeof( dm );
	dm.dmPelsWidth  = mode->width;
	dm.dmPelsHeight = mode->height;
	dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
	dm.dmBitsPerPel = mode->bpp;

	ChangeDisplaySettings( &dm, CDS_FULLSCREEN );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hWndCenter - 
//			width - 
//			height - 
// Output : static void
//-----------------------------------------------------------------------------
static void CenterEngineWindow(HWND hWndCenter, int width, int height)
{
    int     CenterX, CenterY;

	CenterX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	CenterY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	CenterX = (CenterX < 0) ? 0: CenterX;
	CenterY = (CenterY < 0) ? 0: CenterY;

	if(g_pVCR->GetMode() != VCR_Disabled)
	{
		CenterX = CenterY = 0;
	}

	game->SetWindowXY( CenterX, CenterY );

	// In VCR modes, keep it in the upper left so mouse coordinates are always relative to the window.
	SetWindowPos (hWndCenter, NULL, CenterX, CenterY, 0, 0,
				  SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_DRAWFRAME);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hWndCenter - 
//			width - 
//			height - 
// Output : static void
//-----------------------------------------------------------------------------
static void SetEngineWindowOrigin(HWND hWndCenter, int width, int height, int x, int y)
{
	if(g_pVCR->GetMode() != VCR_Disabled)
	{
		x = y = 0;
	}

	game->SetWindowXY( x, y );

	// In VCR modes, keep it in the upper left so mouse coordinates are always relative to the window.
	SetWindowPos (hWndCenter, NULL, x, y, 0, 0,
				  SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_DRAWFRAME);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : window - 
//			*rect - 
// Output : static void
//-----------------------------------------------------------------------------
static void DrawLoadingText( HWND window, RECT *rect )
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

	int r, g, b;
	COLORREF clr;

	util->GetColor( "ENGINE_LOADING", &r, &g, &b );
	
	clr = RGB( r, g, b );
	
	SetTextColor( hdc, clr );
	SetBkMode(hdc, TRANSPARENT);

	HFONT old = (HFONT)::SelectObject(hdc, fnt );
	DrawText( hdc, util->GetString( "loading" ) , -1, rect, 
		DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE );
	
	SelectObject( hdc, old);
	DeleteObject( fnt );
	ReleaseDC( window, hdc );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mode - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVideoMode::AdjustWindowForMode( vmode_t *mode )
{
	RECT WindowRect;

	engine->Snd_ReleaseBuffer();

	// Use Change Display Settings to go full screen
	if ( !videomode->IsWindowedMode() )
	{
		ChangleDisplaySettingsToFullscreen( mode );
	}

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
	
	char *str;
	int x = -1, y = -1;
	if( CommandLine()->CheckParm( "-x", &str ) && str )
	{
		x = atoi( str );
	}
	if( CommandLine()->CheckParm( "-y", &str ) && str )
	{
		y = atoi( str );
	}
		
	if( x != -1 && y != -1 )
	{
		// Set the origin of the window
		SetEngineWindowOrigin( game->GetMainWindow(),
					 WindowRect.right - WindowRect.left,
					 WindowRect.bottom - WindowRect.top , x, y );
	}
	else
	{
		// Now center
		CenterEngineWindow(game->GetMainWindow(),
					 WindowRect.right - WindowRect.left,
					 WindowRect.bottom - WindowRect.top );
	}

	game->SetWindowSize( mode->width, mode->height );

	// Draw something on the window while loading
	DrawLoadingText( game->GetMainWindow(), &WindowRect );

	engine->Snd_AcquireBuffer();

	// Make sure we have updated window information
	UpdateWindowPosition();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVideoMode::Shutdown( void )
{
	ReleaseFullScreen();

	HWND wnd = game->GetMainWindow();
	if ( wnd )
	{
		ShowWindow( wnd, SW_HIDE );
	}

	if ( !GetInitialized() )
		return;

	videomode->SetInitialized( false );

	m_nGameMode = -1;
}
