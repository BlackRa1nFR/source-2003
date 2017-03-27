//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "c_prop_vehicle.h"
#include "hud.h"		
#include <vgui_controls/Controls.h>
#include <Color.h>
#if 0
#include "engine/IVDebugOverlay.h"
#endif

int ScreenTransform( const Vector& point, Vector& screen );

IMPLEMENT_CLIENTCLASS_DT(C_PropVehicleDriveable, DT_PropVehicleDriveable, CPropVehicleDriveable)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropInt( RECVINFO( m_nSpeed ) ),
	RecvPropInt( RECVINFO( m_nRPM ) ),
	RecvPropFloat( RECVINFO( m_flThrottle ) ),
	RecvPropInt( RECVINFO( m_nBoostTimeLeft ) ),
	RecvPropInt( RECVINFO( m_nHasBoost ) ),
	RecvPropInt( RECVINFO( m_nScannerDisabledWeapons ) ),
	RecvPropInt( RECVINFO( m_nScannerDisabledVehicle ) ),
	RecvPropVector( RECVINFO( m_vecLookCrosshair ) ),
	RecvPropVector( RECVINFO( m_vecGunCrosshair ) ),
	RecvPropInt( RECVINFO( m_bEnterAnimOn ) ),
	RecvPropInt( RECVINFO( m_bExitAnimOn ) ),
END_RECV_TABLE()

#define ROLL_CURVE_ZERO		20		// roll less than this is clamped to zero
#define ROLL_CURVE_LINEAR	90		// roll greater than this is copied out

#define PITCH_CURVE_ZERO		10	// pitch less than this is clamped to zero
#define PITCH_CURVE_LINEAR		45	// pitch greater than this is copied out
									// spline in between

ConVar r_VehicleViewClamp( "r_VehicleViewClamp", "1", FCVAR_CHEAT );
ConVar r_VehicleViewDampen( "r_VehicleViewDampen", "1", FCVAR_CHEAT );

