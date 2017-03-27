#include <string.h>
#include "convert.h"
#include "vector.h"
#include "mathlib.h"
#include "cmodel.h"
#include "physics_trace.h"
#include "ivp_physics.hxx"
//#include "ivp_core.hxx"
#include "ivp_surman_polygon.hxx"
//#include "ivp_templates.hxx"
#include "ivp_compact_ledge.hxx"
//#include "ivp_compact_ledge_solver.hxx"
#include "vphysics_interface.h"
#include "ivp_compact_surface.hxx"
#include "utlvector.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DIST_EPSILON	((1.f/256.f))
#define DIST_SQREPSILON	(DIST_EPSILON*DIST_EPSILON)
struct simplexvert_t
{
	Vector	position;
	int		testIndex;
	int		sweepIndex;
	int		obstacleIndex;
};

struct simplex_t
{
	simplexvert_t	verts[4];
	// when a simplex is expanded from an edge to a triangle, we store a plane here
	// so that we can iterate to find a suitable triangle or tetrahedron to continue the
	// intersection test
	Vector			edgeNormal;	// planar constraint
	float			edgeDist;

	int				vertCount;
};


//-----------------------------------------------------------------------------
// Purpose: Implementation for Trace against an IVP object
//-----------------------------------------------------------------------------
class CTraceIVP : public ITraceObject
{
public:
	CTraceIVP( const IVP_Compact_Surface *pSurface, const Vector &origin, const QAngle &angles, CPhysicsTrace &traceapi );
	virtual Vector SupportMap( const Vector &dir, int &index );
	virtual Vector GetVertByIndex( int index );

	// UNDONE: Do general ITraceObject center/offset computation and move the ray to account
	// for this delta like we do in TraceSweepIVP()
	// Then we can shrink the radius of objects with mass centers NOT at the origin
	virtual float Radius( void ) 
	{ 
		return m_radius;
	}

	// UNDONE: Optimize this by storing 3 matrices? (one for each transform that includes rot/scale for HL/IVP)?
	// UNDONE: Not necessary if we remove the coordinate conversion
	inline void TransformDirectionToLocal( const Vector &dir, IVP_U_Float_Point &local )
	{
		IVP_U_Float_Point tmp;
		ConvertDirectionToIVP( dir, tmp );
		m_matrix.vimult3( &tmp, &local );
	}

	inline void TransformPositionToLocal( const Vector &pos, IVP_U_Float_Point &local )
	{
		IVP_U_Float_Point tmp;
		ConvertPositionToIVP( pos, tmp );
		m_matrix.vimult4( &tmp, &local );
	}

	inline void TransformPositionFromLocal( const IVP_U_Float_Point &local, Vector &out )
	{
		IVP_U_Float_Point tmp;
		
		m_matrix.vmult4( &local, &tmp );
		ConvertPositionToHL( tmp, out );
	}

	bool IsValid( void ) { return m_pLedge != NULL; }
	void SetLedge( const IVP_Compact_Ledge *pLedge )
	{
		m_pLedge = pLedge;
	}

	bool SetSingleConvex( void )
	{
		const IVP_Compact_Ledgetree_Node *node = m_pSurface->get_compact_ledge_tree_root();
		if ( node->is_terminal() == IVP_TRUE )
		{
			SetLedge( node->get_compact_ledge() );
			return true;
		}
		SetLedge( NULL );
		return false;
	}
	
	const IVP_Compact_Surface	*m_pSurface;

private:
	const IVP_Compact_Ledge		*m_pLedge;
	IVP_U_Matrix				m_matrix;
	float						m_radius;
	CPhysicsTrace				&m_traceapi;
};

	
CTraceIVP::CTraceIVP( const IVP_Compact_Surface *pSurface, const Vector &origin, const QAngle &angles, CPhysicsTrace &traceapi )
					 : m_traceapi(traceapi)
{
	m_pSurface = pSurface;
	m_pLedge = NULL;

	ConvertRotationToIVP( angles, m_matrix );
	ConvertPositionToIVP( origin, m_matrix.vv );
	// UNDONE: Move this offset calculation into the tracing routines
	// I didn't do this now because it seems to require changes to most of the
	// transform routines - and this would cause bugs.
	
	float centerOffset = VectorLength( m_pSurface->mass_center.k );
	m_radius = ConvertDistanceToHL( m_pSurface->upper_limit_radius + centerOffset );
}


Vector CTraceIVP::SupportMap( const Vector &dir, int &index )
{
	int triCount = m_pLedge->get_n_triangles();
	const IVP_U_Float_Point *pPoints = m_pLedge->get_point_array();

	/// bump the visit index
	m_traceapi.NewVisit();

	IVP_U_Float_Point mapdir;
	TransformDirectionToLocal( dir, mapdir );

	float dot;
#if 0
	int triIndex, edgeIndex;
	m_traceapi.GetStartVert( m_pLedge, mapdir, triIndex, edgeIndex );
#else
	int triIndex = 0, edgeIndex = 0;
#endif
	const IVP_Compact_Triangle *pTri = m_pLedge->get_first_triangle() + triIndex;
	const IVP_Compact_Edge *pEdge = pTri->get_edge( edgeIndex );
	int best = pEdge->get_start_point_index();
	float bestDot = pPoints[best].dot_product( &mapdir );

	// This should never happen.  MAX_CONVEX_VERTS is very large (millions), none of our
	// models have anywhere near this many verts in a convex piece
	Assert(triCount*3<MAX_CONVEX_VERTS);
	// this loop will early out, but keep it from being infinite
	for ( int i = 0; i < triCount; i++ )
	{
		// get the index to the end vert of this edge (start vert on next edge)
		pEdge = pEdge->get_prev();
		int stopVert = pEdge->get_start_point_index();
		
		// loop through the verts that can be reached along edges from this vert
		// stop if you get back to the one you're starting on.
		int vert = stopVert;
		do
		{
			if ( !m_traceapi.WasVisited(vert) )
			{
				//m_traceapi.CountStatDotProduct();
				// this lets us skip doing dot products on this vert
				m_traceapi.VisitVert(vert);
				dot = pPoints[vert].dot_product( &mapdir );
				if ( dot > bestDot )
				{
					bestDot = dot;
					best = vert;
					break;
				}
			}
			// tri opposite next edge, same starting vert as next edge
			pEdge = pEdge->get_opposite()->get_prev();
			vert = pEdge->get_start_point_index();
		} while ( vert != stopVert );

		// if you exhausted the possibilities for this vert, it must be the best vert
		if ( vert != best )
			break;
	}

	index = best;

	Vector out;
	TransformPositionFromLocal( pPoints[best], out ); // transform point position to world space

	return out;
}

