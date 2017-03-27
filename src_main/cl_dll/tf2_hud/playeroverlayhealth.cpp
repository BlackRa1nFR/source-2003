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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "playeroverlayhealth.h"
#include "playeroverlay.h"
#include "CommanderOverlay.h"
#include "hud_commander_statuspanel.h"
//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CHudPlayerOverlayHealth::CHudPlayerOverlayHealth( CHudPlayerOverlay *baseOverlay )
: BaseClass( NULL, "CHudPlayerOverlayHealth" )
{
	m_pBaseOverlay = baseOverlay;

	SetHealth( 0 );

	SetPaintBackgroundEnabled( false );
	// Send mouse inputs (but not cursorenter/exit for now) up to parent
	SetReflectMouse( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CHudPlayerOverlayHealth::~CHudPlayerOverlayHealth( void )
{
}

//-----------------------------------------------------------------------------
// Parse values from the file
//-----------------------------------------------------------------------------

bool CHudPlayerOverlayHealth::Init( KeyValues* pInitData )
{
	if (!pInitData)
		return false;

	if (!ParseRGBA(pInitData, "fgcolor", m_fgColor ))
		return false;

	if (!ParseRGBA(pInitData, "bgcolor", m_bgColor ))
		return false;

	int x, y, w, h;
	if (!ParseRect(pInitData, "position", x, y, w, h ))
		return false;
	SetPos( x, y );
	SetSize( w, h );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : health - 
//-----------------------------------------------------------------------------

void CHudPlayerOverlayHealth::SetHealth( float health )
{
	m_Health = health;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPlayerOverlayHealth::Paint( void )
{
	int w, h;

	GetSize( w, h );
	
	m_pBaseOverlay->SetColorLevel( this, m_fgColor, m_bgColor );

	// Use a color related to health value....
	vgui::surface()->DrawSetColor( 0, 255, 0, 255 * m_pBaseOverlay->GetAlphaFrac() );

	int drawwidth;

	float frac = m_Health;
	frac = min( 1.0, m_Health );
	frac = max( 0.0, m_Health );

	drawwidth = frac * w;

	vgui::surface()->DrawFilledRect( 0, 0, drawwidth, h/2 );

	// This is the hurt part
	if (w != drawwidth)
	{
		vgui::surface()->DrawSetColor( 255, 64, 64, 255 * m_pBaseOverlay->GetAlphaFrac() );
		vgui::surface()->DrawFilledRect( drawwidth, 0, w, h/2 );
	}
}

void CHudPlayerOverlayHealth::OnCursorEntered()
{
	if ( m_pBaseOverlay->GetMouseOverText() )
	{
		StatusPrint( TYPE_HINT, "%s", m_pBaseOverlay->GetMouseOverText() );
	}
}

void CHudPlayerOverlayHealth::OnCursorExited()
{
	if ( m_pBaseOverlay->GetMouseOverText() )
	{
		StatusClear();
	}
}