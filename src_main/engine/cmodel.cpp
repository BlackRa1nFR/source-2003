//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: BSP collision!
//
// $NoKeywords: $
//=============================================================================

#include "cmodel_engine.h"

#include "quakedef.h"
#include <string.h>
#include <stdlib.h>
#include "mathlib.h"
#include "conprint.h"
#include "common.h"
#include "sysexternal.h"
#include "enginestats.h"
#include "measure_section.h"
#include "zone.h"
#include "utlvector.h"
#include "const.h"
#include "gl_model_private.h"
#include "vphysics_interface.h"
#include "icliententity.h"
#include "engine/icollideable.h"


CCollisionBSPData g_BSPData;								// the global collision bsp
CCollisionCounts  g_CollisionCounts;						// collision test counters

Vector trace_start;
Vector trace_end;
Vector trace_mins;
Vector trace_maxs;
Vector trace_extents;

trace_t trace_trace;
trace_t g_StabTrace;

int trace_contents;
qboolean trace_ispoint;

csurface_t nullsurface = { "**empty**", 0 };				// generic null collision model surface

enum
{
	MAX_CHECK_COUNT_DEPTH = 2
};

int s_nCheckCount[MAX_CHECK_COUNT_DEPTH];					// global collision checkcount
int s_nCheckCountDepth = -1;

void BeginCheckCount()
{
	++s_nCheckCountDepth;
	Assert( (s_nCheckCountDepth >= 0) && (s_nCheckCountDepth < MAX_CHECK_COUNT_DEPTH) );
	++s_nCheckCount[s_nCheckCountDepth];
}

int CurrentCheckCount()
{
	return s_nCheckCount[s_nCheckCountDepth];
}

int CurrentCheckCountDepth()
{
	return s_nCheckCountDepth;
}

void EndCheckCount()
{
	--s_nCheckCountDepth;
	Assert( s_nCheckCountDepth >= -1 );
}


typedef CUtlVector<cnode_t> CSubBSPTree;
static CSubBSPTree s_BSPSubTree;

static ConVar map_noareas( "map_noareas", "0", 0 );

