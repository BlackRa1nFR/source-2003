//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "ragdoll_shared.h"
#include "bone_setup.h"
#include "vphysics/constraints.h"
#include "vcollide_parse.h"
#include "vphysics_interface.h"

static void RagdollCreateObjects( IPhysicsCollision *pPhysCollision, IPhysicsEnvironment *pPhysEnv, IPhysicsSurfaceProps *pSurfaceDatabase, ragdoll_t &ragdoll, const ragdollparams_t &params )
{
	ragdoll.listCount = 0;
	ragdoll.pGroup = NULL;
	memset( ragdoll.list, 0, sizeof(ragdoll.list) );
	if ( !params.pCollide || params.pCollide->solidCount > RAGDOLL_MAX_ELEMENTS )
		return;

	IVPhysicsKeyParser *pParse = pPhysCollision->VPhysicsKeyParserCreate( params.pCollide->pKeyValues );
	ragdoll.pGroup = pPhysEnv->CreateConstraintGroup();
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcmpi( pBlock, "solid" ) )
		{
			solid_t solid;
			// collisions off by default
			pParse->ParseSolid( &solid, NULL );
			if ( solid.index >= 0 && solid.index < params.pCollide->solidCount)
			{
				Assert( ragdoll.listCount == solid.index );
				int boneIndex = Studio_BoneIndexByName( params.pStudioHdr, solid.name );
				ragdoll.boneIndex[ragdoll.listCount] = boneIndex;

				if ( boneIndex >= 0 )
				{
					solid.params.rotInertiaLimit = 0.5;
					solid.params.pGameData = params.pGameData;
					int surfaceData = pSurfaceDatabase->GetSurfaceIndex( solid.surfaceprop );
					
					if ( surfaceData < 0 )
						surfaceData = pSurfaceDatabase->GetSurfaceIndex( "default" );

					solid.params.pName = params.pStudioHdr->name;
					ragdoll.list[ragdoll.listCount].pObject = pPhysEnv->CreatePolyObject( params.pCollide->solids[solid.index], surfaceData, vec3_origin, vec3_angle, &solid.params );
					ragdoll.list[ragdoll.listCount].pObject->SetPositionMatrix( params.pCurrentBones[boneIndex], true );
					ragdoll.list[ragdoll.listCount].parentIndex = -1;

					ragdoll.listCount++;
				}
				else
				{
					Msg( "CRagdollProp::CreateObjects:  Couldn't Lookup Bone %s\n",
						solid.name );
				}
			}
		}
		else if ( !strcmpi( pBlock, "ragdollconstraint" ) )
		{
			constraint_ragdollparams_t constraint;
			pParse->ParseRagdollConstraint( &constraint, NULL );
			if ( constraint.childIndex >= 0 && constraint.parentIndex >= 0 )
			{
				Assert(constraint.childIndex<ragdoll.listCount);

				ragdollelement_t &childElement = ragdoll.list[constraint.childIndex];
				// save parent index
				childElement.parentIndex = constraint.parentIndex;

				if ( params.jointFrictionScale > 0 )
				{
					for ( int k = 0; k < 3; k++ )
					{
						constraint.axes[k].torque *= params.jointFrictionScale;
					}
				}
				// this parent/child pair is not usually a parent/child pair in the skeleton.  There
				// are often bones in between that are collapsed for simulation.  So we need to compute
				// the transform.
				Studio_CalcBoneToBoneTransform( params.pStudioHdr, ragdoll.boneIndex[constraint.childIndex], ragdoll.boneIndex[constraint.parentIndex], constraint.constraintToAttached );
				MatrixGetColumn( constraint.constraintToAttached, 3, childElement.originParentSpace );
				// UNDONE: We could transform the constraint limit axes relative to the bone space
				// using this data.  Do we need that feature?
				SetIdentityMatrix( constraint.constraintToReference );
				pPhysEnv->DisableCollisions( ragdoll.list[constraint.parentIndex].pObject, childElement.pObject );
				childElement.pConstraint = pPhysEnv->CreateRagdollConstraint( childElement.pObject, ragdoll.list[constraint.parentIndex].pObject, ragdoll.pGroup, constraint );
			}
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	pPhysCollision->VPhysicsKeyParserDestroy( pParse );
}

