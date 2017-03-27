//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "baseanimating.h"
#include "studio.h"
#include "physics.h"
#include "physics_saverestore.h"
#include "ai_basenpc.h"
#include "vphysics/constraints.h"
#include "bone_setup.h"
#include "physics_prop_ragdoll.h"

const char *GetMassEquivalent(float flMass);


#define	SF_RAGDOLLPROP_DEBRIS		0x0004

LINK_ENTITY_TO_CLASS( physics_prop_ragdoll, CRagdollProp );
LINK_ENTITY_TO_CLASS( prop_ragdoll, CRagdollProp );
EXTERN_SEND_TABLE(DT_Ragdoll)

IMPLEMENT_SERVERCLASS_ST(CRagdollProp, DT_Ragdoll)
	SendPropArray	(SendPropQAngles(SENDINFO_ARRAY(m_ragAngles), 13, 0 ), m_ragAngles),
	SendPropArray	(SendPropVector(SENDINFO_ARRAY(m_ragPos), -1, SPROP_COORD ), m_ragPos),
END_SEND_TABLE()

BEGIN_DATADESC(CRagdollProp)
//	DEFINE_PHYSPTR_ARRAY( CRagdollProp,	m_ragdoll						),
	DEFINE_AUTO_ARRAY	( CRagdollProp,	m_ragdoll.boneIndex,	FIELD_INTEGER	),
	DEFINE_AUTO_ARRAY	( CRagdollProp,	m_ragPos,		FIELD_VECTOR	),
	DEFINE_AUTO_ARRAY	( CRagdollProp,	m_ragAngles,	FIELD_VECTOR	),
	// store these to recreate the initial state of the ragdoll
	DEFINE_FIELD		( CRagdollProp,	m_savedListCount,	FIELD_INTEGER	),
//	DEFINE_FIELD		( CRagdollProp,	m_savedESMins,	FIELD_VECTOR	),
//	DEFINE_FIELD		( CRagdollProp,	m_savedESMaxs,	FIELD_VECTOR	),
	DEFINE_FIELD( CRagdollProp, m_lastUpdateTickCount, FIELD_INTEGER ),
	DEFINE_FIELD( CRagdollProp, m_allAsleep, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRagdollProp, m_hDamageEntity, FIELD_EHANDLE ),

	// think functions
	DEFINE_FUNCTION( CRagdollProp, SetDebrisThink ),

END_DATADESC()

void CRagdollProp::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );

	Relink();

	matrix3x4_t pBoneToWorld[MAXSTUDIOBONES];
	BaseClass::SetupBones( pBoneToWorld, BONE_USED_BY_ANYTHING ); // FIXME: shouldn't this be a subset of the bones
	int collisionGroup = (m_spawnflags & SF_RAGDOLLPROP_DEBRIS) ? COLLISION_GROUP_DEBRIS : COLLISION_GROUP_NONE;
	InitRagdoll( vec3_origin, 0, vec3_origin, pBoneToWorld, pBoneToWorld, 0, collisionGroup, true );

	// only send data when physics moves this object
	NetworkStateManualMode( true );
	m_lastUpdateTickCount = 0;
}

void CRagdollProp::OnSave()
{
	// save this for bone reference
	m_savedListCount = m_ragdoll.listCount;
	VPhysicsSetObject( NULL );
	BaseClass::OnSave();
	VPhysicsSetObject( m_ragdoll.list[0].pObject );
}

void CRagdollProp::OnRestore()
{
	BaseClass::OnRestore();
	matrix3x4_t pBoneToWorld[MAXSTUDIOBONES];
	BaseClass::SetupBones( pBoneToWorld, BONE_USED_BY_ANYTHING ); // FIXME: shouldn't this be a subset of the bones

	QAngle angles;
	Vector origin;
	for ( int i = 0; i < m_savedListCount; i++ )
	{
		angles = m_ragAngles[ i ];
		origin = m_ragPos[i];
		AngleMatrix( angles, origin, pBoneToWorld[m_ragdoll.boneIndex[i]] );
	}
	InitRagdoll( vec3_origin, 0, vec3_origin, pBoneToWorld, pBoneToWorld, 0, GetCollisionGroup(), true );
}

