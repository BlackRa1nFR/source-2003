//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "vmatrix.h"
#include "ragdoll_shared.h"
#include "bone_setup.h"
#include "materialsystem/imesh.h"
#include "engine/ivmodelinfo.h"
#include "iviewrender.h"
#include "tier0/vprof.h"

#define RAGDOLL_VISUALIZE	0
class CRagdoll : public IRagdoll
{
public:
	CRagdoll();
	~CRagdoll( void );
	
	void Init( C_BaseEntity *ent, studiohdr_t *pstudiohdr, const Vector &forceVector, int forceBone, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt );

	virtual void RagdollBone( C_BaseEntity *ent, mstudiobone_t *pbones, int boneCount, bool *boneSimulated, matrix3x4_t *pBoneToWorld );
	virtual Vector GetRagdollOrigin( );
	virtual void GetRagdollBounds( Vector &theMins, Vector &theMaxs );
	
	virtual IPhysicsObject *GetElement( int elementNum );
	virtual void DrawWireframe();
	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );

	bool IsValid() { return m_ragdoll.listCount > 0; }

private:
	ragdoll_t			m_ragdoll;
	Vector				m_mins, m_maxs;
	float				m_radius;
	float				m_lastUpdate;
	bool				m_allAsleep;
#if RAGDOLL_VISUALIZE
	matrix3x4_t			m_savedBone1[MAXSTUDIOBONES];
	matrix3x4_t			m_savedBone2[MAXSTUDIOBONES];
#endif
};


CRagdoll::CRagdoll()
{
	m_ragdoll.listCount = 0;
}


IPhysicsObject *CRagdoll::GetElement( int elementNum )
{ 
	return m_ragdoll.list[elementNum].pObject;
}


void CRagdoll::Init( C_BaseEntity *ent, studiohdr_t *pstudiohdr, const Vector &forceVector, int forceBone, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt )
{
	ragdollparams_t params;
	params.pGameData = static_cast<void *>( ent );
	params.pCollide = modelinfo->GetVCollide( ent->GetModelIndex() );
	params.pStudioHdr = pstudiohdr;
	params.forceVector = forceVector;
	params.forceBoneIndex = forceBone;
	params.forcePosition.Init();
	params.pPrevBones = pPrevBones;
	params.pCurrentBones = pBoneToWorld;
	params.boneDt = dt;
	params.jointFrictionScale = 1.0;
	RagdollCreate( m_ragdoll, params, physcollision, physenv, physprops );
	RagdollActivate( m_ragdoll );

	if ( !m_ragdoll.listCount )
		return;

	Vector mins, maxs, size;
	modelinfo->GetModelBounds( ent->GetModel(), mins, maxs );
	size = (maxs - mins) * 0.5;
	m_radius = size.Length();

	m_mins.Init(-m_radius,-m_radius,-m_radius);
	m_maxs.Init(m_radius,m_radius,m_radius);
#if RAGDOLL_VISUALIZE
	memcpy( m_savedBone1, pPrevBones, sizeof(matrix3x4_t) * pstudiohdr->numbones );
	memcpy( m_savedBone2, pBoneToWorld, sizeof(matrix3x4_t) * pstudiohdr->numbones );
#endif
}

CRagdoll::~CRagdoll( void )
{
	RagdollDestroy( m_ragdoll );
}


void CRagdoll::RagdollBone( C_BaseEntity *ent, mstudiobone_t *pbones, int boneCount, bool *boneSimulated, matrix3x4_t *pBoneToWorld )
{
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		if ( RagdollGetBoneMatrix( m_ragdoll, pBoneToWorld, i ) )
		{
			boneSimulated[m_ragdoll.boneIndex[i]] = true;
		}
	}
}

Vector CRagdoll::GetRagdollOrigin( )
{
	Vector origin;
	m_ragdoll.list[0].pObject->GetPosition( &origin, 0 );
	return origin;
}

void CRagdoll::GetRagdollBounds( Vector &theMins, Vector &theMaxs )
{
	theMins = m_mins;
	theMaxs = m_maxs;
}

