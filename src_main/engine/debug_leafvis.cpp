#include "glquake.h"
#include "const.h"
#include "cmodel.h"
#include "gl_model_private.h"
#include "convar.h"
#include "gl_cvars.h"
#include "gl_matsysiface.h"
#include "materialsystem/imesh.h"
#include "gl_rmain.h"

// Leaf visualization routines
#define MAX_LEAF_PPLANES	64
#define MAX_LEAF_PVERTS		32
#define MAX_TOTAL_PVERTS	(MAX_LEAF_PPLANES * MAX_LEAF_PVERTS)

typedef struct leafvis_s
{
	Vector	verts[ MAX_TOTAL_PVERTS ];
	int		polyVertCount[ MAX_LEAF_PPLANES ];
	int		polyCount;
} leafvis_t;

// Only allocate this after it is turned on
leafvis_t *g_LeafVis = NULL;

static void AddPlaneToList( cplane_t *list, int index, Vector& normal, float dist, int invert )
{
	list += index;

	if ( invert )
	{
		VectorScale( normal, -1, list->normal );
		list->dist = -dist;
	}
	else
	{
		VectorCopy( normal, list->normal );
		list->dist = dist;
	}
}

static int PlaneList( Vector& p, model_t *model, cplane_t *planeList, int maxPlanes )
{
	int			planeIndex = 0;
	mnode_t		*node;
	float		d;
	cplane_t	*plane, *outPlane;
	
	if (!model || !model->brush.nodes)
		Sys_Error ("PlaneList: bad model");

	node = model->brush.nodes;
	while (1)
	{
		if (node->contents >= 0)
			return planeIndex;

		plane = node->plane;
		if ( plane->type <= PLANE_Z )
		{
			d = p[plane->type] - plane->dist;
		}
		else
		{
			d = DotProduct (p,plane->normal) - plane->dist;
		}
		outPlane = planeList + planeIndex;

		if (d > 0)
		{
			// Add this plane, it already points toward the vert
			AddPlaneToList( planeList, planeIndex, plane->normal, plane->dist, false );
			node = node->children[0];
		}
		else
		{
			// Add this plane, reverse direction to point toward the vert
			AddPlaneToList( planeList, planeIndex, plane->normal, plane->dist, true );
			node = node->children[1];
		}
		
		planeIndex ++;

		if ( planeIndex >= maxPlanes )
			return planeIndex;
	}
	
	return planeIndex;
}


//-----------------------------------------------------------------------------
// Purpose: This is a clone of BaseWindingForPlane()
// Input  : *outVerts - an array of preallocated verts to build the polygon in
//			normal - the plane normal
//			dist - the plane constant
// Output : int - vert count (always 4)
//-----------------------------------------------------------------------------
static int PolyFromPlane( Vector *outVerts, Vector& normal, float dist )
{
	int		i, x;
	vec_t	max, v;
	Vector	org, vright, vup;
	
// find the major axis

	max = -MAX_COORD_INTEGER;
	x = -1;
	for (i=0 ; i<3; i++)
	{
		v = fabs(normal[i]);
		if (v > max)
		{
			x = i;
			max = v;
		}
	}

	if (x==-1)
		return 0;

	// Build a unit vector along something other than the major axis
	VectorCopy (vec3_origin, vup);	
	switch (x)
	{
	case 0:
	case 1:
		vup[2] = 1;
		break;		
	case 2:
		vup[0] = 1;
		break;		
	}

	// Remove the component of this vector along the normal
	v = DotProduct (vup, normal);
	VectorMA (vup, -v, normal, vup);
	// Make it a unit (perpendicular)
	VectorNormalize (vup);
	
	// Center of the poly is at normal * dist
	VectorScale (normal, dist, org);
	// Calculate the third orthonormal basis vector for our plane space (this one and vup are in the plane)
	CrossProduct (vup, normal, vright);
	
	// Make the plane's basis vectors big (these are the half-sides of the polygon we're making)
	VectorScale (vup, 9000, vup);
	VectorScale (vright, 9000, vright);

	// Move diagonally away from org to create the corner verts
	VectorSubtract (org, vright, outVerts[0]);	// left
	VectorAdd (outVerts[0], vup, outVerts[0]);	// up
	
	VectorAdd (org, vright, outVerts[1]);		// right
	VectorAdd (outVerts[1], vup, outVerts[1]);	// up
	
	VectorAdd (org, vright, outVerts[2]);		// right
	VectorSubtract (outVerts[2], vup, outVerts[2]);	// down
	
	VectorSubtract (org, vright, outVerts[3]);		// left
	VectorSubtract (outVerts[3], vup, outVerts[3]);	// down

	// The four corners form a planar quadrilateral normal to "normal"
	return 4;
}


