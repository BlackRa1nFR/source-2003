//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include <math.h>
#include <mmsystem.h>
#include "Camera.h"
#include "CullTreeNode.h"
#include "MapDefs.h"
#include "MapDoc.h"
#include "MapEntity.h"
#include "MapWorld.h"
#include "Render3DMS.h"
#include "SSolid.h"
#include "MapStudioModel.h"
#include "Material.h"
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMesh.h"
#include "TextureSystem.h"
#include "ToolInterface.h"
#include "StudioModel.h"
#include "ibsplighting.h"
#include "MapDisp.h"
#include "ToolManager.h"


#define NUM_MIPLEVELS			4

#define CROSSHAIR_DIST_HORIZONTAL		5
#define CROSSHAIR_DIST_VERTICAL			6

#define TEXTURE_AXIS_LENGTH				10	// Texture axis length in world units


// dvs: experiment!
//extern int g_nClipPoints;
//extern Vector g_ClipPoints[4];

//
// Debugging / diagnostic stuff.
//
static bool g_bDrawWireFrameSelection = true;
static bool g_bShowStatistics = false;
static bool g_bUseCullTree = true;
static bool g_bRenderCullBoxes = false;


//
// All renderers share a frame counter to guarantee non-collision of frame numbers.
//
int CRender3DMS::g_nRenderFrame = 0;

//
// Unique texture ID for each texture uploaded to any renderer. Identifies that
// texture across all renderers.
//
int CRender3DMS::g_nTextureID = 0;


