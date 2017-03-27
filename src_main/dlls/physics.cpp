//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface layer for ipion IVP physics.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================


#include "cbase.h"
#include "entitylist.h"
#include "vcollide_parse.h"
#include "soundenvelope.h"
#include "game.h"
#include "utlvector.h"
#include "init_factory.h"
#include "igamesystem.h"
#include "hierarchy.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "world.h"
#include "decals.h"
#include "physics_fx.h"
#include "vphysics_sound.h"
#include "movevars_shared.h"
#include "physics_saverestore.h"
#include "solidsetdefaults.h"
#include "tier0/vprof.h"
#include "engine/IStaticPropMgr.h"
#include "physics_prop_ragdoll.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar phys_timescale( "phys_timescale", "1" );
ConVar phys_speeds( "phys_speeds", "0" );
extern ConVar phys_rolling_drag;

// defined in phys_constraint
extern IPhysicsConstraintEvent *g_pConstraintEvents;


// local variables
static CEntityList *g_pShadowEntities = NULL;
static float g_PhysAverageSimTime;

// local routines
static IPhysicsObject *PhysCreateWorld( CBaseEntity *pWorld );
static void PhysFrame( float deltaTime );

//-----------------------------------------------------------------------------
// Purpose: A little cache of current objects making noises
//-----------------------------------------------------------------------------
struct friction_t
{
	CSoundPatch	*patch;
	CBaseEntity	*pObject;
	float		update;
};

enum
{
	TOUCH_START=0,
	TOUCH_END,
};

struct touchevent_t
{
	CBaseEntity *pEntity0;
	CBaseEntity *pEntity1;
	int			touchType;
	Vector		endPoint;
	Vector		normal;
};

struct damageevent_t
{
	CBaseEntity		*pEntity;
	IPhysicsObject	*pInflictorPhysics;
	Vector			savedVelocity;
	AngularImpulse	savedAngularVelocity;
	CTakeDamageInfo	info;
	bool			bRestoreVelocity;
};

enum
{
	COLLSTATE_ENABLED = 0,
	COLLSTATE_TRYDISABLE = 1,
	COLLSTATE_DISABLED = 2
};

struct penetrateevent_t
{
	EHANDLE			hEntity0;
	EHANDLE			hEntity1;
	float			startTime;
	float			timeStamp;
	int				collisionState;
};

const float FLUID_TIME_MAX = 2.0f; // keep track of last time hitting fluid for up to 2 seconds 
struct fluidevent_t
{
	CBaseEntity		*pEntity;
	float			impactTime;
};

class CCollisionEvent : public IPhysicsCollisionEvent, public IPhysicsCollisionSolver
{
public:
	CCollisionEvent();
	void PreCollision( vcollisionevent_t *pEvent );
	void PostCollision( vcollisionevent_t *pEvent );
	void Friction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit, IPhysicsCollisionData *pData );
	friction_t *FindFriction( CBaseEntity *pObject );
	void ShutdownFriction( friction_t &friction );
	void FrameUpdate();
	void LevelShutdown( void );
	void StartTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData );
	void EndTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData );
	void FluidStartTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid );
	void FluidEndTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid );
	void PostSimulationFrame();
	void ObjectEnterTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject );
	void ObjectLeaveTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject );

	void BufferTouchEvents( bool enable ) { m_bBufferTouchEvents = enable; }
	void AddDamageEvent( CBaseEntity *pEntity, const CTakeDamageInfo &info, IPhysicsObject *pInflictorPhysics, bool bRestoreVelocity, const Vector &savedVel, const AngularImpulse &savedAngVel );

	// IPhysicsCollisionSolver
	int ShouldCollide( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1 );
	int ShouldSolvePenetration( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1, float dt );

private:
	void UpdateFrictionSounds();
	void UpdateTouchEvents();
	void UpdateDamageEvents();
	void UpdatePenetrateEvents( void );
	void UpdateFluidEvents();
	void AddTouchEvent( CBaseEntity *pEntity0, CBaseEntity *pEntity1, int touchType, const Vector &point, const Vector &normal );
	penetrateevent_t &FindOrAddPenetrateEvent( CBaseEntity *pEntity0, CBaseEntity *pEntity1 );
	float DeltaTimeSinceLastFluid( CBaseEntity *pEntity );

	// make the call into the entity system
	void DispatchStartTouch( CBaseEntity *pEntity0, CBaseEntity *pEntity1, const Vector &point, const Vector &normal );
	void DispatchEndTouch( CBaseEntity *pEntity0, CBaseEntity *pEntity1 );
	
	friction_t					m_current[8];
	gamevcollisionevent_t		m_gameEvent;
	CUtlVector<touchevent_t>	m_touchEvents;
	CUtlVector<damageevent_t>	m_damageEvents;
	CUtlVector<penetrateevent_t> m_penetrateEvents;
	CUtlVector<fluidevent_t>	m_fluidEvents;
	bool						m_bBufferTouchEvents;
#if OSCILLATION_DETECT_EXPERIMENT
	float						m_percentFramesWithCollision;
	int							m_frameCollisions;
#endif
};

CCollisionEvent g_Collisions;

class CObjectEvent : public IPhysicsObjectEvent
{
public:
	// these can be used to optimize out queries on sleeping objects
	// Called when an object is woken after sleeping
	virtual void ObjectWake( IPhysicsObject *pObject ) {}
	// called when an object goes to sleep (no longer simulating)
	virtual void ObjectSleep( IPhysicsObject *pObject )
	{
		CBaseEntity *pEntity = (CBaseEntity *)pObject->GetGameData();
		if ( pEntity )
		{
			UTIL_Relink( pEntity );
		}
	}
};
CObjectEvent g_Objects;

class CPhysicsHook : public CBaseGameSystem
{
public:
	virtual bool Init();
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();
	virtual void FrameUpdatePostEntityThink( );

	bool ShouldSimulate()
	{
		return (physenv && !m_bPaused) ? true : false;
	}

	physicssound::soundlist_t m_impactSounds;

	bool m_bPaused;
};


CPhysicsHook	g_PhysicsHook;


//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* PhysicsGameSystem()
{
	return &g_PhysicsHook;
}


//-----------------------------------------------------------------------------
// Purpose: The physics hook callback implementations
//-----------------------------------------------------------------------------
bool CPhysicsHook::Init( void )
{
	factorylist_t factories;
	
	// Get the list of interface factories to extract the physics DLL's factory
	FactoryList_Retrieve( factories );

	if ( !factories.physicsFactory )
		return false;

	if (!(physics = (IPhysics *)factories.physicsFactory( VPHYSICS_INTERFACE_VERSION, NULL )) ||
		!(physcollision = (IPhysicsCollision *)factories.physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL )) ||
		!(physprops = (IPhysicsSurfaceProps *)factories.physicsFactory( VPHYSICS_SURFACEPROPS_INTERFACE_VERSION, NULL ))
		)
		return false;

	PhysParseSurfaceData( "scripts/surfaceproperties.txt", physprops, filesystem );

	return true;
}


