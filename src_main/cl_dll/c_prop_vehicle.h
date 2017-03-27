//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef C_PROP_VEHICLE_H
#define C_PROP_VEHICLE_H
#pragma once

#include "IClientVehicle.h"

class C_PropVehicleDriveable : public C_BaseAnimating, public IClientVehicle
{

	DECLARE_CLASS( C_PropVehicleDriveable, C_BaseAnimating );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_PropVehicleDriveable();
	~C_PropVehicleDriveable();

public:

	// IClientVehicle overrides.
	virtual void OnEnteredVehicle( C_BasePlayer *pPlayer );
	virtual void GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles );
	virtual void GetVehicleFOV( float &flFOV ) { return; }
	virtual void DrawHudElements();
	virtual bool IsPassengerUsingStandardWeapons( int nRole = VEHICLE_DRIVER ) { return false; }
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd );
	virtual void DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );
	virtual C_BasePlayer* GetPassenger( int nRole );
	virtual int	GetPassengerRole( C_BasePlayer *pEnt );
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const;

public:

	// C_BaseEntity overrides.
	virtual IClientVehicle*	GetClientVehicle() { return this; }
	virtual C_BaseEntity	*GetVehicleEnt() { return this; }
	virtual void SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) {}
	virtual void ProcessMovement( C_BasePlayer *pPlayer, CMoveData *pMoveData ) {}
	virtual void FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move ) {}
	virtual bool IsPredicted() const { return false; }
	virtual void ItemPostFrame( C_BasePlayer *pPlayer ) {}
	virtual bool IsSelfAnimating() { return false; };

protected:

	virtual void RestrictView( float *pYawBounds, float *pPitchBounds, float *pRollBounds, QAngle &vecViewAngles );

protected:

	CHandle<C_BasePlayer>		m_hPlayer;
	int							m_nSpeed;
	int							m_nRPM;
	float						m_flThrottle;
	int							m_nBoostTimeLeft;
	int							m_nHasBoost;
	int							m_nScannerDisabledWeapons;
	int							m_nScannerDisabledVehicle;
	Vector						m_vecLookCrosshair;
	CInterpolatedVar<Vector>	m_iv_vecLookCrosshair;
	Vector						m_vecGunCrosshair;
	CInterpolatedVar<Vector>	m_iv_vecGunCrosshair;

	// timers/flags for flashing icons on hud
	int							m_iFlashTimer;
	bool						m_bLockedDim;
	bool						m_bLockedIcon;

	int							m_iScannerWepFlashTimer;
	bool						m_bScannerWepDim;
	bool						m_bScannerWepIcon;

	int							m_iScannerVehicleFlashTimer;
	bool						m_bScannerVehicleDim;
	bool						m_bScannerVehicleIcon;

	float						m_flSequenceChangeTime;
	bool						m_bEnterAnimOn;
	bool						m_bExitAnimOn;
	bool						m_bWasEntering;

	Vector						m_vecLastEyePos;
	Vector						m_vecLastEyeTarget;
	Vector						m_vecEyeSpeed;
	Vector						m_vecTargetSpeed;
};

#endif // C_PROP_VEHICLE_H