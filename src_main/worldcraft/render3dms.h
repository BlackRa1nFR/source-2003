//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef RENDER3DMS_H
#define RENDER3DMS_H
#pragma once


#include "Render3D.h"
#include "Vector4D.h"
#include "UtlVector.h"

//
// Size of the buffer used for picking. See glSelectBuffer for documention on
// the contents of the selection buffer.
//
#define SELECTION_BUFFER_SIZE	50

//
// Size of the texture cache. THis is the maximum number of unique textures that
// a map can refer to and still render properly in Worldcraft.
//
#define TEXTURE_CACHE_SIZE		2048

//
// Maximum number of objects that can be kept in the list of objects to render last.
//
#define MAX_RENDER_LAST_OBJECTS	256

//
// Maximum number of hits that can be returned by ObjectsAt.
//
#define MAX_PICK_HITS			512


class BoundBox;
class CCamera;
class CCullTreeNode;
class CMapClass;
class CMapDoc;
class CMapWorld;
class IEditorTexture;
class IMaterial;
class IMaterialVar;
class CUtlVector;

enum Visibility_t;


typedef CUtlVector<CMapAtom *> CMapAtomVector;


//
// Render state information set via RenderEnable:
//
typedef struct
{
	bool bCenterCrosshair;	// Whether to render the center crosshair.
	bool bDrawFrameRect;	// Whether to render a rectangle around the view.
	bool bDrawGrid;			// Whether to render the grid.
	float fGridSpacing;		// Grid spacing in world units.
	float fGridDistance;	// Maximum distance from camera to draw grid.
	bool bFilterTextures;	// Whether to filter textures.
	bool bReverseSelection;	// Driver issue fix - whether to return the largest (rather than smallest) Z value when picking
} RenderStateInfo_t;


//
// Picking state information used when called from ObjectsAt.
//
typedef struct
{
	bool bPicking;							// Whether we are rendering in pick mode or not.

	float fX;								// Leftmost coordinate of pick rectangle, passed in by caller.
	float fY;								// Topmost coordinate of pick rectangle, passed in by caller.
	float fWidth;							// Width of pick rectangle, passed in by caller.
	float fHeight;							// Height of pick rectangle, passed in by caller.

	RenderHitInfo_t *pHitsDest;				// Final array in which to place pick hits, passed in by caller.
	int nMaxHits;							// Maximum number of hits to place in the 'pHits' array, passed in by caller, must be <= MAX_PICK_HITS.

	RenderHitInfo_t Hits[MAX_PICK_HITS];	// Temporary array in which to place unsorted pick hits.
	int nNumHits;							// Number of hits so far in this pick (number of hits in 'Hits' array).

	unsigned int uSelectionBuffer[SELECTION_BUFFER_SIZE];
	unsigned int uLastZ;
} PickInfo_t;


typedef struct
{
	IEditorTexture *pTexture;		// Pointer to the texture object that implements this texture.
	int nTextureID;			// Unique ID of this texture across all renderers.
	unsigned int uTexture;	// The texture name as returned by OpenGL when the texture was uploaded in this renderer.
} TextureCache_t;


typedef struct
{
    HINSTANCE        hInstance;
    int              iCmdShow;
    HWND             hWnd;
	HDC				 hDC;
    bool             bActive;
    bool             bFullScreen;
    ATOM             wndclass;
    WNDPROC          wndproc;
    int              width;
    int              height;
    int              bpp;
    bool             bChangeBPP;
    bool             bAllowSoft;
    char            *szCmdLine;
    int              argc;
    char           **argv;
    int              nWidth;
    int              nHeight;
    float            gldAspect;
    float            NearClip;
    float            FarClip;
    float            fov;
    int              iResCount;
    int              iVidMode;
} MatWinData_t;


class CRender3DMS : public CRender3D
{
public:

	// Constructor / Destructor.
	CRender3DMS(void);
	virtual ~CRender3DMS(void);

