//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "entitylist.h"

//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission:public CPointEntity
{
public:
	DECLARE_CLASS( CInfoIntermission, CPointEntity );

	void Spawn( void );
	void Think( void );
};

void CInfoIntermission::Spawn( void )
{
	SetSolid( SOLID_NONE );
	m_fEffects |= EF_NODRAW;
	SetLocalAngles( vec3_angle );
	Relink();

	SetNextThink( gpGlobals->curtime + 2 );// let targets spawn !

}

void CInfoIntermission::Think ( void )
{
	CBaseEntity *pTarget;

	// find my target
	pTarget = gEntList.FindEntityByName( NULL, m_target, NULL );

	if ( pTarget )
	{
		Vector dir = pTarget->GetLocalOrigin() - GetLocalOrigin();
		VectorNormalize( dir );
		QAngle angles;
		VectorAngles( dir, angles );
		SetLocalAngles( angles );
	}
}

LINK_ENTITY_TO_CLASS( info_intermission, CInfoIntermission );
