//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PATH3D_H
#define PATH3D_H
#pragma once


#include "Box3D.h"
#include "MapPath.h"


class CMapView2D;
class CMapView3D;


typedef struct
{
	CMapPath *pPath;
	DWORD dwNodeID;
	BOOL bSelected;
} MAPPATHHANDLE;


class Path3D : public Box3D
{

public:

	Path3D();
	~Path3D();

	void AddNodeAt(CPoint pt);

	BOOL DeletePath(CPoint pt);
	BOOL SelectPath(CPoint ptMapScreen);
	BOOL HidePath(CPoint ptMapScreen);

	void EditNodeProperties(CPoint pt);
	void EditPathProperties(CPoint pt);

	int GetSelectedCount() { return m_nSelectedHandles; }
	void GetSelectedCenter(Vector& pt);
	void DeleteSelected();

	void SetPathList(CMapPathList *pList);

	//
	// CBaseTool implemenation.
	//
	virtual void OnDeactivate(ToolID_t eNewTool);
	virtual ToolID_t GetToolID(void) { return TOOL_PATH; }

	virtual bool OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnContextMenu2D(CMapView2D *pView, CPoint point);
	virtual bool OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point);

	virtual void RenderTool2D(CRender2D *pRender);

private:

	enum
	{
		scClear = 0x01,
		scSelect = 0x02,
		scUnselect = 0x04,
		scToggle = 0x08
	};

	// hit test + select:
	int HitTest(CPoint pt, BOOL = FALSE);
	int PathHitTest2D(CPoint pt, MAPPATHHANDLE *pHandle);

	void GetHitInfo(CPoint pt, MAPPATHHANDLE *pHandle);
	void SelectHandle(MAPPATHHANDLE *pHandle, int cmd);

	void StartNew(const Vector &vecStart);

	enum
	{
		flagCreateNewNode = 0x1000
	};

	BOOL StartBoxSelection(Vector& pt3);
	void SelectInBox();
	void EndBoxSelection();
	BOOL IsBoxSelecting() { return m_bBoxSelecting; }

	//
	// Tool3D implementation.
	//
	virtual BOOL IsEmpty();
	virtual BOOL StartTranslation(CPoint &pt);
	virtual BOOL UpdateTranslation(CPoint pt, UINT uFlags, CSize &size = CSize(0, 0));
	virtual void FinishTranslation(BOOL bSave);

	CArray<MAPPATHHANDLE, MAPPATHHANDLE&> m_SelectedHandles;
	int m_nSelectedHandles;

	CMapPathList *m_pPaths;

	MAPPATHHANDLE m_hndTranslate;
	CPoint m_ptOrigHandlePos;

	BOOL m_bBoxSelecting;
	BOOL m_bNew;
	BOOL m_bCreatedNewNode;
	CMapPath *m_pNewPath;

	bool m_bLButtonDown;			// True if the left button is down, false if not.
	CPoint m_ptLDownClient;			// Client position at which left mouse button was pressed.
};


#endif // PATH3D_H
