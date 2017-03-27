//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hud_crosshair.h"
#include "iclientmode.h"
#include "view.h"
#include <vgui_controls/controls.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar	crosshair( "crosshair", "1", FCVAR_ARCHIVE );

using namespace vgui;

int ScreenTransform( const Vector& point, Vector& screen );

DECLARE_HUDELEMENT( CHudCrosshair );

CHudCrosshair::CHudCrosshair( const char *pElementName ) :
  CHudElement( pElementName ), BaseClass( NULL, "HudCrosshair" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pCrosshair = 0;

	m_clrCrosshair = Color( 0, 0, 0, 0 );

	m_vecCrossHairOffsetAngle.Init();
}

void CHudCrosshair::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
}

void CHudCrosshair::Paint( void )
{
	if ( !crosshair.GetInt() )
		return;

	if ( !g_pClientMode->ShouldDrawCrosshair() )
		return;

	if ( !m_pCrosshair )
		return;

	// Don't draw a crosshair when I'm dead
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && !pPlayer->IsAlive() )
		return;

	m_curViewAngles = CurrentViewAngles();
	m_curViewOrigin = CurrentViewOrigin();

	float x, y;
	QAngle angles;
	Vector forward;
	Vector point, screen;

	x = ScreenWidth()/2;
	y = ScreenHeight()/2;

	// this code is wrong
	angles = m_curViewAngles + m_vecCrossHairOffsetAngle;
	AngleVectors( angles, &forward );
	VectorAdd( m_curViewOrigin, forward, point );
	ScreenTransform( point, screen );

	x += 0.5f * screen[0] * ScreenWidth() + 0.5f;
	y += 0.5f * screen[1] * ScreenHeight() + 0.5f;
	
	m_pCrosshair->DrawSelf( 
		x - 0.5f * m_pCrosshair->Width(), 
		y - 0.5f * m_pCrosshair->Height(),
		m_clrCrosshair );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : angle - 
//-----------------------------------------------------------------------------
void CHudCrosshair::SetCrosshairAngle( const QAngle& angle )
{
	VectorCopy( angle, m_vecCrossHairOffsetAngle );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hspr - 
//			rc - 
//			r - 
//			g - 
//			b - 
//-----------------------------------------------------------------------------
void CHudCrosshair::SetCrosshair( CHudTexture *texture, Color& clr )
{
	m_pCrosshair = texture;
	m_clrCrosshair = clr;
}