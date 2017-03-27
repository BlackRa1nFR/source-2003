//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CLIPPER3D_H
#define CLIPPER3D_H
#ifdef _WIN32
#pragma once
#endif


#include <afxtempl.h>
#include "MapClass.h"		// For CMapObjectList
#include "Tool3D.h"
#include "ToolInterface.h"
#include "Render2D.h"
#include "MapFace.h"


class CMapSolid;


//=============================================================================
//
// CClipGroup
//

class CClipGroup
{
public:
    
    enum { FRONT = 0, BACK };
    
    inline CClipGroup();
    ~CClipGroup();

    inline void SetOrigSolid( CMapSolid *pSolid );
    inline CMapSolid *GetOrigSolid( void );

    inline void SetClipSolid( CMapSolid *pSolid, int side );
    inline CMapSolid *GetClipSolid( int side );

private:

    CMapSolid   *m_pOrigSolid;
    CMapSolid   *m_pClipSolids[2];      // front, back
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline CClipGroup::CClipGroup()
{
    m_pOrigSolid = NULL;
    m_pClipSolids[0] = NULL;
    m_pClipSolids[1] = NULL;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CClipGroup::SetOrigSolid( CMapSolid *pSolid )
{
    m_pOrigSolid = pSolid;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline CMapSolid *CClipGroup::GetOrigSolid( void )
{
    return m_pOrigSolid;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CClipGroup::SetClipSolid( CMapSolid *pSolid, int side )
{
    m_pClipSolids[side] = pSolid;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline CMapSolid *CClipGroup::GetClipSolid( int side )
{
    return m_pClipSolids[side];
}


typedef CTypedPtrList<CPtrList, CClipGroup*> CClipObjectList;


class Clipper3D : public Tool3D
{
    friend BOOL AddToClipList( CMapSolid *pSolid, Clipper3D *pClipper );

public:

    enum { FRONT = 0, BACK, BOTH };
    enum { constrainMoveBoth = 0x100 };

    Clipper3D();
	~Clipper3D();

    void IterateClipMode( void );

	inline void ToggleMeasurements( void );

    //
    // Tool3D implementation.
    //
	virtual BOOL IsEmpty();
	virtual void SetEmpty();
	virtual int HitTest( CPoint pt, BOOL = FALSE );

    //
    // CBaseTool implementation.
    //
	virtual void OnActivate(ToolID_t eOldTool);
	virtual void OnDeactivate(ToolID_t eNewTool);
	virtual ToolID_t GetToolID(void) { return TOOL_CLIPPER; }

	virtual void RenderTool2D(CRender2D *pRender);
	virtual void RenderTool3D(CRender3D *pRender);

	virtual bool OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual bool OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point);
	virtual bool OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point);

	virtual bool OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
	
protected:

    //
	// Tool3D implementation.
	//
	virtual BOOL StartTranslation( CPoint &pt );
	virtual BOOL UpdateTranslation( CPoint pt, UINT uFlags = 0, CSize &dragSize = CSize( 0, 0 ) );
	virtual void FinishTranslation( BOOL bSave );

private:

	void OnEscape(void);

	void StartNew( const Vector &vecStart );

    void SetClipObjects( CMapObjectList *pList );
    void SetClipPlane( PLANE *pPlane );

    void SetClipPoint( int index, CPoint pt );
    bool CompareClipPoint( int index, CPoint pt );
    void BuildClipPlane( void );

    void SaveClipResults( void );
    void GetClipResults( void );
    void CalcClipResults( void );
    void ResetClipResults( void );

	void RemoveOrigSolid( CMapSolid *pOrigSolid );
	void SaveClipSolid( CMapSolid *pSolid, CMapSolid *pOrigSolid );

    void DrawBrushExtents(CRender2D *pRender, CMapSolid *pSolid, int nFlags);

    int             m_Mode;                 // current clipping mode { back, front, both }
    
    PLANE           m_ClipPlane;            // the clipping plane -- front/back is uneccesary
    Vector          m_ClipPoints[2];        // 2D clipping points -- used to create the clip plane
    int             m_ClipPointHit;         // the clipping that was "hit" {0, 1, -1}

    CMapObjectList  *m_pOrigObjects;        // list of the initial objects to clip
    CClipObjectList m_ClipResults;          // list of clipped objects

	BOOL            m_bEmpty;               // any objects to clip?
	bool            m_bDrawMeasurements;	// Whether to draw brush dimensions in the 2D view.

	CRender2D		m_Render2D;				// 2d renderer

	bool m_bLButtonDown;					// True if the left button is down, false if not.
	CPoint m_ptLDownClient;					// Client pos at which lbutton was pressed.
};


//-----------------------------------------------------------------------------
// Purpose: Toggles the clipper's rendering of brush measurements in the 2D view.
//-----------------------------------------------------------------------------
inline void Clipper3D::ToggleMeasurements( void )
{
	m_bDrawMeasurements = !m_bDrawMeasurements;
}


#endif // CLIPPER3D_H
