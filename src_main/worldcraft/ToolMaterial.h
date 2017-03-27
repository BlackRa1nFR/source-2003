//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TOOLMATERIAL_H
#define TOOLMATERIAL_H
#ifdef _WIN32
#pragma once
#endif


#include "ToolInterface.h"


class CToolMaterial : public CBaseTool
{
	//
	// CBaseTool implementation.
	//
	virtual ToolID_t GetToolID(void) { return TOOL_FACEEDIT_MATERIAL; }

	virtual bool OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point);

	virtual bool OnLMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnRMouseDown3D(CMapView3D *pView, UINT nFlags, CPoint point);
	virtual bool OnMouseMove3D(CMapView3D *pView, UINT nFlags, CPoint point);

	virtual void UpdateStatusBar();
};


#endif // TOOLMATERIAL_H
