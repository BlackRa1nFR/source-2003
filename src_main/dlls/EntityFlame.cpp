//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Flame entity to be attached to target entity. Serves two purposes:
//
//			1) An entity that can be placed by a level designer and triggered
//			   to ignite a target entity.
//
//			2) An entity that can be created at runtime to ignite a target entity.
//
//=============================================================================

#include "cbase.h"
#include "EntityFlame.h"
#include "fire.h"


BEGIN_DATADESC( CEntityFlame )

	DEFINE_KEYFIELD( CEntityFlame, m_flLifetime, FIELD_FLOAT, "lifetime" ),

	DEFINE_FIELD( CEntityFlame, m_flSize, FIELD_FLOAT ),
	DEFINE_FIELD( CEntityFlame, m_hEntAttached, FIELD_EHANDLE ),
	
	DEFINE_FUNCTION( CEntityFlame, FlameThink ),

	DEFINE_INPUTFUNC( CEntityFlame, FIELD_VOID, "Ignite", InputIgnite ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST( CEntityFlame, DT_EntityFlame )
	SendPropFloat( SENDINFO( m_flSize ), 16, SPROP_NOSCALE ),
	SendPropEHandle( SENDINFO( m_hEntAttached ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( entityflame, CEntityFlame );
LINK_ENTITY_TO_CLASS( env_entity_igniter, CEntityFlame );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEntityFlame::CEntityFlame( void )
{
	m_flSize		= 0.0f;
	m_flLifetime	= 0.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CEntityFlame::InputIgnite( inputdata_t &inputdata )
{
	if (m_target != NULL_STRING)
	{
		CBaseEntity *pTarget = NULL;
		while ((pTarget = gEntList.FindEntityGeneric(pTarget, STRING(m_target), this, inputdata.pActivator)) != NULL)
		{
			// Combat characters know how to catch themselves on fire.
			CBaseCombatCharacter *pBCC = pTarget->MyCombatCharacterPointer();
			if (pBCC)
			{
				// DVS TODO: consider promoting Ignite to CBaseEntity and doing everything here
				pBCC->Ignite(m_flLifetime);
			}
			// Everything else, we handle here.
			else
			{
				CEntityFlame *pFlame = CEntityFlame::Create(pTarget);
				if (pFlame)
				{
					pFlame->SetLifetime(m_flLifetime);
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates a flame and attaches it to a target entity.
// Input  : pTarget - 
//-----------------------------------------------------------------------------
CEntityFlame *CEntityFlame::Create( CBaseEntity *pTarget )
{
	CEntityFlame *pFlame = (CEntityFlame *) CreateEntityByName( "entityflame" );

	if ( pFlame == NULL )
		return NULL;

	/*IPhysicsObject *pPhysicsObject = pTarget->VPhysicsGetObject();

	if ( pPhysicsObject )
	{
		pPhysicsObject->
	}*/

	float size = ( pTarget->WorldAlignMaxs().y - pTarget->WorldAlignMins().y );

	if ( size < 8 )
		size = 8;

	UTIL_SetOrigin( pFlame, pTarget->GetAbsOrigin() );

	pFlame->m_flSize = size;
	pFlame->SetThink( &CEntityFlame::FlameThink );
	pFlame->SetNextThink( gpGlobals->curtime + 0.1f );

	pFlame->AttachToEntity( pTarget );
	pFlame->SetLifetime( 2.0f );

	//Send to the client even though we don't have a model
	pFlame->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	return pFlame;
}


//-----------------------------------------------------------------------------
// Purpose: Attaches the flame to an entity and moves with it
// Input  : pTarget - target entity to attach to
//-----------------------------------------------------------------------------
void CEntityFlame::AttachToEntity( CBaseEntity *pTarget )
{
	// For networking to the client.
	m_hEntAttached = pTarget;

	// So our heat emitter follows the entity around on the server.
	SetParent( pTarget );
	Relink();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lifetime - 
//-----------------------------------------------------------------------------
void CEntityFlame::SetLifetime( float lifetime )
{
	m_flLifetime = gpGlobals->curtime + lifetime;
}


//-----------------------------------------------------------------------------
// Purpose: Burn targets around us
//-----------------------------------------------------------------------------
void CEntityFlame::FlameThink( void )
{
	if ( m_flLifetime < gpGlobals->curtime )
	{
		SetThink( &CEntityFlame::SUB_Remove );
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;
	}

	RadiusDamage( CTakeDamageInfo( this, this, 4.0f, DMG_BURN ), GetAbsOrigin(), m_flSize/2, CLASS_NONE );
	FireSystem_AddHeatInRadius( GetAbsOrigin(), m_flSize/2, 2.0f );

	SetNextThink( gpGlobals->curtime + 0.1f );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEnt -	
//-----------------------------------------------------------------------------
void CreateEntityFlame(CBaseEntity *pEnt)
{
	CEntityFlame::Create( pEnt );
}
