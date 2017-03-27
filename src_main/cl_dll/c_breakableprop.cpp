//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "model_types.h"
#include "vcollide.h"
#include "vcollide_parse.h"
#include "solidsetdefaults.h"
#include "bone_setup.h"
#include "engine/ivmodelinfo.h"
#include "physics.h"
#include "c_breakableprop.h"

IMPLEMENT_CLIENTCLASS_DT(C_BreakableProp, DT_BreakableProp, CBreakableProp)
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BreakableProp::C_BreakableProp( void )
{
	m_pPhysicsObject = NULL;
	m_bClientsideOnly = false;
	m_fDeathTime = -1;
	m_takedamage = DAMAGE_YES;
}

//-----------------------------------------------------------------------------
// Purpose:		
//-----------------------------------------------------------------------------
C_BreakableProp::~C_BreakableProp( void )
{
	if ( m_bClientsideOnly )
	{
		VPhysicsDestroyObject();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create a clientside only c_breakable prop
//-----------------------------------------------------------------------------
bool C_BreakableProp::CreateClientsideProp( const char *pszModelName, Vector vecOrigin, Vector vecForceDir, AngularImpulse vecAngularImp )
{
	if ( !BaseClass::InitializeAsClientEntity( pszModelName, RENDER_GROUP_OPAQUE_ENTITY ) )
		return false;

	m_bClientsideOnly = true;
	SetNextThink( TICK_NEVER_THINK );

	SetAbsOrigin( vecOrigin );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	solid_t tmpSolid;
	PhysModelParseSolid( tmpSolid, this, GetModelIndex() );
	
	/*
	// FIXME: Allow scriptfile override, as per server

	if ( m_massScale > 0 )
	{
		float mass = tmpSolid.params.mass * m_massScale;
		mass = clamp( mass, 0.5, 1e6 );
		tmpSolid.params.mass = mass;
	}

	if ( m_inertiaScale > 0 )
	{
		tmpSolid.params.inertia *= m_inertiaScale;
		if ( tmpSolid.params.inertia < 0.5 )
			tmpSolid.params.inertia = 0.5;
	}

	PhysSolidOverride( tmpSolid, m_iszOverrideScript );
	*/

	m_pPhysicsObject = VPhysicsInitNormal( SOLID_VPHYSICS, 0, false, &tmpSolid );
	if ( m_pPhysicsObject )
	{
		float flForce = m_pPhysicsObject->GetMass();
		vecForceDir *= flForce;

		m_pPhysicsObject->ApplyForceOffset( vecForceDir, GetAbsOrigin() );
	}
	else
	{
		// failed to create a physics object
		Release();
		return false;
	}

	m_fDeathTime = gpGlobals->curtime + 10;
	SetNextClientThink( gpGlobals->curtime + 10 );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BreakableProp::ClientThink( void )
{
	if ( m_bClientsideOnly && m_fDeathTime >= 0 && m_fDeathTime <= gpGlobals->curtime )
	{
		Release();
	}
}

//-----------------------------------------------------------------------------
// Should we collide?
//-----------------------------------------------------------------------------
CollideType_t C_BreakableProp::ShouldCollide( )
{
	if ( m_bClientsideOnly )
		return ENTITY_SHOULD_RESPOND;

	return BaseClass::ShouldCollide();
}

