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
#include "quakedef.h"
#include "world.h"
#include "eiface.h"
#include "server.h"
#include "cmodel_engine.h"
#include "gl_model_private.h"
#include "enginestats.h"
#include "sv_main.h"
#include "vengineserver_impl.h"
#include "collisionutils.h"
#include "vphysics_interface.h"
#include "ispatialpartitioninternal.h"
#include "staticpropmgr.h"
#include "string_t.h"

//-----------------------------------------------------------------------------
// Method to convert edict to index
//-----------------------------------------------------------------------------
static inline int IndexOfEdict( edict_t* pEdict )
{
	return (int)(pEdict - sv.edicts);
}



//============================================================================



//-----------------------------------------------------------------------------
// Purpose: returns a headnode that can be used to collide against this entity
// Input  : *ent - 
// Output : int
//-----------------------------------------------------------------------------
int SV_HullForEntity( edict_t *ent )
{
	model_t		*model;
	IServerEntity *serverEntity = ent->GetIServerEntity();
	Assert( serverEntity );
	if ( !serverEntity )
		return -1;

	int modelindex = serverEntity->GetModelIndex();
	model = sv.GetModel( modelindex );

	if (model->type == mod_brush)
	{
		cmodel_t *pCModel = CM_InlineModelNumber( modelindex - 1 );

		return pCModel->headnode;
	}

	ICollideable *pCollideable = serverEntity->GetCollideable();
	Vector vecMins = pCollideable->WorldAlignMins();
	Vector vecMaxs = pCollideable->WorldAlignMaxs();
	return CM_HeadnodeForBoxHull( vecMins, vecMaxs );
}


/*
===============
SV_ClearWorld

===============
*/
void SV_ClearWorld (void)
{
	// Clean up static props from the previous level
	StaticPropMgr()->LevelShutdown();

	for ( int i = 0; i < 3; i++ )
	{
		if ( host_state.worldmodel->mins[i] < MIN_COORD_INTEGER || host_state.worldmodel->maxs[i] > MAX_COORD_INTEGER )
		{
			Host_EndGame("Map coordinate extents are too large!!\nCheck for errors!\n" );
		}
	}
	SpatialPartition()->Init( host_state.worldmodel->mins, host_state.worldmodel->maxs );

	// Load all static props into the spatial partition
	StaticPropMgr()->LevelInit();
}


/*
===============
SV_UnlinkEdict

===============
*/
void SV_UnlinkEdict (edict_t *ent)
{
	if (ent->partition != PARTITION_INVALID_HANDLE)
	{
		SpatialPartition()->DestroyHandle( ent->partition );
		ent->partition = PARTITION_INVALID_HANDLE;
	}
}


// I blew this away; check version 49 
// edict_t *SV_Radius( Vector& center, float radius, areanode_t *node, edict_t *list )



