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

class CBuyZone : public CBaseTrigger
{
public:
	DECLARE_CLASS( CBuyZone, CBaseTrigger );
	DECLARE_DATADESC();

	CBuyZone();
	void Spawn();
	void EXPORT BuyZoneTouch( CBaseEntity* pOther );
};


LINK_ENTITY_TO_CLASS( func_buy_zone, CBuyZone );

BEGIN_DATADESC( CBuyZone )
	DEFINE_FUNCTION( CBuyZone, BuyZoneTouch )
END_DATADESC()


CBuyZone::CBuyZone()
{
}


void CBuyZone::Spawn()
{
	InitTrigger();
	SetTouch( &CBuyZone::BuyZoneTouch );
}

	
void CBuyZone::BuyZoneTouch( CBaseEntity* pOther )
{
	CCSPlayer *p = dynamic_cast< CCSPlayer* >( pOther );
	if ( p )
	{
		if ( p->GetTeamNumber() == GetTeamNumber() )
		{
			p->m_bInBuyZone = true;
			p->m_flBuyZonePulseTime = gpGlobals->curtime;
		}
	}
}

