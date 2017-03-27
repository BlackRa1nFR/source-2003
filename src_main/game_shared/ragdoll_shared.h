//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef RAGDOLL_SHARED_H
#define RAGDOLL_SHARED_H
#ifdef _WIN32
#pragma once
#endif

class IPhysicsObject;
class IPhysicsConstraint;
class IPhysicsConstraintGroup;
class IPhysicsCollision;
class IPhysicsEnvironment;
class IPhysicsSurfaceProps;
struct matrix3x4_t;

struct vcollide_t;
struct studiohdr_t;

#include "vector.h"

// UNDONE: Remove and make dynamic?
#define RAGDOLL_MAX_ELEMENTS	24
#define RAGDOLL_INDEX_BITS		5			// NOTE 1<<RAGDOLL_INDEX_BITS >= RAGDOLL_MAX_ELEMENTS

struct ragdollelement_t
{
	Vector				originParentSpace;
	IPhysicsObject		*pObject;		// all valid elements have an object
	IPhysicsConstraint	*pConstraint;	// all valid elements have a constraint (except the root)
	int					parentIndex;
};

struct ragdoll_t
{
	int					listCount;
	IPhysicsConstraintGroup *pGroup;
	// store these in separate arrays for save/load
	ragdollelement_t 	list[RAGDOLL_MAX_ELEMENTS];
	int					boneIndex[RAGDOLL_MAX_ELEMENTS];
};

struct ragdollparams_t
{
	void		*pGameData;
	vcollide_t	*pCollide;
	studiohdr_t *pStudioHdr;
	Vector		forceVector;
	int			forceBoneIndex;
	matrix3x4_t	*pPrevBones;
	matrix3x4_t	*pCurrentBones;
	float		boneDt;		// time delta between prev/cur samples
	float		jointFrictionScale;
	Vector		forcePosition;
};


bool RagdollCreate( ragdoll_t &ragdoll, const ragdollparams_t &params,
				   IPhysicsCollision *pPhysCollision, IPhysicsEnvironment *pPhysEnv, IPhysicsSurfaceProps *pSurfaceDatabase );

void RagdollActivate( ragdoll_t &ragdoll );
void RagdollDestroy( ragdoll_t &ragdoll );

// Gets the bone matrix for a ragdoll object
// NOTE: This is different than the object's position because it is
// forced to be rigidly attached in parent space
bool RagdollGetBoneMatrix( const ragdoll_t &ragdoll, matrix3x4_t *pBoneToWorld, int objectIndex );

// Parse the ragdoll and obtain the mapping from each physics element index to a bone index
// returns num phys elements
int RagdollExtractBoneIndices( int *boneIndexOut, studiohdr_t *pStudioHdr, vcollide_t *pCollide );

// computes an exact bbox of the ragdoll's physics objects
void RagdollComputeExactBbox( const ragdoll_t &ragdoll, const Vector &origin, Vector &outMins, Vector &outMaxs );
bool RagdollIsAsleep( const ragdoll_t &ragdoll );

void RagdollApplyAnimationAsVelocity( ragdoll_t &ragdoll, matrix3x4_t *pPrevBones, matrix3x4_t *pCurrentBones, float dt );
void RagdollApplyAnimationAsVelocity( ragdoll_t &ragdoll, matrix3x4_t pBoneToWorld[] );

#endif // RAGDOLL_SHARED_H