// remaps an angular variable to a 3 band function:
// 0 <= t < start :		f(t) = 0
// start <= t <= end :	f(t) = end * spline(( t-start) / (end-start) )  // s curve between clamped and linear
// end < t :			f(t) = t
float RemapAngleRange( float startInterval, float endInterval, float value )
{
	// Fixup the roll
	value = AngleNormalize( value );
	float absAngle = fabs(value);

	// beneath cutoff?
	if ( absAngle < startInterval )
	{
		value = 0;
	}
	// in spline range?
	else if ( absAngle <= endInterval )
	{
		float newAngle = SimpleSpline( (absAngle - startInterval) / (endInterval-startInterval) ) * endInterval;
		// grab the sign from the initial value
		if ( value < 0 )
		{
			newAngle *= -1;
		}
		value = newAngle;
	}
	// else leave it alone, in linear range
	
	return value;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_PropVehicleDriveable::C_PropVehicleDriveable()
{
	m_vecEyeSpeed.Init();

	AddVar( &m_vecLookCrosshair, &m_iv_vecLookCrosshair, LATCH_SIMULATION_VAR );
	AddVar( &m_vecGunCrosshair, &m_iv_vecGunCrosshair, LATCH_SIMULATION_VAR );
}

//-----------------------------------------------------------------------------
// Purpose: De-constructor
//-----------------------------------------------------------------------------
C_PropVehicleDriveable::~C_PropVehicleDriveable()
{
}

C_BasePlayer* C_PropVehicleDriveable::GetPassenger( int nRole )
{
	if (nRole == VEHICLE_DRIVER)
		return m_hPlayer.Get();
	return NULL;
}

//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------
int	C_PropVehicleDriveable::GetPassengerRole( C_BasePlayer *pEnt )
{
	if (m_hPlayer.Get() == pEnt)
	{
		return VEHICLE_DRIVER;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
{
}

//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	Assert( nRole == VEHICLE_DRIVER );
	CBasePlayer *pPlayer = GetPassenger( nRole );
	Assert( pPlayer );

	*pAbsAngles = pPlayer->EyeAngles();

	int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachment( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );
	AngleMatrix( vehicleEyeAngles, vehicleEyePosToWorld );

	// Dampen the eye positional change.
	if ( r_VehicleViewDampen.GetInt() )
	{
		DampenEyePosition( vehicleEyeOrigin, vehicleEyeAngles );
	}

	//HACKHACK -- use activity instead of discrete sequence number
	C_BaseAnimating *ent = dynamic_cast<C_BaseAnimating*>(GetVehicleEnt());
	if ( ent )
	{
		bool changed = (m_bWasEntering != (m_bEnterAnimOn || m_bExitAnimOn));

		if ( m_bEnterAnimOn || m_bExitAnimOn )
		{
			if ( changed )
			{
				m_flSequenceChangeTime = gpGlobals->curtime;
			}
			float flTimeSinceChanged = gpGlobals->curtime - m_flSequenceChangeTime;
			float frac = flTimeSinceChanged / 0.8f;
			frac = clamp( frac, 0.0f, 1.0f );
			if ( frac != 1.0 )
			{
				QAngle outangles;
				InterpolateAngles(*pAbsAngles, vehicleEyeAngles, outangles, frac );
				*pAbsAngles = outangles;
			}
			else
			{
				*pAbsAngles = vehicleEyeAngles;
			}
		}
		else if ( changed )
		{
  			QAngle zero;
  			zero.Init();
  			zero.y = 90;
  			engine->SetViewAngles( zero );
			*pAbsAngles = vehicleEyeAngles;
		}

		m_bWasEntering = (m_bEnterAnimOn || m_bExitAnimOn);
	}

	// Compute the relative rotation between the unperturbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix( *pAbsAngles, cameraToWorld );

	matrix3x4_t worldToEyePos;
	MatrixInvert( vehicleEyePosToWorld, worldToEyePos );

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms( worldToEyePos, cameraToWorld, vehicleCameraToEyePos );

	// Damp out some of the vehicle motion (neck/head would do this)
	vehicleEyeAngles.x = RemapAngleRange( PITCH_CURVE_ZERO, PITCH_CURVE_LINEAR, vehicleEyeAngles.x );
	vehicleEyeAngles.z = RemapAngleRange( ROLL_CURVE_ZERO, ROLL_CURVE_LINEAR, vehicleEyeAngles.z );
	AngleMatrix( vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld );

	// Now treat the relative eye angles as being relative to this new, perturbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms( vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld );

	// output new view abs angles
	MatrixAngles( newCameraToWorld, *pAbsAngles );

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn( newCameraToWorld, 3, *pAbsOrigin );
}


//-----------------------------------------------------------------------------
// Futzes with the clip planes
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// FIXME: Need something a better long-term, this fixes the buggy.
	flZNear = 6;
}

	
//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Simply used to return intensity value based upon current timer passed in
//-----------------------------------------------------------------------------
int GetFlashColorIntensity( int LowIntensity, int HighIntensity, bool Dimming, int Increment, int Timer )
{
	if ( Dimming ) 
		return ( HighIntensity - Timer * Increment );
	else
		return ( LowIntensity + Timer * Increment );
}

void C_PropVehicleDriveable::DrawHudElements( )
{
	CHudTexture *pIcon;

	// draw crosshairs for vehicle guns
	if ( m_vecLookCrosshair[0] != 0 || m_vecLookCrosshair[1] != 0 || m_vecLookCrosshair[2] != 0)
	{
		float x, y;
		Vector screen;

		pIcon = gHUD.GetIcon( "viewhair" );
		if ( pIcon )
		{
			x = ScreenWidth()/2;
			y = ScreenHeight()/2;
			ScreenTransform( m_vecLookCrosshair, screen );
			x += 0.5 * screen[0] * ScreenWidth() + 0.5;
			y -= 0.5 * screen[1] * ScreenHeight() + 0.5;
			x -= pIcon->Width() / 2; 
			y -= pIcon->Height() / 2; 
	
			pIcon->DrawSelf( x, y, gHUD.m_clrNormal );
		}

		pIcon = gHUD.GetIcon( "gunhair" );
		if ( pIcon )
		{
			x = ScreenWidth()/2;
			y = ScreenHeight()/2;
			ScreenTransform( m_vecGunCrosshair, screen );
			x += 0.5 * screen[0] * ScreenWidth() + 0.5;
			y -= 0.5 * screen[1] * ScreenHeight() + 0.5;
			x -= pIcon->Width() / 2; 
			y -= pIcon->Height() / 2; 
			pIcon->DrawSelf( x, y, gHUD.m_clrNormal  );
		}
	}



	int iIconX, iIconY;

	if ( m_nScannerDisabledWeapons )
	{
		// Draw icons for scanners "weps disabled"  
		pIcon = gHUD.GetIcon( "dmg_bio" );
		if ( pIcon )
		{
			iIconY = 467 - pIcon->Height() / 2;
			iIconX = 385;
			if ( !m_bScannerWepIcon )
			{
				pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, 255, 255 ) );
				m_bScannerWepIcon = true;
				m_iScannerWepFlashTimer = 0;
				m_bScannerWepDim = true;
			}
			else
			{
				pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, GetFlashColorIntensity(55, 255, m_bScannerWepDim, 10, m_iScannerWepFlashTimer), 255 ) );
				m_iScannerWepFlashTimer++;
				m_iScannerWepFlashTimer %= 20;
				if(!m_iScannerWepFlashTimer)
					m_bScannerWepDim ^= 1;
			}
		}
	}

	if ( m_nScannerDisabledVehicle )
	{
		// Draw icons for scanners "vehicle disabled"  
		pIcon = gHUD.GetIcon( "dmg_bio" );
		if ( pIcon )
		{
			iIconY = 467 - pIcon->Height() / 2;
			iIconX = 410;
			if ( !m_bScannerVehicleIcon )
			{
				pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, 255, 255 ) );
				m_bScannerVehicleIcon = true;
				m_iScannerVehicleFlashTimer = 0;
				m_bScannerVehicleDim = true;
			}
			else
			{
				pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, GetFlashColorIntensity(55, 255, m_bScannerVehicleDim, 10, m_iScannerVehicleFlashTimer), 255 ) );
				m_iScannerVehicleFlashTimer++;
				m_iScannerVehicleFlashTimer %= 20;
				if(!m_iScannerVehicleFlashTimer)
					m_bScannerVehicleDim ^= 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::RestrictView( float *pYawBounds, float *pPitchBounds,
										   float *pRollBounds, QAngle &vecViewAngles )
{
	int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );

	// Limit the yaw.
	if ( pYawBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.y, vehicleEyeAngles.y );
		flAngleDiff = clamp( flAngleDiff, pYawBounds[0], pYawBounds[1] );
		vecViewAngles.y = vehicleEyeAngles.y + flAngleDiff;
	}

	// Limit the pitch.
	if ( pPitchBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.x, vehicleEyeAngles.x );
		flAngleDiff = clamp( flAngleDiff, pPitchBounds[0], pPitchBounds[1] );
		vecViewAngles.x = vehicleEyeAngles.x + flAngleDiff;
	}

	// Limit the roll.
	if ( pRollBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.z, vehicleEyeAngles.z );
		flAngleDiff = clamp( flAngleDiff, pRollBounds[0], pRollBounds[1] );
		vecViewAngles.z = vehicleEyeAngles.z + flAngleDiff;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
{
	if ( r_VehicleViewClamp.GetInt() )
	{
		float pitchBounds[2] = { -85.0f, 25.0f };
		RestrictView( NULL, pitchBounds, NULL, pCmd->viewangles );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::OnEnteredVehicle( C_BasePlayer *pPlayer )
{
}
