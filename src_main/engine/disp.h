//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef DISPINFO_H
#define DISPINFO_H

#ifdef _WIN32
#pragma once
#endif

//=============================================================================

#include <assert.h>
#include "idispinfo.h"
#include "bspfile.h"
#include "vmatrix.h"
#include "dispnode.h"
#include "builddisp.h"
#include "utlvector.h"
#include "disp_helpers.h"
#include "tier0/fasttimer.h"
#include "dlight.h"
#include "utllinkedlist.h"

struct model_t;
class IMesh;
class CMeshBuilder;
struct ShadowInfo_t;

class CDecalNodeSetupCache;

struct DispNodeInfo_t
{
	enum
	{
		// Indicates if any children at all have triangles
		CHILDREN_HAVE_TRIANGLES = 0x1
	};


	// Indicates which tesselation indices are associated with a node
	unsigned short	m_FirstTesselationIndex;
	unsigned char	m_Count;
	unsigned char	m_Flags;
};



class CDispInfo : public IDispInfo, public CDispUtilsHelper
{
// IDispInfo overrides.
public:
	virtual				~CDispInfo();

	virtual void		GetIntersectingSurfaces( GetIntersectingSurfaces_Struct *pStruct );
	virtual void		RenderWireframeInLightmapPage();
	
	// This is called when lightmaps are being built. If the displacement surface is using a
	// shader that wants alpha in the lightmap, it can set alpha here.
	// It won't fill in past &pLightmapPage[maxLMPagePixels].
	virtual void		UpdateLightmapAlpha( Vector4D *pLightmapPage, int maxLMPageLuxels );

	virtual void		GetBoundingBox( Vector& bbMin, Vector& bbMax );

	virtual void		SetParent( int surfID );
	virtual int			GetParent(); // returns surfID

	virtual void		SetNextInRenderChain( IDispInfo *pDispInfoNext );
	virtual IDispInfo*	GetNextInRenderChain();
	virtual void		SetNextInRayCastChain( IDispInfo *pDispInfoNext );
	virtual IDispInfo*	GetNextInRayCastChain();

	virtual void		ApplyTerrainMod( ITerrainMod *pMod );

	virtual int			ApplyTerrainMod_Speculative( 
		ITerrainMod *pMod,
		CSpeculativeTerrainModVert *pSpeculativeVerts,
		int nMaxVerts );

	virtual void		AddDynamicLights( Vector4D *blocklight );

	virtual DispDecalHandle_t	NotifyAddDecal( decal_t *pDecal );
	virtual void				NotifyRemoveDecal( DispDecalHandle_t h );
	virtual DispShadowHandle_t	AddShadowDecal( ShadowHandle_t shadowHandle );
	virtual void				RemoveShadowDecal( DispShadowHandle_t handle );

	// Compute shadow fragments for a particular shadow
	virtual bool		ComputeShadowFragments( DispShadowHandle_t h, int& vertexCount, int& indexCount );

	virtual bool		GetTag();
	virtual void		SetTag();
	
	virtual CDispLeafLink*	&GetLeafLinkHead();


public:

	//=========================================================================
	//
	// Construction/Decontruction
	//
	CDispInfo();

	// Used for indexing displacements.
	CDispInfo*	GetDispByIndex( int index )		{ return index == 0xFFFF ? 0 : &m_pDispArray->m_pDispInfos[index]; }
	
	// Get this displacement's index into the main array.
	int			GetDispIndex()					{ return this - m_pDispArray->m_pDispInfos; }


	//=========================================================================
	//
	// Flags
	//
	void		SetTouched( bool bTouched );
	bool		IsTouched( void );

	// Used for debugging. This reactivates all the active verts (which will try
	// to activate their dependencies). If all is well, then the only CDispInfo
	// that would need m_bInUse set would be this one.
	void		DebugResolveActiveVerts();

	// Rendering.
	void		ClearLOD();
	
	void		UpdateLOD( 
		bool bFullTesselation, 
		VMatrix const& viewMatrix, 
		Vector const &vViewOrigin,
		Vector const &vViewerSphereCenter,
		CDispInfo **retesselate, 
		int &nRetesselate );
	
	void		DrawDispAxes();
	bool		Render( CGroupMesh *pGroup );
	
	// Add in the contribution of a dynamic light.
	void		AddSingleDynamicLight( dlight_t& dl, Vector4D *blocklight );

	// Add in the contribution of a dynamic alpha light.
	void		AddSingleDynamicAlphaLight( dlight_t& dl, Vector4D *blocklight );

	// Cast a ray against this surface
	bool		TestRay( Ray_t const& ray, float start, float end, float& dist, 
						Vector2D* lightmapUV, Vector2D* texureUV );

// CDispUtilsHelper implementation.
public:

