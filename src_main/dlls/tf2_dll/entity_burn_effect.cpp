//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entity_burn_effect.h"



IMPLEMENT_SERVERCLASS_ST_NOBASE( CEntityBurnEffect, DT_EntityBurnEffect )
	SendPropEHandle( SENDINFO( m_hBurningEntity ) )
END_SEND_TABLE()


LINK_ENTITY_TO_CLASS( entity_burn_effect, CEntityBurnEffect );


CEntityBurnEffect* CEntityBurnEffect::Create( CBaseEntity *pBurningEntity )
{
	CEntityBurnEffect *pEffect = (CEntityBurnEffect*)CreateEntityByName( "entity_burn_effect" );
	if ( pEffect )
	{
		pEffect->m_hBurningEntity = pBurningEntity;
		return pEffect;
	}
	else
	{
		return NULL;
	}
}


bool CEntityBurnEffect::ShouldTransmit( 
	const edict_t *recipient, 
	const void *pvs, 
	int clientArea
	)
{
	CBaseEntity *pEnt = m_hBurningEntity;
	if ( pEnt )
		return pEnt->ShouldTransmit( recipient, pvs, clientArea );
	else
		return false;
}



