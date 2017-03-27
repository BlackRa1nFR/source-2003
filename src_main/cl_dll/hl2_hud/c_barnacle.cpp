//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "c_ai_basenpc.h"
#include "engine/ivmodelinfo.h"
#include "rope_physics.h"
#include "materialsystem/IMaterialSystem.h"
#include "FX_Line.h"
#include "engine/IVDebugOverlay.h"
#include "bone_setup.h"
#include "model_types.h"

#define BARNACLE_TONGUE_POINTS		7

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_NPC_Barnacle : public C_AI_BaseNPC
{
public:

	DECLARE_CLASS( C_NPC_Barnacle, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

	C_NPC_Barnacle( void );

	virtual int	 InternalDrawModel( int flags );

	virtual void GetRenderBounds( Vector &theMins, Vector &theMaxs )
	{
   		modelinfo->GetModelRenderBounds( GetModel(), 0, theMins, theMaxs );

		// Extend our bounding box downwards the length of the tongue
   		theMins -= Vector( 0, 0, m_flAltitude );
	}

	virtual void SetObjectCollisionBox( void )
	{
		SetAbsMins( GetAbsOrigin() + WorldAlignMins() );
		SetAbsMaxs( GetAbsOrigin() + WorldAlignMaxs() );

		// Extend our bounding box downwards the length of the tongue
		Vector vecAbsMins = GetAbsMins();
		vecAbsMins.z -= m_flAltitude;
		SetAbsMins( vecAbsMins );
	}

	void	OnDataChanged( DataUpdateType_t updateType );
	void	InitTonguePhysics( void );
	void	ClientThink( void );
	void	StandardBlendingRules( Vector pos[], Quaternion q[], float currentTime, int boneMask );

protected:
	float	m_flAltitude;
	Vector	m_vecRoot;
	Vector	m_vecTip;

private:
	// Tongue points
	Vector	m_vecTonguePoints[BARNACLE_TONGUE_POINTS];
	CRopePhysics<BARNACLE_TONGUE_POINTS>	m_TonguePhysics;

	// Tongue physics delegate
	class CBarnaclePhysicsDelegate : public CSimplePhysics::IHelper
	{
	public:
		virtual void	GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel );
		virtual void	ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes );
	
		C_NPC_Barnacle	*m_pBarnacle;
	};
	friend class CBarnaclePhysicsDelegate;
	CBarnaclePhysicsDelegate	m_PhysicsDelegate;