//-----------------------------------------------------------------------------
// Purpose: clip a poly to the plane and return the poly on the front side of the plane
// Input  : *inVerts - input polygon
//			vertCount - # verts in input poly
//			*outVerts - destination poly
//			normal - plane normal
//			dist - plane constant
// Output : int - # verts in output poly
//-----------------------------------------------------------------------------

#pragma warning (disable:4701)

static int ClipPolyToPlane( Vector *inVerts, int vertCount, Vector *outVerts, Vector& normal, float dist )
{
	vec_t	dists[MAX_LEAF_PVERTS+4];
	int		sides[MAX_LEAF_PVERTS+4];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	Vector	mid;
	int		outCount;

	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for ( i = 0; i < vertCount; i++ )
	{
		dot = DotProduct( inVerts[i], normal) - dist;
		dists[i] = dot;
		if ( dot > ON_EPSILON )
		{
			sides[i] = SIDE_FRONT;
		}
		else if ( dot < -ON_EPSILON )
		{
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	if (!counts[0])
		return 0;
	
	if (!counts[1])
	{
		// Copy to output verts
		for ( i = 0; i < vertCount; i++ )
		{
			VectorCopy( inVerts[i], outVerts[i] );
		}
		return vertCount;
	}

	outCount = 0;
	for ( i = 0; i < vertCount; i++ )
	{
		Vector& p1 = inVerts[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy( p1, outVerts[outCount]);
			outCount++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy( p1, outVerts[outCount]);
			outCount++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
	// generate a split point
		Vector& p2 = inVerts[(i+1)%vertCount];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (normal[j] == 1)
				mid[j] = dist;
			else if (normal[j] == -1)
				mid[j] = -dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, outVerts[outCount]);
		outCount++;
	}
	
	if (outCount > MAX_LEAF_PVERTS)
	{
		Con_DPrintf("Too many points!!!\n");
		return 0;
	}

	return outCount;
}

#pragma warning (default:4701)


void CSGPlaneList( leafvis_t *pVis, cplane_t *planeList, int planeCount )
{
	int totalVerts = 0;
	Vector	vertsIn[MAX_LEAF_PVERTS], vertsOut[MAX_LEAF_PVERTS];

	pVis->polyCount = 0;
	// Build the CSG solid of this leaf given that the planes in the list define a convex solid
	for ( int i = 0; i < planeCount; i++ )
	{
		// Build a big-ass poly in this plane
		int vertCount = PolyFromPlane( vertsIn, planeList[i].normal, planeList[i].dist );	// BaseWindingForPlane()
		
		// Now chop it by every other plane
		int j;
		for ( j = 0; j < planeCount; j++ )
		{
			// don't clip planes with themselves
			if ( i == j )
				continue;

			// Less than a poly left, something's wrong, don't bother with this polygon
			if ( vertCount < 3 )
				continue;

			// Chop the polygon against this plane
			vertCount = ClipPolyToPlane( vertsIn, vertCount, vertsOut, planeList[j].normal, planeList[j].dist );
			
			// Just copy the verts each time, don't bother swapping pointers (efficiency is not a goal here)
			for ( int k = 0; k < vertCount; k++ )
			{
				VectorCopy( vertsOut[k], vertsIn[k] );
			}
		}

		// We've got a polygon here
		if ( vertCount >= 3 )
		{
			if ( (totalVerts + vertCount) <= MAX_TOTAL_PVERTS && pVis->polyCount <= MAX_LEAF_PPLANES )
			{
				// Copy polygon out
				pVis->polyVertCount[ pVis->polyCount ] = vertCount;
				for ( j = 0; j < vertCount; j++ )
				{
					pVis->verts[ totalVerts ][0] = vertsIn[j][0];
					pVis->verts[ totalVerts ][1] = vertsIn[j][1];
					pVis->verts[ totalVerts ][2] = vertsIn[j][2];
					totalVerts++;
				}
				pVis->polyCount++;
			}
			else
			{
				Con_DPrintf("too many verts or polys\n" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Builds a convex polyhedron of the leaf boundary around p
// Input  : p - point to classify determining the leaf
//-----------------------------------------------------------------------------
void LeafVisBuild( Vector &p )
{
	int planeCount;
	cplane_t planeList[MAX_LEAF_PPLANES];
	Vector	normal;

	static int last_matleafvis = 0;
	static int last_visframe = -1;

	int nowLeafvis = mat_leafvis.GetInt();
	if ( !nowLeafvis )
		return;

	if ( nowLeafvis == last_matleafvis && last_visframe == r_visframecount )
		return;

	last_matleafvis = nowLeafvis;
	last_visframe = r_visframecount;

	if ( !g_LeafVis )
	{
		g_LeafVis = new leafvis_t;
	}
	// Build a list of inward pointing planes of the tree descending to this
	planeCount = PlaneList( p, host_state.worldmodel, planeList, 60 );
	
	VectorCopy( vec3_origin, normal );
	
	// Add world bounding box planes in case the world isn't closed
	// x-axis
	normal[0] = 1;
	AddPlaneToList( planeList, planeCount, normal, MAX_COORD_INTEGER, true );
	planeCount++;
	AddPlaneToList( planeList, planeCount, normal, -MAX_COORD_INTEGER, false );
	planeCount++;
	normal[0] = 0;
	
	// y-axis
	normal[1] = 1;
	AddPlaneToList( planeList, planeCount, normal, MAX_COORD_INTEGER, true );
	planeCount++;
	AddPlaneToList( planeList, planeCount, normal, -MAX_COORD_INTEGER, false );
	planeCount++;
	normal[1] = 0;
	
	// z-axis
	normal[2] = 1;
	AddPlaneToList( planeList, planeCount, normal, MAX_COORD_INTEGER, true );
	planeCount++;
	AddPlaneToList( planeList, planeCount, normal, -MAX_COORD_INTEGER, false );
	planeCount++;

	CSGPlaneList( g_LeafVis, planeList, planeCount );
}

#ifndef SWDS
void DrawLeafvis( leafvis_t *pVis )
{
	int vert = 0;
	materialSystemInterface->Bind( g_materialLeafVisWireframe );
	for ( int i = 0; i < pVis->polyCount; i++ )
	{
		if ( pVis->polyVertCount[i] >= 3 )
		{
			IMesh *pMesh = materialSystemInterface->GetDynamicMesh( );
			CMeshBuilder meshBuilder;
			meshBuilder.Begin( pMesh, MATERIAL_LINES, pVis->polyVertCount[i] );
			for ( int j = 0; j < pVis->polyVertCount[i]; j++ )
			{
				meshBuilder.Position3fv( pVis->verts[ vert + j ].Base() );
				meshBuilder.AdvanceVertex();
				meshBuilder.Position3fv( pVis->verts[ vert + ( ( j + 1 ) % pVis->polyVertCount[i] ) ].Base() );
				meshBuilder.AdvanceVertex();
			}
			meshBuilder.End();
			pMesh->Draw();
		}
		vert += pVis->polyVertCount[i];
	}
}

leafvis_t *g_FrustumVis = NULL;

//-----------------------------------------------------------------------------
// Purpose: Draw the leaf geometry that was computed by LeafVisBuild()
//-----------------------------------------------------------------------------
void LeafVisDraw( void )
{
	if ( g_FrustumVis )
	{
		DrawLeafvis( g_FrustumVis );
	}
	if ( mat_leafvis.GetInt() && g_LeafVis )
	{
		DrawLeafvis( g_LeafVis );
	}
}


void CSGFrustum( Frustum_t &frustum )
{
	if ( !g_FrustumVis )
	{
		g_FrustumVis = new leafvis_t;
	}

	cplane_t planeList[6];
	for ( int i = 0; i < 6; i++ )
	{
		planeList[i] = *frustum.GetPlane( i );
	}
	CSGPlaneList( g_FrustumVis, planeList, 6 );
}
#endif
