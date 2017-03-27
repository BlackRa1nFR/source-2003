//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hud_vehicle.h"
#include "iclientmode.h"
#include "view.h"
#include <vgui_controls/controls.h>
#include <vgui/ISurface.h>
#include "IClientVehicle.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudVehicle );

CHudVehicle::CHudVehicle( const char *pElementName ) :
  CHudElement( pElementName ), BaseClass( NULL, "HudVehicle" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

void CHudVehicle::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : IClientVehicle
//-----------------------------------------------------------------------------
IClientVehicle *CHudVehicle::GetLocalPlayerVehicle()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer ||  !pPlayer->IsInAVehicle() )
	{
		return NULL;
	}

	return pPlayer->GetVehicle();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudVehicle::ShouldDraw()
{
	return GetLocalPlayerVehicle() != NULL;
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVehicle::Paint( void )
{
	IClientVehicle *v = GetLocalPlayerVehicle();
	if ( !v )
		return;

	// Vehicle-based hud...
	v->DrawHudElements();
}