void CRagdollProp::CalcRagdollSize( void )
{
	Vector minbox, maxbox;

	ExtractBbox( GetSequence(), minbox, maxbox );
	SetCollisionBounds( minbox, maxbox );

	if ( m_ragdoll.list[0].pObject )
	{
		Vector rootPos, rootWorldPos;
		m_ragdoll.list[0].pObject->GetPosition( &rootWorldPos, NULL );
		WorldToEntitySpace( rootWorldPos, &rootPos );

		// BUGBUG: This doesn't work because the sequence doesn't necessarily include
		//		   the extrema of pose space.
		// An algorithm that should work correctly is to store a radius in each bone
		// Then walk the skeleton away from the root to each leaf, accumulating distance to parent
		// The node or leaf with the largest radius + Sum(dist to parent) is the ragdoll's radius
		// Next rev of the model file format, I'll add bone radius and fix this.
		Vector dist;
		const Vector &center = EntitySpaceCenter();
		int i;
		for ( i = 0; i < 3; i++ )
		{
			if ( rootPos[i] > center[i] )
			{
				dist[i] = rootPos[i] - EntitySpaceMins()[i];
			}
			else
			{
				dist[i] = EntitySpaceMaxs()[i] - rootPos[i];
			}
		}

		float radius = dist.Length();
		Vector curmins( -radius, -radius, -radius );
		Vector curmaxs( radius, radius, radius );
		SetCollisionBounds( curmins, curmaxs );
	}

	m_savedESMins = EntitySpaceMins();
	m_savedESMaxs = EntitySpaceMaxs();
	Relink();
}

void CRagdollProp::UpdateOnRemove( void )
{
	// Set to null so that the destructor's call to DestroyObject won't destroy
	//  m_pObjects[ 0 ] twice since that's the physics object for the prop
	VPhysicsSetObject( NULL );

	RagdollDestroy( m_ragdoll );
	// Chain to base after doing our own cleanup to mimic
	//  destructor unwind order
	BaseClass::UpdateOnRemove();
}

CRagdollProp::CRagdollProp( void )
{
	m_ragdoll.listCount = 0;
	Assert( (1<<RAGDOLL_INDEX_BITS) >=RAGDOLL_MAX_ELEMENTS );
	m_allAsleep = false;
}

CRagdollProp::~CRagdollProp( void )
{
}

void CRagdollProp::Precache( void )
{
	engine->PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}

void CRagdollProp::InitRagdollAnimation()
{
	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	m_flCycle = 0;
	
	// put into ACT_DIERAGDOLL if it exists, otherwise use sequence 0
	int nSequence = SelectWeightedSequence( ACT_DIERAGDOLL );
	if ( nSequence < 0 )
	{
		ResetSequence( 0 );
	}
	else
	{
		ResetSequence( nSequence );
	}
}

void CRagdollProp::InitRagdoll( const Vector &forceVector, int forceBone, const Vector &forcePos, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt, int collisionGroup, bool activateRagdoll )
{
	SetCollisionGroup( collisionGroup );

	if ( collisionGroup == COLLISION_GROUP_INTERACTIVE_DEBRIS )
	{
		SetThink( &CRagdollProp::SetDebrisThink );
		SetNextThink( gpGlobals->curtime + 5 );
	}
	SetMoveType( MOVETYPE_VPHYSICS );
	SetSolid( SOLID_VPHYSICS );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
	m_takedamage = DAMAGE_EVENTS_ONLY;

	ragdollparams_t params;
	params.pGameData = static_cast<void *>( static_cast<CBaseEntity *>(this) );
	params.pCollide = modelinfo->GetVCollide( GetModelIndex() );
	params.pStudioHdr = GetModelPtr();
	params.forceVector = forceVector;
	params.forceBoneIndex = forceBone;
	params.forcePosition = forcePos;
	params.pPrevBones = pPrevBones;
	params.pCurrentBones = pBoneToWorld;
	params.boneDt = dt;
	params.jointFrictionScale = 1.0;
	RagdollCreate( m_ragdoll, params, physcollision, physenv, physprops );

	if ( activateRagdoll )
	{
		RagdollActivate( m_ragdoll );
	}

	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		UpdateNetworkDataFromVPhysics( m_ragdoll.list[i].pObject, i );
	}
	VPhysicsSetObject( m_ragdoll.list[0].pObject );

	CalcRagdollSize();
}

void CRagdollProp::SetDebrisThink()
{
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		m_ragdoll.list[i].pObject->RecheckCollisionFilter();
	}
}

