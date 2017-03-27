//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "triggers.h"
#include "cs_player.h"


//======================================
// Bomb target area
//
//

class CBombTarget : public CBaseTrigger
{
public:
	DECLARE_CLASS( CBombTarget, CBaseTrigger );
	DECLARE_DATADESC();

	void Spawn();
	void EXPORT BombTargetTouch( CBaseEntity* pOther );
	void EXPORT BombTargetUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};


LINK_ENTITY_TO_CLASS( func_bomb_target, CBombTarget );

BEGIN_DATADESC( CBombTarget )
	DEFINE_FUNCTION( CBombTarget, BombTargetTouch ),
	DEFINE_FUNCTION( CBombTarget, BombTargetUse ),
END_DATADESC()


void CBombTarget::Spawn()
{
	InitTrigger();
	SetTouch( &CBombTarget::BombTargetTouch );
	SetUse( &CBombTarget::BombTargetUse );
}

	
void CBombTarget::BombTargetTouch( CBaseEntity* pOther )
{
	CCSPlayer *p = dynamic_cast< CCSPlayer* >( pOther );
	if ( p )
	{
		if ( p->IsCarryingBomb() )
		{
			p->m_bInBombZone = true;
		}
	}
}


void CBombTarget::BombTargetUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//SUB_UseTargets( NULL, USE_TOGGLE, 0 );
}