void CRagdoll::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	if ( m_lastUpdate == gpGlobals->curtime )
		return;
	m_lastUpdate = gpGlobals->curtime;
	m_allAsleep = RagdollIsAsleep( m_ragdoll );
	if ( m_allAsleep )
	{
		// NOTE: This is the bbox of the ragdoll's physics
		// It's not always correct to use for culling, but it sure beats 
		// using the radius box!
		Vector origin = GetRagdollOrigin();
		RagdollComputeExactBbox( m_ragdoll, origin, m_mins, m_maxs );
		m_mins -= origin;
		m_maxs -= origin;
	}
	else
	{
		m_mins.Init(-m_radius,-m_radius,-m_radius);
		m_maxs.Init(m_radius,m_radius,m_radius);
	}
}

void CRagdoll::DrawWireframe()
{
	IMaterial *pWireframe = materials->FindMaterial("shadertest/wireframevertexcolor", NULL);

	int i;
	matrix3x4_t matrix;
	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		static color32 debugColor = {0,255,255,0};

		// draw the actual physics positions, not the cleaned up animation position
		m_ragdoll.list[i].pObject->GetPositionMatrix( matrix );
		const CPhysCollide *pCollide = m_ragdoll.list[i].pObject->GetCollide();
		engine->DebugDrawPhysCollide( pCollide, pWireframe, matrix, debugColor );
	}

#if RAGDOLL_VISUALIZE
	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		static color32 debugColor = {255,0,0,0};

		const CPhysCollide *pCollide = m_ragdoll.list[i].pObject->GetCollide();
		engine->DebugDrawPhysCollide( pCollide, pWireframe, m_savedBone1[m_ragdoll.boneIndex[i]].matrix, debugColor );
	}
	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		static color32 debugColor = {0,255,0,0};

		const CPhysCollide *pCollide = m_ragdoll.list[i].pObject->GetCollide();
		engine->DebugDrawPhysCollide( pCollide, pWireframe, m_savedBone2[m_ragdoll.boneIndex[i]].matrix, debugColor );
	}
#endif
}


IRagdoll *CreateRagdoll( C_BaseEntity *ent, studiohdr_t *pstudiohdr, const Vector &forceVector, int forceBone, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt )
{
	CRagdoll *pRagdoll = new CRagdoll;
	pRagdoll->Init( ent, pstudiohdr, forceVector, forceBone, pPrevBones, pBoneToWorld, dt );

	if ( !pRagdoll->IsValid() )
	{
		Msg("Bad ragdoll for %s\n", pstudiohdr->name );
		delete pRagdoll;
		pRagdoll = NULL;
	}
	return pRagdoll;
}

template<class Type, int COUNT>
CUtlFixedLinkedList< CInterpolatedVarArray<Type,COUNT>::CInterpolatedVarEntry >	CInterpolatedVarArray<Type,COUNT>::m_VarHistory;


