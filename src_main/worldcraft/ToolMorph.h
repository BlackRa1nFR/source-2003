//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MORPH3D_H
#define MORPH3D_H
#ifdef _WIN32
#pragma once
#endif

#include "MapClass.h"			// dvs: For CMapObjectList
#include "Box3D.h"
#include "SSolid.h"
#include "Resource.h"
#include "ScaleVerticesDlg.h"
#include "ToolInterface.h"
#include "Vector.h"


class IMesh;
class Morph3D;
class CRender2D;
class CRender3D;


const SSHANDLE SSH_SCALEORIGIN = 0xffff0L;


typedef struct
{
	CMapSolid *pMapSolid;
	CSSolid *pStrucSolid;
	SSHANDLE ssh;
} MORPHHANDLE;


class Morph3D : public Box3D
{
public:

	Morph3D();
	virtual ~Morph3D();

	enum
	{
		scSelect = 0x01,
		scUnselect = 0x02,
		scToggle = 0x04,
		scClear = 0x08,
		scSelectAll = 0x10,
	};

	BOOL IsMorphing(CMapSolid *pSolid, CSSolid **pStrucSolidRvl = NULL);

	BOOL SplitFace();
	BOOL CanSplitFace();

	void SelectHandle(MORPHHANDLE *pInfo, UINT cmd = scSelect);
	int SelectHandle2D(MORPHHANDLE *pInfo, UINT cmd = scSelect);
	void DeselectHandle(MORPHHANDLE *pInfo);

	void MoveSelectedHandles(const Vector &Delta);
	int GetSelectedHandleCount(void) { return m_nSelectedHandles; }
	void GetSelectedCenter(Vector& pt);
	SSHANDLETYPE GetSelectedType() { return m_SelectedType; }
	bool IsSelected(MORPHHANDLE &mh);

	void SelectObject(CMapSolid *pSolid, UINT cmd = scSelect);

	void GetMorphBounds(Vector &mins, Vector &maxs, bool bReset);

	// Toggle mode - vertex & edge, vertex, edge.
	void ToggleMode();

	void OnScaleCmd(BOOL bReInit = FALSE);
	void UpdateScale();
	BOOL IsScaling() { return m_bScaling; }

	void GetMorphingObjects(CUtlVector<CMapClass *> &List);

	inline int GetObjectCount(void);
	inline CSSolid *GetFirstObject(POSITION &pos);
	inline CSSolid *GetNextObject(POSITION &pos);

	//
	// Tool3D implementation.
	//
	virtual BOOL IsEmpty() { return !m_StrucSolids.GetCount() && !m_bBoxSelecting; }
	virtual void SetEmpty();
	virtual void FinishTranslation(BOOL bSave);

	//
	// CBaseTool implementation.
	//
	virtual void OnActivate(ToolID_t eOldTool);
	virtual void OnDeactivate(ToolID_t eNewTool);
	virtual ToolID_t GetToolID(void) { return TOOL_MORPH; }

	virtual bool CanDeactivate( void );

	virtual bool OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnChar2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point);

	virtual bool OnLMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseUp3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnIdle3D(CMapView3D *pView, CPoint &point);
	virtual bool OnMouseMove3D(CMapView3D *pView, UINT nFlags, CPoint point);

	virtual void RenderTool2D(CRender2D *pRender);
	virtual void RenderTool3D(CRender3D *pRender);

private:

	void WorldToScreen2D(CPoint &Screen, const Vector &World);

	void OnEscape(void);

	BOOL MorphHitTest3D(CMapView3D *pView, CPoint pt, MORPHHANDLE *pInfo);
	BOOL MorphHitTest2D(CPoint pt, MORPHHANDLE *pInfo);

	void GetHandlePos(MORPHHANDLE *pInfo, Vector& pt);

	SSHANDLE Get2DMatches(CSSolid *pStrucSolid, SSHANDLEINFO &hi, SSHANDLE *hAddSimilarList = NULL, int *pnAddSimilar = NULL);

	void StartTranslation(MORPHHANDLE *pInfo);

	void RenderSolid3D(CRender3D *pRender, CSSolid *pSolid);
	void RenderMorphHandle(CRender3D *pRender, IMesh *pMesh, const Vector2D &ScreenPos, unsigned char *color);

	//
	// Tool3D implementations.
	//
	int HitTest(CPoint pt, BOOL = FALSE);
	
	void UpdateTranslation(Vector& pNewPos);
	virtual BOOL UpdateTranslation(CPoint pt, UINT uFlags, CSize& dragSize = CSize(0,0));

	BOOL StartBoxSelection(Vector& pt3);
	void SelectInBox();
	void EndBoxSelection();
	BOOL IsBoxSelecting() { return m_bBoxSelecting; }

	BOOL StartTranslation(CPoint &pt);

	bool CanDeselectList( void );

	// list of active Structured Solids:
	CTypedPtrList<CPtrList, CSSolid*> m_StrucSolids;
	
	// list of selected nodes:
	CArray<MORPHHANDLE, MORPHHANDLE&> m_SelectedHandles;
	int m_nSelectedHandles;

	// type of selected handles:
	SSHANDLETYPE m_SelectedType;

	// main morph handle:
	MORPHHANDLE m_MorphHandle;
	Vector m_OrigHandlePos;

	// morph bounds:
	BoundBox m_MorphBounds;
	
	// handle mode:
	enum
	{
		hmBoth = 0x01 | 0x02,
		hmVertex = 0x01,
		hmEdge = 0x02
	};

	int m_LastThirdAxis;
	float m_LastThirdAxisExtents[2];

	bool m_bLButtonDown;		// True if we got a left button down message, false if not.
	CPoint m_ptLDownClient;		// Client position at which left mouse button was pressed.
	bool m_bLButtonDownControlState;

	CPoint m_ptLastMouseMovement;

	bool m_bHit;

	MORPHHANDLE m_DragHandle;	// The morph handle that we are dragging.

	bool m_bMorphing;
	bool m_bMovingSelected;	// not moving them yet - might just select this

	int m_HandleMode;
	bool m_bBoxSelecting;
	bool m_bScaling;
	bool m_bUpdateOrg;
	CScaleVerticesDlg m_ScaleDlg;
	Vector *m_pOrigPosList;
	Vector m_ScaleOrg;
};


//-----------------------------------------------------------------------------
// Purpose: Returns the number of solids selected for morphing.
//-----------------------------------------------------------------------------
inline int Morph3D::GetObjectCount(void)
{
	return(m_StrucSolids.GetCount());
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the selected solids.
//-----------------------------------------------------------------------------
inline CSSolid *Morph3D::GetFirstObject(POSITION &pos)
{
	pos = m_StrucSolids.GetHeadPosition();
	if (pos != NULL)
	{
		return(m_StrucSolids.GetNext(pos));
	}
	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the selected solids.
//-----------------------------------------------------------------------------
inline CSSolid *Morph3D::GetNextObject(POSITION &pos)
{
	if (pos != NULL)
	{
		return(m_StrucSolids.GetNext(pos));
	}
	return(NULL);
}

#endif // MORPH3D_H
