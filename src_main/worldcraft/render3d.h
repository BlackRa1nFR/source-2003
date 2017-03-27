//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef RENDER3D_H
#define RENDER3D_H
#pragma once


#include "Worldcraft_MathLib.h"


enum SelectionState_t;	// HACK: for RenderBox


class CSSolid;


//
// Colors for selected faces and edges. Kinda hacky; should probably be elsewhere.
//
#define SELECT_FACE_RED			220
#define SELECT_FACE_GREEN		0
#define SELECT_FACE_BLUE		0

#define SELECT_EDGE_RED			255
#define SELECT_EDGE_GREEN		255
#define SELECT_EDGE_BLUE		0


inline void SelectFaceColor( unsigned char* pColor )
{
	pColor[0] = SELECT_FACE_RED;
	pColor[1] = SELECT_FACE_GREEN;
	pColor[2] = SELECT_FACE_BLUE;
}

inline void SelectEdgeColor( unsigned char* pColor )
{
	pColor[0] = SELECT_EDGE_RED;
	pColor[1] = SELECT_EDGE_GREEN;
	pColor[2] = SELECT_EDGE_BLUE;
}

class BoundBox;
class CCamera;
class CMapDoc;
class CMapAtom;
class CMapDisp;
class IEditorTexture;


enum RenderState_t
{
	RENDER_CENTER_CROSSHAIR,		// Whether to draw the crosshair in the center of the view.
	RENDER_FRAME_RECT,				// Whether to draw a rectangle around the inside of the view.
	RENDER_GRID,					// Whether to draw a projected grid onto solid faces.
	RENDER_FILTER_TEXTURES,			// Whether to filter textures.
	RENDER_POLYGON_OFFSET_FILL,		// Whether to offset filled polygons (for decals)
	RENDER_POLYGON_OFFSET_LINE,		// Whether to offset line polygons (for wireframe selections)
	RENDER_REVERSE_SELECTION,		// Driver issue fix - whether to return the largest (rather than smallest) Z value when picking
};


enum RenderMode_t
{
	RENDER_MODE_NONE = 0,
	RENDER_MODE_DEFAULT,
	RENDER_MODE_WIREFRAME,
	RENDER_MODE_FLAT,
	RENDER_MODE_TRANSLUCENT_FLAT,
	RENDER_MODE_TEXTURED,
	RENDER_MODE_LIGHTMAP_GRID,
	RENDER_MODE_SELECTION_OVERLAY,
	RENDER_MODE_SMOOTHING_GROUP,
};


enum Render3DType_t
{
	Render3DTypeOpenGL = 0,
	Render3DTypeMaterialSystem,
};


enum ProjectionMode_t
{
	PROJECTION_ORTHOGRAPHIC = 0,
	PROJECTION_PERSPECTIVE,
};


enum BoxType_t
{
	BoxType_WireFrame = 0,
	BoxType_Solid,
};


//-----------------------------------------------------------------------------
// Structure used for returning hits when calling ObjectsAt.
//-----------------------------------------------------------------------------
typedef struct
{
	CMapAtom *pObject;		// Pointer to the CMapAtom that was clicked on.
	unsigned int uData;		// Additional data provided by the CMapAtom object.
	unsigned int nDepth;	// Depth value of the object that was clicked on.
} RenderHitInfo_t;


class CRender3D
{
	public:

		virtual ~CRender3D(void) { };

		virtual void BindTexture(IEditorTexture *pTexture) = 0;

		virtual	bool Initialize(HWND hwnd, CMapDoc *pDoc) = 0;
		virtual	void ShutDown(void) = 0;

		virtual	RenderMode_t GetCurrentRenderMode(void) = 0;
		virtual	RenderMode_t GetDefaultRenderMode(void) = 0;
		virtual float GetElapsedTime(void) = 0;
		virtual float GetGridDistance(void) = 0;
		virtual float GetGridSize(void) = 0;
		virtual void GetViewPoint(Vector& pfViewPoint) = 0;
		virtual void GetViewForward(Vector& pfViewForward) = 0;
		virtual void GetViewUp(Vector& pfViewUp) = 0;
		virtual void GetViewRight(Vector& pfViewRight) = 0;
		virtual float GetRenderFrame(void) = 0;

