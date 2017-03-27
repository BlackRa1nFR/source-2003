//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/isurface.h>
#include "clientmode_csnormal.h"
#include "cs_gamerules.h"
#include "hud_numericdisplay.h"


class CHudC4 : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CHudC4, vgui::Panel );

	CHudC4( const char *name );

	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void Init();


private:

	CHudTexture *m_pIcon;
};


DECLARE_HUDELEMENT( CHudC4 );


CHudC4::CHudC4( const char *pName ) :
	vgui::Panel( NULL, pName ), CHudElement( pName )
{
	SetParent( g_pClientMode->GetViewport() );
	m_pIcon = NULL;
}


void CHudC4::Init()
{
}


bool CHudC4::ShouldDraw()
{
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	return pPlayer && pPlayer->HasC4();
}


void CHudC4::Paint()
{
	if ( !m_pIcon )
	{
		m_pIcon = gHUD.GetIcon( "c4" );
	}
	
	if ( m_pIcon )
	{
		int x, y, w, h;
		GetBounds( x, y, w, h );

		vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
		vgui::surface()->DrawSetTexture( m_pIcon->textureId );
		vgui::surface()->DrawTexturedRect( 0, 0, w, h );
	}
}

