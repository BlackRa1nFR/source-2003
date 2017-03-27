//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
// TODO:  Should all of this Map_Vis* stuff be an interface?
//

#include "quakedef.h"
#include "gl_model_private.h"
#include "view_shared.h"
#include "cmodel_engine.h"
#include "tier0/vprof.h"

static ConVar	r_novis( "r_novis","0");
static ConVar	r_lockpvs( "r_lockpvs", "0", 0 );

// ----------------------------------------------------------------------
// Renderer interface to vis
// ----------------------------------------------------------------------
int  r_visframecount = 0;

//-----------------------------------------------------------------------------
// Purpose: For each cluster to be OR'd into vis, remember the origin, the last viewcluster
//  for that origin and the current one, so we can tell when vis is dirty and needs to be
//  recomputed
//-----------------------------------------------------------------------------
typedef struct
{
	Vector	origin;
	int		viewcluster;
	int		oldviewcluster;
} VISCLUSTER;

//-----------------------------------------------------------------------------
// Purpose: Stores info for updating vis data for the map
//-----------------------------------------------------------------------------
typedef struct
{
	// Number of relevant vis clusters
	int			nClusters;
	// Last number ( if != nClusters, recompute vis )
	int			oldnClusters;
	// List of clusters to merge together for final vis
	VISCLUSTER	rgVisClusters[ MAX_VIS_LEAVES ];
	// Composite vis data
	byte		rgCurrentVis[ MAX_MAP_LEAFS/8 ];
} VISINFO;

static VISINFO vis;

//-----------------------------------------------------------------------------
// Purpose: Mark the leaves and nodes that are in the PVS for the current
//  cluster(s)
// Input  : *worldmodel - 
//-----------------------------------------------------------------------------
void Map_VisMark( bool forcenovis, model_t *worldmodel )
{
	VPROF_BUDGET( "Map_VisMark", VPROF_BUDGETGROUP_WORLD_RENDERING );
	mnode_t		*node;
	int			i, c;
	mleaf_t		*leaf;
	int			cluster;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if ( r_lockpvs.GetInt() )
	{
		return;
	}

	bool outsideWorld = false;
	for ( i = 0; i < vis.nClusters; i++ )
	{
		if ( vis.rgVisClusters[ i ].viewcluster != vis.rgVisClusters[ i ].oldviewcluster )
		{
			break;
		}
	}

	// No changes
	if ( i >= vis.nClusters && !forcenovis && ( vis.nClusters == vis.oldnClusters ) )
	{
		return;
	}

	// Update vis frame marker
	r_visframecount++;

	// Update cluster history
	vis.oldnClusters = vis.nClusters;
	for ( i = 0; i < vis.nClusters; i++ )
	{
		vis.rgVisClusters[ i ].oldviewcluster = vis.rgVisClusters[ i ].viewcluster;
		// Outside world?
		if ( vis.rgVisClusters[ i ].viewcluster == -1 )
		{
			outsideWorld = true;
			break;
		}
	}

#ifdef USE_CONVARS
	if ( r_novis.GetInt() || forcenovis || outsideWorld )
	{
		// mark everything
		for (i=0 ; i<worldmodel->brush.numleafs ; i++)
		{
			worldmodel->brush.leafs[i].visframe = r_visframecount;
		}
		for (i=0 ; i<worldmodel->brush.numnodes ; i++)
		{
			worldmodel->brush.nodes[i].visframe = r_visframecount;
		}
		return;
	}
#endif

	// There should always be at least one origin and that's the default render origin in most cases
	assert( vis.nClusters >= 1 );

	CM_Vis( vis.rgCurrentVis, vis.rgVisClusters[ 0 ].viewcluster, DVIS_PVS );

	// Get cluster count
	c = ( CM_NumClusters() + 31 ) / 32 ;

	// Merge in any extra clusters
	for ( i = 1; i < vis.nClusters; i++ )
	{
		byte	mapVis[ MAX_MAP_CLUSTERS/8 ];
		
		CM_Vis( mapVis, vis.rgVisClusters[ i ].viewcluster, DVIS_PVS );
				
		// Copy one dword at a time ( could use memcpy )
		for ( int j = 0 ; j < c ; j++ )
		{
			((int *)vis.rgCurrentVis)[ j ] |= ((int *)mapVis)[ j ];
		}
	}
	
	for ( i = 0, leaf = worldmodel->brush.leafs ; i < worldmodel->brush.numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if ( cluster == -1 )
			continue;

		if (vis.rgCurrentVis[cluster>>3] & (1<<(cluster&7)))
		{
#if 0
			int j;
			unsigned short *start;

			if ( worldmodel->portals )
			{
				start = worldmodel->clusters[ cluster ].portalList;
				for ( j = 0; j < worldmodel->clusters[ cluster ].numportals; j++ )
				{
					worldmodel->portals[ *start ].visframe = r_visframecount;
					start++;
				}
			}
#endif

			node = ( mnode_t * )leaf;
			do
			{
				if ( node->visframe == r_visframecount )
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while ( node );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Purpose: Setup vis for the specified map
// Input  : *worldmodel - the map
//			visorigincount - how many origins to merge together ( usually 1, can be 0 if forcenovis is true )
//			origins[][3] - array of origins to merge together
//			forcenovis - if set to true, ignore all origins and just mark everything as visible ( SLOW rendering!!! )
//-----------------------------------------------------------------------------
void Map_VisSetup( model_t *worldmodel, int visorigincount, const Vector origins[], bool forcenovis /*=false*/ )
{
	assert( visorigincount <= MAX_VIS_LEAVES );

	// Don't crash if the client .dll tries to do something weird/dumb
	vis.nClusters = min( visorigincount, MAX_VIS_LEAVES );

	for ( int i = 0; i < vis.nClusters; i++ )
	{
		int leafIndex = CM_PointLeafnum( origins[ i ] );
		vis.rgVisClusters[ i ].viewcluster = CM_LeafCluster( leafIndex );
		VectorCopy( origins[ i ], vis.rgVisClusters[ i ].origin );
	}
	
	Map_VisMark( forcenovis, worldmodel );
}

//-----------------------------------------------------------------------------
// Purpose: Clear / reset vis data
//-----------------------------------------------------------------------------
void Map_VisClear( void )
{
	vis.nClusters = 1;
	vis.oldnClusters = 1;
	for ( int i = 0; i < MAX_VIS_LEAVES; i++ )
	{
		vis.rgVisClusters[ i ].oldviewcluster = -2;
		VectorClear( vis.rgVisClusters[ i ].origin );
		vis.rgVisClusters[ i ].viewcluster = -2;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the current vis bitfield
// Output : byte
//-----------------------------------------------------------------------------
byte *Map_VisCurrent( void )
{
	return vis.rgCurrentVis;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the first viewcluster ( usually it's the only )
// Output : int
//-----------------------------------------------------------------------------
int Map_VisCurrentCluster( void )
{
	// BUGBUG: The client DLL can hit this assert during a level transition
	// because the temporary entities do visibility calculations during the 
	// wrong part of the frame loop (i.e. before a view has been set up!)
	if ( vis.rgVisClusters[ 0 ].viewcluster < 0 )
	{
		static int visclusterwarningcount = 0;

		if ( ++visclusterwarningcount <= 5 )
		{
			Con_DPrintf( "Map_VisCurrentCluster() < 0!\n" ); 
		}
	}
	return vis.rgVisClusters[ 0 ].viewcluster;
}

