//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef PHYSICS_BONE_FOLLOWER_H
#define PHYSICS_BONE_FOLLOWER_H
#ifdef _WIN32
#pragma once
#endif

class CBoneFollower;

//
// To use bone followers in an entity, contain a CBoneFollowerManager in it. Then:
//		- Call InitBoneFollowers() in the entity's CreateVPhysics().
//		- Call UpdateBoneFollowers() after you move your bones.
//		- Call DestroyBoneFollowers() when your entity's removed

struct physfollower_t
{
	int boneIndex;
	CHandle<CBoneFollower> hFollower;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBoneFollowerManager
{
public:
	CBoneFollowerManager( void );

	// Call this in your entity's CreateVPhysics()
	void InitBoneFollowers( CBaseEntity *pEntity, int iNumBones, const char **pFollowerBoneNames );

	// Call this after you move your bones
	void UpdateBoneFollowers( void );

	// Call this when your entity's removed
	void DestroyBoneFollowers( void );

	physfollower_t *GetBoneFollower( int iFollowerIndex );

private:
	bool CreatePhysicsFollower( physfollower_t &follow, const char *pBoneName );

private:
	CHandle<CBaseAnimating>		m_hOuter;
	int							m_iNumBones;
	CUtlVector<physfollower_t>	m_physBones;
};


class CBoneFollower : public CBaseEntity
{
	DECLARE_CLASS( CBoneFollower, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
public:
	// CBaseEntity
	void DrawDebugGeometryOverlays();
	void SetObjectCollisionBox( void );
	void VPhysicsUpdate( IPhysicsObject *pPhysics );
	bool ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );

	// NOTE: These are forwarded to the parent object!
	void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	void VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit );
	void VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );

	bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );
	// assume these are re-created by the owner objects
	int	 ObjectCaps( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	void Touch( CBaseEntity *pOther );

	// locals
	bool Init( CBaseEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation );
	void UpdateFollower( const Vector &position, const QAngle &orientation, float flInterval );

	// factory
	static CBoneFollower *Create( CBaseEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation );

private:
	CNetworkVar( int, m_modelIndex );
	CNetworkVar( int, m_solidIndex );
};

#endif // PHYSICS_BONE_FOLLOWER_H
