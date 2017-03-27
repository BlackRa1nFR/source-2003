//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CAMERA3D_H
#define CAMERA3D_H
#pragma once


#include "Tool3D.h"
#include "ToolInterface.h"
#include "afxtempl.h"
#include <fstream.h>


class CChunkFile;
class CSaveInfo;


enum ChunkFileResult_t;


const int MAX_CAMERAS = 16;


//
// Defines a camera position/look pair.
//
struct CAMSTRUCT
{
	Vector position;
	Vector look;
};


class Camera3D : public Tool3D
{
public:

	Camera3D(void);

	enum
	{
		constrainMoveBoth = 0x100
	};

	enum SNCTYPE
	{
		sncNext = -1,
		sncFirst = 0,
		sncPrev = 1
	};

	int GetActiveCamera(void) { return iActiveCamera; }
	void GetCameraPos(CAMSTRUCT *pCamPos, int iCamera = -1);
	void Update(const Vector &dir, const Vector &pos);

	//
	// Serialization.
	//
	ChunkFileResult_t LoadVMF(CChunkFile *pFile);
	ChunkFileResult_t SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo);
	void SerializeRMF(fstream &file, BOOL fIsStoring);

	//
	// Tool3D implementation.
	//
	virtual BOOL IsEmpty(void);
	virtual void SetEmpty(void);

	//
	// CBaseTool implementation.
	//
	virtual ToolID_t GetToolID(void) { return TOOL_CAMERA; }

	virtual bool OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point);

	virtual bool OnLMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseUp3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnRMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnRMouseUp3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);

	virtual void RenderTool2D(CRender2D *pRender);

protected:

	//
	// Tool3D implementation.
	//
	virtual int HitTest(CPoint pt, BOOL = FALSE);
	virtual BOOL StartTranslation(CPoint &pt);
	virtual BOOL UpdateTranslation(CPoint pt, UINT flags = 0, CSize &dragSize = CSize(0,0));
	virtual void FinishTranslation(BOOL bSave);

private:

	void StartNew(const Vector &vecStart);

	int GetCameraCount() { return m_nCameras; }
	void SetCameraCount(int nCount);
	void SetCameraPos(CAMSTRUCT *pCamPos, int iCamera = -1);

	void SetNextCamera(SNCTYPE next);
	void DeleteActiveCamera(void);

	void OnEscape(void);

	static ChunkFileResult_t LoadCameraKeyCallback(const char *szKey, const char *szValue, CAMSTRUCT *pCam);
	static ChunkFileResult_t LoadCamerasKeyCallback(const char *szKey, const char *szValue, Camera3D *pCameras);
	static ChunkFileResult_t LoadCameraCallback(CChunkFile *pFile, Camera3D *pCameras);

	int iActiveCamera;

	CArray<CAMSTRUCT, CAMSTRUCT&> Cameras;		// The cameras that have been created.
	int m_nCameras;								// The number of cameras that have been created.
	CAMSTRUCT tCamera;

	enum
	{
		MovePos,
		MoveLook
	};

	int MoveWhat;
};


#endif // CAMERA3D_H