void CRagdollProp::SetDamageEntity( CBaseEntity *pEntity )
{
	// Damage passing
	m_hDamageEntity = pEntity;

	// Set our takedamage to match it
	if ( pEntity )
	{
		m_takedamage = pEntity->m_takedamage;
	}
	else
	{
		m_takedamage = DAMAGE_EVENTS_ONLY;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CRagdollProp::OnTakeDamage( const CTakeDamageInfo &info )
{
	// If we have a damage entity, we want to pass damage to it. Add the
	// Never Ragdoll flag, on the assumption that if the entity dies, we'll
	// actually be taking the role of its ragdoll.
	if ( m_hDamageEntity.Get() )
	{
		CTakeDamageInfo subInfo = info;
		subInfo.AddDamageType( DMG_REMOVENORAGDOLL );
		return m_hDamageEntity->OnTakeDamage( subInfo );
	}

	return BaseClass::OnTakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: Force all the ragdoll's bone's physics objects to recheck their collision filters
//-----------------------------------------------------------------------------
void CRagdollProp::RecheckCollisionFilter( void )
{
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		m_ragdoll.list[i].pObject->RecheckCollisionFilter();
	}
}

void CRagdollProp::DisableCollisions( IPhysicsObject *pObject )
{
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		physenv->DisableCollisions( m_ragdoll.list[i].pObject, pObject );
		m_ragdoll.list[i].pObject->RecheckCollisionFilter();
	}
}

void CRagdollProp::EnableCollisions( IPhysicsObject *pObject )
{
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		physenv->EnableCollisions( m_ragdoll.list[i].pObject, pObject );
		m_ragdoll.list[i].pObject->RecheckCollisionFilter();
	}
}


void CRagdollProp::TraceAttack( const CTakeDamageInfo &info, const Vector &dir, trace_t *ptr )
{
	if ( ptr->physicsbone >= 0 && ptr->physicsbone < m_ragdoll.listCount )
	{
		VPhysicsSwapObject( m_ragdoll.list[ptr->physicsbone].pObject );
	}
	BaseClass::TraceAttack( info, dir, ptr );
}

void CRagdollProp::SetupBones( matrix3x4_t *pBoneToWorld, int boneMask )
{
	studiohdr_t *pStudioHdr = GetModelPtr( );
	bool sim[MAXSTUDIOBONES];
	memset( sim, 0, pStudioHdr->numbones );

	int i;

	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		if ( RagdollGetBoneMatrix( m_ragdoll, pBoneToWorld, i ) )
		{
			sim[m_ragdoll.boneIndex[i]] = true;
		}
	}

	mstudiobone_t *pbones = pStudioHdr->pBone( 0 );
	for ( i = 0; i < pStudioHdr->numbones; i++ )
	{
		if ( sim[i] )
			continue;

		MatrixCopy( pBoneToWorld[pbones[i].parent], pBoneToWorld[ i ] );
	}
}

bool CRagdollProp::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	Vector origin = 0.5*(GetAbsMins() + GetAbsMaxs());
	Vector size = 0.5*(GetAbsMaxs()-GetAbsMins());

	if ( ray.m_IsRay )
	{
		return BaseClass::TestCollision( ray, mask, trace );
	}

	studiohdr_t *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
		return false;

	vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );

	// Just iterate all of the elements and trace the box against each one.
	// NOTE: This is pretty expensive for small/dense characters
	trace_t tr;
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		physcollision->TraceBox( ray, pCollide->solids[i], m_ragPos[i], m_ragAngles[i], &tr );

		if ( tr.fraction < trace.fraction )
		{
			trace = tr;
		}
	}

	if ( trace.fraction >= 1 )
	{
		return false;
	}

	return true;
}


void CRagdollProp::Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	matrix3x4_t startMatrixInv;
	matrix3x4_t startMatrix;

	m_ragdoll.list[0].pObject->GetPositionMatrix( startMatrix );
	MatrixInvert( startMatrix, startMatrixInv );
	
	// object 0 MUST be the one to get teleported!
	VPhysicsSwapObject( m_ragdoll.list[0].pObject );
	BaseClass::Teleport( newPosition, newAngles, newVelocity );

	// Calculate the relative transform of the teleport
	matrix3x4_t xform;
	ConcatTransforms( EntityToWorldTransform(), startMatrixInv, xform );
	UpdateNetworkDataFromVPhysics( m_ragdoll.list[0].pObject, 0 );
	for ( int i = 1; i < m_ragdoll.listCount; i++ )
	{
		matrix3x4_t matrix, newMatrix;
		m_ragdoll.list[i].pObject->GetPositionMatrix( matrix );
		ConcatTransforms( xform, matrix, newMatrix );
		m_ragdoll.list[i].pObject->SetPositionMatrix( newMatrix, true );
		UpdateNetworkDataFromVPhysics( m_ragdoll.list[i].pObject, i );
	}
}

