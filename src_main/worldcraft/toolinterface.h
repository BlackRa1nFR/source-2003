//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the interface that tools implement to allow views to call
//			through them.
//
//=============================================================================

#ifndef TOOLINTERFACE_H
#define TOOLINTERFACE_H
#ifdef _WIN32
#pragma once
#endif


class CMapView2D;
class CMapView3D;
class CRender2D;
class CRender3D;


enum ToolID_t
{
	TOOL_NONE = -1,
	TOOL_POINTER,
	TOOL_BLOCK,
	TOOL_ENTITY,
	TOOL_CAMERA,
	TOOL_DECAL,
	TOOL_MAGNIFY,
	TOOL_MORPH,
	TOOL_CLIPPER,
	TOOL_EDITCORDON,
	TOOL_PATH,
	TOOL_FACEEDIT_MATERIAL,
	TOOL_FACEEDIT_DISP,
	TOOL_OVERLAY,
	TOOL_AXIS_HANDLE,
	TOOL_POINT_HANDLE,
	TOOL_SPHERE,
	TOOL_PICK_FACE,
	TOOL_PICK_ENTITY,
	TOOL_PICK_ANGLES,
};


class CBaseTool
{
public:

	inline CBaseTool();
    virtual ~CBaseTool() {}

	//
	// Called by the tool manager to activate/deactivate tools.
	//
    inline void Activate(ToolID_t eOldTool);
    inline void Deactivate(ToolID_t eNewTool);
	virtual bool CanDeactivate( void ) { return true; }

	inline bool IsActiveTool( void ) { return m_bActiveTool; }

	//
	// Notifications for tool activation/deactivation.
	//
    virtual void OnActivate(ToolID_t eOldTool) {}
    virtual void OnDeactivate(ToolID_t eNewTool) {}

	virtual ToolID_t GetToolID(void) = 0;

	//
	// Called by CMapView2D to define the mapping of the current view.
	// This allows support for old Tool3D tools, which implement this.
	//
	virtual void SetAxesZoom(int nHorz, bool bInvertH, int nVert, bool bInvertV, float flSetZoom) {}

	//
	// Messages sent by the 3D view:
	//
	virtual bool OnContextMenu3D(CMapView3D *pView, CPoint point) { return false; }
	virtual bool OnLMouseDown3D( CMapView3D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnLMouseUp3D( CMapView3D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnLMouseDblClk3D( CMapView3D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnRMouseDown3D( CMapView3D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnRMouseUp3D( CMapView3D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnMouseMove3D( CMapView3D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnMouseWheel3D( CMapView3D *pView, UINT nFlags, short zDelta, CPoint point) { return false; }
	virtual bool OnKeyDown3D( CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags ) { return false; }
	virtual bool OnKeyUp3D( CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags ) { return false; }
	virtual bool OnChar3D( CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags ) { return false; }
	virtual bool OnIdle3D(CMapView3D *pView, CPoint &point) { return false; }

	//
	// Messages sent by the 2D view:
	//
	virtual bool OnContextMenu2D(CMapView2D *pView, CPoint point) { return false; }
	virtual bool OnLMouseDown2D( CMapView2D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnLMouseUp2D( CMapView2D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnLMouseDblClk2D( CMapView3D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnRMouseDown2D( CMapView2D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnRMouseUp2D( CMapView2D *pView, UINT nFlags, CPoint point ) { return false; }
	virtual bool OnMouseMove2D( CMapView2D *pView, UINT nFlags, CPoint point ) { return true; }
	virtual bool OnMouseWheel2D( CMapView2D *pView, UINT nFlags, short zDelta, CPoint point) { return false; }
	virtual bool OnKeyDown2D( CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags ) { return false; }
	virtual bool OnKeyUp2D( CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags ) { return false; }
	virtual bool OnChar2D( CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags ) { return false; }

	//
	// Rendering.
	//
	virtual void RenderTool2D( CRender2D *pRender ) {}
	virtual void RenderTool3D( CRender3D *pRender ) {}
	virtual void UpdateStatusBar( void ) {}

private:

	bool m_bActiveTool;		// Set to true when this is the active tool.
};


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CBaseTool::CBaseTool()
{
	m_bActiveTool = false;
}


//-----------------------------------------------------------------------------
// Purpose: Called whtn this tool is becoming the active tool.
// Input  : eOldTool - The tool that was previously active.
//-----------------------------------------------------------------------------
void CBaseTool::Activate(ToolID_t eOldTool)
{
	OnActivate(eOldTool);
	m_bActiveTool = true;
}


//-----------------------------------------------------------------------------
// Purpose: Called when this tool is no longer the active tool.
// Input  : eNewTool - The tool that is being activated.
//-----------------------------------------------------------------------------
void CBaseTool::Deactivate(ToolID_t eNewTool)
{
	OnDeactivate(eNewTool);
	m_bActiveTool = false;
}

#endif // TOOLINTERFACE_H
