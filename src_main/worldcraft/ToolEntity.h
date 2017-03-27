//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the interface to the entity placement tool.
//
// $NoKeywords: $
//=============================================================================

#ifndef MARKER3D_H
#define MARKER3D_H
#pragma once


#include "ToolInterface.h"
#include "Tool3D.h"


class CRender2D;
class CRender3D;


class Marker3D : public Tool3D
{

public:

	Marker3D(void);
	~Marker3D(void);

	inline void GetPos(Vector &vecPos);

	//
	// Tool3D implementation.
	//
	virtual BOOL IsEmpty(void);
	virtual void SetEmpty(void);

	//
	// CBaseTool implementation.
	//
	virtual ToolID_t GetToolID(void) { return TOOL_ENTITY; }

	virtual bool OnContextMenu2D(CMapView2D *pView, CPoint point);
	virtual bool OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point);

	virtual bool OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnLMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point);
	
	virtual void RenderTool2D(CRender2D *pRender);
	virtual void RenderTool3D(CRender3D *pRender);

protected:

	//
	// Tool3D implementation.
	//
	virtual BOOL StartTranslation(CPoint &pt);
	virtual BOOL UpdateTranslation(CPoint pt, UINT, CSize &dragSize = CSize(0, 0));
	virtual void FinishTranslation(BOOL bSave);
	virtual int HitTest(CPoint pt, BOOL = FALSE);

private:

	void OnEscape(void);
	void StartNew(const Vector &vecStart);
	void CreateMapObject(CMapView2D *pView);

	Vector m_vecPos;			// Current position of the marker.
	Vector m_vecTranslatePos;	// Position while dragging the marker, stored separately so we can hit ESC and put it back.

	bool m_bEmpty;

	bool m_bLButtonDown;		// True if the left button is down, false if not.
};


//-----------------------------------------------------------------------------
// Purpose: Returns the current position of the marker.
//-----------------------------------------------------------------------------
inline void Marker3D::GetPos(Vector &vecPos)
{
	vecPos = m_vecPos;
}


#endif // MARKER3D_H