	// Initialization & shutdown functions.
	bool Initialize(HWND hwnd, CMapDoc *pDoc);
	void ShutDown(void);

	RenderMode_t GetCurrentRenderMode(void);
	RenderMode_t GetDefaultRenderMode(void);
	float GetElapsedTime(void);
	float GetGridDistance(void);
	float GetGridSize(void);
	void GetViewPoint(Vector& pfViewPoint);
	void GetViewForward(Vector& pfViewForward);
	void GetViewUp(Vector& pfViewUp);
	void GetViewRight(Vector& pfViewRight);
	float GetRenderFrame(void) { return g_nRenderFrame; }

	bool DeferRendering() const { return m_DeferRendering; }
	bool IsEnabled(RenderState_t eRenderState);
	bool IsPicking(void);

	// Set functions.
	bool SetSize(int nWidth, int nHeight);

	virtual bool IsInLightingPreview();
	virtual void SetInLightingPreview( bool bLightingPreview );
	
	void SetCamera(CCamera *pCamera);
	void SetRenderMode(RenderMode_t eRenderMode, bool force = false);
	void SetDefaultRenderMode(RenderMode_t eRenderMode);
	void SetProjectionMode( ProjectionMode_t eProjMode );

	// Operations.
	void BindTexture(IEditorTexture *pTexture);
	float LightPlane(Vector& Normal);
	void UncacheAllTextures();

	void PreRender(void);
	void PostRender(void);

	void ResetFocus();

	virtual void Render(void);
	virtual void RenderClear( void );
	virtual void RenderEnable(RenderState_t eRenderState, bool bEnable);

	virtual void RenderWireframeBox(const Vector &Mins, const Vector &Maxs, unsigned char chRed, unsigned char chGreen, unsigned char chBlue);
	virtual void RenderBox(const Vector &Mins, const Vector &Maxs, unsigned char chRed, unsigned char chGreen, unsigned char chBlue, SelectionState_t eBoxSelectionState);
	virtual void RenderArrow(Vector const &vStartPt, Vector const &vEndPt, unsigned char chRed, unsigned char chGreen, unsigned char chBlue);
	virtual void RenderCone(Vector const &vBasePt, Vector const &vTipPt, float fRadius, int nSlices,
		            unsigned char chRed, unsigned char chGreen, unsigned char chBlue );
	virtual void RenderSphere(Vector const &vCenter, float flRadius, int nTheta, int nPhi,
							  unsigned char chRed, unsigned char chGreen, unsigned char chBlue );
	virtual void RenderWireframeSphere(Vector const &vCenter, float flRadius, int nTheta, int nPhi,
							            unsigned char chRed, unsigned char chGreen, unsigned char chBlue );
	virtual void RenderLine(const Vector &vec1, const Vector &vec2, unsigned char r, unsigned char g, unsigned char b);


	int ObjectsAt(float x, float y, float fWidth, float fHeight, RenderHitInfo_t *pObjects, int nMaxObjects);

	void RenderDisplacement( CMapDisp *pMapDisp );
    void SelectDisplacement( CMapDisp *pMapDisp, float x, float y, float width, float height, 
                             RenderHitInfo_t *pObjects, int maxObjects );

	void WorldToScreen(Vector2D &Screen, const Vector &World);
	void ScreenToWorld(Vector &World, const Vector2D &Screen);

	void ScreenToClient(Vector2D &Client, const Vector2D &Screen);
	void ClientToScreen(Vector2D &Screen, const Vector2D &Client);

	// Parallel projection functions.
	void BeginParallel(void);
	void EndParallel(void);

	void DebugHook1(void *pData = NULL);
	void DebugHook2(void *pData = NULL);

	// indicates we need to render an overlay pass...
	bool NeedsOverlay() const;

protected:

	inline void DispatchRender3D(CMapClass *pMapClass);

