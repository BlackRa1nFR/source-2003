//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DISPCOLL_COMMON_H
#define DISPCOLL_COMMON_H
#pragma once

#include "trace.h"
#include "builddisp.h"
#include "terrainmod.h"


#define DISPCOLL_AABB_SIDE_COUNT	6
#define DISPCOLL_TRILIST_SIZE		256

class CDispLeafLink;
class ITerrainMod;
class CNodeVert;

struct RayDispOutput_t
{
	short	ndxVerts[4];	// 3 verts and a pad
	float	u, v;			// the u, v paramters (edgeU = v1 - v0, edgeV = v2 - v0)
	float	dist;			// intersection distance
};

//=============================================================================
//
// Displacement Collision Tree Data
//
class CDispCollTree
{
public:

	//=========================================================================
	//
	// Creation/Destruction
	//
	CDispCollTree();
	~CDispCollTree();

	virtual bool Create( CCoreDispInfo *pDisp );

	//=========================================================================
	//
	// Collision Interface
	//

	// NEW!!!
	bool RayTest( Ray_t const &ray, bool bOneSided = true );
	bool RayTest( Ray_t const &ray, RayDispOutput_t &output );

	bool RayTest( Vector const &rayStart, Vector const &rayEnd );  // return true/false no other collision info
	bool RayTest( Vector const &rayStart, Vector const &rayEnd, float startFrac, float endFrac, CBaseTrace *pTrace );

	bool RayTest( Ray_t &ray, Vector2D &texUV );

	bool AABBSweep( Vector const &rayStart, Vector const &rayEnd, Vector const &boxExtents, 
		            float startFrac, float endFrac, CBaseTrace *pTrace );
	bool AABBIntersect( Vector const &boxCenter, Vector const &boxMin, Vector const &boxMax );
	bool PointInBounds( Vector const &pos, Vector const &boxMin, Vector const &boxMax, bool bIsPoint );

	void ApplyTerrainMod( ITerrainMod *pMod );

	//=========================================================================
	//
	// Utility
	//
	inline void SetPower( int power );
	inline int GetPower( void );

	inline int GetWidth( void );
	inline int GetHeight( void );
	inline int GetSize( void );

	inline void SetCheckCount( int nDepth, int count );
	inline int GetCheckCount( int nDepth ) const;

	inline void GetStabDirection( Vector &dir );

	inline int GetContents( void );

	inline void GetBounds( Vector &boxMin, Vector &boxMax );
	inline void GetBoundsWithFace( Vector &boxMin, Vector &boxMax );

	inline CDispLeafLink*&	GetLeafLinkHead();

	inline void SetSurfaceProps( short surfaceProps )						{ m_SurfaceProps[0] = surfaceProps; }
	inline short GetSurfaceProps( void )									{ return m_SurfaceProps[0]; }

	inline void SetSurfaceProps2( short surfaceProps )						{ m_SurfaceProps[1] = surfaceProps; }
	inline short GetSurfaceProps2( void )									{ return m_SurfaceProps[1]; }

	inline short GetCollisionSurfProp( void )								{ return m_SurfaceProps[m_iLatestSurfProp]; }

	inline void SetTriFlags( short iTri, unsigned short nFlags )			{ m_pTris[iTri].m_nFlags = nFlags; }

protected:

	struct AABB_t
	{
		Vector*	m_Normals;
		float	m_Dists[DISPCOLL_AABB_SIDE_COUNT];
	};

	void AABB_Init( AABB_t &box );

	// Displacement collision triangle data.
	struct Tri_t
	{	
		Vector			m_vecNormal;				// triangle face plane normal
		float			m_flDist;					// traingle face plane distance
		unsigned short	m_uiVerts[3];				// triangle vert/vert normal indices
		unsigned short	m_nFlags;					// triangle surface flags
		unsigned short	m_iSurfProp;				// 0 or 1
	};	

	bool Tris_Alloc( void );
	void Tris_Free( void );
	void Tris_Init( void );
	void Tri_CalcPlane( short iTri );
	
	struct TriList_t
	{
		short	m_Count;
		Tri_t	*m_ppTriList[DISPCOLL_TRILIST_SIZE];
	};

	struct Node_t
	{
		Vector			m_BBox[2];
		short			m_iTris[2];
		int				m_fFlags;
	};

	void Node_Init( Node_t &node );
	inline void Node_SetLeaf( Node_t &node, bool bLeaf );
	inline bool Node_IsLeaf( Node_t const &node );

	void Ray_BuildTriList( Ray_t const &ray, int ndxNode, AABB_t &box, TriList_t &triList );
	bool Ray_NodeTest( Ray_t const &ray, AABB_t const &box );

	// Creation/Destruction
	void Leafs_Create();

