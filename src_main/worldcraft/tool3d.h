//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TOOL3D_H
#define TOOL3D_H
#ifdef _WIN32
#pragma once
#endif


#include "Axes2.h"
#include "MapAtom.h"
#include "ToolInterface.h"


class CMapDoc;


#define inrange(a,minv,maxv)	((a) >= (minv) && (a) <= (maxv))
#pragma warning(disable: 4244)


class Tool3D : public Axes2, public CBaseTool
{
public:

	Tool3D(void);

	// Called by CMapView2D to set up the view mapping.
	virtual void SetAxesZoom(int nHorz, bool bInvertH, int nVert, bool bInvertV, float flSetZoom);

	// Called by CMapDoc's constructor to initialize the tool
	void SetDocument(CMapDoc *pDoc) { m_pDocument = pDoc; }

	virtual BOOL IsEmpty(void) = 0;
	virtual void SetEmpty(void) = 0;

protected:

	virtual int HitTest(CPoint pt, BOOL = FALSE) = 0;
    virtual BOOL StartTranslation(CPoint &pt);
	virtual BOOL UpdateTranslation(CPoint pt, UINT = 0, CSize& size = CSize(0,0)) = 0;
	virtual void FinishTranslation(BOOL bSave);

	enum
	{
		constrainHorz = 0x01,
		constrainVert = 0x02,
		constrainNosnap = 0x04,
		constrainHalfSnap = 0x08,
	};

	bool IsTranslating(void) { return m_bTranslating; }

	void PointToScreen(Vector& pt3, POINT& pt2);

	inline void PointToScreen(POINT &pt)
	{
		Axes2::PointToScreen(pt);
		pt.x *= fZoom;
		pt.y *= fZoom;
	}

	inline void PointToMap(POINT &pt)
	{
		Axes2::PointToScreen(pt);
		pt.x /= fZoom;
		pt.y /= fZoom;
	}

	inline void RectToScreen(RECT &rect)
	{
		Axes2::RectToScreen(rect);
		rect.left *= fZoom;
		rect.right *= fZoom;
		rect.top *= fZoom;
		rect.bottom *= fZoom;
	}

	float fZoom;	// zoom value
	CMapDoc *m_pDocument;

	// Axes when StartTranslation was called:
	// FIXME: these should be moved into Box3D, but derived classes don't call through
	//		  Box3D::StartTranslation, so it doesn't work properly
	int m_axFirstHorz;
	int m_axFirstVert;
	CPoint m_ptOrigin;	// Point at which StartTranslation was called.

private:

	bool m_bTranslating;
};

#endif // TOOL3D_H