// a little debug wrapper to help fix bugs when entity pointers get trashed
#if 0
struct physcheck_t
{
	IPhysicsObject *pPhys;
	char			string[512];
};

CUtlVector< physcheck_t > physCheck;

void PhysCheckAdd( IPhysicsObject *pPhys, const char *pString )
{
	physcheck_t tmp;
	tmp.pPhys = pPhys;
	Q_strncpy( tmp.string, pString ,sizeof(tmp.string));
	physCheck.AddToTail( tmp );
}

const char *PhysCheck( IPhysicsObject *pPhys )
{
	for ( int i = 0; i < physCheck.Size(); i++ )
	{
		if ( physCheck[i].pPhys == pPhys )
			return physCheck[i].string;
	}

	return "unknown";
}
#endif


void CPhysicsHook::LevelInitPreEntity() 
{
	physenv = physics->CreateEnvironment();
	physenv->EnableDeleteQueue( true );

	physenv->SetCollisionSolver( &g_Collisions );
	physenv->SetCollisionEventHandler( &g_Collisions );
	physenv->SetConstraintEventHandler( g_pConstraintEvents );
	physenv->SetObjectEventHandler( &g_Objects );
	
	physenv->SetSimulationTimestep( 0.015 ); // 15 ms per tick
	// HL Game gravity, not real-world gravity
	physenv->SetGravity( Vector( 0, 0, -sv_gravity.GetFloat() ) );
	g_PhysAverageSimTime = 0;

	g_PhysWorldObject = PhysCreateWorld( GetWorldEntity() );

	g_pShadowEntities = new CEntityList;
	m_bPaused = true;
}


void CPhysicsHook::LevelInitPostEntity() 
{
	m_bPaused = false;
}

void CPhysicsHook::LevelShutdownPreEntity() 
{
	if ( !physenv )
		return;
	physenv->SetQuickDelete( true );
}

void CPhysicsHook::LevelShutdownPostEntity() 
{
	if ( !physenv )
		return;

	g_pPhysSaveRestoreManager->ForgetAllModels();

	g_Collisions.LevelShutdown();

	physics->DestroyEnvironment( physenv );
	physenv = NULL;

	g_PhysWorldObject = NULL;

	delete g_pShadowEntities;
	g_pShadowEntities = NULL;
	m_impactSounds.RemoveAll();

}


// called after entities think
void CPhysicsHook::FrameUpdatePostEntityThink( ) 
{
	VPROF_BUDGET( "CPhysicsHook::FrameUpdatePostEntityThink", VPROF_BUDGETGROUP_PHYSICS );
	// update the physics simulation
	PhysFrame( gpGlobals->frametime );
	physicssound::PlayImpactSounds( m_impactSounds );
}


IPhysicsObject *PhysCreateWorld( CBaseEntity *pWorld )
{
	staticpropmgr->CreateVPhysicsRepresentations( physenv, &g_SolidSetup, pWorld );
	return PhysCreateWorld_Shared( pWorld, modelinfo->GetVCollide(1), g_PhysDefaultObjectParams );
}


// vehicle wheels can only collide with things that can't get stuck in them during game physics
// because they aren't in the game physics world at present
static bool WheelCollidesWith( IPhysicsObject *pObj, CBaseEntity *pEntity )
{
#if defined( TF2_DLL )
	if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_OBJECT )
		return false;
#endif

	if ( pEntity->GetMoveType() == MOVETYPE_PUSH || pEntity->GetMoveType() == MOVETYPE_VPHYSICS || pObj->IsStatic() )
		return true;

	if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS )
		return false;

	return false;
}

CCollisionEvent::CCollisionEvent()
{
#if OSCILLATION_DETECT_EXPERIMENT
	m_percentFramesWithCollision = 0;
#endif
}

int CCollisionEvent::ShouldCollide( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1 )
{
	CBaseEntity *pEntity0 = static_cast<CBaseEntity *>(pGameData0);
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *>(pGameData1);

	if ( !pEntity0 || !pEntity1 )
		return 1;

	unsigned short gameFlags0 = pObj0->GetGameFlags();
	unsigned short gameFlags1 = pObj1->GetGameFlags();

	if ( pEntity0 == pEntity1 )
	{
		// allow all-or-nothing per-entity disable
		if ( (gameFlags0 | gameFlags1) & FVPHYSICS_NO_SELF_COLLISIONS )
			return 0;

		// otherwise sub-object collisions must be explicitly disabled on a per-physics-object basis
		return 1;
	}

	// objects that are both constrained to the world don't collide with each other
	if ( (gameFlags0 & gameFlags1) & FVPHYSICS_CONSTRAINT_STATIC )
	{
		return 0;
	}

	// Special collision rules for vehicle wheels
	// Their entity collides with stuff using the normal rules, but they
	// have different rules than the vehicle body for various reasons.
	// sort of a hack because we don't have spheres to represent them in the game
	// world for speculative collisions.
	if ( pObj0->GetCallbackFlags() & CALLBACK_IS_VEHICLE_WHEEL )
	{
		if ( !WheelCollidesWith( pObj1, pEntity1 ) )
			return false;
	}
	if ( pObj1->GetCallbackFlags() & CALLBACK_IS_VEHICLE_WHEEL )
	{
		if ( !WheelCollidesWith( pObj0, pEntity0 ) )
			return false;
	}

	if ( pEntity0->ForceVPhysicsCollide( pEntity1 ) || pEntity1->ForceVPhysicsCollide( pEntity0 ) )
		return 1;

	if ( pEntity0->pev && pEntity1->pev )
	{
		// don't collide with your owner
		if ( pEntity0->GetOwnerEntity() == pEntity1 || pEntity1->GetOwnerEntity() == pEntity0 )
			return 0;
	}

	if ( pEntity0->GetMoveParent() || pEntity1->GetMoveParent() )
	{
		CBaseEntity *pParent0 = GetHighestParent( pEntity0 );
		CBaseEntity *pParent1 = GetHighestParent( pEntity1 );
		
		// NOTE: Don't let siblings/parents collide.  If you want this behavior, do it
		// with constraints, not hierarchy!
		if ( pParent0 == pParent1 )
			return 0;
		IPhysicsObject *p0 = pParent0->VPhysicsGetObject();
		IPhysicsObject *p1 = pParent1->VPhysicsGetObject();
		if ( p0 && p1 )
		{
			if ( !physenv->ShouldCollide( p0, p1 ) )
				return 0;
		}
	}

	int solid0 = pEntity0->GetSolid();
	int solid1 = pEntity1->GetSolid();
	int nSolidFlags0 = pEntity0->GetSolidFlags();
	int nSolidFlags1 = pEntity1->GetSolidFlags();

	int movetype0 = pEntity0->GetMoveType();
	int movetype1 = pEntity1->GetMoveType();

	// entities with non-physical move parents or entities with MOVETYPE_PUSH
	// are considered as "AI movers".  They are unchanged by collision; they exert
	// physics forces on the rest of the system.
	bool aiMove0 = (movetype0==MOVETYPE_PUSH || pEntity0->GetMoveParent()) ? true : false;
	bool aiMove1 = (movetype1==MOVETYPE_PUSH || pEntity1->GetMoveParent()) ? true : false;

	// AI movers don't collide with the world/static/pinned objects or other AI movers
	if ( (aiMove0 && !pObj1->IsMoveable()) ||
		(aiMove1 && !pObj0->IsMoveable()) ||
		(aiMove0 && aiMove1) )
		return 0;

	// BRJ 1/24/03
	// You can remove the assert if it's problematic; I *believe* this condition
	// should be met, but I'm not sure.
	Assert ( (solid0 != SOLID_NONE) && (solid1 != SOLID_NONE) );
	if ( (solid0 == SOLID_NONE) || (solid1 == SOLID_NONE) )
		return 0;

	// not solid doesn't collide with anything
	if ( (nSolidFlags0|nSolidFlags1) & FSOLID_NOT_SOLID )
	{
		// might be a vphysics trigger, collide with everything but "not solid"
		if ( pObj0->IsTrigger() && !(nSolidFlags1 & FSOLID_NOT_SOLID) )
			return 1;
		if ( pObj1->IsTrigger() && !(nSolidFlags0 & FSOLID_NOT_SOLID) )
			return 1;

		return 0;
	}
	
	if ( (nSolidFlags0 & FSOLID_TRIGGER) && 
		!(solid1 == SOLID_VPHYSICS || solid1 == SOLID_BSP || movetype1 == MOVETYPE_VPHYSICS) )
		return 0;

	if ( (nSolidFlags1 & FSOLID_TRIGGER) && 
		!(solid0 == SOLID_VPHYSICS || solid0 == SOLID_BSP || movetype0 == MOVETYPE_VPHYSICS) )
		return 0;

	if ( !g_pGameRules->ShouldCollide( pEntity0->GetCollisionGroup(), pEntity1->GetCollisionGroup() ) )
		return 0;

	// check contents
	if ( !(pObj0->GetContents() & pEntity1->PhysicsSolidMaskForEntity()) || !(pObj1->GetContents() & pEntity0->PhysicsSolidMaskForEntity()) )
		return 0;

	return 1;
}

