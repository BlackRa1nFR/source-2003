//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Projectile shot by wasteland scanner 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	GRENADEPATHFOLLOWER_H
#define	GRENADEPATHFOLLOWER_H

#include "basegrenade_shared.h"

class SmokeTrail;

class CGrenadePathfollower : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenadePathfollower, CBaseGrenade );

	static CGrenadePathfollower* CreateGrenadePathfollower( string_t sModelName, string_t sFlySound, const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner );

	SmokeTrail*		m_pSmokeTrail;
	CBaseEntity*	m_pPathTarget;				// path corner we are heading towards
	float			m_flFlySpeed;
	string_t		m_sFlySound;
	float			m_flNextFlySoundTime;

	Class_T			Classify( void);
	void			Spawn( void );
	void			AimThink( void );
	void 			GrenadeTouch( CBaseEntity *pOther );
	void			Event_Killed( const CTakeDamageInfo &info );
	void			Launch( float flLaunchSpeed, string_t sPathCornerName);
	void			PlayFlySound(void);

	void EXPORT		Detonate(void);

	CGrenadePathfollower(void);
	~CGrenadePathfollower(void);

	DECLARE_DATADESC();
};

#endif	//GRENADEPATHFOLLOWER_H