Vector CTraceIVP::GetVertByIndex( int index )
{
	const IVP_Compact_Poly_Point *pPoints = m_pLedge->get_point_array();

	Vector out;
	TransformPositionFromLocal( pPoints[index], out );
	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Implementation for Trace against an AABB
//-----------------------------------------------------------------------------
class CTraceAABB : public ITraceObject
{
public:
	CTraceAABB( const Vector &hlmins, const Vector &hlmaxs );
	virtual Vector SupportMap( const Vector &dir, int &index );
	virtual Vector GetVertByIndex( int index );
	virtual float Radius( void ) { return m_maxs.Length(); }

private:
	Vector	m_mins;
	Vector	m_maxs;

	bool	m_empty;
};


CTraceAABB::CTraceAABB( const Vector &hlmins, const Vector &hlmaxs )
{
	if ( hlmins == vec3_origin && hlmaxs == vec3_origin )
	{
		m_empty = true;
		m_mins = vec3_origin;
		m_maxs = vec3_origin;
	}
	else
	{
		m_mins = hlmins;
		m_maxs = hlmaxs;
		m_empty = false;
	}
}


Vector CTraceAABB::SupportMap( const Vector &dir, int &index )
{
	Vector out;

	if ( m_empty )
	{
		index = 0;
		return vec3_origin;
	}
	// index is formed by the 3-bit bitfield SzSySx (negative is 0, positive is 1)
	index = 0;
	if ( dir.x < 0 )
	{
		out.x = m_mins.x;
	}
	else 
	{
		index += 1;
		out.x = m_maxs.x;
	}
	if ( dir.y < 0 )
	{
		out.y = m_mins.y;
	}
	else
	{
		index += 2;
		out.y = m_maxs.y;
	}
	if ( dir.z < 0 )
	{
		out.z = m_mins.z;
	}
	else
	{
		index += 4;
		out.z = m_maxs.z;
	}

	return out;
}

Vector CTraceAABB::GetVertByIndex( int index )
{
	Vector out;
	out.x = (index&1)?m_maxs.x:m_mins.x;
	out.y = (index&2)?m_maxs.y:m_mins.y;
	out.z = (index&4)?m_maxs.z:m_mins.z;

	return out;
}


//-----------------------------------------------------------------------------
// Purpose: Implementation for Trace against an IVP object
//-----------------------------------------------------------------------------
class CTraceRay : public ITraceObject
{
public:
	CTraceRay( const Vector &hlstart, const Vector &hlend );
	CTraceRay( const Ray_t &ray );
	virtual Vector SupportMap( const Vector &dir, int &index );
	virtual Vector GetVertByIndex( int index ) { return ( index ) ? m_end : m_start; }
	virtual float Radius( void ) { return m_length * 0.5f; }

	void Reset( float length );

	Vector	m_start;
	Vector	m_end;
	Vector	m_dir;

	float	m_length;
	float	m_bestDist;
};

CTraceRay::CTraceRay( const Vector &hlstart, const Vector &hlend )
{
	m_start = hlstart;
	m_end = hlend;
	m_dir = m_end-m_start;
	m_length = VectorNormalize(m_dir);
	m_bestDist = 0.f;
}

CTraceRay::CTraceRay( Ray_t const& ray )
{
	VectorCopy( ray.m_Start, m_start );
	VectorAdd( m_start, ray.m_Delta, m_end );
	VectorCopy( ray.m_Delta, m_dir );
	m_length = VectorNormalize(m_dir);
	m_bestDist = 0.f;
}

void CTraceRay::Reset( float length )
{
	m_length = length;
	m_end = m_start + length*m_dir;
	m_bestDist = 0.f;
}

Vector CTraceRay::SupportMap( const Vector &dir, int &index )
{
	if ( DotProduct( dir, m_dir ) > 0 )
	{
		index = 1;
		return m_end;
	}
	index = 0;
	return m_start;
}

static char			*map_nullname = "**empty**";
static csurface_t	nullsurface = { map_nullname, 0 };

void CM_ClearTrace( trace_t *trace )
{
	memset( trace, 0, sizeof(*trace));
	trace->fraction = 1.f;
	trace->fractionleftsolid = 0;
	trace->surface = nullsurface;
}

class CDefConvexInfo : public IConvexInfo
{
public:
	IConvexInfo *GetPtr() { return this; }

	virtual unsigned int GetContents( int convexGameData ) { return CONTENTS_SOLID; }
};

class CTraceSolverSweptObject
{
public:
	CTraceSolverSweptObject( trace_t *ptr, ITraceObject *sweepobject, CTraceRay *ray, CTraceIVP *obstacle, const Vector &axis, unsigned int contentsMask, IConvexInfo *pConvexInfo );
	bool SweepSingleConvex( void );
	float SolveMeshIntersection( simplex_t &simplex );

	void SweepLedgeTree_r( const IVP_Compact_Ledgetree_Node *node );
	bool SweepHitsSphereOS( IVP_U_Float_Point *sphereCenter, float radius );

	void InitOSRay( void );
	void DoSweep( void );

	ITraceObject	*m_sweepObject;
	CTraceIVP		*m_obstacle;
	CTraceRay		*m_ray;
	trace_t			*m_pTotalTrace;
	trace_t			m_trace;
	IConvexInfo		*m_pConvexInfo;
	unsigned int	m_contentsMask;
	CDefConvexInfo	m_fakeConvexInfo;


	IVP_U_Float_Point	m_rayCenterOS;
	IVP_U_Float_Point	m_rayStartOS;
	IVP_U_Float_Point	m_rayDirOS;
	IVP_U_Float_Point	m_rayEndOS;
	float				m_rayLengthOS;
	Vector				m_axis;
};

CTraceSolverSweptObject::CTraceSolverSweptObject( trace_t *ptr, ITraceObject *sweepobject, CTraceRay *ray, CTraceIVP *obstacle, const Vector &axis, unsigned int contentsMask, IConvexInfo *pConvexInfo )
{
	m_pTotalTrace = ptr;
	m_sweepObject = sweepobject;
	m_obstacle = obstacle;
	m_ray = ray;
	m_axis = axis;
	m_contentsMask = contentsMask;
	m_pConvexInfo = (pConvexInfo != NULL) ? pConvexInfo : m_fakeConvexInfo.GetPtr();
}


bool CTraceSolverSweptObject::SweepHitsSphereOS( IVP_U_Float_Point *sphereCenter, float radius )
{
	// the ray is actually a line-swept-sphere with sweep object's radius
	radius += m_sweepObject->Radius();

    IVP_DOUBLE qrad_sum = m_rayLengthOS * 0.5f + radius;
    qrad_sum *= qrad_sum;
    IVP_U_Float_Point delta_vec; // quick check for ends of ray
    delta_vec.subtract(sphereCenter, &m_rayCenterOS);
    IVP_DOUBLE quad_center_dist = delta_vec.quad_length();
    
	// Could a ray in any direction away from the ray center intersect this sphere?
	if ( quad_center_dist >= qrad_sum )
	{
		return false;
    }

	// Is the sphere close enough to the ray at the center?
    IVP_DOUBLE qsphere_rad = radius * radius;
    if ( quad_center_dist < qsphere_rad )
	{
		return true;
    }
    
	// If this is a 0 length ray, then the conservative test is 100% accurate
	if ( m_rayLengthOS > 0 )
	{
		// Calculate the perpendicular distance to the sphere 
		// The perpendicular forms a right triangle with the vector between the ray/sphere centers
		// and the ray direction vector.  Calculate the projection of the hypoteneuse along the perpendicular
		IVP_U_Float_Point h;
		h.inline_calc_cross_product(&m_rayDirOS, &delta_vec);
    
		IVP_FLOAT quad_dist = (IVP_FLOAT)h.quad_length();
    
		if( quad_dist < qsphere_rad )
			return true;
    }
    return false;
}

void CTraceSolverSweptObject::SweepLedgeTree_r( const IVP_Compact_Ledgetree_Node *node )
{
    // Recursive function
    // Check whether ray hits the ledge tree node sphere
    IVP_U_Float_Point center; center.set( node->center.k);
	
	if ( !SweepHitsSphereOS( &center, node->radius ) )
		return;

	if ( node->is_terminal() == IVP_TRUE )
	{
		const IVP_Compact_Ledge *ledge = node->get_compact_ledge();
		unsigned int ledgeContents = m_pConvexInfo->GetContents( ledge->get_client_data() );
		if ( m_contentsMask & ledgeContents )
		{
			m_obstacle->SetLedge( ledge );
			if ( SweepSingleConvex() )
			{
				float fraction = m_pTotalTrace->fraction * m_trace.fraction;
				*m_pTotalTrace = m_trace;
				m_pTotalTrace->fraction = fraction;
				m_ray->Reset( m_ray->m_length * m_trace.fraction );

				// Update OS ray to limit tests
				m_rayCenterOS.add_multiple( &m_rayStartOS, &m_rayDirOS, m_ray->m_length*0.5f );
				m_rayEndOS.add_multiple( &m_rayStartOS,  &m_rayDirOS, m_ray->m_length );
				m_rayLengthOS = m_ray->m_length;
				m_pTotalTrace->contents = ledgeContents;
			}
		}
	
		return;
    }
    
    // check nodes children
    SweepLedgeTree_r(node->left_son());
    SweepLedgeTree_r(node->right_son());
}



void CTraceSolverSweptObject::InitOSRay( void )
{
	// transform ray into object space
	m_rayLengthOS = m_ray->m_length;
	m_obstacle->TransformPositionToLocal( m_ray->m_start, m_rayStartOS );

	// no translation on matrix mult because this is a vector
	m_obstacle->TransformDirectionToLocal( m_ray->m_dir, m_rayDirOS );

	// add_multiple with 3 params assumes no initial value (should be set_add_multiple)
	m_rayCenterOS.add_multiple( &m_rayStartOS, &m_rayDirOS, m_rayLengthOS*0.5f );
	m_rayEndOS.add_multiple( &m_rayStartOS,  &m_rayDirOS, m_rayLengthOS );
}


void CTraceSolverSweptObject::DoSweep( void )
{
	InitOSRay();
	
	// iterate ledge tree of obstacle
	const IVP_Compact_Surface *pSurface = m_obstacle->m_pSurface;

    const IVP_Compact_Ledgetree_Node *lt_node_root;
    lt_node_root = pSurface->get_compact_ledge_tree_root();
	SweepLedgeTree_r( lt_node_root );
}

void CPhysicsTrace::SweepBoxIVP( const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, const IVP_Compact_Surface *pSurface, const Vector &surfaceOrigin, const QAngle &surfaceAngles, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( start, end, mins, maxs );
	SweepBoxIVP( ray, MASK_ALL, NULL, pSurface, surfaceOrigin, surfaceAngles, ptr );
}

void CPhysicsTrace::SweepBoxIVP( const Ray_t &raySrc, unsigned int contentsMask, IConvexInfo *pConvexInfo, const IVP_Compact_Surface *pSurface, const Vector &surfaceOrigin, const QAngle &surfaceAngles, trace_t *ptr )
{
	ClearStatDotProduct();
	CM_ClearTrace( ptr );

	CTraceAABB box( -raySrc.m_Extents, raySrc.m_Extents );
	CTraceIVP ivp( pSurface, surfaceOrigin, surfaceAngles, *this );
	CTraceRay ray( raySrc );

	CTraceSolverSweptObject solver( ptr, &box, &ray, &ivp, raySrc.m_Start - surfaceOrigin, contentsMask, pConvexInfo );
	solver.DoSweep();

	VectorAdd( raySrc.m_Start, raySrc.m_StartOffset, ptr->startpos );
	VectorMA( ptr->startpos, ptr->fraction, raySrc.m_Delta, ptr->endpos );
}

void CPhysicsTrace::SweepIVP( const Vector &start, const Vector &end, const IVP_Compact_Surface *pSweptSurface, const QAngle &sweptAngles, const IVP_Compact_Surface *pSurface, const Vector &surfaceOrigin, const QAngle &surfaceAngles, trace_t *ptr )
{
	ClearStatDotProduct();
	CM_ClearTrace( ptr );

	CTraceIVP sweptObject( pSweptSurface, vec3_origin, sweptAngles, *this );
	CTraceIVP ivp( pSurface, surfaceOrigin, surfaceAngles, *this );
	CTraceRay ray( start, end );

	sweptObject.SetSingleConvex();

	CTraceSolverSweptObject solver( ptr, &sweptObject, &ray, &ivp, start - surfaceOrigin, MASK_ALL, NULL );
	solver.DoSweep();
	ptr->endpos = start*(1.f-ptr->fraction) + end * ptr->fraction;
}


static Vector SolveGJKSet( simplex_t &simplex, const simplexvert_t &w );
static void CalculateSeparatingPlane( trace_t *ptr, ITraceObject *sweepObject, CTraceRay *ray, ITraceObject *obstacle, simplex_t &simplex );

bool CTraceSolverSweptObject::SweepSingleConvex( void )
{
	CM_ClearTrace( &m_trace );
	simplex_t simplex;
	simplexvert_t	vert;

	simplex.vertCount = 0;

	// safe loop, max 100 iterations
	for ( int i = 0; i < 100; i++ )
	{
		Vector dir = -m_axis;
		// map the direction into the minkowski sum, get a new surface point
		vert.position = m_sweepObject->SupportMap( dir, vert.testIndex );
		vert.position += m_ray->SupportMap( dir, vert.sweepIndex );
		vert.position -= m_obstacle->SupportMap( m_axis, vert.obstacleIndex );
		
		// found a separating axis, no intersection
		if ( DotProduct( m_axis, vert.position ) > 0.f )
			return false;

		m_axis = SolveGJKSet( simplex, vert );
		float testLen = VectorNormalize( m_axis );
		// contains the origin
		if ( testLen < DIST_EPSILON )
		{
			// now solve for t along the sweep
			if ( m_ray->m_start != m_ray->m_end )
			{
				float dist = SolveMeshIntersection( simplex );
				if ( dist < m_ray->m_length && dist > 0.f )
				{
					m_trace.fraction = (m_ray->m_length - dist) / m_ray->m_length;
					m_trace.endpos = m_ray->m_start*(1.f-m_trace.fraction) + m_ray->m_end*m_trace.fraction;
					CalculateSeparatingPlane( &m_trace, m_sweepObject, m_ray, m_obstacle, simplex );
					float dot = DotProduct( m_ray->m_dir, m_trace.plane.normal );
					if ( fabsf(dot) < 1e-6f )
					{
						//m_trace.fraction = 0.f;
					}
					else
					{
						dist -= (1.f/32.f) / dot;
						if ( dist < m_ray->m_length )
						{
							m_trace.fraction = (m_ray->m_length - dist) / m_ray->m_length;
						}
						else
						{
							m_trace.fraction = 0.f;
						}
					}

					m_trace.contents = CONTENTS_SOLID;
				}
				else
				{
					// UNDONE: This case happens when you start solid as well as when a false 
					// intersection is detected at the very end of the trace
					m_trace.startsolid = true;
					m_trace.allsolid = true;
					m_trace.fraction = 0.f;
				}
			}
			else
			{
				m_trace.startsolid = true;
				m_trace.allsolid = true;
				m_trace.fraction = 0;
			}
			return true;
		}
	}

	return false;
}


// convenience routine - just makes the code a little simpler.
Vector SolvePoint( simplex_t &simplex, simplexvert_t point )
{
	simplex.vertCount = 1;
	simplex.verts[0] = point;
	return point.position;
}

// parametric value for closes point on a line segment (p0->p1) to the origin.
Vector SolveVoronoiRegion2( simplex_t &simplex, const simplexvert_t &newPoint )
{
	Vector d = newPoint.position - simplex.verts[0].position;

	float a = DotProduct(simplex.verts[0].position,d);
	float b = DotProduct(newPoint.position,d);

	if ( a != b )
	{
		float x = -b / (a - b);

		if ( x <= 0.f )
		{
			simplex.verts[0] = newPoint;
			simplex.vertCount = 1;
			return newPoint.position;
		}
		else if ( x >= 1.f )
		{
			simplex.vertCount = 1;
			return simplex.verts[0].position;
		}
		simplex.vertCount = 2;
		simplex.verts[1] = newPoint;

		return x * simplex.verts[0].position + (1.f-x)*newPoint.position;
	}
	else
	{
		simplex.vertCount = 1;
		return simplex.verts[0].position;
	}
}

struct voronoiedge_t
{
	Vector	dir;
	int		score;
	int		vertIndex[2];
	float	dot[2];
	int		faceIndex[2];
	inline void SetVerts( int v0, int v1 ) { vertIndex[0] = v0; vertIndex[1] = v1; }
	inline void SetFaces( int f0, int f1 ) { faceIndex[0] = f0; faceIndex[1] = f1; }
	void ScoreVerts( int *vertScores );
};

struct voronoiface_t
{
	int		vertIndex[3];
	int		offVert;
	int		edgeIndex[3];
	int		normalEdgeIndex[2];

	Vector	normal;
	float	dist;
	int		score;
	int		faceFlip;

	void SetVerts( int v0, int v1, int v2 );
	inline void SetNormalEdges( int e0, int e1 ) { normalEdgeIndex[0] = e0; normalEdgeIndex[1] = e1; }
};

void voronoiface_t::SetVerts( int v0, int v1, int v2 )
{ 
	faceFlip = 0;
	vertIndex[0] = v0; vertIndex[1] = v1; vertIndex[2] = v2;

	int vertUsed[4];

	memset( vertUsed, 0, sizeof(vertUsed) );

	int i;
	for ( i = 0; i < 3; i++ )
	{
		vertUsed[vertIndex[i]] = 1;
	}
	for ( i = 0; i < 4; i++ )
	{
		if ( !vertUsed[i] )
			offVert = i;
	}
}

void voronoiedge_t::ScoreVerts( int *vertScores )
{
	// UNDONE: Worldspace tolerance here instead of zero?
	if ( dot[1] >= dot[0] )
	{
		if ( dot[1] < 0 )
		{
			vertScores[vertIndex[1]]++;
		}
		else if ( dot[0] > 0 )
		{
			vertScores[vertIndex[0]]++;
		}
		else
		{
			score++;
		}
		return;
	}

	if ( dot[1] > 0 )
	{
		vertScores[vertIndex[1]]++;
	}
	else if ( dot[0] < 0 )
	{
		vertScores[vertIndex[0]]++;
	}
	else
	{
		score++;
	}
}

static voronoiedge_t g_VEdges[6];
static voronoiface_t g_VFaces[4];

static void InitEdges()
{
	// guard: only do this once
	static bool first = true;
	if ( !first )
		return;

	first = false;
	memset( g_VEdges, 0, sizeof(g_VEdges) );
	g_VEdges[0].SetVerts(0, 1);
	g_VEdges[1].SetVerts(0, 2);
	g_VEdges[2].SetVerts(1, 2);
	g_VEdges[3].SetVerts(0, 3);
	g_VEdges[4].SetVerts(1, 3);
	g_VEdges[5].SetVerts(2, 3);

	g_VFaces[0].SetVerts( 0, 1, 2 );
	g_VFaces[1].SetVerts( 0, 2, 3 );
	g_VFaces[2].SetVerts( 0, 3, 1 );
	g_VFaces[3].SetVerts( 1, 3, 2 );

	g_VEdges[0].SetFaces( 0, 2 );
	g_VEdges[1].SetFaces( 1, 0 );
	g_VEdges[2].SetFaces( 0, 3 );
	g_VEdges[3].SetFaces( 2, 1 );
	g_VEdges[4].SetFaces( 3, 2 );
	g_VEdges[5].SetFaces( 1, 3 );

	// cross these edge indices to get face normal
	g_VFaces[0].SetNormalEdges( 0, 1 );
	g_VFaces[1].SetNormalEdges( 1, 3 );
	g_VFaces[2].SetNormalEdges( 3, 0 );
	g_VFaces[3].SetNormalEdges( 4, 2 );
}

Vector SolveEdge( simplex_t &simplex, const voronoiedge_t &edge, const simplexvert_t **vertList )
{
	const simplexvert_t &p0 = *vertList[edge.vertIndex[0]];
	const simplexvert_t &p1 = *vertList[edge.vertIndex[1]];

	if ( edge.dot[0] != edge.dot[1] )
	{
		// UNDONE: We already know the sign (and probably value) of this in ScoreVerts()
		float x = -edge.dot[1] / (edge.dot[0] - edge.dot[1]);

		if ( x < 0.f )
			return SolvePoint( simplex, p1 );

		if ( x > 1.f )
			return SolvePoint( simplex, p0 );

		simplex.vertCount = 2;
		// may be aliases, so copy to temp before writing
		simplexvert_t t0 = p0, t1 = p1;
		simplex.verts[0] = t0;
		simplex.verts[1] = t1;

		return x * t0.position + (1.f-x)*t1.position;
	}
	else
	{
		return SolvePoint( simplex, p0 );
	}
}

Vector SolveFace( simplex_t &simplex, const voronoiface_t &face, const simplexvert_t **vertList )
{
	simplex.vertCount = 3;
	simplexvert_t t0 = *vertList[face.vertIndex[0]], t1 = *vertList[face.vertIndex[1]], t2 = *vertList[face.vertIndex[2]];
	// vertList[] may be references to simplex.verts[]
	simplex.verts[0] = t0;
	simplex.verts[1] = t1;
	simplex.verts[2] = t2;
	return face.normal * face.dist;
}

Vector SolveTetrahedron( simplex_t &simplex, const simplexvert_t **vertList )
{
	if ( simplex.vertCount != 3 )
	{
		simplexvert_t t0 = *vertList[0], t1 = *vertList[1], t2 = *vertList[2], t3 = *vertList[3];
		// vertList[] may be references to simplex.verts[]
		simplex.verts[0] = t0;
		simplex.verts[1] = t1;
		simplex.verts[2] = t2;
		simplex.verts[3] = t3;
	}
	else
	{
		simplex.verts[3] = *vertList[3];
	}
	simplex.vertCount = 4;
	return vec3_origin;
}

// UNDONE: Collapse these routines into a single general routine?
Vector SolveVoronoiRegion3( simplex_t &simplex, const simplexvert_t &newPoint )
{
	int i;
	const simplexvert_t *vertList[3] = {&simplex.verts[0], &simplex.verts[1], &newPoint};

	// first test the edge planes
	// These are the boundaries of the voronoi regions around each vert
	// We test them in pairs because verts along an edge share a plane normal
	// and that makes it convenient
	int vertScores[3];
	memset( vertScores, 0, sizeof(vertScores) );

	for ( i = 0; i < 3; i++ )
	{
		const Vector &v1 = vertList[g_VEdges[i].vertIndex[1]]->position;
		const Vector &v0 = vertList[g_VEdges[i].vertIndex[0]]->position;
		g_VEdges[i].dir = v1 - v0;
		VectorNormalize( g_VEdges[i].dir );
		g_VEdges[i].score = 0;
		g_VEdges[i].dot[0] = DotProduct( v0, g_VEdges[i].dir );
		g_VEdges[i].dot[1] = DotProduct( v1, g_VEdges[i].dir );
		g_VEdges[i].ScoreVerts( vertScores );
	}

	// Are we in any of the vert voronoi regions?
	// if on the front (vert) side of all 3 planes at that vert, then yes
	for ( i = 0; i < 3; i++ )
	{
		// yep, return this vert
		if ( vertScores[i] == 2 )
			return SolvePoint( simplex, *vertList[i] );
	}

	voronoiface_t &face0 = g_VFaces[0];
	// compute each face's normal (needed for edge & face tests)
	{
		face0.normal = CrossProduct( g_VEdges[ face0.normalEdgeIndex[0] ].dir, g_VEdges[ face0.normalEdgeIndex[1] ].dir );
		VectorNormalize( face0.normal );

		face0.faceFlip = 0;
		face0.dist = DotProduct( face0.normal, vertList[face0.vertIndex[0]]->position );

		face0.score = 0;
	}

	// test for edge voronoi regions
	for ( i = 0; i < 3; i++ )
	{
		// assumes 0-1, 0-2, 1-2 edges
		int offVert[3] = {2, 1, 0};

		const Vector &vert0 = vertList[g_VEdges[i].vertIndex[0]]->position;

		Vector normal0 = CrossProduct( face0.normal, g_VEdges[i].dir );
		VectorNormalize( normal0 );
		float dot0 = DotProduct( normal0, vert0 );
		
		if ( DotProduct( normal0, vertList[offVert[i]]->position ) > dot0 )
		{
			dot0 = -dot0;
		}

		if ( dot0 < 0 && g_VEdges[i].score )
		{
			// must be in this region because we checked the other 2 bounding
			// planes in the vert region test
			return SolveEdge( simplex, g_VEdges[i], vertList );
		}
		// origin is on face side
		face0.score++;
	}

	//Assert( face0.score == 3 );
	// must be in face region
	return SolveFace( simplex, face0, vertList );
}

Vector SolveVoronoiRegion4( simplex_t &simplex, const simplexvert_t &newPoint )
{
	int i;
	const simplexvert_t *vertList[4] = {&simplex.verts[0], &simplex.verts[1], &simplex.verts[2], &newPoint};

	// first test the edge planes
	// These are the boundaries of the voronoi regions around each vert
	// We test them in pairs because verts along an edge share a plane normal
	// and that makes it convenient
	int vertScores[4];
	memset( vertScores, 0, sizeof(vertScores) );

	for ( i = 0; i < 6; i++ )
	{
		const Vector &v1 = vertList[g_VEdges[i].vertIndex[1]]->position;
		const Vector &v0 = vertList[g_VEdges[i].vertIndex[0]]->position;
		g_VEdges[i].dir = v1 - v0;
		VectorNormalize( g_VEdges[i].dir );
		g_VEdges[i].score = 0;
		g_VEdges[i].dot[0] = DotProduct( v0, g_VEdges[i].dir );
		g_VEdges[i].dot[1] = DotProduct( v1, g_VEdges[i].dir );
		g_VEdges[i].ScoreVerts( vertScores );
	}

	// Are we in any of the vert voronoi regions?
	// if on the front (vert) side of all 3 planes at that vert, then yes
	for ( i = 0; i < 4; i++ )
	{
		// yep, return this vert
		if ( vertScores[i] == 3 )
			return SolvePoint( simplex, *vertList[i] );
	}

	// compute each face's normal (needed for edge & face tests)
	for ( i = 0; i < 4; i++ )
	{
		g_VFaces[i].normal = CrossProduct( g_VEdges[ g_VFaces[i].normalEdgeIndex[0] ].dir, g_VEdges[ g_VFaces[i].normalEdgeIndex[1] ].dir );
		VectorNormalize( g_VFaces[i].normal );

		g_VFaces[i].faceFlip = 0;
		g_VFaces[i].dist = DotProduct( g_VFaces[i].normal, vertList[g_VFaces[i].vertIndex[0]]->position );
		float dot = DotProduct( g_VFaces[i].normal, vertList[g_VFaces[i].offVert]->position );
		if ( dot > g_VFaces[i].dist )
		{
			g_VFaces[i].normal *= -1;
			g_VFaces[i].dist = -g_VFaces[i].dist;
			g_VFaces[i].faceFlip = 1;
		}

		g_VFaces[i].score = 0;
	}

	// test for edge voronoi regions
	for ( i = 0; i < 6; i++ )
	{
		const Vector &vert0 = vertList[g_VEdges[i].vertIndex[0]]->position;
		voronoiface_t &face0 = g_VFaces[g_VEdges[i].faceIndex[0]];

		Vector normal0 = face0.faceFlip ? CrossProduct( face0.normal, g_VEdges[i].dir ) : CrossProduct( g_VEdges[i].dir, face0.normal );
		VectorNormalize( normal0 );
		float dot0 = DotProduct( normal0, vert0 );
		if ( dot0 >= 0 )
		{
			face0.score++;
		}

		voronoiface_t &face1 = g_VFaces[g_VEdges[i].faceIndex[1]];
		Vector normal1 = face1.faceFlip ? CrossProduct( g_VEdges[i].dir, face1.normal ) : CrossProduct( face1.normal, g_VEdges[i].dir );
		VectorNormalize( normal1 );
		float dot1 = DotProduct( normal1, vert0 );
		if ( dot1 >= 0 )
		{
			face1.score++;
		}
		// not inside this edge region
		if ( g_VEdges[i].score == 0 || dot0 >= 0 || dot1 >= 0 )
			continue;

		return SolveEdge( simplex, g_VEdges[i], vertList );
	}

	int score = 0;

	// test each face
	for ( i = 0; i < 4; i++ )
	{
		if ( g_VFaces[i].dist > 0 )
		{
			score++;
			continue;
		}
		if ( g_VFaces[i].score < 3 )
			continue;
		return SolveFace( simplex, g_VFaces[i], vertList );
	}
//	Assert( score == 4 );
	return SolveTetrahedron( simplex, vertList );
}

Vector SolveGJKSet( simplex_t &simplex, const simplexvert_t &w )
{
	switch( simplex.vertCount )
	{
	case 0:
		simplex.vertCount = 1;
		simplex.verts[0] = w;
		return w.position;
	case 1:
		return SolveVoronoiRegion2( simplex, w );
	case 2:
		return SolveVoronoiRegion3( simplex, w );
	case 3:
		return SolveVoronoiRegion4( simplex, w );
	}

	return vec3_origin;
}


// if none of the verts have a sweep index of 1, then they are not dependent on the sweep
// and this is an intersection from the initial object position.
int ClassifySweepVerts( simplex_t &simplex, int skipVert, int moveIndex[4], int staticIndex[4] )
{
	assert( simplex.vertCount <= 4 );
	int moveCount = 0, staticCount = 0;
	for ( int i = 0; i < simplex.vertCount; i++ )
	{
		if ( i == skipVert )
			continue;

		if ( simplex.verts[i].sweepIndex != 0 )
		{
			moveIndex[moveCount] = i;
			moveCount++;
		}
		else
		{
			staticIndex[staticCount] = i;
			staticCount++;
		}
	}

	return moveCount;
}


float SolveTriangleOnePoint( const Vector &move0, const Vector &static0, const Vector &static1, const Vector &direction )
{
	// normal of plane of the new triangle (when the moved triangle contains the origin)
	Vector normal = CrossProduct( static1, static0 );
	VectorNormalize( normal );

	float startDot = DotProduct( normal, move0 );
	float scale = -DotProduct( direction, normal );

	// how far along direction we have to move to get move0 on the new plane
	// will be negative/infinite for unsolvable problems
	float dist = startDot / scale;

	return dist;
}


float SolveTriangleTwoPoint( const Vector &move0, const Vector &move1, const Vector &static0, const Vector &direction, float length )
{
	// triangle's normal
	Vector moveEdge = move1-move0;
	Vector triNormal = CrossProduct( moveEdge, move0-static0 );
	VectorNormalize( triNormal );
	// plane constant for original triangle
	float edgeDot = DotProduct( triNormal, move0 );
	// make the normal point toward the origin
	if ( edgeDot > 0 )
	{
		edgeDot = -edgeDot;
		triNormal = -triNormal;
	}

	// projection of the trace on the triangle's normal
	float edgeScale = DotProduct( triNormal, direction );
	
	// not going to hit the origin in this direction?
	if ( (edgeScale*length) <= DIST_EPSILON )
		return -1.f;

	// plane containing the moving edge
	Vector normal = CrossProduct( direction, moveEdge );
	VectorNormalize( normal );

	// Make sure the normal points the right way
	// calc distance of the static vert from the final plane
	float startDot = DotProduct( normal, static0 ) - DotProduct( normal, move0 );
	if ( startDot < 0 )
	{
		normal = -normal;
		startDot = -startDot;
	}

	// intersect ray from static0->origin with the plane
	Vector ray = -static0;
	VectorNormalize( ray );

	float scale = -DotProduct( ray, normal );
	if ( scale <= 0.f )
		return -1.f;

	float dist = startDot / scale;

	// point on the edge in the final plane
	Vector p = static0 + dist * ray;

	// Now calculate how far from the original edge along direction that point is

	// find distance from p to the original triangle
	float edgeDist = DotProduct( p, triNormal ) - edgeDot;
	
	// project that on the movement direction to find final distance
	edgeDist = edgeDist / edgeScale;


	return edgeDist;
}

float SolveTriangleThreePoint( const Vector &move0, const Vector &move1, const Vector &move2, const Vector &direction )
{

	// normal of plane of the new triangle (when the moved triangle contains the origin)
	Vector normal = CrossProduct( move1-move0, move2-move0 );
	VectorNormalize( normal );

	// Make sure the normal points toward the origin
	float startDot = DotProduct( normal, move0 );
	if ( startDot > 0 )
	{
		normal = -normal;
		startDot = -startDot;
	}

	// projection of the movement direction on the final plane normal
	float scale = -DotProduct( direction, normal );
	if ( scale >= 0 )
		return -1.f;


	// how far along direction we have to move to get move0 on the new plane
	float dist = startDot / scale;

	return dist;
}

// find the point at which the parametric line intersects the origin by moving vertices 
// in the plane of the triangle (origin is in the plane of the triangle)
float CollapseLine( simplex_t &simplex, CTraceRay *ray, Vector &dir )
{
	Vector line = simplex.verts[1].position-simplex.verts[0].position;
	VectorNormalize(line);

	float proj = DotProduct( line, ray->m_dir );
	

	// already collapsed, origin will travel more than epsilon outside the line along the sweep
	if ( fabsf(proj * ray->m_length - ray->m_length) > DIST_EPSILON )
	{
		Vector binormal = CrossProduct( line, ray->m_dir );
		dir = CrossProduct( binormal, line );
		simplex.edgeNormal = binormal;
		simplex.edgeDist = DotProduct( simplex.verts[0].position, simplex.edgeNormal );
		return ray->m_length*2;
	}

	float dist = -1.f;
	int best = -1;
	if ( proj > 0 )
	{
		// sweep is in the direction of vert 1
		float dot = DotProduct(simplex.verts[1].position, ray->m_dir );
		dist = dot / proj;
		best = 1;
	}
	else
	{
		// sweep is in the direction of vert 0
		float dot = -DotProduct(simplex.verts[0].position, ray->m_dir );
		dist = dot / proj;
		best = 0;
	}

	if ( dist >= 0 )
	{
		if ( best != 0 )
		{
			simplex.verts[0] = simplex.verts[best];
		}
		dir = simplex.verts[0].position;
		simplex.vertCount = 1;
		return dist;
	}

	return 0.f;
}

// find the point at which the parametric triangle intersects the origin by moving vertices 
// in the plane of the triangle (origin is in the plane of the triangle)
float CollapseTriangle( simplex_t &simplex, CTraceRay *ray, Vector &dir, bool skipLast )
{
	Vector normal = CrossProduct( simplex.verts[1].position-simplex.verts[0].position, simplex.verts[2].position-simplex.verts[0].position );

	VectorNormalize( normal );
	float normalDist = DotProduct( normal, ray->m_dir );
	// already collapsed, origin will travel more than epsilon through the plane of the triangle
	if ( fabsf(normalDist * ray->m_length) > DIST_EPSILON )
	{
		if ( normalDist > 0 )
			dir = normal;
		else
			dir = -normal;
		return ray->m_length*2;
	}

	int paraVerts[4], staticVerts[4];
	int paraCount = 0;

	float bestDist = ray->m_length;
	int best = -1;
	int edgeCount = skipLast?2:3;
	float dist;
	for ( int i = 0; i < edgeCount; i++ )
	{
		paraCount = ClassifySweepVerts( simplex, i, paraVerts, staticVerts );
		switch( paraCount )
		{
		case 0:
		default:
			dist = -1.f;
			continue;
			break;
		case 1:
			{
				// stab is from the stationary vert, passing through the origin
				Vector stab = -simplex.verts[staticVerts[0]].position;
				VectorNormalize( stab );

				// the moving vert will intersect this plane
				Vector planeNormal = CrossProduct( normal, stab );
				// plane constant is zero (contains the origin)

				// Clip the ray from the moving point along the sweep dir to the plane
				float scale = DotProduct( planeNormal, ray->m_dir );

				if ( scale == 0.f ) // won't intersect
				{
					continue;
				}

				dist = DotProduct( planeNormal, simplex.verts[paraVerts[0]].position );
				dist /= scale;
			}
			break;
		
		case 2:
			{
				Vector planeNormal = CrossProduct( normal, simplex.verts[paraVerts[1]].position-simplex.verts[paraVerts[0]].position );
				VectorNormalize( planeNormal );
				float planeDist = DotProduct( simplex.verts[paraVerts[1]].position, planeNormal );
				float scale = DotProduct( ray->m_dir, planeNormal );
				if ( scale == 0.f )
					continue;
				dist = planeDist / scale;
			}
			break;
		}

		if ( dist > ray->m_bestDist && dist < bestDist )
		{
			bestDist = dist;
			best = i;
		}
	}
	
	if ( best >= 0 )
	{
		if ( best != 2 )
		{
			simplex.verts[best] = simplex.verts[2];
		}
		dir = CrossProduct( simplex.verts[1].position - simplex.verts[0].position, normal );
		// make sure the normal points away from the origin
		if ( DotProduct( dir, simplex.verts[1].position ) < 0 )
		{
			dir = -dir;
		}
		simplex.vertCount = 2;
		simplex.edgeNormal = normal;
		simplex.edgeDist = normalDist;
		return bestDist;
	}

	return 0.f;
}

float CollapseSweep( simplex_t &simplex, CTraceRay *ray, Vector &dir, bool skipLast )
{
	int paraVerts[4], staticVerts[4];
	int paraCount = 0;

	paraCount = ClassifySweepVerts( simplex, -1, paraVerts, staticVerts );
	if ( !paraCount )
		return 0.f;

	if ( simplex.vertCount <= 2 )
		return CollapseLine( simplex, ray, dir );

	if ( simplex.vertCount == 3 )
	{
		return CollapseTriangle( simplex, ray, dir, skipLast );
	}

	int i;
	float dist;
	float bestDist = ray->m_length;
	int best = -1;

	int triCount = skipLast?3:4;

	Vector backdir = -ray->m_dir;
	for ( i = 0; i < triCount; i ++ )
	{
		paraCount = ClassifySweepVerts( simplex, i, paraVerts, staticVerts );

		switch( paraCount )
		{
		case 1:
			dist = SolveTriangleOnePoint( simplex.verts[paraVerts[0]].position, simplex.verts[staticVerts[0]].position, simplex.verts[staticVerts[1]].position, backdir );
			break;
		case 2:
			dist = SolveTriangleTwoPoint( simplex.verts[paraVerts[0]].position, simplex.verts[paraVerts[1]].position, simplex.verts[staticVerts[0]].position, backdir, ray->m_length );
			break;
		case 3:
			dist = SolveTriangleThreePoint( simplex.verts[paraVerts[0]].position, simplex.verts[paraVerts[1]].position, simplex.verts[paraVerts[2]].position, backdir );
			break;
		case 0:
		default:
			dist = -1.f;
			continue;
			break;
		}

		if ( dist > ray->m_bestDist && dist < bestDist )
		{
			bestDist = dist;
			best = i;
		}
	}
	if ( best >= 0 )
	{
		// the best triangle was the one with vertex # "best" eliminated
		if ( best != 3 )
		{
			// shift the last vert down to fill the empty slot
			simplex.verts[best] = simplex.verts[3];
		}
		simplex.vertCount = 3;

		// now simplex 0,1,2 is the triangle the origin has passed through
		dir = CrossProduct( simplex.verts[1].position - simplex.verts[0].position, simplex.verts[2].position - simplex.verts[0].position );
		VectorNormalize( dir );
		// we want the normal to point away from the origin - which means the plane constant of this triangle
		// should be positive
		if ( DotProduct( dir, simplex.verts[0].position ) < 0.f )
		{
			dir = -dir;
		}
		return bestDist;
	}
	return 0.f;
}


void CalculateSeparatingPlane( trace_t *ptr, ITraceObject *sweepObject, CTraceRay *ray, ITraceObject *obstacle, simplex_t &simplex )
{
	int testCount = 1, obstacleCount = 1, testIndex[4], obstacleIndex[4];
	testIndex[0] = simplex.verts[0].testIndex;
	obstacleIndex[0] = simplex.verts[0].obstacleIndex;
	assert( simplex.vertCount <= 4 );
	for ( int i = 1; i < simplex.vertCount; i++ )
	{
		int j;
		for ( j = 0; j < i; j++ )
		{
			if ( simplex.verts[j].obstacleIndex == simplex.verts[i].obstacleIndex )
				break;
		}
		if ( j == i )
		{
			obstacleIndex[obstacleCount++] = simplex.verts[i].obstacleIndex;
		}

		for ( j = 0; j < i; j++ )
		{
			if ( simplex.verts[j].testIndex == simplex.verts[i].testIndex )
				break;
		}
		if ( j == i )
		{
			testIndex[testCount++] = simplex.verts[i].testIndex;
		}
	}

	if ( simplex.vertCount < 3 )
	{
		if ( simplex.vertCount == 2 && testCount == 2 )
		{
			// edge / point
			Vector t0 = sweepObject->GetVertByIndex( testIndex[0] );
			Vector t1 = sweepObject->GetVertByIndex( testIndex[1] );
			Vector tangent = CrossProduct( t1-t0, ray->m_dir );
			ptr->plane.normal = CrossProduct( tangent, t1-t0 );
			VectorNormalize( ptr->plane.normal );
			ptr->plane.dist = DotProduct( t0 + ptr->endpos, ptr->plane.normal );
			return;
		}
	}
	if ( testCount == 3 )
	{
		// face / xxx
		Vector t0 = sweepObject->GetVertByIndex( testIndex[0] );
		Vector t1 = sweepObject->GetVertByIndex( testIndex[1] );
		Vector t2 = sweepObject->GetVertByIndex( testIndex[2] );
		ptr->plane.normal = CrossProduct( t1-t0, t2-t0 );
		VectorNormalize( ptr->plane.normal );
		if ( DotProduct( ptr->plane.normal, ray->m_dir ) > 0 )
		{
			ptr->plane.normal = -ptr->plane.normal;
		}
		ptr->plane.dist = DotProduct( t0 + ptr->endpos, ptr->plane.normal );
	}
	else if ( testCount == 2 && obstacleCount == 2 )
	{
		// edge / edge
		Vector t0 = sweepObject->GetVertByIndex( testIndex[0] );
		Vector t1 = sweepObject->GetVertByIndex( testIndex[1] );
		Vector t2 = obstacle->GetVertByIndex( obstacleIndex[0] );
		Vector t3 = obstacle->GetVertByIndex( obstacleIndex[1] );
		ptr->plane.normal = CrossProduct( t1-t0, t3-t2 );
		VectorNormalize( ptr->plane.normal );
		if ( DotProduct( ptr->plane.normal, ray->m_dir ) > 0 )
		{
			ptr->plane.normal = -ptr->plane.normal;
		}
		ptr->plane.dist = DotProduct( t0 + ptr->endpos, ptr->plane.normal );
	}
	else if ( obstacleCount == 3 )
	{
		// xxx / face
		Vector t0 = obstacle->GetVertByIndex( obstacleIndex[0] );
		Vector t1 = obstacle->GetVertByIndex( obstacleIndex[1] );
		Vector t2 = obstacle->GetVertByIndex( obstacleIndex[2] );
		ptr->plane.normal = CrossProduct( t1-t0, t2-t0 );
		VectorNormalize( ptr->plane.normal );
		if ( DotProduct( ptr->plane.normal, ray->m_dir ) > 0 )
		{
			ptr->plane.normal = -ptr->plane.normal;
		}
		ptr->plane.dist = DotProduct( t0, ptr->plane.normal );
	}
	else
	{
		ptr->plane.normal = -ray->m_dir;
		if ( simplex.vertCount )
		{
			ptr->plane.dist = DotProduct( ptr->plane.normal, obstacle->GetVertByIndex( simplex.verts[0].obstacleIndex ) );
		}
		else
			ptr->plane.dist = 0.f;
	}
}


float CTraceSolverSweptObject::SolveMeshIntersection( simplex_t &simplex )
{
	simplexvert_t	vert;
	bool iterate = false;
	Vector v = vec3_origin;

	// safe loop, max 100 iterations
	for ( int i = 0; i < 100; i++ )
	{
		// collapse to a triangle containing the origin if possible
		float solveDist = CollapseSweep( simplex, m_ray, v, iterate );
		if ( solveDist <= 0.f )
		{
			return 0.0f;
		}
		if ( solveDist <= m_ray->m_length )
		{
			m_ray->m_bestDist = solveDist;
		}

		VectorNormalize(v);
		// map the new separating axis (NOTE: This test is inverted from the GJK - we are trying to get out, not in)
		vert.position = m_sweepObject->SupportMap( v, vert.testIndex );
		vert.position += m_ray->SupportMap( v, vert.sweepIndex );
		vert.position -= m_obstacle->SupportMap( -v, vert.obstacleIndex );
			
		// found a separating axis, we've moved the sweep back enough
		float dist = DotProduct( v, simplex.verts[0].position ) + DIST_EPSILON;
		if ( DotProduct( v, vert.position ) <= dist )
		{
			if ( m_ray->m_bestDist == 0.f )
			{
				int paraVerts[4], staticVerts[4];
				int paraCount = ClassifySweepVerts( simplex, -1, paraVerts, staticVerts );
				// this means the trace is just touching.  Back off by epsion, but return success
				if ( paraCount == simplex.vertCount )
					return 1e-8f;	// HACKHACK: To keep this different from start solid
			}
			return m_ray->m_bestDist;
		}

		// add the new vert
		simplex.verts[simplex.vertCount] = vert;
		simplex.vertCount++;
		iterate = true;

		assert( simplex.vertCount <= 4 );
		// expanded from edge to tri, check to make sure it's planar
		if ( simplex.vertCount == 3 )
		{
			float planar = DotProduct( simplex.edgeNormal, vert.position ) - simplex.edgeDist;
			// planar with the previous constraint?
			if ( fabsf(planar) > DIST_EPSILON )	// no
			{
				// Search for a tetrahedron that crosses the edge plane
				// on the back side of the edge plane.
				Vector edge0 = simplex.verts[1].position - simplex.verts[0].position;
				int side = planar < 0;
				for ( int safeLoop = 0; safeLoop < 20; safeLoop++ )
				{
					// normal to current triangle
					v = CrossProduct( simplex.verts[2].position-simplex.verts[0].position, edge0 );
					float plane = DotProduct( v, simplex.verts[0].position );
					if (  plane < 0 )
					{
						v = -v;
						plane = -plane;
					}
					vert.position = m_sweepObject->SupportMap( v, vert.testIndex );
					vert.position += m_ray->SupportMap( v, vert.sweepIndex );
					vert.position -= m_obstacle->SupportMap( -v, vert.obstacleIndex );

					// separating axis
					if ( DotProduct( vert.position, v ) < (plane+DIST_EPSILON) )
					{
						// collapse back to edge
						simplex.vertCount = 2;
						return m_ray->m_bestDist;
					}

					float edgeDist = DotProduct( vert.position, simplex.edgeNormal );
					
					// found a vert on the other side of the edge normal, exit with a new tetrahedron to collapse
					if ( (edgeDist > (simplex.edgeDist + DIST_EPSILON) && side) || 
						(edgeDist < (simplex.edgeDist - DIST_EPSILON) && !side))
					{
						simplex.vertCount = 4;
						simplex.verts[3] = vert;
						break;
					}

					// on the same side, replace the previous vert and loop
					simplex.verts[2] = vert;
					simplex.vertCount = 3;
				}
			}
			// else planar, continue with this triangle
		}
	}

	return m_ray->m_bestDist;
}


static const Vector g_xpos(1,0,0), g_xneg(-1,0,0);
static const Vector g_ypos(0,1,0), g_yneg(0,-1,0);
static const Vector g_zpos(0,0,1), g_zneg(0,0,-1);

void TraceGetAABB_r( Vector &mins, Vector &maxs, const IVP_Compact_Ledgetree_Node *node, CTraceIVP &ivp )
{
	if ( node->is_terminal() == IVP_TRUE )
	{
		int index;

		ivp.SetLedge( node->get_compact_ledge() );
		AddPointToBounds( ivp.SupportMap( g_xneg, index ), mins, maxs );
		AddPointToBounds( ivp.SupportMap( g_yneg, index ), mins, maxs );
		AddPointToBounds( ivp.SupportMap( g_zneg, index ), mins, maxs );
		AddPointToBounds( ivp.SupportMap( g_xpos, index ), mins, maxs );
		AddPointToBounds( ivp.SupportMap( g_ypos, index ), mins, maxs );
		AddPointToBounds( ivp.SupportMap( g_zpos, index ), mins, maxs );
		return;
	}

	TraceGetAABB_r( mins, maxs, node->left_son(), ivp );
	TraceGetAABB_r( mins, maxs, node->right_son(), ivp );
}

void CPhysicsTrace::GetAABB( Vector &mins, Vector &maxs, const IVP_Compact_Surface *pCollide, const Vector &collideOrigin, const QAngle &collideAngles )
{
	int index;
	CTraceIVP ivp( pCollide, collideOrigin, collideAngles, *this );

	if ( ivp.SetSingleConvex() )
	{
		mins.x = ivp.SupportMap( g_xneg, index ).x;
		mins.y = ivp.SupportMap( g_yneg, index ).y;
		mins.z = ivp.SupportMap( g_zneg, index ).z;

		maxs.x = ivp.SupportMap( g_xpos, index ).x;
		maxs.y = ivp.SupportMap( g_ypos, index ).y;
		maxs.z = ivp.SupportMap( g_zpos, index ).z;
	}
	else
	{
		const IVP_Compact_Ledgetree_Node *lt_node_root;
		lt_node_root = pCollide->get_compact_ledge_tree_root();
		ClearBounds( mins, maxs );
		TraceGetAABB_r( mins, maxs, lt_node_root, ivp );
	}
}

void TraceGetExtent_r( const IVP_Compact_Ledgetree_Node *node, CTraceIVP &ivp, const Vector &dir, float &dot, Vector &point )
{
	if ( node->is_terminal() == IVP_TRUE )
	{
		int index;

		ivp.SetLedge( node->get_compact_ledge() );
		Vector tmp = ivp.SupportMap( dir, index );
		float newDot = DotProduct( tmp, dir );

		if ( newDot > dot )
		{
			dot = newDot;
			point = tmp;
		}
		return;
	}

	TraceGetExtent_r( node->left_son(), ivp, dir, dot, point );
	TraceGetExtent_r( node->right_son(), ivp, dir, dot, point );
}


Vector CPhysicsTrace::GetExtent( const IVP_Compact_Surface *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, const Vector &direction )
{
	int index;
	CTraceIVP ivp( pCollide, collideOrigin, collideAngles, *this );

	if ( ivp.SetSingleConvex() )
	{
		return ivp.SupportMap( direction, index );
	}
	else
	{
		const IVP_Compact_Ledgetree_Node *lt_node_root;
		lt_node_root = pCollide->get_compact_ledge_tree_root();
		Vector out = vec3_origin;
		float tmp = -1e6;
		TraceGetExtent_r( lt_node_root, ivp, direction, tmp, out );
		return out;
	}
}

struct ledgecache_t
{
	const IVP_Compact_Ledge *pLedge;
	unsigned short			startVert[8];
};

class CLedgeCache
{
public:
	CUtlVector<ledgecache_t>	ledges;
	unsigned short GetPackedIndex( const IVP_Compact_Ledge *pLedge, const IVP_U_Float_Point &dir )
	{
		const IVP_Compact_Poly_Point *pPoints = pLedge->get_point_array();
		const IVP_Compact_Triangle *pTri = pLedge->get_first_triangle();
		const IVP_Compact_Edge *pEdge = pTri->get_edge( 0 );
		int best = pEdge->get_start_point_index();
		float bestDot = pPoints[best].dot_product( &dir );
		int triCount = pLedge->get_n_triangles();
		const IVP_Compact_Triangle *pBestTri = pTri;
		// this loop will early out, but keep it from being infinite
		int i;
		for ( i = 0; i < triCount; i++ )
		{
			// get the index to the end vert of this edge (start vert on next edge)
			pEdge = pEdge->get_prev();
			int stopVert = pEdge->get_start_point_index();
			
			// loop through the verts that can be reached along edges from this vert
			// stop if you get back to the one you're starting on.
			int vert = stopVert;
			do
			{
				float dot = pPoints[vert].dot_product( &dir );
				if ( dot > bestDot )
				{
					bestDot = dot;
					best = vert;
					pBestTri = pEdge->get_triangle();
					break;
				}
				// tri opposite next edge, same starting vert as next edge
				pEdge = pEdge->get_opposite()->get_prev();
				vert = pEdge->get_start_point_index();
			} while ( vert != stopVert );

			// if you exhausted the possibilities for this vert, it must be the best vert
			if ( vert != best )
				break;
		}

		int triIndex = pBestTri - pLedge->get_first_triangle();
		int edgeIndex = 0;
		for ( i = 0; i < 3; i++ )
		{
			if ( pBestTri->get_edge(i)->get_start_point_index() == best )
			{
				edgeIndex = i;
				break;
			}
		}

		return (unsigned short) ( (triIndex<<2) + edgeIndex );
	}
};


void CPhysicsTrace::GetStartVert( const IVP_Compact_Ledge *pLedge, const IVP_U_Float_Point &localDirection, int &triIndex, int &edgeIndex )
{
	int count = m_pLedgeCache->ledges.Count();
	ledgecache_t *pCache = NULL;

	for ( int i = 0; i < count; i++ )
	{
		if ( m_pLedgeCache->ledges[i].pLedge == pLedge )
		{
			pCache = &m_pLedgeCache->ledges[i];
			break;
		}
	}

	if ( !pCache )
	{
		int index = m_pLedgeCache->ledges.AddToTail();
		pCache = &m_pLedgeCache->ledges[index];

		pCache->pLedge = pLedge;
		for ( int i = 0; i < 8; i++ )
		{
			IVP_U_Float_Point tmp;
			tmp.k[0] = ( i & 1 ) ? -1 : 1;
			tmp.k[1] = ( i & 2 ) ? -1 : 1;
			tmp.k[2] = ( i & 4 ) ? -1 : 1;
			pCache->startVert[i] = m_pLedgeCache->GetPackedIndex( pLedge, tmp );
		}
	}
	// map dir to index
	int cacheIndex = (localDirection.k[0] < 0 ? 1 : 0) + (localDirection.k[1] < 0 ? 2 : 0) + (localDirection.k[2] < 0 ? 4 : 0 );
	triIndex = pCache->startVert[cacheIndex] >> 2;
	edgeIndex = pCache->startVert[cacheIndex] & 0x3;
}


CPhysicsTrace::CPhysicsTrace()
{
	InitEdges();
	m_pLedgeCache = new CLedgeCache;
}

CPhysicsTrace::~CPhysicsTrace()
{
	delete m_pLedgeCache;
}