static void ReportPenetration( CBaseEntity *pEntity, float duration )
{
	if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		if ( g_pDeveloper->GetInt() > 1 )
		{
			pEntity->m_debugOverlays |= OVERLAY_ABSBOX_BIT;
		}
		pEntity->AddTimedOverlay( "VPhysics Penetration Error!", duration );
	}
}

void CCollisionEvent::UpdatePenetrateEvents( void )
{
	for ( int i = m_penetrateEvents.Count()-1; i >= 0; --i )
	{
		if ( m_penetrateEvents[i].collisionState == COLLSTATE_TRYDISABLE )
		{
			CBaseEntity *pEntity0 = m_penetrateEvents[i].hEntity0;
			CBaseEntity *pEntity1 = m_penetrateEvents[i].hEntity1;
			if ( pEntity0 && pEntity1 )
			{
				IPhysicsObject *pObj0 = pEntity0->VPhysicsGetObject();
				if ( pObj0 )
				{
					PhysForceEntityToSleep( pEntity0, pObj0 );
				}
				IPhysicsObject *pObj1 = pEntity1->VPhysicsGetObject();
				if ( pObj1 )
				{
					PhysForceEntityToSleep( pEntity1, pObj1 );
				}
				m_penetrateEvents[i].collisionState = COLLSTATE_DISABLED;
				continue;
			}
			// missing entity or object, clear event
			m_penetrateEvents.FastRemove(i);
		}
		if ( gpGlobals->curtime - m_penetrateEvents[i].timeStamp > 1.0 )
		{
			if ( m_penetrateEvents[i].collisionState == COLLSTATE_DISABLED )
			{
				CBaseEntity *pEntity0 = m_penetrateEvents[i].hEntity0;
				CBaseEntity *pEntity1 = m_penetrateEvents[i].hEntity1;
				if ( pEntity0 && pEntity1 )
				{
					IPhysicsObject *pObj0 = pEntity0->VPhysicsGetObject();
					IPhysicsObject *pObj1 = pEntity1->VPhysicsGetObject();
					if ( pObj0 && pObj1 )
					{
						m_penetrateEvents[i].collisionState = COLLSTATE_ENABLED;
						continue;
					}
				}
			}
			// haven't penetrated for 1 second, so remove
			m_penetrateEvents.FastRemove(i);
		}
	}
}

penetrateevent_t &CCollisionEvent::FindOrAddPenetrateEvent( CBaseEntity *pEntity0, CBaseEntity *pEntity1 )
{
	int index = -1;
	for ( int i = m_penetrateEvents.Count()-1; i >= 0; --i )
	{
		if ( m_penetrateEvents[i].hEntity0.Get() == pEntity0 && m_penetrateEvents[i].hEntity1.Get() == pEntity1 )
		{
			index = i;
			break;
		}
	}
	if ( index < 0 )
	{
		index = m_penetrateEvents.AddToTail();
		penetrateevent_t &event = m_penetrateEvents[index];
		event.hEntity0 = pEntity0;
		event.hEntity1 = pEntity1;
		event.startTime = gpGlobals->curtime;
		event.collisionState = COLLSTATE_ENABLED;
	}
	penetrateevent_t &event = m_penetrateEvents[index];
	event.timeStamp = gpGlobals->curtime;
	return event;
}


int CCollisionEvent::ShouldSolvePenetration( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1, float dt )
{
	// solve it yourself here and return 0, or have the default implementation do it
	CBaseEntity *pEntity0 = static_cast<CBaseEntity *>(pGameData0);
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *>(pGameData1);
	if ( pEntity0 > pEntity1 )
	{
		// swap sort
		CBaseEntity *pTmp = pEntity0;
		pEntity0 = pEntity1;
		pEntity1 = pTmp;
	}
	penetrateevent_t &event = FindOrAddPenetrateEvent( pEntity0, pEntity1 );
	float eventTime = gpGlobals->curtime - event.startTime;

#if _DEBUG
	const char *pName1 = STRING(pEntity0->GetModelName());
	const char *pName2 = STRING(pEntity1->GetModelName());
	DevMsg(2, "***Inter-penetration between %s AND %s (%.0f, %.0f)\n", pName1?pName1:"(null)", pName2?pName2:"(null)", gpGlobals->curtime, eventTime );
#endif

	if ( eventTime > 3 )
	{
		// two objects have been stuck for more than 3 seconds, try disabling simulation
		event.collisionState = COLLSTATE_TRYDISABLE;

		if ( g_pDeveloper->GetInt() )
		{
			ReportPenetration( pEntity0, 10 );
			ReportPenetration( pEntity1, 10 );
		}
		event.startTime = gpGlobals->curtime;
	}


	return 1;
}


