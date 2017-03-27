//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Things thrown from the hand 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	GRENADEBRICKBAT_H
#define	GRENADEBRICKBAT_H

#include "basegrenade_shared.h"

enum BrickbatAmmo_t;

class CGrenade_Brickbat : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenade_Brickbat, CBaseGrenade );

	virtual void	Spawn( void );
	virtual void	SpawnBrickbatWeapon( void );
	virtual void	Detonate( void ) { return;};
	virtual bool	CreateVPhysics();
	void			BrickbatTouch( CBaseEntity *pOther );
	void			BrickbatThink( void );

	BrickbatAmmo_t	m_nType;
	bool			m_bExplodes;
	bool			m_bBounceToFlat;	// Bouncing to flatten

public:
	DECLARE_DATADESC();
};

#endif	//GRENADEBRICKBAT_H
