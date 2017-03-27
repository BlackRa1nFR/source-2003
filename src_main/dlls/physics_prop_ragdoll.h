//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef PHYSICS_PROP_RAGDOLL_H
#define PHYSICS_PROP_RAGDOLL_H
#ifdef _WIN32
#pragma once
#endif

#include "ragdoll_shared.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CRagdollProp : public CBaseAnimating
{
	DECLARE_CLASS( CRagdollProp, CBaseAnimating );
public:
	CRagdollProp( void );
	~CRagdollProp( void );

	virtual void UpdateOnRemove( void );

	void DrawDebugGeometryOverlays()
	{
		if (m_debugOverlays & OVERLAY_BBOX_BIT) 
		{
			DrawServerHitboxes();
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	void Spawn( void );
	void Precache( void );

	bool ShouldSavePhysics() { return false; }

	DECLARE_SERVERCLASS();
	// Don't treat as a live target
	virtual bool IsAlive( void ) { return false; }
	
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &dir, trace_t *ptr );
	virtual bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );
	virtual void Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
	virtual void SetupBones( matrix3x4_t *pBoneToWorld, int boneMask );
	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );
	virtual void SetObjectCollisionBox( void );

	virtual int DrawDebugTextOverlays(void);

	// locals
	void InitRagdollAnimation();
	void InitRagdoll( const Vector &forceVector, int forceBone, const Vector &forcePos, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt, int collisionGroup, bool activateRagdoll );
	
	void RecheckCollisionFilter( void );
	void DisableCollisions( IPhysicsObject *pObject );
	void EnableCollisions( IPhysicsObject *pObject );
	void SetDebrisThink();
	inline ragdoll_t *GetRagdoll( void ) { return &m_ragdoll; }

	virtual bool	IsRagdoll() { return true; }

	// Damage passing
	virtual void	SetDamageEntity( CBaseEntity *pEntity );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void OnSave();
	virtual void OnRestore();
	DECLARE_DATADESC();
protected:
	void CalcRagdollSize( void );
	ragdoll_t			m_ragdoll;

private:
	void UpdateNetworkDataFromVPhysics( IPhysicsObject *pPhysics, int index );

	CNetworkArray( Vector, m_ragPos, RAGDOLL_MAX_ELEMENTS );
	CNetworkArray( QAngle, m_ragAngles, RAGDOLL_MAX_ELEMENTS );
	Vector				m_savedESMins;
	Vector				m_savedESMaxs;
	unsigned int		m_lastUpdateTickCount;
	bool				m_allAsleep;
	EHANDLE				m_hDamageEntity;
	int					m_savedListCount;
};

CBaseEntity *CreateServerRagdoll( CBaseAnimating *pAnimating, int forceBone, const CTakeDamageInfo &info, int collisionGroup );
CRagdollProp *CreateServerRagdollAttached( CBaseAnimating *pAnimating, const Vector &vecForce, int forceBone, int collisionGroup, IPhysicsObject *pAttached, CBaseAnimating *pParentEntity, int boneAttach, const Vector &originAttached, int parentBoneAttach, const Vector &boneOrigin );
extern void DetachAttachedRagdoll( CBaseEntity *pRagdollIn );

#endif // PHYSICS_PROP_RAGDOLL_H
