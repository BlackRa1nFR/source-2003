//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef RAGDOLL_SHADOW_H
#define RAGDOLL_SHADOW_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"

class CBaseTFPlayer;
class IPhysicsObject;

//-----------------------------------------------------------------------------
// Purpose: A shadow object used to bound the position of a player ragdoll
//-----------------------------------------------------------------------------
class CRagdollShadow : public CBaseProp
{
	DECLARE_CLASS( CRagdollShadow, CBaseProp );
public:
	DECLARE_SERVERCLASS();

	CRagdollShadow( void ) ;

	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics )
	{
		NetworkStateChanged();
		BaseClass::VPhysicsUpdate( pPhysics );
	}

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual bool	ShouldTransmit( const edict_t *recipient, 
						  const void *pvs, int clientArea );

	static CRagdollShadow *Create( CBaseTFPlayer *player, const Vector& force );

public:
	CBaseTFPlayer *m_pPlayer;
	CNetworkVar( int, m_nPlayer );
};

#endif // RAGDOLL_SHADOW_H
