//=========== (C) Copyright 2001 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Base class for simple projectiles
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "CBaseSpriteProjectile.h"

LINK_ENTITY_TO_CLASS( baseprojectile, CBaseSpriteProjectile );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CBaseSpriteProjectile )

	DEFINE_FIELD( CBaseSpriteProjectile, m_iDmg,		FIELD_INTEGER ),
	DEFINE_FIELD( CBaseSpriteProjectile, m_iDmgType,	FIELD_INTEGER ),

END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
void CBaseSpriteProjectile::Spawn(	char *pszModel,
								const Vector &vecOrigin,
								const Vector &vecVelocity,
								edict_t *pOwner,
								MoveType_t	iMovetype,
								MoveCollide_t nMoveCollide,
								int	iDamage,
								int iDamageType )
{
	Precache();

	SetSolid( SOLID_BBOX );
	SetModel( pszModel );

	UTIL_SetSize( this, vec3_origin, vec3_origin );

	m_iDmg = iDamage;
	m_iDmgType = iDamageType;

	SetMoveType( iMovetype, nMoveCollide );

	UTIL_SetOrigin( this, vecOrigin );
	SetAbsVelocity( vecVelocity );

	SetOwnerEntity( Instance( pOwner ) );
}


//---------------------------------------------------------
//---------------------------------------------------------
void CBaseSpriteProjectile::Touch( CBaseEntity *pOther )
{
	CBaseEntity *pOwner;

	pOwner = GetOwnerEntity();

	if( !pOwner )
	{
		pOwner = this;
	}

	trace_t	tr = BaseClass::GetTouchTrace( );

	CTakeDamageInfo info( this, pOwner, m_iDmg, m_iDmgType );
	GuessDamageForce( &info, (tr.endpos - tr.startpos), tr.endpos );
	pOther->TakeDamage( info );
	
	UTIL_Remove( this );
}
