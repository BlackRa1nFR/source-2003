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

#include "engine/IEngineTrace.h"
#include "icliententitylist.h"
#include "ispatialpartitioninternal.h"
#include "icliententity.h"
#include "cmodel_engine.h"
#include "staticpropmgr.h"
#include "server.h"
#include "edict.h"
#include "gl_model_private.h"
#include "world.h"
#include "vphysics_interface.h"
#include "client_class.h"
#include "enginestats.h"
#include "server_class.h"


//-----------------------------------------------------------------------------
// Implementation of IEngineTrace
//-----------------------------------------------------------------------------
class CEngineTrace : public IEngineTrace
{
public:
	// Returns the contents mask at a particular world-space position
	virtual int		GetPointContents( const Vector &vecAbsPosition, IHandleEntity** ppEntity );

	// Traces a ray against a particular edict
	virtual void	ClipRayToEntity( const Ray_t &ray, unsigned int fMask, IHandleEntity *pEntity, trace_t *pTrace );

	// A version that simply accepts a ray (can work as a traceline or tracehull)
	virtual void	TraceRay( const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace );

	// Enumerates over all entities along a ray
	// If triggers == true, it enumerates all triggers along a ray
	virtual void	EnumerateEntities( const Ray_t &ray, bool triggers, IEntityEnumerator *pEnumerator );

	// Same thing, but enumerate entitys within a box
	virtual void	EnumerateEntities( const Vector &vecAbsMins, const Vector &vecAbsMaxs, IEntityEnumerator *pEnumerator );

	// FIXME: Different versions for client + server. Eventually we need to make these go away
	virtual void HandleEntityToCollideable( IHandleEntity *pHandleEntity, ICollideable **ppCollide, const char **ppDebugName ) = 0;
	virtual ICollideable *GetWorldCollideable() = 0;

	// Traces a ray against a particular edict
	void ClipRayToCollideable( const Ray_t &ray, unsigned int fMask, ICollideable *pEntity, trace_t *pTrace );

private:
	// FIXME: Different versions for client + server. Eventually we need to make these go away
	virtual void SetTraceEntity( ICollideable *pCollideable, trace_t *pTrace ) = 0;
	virtual ICollideable *GetCollideable( IHandleEntity *pEntity ) = 0;
	virtual int SpatialPartitionMask() const = 0;
	virtual int SpatialPartitionTriggerMask() const = 0;

	// Figure out point contents for entities at a particular position
	int EntityContents( const Vector &vecAbsPosition );

	// Should we perform the custom raytest?
	bool ShouldPerformCustomRayTest( const Ray_t& ray, ICollideable *pCollideable ) const;

	// Performs the custom raycast
	bool ClipRayToCustom( const Ray_t& ray, unsigned int fMask, ICollideable *pCollideable, trace_t* pTrace );

	// Perform vphysics trace
	bool ClipRayToVPhysics( const Ray_t &ray, unsigned int fMask, ICollideable *pCollideable, trace_t *pTrace );

	// Perform hitbox trace
	bool ClipRayToHitboxes( const Ray_t& ray, unsigned int fMask, ICollideable *pCollideable, trace_t* pTrace );

	// Perform bsp trace
	bool ClipRayToBSP( const Ray_t &ray, unsigned int fMask, ICollideable *pCollideable, trace_t *pTrace );

	// bbox
	bool ClipRayToBBox( const Ray_t &ray, unsigned int fMask, ICollideable *pCollideable, trace_t *pTrace );

	// Clips a trace to another trace
	bool ClipTraceToTrace( trace_t &clipTrace, trace_t *pFinalTrace );

};

class CEngineTraceServer : public CEngineTrace
{
private:
	virtual void HandleEntityToCollideable( IHandleEntity *pEnt, ICollideable **ppCollide, const char **ppDebugName );
	virtual void SetTraceEntity( ICollideable *pCollideable, trace_t *pTrace );
	virtual ICollideable *GetCollideable( IHandleEntity *pEntity );
	virtual int SpatialPartitionMask() const;
	virtual int SpatialPartitionTriggerMask() const;
	virtual ICollideable *GetWorldCollideable();
};

#ifndef SWDS
class CEngineTraceClient : public CEngineTrace
{
private:
	virtual void HandleEntityToCollideable( IHandleEntity *pEnt, ICollideable **ppCollide, const char **ppDebugName );
	virtual void SetTraceEntity( ICollideable *pCollideable, trace_t *pTrace );
	virtual ICollideable *GetCollideable( IHandleEntity *pEntity );
	virtual int SpatialPartitionMask() const;
	virtual int SpatialPartitionTriggerMask() const;
	virtual ICollideable *GetWorldCollideable();
};
#endif