class C_ServerRagdoll : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_ServerRagdoll, C_BaseAnimating );
	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();


	C_ServerRagdoll( void )
	{
		m_elementCount = 0;

		AddVar( m_ragPos, &m_iv_ragPos, LATCH_SIMULATION_VAR | EXCLUDE_AUTO_LATCH );
		AddVar( m_ragAngles, &m_iv_ragAngles, LATCH_SIMULATION_VAR | EXCLUDE_AUTO_LATCH );
	}

	/*
	enum
	{
		RAGDOLL_OUTPUT,
		RAGDOLL_PREVIOUS,

		NUM_RAGDOLL_HISTORIES,
	};
	*/

	// Incoming from network
	Vector		m_ragPos[RAGDOLL_MAX_ELEMENTS];
	QAngle		m_ragAngles[RAGDOLL_MAX_ELEMENTS];

	//Vector		m_ragPosHistory[ NUM_RAGDOLL_HISTORIES ][RAGDOLL_MAX_ELEMENTS];
	//float		m_ragAnglesHistory[ NUM_RAGDOLL_HISTORIES ][RAGDOLL_MAX_ELEMENTS*3];
	CInterpolatedVarArray< Vector, RAGDOLL_MAX_ELEMENTS >	m_iv_ragPos;
	CInterpolatedVarArray< QAngle, RAGDOLL_MAX_ELEMENTS >	m_iv_ragAngles;

	int			m_elementCount;
	int			m_boneIndex[RAGDOLL_MAX_ELEMENTS];

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		BaseClass::PostDataUpdate( updateType );

		m_iv_ragPos.NoteChanged( this, LATCH_SIMULATION_VAR, gpGlobals->curtime );
		m_iv_ragAngles.NoteChanged( this, LATCH_SIMULATION_VAR, gpGlobals->curtime );
	}

	virtual int InternalDrawModel( int flags )
	{
		if ( vcollide_wireframe.GetBool() )
		{
			vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
			IMaterial *pWireframe = materials->FindMaterial("shadertest/wireframevertexcolor", NULL);

			matrix3x4_t matrix;
			for ( int i = 0; i < m_elementCount; i++ )
			{
				static color32 debugColor = {0,255,255,0};

				AngleMatrix( m_ragAngles[i], m_ragPos[i], matrix );
				engine->DebugDrawPhysCollide( pCollide->solids[i], pWireframe, matrix, debugColor );
			}
		}
		return BaseClass::InternalDrawModel( flags );
	}


	virtual studiohdr_t *OnNewModel( void )
	{
		studiohdr_t *hdr = BaseClass::OnNewModel();

		if ( !m_elementCount )
		{
			vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
			if ( !pCollide )
				Error( "C_ServerRagdoll::InitModel: missing vcollide data" );

			m_elementCount = RagdollExtractBoneIndices( m_boneIndex, hdr, pCollide );
		}
		return hdr;
	}

	studiovisible_t TestVisibility( void )
	{
		// FIXME: Use WorldSpaceSurroundingMins
		Vector boxmins, boxmaxs;
		WorldSpaceAABB( &boxmins, &boxmaxs );
		Vector vecDelta;
		VectorSubtract( m_ragPos[0], GetAbsOrigin(), vecDelta );
		boxmins += vecDelta;
		boxmaxs += vecDelta;

		if ( engine->CullBox( boxmins, boxmaxs ) )
			return VIS_NOT_VISIBLE;

		return VIS_IS_VISIBLE;
	}

	void GetRenderBounds( Vector& theMins, Vector& theMaxs )
	{
		theMins = EntitySpaceMins();
		theMaxs = EntitySpaceMaxs();
	}

	virtual void BuildTransformations( Vector *pos, Quaternion q[], const matrix3x4_t &cameraTransform )
	{
		studiohdr_t *hdr = GetModelPtr();
		if ( !hdr )
		{
			return;
		}
		matrix3x4_t bonematrix;
		bool boneSimulated[MAXSTUDIOBONES];

		// no bones have been simulated
		memset( boneSimulated, 0, sizeof(boneSimulated) );
		mstudiobone_t *pbones = hdr->pBone( 0 );
		int i;

		for ( i = 0; i < m_elementCount; i++ )
		{
			int index = m_boneIndex[i];
			if ( index >= 0 )
			{
				boneSimulated[index] = true;
				matrix3x4_t &matrix = m_CachedBones[ index ];

				AngleMatrix( m_ragAngles[i], m_ragPos[i], matrix );
			}
		}
		for ( i = 0; i < hdr->numbones; i++ ) 
		{
			// BUGBUG: Merge this code with the code in c_baseanimating somehow!!!
			// animate all non-simulated bones
			if ( boneSimulated[i] )
			{
				continue;
			}
			else if (CalcProceduralBone( hdr, i, m_CachedBones.Base() ))
			{
				continue;
			}
			else
			{
				QuaternionMatrix( q[i], bonematrix );

				bonematrix[0][3] = pos[i][0];
				bonematrix[1][3] = pos[i][1];
				bonematrix[2][3] = pos[i][2];

				if (pbones[i].parent == -1) 
				{
					ConcatTransforms( cameraTransform, bonematrix, m_CachedBones[i] );

					// Apply client-side effects to the transformation matrix
					// CL_FxTransform( this, pBoneToWorld[i] );
				} 
				else 
				{
					ConcatTransforms( m_CachedBones[pbones[i].parent], bonematrix, m_CachedBones[i] );
				}
			}
		}
	}

	IPhysicsObject *GetElement( int elementNum ) 
	{ 
		return NULL;
	}

