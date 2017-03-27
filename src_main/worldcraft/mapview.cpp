//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Default implementation for interface common to 2D and 3D views.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "MapDoc.h"
#include "MapView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#pragma warning(disable:4244 4305)


IMPLEMENT_DYNCREATE(CMapView, CView)


BEGIN_MESSAGE_MAP(CMapView, CView)
	//{{AFX_MSG_MAP(CMapView)
	//}}AFX_MSG_MAP
	// Standard printing commands
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// Purpose: Constructor, sets to inactive.
//-----------------------------------------------------------------------------
CMapView::CMapView(void)
{
	m_bActive = FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nDrawType - 
//-----------------------------------------------------------------------------
void CMapView::SetDrawType(DrawType_t eDrawType)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
DrawType_t CMapView::GetDrawType(void)
{
	return(VIEW_INVALID); // dvs: something better than -1?
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : point - 
//			&ulFace - 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapView::ObjectAt(CPoint point, ULONG &ulFace)
{
	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pDC - 
//-----------------------------------------------------------------------------
void CMapView::OnDraw(CDC *pDC)
{
}
