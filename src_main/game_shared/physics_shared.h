//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef PHYSICS_SHARED_H
#define PHYSICS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

class IPhysics;
class IPhysicsEnvironment;
class IPhysicsSurfaceProps;
class IPhysicsCollision;
class IPhysicsObject;

extern IPhysicsObject		*g_PhysWorldObject;
extern IPhysics				*physics;
extern IPhysicsCollision	*physcollision;
extern IPhysicsEnvironment	*physenv;
extern IPhysicsSurfaceProps *physprops;

extern const objectparams_t g_PhysDefaultObjectParams;


// VPHYSICS object game-specific flags
#define FVPHYSICS_DMG_SLICE				0x0001		// does slice damage, not just blunt damage
#define FVPHYSICS_CONSTRAINT_STATIC		0x0002		// object is constrained to the world, so it should behave like a static
#define FVPHYSICS_PLAYER_HELD			0x0004		// object is held by the player, so have a very inelastic collision response
#define FVPHYSICS_PART_OF_RAGDOLL		0x0008		// object is part of a client or server ragdoll
#define FVPHYSICS_MULTIOBJECT_ENTITY	0x0010		// object is part of a multi-object entity
#define FVPHYSICS_NO_SELF_COLLISIONS	0x8000		// don't collide with other objects that are part of the same entity

// Convenience routine
// ORs gameFlags with the physics object's current game flags
inline unsigned short PhysSetGameFlags( IPhysicsObject *pPhys, unsigned short gameFlags )
{
	unsigned short flags = pPhys->GetGameFlags();
	flags |= gameFlags;
	pPhys->SetGameFlags( flags );
	
	return flags;
}
// mask off gameFlags
inline unsigned short PhysClearGameFlags( IPhysicsObject *pPhys, unsigned short gameFlags )
{
	unsigned short flags = pPhys->GetGameFlags();
	flags &= ~gameFlags;
	pPhys->SetGameFlags( flags );
	
	return flags;
}


// Create a vphysics object based on a model
IPhysicsObject *PhysModelCreate( CBaseEntity *pEntity, int modelIndex, const Vector &origin, const QAngle &angles, solid_t *pSolid = NULL );

IPhysicsObject *PhysModelCreateBox( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, const Vector &origin, bool isStatic );

// Create a vphysics object based on a BSP model (unmoveable)
IPhysicsObject *PhysModelCreateUnmoveable( CBaseEntity *pEntity, int modelIndex, const Vector &origin, const QAngle &angles );

// Create a vphysics object based on an existing collision model
IPhysicsObject *PhysModelCreateCustom( CBaseEntity *pEntity, const CPhysCollide *pModel, const Vector &origin, const QAngle &angles, const char *pName, bool isStatic, solid_t *pSolid = NULL );

// Create a bbox collision model (these may be shared among entities, they are auto-deleted at end of level. do not manage)
CPhysCollide *PhysCreateBbox( const Vector &mins, const Vector &maxs );

// Create a vphysics sphere object
IPhysicsObject *PhysSphereCreate( CBaseEntity *pEntity, float radius, const Vector &origin, solid_t &solid );

// Destroy a physics object created using PhysModelCreate...()
void PhysDestroyObject( IPhysicsObject *pObject );

// create the world physics objects
IPhysicsObject *PhysCreateWorld_Shared( CBaseEntity *pWorld, vcollide_t *pWorldCollide, const objectparams_t &defaultParams );

// parse the parameters for a single solid from the model's collision data
bool PhysModelParseSolid( solid_t &solid, CBaseEntity *pEntity, int modelIndex );
// parse the parameters for a solid matching a particular index
bool PhysModelParseSolidByIndex( solid_t &solid, CBaseEntity *pEntity, int modelIndex, int solidIndex );

void PhysApplyRollingDrag( IPhysicsObject *pObject, float deltaCollisionTime );
void PhysParseSurfaceData( const char *pFileName, class IPhysicsSurfaceProps *pProps, class IFileSystem *pFileSystem );

// fill out this solid_t with the AABB defaults (high inertia/no rotation)
void PhysGetDefaultAABBSolid( solid_t &solid );

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* PhysicsGameSystem();

#endif // PHYSICS_SHARED_H
