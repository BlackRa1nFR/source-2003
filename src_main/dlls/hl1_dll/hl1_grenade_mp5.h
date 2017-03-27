//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Projectile shot from the MP5 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	GRENADEMP5_H
#define	GRENADEMP5_H

#include "hl1_basegrenade.h"

#define	MAX_MP5_NO_COLLIDE_TIME 0.2

class SmokeTrail;
class CWeaponMP5;

class CGrenadeMP5 : public CHL1BaseGrenade
{
	DECLARE_CLASS( CGrenadeMP5, CHL1BaseGrenade );
public:

	float		m_fSpawnTime;

	void		Spawn( void );
	void		Precache( void );
	void 		GrenadeMP5Touch( CBaseEntity *pOther );
	void		Event_Killed( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType );

public:
	void EXPORT				Detonate(void);

	DECLARE_DATADESC();
};

#endif	//GRENADEMP5_H