void CCollisionEvent::FluidStartTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid ) 
{
	if ( ( pObject == NULL ) || ( pFluid == NULL ) )
		return;

	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	
	if ( pEntity )
	{
		float timeSinceLastCollision = DeltaTimeSinceLastFluid( pEntity );
		
		if ( timeSinceLastCollision < 0.5f )
			return;

		// UNDONE: Use this for splash logic instead?
		// UNDONE: Use angular term too - push splashes in rotAxs cross normal direction?
		Vector normal;
		float dist;
		pFluid->GetSurfacePlane( &normal, &dist );
		Vector vel;
		AngularImpulse angVel;
		pObject->GetVelocity( &vel, &angVel );
		Vector unitVel = vel;
		VectorNormalize( unitVel );
		
		// normal points out of the surface, we want the direction that points in
		float dragScale = pFluid->GetDensity() * physenv->GetSimulationTimestep();
		normal = -normal;
		float linearScale = 0.5 * DotProduct( unitVel, normal ) * pObject->CalculateLinearDrag( normal ) * dragScale;
		linearScale = clamp( linearScale, 0, 1 );
		vel *= -linearScale;

		// UNDONE: Figure out how much of the surface area has crossed the water surface and scale angScale by that
		// For now assume 25%
		Vector rotAxis = angVel;
		VectorNormalize(rotAxis);
		float angScale = 0.25 * pObject->CalculateAngularDrag( angVel ) * dragScale;
		angScale = clamp( angScale, 0, 1 );
		angVel *= -angScale;
		
		// compute the splash before we modify the velocity
		PhysicsSplash( pFluid, pObject, pEntity );

		// now damp out some motion toward the surface
		pObject->AddVelocity( &vel, &angVel );
	}
}

void CCollisionEvent::FluidEndTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid ) 
{
	if ( ( pObject == NULL ) || ( pFluid == NULL ) )
		return;

	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	
	if ( pEntity )
	{
		float timeSinceLastCollision = DeltaTimeSinceLastFluid( pEntity );
		
		if ( timeSinceLastCollision < 0.5f )
			return;

		PhysicsSplash( pFluid, pObject, pEntity );
	}
}

class CSkipKeys : public IVPhysicsKeyHandler
{
public:
	virtual void ParseKeyValue( void *pData, const char *pKey, const char *pValue ) {}
	virtual void SetDefaults( void *pData ) {}
};

void PhysSolidOverride( solid_t &solid, string_t overrideScript )
{
	if ( overrideScript != NULL_STRING)
	{
		// parser destroys this data
		bool collisions = solid.params.enableCollisions;

		char pTmpString[4096];

		// write a header for a solid_t
		Q_strncpy( pTmpString, "solid { ", sizeof(pTmpString) );

		// suck out the comma delimited tokens and turn them into quoted key/values
		char szToken[256];
		const char *pStr = nexttoken(szToken, STRING(overrideScript), ',');
		while ( szToken[0] != 0 )
		{
			Q_strncat( pTmpString, "\"", sizeof(pTmpString) );
			Q_strncat( pTmpString, szToken, sizeof(pTmpString) );
			Q_strncat( pTmpString, "\" ", sizeof(pTmpString) );
			pStr = nexttoken(szToken, pStr, ',');
		}
		// terminate the script
		Q_strncat( pTmpString, "}", sizeof(pTmpString) );

		// parse that sucker
		IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pTmpString );
		CSkipKeys tmp;
		pParse->ParseSolid( &solid, &tmp );
		physcollision->VPhysicsKeyParserDestroy( pParse );

		// parser destroys this data
		solid.params.enableCollisions = collisions;
	}
}


typedef void (*EntityCallbackFunction) ( CBaseEntity *pEntity );

void IterateActivePhysicsEntities( EntityCallbackFunction func )
{
	int activeCount = physenv->GetActiveObjectCount();
	IPhysicsObject **pActiveList = NULL;
	if ( activeCount )
	{
		pActiveList = (IPhysicsObject **)stackalloc( sizeof(IPhysicsObject *)*activeCount );
		physenv->GetActiveObjects( pActiveList );
		for ( int i = 0; i < activeCount; i++ )
		{
			CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(pActiveList[i]->GetGameData());
			if ( pEntity )
			{
				func( pEntity );
			}
		}
	}
}


static void CallbackHighlight( CBaseEntity *pEntity )
{
	pEntity->m_debugOverlays |= OVERLAY_ABSBOX_BIT | OVERLAY_PIVOT_BIT;
}

static void CallbackReport( CBaseEntity *pEntity )
{
	Msg( "%s\n", pEntity->GetClassname() );
}

CON_COMMAND(physics_highlight_active, "Turns on the absbox for all active physics objects")
{
	IterateActivePhysicsEntities( CallbackHighlight );
}

CON_COMMAND(physics_report_active, "Lists all active physics objects")
{
	IterateActivePhysicsEntities( CallbackReport );
}

// Advance physics by time (in seconds)
void PhysFrame( float deltaTime )
{
	static int lastObjectCount = 0;
	entitem_t *pItem;

	if ( !g_PhysicsHook.ShouldSimulate() )
		return;

	// Trap interrupts and clock changes
	if ( deltaTime > 1.0f || deltaTime < 0.0f )
	{
		deltaTime = 0;
		Msg( "Reset physics clock\n" );
	}
	else if ( deltaTime > 0.1f )	// limit incoming time to 100ms
	{
		deltaTime = 0.1f;
	}
	float simRealTime = 0;

	deltaTime *= phys_timescale.GetFloat();
	// !!!HACKHACK -- hard limit scaled time to avoid spending too much time in here
	// Limit to 100 ms
	if ( deltaTime > 0.100f )
		deltaTime = 0.100f;

	bool bProfile = phys_speeds.GetBool();

	if ( bProfile )
	{
		simRealTime = engine->Time();
	}
	g_Collisions.BufferTouchEvents( true );
	physenv->Simulate( deltaTime );

	int activeCount = physenv->GetActiveObjectCount();
	IPhysicsObject **pActiveList = NULL;
	if ( activeCount )
	{
		pActiveList = (IPhysicsObject **)stackalloc( sizeof(IPhysicsObject *)*activeCount );
		physenv->GetActiveObjects( pActiveList );

		for ( int i = 0; i < activeCount; i++ )
		{
			CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(pActiveList[i]->GetGameData());
			if ( pEntity )
			{
				pEntity->VPhysicsUpdate( pActiveList[i] );
			}
		}
	}

	for ( pItem = g_pShadowEntities->m_pItemList; pItem; pItem = pItem->pNext )
	{
		CBaseEntity *pEntity = pItem->hEnt;
		if ( !pEntity )
		{
			Msg( "Dangling pointer to physics entity!!!\n" );
			continue;
		}

		IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();
		// apply updates
		if ( pPhysics && !pPhysics->IsAsleep() )
		{
			pEntity->VPhysicsShadowUpdate( pPhysics );
		}
	}


	if ( bProfile )
	{
		simRealTime = engine->Time() - simRealTime;

		if ( simRealTime < 0 )
			simRealTime = 0;
		g_PhysAverageSimTime *= 0.8;
		g_PhysAverageSimTime += (simRealTime * 0.2);
		if ( lastObjectCount != 0 || activeCount != 0 )
		{
			Msg( "Physics: %3d objects, %4.1fms / AVG: %4.1fms\n", activeCount, simRealTime * 1000, g_PhysAverageSimTime * 1000 );
		}
		
		lastObjectCount = activeCount;
	}
	g_Collisions.BufferTouchEvents( false );
	g_Collisions.FrameUpdate();
}


