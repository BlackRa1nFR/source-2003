//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Projectile shot from the AR2 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	GRENADEAR2_H
#define	GRENADEAR2_H

#include "basegrenade_shared.h"

#define	MAX_AR2_NO_COLLIDE_TIME 0.2

class SmokeTrail;
class CWeaponAR2;

class CGrenadeAR2 : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenadeAR2, CBaseGrenade );

	SmokeTrail			*m_pSmokeTrail;
	float				 m_fSpawnTime;

	void		Spawn( void );
	void		Precache( void );
	void 		GrenadeAR2Touch( CBaseEntity *pOther );
	void		GrenadeAR2Think( void );
	void		Event_Killed( const CTakeDamageInfo &info );

public:
	void EXPORT				Detonate(void);
	CGrenadeAR2(void);

	DECLARE_DATADESC();
};

#endif	//GRENADEAR2_H
