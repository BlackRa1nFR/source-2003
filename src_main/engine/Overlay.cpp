//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Model loading / unloading interface
//
// $NoKeywords: $
//=============================================================================

#include "Overlay.h"
#include "bspfile.h"
#include "modelloader.h"
#include "gl_matsysiface.h"
#include "enginestats.h"
#include "convar.h"
#include "glquake.h"
#include "materialsystem/IMesh.h"
#include "disp.h"
#include "collisionutils.h"
#include "tier0/vprof.h"


//-----------------------------------------------------------------------------
// Externs
//-----------------------------------------------------------------------------
extern int r_surfacevisframe;

//-----------------------------------------------------------------------------
// Convars
//-----------------------------------------------------------------------------
static ConVar r_renderoverlayfragment("r_renderoverlayfragment", "1");
static ConVar r_overlaywireframe( "r_overlaywireframe", "0" );


// Going away!
void Overlay_BuildBasisOrigin( Vector &vecBasisOrigin, int iSurface );
void Overlay_BuildBasis( const Vector &vecBasisNormal, Vector &vecBasisU, Vector &vecBasisV );
void Overlay_OverlayUVToOverlayPlane( const Vector &vecBasisOrigin, const Vector &vecBasisU,
									  const Vector &vecBasisV, const Vector &vecUVPoint,
									  Vector &vecPlanePoint );
void Overlay_WorldToOverlayPlane( const Vector &vecBasisOrigin, const Vector &vecBasisNormal,
								  const Vector &vecWorldPoint, Vector &vecPlanePoint );
void Overlay_OverlayPlaneToWorld( const Vector &vecBasisNormal, int iSurface,
								  const Vector &vecPlanePoint, Vector &vecWorldPoint );
void Overlay_DispUVToWorld( CDispInfo *pDisp, const Vector2D &vecUV, Vector &vecWorld );


void Overlay_TriTLToBR( CDispInfo *pDisp, Vector &vecWorld, float flU, float flV,
						int nSnapU, int nSnapV, int nWidth, int nHeight );
void Overlay_TriBLToTR( CDispInfo *pDisp, Vector &vecWorld, float flU, float flV,
			            int nSnapU, int nSnapV, int nWidth, int nHeight );


//-----------------------------------------------------------------------------
// Structures used to represent the overlay
//-----------------------------------------------------------------------------
typedef unsigned short OverlayFragmentList_t;

enum
{
	OVERLAY_FRAGMENT_LIST_INVALID = (OverlayFragmentList_t)~0,
};

enum
{
	NUM_OVERLAY_TEXCOORDS = 2,
};

struct overlayvert_t
{
	Vector		pos;
	Vector2D	texCoord[NUM_OVERLAY_TEXCOORDS];	// texcoord 0 = the mapped tex coord from worldcraft
													// texcoord 1 is used for alpha and maps the whole texture into the whole overlay
	float		lightCoord[2];
};

struct moverlayfragment_t
{
	int						m_nRenderFrameID;	// So we only render a fragment once a frame!
	int						m_iSurf;			// Surface Id
	int						m_iOverlay;			// Overlay Id
	OverlayFragmentHandle_t	m_hNextRender;
	unsigned short			m_nMaterialSortID;
	CUtlVector<overlayvert_t>	m_aPrimVerts;
};

struct moverlay_t
{
	int				m_nId;
	short			m_nTexInfo;
	OverlayFragmentList_t m_hFirstFragment;
	CUtlVector<int>	m_aFaces;
	float			m_flU[2];
	float			m_flV[2];
	Vector			m_vecUVPoints[4];
	Vector			m_vecOrigin;
	Vector			m_vecBasis[3];		// 0 = u, 1 = v, 2 = normal
};


//-----------------------------------------------------------------------------
// Overlay manager class
//-----------------------------------------------------------------------------
class COverlayMgr : public IOverlayMgr
{
public:
	struct OverlayPrimative_t
	{
		CUtlVector<overlayvert_t>		m_aPrimVerts;			// position, texture and lightmap coords
	};

	typedef CUtlVector<moverlayfragment_t*> OverlayFragmentVector_t;



public:
	COverlayMgr();
	~COverlayMgr();

	// Implementation of IOverlayMgr interface
	virtual bool	LoadOverlays( );
	virtual void	UnloadOverlays( );

	virtual void	CreateFragments( void );
	virtual void	AddFragmentListToRenderList( OverlayFragmentHandle_t iFragment );
	virtual void	RenderOverlays( );

private:
	// Create, destroy material sort order ids...
	int					GetMaterialSortID( IMaterial* pMaterial, int nLightmapPage );
	void				CleanupMaterial( unsigned short nSortOrder );

	moverlay_t			*GetOverlay( int iOverlay );
	moverlayfragment_t	*GetOverlayFragment( OverlayFragmentHandle_t iFragment );

	// Surfaces
	void				Surf_CreateFragments( moverlay_t *pOverlay, int iSurface );
	bool				Surf_PreClipFragment( moverlay_t *pOverlay, moverlayfragment_t &overlayFrag, int iSurface, moverlayfragment_t &surfaceFrag );
	void				Surf_PostClipFragment( moverlay_t *pOverlay, moverlayfragment_t &overlayFrag, int iSurface );
	void				Surf_ClipFragment( moverlay_t *pOverlay, moverlayfragment_t &overlayFrag, int iSurface, moverlayfragment_t &surfaceFrag );

	// Displacements
	void				Disp_CreateFragments( moverlay_t *pOverlay, int iSurface );
	bool				Disp_PreClipFragment( moverlay_t *pOverlay, OverlayFragmentVector_t &aDispFragments, int iSurface );
	void				Disp_PostClipFragment( CDispInfo *pDisp, moverlay_t *pOverlay, OverlayFragmentVector_t &aDispFragments, int iSurface );
	void				Disp_ClipFragment( CDispInfo *pDisp, OverlayFragmentVector_t &aDispFragments );
	void				Disp_DoClip( CDispInfo *pDisp, OverlayFragmentVector_t &aCurrentFragments, cplane_t &clipPlane, 
									 float clipDistStart, int nInterval, int nLoopStart, int nLoopEnd, int nLoopInc );

	// Utility
	OverlayFragmentHandle_t AddFragmentToFragmentList( int nSize );
	OverlayFragmentHandle_t AddFragmentToFragmentList( moverlayfragment_t *pSrc );

	moverlayfragment_t *CreateTempFragment( int nSize );
	moverlayfragment_t *CopyTempFragment( moverlayfragment_t *pSrc );
	void				DestroyTempFragment( moverlayfragment_t *pFragment );

	void				BuildClipPlanes( int iSurface, moverlayfragment_t &surfaceFrag, const Vector &vecBasisNormal, CUtlVector<cplane_t> &m_ClipPlanes );
	void				DoClipFragment( moverlayfragment_t *pFragment, cplane_t *pClipPlane, moverlayfragment_t **ppFront, moverlayfragment_t **ppBack );

	void				InitTexCoords( moverlay_t *pOverlay, moverlayfragment_t &overlayFrag );

private:
	enum
	{
		RENDER_QUEUE_INVALID = 0xFFFF
	};
	
	// Structures used to assign sort order handles
	struct RenderQueueHead_t
	{
		IMaterial *m_pMaterial;
		int m_nLightmapPage;

		OverlayFragmentHandle_t m_hFirstFragment;
		unsigned short m_nNextRenderQueue;	// Index of next queue that has stuff to render
		unsigned short m_nVertexCount;
		unsigned short m_nIndexCount;

		unsigned short m_nRefCount;
	};