void PhysAddShadow( CBaseEntity *pEntity )
{
	g_pShadowEntities->AddEntity( pEntity );
}

void PhysRemoveShadow( CBaseEntity *pEntity )
{
	g_pShadowEntities->DeleteEntity( pEntity );
}


//-----------------------------------------------------------------------------
// CollisionEvent system 
//-----------------------------------------------------------------------------
// NOTE: PreCollision/PostCollision ALWAYS come in matched pairs!!!
void CCollisionEvent::PreCollision( vcollisionevent_t *pEvent )
{
	m_gameEvent.Init( pEvent );

	// gather the pre-collision data that the game needs to track
	for ( int i = 0; i < 2; i++ )
	{
		IPhysicsObject *pObject = pEvent->pObjects[i];
		if ( pObject )
		{
			if ( pObject->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
			{
				CBaseEntity *pOtherEntity = reinterpret_cast<CBaseEntity *>(pEvent->pObjects[!i]->GetGameData());
				if ( pOtherEntity && !pOtherEntity->IsPlayer() )
				{
					Vector velocity;
					AngularImpulse angVel;
					// HACKHACK: If we totally clear this out, then Havok will think the objects
					// are penetrating and generate forces to separate them
					// so make it fairly small and have a tiny collision instead.
					// UNDONE: Clamp this to something small/guaranteed instead of scale and hope for the best?
					pObject->GetVelocity( &velocity, &angVel );
					VectorNormalize(velocity);
					velocity *= 0.1;
					VectorNormalize(angVel);
					pObject->SetVelocity( &velocity, &angVel );
				}
			}
			pObject->GetVelocity( &m_gameEvent.preVelocity[i], &m_gameEvent.preAngularVelocity[i] );
		}
	}
}

void CCollisionEvent::PostCollision( vcollisionevent_t *pEvent )
{
	bool isNPC[2] = {false,false};
	bool isShadow[2] = {false,false};
	int i;

	for ( i = 0; i < 2; i++ )
	{
		IPhysicsObject *pObject = pEvent->pObjects[i];
		if ( pObject )
		{
			CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(pObject->GetGameData());
			isNPC[i] = (pEntity->MyNPCPointer() || pEntity->IsPlayer()) ? true : false;
			unsigned int flags = pObject->GetCallbackFlags();
			m_gameEvent.pEntities[i] = pEntity;
			pObject->GetVelocity( &m_gameEvent.postVelocity[i], NULL );
			if ( flags & CALLBACK_SHADOW_COLLISION )
			{
				isShadow[i] = true;
			}
			if ( pEvent->deltaCollisionTime < 1.0 && phys_rolling_drag.GetBool() )
			{
				PhysApplyRollingDrag( pObject, pEvent->deltaCollisionTime );
			}

			// Shouldn't get impacts with triggers
			Assert( !pObject->IsTrigger() );
		}
	}
	// copy off the post-collision variable data
	m_gameEvent.collisionSpeed = pEvent->collisionSpeed;
	m_gameEvent.pInternalData = pEvent->pInternalData;

	// special case for hitting self, only make one non-shadow call
	if ( m_gameEvent.pEntities[0] == m_gameEvent.pEntities[1] )
	{
		if ( pEvent->isCollision )
		{
			m_gameEvent.pEntities[1] = NULL;
			m_gameEvent.pEntities[0]->VPhysicsCollision( 0, &m_gameEvent );
		}
		return;
	}

	// don't make sounds/effects for NPC/NPC collisions
	if ( isNPC[0] && isNPC[1] )
		return;

#if OSCILLATION_DETECT_EXPERIMENT
	m_frameCollisions = 1;
	if ( m_percentFramesWithCollision >= 0.25 )
	{
		if ( pEvent->deltaCollisionTime < 0.05 )
		{
			PhysForceEntityToSleep( m_gameEvent.pEntities[0], pEvent->pObjects[0] );
			PhysForceEntityToSleep( m_gameEvent.pEntities[1], pEvent->pObjects[1] );
		}
	}
#endif

	if ( isShadow[0] && isShadow[1] )
	{
		pEvent->isCollision = false;
	}

	for ( i = 0; i < 2; i++ )
	{
		if ( pEvent->isCollision )
		{
			m_gameEvent.pEntities[i]->VPhysicsCollision( i, &m_gameEvent );
		}
		if ( pEvent->isShadowCollision && isShadow[i] )
		{
			m_gameEvent.pEntities[i]->VPhysicsShadowCollision( i, &m_gameEvent );
		}
	}
}

void PhysForceEntityToSleep( CBaseEntity *pEntity, IPhysicsObject *pObject )
{
	// UNDONE: Check to see if the object is touching the player first?
	// Might get the player stuck?
	if ( !pObject || !pObject->IsMoveable() )
		return;

	DevMsg(2, "Putting entity to sleep: %s\n", pEntity->GetClassname() );
	CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();
	if ( pAnimating && pAnimating->IsRagdoll() )
	{
		CRagdollProp *pRagdoll = dynamic_cast<CRagdollProp *>(pAnimating);
		if ( pRagdoll )
		{
			ragdoll_t *pRagInfo = pRagdoll->GetRagdoll();
			for ( int i = 0; i < pRagInfo->listCount; i++ )
			{
				pRagInfo->list[i].pObject->Sleep();
			}
			return;
		}
	}
	pObject->Sleep();
}


// Compute enough energy of a reference mass travelling at speed
// makes numbers more intuitive
#define MASS_SPEED2ENERGY(mass, speed)	((speed)*(speed)*(mass))

// energy of a 10kg mass moving at speed
#define MASS10_SPEED2ENERGY(speed)	MASS_SPEED2ENERGY(10,speed)

#define MASS_ENERGY2SPEED(mass,energy)	(FastSqrt((energy)/mass))

void CCollisionEvent::Friction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit, IPhysicsCollisionData *pData )
{
	surfacedata_t *psurf = physprops->GetSurfaceData( surfaceProps );
	surfacedata_t *phit = physprops->GetSurfaceData( surfacePropsHit );


	//Get our friction information
	Vector vecPos, vecVel;
	pData->GetContactPoint( vecPos );
	pObject->GetVelocityAtPoint( vecPos, vecVel );
	
	vecVel = -vecVel;
	VectorNormalize( vecVel );

	switch ( phit->gameMaterial )
	{
	case CHAR_TEX_DIRT:
		
		if ( energy < MASS10_SPEED2ENERGY(15) )
			break;
		
		g_pEffects->Dust( vecPos, vecVel, 1, 16 );
		break;

	case CHAR_TEX_CONCRETE:
		
		if ( energy < MASS10_SPEED2ENERGY(28) )
			break;
		
		g_pEffects->Dust( vecPos, vecVel, 1, 16 );
		break;
	}
	
	//Metal sparks
	if ( energy > MASS10_SPEED2ENERGY(35) )
	{
		// make sparks for metal/concrete scrapes with enough energy
		if ( psurf->gameMaterial == CHAR_TEX_METAL || psurf->gameMaterial == CHAR_TEX_GRATE )
		{	
			switch ( phit->gameMaterial )
			{
			case CHAR_TEX_CONCRETE:
			case CHAR_TEX_METAL:

				g_pEffects->MetalSparks( vecPos, vecVel );
				break;									
			}
		}
	}

	if ( pObject )
	{
		CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(pObject->GetGameData());
		pEntity->VPhysicsFriction( pObject, energy, surfaceProps, surfacePropsHit );
	}
}