//-----------------------------------------------------------------------------
// Expose CVEngineServer to the game + client DLLs
//-----------------------------------------------------------------------------
static CEngineTraceServer	s_EngineTraceServer;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEngineTraceServer, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER, s_EngineTraceServer);

#ifndef SWDS
static CEngineTraceClient	s_EngineTraceClient;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEngineTraceClient, IEngineTrace, INTERFACEVERSION_ENGINETRACE_CLIENT, s_EngineTraceClient);
#endif

//-----------------------------------------------------------------------------
// Expose CVEngineServer to the engine.
//-----------------------------------------------------------------------------
IEngineTrace *g_pEngineTraceServer = &s_EngineTraceServer;
#ifndef SWDS
IEngineTrace *g_pEngineTraceClient = &s_EngineTraceClient;
#endif

//-----------------------------------------------------------------------------
// Client-server neutral method of getting at collideables
//-----------------------------------------------------------------------------
#ifndef SWDS
ICollideable *CEngineTraceClient::GetCollideable( IHandleEntity *pEntity )
{
	Assert( pEntity );

	ICollideable *pProp = StaticPropMgr()->GetStaticProp( pEntity );
	if ( pProp )
		return pProp;

	IClientUnknown *pUnk = entitylist->GetClientUnknownFromHandle( pEntity->GetRefEHandle() );
	return pUnk->GetClientCollideable(); 
}
#endif

ICollideable *CEngineTraceServer::GetCollideable( IHandleEntity *pEntity )
{
	Assert( pEntity );

	ICollideable *pProp = StaticPropMgr()->GetStaticProp( pEntity );
	if ( pProp )
		return pProp;

	IServerNetworkable *pNetEntity = (IServerNetworkable*)pEntity;
	return pNetEntity->GetEdict()->GetCollideable();
}


//-----------------------------------------------------------------------------
// Spatial partition masks for iteration
//-----------------------------------------------------------------------------
#ifndef SWDS
int CEngineTraceClient::SpatialPartitionMask() const
{
	return PARTITION_CLIENT_SOLID_EDICTS;
}
#endif

int CEngineTraceServer::SpatialPartitionMask() const
{
	return PARTITION_ENGINE_SOLID_EDICTS;
}

#ifndef SWDS
int CEngineTraceClient::SpatialPartitionTriggerMask() const
{
	return 0;
}
#endif

int CEngineTraceServer::SpatialPartitionTriggerMask() const
{
	return PARTITION_ENGINE_TRIGGER_EDICTS;
}


//-----------------------------------------------------------------------------
// Spatial partition enumerator looking for entities that we may lie within
//-----------------------------------------------------------------------------
class CPointContentsEnum : public IPartitionEnumerator
{
public:
	CPointContentsEnum( CEngineTrace *pEngineTrace, const Vector &pos ) : m_Contents(CONTENTS_EMPTY) 
	{
		m_pEngineTrace = pEngineTrace;
		m_Pos = pos; 
		m_pCollide = NULL;
	}

	IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		ICollideable *pCollide;
		const char *pDbgName;
		m_pEngineTrace->HandleEntityToCollideable( pHandleEntity, &pCollide, &pDbgName );
		if (!pCollide)
			return ITERATION_CONTINUE;

		// Deal with static props
		// NOTE: I could have added static props to a different list and
		// enumerated them separately, but that would have been less efficient
		if ( StaticPropMgr()->IsStaticProp( pHandleEntity ) )
		{
			Ray_t ray;
			trace_t trace;
			ray.Init( m_Pos, m_Pos );
			m_pEngineTrace->ClipRayToCollideable( ray, MASK_ALL, pCollide, &trace );
			if (trace.startsolid)
			{
				// We're in a static prop; that's solid baby
				// Pretend we hit the world
				m_Contents = CONTENTS_SOLID;
				m_pCollide = m_pEngineTrace->GetWorldCollideable();
				return ITERATION_STOP;
			}
			return ITERATION_CONTINUE;
		}
		
		// We only care about solid volumes
		if ((pCollide->GetSolidFlags() & FSOLID_VOLUME_CONTENTS) == 0)
			return ITERATION_CONTINUE;