	virtual const CPowerInfo*		GetPowerInfo() const;
	virtual CDispNeighbor*			GetEdgeNeighbor( int index );
	virtual CDispCornerNeighbors*	GetCornerNeighbors( int index );
	virtual CDispUtilsHelper*		GetDispUtilsByIndex( int index );


// Initialization functions.
public:

	// These are used to mess with indices.
	int			VertIndex( int x, int y ) const;
	int			VertIndex( CVertIndex const &vert ) const;
	CVertIndex	IndexToVert( int index ) const;

	// Helpers to test decal intersection bit on decals.
	void		SetNodeIntersectsDecal( CDispDecal *pDispDecal, CVertIndex const &nodeIndex );
	int			GetNodeIntersectsDecal( CDispDecal *pDispDecal, CVertIndex const &nodeIndex );


public:

	int			GetDependencyList( unsigned short outList[MAX_TOTAL_DISP_DEPENDENCIES] );

	// Add another corner neighbor.
	void		AddExtraDependency( int iNeighborDisp );

	// Copy data from a ddispinfo_t.
	void		CopyMapDispData( const ddispinfo_t *pBuildDisp );

	// This is called from CreateStaticBuffers_All after the CCoreDispInfo is fully
	// initialized. It just copies the data that it needs.
	bool		CopyCoreDispData( 
		model_t *pWorld,
		const MaterialSystem_SortInfo_t *pSortInfos,
		const CCoreDispInfo *pInfo,
		bool bRestoring );

	// Checks the SURFDRAW_BUMPLIGHT flag and returns NUM_BUMP_VECTS+1 if it's set
	// and 1 if not.
	int			NumLightMaps();


// Rendering functions.
public:

	// Set m_BBoxMin and m_BBoxMax. Uses g_TempDispVerts and assumes m_LODs have been validated.
	void		UpdateBoundingBox();

	// Set m_vCenter and m_flRadius from the bounding box.
	void		UpdateCenterAndRadius();

	bool		IsInsideFrustum();

	// Number of verts per side.
	int			GetSideLength() const;

	// Return the total number of vertices.
	int			NumVerts() const;

	bool		IsTriangleFrontfacing( unsigned short const *indices );

	// Figure out the vector's projection in the decal's space.
	void		DecalProjectVert( Vector const &vPos, CDispDecalBase *pDispDecal, ShadowInfo_t const* pInfo, Vector &out );

	void		EndTriangle( 
		CVertIndex const &nodeIndex, 
		unsigned short *tempIndices,
		int &iCurTriVert );

	void		CullDecals( 
		int iNodeBit,
		CDispDecal **decals, 
		int nDecals, 
		CDispDecal **childDecals, 
		int &nChildDecals );

	// Tesselates a single node, doesn't deal with hierarchy
	void TesselateDisplacementNode( unsigned short *tempIndices, 
		CVertIndex const &nodeIndex, int iLevel, int* pActiveChildren );

	void		TesselateDisplacement_R(
		unsigned short *tempIndices,
		CVertIndex const &nodeIndex,
		int iNodeBitIndex,
		int iLevel
		 );

	void		TesselateDisplacement();

	// Pass all the mesh data in to the material system.
	void		SpecifyDynamicMesh();
	void		SpecifyWalkableDynamicMesh( void );
	void		SpecifyBuildableDynamicMesh( void );

	void		ActivateErrorTermVerts_R( 
		CVertIndex const &nodeIndex, 
		int iLevel );

	void		ActivateErrorTermVerts();

	void		MarkForRetesselation( CDispInfo **retesselate, int &nRetesselate );

	void		ActivateActiveNeighborVerts();

	// Clear all active verts except the corner verts.
	void		InitializeActiveVerts();

	// This function processes all the vertices an error terms in the tree and activates
	// the appropriate vertices to be rendered correctly.
	void		ActivateVerts( 
		bool bFullyTesselate, 
		const VMatrix &viewMatrix,
		Vector const &vViewOrigin
		 );
	
	bool		MeasureLineError( CVertIndex const &collapseVert );

	// Used to activate the specified vertex. Makes sure that any dependent vertices
	// and parents are activated.
	void		ResolveDependencies_R( CVertIndex const &index );

	// Returns a particular vertex
	CDispRenderVert* GetVertex( int i );

	// Methods to compute lightmap coordinates, texture coordinates,
	// and lightmap color based on displacement u,v
	void ComputeLightmapAndTextureCoordinate( RayDispOutput_t const& output, 
		Vector2D* luv, Vector2D* tuv );

