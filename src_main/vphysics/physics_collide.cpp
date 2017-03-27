#include <stdio.h>
#include "interface.h"
#include "vphysics_interface.h"

#include "ivp_physics.hxx"
#include "ivp_surbuild_pointsoup.hxx"
#include "ivp_surbuild_ledge_soup.hxx"
#include "ivp_surman_polygon.hxx"
#include "ivp_compact_surface.hxx"
#include "ivp_compact_ledge.hxx"
#include "ivp_compact_ledge_solver.hxx"
#include "ivp_halfspacesoup.hxx"
#include "ivp_surbuild_halfspacesoup.hxx"
#include "hk_mopp/ivp_surbuild_mopp.hxx"
#include "hk_mopp/ivp_surman_mopp.hxx"
#include "hk_mopp/ivp_compact_mopp.hxx"

#include "convert.h"
#include "vector.h"
#include "cmodel.h"
#include "utlvector.h"
#include "physics_trace.h"
#include "vcollide_parse_private.h"
#include "vphysics_internal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

struct bboxcache_t
{
	Vector			mins;
	Vector			maxs;
	CPhysCollide	*pCollide;
};

class CPhysicsCollision : public IPhysicsCollision
{
public:
	CPhysConvex	*ConvexFromVerts( Vector **pVerts, int vertCount );
	CPhysConvex	*ConvexFromPlanes( float *pPlanes, int planeCount, float mergeDistance );
	float ConvexVolume( CPhysConvex *pConvex );
	float ConvexSurfaceArea( CPhysConvex *pConvex );
	CPhysCollide *ConvertConvexToCollide( CPhysConvex **pConvex, int convexCount );
	// store game-specific data in a convex solid
	void SetConvexGameData( CPhysConvex *pConvex, unsigned int gameData );
	void ConvexFree( CPhysConvex *pConvex );
	
	CPhysPolysoup *PolysoupCreate( void );
	void PolysoupDestroy( CPhysPolysoup *pSoup );
	void PolysoupAddTriangle( CPhysPolysoup *pSoup, const Vector &a, const Vector &b, const Vector &c, int materialIndex7bits );
	CPhysCollide *ConvertPolysoupToCollide( CPhysPolysoup *pSoup );

	int	CollideSize( CPhysCollide *pCollide );
	int	CollideWrite( char *pDest, CPhysCollide *pCollide );
	// Get the AABB of an oriented collide
	virtual void CollideGetAABB( Vector &mins, Vector &maxs, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles );
	virtual Vector CollideGetExtent( const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, const Vector &direction );
	// compute the volume of a collide
	virtual float			CollideVolume( CPhysCollide *pCollide );
	virtual float			CollideSurfaceArea( CPhysCollide *pCollide );

	// Free a collide that was created with ConvertConvexToCollide()
	// UNDONE: Move this up near the other Collide routines when the version is changed
	virtual void			DestroyCollide( CPhysCollide *pCollide );

	CPhysCollide *BBoxToCollide( const Vector &mins, const Vector &maxs );
	// loads a set of solids into a vcollide_t
	virtual void			VCollideLoad( vcollide_t *pOutput, int solidCount, const char *pBuffer, int size );
	// destroyts the set of solids created by VCollideLoad
	virtual void			VCollideUnload( vcollide_t *pVCollide );

	// Trace an AABB against a collide
	void TraceBox( const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr );
	void TraceBox( const Ray_t &ray, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr );
	void TraceBox( const Ray_t &ray, unsigned int contentsMask, IConvexInfo *pConvexInfo, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr );
	// Trace one collide against another
	void TraceCollide( const Vector &start, const Vector &end, const CPhysCollide *pSweepCollide, const QAngle &sweepAngles, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr );

	// begins parsing a vcollide.  NOTE: This keeps pointers to the text
	// If you delete the text and call members of IVPhysicsKeyParser, it will crash
	virtual IVPhysicsKeyParser	*VPhysicsKeyParserCreate( const char *pKeyData );
	// Free the parser created by VPhysicsKeyParserCreate
	virtual void			VPhysicsKeyParserDestroy( IVPhysicsKeyParser *pParser );

	// creates a list of verts from a collision mesh
	int	CreateDebugMesh( const CPhysCollide *pCollisionModel, Vector **outVerts );
	// destroy the list of verts created by CreateDebugMesh
	void DestroyDebugMesh( int vertCount, Vector *outVerts );
	// create a queryable version of the collision model
	ICollisionQuery *CreateQueryModel( CPhysCollide *pCollide );
	// destroy the queryable version
	void DestroyQueryModel( ICollisionQuery *pQuery );

