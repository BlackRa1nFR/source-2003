//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/isurface.h>
#include "c_cs_player.h"
#include "clientmode_csnormal.h"
#include "weapon_c4.h"


class CHudProgressBar : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CHudProgressBar, vgui::Panel );


	CHudProgressBar( const char *name );


	// vgui overrides
	virtual void Paint();
	virtual bool ShouldDraw();
};


DECLARE_HUDELEMENT( CHudProgressBar );


CHudProgressBar::CHudProgressBar( const char *name ) :
	vgui::Panel( NULL, name ), CHudElement( name )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( false );
}


void CHudProgressBar::Paint()
{
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( !pPlayer )
		return;

	int x, y, wide, tall;
	GetBounds( x, y, wide, tall );

	int xOffset=0;
	int yOffset=0;


	vgui::surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );
	vgui::surface()->DrawOutlinedRect( xOffset, yOffset, xOffset+wide, yOffset+tall );

	float percent = (engine->Time() - pPlayer->m_ProgressBarStartTime) / (float)pPlayer->m_ProgressBarTime;
	percent = clamp( percent, 0, 1 );

	vgui::surface()->DrawSetColor( Color( 255, 0, 0, 128 ) );
	vgui::surface()->DrawFilledRect( xOffset+1, yOffset+1, xOffset+(int)(percent*wide)-1, yOffset+tall-1 );
}


bool CHudProgressBar::ShouldDraw()
{
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( pPlayer && 
		pPlayer->m_ProgressBarTime != 0 && 
		pPlayer->m_lifeState != LIFE_DEAD )
	{
		return true;
	}
	else
	{
		return false;
	}
}