	// This little beastie generate decal fragments
	void GenerateDecalFragments( CVertIndex const &nodeIndex, 
		int iNodeBitIndex, unsigned short decalHandle, CDispDecalBase *pDispDecal );

private:
	// Two functions for adding decals
	void TestAddDecalTri( unsigned short *tempIndices, unsigned short decalHandle, CDispDecal *pDispDecal );
	void TestAddDecalTri( unsigned short *tempIndices, unsigned short decalHandle, CDispShadowDecal *pDispDecal );

	// Allocates fragments...
	CDispDecalFragment* AllocateDispDecalFragment( DispDecalHandle_t h );

	// Clears decal fragment lists
	void ClearDecalFragments( DispDecalHandle_t h );
	void ClearAllDecalFragments();

	// Allocates fragments...
	CDispShadowFragment* AllocateShadowDecalFragment( DispShadowHandle_t h );

	// Clears decal fragment lists
	void ClearDecalFragments();

	// Clears shadow decal fragment lists
	void ClearShadowDecalFragments( DispShadowHandle_t h );
	void ClearAllShadowDecalFragments();

	// Used by GenerateDecalFragments
	void GenerateDecalFragments_R( CVertIndex const &nodeIndex, 
		int iNodeBitIndex, unsigned short decalHandle, CDispDecalBase *pDispDecal, int iLevel );

	// Used to create a bitfield to help cull decal tests
	void SetupDecalNodeIntersect( CVertIndex const &nodeIndex, int iNodeBitIndex,
		CDispDecalBase *pDispDecal,	ShadowInfo_t const* pInfo );

	// Used by SetupDecalNodeIntersect
	bool SetupDecalNodeIntersect_R( CVertIndex const &nodeIndex, int iNodeBitIndex, 
		CDispDecalBase *pDispDecal, ShadowInfo_t const* pInfo, int iLevel, CDecalNodeSetupCache* pCache );

public:

	// These bits store the state of the last error TestErrorTerms call.
	// These are the verts that MUST be in the displacement.
	CBitVec<CCoreDispInfo::MAX_VERT_COUNT>	m_ErrorVerts;

	// These bits tell which vertices and nodes are currently active.
	// These start out the same as m_ErrorVerts but have verts added if 
	// a neighbor needs to activate some verts.
	CBitVec<CCoreDispInfo::MAX_VERT_COUNT>	m_ActiveVerts;

	// These are set to 1 if the vert is allowed to become active.
	// This is what takes care of different-sized neighbors.
	CBitVec<CCoreDispInfo::MAX_VERT_COUNT>	m_AllowedVerts;

	int				m_idLMPage;						// lightmap page id

	int				m_ParentSurfID;					// parent surfaceID
	int				m_iPointStart;					// starting point (orientation) on base face

	int				m_iLightmapAlphaStart;

	int             m_Contents;                     // surface contents

public:

	bool            m_bTouched;                     // touched flag

	int             m_fSurfProp;                    // surface properties flag - bump-mapping, etc.

	IDispInfo		*m_pNext;						// for chaining
	IDispInfo		*m_pRayCastNext;				// for ray casting

	Vector			m_BBoxMin;
	Vector			m_BBoxMax;
	
	Vector			m_vCenter;			// Center of bounding box.
	float			m_flRadius;			// Radius of bounding box.

	int				m_Power;			// surface size (sides are 2^n+1).

	unsigned short	*m_pTags;			// property tags

	// List of all indices in the displacement in the current tesselation.
	// This never changes.
	CUtlVector<unsigned short>	m_Indices;
	int m_nIndices;		// The actual # of indices being used (it can be less than m_Indices.Count() if 
						// our LOD is reducing the triangle count).

	CUtlVector<CDispRenderVert> m_Verts;	// vectors that define the surface (size is NumVerts()).

	// Texcoordinates at the four corner points
	Vector2D		m_BaseSurfaceTexCoords[4];

	// Precalculated data for displacements of this size.
	const CPowerInfo	*m_pPowerInfo;	

	// Neighbor info for each side, indexed by NEIGHBOREDGE_ enums.
	// First 4 are edge neighbors, the rest are corners.
	CDispNeighbor			m_EdgeNeighbors[4];
	CDispCornerNeighbors	m_CornerNeighbors[4];

	// Copied from the ddispinfo. Speciifes where in g_DispLightmapSamplePositions the (compressed)
	// lightmap sample positions start.
	int				m_iLightmapSamplePositionStart;
	
	// Non-neighbor dependencies that need to updated when we LOD.
	unsigned short			m_ExtraDependencies[MAX_EXTRA_DEPENDENCIES];
	int						m_nExtraDependencies;
	// The current triangulation for visualizing tag data.
	int				m_nWalkIndexCount;
	unsigned short  *m_pWalkIndices;

	int				m_nBuildIndexCount;
	unsigned short  *m_pBuildIndices;

	// This here's a bunch of per-node information
	DispNodeInfo_t	*m_pNodeInfo;