	virtual IPhysicsCollision *ThreadContextCreate( void );
	virtual void			ThreadContextDestroy( IPhysicsCollision *pThreadContex );
	virtual unsigned int	ReadStat( int statID ) { return m_traceapi.ReportStatDotProduct(); }

private:
	void InitBBoxCache();
	bool IsBBoxCache( CPhysCollide *pCollide );
	void AddBBoxCache( CPhysCollide *pCollide, const Vector &mins, const Vector &maxs );
	CPhysCollide *GetBBoxCache( const Vector &mins, const Vector &maxs );
	CPhysCollide *FastBboxCollide( const CPhysCollide *pCollide, const Vector &mins, const Vector &maxs );

private:
	CPhysicsTrace		m_traceapi;
	CUtlVector<bboxcache_t>	m_bboxCache;
	int					m_bboxVertMap[8];
};

CPhysicsCollision g_PhysicsCollision;
IPhysicsCollision *physcollision = &g_PhysicsCollision;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CPhysicsCollision, IPhysicsCollision, VPHYSICS_COLLISION_INTERFACE_VERSION, g_PhysicsCollision );


class IPhysCollide
{
public:
	virtual ~IPhysCollide() = 0;
	virtual void AddReference() = 0;
	virtual void ReleaseReference() = 0;

	// get a surface manager 
	virtual IVP_SurfaceManager *GetSurfaceManager() = 0;
	virtual void GetAllLedges( IVP_U_BigVector<IVP_Compact_Ledge> &ledges ) = 0;
	virtual unsigned int GetSerializationSize() = 0;
	virtual unsigned int SerializeToBuffer( char *pDest ) = 0;

	static void UnserializeFromBuffer( const char *pBuffer, unsigned int size );
};

//-----------------------------------------------------------------------------
// Abstract compact_surface vs. compact_mopp
//-----------------------------------------------------------------------------
#define IVP_COMPACT_SURFACE_ID		MAKEID('I','V','P','S')
#define IVP_COMPACT_MOPP_ID			MAKEID('M','O','P','P')

// You can disable all of the havok Mopp collision model building by undefining this symbol
//#define ENABLE_IVP_MOPP	1

static const IVP_Compact_Surface *ConvertPhysCollideToCompactSurface( const CPhysCollide *pCollide )
{
	const IVP_Compact_Surface *pSurface = reinterpret_cast<const IVP_Compact_Surface *>(pCollide);
	if ( pSurface->dummy[2] == IVP_COMPACT_MOPP_ID )
	{
		return NULL;
	}

	return pSurface;
}

#if ENABLE_IVP_MOPP
static const IVP_Compact_Mopp *ConvertPhysCollideToCompactMopp( const CPhysCollide *pCollide )
{
	const IVP_Compact_Surface *pSurface = reinterpret_cast<const IVP_Compact_Surface *>(pCollide);
	if ( pSurface->dummy[2] == IVP_COMPACT_MOPP_ID )
	{
		return reinterpret_cast<const IVP_Compact_Mopp *>(pCollide);
	}
	return NULL;
}
#endif

IVP_SurfaceManager *CreateSurfaceManager( const CPhysCollide *pCollisionModel, short &collideType )
{
#if ENABLE_IVP_MOPP
	const IVP_Compact_Mopp *pMopp = ConvertPhysCollideToCompactMopp( pCollisionModel );
	if ( pMopp )
	{
		collideType = COLLIDE_MOPP;
		return new IVP_SurfaceManager_Mopp( pMopp );
		
	}
	else
#endif
	{
		const IVP_Compact_Surface *pSurface = ConvertPhysCollideToCompactSurface( pCollisionModel );
		collideType = COLLIDE_POLY;
		return new IVP_SurfaceManager_Polygon( pSurface );
	}
}

