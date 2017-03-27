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
#include "cbase.h"
#include "iconnectmsg.h"
#include <vgui/IVgui.h>
#include "vguimatsurface/imatsystemsurface.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
//-----------------------------------------------------------------------------
// Purpose: A panel to show 1-3 lines of connection status information
//-----------------------------------------------------------------------------
class CConnectMessagePanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
public:
					CConnectMessagePanel( vgui::VPANEL parent );
	virtual			~CConnectMessagePanel( void );

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void	Paint();
	virtual void	OnTick( void );

	virtual bool	ShouldDraw( void );

private:
	vgui::HFont		m_hFont;

	const ConVar	*scr_connectmsg;
	const ConVar	*scr_connectmsg1;
	const ConVar	*scr_connectmsg2;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CConnectMessagePanel::CConnectMessagePanel( vgui::VPANEL parent )
: BaseClass( NULL, "CConnectMessagePanel" )
{
	SetParent( parent );
	SetSize( ScreenWidth(), ScreenHeight() );
	SetPos( 0, 0 );
	SetVisible( false );
	SetCursor( null );

	m_hFont = 0;

	SetFgColor( Color( 0, 0, 0, 0 ) );
	SetPaintBackgroundEnabled( false );

	scr_connectmsg = cvar->FindVar( "scr_connectmsg" );
	scr_connectmsg1= cvar->FindVar( "scr_connectmsg1" );
	scr_connectmsg2= cvar->FindVar( "scr_connectmsg2" );
	assert( scr_connectmsg && scr_connectmsg1 && scr_connectmsg2 );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 200 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CConnectMessagePanel::~CConnectMessagePanel( void )
{
}

void CConnectMessagePanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Use a large font
	m_hFont = pScheme->GetFont( "Trebuchet18" );
	assert( m_hFont );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConnectMessagePanel::OnTick( void )
{
	SetVisible( ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CConnectMessagePanel::ShouldDraw( void )
{
	if ( !scr_connectmsg || !scr_connectmsg1 || !scr_connectmsg2 )
		return false;

	if ( scr_connectmsg->GetString()[0] == '0' )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CConnectMessagePanel::Paint() 
{
	int w, h, x, y;
	vrect_t rcFill;
	unsigned char color[3];

	if ( ScreenWidth() <= 250)
		w = ScreenWidth()-2;
	else
		w = 250;

	if (ScreenHeight() <= 10)
		h = ScreenHeight() -2;
	else
		h = vgui::surface()->GetFontTall( m_hFont );

	x = 0 + (ScreenWidth() - w)/2 + 1;
	y = ScreenHeight() - h * 5 - h/2;

	rcFill.x      = x - 5;
	rcFill.y      = y - h/2;
	rcFill.width  = w + 10;
	rcFill.height = h * 4;

	color[0] = color[1] = color[2] = 0;

	vgui::surface()->DrawSetColor( 0, 0, 0, 255 );
	vgui::surface()->DrawFilledRect( rcFill.x, rcFill.y, rcFill.x + rcFill.width, rcFill.y + rcFill.height );

	int r, g, b;

	r = 192;
	g = 192;
	b = 192;

	g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, r, g, b, 255, (char *)scr_connectmsg->GetString() );

	if (scr_connectmsg1->GetString()[0] != '0')
	{
		y += vgui::surface()->GetFontTall( m_hFont );
	
		g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, r, g, b, 255, (char *)scr_connectmsg1->GetString() );
	}

	if (scr_connectmsg2->GetString()[0] != '0')
	{
		y += vgui::surface()->GetFontTall( m_hFont );

		g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, r, g, b, 255, (char *)scr_connectmsg2->GetString() );
	}
}

class CConnectMsg : public IConnectMsg
{
private:
	CConnectMessagePanel *connectMessagePanel;
public:
	CConnectMsg( void )
	{
		connectMessagePanel = NULL;
	}
	void Create( vgui::VPANEL parent )
	{
		connectMessagePanel = new CConnectMessagePanel( parent );
	}

	void Destroy( void )
	{
		if ( connectMessagePanel )
		{
			connectMessagePanel->SetParent( (vgui::Panel *)NULL );
			delete connectMessagePanel;
		}
	}
};

static CConnectMsg g_ConnectMsg;
IConnectMsg *connectmsg = ( IConnectMsg * )&g_ConnectMsg;