	// First render queue to render
	unsigned short m_nFirstRenderQueue;

	// Used to assign sort order handles
	CUtlLinkedList<RenderQueueHead_t, unsigned short> m_RenderQueue;

	// All overlays
	CUtlVector<moverlay_t> m_aOverlays;

	// List of all overlay fragments. prev/next links point to the next fragment on a *surface*
	CUtlLinkedList< moverlayfragment_t, unsigned short > m_aFragments;

	// Used to find all fragments associated with a particular overlay
	CUtlLinkedList< OverlayFragmentHandle_t, unsigned short > m_OverlayFragments;
};


//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
static COverlayMgr g_OverlayMgr;
IOverlayMgr *OverlayMgr( void )
{
	return &g_OverlayMgr;
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
COverlayMgr::COverlayMgr()
{
	m_nFirstRenderQueue = RENDER_QUEUE_INVALID;
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
COverlayMgr::~COverlayMgr()
{
	UnloadOverlays();
}


//-----------------------------------------------------------------------------
// Returns a particular overlay
//-----------------------------------------------------------------------------
inline moverlay_t *COverlayMgr::GetOverlay( int iOverlay )
{
	return &m_aOverlays[iOverlay];
}

//-----------------------------------------------------------------------------
// Returns a particular overlay fragment
//-----------------------------------------------------------------------------
inline moverlayfragment_t *COverlayMgr::GetOverlayFragment( OverlayFragmentHandle_t iFragment )
{
	return &m_aFragments[iFragment];
}


//-----------------------------------------------------------------------------
// Cleanup overlays
//-----------------------------------------------------------------------------
void COverlayMgr::UnloadOverlays( )
{
	for ( int i = m_RenderQueue.Count(); --i >= 0; )
	{
		m_RenderQueue[i].m_pMaterial->DecrementReferenceCount();
	}

	m_aOverlays.Purge();
	m_aFragments.Purge();
	m_OverlayFragments.Purge();
	m_RenderQueue.Purge();
	m_nFirstRenderQueue = RENDER_QUEUE_INVALID;
}


//-----------------------------------------------------------------------------
// Create, destroy material sort order ids...
//-----------------------------------------------------------------------------
int COverlayMgr::GetMaterialSortID( IMaterial* pMaterial, int nLightmapPage )
{
	// Search the sort order handles for an enumeration id match (means materials + lightmaps match)
	unsigned short i;
	for ( i = m_RenderQueue.Head(); i != m_RenderQueue.InvalidIndex();
		i = m_RenderQueue.Next(i) )
	{
		// Found a match, lets increment the refcount of this sort order id
		if ((m_RenderQueue[i].m_pMaterial == pMaterial) && (m_RenderQueue[i].m_nLightmapPage == nLightmapPage))
		{
			++m_RenderQueue[i].m_nRefCount;
			return i;
		}
	}

	// Didn't find it, lets assign a new sort order ID, with a refcount of 1
	i = m_RenderQueue.AddToTail();
	RenderQueueHead_t &renderQueue = m_RenderQueue[i];

	renderQueue.m_pMaterial = pMaterial;
	renderQueue.m_nLightmapPage = nLightmapPage;
	renderQueue.m_nRefCount = 1;
	renderQueue.m_hFirstFragment = OVERLAY_FRAGMENT_INVALID;
	renderQueue.m_nNextRenderQueue = RENDER_QUEUE_INVALID;
	renderQueue.m_nVertexCount = 0;
	renderQueue.m_nIndexCount = 0;

	pMaterial->IncrementReferenceCount();

	return i;
}

void COverlayMgr::CleanupMaterial( unsigned short nSortOrder )
{
	RenderQueueHead_t &renderQueue = m_RenderQueue[nSortOrder];

	// Shouldn't be cleaning up while we've got a render list
	Assert( renderQueue.m_nVertexCount == 0 );

	// Decrease the sort order reference count
	if (--renderQueue.m_nRefCount <= 0)
	{
		renderQueue.m_pMaterial->DecrementReferenceCount();

		// No one referencing the sort order number?
		// Then lets clean up the sort order id
		m_RenderQueue.Remove(nSortOrder);
	}
}


//-----------------------------------------------------------------------------
// Adds the fragment list to the list of fragments to render when RenderOverlays is called
//-----------------------------------------------------------------------------
void COverlayMgr::AddFragmentListToRenderList( OverlayFragmentHandle_t iFragment )
{
	OverlayFragmentHandle_t i;
	for ( i = iFragment; i != OVERLAY_FRAGMENT_INVALID; i = m_aFragments.Next(i) )
	{
		// Make sure we don't add the fragment twice...
		moverlayfragment_t *pFragment = GetOverlayFragment(i);
		if ( pFragment->m_nRenderFrameID == r_surfacevisframe )
			continue;

		// Triangle count too low? Skip it...
		int nVertexCount = pFragment->m_aPrimVerts.Count();
		if ( nVertexCount < 3 )
			continue;

		// Update the frame count.
		pFragment->m_nRenderFrameID = r_surfacevisframe;

		// Determine the material associated with the fragment...
		int nMaterialSortID = pFragment->m_nMaterialSortID;

		// Insert the render queue into the list of render queues to render
		RenderQueueHead_t &renderQueue = m_RenderQueue[nMaterialSortID];
		if ( renderQueue.m_hFirstFragment == OVERLAY_FRAGMENT_INVALID )
		{
			renderQueue.m_nNextRenderQueue = m_nFirstRenderQueue;
			m_nFirstRenderQueue = nMaterialSortID;
		}

		// Add to list of fragments for this surface
		// NOTE: Render them in *reverse* order in which they appeared in the list
		// because they are stored in the list in *reverse* order in which they should be rendered.

		// Add the fragment to the bucket of fragments to render...
		pFragment->m_hNextRender = renderQueue.m_hFirstFragment;
		renderQueue.m_hFirstFragment = i;

		Assert( renderQueue.m_nVertexCount + nVertexCount < 65535 );
		renderQueue.m_nVertexCount += nVertexCount;
		renderQueue.m_nIndexCount += 3 * (nVertexCount - 2);
	}
}


//-----------------------------------------------------------------------------
// Renders all queued up overlays
//-----------------------------------------------------------------------------
void COverlayMgr::RenderOverlays( )
{
	VPROF_BUDGET( "COverlayMgr::RenderOverlays", "Overlays" );

	int nNextRenderQueue;
	if (r_renderoverlayfragment.GetInt() == 0)
	{
		for( int i = m_nFirstRenderQueue; i != RENDER_QUEUE_INVALID; i = nNextRenderQueue )
		{
 			RenderQueueHead_t &renderQueue = m_RenderQueue[i];
			nNextRenderQueue = renderQueue.m_nNextRenderQueue;

			// Clean up the render queue for next time...
			renderQueue.m_nVertexCount = 0;
			renderQueue.m_nIndexCount = 0;
			renderQueue.m_hFirstFragment = OVERLAY_FRAGMENT_INVALID;
			renderQueue.m_nNextRenderQueue = RENDER_QUEUE_INVALID;
		}

		m_nFirstRenderQueue = RENDER_QUEUE_INVALID;
		return;
	}

	bool bWireframeFragments = ( r_overlaywireframe.GetInt() != 0 );
	if ( bWireframeFragments )
	{
		materialSystemInterface->Bind( g_materialWorldWireframe );
	}

	// Render sorted by material + lightmap...
	for( int i = m_nFirstRenderQueue; i != RENDER_QUEUE_INVALID; i = nNextRenderQueue )
	{
		RenderQueueHead_t &renderQueue = m_RenderQueue[i];
		nNextRenderQueue = renderQueue.m_nNextRenderQueue;

		Assert( renderQueue.m_nVertexCount > 0 );

		if ( !bWireframeFragments )
		{
			materialSystemInterface->Bind( renderQueue.m_pMaterial, NULL /*proxy*/ );
			materialSystemInterface->BindLightmapPage( renderQueue.m_nLightmapPage );
		}
		
		// Create the mesh/mesh builder.
		IMesh* pMesh = materialSystemInterface->GetDynamicMesh();

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, renderQueue.m_nVertexCount, renderQueue.m_nIndexCount );
		
		int nIndex = 0;

		// We just need to make sure there's a unique sort ID for that. Then we bind once per queue
		OverlayFragmentHandle_t hFragment = renderQueue.m_hFirstFragment;
		for ( ; hFragment != OVERLAY_FRAGMENT_INVALID; hFragment = m_aFragments[hFragment].m_hNextRender )
		{
			moverlayfragment_t *pFragment = &m_aFragments[hFragment];
			int nVertCount = pFragment->m_aPrimVerts.Count();
			const overlayvert_t *pVert = &(pFragment->m_aPrimVerts[0]);

			int iVert;
			for ( iVert = 0; iVert < nVertCount; ++iVert, ++pVert )
			{
				meshBuilder.Position3fv( pVert->pos.Base() );
				meshBuilder.TexCoord2fv( 0, pVert->texCoord[0].Base() );
				meshBuilder.TexCoord2fv( 1, pVert->lightCoord );
				meshBuilder.TexCoord2fv( 2, pVert->texCoord[1].Base() );
				meshBuilder.AdvanceVertex();
			}

			// FIXME: Make this part of a single loop?
			nVertCount -= 2;
			for ( iVert = 0; iVert < nVertCount; ++iVert )
			{
				meshBuilder.FastIndex( nIndex );
				meshBuilder.FastIndex( nIndex + iVert + 1 );
				meshBuilder.FastIndex( nIndex + iVert + 2 );
			}
			nVertCount += 2;
			
			nIndex += nVertCount;
		}

		meshBuilder.End();
		pMesh->Draw();

		// Clean up the render queue for next time...
		renderQueue.m_nVertexCount = 0;
		renderQueue.m_nIndexCount = 0;
		renderQueue.m_hFirstFragment = OVERLAY_FRAGMENT_INVALID;
		renderQueue.m_nNextRenderQueue = RENDER_QUEUE_INVALID;
	}
	
	m_nFirstRenderQueue = RENDER_QUEUE_INVALID;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool COverlayMgr::Surf_PreClipFragment( moverlay_t *pOverlay, moverlayfragment_t &overlayFrag, 
								        int iSurface, moverlayfragment_t &surfaceFrag )
{
	// Convert the overlay uv points to overlay plane points.
	overlayFrag.m_aPrimVerts.SetCount( 4 );
	for( int iVert = 0; iVert < 4; ++iVert )
	{
		Overlay_OverlayUVToOverlayPlane( pOverlay->m_vecOrigin, pOverlay->m_vecBasis[0], 
			                             pOverlay->m_vecBasis[1], pOverlay->m_vecUVPoints[iVert],
										 overlayFrag.m_aPrimVerts[iVert].pos );
	}

	// Overlay texture coordinates.
	InitTexCoords( pOverlay, overlayFrag );

	// Surface
	int nVertCount = surfaceFrag.m_aPrimVerts.Count();
	for ( iVert = 0; iVert < nVertCount; ++iVert )
	{
		// Position.
		Overlay_WorldToOverlayPlane( pOverlay->m_vecOrigin, pOverlay->m_vecBasis[2], 
			                         surfaceFrag.m_aPrimVerts[iVert].pos, surfaceFrag.m_aPrimVerts[iVert].pos );
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void COverlayMgr::Surf_PostClipFragment( moverlay_t *pOverlay, moverlayfragment_t &overlayFrag,
								         int iSurface )
{
	// Get fragment vertex count.
	int nVertCount = overlayFrag.m_aPrimVerts.Count();

	if ( nVertCount == 0 )
		return;

	// Create fragment.
	OverlayFragmentHandle_t hFragment = AddFragmentToFragmentList( nVertCount );
	moverlayfragment_t *pFragment = GetOverlayFragment( hFragment );

	// Get surface context.
	SurfaceCtx_t ctx;
	SurfSetupSurfaceContext( ctx, iSurface );	

	pFragment->m_iOverlay = pOverlay->m_nId;
	pFragment->m_iSurf = iSurface;

	moverlayfragment_t origOverlay;
	origOverlay.m_aPrimVerts.SetSize( 4 );
	for ( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		Overlay_OverlayUVToOverlayPlane( pOverlay->m_vecOrigin, pOverlay->m_vecBasis[0], 
			                             pOverlay->m_vecBasis[1], pOverlay->m_vecUVPoints[iPoint],
										 origOverlay.m_aPrimVerts[iPoint].pos );
	}
	InitTexCoords( pOverlay, origOverlay );

	for ( int iVert = 0; iVert < nVertCount; ++iVert )
	{
		Vector2D vecUV;
		PointInQuadToBarycentric( origOverlay.m_aPrimVerts[0].pos, 
								  origOverlay.m_aPrimVerts[3].pos, 
			                      origOverlay.m_aPrimVerts[2].pos, 
								  origOverlay.m_aPrimVerts[1].pos,
								  overlayFrag.m_aPrimVerts[iVert].pos, vecUV );

		Overlay_OverlayPlaneToWorld( pOverlay->m_vecBasis[2], iSurface, 
			                         overlayFrag.m_aPrimVerts[iVert].pos,
			                         pFragment->m_aPrimVerts[iVert].pos );

		// Texture coordinates.
		Vector2D vecTexCoord;
		for ( int iTexCoord=0; iTexCoord < NUM_OVERLAY_TEXCOORDS; iTexCoord++ )
		{
			TexCoordInQuadFromBarycentric( origOverlay.m_aPrimVerts[0].texCoord[iTexCoord], origOverlay.m_aPrimVerts[3].texCoord[iTexCoord],
										origOverlay.m_aPrimVerts[2].texCoord[iTexCoord], origOverlay.m_aPrimVerts[1].texCoord[iTexCoord],
										vecUV, vecTexCoord );

			pFragment->m_aPrimVerts[iVert].texCoord[iTexCoord][0] = vecTexCoord.x;
			pFragment->m_aPrimVerts[iVert].texCoord[iTexCoord][1] = vecTexCoord.y;
		}

		// Lightmap coordinates.
		Vector2D uv;
		SurfComputeLightmapCoordinate( ctx, iSurface, pFragment->m_aPrimVerts[iVert].pos, uv );
		pFragment->m_aPrimVerts[iVert].lightCoord[0] = uv.x;
		pFragment->m_aPrimVerts[iVert].lightCoord[1] = uv.y;
	}

	// Create the sort ID for this fragment
	const MaterialSystem_SortInfo_t &sortInfo = materialSortInfoArray[MSurf_MaterialSortID( iSurface )];
	mtexinfo_t *pTexInfo = &host_state.worldmodel->brush.texinfo[pOverlay->m_nTexInfo];
	pFragment->m_nMaterialSortID = GetMaterialSortID( pTexInfo->material, sortInfo.lightmapPageID );

	// Add to list of fragments for this overlay
	OverlayFragmentList_t i = m_OverlayFragments.Alloc( true );
	m_OverlayFragments[i] = hFragment;
	m_OverlayFragments.LinkBefore( pOverlay->m_hFirstFragment, i );
	pOverlay->m_hFirstFragment = i;

	// Add to list of fragments for this surface
	// NOTE: Store them in *reverse* order so that when we pull them off for
	// rendering, we can do *that* in reverse order too? Reduces the amount of iteration necessary
	// Therefore, we need to add to the head of the list
	m_aFragments.LinkBefore( MSurf_OverlayFragmentList( iSurface ), hFragment );
	MSurf_OverlayFragmentList( iSurface ) = hFragment;
}


//-----------------------------------------------------------------------------
// Clips an overlay to a surface
//-----------------------------------------------------------------------------
void COverlayMgr::Surf_ClipFragment( moverlay_t *pOverlay, moverlayfragment_t &overlayFrag, 
								     int iSurface, moverlayfragment_t &surfaceFrag )
{
	// Create the clip planes.
	CUtlVector<cplane_t> m_ClipPlanes;
	BuildClipPlanes( iSurface, surfaceFrag, pOverlay->m_vecBasis[2], m_ClipPlanes );

	// Copy the overlay fragment (initial clipped fragment).
	moverlayfragment_t *pClippedFrag = CopyTempFragment( &overlayFrag );

	for( int iPlane = 0; iPlane < m_ClipPlanes.Count(); ++iPlane )
	{
		moverlayfragment_t *pFront = NULL, *pBack = NULL;
		DoClipFragment( pClippedFrag, &m_ClipPlanes[iPlane], &pFront, &pBack );
		DestroyTempFragment( pClippedFrag );
		pClippedFrag = NULL;
		
		// Keep the backside and continue clipping.
		if ( pBack )
		{
			pClippedFrag = pBack;
		}

		if ( pFront )
		{
			DestroyTempFragment( pFront );
		}
	}

	m_ClipPlanes.Purge();

	// Copy the clipped polygon back to the overlay frag.
	overlayFrag.m_aPrimVerts.RemoveAll();
	if ( pClippedFrag )
	{
		overlayFrag.m_aPrimVerts.SetCount( pClippedFrag->m_aPrimVerts.Count() );
		for ( int iVert = 0; iVert < pClippedFrag->m_aPrimVerts.Count(); ++iVert )
		{
			overlayFrag.m_aPrimVerts[iVert].pos = pClippedFrag->m_aPrimVerts[iVert].pos;
			memcpy( overlayFrag.m_aPrimVerts[iVert].texCoord, pClippedFrag->m_aPrimVerts[iVert].texCoord, sizeof( overlayFrag.m_aPrimVerts[iVert].texCoord ) );
		}
	}

	DestroyTempFragment( pClippedFrag );
}


//-----------------------------------------------------------------------------
// Creates overlay fragments for a particular surface
//-----------------------------------------------------------------------------
void COverlayMgr::Surf_CreateFragments( moverlay_t *pOverlay, int iSurface )
{
	moverlayfragment_t overlayFrag, surfaceFrag;

	// The faces get fan tesselated into triangles when rendered - do the same to
	// create the fragments!
	int iFirstVert = MSurf_FirstVertIndex( iSurface );
	
	int nSurfTriangleCount = MSurf_VertCount( iSurface ) - 2;
	for( int iTri = 0; iTri < nSurfTriangleCount; ++iTri )
	{
		// 3 Points in a triangle.
		surfaceFrag.m_aPrimVerts.SetCount( 3 );
		
		int iVert = host_state.worldmodel->brush.vertindices[(iFirstVert)];
		mvertex_t *pVert = &host_state.worldmodel->brush.vertexes[iVert];
		surfaceFrag.m_aPrimVerts[0].pos = pVert->position;
		
		iVert = host_state.worldmodel->brush.vertindices[(iFirstVert+iTri+1)];
		pVert = &host_state.worldmodel->brush.vertexes[iVert];
		surfaceFrag.m_aPrimVerts[1].pos = pVert->position;
		
		iVert = host_state.worldmodel->brush.vertindices[(iFirstVert+iTri+2)];
		pVert = &host_state.worldmodel->brush.vertexes[iVert];
		surfaceFrag.m_aPrimVerts[2].pos = pVert->position;
		
		if ( Surf_PreClipFragment( pOverlay, overlayFrag, iSurface, surfaceFrag ) )
		{
			Surf_ClipFragment( pOverlay, overlayFrag, iSurface, surfaceFrag );
			Surf_PostClipFragment( pOverlay, overlayFrag, iSurface );
		}
		
		// Clean up!
		surfaceFrag.m_aPrimVerts.RemoveAll();
		overlayFrag.m_aPrimVerts.RemoveAll();
	}
}


//-----------------------------------------------------------------------------
// Creates fragments from the overlays loaded in from file
//-----------------------------------------------------------------------------
void COverlayMgr::CreateFragments( void )
{
	int nOverlayCount = m_aOverlays.Count();
	for ( int iOverlay = 0; iOverlay < nOverlayCount; ++iOverlay )
	{
		moverlay_t *pOverlay = &m_aOverlays.Element( iOverlay );
		int nFaceCount = pOverlay->m_aFaces.Count();
		if ( nFaceCount == 0 )
			continue;

		// Initial face defines the face(overlay) plane.
		int iSurface = pOverlay->m_aFaces[0];

		// Build the overlay basis.
		Overlay_BuildBasisOrigin( pOverlay->m_vecOrigin, iSurface );
		Overlay_BuildBasis( pOverlay->m_vecBasis[2], pOverlay->m_vecBasis[0], pOverlay->m_vecBasis[1] );

		// Clip against each face in the face list.
		for( int iFace = 0; iFace < nFaceCount; ++iFace )
		{
			int iSurface = pOverlay->m_aFaces[iFace];
		
			if ( SurfaceHasDispInfo( iSurface ) )
			{
				Disp_CreateFragments( pOverlay, iSurface );
			}
			else
			{
				Surf_CreateFragments( pOverlay, iSurface );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Loads overlays from the lump
//-----------------------------------------------------------------------------
bool COverlayMgr::LoadOverlays( )
{
	CMapLoadHelper lh( LUMP_OVERLAYS );

	doverlay_t	*pOverlayIn;

	pOverlayIn = ( doverlay_t* )lh.LumpBase();
	if ( lh.LumpSize() % sizeof( doverlay_t ) )
		return false;

	int nOverlayCount = lh.LumpSize() / sizeof( doverlay_t );

	// Memory allocation!
	m_aOverlays.SetSize( nOverlayCount );

	for( int iOverlay = 0; iOverlay < nOverlayCount; ++iOverlay, ++pOverlayIn )
	{
		moverlay_t *pOverlayOut = &m_aOverlays.Element( iOverlay );

		pOverlayOut->m_nId = iOverlay;
		pOverlayOut->m_nTexInfo = pOverlayIn->nTexInfo;

		pOverlayOut->m_flU[0] = pOverlayIn->flU[0];
		pOverlayOut->m_flU[1] = pOverlayIn->flU[1];
		pOverlayOut->m_flV[0] = pOverlayIn->flV[0];
		pOverlayOut->m_flV[1] = pOverlayIn->flV[1];

		VectorCopy( pOverlayIn->vecOrigin, pOverlayOut->m_vecOrigin );

		VectorCopy( pOverlayIn->vecUVPoints[0], pOverlayOut->m_vecUVPoints[0] );
		VectorCopy( pOverlayIn->vecUVPoints[1], pOverlayOut->m_vecUVPoints[1] );
		VectorCopy( pOverlayIn->vecUVPoints[2], pOverlayOut->m_vecUVPoints[2] );
		VectorCopy( pOverlayIn->vecUVPoints[3], pOverlayOut->m_vecUVPoints[3] );

		VectorCopy( pOverlayIn->vecBasisNormal, pOverlayOut->m_vecBasis[2] );

		pOverlayOut->m_aFaces.SetSize( pOverlayIn->nFaceCount );
		for( int iFace = 0; iFace < pOverlayIn->nFaceCount; ++iFace )
		{
			pOverlayOut->m_aFaces[iFace] = pOverlayIn->aFaces[iFace];
		}

		pOverlayOut->m_hFirstFragment = OVERLAY_FRAGMENT_LIST_INVALID;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void COverlayMgr::Disp_CreateFragments( moverlay_t *pOverlay, int iSurface )
{
	OverlayFragmentVector_t aDispFragments;

	if ( Disp_PreClipFragment( pOverlay, aDispFragments, iSurface ) )
	{
		IDispInfo *pIDisp = MSurf_DispInfo( iSurface );
		CDispInfo *pDisp = static_cast<CDispInfo*>( pIDisp );
		if ( pDisp )
		{
			Disp_ClipFragment( pDisp, aDispFragments );
			Disp_PostClipFragment( pDisp, pOverlay, aDispFragments, iSurface );
		}
	}

	for ( int i = aDispFragments.Count(); --i >= 0; )
	{
		DestroyTempFragment( aDispFragments[i] );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool COverlayMgr::Disp_PreClipFragment( moverlay_t *pOverlay, OverlayFragmentVector_t &aDispFragments, 
								        int iSurface )
{
	// The faces are not tesselated when they are displaced faces.
	int iFirstVert = MSurf_FirstVertIndex( iSurface );

	// Displaced faces are quads.
	moverlayfragment_t surfaceFrag;
	surfaceFrag.m_aPrimVerts.SetCount( 4 );
	for( int iVert = 0; iVert < 4; ++iVert )
	{
		int iVertex = host_state.worldmodel->brush.vertindices[(iFirstVert+iVert)];
		mvertex_t *pVert = &host_state.worldmodel->brush.vertexes[iVertex];
		surfaceFrag.m_aPrimVerts[iVert].pos = pVert->position;
	}

	// Setup the base fragment to be clipped by the base surface previous to the
	// displaced surface.
	moverlayfragment_t overlayFrag;
	if ( !Surf_PreClipFragment( pOverlay, overlayFrag, iSurface, surfaceFrag ) )
		return false;

	Surf_ClipFragment( pOverlay, overlayFrag, iSurface, surfaceFrag );

	// Get fragment vertex count.
	int nVertCount = overlayFrag.m_aPrimVerts.Count();
	if ( nVertCount == 0 )
		return false;

	// Setup
	moverlayfragment_t *pFragment = CopyTempFragment( &overlayFrag );
	aDispFragments.AddToTail( pFragment );

	IDispInfo *pIDispInfo = MSurf_DispInfo( iSurface );
	CDispInfo *pDispInfo = static_cast<CDispInfo*>( pIDispInfo );
	int iPointStart = pDispInfo->m_iPointStart;

	Vector2D vecTmpUV;
	for ( iVert = 0; iVert < nVertCount; ++iVert )
	{
		PointInQuadToBarycentric( surfaceFrag.m_aPrimVerts[iPointStart].pos,
								  surfaceFrag.m_aPrimVerts[(iPointStart+3)%4].pos,
								  surfaceFrag.m_aPrimVerts[(iPointStart+2)%4].pos,
								  surfaceFrag.m_aPrimVerts[(iPointStart+1)%4].pos,
								  overlayFrag.m_aPrimVerts[iVert].pos,
								  vecTmpUV );

		pFragment->m_aPrimVerts[iVert].pos.x = vecTmpUV.x;
		pFragment->m_aPrimVerts[iVert].pos.y = vecTmpUV.y;
		pFragment->m_aPrimVerts[iVert].pos.z = 0.0f;
	}
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void COverlayMgr::Disp_PostClipFragment( CDispInfo *pDisp, moverlay_t *pOverlay, 
										 OverlayFragmentVector_t &aDispFragments, int iSurface )
{
	if ( aDispFragments.Count() == 0 )
		return;

	// Get surface context.
	SurfaceCtx_t ctx;
	SurfSetupSurfaceContext( ctx, iSurface );	

	// The faces are not tesselated when they are displaced faces.
	int iFirstVert = MSurf_FirstVertIndex( iSurface );

	// Displaced faces are quads.
	moverlayfragment_t surfaceFrag;
	surfaceFrag.m_aPrimVerts.SetCount( 4 );
	for( int iVert = 0; iVert < 4; ++iVert )
	{
		int iVertex = host_state.worldmodel->brush.vertindices[(iFirstVert+iVert)];
		mvertex_t *pVert = &host_state.worldmodel->brush.vertexes[iVertex];
		surfaceFrag.m_aPrimVerts[iVert].pos = pVert->position;
	}

	Vector2D lightCoords[4];
	int nInterval = pDisp->GetSideLength();
	lightCoords[0].x = pDisp->GetVertex( 0 )->m_LMCoords.x;
	lightCoords[0].y = pDisp->GetVertex( 0 )->m_LMCoords.y;
	lightCoords[1].x = pDisp->GetVertex( nInterval - 1 )->m_LMCoords.x;
	lightCoords[1].y = pDisp->GetVertex( nInterval - 1 )->m_LMCoords.y;
	lightCoords[2].x = pDisp->GetVertex( ( nInterval * nInterval ) - 1 )->m_LMCoords.x;
	lightCoords[2].y = pDisp->GetVertex( ( nInterval * nInterval ) - 1 )->m_LMCoords.y;
	lightCoords[3].x = pDisp->GetVertex( nInterval * ( nInterval - 1 ) )->m_LMCoords.x;
	lightCoords[3].y = pDisp->GetVertex( nInterval * ( nInterval - 1 ) )->m_LMCoords.y;

	// Get the number of displacement fragments.
	int nFragCount = aDispFragments.Count();
	for ( int iFrag = 0; iFrag < nFragCount; ++iFrag )
	{
		moverlayfragment_t *pDispFragment = aDispFragments[iFrag];
		if ( !pDispFragment )
			continue;

		int nVertCount = pDispFragment->m_aPrimVerts.Count();
		if ( nVertCount == 0 )
			continue;

		// Create fragment.
		OverlayFragmentHandle_t hFragment = AddFragmentToFragmentList( nVertCount );
		moverlayfragment_t *pFragment = GetOverlayFragment( hFragment );

		pFragment->m_iOverlay = pOverlay->m_nId;
		pFragment->m_iSurf = iSurface;

		Vector2D vecTmpUV;
		Vector	 vecTmp;
		for ( int iVert = 0; iVert < nVertCount; ++iVert )
		{
			vecTmpUV.x = pDispFragment->m_aPrimVerts[iVert].pos.x;
			vecTmpUV.y = pDispFragment->m_aPrimVerts[iVert].pos.y;

			Overlay_DispUVToWorld( pDisp, vecTmpUV, pFragment->m_aPrimVerts[iVert].pos );

			// Texture coordinates.
			memcpy( pFragment->m_aPrimVerts[iVert].texCoord, pDispFragment->m_aPrimVerts[iVert].texCoord, sizeof( pFragment->m_aPrimVerts[iVert].texCoord ) );

			// Lightmap coordinates.
			vecTmpUV.x = clamp( vecTmpUV.x, 0.0f, 1.0f );
			vecTmpUV.y = clamp( vecTmpUV.y, 0.0f, 1.0f );

			Vector2D uv;
			TexCoordInQuadFromBarycentric( lightCoords[0], lightCoords[1], lightCoords[2], lightCoords[3], 
				                           vecTmpUV, uv ); 
			pFragment->m_aPrimVerts[iVert].lightCoord[0] = uv.x;
			pFragment->m_aPrimVerts[iVert].lightCoord[1] = uv.y;
		}

		// Create the sort ID for this fragment
		const MaterialSystem_SortInfo_t &sortInfo = materialSortInfoArray[MSurf_MaterialSortID( iSurface )];
		mtexinfo_t *pTexInfo = &host_state.worldmodel->brush.texinfo[pOverlay->m_nTexInfo];
		pFragment->m_nMaterialSortID = GetMaterialSortID( pTexInfo->material, sortInfo.lightmapPageID );

		// Add to list of fragments for this overlay
		OverlayFragmentList_t i = m_OverlayFragments.Alloc( true );
		m_OverlayFragments[i] = hFragment;
		m_OverlayFragments.LinkBefore( pOverlay->m_hFirstFragment, i );
		pOverlay->m_hFirstFragment = i;

		// Add to list of fragments for this surface
		// NOTE: Store them in *reverse* order so that when we pull them off for
		// rendering, we can do *that* in reverse order too? Reduces the amount of iteration necessary
		// Therefore, we need to add to the head of the list
		m_aFragments.LinkBefore( MSurf_OverlayFragmentList( iSurface ), hFragment );
		MSurf_OverlayFragmentList( iSurface ) = hFragment;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void COverlayMgr::Disp_ClipFragment( CDispInfo *pDisp, OverlayFragmentVector_t &aDispFragments )
{
	cplane_t	clipPlane;

	// Cache the displacement interval.
	const CPowerInfo *pPowerInfo = pDisp->GetPowerInfo();
	int nInterval = ( 1 << pPowerInfo->GetPower() );

	// Displacement-space clipping in V.
	clipPlane.normal.Init( 1.0f, 0.0f, 0.0f );
	Disp_DoClip( pDisp, aDispFragments, clipPlane, 1.0f, nInterval, 1, nInterval, 1 );

	// Displacement-space clipping in U.
	clipPlane.normal.Init( 0.0f, 1.0f, 0.0f );
	Disp_DoClip( pDisp, aDispFragments, clipPlane, 1.0f, nInterval, 1, nInterval, 1 );

	// Displacement-space clipping UV from top-left to bottom-right.
	clipPlane.normal.Init( 0.707f, 0.707f, 0.0f );  // 45 degrees
	Disp_DoClip( pDisp, aDispFragments, clipPlane, 0.707f, nInterval, 2, ( nInterval * 2 - 1 ), 2 );

	// Displacement-space clipping UV from bottom-left to top-right.
	clipPlane.normal.Init( -0.707f, 0.707f, 0.0f );  // 135 degrees
	Disp_DoClip( pDisp, aDispFragments, clipPlane, 0.707f, nInterval, -( nInterval - 2 ), ( nInterval - 1 ), 2 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void COverlayMgr::Disp_DoClip( CDispInfo *pDisp, OverlayFragmentVector_t &aDispFragments, cplane_t &clipPlane, 
							   float clipDistStart, int nInterval, 
							   int nLoopStart, int nLoopEnd, int nLoopInc )
{
	// Setup interval information.
	float flInterval = static_cast<float>( nInterval );
	float flOOInterval = 1.0f / flInterval;

	// Holds the current set of clipped faces.
	OverlayFragmentVector_t aClippedFragments;

	for ( int iInterval = nLoopStart; iInterval < nLoopEnd; iInterval += nLoopInc )
	{
		// Copy the current list to clipped face list.
		aClippedFragments.CopyArray( aDispFragments.Base(), aDispFragments.Count() );
		aDispFragments.Purge();

		// Clip in V.
		int nFragCount = aClippedFragments.Count();
		for ( int iFrag = 0; iFrag < nFragCount; iFrag++ )
		{
			moverlayfragment_t *pClipFrag = aClippedFragments[iFrag];
			if ( pClipFrag )
			{
				moverlayfragment_t *pFront = NULL, *pBack = NULL;

				clipPlane.dist = clipDistStart * ( ( float )iInterval * flOOInterval );
				DoClipFragment( pClipFrag, &clipPlane, &pFront, &pBack );
				DestroyTempFragment( pClipFrag );
				pClipFrag = NULL;

				if ( pFront )
				{
					aDispFragments.AddToTail( pFront );
				}

				if ( pBack )
				{
					aDispFragments.AddToTail( pBack );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void COverlayMgr::InitTexCoords( moverlay_t *pOverlay, moverlayfragment_t &overlayFrag )
{
	// Overlay texture coordinates.
	overlayFrag.m_aPrimVerts[0].texCoord[0].Init( pOverlay->m_flU[0], pOverlay->m_flV[0] );
	overlayFrag.m_aPrimVerts[1].texCoord[0].Init( pOverlay->m_flU[0], pOverlay->m_flV[1] );
	overlayFrag.m_aPrimVerts[2].texCoord[0].Init( pOverlay->m_flU[1], pOverlay->m_flV[1] );
	overlayFrag.m_aPrimVerts[3].texCoord[0].Init( pOverlay->m_flU[1], pOverlay->m_flV[0] );

	overlayFrag.m_aPrimVerts[0].texCoord[1].Init( 0, 0 );
	overlayFrag.m_aPrimVerts[1].texCoord[1].Init( 0, 1 );
	overlayFrag.m_aPrimVerts[2].texCoord[1].Init( 1, 1 );
	overlayFrag.m_aPrimVerts[3].texCoord[1].Init( 1, 0 );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void COverlayMgr::DoClipFragment( moverlayfragment_t *pFragment, cplane_t *pClipPlane,
								  moverlayfragment_t **ppFront, moverlayfragment_t **ppBack )
{
	const float OVERLAY_EPSILON	= 0.0001f;

	// Verify.
	if ( !pFragment )
		return;

	float	flDists[128];
	int		nSides[128];
	int		nSideCounts[3];

	//
	// Determine "sidedness" of all the polygon points.
	//
	nSideCounts[0] = nSideCounts[1] = nSideCounts[2] = 0;
	for ( int iVert = 0; iVert < pFragment->m_aPrimVerts.Count(); ++iVert )
	{
		flDists[iVert] = pClipPlane->normal.Dot( pFragment->m_aPrimVerts[iVert].pos ) - pClipPlane->dist;

		if ( flDists[iVert] > OVERLAY_EPSILON )
		{
			nSides[iVert] = SIDE_FRONT;
		}
		else if ( flDists[iVert] < -OVERLAY_EPSILON )
		{
			nSides[iVert] = SIDE_BACK;
		}
		else
		{
			nSides[iVert] = SIDE_ON;
		}

		nSideCounts[nSides[iVert]]++;
	}

	// Wrap around (close the polygon).
	nSides[iVert] = nSides[0];
	flDists[iVert] =  flDists[0];

	// All points in back - no split (copy face to back).
	if( !nSideCounts[SIDE_FRONT] )
	{
		*ppBack = CopyTempFragment( pFragment );
		return;
	}

	// All points in front - no split (copy face to front).
	if( !nSideCounts[SIDE_BACK] )
	{
		*ppFront = CopyTempFragment( pFragment );
		return;
	}

	// Build new front and back faces.
	// NOTE: Gotta create them first
	moverlayfragment_t *pFront = CreateTempFragment( 0 );
	moverlayfragment_t *pBack = CreateTempFragment( 0 );
	if ( !pFront || !pBack )
	{
		DestroyTempFragment( pFront );
		DestroyTempFragment( pBack );
		return;
	}

	int nVertCount = pFragment->m_aPrimVerts.Count();
	for ( iVert = 0; iVert < nVertCount; ++iVert )
	{
		// "On" clip plane.
		if ( nSides[iVert] == SIDE_ON )
		{
			pFront->m_aPrimVerts.AddToTail( pFragment->m_aPrimVerts[iVert] );
			pBack->m_aPrimVerts.AddToTail( pFragment->m_aPrimVerts[iVert] );
			continue;
		}

		// "In back" of clip plane.
		if ( nSides[iVert] == SIDE_BACK )
		{
			pBack->m_aPrimVerts.AddToTail( pFragment->m_aPrimVerts[iVert] );
		}

		// "In front" of clip plane.
		if ( nSides[iVert] == SIDE_FRONT )
		{
			pFront->m_aPrimVerts.AddToTail( pFragment->m_aPrimVerts[iVert] );
		}

		if ( nSides[iVert+1] == SIDE_ON || nSides[iVert+1] == nSides[iVert] )
			continue;

		// Split!
		float fraction = flDists[iVert] / ( flDists[iVert] - flDists[iVert+1] );

		overlayvert_t vert;
		vert.pos = pFragment->m_aPrimVerts[iVert].pos + fraction * ( pFragment->m_aPrimVerts[(iVert+1)%nVertCount].pos - 
			                                                         pFragment->m_aPrimVerts[iVert].pos );
		for ( int iTexCoord=0; iTexCoord < NUM_OVERLAY_TEXCOORDS; iTexCoord++ )
		{
			vert.texCoord[iTexCoord][0] = pFragment->m_aPrimVerts[iVert].texCoord[iTexCoord][0] + fraction * ( pFragment->m_aPrimVerts[(iVert+1)%nVertCount].texCoord[iTexCoord][0] - 
				                                                                         pFragment->m_aPrimVerts[iVert].texCoord[iTexCoord][0] );
			vert.texCoord[iTexCoord][1] = pFragment->m_aPrimVerts[iVert].texCoord[iTexCoord][1] + fraction * ( pFragment->m_aPrimVerts[(iVert+1)%nVertCount].texCoord[iTexCoord][1] - 
																						pFragment->m_aPrimVerts[iVert].texCoord[iTexCoord][1] );
		}

		pFront->m_aPrimVerts.AddToTail( vert );
		pBack->m_aPrimVerts.AddToTail( vert );
	}

	*ppFront = pFront;
	*ppBack = pBack;
}


//-----------------------------------------------------------------------------
// Copies a fragment into the main fragment list
//-----------------------------------------------------------------------------
OverlayFragmentHandle_t COverlayMgr::AddFragmentToFragmentList( int nSize )
{
	// Add to list of fragments.
	int iFragment = m_aFragments.Alloc( true );

	moverlayfragment_t &frag = m_aFragments[iFragment];

	frag.m_iSurf = -1;
	frag.m_iOverlay = -1;
	frag.m_nRenderFrameID = -1;
	frag.m_nMaterialSortID = 0xFFFF;
	frag.m_hNextRender = OVERLAY_FRAGMENT_INVALID;

	if ( nSize > 0 )
	{
		frag.m_aPrimVerts.SetSize( nSize );
	}

	return iFragment;
}


//-----------------------------------------------------------------------------
// Copies a fragment into the main fragment list
//-----------------------------------------------------------------------------
OverlayFragmentHandle_t COverlayMgr::AddFragmentToFragmentList( moverlayfragment_t *pSrc )
{
	// Add to list of fragments.
	int iFragment = m_aFragments.Alloc( true );

	moverlayfragment_t &frag = m_aFragments[iFragment];

	frag.m_iSurf = pSrc->m_iSurf;
	frag.m_iOverlay = pSrc->m_iOverlay;
	frag.m_aPrimVerts.CopyArray( pSrc->m_aPrimVerts.Base(), pSrc->m_aPrimVerts.Count() );

	return iFragment;
}


//-----------------------------------------------------------------------------
// Temp fragments for clipping algorithms
//-----------------------------------------------------------------------------
moverlayfragment_t *COverlayMgr::CreateTempFragment( int nSize )
{
	moverlayfragment_t *pDst =  new moverlayfragment_t;
	if ( pDst )
	{
		pDst->m_iSurf = -1;
		pDst->m_iOverlay = -1;
		if ( nSize > 0 )
		{
			pDst->m_aPrimVerts.SetSize( nSize );
		}
	}

	return pDst;
}


//-----------------------------------------------------------------------------
// Temp fragments for clipping algorithms
//-----------------------------------------------------------------------------
moverlayfragment_t *COverlayMgr::CopyTempFragment( moverlayfragment_t *pSrc )
{
	moverlayfragment_t *pDst =  new moverlayfragment_t;
	if ( pDst )
	{
		pDst->m_iSurf = pSrc->m_iSurf;
		pDst->m_iOverlay = pSrc->m_iOverlay;
		pDst->m_aPrimVerts.CopyArray( pSrc->m_aPrimVerts.Base(), pSrc->m_aPrimVerts.Count() );
	}

	return pDst;
}

//-----------------------------------------------------------------------------
// Temp fragments for clipping algorithms
//-----------------------------------------------------------------------------
void COverlayMgr::DestroyTempFragment( moverlayfragment_t *pFragment )
{
	if ( pFragment )
	{
		delete pFragment;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void COverlayMgr::BuildClipPlanes( int iSurface, moverlayfragment_t &surfaceFrag, 
								   const Vector &vecBasisNormal, 
								   CUtlVector<cplane_t> &m_ClipPlanes )
{
	int nVertCount = surfaceFrag.m_aPrimVerts.Count();
	for ( int iVert = 0; iVert < nVertCount; ++iVert )
	{
		Vector vecEdge;
		vecEdge = surfaceFrag.m_aPrimVerts[(iVert+1)%nVertCount].pos - surfaceFrag.m_aPrimVerts[iVert].pos;
		VectorNormalize( vecEdge );

		int iPlane = m_ClipPlanes.AddToTail();
		cplane_t *pPlane = &m_ClipPlanes[iPlane];

		pPlane->normal = vecBasisNormal.Cross( vecEdge );
		pPlane->dist = pPlane->normal.Dot( surfaceFrag.m_aPrimVerts[iVert].pos );

		// Check normal facing.
		float flDistance = pPlane->normal.Dot( surfaceFrag.m_aPrimVerts[(iVert+2)%nVertCount].pos ) - pPlane->dist;
		if ( flDistance > 0.0 )
		{
			// Flip
			pPlane->normal.Negate();
			pPlane->dist = -pPlane->dist;
		}
	}
}



//=============================================================================
//
// Code below this line will get moved out into common code!!!!!!!!!!!!!
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Overlay_BuildBasisOrigin( Vector &vecBasisOrigin, int iSurface )
{
	cplane_t surfacePlane = MSurf_Plane( iSurface );
	VectorNormalize( surfacePlane.normal );

	// Get the distance from entity origin to face plane.
	float flDist = surfacePlane.normal.Dot( vecBasisOrigin ) - surfacePlane.dist;
	
	// Move the basis origin to the position of the entity projected into the face plane.
	vecBasisOrigin = vecBasisOrigin - ( flDist * surfacePlane.normal );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Overlay_BuildBasis( const Vector &vecBasisNormal, Vector &vecBasisU, Vector &vecBasisV )
{
	// Verify incoming data.
	Assert( vecBasisNormal.IsValid() );
	if ( !vecBasisNormal.IsValid() )	
		return;

	// Find the major vector component.
	int nMajorAxis = 0;
	float flAxisValue = vecBasisNormal[0];
	if ( fabs( vecBasisNormal[1] ) > fabs( flAxisValue ) ) { nMajorAxis = 1; flAxisValue = vecBasisNormal[1]; }
	if ( fabs( vecBasisNormal[2] ) > fabs( flAxisValue ) ) { nMajorAxis = 2; }
	if ( ( nMajorAxis == 1 ) || ( nMajorAxis == 2 ) )
	{
		vecBasisU.Init( 1.0f, 0.0f, 0.0f );
	}
	else
	{
		vecBasisU.Init( 0.0f, 1.0f, 0.0f );
	}

	vecBasisV = vecBasisNormal.Cross( vecBasisU );
	VectorNormalize( vecBasisV );

	vecBasisU = vecBasisV.Cross( vecBasisNormal );
	VectorNormalize( vecBasisU );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Overlay_TriTLToBR( CDispInfo *pDisp, Vector &vecWorld, float flU, float flV,
						int nSnapU, int nSnapV, int nWidth, int nHeight )
{
	const float TRIEDGE_EPSILON = 0.001f;

	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	if ( nNextU == nWidth)	 { --nNextU; }
	if ( nNextV == nHeight ) { --nNextV; }

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	if( ( flFracU + flFracV ) >= ( 1.0f + TRIEDGE_EPSILON ) )
	{
		int triIndices[3];
		triIndices[0] = nNextV * nWidth + nSnapU;
		triIndices[1] = nNextV * nWidth + nNextU;
		triIndices[2] = nSnapV * nWidth + nNextU;

		Vector triVerts[3];
		for( int i = 0; i < 3; i++ )
		{
			CDispRenderVert *pVert = pDisp->GetVertex( triIndices[i] );
			triVerts[i] = pVert->m_vPos;
		}

		Vector edgeU, edgeV;
		edgeU = triVerts[1] - triVerts[0];
		edgeV = triVerts[1] - triVerts[2];

		vecWorld = triVerts[1] + edgeU * ( flFracU - 1.0f ) + edgeV * ( flFracV - 1.0f );
	}
	else
	{
		int triIndices[3];
		triIndices[0] = nSnapV * nWidth + nSnapU;
		triIndices[1] = nNextV * nWidth + nSnapU;
		triIndices[2] = nSnapV * nWidth + nNextU;

		Vector triVerts[3];
		for( int i = 0; i < 3; i++ )
		{
			CDispRenderVert *pVert = pDisp->GetVertex( triIndices[i] );
			triVerts[i] = pVert->m_vPos;
		}

		Vector edgeU, edgeV;
		edgeU = triVerts[2] - triVerts[0];
		edgeV = triVerts[1] - triVerts[0];

		// get the position
		vecWorld = triVerts[0] + edgeU * flFracU + edgeV * flFracV;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Overlay_TriBLToTR( CDispInfo *pDisp, Vector &vecWorld, float flU, float flV,
			            int nSnapU, int nSnapV, int nWidth, int nHeight )
{
	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	if ( nNextU == nWidth)	 { --nNextU; }
	if ( nNextV == nHeight ) { --nNextV; }

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	if( flFracU < flFracV )
	{
		int triIndices[3];
		triIndices[0] = nSnapV * nWidth + nSnapU;
		triIndices[1] = nNextV * nWidth + nSnapU;
		triIndices[2] = nNextV * nWidth + nNextU;

		Vector triVerts[3];
		for( int i = 0; i < 3; i++ )
		{
			CDispRenderVert *pVert = pDisp->GetVertex( triIndices[i] );
			triVerts[i] = pVert->m_vPos;
		}

		Vector edgeU, edgeV;
		edgeU = triVerts[2] - triVerts[1];
		edgeV = triVerts[0] - triVerts[1];

		vecWorld = triVerts[1] + edgeU * flFracU + edgeV * ( 1.0f - flFracV );
	}
	else
	{
		int triIndices[3];
		triIndices[0] = nSnapV * nWidth + nSnapU;
		triIndices[1] = nNextV * nWidth + nNextU;
		triIndices[2] = nSnapV * nWidth + nNextU;

		Vector triVerts[3];
		for( int i = 0; i < 3; i++ )
		{
			CDispRenderVert *pVert = pDisp->GetVertex( triIndices[i] );
			triVerts[i] = pVert->m_vPos;
		}

		Vector edgeU, edgeV;
		edgeU = triVerts[0] - triVerts[2];
		edgeV = triVerts[1] - triVerts[2];

		// get the position
		vecWorld = triVerts[2] + edgeU * ( 1.0f - flFracU ) + edgeV * flFracV;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Overlay_DispUVToWorld( CDispInfo *pDisp, const Vector2D &vecUV, Vector &vecWorld )
{
	// Get the displacement power.
	const CPowerInfo *pPowerInfo = pDisp->GetPowerInfo();
	int nWidth = ( ( 1 << pPowerInfo->GetPower() ) + 1 );
	int nHeight = nWidth;

	// Scale the U, V coordinates to the displacement grid size.
	float flU = vecUV.x * static_cast<float>( nWidth - 1.000001f );
	float flV = vecUV.y * static_cast<float>( nHeight - 1.000001f );

	// Find the base U, V.
	int nSnapU = static_cast<int>( flU );
	int nSnapV = static_cast<int>( flV );

	// Use this to get the triangle orientation.
	bool bOdd = ( ( ( nSnapV * nWidth ) + nSnapU ) % 2 == 1 );

	// Top Left to Bottom Right
	if( bOdd )
	{
		Overlay_TriTLToBR( pDisp, vecWorld, flU, flV, nSnapU, nSnapV, nWidth, nHeight );
	}
	// Bottom Left to Top Right
	else
	{
		Overlay_TriBLToTR( pDisp, vecWorld, flU, flV, nSnapU, nSnapV, nWidth, nHeight );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_OverlayUVToOverlayPlane( const Vector &vecBasisOrigin, const Vector &vecBasisU,
									  const Vector &vecBasisV, const Vector &vecUVPoint,
									  Vector &vecPlanePoint )
{
	vecPlanePoint = ( vecUVPoint.x * vecBasisU ) + ( vecUVPoint.y * vecBasisV );
	vecPlanePoint += vecBasisOrigin;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_WorldToOverlayPlane( const Vector &vecBasisOrigin, const Vector &vecBasisNormal,
								  const Vector &vecWorldPoint, Vector &vecPlanePoint )
{
	Vector vecDelta = vecWorldPoint - vecBasisOrigin;
	float flDistance = vecBasisNormal.Dot( vecDelta );
	vecPlanePoint = vecWorldPoint - ( flDistance * vecBasisNormal );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_OverlayPlaneToWorld( const Vector &vecBasisNormal, int iSurface,
								  const Vector &vecPlanePoint, Vector &vecWorldPoint )
{
	cplane_t surfacePlane = MSurf_Plane( iSurface );
	VectorNormalize( surfacePlane.normal );
	float flDistanceToSurface = surfacePlane.normal.Dot( vecPlanePoint ) - surfacePlane.dist;

	float flDistance = ( 1.0f / surfacePlane.normal.Dot( vecBasisNormal ) ) * flDistanceToSurface;

	vecWorldPoint = vecPlanePoint - ( vecBasisNormal * flDistance );
}
