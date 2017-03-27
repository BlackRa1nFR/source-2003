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

#ifndef	GRENADESCANNER_H
#define	GRENADESCANNER_H

#include "basegrenade_shared.h"

class SmokeTrail;

class CGrenadeScanner : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenadeScanner, CBaseGrenade );

	SmokeTrail*	m_pSmokeTrail;

	void		Spawn( void );
	void		Precache( void );
	void		AimThink( void );
	void 		GrenadeScannerTouch( CBaseEntity *pOther );
	void		Event_Killed( const CTakeDamageInfo &info );

public:
	void EXPORT				Detonate(void);
	CGrenadeScanner(void);

	DECLARE_DATADESC();
};

#endif	//GRENADESCANNER_H