private:
	C_ServerRagdoll( const C_ServerRagdoll & );
};

EXTERN_RECV_TABLE(DT_Ragdoll);
IMPLEMENT_CLIENTCLASS_DT(C_ServerRagdoll, DT_Ragdoll, CRagdollProp)
	RecvPropArray(RecvPropQAngles(RECVINFO(m_ragAngles[0])), m_ragAngles),
	RecvPropArray(RecvPropVector(RECVINFO(m_ragPos[0])), m_ragPos),
END_RECV_TABLE()

#define ATTACH_INTERP_TIME	0.2
class C_ServerRagdollAttached : public C_ServerRagdoll
{
	DECLARE_CLASS( C_ServerRagdollAttached, C_ServerRagdoll );
public:
	C_ServerRagdollAttached( void ) 
	{
		m_bFollowing = false;
		m_vecOffset.Init();
	}
	DECLARE_CLIENTCLASS();

	virtual void BuildTransformations( Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform )
	{
		VPROF_BUDGET( "C_ServerRagdollAttached::SetupBones", VPROF_BUDGETGROUP_OTHER_ANIMATION );

		studiohdr_t *hdr = GetModelPtr();
		if ( !hdr )
			return;

		float frac = RemapVal( gpGlobals->curtime, m_followTime, m_followTime+ATTACH_INTERP_TIME, 0, 1 );
		frac = clamp( frac, 0, 1 );
		// interpolate offset over some time
		Vector offset = m_vecOffset * (1-frac);

		C_BaseAnimating *follow = NULL;
		Vector worldOrigin;
		worldOrigin.Init();

		if ( IsFollowingEntity() )
		{
			follow = FindFollowedEntity();
			Assert( follow != this );
			if ( follow )
			{
				follow->SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime );

				matrix3x4_t boneToWorld;
				follow->GetCachedBoneMatrix( m_boneIndexAttached, boneToWorld );
				VectorTransform( m_attachmentPointBoneSpace, boneToWorld, worldOrigin );
			}
				
		}
		BaseClass::BuildTransformations( pos, q, cameraTransform );

		if ( follow )
		{
			int index = m_boneIndex[m_ragdollAttachedObjectIndex];
			matrix3x4_t &matrix = m_CachedBones[ index ];
			Vector ragOrigin;
			VectorTransform( m_attachmentPointRagdollSpace, matrix, ragOrigin );
			offset = worldOrigin - ragOrigin;
			// fixes culling
			SetAbsOrigin( worldOrigin );
			m_vecOffset = offset;
		}

		for ( int i = 0; i < hdr->numbones; i++ )
		{
			Vector pos;
			matrix3x4_t &matrix = m_CachedBones[ i ];
			MatrixGetColumn( matrix, 3, pos );
			pos += offset;
			MatrixSetColumn( pos, 3, matrix );
		}
	}
	void OnDataChanged( DataUpdateType_t updateType );

	Vector		m_attachmentPointBoneSpace;
	Vector		m_vecOffset;
	Vector		m_attachmentPointRagdollSpace;
	int			m_ragdollAttachedObjectIndex;
	int			m_boneIndexAttached;
	float		m_followTime;
	bool		m_bFollowing;
private:
	C_ServerRagdollAttached( const C_ServerRagdollAttached & );
};

EXTERN_RECV_TABLE(DT_Ragdoll_Attached);
IMPLEMENT_CLIENTCLASS_DT(C_ServerRagdollAttached, DT_Ragdoll_Attached, CRagdollPropAttached)
	RecvPropInt( RECVINFO( m_boneIndexAttached ) ),
	RecvPropInt( RECVINFO( m_ragdollAttachedObjectIndex ) ),
	RecvPropVector(RECVINFO(m_attachmentPointBoneSpace) ),
	RecvPropVector(RECVINFO(m_attachmentPointRagdollSpace) ),
END_RECV_TABLE()

void C_ServerRagdollAttached::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bFollowing != IsFollowingEntity() )
	{
		if ( m_bFollowing )
		{
			m_followTime = gpGlobals->curtime;
		}
		m_bFollowing = IsFollowingEntity();
	}
}
