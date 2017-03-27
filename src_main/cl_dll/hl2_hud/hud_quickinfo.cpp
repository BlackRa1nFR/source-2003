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
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include <vgui_controls/controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

#define	QINFO_FADE_TIME				150

#define	HEALTH_WARNING_THRESHOLD	25

static ConVar	hud_quickinfo( "hud_quickinfo", "0", FCVAR_ARCHIVE );

/*
==================================================
CHUDQuickInfo 
==================================================
*/

using namespace vgui;

class CHUDQuickInfo : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHUDQuickInfo, vgui::Panel );
public:
	CHUDQuickInfo( const char *pElementName );
	void Init( void );
	void VidInit( void );
	bool ShouldDraw( void );
	virtual void Paint();
	
	virtual void ApplySchemeSettings( IScheme *scheme );
private:
	
	void DrawWarning( int x, int y, CHudTexture *icon, float &time );

	int		m_lastAmmo;
	int		m_lastHealth;

	float	m_ammoFade;
	float	m_healthFade;
	float	m_fFade;
	float	m_fDamageFade;

	bool	m_warnAmmo;
	bool	m_warnHealth;
};

DECLARE_HUDELEMENT( CHUDQuickInfo );

//
//-----------------------------------------------------
//
CHUDQuickInfo::CHUDQuickInfo( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HUDQuickInfo" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

void CHUDQuickInfo::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
}


/*
==================================================
Init
==================================================
*/

void CHUDQuickInfo::Init( void )
{
	m_ammoFade		= 0.0f;
	m_healthFade	= 0.0f;
	m_fFade			= 0.0f;

	m_lastAmmo		= 0;
	m_lastHealth	= 100;

	m_warnAmmo		= false;
	m_warnHealth	= false;
}

/*
==================================================
VidInit
==================================================
*/

void CHUDQuickInfo::VidInit( void )
{
	Init();
}

/*
==================================================
DrawWarning
==================================================
*/

void CHUDQuickInfo::DrawWarning( int x, int y, CHudTexture *icon, float &time )
{
	float scale	= (int)( fabs(sin(gpGlobals->curtime*8.0f)) * 128.0);

	//Only fade out at the low point of our blink
	if ( time <= (gpGlobals->frametime * 200.0f) )
	{
		if ( scale < 40 )
		{
			time = 0.0f;
			return;
		}
		else
		{
			//Counteract the offset below to survive another frame
			time += (gpGlobals->frametime * 200.0f);
		}
	}
	
	//Update our time
	time -= (gpGlobals->frametime * 200.0f);
	Color caution = gHUD.m_clrCaution;
	caution[3] = scale * 255;

	icon->DrawSelf( x, y, caution );
}

/*
==================================================
Draw
==================================================
*/

bool CHUDQuickInfo::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && hud_quickinfo.GetInt() );
}

/*
==================================================
Draw
==================================================
*/

void CHUDQuickInfo::Paint()
{
	CHudTexture	*icon_c		= gHUD.GetIcon( "crosshair" );
	CHudTexture	*icon_rb	= gHUD.GetIcon( "crosshair_right" );
	CHudTexture	*icon_lb	= gHUD.GetIcon( "crosshair_left" );

	if ( !icon_c || !icon_rb || !icon_lb )
		return;

	int		xCenter	= ( ScreenWidth() / 2 ) - icon_c->Width() / 2;
	int		yCenter = ( ScreenHeight() / 2 ) - icon_c->Height() / 2;
	int		scalar;
	
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();

	if ( player == NULL )
		return;

	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

	if ( pWeapon == NULL )
		return;

	//Get our values
	int	health	= player->GetHealth();
	int	ammo	= pWeapon->Clip1();

	if ( m_fDamageFade > 0.0f )
	{
		m_fDamageFade -= (gpGlobals->frametime * 200.0f);
	}

	//Check our health for a warning
	if ( health != m_lastHealth )
	{
		if ( health < m_lastHealth )
		{
			m_fDamageFade = QINFO_FADE_TIME;
		}

		m_fFade			= QINFO_FADE_TIME;
		m_lastHealth	= health;

		if ( health <= HEALTH_WARNING_THRESHOLD )
		{
			if ( m_warnHealth == false )
			{
				m_healthFade = 255;
				m_warnHealth = true;
				
				CLocalPlayerFilter filter;
				C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "HUDQuickInfo.LowHealth" );
			}
		}
		else
		{
			m_warnHealth = false;
		}
	}

	// Check our ammo for a warning
	if ( ammo != m_lastAmmo )
	{
		m_fFade		= QINFO_FADE_TIME;
		m_lastAmmo	= ammo;

		if ( ( (float) ammo / (float) pWeapon->GetMaxClip1() ) <= ( 1.0f - CLIP_PERC_THRESHOLD ) )
		{
			if ( m_warnAmmo == false )
			{
				m_ammoFade = 255;
				m_warnAmmo = true;

				CLocalPlayerFilter filter;
				C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "HUDQuickInfo.LowAmmo" );
			}
		}
		else
		{
			m_warnAmmo = false;
		}
	}

	//Get our crosshair intensity
	if ( m_fFade > 0.0f )
	{
		m_fFade  -= (gpGlobals->frametime * 50.0f);

		if ( m_fFade  < 128.0f )
		{
			scalar = (int) max( 16, (m_fFade) );
		}
		else
		{
			scalar = 128;
		}
	}
	else
	{
		scalar = 16;
	}

	if ( player->IsInAVehicle() )
	{
		scalar = 48;
	}

	Color clrNormal = gHUD.m_clrNormal;
	clrNormal[3] = 255 * scalar;
	icon_c->DrawSelf( xCenter, yCenter, clrNormal );

	int	sinScale = (int)( fabs(sin(gpGlobals->curtime*8.0f)) * 128.0f );

	//Update our health
	if ( m_healthFade > 0.0f )
	{
		DrawWarning( xCenter - 10, yCenter-5, icon_lb, m_healthFade );
	}
	else
	{
		float	healthPerc = (float) health / 100.0f;

		Color healthColor = m_warnHealth ? gHUD.m_clrCaution : gHUD.m_clrNormal;
		
		if ( m_warnHealth )
		{
			healthColor[3] = 255 * sinScale;
		}
		else
		{
			healthColor[3] = 255 * scalar;
		}
		
		gHUD.DrawIconProgressBar( xCenter - 10, yCenter-5, icon_lb, ( 1.0f - healthPerc ), healthColor, CHud::HUDPB_VERTICAL );
	}

	//Update our ammo
	if ( m_ammoFade > 0.0f )
	{
		DrawWarning( xCenter + icon_rb->Width() - 6, yCenter-5, icon_rb, m_ammoFade );
	}
	else
	{
		float ammoPerc = 1.0f - ( (float) ammo / (float) pWeapon->GetMaxClip1() );

		Color ammoColor = m_warnAmmo ? gHUD.m_clrCaution : gHUD.m_clrNormal;
		
		if ( m_warnAmmo )
		{
			ammoColor[3] = 255 * sinScale;
		}
		else
		{
			ammoColor[3] = 255 * scalar;
		}
		
		gHUD.DrawIconProgressBar( xCenter + icon_rb->Width() - 6, yCenter-5, icon_rb, ammoPerc, ammoColor, CHud::HUDPB_VERTICAL );
	}
}
