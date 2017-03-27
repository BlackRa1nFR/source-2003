//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "parsemsg.h"
#include "hud_suitpower.h"
#include "hud_macros.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudSuitPower );

#define SUITPOWER_INIT -1

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudSuitPower::CHudSuitPower( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSuitPower" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSuitPower::Init( void )
{
	m_flSuitPower = SUITPOWER_INIT;
	m_bSuitPowerLow = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSuitPower::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSuitPower::OnThink( void )
{
	float flCurrentPower = 0;
	C_BaseHLPlayer *local = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( local )
	{
		flCurrentPower = local->m_HL2Local.m_flSuitPower;
	}

	// Only update if we've changed suit power
	if ( flCurrentPower == m_flSuitPower )
		return;

	if ( flCurrentPower >= 100.0f && m_flSuitPower < 100.0f )
	{
		// we've reached max power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerMax");
	}
	else if ( flCurrentPower < 100.0f && m_flSuitPower >= 100.0f )
	{
		// we've lost power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerNotMax");
	}

	m_flSuitPower = flCurrentPower;
}

//-----------------------------------------------------------------------------
// Purpose: draws the power bar
//-----------------------------------------------------------------------------
void CHudSuitPower::Paint()
{
	// get bar chunks
	int chunkCount = m_flBarWidth / (m_flBarChunkWidth + m_flBarChunkGap);
	int enabledChunks = (int)((float)chunkCount * (m_flSuitPower / 100.0f) + 0.5f );

	// see if we've changed power state
	bool lowPower = false;
	if (enabledChunks <= (chunkCount / 4))
	{
		lowPower = true;
	}
	if (m_bSuitPowerLow != lowPower)
	{
		if (lowPower)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerDecreasedBelow25");
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerIncreasedAbove25");
		}

		m_bSuitPowerLow = lowPower;
	}

	// draw the suit power bar
	surface()->DrawSetColor( m_AuxPowerColor );
	int xpos = m_flBarInsetX, ypos = m_flBarInsetY;
	{for (int i = 0; i < enabledChunks; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}}
	// draw the exhausted portion of the bar.
	surface()->DrawSetColor( Color( m_AuxPowerColor[0], m_AuxPowerColor[1], m_AuxPowerColor[2], m_iAuxPowerDisabledAlpha ) );
	{for (int i = enabledChunks; i < chunkCount; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}}

	// draw our name
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(m_AuxPowerColor);
	surface()->DrawSetTextPos(text_xpos, text_ypos);
	for (wchar_t *wch = L"AUX POWER"; *wch != 0; wch++)
	{
		surface()->DrawUnicodeChar(*wch);
	}
}


