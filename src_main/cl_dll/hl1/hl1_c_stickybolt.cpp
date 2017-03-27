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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_basetempentity.h"
#include "fx.h"
#include "decals.h"
#include "iefx.h"
#include "engine/IEngineSound.h"
#include "materialsystem/IMaterialVar.h"
#include "c_splash.h"
#include "ieffects.h"
#include "engine/IEngineTrace.h"
#include "vphysics/constraints.h"
#include "engine/ivmodelinfo.h"
#include "tempent.h"
#include "c_te_legacytempents.h"

extern IPhysicsSurfaceProps *physprops;
IPhysicsObject *GetWorldPhysObject( void );

extern ITempEnts* tempents;

void GetBreakParams( constraint_breakableparams_t &params )
{
	params.Defaults();
	params.forceLimit = lbs2kg(0);
	params.torqueLimit = lbs2kg(0);
}

class CRagdollBoltEnumerator : public IPartitionEnumerator
{
public:
	//Forced constructor   
	CRagdollBoltEnumerator( Ray_t& shot, float force, Vector vOrigin )
	{
		m_rayShot = shot;
		m_flForce = force;
		m_vWorld = vOrigin;
	}

	//Actual work code
	IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
		if ( pEnt == NULL )
			return ITERATION_CONTINUE;

		C_BaseAnimating *pModel = static_cast< C_BaseAnimating * >( pEnt );

		if ( pModel == NULL )
			return ITERATION_CONTINUE;

		trace_t tr;
		enginetrace->ClipRayToEntity( m_rayShot, MASK_SHOT, pModel, &tr );

		IPhysicsObject	*pPhysicsObject = pModel->VPhysicsGetObject();
		
		if ( pPhysicsObject == NULL )
			return ITERATION_CONTINUE;

		if ( tr.fraction < 1.0 )
		{
			IPhysicsObject *pReference = GetWorldPhysObject();

			if ( !pReference || !pPhysicsObject )
				 return ITERATION_CONTINUE;
			
			constraint_ballsocketparams_t ballsocket;
			ballsocket.Defaults();
			
			Vector Origin;
			
			pPhysicsObject->GetPosition( &Origin, NULL );

			if ( ( Origin- m_vWorld).Length () < 64 )
			{
				pReference->WorldToLocal( ballsocket.constraintPosition[0], m_vWorld );
				pPhysicsObject->WorldToLocal( ballsocket.constraintPosition[1], Origin );
				
				GetBreakParams( ballsocket.constraint );

				ballsocket.constraint.torqueLimit = 0;

				m_pConstraint = physenv->CreateBallsocketConstraint( pReference, pPhysicsObject, NULL, ballsocket );

				pPhysicsObject->ApplyForceCenter( Vector(0, 0, 1 ) * 100);
												
				return ITERATION_STOP;
			}
			else
				return ITERATION_CONTINUE;
		}

		return ITERATION_CONTINUE;
	}

	Ray_t	m_rayShot;
	float	m_flForce;
	Vector  m_vWorld;
	IPhysicsConstraint	*m_pConstraint;
};

//-----------------------------------------------------------------------------
// Purpose: Kills Player Attachments
//-----------------------------------------------------------------------------
class C_TEStickyBolt : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEStickyBolt, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEStickyBolt( void );
	virtual			~C_TEStickyBolt( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

	void			FindRagdollInRay( void ); 

public:
	int				m_nEntIndex;
	Vector			m_vecOrigin;
	Vector			m_vecDirection;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEStickyBolt::C_TEStickyBolt( void )
{
	m_nEntIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEStickyBolt::~C_TEStickyBolt( void )
{
}

void C_TEStickyBolt::FindRagdollInRay( void )
{
	Ray_t	shotRay;

	Vector vecEnd = m_vecOrigin + m_vecDirection * 128;

	shotRay.Init( m_vecOrigin, vecEnd );

	CRagdollBoltEnumerator	ragdollEnum( shotRay, 100000, m_vecOrigin );
	partition->EnumerateElementsAlongRay( PARTITION_CLIENT_RESPONSIVE_EDICTS, shotRay, false, &ragdollEnum );

	model_t *pModel = (model_t *)engine->LoadModel( "models/crossbow_bolt.mdl" );

	QAngle vAngles;

	VectorAngles( -m_vecDirection, vAngles );
	tempents->SpawnTempModel( pModel, m_vecOrigin + m_vecDirection * 16, vAngles, Vector(0, 0, 0 ), 1, FTENT_NEVERDIE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEStickyBolt::PostDataUpdate( DataUpdateType_t updateType )
{
	FindRagdollInRay();
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEStickyBolt, DT_TEStickyBolt, CTEStickyBolt)
	RecvPropInt( RECVINFO(m_nEntIndex)),
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropVector( RECVINFO(m_vecDirection)),
END_RECV_TABLE()