#define MAX_TOTAL_ENT_LEAFS		128
//-----------------------------------------------------------------------------
// Purpose: Builds the cluster list for an entity
// Input  : *pEdict - 
//-----------------------------------------------------------------------------
void SV_BuildEntityClusterList( edict_t *pEdict )
{
	int		i, j;
	int		topnode;
	int		leafCount;
	int		leafs[MAX_TOTAL_ENT_LEAFS], clusters[MAX_TOTAL_ENT_LEAFS];
	int		area;

	IServerEntity *serverEntity = pEdict->GetIServerEntity();
	Assert( serverEntity );
	if ( !serverEntity )
		return;

	pEdict->clusterCount = 0;
	topnode = -1;
	pEdict->areanum = 0;
	pEdict->areanum2 = 0;

	//get all leafs, including solids
	leafCount = CM_BoxLeafnums( serverEntity->GetAbsMins(), serverEntity->GetAbsMaxs(), leafs, MAX_TOTAL_ENT_LEAFS, &topnode );

	// set areas
	for ( i = 0; i < leafCount; i++ )
	{
		clusters[i] = CM_LeafCluster( leafs[i] );
		area = CM_LeafArea( leafs[i] );
		if ( area )
		{	// doors may legally straggle two areas,
			// but nothing should evern need more than that
			if ( pEdict->areanum && pEdict->areanum != area )
			{
				if ( pEdict->areanum2 && pEdict->areanum2 != area && sv.state == ss_loading )
				{
					Con_DPrintf ("Object touching 3 areas at %f %f %f\n",
						serverEntity->GetAbsMins()[0], serverEntity->GetAbsMins()[1], serverEntity->GetAbsMins()[2]);
				}
				pEdict->areanum2 = area;
			}
			else
			{
				pEdict->areanum = area;
			}
		}
	}

	pEdict->headnode = topnode;	// save headnode

	if ( leafCount >= MAX_TOTAL_ENT_LEAFS )
	{	// assume we missed some leafs, and mark by headnode
		pEdict->clusterCount = -1;
	}
	else
	{
		for ( i = 0; i < leafCount; i++ )
		{
			if (clusters[i] == -1)
				continue;		// not a visible leaf

			for ( j = 0; j < i; j++ )
			{
				if (clusters[j] == clusters[i])
					break;
			}

			if ( j == i )
			{
				if ( pEdict->clusterCount == MAX_ENT_CLUSTERS )
				{	// assume we missed some leafs, and mark by headnode
					pEdict->clusterCount = -1;
					break;
				}

				pEdict->clusters[pEdict->clusterCount++] = clusters[i];
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Temporarily unlink from the collision tree to perform some collision
//			operations and then put the object back in
// Input  : *ent - entity to fast unlink
//-----------------------------------------------------------------------------
int SV_FastUnlink( edict_t *ent )
{
	// Returns a temporary handle
	return SpatialPartition()->FastRemove( ent->partition );
}

//-----------------------------------------------------------------------------
// Purpose: Undo fast unlink.
// Input  : *ent - 
//-----------------------------------------------------------------------------

void SV_FastRelink( edict_t *ent, int tempHandle )
{
	SpatialPartition()->FastInsert( ent->partition, tempHandle );
}


//-----------------------------------------------------------------------------
// Little enumeration class used to try touching all triggers
//-----------------------------------------------------------------------------
class CTouchLinks : public IPartitionEnumerator
{
public:
	CTouchLinks( edict_t* pEnt, const Vector* pPrevAbsOrigin ) : m_TouchedEntities( 8, 8 )
	{
		m_pEnt = pEnt;
		IServerEntity *serverEntity = pEnt->GetIServerEntity();
		Assert( serverEntity );
		if (pPrevAbsOrigin)
		{
			Vector mins, maxs;

			VectorSubtract( serverEntity->GetAbsMins(), serverEntity->GetAbsOrigin(), mins );
			VectorSubtract( serverEntity->GetAbsMaxs(), serverEntity->GetAbsOrigin(), maxs );

			m_Ray.Init( *pPrevAbsOrigin, serverEntity->GetAbsOrigin(), mins, maxs );
		}
		else
		{
			m_Ray.m_Start = serverEntity->GetAbsOrigin();
			m_mins = serverEntity->GetAbsMins() - m_Ray.m_Start;
			m_maxs = serverEntity->GetAbsMaxs() - m_Ray.m_Start;
			m_Ray.m_IsSwept = false;
		}
	}

	IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		// Static props should never be in the trigger list 
		Assert( !StaticPropMgr()->IsStaticProp( pHandleEntity ) );

		IServerNetworkable *pNetworkable = static_cast<IServerNetworkable*>( pHandleEntity );
		Assert( pNetworkable );

		// Convert the IHandleEntity to an edict_t*...
		// Context is the thing we're testing everything against		
		edict_t* pTouch = pNetworkable->GetEdict();

		// Can't bump against itself
		if ( pTouch == m_pEnt )
			return ITERATION_CONTINUE;

		IServerEntity *serverEntity = pTouch->GetIServerEntity();
		if ( !serverEntity )
			return ITERATION_CONTINUE;

		// Hmmm.. everything in this list should be a trigger....
		ICollideable *pCollideable = serverEntity->GetCollideable();
		Assert(pCollideable->GetSolidFlags() & FSOLID_TRIGGER );
		if ( (pCollideable->GetSolidFlags() & FSOLID_TRIGGER) == 0 )
			return ITERATION_CONTINUE;

		model_t* pModel = sv.GetModel( pCollideable->GetCollisionModelIndex() );
		if ( pModel && pModel->type == mod_brush )
		{
			int headnode = SV_HullForEntity( pTouch );

			int contents;
			if (!m_Ray.m_IsSwept)
			{
				contents = CM_TransformedBoxContents( m_Ray.m_Start, m_mins, m_maxs,
					headnode, serverEntity->GetAbsOrigin(), serverEntity->GetAbsAngles() );
			}
			else
			{
				trace_t trace;
				CM_TransformedBoxTrace( m_Ray, headnode, MASK_ALL, serverEntity->GetAbsOrigin(), 
					serverEntity->GetAbsAngles(), trace );
				contents = trace.contents;
			}

			if ( !(contents & MASK_SOLID) )
				return ITERATION_CONTINUE;
		}

		m_TouchedEntities.AddToTail( pTouch );

		return ITERATION_CONTINUE;
	}

	void HandleTouchedEntities( )
	{
		for ( int i = 0; i < m_TouchedEntities.Count(); ++i )
		{
			serverGameEnts->MarkEntitiesAsTouching( m_TouchedEntities[i], m_pEnt );
		}
	}

	Ray_t m_Ray;

private:
	edict_t *m_pEnt;
	Vector m_mins;
	Vector m_maxs;
	CUtlVector< edict_t* > m_TouchedEntities;
};


// enumerator class that's used to update touch links for a trigger when 
// it moves or changes solid type
class CTriggerMoved : public IPartitionEnumerator
{
public:
	CTriggerMoved( edict_t *pTriggerEntity ) : m_TouchedEntities( 8, 8 )
	{
		m_headnode = -1;
		m_pTriggerEntity = pTriggerEntity;
		IServerEntity *pServerTrigger = pTriggerEntity->GetIServerEntity();
		if ( !pServerTrigger )
		{
			m_origin.Init();
			m_angles.Init();
		}
		else
		{
			model_t *pModel = sv.GetModel( pServerTrigger->GetModelIndex() );
			if ( pModel && pModel->type == mod_brush )
			{
				m_headnode = SV_HullForEntity( pTriggerEntity );
			}
			m_origin = pServerTrigger->GetAbsOrigin();
			m_angles = pServerTrigger->GetAbsAngles();
		}
	}

	IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		// skip static props, the game DLL doesn't care about them
		if ( StaticPropMgr()->IsStaticProp( pHandleEntity ) )
			return ITERATION_CONTINUE;

		IServerNetworkable *pNetworkable = static_cast< IServerNetworkable* >( pHandleEntity );
		Assert( pNetworkable );

		// Convert the user ID to and edict_t*...
		edict_t* pTouch = pNetworkable->GetEdict();

		// Can't ever touch itself because it's in the other list
		if ( pTouch == m_pTriggerEntity )
			return ITERATION_CONTINUE;

		IServerEntity *serverEntity = pTouch->GetIServerEntity();
		if ( !serverEntity )
			return ITERATION_CONTINUE;

		if ( m_headnode >= 0 )
		{
			Vector vecMins, vecMaxs;
			CM_WorldAlignBounds( serverEntity->GetCollideable(), &vecMins, &vecMaxs );
			int contents = CM_TransformedBoxContents( serverEntity->GetAbsOrigin(), 
					vecMins, vecMaxs, m_headnode, m_origin, m_angles );
			if ( !(contents & MASK_SOLID) )
				return ITERATION_CONTINUE;
		}

		m_TouchedEntities.AddToTail( pTouch );

		return ITERATION_CONTINUE;
	}

	void HandleTouchedEntities( )
	{
		for ( int i = 0; i < m_TouchedEntities.Count(); ++i )
		{
			serverGameEnts->MarkEntitiesAsTouching( m_TouchedEntities[i], m_pTriggerEntity );
		}
	}

private:
	Vector	m_origin;
	QAngle	m_angles;
	edict_t	*m_pTriggerEntity;
	int		m_headnode;
	CUtlVector< edict_t* > m_TouchedEntities;
};


/*
===============
SV_LinkEdict

===============
*/
void SV_LinkEdict( edict_t *ent, qboolean touch_triggers, const Vector* pPrevAbsOrigin )
{
	IServerEntity *pServerEntity = ent->GetIServerEntity();
	if ( !pServerEntity )
		return;		
	
	// Remove it from whatever lists it may be in at the moment
	// We'll re-add it below if we need to.
	if (ent->partition != PARTITION_INVALID_HANDLE)
	{
		SpatialPartition()->Remove( ent->partition );
	}

	if (ent->free)
		return;

	if (ent == sv.edicts)
		return;		// don't add the world

	pServerEntity->CalcAbsolutePosition();

	// set the abs box
	pServerEntity->SetObjectCollisionBox();
	
	// link to PVS leafs
	SV_BuildEntityClusterList( ent );

	// Update KD tree information
	ICollideable *pCollide = pServerEntity->GetCollideable();
	if (ent->partition == PARTITION_INVALID_HANDLE)
	{
		// Here, we haven't added the entity to the partition before
		// So we have to make a new partition handle.
		ent->partition = SpatialPartition()->CreateHandle( pCollide->GetEntityHandle() );
	}

	// Here, we indicate the entity has moved. Note that this call does
	// some fast early-outing to prevent unnecessary work
	SpatialPartition()->ElementMoved( ent->partition, pServerEntity->GetAbsMins(), pServerEntity->GetAbsMaxs() );

	// Make sure it's in the list of all entities
	SpatialPartition()->Insert( PARTITION_ENGINE_NON_STATIC_EDICTS, ent->partition );

	SolidType_t iSolid = pCollide->GetSolid();
	int nSolidFlags = pCollide->GetSolidFlags();
	bool bIsSolid = IsSolid( iSolid, nSolidFlags ) || ((nSolidFlags & FSOLID_TRIGGER) != 0);
	if ( !bIsSolid )
	{
		// If this ent's touch list isn't empty, it's transitioning to not solid
		if ( pServerEntity->IsCurrentlyTouching() )
		{
			// mark ent so that at the end of frame it will check to see if it's no longer touch ents
			pServerEntity->SetCheckUntouch( true );
		}
		return;
	}

	// Insert it into the appropriate lists.
	// We have to continually reinsert it because its solid type may have changed
	SpatialPartitionListMask_t mask = 0;
	if (( nSolidFlags & FSOLID_NOT_SOLID ) == 0)
	{
		mask |=	PARTITION_ENGINE_SOLID_EDICTS;
	}
	if (( nSolidFlags & FSOLID_TRIGGER ) != 0 )
	{
		mask |=	PARTITION_ENGINE_TRIGGER_EDICTS;
	}
	Assert( mask != 0 );
	SpatialPartition()->Insert( mask, ent->partition );

	if ( iSolid == SOLID_BSP ) 
	{
		model_t	*model = sv.GetModel( pServerEntity->GetModelIndex() );
		if ( !model && strlen( STRING( pServerEntity->GetModelName() ) ) == 0 ) 
		{
			Con_DPrintf( "Inserted %s with no model\n", STRING( ent->classname ) );
			return;
		}
	}

	// if touch_triggers, touch all entities at this node and descend for more
	if (touch_triggers)
	{
		// mark ent so that at the end of frame it will check to see if it's no longer touch ents
		pServerEntity->SetCheckUntouch( true );

		// If this is a trigger that's moved, then query the solid list instead
		if ( mask == PARTITION_ENGINE_TRIGGER_EDICTS )
		{
			CTriggerMoved triggerEnum( ent );

			SpatialPartition()->EnumerateElementsInBox( PARTITION_ENGINE_SOLID_EDICTS,
				pServerEntity->GetAbsMins(), pServerEntity->GetAbsMaxs(), false, &triggerEnum );

			triggerEnum.HandleTouchedEntities( );
		}
		else
		{
			if (!pPrevAbsOrigin)
			{
				CTouchLinks touchEnumerator(ent, NULL);

				SpatialPartition()->EnumerateElementsInBox( PARTITION_ENGINE_TRIGGER_EDICTS,
					pServerEntity->GetAbsMins(), pServerEntity->GetAbsMaxs(), false, &touchEnumerator );

				touchEnumerator.HandleTouchedEntities( );
			}
			else
			{
				CTouchLinks touchEnumerator(ent, pPrevAbsOrigin);

				// A version that checks against an extruded ray indicating the motion
				SpatialPartition()->EnumerateElementsAlongRay( PARTITION_ENGINE_TRIGGER_EDICTS,
					touchEnumerator.m_Ray, false, &touchEnumerator );

				touchEnumerator.HandleTouchedEntities( );
			}
		}
	}
}