private:
	C_NPC_Barnacle( const C_NPC_Barnacle & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT( C_NPC_Barnacle, DT_Barnacle, CNPC_Barnacle )
	RecvPropFloat( RECVINFO( m_flAltitude ) ),
	RecvPropVector( RECVINFO( m_vecRoot ) ),
	RecvPropVector( RECVINFO( m_vecTip ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_NPC_Barnacle::C_NPC_Barnacle( void )
{
	m_PhysicsDelegate.m_pBarnacle = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		InitTonguePhysics();

		// We want to think every frame.
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::InitTonguePhysics( void )
{
	// Init tongue spline
	// First point is at the top
	m_TonguePhysics.SetupSimulation( m_flAltitude / (BARNACLE_TONGUE_POINTS-1), &m_PhysicsDelegate );
	m_TonguePhysics.Restart();

	// Initialize the positions of the nodes.
	m_TonguePhysics.GetFirstNode()->m_vPos = m_vecRoot;
	m_TonguePhysics.GetFirstNode()->m_vPrevPos = m_TonguePhysics.GetFirstNode()->m_vPos;
	float flAltitude = m_flAltitude;
	for( int i = 1; i < m_TonguePhysics.NumNodes(); i++ )
	{
		flAltitude *= 0.5;
		CSimplePhysics::CNode *pNode = m_TonguePhysics.GetNode( i );
		pNode->m_vPos = m_TonguePhysics.GetNode(i-1)->m_vPos - Vector(0,0,flAltitude);
		pNode->m_vPrevPos = pNode->m_vPos;

		// Set the length of the node's spring
		//m_TonguePhysics.ResetNodeSpringLength( i-1, flAltitude );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::ClientThink( void )
{
	m_TonguePhysics.Simulate( gpGlobals->frametime );

	// Set the spring's length to that of the tongue's extension
	m_TonguePhysics.ResetSpringLength( m_flAltitude / (BARNACLE_TONGUE_POINTS-1) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::StandardBlendingRules( Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( pos, q, currentTime, boneMask );

	// Don't do anything while dead
	if ( !IsAlive() )
		return;

	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
		return;

	int firstBone = Studio_BoneIndexByName( hdr, "Barnacle.tongue1" );

	Vector vecPrev = pos[firstBone-1];
	Vector vecCurr = vec3_origin;
	for ( int i = 0; i <= BARNACLE_TONGUE_POINTS; i++ )
	{
		// We double up the bones at the last node.
		Vector vecCurr;
		if ( i == BARNACLE_TONGUE_POINTS )
		{
			vecCurr = m_TonguePhysics.GetLastNode()->m_vPos;
		}
		else
		{
			vecCurr = m_TonguePhysics.GetNode(i)->m_vPos;
		}

		//debugoverlay->AddBoxOverlay( vecCurr, -Vector(2,2,2), Vector(2,2,2), vec3_angle, 0,255,0, 128, 0.1 );

		// Fill out the positions in local space
		VectorITransform( vecCurr, EntityToWorldTransform(), pos[firstBone+i] );
		vecCurr = pos[firstBone+i];

		// Fill out the angles
		Vector vecForward = (vecCurr - vecPrev);
		VectorNormalize( vecForward );
		QAngle vecAngle;
		VectorAngles( vecForward, vecAngle );
		AngleQuaternion( vecAngle, q[firstBone+i] );

		vecPrev = vecCurr;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws the object
//-----------------------------------------------------------------------------
int C_NPC_Barnacle::InternalDrawModel( int flags )
{
	if ( !GetModel() )
		return 0;

	// Make sure hdr is valid for drawing
	if ( !GetModelPtr() )
		return 0;

	// UNDONE: With a bit of work on the model->world transform, we can probably
	// move the frustum culling into the client DLL entirely.  Then TestVisibility()
	// can just return true/false and only be called when frustumcull is set.
	if ( flags & STUDIO_FRUSTUMCULL )
	{
		switch ( TestVisibility() )
		{
		// not visible, don't draw
		case VIS_NOT_VISIBLE:
			return 0;
		
		// definitely visible, disable engine check
		case VIS_IS_VISIBLE:
			flags &= ~STUDIO_FRUSTUMCULL;
			break;
		
		default:
		case VIS_USE_ENGINE:
			break;
		}
	}

	Vector vecMins, vecMaxs;
	GetRenderBounds( vecMins, vecMaxs );
	int drawn = modelrender->DrawModel( 
		flags, 
		this,
		GetModelInstance(),
		index, 
		GetModel(),
		GetAbsOrigin(),
		GetAbsAngles(),
		GetSequence(),
		m_nSkin,
		m_nBody,
		m_nHitboxSet,
		&GetAbsMins(),
		&GetAbsMaxs() );

	if ( vcollide_wireframe.GetBool() )
	{
		if ( IsRagdoll() )
		{
			m_pRagdoll->DrawWireframe();
		}
		else
		{
			vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
			if ( pCollide && pCollide->solidCount == 1 )
			{
				static color32 debugColor = {0,255,255,0};
				matrix3x4_t matrix;
				AngleMatrix( GetAbsAngles(), GetAbsOrigin(), matrix );
				engine->DebugDrawPhysCollide( pCollide->solids[0], NULL, matrix, debugColor );
			}
		}
	}

	return drawn;
}

//===============================================================================================================================
// BARNACLE TONGUE PHYSICS
//===============================================================================================================================
#define TONGUE_GRAVITY			0, 0, -1000
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::CBarnaclePhysicsDelegate::GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel )
{
	// Gravity.
	pAccel->Init( TONGUE_GRAVITY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::CBarnaclePhysicsDelegate::ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes )
{
	// Startpoint always stays at the root
	pNodes[0].m_vPos = m_pBarnacle->m_vecRoot;

	// Endpoint always stays at the tip
	pNodes[nNodes-1].m_vPos = m_pBarnacle->m_vecTip;
}
