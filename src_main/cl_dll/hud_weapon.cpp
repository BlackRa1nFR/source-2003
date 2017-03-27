//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui_controls/controls.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>
#include "hud_crosshair.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudWeapon : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudWeapon, vgui::Panel );
public:
	CHudWeapon( const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );

private:
	CHudCrosshair *m_pCrosshair;
};

DECLARE_HUDELEMENT( CHudWeapon );

CHudWeapon::CHudWeapon( const char *pElementName ) :
  CHudElement( pElementName ), BaseClass( NULL, "HudWeapon" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pCrosshair = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scheme - 
//-----------------------------------------------------------------------------
void CHudWeapon::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );

	m_pCrosshair =(CHudCrosshair *)GET_HUDELEMENT( CHudCrosshair );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeapon::Paint( void )
{
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->Redraw();
	}
	else
	{
		if ( m_pCrosshair )
		{
			m_pCrosshair->SetCrosshair(0, Color( 0, 0, 0, 0 ) );
		}
	}
}