friction_t *CCollisionEvent::FindFriction( CBaseEntity *pObject )
{
	friction_t *pFree = NULL;

	for ( int i = 0; i < ARRAYSIZE(m_current); i++ )
	{
		if ( !m_current[i].pObject && !pFree )
			pFree = &m_current[i];

		if ( m_current[i].pObject == pObject )
			return &m_current[i];
	}

	return pFree;
}

void CCollisionEvent::ShutdownFriction( friction_t &friction )
{
//	Msg( "Scrape Stop %s \n", STRING(friction.pObject->m_iClassname) );
	CSoundEnvelopeController::GetController().SoundDestroy( friction.patch );
	friction.patch = NULL;
	friction.pObject = NULL;
}

void CCollisionEvent::PostSimulationFrame()
{
	UpdateDamageEvents();
}

void CCollisionEvent::FrameUpdate( void )
{
	UpdateFrictionSounds();
	UpdateTouchEvents();
	UpdatePenetrateEvents();
	UpdateFluidEvents();
	UpdateDamageEvents(); // if there was no PSI in physics, we'll still need to do some of these because collisions are solved in between PSIs

#if OSCILLATION_DETECT_EXPERIMENT
	int ticks = physenv->GetTimestepsSimulatedLast();

	// It takes 5 seconds at 100% to move the average to 100%
	const float secondsPer100Percent = 5.0;
	float secondsPerTick = physenv->GetSimulationTimestep();

	float ticksPer100Percent = secondsPerTick / secondsPer100Percent;

	if ( gpGlobals->frametime && ticks )
	{
		for ( int i = 0; i < ticks; i++ )
		{
			m_percentFramesWithCollision = (m_percentFramesWithCollision * (1-ticksPer100Percent)) + (m_frameCollisions * ticksPer100Percent);
		}

		//Msg("Collision! : %.2f (%d -- %d)\n", m_percentFramesWithCollision, ticks, m_frameCollisions );
		m_frameCollisions = 0;
	}
#endif
}

void CCollisionEvent::UpdateFluidEvents( void )
{
	for ( int i = m_fluidEvents.Count()-1; i >= 0; --i )
	{
		if ( (gpGlobals->curtime - m_fluidEvents[i].impactTime) > FLUID_TIME_MAX )
		{
			m_fluidEvents.FastRemove(i);
		}
	}
}


float CCollisionEvent::DeltaTimeSinceLastFluid( CBaseEntity *pEntity )
{
	for ( int i = m_fluidEvents.Count()-1; i >= 0; --i )
	{
		if ( m_fluidEvents[i].pEntity == pEntity )
		{
			return gpGlobals->curtime - m_fluidEvents[i].impactTime;
		}
	}

	int index = m_fluidEvents.AddToTail();
	m_fluidEvents[index].pEntity = pEntity;
	m_fluidEvents[index].impactTime = gpGlobals->curtime;
	return FLUID_TIME_MAX;
}

void CCollisionEvent::UpdateFrictionSounds( void )
{
	for ( int i = 0; i < ARRAYSIZE(m_current); i++ )
	{
		if ( m_current[i].patch )
		{
			if ( m_current[i].update < (gpGlobals->curtime-0.1f) )
			{
				ShutdownFriction( m_current[i] );
			}
		}
	}
}


void CCollisionEvent::DispatchStartTouch( CBaseEntity *pEntity0, CBaseEntity *pEntity1, const Vector &point, const Vector &normal )
{
	trace_t trace;
	memset( &trace, 0, sizeof(trace) );
	trace.endpos = point;
	trace.plane.dist = DotProduct( point, normal );
	trace.plane.normal = normal;

	// NOTE: This sets up the touch list for both entities, no call to pEntity1 is needed
	pEntity0->PhysicsMarkEntitiesAsTouchingEventDriven( pEntity1, trace );
}

void CCollisionEvent::DispatchEndTouch( CBaseEntity *pEntity0, CBaseEntity *pEntity1 )
{
	// frees the event-driven touchlinks
	pEntity0->PhysicsNotifyOtherOfUntouch( pEntity0, pEntity1 );
	pEntity1->PhysicsNotifyOtherOfUntouch( pEntity1, pEntity0 );
}

void CCollisionEvent::UpdateTouchEvents( void )
{
	for ( int i = 0; i < m_touchEvents.Count(); i++ )
	{
		const touchevent_t &event = m_touchEvents[i];
		if ( event.touchType == TOUCH_START )
		{
			DispatchStartTouch( event.pEntity0, event.pEntity1, event.endPoint, event.normal );
		}
		else
		{
			// TOUCH_END
			DispatchEndTouch( event.pEntity0, event.pEntity1 );
		}
	}
	m_touchEvents.RemoveAll();
}

void CCollisionEvent::UpdateDamageEvents( void )
{
	for ( int i = 0; i < m_damageEvents.Count(); i++ )
	{
		const damageevent_t &event = m_damageEvents[i];

		// Track changes in the entity's life state
		int iEntBits = event.pEntity->IsAlive() ? 0x0001 : 0;
		iEntBits |= event.pEntity->IsMarkedForDeletion() ? 0x0002 : 0;
		iEntBits |= (event.pEntity->GetSolidFlags() & FSOLID_NOT_SOLID) ? 0x0004 : 0;
		event.pEntity->TakeDamage( event.info );
		int iEntBits2 = event.pEntity->IsAlive() ? 0x0001 : 0;
		iEntBits2 |= event.pEntity->IsMarkedForDeletion() ? 0x0002 : 0;
		iEntBits2 |= (event.pEntity->GetSolidFlags() & FSOLID_NOT_SOLID) ? 0x0004 : 0;

		if ( event.bRestoreVelocity && event.pInflictorPhysics && iEntBits != iEntBits2 )
		{
			event.pInflictorPhysics->SetVelocity( &event.savedVelocity, &event.savedAngularVelocity );
		}
	}
	m_damageEvents.RemoveAll();
}


