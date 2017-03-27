//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "IClientVehicle.h"
#include "hud.h"		
#include <vgui_controls/Controls.h>
#include <Color.h>

int ScreenTransform( const Vector& point, Vector& screen );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PropCrane : public C_BaseAnimating, public IClientVehicle
{
	DECLARE_CLASS( C_PropCrane, C_BaseAnimating );
public:
	DECLARE_CLIENTCLASS();

	C_PropCrane();
	
	void PostDataUpdate( DataUpdateType_t updateType );

// IClientVehicle overrides.
public:
	virtual void OnEnteredVehicle( C_BasePlayer *pPlayer );
	virtual void GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles );
	virtual void GetVehicleFOV( float &flFOV ) { return; }
	virtual void DrawHudElements();
	virtual bool IsPassengerUsingStandardWeapons( int nRole = VEHICLE_DRIVER ) { return false; }
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd ) {}
	virtual C_BasePlayer* GetPassenger( int nRole );
	virtual int	GetPassengerRole( C_BasePlayer *pEnt );
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const;

// C_BaseEntity overrides.
public:
	virtual IClientVehicle*	GetClientVehicle() { return this; }
	virtual C_BaseEntity	*GetVehicleEnt() { return this; }
	virtual void SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) {}
	virtual void ProcessMovement( C_BasePlayer *pPlayer, CMoveData *pMoveData ) {}
	virtual void FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move ) {}
	virtual bool IsPredicted() const { return false; }
	virtual void ItemPostFrame( C_BasePlayer *pPlayer ) {}
	virtual bool IsSelfAnimating() { return false; };

private:
	CHandle<C_BasePlayer>	m_hPlayer;

	int		m_nEnterSequence;
	int		m_nExitSequence;
	int		m_nPreviousSequence;
	float	m_flSequenceChangeTime;
};

IMPLEMENT_CLIENTCLASS_DT(C_PropCrane, DT_PropCrane, CPropCrane)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
END_RECV_TABLE()


#define ROLL_CURVE_ZERO		5		// roll less than this is clamped to zero
#define ROLL_CURVE_LINEAR	45		// roll greater than this is copied out

#define PITCH_CURVE_ZERO		10	// pitch less than this is clamped to zero
#define PITCH_CURVE_LINEAR		45	// pitch greater than this is copied out
									// spline in between
extern float RemapAngleRange( float startInterval, float endInterval, float value );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PropCrane::C_PropCrane( void )
{
	m_nEnterSequence = -1;
	m_nExitSequence = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropCrane::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Store off the sequence numbers of our enter / exit anims
		C_BaseAnimating *ent = dynamic_cast<C_BaseAnimating*>(GetVehicleEnt());
		if ( ent )
		{
			m_nEnterSequence = ent->LookupSequence( "enter" );
			m_nExitSequence = ent->LookupSequence( "exit" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BasePlayer* C_PropCrane::GetPassenger( int nRole )
{
	if (nRole == VEHICLE_DRIVER)
		return m_hPlayer.Get();
	return NULL;
}

//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------
int	C_PropCrane::GetPassengerRole( C_BasePlayer *pEnt )
{
	if (m_hPlayer.Get() == pEnt)
	{
		return VEHICLE_DRIVER;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void C_PropCrane::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	Assert( nRole == VEHICLE_DRIVER );
	CBasePlayer *pPlayer = GetPassenger( nRole );
	Assert( pPlayer );

	*pAbsAngles = pPlayer->EyeAngles();


	int eyeAttachmentIndex = LookupAttachment("vehicle_driver_eyes");
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachment( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );
	AngleMatrix( vehicleEyeAngles, vehicleEyePosToWorld );


	//HACKHACK -- use activity instead of discrete sequence number
	C_BaseAnimating *ent = dynamic_cast<C_BaseAnimating*>(GetVehicleEnt());
	if ( ent )
	{
		int sequence = ent->GetSequence();
		bool changed = sequence != m_nPreviousSequence;

		if ( sequence == m_nEnterSequence || sequence == m_nExitSequence )
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
		else if ( m_nPreviousSequence == m_nEnterSequence && changed )
		{
			QAngle zero;
  			zero.Init();
  			zero.y = 90;
  			engine->SetViewAngles( zero );
			*pAbsAngles = vehicleEyeAngles;
		}
		m_nPreviousSequence = sequence;
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
void C_PropCrane::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// FIXME: Need something a better long-term, this fixes the buggy.
	flZNear = 6;
}

	
//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------
void C_PropCrane::DrawHudElements( )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropCrane::OnEnteredVehicle( C_BasePlayer *pPlayer )
{
}
