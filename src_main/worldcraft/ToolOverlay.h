//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OVERLAY3D_H
#define OVERLAY3D_H
#pragma once

#include <afxwin.h>
#include "Box3D.h"
#include "ToolInterface.h"
#include "MapOverlay.h"

class CMapDoc;

class CToolOverlay : public Box3D
{
public:

	//=========================================================================
	//
	// Constructur/Destructor
	//
	CToolOverlay();
	~CToolOverlay();

	//=========================================================================
	//
	// CBaseTool virtual implementations
	//
	ToolID_t	GetToolID( void ) { return TOOL_OVERLAY; }
	BOOL		IsEmpty( void ) { return m_bEmpty; }
	void		SetEmpty( void ) { m_bEmpty = TRUE; }

    void		OnActivate( ToolID_t eOldTool );
    void		OnDeactivate( ToolID_t eOldTool );

	bool		OnLMouseUp3D( CMapView3D *pView, UINT nFlags, CPoint point );
    bool		OnLMouseDown3D( CMapView3D *pView, UINT nFlags, CPoint point );
	bool		OnMouseMove3D( CMapView3D *pView, UINT nFlags, CPoint point );

	bool		OnContextMenu2D( CMapView2D *pView, CPoint point );

protected:

	int			HitTest( CPoint pt, BOOL = FALSE );
	BOOL		UpdateTranslation( CPoint pt, UINT = 0, CSize &size = CSize( 0, 0 ) );

private:

	void		HandlesReset( void );
	bool		HandlesHitTest( CPoint const &pt );
	void		OnDrag( Vector const &vecRayStart, Vector const &vecRayEnd, bool bShift );
	void		ReClip( void );
	void		ReClipAndReCenterEntity( void );
	void		SetupHandleDragUndo( void );
	void		SnapHandle( Vector &vecHandlePt );
	bool		HandleInBBox( CMapOverlay *pOverlay, Vector const &vecHandlePt );
	bool		HandleSnap( CMapOverlay *pOverlay, Vector &vecHandlePt );
	void		UpdatePropertiesBox( void );
	bool		CreateOverlay( CMapSolid *pSolid, ULONG iFace, CMapView3D *pView, CPoint point );
	void		AddInitialFaceToSideList( CMapEntity *pEntity, CMapFace *pFace );
	void		OverlaySelection( CMapView3D *pView, UINT nFlags, CPoint point );

private:

	bool			m_bEmpty;
	bool			m_bDragging;		// Are we dragging overlay handles?
	CMapDoc			*m_pDoc;			// Cached for ease of use - the active document
	CMapOverlay		*m_pActiveOverlay;	// The overlay currently being acted upon
};

#endif // OVERLAY3D_H