		virtual void UncacheAllTextures() = 0;

		virtual bool IsEnabled(RenderState_t eRenderState) = 0;
		virtual bool IsPicking(void) = 0;

		virtual float LightPlane(Vector& Normal) = 0;

		virtual int ObjectsAt(float x, float y, float fWidth, float fHeight, RenderHitInfo_t *pObjects, int nMaxObjects) = 0;

		virtual bool IsInLightingPreview() = 0;
		virtual void SetInLightingPreview( bool bLightingPreview ) = 0;

		virtual	void SetCamera(CCamera *pCamera) = 0;
		virtual	void SetDefaultRenderMode(RenderMode_t eRenderMode) = 0;
		virtual	void SetRenderMode(RenderMode_t eRenderMode, bool force = false) = 0;
		virtual	bool SetSize(int nWidth, int nHeight) = 0;
		virtual void SetProjectionMode( ProjectionMode_t eProjMode ) = 0;

        virtual void PreRender( void ) = 0;
        virtual void PostRender( void ) = 0;

		virtual void Render(void) = 0;
        virtual void RenderClear( void ) = 0;
		virtual void RenderEnable(RenderState_t eRenderState, bool bEnable) = 0;
		virtual void RenderWireframeBox(const Vector &Mins, const Vector &Maxs, unsigned char chRed, unsigned char chGreen, unsigned char chBlue) = 0;
		virtual void RenderBox(const Vector &Mins, const Vector &Maxs, unsigned char chRed, unsigned char chGreen, unsigned char chBlue, SelectionState_t eBoxSelectionState) = 0;
		virtual void RenderArrow(Vector const &vStartPt, Vector const &vEndPt, unsigned char chRed, unsigned char chGreen, unsigned char chBlue) = 0;
		virtual void RenderCone(Vector const &vBasePt, Vector const &vTipPt, float fRadius, int nSlices,
		                        unsigned char chRed, unsigned char chGreen, unsigned char chBlue ) = 0;
		virtual void RenderSphere(Vector const &vCenter, float flRadius, int nTheta, int nPhi, unsigned char chRed, unsigned char chGreen, unsigned char chBlue) = 0;
		virtual void RenderWireframeSphere(Vector const &vCenter, float flRadius, int nTheta, int nPhi, unsigned char chRed, unsigned char chGreen, unsigned char chBlue) = 0;
        virtual void RenderDisplacement( CMapDisp *pMapDisp ) = 0;
		virtual void RenderLine(const Vector &vec1, const Vector &vec2, unsigned char r, unsigned char g, unsigned char b) = 0;

        virtual void SelectDisplacement( CMapDisp *pMapDisp, float x, float y, float width, float height, 
                                         RenderHitInfo_t *pObjects, int maxObjects ) = 0;

		virtual void WorldToScreen(Vector2D &Screen, const Vector &World) = 0;
		virtual void ScreenToWorld(Vector &World, const Vector2D &Screen) = 0;

		virtual void ScreenToClient(Vector2D &Client, const Vector2D &Screen) = 0;
		virtual void ClientToScreen(Vector2D &Screen, const Vector2D &Client) = 0;

		virtual void BeginRenderHitTarget(CMapAtom *pObject, unsigned int uHandle = 0) = 0;
		virtual void EndRenderHitTarget(void) = 0;

		virtual void BeginParallel(void) = 0;
		virtual void EndParallel(void) = 0;

		// HACK: This is a fixup for bizarre MFC behavior with the drop-down boxes
		virtual void ResetFocus() = 0;

		// indicates we need to render an overlay pass...
		virtual bool NeedsOverlay() const = 0;

		// should I sort renders?
		virtual bool DeferRendering() const = 0;
		
		virtual void DebugHook1(void *pData = NULL) = 0;
		virtual void DebugHook2(void *pData = NULL) = 0;
};


CRender3D *CreateRender3D(Render3DType_t eRender3DType);

#endif // RENDER3D_H
