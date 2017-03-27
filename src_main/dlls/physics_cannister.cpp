//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//=============================================================================

#include "cbase.h"
#include "basecombatcharacter.h"
#include "entityoutput.h"
#include "physics.h"
#include "explode.h"
#include "vphysics_interface.h"
#include "collisionutils.h"
#include "steamjet.h"
#include "eventqueue.h"
#include "soundflags.h"
#include "engine/IEngineSound.h"
#include "props.h"
#include "physics_cannister.h"
#include "globals.h"
#include "physics_saverestore.h"


#define SF_CANNISTER_ASLEEP		0x0001
#define SF_CANNISTER_EXPLODE	0x0002

BEGIN_SIMPLE_DATADESC( CThrustController )

	DEFINE_FIELD( CThrustController, m_thrustVector,	FIELD_VECTOR ),
	DEFINE_FIELD( CThrustController, m_torqueVector,	FIELD_VECTOR ),
	DEFINE_KEYFIELD( CThrustController, m_thrust,		FIELD_FLOAT, "thrust" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( physics_cannister, CPhysicsCannister );

BEGIN_DATADESC( CPhysicsCannister )

	DEFINE_OUTPUT( CPhysicsCannister,	m_onActivate, "OnActivate" ),

	DEFINE_FIELD( CPhysicsCannister,	m_thrustOrigin, FIELD_VECTOR ),	// this is a position, but in local space
	DEFINE_EMBEDDED( CPhysicsCannister, m_thruster ),
	DEFINE_PHYSPTR( CPhysicsCannister,	m_pController ),
	DEFINE_FIELD( CPhysicsCannister,	m_pJet, FIELD_CLASSPTR ),
	DEFINE_FIELD( CPhysicsCannister,	m_active, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( CPhysicsCannister, m_thrustTime, FIELD_FLOAT, "fuel" ),
	DEFINE_KEYFIELD( CPhysicsCannister, m_damage, FIELD_FLOAT, "expdamage" ),
	DEFINE_KEYFIELD( CPhysicsCannister, m_damageRadius, FIELD_FLOAT, "expradius" ),
	DEFINE_FIELD( CPhysicsCannister,	m_activateTime, FIELD_TIME ),
	DEFINE_KEYFIELD( CPhysicsCannister, m_gasSound, FIELD_SOUNDNAME, "gassound" ),
	DEFINE_FIELD( CPhysicsCannister,	m_bFired, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( CPhysicsCannister, FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC( CPhysicsCannister, FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC( CPhysicsCannister, FIELD_VOID, "Explode", InputExplode ),
	DEFINE_INPUTFUNC( CPhysicsCannister, FIELD_VOID, "Wake", InputWake ),

	DEFINE_THINKFUNC( CPhysicsCannister, BeginShutdownThink ),
	DEFINE_ENTITYFUNC( CPhysicsCannister, ExplodeTouch ),

END_DATADESC()

void CPhysicsCannister::Spawn( void )
{
	Precache();
	SetModel( STRING(GetModelName()) );
	SetBloodColor( DONT_BLEED );

	AddSolidFlags( FSOLID_CUSTOMRAYTEST );
	m_takedamage = DAMAGE_YES;
	SetNextThink( TICK_NEVER_THINK );

	if ( m_iHealth <= 0 )
		m_iHealth = 25;

	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	m_flCycle = 0;
	m_bFired = false;

	Relink();

	// not thrusting
	m_active = false;

	CreateVPhysics();
}

void CPhysicsCannister::OnRestore()
{
	BaseClass::OnRestore();
	if ( m_pController )
	{
		m_pController->SetEventHandler( &m_thruster );
	}
}

bool CPhysicsCannister::CreateVPhysics()
{
	bool asleep = HasSpawnFlags(SF_CANNISTER_ASLEEP);

	VPhysicsInitNormal( SOLID_VPHYSICS, 0, asleep );

	// only send data when physics moves this object
	NetworkStateManualMode( true );

	return true;
}

bool CPhysicsCannister::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	if ( !IsBoxIntersectingRay( GetAbsMins(), GetAbsMaxs(), ray.m_Start, ray.m_Delta ) )
		return false;
	
	return BaseClass::TestCollision( ray, mask, trace );
}

Vector CPhysicsCannister::CalcLocalThrust( const Vector &offset )
{
	matrix3x4_t nozzleMatrix;
	Vector thrustDirection;

	GetAttachment( LookupAttachment("nozzle"), nozzleMatrix );
	MatrixGetColumn( nozzleMatrix, 2, thrustDirection );
	MatrixGetColumn( nozzleMatrix, 3, m_thrustOrigin );
	thrustDirection = -5*thrustDirection + offset;
	VectorNormalize( thrustDirection );
	return thrustDirection;
}


CPhysicsCannister::~CPhysicsCannister( void )
{
}

void CPhysicsCannister::Precache( void )
{
	PropBreakablePrecacheAll( GetModelName() );
	if ( m_gasSound != NULL_STRING )
	{
		enginesound->PrecacheSound( STRING(m_gasSound) );
	}
	BaseClass::Precache();
}

int CPhysicsCannister::OnTakeDamage( const CTakeDamageInfo &info )
{
	// HACKHACK: Shouldn't g_vecAttackDir be a parameter to this function?
	if ( !m_takedamage )
		return 0;

	if ( !m_active )
	{
		m_iHealth -= info.GetDamage();
		if ( m_iHealth < 0 )
		{
			Explode();
		}
		else
		{
			// explosions that don't destroy will activate
			// 50% of the time blunt damage will activate as well
			if ( (info.GetDamageType() & DMG_BLAST) ||
				( (info.GetDamageType() & (DMG_CLUB|DMG_SLASH|DMG_CRUSH) ) && random->RandomInt(1,100) < 50 ) )
			{
				CannisterActivate( info.GetAttacker(), g_vecAttackDir );
			}
		}
		return 1;
	}

	if ( (gpGlobals->curtime - m_activateTime) <= 0.1 )
		return 0;

	if ( info.GetDamageType() & (DMG_BULLET|DMG_BUCKSHOT|DMG_BURN|DMG_BLAST) )
	{
		Explode();
	}

	return 0;
}


void CPhysicsCannister::TraceAttack( const CTakeDamageInfo &info, const Vector &dir, trace_t *ptr )
{
	if ( !m_active && ptr->hitgroup != 0 )
	{
		Vector direction = -dir;
		direction.z -= 5;
		VectorNormalize( direction );
		CannisterActivate( info.GetAttacker(), direction );
	}
	BaseClass::TraceAttack( info, dir, ptr );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::CannisterActivate( CBaseEntity *pActivator, const Vector &thrustOffset )
{
	// already active or spent
	if ( m_active || !m_thrustTime )
	{
		return;
	}

	Vector thrustDirection = CalcLocalThrust( thrustOffset );
	m_onActivate.FireOutput( pActivator, this, 0 );
	m_thruster.CalcThrust( m_thrustOrigin, thrustDirection, VPhysicsGetObject() );
	m_pController = physenv->CreateMotionController( &m_thruster );
	IPhysicsObject *pPhys = VPhysicsGetObject();
	m_pController->AttachObject( pPhys );
	// Make sure the object is simulated
	pPhys->Wake();

	m_active = true;
	m_activateTime = gpGlobals->curtime;
	SetNextThink( gpGlobals->curtime + m_thrustTime );
	SetThink( &CPhysicsCannister::BeginShutdownThink );

	QAngle angles;
	VectorAngles( -thrustDirection, angles );
	m_pJet = dynamic_cast<CSteamJet *>( CBaseEntity::Create( "env_steam", m_thrustOrigin, angles, this ) );
	m_pJet->SetParent( this );

	float extra = m_thruster.m_thrust * (1/5000.f);
	extra = clamp( extra, 0, 1 );

	m_pJet->m_SpreadSpeed = 15 * m_thruster.m_thrust * 0.001;
	m_pJet->m_Speed = 128 + 100 * extra;
	m_pJet->m_StartSize = 10;
	m_pJet->m_EndSize = 25;

	m_pJet->m_Rate = 52 + (int)extra*20;
	m_pJet->m_JetLength = 64;
	m_pJet->m_clrRender = m_clrRender;

	m_pJet->Use( this, this, USE_ON, 1 );
	if ( m_gasSound != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );
		EmitSound( filter, entindex(), CHAN_ITEM, STRING(m_gasSound), 1.0, ATTN_NORM );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The cannister's been fired by a weapon, so it should stay pretty accurate
//-----------------------------------------------------------------------------
void CPhysicsCannister::CannisterFire( CBaseEntity *pActivator )
{
	m_bFired = true;

	// Increase thrust
	m_thruster.m_thrust *= 4;

	// Only last a short time
	m_thrustTime = 10.0;

	// Explode on contact
	SetTouch( &CPhysicsCannister::ExplodeTouch );

	CannisterActivate( pActivator, vec3_origin );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for activating the cannister.
//-----------------------------------------------------------------------------
void CPhysicsCannister::InputActivate( inputdata_t &data )
{
	CannisterActivate( data.pActivator, Vector(0,0.1,-0.25) );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for deactivating the cannister.
//-----------------------------------------------------------------------------
void CPhysicsCannister::InputDeactivate(inputdata_t &data)
{
	Deactivate();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for making the cannister go boom.
//-----------------------------------------------------------------------------
void CPhysicsCannister::InputExplode(inputdata_t &data)
{
	Explode();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for waking up the cannister if it is sleeping.
//-----------------------------------------------------------------------------
void CPhysicsCannister::InputWake( inputdata_t &data )
{
	IPhysicsObject *pPhys = VPhysicsGetObject();
	if ( pPhys != NULL )
	{
		pPhys->Wake();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::Deactivate(void)
{
	if ( !m_pController )
		return;

	m_pController->DetachObject( VPhysicsGetObject() );
	physenv->DestroyMotionController( m_pController );
	m_pController = NULL;
	SetNextThink( TICK_NEVER_THINK );
	m_thrustTime = 0;
	m_active = false;
	if ( m_pJet )
	{
		ShutdownJet();
	}
	if ( m_gasSound != NULL_STRING )
	{
		StopSound( entindex(), CHAN_ITEM, STRING(m_gasSound) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::Explode(void)
{
	// don't recurse
	m_takedamage = 0;
	Deactivate();

	Vector velocity;
	AngularImpulse angVelocity;
	IPhysicsObject *pPhysics = VPhysicsGetObject();
	pPhysics->GetVelocity( &velocity, &angVelocity );
	PropBreakableCreateAll( GetModelIndex(), pPhysics, GetAbsOrigin(), GetAbsAngles(), velocity, angVelocity, 1.0, 20, COLLISION_GROUP_DEBRIS );
	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), NULL, m_damage, 0, true );
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Explode when I next hit a damageable entity
//-----------------------------------------------------------------------------
void CPhysicsCannister::ExplodeTouch( CBaseEntity *pOther )
{
	if ( !pOther->m_takedamage )
		return;

	Explode();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	if ( m_bFired && m_active )
	{
		int otherIndex = !index;
		CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
		if ( pEvent->deltaCollisionTime < 0.5 && (pHitEntity == this) )
			return;

		// If we hit hard enough. explode
		if ( pEvent->collisionSpeed > 1000 )
		{
			Explode();
			return;
		}
	}

	BaseClass::VPhysicsCollision( index, pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsCannister::ShutdownJet( void )
{
	g_EventQueue.AddEvent( m_pJet, "kill", 5, NULL, NULL );

	m_pJet->m_bEmit = false;
	m_pJet->m_Rate = 0;
	m_pJet = NULL;
	SetNextThink( TICK_NEVER_THINK );
}

// The think just shuts the cannister down
void CPhysicsCannister::BeginShutdownThink( void )
{
	Deactivate();
}


