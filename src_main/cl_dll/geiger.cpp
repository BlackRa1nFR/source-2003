/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// Geiger.cpp
//
// implementation of CHudAmmo class
//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "parsemsg.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystemBase.h"
#include "iclientmode.h"
#include <vgui_controls/controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudGeiger: public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudGeiger, vgui::Panel );
public:
	CHudGeiger( const char *pElementName );
	void Init( void );
	void VidInit( void );
	bool ShouldDraw( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );
	void MsgFunc_Geiger(const char *pszName, int iSize, void *pbuf);
	
private:
	int m_iGeigerRange;

};

DECLARE_HUDELEMENT( CHudGeiger );
DECLARE_HUD_MESSAGE( CHudGeiger, Geiger );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudGeiger::CHudGeiger( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudGeiger" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

void CHudGeiger::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGeiger::Init(void)
{
	HOOK_MESSAGE( Geiger );

	m_iGeigerRange = 0;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGeiger::VidInit(void)
{
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGeiger::MsgFunc_Geiger( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	// update geiger data
	m_iGeigerRange = READ_BYTE();
	m_iGeigerRange = m_iGeigerRange << 2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudGeiger::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( m_iGeigerRange < 1000 && m_iGeigerRange > 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGeiger::Paint()
{
	int pct;
	float flvol=0;
	bool highsound = false;
	
	// piecewise linear is better than continuous formula for this
	if (m_iGeigerRange > 800)
	{
		pct = 0;			//Msg ( "range > 800\n");
	}
	else if (m_iGeigerRange > 600)
	{
		pct = 2;
		flvol = 0.4;		//Msg ( "range > 600\n");
	}
	else if (m_iGeigerRange > 500)
	{
		pct = 4;
		flvol = 0.5;		//Msg ( "range > 500\n");
	}
	else if (m_iGeigerRange > 400)
	{
		pct = 8;
		flvol = 0.6;		//Msg ( "range > 400\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 300)
	{
		pct = 8;
		flvol = 0.7;		//Msg ( "range > 300\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 200)
	{
		pct = 28;
		flvol = 0.78;		//Msg ( "range > 200\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 150)
	{
		pct = 40;
		flvol = 0.80;		//Msg ( "range > 150\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 100)
	{
		pct = 60;
		flvol = 0.85;		//Msg ( "range > 100\n");
		highsound = true;
	}
	else if (m_iGeigerRange > 75)
	{
		pct = 80;
		flvol = 0.9;		//Msg ( "range > 75\n");
		//gflGeigerDelay = cl.time + GEIGERDELAY * 0.75;
		highsound = true;
	}
	else if (m_iGeigerRange > 50)
	{
		pct = 90;
		flvol = 0.95;		//Msg ( "range > 50\n");
	}
	else
	{
		pct = 95;
		flvol = 1.0;		//Msg ( "range < 50\n");
	}

	flvol = (flvol * (random->RandomInt(0,127)) / 255) + 0.25;

	if ( random->RandomInt(0,127) < pct )
	{
		char sz[256];
		if ( highsound )
		{
			strcpy( sz, "Geiger.BeepHigh" );
		}
		else
		{
			strcpy( sz, "Geiger.BeepLow" );
		}

		CSoundParameters params;

		if ( C_BaseEntity::GetParametersForSound( sz, params ) )
		{
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, params.channel,
				params.soundname, flvol, params.soundlevel, 0, params.pitch); 
		}
	}
}
