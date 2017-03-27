//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPVIEW3D_H
#define MAPVIEW3D_H
#ifdef _WIN32
#pragma once
#endif

#include "Keyboard.h"
#include "MapView.h"
#include "Render3D.h"


class Axes2;
class CMapAtom;
class CRender3D;
class CCamera;
class CTitleWnd;
class CMapDecal;


class CMapView3D : public CMapView
{
	protected:

		CMapView3D();
		DECLARE_DYNCREATE(CMapView3D)

	public:

		CMapDoc *GetDocument(void);

	public:

		enum
		{
			updNothing = 0x00,
			updMorphOnly = 0x01,
			updAll = 0x02,
			updRedrawNow = 0x04
		};

		void Activate(BOOL bActivate);

		void Render(void);
		
		void SetCamera(const Vector &vecPos, const Vector &vecLookAt);
		const CCamera *GetCamera() { return m_pCamera; }

		inline void ClientToScreen(Vector2D &Screen, const Vector2D &Client);
		inline void WorldToScreen(Vector2D &Screen, const Vector &World);
		inline void WorldToClient(Vector2D &Client, const Vector &World);
		inline void ScreenToWorld(Vector &World, const Vector2D &Screen);
		void GetHitPos(CPoint point, PLANE &plane, Vector &pos);
		void CalcBestAxes(Axes2 &Axes);
		void ProcessInput(void);

		void UpdateStatusBar();

		// Called by the camera tool to control the camera.
		void EnableMouseLook(bool bEnable);
		void EnableRotating(bool bEnable);
		void EnableStrafing(bool bEnable);
		void UpdateCameraVariables(void);

		void MoveForward(float flDistance);
		void MoveUp(float flDistance);
		void MoveRight(float flDistance);
		void Pitch(float flDegrees);
		void Yaw(float flDegrees);

		void BeginPick(void);
		void EndPick(void);

		DrawType_t GetDrawType() { return m_eDrawType; }
		void SetDrawType(DrawType_t eDrawType);

		int ObjectsAt(float x, float y, float fWidth, float fHeight, RenderHitInfo_t *pObjects, int nMaxObjects);
		CMapClass *NearestObjectAt(CPoint point, ULONG &ulFace);
		void BuildRay(CPoint point, Vector &start, Vector &end);

		void RenderPreloadObject(CMapAtom *pObject);

		// Release all video memory
		void ReleaseVideoMemory();

		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CMapView3D)
		public:
		virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
		virtual BOOL DestroyWindow();
		virtual void OnInitialUpdate();
		protected:
		virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
		virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
		virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
		virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
		virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
		//}}AFX_VIRTUAL

	public:
		virtual ~CMapView3D();
		#ifdef _DEBUG
		virtual void AssertValid() const;
		virtual void Dump(CDumpContext& dc) const;
		#endif

	private:

		void EnableCrosshair(bool bEnable);
	
		bool ControlCamera(void);

		//
		// Keyboard processing.
		//
		void InitializeKeyMap(void);
		void ProcessMouse(void);
		void ProcessKeys(float fElapsedTime);
		void ProcessMovementKeys(float fElapsedTime);
		float GetKeyScale(unsigned int uKeyState);

		enum
		{
			MVTIMER_PICKNEXT = 0
		};

		bool m_bMouseLook;				// Set to true when we override the mouse processing to use mouselook.
		bool m_bStrafing;
		bool m_bRotating;
		CPoint m_ptLastMouseMovement;	// Last position used for tracking the mouse for camera control.

		DWORD m_dwTimeLastInputSample;	// Used for framerate-independent input processing.

		float m_fForwardSpeed;			// Current forward speed, in world units per second.
		float m_fStrafeSpeed;			// Current side-to-side speed, in world units per second.
		float m_fVerticalSpeed;			// Current up-down speed, in world units per second.

		float m_fForwardSpeedMax;		// Maximum forward speed, in world units per second.
		float m_fStrafeSpeedMax;		// Maximum side-to-side speed, in world units per second.
		float m_fVerticalSpeedMax;		// Maximum up-down speed, in world units per second.

		float m_fForwardAcceleration;	// Forward acceleration, in world units per second squared.
		float m_fStrafeAcceleration;	// Side-to-side acceleration, in world units per second squared.
		float m_fVerticalAcceleration;	// Up-down acceleration, in world units per second squared.

		DrawType_t m_eDrawType;			// How we render - wireframe, flat, textured, lightmap grid, or lighting preview.
		bool m_bLightingPreview;

		void PickNextObject(void);

		CTitleWnd *m_pwndTitle;	// Title window.

		CRender3D *m_pRender;	// Performs the 3D rendering in our window.
		CCamera *m_pCamera;		// Defines the camera position and settings for this view.
		CKeyboard m_Keyboard;	// Handles binding of keys and mouse buttons to logical functions.

		//{{AFX_MSG(CMapView3D)
	protected:
		afx_msg void OnTimer(UINT nIDEvent);
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
		afx_msg void OnRButtonDown( UINT nFlags, CPoint point );
		afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint point);
		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnSetFocus(CWnd* pOldWnd);
		afx_msg void OnDraw(CDC *pDC);
		afx_msg BOOL OnFaceAlign(UINT uCmd);
		afx_msg BOOL OnFaceJustify(UINT uCmd);
		afx_msg void OnView3dWireframe(void);
		afx_msg void OnView3dPolygon(void);
		afx_msg void OnView3dTextured(void);
		afx_msg void OnView3dLightmapGrid(void);
		afx_msg void OnView3dLightingPreview(void);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnSysCommand(UINT uID, LPARAM lParam);
		afx_msg void OnKillFocus(CWnd *pNewWnd);
		afx_msg void OnNcPaint( );
		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()
};


#ifndef _DEBUG
inline CMapDoc *CMapView3D::GetDocument(void)
{
	return((CMapDoc*)m_pDocument);
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Coordinate system conversion functions.
//-----------------------------------------------------------------------------
void CMapView3D::ClientToScreen(Vector2D &Screen, const Vector2D &Client)
{
	if (m_pRender)
	{
		m_pRender->ClientToScreen(Screen, Client);
	}
}


void CMapView3D::WorldToScreen(Vector2D &Screen, const Vector &World)
{
	if (m_pRender)
	{
		m_pRender->WorldToScreen(Screen, World);
	}
}


void CMapView3D::WorldToClient(Vector2D &Client, const Vector &World)
{
	if (m_pRender)
	{
		Vector2D vecScreen;
		m_pRender->WorldToScreen(vecScreen, World);
		m_pRender->ScreenToClient(Client, vecScreen);
	}
}


void CMapView3D::ScreenToWorld(Vector &World, const Vector2D &Screen)
{
	if (m_pRender)
	{
		m_pRender->ScreenToWorld(World, Screen);
	}
}


#endif // MAPVIEW3D_H