	// Stores whether this dispinfo wants to be retesselated.
	bool			m_bRetesselate;

	// This is set to force it to be retesselated next frame (set when 
	// decals get added for instance).
	bool			m_bForceRetesselate;
	
	// Where the viewer was when we last tesselated.
	// When the viewer moves out of the sphere, UpdateLOD is called.
	Vector			m_ViewerSphereCenter;

	// Used to make sure it doesn't activate verts in the wrong dispinfos.
	bool			m_bInUse;

	// Set to true if the last UpdateLOD call said to fully tesselate.
	bool			m_bFullyTesselated;

	// This is set to true when the terrain is modified - it is forced to be
	// retesselated the next frame.
	bool			m_bForceRebuild;

	// Used to get material..
	CGroupMesh		*m_pMesh;
	int				m_iVertOffset;
	int				m_iIndexOffset;

	// Decals + Shadow decals
	DispDecalHandle_t	m_FirstDecal;
	DispShadowHandle_t	m_FirstShadowDecal;

	unsigned short	m_Index;	// helps in debugging
	
	// Current tag value.		
	unsigned short	m_Tag;
	CDispArray		*m_pDispArray;

	// Chain to link into leaves.
	CDispLeafLink	*m_pLeafLinkHead;

	// The frame code when we last updated LOD. Prevents us from updating it more than once per frame.
	int				m_LastUpdateFrameCount;

	// Cached once per frame
	static float	g_flDispRadius;	
};


extern int g_nDispTris;
extern CCycleCount g_DispRenderTime;


// --------------------------------------------------------------------------------- //
// CDispInfo functions.
// --------------------------------------------------------------------------------- //

inline int CDispInfo::GetSideLength() const
{
	return m_pPowerInfo->m_SideLength;
}


inline int CDispInfo::NumVerts() const
{
	Assert( m_Verts.Count() == m_pPowerInfo->m_MaxVerts );
	return m_pPowerInfo->m_MaxVerts;
}

//-----------------------------------------------------------------------------
// Returns a particular vertex
//-----------------------------------------------------------------------------

inline CDispRenderVert* CDispInfo::GetVertex( int i )
{
	assert( i < NumVerts() );
	return &m_Verts[i];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CDispInfo::SetTouched( bool bTouched )
{
	m_bTouched = bTouched;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool CDispInfo::IsTouched( void )
{
	return m_bTouched; 
}


inline int CDispInfo::VertIndex( int x, int y ) const
{
	assert( x >= 0 && x < GetSideLength() && y >= 0 && y < GetSideLength() );
	return y * GetSideLength() + x;
}


inline int CDispInfo::VertIndex( CVertIndex const &vert ) const
{
	assert( vert.x >= 0 && vert.x < GetSideLength() && vert.y >= 0 && vert.y < GetSideLength() );
	return vert.y * GetSideLength() + vert.x;
}


// This iterates over all the lightmap samples and for each one, calls:
// T::ProcessLightmapSample( Vector const &vPos, int t, int s, int tmax, int smax );
template<class T>
inline void IterateLightmapSamples( CDispInfo *pDisp, T &callback )
{
	ASSERT_SURF_VALID( pDisp->m_ParentSurfID );

	int smax = MSurf_LightmapExtents( pDisp->m_ParentSurfID )[0] + 1;
	int tmax = MSurf_LightmapExtents( pDisp->m_ParentSurfID )[1] + 1;

	unsigned char *pCurSample = &g_DispLightmapSamplePositions[pDisp->m_iLightmapSamplePositionStart];
	for( int t = 0 ; t<tmax ; t++ )
	{
		for( int s=0 ; s<smax ; s++ )
		{
			// Figure out what triangle this sample is on.
			// NOTE: this usually stores 4 bytes per lightmap sample.
			// It's a lot simpler and faster to just store the position but then it's
			// 16 bytes instead of 4.
			int iTri;
			if( *pCurSample == 255 )
			{
				++pCurSample;
				iTri = *pCurSample + 255;
			}
			else
			{
				iTri = *pCurSample;
			}
			++pCurSample;

			float a = (float)*(pCurSample++) / 255.1f;
			float b = (float)*(pCurSample++) / 255.1f;
			float c = (float)*(pCurSample++) / 255.1f;

			CTriInfo *pTri = &pDisp->m_pPowerInfo->m_pTriInfos[iTri];
			Vector vPos = 
				pDisp->m_Verts[pTri->m_Indices[0]].m_vPos * a +
				pDisp->m_Verts[pTri->m_Indices[1]].m_vPos * b +
				pDisp->m_Verts[pTri->m_Indices[2]].m_vPos * c;

			callback.ProcessLightmapSample( vPos, t, s, tmax, smax );
		}
	}
}

#endif // DISPINFO_H