static void GetAllLedges( const CPhysCollide *pCollide, IVP_U_BigVector<IVP_Compact_Ledge> &ledges )
{
	const IVP_Compact_Surface *pSurface = ConvertPhysCollideToCompactSurface( const_cast<CPhysCollide *>(pCollide) );
	if ( pSurface )
	{
		IVP_Compact_Ledge_Solver::get_all_ledges( pSurface, &ledges );
	}
#if ENABLE_IVP_MOPP
	else
	{
		IVP_Compact_Mopp *pMopp = (IVP_Compact_Mopp *)pCollide;
		IVP_Compact_Ledge_Solver::get_all_ledges( pMopp, &ledges );
	}
#endif
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Create a convex element from a point cloud
// Input  : **pVerts - array of points
//			vertCount - length of array
// Output : opaque pointer to convex element
//-----------------------------------------------------------------------------
CPhysConvex	*CPhysicsCollision::ConvexFromVerts( Vector **pVerts, int vertCount )
{
	IVP_U_Vector<IVP_U_Point> points;
	int i;

	for ( i = 0; i < vertCount; i++ )
	{
		IVP_U_Point *tmp = new IVP_U_Point;
		
		ConvertPositionToIVP( *pVerts[i], *tmp );

		BEGIN_IVP_ALLOCATION();
		points.add( tmp );
		END_IVP_ALLOCATION();
	}

	BEGIN_IVP_ALLOCATION();
	IVP_Compact_Ledge *pLedge = IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge( &points );
	END_IVP_ALLOCATION();

	for ( i = 0; i < points.len(); i++ )
	{
		delete points.element_at(i);
	}
	points.clear();

	return reinterpret_cast<CPhysConvex *>(pLedge);
}

// produce a convex element from planes (csg of planes)
CPhysConvex	*CPhysicsCollision::ConvexFromPlanes( float *pPlanes, int planeCount, float mergeDistance )
{
	// NOTE: We're passing in planes with outward-facing normals
	// Ipeon expects inward facing ones; we'll need to reverse plane directon
	struct listplane_t
	{
		float normal[3];
		float dist;
	};

	listplane_t *pList = (listplane_t *)pPlanes;
	IVP_U_Hesse plane;
    IVP_Halfspacesoup halfspaces;

	mergeDistance = ConvertDistanceToIVP( mergeDistance );

	for ( int i = 0; i < planeCount; i++ )
	{
		Vector tmp( -pList[i].normal[0], -pList[i].normal[1], -pList[i].normal[2] ); 
		ConvertPlaneToIVP( tmp, -pList[i].dist, plane );
		halfspaces.add_halfspace( &plane );
	}
	
    IVP_Compact_Ledge *pLedge = IVP_SurfaceBuilder_Halfspacesoup::convert_halfspacesoup_to_compact_ledge( &halfspaces, mergeDistance );
	return reinterpret_cast<CPhysConvex *>( pLedge );
}


//-----------------------------------------------------------------------------
// Purpose: copies the first vert int pLedge to out
// Input  : *pLedge - compact ledge
//			*out - destination float array for the vert
//-----------------------------------------------------------------------------
static void LedgeInsidePoint( IVP_Compact_Ledge *pLedge, Vector& out )
{
	IVP_Compact_Triangle *pTri = pLedge->get_first_triangle();
	const IVP_Compact_Edge *pEdge = pTri->get_edge( 0 );
	const IVP_U_Float_Point *pPoint = pEdge->get_start_point( pLedge );
	ConvertPositionToHL( *pPoint, out );
}


//-----------------------------------------------------------------------------
// Purpose: Calculate the volume of a tetrahedron with these vertices
// Input  : p0 - points of tetrahedron
//			p1 - 
//			p2 - 
//			p3 - 
// Output : float (volume in units^3)
//-----------------------------------------------------------------------------
static float TetrahedronVolume( const Vector &p0, const Vector &p1, const Vector &p2, const Vector &p3 )
{
	Vector a, b, c, cross;
	float volume = 1.0f / 6.0f;

	a = p1 - p0;
	b = p2 - p0;
	c = p3 - p0;
	cross = CrossProduct( b, c );

	volume *= DotProduct( a, cross );
	if ( volume < 0 )
		return -volume;
	return volume;
}


static float TriangleArea( const Vector &p0, const Vector &p1, const Vector &p2 )
{
	Vector e0 = p1 - p0;
	Vector e1 = p2 - p0;
	Vector cross;

	CrossProduct( e0, e1, cross );
	return 0.5 * cross.Length();
}


//-----------------------------------------------------------------------------
// Purpose: Tetrahedronalize this ledge and compute it's volume in BSP space
// Input  : convex - the ledge
// Output : float - volume in HL units (in^3)
//-----------------------------------------------------------------------------
float CPhysicsCollision::ConvexVolume( CPhysConvex *pConvex )
{
	IVP_Compact_Ledge *pLedge = (IVP_Compact_Ledge *)pConvex;
	int triangleCount = pLedge->get_n_triangles();

	IVP_Compact_Triangle *pTri = pLedge->get_first_triangle();

	Vector vert;
	float volume = 0;
	// vert is in HL units
	LedgeInsidePoint( pLedge, vert );

	for ( int j = 0; j < triangleCount; j++ )
	{
		Vector points[3];
		for ( int k = 0; k < 3; k++ )
		{
			const IVP_Compact_Edge *pEdge = pTri->get_edge( k );
			const IVP_U_Float_Point *pPoint = pEdge->get_start_point( pLedge );
			ConvertPositionToHL( *pPoint, points[k] );
		}
		volume += TetrahedronVolume( vert, points[0], points[1], points[2] );

		pTri = pTri->get_next_tri();
	}

	return volume;
}


float CPhysicsCollision::ConvexSurfaceArea( CPhysConvex *pConvex )
{
	IVP_Compact_Ledge *pLedge = (IVP_Compact_Ledge *)pConvex;
	int triangleCount = pLedge->get_n_triangles();

	IVP_Compact_Triangle *pTri = pLedge->get_first_triangle();

	float area = 0;

	for ( int j = 0; j < triangleCount; j++ )
	{
		Vector points[3];
		for ( int k = 0; k < 3; k++ )
		{
			const IVP_Compact_Edge *pEdge = pTri->get_edge( k );
			const IVP_U_Float_Point *pPoint = pEdge->get_start_point( pLedge );
			ConvertPositionToHL( *pPoint, points[k] );
		}
		area += TriangleArea( points[0], points[1], points[2] );

		pTri = pTri->get_next_tri();
	}

	return area;
}

// Convert an array of convex elements to a compiled collision model (this deletes the convex elements)
CPhysCollide *CPhysicsCollision::ConvertConvexToCollide( CPhysConvex **pConvex, int convexCount )
{
	if ( !convexCount || !pConvex )
		return NULL;

	BEGIN_IVP_ALLOCATION();

	IVP_SurfaceBuilder_Ledge_Soup builder;

	for ( int i = 0; i < convexCount; i++ )
	{
		builder.insert_ledge( (IVP_Compact_Ledge *)pConvex[i] );
	}
	// THIS FREES THE LEDGES!
	IVP_Compact_Surface *pSurface = builder.compile();
	pSurface->dummy[2] = IVP_COMPACT_SURFACE_ID;

	END_IVP_ALLOCATION();

	return reinterpret_cast<CPhysCollide *>(pSurface);
}

static void InitBoxVerts( Vector *boxVerts, Vector **ppVerts, const Vector &mins, const Vector &maxs )
{
	for (int i = 0; i < 8; ++i)
	{
		boxVerts[i][0] = (i & 0x1) ? maxs[0] : mins[0];
		boxVerts[i][1] = (i & 0x2) ? maxs[1] : mins[1];
		boxVerts[i][2] = (i & 0x4) ? maxs[2] : mins[2];
		if ( ppVerts )
		{
			ppVerts[i] = &boxVerts[i];
		}
	}
}


#define FAST_BBOX 1
CPhysCollide *CPhysicsCollision::FastBboxCollide( const CPhysCollide *pCollide, const Vector &mins, const Vector &maxs )
{
	Vector boxVerts[8];
	InitBoxVerts( boxVerts, NULL, mins, maxs );
	// copy the compact ledge at bboxCache 0
	// stuff the verts in there
	const IVP_Compact_Surface *pSurface = ConvertPhysCollideToCompactSurface( pCollide );
	Assert( pSurface );
	const IVP_Compact_Ledgetree_Node *node = pSurface->get_compact_ledge_tree_root();
	Assert( node->is_terminal() == IVP_TRUE );
	const IVP_Compact_Ledge *pLedge = node->get_compact_ledge();
	int ledgeSize = pLedge->get_size();
	IVP_Compact_Ledge *pNewLedge = (IVP_Compact_Ledge *)ivp_malloc_aligned( ledgeSize, 16 );
	memcpy( pNewLedge, pLedge, ledgeSize );
	pNewLedge->set_client_data(0);
	IVP_Compact_Poly_Point *pPoints = pNewLedge->get_point_array();
	for ( int i = 0; i < 8; i++ )
	{
		IVP_U_Float_Hesse ivp;
		ConvertPositionToIVP( boxVerts[m_bboxVertMap[i]], ivp );
		ivp.hesse_val = 0;
		pPoints[i].set4(&ivp);
	}
	CPhysConvex *pConvex = (CPhysConvex *)pNewLedge;
	return ConvertConvexToCollide( &pConvex, 1 );
}

void CPhysicsCollision::InitBBoxCache()
{
	Vector boxVerts[8], *ppVerts[8];
	Vector mins(-16,-16,0), maxs(16,16,72);
	// init with the player box
	InitBoxVerts( boxVerts, ppVerts, mins, maxs );
	// Generate a convex hull from the verts
	CPhysConvex *pConvex = ConvexFromVerts( ppVerts, 8 );
	IVP_Compact_Poly_Point *pPoints = reinterpret_cast<IVP_Compact_Ledge *>(pConvex)->get_point_array();
	for ( int i = 0; i < 8; i++ )
	{
		int nearest = -1;
		float minDist = 0.1;
		Vector tmp;
		ConvertPositionToHL( pPoints[i], tmp );
		for ( int j = 0; j < 8; j++ )
		{
			float dist = (boxVerts[j] - tmp).Length();
			if ( dist < minDist )
			{
				minDist = dist;
				nearest = j;
			}
		}
		
		m_bboxVertMap[i] = nearest;

#if _DEBUG
		for ( int k = 0; k < i; k++ )
		{
			Assert( m_bboxVertMap[k] != m_bboxVertMap[i] );
		}
#endif
		// NOTE: If this is wrong, you can disable FAST_BBOX above to fix
		AssertMsg( nearest != -1, "CPhysCollide: Vert map is wrong\n" );
	}
	CPhysCollide *pCollide = ConvertConvexToCollide( &pConvex, 1 );
	AddBBoxCache( pCollide, mins, maxs );
}

CPhysCollide *CPhysicsCollision::BBoxToCollide( const Vector &mins, const Vector &maxs )
{
	// can't create a collision model for an empty box !
	if ( mins == maxs )
	{
		return NULL;
	}

	// find this bbox in the cache
	CPhysCollide *pCollide = GetBBoxCache( mins, maxs );
	if ( pCollide )
		return pCollide;

	// FAST_BBOX: uses an existing compact ledge as a template for fast generation
	// building convex hulls from points is slow
#if FAST_BBOX
	if ( m_bboxCache.Count() == 0 )
	{
		InitBBoxCache();
	}
	pCollide = FastBboxCollide( m_bboxCache[0].pCollide, mins, maxs );
#else
	Vector boxVerts[8], *ppVerts[8];
	InitBoxVerts( boxVerts, ppVerts, mins, maxs );
	// Generate a convex hull from the verts
	CPhysConvex *pConvex = ConvexFromVerts( ppVerts, 8 );
	pCollide = ConvertConvexToCollide( &pConvex, 1 );
#endif
	AddBBoxCache( pCollide, mins, maxs );
	return pCollide;
}

bool CPhysicsCollision::IsBBoxCache( CPhysCollide *pCollide )
{
	// UNDONE: Sort the list so it can be searched spatially instead of linearly?
	for ( int i = m_bboxCache.Count()-1; i >= 0; i-- )
	{
		if ( m_bboxCache[i].pCollide == pCollide )
			return true;
	}
	return false;
}

void CPhysicsCollision::AddBBoxCache( CPhysCollide *pCollide, const Vector &mins, const Vector &maxs )
{
	int index = m_bboxCache.AddToTail();
	bboxcache_t *pCache = &m_bboxCache[index];
	pCache->pCollide = pCollide;
	pCache->mins = mins;
	pCache->maxs = maxs;
}

CPhysCollide *CPhysicsCollision::GetBBoxCache( const Vector &mins, const Vector &maxs )
{
	for ( int i = m_bboxCache.Count()-1; i >= 0; i-- )
	{
		if ( m_bboxCache[i].mins == mins && m_bboxCache[i].maxs == maxs )
			return m_bboxCache[i].pCollide;
	}
	return NULL;
}


void CPhysicsCollision::ConvexFree( CPhysConvex *pConvex )
{
	ivp_free_aligned( pConvex );
}

// Get the size of the collision model for serialization
int	CPhysicsCollision::CollideSize( CPhysCollide *pCollide )
{
	if ( ConvertPhysCollideToCompactSurface( pCollide ) )
	{
		return ConvertPhysCollideToCompactSurface( pCollide )->byte_size;
	}
#if ENABLE_IVP_MOPP
	else
	{
		return ConvertPhysCollideToCompactMopp( pCollide )->byte_size;
	}
#endif

	return 0;
}

int	CPhysicsCollision::CollideWrite( char *pDest, CPhysCollide *pCollide )
{
	int size = CollideSize(pCollide);
	memcpy( pDest, pCollide, size );
	return size;
}

class CPhysPolysoup
{
public:
	CPhysPolysoup();
#if ENABLE_IVP_MOPP
	IVP_SurfaceBuilder_Mopp m_builder;
#else
	IVP_SurfaceBuilder_Ledge_Soup m_builder;
#endif
    IVP_U_Vector<IVP_U_Point> m_points;
	IVP_U_Point m_triangle[3];

	bool m_isValid;
};

CPhysPolysoup::CPhysPolysoup()
{
	m_isValid = false;
    m_points.add( &m_triangle[0] );
    m_points.add( &m_triangle[1] );
    m_points.add( &m_triangle[2] );
}

CPhysPolysoup *CPhysicsCollision::PolysoupCreate( void )
{
	return new CPhysPolysoup;
}

void CPhysicsCollision::PolysoupDestroy( CPhysPolysoup *pSoup )
{
	delete pSoup;
}

void CPhysicsCollision::PolysoupAddTriangle( CPhysPolysoup *pSoup, const Vector &a, const Vector &b, const Vector &c, int materialIndex7bits )
{
	pSoup->m_isValid = true;
	ConvertPositionToIVP( a, pSoup->m_triangle[0] );
	ConvertPositionToIVP( b, pSoup->m_triangle[1] );
	ConvertPositionToIVP( c, pSoup->m_triangle[2] );
	IVP_Compact_Ledge *pLedge = IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge(&pSoup->m_points);
	if ( !pLedge )
	{
		Warning("Degenerate Triangle\n");
		Warning("(%.2f, %.2f, %.2f), ", a.x, a.y, a.z );
		Warning("(%.2f, %.2f, %.2f), ", b.x, b.y, b.z );
		Warning("(%.2f, %.2f, %.2f)\n", c.x, c.y, c.z );
		return;
	}
	IVP_Compact_Triangle *pTriangle = pLedge->get_first_triangle();
	pTriangle->set_material_index( materialIndex7bits );
	pSoup->m_builder.insert_ledge(pLedge);
}

CPhysCollide *CPhysicsCollision::ConvertPolysoupToCollide( CPhysPolysoup *pSoup )
{
	if ( !pSoup->m_isValid )
		return NULL;

#if ENABLE_IVP_MOPP
    IVP_Compact_Mopp *pSurface = pSoup->m_builder.compile();
	pSurface->dummy = IVP_COMPACT_MOPP_ID;
#else
    IVP_Compact_Surface *pSurface = pSoup->m_builder.compile();
	pSurface->dummy[2] = IVP_COMPACT_SURFACE_ID;
#endif

	// There's a bug in IVP where the duplicated triangles (for 2D)
	// don't get the materials set properly, so copy them
	IVP_U_BigVector<IVP_Compact_Ledge> ledges;
	GetAllLedges( reinterpret_cast<CPhysCollide *>(pSurface), ledges );

	for ( int i = 0; i < ledges.len(); i++ )
	{
		IVP_Compact_Ledge *pLedge = ledges.element_at( i );
		int triangleCount = pLedge->get_n_triangles();

		IVP_Compact_Triangle *pTri = pLedge->get_first_triangle();
		int materialIndex = pTri->get_material_index();
		if ( !materialIndex )
		{
			for ( int j = 0; j < triangleCount; j++ )
			{
				if ( pTri->get_material_index() != 0 )
				{
					materialIndex = pTri->get_material_index();
				}
				pTri = pTri->get_next_tri();
			}
		}
		for ( int j = 0; j < triangleCount; j++ )
		{
			pTri->set_material_index( materialIndex );
			pTri = pTri->get_next_tri();
		}
	}


	return reinterpret_cast<CPhysCollide *>(pSurface);
}

int CPhysicsCollision::CreateDebugMesh( const CPhysCollide *pCollisionModel, Vector **outVerts )
{
	int i;

	IVP_U_BigVector<IVP_Compact_Ledge> ledges;
	GetAllLedges( pCollisionModel, ledges );

	int vertCount = 0;
	
	for ( i = 0; i < ledges.len(); i++ )
	{
		IVP_Compact_Ledge *pLedge = ledges.element_at( i );
		vertCount += pLedge->get_n_triangles() * 3;
	}
	Vector *verts = new Vector[ vertCount ];
		
	int vertIndex = 0;
	for ( i = 0; i < ledges.len(); i++ )
	{
		IVP_Compact_Ledge *pLedge = ledges.element_at( i );
		int triangleCount = pLedge->get_n_triangles();

		IVP_Compact_Triangle *pTri = pLedge->get_first_triangle();
		for ( int j = 0; j < triangleCount; j++ )
		{
			for ( int k = 2; k >= 0; k-- )
			{
				const IVP_Compact_Edge *pEdge = pTri->get_edge( k );
				const IVP_U_Float_Point *pPoint = pEdge->get_start_point( pLedge );

				Vector* pVec = verts + vertIndex;
				ConvertPositionToHL( *pPoint, *pVec );
				vertIndex++;
			}
			pTri = pTri->get_next_tri();
		}
	}

	*outVerts = verts;
	return vertCount;
}


void CPhysicsCollision::DestroyDebugMesh( int vertCount, Vector *outVerts )
{
	delete[] outVerts;
}


void CPhysicsCollision::SetConvexGameData( CPhysConvex *pConvex, unsigned int gameData )
{
	IVP_Compact_Ledge *pLedge = reinterpret_cast<IVP_Compact_Ledge *>( pConvex );
	pLedge->set_client_data( gameData );
}


void CPhysicsCollision::TraceBox( const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr )
{
	m_traceapi.SweepBoxIVP( start, end, mins, maxs, (const IVP_Compact_Surface *)pCollide, collideOrigin, collideAngles, ptr );
}

void CPhysicsCollision::TraceBox( const Ray_t &ray, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr )
{
	TraceBox( ray, MASK_ALL, NULL, pCollide, collideOrigin, collideAngles, ptr );
}

void CPhysicsCollision::TraceBox( const Ray_t &ray, unsigned int contentsMask, IConvexInfo *pConvexInfo, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr )
{
	m_traceapi.SweepBoxIVP( ray, contentsMask, pConvexInfo, (const IVP_Compact_Surface *)pCollide, collideOrigin, collideAngles, ptr );
}

// Trace one collide against another
void CPhysicsCollision::TraceCollide( const Vector &start, const Vector &end, const CPhysCollide *pSweepCollide, const QAngle &sweepAngles, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr )
{
	m_traceapi.SweepIVP( start, end, (const IVP_Compact_Surface *)pSweepCollide, sweepAngles, (const IVP_Compact_Surface *)pCollide, collideOrigin, collideAngles, ptr );
}

void CPhysicsCollision::CollideGetAABB( Vector &mins, Vector &maxs, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles )
{
	m_traceapi.GetAABB( mins, maxs, (const IVP_Compact_Surface *)pCollide, collideOrigin, collideAngles );
}


Vector CPhysicsCollision::CollideGetExtent( const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, const Vector &direction )
{
	if ( !pCollide )
		return collideOrigin;

	return m_traceapi.GetExtent( (const IVP_Compact_Surface *)pCollide, collideOrigin, collideAngles, direction );
}

// Free a collide that was created with ConvertConvexToCollide()
void CPhysicsCollision::DestroyCollide( CPhysCollide *pCollide )
{
	if ( !IsBBoxCache( pCollide ) )
	{
		ivp_free_aligned( pCollide );
	}
}

// calculate the volume of a collide by calling ConvexVolume on its parts
float CPhysicsCollision::CollideVolume( CPhysCollide *pCollide )
{
	IVP_U_BigVector<IVP_Compact_Ledge> ledges;
	GetAllLedges( pCollide, ledges );

	float volume = 0;
	for ( int i = 0; i < ledges.len(); i++ )
	{
		volume += ConvexVolume( (CPhysConvex *)ledges.element_at(i) );
	}

	return volume;
}

// calculate the volume of a collide by calling ConvexVolume on its parts
float CPhysicsCollision::CollideSurfaceArea( CPhysCollide *pCollide )
{
	IVP_U_BigVector<IVP_Compact_Ledge> ledges;
	GetAllLedges( pCollide, ledges );

	float area = 0;
	for ( int i = 0; i < ledges.len(); i++ )
	{
		area += ConvexSurfaceArea( (CPhysConvex *)ledges.element_at(i) );
	}

	return area;
}


// loads a set of solids into a vcollide_t
void CPhysicsCollision::VCollideLoad( vcollide_t *pOutput, int solidCount, const char *pBuffer, int bufferSize )
{
	memset( pOutput, 0, sizeof(*pOutput) );
	int position = 0;

	pOutput->solidCount = solidCount;
	pOutput->solids = new CPhysCollide *[solidCount];

	BEGIN_IVP_ALLOCATION();

	for ( int i = 0; i < solidCount; i++ )
	{
		int size;
		memcpy( &size, pBuffer + position, sizeof(int) );
		position += sizeof(int);

		pOutput->solids[i] = (CPhysCollide *)ivp_malloc_aligned( size, 32 );
		memcpy( pOutput->solids[i], pBuffer + position, size );
		position += size;
	}

	END_IVP_ALLOCATION();

	int keySize = bufferSize - position;

	pOutput->pKeyValues = new char[keySize];
	memcpy( pOutput->pKeyValues, pBuffer + position, keySize );
}

// destroyts the set of solids created by VCollideCreateCPhysCollide
void CPhysicsCollision::VCollideUnload( vcollide_t *pVCollide )
{
	for ( int i = 0; i < pVCollide->solidCount; i++ )
	{
#if _DEBUG
		// HACKHACK: 1024 is just "some big number"
		// GetActiveEnvironmentByIndex() will eventually return NULL when there are no more environments.
		// In HL2 & TF2, there are only 2 environments - so j > 1 is probably an error!
		for ( int j = 0; j < 1024; j++ )
		{
			IPhysicsEnvironment *pEnv = g_PhysicsInternal->GetActiveEnvironmentByIndex( j );
			if ( !pEnv )
				break;

			if ( pEnv->IsCollisionModelUsed( (CPhysCollide *)pVCollide->solids[i] ) )
			{
 				AssertMsg(0, "Freed collision model while in use!!!\n");
				return;
			}
		}
#endif
		ivp_free_aligned( pVCollide->solids[i] );
	}
	delete[] pVCollide->solids;
	delete[] pVCollide->pKeyValues;
	memset( pVCollide, 0, sizeof(*pVCollide) );
}

// begins parsing a vcollide.  NOTE: This keeps pointers to the vcollide_t
// If you delete the vcollide_t and call members of IVCollideParse, it will crash
IVPhysicsKeyParser *CPhysicsCollision::VPhysicsKeyParserCreate( const char *pKeyData )
{
	return CreateVPhysicsKeyParser( pKeyData );
}

// Free the parser created by VPhysicsKeyParserCreate
void CPhysicsCollision::VPhysicsKeyParserDestroy( IVPhysicsKeyParser *pParser )
{
	DestroyVPhysicsKeyParser( pParser );
}

IPhysicsCollision *CPhysicsCollision::ThreadContextCreate( void )
{ 
	return new CPhysicsCollision; 
}

void CPhysicsCollision::ThreadContextDestroy( IPhysicsCollision *pThreadContext )
{

	delete (CPhysicsCollision *)pThreadContext;
}


class CCollisionQuery : public ICollisionQuery
{
public:
	CCollisionQuery( CPhysCollide *pCollide );
	~CCollisionQuery( void ) {}

	// number of convex pieces in the whole solid
	virtual int		ConvexCount( void );
	// triangle count for this convex piece
	virtual int		TriangleCount( int convexIndex );
	
	// get the stored game data
	virtual unsigned int GetGameData( int convexIndex );

	// Gets the triangle's verts to an array
	virtual void	GetTriangleVerts( int convexIndex, int triangleIndex, Vector *verts );
	
	// UNDONE: This doesn't work!!!
	virtual void	SetTriangleVerts( int convexIndex, int triangleIndex, const Vector *verts );
	
	// returns the 7-bit material index
	virtual int		GetTriangleMaterialIndex( int convexIndex, int triangleIndex );
	// sets a 7-bit material index for this triangle
	virtual void	SetTriangleMaterialIndex( int convexIndex, int triangleIndex, int index7bits );

private:
	IVP_Compact_Triangle *Triangle( IVP_Compact_Ledge *pLedge, int triangleIndex );

	IVP_U_BigVector <IVP_Compact_Ledge> m_ledges;
};


// create a queryable version of the collision model
ICollisionQuery *CPhysicsCollision::CreateQueryModel( CPhysCollide *pCollide )
{
	return new CCollisionQuery( pCollide );
}

	// destroy the queryable version
void CPhysicsCollision::DestroyQueryModel( ICollisionQuery *pQuery )
{
	delete pQuery;
}


CCollisionQuery::CCollisionQuery( CPhysCollide *pCollide )
{
	GetAllLedges( pCollide, m_ledges );
}


	// number of convex pieces in the whole solid
int	CCollisionQuery::ConvexCount( void )
{
	return m_ledges.len();
}

	// triangle count for this convex piece
int CCollisionQuery::TriangleCount( int convexIndex )
{
	IVP_Compact_Ledge *pLedge = m_ledges.element_at(convexIndex);
	if ( pLedge )
	{
		return pLedge->get_n_triangles();
	}

	return 0;
}


unsigned int CCollisionQuery::GetGameData( int convexIndex )
{
	IVP_Compact_Ledge *pLedge = m_ledges.element_at( convexIndex );
	if ( pLedge )
		return pLedge->get_client_data();
	return 0;
}

	// Gets the triangle's verts to an array
void CCollisionQuery::GetTriangleVerts( int convexIndex, int triangleIndex, Vector *verts )
{
	IVP_Compact_Ledge *pLedge = m_ledges.element_at( convexIndex );
	IVP_Compact_Triangle *pTriangle = Triangle( pLedge, triangleIndex );

	int vertIndex = 0;
	for ( int k = 2; k >= 0; k-- )
	{
		const IVP_Compact_Edge *pEdge = pTriangle->get_edge( k );
		const IVP_U_Float_Point *pPoint = pEdge->get_start_point( pLedge );

		Vector* pVec = verts + vertIndex;
		ConvertPositionToHL( *pPoint, *pVec );
		vertIndex++;
	}
}

	
	// UNDONE: This doesn't work!!!
void CCollisionQuery::SetTriangleVerts( int convexIndex, int triangleIndex, const Vector *verts )
{
	IVP_Compact_Ledge *pLedge = m_ledges.element_at( convexIndex );
	Triangle( pLedge, triangleIndex );
}

	
int CCollisionQuery::GetTriangleMaterialIndex( int convexIndex, int triangleIndex )
{
	IVP_Compact_Ledge *pLedge = m_ledges.element_at( convexIndex );
	IVP_Compact_Triangle *pTriangle = Triangle( pLedge, triangleIndex );

	return pTriangle->get_material_index();
}

void CCollisionQuery::SetTriangleMaterialIndex( int convexIndex, int triangleIndex, int index7bits )
{
	IVP_Compact_Ledge *pLedge = m_ledges.element_at( convexIndex );
	IVP_Compact_Triangle *pTriangle = Triangle( pLedge, triangleIndex );

	pTriangle->set_material_index( index7bits );
}


IVP_Compact_Triangle *CCollisionQuery::Triangle( IVP_Compact_Ledge *pLedge, int triangleIndex )
{
	if ( !pLedge )
		return NULL;

	return pLedge->get_first_triangle() + triangleIndex;
}


#if 0
void TestCubeVolume( void )
{
	float volume = 0;
	Vector verts[8];
	typedef struct 
	{
		int a, b, c;
	} triangle_t;

	triangle_t triangles[12] = 
	{
		{0,1,3}, // front	0123
		{0,3,2},
		{4,5,1}, // top		4501
		{4,1,0},
		{2,3,7}, // bottom	2367
		{2,7,6},
		{1,5,7}, // right	1537
		{1,7,3},
		{4,0,2}, // left	4062
		{4,2,6},
		{5,4,6}, // back	5476
		{5,6,7}
	};

	int i = 0;
	for ( int x = -1; x <= 1; x +=2 )
		for ( int y = -1; y <= 1; y +=2 )
			for ( int z = -1; z <= 1; z +=2 )
			{
				verts[i][0] = x;
				verts[i][1] = y;
				verts[i][2] = z;
				i++;
			}


	for ( i = 0; i < 12; i++ )
	{
		triangle_t *pTri = triangles + i;
		volume += TetrahedronVolume( verts[0], verts[pTri->a], verts[pTri->b], verts[pTri->c] );
	}
	// should report a volume of 8.  This is a cube that is 2 on a side
	printf("Test volume %.4f\n", volume );
}
#endif