void RagdollActivate( ragdoll_t &ragdoll )
{
	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		// now that the relationships are set, activate the collision system
		ragdoll.list[i].pObject->EnableCollisions( true );
		ragdoll.list[i].pObject->Wake();
	}
	if ( ragdoll.pGroup )
	{
		ragdoll.pGroup->Activate();
	}
}


bool RagdollCreate( ragdoll_t &ragdoll, const ragdollparams_t &params, IPhysicsCollision *pPhysCollision, IPhysicsEnvironment *pPhysEnv, IPhysicsSurfaceProps *pSurfaceDatabase )
{
	RagdollCreateObjects( pPhysCollision, pPhysEnv, pSurfaceDatabase, ragdoll, params );

	if ( !ragdoll.listCount )
		return false;

	int forceBone = params.forceBoneIndex;
	
	int i;
	float totalMass = 0;
	for ( i = 0; i < ragdoll.listCount; i++ )
	{
		totalMass += ragdoll.list[i].pObject->GetMass();
	}
	totalMass = max(totalMass,1);

	// distribute half to the forced bone, half to the rest of the model
	Vector nudgeForce = params.forceVector;
	if ( forceBone >= 0 && forceBone <= ragdoll.listCount )
	{
		nudgeForce *= 0.5;
		ragdoll.list[forceBone].pObject->ApplyForceCenter( nudgeForce );
	}
	else if ( params.forcePosition != vec3_origin )
	{
		for ( i = 0; i < ragdoll.listCount; i++ )
		{
			float scale = ragdoll.list[i].pObject->GetMass() / totalMass;
			ragdoll.list[i].pObject->ApplyForceOffset( scale * nudgeForce, params.forcePosition );
		}
		nudgeForce.Init();
	}

	RagdollApplyAnimationAsVelocity( ragdoll, params.pPrevBones, params.pCurrentBones, params.boneDt );

	for ( i = 0; i < ragdoll.listCount; i++ )
	{
		float scale = ragdoll.list[i].pObject->GetMass() / totalMass;
		ragdoll.list[i].pObject->ApplyForceCenter( scale * nudgeForce );
		PhysSetGameFlags( ragdoll.list[i].pObject, FVPHYSICS_PART_OF_RAGDOLL );
	}
	return true;
}


void RagdollApplyAnimationAsVelocity( ragdoll_t &ragdoll, matrix3x4_t *pPrevBones, matrix3x4_t *pCurrentBones, float dt )
{
	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		Vector velocity;
		AngularImpulse angVel;
		int boneIndex = ragdoll.boneIndex[i];
		CalcBoneDerivatives( velocity, angVel, pPrevBones[boneIndex], pCurrentBones[boneIndex], dt );
		
		Vector localVelocity;
		AngularImpulse localAngVelocity;

		// move these derivatives into the local bone space of the "current" bone
		VectorIRotate( velocity, pCurrentBones[boneIndex], localVelocity );
		VectorIRotate( angVel, pCurrentBones[boneIndex], localAngVelocity );

		// move those bone-local coords back to world space using the ragdoll transform
		ragdoll.list[i].pObject->LocalToWorldVector( velocity, localVelocity );
		ragdoll.list[i].pObject->LocalToWorldVector( angVel, localAngVelocity );

		ragdoll.list[i].pObject->AddVelocity( &velocity, &angVel );
	}
}

