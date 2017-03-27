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
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include <KeyValues.h>
#include "playeroverlay.h"
#include "playeroverlaysquad.h"
#include <KeyValues.h>
#include "PanelMetaClassMgr.h"
#include "hud_commander_statuspanel.h"
#include <vgui/IScheme.h>
#include <vgui/IVgui.h>

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CHudPlayerOverlaySquad::CHudPlayerOverlaySquad( CHudPlayerOverlay *baseOverlay, const char *squadname ) : 
vgui::Label( (vgui::Panel *)NULL, "OverlaySquad", squadname )
{
	m_pBaseOverlay = baseOverlay;

	strcpy( m_szSquad, squadname );

	SetPaintBackgroundEnabled( false );

	// Send mouse inputs (but not cursorenter/exit for now) up to parent
	SetReflectMouse( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudPlayerOverlaySquad::~CHudPlayerOverlaySquad( void )
{
}


//-----------------------------------------------------------------------------
// Initialization 
//-----------------------------------------------------------------------------
bool CHudPlayerOverlaySquad::Init( KeyValues* pInitData )
{
	if (!pInitData)
		return false;

	SetContentAlignment( vgui::Label::a_west );

	if (!ParseRGBA(pInitData, "fgcolor", m_fgColor ))
		return false;

	if (!ParseRGBA(pInitData, "bgcolor", m_bgColor))
		return false;

	int x, y, w, h;
	if (!ParseRect(pInitData, "position", x, y, w, h ))
		return false;
	SetPos( x, y );
	SetSize( w, h );

	return true;
}

void CHudPlayerOverlaySquad::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetFont( pScheme->GetFont( "primary" ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CHudPlayerOverlaySquad::SetSquad( const char *squadname )
{
	strcpy( m_szSquad, squadname );
	SetText( squadname );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPlayerOverlaySquad::Paint()
{
	m_pBaseOverlay->SetColorLevel( this, m_fgColor, m_bgColor );

	BaseClass::Paint();
}

void CHudPlayerOverlaySquad::SetReflectMouse( bool reflect )
{
	m_bReflectMouse = true;
}

void CHudPlayerOverlaySquad::OnCursorMoved(int x,int y)
{
	if ( !m_bReflectMouse )
		return;

	if ( !GetParent() )
		return;

	LocalToScreen( x, y );

	vgui::ivgui()->PostMessage( 
		GetParent()->GetVPanel(), 
		new KeyValues( "CursorMoved", "xpos", x, "ypos", y ), 
		GetVPanel()  );
}

void CHudPlayerOverlaySquad::OnMousePressed(vgui::MouseCode code)
{
	if ( !m_bReflectMouse )
		return;

	if ( !GetParent() )
		return;

	vgui::ivgui()->PostMessage( 
		GetParent()->GetVPanel(), 
		new KeyValues( "MousePressed", "code", code ), 
		GetVPanel()  );
}

void CHudPlayerOverlaySquad::OnMouseDoublePressed(vgui::MouseCode code)
{
	if ( !m_bReflectMouse )
		return;

	if ( !GetParent() )
		return;

	vgui::ivgui()->PostMessage( 
		GetParent()->GetVPanel(), 
		new KeyValues( "MouseDoublePressed", "code", code ), 
		GetVPanel()  );
}

void CHudPlayerOverlaySquad::OnMouseReleased(vgui::MouseCode code)
{
	if ( !m_bReflectMouse )
		return;

	if ( !GetParent() )
		return;

	vgui::ivgui()->PostMessage( 
		GetParent()->GetVPanel(), 
		new KeyValues( "MouseReleased", "code", code ), 
		GetVPanel()  );
}

void CHudPlayerOverlaySquad::OnMouseWheeled(int delta)
{
	if ( !m_bReflectMouse )
		return;

	if ( !GetParent() )
		return;

	vgui::ivgui()->PostMessage( 
		GetParent()->GetVPanel(), 
		new KeyValues( "MouseWheeled", "delta", delta ), 
		GetVPanel()  );
}


void CHudPlayerOverlaySquad::OnCursorEntered()
{
	if ( m_pBaseOverlay->GetMouseOverText() )
	{
		StatusPrint( TYPE_HINT, "%s", m_pBaseOverlay->GetMouseOverText() );
	}
}

void CHudPlayerOverlaySquad::OnCursorExited()
{
	if ( m_pBaseOverlay->GetMouseOverText() )
	{
		StatusClear();
	}
}