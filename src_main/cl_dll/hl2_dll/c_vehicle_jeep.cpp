//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_prop_vehicle.h"

ConVar r_JeepViewBlendTo( "r_JeepViewBlendTo", "1", FCVAR_CHEAT );
ConVar r_JeepViewBlendToScale( "r_JeepViewBlendToScale", "0.03", FCVAR_CHEAT );
ConVar r_JeepViewBlendToTime( "r_JeepViewBlendToTime", "1.5", FCVAR_CHEAT );

ConVar r_JeepViewDampenFreq( "r_JeepViewDampenFreq", "7.0", FCVAR_CHEAT );
ConVar r_JeepViewDampenDamp( "r_JeepViewDampenDamp", "1.0", FCVAR_CHEAT );
ConVar r_JeepViewZHeight( "r_JeepViewZHeight", "10.0", FCVAR_CHEAT );

#define JEEP_DELTA_LENGTH_MAX	12.0f			// 1 foot
#define JEEP_FRAMETIME_MIN		1e-6

//=============================================================================
//
// Client-side Jeep Class
//
class C_PropJeep : public C_PropVehicleDriveable
{

	DECLARE_CLASS( C_PropJeep, C_PropVehicleDriveable );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_PropJeep();
	~C_PropJeep();

public:

	void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd );
	void DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );

private:

	void DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void ComputePDControllerCoefficients( float *pCoefficientsOut, float flFrequency, float flDampening, float flDeltaTime );

private:

	float		m_flViewAngleDeltaTime;
};

IMPLEMENT_CLIENTCLASS_DT( C_PropJeep, DT_PropJeep, CPropJeep )
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_PropJeep::C_PropJeep()
{
	m_flViewAngleDeltaTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
C_PropJeep::~C_PropJeep()
{
}

//-----------------------------------------------------------------------------
// Purpose: Blend view angles.
//-----------------------------------------------------------------------------
void C_PropJeep::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
{
	if ( r_JeepViewBlendTo.GetInt() )
	{
		// Check to see if the mouse has been touched in a bit or that we are not throttling.
		if ( ( pCmd->mousedx != 0 || pCmd->mousedy != 0 ) || ( fabsf( m_flThrottle ) < 0.01f ) )
		{
			m_flViewAngleDeltaTime = 0.0f;
		}
		else
		{
			m_flViewAngleDeltaTime += gpGlobals->frametime;
		}

		if ( m_flViewAngleDeltaTime > r_JeepViewBlendToTime.GetFloat() )
		{
			// Blend the view angles.
			int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
			Vector vehicleEyeOrigin;
			QAngle vehicleEyeAngles;
			GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );
			
			QAngle outAngles;
			InterpolateAngles( pCmd->viewangles, vehicleEyeAngles, outAngles, r_JeepViewBlendToScale.GetFloat() );
			pCmd->viewangles = outAngles;
		}
	}

	BaseClass::UpdateViewAngles( pLocalPlayer, pCmd );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropJeep::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
{
	// Get the frametime. (Check to see if enough time has passed to warrent dampening).
	float flFrameTime = gpGlobals->frametime;
	if ( flFrameTime < JEEP_FRAMETIME_MIN )
	{
//		vecVehicleEyePos = m_vecLastDampEyePos;
		return;
	}

	// Dampen forward/backward motion.
	DampenForwardMotion( vecVehicleEyePos, vecVehicleEyeAngles, flFrameTime );

	// Blend up/down motion.
	DampenUpMotion( vecVehicleEyePos, vecVehicleEyeAngles, flFrameTime );

	// Keep static the sideways motion.
}


//-----------------------------------------------------------------------------
// Use the controller as follows:
// speed += ( pCoefficientsOut[0] * ( targetPos - currentPos ) + pCoefficientsOut[1] * ( targetSpeed - currentSpeed ) ) * flDeltaTime;
//-----------------------------------------------------------------------------
void C_PropJeep::ComputePDControllerCoefficients( float *pCoefficientsOut,
												  float flFrequency, float flDampening,
												  float flDeltaTime )
{
	float flKs = 9.0f * flFrequency * flFrequency;
	float flKd = 4.5f * flFrequency * flDampening;

	float flScale = 1.0f / ( 1.0f + flKd * flDeltaTime + flKs * flDeltaTime * flDeltaTime );

	pCoefficientsOut[0] = flKs * flScale;
	pCoefficientsOut[1] = ( flKd + flKs * flDeltaTime ) * flScale;
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropJeep::DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
{
	// Get forward vector.
	Vector vecForward;
	AngleVectors( vecVehicleEyeAngles, &vecForward);

	// Simulate the eye position forward based on the data from last frame
	// (assumes no acceleration - it will get that from the "spring").
	Vector vecCurrentEyePos = m_vecLastEyePos + m_vecEyeSpeed * flFrameTime;

	// Calculate target speed based on the current vehicle eye position and the last vehicle eye position and frametime.
	Vector vecVehicleEyeSpeed = ( vecVehicleEyePos - m_vecLastEyeTarget ) / flFrameTime;
	m_vecLastEyeTarget = vecVehicleEyePos;	

	// Calculate the speed and position deltas.
	Vector vecDeltaSpeed = vecVehicleEyeSpeed - m_vecEyeSpeed;
	Vector vecDeltaPos = vecVehicleEyePos - vecCurrentEyePos;

	// Clamp.
	if ( vecDeltaPos.Length() > JEEP_DELTA_LENGTH_MAX )
	{
		float flSign = vecForward.Dot( vecVehicleEyeSpeed ) >= 0.0f ? 1.0f : -1.0f;
		vecVehicleEyePos += flSign * ( vecForward * JEEP_DELTA_LENGTH_MAX );
		m_vecLastEyePos = vecVehicleEyePos;
		m_vecEyeSpeed = vecVehicleEyeSpeed;
		return;
	}

	// Generate an updated (dampening) speed for use in next frames position extrapolation.
	float flCoefficients[2];
	ComputePDControllerCoefficients( flCoefficients, r_JeepViewDampenFreq.GetFloat(), r_JeepViewDampenDamp.GetFloat(), flFrameTime );
	m_vecEyeSpeed += ( ( flCoefficients[0] * vecDeltaPos + flCoefficients[1] * vecDeltaSpeed ) * flFrameTime );

	// Save off data for next frame.
	m_vecLastEyePos = vecCurrentEyePos;

	// Move eye forward/backward.
	Vector vecForwardOffset = vecForward * ( vecForward.Dot( vecDeltaPos ) );
	vecVehicleEyePos -= vecForwardOffset;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropJeep::DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
{
	// Get up vector.
	Vector vecUp;
	AngleVectors( vecVehicleEyeAngles, NULL, NULL, &vecUp );
	vecUp.z = clamp( vecUp.z, 0.0f, vecUp.z );
	vecVehicleEyePos.z += r_JeepViewZHeight.GetFloat() * vecUp.z;

	// NOTE: Should probably use some damped equation here.
}