void CRagdollProp::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	if ( m_lastUpdateTickCount == (unsigned int)gpGlobals->tickcount )
		return;

	m_lastUpdateTickCount = gpGlobals->tickcount;
	NetworkStateChanged();

	matrix3x4_t boneToWorld[MAXSTUDIOBONES];
	QAngle angles;
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		RagdollGetBoneMatrix( m_ragdoll, boneToWorld, i );
		
		Vector vNewPos;
		MatrixAngles( boneToWorld[m_ragdoll.boneIndex[i]], angles, vNewPos );
		m_ragPos.Set( i, vNewPos );
		m_ragAngles.Set( i, angles );
	}

	m_allAsleep = RagdollIsAsleep( m_ragdoll );
	SetAbsOrigin( m_ragPos[0] );
	engine->RelinkEntity( pev, true );
}

void CRagdollProp::UpdateNetworkDataFromVPhysics( IPhysicsObject *pPhysics, int index )
{
	Assert(index < m_ragdoll.listCount);

	QAngle angles;
	Vector vPos;
	m_ragdoll.list[index].pObject->GetPosition( &vPos, &angles );
	m_ragPos.Set( index, vPos );
	m_ragAngles.Set( index, angles );
	// move/relink if root moved
	if ( index == 0 )
	{
		SetAbsOrigin( m_ragPos[0] );
		engine->RelinkEntity( pev, true );
	}
}


void CRagdollProp::SetObjectCollisionBox( void )
{
	if ( m_allAsleep )
	{
		Vector fullMins, fullMaxs;
		RagdollComputeExactBbox( m_ragdoll, GetAbsOrigin(), fullMins, fullMaxs );
		SetCollisionBounds( fullMins - GetAbsOrigin(), fullMaxs - GetAbsOrigin() );
		SetAbsMins( fullMins );
		SetAbsMaxs( fullMaxs );
	}
	else
	{
		SetCollisionBounds( m_savedESMins, m_savedESMaxs );
		ComputeSurroundingBox();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CRagdollProp::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		if (VPhysicsGetObject())
		{
			char tempstr[512];
			Q_snprintf(tempstr, sizeof(tempstr),"Mass: %.2f kg / %.2f lb (%s)", VPhysicsGetObject()->GetMass(), kg2lbs(VPhysicsGetObject()->GetMass()), GetMassEquivalent(VPhysicsGetObject()->GetMass()));
			NDebugOverlay::EntityText(entindex(), text_offset, tempstr, 0);
			text_offset++;
		}
	}

	return text_offset;
}

//===============================================================================================================
// RagdollPropAttached
//===============================================================================================================
class CRagdollPropAttached : public CRagdollProp
{
	DECLARE_CLASS( CRagdollPropAttached, CRagdollProp );
public:

	CRagdollPropAttached()
	{
		m_bShouldDetach = false;
	}

	~CRagdollPropAttached()
	{
		physenv->DestroyConstraint( m_pAttachConstraint );
		m_pAttachConstraint = NULL;
	}

	void InitRagdollAttached( IPhysicsObject *pAttached, const Vector &forceVector, int forceBone, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt, int collisionGroup, CBaseAnimating *pFollow, int boneIndexRoot, const Vector &boneLocalOrigin, int parentBoneAttach, const Vector &worldAttachOrigin );
	void DetachOnNextUpdate();
	void VPhysicsUpdate( IPhysicsObject *pPhysics );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	void Detach();
	CNetworkVar( int, m_boneIndexAttached );
	CNetworkVar( int, m_ragdollAttachedObjectIndex );
	CNetworkVector( m_attachmentPointBoneSpace );
	CNetworkVector( m_attachmentPointRagdollSpace );
	bool		m_bShouldDetach;
	IPhysicsConstraint	*m_pAttachConstraint;
};

LINK_ENTITY_TO_CLASS( prop_ragdoll_attached, CRagdollPropAttached );
EXTERN_SEND_TABLE(DT_Ragdoll_Attached)

IMPLEMENT_SERVERCLASS_ST(CRagdollPropAttached, DT_Ragdoll_Attached)
	SendPropInt( SENDINFO( m_boneIndexAttached ), MAXSTUDIOBONEBITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_ragdollAttachedObjectIndex ), RAGDOLL_INDEX_BITS, SPROP_UNSIGNED ),
	SendPropVector(SENDINFO(m_attachmentPointBoneSpace), -1,  SPROP_COORD ),
	SendPropVector(SENDINFO(m_attachmentPointRagdollSpace), -1,  SPROP_COORD ),
