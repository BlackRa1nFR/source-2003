//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef VEHICLE_SOUNDS_H
#define VEHICLE_SOUNDS_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
enum vehiclesound
{
	VS_ENGINE_START,
	VS_ENGINE_STOP,
	VS_ENGINE_IDLE,
	VS_TURBO,
	VS_TURBO_OFF,
	VS_SKID_FRICTION_LOW,
	VS_SKID_FRICTION_NORMAL,
	VS_SKID_FRICTION_HIGH,
	VS_SPEED_SOUND,
	VS_ENGINE2_START,
	VS_ENGINE2_STOP,
	VS_MISC1,
	VS_MISC2,
	VS_MISC3,
	VS_MISC4,

	VS_NUM_SOUNDS,
};

enum vehiclesound_gear
{
	VS_ENGINE_LOOP,
	VS_FOOT_OFF_GAS,
	VS_FOOT_OFF_GAS_SLOW,
	VS_DOWNSHIFTED,

	VS_NUM_GEAR_SOUNDS,
};

extern char *vehiclesound_parsenames[VS_NUM_SOUNDS];
extern char *vehiclesound_gear_parsenames[VS_NUM_GEAR_SOUNDS];

// This is a list of vehiclesounds to automatically stop when the vehicle's driver exits the vehicle
#define NUM_SOUNDS_TO_STOP_ON_EXIT	4
extern vehiclesound g_iSoundsToStopOnExit[NUM_SOUNDS_TO_STOP_ON_EXIT];

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct vehicle_gear_t
{
	DECLARE_DATADESC();

	float		flMinSpeed;
	float		flMaxSpeed;
	float		flMinPitch[ VS_NUM_GEAR_SOUNDS ];
	float		flMaxPitch[ VS_NUM_GEAR_SOUNDS ];
	float		flSpeedApproachFactor;
	string_t	iszSound[ VS_NUM_GEAR_SOUNDS ];
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct vehiclesounds_t
{
	DECLARE_DATADESC();

	string_t					iszSound[ VS_NUM_SOUNDS ];
	CUtlVector<vehicle_gear_t>	pGears;
};

//-----------------------------------------------------------------------------
// Purpose: A KeyValues parse for vehicle sound blocks
//-----------------------------------------------------------------------------
class CVehicleSoundsParser : public IVPhysicsKeyHandler
{
public:
	CVehicleSoundsParser( void ) 
	{
		m_iCurrentGear = -1;
	}

	void	ParseVehicleSounds( const char *pScriptName, vehiclesounds_t *pSounds );

private:
	virtual void ParseKeyValue( void *pData, const char *pKey, const char *pValue );
	virtual void SetDefaults( void *pData );

	// Index of the gear we're currently reading data into
	int	m_iCurrentGear;
};

#endif // VEHICLE_SOUNDS_H
