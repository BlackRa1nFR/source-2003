//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Common collision utility methods
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "collisionutils.h"
#include "cmodel.h"
#include "mathlib.h"
#include "vector.h"
#include "tier0/dbg.h"
#include <float.h>
#include "vector4d.h"

#define UNINIT		-99999.0


//-----------------------------------------------------------------------------
// Compute the offset in t along the ray that we'll use for the collision
//-----------------------------------------------------------------------------
static float ComputeBoxOffset( const Ray_t& ray )
{
	if (ray.m_IsRay)
		return 1e-3f;

	// Find the projection of the box diagonal along the ray...
	float offset = FloatMakePositive(ray.m_Extents[0] * ray.m_Delta[0]) +
					FloatMakePositive(ray.m_Extents[1] * ray.m_Delta[1]) +
					FloatMakePositive(ray.m_Extents[2] * ray.m_Delta[2]);

	// We need to divide twice: Once to normalize the computation above
	// so we get something in units of extents, and the second to normalize
	// that with respect to the entire raycast.
	offset *= InvRSquared( ray.m_Delta );

	// 1e-3 is an epsilon
	return offset + 1e-3;
}


//-----------------------------------------------------------------------------
// Intersects a swept box against a triangle
//-----------------------------------------------------------------------------
float IntersectRayWithTriangle( const Ray_t& ray, 
		const Vector& v1, const Vector& v2, const Vector& v3, bool oneSided )
{
	// This is cute: Use barycentric coordinates to represent the triangle
	// Vo(1-u-v) + V1u + V2v and intersect that with a line Po + Dt
	// This gives us 3 equations + 3 unknowns, which we can solve with
	// Cramer's rule...
	//		E1x u + E2x v - Dx t = Pox - Vox
	// There's a couple of other optimizations, Cramer's rule involves
	// computing the determinant of a matrix which has been constructed
	// by three vectors. It turns out that 
	// det | A B C | = -( A x C ) dot B or -(C x B) dot A
	// which we'll use below..

	Vector edge1, edge2, org;
	VectorSubtract( v2, v1, edge1 );
	VectorSubtract( v3, v1, edge2 );

	// Cull out one-sided stuff
	if (oneSided)
	{
		Vector normal;
		CrossProduct( edge1, edge2, normal );
		if (DotProduct( normal, ray.m_Delta ) >= 0.0f)
			return -1.0f;
	}

	// FIXME: This is inaccurate, but fast for boxes
	// We want to do a fast separating axis implementation here
	// with a swept triangle along the reverse direction of the ray.

	// Compute some intermediary terms
	Vector dirCrossEdge2, orgCrossEdge1;
	CrossProduct( ray.m_Delta, edge2, dirCrossEdge2 );

	// Compute the denominator of Cramer's rule:
	//		| -Dx E1x E2x |
	// det	| -Dy E1y E2y | = (D x E2) dot E1
	//		| -Dz E1z E2z |
	float denom = DotProduct( dirCrossEdge2, edge1 );
	if( FloatMakePositive( denom ) < 1e-6 )
		return -1.0f;
	denom = 1.0f / denom;

	// Compute u. It's gotta lie in the range of 0 to 1.
	//				   | -Dx orgx E2x |
	// u = denom * det | -Dy orgy E2y | = (D x E2) dot org
	//				   | -Dz orgz E2z |
	VectorSubtract( ray.m_Start, v1, org );
	float u = DotProduct( dirCrossEdge2, org ) * denom;
	if ((u < 0.0f) || (u > 1.0f))
		return -1.0f;

	// Compute t and v the same way...
	// In barycentric coords, u + v < 1
	CrossProduct( org, edge1, orgCrossEdge1 );
	float v = DotProduct( orgCrossEdge1, ray.m_Delta ) * denom;
	if ((v < 0.0f) || (v + u > 1.0f))
		return -1.0f;

	// Compute the distance along the ray direction that we need to fudge 
	// when using swept boxes
	float boxt = ComputeBoxOffset( ray );
	float t = DotProduct( orgCrossEdge1, edge2 ) * denom;
	if ((t < -boxt) || (t > 1.0f + boxt))
		return -1.0f;

	return clamp( t, 0, 1 );
}


//-----------------------------------------------------------------------------
// computes the barycentric coordinates of an intersection
//-----------------------------------------------------------------------------

bool ComputeIntersectionBarycentricCoordinates( const Ray_t& ray, 
		const Vector& v1, const Vector& v2, const Vector& v3, float& u, float& v,
		float *t )
{
	Vector edge1, edge2, org;
	VectorSubtract( v2, v1, edge1 );
	VectorSubtract( v3, v1, edge2 );

	// Compute some intermediary terms
	Vector dirCrossEdge2, orgCrossEdge1;
	CrossProduct( ray.m_Delta, edge2, dirCrossEdge2 );

	// Compute the denominator of Cramer's rule:
	//		| -Dx E1x E2x |
	// det	| -Dy E1y E2y | = (D x E2) dot E1
	//		| -Dz E1z E2z |
	float denom = DotProduct( dirCrossEdge2, edge1 );
	if( FloatMakePositive( denom ) < 1e-6 )
		return false;
	denom = 1.0f / denom;

	// Compute u. It's gotta lie in the range of 0 to 1.
	//				   | -Dx orgx E2x |
	// u = denom * det | -Dy orgy E2y | = (D x E2) dot org
	//				   | -Dz orgz E2z |
	VectorSubtract( ray.m_Start, v1, org );
	u = DotProduct( dirCrossEdge2, org ) * denom;

	// Compute t and v the same way...
	// In barycentric coords, u + v < 1
	CrossProduct( org, edge1, orgCrossEdge1 );
	v = DotProduct( orgCrossEdge1, ray.m_Delta ) * denom;

	// Compute the distance along the ray direction that we need to fudge 
	// when using swept boxes
	if( t )
	{
		float boxt = ComputeBoxOffset( ray );
		*t = DotProduct( orgCrossEdge1, edge2 ) * denom;
		if( ( *t < -boxt ) || ( *t > 1.0f + boxt ) )
			return false;
	}

	return true;
}


#if 0
#define SA_EPSILON	0.0f 

