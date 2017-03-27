//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PHYSICS_TRACE_H
#define PHYSICS_TRACE_H
#ifdef _WIN32
#pragma once
#endif


class Vector;
class QAngle;
class CGameTrace;
class CTraceRay;
class IVP_Compact_Surface;
typedef CGameTrace trace_t;
struct Ray_t;
class CLedgeCache;
class IVP_Compact_Surface;
class IVP_Compact_Mopp;
class IConvexInfo;
enum
{
	COLLIDE_POLY = 0,
	COLLIDE_MOPP = 1,
	COLLIDE_BALL = 2,
};

class ITraceObject
{
public:
	virtual Vector SupportMap( const Vector &dir, int &index ) = 0;
	virtual Vector GetVertByIndex( int index ) = 0;
	virtual float Radius( void ) = 0;
};

// This is the size of the vertex hash
#define CONVEX_HASH_SIZE		512
// The little hashing trick below allows 64K verts per hash entry
#define	MAX_CONVEX_VERTS		((CONVEX_HASH_SIZE * (1<<16))-1)

class CPhysicsTrace
{
public:
	CPhysicsTrace();
	~CPhysicsTrace();
	// Calculate the intersection of a swept box (mins/maxs) against an IVP object.  All coords are in HL space.
	void SweepBoxIVP( const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, const IVP_Compact_Surface *pSurface, const Vector &surfaceOrigin, const QAngle &surfaceAngles, trace_t *ptr );
	void SweepBoxIVP( const Ray_t &raySrc, unsigned int contentsMask, IConvexInfo *pConvexInfo, const IVP_Compact_Surface *pSurface, const Vector &surfaceOrigin, const QAngle &surfaceAngles, trace_t *ptr );

	// Calculate the intersection of a swept compact surface against another compact surface.  All coords are in HL space.
	// NOTE: BUGBUG: swept surface must be single convex!!!
	void SweepIVP( const Vector &start, const Vector &end, const IVP_Compact_Surface *pSweptSurface, const QAngle &sweptAngles, const IVP_Compact_Surface *pSurface, const Vector &surfaceOrigin, const QAngle &surfaceAngles, trace_t *ptr );

	// get an AABB for an oriented collide
	void GetAABB( Vector &mins, Vector &maxs, const IVP_Compact_Surface *pCollide, const Vector &collideOrigin, const QAngle &collideAngles );

	// get the support map/extent for a collide along the axis given by "direction"
	Vector GetExtent( const IVP_Compact_Surface *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, const Vector &direction );
	// Use hash table to get a starting vertex
	void GetStartVert( const IVP_Compact_Ledge *pLedge, const IVP_U_Float_Point &localDirection, int &triIndex, int &edgeIndex );

	inline unsigned short VertIndexToID( int vertIndex );
	inline void VisitVert( int vertIndex );
	inline bool WasVisited( int vertIndex );
	inline void NewVisit( void );
	inline void CountStatDotProduct();
	inline void ClearStatDotProduct();
	inline unsigned int ReportStatDotProduct() { return m_statsDotProduct; }

private:

	// Store the current increment and the vertex ID (rotating hash) to guarantee no collisions
	struct vertmarker_t
	{
		unsigned short visitID;
		unsigned short vertID;
	};

	vertmarker_t		m_vertVisit[CONVEX_HASH_SIZE];
	unsigned short		m_vertVisitID;
	unsigned int		m_statsDotProduct;
	CLedgeCache			*m_pLedgeCache;
};

inline void CPhysicsTrace::CountStatDotProduct()
{
	m_statsDotProduct++;
}
inline void CPhysicsTrace::ClearStatDotProduct()
{
	m_statsDotProduct = 0;
}

// Calculate the intersection of a swept box (mins/maxs) against an IVP object.  All coords are in HL space.
inline unsigned short CPhysicsTrace::VertIndexToID( int vertIndex )
{
	// A little hashing trick here:
	// rotate the hash key each time you wrap around at 64K
	// That way, the index will not collide until you've hit 64K # hash entries times
	int high = vertIndex >> 16;
	return (unsigned short) ((vertIndex + high) & 0xFFFF);
}

inline void CPhysicsTrace::VisitVert( int vertIndex ) 
{
	int index = vertIndex & (CONVEX_HASH_SIZE-1);
	m_vertVisit[index].visitID = m_vertVisitID;
	m_vertVisit[index].vertID = VertIndexToID(vertIndex);
}

inline bool CPhysicsTrace::WasVisited( int vertIndex ) 
{
	unsigned short hashIndex = vertIndex & (CONVEX_HASH_SIZE-1);
	unsigned short id = VertIndexToID(vertIndex);
	if ( m_vertVisit[hashIndex].visitID == m_vertVisitID && m_vertVisit[hashIndex].vertID == id )
		return true;

	return false;
}

inline void CPhysicsTrace::NewVisit( void ) 
{ 
	m_vertVisitID++;
	if ( m_vertVisitID == 0 )
	{
		memset( m_vertVisit, 0, sizeof(m_vertVisit) );
	}

}



extern IVP_SurfaceManager *CreateSurfaceManager( const CPhysCollide *pCollisionModel, short &collideType );

#endif // PHYSICS_TRACE_H
