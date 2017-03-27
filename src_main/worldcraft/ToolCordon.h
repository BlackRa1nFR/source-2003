//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the cordon tool. The cordon tool defines a rectangular
//			volume that acts as a visibility filter. Only objects that intersect
//			the cordon are rendered in the views. When saving the MAP file while
//			the cordon tool is active, only brushes that intersect the cordon
//			bounds are saved. The cordon box is replaced by brushes in order to
//			seal the map.
//
// $NoKeywords: $
//=============================================================================

#ifndef CORDON3D_H
#define CORDON3D_H
#pragma once


#include "Box3D.h"
#include "ToolInterface.h"


class CChunkFile;
class CSaveInfo;
class CMapWorld;
class CMapView2D;
class CMapView3D;


enum ChunkFileResult_t;


class Cordon3D : public Box3D
{
	typedef Box3D BaseClass;

	public:

		Cordon3D(void);

		CMapWorld *CreateTempWorld(void);

		ChunkFileResult_t LoadVMF(CChunkFile *pFile);
		ChunkFileResult_t SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo);

		inline bool IsCordonActive(void);
		inline void SetCordonActive(bool bActive);
		inline void EnableEdit(bool bEnable);

		//
		// CBaseTool implementation.
		//
		virtual void OnActivate(ToolID_t eOldTool);
		virtual void OnDeactivate(ToolID_t eNewTool);
		virtual ToolID_t GetToolID(void) { return TOOL_EDITCORDON; }

		virtual bool OnLMouseDown2D(CMapView2D *pView, UINT nFlags, CPoint point);
		virtual bool OnLMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point);
		virtual bool OnMouseMove2D(CMapView2D *pView, UINT nFlags, CPoint point);
		virtual bool OnRMouseUp2D(CMapView2D *pView, UINT nFlags, CPoint point);
		virtual bool OnKeyDown2D(CMapView2D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);
		virtual bool OnKeyDown3D(CMapView3D *pView, UINT nChar, UINT nRepCnt, UINT nFlags);

		virtual void RenderTool2D(CRender2D *pRender);
		virtual void RenderTool3D(CRender3D *pRender);
		
	private:

		void UpdateRenderState(void);
		void OnEscape(void);

		static ChunkFileResult_t LoadCordonKeyCallback(const char *pszKey, const char *pszValue, Cordon3D *pCordon);

		bool m_bCordonActive;	// Whether the cordon is turned on.
		bool m_bEnableEdit;		// Whether the resize handles should be enabled.

		bool m_bLButtonDown;	// True if the left button is down, false if not.
		CPoint m_ptLDownClient;	// Client position at which left mouse button was pressed.
};


//-----------------------------------------------------------------------------
// Purpose: Returns true if the cordon is active, false if not.
//-----------------------------------------------------------------------------
bool Cordon3D::IsCordonActive(void)
{
	return(m_bCordonActive);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the state of the cordon.
//-----------------------------------------------------------------------------
void Cordon3D::SetCordonActive(bool bActive)
{
	m_bCordonActive = bActive;
	if (bActive)
	{
		bEmpty = FALSE;
	}

	UpdateRenderState();
}


//-----------------------------------------------------------------------------
// Purpose: Enables or disables editing of the cordon bounds.
//-----------------------------------------------------------------------------
void Cordon3D::EnableEdit(bool bEnable)
{
	m_bEnableEdit = bEnable;
	UpdateRenderState();
}


#endif // CORDON3D_H
