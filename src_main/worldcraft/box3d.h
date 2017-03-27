//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BOX3D_H
#define BOX3D_H
#pragma once


#include "Tool3D.h"
#include "BoundBox.h"


class CMapSolid;
class CRender3D;


//
// Formats for displaying world units.
//
enum WorldUnits_t
{
	Units_None,
	Units_Inches,
	Units_Feet_Inches,
};


class Box3D : public Tool3D, public BoundBox
{

friend CMapSolid;

public:

	Box3D(void);

	void GetRotateAngles(QAngle& pfAngles);
	inline BOOL IsMoving(void) { return(GetTransOrigin() == Box3D::inclMain); }
	inline BOOL IsRotating(void) { return((stat.TranslateMode == modeRotate) && (GetTransOrigin() != Box3D::inclMain)); }
	inline BOOL IsShearing(void) { return((stat.TranslateMode == modeShear) && (GetTransOrigin() != Box3D::inclMain)); }

	void GetMins(Vector &mins) const { mins = stat.mins; }
	void GetMaxs(Vector &maxs) const { maxs = stat.maxs; }

	void TranslatePoint(Vector& pt);

	void Set(Vector &_bmins, Vector &_bmaxs);
	void Set(CPoint pt1, CPoint pt2);

	static inline void SetWorldUnits(WorldUnits_t eWorldUnits);
	static inline WorldUnits_t GetWorldUnits(void);

	//
	// Tool3D implementation.
	//
	virtual BOOL IsEmpty(void);
	virtual void SetEmpty(void);

	//
	// CBaseTool implemenation.
	//
	virtual void RenderTool2D(CRender2D *pRender);
	virtual void RenderTool3D(CRender3D *pRender);

	virtual void UpdateStatusBar();

protected:

	enum
	{
		constrain15 = 0x100,
		constrainCenter = 0x200,
		constrainKeepScale = 0x400
	};

	enum
	{
		expandbox = 0x01,
		newboxnohandles = 0x02,
		thicklines = 0x04,
		boundstext = 0x08,
	};

	enum TransformMode_t
	{
		_modeFirst,
		modeScale,
		modeRotate,
		modeShear,
		_modeLast
	};

	// dvs: This is terrible. Transformation origins are kept in the same field
	//		as the handle grabbed to initiate the transformation. See IsRotating() to
	//		know why this is bad.
	enum TransformHandle_t
	{
		inclNone = -1,
		inclMain,						// main rect
		inclOrigin,						// rotation origin
		inclTop	= 0x10,					// handles [..]
		inclRight = 0x20,
		inclBottom = 0x40,
		inclLeft = 0x80,
		inclTopRight = 0x10 | 0x20,
		inclBottomRight = 0x40 | 0x20,
		inclBottomLeft = 0x40 | 0x80,
		inclTopLeft = 0x10 | 0x80
	};

	void StartNew(const Vector &vecStart);
	void StartNew(const Vector &vecStart, const Vector &mins, const Vector &maxs);

	inline int GetTranslateMode() { return stat.TranslateMode; }
	inline int GetTransOrigin() { return iTransOrigin; }

	void ToggleTranslateMode(void);
	void EnableHandles(bool bEnable);

	void SetDrawFlags(DWORD dwFlags);
	DWORD GetDrawFlags() { return dwDrawFlags; }
	void SetDrawColors(COLORREF dwHandleColor, COLORREF dwBoxColor);

	void GetStatusString(char *pszBuf);

	void UpdateSavedBounds( void );
	const Vector NearestCorner(float x, float y);

	BOOL StartTranslation(CPoint &pt, const Vector *vecForceMoveRef = NULL);

	//
	// Tool3D implementation.
	//
	virtual int HitTest(CPoint pt, BOOL bValidOnly);
	virtual BOOL UpdateTranslation(CPoint pt, UINT uConstraints = 0, CSize& dragSize = CSize(0,0));
	virtual void FinishTranslation(BOOL bSave);

	static WorldUnits_t m_eWorldUnits;

	Vector tbmins;
	Vector tbmaxs;	// used during translation.
	Vector ptMoveRef;
	Vector sizeMoveOfs;

	COLORREF m_clrHandle;
	COLORREF m_clrBox;

	struct BoxStatus_t
	{
		BOOL bTranslating;
		BOOL bNewBox;
		bool bEnableHandles;
		int TranslateMode;
		float fRotateAngle;
		float fStartRotateAngle;
		Vector rotate_org;
		int iTranslateAxis;
		int iShearX, iShearY;

		// last drawn reference point if moving objects:
		Vector ptRef;

		// only used by lastdrawstat:
		Vector mins;
		Vector maxs;
	};

	BoxStatus_t stat;

	int iTransOrigin;

	Vector m_Scale;

	BOOL bPreventOverlap;
	DWORD dwDrawFlags;
	BOOL bEmpty;

private:

	void UpdateCursor(TransformHandle_t eHandleHit, TransformMode_t eTransformMode);
	int FixOrigin(int iOrigin);
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
WorldUnits_t Box3D::GetWorldUnits(void)
{
	return(m_eWorldUnits);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Box3D::SetWorldUnits(WorldUnits_t eWorldUnits)
{
	m_eWorldUnits = eWorldUnits;
}


#endif // BOX3D_H
