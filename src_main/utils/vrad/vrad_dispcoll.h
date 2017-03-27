//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VRAD_DISPCOLL_H
#define VRAD_DISPCOLL_H
#pragma once

#include <assert.h>
#include "DispColl_Common.h"

//=============================================================================
//
// VRAD specific collision
//
#define VRAD_QUAD_SIZE			4
#define DISPVERT_MAX_COUNT		289

class CVRADDispColl : public CDispCollTree
{
public:

	//=========================================================================
	//
	// Creation/Destruction Functions
	//
	CVRADDispColl();
	~CVRADDispColl();

	bool Create( CCoreDispInfo *pDisp );

	//=========================================================================
	//
	// Operations Functions
	//
	void BaseFacePlaneToDispUV( Vector const &planePt, Vector2D &dispUV );
	void DispUVToSurfPt( Vector2D const &dispUV, Vector &surfPt, float pushEps );
	void DispUVToSurfNormal( Vector2D const &dispUV, Vector &surfNormal );

	void ClosestBaseFaceData( Vector const &worldPt, Vector &closePt, Vector &closeNormal );

	void InitPatch( int ndxPatch, int ndxParentPatch, bool bFirst );
	bool MakePatch( int ndxPatch );

	bool GetMajorAxisUV( void );		// return u-axis major (true/false)

	//=========================================================================
	//
	// Attrib Functions
	//
	inline void SetParentIndex( int ndxParent );
	inline int GetParentIndex( void );
	inline void GetParentFaceNormal( Vector &normal );

	inline int GetPointCount( void );
	inline void GetPoint( int ndxPt, Vector &pt );
	inline void GetPointNormal( int ndxPt, Vector &ptNormal );
	inline int GetPointOffset( void ) { return m_PointOffset; }

	inline void GetVert( int ndxVert, Vector &v );
	inline void GetVertNormal( int ndxVert, Vector &normal );

	inline float GetSampleRadius2( void );
	inline void GetSampleBBox( Vector &boxMin, Vector &boxMax );

	inline float GetPatchSampleRadius2( void );

	inline Vector2D const& GetLuxelCoord( int i );

public:

	static  float	s_MinChopLength;
	static	float	s_MaxChopLength;

	Vector	m_LightmapOrigin;						// the lightmap origin in world space (base face relative)
	Vector	m_WorldToLightmapSpace[2];				// u = ( world - lightorigin ) dot worldToLightmap[0]
	Vector	m_LightmapToWorldSpace[2];				// world = lightorigin + u * lightmapToWorld[0] 

protected:

	void CalcSampleRadius2AndBox( dface_t *pFace );
	void GetMinorAxes( Vector const &vNormal, int &axis0, int &axis1 );
	void ClampUV( float &value );
	void GetSurfaceMinMax( Vector &boxMin, Vector &boxMax );

	void DispUVToSurfDiagonalBLtoTR( int snapU, int snapV, int nextU, int nextV,
									 float fracU, float fracV, Vector &surfPt, float pushEps );
	void DispUVToSurfDiagonalTLtoBR( int snapU, int snapV, int nextU, int nextV,
									 float fracU, float fracV, Vector &surfPt, float pushEps );

	void BuildVNodes( void );

	bool MakeParentPatch( int ndxPatch );
	bool MakeChildPatch( int ndxPatch );
	void GetNodesInPatch( int ndxPatch, int *pVNodes, int &vNodeCount );

protected:

	struct VNode_t
	{
		Vector		patchOrigin;
		Vector2D	patchOriginUV;
		Vector		patchNormal;
		float		patchArea;
		Vector		patchBounds[2];
	};

	int					m_ndxParent;							// parent index

	float				m_SampleRadius2;						// sampling radius
	float				m_PatchSampleRadius2;					// patch sampling radius (max bound)
	Vector				m_SampleBBox[2];						// sampling bounding box size

	int					m_PointCount;							// number of points in the face (should be 4!)
	Vector				m_Points[VRAD_QUAD_SIZE];				// points
	Vector				m_PointNormals[VRAD_QUAD_SIZE];			// normals at points

	VNode_t				*m_pVNodes;								// make an array parallel to the nodes array in the base class
														// and store vrad specific node data

	Vector2D*			m_pLuxelCoords;							// Lightmap coordinates
	int					m_PointOffset;

//	CUtlVector<int>		m_iNeighbors;					// list of neighboring indices
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CVRADDispColl::SetParentIndex( int ndxParent )
{
	m_ndxParent = ndxParent;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CVRADDispColl::GetParentIndex( void )
{
	return m_ndxParent;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CVRADDispColl::GetPointCount( void )
{
	return m_PointCount;
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CVRADDispColl::GetPoint( int ndxPt, Vector &pt )
{
	assert( ndxPt >= 0 );
	assert( ndxPt < VRAD_QUAD_SIZE );

	pt = m_Points[ndxPt];
}

	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CVRADDispColl::GetPointNormal( int ndxPt, Vector &ptNormal )
{
	assert( ndxPt >= 0 );
	assert( ndxPt < VRAD_QUAD_SIZE );

	ptNormal = m_PointNormals[ndxPt];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CVRADDispColl::GetParentFaceNormal( Vector &normal )
{
	Vector tmp[2];
	tmp[0] = m_Points[1] - m_Points[0];
	tmp[1] = m_Points[3] - m_Points[0];
	normal = CrossProduct( tmp[1], tmp[0] );
	VectorNormalize( normal ); 
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CVRADDispColl::GetVert( int ndxVert, Vector &v )
{
#ifdef _DEBUG
	int size = GetSize();
	assert( ndxVert >= 0 );
	assert( ndxVert < size );
#endif

	v = m_pVerts[ndxVert];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CVRADDispColl::GetVertNormal( int ndxVert, Vector &normal )
{
#ifdef _DEBUG
	int size = GetSize();
	assert( ndxVert >= 0 );
	assert( ndxVert < size );
#endif

	normal = m_pVertNormals[ndxVert];
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline float CVRADDispColl::GetSampleRadius2( void )
{
	return m_SampleRadius2;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline float CVRADDispColl::GetPatchSampleRadius2( void )
{
	return m_PatchSampleRadius2;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CVRADDispColl::GetSampleBBox( Vector &boxMin, Vector &boxMax )
{
	boxMin = m_SampleBBox[0];
	boxMax = m_SampleBBox[1];
}

//-----------------------------------------------------------------------------
// returns the luxel coordinate
//-----------------------------------------------------------------------------

inline Vector2D const& CVRADDispColl::GetLuxelCoord( int i )
{
	assert( m_pLuxelCoords );
	return m_pLuxelCoords[i]; 
}


#endif // VRAD_DISPCOLL_H