
//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: This is the abstraction layer for the physics simulation system
// Any calls to the external physics library (ipion) should be made through this
// layer.  Eventually, the physics system will probably become a DLL and made 
// accessible to the client & server side code.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef PHYSICS_H
#define PHYSICS_H

#ifdef _WIN32
#pragma once
#endif

#include "physics_shared.h"

class CBaseEntity;
class IPhysicsMaterial;
class IPhysicsConstraint;
class IPhysicsSpring;
class IPhysicsSurfaceProps;
class CTakeDamageInfo;
class ConVar;

extern IPhysicsMaterial		*g_Material;
extern ConVar phys_pushscale;
extern ConVar phys_timescale;

struct objectparams_t;
extern IPhysicsGameTrace	*physgametrace;


struct gamevcollisionevent_t : public vcollisionevent_t
{
	void Init( vcollisionevent_t *pEvent ) { *((vcollisionevent_t *)this) = *pEvent; }
	Vector			preVelocity[2];
	Vector			postVelocity[2];
	AngularImpulse	preAngularVelocity[2];
	CBaseEntity		*pEntities[2];
};

// parse solid parameter overrides out of a string 
void PhysSolidOverride( solid_t &solid, string_t overrideScript );

void PhysAddShadow( CBaseEntity *pEntity );
void PhysRemoveShadow( CBaseEntity *pEntity );

void PhysCollisionSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, float deltaTime, float speed );
void PhysFrictionSound( CBaseEntity *pEntity, IPhysicsObject *pObject, float energy, int surfaceProps );

// plays the impact sound for a particular material
void PhysicsImpactSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, float volume );

void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info );
void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex );

void PhysCleanupFrictionSounds( CBaseEntity *pEntity );

// force a physics entity to sleep immediately
void PhysForceEntityToSleep( CBaseEntity *pEntity, IPhysicsObject *pObject );

// teleport an entity to it's position relative to an object it's constrained to
void PhysTeleportConstrainedEntity( CBaseEntity *pTeleportSource, IPhysicsObject *pObject0, IPhysicsObject *pObject1, const Vector &prevPosition, const QAngle &prevAngles, bool physicsRotate );

#endif		// PHYSICS_H
