//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Flatbed truck that can carry multiple stationary objects
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_vehicle_flatbed.h"
#include "engine/IEngineSound.h"
#include "VGuiScreen.h"
#include "ammodef.h"
#include "in_buttons.h"

#define FLATBED_MINS			Vector(-30, -50, -10)
#define FLATBED_MAXS			Vector( 30,  50, 55)
#define FLATBED_MODEL			"models/objects/obj_flatbed.mdl"
#define FLATBED_START_SOUND		"vehicles/diesel_start1.wav"
#define FLATBED_RUNNING_SOUND	"vehicles/diesel_loop2.wav"
  

IMPLEMENT_SERVERCLASS_ST(CVehicleFlatbed, DT_VehicleFlatbed)
END_SEND_TABLE();

LINK_ENTITY_TO_CLASS(vehicle_flatbed, CVehicleFlatbed);
PRECACHE_REGISTER(vehicle_flatbed);

// CVars
ConVar	vehicle_flatbed_health( "vehicle_flatbed_health","800", FCVAR_NONE, "Flatbed health" );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CVehicleFlatbed::CVehicleFlatbed()
{

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVehicleFlatbed::Precache()
{
	engine->PrecacheModel( FLATBED_MODEL );
	enginesound->PrecacheSound( FLATBED_START_SOUND );
	enginesound->PrecacheSound( FLATBED_RUNNING_SOUND );

	PrecacheVGuiScreen( "screen_vehicle_battering_ram" );
	
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVehicleFlatbed::Spawn()
{
	SetVehicleSounds( FLATBED_START_SOUND, FLATBED_RUNNING_SOUND, NULL, NULL );
	SetModel( FLATBED_MODEL );
	
	// This size is used for placement only...
	UTIL_SetSize(this, FLATBED_MINS, FLATBED_MAXS);
	m_takedamage = DAMAGE_YES;
	m_iHealth = vehicle_flatbed_health.GetInt();

	SetType( OBJ_FLATBED );
	SetMaxPassengerCount( 1 );

	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: Gets info about the control panels
//-----------------------------------------------------------------------------
void CVehicleFlatbed::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "screen_vehicle_battering_ram";
}


