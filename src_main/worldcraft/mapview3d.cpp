//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements the 3D view message handling. This class is responsible
//			for 3D camera control, activating tools in the 3D view, calling
//			into the renderer when necessary, and synchronizing the 2D camera
//			information with the 3D camera.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include <oleauto.h>
#include <oaidl.h>
#include <afxpriv.h>
#include <mmsystem.h>
#include "Camera.h"
#include "GlobalFunctions.h"
#include "Gizmo.h"
#include "History.h"
#include "Keyboard.h"
#include "MainFrm.h"
#include "MapDoc.h"
#include "MapDecal.h"
#include "MapEntity.h"
#include "MapSolid.h"
#include "MapStudioModel.h"
#include "MapWorld.h"
#include "MapView3D.h"
#include "MapView2D.h"
#include "ObjectProperties.h"
#include "ObjectBar.h"
#include "Options.h"
#include "StatusBarIDs.h"
#include "TitleWnd.h"
#include "ToolManager.h"
#include "Worldcraft.h"
#include "Vector.h"
#include "MapOverlay.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#pragma warning(disable:4244 4305)


#define CAMERA_FRONT_PLANE_DISTANCE		8.0f
#define CAMERA_HORIZONTAL_FOV			90.0f


typedef struct
{
	CMapObjectList *pList;
	POINT pt;
	CMapWorld *pWorld;
} SELECT3DINFO;


int g_nClipPoints = 0;
Vector g_ClipPoints[4];


//
// Defines the logical keys.
//
#define LOGICAL_KEY_FORWARD			0
#define LOGICAL_KEY_BACK			1
#define LOGICAL_KEY_LEFT			2
#define LOGICAL_KEY_RIGHT			3
#define LOGICAL_KEY_UP				4
#define LOGICAL_KEY_DOWN			5
#define LOGICAL_KEY_PITCH_UP		6
#define LOGICAL_KEY_PITCH_DOWN		7
#define LOGICAL_KEY_YAW_LEFT		8
#define LOGICAL_KEY_YAW_RIGHT		9

//
// Rotation speeds, in degrees per second.
//
#define YAW_SPEED					180
#define PITCH_SPEED					180
#define ROLL_SPEED					180


IMPLEMENT_DYNCREATE(CMapView3D, CMapView)


BEGIN_MESSAGE_MAP(CMapView3D, CMapView)
	//{{AFX_MSG_MAP(CMapView3D)
	ON_WM_KILLFOCUS()
	ON_WM_TIMER()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONUP()
	ON_WM_CHAR()
	ON_WM_SETFOCUS()
	ON_WM_NCPAINT()
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_COMMAND(ID_VIEW_3DWIREFRAME, OnView3dWireframe)
	ON_COMMAND(ID_VIEW_3DPOLYGON, OnView3dPolygon)
	ON_COMMAND(ID_VIEW_3DTEXTURED, OnView3dTextured)
	ON_COMMAND(ID_VIEW_3DLIGHTMAP_GRID, OnView3dLightmapGrid)
	ON_COMMAND(ID_VIEW_LIGHTINGPREVIEW, OnView3dLightingPreview)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Constructor. Initializes data members to default values.