	void Leafs_Create_R( 
		int iNode, 
		int iLevel,
		CNodeVert const &bottomLeft, 
		CNodeVert const &topRight );

	void Nodes_Create();
	void Nodes_CreateRecur( int ndxNode, int termLevel );
	void Nodes_CalcBounds( Node_t *pNode, int ndxNode );
	void Nodes_CalcLeafBounds( Node_t *pNode, int ndxNode );
	inline int Nodes_CalcCount( int power );
	inline int Nodes_GetIndexFromComponents( int x, int y );
	inline int Nodes_GetParent( int ndxNode );
	inline int Nodes_GetChild( int ndxNode, int direction );
	inline int Nodes_GetLevel( int ndxNode );
	int Nodes_Alloc( void );
	void Nodes_Free( void );

	void CalcFullBBox();
	int AllocVertData( void );
	void FreeVertData( void );

	// Collision Functions
	inline bool PointInBox( Vector const &pos, Vector const &boxMin, Vector &boxMax );

	void Ray_BuildTriList( Vector const &rayStart, Vector const &rayEnd, int ndxNode, AABB_t &AABBox, TriList_t &triList ); 
	bool FASTCALL Ray_NodeTest( Vector const &rayStart, Vector const &rayEnd, AABB_t const &AABBox );
	void Ray_IntersectTriList( Vector const &rayStart, Vector const &rayEnd, float startFrac, float endFrac, CBaseTrace *pTrace, TriList_t const &triList );
	bool Ray_IntersectTriListTest( Vector const &rayStart, Vector const &rayEnd, TriList_t const &triList );

	void AABB_BuildTriList( Vector const &boxCenter, Vector const &boxExtents, int ndxNode, TriList_t &triList );
	bool AABB_NodeTest( Vector const &boxCenter, Vector const &boxExtents, Node_t *pNode );
	bool AABB_IntersectTriList( Vector const &boxCenter, Vector const &boxExtents, TriList_t const &triList );
	
	void SweptAABB_BuildTriList( Vector const &rayStart, Vector const &rayEnd, Vector const &boxExtents, 
		                         int ndxNode, AABB_t &AABBox, TriList_t &triList );
	bool SweptAABB_NodeTest( Vector const &rayStart, Vector const &rayEnd,
							 Vector const &boxExtents, AABB_t const &AABBox );
	int SweptAABB_CullTriList( Vector const &rayStart, Vector const &rayEnd,
							   Vector const &boxExtents, TriList_t &triList );
	void SweptAABB_IntersectTriList( Vector const &rayStart, Vector const &rayEnd, 
									 Vector const &boxExtents, float startFrac, float endFrac, 
									 CBaseTrace *pTrace, TriList_t const &triList );


	bool SeparatingAxisAABoxTriangle( Vector const &boxCenter, Vector const &boxExtents,
						          Vector const &v1, Vector const &v2, Vector const &v3,
						          Vector const &planeNormal, float planeDist );


	void IntersectAABoxSweptTriangle( Vector const &boxStart, Vector const &boxEnd,
								      Vector const &boxExtents, Vector const &v1,
								      Vector const &v2, Vector const &v3,
								      Vector const &triNormal, float triDist, unsigned short triFlags, unsigned short triSurfProp,
								      /*float &fraction,*/ CBaseTrace *pTrace, bool bStartOutside );

	inline bool AxialPlanesXYZ( Vector const &v1, Vector const &v2, Vector const &v3,
								Vector const &boxStart, Vector const &boxEnd, Vector const &boxExtents,
								Vector const &sweptDir, Vector const &triNormal, float &fracStart, float &fracEnd,
								bool &bStartOutSolid, bool &bStartInSolid );
	inline bool FacePlane( Vector const &triNormal, float triDist,
						   Vector const &boxStart, Vector const &boxEnd, Vector const &boxExtents,
						   float &fracStart, float &fracEnd,
						  /* debug */ Vector const &v1, Vector const &v2, Vector const &v3,
						   bool &bStartOutSolid, bool &bEndOutSolid );
	inline bool EdgeCrossAxialX( Vector const &edge, Vector const &ptOnEdge,
								  Vector const &ptOffEdge, Vector const &boxExtents,
								  Vector const &boxStart, Vector const &boxEnd, Vector const &triNormal,
								  float triDist, float &fracStart, float &fracEnd, 
								  bool &bStartOutSolid, bool &bEndOutSolid, int index );
	inline bool EdgeCrossAxialY( Vector const &edge, Vector const &ptOnEdge,
								 Vector const &ptOffEdge, Vector const &boxExtents,
								 Vector const &boxStart, Vector const &boxEnd, Vector const &triNormal,
								 float triDist, float &fracStart, float &fracEnd, 
								 bool &bStartOutSolid, bool &bEndOutSolid, int index );
	inline bool EdgeCrossAxialZ( Vector const &edge, Vector const &ptOnEdge,
								  Vector const &ptOffEdge, Vector const &boxExtents,
								  Vector const &boxStart, Vector const &boxEnd, Vector const &triNormal,
								  float triDist, float &fracStart, float &fracEnd, 
								  bool &bStartOutSolid, bool &bEndOutSolid, int index );
	inline bool EdgeCrossSweptDir( Vector const &edge, Vector const &ptOnEdge,
								   Vector const &ptOffEdge, Vector const &sweptDir, 
								   Vector const &boxExtents, Vector const &boxStart, 
								   Vector const &boxEnd, float &fracStart, float &fracEnd );
	inline bool DirectionOfMotionCrossAxialPlanes( Vector const &boxStart, Vector const &boxEnd, Vector const &vDir,
												   Vector const &boxExtents, float &fracStart, float &fracEnd,
												   Vector const &v1, Vector const &v2, Vector const &v3 );