		model_t* pModel = (model_t*)pCollide->GetCollisionModel();
		if ( pModel && pModel->type == mod_brush )
		{
			Assert( pCollide->GetCollisionModelIndex() < MAX_MODELS && pCollide->GetCollisionModelIndex() >= 0 );
			int nHeadNode = GetModelHeadNode( pCollide );
			int contents = CM_TransformedPointContents( m_Pos, nHeadNode, 
				pCollide->GetCollisionOrigin(), pCollide->GetCollisionAngles() );

			if (contents != CONTENTS_EMPTY)
			{
				// Return the contents of the first thing we hit
				m_Contents = contents;
				m_pCollide = pCollide;
				return ITERATION_STOP;
			}
		}

		return ITERATION_CONTINUE;
	}

private:
	int GetModelHeadNode( ICollideable *pCollide )
	{
		int modelindex = pCollide->GetCollisionModelIndex();
		if(modelindex >= MAX_MODELS || modelindex < 0)
			return -1;

		model_t *pModel = (model_t*)pCollide->GetCollisionModel();
		if(!pModel)
			return -1;

		if(cmodel_t *pCModel = CM_InlineModelNumber(modelindex-1))
			return pCModel->headnode;
		else
			return -1;
	}

public:
	int m_Contents;
	ICollideable *m_pCollide;

private:
	CEngineTrace *m_pEngineTrace;
	Vector m_Pos;
};


//-----------------------------------------------------------------------------
// Returns the contents mask at a particular world-space position
//-----------------------------------------------------------------------------
int	CEngineTrace::GetPointContents( const Vector &vecAbsPosition, IHandleEntity** ppEntity )
{
	// First check the collision model
	int nContents = CM_PointContents( vecAbsPosition, 0 );
	if ( nContents & MASK_CURRENT )
	{
		nContents = CONTENTS_WATER;
	}
	
	if ( nContents != CONTENTS_SOLID )
	{
		CPointContentsEnum contentsEnum(this, vecAbsPosition);
		SpatialPartition()->EnumerateElementsAtPoint( SpatialPartitionMask(),
			vecAbsPosition, false, &contentsEnum );

		int nEntityContents = contentsEnum.m_Contents;
		if ( nEntityContents & MASK_CURRENT )
			nContents = CONTENTS_WATER;
		if ( nEntityContents != CONTENTS_EMPTY )
		{
			if (ppEntity)
			{
				*ppEntity = contentsEnum.m_pCollide->GetEntityHandle();
			}

			return nEntityContents;
		}
	}

	if (ppEntity)
	{
		*ppEntity = GetWorldCollideable()->GetEntityHandle();
	}

	return nContents;
}


//-----------------------------------------------------------------------------
// Should we perform the custom raytest?
//-----------------------------------------------------------------------------
inline bool CEngineTrace::ShouldPerformCustomRayTest( const Ray_t& ray, ICollideable *pCollideable ) const
{
	// No model? The entity's got its own collision detector maybe
	// Does the entity force box or ray tests to go through its code?
	return( (pCollideable->GetSolid() == SOLID_CUSTOM) ||
			(ray.m_IsRay && (pCollideable->GetSolidFlags() & FSOLID_CUSTOMRAYTEST )) || 
			(!ray.m_IsRay && (pCollideable->GetSolidFlags() & FSOLID_CUSTOMBOXTEST )) );
}


