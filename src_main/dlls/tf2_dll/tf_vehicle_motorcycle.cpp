//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "engine/IEngineSound.h"
#include "VGuiScreen.h"
#include "tf_basefourwheelvehicle.h"
#include "vphysics/vehicles.h"

#define MOTORCYCLE_MINS			Vector(-30, -50, -10)
#define MOTORCYCLE_MAXS			Vector( 30,  50, 55)
#define MOTORCYCLE_MODEL			"models/objects/vehicle_motorcycle.mdl"
#define MOTORCYCLE_START_SOUND	"vehicles/diesel_start1.wav"
#define MOTORCYCLE_RUNNING_SOUND "vehicles/diesel_loop2.wav"
#define MOTORCYCLE_TURBO_SOUND	"vehicles/diesel_nitroboost.wav"

// ------------------------------------------------------------------------ //
// Purpose:
// ------------------------------------------------------------------------ //
class CVehicleMotorcycle : public CBaseTFFourWheelVehicle
{
	DECLARE_CLASS( CVehicleMotorcycle, CBaseTFFourWheelVehicle );

public:
	DECLARE_SERVERCLASS();

	static CVehicleMotorcycle* Create(const Vector &vOrigin, const QAngle &vAngles);

	CVehicleMotorcycle();

	virtual void	Spawn();
	virtual void	Precache();
	virtual void	GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	virtual bool	CanTakeEMPDamage( void ) { return true; }

	// Vehicle overrides
	virtual bool	IsPassengerVisible( int nRole ) { return true; }
};

IMPLEMENT_SERVERCLASS_ST(CVehicleMotorcycle, DT_VehicleMotorcycle)
END_SEND_TABLE();

LINK_ENTITY_TO_CLASS(vehicle_motorcycle, CVehicleMotorcycle);
PRECACHE_REGISTER(vehicle_motorcycle);

// CVars
ConVar	vehicle_motorcycle_health( "vehicle_motorcycle_health","200", FCVAR_NONE, "Motorcycle health" );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CVehicleMotorcycle::CVehicleMotorcycle()
{

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVehicleMotorcycle::Precache()
{
	engine->PrecacheModel( MOTORCYCLE_MODEL );
	enginesound->PrecacheSound( MOTORCYCLE_START_SOUND );
	enginesound->PrecacheSound( MOTORCYCLE_RUNNING_SOUND );
	enginesound->PrecacheSound( MOTORCYCLE_TURBO_SOUND );

	PrecacheVGuiScreen( "screen_vulnerable_point");
	
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVehicleMotorcycle::Spawn()
{
	SetVehicleSounds( MOTORCYCLE_START_SOUND, MOTORCYCLE_RUNNING_SOUND, NULL, MOTORCYCLE_TURBO_SOUND );
	
	SetModel( MOTORCYCLE_MODEL );
	
	// This size is used for placement only...
	UTIL_SetSize(this, MOTORCYCLE_MINS, MOTORCYCLE_MAXS);
	m_takedamage = DAMAGE_YES;
	m_iHealth = vehicle_motorcycle_health.GetInt();

	SetType( OBJ_VEHICLE_MOTORCYCLE );
	SetMaxPassengerCount( 4 );

	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: Gets info about the control panels
//-----------------------------------------------------------------------------
void CVehicleMotorcycle::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "screen_vulnerable_point";
}


