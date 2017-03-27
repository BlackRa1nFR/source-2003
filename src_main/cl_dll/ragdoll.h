//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef RAGDOLL_H
#define RAGDOLL_H

#ifdef _WIN32
#pragma once
#endif


class C_BaseEntity;
struct studiohdr_t;
struct mstudiobone_t;
class Vector;
class IPhysicsObject;

class IRagdoll
{
public:
	virtual ~IRagdoll() {}

	virtual void RagdollBone( C_BaseEntity *ent, mstudiobone_t *pbones, int boneCount, bool *boneSimulated, matrix3x4_t *pBoneToWorld ) = 0;
	virtual Vector GetRagdollOrigin( ) = 0;
	virtual void GetRagdollBounds( Vector &mins, Vector &maxs ) = 0;
	virtual IPhysicsObject *GetElement( int elementNum ) = 0;
	virtual void DrawWireframe( void ) = 0;
	virtual void VPhysicsUpdate( IPhysicsObject *pObject ) = 0;
};

IRagdoll *CreateRagdoll( C_BaseEntity *ent, studiohdr_t *pstudiohdr, const Vector &forceVector, int forceBone, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt );


#endif // RAGDOLL_H