//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Projectile shot by mortar synth
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	GRENADEENERGY_H
#define	GRENADEENERGY_H

#include "basegrenade_shared.h"

class CGrenadeEnergy : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenadeEnergy, CBaseGrenade );

	static void Shoot( CBaseEntity* pOwner, const Vector &vStart, Vector vVelocity );

public:
	void		Spawn( void );
	void		Precache( void );
	void		Animate( void );
	void 		GrenadeEnergyTouch( CBaseEntity *pOther );
	void		Event_Killed( const CTakeDamageInfo &info );

	int			m_flMaxFrame;
	int			m_nEnergySprite;
	float		m_flLaunchTime;		// When was this thing launched

	void EXPORT				Detonate(void);

	DECLARE_DATADESC();
};

#endif	//GRENADEENERGY_H
