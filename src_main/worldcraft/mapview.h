//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Defines functionality common to 2D and 3D views.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPVIEW_H
#define MAPVIEW_H
#pragma once

class CMapAtom;
class CMapClass;



enum DrawType_t
{
	VIEW_INVALID = -1,
	VIEW2D_XY = 0,
	VIEW2D_YZ,
	VIEW2D_XZ,
	VIEW3D_WIREFRAME,
	VIEW3D_POLYGON,
	VIEW3D_TEXTURED,
	VIEW3D_LIGHTMAP_GRID,
	VIEW3D_LIGHTING_PREVIEW,
	VIEW3D_SMOOTHING_GROUP
};



class CMapView : public CView
{
	protected:

		CMapView(void);
		DECLARE_DYNCREATE(CMapView)

	public:

		virtual void Activate(BOOL bActivate) { m_bActive = bActivate; }
		inline BOOL IsActive(void) { return(m_bActive); }

		virtual void SetDrawType(DrawType_t eDrawType);
		virtual DrawType_t GetDrawType(void);
		virtual CMapClass *ObjectAt(CPoint point, ULONG &ulFace);
		virtual void RenderPreloadObject(CMapAtom *pObject) { }

		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CMapView3D)
		public:
		virtual void OnDraw(CDC* pDC);  // overridden to draw this view
		//}}AFX_VIRTUAL

		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CMapView3D)
		//}}AFX_VIRTUAL

	protected:

		BOOL m_bActive;

		// Generated message map functions
		//{{AFX_MSG(CMapView3D)
		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()
};


#endif // MAPVIEW_H
