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
#include "stdafx.h"
#include <stdio.h>
#define INITGUID  // this is needed only in this file to initialize the d3d guids
#include <ddraw.h>
#include "ilauncherdirectdraw.h"
#include "igame.h"
#include "ivideomode.h"
#include "vmodes.h"
#include "modes.h"
#include "vstdlib/icommandline.h"

//-----------------------------------------------------------------------------
// Purpose: Video mode enumeration only.
// FIXME:  Should retrieve mode list from the material system?
//-----------------------------------------------------------------------------
class CLauncherDirectDraw : public ILauncherDirectDraw
{
public:
	typedef struct
	{
		int		bpp;
		bool	checkaspect;
	} DDENUMPARAMS;

						CLauncherDirectDraw( void );
	virtual				~CLauncherDirectDraw( void );

	bool				Init( void );
	void				Shutdown( void );

	void				EnumDisplayModes( int bpp );

private:
	// DirectDraw object
	LPDIRECTDRAW		m_pDirectDraw;        
};

static CLauncherDirectDraw g_LauncherDirectDraw;
ILauncherDirectDraw *directdraw = ( ILauncherDirectDraw * )&g_LauncherDirectDraw;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CLauncherDirectDraw::CLauncherDirectDraw( void )
{
	m_pDirectDraw			= (LPDIRECTDRAW)0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CLauncherDirectDraw::~CLauncherDirectDraw( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CLauncherDirectDraw::Init( void )
{
	assert( !m_pDirectDraw );

	// Create the main DirectDraw object
	if ( DirectDrawCreate( NULL, &m_pDirectDraw, NULL ) != DD_OK )
	{
		return false;
	}

	m_pDirectDraw->SetCooperativeLevel( NULL, DDSCL_NORMAL );

	if ( !videomode->IsWindowedMode() )
	{
		m_pDirectDraw->SetCooperativeLevel
		( 
			game->GetMainWindow(), 
			DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
		);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLauncherDirectDraw::Shutdown(void)
 {
	if ( !m_pDirectDraw )
		return;

	m_pDirectDraw->FlipToGDISurface();
	m_pDirectDraw->Release();
	m_pDirectDraw = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pdds - 
//			lParam - 
// Output : static HRESULT CALLBACK
//-----------------------------------------------------------------------------
static HRESULT CALLBACK ModeCallback( LPDDSURFACEDESC pdds, LPVOID lParam )
{
    int		width			= pdds->dwWidth;
    int		height			= pdds->dwHeight;
    int		bpp				= pdds->ddpfPixelFormat.dwRGBBitCount;
	float	aspect			= (float)height/(float)width;

	CLauncherDirectDraw::DDENUMPARAMS *params = ( CLauncherDirectDraw::DDENUMPARAMS * )lParam;
	assert( params );

	int		requestedbpp	= params->bpp;
	bool	checkaspect		= params->checkaspect;

	// Promote 15 to 16 bit
	if ( bpp == 15 )
	{
		bpp = 16;
	}

	// The aspect ratio and bpp limits are self-imposed while the height limit is
	//  imposed by the engine
	if ( ( ( aspect == 0.75 ) || !checkaspect ) && 
//		 ( width >= 640 ) && ( height >= 480 ) &&
		 ( requestedbpp == bpp ) )
	{
		// Try to add a new mode
		vmode_t *mode = videomode->AddMode();
		if ( !mode )
		{
			// List full, stop now
			return TRUE;
		}
		
		mode->width			= width;;
		mode->height		= height;
		mode->bpp			= bpp;
		// Set description
		sprintf( mode->modedesc, "%d x %d", mode->width, mode->height );
	}

	// Keep going
    return S_FALSE;
}

//-----------------------------------------------------------------------------
// Purpose: Enumerate available video modes
// Input  : bpp - 
//-----------------------------------------------------------------------------
void CLauncherDirectDraw::EnumDisplayModes( int bpp )
{
	static DDENUMPARAMS params;
	params.bpp = bpp;
	if ( CommandLine()->CheckParm( "-anyaspect" ) )
	{
		params.checkaspect = false;
	}
	else 
	{
		params.checkaspect = true;
	}

	m_pDirectDraw->EnumDisplayModes( 0, NULL, (void *)&params, ModeCallback );
}