	/*inline*/ bool ResolveRayPlaneIntersect( float distStart, float distEnd, float &fracStart, float &fracEnd, Vector const &boxDir, 
		                                  Vector const &normal, float planeDist, bool &bStartOutSolid, bool &bEndOutSolid, int index );

	// utility
	int GetEdgePointIndex( CCoreDispInfo *pDisp, int edgeIndex, int edgePtIndex, bool bClockWise );
	bool VectorsEqualWithTolerance( Vector &v1, Vector &v2, float tolerance );

protected:

	enum { MAX_CHECK_COUNT_DEPTH = 2 };

	int					m_Power;				// size of the displacement ( 2^power + 1 )

	Vector				m_SurfPoints[4];		// Base surface points.
	int                 m_Contents;				// the displacement surface "contents" (solid, etc...)
	unsigned short		m_iLatestSurfProp;		//
	short				m_SurfaceProps[2];		// surface properties (save off from texdata for impact responses)

	Vector				m_StabDir;				// the direction to stab for this displacement surface (is the base face normal)
	Vector				m_BBoxWithFace[2];		// the bounding box of the displacement surface and base face

	int					m_CheckCount[MAX_CHECK_COUNT_DEPTH];			// per frame collision flag (so we check only once)
	
	short				m_VertCount;			// number of vertices on displacement collision surface
	Vector				*m_pVerts;				// list of displacement vertices
	Vector				*m_pOriginalVerts;		// Original vertex positions, used for limiting terrain mods.
	Vector				*m_pVertNormals;		// list of displacement vertex normals

	unsigned short		m_nTriCount;			// number of triangles on displacement collision surface
	Tri_t				*m_pTris;				// displacement surface triangles

	short				m_NodeCount;			// number of nodes in displacement collision tree
	Node_t				*m_pNodes;				// list of nodes

	CDispLeafLink		*m_pLeafLinkHead;		// List that links it into the leaves.
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CDispCollTree::SetPower( int power )
{
	m_Power = power;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetPower( void )
{
	return m_Power;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetWidth( void )
{
	return ( ( 1 << m_Power ) + 1 );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetHeight( void )
{
	return ( ( 1 << m_Power ) + 1 );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetSize( void )
{
    return ( ( ( 1 << m_Power ) + 1 ) * ( ( 1 << m_Power ) + 1 ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CDispCollTree::GetStabDirection( Vector &dir )
{
	dir = m_StabDir;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetContents( void )
{
	return m_Contents;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CDispCollTree::SetCheckCount( int nDepth, int count )
{
	Assert( (nDepth >= 0) && (nDepth < MAX_CHECK_COUNT_DEPTH) );
	m_CheckCount[nDepth] = count;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CDispCollTree::GetCheckCount( int nDepth ) const
{
	Assert( (nDepth >= 0) && (nDepth < MAX_CHECK_COUNT_DEPTH) );
	return m_CheckCount[nDepth];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CDispCollTree::GetBounds( Vector &boxMin, Vector &boxMax )
{
	boxMin = m_BBoxWithFace[0];
	boxMax = m_BBoxWithFace[1];
}


//-----------------------------------------------------------------------------
// Purpose: calculate the number of tree nodes given the size of the 
//          displacement surface
//   Input: power - size of the displacement surface
//  Output: int - the number of tree nodes
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_CalcCount( int power )
{
	return ( ( 1 << ( ( power + 1 ) << 1 ) ) / 3 );
}


inline CDispLeafLink*& CDispCollTree::GetLeafLinkHead()
{
	return m_pLeafLinkHead;
}


//=============================================================================
//
// Global Helper Functions
//
CDispCollTree *DispCollTrees_Alloc( int count );
void DispCollTrees_Free( CDispCollTree *pTrees );





#endif // DISPCOLL_COMMON_H