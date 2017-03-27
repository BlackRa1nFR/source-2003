//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:			A gib is a chunk of a body, or a piece of wood/metal/rocks/etc.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "baseanimating.h"
#include "Sprite.h"

extern CBaseEntity *CreateRagGib( const char *szModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecForce );

#ifndef GIB_H
#define GIB_H
#pragma once 

#define GERMAN_GIB_COUNT		4
#define	HUMAN_GIB_COUNT			6
#define ALIEN_GIB_COUNT			4

enum GibType_e
{
	GIB_HUMAN,
	GIB_ALIEN,
};

class CGib : public CBaseAnimating
{
public:
	DECLARE_CLASS( CGib, CBaseAnimating );

	void Spawn( const char *szGibModel );
	void InitGib( CBaseEntity *pVictim, float fMaxVelocity, float fMinVelocity );
	void BounceGibTouch ( CBaseEntity *pOther );
	void StickyGibTouch ( CBaseEntity *pOther );
	void WaitTillLand( void );
	void LimitVelocity( void );

	virtual int	ObjectCaps( void ) { return (BaseClass :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }
	static	void SpawnHeadGib( CBaseEntity *pVictim );
	static	void SpawnRandomGibs( CBaseEntity *pVictim, int cGibs, GibType_e eGibType );
	static  void SpawnStickyGibs( CBaseEntity *pVictim, Vector vecOrigin, int cGibs );
	static	void SpawnSpecificGibs( CBaseEntity *pVictim, int nNumGibs, float fMaxVelocity, float fMinVelocity, const char* cModelName, float flLifetime = 25);

	void SetSprite( CBaseEntity *pSprite )
	{
		m_hSprite = pSprite;	
	}

	CBaseEntity *GetSprite( void )
	{
		return m_hSprite.Get();
	}

	DECLARE_DATADESC();

public:
	void SetBloodColor( int nBloodColor );

	int		m_cBloodDecals;
	int		m_material;
	float	m_lifeTime;

private:
	// A little piece of duplicated code
	void AdjustVelocityBasedOnHealth( int nHealth, Vector &vecVelocity );
	int		m_bloodColor;

	EHANDLE m_hSprite;
};

class CRagGib : public CBaseAnimating
{
public:
	DECLARE_CLASS( CRagGib, CBaseAnimating );

	void Spawn( const char *szModel, const Vector &vecOrigin, const Vector &vecForce );
};


#endif	//GIB_H