void	CM_InitBoxHull (CCollisionBSPData *pBSPData);
void	FloodAreaConnections (CCollisionBSPData *pBSPData);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
vcollide_t *CM_GetVCollide( int modelIndex )
{
	cmodel_t *pModel = CM_InlineModelNumber( modelIndex );
	if( !pModel )
		return NULL;

	// return the model's collision data
	return &pModel->vcollisionData;
} 


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
cmodel_t *CM_InlineModel( const char *name )
{
	// error checking!
	if( !name )
		return NULL; 

	// JAYHL2: HACKHACK Get rid of this
	if( !strncmp( name, "maps/", 5 ) )
		return CM_InlineModelNumber( 0 );

	// check for valid name
	if( name[0] != '*' )
		Sys_Error( "CM_InlineModel: bad model name!" );

	// check for valid model
	int ndxModel = atoi( name + 1 );
	if( ( ndxModel < 1 ) || ( ndxModel >= GetCollisionBSPData()->numcmodels ) )
		Sys_Error( "CM_InlineModel: bad model number!" );

	return CM_InlineModelNumber( ndxModel );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
cmodel_t *CM_InlineModelNumber( int index )
{
	CCollisionBSPData *pBSPDataData = GetCollisionBSPData();

	if( ( index < 0 ) || ( index > pBSPDataData->numcmodels ) )
		return NULL;

	return ( &pBSPDataData->map_cmodels[ index ] );
}


int CM_BrushContents_r( CCollisionBSPData *pBSPData, int nodenum )
{
	int contents = 0;

	while (1)
	{
		if (nodenum < 0)
		{
			int leafIndex = -1 - nodenum;
			cleaf_t &leaf = pBSPData->map_leafs[leafIndex];
			
			for ( int i = 0; i < leaf.numleafbrushes; i++ )
			{
				unsigned short brushIndex = pBSPData->map_leafbrushes[ leaf.firstleafbrush + i ];
				contents |= pBSPData->map_brushes[brushIndex].contents;
			}

			return contents;
		}

		cnode_t &node = pBSPData->map_rootnode[nodenum];
		contents |= CM_BrushContents_r( pBSPData, node.children[0] );
		nodenum = node.children[1];
	}

	return contents;
}


int CM_InlineModelContents( int index )
{
	cmodel_t *pModel = CM_InlineModelNumber( index );
	if ( !pModel )
		return 0;

	return CM_BrushContents_r( GetCollisionBSPData(), pModel->headnode );
}
			

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int	CM_NumClusters( void )
{
	return GetCollisionBSPData()->numclusters;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
char *CM_EntityString( void )
{
	return GetCollisionBSPData()->map_entitystring.Base();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int	CM_LeafContents( int leafnum )
{
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	Assert( leafnum >= 0 );
	Assert( leafnum < pBSPData->numleafs );

	return pBSPData->map_leafs[leafnum].contents;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int	CM_LeafCluster( int leafnum )
{
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	Assert( leafnum >= 0 );
	Assert( leafnum < pBSPData->numleafs );

	return pBSPData->map_leafs[leafnum].cluster;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int	CM_LeafArea( int leafnum )
{
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	Assert( leafnum >= 0 );
	Assert( leafnum < pBSPData->numleafs );

	return pBSPData->map_leafs[leafnum].area;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CM_FreeMap(void)
{
	// get the current collision bsp -- there is only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	// free the collision bsp data
	CollisionBSPData_Destroy( pBSPData );
}


// This turns on all the area portals that are "always on" in the map.
void CM_InitPortalOpenState( CCollisionBSPData *pBSPData )
{
	for ( int i=0; i < pBSPData->numportalopen; i++ )
	{
		pBSPData->portalopen[i] = false;
	}
}


/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
cmodel_t *CM_LoadMap( const char *name, qboolean clientload, unsigned *checksum )
{
	static unsigned	int last_checksum = 0xFFFFFFFF;

	// get the current bsp -- there is currently only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	Assert( physcollision );

	// UNDONE: Don't load the map twice - call this from a function that loads once,
	// parses this part first, then parses/copies the renderer data
	// NOTE: Measurement has determined that loading twice isn't a big speed hit - maybe don't worry
	// about it?  Measure again with larger maps and decide.
	if( !strcmp( pBSPData->map_name, name ) && ( clientload /*|| !Cvar_VariableValue ("flushmap")*/ ) )
	{
		*checksum = last_checksum;
		if (!clientload)
		{
			CM_InitPortalOpenState( pBSPData );
			FloodAreaConnections( pBSPData );
		}
		return &pBSPData->map_cmodels[0];		// still have the right version
	}
	// free any previously load map
	CM_FreeMap();
	// only pre-load if the map doesn't already exist
	CollisionBSPData_PreLoad( pBSPData );

	if( !name || !name[0] )
	{
		*checksum = 0;
		return &pBSPData->map_cmodels[0];			// cinematic servers won't have anything at all
	}

	// read in the collision model data
	CMapLoadHelper::Init( 0, name );
	CollisionBSPData_Load( name, pBSPData );
	CMapLoadHelper::Shutdown( );

	CM_InitBoxHull( pBSPData );

    // Push the displacement bounding boxes down the tree and set leaf data.
    CM_DispTreeLeafnum( pBSPData );

	CM_InitPortalOpenState( pBSPData );
	FloodAreaConnections(pBSPData);

	// initialize counters
	CollisionCounts_Init( &g_CollisionCounts );

	return &pBSPData->map_cmodels[0];
}


//-----------------------------------------------------------------------------
//
// Methods associated with colliding against the world + models
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// returns a vcollide that can be used to collide against this model
//-----------------------------------------------------------------------------
vcollide_t* CM_VCollideForModel( int modelindex, model_t* pModel )
{
	extern vcollide_t *Mod_VCollide( model_t* pModel );
	switch( pModel->type )
	{
	case mod_brush:
		return CM_GetVCollide( modelindex-1 );
	case mod_studio:
		return Mod_VCollide( pModel );
	}

	return 0;
}




//=======================================================================

cplane_t	*box_planes;
int			box_headnode;
cbrush_t	*box_brush;
cleaf_t		*box_leaf;

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull( CCollisionBSPData *pBSPData )
{
	int			i;
	int			side;
	cnode_t		*c;
	cplane_t	*p;
	cbrushside_t	*s;

	box_headnode = pBSPData->numnodes;
	box_planes = &pBSPData->map_planes[pBSPData->numplanes];

	box_brush = &pBSPData->map_brushes[pBSPData->numbrushes];
	memset( box_brush, 0, sizeof(*box_brush) );
	box_brush->numsides = 6;
	box_brush->firstbrushside = pBSPData->numbrushsides;
	box_brush->contents = CONTENTS_SOLID;

	box_leaf = &pBSPData->map_leafs[pBSPData->numleafs];
	memset( box_leaf, 0, sizeof(*box_leaf) );
	box_leaf->contents = CONTENTS_SOLID;
	box_leaf->firstleafbrush = pBSPData->numleafbrushes;
	box_leaf->numleafbrushes = 1;

	pBSPData->map_leafbrushes[pBSPData->numleafbrushes] = pBSPData->numbrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &pBSPData->map_brushsides[pBSPData->numbrushsides+i];
		s->plane = &pBSPData->map_planes[(pBSPData->numplanes+i*2+side)];
		s->surface = &nullsurface;
		s->bBevel = false;

		// nodes
		c = &pBSPData->map_rootnode[box_headnode+i];
		c->plane = &pBSPData->map_planes[(pBSPData->numplanes+i*2)];
		c->children[side] = -1 - pBSPData->emptyleaf;
		if (i != 5)
		{
			c->children[side^1] = box_headnode+i + 1;
		}
		else
		{
			c->children[side^1] = -1 - pBSPData->numleafs;
		}

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;
	}	
}


/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
int	CM_HeadnodeForBoxHull(const Vector& mins, const Vector& maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_headnode;
}


/*
==================
CM_PointLeafnum_r

==================
*/
int CM_PointLeafnum_r( CCollisionBSPData *pBSPData, const Vector& p, int num)
{
	float		d;
	cnode_t		*node;
	cplane_t	*plane;

	while (num >= 0)
	{
		node = pBSPData->map_rootnode + num;
		plane = node->plane;
		
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	g_CollisionCounts.m_PointContents++;			// optimize counter

	return -1 - num;
}

int CM_PointLeafnum (const Vector& p)
{
	// get the current collision bsp -- there is only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	if (!pBSPData->numplanes)
		return 0;		// sound may call this without map loaded
	return CM_PointLeafnum_r (pBSPData, p, 0);
}



/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/

static int		leaf_count, leaf_maxcount;
static int		*leaf_list;
static const Vector *leaf_mins, *leaf_maxs;
static int		leaf_topnode;

void CM_BoxLeafnums_r( CCollisionBSPData *pBSPData, int nodenum )
{
	cplane_t	*plane;
	cnode_t		*node;
	int		s;
	int prev_topnode = -1;

	while (1)
	{
		if (nodenum < 0)
		{
			// This handles the case when the box lies completely
			// within a single node. In that case, the top node should be
			// the parent of the leaf
			if (leaf_topnode == -1)
				leaf_topnode = prev_topnode;

			if (leaf_count >= leaf_maxcount)
			{
//				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
				return;
			}
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}
	
		node = &pBSPData->map_rootnode[nodenum];
		plane = node->plane;
//		s = BoxOnPlaneSide (leaf_mins, leaf_maxs, plane);
//		s = BOX_ON_PLANE_SIDE(*leaf_mins, *leaf_maxs, plane);
		Vector mins, maxs;
		mins = *leaf_mins;
		maxs = *leaf_maxs;
		s = BoxOnPlaneSide2( mins, maxs, plane );

		prev_topnode = nodenum;
		if (s == 1)
			nodenum = node->children[0];
		else if (s == 2)
			nodenum = node->children[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			CM_BoxLeafnums_r (pBSPData, node->children[0]);
			nodenum = node->children[1];
		}
	}
}

int	CM_BoxLeafnums_headnode ( CCollisionBSPData *pBSPData, const Vector& mins, const Vector& maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = &mins;
	leaf_maxs = &maxs;

	leaf_topnode = -1;

	CM_BoxLeafnums_r (pBSPData, headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

int	CM_BoxLeafnums ( const Vector& mins, const Vector& maxs, int *list, int listsize, int *topnode)
{
	// get the current collision bsp -- there is only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	return CM_BoxLeafnums_headnode (pBSPData, mins, maxs, list,
		listsize, pBSPData->map_cmodels[0].headnode, topnode);
}


//-----------------------------------------------------------------------------
// Inserts a node into the constrained bsp tree (a subset within a particular volume)
//-----------------------------------------------------------------------------

static int InsertNodeIntoTree( CCollisionBSPData *pBSPData, int node, int parent, int child )
{
	if (node >= 0)
	{
		// Split nodes must remain in the sub-tree
		cnode_t element;
		element.plane = pBSPData->map_nodes[node].plane;
		element.children[0] = element.children[1] = -1;
		int idx = s_BSPSubTree.AddToTail( element );

		// Hook into the tree
		if (parent != -1)
		{
			s_BSPSubTree[parent].children[child] = idx;
		}

		return idx;
	}
	else
	{
		// In this case, we're inserting a leaf. Just use the leaf index
		// instead of the subtree node index.
		if (parent != -1)
		{
			s_BSPSubTree[parent].children[child] = node;
		}
		else
		{
			// Note that it is absolutely possible for us to be able to
			// create a subtree that has no splits and therefore no nodes
			// this is a bit of trickery that makes a single node tree in this
			// case which always chooses the left child...
			// we're gonna make a plane with a normal (0,0,0) and a dist 1
			// n dot p will always be < intercept
			static cplane_t zeroPlane;
			if (zeroPlane.dist == 0.0f)
			{
				zeroPlane.dist = 1.0f;
				zeroPlane.type = 3;
			}

			cnode_t element;
			element.plane = &zeroPlane;
			element.children[0] = node;
			element.children[1] = -1;
			s_BSPSubTree.AddToTail( element );
		}

		return -1;
	}
}

static void CM_BuildSubTree_r( CCollisionBSPData *pBSPData, int nodenum, 
									int parentNode, int childNum )
{
	cplane_t	*plane;
	cnode_t		*node;
	int		s;

	while (1)
	{
		if (nodenum < 0)
		{
			// Leaves nust remain in the subtree
			InsertNodeIntoTree( pBSPData, nodenum, parentNode, childNum );
			return;
		}
	
		node = &pBSPData->map_nodes[nodenum];
		plane = node->plane;
//		s = BoxOnPlaneSide (leaf_mins, leaf_maxs, plane);
//		s = BOX_ON_PLANE_SIDE(*leaf_mins, *leaf_maxs, plane);
		s = BoxOnPlaneSide2( *leaf_mins, *leaf_maxs, plane );

		if (s == 1)
			nodenum = node->children[0];
		else if (s == 2)
			nodenum = node->children[1];
		else
		{
			// Split nodes must remain in the sub-tree
			parentNode = InsertNodeIntoTree( pBSPData, nodenum, parentNode, childNum );

			// go down both
			CM_BuildSubTree_r (pBSPData, node->children[0], parentNode, 0);

			childNum = 1;
			nodenum = node->children[1];
		}
	}
}

//-----------------------------------------------------------------------------
// This builds a subtree that lies within the bounding volume
//-----------------------------------------------------------------------------

void CM_BuildBSPSubTree( const Vector& mins, const Vector& maxs )
{
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	// We can't be using the subtree and building it also...
	Assert( pBSPData->map_rootnode == pBSPData->map_nodes.Base() );

	s_BSPSubTree.RemoveAll();

	leaf_mins = &mins;
	leaf_maxs = &maxs;

	CM_BuildSubTree_r( pBSPData, 0, -1, -1 );
}

//-----------------------------------------------------------------------------
// This here hooks in the subtree for all computations
//-----------------------------------------------------------------------------

void CM_BeginBSPSubTree( )
{
	// Hook in the subtree
	CCollisionBSPData *pBSPData = GetCollisionBSPData();
	pBSPData->map_rootnode = s_BSPSubTree.Base();
}

void CM_EndBSPSubTree( )
{
	// Unhook the subtree
	CCollisionBSPData *pBSPData = GetCollisionBSPData();
	pBSPData->map_rootnode = pBSPData->map_nodes.Base();
}


/*
==================
CM_PointContents

==================
*/
int CM_PointContents ( const Vector &p, int headnode)
{
	int		l;

	// get the current collision bsp -- there is only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	if (!pBSPData->numnodes)	// map not loaded
		return 0;

	l = CM_PointLeafnum_r (pBSPData, p, headnode);

	return pBSPData->map_leafs[l].contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int	CM_TransformedPointContents ( const Vector& p, int headnode, const Vector& origin, QAngle const& angles)
{
	Vector		p_l;
	Vector		temp;
	Vector		forward, right, up;
	int			l;

	MEASURECODE( "CM_TransformedPointContents" );

	// get the current collision bsp -- there is only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	// subtract origin offset
	VectorSubtract (p, origin, p_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode && 
	(angles[0] || angles[1] || angles[2]) )
	{
		AngleVectors (angles, &forward, &right, &up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	l = CM_PointLeafnum_r (pBSPData, p_l, headnode);

	return pBSPData->map_leafs[l].contents;
}


/*
===============================================================================

BOX TRACING

===============================================================================
*/

/*
================
CM_ClipBoxToBrush
================
*/
void FASTCALL CM_ClipBoxToBrush( CCollisionBSPData *pBSPData, const Vector& mins, const Vector& maxs, const Vector& p1, const Vector& p2,
										     trace_t *trace, cbrush_t *brush )
{
	
	if (!brush->numsides)
		return;

	g_CollisionCounts.m_BrushTraces++;

	float enterfrac = NEVER_UPDATED;
	float leavefrac = 1.f;
	cplane_t* clipplane = NULL;

	bool getout = false;
	bool startout = false;
	cbrushside_t* leadside = NULL;

	float dist;

	cbrushside_t *side = &pBSPData->map_brushsides[brush->firstbrushside];
	for (int i=0 ; i<brush->numsides ;i++, side++)
	{
		cplane_t *plane = side->plane;

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: special case for axial - NOTE: These axial planes are not always positive!!!
			// FIXME: use signbits into 8 way lookup for each mins/maxs

			// Compute the sign bits
			//unsigned nSigns  =  ( *(int*)(&plane->normal[0])) & 0x80000000;
			//		   nSigns |= (( *(int*)(&plane->normal[1]) & 0x80000000 ) >> 1);
			//         nSigns |= (( *(int*)(&plane->normal[2]) & 0x80000000 ) >> 2);

			
			Vector ofs;		
			ofs[0] = ( plane->normal[0] < 0.f ) ? maxs[0] : mins[0];
			ofs[1] = ( plane->normal[1] < 0.f ) ? maxs[1] : mins[1];
			ofs[2] = ( plane->normal[2] < 0.f ) ? maxs[2] : mins[2];

			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
		}
		else
		{
			// special point case
			dist = plane->dist;
			// don't trace rays against bevel planes 
			if( side->bBevel )
				continue;
		}

		float d1 = DotProduct (p1, plane->normal) - dist;
		float d2 = DotProduct (p2, plane->normal) - dist;

		// if completely in front of face, no intersection
		if( d1 > 0.f )
		{
			startout = true;

			// d1 > 0.f && d2 > 0.f
			if( d2 > 0.f )
				return;

		} else
		{
			// d1 <= 0.f && d2 <= 0.f
			if( d2 <= 0.f )
				continue;
 
			// d2 > 0.f
			getout = true;
		}

		// crosses face
		if (d1 > d2)
		{	// enter
			// JAY: This could be negative if d1 is less than the epsilon.
			// If the trace is short (d1-d2 is small) then it could produce a large
			// negative fraction.  I can't believe this didn't break Q2!
			float f = (d1-DIST_EPSILON);
			if ( f < 0.f )
				f = 0.f;
			f = f / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			float f = (d1+DIST_EPSILON) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	// when this happens, we entered the brush *after* leaving the previous brush.
	// Therefore, we're still outside!

	// NOTE: We only do this test against points because fractionleftsolid is
	// not possible to compute for brush sweeps without a *lot* more computation
	// So, client code will never get fractionleftsolid for box sweeps
	if (trace_ispoint && startout)
	{ 
		// Add a little sludge.  The sludge should already be in the fractionleftsolid
		// (for all intents and purposes is a leavefrac value) and enterfrac values.  
		// Both of these values have +/- DIST_EPSILON values calculated in.  Thus, I 
		// think the test should be against "0.0."  If we experience new "left solid"
		// problems you may want to take a closer look here!
//		if ((trace->fractionleftsolid - enterfrac) > -1e-6)
		if ((trace->fractionleftsolid - enterfrac) > 0.0f )
			startout = false;
	}

	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		// return starting contents
		trace->contents = brush->contents;

		if (!getout)
		{
			trace->allsolid = true;
			trace->fraction = 0.0f;
			trace->fractionleftsolid = 1.0f;
		}
		else
		{
			// if leavefrac == 1, this means it's never been updated or we're in allsolid
			// the allsolid case was handled above
			if ((leavefrac != 1) && (leavefrac > trace->fractionleftsolid))
			{
				trace->fractionleftsolid = leavefrac;

				// This could occur if a previous trace didn't start us in solid
				if (trace->fraction <= leavefrac)
				{
					trace->fraction = 1.0f;
					trace->surface = nullsurface;
				}
			}
		}
		return;
	}

	// We haven't hit anything at all until we've left...
	if (enterfrac < leavefrac)
	{
		if (enterfrac > NEVER_UPDATED && enterfrac < trace->fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace->fraction = enterfrac;
			trace_bDispHit = false;
			trace->plane = *clipplane;
			trace->surface = *leadside->surface;
			trace->contents = brush->contents;
		}
	}
}

/*
================
CM_TestBoxInBrush
================
*/
void CM_TestBoxInBrush( CCollisionBSPData *pBSPData, const Vector& mins, const Vector& maxs, const Vector& p1,
					    trace_t *trace, cbrush_t *brush )
{
	int			i, j;
	cplane_t	*plane;
	float		dist;
	Vector		ofs(0,0,0);
	float		d1;
	cbrushside_t	*side;

	if (!brush->numsides)
		return;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &pBSPData->map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}
		dist = DotProduct (ofs, plane->normal);
		dist = plane->dist - dist;

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;

	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->fractionleftsolid = 1.0f;
	trace->contents = brush->contents;
}


/*
================
CM_TraceToLeaf
================
*/
void FASTCALL CM_TraceToLeaf( CCollisionBSPData *pBSPData, int ndxLeaf, float startFrac, float endFrac )
{
	int nCurrentCheckCount = CurrentCheckCount();
	int nDepth = CurrentCheckCountDepth();

	// get the leaf
	cleaf_t *pLeaf = &pBSPData->map_leafs[ndxLeaf];

	//
	// trace ray/box sweep against all brushes in this leaf
	//
	for( int ndxLeafBrush = 0; ndxLeafBrush < pLeaf->numleafbrushes; ndxLeafBrush++ )
	{
		// get the current brush
		int ndxBrush = pBSPData->map_leafbrushes[pLeaf->firstleafbrush+ndxLeafBrush];
		cbrush_t *pBrush = &pBSPData->map_brushes[ndxBrush];

		// make sure we only check this brush once per trace/stab
		if( pBrush->checkcount[nDepth] == nCurrentCheckCount )
			continue;

		// mark the brush as checked
		pBrush->checkcount[nDepth] = nCurrentCheckCount;

		// only collide with objects you are interested in
		if( !( pBrush->contents & trace_contents ) )
			continue;

		// trace against the brush and find impact point -- if any?
		// NOTE: trace_trace.fraction == 0.0f only when trace starts inside of a brush!
		CM_ClipBoxToBrush( pBSPData, trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, pBrush );
		if( !trace_trace.fraction )
			return;
	}

	Assert( nDepth == CurrentCheckCountDepth() );
	Assert( nCurrentCheckCount == CurrentCheckCount() );

	// TODO: this may be redundant
	if( trace_trace.startsolid )
		return;

	// Collide (test) against displacement surfaces in this leaf.
	if( pLeaf->m_pDisplacements )
	{		
		//
		// trace ray/swept box against all displacement surfaces in this leaf
		//
		for( CDispIterator it( pLeaf->m_pDisplacements, CDispLeafLink::LIST_LEAF ); it.IsValid(); )
		{
			CDispCollTree *pDispTree = static_cast<CDispCollTree*>( it.Inc()->m_pDispInfo );
			
			// make sure we only check this brush once per trace/stab
			if( pDispTree->GetCheckCount(nDepth) == nCurrentCheckCount )
				continue;
			
			// mark the brush as checked
			if( !trace_ispoint )
			{
				pDispTree->SetCheckCount( nDepth, nCurrentCheckCount );
			}
			
			// only collide with objects you are interested in
			if( !( pDispTree->GetContents() & trace_contents ) )
				continue;

			CM_TraceToDispTree( pDispTree, trace_start, trace_end, trace_mins, trace_maxs, 
				                startFrac, endFrac, &trace_trace, ( trace_ispoint == 1 ) );
			if( !trace_trace.fraction )
				break;
		}
		
		CM_PostTraceToDispTree();
	}

	Assert( nDepth == CurrentCheckCountDepth() );
	Assert( nCurrentCheckCount == CurrentCheckCount() );
}


/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf( CCollisionBSPData *pBSPData, int ndxLeaf )
{
	int nCurrentCheckCount = CurrentCheckCount();
	int nDepth = CurrentCheckCountDepth();

	// get the leaf
	cleaf_t *pLeaf = &pBSPData->map_leafs[ndxLeaf];

	//
	// trace ray/box sweep against all brushes in this leaf
	//
	for( int ndxLeafBrush = 0; ndxLeafBrush < pLeaf->numleafbrushes; ndxLeafBrush++ )
	{
		// get the current brush
		int ndxBrush = pBSPData->map_leafbrushes[pLeaf->firstleafbrush+ndxLeafBrush];
		cbrush_t *pBrush = &pBSPData->map_brushes[ndxBrush];

		// make sure we only check this brush once per trace/stab
		if( pBrush->checkcount[nDepth] == nCurrentCheckCount )
			continue;

		// mark the brush as checked
		pBrush->checkcount[nDepth] = nCurrentCheckCount;

		// only collide with objects you are interested in
		if( !( pBrush->contents & trace_contents ) )
			continue;

		//
		// test to see if the point/box is inside of any solid
		// NOTE: trace_trace.fraction == 0.0f only when trace starts inside of a brush!
		//
		CM_TestBoxInBrush( pBSPData, trace_mins, trace_maxs, trace_start, &trace_trace, pBrush );
		if( !trace_trace.fraction )
			return;
	}

	Assert( nDepth == CurrentCheckCountDepth() );
	Assert( nCurrentCheckCount == CurrentCheckCount() );

	// TODO: this may be redundant
	if( trace_trace.startsolid )
		return;

	// if there are no displacement surfaces in this leaf -- we are done testing
	if( pLeaf->m_pDisplacements )
	{
		// test to see if the point/box is inside of any of the displacement surface
		CM_TestInDispTree( pBSPData, pLeaf, trace_start, trace_mins, trace_maxs, trace_contents, &trace_trace );
	}

	Assert( nDepth == CurrentCheckCountDepth() );
	Assert( nCurrentCheckCount == CurrentCheckCount() );
}


/*
==================
CM_RecursiveHullCheck

==================
Attempt to do whatever is nessecary to get this function to unroll at least once
*/
void FASTCALL CM_RecursiveHullCheck ( CCollisionBSPData *pBSPData,
	int num, float p1f, float p2f, const Vector& p1, const Vector& p2)
{
	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	cnode_t		*node = NULL;
	cplane_t	*plane;
	float		t1 = 0, t2 = 0, offset = 0;
	float		frac, frac2;
	float		idist;
	Vector		mid;
	int			side;
	float		midf;


	// find the point distances to the seperating plane
	// and the offset for the size of the box

	// NJS: Hoisted loop invariant comparison to trace_ispoint

	if( trace_ispoint )
	{
		while( num >= 0 )
		{
			node = pBSPData->map_rootnode + num;
			plane = node->plane;

			if (plane->type < 3)
			{
				t1 = p1[plane->type] - plane->dist;
				t2 = p2[plane->type] - plane->dist;
				offset = trace_extents[plane->type];
			}
			else
			{
				t1 = DotProduct (plane->normal, p1) - plane->dist;
				t2 = DotProduct (plane->normal, p2) - plane->dist;
				offset = 0;
			}

			// see which sides we need to consider
			if (t1 > offset && t2 > offset )
	//		if (t1 >= offset && t2 >= offset)
			{
				num = node->children[0];
				continue;
			}
			if (t1 < -offset && t2 < -offset)
			{
				num = node->children[1];
				continue;
			}
			break;
		}
	} else
	{
		while( num >= 0 )
		{
			node = pBSPData->map_rootnode + num;
			plane = node->plane;

			if (plane->type < 3)
			{
				t1 = p1[plane->type] - plane->dist;
				t2 = p2[plane->type] - plane->dist;
				offset = trace_extents[plane->type];
			}
			else
			{
				t1 = DotProduct (plane->normal, p1) - plane->dist;
				t2 = DotProduct (plane->normal, p2) - plane->dist;
				offset = fabs(trace_extents[0]*plane->normal[0]) +
						 fabs(trace_extents[1]*plane->normal[1]) +
						 fabs(trace_extents[2]*plane->normal[2]);
			}

			// see which sides we need to consider
			if (t1 > offset && t2 > offset )
	//		if (t1 >= offset && t2 >= offset)
			{
				num = node->children[0];
				continue;
			}
			if (t1 < -offset && t2 < -offset)
			{
				num = node->children[1];
				continue;
			}
			break;
		}

	}

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToLeaf (pBSPData, -1-num, p1f, p2f);
		return;
	}
	
	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset - DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	frac = clamp( frac, 0, 1 );
	midf = p1f + (p2f - p1f)*frac;
	VectorLerp( p1, p2, frac, mid );

	CM_RecursiveHullCheck (pBSPData, node->children[side], p1f, midf, p1, mid);

	// go past the node
	frac2 = clamp( frac2, 0, 1 );
	midf = p1f + (p2f - p1f)*frac2;
	VectorLerp( p1, p2, frac2, mid );

	CM_RecursiveHullCheck (pBSPData, node->children[side^1], midf, p2f, mid, p2);
}

void CM_ClearTrace( trace_t *trace )
{
	memset( trace, 0, sizeof(*trace));
	trace->fraction = 1.f;
	trace->fractionleftsolid = 0;
	trace->surface = nullsurface;
}


//-----------------------------------------------------------------------------
//
// The following versions use ray... gradually I'm gonna remove other versions
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Computes the ray endpoints given a trace.
//-----------------------------------------------------------------------------
static inline void CM_ComputeTraceEndpoints( const Ray_t& ray, trace_t& tr )
{
	// The ray start is the center of the extents; compute the actual start
	Vector start;
	VectorAdd( ray.m_Start, ray.m_StartOffset, start );

	if (trace_trace.fraction == 1)
		VectorAdd(start, ray.m_Delta, tr.endpos);
	else
		VectorMA( start, tr.fraction, ray.m_Delta, tr.endpos );

	if (trace_trace.fractionleftsolid == 0)
	{
		VectorCopy (start, tr.startpos);
	}
	else
	{
		if (tr.fractionleftsolid == 1.0f)
		{
			tr.startsolid = tr.allsolid = 1;
			tr.fraction = 0.0f;
			VectorCopy( start, tr.endpos );
		}

		VectorMA( start, tr.fractionleftsolid, ray.m_Delta, tr.startpos );
	}
}

//-----------------------------------------------------------------------------
// Test an unswept box
//-----------------------------------------------------------------------------

static inline void CM_UnsweptBoxTrace( CCollisionBSPData *pBSPData, 
								const Ray_t& ray, int headnode, int brushmask )
{
	int		leafs[1024];
	int		i, numleafs;
	Vector	boxMins, boxMaxs;
	int		topnode;

	VectorAdd (ray.m_Start, ray.m_Extents, boxMaxs);
	VectorSubtract (ray.m_Start, ray.m_Extents, boxMins);

	// Bloat the box a little
	for (i=0 ; i<3 ; i++)
	{
		boxMins[i] -= 1;
		boxMaxs[i] += 1;
	}

	bool bFoundNonSolidLeaf = false;
	numleafs = CM_BoxLeafnums_headnode ( pBSPData, boxMins, boxMaxs, leafs, 1024, headnode, &topnode);
	for (i=0 ; i<numleafs ; i++)
	{
		if ((pBSPData->map_leafs[leafs[i]].contents & CONTENTS_SOLID) == 0)
		{
			bFoundNonSolidLeaf = true;
		}

		CM_TestInLeaf ( pBSPData, leafs[i] );
		if (trace_trace.allsolid)
			break;
	}

	if (!bFoundNonSolidLeaf)
	{
		trace_trace.allsolid = trace_trace.startsolid = 1;
		trace_trace.fraction = 0.0f;
		trace_trace.fractionleftsolid = 1.0f;
	}
}

void CM_BoxTrace( const Ray_t& ray, int headnode, int brushmask, bool computeEndpt, trace_t& tr )
{
	g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_BOX_TRACES, 1 );
	MEASURE_TIMED_STAT( ENGINE_STATS_BOX_TRACE_TIME );
	
	// for multi-check avoidance
	BeginCheckCount();		

	// for statistics, may be zeroed
	g_CollisionCounts.m_Traces++;		

	// fill in a default trace
	CM_ClearTrace( &trace_trace );

	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	// check if the map is not loaded
	if (!pBSPData->numnodes)	
	{
		tr = trace_trace;
		EndCheckCount();
		return;
	}

	trace_bDispHit = false;
	trace_StabDir.Init();
	trace_contents = brushmask;
	VectorCopy (ray.m_Start, trace_start);
	VectorAdd  (ray.m_Start, ray.m_Delta, trace_end);
	VectorMultiply (ray.m_Extents, -1.0f, trace_mins);
	VectorCopy (ray.m_Extents, trace_maxs);
	VectorCopy (ray.m_Extents, trace_extents);
	trace_ispoint = ray.m_IsRay;


	if (!ray.m_IsSwept)
	{
		// check for position test special case
		CM_UnsweptBoxTrace( pBSPData, ray, headnode, brushmask );
	}
	else
	{
		// general sweeping through world
		CM_RecursiveHullCheck( pBSPData, headnode, 0, 1, trace_start, trace_end );
	}
	// Compute the trace start + end points
	if (computeEndpt)
	{
		CM_ComputeTraceEndpoints( ray, trace_trace );
	}

	// Copy off the results
	tr = trace_trace;
	EndCheckCount();
	Assert( !ray.m_IsRay || tr.allsolid || (tr.fraction >= tr.fractionleftsolid) );
}


void CM_TransformedBoxTrace( const Ray_t& ray, int headnode, int brushmask,
							const Vector& origin, QAngle const& angles, trace_t& tr )
{
	matrix3x4_t	localToWorld;
	Ray_t ray_l;

	MEASURECODE( "CM_TransformedBoxTrace" );

	// subtract origin offset
	VectorCopy( ray.m_StartOffset, ray_l.m_StartOffset );
	VectorCopy( ray.m_Extents, ray_l.m_Extents );

	// Are we rotated?
	bool rotated = (headnode != box_headnode) && (angles[0] || angles[1] || angles[2]);

	// rotate start and end into the models frame of reference
	if (rotated)
	{
		// NOTE: In this case, the bbox is rotated into the space of the BSP as well
		// to insure consistency at all orientations, we must rotate the origin of the ray
		// and reapply the offset to the center of the box.  That way all traces with the 
		// same box centering will have the same transformation into local space
		Vector worldOrigin = ray.m_Start + ray.m_StartOffset;
		AngleMatrix( angles, origin, localToWorld );
		VectorIRotate( ray.m_Delta, localToWorld, ray_l.m_Delta );
		VectorITransform( worldOrigin, localToWorld, ray_l.m_Start );
		ray_l.m_Start -= ray.m_StartOffset;
	}
	else
	{
		VectorSubtract( ray.m_Start, origin, ray_l.m_Start );
		VectorCopy( ray.m_Delta, ray_l.m_Delta );
	}

	ray_l.m_IsRay = ray.m_IsRay;
	ray_l.m_IsSwept = ray.m_IsSwept;

	// sweep the box through the model, don't compute endpoints
	CM_BoxTrace( ray_l, headnode, brushmask, false, tr );

	// If we hit, gotta fix up the normal...
	if (( tr.fraction != 1 ) && rotated )
	{
		// transform the normal from the local space of this entity to world space
		Vector temp;
		VectorCopy (tr.plane.normal, temp);
		VectorRotate( temp, localToWorld, tr.plane.normal );
	}

	CM_ComputeTraceEndpoints( ray, tr );
}

int CM_TransformedBoxContents( const Vector& pos, const Vector& mins, const Vector& maxs, int headnode, const Vector& origin, QAngle const& angles )
{
	Ray_t ray;
	ray.Init( pos, pos, mins, maxs );

	trace_t trace;
	CM_TransformedBoxTrace( ray, headnode, MASK_ALL, origin, angles, trace );
	return trace.contents;
}

/*
===============================================================================

PVS / PAS

===============================================================================
*/

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pBSPData - 
//			*out - 
//-----------------------------------------------------------------------------
void CM_NullVis( CCollisionBSPData *pBSPData, byte *out )
{
	int numClusterBytes = (pBSPData->numclusters+7)>>3;	
	byte *out_p = out;

	while (numClusterBytes)
	{
		*out_p++ = 0xff;
		numClusterBytes--;
	}
}

/*
===================
CM_DecompressVis
===================
*/
void CM_DecompressVis( CCollisionBSPData *pBSPData, int cluster, int visType, byte *out )
{
	int		c;
	byte	*out_p;
	int		numClusterBytes;
	
	if ( !pBSPData )
	{
#ifdef _WIN32
		__asm int 3
#elif _LINUX
		__asm("int $3");
#endif
	}

	if ( cluster > pBSPData->numclusters || cluster < 0 )
	{
#ifdef _WIN32
		__asm int 3
#elif _LINUX
		__asm("int $3");
#endif
		//AssertMsg(0, "Bad vis cluster\n");

		CM_NullVis( pBSPData, out );
		return;
	}

	// no vis info, so make all visible
	if ( !pBSPData->numvisibility || !pBSPData->map_vis )
	{	
		CM_NullVis( pBSPData, out );
		return;		
	}

	byte *in = ((byte *)pBSPData->map_vis) + pBSPData->map_vis->bitofs[cluster][visType];
	numClusterBytes = (pBSPData->numclusters+7)>>3;	
	out_p = out;

	// no vis info, so make all visible
	if ( !in )
	{	
		CM_NullVis( pBSPData, out );
		return;		
	}

	do
	{
		if (*in)
		{
			*out_p++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((out_p - out) + c > numClusterBytes)
		{
			c = numClusterBytes - (out_p - out);
			Con_Printf( "warning: Vis decompression overrun\n" );
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < numClusterBytes);
}

//-----------------------------------------------------------------------------
// Purpose: Decompress the RLE bitstring for PVS or PAS of one cluster
// Input  : *dest - buffer to store the decompressed data
//			cluster - index of cluster of interest
//			visType - DVIS_PAS or DVIS_PAS
// Output : byte * - pointer to the filled buffer
//-----------------------------------------------------------------------------
byte *CM_Vis( byte *dest, int cluster, int visType )
{
	// get the current collision bsp -- there is only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	if ( !dest || visType > 2 || visType < 0 )
	{
		Sys_Error( "CM_Vis: error");
		return NULL;
	}

	if ( cluster == -1 )
	{
		memset( dest, 0, (pBSPData->numclusters+7)>>3 );
	}
	else
	{
		CM_DecompressVis( pBSPData, cluster, visType, dest );
	}

	return dest;
}

static byte	pvsrow[MAX_MAP_LEAFS/8];
static byte	pasrow[MAX_MAP_LEAFS/8];

byte	*CM_ClusterPVS (int cluster)
{
	return CM_Vis( pvsrow, cluster, DVIS_PVS );
}

byte	*CM_ClusterPAS (int cluster)
{
	return CM_Vis( pasrow, cluster, DVIS_PAS );
}


/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void FloodArea_r (CCollisionBSPData *pBSPData, carea_t *area, int floodnum)
{
	int		i;
	dareaportal_t	*p;

	if (area->floodvalid == pBSPData->floodvalid)
	{
		if (area->floodnum == floodnum)
			return;
		Sys_Error( "FloodArea_r: reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = pBSPData->floodvalid;
	p = &pBSPData->map_areaportals[area->firstareaportal];
	for (i=0 ; i<area->numareaportals ; i++, p++)
	{
		if (pBSPData->portalopen[p->m_PortalKey])
		{
			FloodArea_r (pBSPData, &pBSPData->map_areas[p->otherarea], floodnum);
		}
	}
}

/*
====================
FloodAreaConnections


====================
*/
void	FloodAreaConnections ( CCollisionBSPData *pBSPData )
{
	int		i;
	carea_t	*area;
	int		floodnum;

	// all current floods are now invalid
	pBSPData->floodvalid++;
	floodnum = 0;

	// area 0 is not used
	for (i=1 ; i<pBSPData->numareas ; i++)
	{
		area = &pBSPData->map_areas[i];
		if (area->floodvalid == pBSPData->floodvalid)
			continue;		// already flooded into
		floodnum++;
		FloodArea_r (pBSPData, area, floodnum);
	}

}

void CM_SetAreaPortalState( int portalnum, int isOpen )
{
	// get the current collision bsp -- there is only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	// Portalnums in the BSP file are 1-based instead of 0-based
	if (portalnum > pBSPData->numareaportals)
	{
		Sys_Error( "portalnum > numareaportals");
	}

	pBSPData->portalopen[portalnum] = isOpen;
	FloodAreaConnections (pBSPData);
}

qboolean	CM_AreasConnected (int area1, int area2)
{
	// get the current collision bsp -- there is only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	if (map_noareas.GetInt())
		return true;

	if (area1 >= pBSPData->numareas || area2 >= pBSPData->numareas)
	{
		Sys_Error( "area >= numareas");
	}

	return (pBSPData->map_areas[area1].floodnum == pBSPData->map_areas[area2].floodnum);
}


/*
=================
CM_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_WriteAreaBits (byte *buffer, int area)
{
	int		i;
	int		floodnum;
	int		bytes;

	// get the current collision bsp -- there is only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	bytes = (pBSPData->numareas+7)>>3;

	if ( map_noareas.GetInt() )
	{	// for debugging, send everything
		memset (buffer, 255, 3);
	}
	else
	{
		memset (buffer, 0, 32);

		floodnum = pBSPData->map_areas[area].floodnum;
		for (i=0 ; i<pBSPData->numareas ; i++)
		{
			if (pBSPData->map_areas[i].floodnum == floodnum || !area)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}


bool CM_GetAreaPortalPlane( const Vector &vViewOrigin, int portalKey, VPlane *pPlane )
{
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	// First, find the leaf and area the viewer is in.
	int iLeaf = CM_PointLeafnum( vViewOrigin );
	if( iLeaf < 0 || iLeaf >= pBSPData->numleafs )
		return false;
	
	int iArea = pBSPData->map_leafs[iLeaf].area;
	if( iArea < 0 || iArea >= pBSPData->numareas )
		return false;

	carea_t *pArea = &pBSPData->map_areas[iArea];
	for( int i=0; i < pArea->numareaportals; i++ )
	{
		dareaportal_t *pPortal = &pBSPData->map_areaportals[pArea->firstareaportal + i];

		if( pPortal->m_PortalKey == portalKey )
		{
			cplane_t *pMapPlane = &pBSPData->map_planes[pPortal->planenum];
			pPlane->m_Normal = pMapPlane->normal;
			pPlane->m_Dist = pMapPlane->dist;
			return true;
		}
	}

	return false;
}


/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
qboolean CM_HeadnodeVisible (int nodenum, byte *visbits)
{
	int		leafnum;
	int		cluster;
	cnode_t	*node;

	// get the current collision bsp -- there is only one!
	CCollisionBSPData *pBSPData = GetCollisionBSPData();

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = pBSPData->map_leafs[leafnum].cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &pBSPData->map_rootnode[nodenum];
	if (CM_HeadnodeVisible(node->children[0], visbits))
		return true;
	return CM_HeadnodeVisible(node->children[1], visbits);
}



//-----------------------------------------------------------------------------
// Purpose: returns true if the box is in a cluster that is visible in the visbits
// Input  : mins - box extents
//			maxs - 
//			*visbits - pvs or pas of some cluster
// Output : true if visible, false if not
//-----------------------------------------------------------------------------
#define MAX_BOX_LEAVES	256
int	CM_BoxVisible( const Vector& mins, const Vector& maxs, byte *visbits )
{
	int leafList[MAX_BOX_LEAVES];
	int topnode;

	g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_BOX_VISIBLE_TESTS, 1 );
	MEASURE_TIMED_STAT( ENGINE_STATS_BOX_VISIBLE_TEST_TIME );
	
	// FIXME: Could save a loop here by traversing the tree in this routine like the code above
	int count = CM_BoxLeafnums( mins, maxs, leafList, MAX_BOX_LEAVES, &topnode );
	for ( int i = 0; i < count; i++ )
	{
		int cluster = CM_LeafCluster( leafList[i] );

		if (visbits[cluster>>3] & (1<<(cluster&7)))
		{
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Returns the world-space center of an entity
//-----------------------------------------------------------------------------
void CM_WorldSpaceCenter( ICollideable *pCollideable, Vector *pCenter )
{
	// FIXME: Sucky! Is there a better way of doing this?
	if ( !pCollideable->IsBoundsDefinedInEntitySpace() )
	{
		Vector vecMins = pCollideable->WorldAlignMins();
		Vector vecMaxs = pCollideable->WorldAlignMaxs();
		VectorAdd( vecMins, vecMaxs, *pCenter );
		*pCenter *= 0.5f;
		*pCenter += pCollideable->GetCollisionOrigin();
	}
	else
	{
		if ( pCollideable->GetCollisionAngles() == vec3_angle )
		{
			Vector vecMins = pCollideable->EntitySpaceMins();
			Vector vecMaxs = pCollideable->EntitySpaceMaxs();
			VectorAdd( vecMins, vecMaxs, *pCenter );
			*pCenter *= 0.5f;
			*pCenter += pCollideable->GetCollisionOrigin();
		}
		else
		{
			Vector vecMins = pCollideable->EntitySpaceMins();
			Vector vecMaxs = pCollideable->EntitySpaceMaxs();
			Vector vecLocalCenter;
			VectorAdd( vecMins, vecMaxs, vecLocalCenter );
			vecLocalCenter *= 0.5f;
			if ( vecLocalCenter != vec3_origin )
			{
				matrix3x4_t entityToWorld;
				AngleMatrix( pCollideable->GetCollisionAngles(), pCollideable->GetCollisionOrigin(), entityToWorld );
				VectorTransform( vecLocalCenter, entityToWorld, *pCenter );
			}
			else
			{
				*pCenter = pCollideable->GetCollisionOrigin();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Returns the world-space center of an entity
//-----------------------------------------------------------------------------
void CM_WorldAlignBounds( ICollideable *pCollideable, Vector *pMins, Vector *pMaxs )
{
	Vector vecWorldMins, vecWorldMaxs;
	if ( !pCollideable->IsBoundsDefinedInEntitySpace() )
	{
		vecWorldMins = pCollideable->WorldAlignMins();
		vecWorldMaxs = pCollideable->WorldAlignMaxs();
	}
	else
	{
		// NOTE: This is a *conservative* test; the trigger will hit sometimes even
		// if we didn't quite touch it. We gotta do this because CM_TransformedBoxContents
		// expects the swept box to be defined in world-aligned space
		if ( pCollideable->GetCollisionAngles() == vec3_angle )
		{
			vecWorldMins = pCollideable->EntitySpaceMins();
			vecWorldMaxs = pCollideable->EntitySpaceMaxs();
		}
		else
		{
			matrix3x4_t worldToEntity;
			AngleIMatrix( pCollideable->GetCollisionAngles(), pCollideable->GetCollisionOrigin(), worldToEntity );

			ITransformAABB( worldToEntity, pCollideable->EntitySpaceMins(), pCollideable->EntitySpaceMaxs(), vecWorldMins, vecWorldMaxs ); 
			vecWorldMins -= pCollideable->GetCollisionOrigin();
			vecWorldMaxs -= pCollideable->GetCollisionOrigin();
		}
	}
}