END_SEND_TABLE()

BEGIN_DATADESC(CRagdollPropAttached)
	DEFINE_FIELD( CRagdollPropAttached,	m_boneIndexAttached,	FIELD_INTEGER ),
	DEFINE_FIELD( CRagdollPropAttached, m_ragdollAttachedObjectIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CRagdollPropAttached,	m_attachmentPointBoneSpace,	FIELD_VECTOR ),
	DEFINE_FIELD( CRagdollPropAttached, m_attachmentPointRagdollSpace, FIELD_VECTOR ),
	DEFINE_FIELD( CRagdollPropAttached, m_bShouldDetach, FIELD_BOOLEAN ),
	DEFINE_PHYSPTR( CRagdollPropAttached, m_pAttachConstraint ),
END_DATADESC()


static void SyncAnimatingWithPhysics( CBaseAnimating *pAnimating )
{
	IPhysicsObject *pPhysics = pAnimating->VPhysicsGetObject();
	if ( pPhysics )
	{
		Vector pos;
		pPhysics->GetShadowPosition( &pos, NULL );
		pAnimating->SetAbsOrigin( pos );
	}
}


CBaseEntity *CreateServerRagdoll( CBaseAnimating *pAnimating, int forceBone, const CTakeDamageInfo &info, int collisionGroup )
{
	SyncAnimatingWithPhysics( pAnimating );

	CRagdollProp *pRagdoll = (CRagdollProp *)CBaseEntity::CreateNoSpawn( "prop_ragdoll", pAnimating->GetAbsOrigin(), vec3_angle, NULL );
	pRagdoll->CopyAnimationDataFrom( pAnimating );

	pRagdoll->InitRagdollAnimation();
	matrix3x4_t pBoneToWorld[MAXSTUDIOBONES];
	pAnimating->SetupBones( pBoneToWorld, BONE_USED_BY_ANYTHING );
	
	// Is this a vehicle / NPC collision?
	if ( (info.GetDamageType() & DMG_VEHICLE) && pAnimating->MyNPCPointer() )
	{
		// init the ragdoll with no forces
		pRagdoll->InitRagdoll( vec3_origin, -1, vec3_origin, pBoneToWorld, pBoneToWorld, 0.1, collisionGroup, true );

		// apply vehicle forces
		// Get a list of bones with hitboxes below the plane of impact
		int boxList[128];
		Vector normal(0,0,-1);
		int count = pAnimating->GetHitboxesFrontside( boxList, ARRAYSIZE(boxList), normal, DotProduct( normal, info.GetDamagePosition() ) );
		
		// distribute force over mass of entire character
		float massScale = Studio_GetMass(pAnimating->GetModelPtr());
		massScale = clamp( massScale, 1, 1e4 );
		massScale = 1 / massScale;

		// distribute the force
		// BUGBUG: This will hit the same bone twice if it has two hitboxes!!!!
		ragdoll_t *pRagInfo = pRagdoll->GetRagdoll();
		for ( int i = 0; i < count; i++ )
		{
			int physBone = pAnimating->GetPhysicsBone( pAnimating->GetHitboxBone( boxList[i] ) );
			IPhysicsObject *pPhysics = pRagInfo->list[physBone].pObject;
			pPhysics->ApplyForceCenter( info.GetDamageForce() * pPhysics->GetMass() * massScale );
		}
	}
	else
	{
		pRagdoll->InitRagdoll( info.GetDamageForce(), forceBone, info.GetDamagePosition(), pBoneToWorld, pBoneToWorld, 0.1, collisionGroup, true );
	}

	return pRagdoll;
}

void CRagdollPropAttached::DetachOnNextUpdate()
{
	m_bShouldDetach = true;
}

void CRagdollPropAttached::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	if ( m_bShouldDetach )
	{
		Detach();
		m_bShouldDetach = false;
	}
	BaseClass::VPhysicsUpdate( pPhysics );
}

void CRagdollPropAttached::Detach()
{
	StopFollowingEntity();
	SetOwnerEntity( NULL );
	SetAbsAngles( vec3_angle );
	SetMoveType( MOVETYPE_VPHYSICS );
	physenv->DestroyConstraint( m_pAttachConstraint );
	m_pAttachConstraint = NULL;

	// Go non-solid
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	Relink();
	RecheckCollisionFilter();
}

