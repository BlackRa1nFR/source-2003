//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TOOLBLOCK_H
#define TOOLBLOCK_H
#ifdef _WIN32
#pragma once
#endif


#include "Box3D.h"
#include "ToolInterface.h"


class CToolBlockMessageWnd;


class CToolBlock : public Box3D
{

friend class CToolBlockMessageWnd;

public:

	CToolBlock();
	~CToolBlock();

	//
	// CBaseTool implementation.
	//
	virtual ToolID_t GetToolID(void) { return TOOL_BLOCK; }

	virtual bool OnContextMenu2D(CMapView2D *pView, CPoint point);
	virtual bool OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point);

	virtual bool OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnMouseMove3D(CMapView3D *pView, UINT nFlags, CPoint point);

	virtual void RenderTool2D(CRender2D *pRender);
	virtual void RenderTool3D(CRender3D *pRender);

private:

	void CreateMapObject(CMapView2D *pView);
	void OnEscape();

	void SetBlockCursor();

	bool m_bLButtonDown;		// True if we got a left button down message, false if not.
	CPoint m_ptLDownClient;		// Client position at which left mouse button was pressed.
};


#endif // TOOLBLOCK_H