void RagdollApplyAnimationAsVelocity( ragdoll_t &ragdoll, matrix3x4_t pBoneToWorld[] )
{
	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		matrix3x4_t inverse;
		MatrixInvert( pBoneToWorld[i], inverse );
		Quaternion q;
		Vector pos;
		MatrixAngles( inverse, q, pos );

		Vector velocity;
		AngularImpulse angVel;
		float flSpin;

		Vector localVelocity;
		AngularImpulse localAngVelocity;

		QuaternionAxisAngle( q, localAngVelocity, flSpin );
		localAngVelocity *= flSpin;
		localVelocity = pos;

		// move those bone-local coords back to world space using the ragdoll transform
		ragdoll.list[i].pObject->LocalToWorldVector( velocity, localVelocity );
		ragdoll.list[i].pObject->LocalToWorldVector( angVel, localAngVelocity );

		ragdoll.list[i].pObject->AddVelocity( &velocity, &angVel );
	}
}


void RagdollDestroy( ragdoll_t &ragdoll )
{
	if ( !ragdoll.listCount )
		return;

	int i;
	for ( i = 0; i < ragdoll.listCount; i++ )
	{
		physenv->DestroyConstraint( ragdoll.list[i].pConstraint );
		ragdoll.list[i].pConstraint = NULL;
	}
	for ( i = 0; i < ragdoll.listCount; i++ )
	{
		physenv->DestroyObject( ragdoll.list[i].pObject );
		ragdoll.list[i].pObject = NULL;
	}
	physenv->DestroyConstraintGroup( ragdoll.pGroup );
	ragdoll.pGroup = NULL;
	ragdoll.listCount = 0;
}

// Parse the ragdoll and obtain the mapping from each physics element index to a bone index
// returns num phys elements
int RagdollExtractBoneIndices( int *boneIndexOut, studiohdr_t *pStudioHdr, vcollide_t *pCollide )
{
	int elementCount = 0;

	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcmpi( pBlock, "solid" ) )
		{
			solid_t solid;
			pParse->ParseSolid( &solid, NULL );
			if ( solid.index >= 0 && solid.index < pCollide->solidCount)
			{
				if ( elementCount < RAGDOLL_MAX_ELEMENTS )
				{
					boneIndexOut[elementCount] = Studio_BoneIndexByName( pStudioHdr, solid.name );
					elementCount++;
				}
			}
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );

	return elementCount;
}

bool RagdollGetBoneMatrix( const ragdoll_t &ragdoll, matrix3x4_t *pBoneToWorld, int objectIndex )
{
	int boneIndex = ragdoll.boneIndex[objectIndex];
	if ( boneIndex < 0 )
		return false;

	const ragdollelement_t &element = ragdoll.list[objectIndex];

	element.pObject->GetPositionMatrix( pBoneToWorld[boneIndex] );
	if ( element.parentIndex >= 0 )
	{
		// overwrite the position from physics to force rigid attachment
		// UNDONE: If we support other types of constraints (or multiple constraints per object)
		// make sure these don't fight !
		int parentBoneIndex = ragdoll.boneIndex[element.parentIndex];
		Vector out;
		VectorTransform( element.originParentSpace, pBoneToWorld[parentBoneIndex], out );
		MatrixSetColumn( out, 3, pBoneToWorld[boneIndex] );
	}

	return true;
}

void RagdollComputeExactBbox( const ragdoll_t &ragdoll, const Vector &origin, Vector &outMins, Vector &outMaxs )
{
	outMins = origin;
	outMaxs = origin;

	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		Vector mins, maxs;
		Vector objectOrg;
		QAngle objectAng;
		IPhysicsObject *pObject = ragdoll.list[i].pObject;
		pObject->GetPosition( &objectOrg, &objectAng );
		physcollision->CollideGetAABB( mins, maxs, pObject->GetCollide(), objectOrg, objectAng );
		for ( int j = 0; j < 3; j++ )
		{
			if ( mins[j] < outMins[j] )
			{
				outMins[j] = mins[j];
			}
			if ( maxs[j] > outMaxs[j] )
			{
				outMaxs[j] = maxs[j];
			}
		}
	}
}

bool RagdollIsAsleep( const ragdoll_t &ragdoll )
{
	for ( int i = 0; i < ragdoll.listCount; i++ )
	{
		if ( !ragdoll.list[i].pObject->IsAsleep() )
			return false;
	}

	return true;
}