void CCollisionEvent::AddTouchEvent( CBaseEntity *pEntity0, CBaseEntity *pEntity1, int touchType, const Vector &point, const Vector &normal )
{
	if ( !pEntity0 || !pEntity1 )
		return;

	int index = m_touchEvents.AddToTail();
	touchevent_t &event = m_touchEvents[index];
	event.pEntity0 = pEntity0;
	event.pEntity1 = pEntity1;
	event.touchType = touchType;
	event.endPoint = point;
	event.normal = normal;
}

void CCollisionEvent::AddDamageEvent( CBaseEntity *pEntity, const CTakeDamageInfo &info, IPhysicsObject *pInflictorPhysics, bool bRestoreVelocity, const Vector &savedVel, const AngularImpulse &savedAngVel )
{
	if ( !( info.GetDamageType() & (DMG_BURN | DMG_DROWN | DMG_TIMEBASED) ) )
	{
		Assert( info.GetDamageForce() != vec3_origin && info.GetDamagePosition() != vec3_origin );
	}

	int index = m_damageEvents.AddToTail();
	damageevent_t &event = m_damageEvents[index];
	event.pEntity = pEntity;
	event.info = info;
	event.pInflictorPhysics = pInflictorPhysics;
	event.bRestoreVelocity = bRestoreVelocity;
	if ( !pInflictorPhysics || !pInflictorPhysics->IsMoveable() )
	{
		event.bRestoreVelocity = false;
	}
	
	// NOTE: Save off the state of the object before collision
	// restore if the impact is a kill
	// UNDONE: Should we absorb some energy here?
	// NOTE: we can't save a delta because there could be subsequent post-fatal collisions
	event.savedVelocity = savedVel;
	event.savedAngularVelocity = savedAngVel;
}

void CCollisionEvent::LevelShutdown( void )
{
	for ( int i = 0; i < ARRAYSIZE(m_current); i++ )
	{
		if ( m_current[i].patch )
		{
			ShutdownFriction( m_current[i] );
		}
	}
}


void CCollisionEvent::StartTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData )
{
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *>(pObject1->GetGameData());
	CBaseEntity *pEntity2 = static_cast<CBaseEntity *>(pObject2->GetGameData());
	Vector endPoint, normal;
	pTouchData->GetContactPoint( endPoint );
	pTouchData->GetSurfaceNormal( normal );
	if ( !m_bBufferTouchEvents )
	{
		DispatchStartTouch( pEntity1, pEntity2, endPoint, normal );
	}
	else
	{
		AddTouchEvent( pEntity1, pEntity2, TOUCH_START, endPoint, normal );
	}
}

void CCollisionEvent::EndTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData )
{
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *>(pObject1->GetGameData());
	CBaseEntity *pEntity2 = static_cast<CBaseEntity *>(pObject2->GetGameData());
	Vector endPoint, normal;
	pTouchData->GetContactPoint( endPoint );
	pTouchData->GetSurfaceNormal( normal );

	if ( !m_bBufferTouchEvents )
	{
		DispatchEndTouch( pEntity1, pEntity2 );
	}
	else
	{
		AddTouchEvent( pEntity1, pEntity2, TOUCH_END, vec3_origin, vec3_origin );
	}
}


// UNDONE: This is functional, but minimally.
void CCollisionEvent::ObjectEnterTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject )
{
	CBaseEntity *pTriggerEntity = static_cast<CBaseEntity *>(pTrigger->GetGameData());
	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	pTriggerEntity->StartTouch( pEntity );
}

void CCollisionEvent::ObjectLeaveTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject )
{
	CBaseEntity *pTriggerEntity = static_cast<CBaseEntity *>(pTrigger->GetGameData());
	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	pTriggerEntity->EndTouch( pEntity );
}

/*
// Example code that uses VPhysics trigger implementation
class CTriggerVPhysics : public CBaseEntity
{
	DECLARE_CLASS( CTriggerVPhysics, CBaseEntity );
public:
	void Spawn()
	{
		SetSolid( SOLID_VPHYSICS );	
		AddSolidFlags( FSOLID_NOT_SOLID );

		// NOTE: Don't make yourself FSOLID_TRIGGER here or you'll get game 
		// collisions AND vphysics collisions.  You don't want any game collisions
		// so just use FSOLID_NOT_SOLID

		SetMoveType( MOVETYPE_NONE );
		SetModel( STRING( GetModelName() ) );    // set size and link into world

		CreateVPhysics();
	}
	bool CreateVPhysics()
	{
		IPhysicsObject *pPhysics = VPhysicsInitStatic();
		pPhysics->BecomeTrigger();
		return true;
	}

	void StartTouch( CBaseEntity *pOther )
	{
		Msg("ST: %s\n", pOther->GetClassname() );
		// add to a motion controller here?
	}
	void EndTouch( CBaseEntity *pOther )
	{
		Msg("ET: %s\n", pOther->GetClassname() );
		// remove from the motion controller here?
	}
};

LINK_ENTITY_TO_CLASS( trigger_vphysics, CTriggerVPhysics );
*/

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// External interface to collision sounds
//-----------------------------------------------------------------------------

void PhysicsImpactSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, float volume )
{
	physicssound::AddImpactSound( g_PhysicsHook.m_impactSounds, pEntity, pEntity->entindex(), channel, pPhysObject, surfaceProps, volume );
}

void PhysCollisionSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, float deltaTime, float speed )
{
	if ( deltaTime < 0.05f || speed < 70.0f )
		return;

	float volume = speed * speed * (1.0f/(320.0f*320.0f));	// max volume at 320 in/s
	if ( volume > 1.0f )
		volume = 1.0f;

	PhysicsImpactSound( pEntity, pPhysObject, channel, surfaceProps, volume );
}

#define ENERGY_VOLUME_SCALE		(1.0f / 15500.0f)