	// Perspective projection functions.
	void SetupCamera(CCamera *pCamera);
	void SetupRenderMode(RenderMode_t eRenderMode, bool bForce = false);

	// Rendering functions.
	void RenderMapClass(CMapClass *pMapClass);
	void RenderNode(CCullTreeNode *pNode, bool bForce);
	void RenderObjectList(CMapAtomVector &Objects);
	void RenderOverlayElements(void);
	void RenderTools(void);
	void RenderTree(void);
    void RenderDisplacementTexels( CMapDisp *pMapDisp, int startWidth,
                                   int startHeight, float interval,
                                   float color[3], int orientation, bool *bSelected );
    //void RenderDisplacementTexels( CMapDisp *pMapDisp, int startWidth, int startHeight, float interval, float color[3], bool *bSelected );
	void RenderPointsFile(void);
	void RenderWorldAxes();

	// Utility functions.
	void Preload(CMapClass *pParent);
	void GetFrustumPlanes(Vector4D Planes[6]);
	Visibility_t IsBoxVisible(Vector const &BoxMins, Vector const &BoxMaxs);

	// Picking functions.
	void BeginRenderHitTarget(CMapAtom *pObject, unsigned int uHandle = 0);
	void EndRenderHitTarget(void);

	// Frustum methods
	void ComputeFrustumRenderGeometry();
	void RenderFrustum();

	static int g_nRenderFrame;			// Last rendered frame. Stored as a cookie in each rendered object.
	static int g_nTextureID;			// Unique ID assigned to texture objects as they are uploaded to the driver. 

    ProjectionMode_t    m_ProjMode;     // projection mode - PERSPECTIVE or ORTHOGRAPHIC

	float m_fFrameRate;					// Framerate in frames per second, calculated once per second.
	int m_nFramesThisSample;			// Number of frames rendered in the current sample period.
	DWORD m_dwTimeLastSample;			// Time when the framerate was last calculated.

	DWORD m_dwTimeLastFrame;			// The time when the previous frame was rendered.
	float m_fTimeElapsed;				// Milliseconds elapsed since the last frame was rendered.

	CCamera *m_pCamera;					// Camera to use for perspective transformations.
	Vector4D m_FrustumPlanes[6];		// Plane normals and constants for the current view frustum.
	CMapDoc *m_pDoc;					// Document to render.
	MatWinData_t m_WinData;				// Defines our render window parameters.
	RenderMode_t m_eCurrentRenderMode;	// Current render mode setting - Wireframe, flat, or textured.
	RenderMode_t m_eDefaultRenderMode;	// Default render mode - Wireframe, flat, or textured.
	PickInfo_t m_Pick;					// Contains information used when rendering in pick mode.
	RenderStateInfo_t m_RenderState;	// Render state set via RenderEnable.

	bool m_bDroppedCamera;				// Whether we have dropped the camera for debugging.
	bool m_DeferRendering;				// Used when we want to sort lovely opaque objects
	CCamera *m_pDropCamera;				// Dropped camera to use for debugging.

	CMapAtomVector m_RenderLastObjects;		// List of objects to render after all the other objects.

	IMaterial* m_pBoundMaterial;		// The currently bound material

	IMaterial* m_pWireframe[2];			// default wireframe material
	IMaterial* m_pFlat[2];				// default flat material
	IMaterial* m_pTranslucentFlat[2];	// default translucent flat material
	IMaterial* m_pLightmapGrid[2];		// default lightmap grid material
	IMaterial* m_pSelectionOverlay[2];	// for selecting actual textures
	IMaterial* m_pVertexColor[2];		// for selecting actual textures

	bool m_DecalMode;
	bool m_bLightingPreview;

	// for debugging... render the view frustum
#ifdef _DEBUG
	Vector m_FrustumRenderPoint[8];
	bool m_bRenderFrustum;
	bool m_bRecomputeFrustumRenderGeometry;
#endif
};


#endif // RENDER3DGL_H
