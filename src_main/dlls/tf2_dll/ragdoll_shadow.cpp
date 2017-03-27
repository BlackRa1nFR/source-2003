//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Resource chunks
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "ragdoll_shadow.h"
#include "tf_player.h"
#include "sendproxy.h"

// FIXME Hook up a real player standin
static char *sRagdollShadowModel = "models/player/human_commando.mdl";


IMPLEMENT_SERVERCLASS_ST( CRagdollShadow, DT_RagdollShadow )
	SendPropInt( SENDINFO( m_nPlayer ), 10, SPROP_UNSIGNED ),

	SendPropExclude( "DT_BaseEntity", "m_angAbsRotation[0]" ),
	SendPropExclude( "DT_BaseEntity", "m_angAbsRotation[1]" ),
	SendPropExclude( "DT_BaseEntity", "m_angAbsRotation[2]" ),

END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( ragdoll_shadow, CRagdollShadow );
PRECACHE_REGISTER( ragdoll_shadow );

CRagdollShadow::CRagdollShadow( void )
{
	m_pPlayer = NULL;
	m_nPlayer = 0;

	NetworkStateManualMode( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRagdollShadow::Spawn( )
{
	// Init value & model
	if ( m_pPlayer )
	{
		SetModelName( m_pPlayer->GetModelName() );
	}
	else
	{
		SetModelName( MAKE_STRING( sRagdollShadowModel ) );
	}

	BaseClass::Spawn();

	// Create the object in the physics system
	IPhysicsObject *pPhysics = VPhysicsInitNormal( SOLID_VPHYSICS, FSOLID_NOT_SOLID, false );
//	IPhysicsObject *pPhysics = VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
	
	// disable physics sounds on this object
	pPhysics->SetMaterialIndex( physprops->GetSurfaceIndex("default_silent") );

	UTIL_SetSize( this, Vector(-36,-36, 0), Vector(36,36,72) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : **ppSendTable - 
//			*recipient - 
//			*pvs - 
//			clientArea - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRagdollShadow::ShouldTransmit( const edict_t *recipient, 
  const void *pvs, int clientArea )
{
	// Always send to local player
	if ( Instance( recipient ) == GetOwnerEntity() )
		return true;

	return BaseClass::ShouldTransmit( recipient, pvs, clientArea );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRagdollShadow::Precache( void )
{
	engine->PrecacheModel( sRagdollShadowModel );
}

//-----------------------------------------------------------------------------
// Purpose: Create a resource chunk
//-----------------------------------------------------------------------------
CRagdollShadow *CRagdollShadow::Create( CBaseTFPlayer *player, const Vector& force )
{
	CRagdollShadow *pRagdollShadow = (CRagdollShadow*)CreateEntityByName("ragdoll_shadow");

	UTIL_SetOrigin( pRagdollShadow, player->GetAbsOrigin() );

	pRagdollShadow->m_pPlayer = player;
	pRagdollShadow->m_nPlayer = player->entindex();

	pRagdollShadow->Spawn();
	pRagdollShadow->SetAbsVelocity( force );
	pRagdollShadow->SetLocalAngles( vec3_angle );
	pRagdollShadow->SetLocalAngularVelocity( RandomAngle( -100, 100 ) );

	//pRagdollShadow->m_fEffects |= EF_NODRAW;
	pRagdollShadow->m_fEffects |= EF_NOSHADOW;

	pRagdollShadow->m_lifeState = LIFE_DYING;

	IPhysicsObject *pPhysicsObject = pRagdollShadow->VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		AngularImpulse tmp;
		QAngleToAngularImpulse( pRagdollShadow->GetLocalAngularVelocity(), tmp );
		pPhysicsObject->AddVelocity( &pRagdollShadow->GetAbsVelocity(), &tmp );
	}

	return pRagdollShadow;
}