//-----------------------------------------------------------------------------
// Purpose: Determines whether a point is inside of a bounding box.
// Input  : pfPoint - The point.
//			pfMins - The box mins.
//			pfMaxs - The box maxes.
// Output : Returns true if the point is inside the box, false if not.
//-----------------------------------------------------------------------------
static bool PointInBox(float *pfPoint, float *pfMins, float *pfMaxs)
{
	if ((pfPoint[0] < pfMins[0]) || (pfPoint[0] > pfMaxs[0]))
	{
		return(false);
	}

	if ((pfPoint[1] < pfMins[1]) || (pfPoint[1] > pfMaxs[1]))
	{
		return(false);
	}

	if ((pfPoint[2] < pfMins[2]) || (pfPoint[2] > pfMaxs[2]))
	{
		return(false);
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Callback comparison function for sorting objects clicked on while
//			in selection mode.
// Input  : pHit1 - First hit to compare.
//			pHit2 - Second hit to compare.
// Output : Sorts by increasing depth value. Returns -1, 0, or 1 per qsort spec.
//-----------------------------------------------------------------------------
static int _CompareHits(const void *pHit1, const void *pHit2)
{
	if (((RenderHitInfo_t *)pHit1)->nDepth < ((RenderHitInfo_t *)pHit2)->nDepth)
	{
		return(-1);
	}

	if (((RenderHitInfo_t *)pHit1)->nDepth > ((RenderHitInfo_t *)pHit2)->nDepth)
	{
		return(1);
	}

	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: Callback comparison function for sorting objects clicked on while
//			in selection mode. The reverse sort is used for cards that return
//			depth values in reverse (larger numbers are closer to the camera).
// Input  : pHit1 - First hit to compare.
//			pHit2 - Second hit to compare.
// Output : Sorts by decreasing depth value. Returns -1, 0, or 1 per qsort spec.
//-----------------------------------------------------------------------------
static int _CompareHitsReverse(const void *pHit1, const void *pHit2)
{
	if (((RenderHitInfo_t *)pHit1)->nDepth > ((RenderHitInfo_t *)pHit2)->nDepth)
	{
		return(-1);
	}

	if (((RenderHitInfo_t *)pHit1)->nDepth < ((RenderHitInfo_t *)pHit2)->nDepth)
	{
		return(1);
	}

	return(0);
}


//-----------------------------------------------------------------------------
// Purpose: Calculates lighting for a given face.
// Input  : Normal - vector that is normal to the face being lit.
// Output : Returns a number from [0.2, 1.0]
//-----------------------------------------------------------------------------
float CRender3DMS::LightPlane(Vector& Normal)
{
	static Vector Light( 1.0f, 2.0f, 3.0f );
	static bool bFirst = true;

	if (bFirst)
	{
		VectorNormalize(Light);
		bFirst = false;
	}

	float fShade = 0.8f + (0.5f * DotProduct(Normal, Light));
	if (fShade > 1.0f)
	{
		fShade = 1.0f;
	}

	return(fShade);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRender3DMS::CRender3DMS(void)
{
	memset(&m_WinData, 0, sizeof(m_WinData));
	m_WinData.bAllowSoft = true;

	m_pCamera = NULL;
	memset(m_FrustumPlanes, 0, sizeof(m_FrustumPlanes));

	m_pDoc = NULL;

	m_pDropCamera = new CCamera;
	m_bDroppedCamera = false;
	m_DeferRendering = false;

	m_fFrameRate = 0;
	m_nFramesThisSample = 0;
	m_dwTimeLastSample = 0;
	m_dwTimeLastFrame = 0;
	m_fTimeElapsed = 0;

	memset(&m_Pick, 0, sizeof(m_Pick));
	m_Pick.bPicking = false;

	m_eDefaultRenderMode = RENDER_MODE_WIREFRAME;
	m_eCurrentRenderMode = RENDER_MODE_NONE;

	m_ProjMode = PROJECTION_PERSPECTIVE;

	memset(&m_RenderState, 0, sizeof(m_RenderState));

	m_pBoundMaterial = 0;

	for (int i = 0; i < 2; ++i)
	{
		m_pFlat[i] = 0;
		m_pWireframe[i] = 0;
		m_pTranslucentFlat[i] = 0;
		m_pLightmapGrid[i] = 0;
		m_pSelectionOverlay[i] = 0;
		m_pVertexColor[i] = 0;
	}
	m_DecalMode = false;
	m_bLightingPreview = false;
#ifdef _DEBUG
	m_bRenderFrustum = false;
	m_bRecomputeFrustumRenderGeometry = false;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRender3DMS::~CRender3DMS(void)
{
	if (m_pDropCamera != NULL)
	{
		delete m_pDropCamera;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Prepares for rendering the parallel-projected components of the 3D view.
//-----------------------------------------------------------------------------

void CRender3DMS::BeginParallel(void)
{
	MaterialSystemInterface()->MatrixMode(MATERIAL_PROJECTION);
	MaterialSystemInterface()->PushMatrix();
    MaterialSystemInterface()->LoadIdentity();

	MaterialSystemInterface()->Scale( 1, -1, 1 );
	MaterialSystemInterface()->Ortho(0, 0, m_WinData.nWidth, m_WinData.nHeight, -COORD_NOTINIT, COORD_NOTINIT);

	MaterialSystemInterface()->MatrixMode(MATERIAL_VIEW);
	MaterialSystemInterface()->PushMatrix();
    MaterialSystemInterface()->LoadIdentity();

	MaterialSystemInterface()->CullMode( MATERIAL_CULLMODE_CW );
}


//-----------------------------------------------------------------------------
// Purpose: Called before rendering an object that should be hit tested when
//			rendering in selection mode.
// Input  : pObject - Map atom pointer that will be returned from the ObjectsAt
//			routine if this rendered object is positively hit tested.
//-----------------------------------------------------------------------------
void CRender3DMS::BeginRenderHitTarget(CMapAtom *pObject, unsigned int uHandle)
{
	if (m_Pick.bPicking)
	{
		MaterialSystemInterface()->PushSelectionName((unsigned int)pObject);
		MaterialSystemInterface()->PushSelectionName(uHandle);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Binds a texture for rendering. If the texture has never been bound
//			to this rendering context, it is uploaded to the driver.
// Input  : pTexture - Pointer to the texture object being bound.
//-----------------------------------------------------------------------------

void CRender3DMS::BindTexture(IEditorTexture *pTexture)
{ 
	// These textures must be CMaterials....
	if (m_pBoundMaterial != pTexture->GetMaterial())
	{
		m_pBoundMaterial = pTexture->GetMaterial();
		SetupRenderMode( m_eCurrentRenderMode, true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called after rendering an object that should be hit tested when
//			rendering in selection mode.
// Input  : pObject - Map atom pointer that will be returned from the ObjectsAt
//			routine if this rendered object is positively hit tested.
// Input  : pObject - 
//-----------------------------------------------------------------------------
void CRender3DMS::EndRenderHitTarget(void)
{
	if (m_Pick.bPicking)
	{
		//
		// Pop the name and the handle from the stack.
		//

		MaterialSystemInterface()->PopSelectionName();
		MaterialSystemInterface()->PopSelectionName();

		if ((MaterialSystemInterface()->SelectionMode(true) != 0) && (m_Pick.nNumHits < MAX_PICK_HITS))
		{
			if (m_Pick.uSelectionBuffer[0] == 2)
			{
				m_Pick.Hits[m_Pick.nNumHits].pObject = (CMapAtom *)m_Pick.uSelectionBuffer[3];
				m_Pick.Hits[m_Pick.nNumHits].uData = m_Pick.uSelectionBuffer[4];
				m_Pick.Hits[m_Pick.nNumHits].nDepth = m_Pick.uSelectionBuffer[1];
				m_Pick.nNumHits++;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : void - 
//-----------------------------------------------------------------------------
void CRender3DMS::EndParallel(void)
{
	MaterialSystemInterface()->CullMode( MATERIAL_CULLMODE_CCW );

	MaterialSystemInterface()->MatrixMode(MATERIAL_VIEW);
	MaterialSystemInterface()->PopMatrix();

	MaterialSystemInterface()->MatrixMode(MATERIAL_PROJECTION);
	MaterialSystemInterface()->PopMatrix();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
float CRender3DMS::GetElapsedTime(void)
{
	return(m_fTimeElapsed);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current render mode.
//-----------------------------------------------------------------------------
RenderMode_t CRender3DMS::GetDefaultRenderMode(void)
{
	return(m_eDefaultRenderMode);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current render mode.
//-----------------------------------------------------------------------------
RenderMode_t CRender3DMS::GetCurrentRenderMode(void)
{
	return(m_eCurrentRenderMode);
}


//-----------------------------------------------------------------------------
// Computes us some geometry to render the frustum planes
//-----------------------------------------------------------------------------

void CRender3DMS::ComputeFrustumRenderGeometry()
{
#ifdef _DEBUG
	Vector viewPoint;
	m_pCamera->GetViewPoint(viewPoint);

	// Find lines along each of the plane intersections.
	// We know these lines are perpendicular to both plane normals,
	// so we can take the cross product to find them.
	static int edgeIdx[4][2] =
	{
		{ 0, 2 }, { 0, 3 }, { 1, 3 }, { 1, 2 }
	};

	int i;
	Vector edges[4];
	for ( i = 0; i < 4; ++i)
	{
		CrossProduct( m_FrustumPlanes[edgeIdx[i][0]].AsVector3D(),
			m_FrustumPlanes[edgeIdx[i][1]].AsVector3D(), edges[i] );
		VectorNormalize( edges[i] );
	}

	// Figure out four near points by intersection lines with the near plane
	// Figure out four far points by intersection with lines against far plane
	for (i = 0; i < 4; ++i)
	{
		float t = (m_FrustumPlanes[4][3] - DotProduct(m_FrustumPlanes[4].AsVector3D(), viewPoint)) /
			DotProduct(m_FrustumPlanes[4].AsVector3D(), edges[i]);
		VectorMA( viewPoint, t, edges[i], m_FrustumRenderPoint[i] );

		/*
		t = (m_FrustumPlanes[5][3] - DotProduct(m_FrustumPlanes[5], viewPoint)) /
			DotProduct(m_FrustumPlanes[5], edges[i]);
		VectorMA( viewPoint, t, edges[i], m_FrustumRenderPoint[i + 4] );
		*/
		if (t < 0)
		{
			edges[i] *= -1;
		}

		VectorMA( m_FrustumRenderPoint[i], 200.0, edges[i], m_FrustumRenderPoint[i + 4] );
	}
#endif
}

//-----------------------------------------------------------------------------
// renders the frustum
//-----------------------------------------------------------------------------

void CRender3DMS::RenderFrustum( )
{
#ifdef _DEBUG
	static int indices[] = 
	{
		0, 1, 1, 2, 2, 3, 3, 0,	// near square
		4, 5, 5, 6, 6, 7, 7, 4,	// far square
		0, 4, 1, 5, 2, 6, 3, 7	// connections between them
	};

	SetRenderMode( RENDER_MODE_WIREFRAME ); 
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();

	int numIndices = sizeof(indices) / sizeof(int);
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 8, numIndices );

	int i;
	for ( i = 0; i < 8; ++i )
	{
		meshBuilder.Position3fv( m_FrustumRenderPoint[i].Base() );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();
	}

	for ( i = 0; i < numIndices; ++i )
	{
		meshBuilder.Index( indices[i] );
		meshBuilder.AdvanceIndex();
	}

	meshBuilder.End();
	pMesh->Draw();

	SetRenderMode( RENDER_MODE_DEFAULT ); 
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Planes[6] - 
//-----------------------------------------------------------------------------

void CRender3DMS::GetFrustumPlanes(Vector4D Planes[6])
{
	//
	// Get the modelview and projection matrices from OpenGL.
	//
	matrix4_t ViewMatrix;
	matrix4_t ProjectionMatrix;
	matrix4_t CameraMatrix;
	Vector ViewPoint;

	m_pCamera->GetViewPoint(ViewPoint);

	MaterialSystemInterface()->GetMatrix(MATERIAL_VIEW, &ViewMatrix[0][0]);
	MaterialSystemInterface()->GetMatrix(MATERIAL_PROJECTION, &ProjectionMatrix[0][0]);

	TransposeMatrix( ViewMatrix );
    TransposeMatrix( ProjectionMatrix );

	MultiplyMatrix( ProjectionMatrix, ViewMatrix, CameraMatrix);

	//
	// Now the plane coefficients can be pulled directly out of the the camera
	// matrix as follows:
	//
	// Right :  first_column  - fourth_column
	// Left  : -first_column  - fourth_column
	// Top   :  second_column - fourth_column
	// Bottom: -second_column - fourth_column
	// Front : -third_column  - fourth_column
	// Back  :  third_column  + fourth_column
	//
	// dvs: My plane constants should be coming directly from the matrices,
	//		but they aren't (for some reason). Instead I calculate the plane
	//		constants myself. Sigh.
	//
	Planes[0][0] = CameraMatrix[0][0] - CameraMatrix[3][0];
	Planes[0][1] = CameraMatrix[0][1] - CameraMatrix[3][1];
	Planes[0][2] = CameraMatrix[0][2] - CameraMatrix[3][2];
	VectorNormalize(Planes[0].AsVector3D());
	Planes[0][3] = DotProduct(ViewPoint, Planes[0].AsVector3D());

	Planes[1][0] = -CameraMatrix[0][0] - CameraMatrix[3][0];
	Planes[1][1] = -CameraMatrix[0][1] - CameraMatrix[3][1];
	Planes[1][2] = -CameraMatrix[0][2] - CameraMatrix[3][2];
	VectorNormalize(Planes[1].AsVector3D());
	Planes[1][3] = DotProduct(ViewPoint, Planes[1].AsVector3D());

	Planes[2][0] = CameraMatrix[1][0] - CameraMatrix[3][0];
	Planes[2][1] = CameraMatrix[1][1] - CameraMatrix[3][1];
	Planes[2][2] = CameraMatrix[1][2] - CameraMatrix[3][2];
	VectorNormalize(Planes[2].AsVector3D());
	Planes[2][3] = DotProduct(ViewPoint, Planes[2].AsVector3D());

	Planes[3][0] = -CameraMatrix[1][0] - CameraMatrix[3][0];
	Planes[3][1] = -CameraMatrix[1][1] - CameraMatrix[3][1];
	Planes[3][2] = -CameraMatrix[1][2] - CameraMatrix[3][2];
	VectorNormalize(Planes[3].AsVector3D());
	Planes[3][3] = DotProduct(ViewPoint, Planes[3].AsVector3D());

	Planes[4][0] = -CameraMatrix[2][0] - CameraMatrix[3][0];
	Planes[4][1] = -CameraMatrix[2][1] - CameraMatrix[3][1];
	Planes[4][2] = -CameraMatrix[2][2] - CameraMatrix[3][2];
	VectorNormalize(Planes[4].AsVector3D());
	m_pCamera->GetMoveForward(ViewPoint, m_pCamera->GetNearClip());
	Planes[4][3] = DotProduct(ViewPoint, Planes[4].AsVector3D());

	Planes[5][0] = CameraMatrix[2][0] + CameraMatrix[3][0];
	Planes[5][1] = CameraMatrix[2][1] + CameraMatrix[3][1];
	Planes[5][2] = CameraMatrix[2][2] + CameraMatrix[3][2];
	VectorNormalize(Planes[5].AsVector3D());
	m_pCamera->GetMoveForward(ViewPoint, m_pCamera->GetFarClip());
	Planes[5][3] = DotProduct(ViewPoint, Planes[5].AsVector3D());
}


//-----------------------------------------------------------------------------
// Purpose: Returns the 3D grid spacing, in world units.
//-----------------------------------------------------------------------------
float CRender3DMS::GetGridDistance(void)
{
	return(m_RenderState.fGridDistance);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the 3D grid spacing, in world units.
//-----------------------------------------------------------------------------
float CRender3DMS::GetGridSize(void)
{
	return(m_RenderState.fGridSpacing);
}


//-----------------------------------------------------------------------------
// Purpose: Gets the camera viewpoint in world coordinates.
// Input  : pfViewPoint - Pointer to receive the X, Y, Z coordinates of the camera.
//-----------------------------------------------------------------------------
void CRender3DMS::GetViewForward(Vector& pfViewForward)
{
	m_pCamera->GetViewForward(pfViewForward);
}


//-----------------------------------------------------------------------------
// Purpose: Gets the camera viewpoint in world coordinates.
// Input  : pfViewPoint - Pointer to receive the X, Y, Z coordinates of the camera.
//-----------------------------------------------------------------------------
void CRender3DMS::GetViewPoint(Vector& pfViewPoint)
{
	m_pCamera->GetViewPoint(pfViewPoint);
}


//-----------------------------------------------------------------------------
// Purpose: Gets the camera viewpoint in world coordinates.
// Input  : pfViewPoint - Pointer to receive the X, Y, Z coordinates of the camera.
//-----------------------------------------------------------------------------
void CRender3DMS::GetViewRight(Vector& pfViewRight)
{
	m_pCamera->GetViewRight(pfViewRight);
}


//-----------------------------------------------------------------------------
// Purpose: Gets the camera viewpoint in world coordinates.
// Input  : pfViewPoint - Pointer to receive the X, Y, Z coordinates of the camera.
//-----------------------------------------------------------------------------
void CRender3DMS::GetViewUp(Vector& pfViewUp)
{
	m_pCamera->GetViewUp(pfViewUp);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hwnd - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRender3DMS::Initialize(HWND hwnd, CMapDoc *pDoc)
{
	ASSERT(hwnd != NULL);
	ASSERT(pDoc != NULL);
	ASSERT(pDoc->GetMapWorld() != NULL);
	  
	if (!MaterialSystemInterface()->AddView( hwnd ))
	{
		return false;
	}

	MaterialSystemInterface()->SetView( hwnd );

	m_WinData.hWnd = hwnd;
	m_pDoc = pDoc;

	if ((m_WinData.hDC = GetDC(m_WinData.hWnd)) == NULL)
	{
	   ChangeDisplaySettings(NULL, 0);
	   MessageBox(NULL, "GetDC on main window failed", "FATAL ERROR", MB_OK);
	   return(false);
	}

	RECT rect;
	GetClientRect(m_WinData.hWnd, &rect);
	SetSize(rect.right, rect.bottom);

	// Preload all our stuff (textures, etc) for rendering.
	Preload(m_pDoc->GetMapWorld());

	// Store off the three materials we use most often...
	m_pWireframe[0] = g_Textures.FindActiveTexture("editor/wireframe")->GetMaterial();
	m_pWireframe[1] = m_pWireframe[0];
	m_pFlat[0] = g_Textures.FindActiveTexture("editor/flat")->GetMaterial();
	m_pFlat[1] = g_Textures.FindActiveTexture("editor/flatdecal")->GetMaterial();
	m_pTranslucentFlat[0] = g_Textures.FindActiveTexture("editor/translucentflat")->GetMaterial();
	m_pTranslucentFlat[1] = g_Textures.FindActiveTexture("editor/translucentflatdecal")->GetMaterial();
	m_pLightmapGrid[0] = g_Textures.FindActiveTexture("editor/lightmapgrid")->GetMaterial();
	m_pLightmapGrid[1] = g_Textures.FindActiveTexture("editor/lightmapgriddecal")->GetMaterial();
	m_pSelectionOverlay[0] = g_Textures.FindActiveTexture("editor/selectionoverlay")->GetMaterial();
	m_pSelectionOverlay[1] = m_pSelectionOverlay[0]; 
	m_pVertexColor[0] = g_Textures.FindActiveTexture("editor/vertexcolor")->GetMaterial();
	m_pVertexColor[1] = m_pVertexColor[0];

	for (int i = 0; i < 2; ++i)
	{
		if ((!m_pWireframe[i]) || (!m_pFlat[i]) || (!m_pTranslucentFlat[i]) || (!m_pLightmapGrid[i]) || (!m_pSelectionOverlay[i]) || (!m_pVertexColor[i]))
		{
		   MessageBox(NULL, "Missing one or more materials from the materials\\Worldcraft folder.", "FATAL ERROR", MB_OK);
		   return(false);
		}
	}

	// Allocate space for sorting objects with masked textures.
	m_RenderLastObjects.EnsureCapacity(MAX_RENDER_LAST_OBJECTS);

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Determines the visibility of the given axis-aligned bounding box.
// Input  : pBox - Bounding box to evaluate.
// Output : VIS_TOTAL if the box is entirely within the view frustum.
//			VIS_PARTIAL if the box is partially within the view frustum.
//			VIS_NONE if the box is entirely outside the view frustum.
//-----------------------------------------------------------------------------
Visibility_t CRender3DMS::IsBoxVisible(Vector const &BoxMins, Vector const &BoxMaxs)
{
	Vector NearVertex;
	Vector FarVertex;

	//
	// Build the near and far vertices based on the octant of the plane normal.
	//
	for (int i = 0, nInPlanes = 0; i < 6; i++)
	{
		if (m_FrustumPlanes[i][0] > 0)
		{
			NearVertex[0] = BoxMins[0];
			FarVertex[0] = BoxMaxs[0];
		}
		else
		{
			NearVertex[0] = BoxMaxs[0];
			FarVertex[0] = BoxMins[0];
		}

		if (m_FrustumPlanes[i][1] > 0)
		{
			NearVertex[1] = BoxMins[1];
			FarVertex[1] = BoxMaxs[1];
		}
		else
		{
			NearVertex[1] = BoxMaxs[1];
			FarVertex[1] = BoxMins[1];
		}

		if (m_FrustumPlanes[i][2] > 0)
		{
			NearVertex[2] = BoxMins[2];
			FarVertex[2] = BoxMaxs[2];
		}
		else
		{
			NearVertex[2] = BoxMaxs[2];
			FarVertex[2] = BoxMins[2];
		}

		if (DotProduct(m_FrustumPlanes[i].AsVector3D(), NearVertex) >= m_FrustumPlanes[i][3])
		{
			return(VIS_NONE);
		}

		if (DotProduct(m_FrustumPlanes[i].AsVector3D(), FarVertex) < m_FrustumPlanes[i][3])
		{
			nInPlanes++;
		}
	}

	if (nInPlanes == 6)
	{
		return(VIS_TOTAL);
	}
	
	return(VIS_PARTIAL);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eRenderState - 
// Output : Returns true if the render state is enabled, false if it is disabled.
//-----------------------------------------------------------------------------
bool CRender3DMS::IsEnabled(RenderState_t eRenderState)
{
	switch (eRenderState)
	{
		case RENDER_CENTER_CROSSHAIR:
		{
			return(m_RenderState.bCenterCrosshair);
		}

		case RENDER_FRAME_RECT:
		{
			return(m_RenderState.bDrawFrameRect);
		}

		case RENDER_GRID:
		{
			return(m_RenderState.bDrawGrid);
		}

		case RENDER_REVERSE_SELECTION:
		{
			return(m_RenderState.bReverseSelection);
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Determines whether we are rendering for for selection or not.
// Output : Returns true if we are rendering for selection, false if rendering normally.
//-----------------------------------------------------------------------------
bool CRender3DMS::IsPicking(void)
{
	return(m_Pick.bPicking);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the map objects within the rectangle whose upper left corner
//			is at the client coordinates (x, y) and whose width and height are
//			fWidth and fHeight.
// Input  : x - Leftmost point in the rectangle, in client coordinates.
//			y - Topmost point in the rectangle, in client coordinates.
//			fWidth - Width of rectangle, in client coordinates.
//			fHeight - Height of rectangle, in client coordinates.
//			pObjects - Pointer to buffer to receive objects intersecting the rectangle.
//			nMaxObjects - Maximum number of object pointers to place in the buffer.
// Output : Returns the number of object pointers placed in the buffer pointed to
//			by 'pObjects'.
//-----------------------------------------------------------------------------
int CRender3DMS::ObjectsAt(float x, float y, float fWidth, float fHeight, RenderHitInfo_t *pObjects, int nMaxObjects)
{
	m_Pick.fX = x;
	m_Pick.fY = m_WinData.nHeight - (y + 1);
	m_Pick.fWidth = fWidth;
	m_Pick.fHeight = fHeight;
	m_Pick.pHitsDest = pObjects;
	m_Pick.nMaxHits = min(nMaxObjects, MAX_PICK_HITS);
	m_Pick.nNumHits = 0;

	if (!m_RenderState.bReverseSelection)
	{
		m_Pick.uLastZ = 0xFFFFFFFF;
	}
	else
	{
		m_Pick.uLastZ = 0;
	}

	m_Pick.bPicking = true;

	RenderMode_t eOldMode = GetDefaultRenderMode();
	SetDefaultRenderMode( RENDER_MODE_TEXTURED );

	bool bOldLightPreview = IsInLightingPreview();
	SetInLightingPreview( false );

	Render();

	SetDefaultRenderMode( eOldMode );
	SetInLightingPreview( bOldLightPreview );

	m_Pick.bPicking = false;

	MaterialSystemInterface()->SelectionMode(false);

	return(m_Pick.nNumHits);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CRender3DMS::RenderDisplacementTexels( CMapDisp *pMapDisp, int startWidth,
                                            int startHeight, float interval,
                                            float color[3], int orientation, bool *bSelected )
{
	//
	// get greatest distance to normalize the "coloring" of the surface by
	//
	int size = pMapDisp->GetSize();
	float maxDistance = 0.0f;
	for( int i = 0; i < size; i++ )
	{
		float distance = pMapDisp->GetFieldDistance( i );
		if( distance > maxDistance )
		{
			maxDistance = distance;
		}
	}

	if( maxDistance == 0.0f )
	{
		maxDistance = 0.01f;
	}

	//
	// get image width and height
	//
    int width = pMapDisp->GetWidth();
    int height = pMapDisp->GetHeight();


    MaterialSystemInterface()->PushMatrix();
    MaterialSystemInterface()->Translate( startWidth, startHeight, 0.0f );
    MaterialSystemInterface()->Translate( -( width * interval ) / 2.0f, -( height * interval ) / 2.0f, 0.0f );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();

	unsigned char displacementColor[3] = { 0, 0, 0 };
    for( i = 0; i < height; i++ )
    {
        for( int j = 0; j < width; j++ )
        {
			if( m_Pick.bPicking )
			{
	            MaterialSystemInterface()->PushSelectionName( ( unsigned int )( i * width + j ) );
			}
			else
			{
				float colorValue = pMapDisp->GetFieldDistance( i * width + j ) / maxDistance;
				displacementColor[2] = 255 * colorValue;
			}

            meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

            meshBuilder.Position3f( ( j * interval ), ( i * interval ), 10.0f );
			meshBuilder.Color3ubv( displacementColor );
			meshBuilder.AdvanceVertex();

            meshBuilder.Position3f( ( j * interval ), ( i * interval + interval ), 10.0f );
			meshBuilder.Color3ubv( displacementColor );
			meshBuilder.AdvanceVertex();

            meshBuilder.Position3f( ( j * interval + interval ), ( i * interval + interval ), 10.0f );            
			meshBuilder.Color3ubv( displacementColor );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3f( ( j * interval + interval ), ( i * interval ), 10.0f );
			meshBuilder.Color3ubv( displacementColor );
			meshBuilder.AdvanceVertex();

            meshBuilder.End();
			pMesh->Draw();

			if( m_Pick.bPicking )
			{
				if( MaterialSystemInterface()->SelectionMode( true ) != 0 )
                {
					pMapDisp->SetTexelHitIndex( ( i*width+j ) );
                    *bSelected = true;
                }
			}
        }
    }

    //
    // outline the texels in the image
    //
	if( !m_Pick.bPicking )
	{
		SetRenderMode( RENDER_MODE_WIREFRAME );
		IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
		
		for( i = 0; i < height; i++ )
		{
			for( int j = 0; j < width; j++ )
			{
				meshBuilder.Begin( pMesh, MATERIAL_LINE_STRIP, 4 );

				meshBuilder.Position3f( ( j * interval ), ( i * interval ), 10.0f );
				meshBuilder.Color3f( color[0], color[1], color[2] );
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f( ( j * interval ), ( i * interval + interval ), 10.0f );
				meshBuilder.Color3f( color[0], color[1], color[2] );
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f( ( j * interval + interval ), ( i * interval + interval ), 10.0f );            
				meshBuilder.Color3f( color[0], color[1], color[2] );
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f( ( j * interval + interval ), ( i * interval ), 10.0f );
				meshBuilder.Color3f( color[0], color[1], color[2] );
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3f( ( j * interval ), ( i * interval ), 10.0f );
				meshBuilder.Color3f( color[0], color[1], color[2] );
				meshBuilder.AdvanceVertex();

				meshBuilder.End();
				pMesh->Draw();
			}
		}
		SetRenderMode( RENDER_MODE_DEFAULT );
	}

    MaterialSystemInterface()->PopMatrix();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CRender3DMS::RenderDisplacement( CMapDisp *pMapDisp )
{
    int   startWidth, startHeight;
    int   interval;
    float color[3];
    bool  bSelected = false;
    int   orientation = 0;

    // sanity check
    if( pMapDisp->GetSize() == 0 )
        return;

    //
    // get image info
    //
    int width = pMapDisp->GetWidth();
    int height = pMapDisp->GetHeight();

    // setup the renderer
	PreRender();

    //
    // render the primary displacement map
    //
    startWidth = startHeight = 0;
    interval = 10;
    color[0] = 1.0f; color[1] = 1.0f; color[2] = 0.0f;
    RenderDisplacementTexels( pMapDisp, startWidth, startHeight, interval, color, orientation, &bSelected );

#if 0
    //
    // render the neighbors -- if any?
    //
    for( int i = 0; i < 8; i++ )
    {
		CMapDisp *pNeighborMapDisp = NULL;
		int orientation;
		if( i < 4 )
		{
			pMapDisp->GetEdgeNeighbor( i, &pNeighborMapDisp, &orientation );
		}
		else
		{
			// temporary until 3d view stuff is done!!!
			int cornerCount = pMapDisp->GetCornerNeighborCount( i - 4 );
			if( cornerCount >= 1 )
			{
				pMapDisp->GetCornerNeighbor( i - 4, 0, &pNeighborMapDisp, &orientation );
			}
			else
			{
				pNeighborMapDisp = NULL;
			}
		}

        if( !pNeighborMapDisp )
            continue;

        //
        // find orienation of neighbor based on primary displacement map
        //
#if 0
        for( orientation = 0; orientation < 8; orientation++ )
        {
            CMapDisp *pNeighbor = pDispNeighbor->GetNeighbor( orientation );
            if( pNeighbor == pMapDisp )
                break;
        }

        if( ( orientation == 1 ) || ( orientation == 3 ) || ( orientation == 5 ) || ( orientation == 7 ) )
        {
            int edge = ( ( ( i - 1 ) / 2 ) + 2 ) % 4;
            orientation = ( orientation - 1 ) / 2;
            orientation = ( ( edge - orientation ) + 4 ) % 4;
            orientation = 4 - orientation;
        }
        else
        {
            int edge = ( ( i / 2 ) + 2 ) % 4;
            orientation = orientation / 2;
            orientation = ( ( edge - orientation ) + 4 ) % 4;
            orientation = 4 - orientation;
        }
#endif

        // reset the selected flag
        bSelected = false;

        // i am sure there is a better way to do this!!!!
        switch( i )
        {
        case 0:

            
			startWidth = -( width * interval );
            startHeight = 0;
            color[0] = 0.0f;
            color[1] = 1.0f;
            color[2] = 0.0f;
            break;
        case 1:
            startWidth = 0;
            startHeight = height * interval;
            color[0] = 0.0f;
            color[1] = 0.9f;
            color[2] = 0.0f;
            break;
        case 2:
            startWidth = width * interval;
            startHeight = 0;
			color[0] = 0.0f;
            color[1] = 0.8f;
            color[2] = 0.0f;
            break;
        case 3:
            startWidth = 0;
            startHeight = -( height * interval );
            color[0] = 0.0f;
            color[1] = 0.7f;
            color[2] = 0.0f;
            break;
        case 4:
            startWidth = -( width * interval );
            startHeight = -( height * interval );
            color[0] = 0.0f;
            color[1] = 0.6f;
            color[2] = 0.0f;
            break;
        case 5:
            startWidth = width * interval;
            startHeight = -( height * interval );
            color[0] = 0.0f;
            color[1] = 0.5f;
            color[2] = 0.0f;
            break;
        case 6:
            startWidth = -( width * interval );
            startHeight = height * interval;
            color[0] = 0.0f;
            color[1] = 0.4f;
            color[2] = 0.0f;
            break;
        case 7:
            startWidth = width * interval;
            startHeight = height * interval;
            color[0] = 0.0f;
            color[1] = 0.3f;
            color[2] = 0.0f;
            break;
        }

        RenderDisplacementTexels( pNeighborMapDisp, startWidth, startHeight, interval, color, orientation, &bSelected );
        if( bSelected )
            pMapDisp->SetDispMapHitIndex( i );
    }
#endif

    //
    // finish the renderer
    //
	RenderOverlayElements();
	PostRender();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CRender3DMS::SelectDisplacement( CMapDisp *pMapDisp, float x, float y, 
									  float width, float height, 
                                      RenderHitInfo_t *pObjects, int maxObjects )
{
	m_Pick.fX = x;
	m_Pick.fY = m_WinData.nHeight - ( y + 1 );
	m_Pick.fWidth = width;
	m_Pick.fHeight = height;
	m_Pick.pHitsDest = pObjects;
	m_Pick.nMaxHits = min(maxObjects, MAX_PICK_HITS);
	m_Pick.nNumHits = 0;
	m_Pick.uLastZ = 0xFFFFFFFF;

	m_Pick.bPicking = true;

	RenderDisplacement( pMapDisp );

	m_Pick.bPicking = false;

	MaterialSystemInterface()->SelectionMode( false );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CRender3DMS::RenderClear( void )
{
    PreRender();
    PostRender();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRender3DMS::PreRender(void)
{
	g_nRenderFrame++;

	//
	// Determine the elapsed time since the last frame was rendered.
	//
	DWORD dwTimeNow = timeGetTime();
	if (m_dwTimeLastFrame == 0)
	{
		m_dwTimeLastFrame = dwTimeNow;
	}
	DWORD dwTimeElapsed = dwTimeNow - m_dwTimeLastFrame;
	m_fTimeElapsed = (float)dwTimeElapsed / 1000.0;
	m_dwTimeLastFrame = dwTimeNow;

	//
	// Animate the models based on elapsed time.
	//
	CMapStudioModel::AdvanceAnimation(GetElapsedTime());

	// We're drawing to this view now
	MaterialSystemInterface()->SetView( m_WinData.hWnd );
	MaterialSystemInterface()->Viewport(0, 0, m_WinData.nWidth, m_WinData.nHeight);

	//
	// Setup the camera position, orientation, and FOV.
	//
	SetupCamera(m_pCamera);

	//
	// Clear the frame buffer and Z buffer.
	//
	if (!m_Pick.bPicking)
	{
		MaterialSystemInterface()->ClearBuffers( true, true );
	}

	//
	// Setup material system for wireframe, flat, or textured rendering.
	//
	SetupRenderMode(m_eDefaultRenderMode, true);

	//
	// Cache per-frame information from the doc.
	//
	m_RenderState.fGridSpacing = m_pDoc->GetGridSpacing();
	m_RenderState.fGridDistance = m_RenderState.fGridSpacing * 10;
	if (m_RenderState.fGridDistance > 2048)
	{
		m_RenderState.fGridDistance = 2048;
	}
	else if (m_RenderState.fGridDistance < 64)
	{
		m_RenderState.fGridDistance = 64;
	}

	// We do bizarro reverse culling in WC
	MaterialSystemInterface()->CullMode( MATERIAL_CULLMODE_CCW );

	//
	// Empty the list of objects to render last.
	//
	m_RenderLastObjects.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRender3DMS::PostRender(void)
{
	if (m_Pick.bPicking)
	{
		MaterialSystemInterface()->Flush();

		//
		// Some OpenGL drivers, such as the ATI Rage Fury Max, return selection buffer Z values
		// in reverse order. For these cards, we must reverse the selection order.
		//
		if (m_Pick.nNumHits > 1)
		{
			if (!m_RenderState.bReverseSelection)
			{
				qsort(m_Pick.Hits, m_Pick.nNumHits, sizeof(m_Pick.Hits[0]), _CompareHits);
			}
			else
			{
				qsort(m_Pick.Hits, m_Pick.nNumHits, sizeof(m_Pick.Hits[0]), _CompareHitsReverse);
			}
		}

		//
		// Copy the requested number of nearest hits into the destination buffer.
		//
		int nHitsToCopy = min(m_Pick.nNumHits, m_Pick.nMaxHits);
		if (nHitsToCopy != 0)
		{
			memcpy(m_Pick.pHitsDest, m_Pick.Hits, sizeof(m_Pick.Hits[0]) * nHitsToCopy);
		}
	}

	//
	// Copy the GL buffer contents to our window's device context unless we're in pick mode.
	//
	if (!m_Pick.bPicking)
	{
		MaterialSystemInterface()->SwapBuffers();

		if (g_bShowStatistics)
		{
			//
			// Calculate frame rate.
			//
			if (m_dwTimeLastSample != 0)
			{
				DWORD dwTimeNow = timeGetTime();
				DWORD dwTimeElapsed = dwTimeNow - m_dwTimeLastSample;
				if ((dwTimeElapsed > 1000) && (m_nFramesThisSample > 0))
				{
					float fTimeElapsed = (float)dwTimeElapsed / 1000.0;
					m_fFrameRate = m_nFramesThisSample / fTimeElapsed;
					m_nFramesThisSample = 0;
					m_dwTimeLastSample = dwTimeNow;
				}
			}
			else
			{
				m_dwTimeLastSample = timeGetTime();
			}
		
			m_nFramesThisSample++;

			//
			// Display the frame rate and camera position.
			//
			char szText[100];
			Vector ViewPoint;
			m_pCamera->GetViewPoint(ViewPoint);
			int nLen = sprintf(szText, "FPS=%3.2f Pos=[%.f %.f %.f]", m_fFrameRate, ViewPoint[0], ViewPoint[1], ViewPoint[2]);
			TextOut(m_WinData.hDC, 2, 18, szText, nLen);
		}
	}
}


//-----------------------------------------------------------------------------
// Renders the world axes 
//-----------------------------------------------------------------------------
void CRender3DMS::RenderWorldAxes()
{
	// Render the world axes.
	SetRenderMode( RENDER_MODE_WIREFRAME );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh( );
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 3 );

	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.Position3f(0, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.Position3f(100, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(0, 255, 0);
	meshBuilder.Position3f(0, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(0, 255, 0);
	meshBuilder.Position3f(0, 100, 0);
	meshBuilder.AdvanceVertex();
	
	meshBuilder.Color3ub(0, 0, 255);
	meshBuilder.Position3f(0, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3ub(0, 0, 255);
	meshBuilder.Position3f(0, 0, 100);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	SetRenderMode( RENDER_MODE_DEFAULT );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRender3DMS::Render(void)
{
	if ((m_WinData.nWidth == 0) || (m_WinData.nHeight == 0))
	{
		// Not initialized yet.
		return;
	}

	if (!m_pCamera)
	{
		// Camera not set up yet.
		return;
	}

	PreRender();
	
	RenderWorldAxes();

	//
	// Deferred rendering lets us sort everything here by material.
	//
	if (!IsPicking())
	{
		m_DeferRendering = true;
	}

	if (IsInLightingPreview())
	{
		// Lighting preview?
		IBSPLighting *pBSPLighting = m_pDoc->GetBSPLighting();
		if (pBSPLighting)
		{
			pBSPLighting->Draw();
		}
	}

	//
	// Render the world using octree culling.
	//
	if (g_bUseCullTree)
	{
		RenderTree();
	}
	//
	// Render the world without octree culling.
	//
	else
	{
		RenderMapClass(m_pDoc->GetMapWorld());
	}

	if (!IsPicking())
	{
		m_DeferRendering = false;

		// An optimization... render tree doesn't actually render anythung
		// This here will do the rendering, sorted by material by pass
		CMapFace::RenderOpaqueFaces(this);
	}

	RenderTools();
	RenderPointsFile();

	if (m_RenderLastObjects.Size() > 0)
	{
		// Need to sort	our last objects by their center...
		// We want to render these back to front
		float* pDepth = (float*)_alloca( m_RenderLastObjects.Size() * sizeof(float) );

		Vector direction, center;
		m_pCamera->GetViewForward(direction);
		for (int i = 0; i < m_RenderLastObjects.Size(); ++i )
		{
			CMapAtom* pMapAtom = m_RenderLastObjects[i]; 
			((CMapClass*)pMapAtom)->GetOrigin(center);

			float depth = center.Dot( direction );
			
			// Yeah, this isn't incredibly fast. But it was easy to write!
			int j = i;
			while ( --j >= 0 )
			{
				if (pDepth[j] > depth)
					break;

				// move up...
				pDepth[j + 1] = pDepth[j];
				m_RenderLastObjects[j + 1] = m_RenderLastObjects[j];
			}

			// insert after j
			++j;
			pDepth[j] = depth;
			m_RenderLastObjects[j] = pMapAtom;
		}

		RenderObjectList(m_RenderLastObjects);
	}

#ifdef _DEBUG
	if (m_bRenderFrustum)
	{
		RenderFrustum();
	}
#endif

	//
	// Render any 2D elements that overlay the 3D view, like a center crosshair.
	//
	RenderOverlayElements();

	PostRender();
}


//-----------------------------------------------------------------------------
// Purpose: render an arrow of a given color at a given position (start and end)
//          in world space
// Input  : vStartPt - the arrow starting point
//          vEndPt - the arrow ending point (the head of the arrow)
//          chRed, chGree, chBlue - the arrow color
//-----------------------------------------------------------------------------
void CRender3DMS::RenderArrow( Vector const &vStartPt, Vector const &vEndPt, 
							   unsigned char chRed, unsigned char chGreen, unsigned char chBlue )
{
	// set to a flat shaded render mode
	SetRenderMode( RENDER_MODE_FLAT );

	//
	// render the stick portion of the arrow
	//
	CMeshBuilder meshBuilder;
	IMesh *pMesh = MaterialSystemInterface()->GetDynamicMesh();
	if( !pMesh )
		return;

	meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );
	meshBuilder.Position3f( vStartPt.x, vStartPt.y, vStartPt.z );
	meshBuilder.Color3ub( chRed, chGreen, chBlue );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( vEndPt.x, vEndPt.y, vEndPt.z );
	meshBuilder.Color3ub( chRed, chGreen, chBlue );
	meshBuilder.AdvanceVertex();
	meshBuilder.End();
	pMesh->Draw();

	// set to a default render mode
	SetRenderMode( RENDER_MODE_DEFAULT );

	//
	// render the tip of the arrow
	//
	Vector coneAxis = vEndPt - vStartPt;
	float length = VectorNormalize( coneAxis );
	float length8 = length * 0.125;
	length -= length8;
	
	Vector vBasePt;
	vBasePt = vStartPt + coneAxis * length;

	RenderCone( vBasePt, vEndPt, ( length8 * 0.333 ), 6, chRed, chGreen, chBlue );
}


//-----------------------------------------------------------------------------
// Purpose: Renders a box in flat shaded or wireframe depending on our render mode.
// Input  : chRed - 
//			chGreen - 
//			chBlue - 
//-----------------------------------------------------------------------------
void CRender3DMS::RenderBox(const Vector &Mins, const Vector &Maxs,
		unsigned char chRed, unsigned char chGreen, unsigned char chBlue, SelectionState_t eBoxSelectionState)
{
	Vector FacePoints[8];

	FacePoints[0][0] = Mins[0];
	FacePoints[0][1] = Mins[1];
	FacePoints[0][2] = Mins[2];

	FacePoints[1][0] = Mins[0];
	FacePoints[1][1] = Mins[1];
	FacePoints[1][2] = Maxs[2];

	FacePoints[2][0] = Mins[0];
	FacePoints[2][1] = Maxs[1];
	FacePoints[2][2] = Mins[2];

	FacePoints[3][0] = Mins[0];
	FacePoints[3][1] = Maxs[1];
	FacePoints[3][2] = Maxs[2];

	FacePoints[4][0] = Maxs[0];
	FacePoints[4][1] = Mins[1];
	FacePoints[4][2] = Mins[2];

	FacePoints[5][0] = Maxs[0];
	FacePoints[5][1] = Mins[1];
	FacePoints[5][2] = Maxs[2];

	FacePoints[6][0] = Maxs[0];
	FacePoints[6][1] = Maxs[1];
	FacePoints[6][2] = Mins[2];

	FacePoints[7][0] = Maxs[0];
	FacePoints[7][1] = Maxs[1];
	FacePoints[7][2] = Maxs[2];

	int nFaces[6][4] =
	{
		{ 0, 2, 3, 1 },
		{ 0, 1, 5, 4 },
		{ 4, 5, 7, 6 },
		{ 2, 6, 7, 3 },
		{ 1, 3, 7, 5 },
		{ 0, 4, 6, 2 }
	};

	RenderMode_t eRenderModeThisPass;
	int nPasses;

	if ((eBoxSelectionState != SELECT_NONE) && (GetDefaultRenderMode() != RENDER_MODE_WIREFRAME))
	{
		nPasses = 2;
	}
	else
	{
		nPasses = 1;
	}

	for (int nPass = 1; nPass <= nPasses; nPass++)
	{
		if (nPass == 1)
		{
			eRenderModeThisPass = GetDefaultRenderMode();

			// There's no texture for a bounding box.
			if ((eRenderModeThisPass == RENDER_MODE_TEXTURED) ||
				(eRenderModeThisPass == RENDER_MODE_LIGHTMAP_GRID))
			{
				eRenderModeThisPass = RENDER_MODE_FLAT;
			}

			SetRenderMode(eRenderModeThisPass);
		}
		else
		{
			eRenderModeThisPass = RENDER_MODE_WIREFRAME;
			SetRenderMode(eRenderModeThisPass);
		}

		for (int nFace = 0; nFace < 6; nFace++)
		{
			Vector Edge1, Edge2, Normal;
			int nP1, nP2, nP3, nP4;

			nP1 = nFaces[nFace][0];
			nP2 = nFaces[nFace][1];
			nP3 = nFaces[nFace][2];
			nP4 = nFaces[nFace][3];

			VectorSubtract(FacePoints[nP4], FacePoints[nP1], Edge1);
			VectorSubtract(FacePoints[nP2], FacePoints[nP1], Edge2);
			CrossProduct(Edge1, Edge2, Normal);
			VectorNormalize(Normal);

			//
			// If we are rendering using one of the lit modes, calculate lighting.
			// 
			unsigned char color[3];

			assert( (eRenderModeThisPass != RENDER_MODE_TEXTURED) &&
				(eRenderModeThisPass != RENDER_MODE_LIGHTMAP_GRID) ); 
			if ((eRenderModeThisPass == RENDER_MODE_FLAT))
			{
				float fShade = LightPlane(Normal);

				//
				// For flat and textured mode use the face color with lighting.
				//
				if (eBoxSelectionState != SELECT_NONE)
				{
					color[0] = SELECT_FACE_RED * fShade;
					color[1] = SELECT_FACE_GREEN * fShade;
					color[2] = SELECT_FACE_BLUE * fShade;
				}
				else
				{
					color[0] = chRed * fShade;
					color[1] = chGreen * fShade;
					color[2] = chBlue * fShade;
				}
			}
			//
			// For wireframe mode use the face color without lighting.
			//
			else
			{
				if (eBoxSelectionState != SELECT_NONE)
				{
					color[0] = SELECT_FACE_RED;
					color[1] = SELECT_FACE_GREEN;
					color[2] = SELECT_FACE_BLUE;
				}
				else
				{
					color[0] = chRed;
					color[1] = chGreen;
					color[2] = chBlue;
				}
			}

			//
			// Draw the face.
			//
			bool wireframe = (eRenderModeThisPass == RENDER_MODE_WIREFRAME);

			CMeshBuilder meshBuilder;
			IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();
			meshBuilder.DrawQuad( pMesh, FacePoints[nP1].Base(), FacePoints[nP2].Base(), 
				FacePoints[nP3].Base(), FacePoints[nP4].Base(), color, wireframe );
		}
	}

	SetRenderMode(RENDER_MODE_DEFAULT);
}


//-----------------------------------------------------------------------------
// Purpose: render a cone of a given color at a given position in world space
// Intput : vBasePt - the start point of the cone (the base point)
//          vTipPt - the end point of the cone (the peak)
//          fRadius - the radius (at the base) of the cone
//          nSlices - the number of slices (segments) making up the cone
//          chRed, chGreen, chBlue - the cone color
//-----------------------------------------------------------------------------
void CRender3DMS::RenderCone( Vector const &vBasePt, Vector const &vTipPt, float fRadius, int nSlices,
		                      unsigned char chRed, unsigned char chGreen, unsigned char chBlue )
{
	// get the angle between slices (in radians)
	float sliceAngle = ( 2 * M_PI ) / ( float )nSlices;

	//
	// allocate ALIGNED!!!!!!! vectors for cone base
	//
	int size = nSlices * sizeof( Vector );
	size += 16 + sizeof( Vector* );
	byte *ptr = ( byte* )_alloca( size );
	long data = ( long )ptr;
	
	data += 16 + sizeof( Vector* ) - 1;
	data &= -16;

	(( void** )data)[-1] = ptr;

	Vector *pPts = ( Vector* )data;
	if( !pPts )
		return;

	//
	// calculate the cone's base points in a local space (x,y plane)
	//
	for( int i = 0; i < nSlices; i++ )
	{
		pPts[i].x = fRadius * cos( ( sliceAngle * -i ) );
		pPts[i].y = fRadius * sin( ( sliceAngle * -i ) );
		pPts[i].z = 0.0f;
	}

	//
	// get cone tip in local space
	//
	Vector coneAxis = vTipPt - vBasePt;
	float length = coneAxis.Length();
	Vector tipPt( 0.0f, 0.0f, length );

	//
	// create cone faces
	//
	CMapFaceList m_Faces;
	Vector ptList[3];
	
	// triangulate the base
	for( i = 0; i < ( nSlices - 2 ); i++ )
	{	
		ptList[0] = pPts[0];
		ptList[1] = pPts[i+1];
		ptList[2] = pPts[i+2];

		// add face to list
		CMapFace *pFace = new CMapFace;
		if( !pFace )
			return;
		pFace->SetRenderColor( chRed, chGreen, chBlue );
		pFace->CreateFace( ptList, 3 );
		pFace->RenderUnlit( true );
		m_Faces.AddToTail( pFace );
	}

	// triangulate the sides
	for( i = 0; i < nSlices; i++ )
	{
		ptList[0] = pPts[i];
		ptList[1] = tipPt;
		ptList[2] = pPts[(i+1)%nSlices];

		// add face to list
		CMapFace *pFace = new CMapFace;
		if( !pFace )
			return;
		pFace->SetRenderColor( chRed, chGreen, chBlue );
		pFace->CreateFace( ptList, 3 );
		pFace->RenderUnlit( true );
		m_Faces.AddToTail( pFace );
	}

	//
	// rotate base points into world space as they are being rendered
	//
	VectorNormalize( coneAxis );
	QAngle rotAngles;
	VectorAngles( coneAxis, rotAngles );
	rotAngles[PITCH] += 90;

	MaterialSystemInterface()->MatrixMode( MATERIAL_MODEL ); 
	MaterialSystemInterface()->PushMatrix();
	MaterialSystemInterface()->LoadIdentity();

	MaterialSystemInterface()->Translate( vBasePt.x, vBasePt.y, vBasePt.z );

	MaterialSystemInterface()->Rotate( rotAngles[YAW], 0, 0, 1 );
	MaterialSystemInterface()->Rotate( rotAngles[PITCH], 0, 1, 0 );
	MaterialSystemInterface()->Rotate( rotAngles[ROLL], 1, 0, 0 );

	// set to a flat shaded render mode
	SetRenderMode( RENDER_MODE_FLAT );

	for ( i = 0; i < m_Faces.Count(); i++ )
	{
		CMapFace *pFace = m_Faces.Element( i );
		if( !pFace )
			continue;
		pFace->Render3D( this );
	}

	MaterialSystemInterface()->PopMatrix();

	// set back to default render mode
	SetRenderMode( RENDER_MODE_DEFAULT );

	//
	// delete the faces in the list
	//
	for ( i = 0; i < m_Faces.Count(); i++ )
	{
		CMapFace *pFace = m_Faces.Element( i );
		delete pFace;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws a line in the specified color.
// Input  : vec1 - Start position.
//			vec2 - End position.
//			r g b - Line color [0..255]
//-----------------------------------------------------------------------------
void CRender3DMS::RenderLine(const Vector &vec1, const Vector &vec2, unsigned char r, unsigned char g, unsigned char b)
{
	IMesh *pMesh = MaterialSystemInterface()->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin(pMesh, MATERIAL_LINES, 1);

	meshBuilder.Position3fv(vec1.Base());
	meshBuilder.Color3ub(r, g, b);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv(vec2.Base());
	meshBuilder.Color3ub(r, g, b);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vCenter - 
//			flRadius - 
//			nTheta - Number of vertical slices in the sphere.
//			nPhi - Number of horizontal slices in the sphere.
//			chRed - 
//			chGreen - 
//			chBlue - 
//-----------------------------------------------------------------------------
void CRender3DMS::RenderSphere(Vector const &vCenter, float flRadius, int nTheta, int nPhi,
							   unsigned char chRed, unsigned char chGreen, unsigned char chBlue )
{
	int nTriangles =  2 * nTheta * ( nPhi - 1 ); // Two extra degenerate triangles per row (except the last one)
	int nIndices = 2 * ( nTheta + 1 ) * ( nPhi - 1 );

	MaterialSystemInterface()->Bind( m_pVertexColor[0] );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, nTriangles, nIndices );

	//
	// Build the index buffer.
	//
	int i, j;
	for ( i = 0; i < nPhi; ++i )
	{
		for ( j = 0; j < nTheta; ++j )
		{
			float u = j / ( float )( nTheta - 1 );
			float v = i / ( float )( nPhi - 1 );
			float theta = 2.0f * M_PI * u;
			float phi = M_PI * v;

			Vector vecPos;
			vecPos.x = flRadius * sin(phi) * cos(theta);
			vecPos.y = flRadius * sin(phi) * sin(theta); 
			vecPos.z = flRadius * cos(phi);

			Vector vecNormal = vecPos;
			VectorNormalize(vecNormal);

			float flScale = LightPlane(vecNormal);

			unsigned char red = chRed * flScale;
			unsigned char green = chGreen * flScale;
			unsigned char blue = chBlue * flScale;

			vecPos += vCenter;

			meshBuilder.Position3f( vecPos.x, vecPos.y, vecPos.z );
			meshBuilder.Color3ub( red, green, blue );
			meshBuilder.AdvanceVertex();
		}
	}

	//
	// Emit the triangle strips.
	//
	int idx = 0;
	for ( i = 0; i < nPhi - 1; ++i )
	{
		for ( j = 0; j < nTheta; ++j )
		{
			idx = nTheta * i + j;

			meshBuilder.Index( idx + nTheta );
			meshBuilder.AdvanceIndex();

			meshBuilder.Index( idx );
			meshBuilder.AdvanceIndex();
		}

		//
		// Emit a degenerate triangle to skip to the next row without
		// a connecting triangle.
		//
		if ( i < nPhi - 2 )
		{
			meshBuilder.Index( idx );
			meshBuilder.AdvanceIndex();

			meshBuilder.Index( idx + nTheta + 1 );
			meshBuilder.AdvanceIndex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRender3DMS::RenderWireframeSphere(Vector const &vCenter, float flRadius, int nTheta, int nPhi,
							            unsigned char chRed, unsigned char chGreen, unsigned char chBlue )
{
	SetRenderMode(RENDER_MODE_WIREFRAME);

	// Make one more coordinate because (u,v) is discontinuous.
	++nTheta;

	int nVertices = nPhi * nTheta; 
	int nIndices = ( nTheta - 1 ) * 4 * ( nPhi - 1 );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_LINES, nVertices, nIndices );

	int i, j;
	for ( i = 0; i < nPhi; ++i )
	{
		for ( j = 0; j < nTheta; ++j )
		{
			float u = j / ( float )( nTheta - 1 );
			float v = i / ( float )( nPhi - 1 );
			float theta = 2.0f * M_PI * u;
			float phi = M_PI * v;

			int idx = nTheta * i + j;

			meshBuilder.Position3f( vCenter.x + ( flRadius * sin(phi) * cos(theta) ),
				                    vCenter.y + ( flRadius * sin(phi) * sin(theta) ), 
									vCenter.z + ( flRadius * cos(phi) ) );
			meshBuilder.Color3ub( chRed, chGreen, chBlue );
			meshBuilder.AdvanceVertex();
		}
	}

	for ( i = 0; i < nPhi - 1; ++i )
	{
		for ( j = 0; j < nTheta - 1; ++j )
		{
			int idx = nTheta * i + j;

			meshBuilder.Index( idx );
			meshBuilder.AdvanceIndex();

			meshBuilder.Index( idx + nTheta );
			meshBuilder.AdvanceIndex();

			meshBuilder.Index( idx );
			meshBuilder.AdvanceIndex();

			meshBuilder.Index( idx + 1 );
			meshBuilder.AdvanceIndex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();

	SetRenderMode(RENDER_MODE_DEFAULT);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ppObjects - 
//			nObjects - 
//-----------------------------------------------------------------------------
void CRender3DMS::RenderObjectList(CMapAtomVector &Objects)
{
	for (int nObject = 0; nObject < Objects.Size(); nObject++)
	{
		Objects[nObject]->Render3D(this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pDrawDC - 
//-----------------------------------------------------------------------------
void CRender3DMS::RenderPointsFile(void)
{
	int &nPFPoints = m_pDoc->m_nPFPoints;
	if (nPFPoints > 0)
	{
		Vector* &pPFPoints = m_pDoc->m_pPFPoints;

		SetRenderMode(RENDER_MODE_WIREFRAME);

		// FIXME:  glLineWidth(2);

		CMeshBuilder meshBuilder;
		IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh( );
		meshBuilder.Begin( pMesh, MATERIAL_LINE_STRIP, nPFPoints - 1 );

		for (int i = 0; i < nPFPoints; i++)
		{
			meshBuilder.Position3f(pPFPoints[i][0], pPFPoints[i][1], pPFPoints[i][2]);
			meshBuilder.Color3ub(255, 0, 0);
			meshBuilder.AdvanceVertex();
		}

		meshBuilder.End();
		pMesh->Draw();

		// FIXME:  glLineWidth(1);

		SetRenderMode(RENDER_MODE_DEFAULT);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws a wireframe box using the given color.
// Input  : pfMins - Pointer to the box minima in all 3 dimensions.
//			pfMins - Pointer to the box maxima in all 3 dimensions.
//			chRed, chGreen, chBlue - Red, green, and blue color compnents for the box.
//-----------------------------------------------------------------------------
void CRender3DMS::RenderWireframeBox(const Vector &Mins, const Vector &Maxs,
		unsigned char chRed, unsigned char chGreen, unsigned char chBlue)
{
	//
	// Draw the box bottom, top, and one corner edge.
	//

	SetRenderMode( RENDER_MODE_WIREFRAME );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh( );

	meshBuilder.Begin( pMesh, MATERIAL_LINE_STRIP, 9 );

	meshBuilder.Position3f(Mins[0], Mins[1], Mins[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Maxs[0], Mins[1], Mins[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Maxs[0], Maxs[1], Mins[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Mins[0], Maxs[1], Mins[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Mins[0], Mins[1], Mins[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Mins[0], Mins[1], Maxs[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Maxs[0], Mins[1], Maxs[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Maxs[0], Maxs[1], Maxs[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Mins[0], Maxs[1], Maxs[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Mins[0], Mins[1], Maxs[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	//
	// Draw the three missing edges.
	//
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 3 );

	meshBuilder.Position3f(Maxs[0], Mins[1], Mins[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f(Maxs[0], Mins[1], Maxs[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Maxs[0], Maxs[1], Mins[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f(Maxs[0], Maxs[1], Maxs[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(Mins[0], Maxs[1], Mins[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f(Mins[0], Maxs[1], Maxs[2]);
	meshBuilder.Color3ub(chRed, chGreen, chBlue);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	SetRenderMode( RENDER_MODE_DEFAULT );
}


//-----------------------------------------------------------------------------
// Purpose: Renders this object (and all of its children) if it is visible and
//			has not already been rendered this frame.
// Input  : pMapClass - Pointer to the object to be rendered.
//-----------------------------------------------------------------------------
void CRender3DMS::RenderMapClass(CMapClass *pMapClass)
{
	ASSERT(pMapClass != NULL);

	if ((pMapClass != NULL) && (pMapClass->GetRenderFrame() != g_nRenderFrame))
	{
		if (pMapClass->IsVisible())
		{
			//
			// Render this object's culling box if it is enabled.
			//
			if (g_bRenderCullBoxes)
			{
				Vector mins;
				Vector maxs;
				pMapClass->GetCullBox(mins, maxs);
				RenderWireframeBox(mins, maxs, 255, 0, 0);
			}

			//
			// If we should render this object after all the other objects,
			// just add it to a list of objects to render last. Otherwise, render it now.
			//
			if (!pMapClass->ShouldRenderLast())
			{
				pMapClass->Render3D(this);
			}
			else
			{
				m_RenderLastObjects.AddToTail(pMapClass);
			}

			//
			// Render this object's children.
			//
			POSITION pos;
			CMapClass *pChild = pMapClass->GetFirstChild(pos);
			while (pChild != NULL)
			{
				Vector vecMins;
				Vector vecMaxs;
				pChild->GetCullBox(vecMins, vecMaxs);

				if (IsBoxVisible(vecMins, vecMaxs) != VIS_NONE)
				{
					RenderMapClass(pChild);
				}

				pChild = pMapClass->GetNextChild(pos);
			}
		}

		//
		// Consider this object as handled for this frame.
		//
		pMapClass->SetRenderFrame(g_nRenderFrame);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Prepares all objects in this node for rendering.
// Input  : pParent - 
//-----------------------------------------------------------------------------
void CRender3DMS::Preload(CMapClass *pParent)
{
	ASSERT(pParent != NULL);

	if (pParent != NULL)
	{
		//
		// Preload this object's children.
		//
		POSITION pos;
		CMapClass *pChild = pParent->GetFirstChild(pos);
		while (pos != NULL)
		{
			pChild->RenderPreload(this, true);
			pChild = pParent->GetNextChild(pos);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Renders all objects in this node if this node is visible.
// Input  : pNode - The node to render.
//			bForce - If true, don't check for visibility, just render the node
//				and all of its children.
//-----------------------------------------------------------------------------
void CRender3DMS::RenderNode(CCullTreeNode *pNode, bool bForce )
{
	//
	// Render all child nodes first.
	//
	CCullTreeNode *pChild;
	int nChildren = pNode->GetChildCount();
	if (nChildren != 0)
	{
		for (int nChild = 0; nChild < nChildren; nChild++)
		{
			pChild = pNode->GetCullTreeChild(nChild);
			ASSERT(pChild != NULL);

			if (pChild != NULL)
			{
				//
				// Only bother checking nodes with children or objects.
				//
				if ((pChild->GetChildCount() != 0) || (pChild->GetObjectCount() != 0))
				{
					bool bForceThisChild = bForce;
					Visibility_t eVis = VIS_NONE;

					if (!bForceThisChild)
					{
						Vector vecMins;
						Vector vecMaxs;
						pChild->GetBounds(vecMins, vecMaxs);
						eVis = IsBoxVisible(vecMins, vecMaxs);
						if (eVis == VIS_TOTAL)
						{
							bForceThisChild = true;
						}
					}

					if ((bForceThisChild) || (eVis != VIS_NONE))
					{
						RenderNode(pChild, bForceThisChild);
					}
				}
			}
		}
	}
	else
	{
		//
		// Now render the contents of this node.
		//
		CMapClass *pObject;
		int nObjects = pNode->GetObjectCount();
		for (int nObject = 0; nObject < nObjects; nObject++)
		{
			pObject = pNode->GetCullTreeObject(nObject);
			ASSERT(pObject != NULL);

			Vector vecMins;
			Vector vecMaxs;
			pObject->GetCullBox(vecMins, vecMaxs);
			if (IsBoxVisible(vecMins, vecMaxs) != VIS_NONE)
			{
				RenderMapClass(pObject);
			}
		}
	}
}


static void RenderCrossHair( CRender3D* pRender, int windowWidth, int windowHeight )
{
	int nCenterX;
	int nCenterY;

	nCenterX = windowWidth / 2;
	nCenterY = windowHeight / 2;

	// Render the world axes
	pRender->SetRenderMode( RENDER_MODE_WIREFRAME );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh( );
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 6 );

	meshBuilder.Position3f(nCenterX - CROSSHAIR_DIST_HORIZONTAL, nCenterY - 1, 0);
	meshBuilder.Color3ub(0, 0, 0);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f(nCenterX + CROSSHAIR_DIST_HORIZONTAL + 1, nCenterY - 1, 0);
	meshBuilder.Color3ub(0, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(nCenterX - CROSSHAIR_DIST_HORIZONTAL, nCenterY + 1, 0);
	meshBuilder.Color3ub(0, 0, 0);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f(nCenterX + CROSSHAIR_DIST_HORIZONTAL + 1, nCenterY + 1, 0);
	meshBuilder.Color3ub(0, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(nCenterX - 1, nCenterY - CROSSHAIR_DIST_VERTICAL, 0);
	meshBuilder.Color3ub(0, 0, 0);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f(nCenterX - 1, nCenterY + CROSSHAIR_DIST_VERTICAL, 0);
	meshBuilder.Color3ub(0, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(nCenterX + 1, nCenterY - CROSSHAIR_DIST_VERTICAL, 0);
	meshBuilder.Color3ub(0, 0, 0);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f(nCenterX + 1, nCenterY + CROSSHAIR_DIST_VERTICAL, 0);
	meshBuilder.Color3ub(0, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(nCenterX - CROSSHAIR_DIST_HORIZONTAL, nCenterY, 0);
	meshBuilder.Color3ub(255, 255, 255);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f(nCenterX + CROSSHAIR_DIST_HORIZONTAL + 1, nCenterY, 0);
	meshBuilder.Color3ub(255, 255, 255);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(nCenterX, nCenterY - CROSSHAIR_DIST_VERTICAL, 0);
	meshBuilder.Color3ub(255, 255, 255);
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f(nCenterX, nCenterY + CROSSHAIR_DIST_VERTICAL, 0);
	meshBuilder.Color3ub(255, 255, 255);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}


static void RenderFrameRect( CRender3D* pRender, int windowWidth, int windowHeight )
{
	// Render the world axes.
	pRender->SetRenderMode( RENDER_MODE_WIREFRAME );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = MaterialSystemInterface()->GetDynamicMesh( );
	meshBuilder.Begin( pMesh, MATERIAL_LINE_STRIP, 4 );

	meshBuilder.Position3f(1, 0, 0);
	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(windowWidth, 0, 0);
	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(windowWidth, windowHeight - 1, 0);
	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(1, windowHeight - 1, 0);
	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f(1, 0, 0);
	meshBuilder.Color3ub(255, 0, 0);
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Purpose: Renders 2D elements that overlay the 3D objects.
//-----------------------------------------------------------------------------
void CRender3DMS::RenderOverlayElements(void)
{
	if ((m_RenderState.bCenterCrosshair) || (m_RenderState.bDrawFrameRect))
		BeginParallel();

	if (m_RenderState.bCenterCrosshair)
		RenderCrossHair( this, m_WinData.nWidth, m_WinData.nHeight );

	if (m_RenderState.bDrawFrameRect)
		RenderFrameRect( this, m_WinData.nWidth, m_WinData.nHeight );

	if ((m_RenderState.bCenterCrosshair) || (m_RenderState.bDrawFrameRect))
		EndParallel();
}


//-----------------------------------------------------------------------------
// Purpose: Gives all the tools a chance to render themselves.
//-----------------------------------------------------------------------------
void CRender3DMS::RenderTools(void)
{
	int nToolCount = g_pToolManager->GetToolCount();
	for (int i = 0; i < nToolCount; i++)
	{
		CBaseTool *pTool = g_pToolManager->GetTool(i);
		pTool->RenderTool3D(this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRender3DMS::RenderTree(void)
{
	ASSERT(m_pDoc != NULL);
	if (m_pDoc == NULL)
	{
		return;
	}

	CMapWorld *pWorld = m_pDoc->GetMapWorld();
	if (pWorld == NULL)
	{
		return;
	}

	//
	// Recursively traverse the culling tree, rendering visible nodes.
	//
	CCullTreeNode *pTree = pWorld->CullTree_GetCullTree();
	if (pTree != NULL)
	{
		Vector vecMins;
		Vector vecMaxs;
		pTree->GetBounds(vecMins, vecMaxs);
		Visibility_t eVis = IsBoxVisible(vecMins, vecMaxs);

		if (eVis != VIS_NONE)
		{
			RenderNode(pTree, eVis == VIS_TOTAL);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether we are in lighting preview mode or not.
//-----------------------------------------------------------------------------
bool CRender3DMS::IsInLightingPreview()
{
	return m_bLightingPreview;
}


//-----------------------------------------------------------------------------
// Purpose: Enables/disables lighting preview mode.
//-----------------------------------------------------------------------------
void CRender3DMS::SetInLightingPreview( bool bLightingPreview )
{
	m_bLightingPreview = bLightingPreview;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCamera - 
//-----------------------------------------------------------------------------
void CRender3DMS::SetCamera(CCamera *pCamera)
{
	m_pCamera = pCamera;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRender3DMS::SetProjectionMode( ProjectionMode_t eProjMode )
{
	m_ProjMode = eProjMode;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eRenderMode - 
//-----------------------------------------------------------------------------
void CRender3DMS::SetDefaultRenderMode(RenderMode_t eRenderMode)
{
	if (eRenderMode == RENDER_MODE_DEFAULT)
	{
		eRenderMode = RENDER_MODE_WIREFRAME;
	}

	m_eDefaultRenderMode = eRenderMode;
	if (m_eCurrentRenderMode != eRenderMode)
	{
		SetupRenderMode(eRenderMode);
	}
}



void CRender3DMS::ResetFocus()
{
	// A bizarre workaround; the drop-down menu somehow
	// sets some wierd state that causes the whole screen to not be updated
	InvalidateRect( m_WinData.hWnd, 0, false );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eRenderMode - 
//-----------------------------------------------------------------------------
void CRender3DMS::SetRenderMode(RenderMode_t eRenderMode, bool force)
{
	if (eRenderMode == RENDER_MODE_DEFAULT)
	{
		eRenderMode = m_eDefaultRenderMode;
	}

	if (m_eCurrentRenderMode != eRenderMode || force)
	{
		SetupRenderMode(eRenderMode, force);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles changes in size of the render window.
// Input  : nWidth - New window width.
//			nHeight - New window height.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRender3DMS::SetSize(int nWidth, int nHeight)
{
	//
	// Update our window data.
	//
	m_WinData.nWidth = nWidth;
	m_WinData.nHeight = nHeight;
	m_WinData.gldAspect = (float)m_WinData.nWidth / (float)m_WinData.nHeight;

	MaterialSystemInterface()->Viewport(0, 0, m_WinData.nWidth, m_WinData.nHeight);

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Prepares for rendering perspective-projected components of the 3D view.
// Input  : *pCamera - 
//-----------------------------------------------------------------------------
void CRender3DMS::SetupCamera(CCamera *pCamera)
{
	ASSERT(pCamera != NULL);
	if (pCamera == NULL)
	{
		return;
	}

	//
	// Set up our perspective transformation.
	//
	MaterialSystemInterface()->MatrixMode(MATERIAL_PROJECTION);
	MaterialSystemInterface()->LoadIdentity();

//	MaterialSystemInterface()->PickMatrix(m_Pick.fX, m_Pick.fY, m_WinData.nWidth / 2.0, m_WinData.nHeight / 2.0);

	if (m_Pick.bPicking)
	{
		MaterialSystemInterface()->PickMatrix(m_Pick.fX, m_Pick.fY, m_Pick.fWidth, m_Pick.fHeight);
		MaterialSystemInterface()->SelectionBuffer(m_Pick.uSelectionBuffer, sizeof(m_Pick.uSelectionBuffer));
		MaterialSystemInterface()->SelectionMode(true);
		MaterialSystemInterface()->ClearSelectionNames();
	}

	if( m_ProjMode == PROJECTION_PERSPECTIVE )
	{
		MaterialSystemInterface()->PerspectiveX( pCamera->GetHorizontalFOV(), 
			m_WinData.gldAspect, pCamera->GetNearClip(), pCamera->GetFarClip() );
	}
	else if( m_ProjMode == PROJECTION_ORTHOGRAPHIC )
	{
		MaterialSystemInterface()->Scale( 1, -1, 1 );
		MaterialSystemInterface()->Ortho( 
				-m_WinData.nWidth / 2.0, -m_WinData.nHeight / 2.0,
				 m_WinData.nWidth / 2.0, m_WinData.nHeight / 2.0,
				 pCamera->GetNearClip(), pCamera->GetFarClip() );
	}

	//
	// Set up the view matrix.
	//
	matrix4_t ViewMatrix;
	pCamera->GetViewMatrix(ViewMatrix);
	TransposeMatrix( ViewMatrix );
	MaterialSystemInterface()->MatrixMode(MATERIAL_VIEW);
	MaterialSystemInterface()->LoadMatrix(&ViewMatrix[0][0]);

    //
    // handle the orthographic zoom
    //
    if( m_ProjMode == PROJECTION_ORTHOGRAPHIC )
    {
        float scale = m_pCamera->GetZoom();
        MaterialSystemInterface()->Scale( scale, scale, scale );
    }

	//
	// Build the frustum planes for view volume culling.
	//
	CCamera *pTempCamera;

	if (m_bDroppedCamera)
	{
		pTempCamera = m_pCamera;
		m_pCamera = m_pDropCamera;
	}

	GetFrustumPlanes(m_FrustumPlanes);

	// For debugging frustum planes
#ifdef _DEBUG
	if (m_bRecomputeFrustumRenderGeometry)
	{
		ComputeFrustumRenderGeometry();
		m_bRecomputeFrustumRenderGeometry = false;
	}
#endif

	if (m_bDroppedCamera)
	{
		m_pCamera = pTempCamera;
	}


	// tell studiorender that we've updated the camera.
	Vector viewOrigin, viewRight, viewUp, viewPlaneNormal, viewTarget;
	m_pCamera->GetViewPoint( viewOrigin );
	m_pCamera->GetViewRight( viewRight );
	m_pCamera->GetViewUp( viewUp );
	m_pCamera->GetViewTarget( viewTarget );
	VectorSubtract( viewTarget, viewOrigin, viewPlaneNormal );
	VectorNormalize( viewPlaneNormal );
	StudioModel::UpdateViewState( viewOrigin, viewRight, viewUp, viewPlaneNormal );
}


//-----------------------------------------------------------------------------
// indicates we need to render an overlay pass...
//-----------------------------------------------------------------------------
bool CRender3DMS::NeedsOverlay() const
{
	return (m_eCurrentRenderMode == RENDER_MODE_LIGHTMAP_GRID) ||
		(m_eCurrentRenderMode == RENDER_MODE_TEXTURED);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eRenderMode - 
//-----------------------------------------------------------------------------
void CRender3DMS::SetupRenderMode(RenderMode_t eRenderMode, bool bForce)
{
	// Fix up the render mode based on whether we're picking:
	// we can't be doing wireframe if we're picking cause it won't
	// draw the whole thing.
	if (m_Pick.bPicking)
		eRenderMode = RENDER_MODE_FLAT;

	if ((m_eCurrentRenderMode == eRenderMode) && (!bForce))
	{
		return;
	}

	// Bind the appropriate material based on our mode
	switch(eRenderMode)
	{
	case RENDER_MODE_LIGHTMAP_GRID:
		assert( m_pLightmapGrid[m_DecalMode] );
		MaterialSystemInterface()->Bind( m_pLightmapGrid[m_DecalMode] );
		break;

	case RENDER_MODE_SELECTION_OVERLAY:
		// Ensures we get red even for decals
		assert( m_pSelectionOverlay[m_DecalMode] );
		MaterialSystemInterface()->Bind( m_pSelectionOverlay[m_DecalMode] );
		break;

	case RENDER_MODE_TEXTURED:
		assert( m_pFlat[m_DecalMode] );
		if (m_pBoundMaterial)
			MaterialSystemInterface()->Bind( m_pBoundMaterial );
		else
			MaterialSystemInterface()->Bind( m_pFlat[m_DecalMode] );
		break;

	case RENDER_MODE_FLAT:
		assert( m_pFlat[m_DecalMode] );
		MaterialSystemInterface()->Bind( m_pFlat[m_DecalMode] );
		break;
	
	case RENDER_MODE_TRANSLUCENT_FLAT:
		assert( m_pTranslucentFlat[m_DecalMode] );
		MaterialSystemInterface()->Bind( m_pTranslucentFlat[m_DecalMode] );
		break;
		
	case RENDER_MODE_WIREFRAME:
		assert( m_pWireframe[m_DecalMode] );
		MaterialSystemInterface()->Bind( m_pWireframe[m_DecalMode] );
		break;

	case RENDER_MODE_SMOOTHING_GROUP:
		assert( m_pFlat[m_DecalMode] );
		MaterialSystemInterface()->Bind( m_pFlat[m_DecalMode] );
		break;
	}

	m_eCurrentRenderMode = eRenderMode;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRender3DMS::ShutDown(void)
{
	MaterialSystemInterface()->RemoveView( m_WinData.hWnd );

	if (m_WinData.hDC)
	{
		m_WinData.hDC = NULL;
	}
	
	if (m_WinData.bFullScreen)
	{
		ChangeDisplaySettings(NULL, 0);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Uncaches all cached textures
//-----------------------------------------------------------------------------
void CRender3DMS::UncacheAllTextures()
{
}


//-----------------------------------------------------------------------------
// Purpose: Enables and disables various rendering parameters.
// Input  : eRenderState - Parameter to enable or disable. See RenderState_t.
//			bEnable - true to enable, false to disable the specified render state.
//-----------------------------------------------------------------------------
void CRender3DMS::RenderEnable(RenderState_t eRenderState, bool bEnable)
{
	switch (eRenderState)
	{
		case RENDER_POLYGON_OFFSET_FILL:
		{
			m_DecalMode = bEnable;
			SetupRenderMode( m_eCurrentRenderMode, true );
		}
		break;

		case RENDER_POLYGON_OFFSET_LINE:
		{
			assert(0);
			/* FIXME:
			Think we'll need to have two versions of the wireframe material
			one which ztests with offset + culling, the other which doesn't
			ztest, doesn't offect, and doesn't cull??!?

			m_pWireframeIgnoreZ->SetIntValue( bEnable );
			m_pWireframe->GetMaterial()->InitializeStateSnapshots();
			/*
			if (bEnable)
			{
				glEnable(GL_POLYGON_OFFSET_LINE);
				glPolygonOffset(-1, -1);
			}
			else
			{
				glDisable(GL_POLYGON_OFFSET_LINE);
			}
			*/
			break;
		}

		case RENDER_CENTER_CROSSHAIR:
		{
			m_RenderState.bCenterCrosshair = bEnable;
			break;
		}

		case RENDER_FRAME_RECT:
		{
			m_RenderState.bDrawFrameRect = bEnable;
			break;
		}

		case RENDER_GRID:
		{
			m_RenderState.bDrawGrid = bEnable;
			break;
		}

		case RENDER_FILTER_TEXTURES:
		{
			m_RenderState.bFilterTextures = bEnable;
			break;
		}

		case RENDER_REVERSE_SELECTION:
		{
			m_RenderState.bReverseSelection = bEnable;
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Client - 
//			Screen - 
//-----------------------------------------------------------------------------
void CRender3DMS::ScreenToClient(Vector2D &Client, const Vector2D &Screen)
{
	Client[0] = Screen[0];
	Client[1] = Screen[1];
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Screen - 
//			Client - 
//-----------------------------------------------------------------------------
void CRender3DMS::ClientToScreen(Vector2D &Screen, const Vector2D &Client)
{
	Screen[0] = Client[0];
	Screen[1] = Client[1];
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Screen - 
//			World - 
//-----------------------------------------------------------------------------
void CRender3DMS::WorldToScreen(Vector2D &Screen, const Vector &World)
{
	// FIXME: Should this computation move into the material system?
	matrix4_t ViewMatrix;
	matrix4_t ProjectionMatrix;
	matrix4_t ViewProjMatrix;
	int viewX, viewY, viewWidth, viewHeight;

	MaterialSystemInterface()->GetMatrix(MATERIAL_VIEW, &ViewMatrix[0][0]);
	MaterialSystemInterface()->GetMatrix(MATERIAL_PROJECTION, &ProjectionMatrix[0][0]);
	MaterialSystemInterface()->GetViewport( viewX, viewY, viewWidth, viewHeight );

	TransposeMatrix( ViewMatrix );
	TransposeMatrix( ProjectionMatrix );

	MultiplyMatrix( ProjectionMatrix, ViewMatrix, ViewProjMatrix);	

	Vector ProjPoint;
	ProjectPoint( ProjPoint, World, ViewProjMatrix );

	// NOTE: The negative sign on y is because wc wants to think about screen
	// coordinates in a different way than the material system
	Screen[0] = 0.5 * (ProjPoint[0] + 1.0) * viewWidth + viewX;
	Screen[1] = 0.5 * (-ProjPoint[1] + 1.0) * viewHeight + viewY;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : World - 
//			Screen - 
//-----------------------------------------------------------------------------
void CRender3DMS::ScreenToWorld(Vector &World, const Vector2D &Screen)
{
	// FIXME: Should this computation move into the material system?
	// FIXME: Haven't debugged this yet

	matrix4_t ViewMatrix;
	matrix4_t ProjectionMatrix;
	matrix4_t ViewProjInvMatrix;
	int viewX, viewY, viewWidth, viewHeight;
	Vector temp;

	MaterialSystemInterface()->GetMatrix(MATERIAL_VIEW, &ViewMatrix[0][0]);
	MaterialSystemInterface()->GetMatrix(MATERIAL_PROJECTION, &ProjectionMatrix[0][0]);
	MaterialSystemInterface()->GetViewport( viewX, viewY, viewWidth, viewHeight );

	TransposeMatrix( ViewMatrix );
	TransposeMatrix( ProjectionMatrix );

	MultiplyMatrix( ProjectionMatrix, ViewMatrix, ViewProjInvMatrix);	
	MatrixInvert( ViewProjInvMatrix );

	temp[0] = 2.0 * (Screen[0] - viewX) / viewWidth - 1;
	temp[1] = -2.0 * (Screen[1] - viewY) / viewHeight + 1;
	temp[2] = 0.0;

	ProjectPoint( World, temp, ViewProjInvMatrix );
}


//-----------------------------------------------------------------------------
// Purpose: Groovy little debug hook; can be whatever I want or need.
// Input  : pData - 
//-----------------------------------------------------------------------------
void CRender3DMS::DebugHook1(void *pData)
{
	g_bShowStatistics = !g_bShowStatistics;

#ifdef _DEBUG
	m_bRecomputeFrustumRenderGeometry = true;
	m_bRenderFrustum = true;
#endif

	//if (!m_bDroppedCamera)
	//{
	//	*m_pDropCamera = *m_pCamera;
	//	m_bDroppedCamera = true;
	//}
	//else
	//{
	//	m_bDroppedCamera = false;
	//}
}


//-----------------------------------------------------------------------------
// Purpose: Another groovy little debug hook; can be whatever I want or need.
// Input  : pData - 
//-----------------------------------------------------------------------------
void CRender3DMS::DebugHook2(void *pData)
{
	g_bRenderCullBoxes = !g_bRenderCullBoxes;
}