//--------------------------------------------------------------------------
// Purpose:
//
// NOTE:
//	triangle points are given in clockwise order
//
//    1				edge0 = 1 - 0
//    | \           edge1 = 2 - 1
//    |  \          edge2 = 0 - 2
//    |   \
//    |    \
//    0-----2
//
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossX2( float edgeZ, float edgeY, float absEdgeZ, float absEdgeY,
							     const Vector &p1, const Vector &p3, const Vector &extents )
{
	//
	// Cross Product( axialX(1,0,0) x edge ):
	//   normal.x = 0.0f
	//   normal.y = edge.z
	//   normal.z = -edge.y
	//
	// Triangle Point Distances:
	//   dist(x) = normal.y * pt(x).y + normal.z * pt(x).z
	//
	float dist1 = edgeZ * p1.y - edgeY * p1.z;
	float dist3 = edgeZ * p3.y - edgeY * p3.z;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.y ) * extents.y + abs( normal.z ) * extents.z
	//
	float distBox = absEdgeZ * extents.y + absEdgeY * extents.z;

	//
	// either dist1, dist3 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//
	if( dist1 < dist3 )
	{
		if( ( dist1 > ( distBox + SA_EPSILON ) ) || ( dist3 < -( distBox + SA_EPSILON ) ) )
			return false;
	}
	else
	{
		if( ( dist3 > ( distBox + SA_EPSILON ) ) || ( dist1 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossX3( float edgeZ, float edgeY, float absEdgeZ, float absEdgeY,
								 const Vector &p1, const Vector &p2, const Vector &extents )
{
	//
	// Cross Product( axialX(1,0,0) x edge ):
	//   normal.x = 0.0f
	//   normal.y = edge.z
	//   normal.z = -edge.y
	//
	// Triangle Point Distances:
	//   dist(x) = normal.y * pt(x).y + normal.z * pt(x).z
	//
	float dist1 = edgeZ * p1.y - edgeY * p1.z;
	float dist2 = edgeZ * p2.y - edgeY * p2.z;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.y ) * extents.y + abs( normal.z ) * extents.z
	//
	float distBox = absEdgeZ * extents.y + absEdgeY * extents.z;

	//
	// either dist1, dist2 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//	
	if( dist1 < dist2 )
	{
		if( ( dist1 > ( distBox + SA_EPSILON ) ) || ( dist2 < -( distBox + SA_EPSILON ) ) )
			return false;
	}
	else
	{
		if( ( dist2 > ( distBox + SA_EPSILON ) ) || ( dist1 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossY2( float edgeZ, float edgeX, float absEdgeZ, float absEdgeX,
								 const Vector &p1, const Vector &p3, const Vector &extents )
{
	//
	// Cross Product( axialY(0,1,0) x edge ):
	//   normal.x = -edge.z
	//   normal.y = 0.0f
	//   normal.z = edge.x
	//
	// Triangle Point Distances:
	//   dist(x) = normal.x * pt(x).x + normal.z * pt(x).z
	//
	float dist1 = -edgeZ * p1.x + edgeX * p1.z;
	float dist3 = -edgeZ * p3.x + edgeX * p3.z;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.x ) * extents.x + abs( normal.z ) * extents.z
	//
	float distBox = absEdgeZ * extents.x + absEdgeX * extents.z;

	//
	// either dist1, dist3 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//	
	if( dist1 < dist3 )
	{
		if( ( dist1 > distBox ) || ( dist3 < -( distBox + SA_EPSILON ) ) )
			return false;
	}
	else
	{
		if( ( dist3 > ( distBox + SA_EPSILON ) ) || ( dist1 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossY3( float edgeZ, float edgeX, float absEdgeZ, float absEdgeX,
								 const Vector &p1, const Vector &p2, const Vector &extents )
{
	//
	// Cross Product( axialY(0,1,0) x edge ):
	//   normal.x = -edge.z
	//   normal.y = 0.0f
	//   normal.z = edge.x
	//
	// Triangle Point Distances:
	//   dist(x) = normal.x * pt(x).x + normal.z * pt(x).z
	//
	float dist1 = -edgeZ * p1.x + edgeX * p1.z;
	float dist2 = -edgeZ * p2.x + edgeX * p2.z;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.x ) * extents.x + abs( normal.z ) * extents.z
	//
	float distBox = absEdgeZ * extents.x + absEdgeX * extents.z;

	//
	// either dist1, dist2 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//	
	if( dist1 < dist2 )
	{
		if( ( dist1 > ( distBox + SA_EPSILON ) ) || ( dist2 < -( distBox + SA_EPSILON ) ) )
			return false;
	}
	else
	{
		if( ( dist2 > ( distBox + SA_EPSILON ) ) || ( dist1 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossZ1( float edgeY, float edgeX, float absEdgeY, float absEdgeX,
								 const Vector &p2, const Vector &p3, const Vector &extents )
{
	//
	// Cross Product( axialZ(0,0,1) x edge ):
	//   normal.x = edge.y
	//   normal.y = -edge.x
	//   normal.z = 0.0f
	//
	// Triangle Point Distances:
	//   dist(x) = normal.x * pt(x).x + normal.y * pt(x).y
	//
	float dist2 = edgeY * p2.x - edgeX * p2.y;
	float dist3 = edgeY * p3.x - edgeX * p3.y;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.x ) * extents.x + abs( normal.y ) * extents.y
	//
	float distBox = absEdgeY * extents.x + absEdgeX * extents.y; 

	//
	// either dist2, dist3 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//	
	if( dist3 < dist2 )
	{
		if( ( dist3 > ( distBox + SA_EPSILON ) ) || ( dist2 < -( distBox + SA_EPSILON ) ) ) 
			return false;
	}
	else
	{
		if( ( dist2 > ( distBox + SA_EPSILON ) ) || ( dist3 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossZ2( float edgeY, float edgeX, float absEdgeY, float absEdgeX,
								 const Vector &p1, const Vector &p3, const Vector &extents )
{
	//
	// Cross Product( axialZ(0,0,1) x edge ):
	//   normal.x = edge.y
	//   normal.y = -edge.x
	//   normal.z = 0.0f
	//
	// Triangle Point Distances:
	//   dist(x) = normal.x * pt(x).x + normal.y * pt(x).y
	//
	float dist1 = edgeY * p1.x - edgeX * p1.y;
	float dist3 = edgeY * p3.x - edgeX * p3.y;

	//
	// take advantage of extents symmetry
	//   dist = abs( normal.x ) * extents.x + abs( normal.y ) * extents.y
	//
	float distBox = absEdgeY * extents.x + absEdgeX * extents.y; 

	//
	// either dist1, dist3 is the closest point to the box, determine which
	// and test of overlap with box(AABB)
	//	
	if( dist1 < dist3 )
	{
		if( ( dist1 > ( distBox + SA_EPSILON ) ) || ( dist3 < -( distBox + SA_EPSILON ) ) ) 
			return false;
	}
	else
	{
		if( ( dist3 > ( distBox + SA_EPSILON ) ) || ( dist1 < -( distBox + SA_EPSILON ) ) )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: find the minima and maxima of the 3 given values
//-----------------------------------------------------------------------------
inline void FindMinMax( float v1, float v2, float v3, float &min, float &max )
{
	min = max = v1;
	if( v2 < min ) { min = v2; }
	if( v2 > max ) { max = v2; }
	if( v3 < min ) { min = v3; }
	if( v3 > max ) { max = v3; }
}


//--------------------------------------------------------------------------
// Purpose: test for intersection between a plane and axial-aligned box
//--------------------------------------------------------------------------
inline bool OverlapPlaneAABox( const Vector &planeNormal, float planeDist,
								 const Vector &boxCenter, const Vector &boxExtents )
{
	//
	// find the closest box point to the plane
	//
	Vector closePt;
	for( int ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
	{
		if( planeNormal[ndxAxis] < 0.0f )
		{
			closePt[ndxAxis] = boxCenter[ndxAxis] + boxExtents[ndxAxis];
		}
		else
		{
			closePt[ndxAxis] = boxCenter[ndxAxis] - boxExtents[ndxAxis];
		}
	}

	float dist = planeNormal.Dot( closePt ) - planeDist;
	if( dist > SA_EPSILON/*0.0f*/ )
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Test for an intersection (overlap) between an axial-aligned bounding 
//          box (AABB) and a triangle.
//
// Using the "Separating-Axis Theorem" to test for intersections between
// a triangle and an axial-aligned bounding box (AABB).
// 1. 4 Face Plane Tests - these are defined by the axial planes (x,y,z) making 
//                         up the AABB and the triangle plane
// 2. 9 Edge Planes Tests - the 3 edges of the triangle crossed with all 3 axial 
//                          planes (x, y, z)
// Output: false = separating axis (no intersection)
//         true = intersection
//-----------------------------------------------------------------------------
bool SeparatingAxisAABoxTriangle( const Vector &boxCenter, const Vector &boxExtents,
						          const Vector &v1, const Vector &v2, const Vector &v3,
						          const Vector &planeNormal, float planeDist )
{
	//
	// test against the triangle face plane
	//
	if( !OverlapPlaneAABox( planeNormal, planeDist, boxCenter, boxExtents ) ) { return false; }

	//
	// test the axial planes (x,y,z) against the min, max of the triangle
	//
	float min, max;
	Vector p1, p2, p3;

	// x plane
	p1.x = v1.x - boxCenter.x;
	p2.x = v2.x - boxCenter.x;
	p3.x = v3.x - boxCenter.x;
	FindMinMax( p1.x, p2.x, p3.x, min, max );
	if( ( min > ( boxExtents.x + SA_EPSILON ) ) || ( max < -( boxExtents.x + SA_EPSILON ) ) ) { return false; }

	// y plane
	p1.y = v1.y - boxCenter.y;
	p2.y = v2.y - boxCenter.y;
	p3.y = v3.y - boxCenter.y;
	FindMinMax( p1.y, p2.y, p3.y, min, max );
	if( ( min > ( boxExtents.y + SA_EPSILON ) ) || ( max < -( boxExtents.y + SA_EPSILON ) ) ) { return false; }

	// z plane
	p1.z = v1.z - boxCenter.z;
	p2.z = v2.z - boxCenter.z;
	p3.z = v3.z - boxCenter.z;
	FindMinMax( p1.z, p2.z, p3.z, min, max );
	if( ( min > ( boxExtents.z + SA_EPSILON ) ) || ( max < -( boxExtents.z + SA_EPSILON ) ) ) { return false; }

	//
	// test the 9 edge cases
	//
	Vector edge, absEdge;

	// edge 0 (cross x,y,z)
	edge = p2 - p1;
	absEdge.y = FloatMakePositive( edge.y );
	absEdge.z = FloatMakePositive( edge.z );
	if( !AxisTestEdgeCrossX2( edge.z, edge.y, absEdge.z, absEdge.y, p1, p3, boxExtents ) ) { return false; }
	absEdge.x = FloatMakePositive( edge.x );
	if( !AxisTestEdgeCrossY2( edge.z, edge.x, absEdge.z, absEdge.x, p1, p3, boxExtents ) ) { return false; }
	if( !AxisTestEdgeCrossZ1( edge.y, edge.x, absEdge.y, absEdge.x, p2, p3, boxExtents ) ) { return false; }

	// edge 1 (cross x,y,z)
	edge = p3 - p2;
	absEdge.y = FloatMakePositive( edge.y );
	absEdge.z = FloatMakePositive( edge.z );
	if( !AxisTestEdgeCrossX2( edge.z, edge.y, absEdge.z, absEdge.y, p1, p2, boxExtents ) ) { return false; }
	absEdge.x = FloatMakePositive( edge.x );
	if( !AxisTestEdgeCrossY2( edge.z, edge.x, absEdge.z, absEdge.x, p1, p2, boxExtents ) ) { return false; }
	if( !AxisTestEdgeCrossZ2( edge.y, edge.x, absEdge.y, absEdge.x, p1, p3, boxExtents ) ) { return false; }

	// edge 2 (cross x,y,z)
	edge = p1 - p3;
	absEdge.y = FloatMakePositive( edge.y );
	absEdge.z = FloatMakePositive( edge.z );
	if( !AxisTestEdgeCrossX3( edge.z, edge.y, absEdge.z, absEdge.y, p1, p2, boxExtents ) ) { return false; }
	absEdge.x = FloatMakePositive( edge.x );
	if( !AxisTestEdgeCrossY3( edge.z, edge.x, absEdge.z, absEdge.x, p1, p2, boxExtents ) ) { return false; }
	if( !AxisTestEdgeCrossZ1( edge.y, edge.x, absEdge.y, absEdge.x, p2, p3, boxExtents ) ) { return false; }

	return true;
}
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CalcClosestBoxPoint( const Vector &planeNormal, const Vector &boxStart, 
						         const Vector &boxExtents, Vector &boxPt )
{
	boxPt = boxStart;
	( planeNormal[0] < 0.0f ) ? boxPt[0] += boxExtents[0] : boxPt[0] -= boxExtents[0];
	( planeNormal[1] < 0.0f ) ? boxPt[1] += boxExtents[1] : boxPt[1] -= boxExtents[1];
	( planeNormal[2] < 0.0f ) ? boxPt[2] += boxExtents[2] : boxPt[2] -= boxExtents[2];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool SeparatingAxisAABoxSweptTriangle( const Vector &boxStart, const Vector &boxEnd,
								       const Vector &boxExtents, const Vector &v1,
								       const Vector &v2, const Vector &v3,
								       const Vector &triNormal, float triDist,
								       bool bStartOutside )
{
	//
	// make sure the box and triangle are not initially intersecting!!
	// NOTE: if bStartOutside is set -- this is assumed
	//
	if( !bStartOutside )
	{
		// check for interection -- if not intersecting continue, otherwise
		// return and let the "in solid" functions handle it
	}

	// get ray direction
	Vector rayDir = boxEnd - boxStart;

	//
	// make sure objects are traveling toward one another
	//
	if( triNormal.Dot( rayDir ) > 0.0f )
		return false;

	//
	// sweep the triangle in the opposite direction of travel
	//
	Vector sv1, sv2, sv3;
	sv1 = v1 - rayDir;
	sv2 = v2 - rayDir;
	sv3 = v3 - rayDir;

	//
	// test axial planes (x, y, z)
	//
	float dist;
	bool bOutside;
	for( int ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
	{
		// assume triangle will lie outside of the given plane
		bOutside = true;

		if( rayDir[ndxAxis] > 0.0f )
		{
			dist = boxStart[ndxAxis] + boxExtents[ndxAxis];

			if( sv1[ndxAxis] < dist ) { bOutside = false; continue; }
			if( sv2[ndxAxis] < dist ) { bOutside = false; continue; }
			if( sv3[ndxAxis] < dist ) { bOutside = false; continue; }
		}
		else
		{
			dist = boxStart[ndxAxis] - boxExtents[ndxAxis];

			if( sv1[ndxAxis] > dist ) { bOutside = false; continue; }
			if( sv2[ndxAxis] > dist ) { bOutside = false; continue; }
			if( sv3[ndxAxis] > dist ) { bOutside = false; continue; }
		}

		if( bOutside )
			return false;
	}

	//
	// test 9 edges
	//
	Vector normal;
	Vector edge;
	Vector boxPt;
	
	//
	// edge 1
	//
	edge = sv2 - sv1;

	// edge1 cross axial-x = ( 0.0, edgeZ, -edgeY )
	normal.x = 0.0f;  normal.y = edge.z;  normal.z = -edge.y;
	CalcClosestBoxPoint( normal, boxStart, boxExtents, boxPt );
	if( ( ( normal.y * ( boxPt.y - sv1.y ) ) + ( normal.z * ( boxPt.z - sv1.z ) ) ) > 0.0f ) { return false; }

	// edge1 cross axial-y = ( -edgeZ, 0.0, edgeX )
	normal.x = -edge.z;  normal.y = 0.0f;  normal.z = edge.x;
	CalcClosestBoxPoint( normal, boxStart, boxExtents, boxPt );
	if( ( ( normal.x * ( boxPt.x - sv1.x ) ) + ( normal.z * ( boxPt.z - sv1.z ) ) ) > 0.0f ) { return false; }

	// edge1 cross axial-z = ( edgeY, -edgeX, 0.0 )
	normal.x = edge.y;  normal.y = -edge.x;  normal.z = 0.0f;
	CalcClosestBoxPoint( normal, boxStart, boxExtents, boxPt );
	if( ( ( normal.x * ( boxPt.x - sv1.x ) ) + ( normal.y * ( boxPt.y - sv1.y ) ) ) > 0.0f ) { return false; }

	//
	// edge 2
	//
	edge = sv3 - sv2;

	// edge1 cross axial-x = ( 0.0, edgeZ, -edgeY )
	normal.x = 0.0f;  normal.y = edge.z;  normal.z = -edge.y;
	CalcClosestBoxPoint( normal, boxStart, boxExtents, boxPt );
	if( ( ( normal.y * ( boxPt.y - sv2.y ) ) + ( normal.z * ( boxPt.z - sv2.z ) ) ) > 0.0f ) { return false; }

	// edge1 cross axial-y = ( -edgeZ, 0.0, edgeX )
	normal.x = -edge.z;  normal.y = 0.0f;  normal.z = edge.x;
	CalcClosestBoxPoint( normal, boxStart, boxExtents, boxPt );
	if( ( ( normal.x * ( boxPt.x - sv2.x ) ) + ( normal.z * ( boxPt.z - sv2.z ) ) ) > 0.0f ) { return false; }

	// edge1 cross axial-z = ( edgeY, -edgeX, 0.0 )
	normal.x = edge.y;  normal.y = -edge.x;  normal.z = 0.0f;
	CalcClosestBoxPoint( normal, boxStart, boxExtents, boxPt );
	if( ( ( normal.x * ( boxPt.x - sv2.x ) ) + ( normal.y * ( boxPt.y - sv2.y ) ) ) > 0.0f ) { return false; }

	//
	// edge 3
	//
	edge = sv1 - sv3;

	// edge1 cross axial-x = ( 0.0, edgeZ, -edgeY )
	normal.x = 0.0f;  normal.y = edge.z;  normal.z = -edge.y;
	CalcClosestBoxPoint( normal, boxStart, boxExtents, boxPt );
	if( ( ( normal.y * ( boxPt.y - sv1.y ) ) + ( normal.z * ( boxPt.z - sv1.z ) ) ) > 0.0f ) { return false; }

	// edge1 cross axial-y = ( -edgeZ, 0.0, edgeX )
	normal.x = -edge.z;  normal.y = 0.0f;  normal.z = edge.x;
	CalcClosestBoxPoint( normal, boxStart, boxExtents, boxPt );
	if( ( ( normal.x * ( boxPt.x - sv1.x ) ) + ( normal.z * ( boxPt.z - sv1.z ) ) ) > 0.0f ) { return false; }

	// edge1 cross axial-z = ( edgeY, -edgeX, 0.0 )
	normal.x = edge.y;  normal.y = -edge.x;  normal.z = 0.0f;
	CalcClosestBoxPoint( normal, boxStart, boxExtents, boxPt );
	if( ( ( normal.x * ( boxPt.x - sv1.x ) ) + ( normal.y * ( boxPt.y - sv1.y ) ) ) > 0.0f ) { return false; }

	//
	// triangle face plane test
	//
	CalcClosestBoxPoint( triNormal, boxStart, boxExtents, boxPt );
	if( ( ( triNormal.x * ( boxPt.x - sv1.x ) ) +
		  ( triNormal.y * ( boxPt.y - sv1.y ) ) +
		  ( triNormal.z * ( boxPt.z - sv1.z ) ) ) > 0.0f )
		  return false;

	return true;
}


#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void FindMin( float v1, float v2, float v3, float &min )
{
	min = v1;
	if( v2 < min ) { min = v2; }
	if( v3 < min ) { min = v3; }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void FindMax( float v1, float v2, float v3, float &max )
{
	max = v1;
	if( v2 > max ) { max = v2; }
	if( v3 > max ) { max = v3; }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline CalcClosestExtents( const Vector &planeNormal, const Vector &boxExtents, 
						   Vector &boxPt )
{
	( planeNormal[0] < 0.0f ) ? boxPt[0] = boxExtents[0] : boxPt[0] = -boxExtents[0];
	( planeNormal[1] < 0.0f ) ? boxPt[1] = boxExtents[1] : boxPt[1] = -boxExtents[1];
	( planeNormal[2] < 0.0f ) ? boxPt[2] = boxExtents[2] : boxPt[2] = -boxExtents[2];
}


#define COLLISION_EPSILON   0.00001f

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool ResolveDistances( float distStart, float distEnd,
							  float &fracStart, float &fracEnd )
{
	if( ( distStart > COLLISION_EPSILON ) && ( distEnd < -COLLISION_EPSILON ) ) 
	{
		// update fraction
		float t = ( distStart - COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t > fracStart )
		{
			fracStart = t;
		}
	}
	else if( ( distStart < -COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		// update fraction
		float t = ( distStart + COLLISION_EPSILON ) / ( distStart - distEnd );
		if( t < fracEnd )
		{
			fracEnd = t;
		}
	}
	else if( ( distStart > COLLISION_EPSILON ) && ( distEnd > COLLISION_EPSILON ) )
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float IntersectAABoxSweptTriangle( const Vector &boxStart, const Vector &boxEnd,
								   const Vector &boxExtents, const Vector &v1,
								   const Vector &v2, const Vector &v3,
								   const Vector &triNormal, float triDist,
								   bool bStartOutside )
{
	//
	// make sure the box and triangle are not initially intersecting!!
	// NOTE: if bStartOutside is set -- this is assumed
	//
	if( !bStartOutside )
	{
		// check for interection -- if not intersecting continue, otherwise
		// return and let the "in solid" functions handle it
	}

	//
	// make sure objects are traveling toward one another
	//
	Vector rDir = boxEnd - boxStart;
	if( triNormal.Dot( rDir ) > 0.0f )
		return -1.0f;

#if 1
	for( int i = 0; i < 3; i++ )
	{
		if( triNormal[i] > 0.1f && triNormal[i] < 0.9 )
			_asm int 3;
	}
#endif

	// initialize the fraction - 1.0f = no intersection
	float fracStart = -1.0f;
	float fracEnd = 1.0f;
	
	// copy these as they will get changed!!!
	Vector rStart = boxStart;
	Vector rEnd = boxEnd;

	//
	// test axial planes (x, y, z)
	//
	float distStart, distEnd;
	float dist;
	Vector normal;
	for( int ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
	{
		if( rDir[ndxAxis] > 0.0f )
		{
			FindMin( v1[ndxAxis], v2[ndxAxis], v3[ndxAxis], dist );
			dist -= boxExtents[ndxAxis];
			distStart = -boxStart[ndxAxis] + dist;
			distEnd = -boxEnd[ndxAxis] + dist;

			normal.Init();
			normal[ndxAxis] = -1.0f;

			if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
				return -1.0f;
		}
		else
		{
			FindMax( v1[ndxAxis], v2[ndxAxis], v3[ndxAxis], dist );
			dist += boxExtents[ndxAxis];
			distStart = boxStart[ndxAxis] - dist;
			distEnd = boxEnd[ndxAxis] - dist;

			normal.Init();
			normal[ndxAxis] = 1.0f;

			if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
				return -1.0f;
		}
	}

	//
	// triangle face plane
	//
	Vector boxPt;
	float  expandDist;

	CalcClosestExtents( triNormal, boxExtents, boxPt );
	expandDist = triDist - DotProduct( triNormal, boxPt );
	distStart = DotProduct( triNormal, rStart ) - expandDist;
	distEnd = DotProduct( triNormal, rEnd ) - expandDist;
	if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
		return -1.0f;

	//
	// test 9 edges
	//
	Vector edge;
		
	//
	// edge 1
	//
	edge = v2 - v1;

	// edge1 cross axial-x = ( 0.0, edgeZ, -edgeY )
	normal.x = 0.0f;  normal.y = edge.z;  normal.z = -edge.y;
	VectorNormalize( normal );
	dist = ( normal.y * v1.y ) + ( normal.z * v1.z ); 

	if( ( DotProduct( normal, v3 ) - dist ) > 0.0f )
	{
		normal.x = -normal.x;
		normal.y = -normal.y;
		normal.z = -normal.z;
		dist = -dist;
	}
	

	CalcClosestExtents( normal, boxExtents, boxPt );	
	expandDist = dist - ( ( normal.y * boxPt.y ) + ( normal.z * boxPt.z ) );
	distStart = ( normal.y * rStart.y ) + ( normal.z * rStart.z ) - expandDist;
	distEnd = ( normal.y * rEnd.y ) + ( normal.z * rEnd.z ) - expandDist;
	if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
		return -1.0f;

	// edge1 cross axial-y = ( -edgeZ, 0.0, edgeX )
	normal.x = -edge.z;  normal.y = 0.0f;  normal.z = edge.x;
	VectorNormalize( normal );
	dist = ( normal.x * v1.x ) + ( normal.z * v1.z );

	if( ( DotProduct( normal, v3 ) - dist ) > 0.0f )
	{
		normal.x = -normal.x;
		normal.y = -normal.y;
		normal.z = -normal.z;
		dist = -dist;
	}


	CalcClosestExtents( normal, boxExtents, boxPt );	
	expandDist = dist - ( ( normal.x * boxPt.x ) + ( normal.z * boxPt.z ) );
	distStart = ( normal.x * rStart.x ) + ( normal.z * rStart.z ) - expandDist;
	distEnd = ( normal.x * rEnd.x ) + ( normal.z * rEnd.z ) - expandDist;
	if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
		return -1.0f;

	// edge1 cross axial-z = ( edgeY, -edgeX, 0.0 )
	normal.x = edge.y;  normal.y = -edge.x;  normal.z = 0.0f;
	VectorNormalize( normal );
	dist = ( normal.x * v1.x ) + ( normal.y * v1.y );

	if( ( DotProduct( normal, v3 ) - dist ) > 0.0f )
	{
		normal.x = -normal.x;
		normal.y = -normal.y;
		normal.z = -normal.z;
		dist = -dist;
	}

	CalcClosestExtents( normal, boxExtents, boxPt );	
	expandDist = dist - ( ( normal.x * boxPt.x ) + ( normal.y * boxPt.y ) );
	distStart = ( normal.x * rStart.x ) + ( normal.y * rStart.y ) - expandDist;
	distEnd = ( normal.x * rEnd.x ) + ( normal.y * rEnd.y ) - expandDist;
	if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
		return -1.0f;

	//
	// edge 2
	//
	edge = v3 - v2;

	// edge2 cross axial-x = ( 0.0, edgeZ, -edgeY )
	normal.x = 0.0f;  normal.y = edge.z;  normal.z = -edge.y;
	VectorNormalize( normal );
	dist = ( normal.y * v2.y ) + ( normal.z * v2.z ); 

	if( ( DotProduct( normal, v1 ) - dist ) > 0.0f )
	{
		normal.x = -normal.x;
		normal.y = -normal.y;
		normal.z = -normal.z;
		dist = -dist;
	}


	CalcClosestExtents( normal, boxExtents, boxPt );	
	expandDist = dist - ( ( normal.y * boxPt.y ) + ( normal.z * boxPt.z ) );
	distStart = ( normal.y * rStart.y ) + ( normal.z * rStart.z ) - expandDist;
	distEnd = ( normal.y * rEnd.y ) + ( normal.z * rEnd.z ) - expandDist;
	if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
		return -1.0f;

	// edge2 cross axial-y = ( -edgeZ, 0.0, edgeX )
	normal.x = -edge.z;  normal.y = 0.0f;  normal.z = edge.x;
	VectorNormalize( normal );
	dist = ( normal.x * v2.x ) + ( normal.z * v2.z );

	if( ( DotProduct( normal, v1 ) - dist ) > 0.0f )
	{
		normal.x = -normal.x;
		normal.y = -normal.y;
		normal.z = -normal.z;
		dist = -dist;
	}

	CalcClosestExtents( normal, boxExtents, boxPt );	
	expandDist = dist - ( ( normal.x * boxPt.x ) + ( normal.z * boxPt.z ) );
	distStart = ( normal.x * rStart.x ) + ( normal.z * rStart.z ) - expandDist;
	distEnd = ( normal.x * rEnd.x ) + ( normal.z * rEnd.z ) - expandDist;
	if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
		return -1.0f;

	// edge2 cross axial-z = ( edgeY, -edgeX, 0.0 )
	normal.x = edge.y;  normal.y = -edge.x;  normal.z = 0.0f;
	VectorNormalize( normal );
	dist = ( normal.x * v2.x ) + ( normal.y * v2.y );

	if( ( DotProduct( normal, v1 ) - dist ) > 0.0f )
	{
		normal.x = -normal.x;
		normal.y = -normal.y;
		normal.z = -normal.z;
		dist = -dist;
	}

	CalcClosestExtents( normal, boxExtents, boxPt );	
	expandDist = dist - ( ( normal.x * boxPt.x ) + ( normal.y * boxPt.y ) );
	distStart = ( normal.x * rStart.x ) + ( normal.y * rStart.y ) - expandDist;
	distEnd = ( normal.x * rEnd.x ) + ( normal.y * rEnd.y ) - expandDist;
	if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
		return -1.0f;

	//
	// edge 3
	//
	edge = v1 - v3;

	// edge3 cross axial-x = ( 0.0, edgeZ, -edgeY )
	normal.x = 0.0f;  normal.y = edge.z;  normal.z = -edge.y;
	VectorNormalize( normal );
	dist = ( normal.y * v1.y ) + ( normal.z * v1.z ); 

	if( ( DotProduct( normal, v2 ) - dist ) > 0.0f )
	{
		normal.x = -normal.x;
		normal.y = -normal.y;
		normal.z = -normal.z;
		dist = -dist;
	}

	CalcClosestExtents( normal, boxExtents, boxPt );	
	expandDist = dist - ( ( normal.y * boxPt.y ) + ( normal.z * boxPt.z ) );
	distStart = ( normal.y * rStart.y ) + ( normal.z * rStart.z ) - expandDist;
	distEnd = ( normal.y * rEnd.y ) + ( normal.z * rEnd.z ) - expandDist;
	if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
		return -1.0f;

	// edge3 cross axial-y = ( -edgeZ, 0.0, edgeX )
	normal.x = -edge.z;  normal.y = 0.0f;  normal.z = edge.x;
	VectorNormalize( normal );
	dist = ( normal.x * v1.x ) + ( normal.z * v1.z );

	if( ( DotProduct( normal, v2 ) - dist ) > 0.0f )
	{
		normal.x = -normal.x;
		normal.y = -normal.y;
		normal.z = -normal.z;
		dist = -dist;
	}

	CalcClosestExtents( normal, boxExtents, boxPt );	
	expandDist = dist - ( ( normal.x * boxPt.x ) + ( normal.z * boxPt.z ) );
	distStart = ( normal.x * rStart.x ) + ( normal.z * rStart.z ) - expandDist;
	distEnd = ( normal.x * rEnd.x ) + ( normal.z * rEnd.z ) - expandDist;
	if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
		return -1.0f;

	// edge3 cross axial-z = ( edgeY, -edgeX, 0.0 )
	normal.x = edge.y;  normal.y = -edge.x;  normal.z = 0.0f;
	VectorNormalize( normal );
	dist = ( normal.x * v1.x ) + ( normal.y * v1.y );

	if( ( DotProduct( normal, v2 ) - dist ) > 0.0f )
	{
		normal.x = -normal.x;
		normal.y = -normal.y;
		normal.z = -normal.z;
		dist = -dist;
	}

	CalcClosestExtents( normal, boxExtents, boxPt );	
	expandDist = dist - ( ( normal.x * boxPt.x ) + ( normal.y * boxPt.y ) );
	distStart = ( normal.x * rStart.x ) + ( normal.y * rStart.y ) - expandDist;
	distEnd = ( normal.x * rEnd.x ) + ( normal.y * rEnd.y ) - expandDist;
	if( !ResolveDistances( distStart, distEnd, fracStart, fracEnd ) )
		return -1.0f;

	// no collision
	if( fracEnd < fracStart )
		return -1.0f;

	return fracStart;
}
#endif


//-----------------------------------------------------------------------------
// Intersects a plane with a triangle (requires barycentric definition)
//-----------------------------------------------------------------------------

int IntersectTriangleWithPlaneBarycentric( const Vector& org, const Vector& edgeU,
		const Vector& edgeV, const Vector4D& plane, Vector2D* pIntersection )
{
	// This uses a barycentric method, since we need that to determine
	// interpolated points, alphas, and normals
	// Given the plane equation P dot N + d = 0
	// and the barycentric coodinate equation P = Org + EdgeU * u + EdgeV * v
	// Plug em in. Intersection occurs at u = 0 or v = 0 or u + v = 1

	float orgDotNormal = DotProduct( org, plane.AsVector3D() );
	float edgeUDotNormal = DotProduct( edgeU, plane.AsVector3D() );
	float edgeVDotNormal = DotProduct( edgeV, plane.AsVector3D() );

	int ptIdx = 0;

	// u = 0
	if ( edgeVDotNormal != 0.0f )
	{
		pIntersection[ptIdx].x = 0.0f;
		pIntersection[ptIdx].y = - ( orgDotNormal - plane.w ) / edgeVDotNormal;
		if ((pIntersection[ptIdx].y >= 0.0f) && (pIntersection[ptIdx].y <= 1.0f))
			++ptIdx;
	}

	// v = 0
	if ( edgeUDotNormal != 0.0f )
	{
		pIntersection[ptIdx].x = - ( orgDotNormal - plane.w ) / edgeUDotNormal;
		pIntersection[ptIdx].y = 0.0f;
		if ((pIntersection[ptIdx].x >= 0.0f) && (pIntersection[ptIdx].x <= 1.0f))
			++ptIdx;
	}

	// u + v = 1
	if (ptIdx == 2)
		return ptIdx;

	if ( edgeVDotNormal != edgeUDotNormal )
	{
		pIntersection[ptIdx].x = - ( orgDotNormal - plane.w + edgeVDotNormal) / 
			( edgeUDotNormal - edgeVDotNormal);
		pIntersection[ptIdx].y = 1.0f - pIntersection[ptIdx].x;;
		if ((pIntersection[ptIdx].x >= 0.0f) && (pIntersection[ptIdx].x <= 1.0f) &&
			 (pIntersection[ptIdx].y >= 0.0f) && (pIntersection[ptIdx].y <= 1.0f))
			++ptIdx;
	}

	Assert( ptIdx < 3 );
	return ptIdx;
}


//-----------------------------------------------------------------------------
// Returns true if a box intersects with a sphere
//-----------------------------------------------------------------------------
bool IsSphereIntersectingSphere( const Vector& center1, float radius1, 
								 const Vector& center2, float radius2 )
{
	Vector delta;
	VectorSubtract( center2, center1, delta );
	float distSq = delta.LengthSqr();
	float radiusSum = radius1 + radius2;
	return (distSq <= (radiusSum * radiusSum));
}


//-----------------------------------------------------------------------------
// Returns true if a box intersects with a sphere
//-----------------------------------------------------------------------------
bool IsBoxIntersectingSphere( const Vector& boxMin, const Vector& boxMax, 
						const Vector& center, float radius )
{
	// See Graphics Gems, box-sphere intersection
	float dmin = 0.0f;
	float flDelta;

	// Unrolled the loop.. this is a big cycle stealer...
	if (center[0] < boxMin[0])
	{
		flDelta = center[0] - boxMin[0];
		dmin += flDelta * flDelta;
	}
	else if (center[0] > boxMax[0])
	{
		flDelta = boxMax[0] - center[0];
		dmin += flDelta * flDelta;
	}

	if (center[1] < boxMin[1])
	{
		flDelta = center[1] - boxMin[1];
		dmin += flDelta * flDelta;
	}
	else if (center[1] > boxMax[1])
	{
		flDelta = boxMax[1] - center[1];
		dmin += flDelta * flDelta;
	}

	if (center[2] < boxMin[2])
	{
		flDelta = center[2] - boxMin[2];
		dmin += flDelta * flDelta;
	}
	else if (center[2] > boxMax[2])
	{
		flDelta = boxMax[2] - center[2];
		dmin += flDelta * flDelta;
	}

	return dmin < radius * radius;
}

bool IsBoxIntersectingSphereExtents( const Vector& boxCenter, const Vector& boxHalfDiag, 
						const Vector& center, float radius )
{
	// See Graphics Gems, box-sphere intersection
	float dmin = 0.0f;
	float flDelta, flDiff;

	// Unrolled the loop.. this is a big cycle stealer...
	flDiff = FloatMakePositive( center.x - boxCenter.x );
	if (flDiff > boxHalfDiag.x)
	{
		flDelta = flDiff - boxHalfDiag.x;
		dmin += flDelta * flDelta;
	}

	flDiff = FloatMakePositive( center.y - boxCenter.y );
	if (flDiff > boxHalfDiag.y)
	{
		flDelta = flDiff - boxHalfDiag.y;
		dmin += flDelta * flDelta;
	}

	flDiff = FloatMakePositive( center.z - boxCenter.z );
	if (flDiff > boxHalfDiag.z)
	{
		flDelta = flDiff - boxHalfDiag.z;
		dmin += flDelta * flDelta;
	}

	return dmin < radius * radius;
}


//-----------------------------------------------------------------------------
// Returns true if a rectangle intersects with a circle
//-----------------------------------------------------------------------------
bool IsCircleIntersectingRectangle( const Vector2D& boxMin, const Vector2D& boxMax, 
						      const Vector2D& center, float radius )
{
	// See Graphics Gems, box-sphere intersection
	float dmin = 0.0f;
	float flDelta;

	if (center[0] < boxMin[0])
	{
		flDelta = center[0] - boxMin[0];
		dmin += flDelta * flDelta;
	}
	else if (center[0] > boxMax[0])
	{
		flDelta = boxMax[0] - center[0];
		dmin += flDelta * flDelta;
	}

	if (center[1] < boxMin[1])
	{
		flDelta = center[1] - boxMin[1];
		dmin += flDelta * flDelta;
	}
	else if (center[1] > boxMax[1])
	{
		flDelta = boxMax[1] - center[1];
		dmin += flDelta * flDelta;
	}

	return dmin < radius * radius;
}


//-----------------------------------------------------------------------------
// returns true if the point is in the box
//-----------------------------------------------------------------------------
bool IsPointInBox( const Vector& pt, const Vector& boxMin, const Vector& boxMax )
{
	Assert( boxMin[0] <= boxMax[0] );
	Assert( boxMin[1] <= boxMax[1] );
	Assert( boxMin[2] <= boxMax[2] );

	if ( (pt[0] > boxMax[0]) || (pt[0] < boxMin[0]) )
		return false;
	if ( (pt[1] > boxMax[1]) || (pt[1] < boxMin[1]) )
		return false;
	if ( (pt[2] > boxMax[2]) || (pt[2] < boxMin[2]) )
		return false;
	return true;
}


bool IsPointInCone( const Vector &pt, const Vector &origin, const Vector &axis, float cosAngle, float length )
{
	Vector delta = pt - origin;
	float dist = VectorNormalize( delta );
	float dot = DotProduct( delta, axis );
	if ( dot < cosAngle )
		return false;
	if ( dist * dot > length )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// returns true if there's an intersection between two boxes
//-----------------------------------------------------------------------------
bool IsBoxIntersectingBox( const Vector& boxMin1, const Vector& boxMax1, 
						const Vector& boxMin2, const Vector& boxMax2 )
{
	Assert( boxMin1[0] <= boxMax1[0] );
	Assert( boxMin1[1] <= boxMax1[1] );
	Assert( boxMin1[2] <= boxMax1[2] );
	Assert( boxMin2[0] <= boxMax2[0] );
	Assert( boxMin2[1] <= boxMax2[1] );
	Assert( boxMin2[2] <= boxMax2[2] );

	if ( (boxMin1[0] > boxMax2[0]) || (boxMax1[0] < boxMin2[0]) )
		return false;
	if ( (boxMin1[1] > boxMax2[1]) || (boxMax1[1] < boxMin2[1]) )
		return false;
	if ( (boxMin1[2] > boxMax2[2]) || (boxMax1[2] < boxMin2[2]) )
		return false;
	return true;
}

bool IsBoxIntersectingBoxExtents( const Vector& boxCenter1, const Vector& boxHalfDiagonal1, 
						   const Vector& boxCenter2, const Vector& boxHalfDiagonal2 )
{
	Vector vecDelta, vecSize;
	VectorSubtract( boxCenter1, boxCenter2, vecDelta );
	VectorAdd( boxHalfDiagonal1, boxHalfDiagonal2, vecSize );
	return ( FloatMakePositive( vecDelta.x ) <= vecSize.x ) &&
			( FloatMakePositive( vecDelta.y ) <= vecSize.y ) &&
			( FloatMakePositive( vecDelta.z ) <= vecSize.z );
}


//-----------------------------------------------------------------------------
// returns true if there's an intersection between box and ray
//-----------------------------------------------------------------------------

bool IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
							const Vector& origin, const Vector& delta )
{
	Assert( boxMin[0] <= boxMax[0] );
	Assert( boxMin[1] <= boxMax[1] );
	Assert( boxMin[2] <= boxMax[2] );

	// FIXME: Surely there's a faster way
	float tmin = -FLT_MAX;
	float tmax = FLT_MAX;

	for (int i = 0; i < 3; ++i)
	{
		// Parallel case...
		if (FloatMakePositive(delta[i]) < 1e-8)
		{
			// Check that origin is in the box
			// if not, then it doesn't intersect..
			if ( (origin[i] < boxMin[i]) || (origin[i] > boxMax[i]) )
				return false;

			continue;
		}

		// non-parallel case
		// Find the t's corresponding to the entry and exit of
		// the ray along x, y, and z. The find the furthest entry
		// point, and the closest exit point. Once that is done,
		// we know we don't collide if the closest exit point
		// is behind the starting location. We also don't collide if
		// the closest exit point is in front of the furthest entry point

		float invDelta = 1.0f / delta[i];
		float t1 = (boxMin[i] - origin[i]) * invDelta;
		float t2 = (boxMax[i] - origin[i]) * invDelta;
		if (t1 > t2)
		{
			float temp = t1;
			t1 = t2;
			t2 = temp;
		}
		if (t1 > tmin)
			tmin = t1;
		if (t2 < tmax)
			tmax = t2;
		if (tmin > tmax)
			return false;
		if (tmax < 0)
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Intersects a ray with a ray, return true if they intersect
// t, s = parameters of closest approach (if not intersecting!)
//-----------------------------------------------------------------------------
bool IntersectRayWithRay( const Ray_t &ray0, const Ray_t &ray1, float &t, float &s )
{
	//
	// r0 = p0 + v0t
	// r1 = p1 + v1s
	//
	// intersection : r0 = r1 :: p0 + v0t = p1 + v1s
	// NOTE: v(0,1) are unit direction vectors
	//
	// subtract p0 from both sides and cross with v1 (NOTE: v1 x v1 = 0)
	//  (v0 x v1)t = ((p1 - p0 ) x v1)
	//
	// dotting  with (v0 x v1) and dividing by |v0 x v1|^2
	//	t = Det | (p1 - p0) , v1 , (v0 x v1) | / |v0 x v1|^2
	//  s = Det | (p1 - p0) , v0 , (v0 x v1) | / |v0 x v1|^2
	//
	//  Det | A B C | = -( A x C ) dot B or -( C x B ) dot A
	// 
	//  NOTE: if |v0 x v1|^2 = 0, then the lines are parallel
	//
	Vector v0( ray0.m_Delta );
	Vector v1( ray1.m_Delta );
	VectorNormalize( v0 );
	VectorNormalize( v1 );

	Vector v0xv1 = v0.Cross( v1 );
	float length = v0xv1.Length();
	if( length == 0.0f )
		return false;		// parallel

	Vector p1p0 = ray1.m_Start - ray0.m_Start;

	Vector AxC = p1p0.Cross( v0xv1 );
	AxC.Negate();
	float detT = AxC.Dot( v1 );
	
	AxC = p1p0.Cross( v0xv1 );
	AxC.Negate();
	float detS = AxC.Dot( v0 );

	t = detT / ( length * length );
	s = detS / ( length * length );

	// intersection????
	Vector i0, i1;
	i0 = v0 * t;
	i1 = v1 * s;
	i0 += ray0.m_Start;
	i1 += ray1.m_Start;
	if( i0.x == i1.x && i0.y == i1.y && i0.z == i1.z )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Intersects a ray with a plane, returns distance t along ray.
//-----------------------------------------------------------------------------

float IntersectRayWithPlane( const Ray_t& ray, const cplane_t& plane )
{
	float denom	= DotProduct( ray.m_Delta, plane.normal );
	if (denom == 0.0f)
		return 0.0f;

	denom = 1.0f / denom;
	return (plane.dist - DotProduct( ray.m_Start, plane.normal )) * denom;
}

float IntersectRayWithPlane( const Vector& org, const Vector& dir, const cplane_t& plane )
{
	float denom	= DotProduct( dir, plane.normal );
	if (denom == 0.0f)
		return 0.0f;

	denom = 1.0f / denom;
	return (plane.dist - DotProduct( org, plane.normal )) * denom;
}

float IntersectRayWithPlane( const Vector& org, const Vector& dir, const Vector& normal, float dist )
{
	float denom	= DotProduct( dir, normal );
	if (denom == 0.0f)
		return 0.0f;

	denom = 1.0f / denom;
	return (dist - DotProduct( org, normal )) * denom;
}

float IntersectRayWithAAPlane( const Vector& vecStart, const Vector& vecEnd, int nAxis, float flSign, float flDist )
{
	float denom	= flSign * (vecEnd[nAxis] - vecStart[nAxis]);
	if (denom == 0.0f)
		return 0.0f;

	denom = 1.0f / denom;
	return (flDist - flSign * vecStart[nAxis]) * denom;
}


bool IntersectRayWithBox( const Vector &rayStart, const Vector &rayDelta, const Vector &boxMins, const Vector &boxMaxs, float epsilon, float &t, int &hitside, bool &startsolid )
{
	int			i;
	float		enterfrac, leavefrac;
	float		d1, d2;
	float		f;

	hitside = -1;
	enterfrac = -1.0f;
	leavefrac = 1.0f;

	Vector rayEnd = rayStart;
	rayEnd += rayDelta;
	startsolid = true;

	for ( i = 0 ;i < 6; i++ )
	{
		if ( i >= 3 )
		{
			d1 = rayStart[i-3] - boxMaxs[i-3];
			d2 = rayEnd[i-3] - boxMaxs[i-3];
		}
		else
		{
			d1 = -rayStart[i] + boxMins[i];
			d2 = -rayEnd[i] + boxMins[i];
		}

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 > 0)
			return false;

		// completely inside, check next face
		if (d1 <= 0 && d2 <= 0)
			continue;

		if (d1 > 0)
			startsolid = false;

		// crosses face
		if (d1 > d2)
		{
			f = d1-epsilon;
			if ( f < 0 )
				f = 0;
			f = f / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				hitside = i;
			}
		}
		else
		{ // leave
			f = (d1+epsilon) / (d1-d2);
			if (f < leavefrac)
			{
				leavefrac = f;
			}
		}
	}
	if (enterfrac < leavefrac && enterfrac >= 0.0f)
	{
		t = enterfrac;
		return true;
	}

	if ( startsolid )
	{
		t = 0;
		hitside = 0;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void GetNonMajorAxes( const Vector &vNormal, Vector2D &axes )
{
	axes[0] = 0;
	axes[1] = 1;

	if( FloatMakePositive( vNormal.x ) > FloatMakePositive( vNormal.y ) )
	{
		if( FloatMakePositive( vNormal.x ) > FloatMakePositive( vNormal.z ) )
		{
			axes[0] = 1;
			axes[1] = 2;
		}
	}
	else
	{
		if( FloatMakePositive( vNormal.y ) > FloatMakePositive( vNormal.z ) )
		{
			axes[0] = 0;
			axes[1] = 2;
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
QuadBarycentricRetval_t QuadWithParallelEdges( const Vector &vecOrigin, 
	const Vector &vecU, float lengthU, const Vector &vecV, float lengthV,
	const Vector &pt, Vector2D &vecUV )
{
	Ray_t rayAxis;
	Ray_t rayPt;

	//
	// handle the u axis
	//
	rayAxis.m_Start = vecOrigin;
	rayAxis.m_Delta = vecU;
	rayAxis.m_IsRay = true;

	rayPt.m_Start = pt;
	rayPt.m_Delta = vecV * -( lengthV * 10.0f );
	rayPt.m_IsRay = true;

	float s, t;
	IntersectRayWithRay( rayAxis, rayPt, t, s );
	vecUV[0] = t / lengthU;

	//
	// handle the v axis
	//
	rayAxis.m_Delta = vecV;

	rayPt.m_Delta = vecU * -( lengthU * 10.0f );

	IntersectRayWithRay( rayAxis, rayPt, t, s );
	vecUV[1] = t / lengthV;

	// inside of the quad??
	if( ( vecUV[0] < 0.0f ) || ( vecUV[0] > 1.0f ) || 
		( vecUV[1] < 0.0f ) || ( vecUV[1] > 1.0f ) )
		return BARY_QUADRATIC_FALSE;

	return BARY_QUADRATIC_TRUE;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void ResolveQuadratic( double tPlus, double tMinus, 
					   const Vector axisU0, const Vector axisU1,
					   const Vector axisV0, const Vector axisV1,
					   const Vector axisOrigin, const Vector pt,
					   int projU, double &s, double &t )
{
	// calculate the sPlus, sMinus pair(s)
	double sDenomPlus = ( axisU0[projU] * ( 1 - tPlus ) ) + ( axisU1[projU] * tPlus );
	double sDenomMinus = ( axisU0[projU] * ( 1 - tMinus ) ) + ( axisU1[projU] * tMinus );

	double sPlus = UNINIT, sMinus = UNINIT;
	if( FloatMakePositive( sDenomPlus ) >= 1e-5 )
	{
		sPlus = ( pt[projU] - axisOrigin[projU] - ( axisV0[projU] * tPlus ) ) / sDenomPlus; 
	}

	if( FloatMakePositive( sDenomMinus ) >= 1e-5 )
	{
		sMinus = ( pt[projU] - axisOrigin[projU] - ( axisV0[projU] * tMinus ) ) / sDenomMinus; 
	}
	
	if( ( tPlus >= 0.0 ) && ( tPlus <= 1.0 ) && ( sPlus >= 0.0 ) && ( sPlus <= 1.0 ) )
	{
		s = sPlus;
		t = tPlus;
		return;
	}

	if( ( tMinus >= 0.0 ) && ( tMinus <= 1.0 ) && ( sMinus >= 0.0 ) && ( sMinus <= 1.0 ) )
	{
		s = sMinus;
		t = tMinus;
		return;
	}

	double s0, t0, s1, t1;

	s0 = sPlus;
	t0 = tPlus;
	if( s0 >= 1.0 ) { s0 -= 1.0; }
	if( t0 >= 1.0 ) { t0 -= 1.0; }

	s1 = sMinus;
	t1 = tMinus;
	if( s1 >= 1.0 ) { s1 -= 1.0; }
	if( t1 >= 1.0 ) { t1 -= 1.0; }

	s0 = FloatMakePositive( s0 );
	t0 = FloatMakePositive( t0 );
	s1 = FloatMakePositive( s1 );
	t1 = FloatMakePositive( t1 );

	double max0, max1;
	max0 = s0;
	if( t0 > max0 ) { max0 = t0; }
	max1 = s1;
	if( t1 > max1 ) { max1 = t1; }

	if( max0 > max1 )
	{
		s = sMinus;
		t = tMinus;
	}
	else
	{
		s = sPlus;
		t = tPlus;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

QuadBarycentricRetval_t PointInQuadToBarycentric( const Vector &v1, const Vector &v2, 
	const Vector &v3, const Vector &v4, const Vector &point, Vector2D &uv )
{
#define PIQ_TEXTURE_EPSILON		0.001
#define PIQ_PLANE_EPSILON		0.1
#define PIQ_DOT_EPSILON			0.99f

	//
	// Think of a quad with points v1, v2, v3, v4 and u, v line segments
	// u0 = v2 - v1
	// u1 = v3 - v4
	// v0 = v4 - v1
	// v1 = v3 - v2
	//
	Vector axisU[2], axisV[2];
	Vector axisUNorm[2], axisVNorm[2];
	axisU[0] = axisUNorm[0] = v2 - v1;
	axisU[1] = axisUNorm[1] = v3 - v4;
	axisV[0] = axisVNorm[0] = v4 - v1;
	axisV[1] = axisVNorm[1] = v3 - v2;

	float lengthU[2], lengthV[2];
	lengthU[0] = VectorNormalize( axisUNorm[0] );
	lengthU[1] = VectorNormalize( axisUNorm[1] );
	lengthV[0] = VectorNormalize( axisVNorm[0] );
	lengthV[1] = VectorNormalize( axisVNorm[1] );

	//
	// check for an early out - parallel opposite edges!
	// NOTE: quad property if 1 set of opposite edges is parallel and equal
	//       in length, then the other set of edges is as well
	//
	if( axisUNorm[0].Dot( axisUNorm[1] ) > PIQ_DOT_EPSILON )
	{
		if( FloatMakePositive( lengthU[0] - lengthU[1] ) < PIQ_PLANE_EPSILON )
		{
			return QuadWithParallelEdges( v1, axisUNorm[0], lengthU[0], axisVNorm[0], lengthV[0], point, uv );
		}
	}

	//
	// since we are solving for s in our equations below we need to ensure that
	// the v axes are non-parallel
	//
	bool bFlipped = false;
	if( axisVNorm[0].Dot( axisVNorm[1] ) > PIQ_DOT_EPSILON )
	{
		Vector tmp[2];
		tmp[0] = axisV[0];
		tmp[1] = axisV[1];
		axisV[0] = axisU[0];
		axisV[1] = axisU[1];
		axisU[0] = tmp[0];
		axisU[1] = tmp[1];
		bFlipped = true;
	}

	//
	// get the "projection" axes
	//
	Vector2D projAxes;
	Vector vNormal = axisU[0].Cross( axisV[0] );
	GetNonMajorAxes( vNormal, projAxes );

	//
	// NOTE: axisU[0][projAxes[0]] < axisU[0][projAxes[1]], 
	//       this is done to decrease error when dividing later
	//
	if( FloatMakePositive( axisU[0][projAxes[0]] ) < FloatMakePositive( axisU[0][projAxes[1]] ) )
	{
		int tmp = projAxes[0];
		projAxes[0] = projAxes[1];
		projAxes[1] = tmp;
	}

	// Here's how we got these equations:
	//
	// Given the points and u,v line segments above...
	//
	// Then:
	//
	// (1.0) PT = P0 + U0 * s + V * t
	//
	// where
	//
	// (1.1) V = V0 + s * (V1 - V0)
	// (1.2) U = U0 + t * (U1 - U0)
	// 
	// Therefore (from 1.1 + 1.0):
	// PT - P0 = U0 * s + (V0 + s * (V1-V0)) * t
	// Group s's:
	// PT - P0 - t * V0 = s * (U0 + t * (V1-V0))
	// Two equations and two unknowns in x and y get you the following quadratic:
	//
	// solve the quadratic
	//
	double s = 0.0, t = 0.0;
	double A, negB, C;

	A = ( axisU[0][projAxes[1]] * axisV[0][projAxes[0]] ) - 
		( axisU[0][projAxes[0]] * axisV[0][projAxes[1]] ) - 
		( axisU[1][projAxes[1]] * axisV[0][projAxes[0]] ) + 
		( axisU[1][projAxes[0]] * axisV[0][projAxes[1]] );
	C = ( v1[projAxes[1]] * axisU[0][projAxes[0]] ) - 
		( point[projAxes[1]] * axisU[0][projAxes[0]] ) - 
		( v1[projAxes[0]] * axisU[0][projAxes[1]] ) + 
		( point[projAxes[0]] * axisU[0][projAxes[1]] );
	negB = C - 
		  ( v1[projAxes[1]] * axisU[1][projAxes[0]] ) + 
		  ( point[projAxes[1]] * axisU[1][projAxes[0]] ) + 
		  ( v1[projAxes[0]] * axisU[1][projAxes[1]] ) - 
		  ( point[projAxes[0]] * axisU[1][projAxes[1]] ) + 
		  ( axisU[0][projAxes[1]] * axisV[0][projAxes[0]] ) - 
		  ( axisU[0][projAxes[0]] * axisV[0][projAxes[1]] );

	if( ( A > -PIQ_PLANE_EPSILON ) && ( A < PIQ_PLANE_EPSILON ) )
	{
		// shouldn't be here -- this should have been take care of in the "early out"
//		Assert( 0 );

		Vector vecUAvg, vecVAvg;
		vecUAvg = ( axisUNorm[0] + axisUNorm[1] ) * 0.5f;
		vecVAvg = ( axisVNorm[0] + axisVNorm[1] ) * 0.5f;
		
		float fLengthUAvg = ( lengthU[0] + lengthU[1] ) * 0.5f;
		float fLengthVAvg = ( lengthV[0] + lengthV[1] ) * 0.5f;

		return QuadWithParallelEdges( v1, vecUAvg, fLengthUAvg, vecVAvg, fLengthVAvg, point, uv );

#if 0
		// legacy code -- kept here for completeness!

		// not a quadratic -- solve linearly
		t = C / negB;

		// See (1.2) above
		float ui = axisU[0][projAxes[0]] + t * ( axisU[1][projAxes[0]] - axisU[0][projAxes[0]] );
		if( FloatMakePositive( ui ) >= 1e-5 )
		{
			// See (1.0) above
			s = ( point[projAxes[0]] - v1[projAxes[0]] - axisV[0][projAxes[0]] * t ) / ui;
		}
#endif
	}
	else
	{
		// (-b +/- sqrt( b^2 - 4ac )) / 2a
		double discriminant = (negB*negB) - (4.0f * A * C);
		if( discriminant < 0.0f )
		{
			uv[0] = -99999.0f;
			uv[1] = -99999.0f;
			return BARY_QUADRATIC_NEGATIVE_DISCRIMINANT;
		}

		double quad = sqrt( discriminant );
		double QPlus = ( negB + quad ) / ( 2.0f * A );
		double QMinus = ( negB - quad ) / ( 2.0f * A );

		ResolveQuadratic( QPlus, QMinus, axisU[0], axisU[1], axisV[0], axisV[1], v1, point, projAxes[0], s, t );
	}

	if( !bFlipped )
	{
		uv[0] = ( float )s;
		uv[1] = ( float )t;
	}
	else
	{
		uv[0] = ( float )t;
		uv[1] = ( float )s;
	}

	// inside of the quad??
	if( ( uv[0] < 0.0f ) || ( uv[0] > 1.0f ) || ( uv[1] < 0.0f ) || ( uv[1] > 1.0f ) )
		return BARY_QUADRATIC_FALSE;
	
	return BARY_QUADRATIC_TRUE;

#undef PIQ_TEXTURE_EPSILON
#undef PIQ_PLANE_EPSILON
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void PointInQuadFromBarycentric( const Vector &v1, const Vector &v2, const Vector &v3, const Vector &v4,
								 const Vector2D &uv, Vector &point )
{
	//
	// Think of a quad with points v1, v2, v3, v4 and u, v line segments
	// find the ray from v0 edge to v1 edge at v
	//
	Vector vPts[2];
	VectorLerp( v1, v4, uv[1], vPts[0] );
	VectorLerp( v2, v3, uv[1], vPts[1] );
	VectorLerp( vPts[0], vPts[1], uv[0], point ); 
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void TexCoordInQuadFromBarycentric( const Vector2D &v1, const Vector2D &v2, const Vector2D &v3, const Vector2D &v4,
								    const Vector2D &uv, Vector2D &texCoord )
{
	//
	// Think of a quad with points v1, v2, v3, v4 and u, v line segments
	// find the ray from v0 edge to v1 edge at v
	//
	Vector2D vCoords[2];
	Vector2DLerp( v1, v4, uv[1], vCoords[0] );
	Vector2DLerp( v2, v3, uv[1], vCoords[1] );
	Vector2DLerp( vCoords[0], vCoords[1], uv[0], texCoord ); 
}


//-----------------------------------------------------------------------------
// Compute point from barycentric specification
// Edge u goes from v0 to v1, edge v goes from v0 to v2
//-----------------------------------------------------------------------------

void ComputePointFromBarycentric( const Vector& v0, const Vector& v1, const Vector& v2, 
								 float u, float v, Vector& pt )
{
	Vector edgeU, edgeV;
	VectorSubtract( v1, v0, edgeU );
	VectorSubtract( v2, v0, edgeV );
	VectorMA( v0, u, edgeU, pt );
	VectorMA( pt, v, edgeV, pt );
}

void ComputePointFromBarycentric( const Vector2D& v0, const Vector2D& v1, const Vector2D& v2, 
								 float u, float v, Vector2D& pt )
{
	Vector2D edgeU, edgeV;
	Vector2DSubtract( v1, v0, edgeU );
	Vector2DSubtract( v2, v0, edgeV );
	Vector2DMA( v0, u, edgeU, pt );
	Vector2DMA( pt, v, edgeV, pt );
}


//-----------------------------------------------------------------------------
// Compute a matrix that has the correct orientation but which has an origin at
// the center of the bounds
//-----------------------------------------------------------------------------
static void ComputeCenterMatrix( const Vector& origin, const QAngle& angles, 
	const Vector& mins, const Vector& maxs, matrix3x4_t& matrix )
{
	Vector centroid;
	VectorAdd( mins, maxs, centroid );
	centroid *= 0.5f;
	AngleMatrix( angles, matrix );

	Vector worldCentroid;
	VectorRotate( centroid, matrix, worldCentroid );
	worldCentroid += origin;
	MatrixSetColumn( worldCentroid, 3, matrix );
}

static void ComputeCenterIMatrix( const Vector& origin, const QAngle& angles, 
	const Vector& mins, const Vector& maxs, matrix3x4_t& matrix )
{
	Vector centroid;
	VectorAdd( mins, maxs, centroid );
	centroid *= -0.5f;
	AngleIMatrix( angles, matrix );

	// For the translational component here, note that the origin in world space
	// is T = R * C + O, (R = rotation matrix, C = centroid in local space, O = origin in world space)
	// The IMatrix translation = - transpose(R) * T = -C - transpose(R) * 0
	Vector localOrigin;
	VectorRotate( origin, matrix, localOrigin );
	centroid -= localOrigin;
	MatrixSetColumn( centroid, 3, matrix );
}


//-----------------------------------------------------------------------------
// Compute a matrix which is the absolute value of another
//-----------------------------------------------------------------------------
static inline void ComputeAbsMatrix( const matrix3x4_t& in, matrix3x4_t& out )
{
	FloatBits(out[0][0]) = FloatAbsBits(in[0][0]);
	FloatBits(out[0][1]) = FloatAbsBits(in[0][1]);
	FloatBits(out[0][2]) = FloatAbsBits(in[0][2]);
	FloatBits(out[1][0]) = FloatAbsBits(in[1][0]);
	FloatBits(out[1][1]) = FloatAbsBits(in[1][1]);
	FloatBits(out[1][2]) = FloatAbsBits(in[1][2]);
	FloatBits(out[2][0]) = FloatAbsBits(in[2][0]);
	FloatBits(out[2][1]) = FloatAbsBits(in[2][1]);
	FloatBits(out[2][2]) = FloatAbsBits(in[2][2]);
}


//-----------------------------------------------------------------------------
// Compute a separating plane between two boxes (expensive!)
// Returns false if no separating plane exists
//-----------------------------------------------------------------------------
bool ComputeSeparatingPlane( const Vector& org1, const QAngle& angles1, const Vector& min1, const Vector& max1, 
	const Vector& org2, const QAngle& angles2, const Vector& min2, const Vector& max2, 
	float tolerance, cplane_t* pPlane )
{
	// The various separating planes can be either
	// 1) A plane parallel to one of the box face planes
	// 2) A plane parallel to the cross-product of an edge from each box

	// First, compute the basis of second box in the space of the first box
	// NOTE: These basis place the origin at the centroid of each box!
	matrix3x4_t	worldToBox1, box2ToWorld, box2ToBox1;
	ComputeCenterIMatrix( org1, angles1, min1, max1, worldToBox1 );
	ComputeCenterMatrix( org2, angles2, min2, max2, box2ToWorld );
	ConcatTransforms( worldToBox1, box2ToWorld, box2ToBox1 );

	// Then compute the size of the two boxes
	Vector box1Size, box2Size;
	VectorSubtract( max1, min1, box1Size );
	VectorSubtract( max2, min2, box2Size );
	box1Size *= 0.5f;
	box2Size *= 0.5f;

	// We're going to be using the origin of box2 in the space of box1 alot,
	// lets extract it from the matrix....
	Vector box2Origin;
	MatrixGetColumn( box2ToBox1, 3, box2Origin );

	// Next get the absolute values of these entries and store in absbox2ToBox1.
	matrix3x4_t absBox2ToBox1;
	ComputeAbsMatrix( box2ToBox1, absBox2ToBox1 );

	// There are 15 tests to make.  The first 3 involve trying planes parallel
	// to the faces of the first box.

	// NOTE: The algorithm here involves finding the projections of the two boxes
	// onto a particular line. If the projections on the line do not overlap,
	// that means that there's a plane perpendicular to the line which separates 
	// the two boxes; and we've therefore found a separating plane.

	// The way we check for overlay is we find the projections of the two boxes
	// onto the line, and add them up. We compare the sum with the projection
	// of the relative center of box2 onto the same line.

	Vector tmp;
	float boxProjectionSum;
	float originProjection;
	
	// NOTE: For these guys, we're taking advantage of the fact that the ith
	// row of the box2ToBox1 is the direction of the box1 (x,y,z)-axis
	// transformed into the space of box2.

	// First side of box 1
	boxProjectionSum = box1Size.x + MatrixRowDotProduct( absBox2ToBox1, 0, box2Size );
	originProjection = FloatMakePositive( box2Origin.x ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		VectorCopy( worldToBox1[0], pPlane->normal.Base() );
		return true;
	}
	
	// Second side of box 1
	boxProjectionSum = box1Size.y + MatrixRowDotProduct( absBox2ToBox1, 1, box2Size );
	originProjection = FloatMakePositive( box2Origin.y ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		VectorCopy( worldToBox1[1], pPlane->normal.Base() );
		return true;
	}
	
	// Third side of box 1
	boxProjectionSum = box1Size.z + MatrixRowDotProduct( absBox2ToBox1, 2, box2Size );
	originProjection = FloatMakePositive( box2Origin.z ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		VectorCopy( worldToBox1[2], pPlane->normal.Base() );
		return true;
	}
	
	// The next three involve checking splitting planes parallel to the
	// faces of the second box.

	// NOTE: For these guys, we're taking advantage of the fact that the 0th
	// column of the box2ToBox1 is the direction of the box2 x-axis
	// transformed into the space of box1.
	// Here, we're determining the distance of box2's center from box1's center
	// by projecting it onto a line parallel to box2's axis
	
	// First side of box 2
	boxProjectionSum = box2Size.x +	MatrixColumnDotProduct( absBox2ToBox1, 0, box1Size );
	originProjection = FloatMakePositive( MatrixColumnDotProduct( box2ToBox1, 0, box2Origin ) ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 0, pPlane->normal );	
		return true;
	}
	
	// Second side of box 2
	boxProjectionSum = box2Size.y +	MatrixColumnDotProduct( absBox2ToBox1, 1, box1Size );
	originProjection = FloatMakePositive( MatrixColumnDotProduct( box2ToBox1, 1, box2Origin ) ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 1, pPlane->normal );	
		return true;
	}
	
	// Third side of box 2
	boxProjectionSum = box2Size.z +	MatrixColumnDotProduct( absBox2ToBox1, 2, box1Size );
	originProjection = FloatMakePositive( MatrixColumnDotProduct( box2ToBox1, 2, box2Origin ) ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 2, pPlane->normal );	
		return true;
	}
	
	// Next check the splitting planes which are orthogonal to the pairs
	// of edges, one from box1 and one from box2.  As only direction matters,
	// there are 9 pairs since each box has 3 distinct edge directions.
	
	// Here, we take advantage of the fact that the edges from box 1 are all
	// axis aligned; therefore the crossproducts are simplified. Let's walk through
	// the example of b1e1 x b2e1:

	// In this example, the line to check is perpendicular to b1e1 + b2e2
	// we can compute this line by taking the cross-product:
	//
	// [  i  j  k ]
	// [  1  0  0 ] = - ez j + ey k = l1
	// [ ex ey ez ]

	// Where ex, ey, ez is the components of box2's x axis in the space of box 1,
	// which is == to the 0th column of of box2toBox1

	// The projection of box1 onto this line = the absolute dot product of the box size
	// against the line, which =
	// AbsDot( box1Size, l1 ) = abs( -ez * box1.y ) + abs( ey * box1.z )

	// To compute the projection of box2 onto this line, we'll do it in the space of box 2
	//
	// [  i  j  k ]
	// [ fx fy fz ] = fz j - fy k = l2
	// [  1  0  0 ]

	// Where fx, fy, fz is the components of box1's x axis in the space of box 2,
	// which is == to the 0th row of of box2toBox1

	// The projection of box2 onto this line = the absolute dot product of the box size
	// against the line, which =
	// AbsDot( box2Size, l2 ) = abs( fz * box2.y ) + abs ( fy * box2.z )

	// The projection of the relative origin position on this line is done in the 
	// space of box 1:
	//
	// originProjection = DotProduct( <-ez j + ey k>, box2Origin ) =
	//		-ez * box2Origin.y + ey * box2Origin.z
	
	// b1e1 x b2e1
	boxProjectionSum =
		box1Size.y * absBox2ToBox1[2][0] + box1Size.z * absBox2ToBox1[1][0] +
		box2Size.y * absBox2ToBox1[0][2] + box2Size.z * absBox2ToBox1[0][1];
	originProjection = FloatMakePositive( -box2Origin.y * box2ToBox1[2][0] + box2Origin.z * box2ToBox1[1][0] ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 0, tmp );	
		CrossProduct( worldToBox1[0], tmp.Base(), pPlane->normal.Base() ); 
		return true;
	}
	
	// b1e1 x b2e2
	boxProjectionSum =
		box1Size.y * absBox2ToBox1[2][1] + box1Size.z * absBox2ToBox1[1][1] +
		box2Size.x * absBox2ToBox1[0][2] + box2Size.z * absBox2ToBox1[0][0];
	originProjection = FloatMakePositive( -box2Origin.y * box2ToBox1[2][1] + box2Origin.z * box2ToBox1[1][1] ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 1, tmp );	
		CrossProduct( worldToBox1[0], tmp.Base(), pPlane->normal.Base() ); 
		return true;
	}
	
	// b1e1 x b2e3
	boxProjectionSum =
		box1Size.y * absBox2ToBox1[2][2] + box1Size.z * absBox2ToBox1[1][2] +
		box2Size.x * absBox2ToBox1[0][1] + box2Size.y * absBox2ToBox1[0][0];
	originProjection = FloatMakePositive( -box2Origin.y * box2ToBox1[2][2] + box2Origin.z * box2ToBox1[1][2] ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 2, tmp );	
		CrossProduct( worldToBox1[0], tmp.Base(), pPlane->normal.Base() ); 
		return true;
	}
	
	// b1e2 x b2e1
	boxProjectionSum =
		box1Size.x * absBox2ToBox1[2][0] + box1Size.z * absBox2ToBox1[0][0] +
		box2Size.y * absBox2ToBox1[1][2] + box2Size.z * absBox2ToBox1[1][1];
	originProjection = FloatMakePositive( box2Origin.x * box2ToBox1[2][0] - box2Origin.z * box2ToBox1[0][0] ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 0, tmp );	
		CrossProduct( worldToBox1[1], tmp.Base(), pPlane->normal.Base() ); 
		return true;
	}
	
	// b1e2 x b2e2
	boxProjectionSum =
		box1Size.x * absBox2ToBox1[2][1] + box1Size.z * absBox2ToBox1[0][1] +
		box2Size.x * absBox2ToBox1[1][2] + box2Size.z * absBox2ToBox1[1][0];
	originProjection = FloatMakePositive( box2Origin.x * box2ToBox1[2][1] - box2Origin.z * box2ToBox1[0][1] ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 1, tmp );	
		CrossProduct( worldToBox1[1], tmp.Base(), pPlane->normal.Base() ); 
		return true;
	}
	
	// b1e2 x b2e3
	boxProjectionSum =
		box1Size.x * absBox2ToBox1[2][2] + box1Size.z * absBox2ToBox1[0][2] +
		box2Size.x * absBox2ToBox1[1][1] + box2Size.y * absBox2ToBox1[1][0];
	originProjection = FloatMakePositive( box2Origin.x * box2ToBox1[2][2] - box2Origin.z * box2ToBox1[0][2] ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 2, tmp );	
		CrossProduct( worldToBox1[1], tmp.Base(), pPlane->normal.Base() ); 
		return true;
	}
	
	// b1e3 x b2e1
	boxProjectionSum =
		box1Size.x * absBox2ToBox1[1][0] + box1Size.y * absBox2ToBox1[0][0] +
		box2Size.y * absBox2ToBox1[2][2] + box2Size.z * absBox2ToBox1[2][1];
	originProjection = FloatMakePositive( -box2Origin.x * box2ToBox1[1][0] + box2Origin.y * box2ToBox1[0][0] ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 0, tmp );	
		CrossProduct( worldToBox1[2], tmp.Base(), pPlane->normal.Base() ); 
		return true;
	}
	
	// b1e3 x b2e2
	boxProjectionSum =
		box1Size.x * absBox2ToBox1[1][1] + box1Size.y * absBox2ToBox1[0][1] +
		box2Size.x * absBox2ToBox1[2][2] + box2Size.z * absBox2ToBox1[2][0];
	originProjection = FloatMakePositive( -box2Origin.x * box2ToBox1[1][1] + box2Origin.y * box2ToBox1[0][1] ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 1, tmp );	
		CrossProduct( worldToBox1[2], tmp.Base(), pPlane->normal.Base() ); 
		return true;
	}
	
	// b1e3 x b2e3
	boxProjectionSum =
		box1Size.x * absBox2ToBox1[1][2] + box1Size.y * absBox2ToBox1[0][2] +
		box2Size.x * absBox2ToBox1[2][1] + box2Size.y * absBox2ToBox1[2][0];
	originProjection = FloatMakePositive( -box2Origin.x * box2ToBox1[1][2] + box2Origin.y * box2ToBox1[0][2] ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 2, tmp );	
		CrossProduct( worldToBox1[2], tmp.Base(), pPlane->normal.Base() ); 
		return true;
	}
	
	return false;
}


