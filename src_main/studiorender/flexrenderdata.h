//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FLEXRENDERDATA_H
#define FLEXRENDERDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"
#include "utlvector.h"
#include "studio.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

struct mstudiomesh_t;

//-----------------------------------------------------------------------------
// Stores flex vertex data for the lifetime of the model rendering
//-----------------------------------------------------------------------------

struct CachedVertex_t
{
	Vector		m_Position;
	Vector		m_Normal;
	Vector4D	m_TangentS;

	CachedVertex_t() {}

	CachedVertex_t( CachedVertex_t const& src )
	{
		VectorCopy( src.m_Position, m_Position );
		VectorCopy( src.m_Normal, m_Normal );
		Vector4DCopy( src.m_TangentS, m_TangentS );
		Assert( m_TangentS.w == 1.0f || m_TangentS.w == -1.0f );
	}
};

class CCachedRenderData
{
public:
	// Constructor
	CCachedRenderData();

	// Call this when we start to render a new model
	void StartModel();

	// Used to hook ourselves into a particular body part, model, and mesh
	void SetBodyPart( int bodypart );
	void SetModel( int model );
	void SetMesh( int mesh );

	// For faster setup in the decal code
	void SetBodyModelMesh( int body, int model, int mesh );

	// Used to set up a flex computation
	bool IsFlexComputationDone( ) const;

	// Used to set up a computation (for world or flex data)
	void SetupComputation( mstudiomesh_t *pMesh, bool flexComputation = false );

	// Is a particular vertex flexed?
	bool IsVertexFlexed( int vertex ) const;

	// Checks to see if the vertex is defined
	bool IsVertexPositionCached( int vertex ) const;

	// Gets a flexed vertex
	CachedVertex_t* GetFlexVertex( int vertex );

	// Creates a new flexed vertex to be associated with a vertex
	CachedVertex_t* CreateFlexVertex( int vertex );

	// Gets a flexed vertex
	CachedVertex_t* GetWorldVertex( int vertex );

	// Creates a new flexed vertex to be associated with a vertex
	CachedVertex_t* CreateWorldVertex( int vertex );

private:
	// Used to create the flex render data. maps 
	struct CacheIndex_t
	{
		int				m_Tag;
		unsigned short	m_VertexIndex;
	};

	// A dictionary for the cached data
	struct CacheDict_t
	{
		unsigned short	m_FirstIndex;
		unsigned short	m_IndexCount;
		int				m_Tag;
		int				m_FlexTag;

		CacheDict_t() : m_Tag(0), m_FlexTag(0) {}
	};

	typedef CUtlVector< CacheDict_t >		CacheMeshDict_t;
	typedef CUtlVector< CacheMeshDict_t >	CacheModelDict_t;
	typedef CUtlVector<	CacheModelDict_t >	CacheBodyPartDict_t;

	// Flex data, temporarily allocated for the lifespan of rendering
	// Can't use UtlVector due to alignment issues
	int				m_FlexVertexCount;
	CachedVertex_t	m_pFlexVerts[MAXSTUDIOVERTS+1];

	// World data, temporarily allocated for the lifespan of rendering
	// Can't use UtlVector due to alignment issues
	int				m_WorldVertexCount;
	CachedVertex_t	m_pWorldVerts[MAXSTUDIOVERTS+1];

	// Maps actual mesh vertices into flex + world indices
	int				m_IndexCount;
	CacheIndex_t	m_pFlexIndex[MAXSTUDIOVERTS+1];
	CacheIndex_t	m_pWorldIndex[MAXSTUDIOVERTS+1];

	CacheBodyPartDict_t m_CacheDict;

	// The flex tag
	int m_CurrentTag;

	// the current body, model, and mesh
	int m_Body;
	int m_Model;
	int m_Mesh;

	// mapping for the current mesh to flex data
	CacheIndex_t*	m_pFirstFlexIndex;
	CacheIndex_t*	m_pFirstWorldIndex;
};


//-----------------------------------------------------------------------------
// Checks to see if the vertex is defined
//-----------------------------------------------------------------------------

inline bool CCachedRenderData::IsVertexFlexed( int vertex ) const
{
	return (m_pFirstFlexIndex && (m_pFirstFlexIndex[vertex].m_Tag == m_CurrentTag));
}

//-----------------------------------------------------------------------------
// Gets an existing flexed vertex associated with a vertex
//-----------------------------------------------------------------------------

inline CachedVertex_t* CCachedRenderData::GetFlexVertex( int vertex )
{
	assert( m_pFirstFlexIndex );
	assert( m_pFirstFlexIndex[vertex].m_Tag == m_CurrentTag );
	return &m_pFlexVerts[ m_pFirstFlexIndex[vertex].m_VertexIndex ];
}

//-----------------------------------------------------------------------------
// Checks to see if the vertex is defined
//-----------------------------------------------------------------------------

inline bool CCachedRenderData::IsVertexPositionCached( int vertex ) const
{
	return (m_pFirstWorldIndex && (m_pFirstWorldIndex[vertex].m_Tag == m_CurrentTag));
}

//-----------------------------------------------------------------------------
// Gets an existing world vertex associated with a vertex
//-----------------------------------------------------------------------------

inline CachedVertex_t* CCachedRenderData::GetWorldVertex( int vertex )
{
	assert( m_pFirstWorldIndex );
	assert( m_pFirstWorldIndex[vertex].m_Tag == m_CurrentTag );
	return &m_pWorldVerts[ m_pFirstWorldIndex[vertex].m_VertexIndex ];
}

//-----------------------------------------------------------------------------
// For faster setup in the decal code
//-----------------------------------------------------------------------------

inline void CCachedRenderData::SetBodyModelMesh( int body, int model, int mesh)
{
	m_Body = body;
	m_Model = model;
	m_Mesh = mesh;

	// At this point, we should have all 3 defined.
	CacheDict_t& dict = m_CacheDict[m_Body][m_Model][m_Mesh];

	if (dict.m_Tag == m_CurrentTag)
	{
		m_pFirstFlexIndex = &m_pFlexIndex[dict.m_FirstIndex];
		m_pFirstWorldIndex = &m_pWorldIndex[dict.m_FirstIndex];
	}
	else
	{
		m_pFirstFlexIndex = 0;
		m_pFirstWorldIndex = 0;
	}
}


#endif // FLEXRENDERDATA_H