//-----------------------------------------------------------------------------
// Performs the custom raycast
//-----------------------------------------------------------------------------
bool CEngineTrace::ClipRayToCustom( const Ray_t& ray, unsigned int fMask, ICollideable *pCollideable, trace_t* pTrace )
{
	if ( pCollideable->TestCollision( ray, fMask, *pTrace ))
	{
		// FIXME: This seems totally wrong.... should this be removed?
		// Fix up the trace start and end by our offset...
		
		// MD: these VectorAdds seem to produce totally wrong results and I hit asserts later in CEngineTrace::TraceRay.
		//VectorAdd( pTrace->startpos, ray.m_StartOffset, pTrace->startpos );
		//VectorAdd( pTrace->endpos, ray.m_StartOffset, pTrace->endpos );
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Performs the hitbox raycast, returns true if the hitbox test was made
//-----------------------------------------------------------------------------
bool CEngineTrace::ClipRayToHitboxes( const Ray_t& ray, unsigned int fMask, ICollideable *pCollideable, trace_t* pTrace )
{
	trace_t hitboxTrace;
	CM_ClearTrace( &hitboxTrace );

	// Keep track of the contents of what was hit initially
	hitboxTrace.contents = pTrace->contents;
	VectorAdd( ray.m_Start, ray.m_StartOffset, hitboxTrace.startpos );
	VectorAdd( hitboxTrace.startpos, ray.m_Delta, hitboxTrace.endpos );

	// At the moment, it has to be a true ray to work with hitboxes
	if ( !ray.m_IsRay )
		return false;

	// If the hitboxes weren't even tested, then just use the original trace
	if (!pCollideable->TestHitboxes( ray, fMask, hitboxTrace ))
		return false;

	// If they *were* tested and missed, clear the original trace
	if (!hitboxTrace.DidHit())
	{
		CM_ClearTrace( pTrace );
		pTrace->startpos = hitboxTrace.startpos;
		pTrace->endpos = hitboxTrace.endpos;
	}
	else if ( pCollideable->GetSolid() != SOLID_VPHYSICS )
	{
		// If we also hit the hitboxes, maintain fractionleftsolid +
		// startpos because those are reasonable enough values and the
		// hitbox code doesn't set those itself.
		Vector vecStartPos = pTrace->startpos;
		float flFractionLeftSolid = pTrace->fractionleftsolid;

		*pTrace = hitboxTrace;

		if (hitboxTrace.startsolid)
		{
			pTrace->startpos = vecStartPos;
			pTrace->fractionleftsolid = flFractionLeftSolid;
		}
	}
	else
	{
		// Fill out the trace hitbox details
		pTrace->contents = hitboxTrace.contents;
		pTrace->hitgroup = hitboxTrace.hitgroup;
		pTrace->hitbox = hitboxTrace.hitbox;
		pTrace->physicsbone = hitboxTrace.physicsbone;
		Assert( pTrace->physicsbone >= 0 );

		// Fill out the surfaceprop details from the hitbox. Use the physics bone instead of the hitbox bone
		const model_t *pModel = pCollideable->GetCollisionModel();
		studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( (model_t*)pModel );
		mstudiobone_t *pBone = pStudioHdr->pBone( pTrace->physicsbone );
		pTrace->surface.name = "**studio**";
		pTrace->surface.flags = SURF_HITBOX;
		pTrace->surface.surfaceProps = physprop->GetSurfaceIndex( pBone->pszSurfaceProp() );
	}

	return true;
}


class CBrushConvexInfo : public IConvexInfo
{
public:
	CBrushConvexInfo()
	{
		m_pBSPData = GetCollisionBSPData();
	}
	virtual unsigned int GetContents( int convexGameData )
	{
		return m_pBSPData->map_brushes[convexGameData].contents;
	}

private:
	CCollisionBSPData *m_pBSPData;
};

//-----------------------------------------------------------------------------
// Perform vphysics trace
//-----------------------------------------------------------------------------
bool CEngineTrace::ClipRayToVPhysics( const Ray_t &ray, unsigned int fMask, ICollideable *pEntity, trace_t *pTrace )
{
	bool bTraced = false;

	// use the vphysics model for rotated brushes and vphysics simulated objects
	if ( pEntity->GetSolid() == SOLID_VPHYSICS )
	{
		const model_t *pModel = pEntity->GetCollisionModel();
		// use the regular code for raytraces against brushes
		// do ray traces with normal code, but use vphysics to do box traces
		if ( !ray.m_IsRay || pModel->type != mod_brush )
		{
			int nModelIndex = pEntity->GetCollisionModelIndex();

			// BUGBUG: This only works when the vcollide in question is the first solid in the model
			vcollide_t *pCollide = CM_VCollideForModel( nModelIndex, (model_t*)pModel );

			if ( pCollide && pCollide->solidCount )
			{
				CBrushConvexInfo brushConvex;
				IConvexInfo *pConvexInfo = ( pModel->type == mod_brush ) ? &brushConvex : NULL;

				physcollision->TraceBox( 
					ray,
					fMask,
					pConvexInfo,
					pCollide->solids[0], // UNDONE: Support other solid indices?!?!?!? (forced zero)
					pEntity->GetCollisionOrigin(), 
					pEntity->GetCollisionAngles(), 
					pTrace );
				bTraced = true;
			}
		}
	}

	return bTraced;
}


//-----------------------------------------------------------------------------
// Perform bsp trace
//-----------------------------------------------------------------------------
bool CEngineTrace::ClipRayToBSP( const Ray_t &ray, unsigned int fMask, ICollideable *pEntity, trace_t *pTrace )
{
	const model_t *pModel = pEntity->GetCollisionModel();
	if ( pModel && pModel->type == mod_brush )
	{
		int nModelIndex = pEntity->GetCollisionModelIndex();
		cmodel_t *pCModel = CM_InlineModelNumber( nModelIndex - 1 );
		int nHeadNode = pCModel->headnode;

		CM_TransformedBoxTrace( ray, nHeadNode, fMask, pEntity->GetCollisionOrigin(), pEntity->GetCollisionAngles(), *pTrace );
		return true;
	}
	return false;
}


bool CEngineTrace::ClipRayToBBox( const Ray_t &ray, unsigned int fMask, ICollideable *pEntity, trace_t *pTrace )
{
	if ( pEntity->GetSolid() != SOLID_BBOX )
		return false;

	int nHeadNode = CM_HeadnodeForBoxHull( pEntity->WorldAlignMins(), pEntity->WorldAlignMaxs() );

	// bboxes don't rotate
	CM_TransformedBoxTrace( ray, nHeadNode, fMask, pEntity->GetCollisionOrigin(), vec3_angle, *pTrace );
	return true;
}


//-----------------------------------------------------------------------------
// Main entry point for clipping rays to entities
//-----------------------------------------------------------------------------
#ifndef SWDS
void CEngineTraceClient::SetTraceEntity( ICollideable *pCollideable, trace_t *pTrace )
{
	if ( !pTrace->DidHit() )
		return;

	// FIXME: This is only necessary because of traces occurring during
	// LevelInit (a suspect time to be tracing)
	if (!pCollideable)
	{
		pTrace->m_pEnt = NULL;
		return;
	}

	IClientUnknown *pUnk = (IClientUnknown*)pCollideable->GetEntityHandle();
	if ( !StaticPropMgr()->IsStaticProp( pUnk ) )
	{
		pTrace->m_pEnt = (CBaseEntity*)(pUnk->GetIClientEntity());
	}
	else
	{
		// For static props, point to the world, hitbox is the prop index
		pTrace->m_pEnt = (CBaseEntity*)(entitylist->GetClientEntity(0));
		pTrace->hitbox = StaticPropMgr()->GetStaticPropIndex( pUnk ) + 1;
	}
}
#endif

void CEngineTraceServer::SetTraceEntity( ICollideable *pCollideable, trace_t *pTrace )
{
	if ( !pTrace->DidHit() )
		return;

	IHandleEntity *pHandleEntity = pCollideable->GetEntityHandle();
	if ( !StaticPropMgr()->IsStaticProp( pHandleEntity ) )
	{
		pTrace->m_pEnt = (CBaseEntity*)(pHandleEntity);
	}
	else
	{
		// For static props, point to the world, hitbox is the prop index
		pTrace->m_pEnt = (CBaseEntity*)(sv.edicts->GetIServerEntity());
		pTrace->hitbox = StaticPropMgr()->GetStaticPropIndex( pHandleEntity ) + 1;
	}
}


//-----------------------------------------------------------------------------
// Traces a ray against a particular edict
//-----------------------------------------------------------------------------
void CEngineTrace::ClipRayToCollideable( const Ray_t &ray, unsigned int fMask, ICollideable *pEntity, trace_t *pTrace )
{
	CM_ClearTrace( pTrace );
	VectorAdd( ray.m_Start, ray.m_StartOffset, pTrace->startpos );
	VectorAdd( pTrace->startpos, ray.m_Delta, pTrace->endpos );

	const model_t *pModel = pEntity->GetCollisionModel();
	bool bIsStudioModel = pModel && pModel->type == mod_studio;

	// Cull if the collision mask isn't set + we're not testing hitboxes.
	if ( bIsStudioModel && (( fMask & CONTENTS_HITBOX ) == 0) )
	{
		studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( (model_t*)pModel );
		if ( ( fMask & pStudioHdr->contents ) == 0)
			return;
	}

	bool bTraced = false;
	bool bCustomPerformed = false;
	if ( ShouldPerformCustomRayTest( ray, pEntity ) )
	{
		ClipRayToCustom( ray, fMask, pEntity, pTrace );
		bTraced = true;
		bCustomPerformed = true;
	}
	else
	{
		bTraced = ClipRayToVPhysics( ray, fMask, pEntity, pTrace );	
	}

	if ( !bTraced )
	{
		bTraced = ClipRayToBSP( ray, fMask, pEntity, pTrace );
	}

	// Hitboxes..
	bool bTracedHitboxes = false;
	if ( bIsStudioModel && (fMask & CONTENTS_HITBOX) )
	{
		// Until hitboxes are no longer implemented as custom raytests,
		// don't bother to do the work twice
		if (!bCustomPerformed)
		{
			bTraced = ClipRayToHitboxes( ray, fMask, pEntity, pTrace );
			if ( bTraced )
			{
				// Hitboxes will set the surface properties
				bTracedHitboxes = true;
			}
		}
	}

	if ( !bTraced )
	{
		ClipRayToBBox( ray, fMask, pEntity, pTrace );
	}

	if ( bIsStudioModel && !bTracedHitboxes && pTrace->DidHit() )
	{
		studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( (model_t*)pModel );
		pTrace->contents = pStudioHdr->contents;
		// use the default surface properties
		pTrace->surface.name = "**studio**";
		pTrace->surface.flags = 0;
		pTrace->surface.surfaceProps = physprop->GetSurfaceIndex( pStudioHdr->pszSurfaceProp() );
	}

	if (pTrace->DidHit())
	{
		SetTraceEntity( pEntity, pTrace );
	}

#ifdef _DEBUG
	Vector vecOffset, vecEndTest;
	VectorAdd( ray.m_Start, ray.m_StartOffset, vecOffset );
	VectorMA( vecOffset, pTrace->fractionleftsolid, ray.m_Delta, vecEndTest );
	Assert( VectorsAreEqual( vecEndTest, pTrace->startpos, 0.1f ) );
	VectorMA( vecOffset, pTrace->fraction, ray.m_Delta, vecEndTest );
	Assert( VectorsAreEqual( vecEndTest, pTrace->endpos, 0.1f ) );
#endif
}


//-----------------------------------------------------------------------------
// Main entry point for clipping rays to entities
//-----------------------------------------------------------------------------
void CEngineTrace::ClipRayToEntity( const Ray_t &ray, unsigned int fMask, IHandleEntity *pEntity, trace_t *pTrace )
{
	ClipRayToCollideable( ray, fMask, GetCollideable(pEntity), pTrace );
}


//-----------------------------------------------------------------------------
// Grabs all entities along a ray
//-----------------------------------------------------------------------------
class CEntitiesAlongRay : public IPartitionEnumerator
{
public:
	CEntitiesAlongRay( ) : m_EntityHandles(0, 32) {}

	void Reset()
	{
		m_EntityHandles.RemoveAll();
	}

	IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		m_EntityHandles.AddToTail( pHandleEntity );
		return ITERATION_CONTINUE;
	}

	CUtlVector< IHandleEntity * >	m_EntityHandles;
};


//-----------------------------------------------------------------------------
// Makes sure the final trace is clipped to the clip trace
// Returns true if clipping occurred
//-----------------------------------------------------------------------------
bool CEngineTrace::ClipTraceToTrace( trace_t &clipTrace, trace_t *pFinalTrace )
{
	if (clipTrace.allsolid || clipTrace.startsolid || (clipTrace.fraction < pFinalTrace->fraction))
	{
		if (pFinalTrace->startsolid)
		{
			float flFractionLeftSolid = pFinalTrace->fractionleftsolid;
			Vector vecStartPos = pFinalTrace->startpos;

			*pFinalTrace = clipTrace;
			pFinalTrace->startsolid = true;

			if ( flFractionLeftSolid > clipTrace.fractionleftsolid )
			{
				pFinalTrace->fractionleftsolid = flFractionLeftSolid;
				pFinalTrace->startpos = vecStartPos;
			}
		}
		else
		{
			*pFinalTrace = clipTrace;
		}
		return true;
	}

	if (clipTrace.startsolid)
	{
		pFinalTrace->startsolid = true;

		if ( clipTrace.fractionleftsolid > pFinalTrace->fractionleftsolid )
		{
			pFinalTrace->fractionleftsolid = clipTrace.fractionleftsolid;
			pFinalTrace->startpos = clipTrace.startpos;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Converts a user id to a collideable + username
//-----------------------------------------------------------------------------
void CEngineTraceServer::HandleEntityToCollideable( IHandleEntity *pHandleEntity, ICollideable **ppCollide, const char **ppDebugName )
{
	*ppCollide = StaticPropMgr()->GetStaticProp( pHandleEntity );
	if ( *ppCollide	)
	{
		*ppDebugName = "static prop";
		return;
	}

	IServerNetworkable *pServerNetworkable = static_cast<IServerNetworkable*>(pHandleEntity);
	if ( !pServerNetworkable )
	{
		*ppCollide = NULL;
		*ppDebugName = "<null>";
		return;
	}

	edict_t *pEdict = pServerNetworkable->GetEdict();
	Assert( pEdict );
	*ppCollide = pEdict->GetCollideable();
	*ppDebugName = pServerNetworkable->GetServerClass()->GetName();
}

#ifndef SWDS
void CEngineTraceClient::HandleEntityToCollideable( IHandleEntity *pHandleEntity, ICollideable **ppCollide, const char **ppDebugName )
{
	*ppCollide = StaticPropMgr()->GetStaticProp( pHandleEntity );
	if ( *ppCollide	)
	{
		*ppDebugName = "static prop";
		return;
	}

	IClientUnknown *pUnk = static_cast<IClientUnknown*>(pHandleEntity);
	if ( !pUnk )
	{
		*ppCollide = NULL;
		*ppDebugName = "<null>";
		return;
	}
	
	*ppCollide = pUnk->GetClientCollideable();
	*ppDebugName = "client entity";
	IClientNetworkable *pNetwork = pUnk->GetClientNetworkable();
	if (pNetwork)
	{
		if (pNetwork->GetClientClass())
		{
			*ppDebugName = pNetwork->GetClientClass()->m_pNetworkName;
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Returns the world collideable for trace setting
//-----------------------------------------------------------------------------
#ifndef SWDS
ICollideable *CEngineTraceClient::GetWorldCollideable()
{
	IClientEntity *pUnk = entitylist->GetClientEntity( 0 );
	Assert( pUnk );
	return pUnk ? pUnk->GetClientCollideable() : NULL;
}
#endif

ICollideable *CEngineTraceServer::GetWorldCollideable()
{
	return sv.edicts->GetCollideable();
}


//-----------------------------------------------------------------------------
// A version that simply accepts a ray (can work as a traceline or tracehull)
//-----------------------------------------------------------------------------
void CEngineTrace::TraceRay( const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace )
{
	CTraceFilterHitAll traceFilter;
	if ( !pTraceFilter )
	{
		pTraceFilter = &traceFilter;
	}

	// Gather statistics.
	g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_TRACE_LINES, 1 );
	MEASURE_TIMED_STAT( ENGINE_STATS_TRACE_LINE_TIME );
	
	CM_ClearTrace( pTrace );

	// Collide with the world.
	if ( pTraceFilter->GetTraceType() != TRACE_ENTITIES_ONLY )
	{
		ICollideable *pCollide = GetWorldCollideable();

		// Make sure the world entity is unrotated
		// FIXME: BAH! The !pCollide test here is because of
		// CStaticProp::PrecacheLighting.. it's occurring too early
		// need to fix that later
		Assert(!pCollide || pCollide->GetCollisionOrigin() == vec3_origin );
		Assert(!pCollide || pCollide->GetCollisionAngles() == vec3_angle );

		CM_BoxTrace( ray, 0, fMask, true, *pTrace );
		SetTraceEntity( pCollide, pTrace );

		// Blocked by the world.
		if ( pTrace->fraction == 0 )
			return;

		// Early out if we only trace against the world
		if ( pTraceFilter->GetTraceType() == TRACE_WORLD_ONLY )
			return;
	}

	// Save the world collision fraction.
	float flWorldFraction = pTrace->fraction;

	// Create a ray that extends only until we hit the world
	// and adjust the trace accordingly
	Ray_t entityRay = ray;
	entityRay.m_Delta *= pTrace->fraction;

	// We know this is safe because if pTrace->fraction == 0
	// we would have exited above
	pTrace->fractionleftsolid /= pTrace->fraction;
 	pTrace->fraction = 1.0;

	// Collide with entities along the ray
	// FIXME: Hitbox code causes this to be re-entrant for the IK stuff.
	// If we could eliminate that, this could be static and therefore
	// not have to reallocate memory all the time
	CEntitiesAlongRay enumerator;
	enumerator.Reset();
	SpatialPartition()->EnumerateElementsAlongRay( SpatialPartitionMask(), entityRay, false, &enumerator );

	bool bNoStaticProps = pTraceFilter->GetTraceType() == TRACE_ENTITIES_ONLY;
	bool bFilterStaticProps = pTraceFilter->GetTraceType() == TRACE_EVERYTHING_FILTER_PROPS;

	trace_t tr;
	ICollideable *pCollideable;
	const char *pDebugName;
	int nCount = enumerator.m_EntityHandles.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		// Generate a collideable
		IHandleEntity *pHandleEntity = enumerator.m_EntityHandles[i];
		HandleEntityToCollideable( pHandleEntity, &pCollideable, &pDebugName );

		// Check for error condition
		if ( !IsSolid( pCollideable->GetSolid(), pCollideable->GetSolidFlags() ) )
		{
			char temp[1024];
			Q_snprintf(temp, sizeof( temp ), "%s in solid list (not solid)\n", pDebugName );
			Sys_Error (temp);
		}

		if ( !StaticPropMgr()->IsStaticProp( pHandleEntity ) )
		{
			if ( !pTraceFilter->ShouldHitEntity( pHandleEntity, fMask ) )
				continue;
		}
		else
		{
			// FIXME: Could remove this check here by
			// using a different spatial partition mask. Look into it
			// if we want more speedups here.
			if ( bNoStaticProps )
				continue;

			if ( bFilterStaticProps )
			{
				if ( !pTraceFilter->ShouldHitEntity( pHandleEntity, fMask ) )
					continue;
			}
		}

		ClipRayToCollideable( entityRay, fMask, pCollideable, &tr );

		// Make sure the ray is always shorter than it currently is
		ClipTraceToTrace( tr, pTrace );

		// Stop if we're in allsolid
		if (pTrace->allsolid)
			break;
	}

	// Fix up the fractions so they are appropriate given the original
	// unclipped-to-world ray
	pTrace->fraction *= flWorldFraction;
	pTrace->fractionleftsolid *= flWorldFraction;

#ifdef _DEBUG
	Vector vecOffset, vecEndTest;
	VectorAdd( ray.m_Start, ray.m_StartOffset, vecOffset );
	VectorMA( vecOffset, pTrace->fractionleftsolid, ray.m_Delta, vecEndTest );
	Assert( VectorsAreEqual( vecEndTest, pTrace->startpos, 0.1f ) );
	VectorMA( vecOffset, pTrace->fraction, ray.m_Delta, vecEndTest );
	Assert( VectorsAreEqual( vecEndTest, pTrace->endpos, 0.1f ) );
//	Assert( !ray.m_IsRay || pTrace->allsolid || pTrace->fraction >= pTrace->fractionleftsolid );
#endif

	if ( !ray.m_IsRay )
	{
		// Make sure no fractionleftsolid can be used with box sweeps
		VectorAdd( ray.m_Start, ray.m_StartOffset, pTrace->startpos );
		pTrace->fractionleftsolid = 0;

#ifdef _DEBUG
		pTrace->fractionleftsolid = VEC_T_NAN;
#endif
	}
}


//-----------------------------------------------------------------------------
// Lets clients know about all edicts along a ray
//-----------------------------------------------------------------------------
class CEnumerationFilter : public IPartitionEnumerator
{
public:
	CEnumerationFilter( CEngineTrace *pEngineTrace, IEntityEnumerator* pEnumerator ) : 
		m_pEngineTrace(pEngineTrace), m_pEnumerator(pEnumerator) {}

	IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		// Don't enumerate static props
		if ( StaticPropMgr()->IsStaticProp( pHandleEntity ) )
			return ITERATION_CONTINUE;

		if ( !m_pEnumerator->EnumEntity( pHandleEntity ) )
		{
			return ITERATION_STOP;
		}

		return ITERATION_CONTINUE;
	}

private:
	IEntityEnumerator* m_pEnumerator;
	CEngineTrace *m_pEngineTrace;
};


//-----------------------------------------------------------------------------
// Enumerates over all entities along a ray
// If triggers == true, it enumerates all triggers along a ray
//-----------------------------------------------------------------------------
void CEngineTrace::EnumerateEntities( const Ray_t &ray, bool bTriggers, IEntityEnumerator *pEnumerator )
{
	// FIXME: If we store CBaseHandles directly in the spatial partition, this method
	// basically becomes obsolete. The spatial partition can be queried directly.
	CEnumerationFilter enumerator( this, pEnumerator );

	int fMask = !bTriggers ? SpatialPartitionMask() : SpatialPartitionTriggerMask();

	// NOTE: Triggers currently don't exist on the client
	if (fMask)
	{
		SpatialPartition()->EnumerateElementsAlongRay( fMask, ray, false, &enumerator );
	}
}


//-----------------------------------------------------------------------------
// Lets clients know about all entities in a box
//-----------------------------------------------------------------------------
void CEngineTrace::EnumerateEntities( const Vector &vecAbsMins, const Vector &vecAbsMaxs, IEntityEnumerator *pEnumerator )
{
	// FIXME: If we store CBaseHandles directly in the spatial partition, this method
	// basically becomes obsolete. The spatial partition can be queried directly.
	CEnumerationFilter enumerator( this, pEnumerator );
	SpatialPartition()->EnumerateElementsInBox( SpatialPartitionMask(),
		vecAbsMins, vecAbsMaxs, false, &enumerator );
}
