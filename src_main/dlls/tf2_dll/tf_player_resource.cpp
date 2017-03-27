//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "player.h"
#include "player_resource.h"
#include "tf_player_resource.h"

// Datatable
IMPLEMENT_SERVERCLASS_ST(CTFPlayerResource, DT_TFPlayerResource)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_player_manager, CTFPlayerResource );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::Spawn( void )
{
	BaseClass::Spawn();

	// Use autodetect, but only once every second.
	NetworkStateSetUpdateInterval( 2.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::GrabPlayerData( void )
{
	BaseClass::GrabPlayerData();
}