//-----------------------------------------------------------------------------
CMapView3D::CMapView3D(void)
{
	m_eDrawType = VIEW3D_WIREFRAME;
	m_pRender = NULL;
	m_pCamera = NULL;

	m_dwTimeLastInputSample = 0;

	m_fForwardSpeed = 0;
	m_fStrafeSpeed = 0;
	m_fVerticalSpeed = 0;

	m_pwndTitle = NULL;
	m_bLightingPreview = false;

	m_bMouseLook = false;
	m_bStrafing = false;
	m_bRotating = false;

	m_ptLastMouseMovement.x = 0;
	m_ptLastMouseMovement.y = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Releases dynamically allocated resources.
//-----------------------------------------------------------------------------
CMapView3D::~CMapView3D(void)
{
	if (m_pCamera != NULL)
	{
		delete m_pCamera;
	}

	if (m_pRender != NULL)
	{
		m_pRender->ShutDown();
		delete m_pRender;
	}

	if (m_pwndTitle != NULL)
	{
		delete m_pwndTitle;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Resets the keyboard state for this view. This prevents bad behavior
//			when moving the mouse between views while holding keys down.
// Input  : bActivate - Whether this view is becoming active or inactive.
//-----------------------------------------------------------------------------
void CMapView3D::Activate(BOOL bActivate)
{
	m_Keyboard.ClearKeyStates();

	//
	// Reset the last input sample time.
	//
	m_dwTimeLastInputSample = 0;

	CMapView::Activate(bActivate);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cs - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CMapView3D::PreCreateWindow(CREATESTRUCT& cs)
{
	static CString className;
	
	if(className.IsEmpty())
	{
		//
		// We need the CS_OWNDC bit so that we don't need to call GetDC every time we render. That fixes the flicker under Win98.
		//
		className = AfxRegisterWndClass(CS_BYTEALIGNCLIENT | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC, NULL, HBRUSH(GetStockObject(BLACK_BRUSH)));
	}

	cs.lpszClass = className;

	return CView::PreCreateWindow(cs);
}


//-----------------------------------------------------------------------------
// Purpose: Disables mouselook when the view loses focus. This ensures that the
//			cursor is shown and not locked in the center of the 3D view.
// Input  : pNewWnd - The window getting focus.
//-----------------------------------------------------------------------------
void CMapView3D::OnKillFocus(CWnd *pNewWnd)
{
	EnableMouseLook(false);
	EnableRotating(false);
	EnableStrafing(false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nDrawType - 
//-----------------------------------------------------------------------------
void CMapView3D::SetDrawType(DrawType_t eDrawType)
{
	RenderMode_t eRenderMode;

	// Turn off the dialog.
	if ( m_eDrawType == VIEW3D_SMOOTHING_GROUP )
	{
		CMainFrame *pMainFrame = GetMainWnd();
		if ( pMainFrame )
		{		
			CFaceSmoothingVisualDlg *pSmoothDlg = pMainFrame->GetSmoothingGroupDialog();
			pSmoothDlg->ShowWindow( SW_HIDE );
		}
	}

	m_bLightingPreview = false;
	switch (eDrawType)
	{
		case VIEW3D_WIREFRAME:
		{
			eRenderMode = RENDER_MODE_WIREFRAME;
			break;
		}

		case VIEW3D_POLYGON:
		{
			eRenderMode = RENDER_MODE_FLAT;
			break;
		}

		case VIEW3D_TEXTURED:
		{
			eRenderMode = RENDER_MODE_TEXTURED;
			break;
		}

		case VIEW3D_LIGHTMAP_GRID:
		{
			eRenderMode = RENDER_MODE_LIGHTMAP_GRID;
			break;
		}

		case VIEW3D_LIGHTING_PREVIEW:
		{
			// Only enter lighting preview if the doc has lighting data.
			CMapDoc *pDoc = GetDocument();
			m_bLightingPreview = ( pDoc && pDoc->GetBSPLighting() );
			
			eRenderMode = RENDER_MODE_TEXTURED;
			break;
		}

		case VIEW3D_SMOOTHING_GROUP:
		{
			CMainFrame *pMainFrame = GetMainWnd();
			if ( pMainFrame )
			{		
				CFaceSmoothingVisualDlg *pSmoothDlg = pMainFrame->GetSmoothingGroupDialog();
				pSmoothDlg->ShowWindow( SW_SHOW );
			}

			// Always set the initial group to visualize (zero).
			CMapDoc *pDoc = GetDocument();
			pDoc->SetSmoothingGroupVisual( 0 );

			eRenderMode = RENDER_MODE_SMOOTHING_GROUP;
			break;
		}

		default:
		{
			ASSERT(FALSE);
			eDrawType = VIEW3D_WIREFRAME;
			eRenderMode = RENDER_MODE_WIREFRAME;
			break;
		}
	}

	m_eDrawType = eDrawType;

	//
	// Set renderer to use the new rendering mode.
	//
	if (m_pRender != NULL)
	{
		m_pRender->SetDefaultRenderMode(eRenderMode);
		m_pRender->SetInLightingPreview( m_bLightingPreview );

		// Somehow, this drop down box screws up MFC's notion
		// of what we're supposed to be updating. This is a workaround.
		m_pRender->ResetFocus();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the position and direction of the camera for this view.
//-----------------------------------------------------------------------------
void CMapView3D::SetCamera(const Vector &vecPos, const Vector &vecLookAt)
{
	m_pCamera->SetViewPoint(vecPos);
	m_pCamera->SetViewTarget(vecLookAt);
}


//-----------------------------------------------------------------------------
// Purpose: Pick next object (called by OnTimer())
//-----------------------------------------------------------------------------
void CMapView3D::PickNextObject(void)
{
	CMapDoc *pDoc = GetDocument();

	// set current document hit
	pDoc->SetCurHit(CMapDoc::hitNext);
}


//-----------------------------------------------------------------------------
// Purpose: Prepares to print.
// Input  : Per CView::OnPreparePrinting.
// Output : Returns nonzero to begin printing, zero to cancel printing.
//-----------------------------------------------------------------------------
BOOL CMapView3D::OnPreparePrinting(CPrintInfo* pInfo)
{
	return(DoPreparePrinting(pInfo));
}


//-----------------------------------------------------------------------------
// Purpose: Begins printing.
// Input  : Per CView::OnBeginPrinting.
//-----------------------------------------------------------------------------
void CMapView3D::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}


//-----------------------------------------------------------------------------
// Purpose: Cleans up after printing.
// Input  : Per CView::OnEndPrinting.
//-----------------------------------------------------------------------------
void CMapView3D::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}


//-----------------------------------------------------------------------------
// Purpose: Debugging functions.
//-----------------------------------------------------------------------------
#ifdef _DEBUG
void CMapView3D::AssertValid() const
{
	CView::AssertValid();
}

void CMapView3D::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMapDoc* CMapView3D::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMapDoc)));
	return (CMapDoc*)m_pDocument;
}
#endif //_DEBUG
 

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIDEvent - 
//-----------------------------------------------------------------------------
void CMapView3D::OnTimer(UINT nIDEvent) 
{
	static BOOL bPicking = FALSE;

	switch (nIDEvent)
	{
		case MVTIMER_PICKNEXT:
		{
			if (bPicking)
			{
				return;
			}

			bPicking = TRUE;
			PickNextObject();
			bPicking = FALSE;

			//
			// Force an update of the 3D views.
			//
			CMapDoc *pDoc = GetDocument();
			if (pDoc != NULL)
			{
				pDoc->Update3DViews();
			}
			break;
		}
	}

	CView::OnTimer(nIDEvent);
}


//-----------------------------------------------------------------------------
// Purpose: Called just before we are destroyed.
//-----------------------------------------------------------------------------
BOOL CMapView3D::DestroyWindow() 
{
	KillTimer(MVTIMER_PICKNEXT);
	return CView::DestroyWindow();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapView3D::UpdateStatusBar(void)
{
	if (!IsWindow(m_hWnd))
	{
		return;
	}

	SetStatusText(SBI_GRIDZOOM, "");
	SetStatusText(SBI_COORDS, "");
}


//-----------------------------------------------------------------------------
// Purpose: Sets up key bindings for the 3D view.
//-----------------------------------------------------------------------------
void CMapView3D::InitializeKeyMap(void)
{
	m_Keyboard.RemoveAllKeyMaps();

	if (!Options.view2d.bNudge)
	{
		m_Keyboard.AddKeyMap(VK_LEFT, 0, LOGICAL_KEY_YAW_LEFT);
		m_Keyboard.AddKeyMap(VK_RIGHT, 0, LOGICAL_KEY_YAW_RIGHT);
		m_Keyboard.AddKeyMap(VK_DOWN, 0, LOGICAL_KEY_PITCH_DOWN);
		m_Keyboard.AddKeyMap(VK_UP, 0, LOGICAL_KEY_PITCH_UP);

		m_Keyboard.AddKeyMap(VK_LEFT, KEY_MOD_SHIFT, LOGICAL_KEY_LEFT);
		m_Keyboard.AddKeyMap(VK_RIGHT, KEY_MOD_SHIFT, LOGICAL_KEY_RIGHT);
		m_Keyboard.AddKeyMap(VK_DOWN, KEY_MOD_SHIFT, LOGICAL_KEY_DOWN);
		m_Keyboard.AddKeyMap(VK_UP, KEY_MOD_SHIFT, LOGICAL_KEY_UP);
	}

	if (Options.view3d.bUseMouseLook)
	{
		m_Keyboard.AddKeyMap('W', 0, LOGICAL_KEY_FORWARD);
		m_Keyboard.AddKeyMap('A', 0, LOGICAL_KEY_LEFT);
		m_Keyboard.AddKeyMap('D', 0, LOGICAL_KEY_RIGHT);
		m_Keyboard.AddKeyMap('S', 0, LOGICAL_KEY_BACK);
	}
	else
	{
		m_Keyboard.AddKeyMap('D', 0, LOGICAL_KEY_FORWARD);
		m_Keyboard.AddKeyMap('C', 0, LOGICAL_KEY_BACK);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pWnd - 
//			point - 
//-----------------------------------------------------------------------------
void CMapView3D::OnContextMenu(CWnd *pWnd, CPoint point)
{
    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		if (pTool->OnContextMenu3D(this, point))
		{
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles the key down event.
// Input  : Per CWnd::OnKeyDown.
//-----------------------------------------------------------------------------
void CMapView3D::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CMapDoc *pDoc = GetDocument();
	if (pDoc == NULL)
	{
		return;
	}

	//
	// 'z' toggles mouselook.
	//
	if (((char)tolower(nChar) == 'z') && !(nFlags & 0x4000) && (Options.view3d.bUseMouseLook))
	{
		CMapDoc *pDoc = GetDocument();
		if (pDoc != NULL)
		{
			EnableMouseLook(!m_bMouseLook);

			//
			// If we just stopped mouse looking, update the camera variables.
			//
			if (!m_bMouseLook)
			{
				UpdateCameraVariables();
			}
		}
		
		return;
	}

    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		if (pTool->OnKeyDown3D(this, nChar, nRepCnt, nFlags))
		{
			return;
		}
	}

	m_Keyboard.OnKeyDown(nChar, nRepCnt, nFlags);

	switch (nChar)
	{
		case VK_DELETE:
		{
			pDoc->OnCmdMsg(ID_EDIT_DELETE, CN_COMMAND, NULL, NULL);
			break;
		}

		case VK_NEXT:
		{
			pDoc->OnCmdMsg(ID_EDIT_SELNEXT, CN_COMMAND, NULL, NULL);
			break;
		}

		case VK_PRIOR:
		{
			pDoc->OnCmdMsg(ID_EDIT_SELPREV, CN_COMMAND, NULL, NULL);
			break;
		}

		//
		// Move the back clipping plane closer in.
		//
		case '1':
		{
			float fBack = m_pCamera->GetFarClip();
			if (fBack >= 2000)
			{
				m_pCamera->SetFarClip(fBack - 1000);
				Options.view3d.iBackPlane = fBack;
			}
			else if (fBack > 500)
			{
				m_pCamera->SetFarClip(fBack - 250);
				Options.view3d.iBackPlane = fBack;
			}
			break;
		}

		//
		// Move the back clipping plane farther away.
		//
		case '2':
		{
			float fBack = m_pCamera->GetFarClip();
			if ((fBack <= 9000) && (fBack > 1000))
			{
				m_pCamera->SetFarClip(fBack + 1000);
				Options.view3d.iBackPlane = fBack;
			}
			else if (fBack < 10000)
			{
				m_pCamera->SetFarClip(fBack + 250);
				Options.view3d.iBackPlane = fBack;
			}
			break;
		}

		case 'O':
		case 'o':
		{
			m_pRender->DebugHook1();
			break;
		}

		case 'I':
		case 'i':
		{
			m_pRender->DebugHook2();
			break;
		}

		case 'P':
		case 'p':
		{
			pDoc->OnToggle3DGrid();
			break;
		}

		default:
		{
			break;
		}
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}


//-----------------------------------------------------------------------------
// Purpose: Handles key release events.
// Input  : Per CWnd::OnKeyup
//-----------------------------------------------------------------------------
void CMapView3D::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool)
	{
		if (pTool->OnKeyUp3D(this, nChar, nRepCnt, nFlags))
		{
			return;
		}
	}

	m_Keyboard.OnKeyUp(nChar, nRepCnt, nFlags);

	UpdateCameraVariables();
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}


//-----------------------------------------------------------------------------
// Purpose: Called when the view is resized.
// Input  : nType - 
//			cx - 
//			cy - 
//-----------------------------------------------------------------------------
void CMapView3D::OnSize(UINT nType, int cx, int cy) 
{
	if (cy && cx && m_pRender)
	{
		m_pRender->SetSize(cx, cy);
	}

	CView::OnSize(nType, cx, cy);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lpCreateStruct - 
// Output : 
//-----------------------------------------------------------------------------
int CMapView3D::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return(CView::OnCreate(lpCreateStruct));
}


//-----------------------------------------------------------------------------
// Purpose: Finds the axis that is most closely aligned with the given vector.
// Input  : Vector - Vector to find closest axis to.
// Output : Returns an axis index as follows:
//				0 - Positive X axis.
//				1 - Positive Y axis.
//				2 - Positive Z axis.
//				3 - Negative X axis.
//				4 - Negative Y axis.
//				5 - Negative Z axis.
//-----------------------------------------------------------------------------
int ClosestAxis(const Vector& v)
{
	Vector TestAxis;
	float fBestDot;
	float fTestDot;
	int nAxis;
	
	fBestDot = 0;
	nAxis = 0;

	for (int i = 0; i < 6; i++)
	{
		TestAxis[0] = (float)((i % 3) == 0 );
		TestAxis[1] = (float)((i % 3) == 1 );
		TestAxis[2] = (float)((i % 3) == 2 );

		if (i >= 3)
		{
			TestAxis[0] = -TestAxis[0];
			TestAxis[1] = -TestAxis[1];
			TestAxis[2] = -TestAxis[2];
		}

		fTestDot = DotProduct(v, TestAxis);
		if (fTestDot > fBestDot)
		{
			fBestDot = fTestDot;
			nAxis = i;
		}
	}

	return(nAxis);
}


//-----------------------------------------------------------------------------
// Purpose: Determines best mapping of 2D axes in the 3D view. This is used for
//			2D oberations in the 3D view, like vertex nudging.
// Input  : axes - 2D axis information.
//-----------------------------------------------------------------------------
void CMapView3D::CalcBestAxes(Axes2 &Axes)
{
	Vector ViewUp;
	Vector ViewRight;
	int nVertical;
	int nHorizontal;

	//
	// Find the axis closest to "Up".
	//
	m_pCamera->GetViewUp(ViewUp);
	nVertical = ClosestAxis(ViewUp);

	//
	// Find the axis closest to "Right".
	//
	m_pCamera->GetViewRight(ViewRight);
	nHorizontal = ClosestAxis(ViewRight);

	//
	// Set the axes accordingly.
	//
	Axes.SetAxes(nHorizontal % 3, nHorizontal > 2, nVertical % 3, nVertical < 3);
}


//-----------------------------------------------------------------------------
// Purpose: Synchronizes the 2D camera information with the 3D view.
//-----------------------------------------------------------------------------
void CMapView3D::UpdateCameraVariables(void)
{
	if (!m_pCamera)
	{
		return;
	}

	Vector ViewPoint;
	Vector ViewForward;

	m_pCamera->GetViewPoint(ViewPoint);
	m_pCamera->GetViewForward(ViewForward);

	GetDocument()->Camera_Update(ViewPoint, ViewForward);

	if (g_pToolManager->GetToolID() == TOOL_CAMERA)
	{
		if (Options.view2d.bCenteroncamera)
		{
			VIEW2DINFO vi;
			vi.wFlags = VI_CENTER;

			m_pCamera->GetViewPoint(vi.ptCenter);
			GetDocument()->SetView2dInfo(vi);
		}
		else
		{
			GetDocument()->ToolUpdateViews(CMapView2D::updTool);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles the left mouse button double click event.
//-----------------------------------------------------------------------------
void CMapView3D::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	//
	// Don't forward message if we are controlling the camera.
	//
	if ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0)
	{
		return;
	}

	//
	// Pass the message to the active tool.
	//
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool != NULL)
	{
		if (pTool->OnLMouseDblClk3D( this, nFlags, point ))
		{
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles the left mouse button down event.
//-----------------------------------------------------------------------------
void CMapView3D::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0)
	{
		EnableRotating(true);
		return;
	}

    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool != NULL)
	{
		if (pTool->OnLMouseDown3D(this, nFlags, point))
		{
			return;
		}
	}

	CView::OnLButtonDown(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: Called by the selection tool to begin timed selection by depth.
//-----------------------------------------------------------------------------
void CMapView3D::BeginPick(void)
{
	OnTimer(MVTIMER_PICKNEXT);
	SetTimer(MVTIMER_PICKNEXT, 500, NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Called by the selection tool to end timed selection by depth.
//-----------------------------------------------------------------------------
void CMapView3D::EndPick(void)
{
	//
	// Kill pick timer.
	//
	KillTimer(MVTIMER_PICKNEXT);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nFlags - 
//			point - 
//-----------------------------------------------------------------------------
void CMapView3D::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_bRotating)
	{
		EnableRotating(false);
		UpdateCameraVariables();
		return;
	}

	//
	// Pass the message to the active tool.
	//
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool != NULL)
	{
		if (pTool->OnLMouseUp3D(this, nFlags, point))
		{
			return;
		}
	}

	CView::OnLButtonUp(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: Creates the renderer and the camera and initializes them.
//-----------------------------------------------------------------------------
void CMapView3D::OnInitialUpdate(void)
{
	InitializeKeyMap();

	//
	// Create a title window.
	//
	m_pwndTitle = CTitleWnd::CreateTitleWnd(this, ID_2DTITLEWND);
	ASSERT(m_pwndTitle != NULL);
	if (m_pwndTitle != NULL)
	{
		m_pwndTitle->SetTitle("camera");
	}

	//
	// CMainFrame::LoadWindowStates calls InitialUpdateFrame which causes us to get two
	// OnInitialUpdate messages! Check for a NULL renderer to avoid processing twice.
	//
	if (m_pRender != NULL)
	{
		return;
	}

	//
	// Create and initialize the renderer.
	//
	m_pRender = CreateRender3D(Render3DTypeMaterialSystem);
	ASSERT(m_pRender != NULL);
	if (m_pRender == NULL)
	{
		ASSERT(m_pRender != NULL);
		return;
	}

	CMapDoc *pDoc = GetDocument();
	if (pDoc == NULL)
	{
		ASSERT(pDoc != NULL);
		return;
	}

	m_pRender->Initialize(m_hWnd, pDoc);
	SetDrawType(m_eDrawType);

	//
	// Create and initialize the camera.
	//
	m_pCamera = new CCamera();
	ASSERT(m_pCamera != NULL);
	if (m_pCamera == NULL)
	{
		return;
	}

	m_fForwardSpeedMax = Options.view3d.nForwardSpeedMax;
	m_fStrafeSpeedMax = Options.view3d.nForwardSpeedMax * 0.75f;
	m_fVerticalSpeedMax = Options.view3d.nForwardSpeedMax * 0.5f;

	//
	// Calculate the acceleration based on max speed and the time to max speed.
	//
	if (Options.view3d.nTimeToMaxSpeed != 0)
	{
		m_fForwardAcceleration = m_fForwardSpeedMax / (Options.view3d.nTimeToMaxSpeed / 1000.0f);
		m_fStrafeAcceleration = m_fStrafeSpeedMax / (Options.view3d.nTimeToMaxSpeed / 1000.0f);
		m_fVerticalAcceleration = m_fVerticalSpeedMax / (Options.view3d.nTimeToMaxSpeed / 1000.0f);
	}
	else
	{
		m_fForwardAcceleration = 0;
		m_fStrafeAcceleration = 0;
		m_fVerticalAcceleration = 0;
	}

	//
	// Set up the frustum. We set the vertical FOV to zero because the renderer
	// only uses the horizontal FOV.
	//
	m_pCamera->SetFrustum(CAMERA_HORIZONTAL_FOV, 0, CAMERA_FRONT_PLANE_DISTANCE, Options.view3d.iBackPlane);

	//
	// Set the distance at which studio models become bounding boxes.
	//
	CMapStudioModel::SetRenderDistance(Options.view3d.nModelDistance);
	CMapStudioModel::EnableAnimation(Options.view3d.bAnimateModels);

	//
	// Enable or disable reverse selection.
	//
	m_pRender->RenderEnable(RENDER_REVERSE_SELECTION, (Options.view3d.bReverseSelection == TRUE));

	//
	// Enable or disable the 3D grid.
	//
	m_pRender->RenderEnable(RENDER_GRID, pDoc->Is3DGridEnabled());

	//
	// Enable or disable texture filtering.
	//
	m_pRender->RenderEnable(RENDER_FILTER_TEXTURES, (Options.view3d.bFilterTextures == TRUE));

	//
	// Get the initial viewpoint and view direction from the default camera in the document.
	//
	Vector vecPos;
	Vector vecLookAt;
	pDoc->Camera_Get(vecPos, vecLookAt);
	SetCamera(vecPos, vecLookAt);

	//
	// Attach the camera to the renderer.
	//
	m_pRender->SetCamera(m_pCamera);

	CView::OnInitialUpdate();
}


//-----------------------------------------------------------------------------
// Purpose: Turns on wireframe mode from the floating "Camera" menu.
//-----------------------------------------------------------------------------
void CMapView3D::OnView3dWireframe(void)
{
	SetDrawType(VIEW3D_WIREFRAME);
}


//-----------------------------------------------------------------------------
// Purpose: Turns on flat shaded mode from the floating "Camera" menu.
//-----------------------------------------------------------------------------
void CMapView3D::OnView3dPolygon(void)
{
	SetDrawType(VIEW3D_POLYGON);
}


//-----------------------------------------------------------------------------
// Purpose: Turns on textured mode from the floating "Camera" menu.
//-----------------------------------------------------------------------------
void CMapView3D::OnView3dTextured(void)
{
	SetDrawType(VIEW3D_TEXTURED);
}


//-----------------------------------------------------------------------------
// Purpose: Turns on lightmap grid mode from the floating "Camera" menu.
//-----------------------------------------------------------------------------
void CMapView3D::OnView3dLightmapGrid(void)
{
	SetDrawType(VIEW3D_LIGHTMAP_GRID);
}


//-----------------------------------------------------------------------------
// Purpose: Turns on lighting preview mode from the floating "Camera" menu.
//-----------------------------------------------------------------------------
void CMapView3D::OnView3dLightingPreview(void)
{
	SetDrawType(VIEW3D_LIGHTING_PREVIEW);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bActivate - 
//			pActivateView - 
//			pDeactiveView - 
//-----------------------------------------------------------------------------
void CMapView3D::OnActivateView(BOOL bActivate, CView *pActivateView, CView *pDeactiveView)
{
	if (bActivate)
	{
		CMapDoc *pDoc = GetDocument();
		CMapDoc::SetActiveMapDoc(pDoc);
		pDoc->SetActiveView(this);

		UpdateStatusBar();

		// tell doc to update title
		pDoc->UpdateTitle(this);
	}

	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pDC - 
//-----------------------------------------------------------------------------
void CMapView3D::OnDraw(CDC *pDC)
{
	if (APP()->IsActiveApp() && APP()->Is3DRenderEnabled())
	{
		Render();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDC - 
//-----------------------------------------------------------------------------
void CMapView3D::OnSetFocus(CWnd *pOldWnd)
{
	// Make sure the whole window region is marked as invalid 
	Invalidate(false);
}


//-----------------------------------------------------------------------------
// Purpose: Called to paint the non client area of the window.
//-----------------------------------------------------------------------------
void CMapView3D::OnNcPaint(void)
{
	// Make sure the whole window region is marked as invalid 
	Invalidate(false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pSender - 
//			lHint - 
//			pHint - 
//-----------------------------------------------------------------------------
void CMapView3D::OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint)
{
	if (lHint & MAPVIEW_UPDATE_2D)
	{
		return;
	}

	//
	// One of the options in the 3D options page is changing.
	//
	if (lHint & MAPVIEW_OPTIONS_CHANGED)
	{
		InitializeKeyMap();

		m_pCamera->SetFrustum(CAMERA_HORIZONTAL_FOV, 0, CAMERA_FRONT_PLANE_DISTANCE, Options.view3d.iBackPlane);
		CMapStudioModel::SetRenderDistance(Options.view3d.nModelDistance);
		CMapStudioModel::EnableAnimation(Options.view3d.bAnimateModels);

		m_pRender->RenderEnable(RENDER_REVERSE_SELECTION, (Options.view3d.bReverseSelection == TRUE));

		m_fForwardSpeedMax = Options.view3d.nForwardSpeedMax;
		m_fStrafeSpeedMax = Options.view3d.nForwardSpeedMax * 0.75f;
		m_fVerticalSpeedMax = Options.view3d.nForwardSpeedMax * 0.5f;

		//
		// Calculate the acceleration based on max speed and the time to max speed.
		//
		if (Options.view3d.nTimeToMaxSpeed != 0)
		{
			m_fForwardAcceleration = m_fForwardSpeedMax / (Options.view3d.nTimeToMaxSpeed / 1000.0f);
			m_fStrafeAcceleration = m_fStrafeSpeedMax / (Options.view3d.nTimeToMaxSpeed / 1000.0f);
			m_fVerticalAcceleration = m_fVerticalSpeedMax / (Options.view3d.nTimeToMaxSpeed / 1000.0f);
		}
		else
		{
			m_fForwardAcceleration = 0;
			m_fStrafeAcceleration = 0;
			m_fVerticalAcceleration = 0;
		}

		//
		// Update because the app may not be idle (if the user is in the options screen, for instance).
		//
		Invalidate( false );
	}

	if (lHint & MAPVIEW_UPDATE_DISPLAY)
	{
		CMapDoc *pDoc = GetDocument();
		if ((pDoc != NULL) && (m_pRender != NULL))
		{
			m_pRender->RenderEnable(RENDER_GRID, pDoc->Is3DGridEnabled());
			m_pRender->RenderEnable(RENDER_FILTER_TEXTURES, (Options.view3d.bFilterTextures == TRUE));
		}
	}

	if (lHint & MAPVIEW_UPDATE_OBJECTS)
	{
		// dvs: could use this hint to update the octree
	}
}


//-----------------------------------------------------------------------------
// Purpose: Determines the object at the point (point.x, point.y) in the 3D view.
// Input  : point - Point to use for hit test.
//			ulFace - Index of face in object that was hit.
// Output : Returns a pointer to the CMapClass object at the coordinates, NULL if none.
//-----------------------------------------------------------------------------
CMapClass *CMapView3D::NearestObjectAt(CPoint point, ULONG &ulFace)
{
	ulFace = 0;
	if (m_pRender == NULL)
	{
		return(NULL);
	}

	RenderHitInfo_t Hits;

	if (m_pRender->ObjectsAt(point.x, point.y, 1, 1, &Hits, 1) != 0)
	{
		//
		// If they clicked on a solid, the index of the face they clicked on is stored
		// in array index [1].
		//
		CMapAtom *pObject = (CMapAtom *)Hits.pObject;
		CMapSolid *pSolid = dynamic_cast<CMapSolid *>(pObject);
		if (pSolid != NULL)
		{
			ulFace = Hits.uData;
			return(pSolid);
		}

		return((CMapClass *)pObject);
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Casts a ray from the viewpoint through the given plane and determines
//			the point of intersection of the ray on the plane.
// Input  : point - Point in client screen coordinates.
//			plane - Plane being 'clicked' on.
//			pos - Returns the point on the plane that projects to the given point.
//-----------------------------------------------------------------------------
void CMapView3D::GetHitPos(CPoint point, PLANE &plane, Vector &pos)
{
	//
	// Find the point they clicked on in world coordinates. It lies on the near
	// clipping plane.
	//
	Vector2D Client;
	Vector2D Screen;
	Vector ClickPoint;

	Client[0] = point.x;
	Client[1] = point.y;

	m_pRender->ClientToScreen(Screen, Client);
	m_pRender->ScreenToWorld(ClickPoint, Screen);

	//
	// Build a ray from the viewpoint through the point on the near clipping plane.
	//
	Vector ViewPoint;
	Vector Ray;
	m_pCamera->GetViewPoint(ViewPoint);
	VectorSubtract(ClickPoint, ViewPoint, Ray);

	//
	// Find the point of intersection of the ray with the given plane.
	//
	float t = DotProduct(plane.normal, ViewPoint) - plane.dist;
	t = t / -DotProduct(plane.normal, Ray);
	
	for (int i = 0; i < 3; i++)
	{
		pos[i] = ViewPoint[i] + t * Ray[i];
	}
}


//-----------------------------------------------------------------------------
// Purpose: Casts a ray from the viewpoint through the given plane and determines
//			the point of intersection of the ray on the plane.
// Input  : point - Point in client screen coordinates.
//			plane - Plane being 'clicked' on.
//			pos - Returns the point on the plane that projects to the given point.
//-----------------------------------------------------------------------------
void CMapView3D::BuildRay(CPoint point, Vector &start, Vector &end)
{
	//
	// Find the point they clicked on in world coordinates. It lies on the near
	// clipping plane.
	//
	Vector2D Client;
	Vector2D Screen;
	Vector ClickPoint;

	Client[0] = point.x;
	Client[1] = point.y;

	m_pRender->ClientToScreen(Screen, Client);
	m_pRender->ScreenToWorld(ClickPoint, Screen);

	//
	// Build a ray from the viewpoint through the point on the near clipping plane.
	//
	Vector ViewPoint;
	Vector Ray;
	m_pCamera->GetViewPoint(ViewPoint);
	Ray = ClickPoint - ViewPoint;
	VectorNormalize( Ray );

	Ray = Ray * 8192;
	start = ClickPoint;

	end = ClickPoint + Ray;
}


//-----------------------------------------------------------------------------
// Purpose: Finds all objects under the given rectangular region in the view.
// Input  : x - Client x coordinate.
//			y - Client y coordinate.
//			fWidth - Width of region in client pixels.
//			fHeight - Height of region in client pixels.
//			pObjects - Receives objects in the given region.
//			nMaxObjects - Size of the array pointed to by pObjects.
// Output : Returns the number of objects in the given region.
//-----------------------------------------------------------------------------
int CMapView3D::ObjectsAt(float x, float y, float fWidth, float fHeight, RenderHitInfo_t *pObjects, int nMaxObjects)
{
	if (m_pRender != NULL)
	{
		return m_pRender->ObjectsAt(x, y, fWidth, fHeight, pObjects, nMaxObjects);
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Makes sure that this view has focus if the mouse moves over it.
//-----------------------------------------------------------------------------
void CMapView3D::OnMouseMove(UINT nFlags, CPoint point)
{
	//
	// Make sure we are the active view.
	//
	if (!IsActive())
	{
		CMapDoc *pDoc = GetDocument();
		pDoc->SetActiveView(this);
	}

	//
	// If we are the active application, make sure this view has the input focus.
	//	
	if (APP()->IsActiveApp())
	{
		if (GetFocus() != this)
		{
			SetFocus();
		}
	}

	//
	// Pass mouse message to the active tool.
	//
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool != NULL)
	{
		if (pTool->OnMouseMove3D(this, nFlags, point))
		{
			return;
		}
	}

	//
	// The tool didn't handle the message. Make sure the cursor is set.
	//
	SetCursor(APP()->LoadStandardCursor(IDC_ARROW));

	CView::OnMouseMove(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapView3D::ProcessInput(void)
{
	if (m_dwTimeLastInputSample == 0)
	{
		m_dwTimeLastInputSample = timeGetTime();
	}

	DWORD dwTimeNow = timeGetTime();

	float fElapsedTime = (float)(dwTimeNow - m_dwTimeLastInputSample) / 1000.0f;

	m_dwTimeLastInputSample = dwTimeNow;

	// Clamp (can get really big when we cache textures in )
	if (fElapsedTime > 0.3f)
		fElapsedTime = 0.3f;

	ProcessKeys(fElapsedTime);

	ProcessMouse();
}


//-----------------------------------------------------------------------------
// Purpose: Applies an acceleration to a velocity, allowing instantaneous direction
//			change and zeroing the velocity in the absence of acceleration.
// Input  : fVelocity - Current velocity.
//			fAccel - Amount of acceleration to apply.
//			fTimeScale - The time for which the acceleration should be applied.
//			fMaxVelocity - The maximum velocity to allow.
// Output : Returns the new velocity.
//-----------------------------------------------------------------------------
static float Accelerate(float fVelocity, float fAccel, float fAccelScale, float fTimeScale, float fVelocityMax)
{
	//
	// If we have a finite acceleration in this direction, apply it to the velocity.
	//
	if ((fAccel != 0) && (fAccelScale != 0))
	{
		//
		// Check for direction reversal - zero velocity when reversing.
		//
		if (fAccelScale > 0)
		{
			if (fVelocity < 0)
			{
				fVelocity = 0;
			}
		}
		else if (fAccelScale < 0)
		{
			if (fVelocity > 0)
			{
				fVelocity = 0;
			}
		}

		//
		// Apply the acceleration.
		//
		fVelocity += fAccel * fAccelScale * fTimeScale;
		if (fVelocity > fVelocityMax)
		{
			fVelocity = fVelocityMax;
		}
		else if (fVelocity < -fVelocityMax)
		{
			fVelocity = -fVelocityMax;
		}

	}
	//
	// If we have infinite acceleration, go straight to maximum velocity.
	//
	else if (fAccelScale != 0)
	{
		fVelocity = fVelocityMax * fAccelScale;
	}
	//
	// Else no velocity in this direction at all.
	//
	else
	{
		fVelocity = 0;
	}

	return(fVelocity);
}


//-----------------------------------------------------------------------------
// Purpose: Moves the camera based on the keyboard state.
//-----------------------------------------------------------------------------
void CMapView3D::ProcessMovementKeys(float fElapsedTime)
{
	//
	// Read the state of the camera movement keys.
	//
	float fBack = m_Keyboard.GetKeyScale(LOGICAL_KEY_BACK);
	float fMoveForward = m_Keyboard.GetKeyScale(LOGICAL_KEY_FORWARD) - fBack;

	float fLeft = m_Keyboard.GetKeyScale(LOGICAL_KEY_LEFT);
	float fMoveRight = m_Keyboard.GetKeyScale(LOGICAL_KEY_RIGHT) - fLeft;

	float fDown = m_Keyboard.GetKeyScale(LOGICAL_KEY_DOWN);
	float fMoveUp = m_Keyboard.GetKeyScale(LOGICAL_KEY_UP) - fDown;

	float fPitchUp = m_Keyboard.GetKeyScale(LOGICAL_KEY_PITCH_UP);
	float fPitchDown = m_Keyboard.GetKeyScale(LOGICAL_KEY_PITCH_DOWN);

	float fYawLeft = m_Keyboard.GetKeyScale(LOGICAL_KEY_YAW_LEFT);
	float fYawRight = m_Keyboard.GetKeyScale(LOGICAL_KEY_YAW_RIGHT);

	//
	// Apply pitch and yaw if they are nonzero.
	//
	if ((fPitchDown - fPitchUp) != 0)
	{
		m_pCamera->Pitch((fPitchDown - fPitchUp) * fElapsedTime * PITCH_SPEED);
	}

	if ((fYawRight - fYawLeft) != 0)
	{
		m_pCamera->Yaw((fYawRight - fYawLeft) * fElapsedTime * YAW_SPEED);
	}

	//
	// Apply the accelerations to the forward, strafe, and vertical speeds. They are actually
	// velocities because they are signed values.
	//
	m_fForwardSpeed = Accelerate(m_fForwardSpeed, m_fForwardAcceleration, fMoveForward, fElapsedTime, m_fForwardSpeedMax);
	m_fStrafeSpeed = Accelerate(m_fStrafeSpeed, m_fStrafeAcceleration, fMoveRight, fElapsedTime, m_fStrafeSpeedMax);
	m_fVerticalSpeed = Accelerate(m_fVerticalSpeed, m_fVerticalAcceleration, fMoveUp, fElapsedTime, m_fVerticalSpeedMax);

	//
	// Move the camera if any of the speeds are nonzero.
	//
	if (m_fForwardSpeed != 0)
	{
		m_pCamera->MoveForward(m_fForwardSpeed * fElapsedTime);
	}

	if (m_fStrafeSpeed != 0)
	{
		m_pCamera->MoveRight(m_fStrafeSpeed * fElapsedTime);
	}

	if (m_fVerticalSpeed != 0)
	{
		m_pCamera->MoveUp(m_fVerticalSpeed * fElapsedTime);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapView3D::ProcessKeys(float fElapsedTime)
{
	ProcessMovementKeys(fElapsedTime);

	m_Keyboard.ClearImpulseFlags();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMapView3D::ControlCamera(void)
{
	if (!m_bStrafing && !m_bRotating && !m_bMouseLook)
	{
		return false;
	}

	//
	// Get the cursor position in client coordinates.
	//
	CPoint point;
	GetCursorPos(&point);
	ScreenToClient(&point);

	bool bShift = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);

	CRect rect;
	GetClientRect(&rect);

	CPoint WindowCenter = rect.CenterPoint();
	CSize MouseLookDelta = point - WindowCenter;

	//
	// If strafing, left-right movement moves the camera from side to side.
	// Up-down movement either moves the camera forward and back if the SHIFT
	// key is held down, or up and down if the SHIFT key is not held down.
	// If rotating and strafing simultaneously, the behavior is as if SHIFT is
	// held down.
	//
	if (m_bStrafing)
	{
		if (bShift || m_bRotating)
		{
			MoveForward(-MouseLookDelta.cy * 2);
		}
		else
		{
			MoveUp(-MouseLookDelta.cy * 2);
		}

		MoveRight(MouseLookDelta.cx * 2);
	}
	//
	// If mouse looking, left-right movement controls yaw, and up-down
	// movement controls pitch.
	//
	else
	{
		//
		// Up-down mouse movement changes the camera pitch.
		//
		if (MouseLookDelta.cy)
		{
			float fTheta = MouseLookDelta.cy * 0.4;
			if (Options.view3d.bReverseY)
			{
				fTheta = -fTheta;
			}

			Pitch(fTheta);
		}
	
		//
		// Left-right mouse movement changes the camera yaw.
		//
		if (MouseLookDelta.cx)
		{
			float fTheta = MouseLookDelta.cx * 0.4;
			Yaw(fTheta);
		}
	}

	if (MouseLookDelta.cx || MouseLookDelta.cy)
	{
		CWnd::ClientToScreen(&WindowCenter);
		SetCursorPos(WindowCenter.x, WindowCenter.y);
	}

	m_ptLastMouseMovement = point;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Called by RunFrame to tell this view to process mouse input. This
//			function samples the cursor position and takes the appropriate
//			action based on the current mode (camera, morphing).
//-----------------------------------------------------------------------------
void CMapView3D::ProcessMouse(void)
{
	if (ControlCamera())
	{
		return;
	}

	//
	// If not in mouselook mode, only process mouse messages if there
	// is an active tool.
	//
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool != NULL)
	{
		//
		// Get the cursor position in client coordinates.
		//
		CPoint point;
		GetCursorPos(&point);
		ScreenToClient(&point);

		//
		// Pass the message to the tool.
		//
		if (pTool->OnIdle3D(this, point))
		{
			APP()->SetForceRenderNextFrame();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles mouse wheel events. The mouse wheel is used in camera mode
//			to dolly the camera forward and back.
// Input  : Per CWnd::OnMouseWheel.
//-----------------------------------------------------------------------------
BOOL CMapView3D::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool != NULL)
	{
		if (pTool->OnMouseWheel3D(this, nFlags, zDelta, point))
		{
			return(TRUE);
		}
	}

	m_pCamera->MoveForward(zDelta / 2);
	
	//
	// Render now to avoid an ugly lag between the 2D views and the 3D view
	// when "center 2D views on camera" is enabled.
	//
	Invalidate(false);

	UpdateCameraVariables();

	return CView::OnMouseWheel(nFlags, zDelta, point);
}


//-----------------------------------------------------------------------------
// Purpose: Handles right mouse button down events.
// Input  : Per CWnd::OnRButtonDown.
//-----------------------------------------------------------------------------
void CMapView3D::OnRButtonDown(UINT nFlags, CPoint point) 
{
	if ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0)
	{
		EnableStrafing(true);
		return;
	}

	//
	// Pass the message to the active tool.
	//
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool != NULL)
	{
		if (pTool->OnRMouseDown3D( this, nFlags, point ))
		{
			return;
		}
	}

	CView::OnRButtonDown(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: Handles right mouse button up events.
// Input  : Per CWnd::OnRButtonUp.
//-----------------------------------------------------------------------------
void CMapView3D::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_bStrafing)
	{
		//
		// Turn off strafing and update the 2D views.
		//
		EnableStrafing(false);
		UpdateCameraVariables();
		return;
	}

	//
	// Pass the message to the active tool.
	//
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool != NULL)
	{
		if (pTool->OnRMouseUp3D( this, nFlags, point ))
		{
			return;
		}
	}

	CView::OnRButtonUp(nFlags, point);
}


//-----------------------------------------------------------------------------
// Purpose: Handles character events.
// Input  : Per CWnd::OnChar.
//-----------------------------------------------------------------------------
void CMapView3D::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    //
	// Pass the message to the active tool.
    //
	CBaseTool *pTool = g_pToolManager->GetTool();
	if (pTool != NULL)
	{
		if (pTool->OnChar3D(this, nChar, nRepCnt, nFlags))
		{
			return;
		}
	}

	CView::OnChar(nChar, nRepCnt, nFlags);
}


//-----------------------------------------------------------------------------
// Purpose: Called when mouselook is enabled. The cursor is moved to the center
//			of the screen and hidden.
// Input  : bEnable - true to lock and hide the cursor, false to unlock and show it.
//-----------------------------------------------------------------------------
void CMapView3D::EnableCrosshair(bool bEnable)
{
	CRect Rect;
	CPoint Point;

	GetClientRect(&Rect);
	CWnd::ClientToScreen(&Rect);
	Point = Rect.CenterPoint();
	SetCursorPos(Point.x, Point.y);

	if (bEnable)
	{
		ClipCursor(&Rect);
	}
	else
	{
		ClipCursor(NULL);
	}

	ShowCursor(bEnable ? FALSE : TRUE);
	m_pRender->RenderEnable(RENDER_CENTER_CROSSHAIR, bEnable);
}


//-----------------------------------------------------------------------------
// Purpose: Enables or disables mouselook. When mouselooking, the cursor is hidden
//			and a crosshair is rendered in the center of the view.
// Input  : bEnable - TRUE to enable, FALSE to disable mouselook.
//-----------------------------------------------------------------------------
//void CMapView3D::EnableMouseLook(bool bEnable)
//{
//	if (m_bMouseLook != bEnable)
//	{
//		CMapDoc *pDoc = GetDocument();
//		if (pDoc != NULL)
//		{
//			EnableCrosshair(bEnable);
//			m_bMouseLook = bEnable;
//		}
//	}
//}


//-----------------------------------------------------------------------------
// Purpose: Enables or disables mouselook. When mouselooking, the cursor is hidden
//			and a crosshair is rendered in the center of the view.
//-----------------------------------------------------------------------------
void CMapView3D::EnableMouseLook(bool bEnable)
{
	if (m_bMouseLook != bEnable)
	{
		if (!(m_bStrafing || m_bRotating))
		{
			EnableCrosshair(bEnable);
		}

		m_bMouseLook = bEnable;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Enables or disables camera rotating. When rotating, the cursor is hidden
//			and a crosshair is rendered in the center of the view.
//-----------------------------------------------------------------------------
void CMapView3D::EnableRotating(bool bEnable)
{
	if (m_bRotating != bEnable)
	{
		if (!(m_bStrafing || m_bMouseLook))
		{
			EnableCrosshair(bEnable);
		}

		m_bRotating = bEnable;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Enables or disables camera strafing. When strafing, the cursor is hidden
//			and a crosshair is rendered in the center of the view.
//-----------------------------------------------------------------------------
void CMapView3D::EnableStrafing(bool bEnable)
{
	if (m_bStrafing != bEnable)
	{
		if (!(m_bMouseLook || m_bRotating))
		{
			EnableCrosshair(bEnable);
		}

		m_bStrafing = bEnable;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Actually renders the 3D view. Called from the frame loop and from
//			some mouse messages when timely updating is important.
//-----------------------------------------------------------------------------
void CMapView3D::Render(void)
{
	if (m_pRender != NULL)
	{
		// dvs: revisit drawing red frame around active view
		//m_pRender->RenderEnable(RENDER_FRAME_RECT, IsActive());
		m_pRender->Render();
	}

	if (m_pwndTitle != NULL)
	{
		m_pwndTitle->BringWindowToTop();
		m_pwndTitle->Invalidate();
		m_pwndTitle->UpdateWindow();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//-----------------------------------------------------------------------------
void CMapView3D::RenderPreloadObject(CMapAtom *pObject)
{
	if ((pObject != NULL) && (m_pRender != NULL))
	{
		pObject->RenderPreload(m_pRender, false);
	}
}


//-----------------------------------------------------------------------------
// Release all video memory.
//-----------------------------------------------------------------------------
void CMapView3D::ReleaseVideoMemory(void)
{
	m_pRender->UncacheAllTextures();
}


//-----------------------------------------------------------------------------
// Purpose: Moves the camera forward by flDistance units. Negative units move back.
//-----------------------------------------------------------------------------
void CMapView3D::MoveForward(float flDistance)
{
	if (m_pCamera != NULL)
	{
		m_pCamera->MoveForward(flDistance);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Moves the camera up by flDistance units. Negative units move down.
//-----------------------------------------------------------------------------
void CMapView3D::MoveUp(float flDistance)
{
	if (m_pCamera != NULL)
	{
		m_pCamera->MoveUp(flDistance);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Moves the camera right by flDistance units. Negative units move left.
//-----------------------------------------------------------------------------
void CMapView3D::MoveRight(float flDistance)
{
	if (m_pCamera != NULL)
	{
		m_pCamera->MoveRight(flDistance);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Pitches the camera forward by flDegrees degrees. Negative units pitch back.
//-----------------------------------------------------------------------------
void CMapView3D::Pitch(float flDegrees)
{
	if (m_pCamera != NULL)
	{
		m_pCamera->Pitch(flDegrees);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Yaws the camera left by flDegrees degrees. Negative units yaw right.
//-----------------------------------------------------------------------------
void CMapView3D::Yaw(float flDegrees)
{
	if (m_pCamera != NULL)
	{
		m_pCamera->Yaw(flDegrees);
	}
}