void PhysFrictionSound( CBaseEntity *pEntity, IPhysicsObject *pObject, float energy, int surfaceProps )
{
	if ( !pEntity || energy < 75.0 || surfaceProps < 0 )
		return;
	
	// rescale the incoming energy
	energy *= ENERGY_VOLUME_SCALE;
	// volume of scrape is proportional to square of energy (steeper rolloff at low energies)
	float volume = energy * energy;

	// cut out the quiet sounds
	// UNDONE: Separate threshold for starting a sound vs. continuing?
	if ( volume > (1.0f/128.0f) )
	{
		friction_t *pFriction = g_Collisions.FindFriction( pEntity );
		if ( !pFriction )
			return;

		if ( volume > 1.0f )
			volume = 1.0f;
		else if ( volume < 0.05f )
			volume = 0.05f;

		surfacedata_t *psurf = physprops->GetSurfaceData( surfaceProps );
		int offset = 0;
		if ( psurf->scrapeCount )
		{
			offset = random->RandomInt(0, psurf->scrapeCount-1);
		}
		const char *pSound = physprops->GetString( psurf->scrape, 0 );

		CSoundParameters params;
		if ( !CBaseEntity::GetParametersForSound( pSound, params ) )
			return;

		if ( !pFriction->pObject )
		{
			pFriction->pObject = pEntity;
			CPASAttenuationFilter filter( pEntity, params.soundlevel );
			pFriction->patch = CSoundEnvelopeController::GetController().SoundCreate( 
				filter, pEntity->entindex(), CHAN_BODY, params.soundname, params.soundlevel );
			CSoundEnvelopeController::GetController().Play( pFriction->patch, params.volume * volume, params.pitch );
		}
		else
		{
			float pitch = (volume * (params.pitchhigh - params.pitchlow)) + params.pitchlow;
			CSoundEnvelopeController::GetController().SoundChangeVolume( pFriction->patch, params.volume * volume, 0.1f );
			CSoundEnvelopeController::GetController().SoundChangePitch( pFriction->patch, pitch, 0.1f );
		}

		pFriction->update = gpGlobals->curtime;
	}
}

void PhysCleanupFrictionSounds( CBaseEntity *pEntity )
{
	friction_t *pFriction = g_Collisions.FindFriction( pEntity );
	if ( pFriction && pFriction->patch )
	{
		g_Collisions.ShutdownFriction( *pFriction );
	}
}


void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex )
{
	Assert( physenv->IsInSimulation() );
	int otherIndex = !hurtIndex;
	g_Collisions.AddDamageEvent( pEntity, info, event.pObjects[otherIndex], true, event.preVelocity[otherIndex], event.preAngularVelocity[otherIndex] );
}

void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info )
{
	if ( physenv->IsInSimulation() )
	{
		CBaseEntity *pInflictor = info.GetInflictor();
		IPhysicsObject *pInflictorPhysics = (pInflictor) ? pInflictor->VPhysicsGetObject() : NULL;
		g_Collisions.AddDamageEvent( pEntity, info, pInflictorPhysics, false, vec3_origin, vec3_origin );
		if ( pEntity && info.GetInflictor() )
		{
			DevMsg( 2, "Warning: Physics damage event with no recovery info!\nObjects: %s, %s\n", pEntity->GetClassname(), info.GetInflictor()->GetClassname() );
		}
	}
	else
	{
		pEntity->TakeDamage( info );
	}
}


void CC_AirDensity( void )
{
	if ( !physenv )
		return;

	if ( engine->Cmd_Argc() < 2 )
	{
		Msg( "air_density <value>\nCurrent air density is %.2f\n", physenv->GetAirDensity() );
	}
	else
	{
		float density = atof(engine->Cmd_Argv(1));
		physenv->SetAirDensity( density );
	}
}
static ConCommand air_density("air_density", CC_AirDensity, "Changes the density of air for drag computations.", FCVAR_CHEAT);

//=============================================================================
//
// Physics Game Trace
//
class CPhysicsGameTrace : public IPhysicsGameTrace
{
public:

	void VehicleTraceRay( const Ray_t &ray, void *pVehicle, trace_t *pTrace );
	void VehicleTraceRayWithWater( const Ray_t &ray, void *pVehicle, trace_t *pTrace );
	bool VehiclePointInWater( const Vector &vecPoint );
};

CPhysicsGameTrace g_PhysGameTrace;
IPhysicsGameTrace *physgametrace = &g_PhysGameTrace;

//-----------------------------------------------------------------------------
// Purpose: Game ray-traces in vphysics.
//-----------------------------------------------------------------------------
void CPhysicsGameTrace::VehicleTraceRay( const Ray_t &ray, void *pVehicle, trace_t *pTrace )
{
	CBaseEntity *pBaseEntity = static_cast<CBaseEntity*>( pVehicle );
	UTIL_TraceRay( ray, MASK_SOLID, pBaseEntity, COLLISION_GROUP_NONE, pTrace );
}

//-----------------------------------------------------------------------------
// Purpose: Game ray-traces in vphysics.
//-----------------------------------------------------------------------------
void CPhysicsGameTrace::VehicleTraceRayWithWater( const Ray_t &ray, void *pVehicle, trace_t *pTrace )
{
	CBaseEntity *pBaseEntity = static_cast<CBaseEntity*>( pVehicle );
	UTIL_TraceRay( ray, MASK_SOLID|MASK_WATER, pBaseEntity, COLLISION_GROUP_NONE, pTrace );
}

//-----------------------------------------------------------------------------
// Purpose: Test to see if a vehicle point is in water.
//-----------------------------------------------------------------------------
bool CPhysicsGameTrace::VehiclePointInWater( const Vector &vecPoint )
{
	return ( ( UTIL_PointContents( vecPoint ) & MASK_WATER ) != 0 );
}

#if 0

#include "filesystem.h"
//-----------------------------------------------------------------------------
// Purpose: This will append a collide to a glview file.  Then you can view the 
//			collisionmodels with glview.
// Input  : *pCollide - collision model
//			&origin - position of the instance of this model
//			&angles - orientation of instance
//			*pFilename - output text file
//-----------------------------------------------------------------------------
// examples:
// world:
//	DumpCollideToGlView( pWorldCollide->solids[0], vec3_origin, vec3_origin, "jaycollide.txt" );
// static_prop:
//	DumpCollideToGlView( info.m_pCollide->solids[0], info.m_Origin, info.m_Angles, "jaycollide.txt" );
//
//-----------------------------------------------------------------------------
void DumpCollideToGlView( CPhysCollide *pCollide, const Vector &origin, const QAngle &angles, const char *pFilename )
{
	if ( !pCollide )
		return;

	printf("Writing %s...\n", pFilename );
	Vector *outVerts;
	int vertCount = physcollision->CreateDebugMesh( pCollide, &outVerts );
	FileHandle_t fp = filesystem->Open( pFilename, "ab" );
	int triCount = vertCount / 3;
	int vert = 0;
	VMatrix tmp = SetupMatrixOrgAngles( origin, angles );
	int i;
	for ( i = 0; i < vertCount; i++ )
	{
		outVerts[i] = tmp.VMul4x3( outVerts[i] );
	}
	for ( i = 0; i < triCount; i++ )
	{
		filesystem->FPrintf( fp, "3\n" );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f 1 0 0\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f 0 1 0\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f 0 0 1\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
	}
	filesystem->Close( fp );
	physcollision->DestroyDebugMesh( vertCount, outVerts );
}
#endif

