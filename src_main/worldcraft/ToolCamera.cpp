//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "ToolCamera.h"
#include "SaveInfo.h"
#include "MainFrm.h"			// dvs: remove?
#include "MapDefs.h"
#include "MapDoc.h"
#include "MapView2D.h"
#include "MapView3D.h"
#include "Options.h"
#include "Render2D.h"
#include "StatusBarIDs.h"		// dvs: remove
#include "ToolManager.h"
#include "worldcraft_mathlib.h"


#define CAMERA_HANDLE_RADIUS 5


#pragma warning(disable:4244)


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Camera3D::Camera3D(void)
{
	Cameras.SetSize(MAX_CAMERAS);
	SetEmpty();

	memset(&Cameras[0], 0, sizeof CAMSTRUCT);
	iActiveCamera = -1;
	m_nCameras = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if we are dragging a camera, false if not. // dvs: rename
//-----------------------------------------------------------------------------
BOOL Camera3D::IsEmpty(void)
{
	return (m_nCameras == 0);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Camera3D::SetEmpty(void)
{
	m_nCameras = 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			BOOL - 
// Output : int
//-----------------------------------------------------------------------------
int Camera3D::HitTest(CPoint pt, BOOL)
{
	CRect r;

	for(int i = 0; i < m_nCameras; i++)
	{
		Vector& camPos	= Cameras[i].position;
		Vector& lookPos	= Cameras[i].look;

		r.SetRect(camPos[axHorz], camPos[axVert], camPos[axHorz], camPos[axVert]);
		RectToScreen(r);
		r.InflateRect(CAMERA_HANDLE_RADIUS, CAMERA_HANDLE_RADIUS);
		if(r.PtInRect(pt))
		{
			return MAKELONG(i, MovePos);
		}
		
		r.SetRect(lookPos[axHorz], lookPos[axVert], lookPos[axHorz], lookPos[axVert]);
		RectToScreen(r);
		r.InflateRect(CAMERA_HANDLE_RADIUS, CAMERA_HANDLE_RADIUS);
		if(r.PtInRect(pt))
		{
			return MAKELONG(i, MoveLook);
		}
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Camera3D::StartTranslation(CPoint &pt)
{
	DWORD dwHit = HitTest(pt);

	if(int(dwHit) == -1)	// nothing
		return FALSE;

	MoveWhat = HIWORD(dwHit);
	iActiveCamera = LOWORD(dwHit);

	tCamera = Cameras[iActiveCamera];

	Tool3D::StartTranslation(pt);

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bSave - 
//-----------------------------------------------------------------------------
void Camera3D::FinishTranslation(BOOL bSave)
{
	if (bSave)
	{
		Cameras[iActiveCamera] = tCamera;
		if (iActiveCamera == m_nCameras)
		{
			++m_nCameras;
		}
	}

	Tool3D::FinishTranslation(bSave);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			uFlags - 
//			CSize& - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Camera3D::UpdateTranslation(CPoint pt, UINT uFlags, CSize&)
{
	BOOL bNeedsUpdate = FALSE;

	float delta[3];
	for(int i = 0; i < 3; i++)
	{
		delta[i] = tCamera.look[i] - tCamera.position[i];
	}

	switch(MoveWhat)
	{
		case MovePos:
		{
			Vector vecCompare = tCamera.position;
			tCamera.position[axHorz] = pt.x;
			tCamera.position[axVert] = pt.y;
			bNeedsUpdate = !CompareVectors2D(tCamera.position, vecCompare);
			if(bNeedsUpdate && (uFlags & constrainMoveBoth))
			{
				tCamera.look[axHorz] = pt.x + delta[axHorz];
				tCamera.look[axVert] = pt.y + delta[axVert];
			}
			break;
		}
		case MoveLook:	
		{
			Vector vecCompare = tCamera.look;
			tCamera.look[axHorz] = pt.x;
			tCamera.look[axVert] = pt.y;
			bNeedsUpdate = !CompareVectors2D(tCamera.look, vecCompare);
			if(bNeedsUpdate && (uFlags & constrainMoveBoth))
			{
				tCamera.position[axHorz] = pt.x - delta[axHorz];
				tCamera.position[axVert] = pt.y - delta[axVert];
			}
			break;
		}
	}

	return bNeedsUpdate;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCamPos - 
//			iCamera - 
//-----------------------------------------------------------------------------
void Camera3D::GetCameraPos(CAMSTRUCT *pCamPos, int iCamera)
{
	if(iCamera == -1)
		iCamera = iActiveCamera;
	if(!inrange(iCamera, 0, m_nCameras))
		return;
	memcpy(pCamPos, &Cameras[iCamera], sizeof(CAMSTRUCT));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pCamPos - 
//			iCamera - 
//-----------------------------------------------------------------------------
void Camera3D::SetCameraPos(CAMSTRUCT *pCamPos, int iCamera)
{
	if(iCamera == -1)
		iCamera = iActiveCamera;
	if(!inrange(iCamera, 0, m_nCameras))
		return;
	memcpy(&Cameras[iCamera], pCamPos, sizeof(CAMSTRUCT));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt3 - 
//-----------------------------------------------------------------------------
void Camera3D::StartNew(const Vector &vecStart)
{
	tCamera.position = vecStart;
	tCamera.look = vecStart;

	MoveWhat = MoveLook;

	// set iactivecamera
	Cameras.SetAtGrow(m_nCameras, tCamera);
	iActiveCamera = m_nCameras;
	++m_nCameras;

	CPoint pt(vecStart[axHorz], vecStart[axVert]);
	Tool3D::StartTranslation(pt);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void Camera3D::RenderTool2D(CRender2D *pRender)
{
	if (!IsActiveTool())
	{
		return;
	}

	CAMSTRUCT *pDrawCam;
	CRect r;
	CRect rHandle;

	for (int i = 0; i < m_nCameras; i++)
	{
		if (IsTranslating() && (i == iActiveCamera))
		{
			pDrawCam = &tCamera;
		}
 		else
		{
			pDrawCam = &Cameras[i];
		}

		// topleft becomes campos, bottomright becomes lookpos
		r.SetRect(pDrawCam->position[axHorz], pDrawCam->position[axVert], pDrawCam->look[axHorz], pDrawCam->look[axVert]);
		RectToScreen(r);

		//
		// Draw the line between.
		//
		if (i == iActiveCamera)
		{
			pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 255, 0, 0);
		}
		else
		{
			pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 0, 255, 255);
		}

		pRender->MoveTo(r.left, r.top);
		pRender->LineTo(r.right, r.bottom);

		//
		// Draw camera handle.
		//
		pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 0, 0, 0);
		pRender->SetFillColor(0, 255, 255);

		rHandle.SetRect(r.left, r.top, r.left, r.top);
		pRender->DrawEllipse(rHandle.CenterPoint(), CAMERA_HANDLE_RADIUS, CAMERA_HANDLE_RADIUS, true);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles key values being read from the MAP file.
// Input  : szKey - Key being loaded.
//			szValue - Value of the key being loaded.
//			pCam - Camera structure to place the values into.
// Output : Returns ChunkFile_Ok to indicate success.
//-----------------------------------------------------------------------------
ChunkFileResult_t Camera3D::LoadCameraKeyCallback(const char *szKey, const char *szValue, CAMSTRUCT *pCam)
{
	if (!stricmp(szKey, "look"))
	{
		CChunkFile::ReadKeyValueVector3(szValue, pCam->look);
	}
	else if (!stricmp(szKey, "position"))
	{
		CChunkFile::ReadKeyValueVector3(szValue, pCam->position);
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: Handles key values being read from the MAP file.
// Input  : szKey - Key being loaded.
//			szValue - Value of the key being loaded.
//			pCam - Camera structure to place the values into.
// Output : Returns ChunkFile_Ok to indicate success.
//-----------------------------------------------------------------------------
ChunkFileResult_t Camera3D::LoadCamerasKeyCallback(const char *szKey, const char *szValue, Camera3D *pCameras)
{
	if (!stricmp(szKey, "activecamera"))
	{
		pCameras->iActiveCamera = atoi(szValue);
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pLoadInfo - 
//			*pSolid - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t Camera3D::LoadCameraCallback(CChunkFile *pFile, Camera3D *pCameras)
{
	int nCount = pCameras->GetCameraCount();
	pCameras->SetCameraCount(nCount + 1);

	CAMSTRUCT Cam;
	memset(&Cam, 0, sizeof(Cam));

	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadCameraKeyCallback, &Cam);

	if (eResult == ChunkFile_Ok)
	{
		pCameras->SetCameraPos(&Cam, nCount);
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nCount - 
//-----------------------------------------------------------------------------
void Camera3D::SetCameraCount(int nCount)
{
	Cameras.SetSize(nCount);
	m_nCameras = nCount;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t Camera3D::LoadVMF(CChunkFile *pFile)
{
	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("camera", (ChunkHandler_t)LoadCameraCallback, this);

	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadCamerasKeyCallback, this);
	pFile->PopHandlers();

	if (eResult == ChunkFile_Ok)
	{
		//
		// Make sure the active camera is legal.
		//
		if (m_nCameras == 0)
		{
			iActiveCamera = -1;
		}
		else if (!inrange(iActiveCamera, 0, m_nCameras))
		{
			iActiveCamera = 0;
		}
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &dir - 
//			&pos - 
//-----------------------------------------------------------------------------
void Camera3D::Update(const Vector &dir, const Vector &pos)
{
	if(!inrange(iActiveCamera, 0, m_nCameras))
		return;

	Vector& camPos	= Cameras[iActiveCamera].position;
	Vector& lookPos	= Cameras[iActiveCamera].look;

	// get current length
	Vector delta;
	for(int i = 0; i < 3; i++)
		delta[i] = camPos[i] - lookPos[i];

	float length = VectorLength(delta);

	camPos = pos;

	for(i = 0; i < 3; i++)
		lookPos[i] = camPos[i] + dir[i] * length;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void Camera3D::SetNextCamera(SNCTYPE type)
{
	if(!m_nCameras)
	{
		iActiveCamera = -1;
		return;
	}
		
	switch(type)
	{
	case sncNext:
		++iActiveCamera;
		if(iActiveCamera >= m_nCameras)
			iActiveCamera = 0;
		break;
	case sncPrev:
		--iActiveCamera;
		if(iActiveCamera < 0)
			iActiveCamera = m_nCameras-1;
		break;
	case sncFirst:
		iActiveCamera = 0;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Camera3D::DeleteActiveCamera()
{
	if(!inrange(iActiveCamera, 0, m_nCameras))
		return;

	Cameras.RemoveAt(iActiveCamera);
	--m_nCameras;
	if(iActiveCamera >= m_nCameras)
		iActiveCamera = m_nCameras-1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : file - 
//			fIsStoring - 
//-----------------------------------------------------------------------------
void Camera3D::SerializeRMF(fstream& file, BOOL fIsStoring)
{
	float fVersion = 0.2f, fThisVersion;

	if(fIsStoring)
	{
		file.write((char*)&fVersion, sizeof fVersion);

		file.write((char*)&iActiveCamera, sizeof iActiveCamera);
		file.write((char*)&m_nCameras, sizeof m_nCameras);
		for(int i = 0; i < m_nCameras; i++)
		{
			file.write((char*)&Cameras[i], sizeof(CAMSTRUCT));
		}
	}
	else
	{
		file.read((char*)&fThisVersion, sizeof fThisVersion);

		if(fThisVersion >= 0.2f)
		{
			file.read((char*)&iActiveCamera, sizeof iActiveCamera);
		}

		file.read((char*)&m_nCameras, sizeof m_nCameras);
		Cameras.SetSize(m_nCameras);
		for(int i = 0; i < m_nCameras; i++)
		{
			file.read((char*)&Cameras[i], sizeof(CAMSTRUCT));
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t Camera3D::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	ChunkFileResult_t eResult = pFile->BeginChunk("cameras");
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("activecamera", iActiveCamera);
	}
	
	if (eResult == ChunkFile_Ok)
	{
		for (int i = 0; i < m_nCameras; i++)
		{
			eResult = pFile->BeginChunk("camera");

			if (eResult == ChunkFile_Ok)
			{
				eResult = pFile->WriteKeyValueVector3("position", Cameras[i].position);
			}
			
			if (eResult == ChunkFile_Ok)
			{
				eResult = pFile->WriteKeyValueVector3("look", Cameras[i].look);
			}

			if (eResult == ChunkFile_Ok)
			{
				eResult = pFile->EndChunk();
			}

			if (eResult != ChunkFile_Ok)
			{
				break;
			}
		}
	}
			
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Handles the key down event in the 2D view.
// Input  : Per CWnd::OnKeyDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Camera3D::OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_DELETE || nChar == VK_NEXT || nChar == VK_PRIOR)
	{
		CMapDoc *pDoc = pView->GetDocument();

		if (nChar == VK_DELETE)
		{
			DeleteActiveCamera();
		}
		else if (nChar == VK_NEXT)
		{
			SetNextCamera(Camera3D::sncNext);
		}
		else
		{
			SetNextCamera(Camera3D::sncPrev);
		}
		
		pDoc->UpdateAllCameras(*this);
		pDoc->ToolUpdateViews(CMapView2D::updTool);

		return true;
	}
	else if (nChar == VK_ESCAPE)
	{
		OnEscape();
		return true;	
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the left mouse button down event in the 2D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Camera3D::OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point) 
{
	CMapDoc *pDoc = pView->GetDocument();

	pView->SetCapture();

	//
	// Convert to some odd coordinate space that the base tools code uses.
	//
  	CPoint ptScreen = point;
	ptScreen.x += pView->GetScrollPos(SB_HORZ);
	ptScreen.y += pView->GetScrollPos(SB_VERT);
	
	//
	// Convert point to world coords.
	//
	pView->ClientToWorld(point);

	//
	// If there are no cameras created yet or they are holding down
	// the SHIFT key, create a new camera now.
	//
	if (IsEmpty() || (nFlags & MK_SHIFT))
	{
		//
		// Build a point in world space to place the new camera.
		//
		Vector vecStart(COORD_NOTINIT, COORD_NOTINIT, COORD_NOTINIT);

		if (pDoc->Selection_GetCount() != 0)
		{
			pDoc->Selection_GetBoundsCenter(vecStart);
		}

		vecStart[axHorz] = point.x;
		vecStart[axVert] = point.y;

		pDoc->GetBestVisiblePoint(vecStart);

		//
		// Create a new camera.
		//
		StartNew(vecStart);
		pView->SetUpdateFlag(CMapView2D::updTool);
	}
	//
	// Otherwise, try to drag an existing camera handle.
	//
	else
	{
		StartTranslation(ptScreen);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the left mouse button up event in the 2D view.
// Input  : Per CWnd::OnLButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Camera3D::OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point) 
{
	ReleaseCapture();

	if (IsTranslating())
	{
		FinishTranslation(TRUE);
		m_pDocument->ToolUpdateViews(CMapView2D::updTool);
		m_pDocument->UpdateAllCameras(*this);
	}

	m_pDocument->UpdateStatusbar();
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the mouse move event in the 2D view.
// Input  : Per CWnd::OnMouseMove.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Camera3D::OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point) 
{
	CMapDoc *pDoc = pView->GetDocument();
	if (!pDoc)
	{
		return true;
	}

	bool bCursorSet = false;
	BOOL bDisableSnap = (GetAsyncKeyState(VK_MENU) & 0x8000) ? TRUE : FALSE;
					    
	//
	// Make sure the point is visible.
	//
	pView->ToolScrollToPoint(point);

	//
	// Convert to some odd coordinate space that the base tools code uses.
	//
  	CPoint ptScreen = point;
	ptScreen.x += pView->GetScrollPos(SB_HORZ);
	ptScreen.y += pView->GetScrollPos(SB_VERT);

	//
	// Convert to world coords.
	//
	Vector vecWorld;
	pView->ClientToWorld(vecWorld, point);
	point.x = vecWorld[axHorz];
	point.y = vecWorld[axVert];

	//
	// Update status bar position display.
	//
	char szBuf[128];
	m_pDocument->Snap(vecWorld);
	sprintf(szBuf, " @%.0f, %.0f ", vecWorld[axHorz], vecWorld[axVert]);
	SetStatusText(SBI_COORDS, szBuf);
	
	if (IsTranslating())
	{
		// cursor is cross here
		bCursorSet = true;

		UINT uConstraints = 0;

		if (bDisableSnap)
		{
			uConstraints |= Tool3D::constrainNosnap;
		}

		if(nFlags & MK_CONTROL)
		{
			uConstraints |= Camera3D::constrainMoveBoth;
		}

		if (UpdateTranslation(point, uConstraints, CSize(0,0)))
		{
			pDoc->ToolUpdateViews(CMapView2D::updTool);
			pDoc->Update3DViews();
		}
	}
	else if (!IsEmpty())
	{
		//
		// If the cursor is on a handle, set it to a cross.
		//
		if (HitTest(ptScreen, TRUE) != -1)
		{
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
			bCursorSet = true;
		}
	}

	if (!bCursorSet)
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the left mouse button down event in the 3D view.
// Input  : Per CWnd::OnLButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Camera3D::OnLMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	pView->EnableRotating(true);
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the left mouse up down event in the 3D view.
// Input  : Per CWnd::OnLButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Camera3D::OnLMouseUp3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	pView->EnableRotating(false);
	pView->UpdateCameraVariables();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the right mouse button down event in the 3D view.
// Input  : Per CWnd::OnRButtonDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Camera3D::OnRMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	pView->EnableStrafing(true);
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the right mouse button up event in the 3D view.
// Input  : Per CWnd::OnRButtonUp.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Camera3D::OnRMouseUp3D(CMapView3D *pView, UINT nFlags, CPoint point)
{
	pView->EnableStrafing(false);
	pView->UpdateCameraVariables();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the key down event in the 3D view.
// Input  : Per CWnd::OnKeyDown.
// Output : Returns true if the message was handled, false if not.
//-----------------------------------------------------------------------------
bool Camera3D::OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_DELETE || nChar == VK_NEXT || nChar == VK_PRIOR)
	{
		CMapDoc *pDoc = pView->GetDocument();

		if (nChar == VK_DELETE)
		{
			DeleteActiveCamera();
		}
		else if (nChar == VK_NEXT)
		{
			SetNextCamera(Camera3D::sncNext);
		}
		else
		{
			SetNextCamera(Camera3D::sncPrev);
		}
		
		pDoc->UpdateAllCameras(*this);
		pDoc->ToolUpdateViews(CMapView2D::updTool);
		return true;
	}
	else if (nChar == VK_ESCAPE)
	{
		OnEscape();
		return true;	
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Handles the escape key in the 2D or 3D views.
//-----------------------------------------------------------------------------
void Camera3D::OnEscape(void)
{
	//
	// Stop using the camera tool.
	//
	g_pToolManager->SetTool(TOOL_POINTER);
}

