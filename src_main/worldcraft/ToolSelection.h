//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SELECTION3D_H
#define SELECTION3D_H
#ifdef _WIN32
#pragma once
#endif


#include "Box3D.h"
#include "MapClass.h"			// For CMapObjectList
#include "ToolInterface.h"
#include "UtlVector.h"


class CMapWorld;
class CMapView;
class CMapView2D;
class CMapView3D;
class GDinputvariable;
class CRender2D;


//
// Modes that determine which selection handles render.
//
enum SelectionHandleMode_t
{
	HandleMode_SelectionOnly,
	HandleMode_HelpersOnly,
	HandleMode_Both,
};


class Selection3D : public Box3D
{

public:

	Selection3D();
	~Selection3D();

	void Add(CMapClass *p);

	void Remove(CMapClass *p);
	inline void RemoveAll();
	void RemoveInvisibles();
	void RemoveDead();

	inline int GetCount();
	inline CMapClass *GetObject(int nIndex);
	CMapObjectList *GetList(); // FIXME: hack for interim support of old code

	bool IsSelected(CMapClass *pobj);

	void SetHandleMode(SelectionHandleMode_t eHandleMode);
	inline SelectionHandleMode_t GetHandleMode();

	inline BOOL IsBoxSelecting() { return bBoxSelection; }
	void EndBoxSelection();

	inline SelectMode_t GetSelectMode();
	inline void SetSelectMode(SelectMode_t eSelectMode);

	inline void GetLastValidBounds(Vector &vecMins, Vector &vecMaxs);

	//
	// Tool3D implementation. 
	//
	virtual void SetEmpty();
	virtual BOOL IsEmpty();

	//
	// CBaseTool implementation.
	//
	virtual void OnActivate(ToolID_t eOldTool);
	virtual void OnDeactivate(ToolID_t eNewTool);
	virtual ToolID_t GetToolID() { return TOOL_POINTER; }

	virtual bool OnContextMenu2D(CMapView2D *pView, CPoint point);
	virtual bool OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point);

	virtual bool OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnLMouseDblClk3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseUp3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnMouseMove3D(CMapView3D *pView, UINT nFlags, CPoint point);

	virtual void RenderTool2D(CRender2D *pRender);
	virtual void RenderTool3D(CRender3D *pRender);

	void UpdateBounds();

	BoundBox TotalArea;	// bounded area of last transformation
	BOOL bBoxSelection;

private:

	BOOL UpdateTranslation(const Vector &vecPoint, UINT uConstraints, CSize &dragSize);

	void TransformSelection();
	BOOL StartBoxSelection(Vector &pt3);

	void UpdateHandleState();

	bool SelectAt(CPoint point, bool bMakeFirst);

	unsigned int GetConstraints(bool bDisableSnap, int nKeyFlags);

	// Callback function for drawing the selection contents.
	static BOOL DrawObject(CMapClass *pobj, Selection3D *pSel);

	GDinputvariable *ChooseEyedropperVar(CMapView *pView, CUtlVector<GDinputvariable *> &VarList);
	bool IsAnEntitySelected();
	CMapEntity *FindEntityInTree(CMapClass *pObject);

	bool StartSelectionTranslation(CPoint &point, bool bMoveOnly);
	void SelectInBox(CMapDoc *pDoc, bool bInsideOnly);

	void SetEyedropperCursor();

	void EyedropperPick2D(CMapView2D *pView, CPoint point);
	void EyedropperPick3D(CMapView3D *pView, CPoint point);
	void EyedropperPick(CMapView *pView, CMapClass *pObject);

	void OnEscape(CMapDoc *pDoc);

	//
	// Tool3D implementation.
	//
	virtual int HitTest(CPoint pt, BOOL bValidOnly);
	virtual void FinishTranslation(BOOL bSave);

	CUtlVector<CMapClass *> m_SelectionList;	// The list of selected objects.

	bool m_bLButtonDown;		// True if the left button is down, false if not.
	bool m_bLeftDragged;		// Have they dragged the mouse with the left button down?

	bool m_bEyedropper;			// True if we are holding down the eyedropper hotkey.

	bool m_bSelected;			// Did we select an object on left button down?
	CPoint m_ptLDownClient;		// Client pos at which lbutton was pressed.

	CUtlVector<CMapClass *> m_ToolHelperList;	// A list of currently selected helpers that implement the CBaseTool interface.

	SelectMode_t m_eSelectMode;				// Controls what gets selected based on what the user clicked on.
	SelectionHandleMode_t m_eHandleMode;	// Determines which handles are visible for selected objects.

	CRender2D *m_pRender;		// Cached pointer to the renderer used when calling DrawObject.
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int Selection3D::GetCount()
{
	return m_SelectionList.Count();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapClass *Selection3D::GetObject(int nIndex)
{
	return m_SelectionList.Element(nIndex);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Selection3D::RemoveAll()
{
	m_SelectionList.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current handle mode. The handle mode determines which
//			handles get rendered - the default handles, helper handles, or both.
//-----------------------------------------------------------------------------
SelectionHandleMode_t Selection3D::GetHandleMode()
{
	return m_eHandleMode;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current selection mode. The selection mode determines
//			what gets selected when the user clicks on something - the group,
//			the entity, or the solid.
//-----------------------------------------------------------------------------
SelectMode_t Selection3D::GetSelectMode()
{
	return m_eSelectMode;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the current selection mode. The selection mode determines
//			what gets selected when the user clicks on something - the group,
//			the entity, or the solid.
//-----------------------------------------------------------------------------
void Selection3D::SetSelectMode(SelectMode_t eSelectMode)
{
	m_eSelectMode = eSelectMode;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the bounds of the last selection set. Used by the block tool
//			to start new shapes using the bounds of the previous selection.
//-----------------------------------------------------------------------------
void Selection3D::GetLastValidBounds(Vector &vecMins, Vector &vecMaxs)
{
	vecMins = stat.mins;
	vecMaxs = stat.maxs;
}


#endif // SELECTION3D_H