void CRagdollPropAttached::InitRagdollAttached( IPhysicsObject *pAttached, const Vector &forceVector, int forceBone, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt, int collisionGroup, CBaseAnimating *pFollow, int boneIndexRoot, const Vector &boneLocalOrigin, int parentBoneAttach, const Vector &worldAttachOrigin )
{
	int ragdollAttachedIndex = 0;
	if ( parentBoneAttach > 0 )
	{
		studiohdr_t *pStudioHdr = GetModelPtr();
		mstudiobone_t *pBone = pStudioHdr->pBone( parentBoneAttach );
		ragdollAttachedIndex = pBone->physicsbone;
	}

	InitRagdoll( forceVector, forceBone, vec3_origin, pPrevBones, pBoneToWorld, dt, collisionGroup, false );
	
	IPhysicsObject *pRefObject = m_ragdoll.list[ragdollAttachedIndex].pObject;

	Vector attachmentPointRagdollSpace;
	pRefObject->WorldToLocal( attachmentPointRagdollSpace, worldAttachOrigin );

	constraint_ragdollparams_t constraint;
	constraint.Defaults();
	matrix3x4_t tmp, worldToAttached, worldToReference, constraintToWorld;

	Vector offsetWS;
	pAttached->LocalToWorld( offsetWS, boneLocalOrigin );

	AngleMatrix( QAngle(0, pFollow->GetAbsAngles().y, 0 ), offsetWS, constraintToWorld );

	constraint.axes[0].SetAxisFriction( -2, 2, 20 );
	constraint.axes[1].SetAxisFriction( 0, 0, 0 );
	constraint.axes[2].SetAxisFriction( -15, 15, 20 );
	
	pAttached->GetPositionMatrix( tmp );
	MatrixInvert( tmp, worldToAttached );

	pRefObject->GetPositionMatrix( tmp );
	MatrixInvert( tmp, worldToReference );

	ConcatTransforms( worldToReference, constraintToWorld, constraint.constraintToReference );
	ConcatTransforms( worldToAttached, constraintToWorld, constraint.constraintToAttached );

	// for now, just slam this to be the passed in value
	MatrixSetColumn( attachmentPointRagdollSpace, 3, constraint.constraintToReference );

	DisableCollisions( pAttached );
	m_pAttachConstraint = physenv->CreateRagdollConstraint( pRefObject, pAttached, m_ragdoll.pGroup, constraint );

	FollowEntity( pFollow );
	SetOwnerEntity( pFollow );
	RagdollActivate( m_ragdoll );

	Relink();
	m_boneIndexAttached = boneIndexRoot;
	m_ragdollAttachedObjectIndex = ragdollAttachedIndex;
	m_attachmentPointBoneSpace = boneLocalOrigin;
	
	Vector vTemp;
	MatrixGetColumn( constraint.constraintToReference, 3, vTemp );
	m_attachmentPointRagdollSpace = vTemp;
}

CRagdollProp *CreateServerRagdollAttached( CBaseAnimating *pAnimating, const Vector &vecForce, int forceBone, int collisionGroup, IPhysicsObject *pAttached, CBaseAnimating *pParentEntity, int boneAttach, const Vector &originAttached, int parentBoneAttach, const Vector &boneOrigin )
{
	CRagdollPropAttached *pRagdoll = (CRagdollPropAttached *)CBaseEntity::CreateNoSpawn( "prop_ragdoll_attached", pAnimating->GetAbsOrigin(), vec3_angle, NULL );
	pRagdoll->CopyAnimationDataFrom( pAnimating );

	pRagdoll->InitRagdollAnimation();
	matrix3x4_t pBoneToWorld[MAXSTUDIOBONES];
	pAnimating->SetupBones( pBoneToWorld, BONE_USED_BY_ANYTHING );
	pRagdoll->InitRagdollAttached( pAttached, vecForce, forceBone, pBoneToWorld, pBoneToWorld, 0.1, collisionGroup, pParentEntity, boneAttach, boneOrigin, parentBoneAttach, originAttached );
	
	return pRagdoll;
}

void DetachAttachedRagdoll( CBaseEntity *pRagdollIn )
{
	CRagdollPropAttached *pRagdoll = dynamic_cast<CRagdollPropAttached *>(pRagdollIn);

	if ( pRagdoll )
	{
		pRagdoll->DetachOnNextUpdate();
	}
}

