//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Tripmine
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef	TRIPWIRE_H
#define	TRIPWIRE_H

#ifdef _WIN32
#pragma once
#endif

#include "basegrenade_shared.h"

class CRopeKeyframe;

// ####################################################################
//   CTripwireHook
//
//		This is what the tripwire shoots out at the end of the rope
// ####################################################################
class CTripwireHook : public CBaseAnimating
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CTripwireHook, CBaseAnimating );

	EHANDLE m_hGrenade;
	bool	m_bAttached;

	void Spawn( void );
	void Precache( void );
	bool CreateVPhysics( void );
	void EndTouch( CBaseEntity *pOther );
	void SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
};

class CTripwireGrenade : public CBaseGrenade
{
public:
	DECLARE_CLASS( CTripwireGrenade, CBaseGrenade );

	CTripwireGrenade();
	void Spawn( void );
	void Precache( void );

	int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	
	void WarningThink( void );
	void PowerupThink( void );
	void RopeBreakThink( void );
	void FireThink( void );
	void Event_Killed( const CTakeDamageInfo &info );
	void Attach( void );

	void MakeRope( void );
	void BreakRope( void );
	void ShakeRope( void );
	void FireMissile(const Vector &vTargetPos);

private:
	float			m_flPowerUp;
	Vector			m_vecDir;

	int				m_nMissileCount;

	Vector			m_vTargetPos;
	Vector			m_vTargetOffset;

	CRopeKeyframe*	m_pRope;
	CTripwireHook*  m_pHook;

	DECLARE_DATADESC();
};

#endif	//TRIPWIRE_H
