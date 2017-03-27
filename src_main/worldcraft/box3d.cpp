//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "Box3D.h"
#include "GlobalFunctions.h"
#include "MapDoc.h"
#include "Options.h"
#include "Render2D.h"
#include "Render3D.h"
#include "RenderUtils.h"
#include "resource.h"
#include "StatusBarIDs.h"
#include "worldcraft_mathlib.h"


#pragma warning(disable:4244)


const int HANDLEOFFSET = 6;
const int HANDLEDIAMETER = 8;
const int HANDLERADIUS = 4;


extern float g_MAX_MAP_COORD; // dvs: move these into Globals.h!!
extern float g_MIN_MAP_COORD; // dvs: move these into Globals.h!!


WorldUnits_t Box3D::m_eWorldUnits = Units_None;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Box3D::Box3D(void)
{
	SetEmpty();
	SetDrawFlags(0);
	stat.TranslateMode = modeScale;
	stat.bEnableHandles = true;
	SetDrawColors(Options.colors.clrToolHandle, Options.colors.clrToolBlock);

	m_ptOrigin.x = 0;
	m_ptOrigin.y = 0;

	stat.mins = vec3_origin;
	stat.maxs = Vector(64, 64, 64);

	axHorz = 0;
	axVert = 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iOrigin - 
// Output : int
//-----------------------------------------------------------------------------
int Box3D::FixOrigin(int iOrigin)
{
	// reverse origin if either of the axis reversal flags is on
	if (bInvertVert)
	{
		if(iOrigin & inclTop)
		{
			iOrigin &= ~inclTop;
			iOrigin |= inclBottom;
		}
		else if(iOrigin & inclBottom)
		{
			iOrigin &= ~inclBottom;
			iOrigin |= inclTop;
		}
	}

	if(bInvertHorz)
	{
		if(iOrigin & inclLeft)
		{
			iOrigin &= ~inclLeft;
			iOrigin |= inclRight;
		}
		else if(iOrigin & inclRight)
		{
			iOrigin &= ~inclRight;
			iOrigin |= inclLeft;
		}
	}

	return iOrigin;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			bValidOnly - 
// Output : Returns the handle under the given point, -1 if there is none.
//-----------------------------------------------------------------------------
int Box3D::HitTest(CPoint pt, BOOL bValidOnly)
{
	int nHandleHit = inclNone;

	// Build a rect from our bounds to hit test against.
	CRect rCheck(bmins[axHorz], bmins[axVert], bmaxs[axHorz], bmaxs[axVert]);

	// Convert click point to map coordinates:
	vec_t clickX, clickY;
	clickX = pt.x;
	clickY = pt.y;
	clickX /= fZoom;
	clickY /= fZoom;
	if (bInvertHorz)
		clickX = -clickX;
	if (bInvertVert)
		clickY = -clickY;
		
	RectToScreen(rCheck);
	rCheck.NormalizeRect();

	// check inclusion in main rect:
	if (rCheck.PtInRect(pt))
	{
		// The point is inside the main rect.
		nHandleHit = inclMain;
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEALL));
	}

	if (!stat.bEnableHandles)
	{
		// Handles are turned off, so we don't need to do any more testing.
		// Return whether we hit the main rect or not.
		return nHandleHit;
	}

	//
	// Hit test against all the handles.
	//
	float fCheckRadius = (float)HANDLERADIUS / fZoom;
	float rhTop, rhBottom, rhLeft, rhRight;
	Vector ptCheck;
	Vector delta;

	// Use map coordinates now
	delta[axHorz] = (bmaxs[axHorz] - bmins[axHorz])/2.0f;
	delta[axVert] = (bmaxs[axVert] - bmins[axVert])/2.0f;

	int x, y;
	BOOL bFound = FALSE;

	// matrix of handle definitions:
	int HandleDefs[3][3] = 
	{
		{ inclTopLeft,		inclTop,		inclTopRight },
		{ inclLeft,			-1,				inclRight },
		{ inclBottomLeft,	inclBottom,		inclBottomRight }
	};

	float deltaXStep = delta[axHorz];
	float deltaYStep = delta[axVert];

	float fXAdjust;
	float fYAdjust;

	//Adjust for handle offset from BBOX
	fXAdjust = (float)HANDLEOFFSET/fZoom;
	fYAdjust = (float)HANDLEOFFSET/fZoom;

	ptCheck[1] = bmins[axVert];

	float fDist;
	float fBestDistance;
	fBestDistance = 1000000.0f;
	int nX, nY;

	for (y = 0; y < 3; y++, ptCheck[1] += deltaYStep)
	{
		rhTop    = ptCheck[1] + (y - 1)*fYAdjust - fCheckRadius;
		rhBottom = ptCheck[1] + (y - 1)*fYAdjust + fCheckRadius;

		if (clickY < rhTop)
			break;
		
		if (clickY > rhBottom)
			continue;

		ptCheck[0] = bmins[axHorz];

		for (x = 0; x < 3; x++, ptCheck[0] += deltaXStep)
		{
			if (x == 1 && y == 1)	// no dead center
				continue;

			rhLeft  = ptCheck[0] + (x - 1)*fXAdjust - fCheckRadius;
			rhRight = ptCheck[0] + (x - 1)*fXAdjust + fCheckRadius;

			if (clickX >= rhLeft && clickX <= rhRight &&
				clickY >= rhTop  && clickY <= rhBottom)
			{
				// Calculate distance
				fDist =  (ptCheck[0] - clickX)*(ptCheck[0] - clickX);
				fDist += (ptCheck[1] - clickY)*(ptCheck[1] - clickY);

				if (fDist < fBestDistance)
				{
					bFound = TRUE;
					fBestDistance = fDist;
					nX = x;
					nY = y;
				}
			}
		}
	}

	if (!bFound)
	{
		return nHandleHit;
	}

	int xAdj, yAdj;
	xAdj = (!bInvertHorz) ? nX : (2 - nX);
	yAdj = (!bInvertVert) ? nY : (2 - nY);
	nHandleHit = FixOrigin(HandleDefs[yAdj][xAdj]);

	if(bValidOnly)
	{
		switch(stat.TranslateMode)
		{
		case modeScale:
			// all ok
			break;
		case modeRotate:
			// corners ok
			if(nHandleHit == inclTopLeft || nHandleHit == inclTopRight || 
				nHandleHit == inclBottomLeft || nHandleHit == inclBottomRight ||
				nHandleHit == inclMain)
				break;
			return -1;
		case modeShear:
			// sides ok
			if(nHandleHit == inclTop || nHandleHit == inclRight || 
				nHandleHit == inclBottom || nHandleHit == inclLeft ||
				nHandleHit == inclMain)
			   break;
			return -1;
		}
	}

	UpdateCursor((TransformHandle_t)nHandleHit, (TransformMode_t)stat.TranslateMode);

	return nHandleHit;
}


//-----------------------------------------------------------------------------
// Purpose: Set the cursor based on the hit test results and current translate mode.
// Input  : eHandleHit - The handle that the cursor is over.
//			eTransformMode - The current transform mode of the tool - scale, rotate, or shear.
//-----------------------------------------------------------------------------
void Box3D::UpdateCursor(TransformHandle_t eHandleHit, TransformMode_t eTransformMode)
{
	if (eHandleHit == inclMain)
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEALL));
	}
	else if (eTransformMode == modeRotate)
	{
		SetCursor(AfxGetApp()->LoadCursor(IDC_ROTATE));
	}
	else if (eTransformMode == modeScale)
	{
		switch (FixOrigin(eHandleHit))
		{
			case inclTopLeft:
			case inclBottomRight:
			{
				SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENWSE));
				break;
			}

			case inclTopRight:
			case inclBottomLeft:
			{
				SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENESW));
				break;
			}

			case inclRight:
			case inclLeft:
			{
				SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
				break;
			}

			case inclTop:
			case inclBottom:
			{
				SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
				break;
			}
		}
	}
	else if (eTransformMode == modeShear)
	{
		switch (FixOrigin(eHandleHit))
		{
			case inclRight:
			case inclLeft:
			{
				SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
				break;
			}

			case inclTop:
			case inclBottom:
			{
				SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
				break;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : x1 - 
//			y1 - 
//			x2 - 
//			y2 - 
//-----------------------------------------------------------------------------
static float dist(float x1, float y1, float x2, float y2)
{
	return _hypot(fabs(x2-x1), fabs(y2-y1));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEnable - 
//-----------------------------------------------------------------------------
void Box3D::EnableHandles(bool bEnable)
{
	stat.bEnableHandles = bEnable;
}


//-----------------------------------------------------------------------------
// Purpose: Finds the corner nearest to a given point in world coordinates.
// Output : Returns the corner in world coordinates (axThird is always 0).
//-----------------------------------------------------------------------------
const Vector Box3D::NearestCorner(float x, float y)
{
	Vector vecCorner;

	float lowdist = dist(bmins[axHorz], bmins[axVert], x, y);
	vecCorner[axHorz] = bmins[axHorz]; 
	vecCorner[axVert] = bmins[axVert];
	vecCorner[axThird] = 0;

	float d = dist(bmaxs[axHorz], bmins[axVert], x, y);
	if (d < lowdist)
	{
		lowdist = d;
		vecCorner[axHorz] = bmaxs[axHorz]; 
		vecCorner[axVert] = bmins[axVert];
	}

	d = dist(bmaxs[axHorz], bmaxs[axVert], x, y);
	if (d < lowdist)
	{
		lowdist = d;
		vecCorner[axHorz] = bmaxs[axHorz]; 
		vecCorner[axVert] = bmaxs[axVert];
	}

	d = dist(bmins[axHorz], bmaxs[axVert], x, y);
	if (d < lowdist)
	{
		lowdist = d;
		vecCorner[axHorz] = bmins[axHorz]; 
		vecCorner[axVert] = bmaxs[axVert];
	}

	return vecCorner;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			ptForceMoveRef - 
// Output : Returns TRUE if pt hits a handle or is in box area, FALSE otherwise.
//-----------------------------------------------------------------------------
BOOL Box3D::StartTranslation(CPoint &pt, const Vector *ptForceMoveRef)
{
	int nHandleHit = HitTest(pt, TRUE);
	
	if (nHandleHit == inclNone)
	{
		return FALSE;
	}

	// Convert click point to map coordinates:
	vec_t clickX, clickY;
	clickX = pt.x;
	clickY = pt.y;

	// For normal processing
	PointToMap(pt);

	clickX /= fZoom;
	clickY /= fZoom;
	if (bInvertHorz)
		clickX = -clickX;
	if (bInvertVert)
		clickY = -clickY;

	iTransOrigin = nHandleHit;

	tbmins = bmins;
	tbmaxs = bmaxs;

	ZeroVector(m_Scale);

	bPreventOverlap = TRUE;
	stat.bNewBox = FALSE;
	stat.iTranslateAxis = axThird;

	if (stat.TranslateMode == modeRotate)
	{
		//
		// The rotation center is the center of our box, unless one is forced upon us.
		//
		if (ptForceMoveRef == NULL)
		{
			stat.rotate_org = (bmins + bmaxs) / 2;
		}
		else
		{
			stat.rotate_org = *ptForceMoveRef;
		}

		stat.fStartRotateAngle = fixang(lineangle(stat.rotate_org[axHorz], stat.rotate_org[axVert], pt.x, pt.y));
	}

	if (iTransOrigin == inclMain)
	{
		//
		// Find a reference point for the drag. Pick the corner of the bounds
		// nearest to the cursor.
		//
		if (ptForceMoveRef == NULL)
		{
			ptMoveRef = NearestCorner(clickX, clickY);
		}
		else
		{
			ptMoveRef = *ptForceMoveRef;
			ptMoveRef[axThird] = 0;
		}

		//
		// Calculate the drag offset and new bounds.
		//
		sizeMoveOfs[axHorz] = pt.x - ptMoveRef[axHorz];
		sizeMoveOfs[axVert] = pt.y - ptMoveRef[axVert];
		sizeMoveOfs[axThird] = 0;

		tbmins = bmins - ptMoveRef;
		tbmaxs = bmaxs - ptMoveRef;
	}

	Tool3D::StartTranslation(pt);

	UpdateSavedBounds();

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &r - 
//			angle - 
// Output : static void
//-----------------------------------------------------------------------------
static void RectRotate(CRect &r, double angle)
{
	float x, y;
	rotate_coords(&x, &y, r.left, r.top, angle);
	r.left = x; r.top = y;
	rotate_coords(&x, &y, r.right, r.top, angle);
	r.right = x; r.top = y;
	rotate_coords(&x, &y, r.right, r.bottom, angle);
	r.right = x; r.bottom = y;
	rotate_coords(&x, &y, r.left, r.bottom, angle);
	r.left = x; r.bottom = y;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszBuf - 
//-----------------------------------------------------------------------------
void Box3D::GetStatusString(char *pszBuf)
{
	*pszBuf = '\0';

	if (!IsTranslating())
	{
		if (!IsEmpty())
		{
			Vector bsize = bmaxs - bmins;

			switch (m_eWorldUnits)
			{
				case Units_None:
				{
					sprintf(pszBuf, " %dw %dl %dh", (int)fabs(bsize[0]), (int)fabs(bsize[1]), (int)fabs(bsize[2]));
					break;
				}

				case Units_Inches:
				{
					sprintf(pszBuf, " %d\"w %d\"l %d\"h", (int)fabs(bsize[0]), (int)fabs(bsize[1]), (int)fabs(bsize[2]));
					break;
				}

				case Units_Feet_Inches:
				{
					int nFeetWide = (int)fabs(bsize[0]) / 12;
					int nInchesWide = (int)fabs(bsize[0]) % 12;

					int nFeetLong = (int)fabs(bsize[1]) / 12;
					int nInchesLong = (int)fabs(bsize[1]) % 12;

					int nFeetHigh = (int)fabs(bsize[2]) / 12;
					int nInchesHigh = (int)fabs(bsize[2]) % 12;

					sprintf(pszBuf, " %d' %d\"w %d' %d\"l %d' %d\"h", nFeetWide, nInchesWide, nFeetLong, nInchesLong, nFeetHigh, nInchesHigh);
					break;
				}
			}
		}
	}
	else if (iTransOrigin == inclMain)
	{
		return;
	}
	else
	{
		switch (stat.TranslateMode)
		{
			case modeScale:
			{
				Vector tbsize = tbmaxs - tbmins;

				switch (GetWorldUnits())
				{
					case Units_None:
					{
						sprintf(pszBuf, " %d x %d ", (int)fabs(tbsize[axHorz]), (int)fabs(tbsize[axVert]));
						break;
					}

					case Units_Inches:
					{
						sprintf(pszBuf, " %d\" x %d\" ", (int)fabs(tbsize[axHorz]), (int)fabs(tbsize[axVert]));
						break;
					}

					case Units_Feet_Inches:
					{
						int nFeetWide = (int)fabs(tbsize[axHorz]) / 12;
						int nInchesWide = (int)fabs(tbsize[axHorz]) % 12;

						int nFeetHigh = (int)fabs(tbsize[axVert]) / 12;
						int nInchesHigh = (int)fabs(tbsize[axVert]) % 12;

						sprintf(pszBuf, " %d' %d\" x %d' %d\" ", nFeetWide, nInchesWide, nFeetHigh, nInchesHigh);
						break;
					}
				}
				break;
			}
			
			case modeShear:
			{
				if (iTransOrigin == inclTop || iTransOrigin == inclBottom)
				{
					sprintf(pszBuf, " shear: %d ", int(stat.iShearX));
				}
				else
				{
					sprintf(pszBuf, " shear: %d ", int(stat.iShearY));
				}
				break;
			}

			case modeRotate:
			{
				sprintf(pszBuf, " %.2f%c", stat.fRotateAngle, 0xF8);
				break;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Box3D::UpdateStatusBar()
{
	char szBuf[MAX_PATH];
	GetStatusString(szBuf);
	SetStatusText(SBI_SIZE, szBuf);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt - 
//			uConstraints - 
//			dragSize - 
// Output : 
//-----------------------------------------------------------------------------
BOOL Box3D::UpdateTranslation(CPoint pt, UINT uConstraints, CSize &dragSize)
{
	if (iTransOrigin == inclNone)
	{
		return FALSE;
	}
	else if (iTransOrigin == inclMain)
	{
		goto MoveObjects;
	}

	if (stat.TranslateMode == modeRotate)
	{
		// find rotation angle & return
		stat.fRotateAngle = fixang(lineangle(stat.rotate_org[axHorz], stat.rotate_org[axVert], pt.x, pt.y));
		stat.fRotateAngle -= stat.fStartRotateAngle;
		stat.fRotateAngle = fixang(stat.fRotateAngle);

		if (bInvertHorz + bInvertVert == 1)
		{
			stat.fRotateAngle = fixang(360.0 - stat.fRotateAngle);
		}

		stat.fRotateAngle -= fmod(double(stat.fRotateAngle), double(.5));

		if (uConstraints & constrain15)
		{
			stat.fRotateAngle -= fmod(double(stat.fRotateAngle), double(15.0));
		}

		return TRUE;
	}

	if (stat.TranslateMode == modeShear)
	{
		switch(iTransOrigin)
		{
		case inclTop:
		case inclBottom:
			stat.iShearX = m_pDocument->Snap(pt.x - m_ptOrigin.x);
			stat.iShearY = bmaxs[axVert] - bmins[axVert];
			break;
		case inclLeft:
		case inclRight:
			stat.iShearX = bmaxs[axHorz] - bmins[axHorz];
			stat.iShearY = m_pDocument->Snap(pt.y - m_ptOrigin.y);
			break;
		}

		return TRUE;
	}

MoveObjects:
	// scaling / moving
	m_Scale[axThird] = 1.0;
	BOOL bNeedsUpdate = FALSE;

	bool bSnapToGrid = !(uConstraints & constrainNosnap);

	int nSnapFlags = 0;
	if (uConstraints & constrainHalfSnap)
	{
		nSnapFlags |= SNAP_HALF_GRID;
	}

	if (iTransOrigin == inclMain)
	{
		Vector vecCompare = ptMoveRef;

		ptMoveRef[axHorz] = pt.x - sizeMoveOfs[axHorz];
		ptMoveRef[axVert] = pt.y - sizeMoveOfs[axVert];

		if (bSnapToGrid)
		{
			m_pDocument->Snap(ptMoveRef, nSnapFlags);
		}

		m_Scale[axHorz] = (tbmins[axHorz] + ptMoveRef[axHorz]) - bmins[axHorz];
		m_Scale[axVert] = (tbmins[axVert] + ptMoveRef[axVert]) - bmins[axVert];
		m_Scale[axThird] = 0.0f;

		bNeedsUpdate = !CompareVectors2D(vecCompare, ptMoveRef);

		if (!bNeedsUpdate)
		{
			if (dragSize.cx >= 4 || dragSize.cy >= 4)
				bNeedsUpdate = TRUE;
		}
	}
	else 
	{
		// update bounding box
		if (iTransOrigin & inclTop)
		{
			float fOld = tbmins[axVert];

			tbmins[axVert] = bmins[axVert] - (m_ptOrigin.y - pt.y);
			if (bSnapToGrid)
			{
				tbmins[axVert] = m_pDocument->Snap(tbmins[axVert]);
			}

			if(tbmins[axVert] > (tbmaxs[axVert] - 1) && bPreventOverlap)
				tbmins[axVert] = tbmaxs[axVert] - 1;
			if(fOld != tbmins[axVert])
				bNeedsUpdate = TRUE;
			m_Scale[axVert] = double(tbmaxs[axVert] - tbmins[axVert]) /
				double(bmaxs[axVert] - bmins[axVert]);
		}
		else if(iTransOrigin & inclBottom)
		{
			float fOld = tbmaxs[axVert];
			tbmaxs[axVert] = bmaxs[axVert] + (pt.y - m_ptOrigin.y);
			if (bSnapToGrid)
			{
				tbmaxs[axVert] = m_pDocument->Snap(tbmaxs[axVert]);
			}

			if(tbmaxs[axVert] < (tbmins[axVert] + 1) && bPreventOverlap)
				tbmaxs[axVert] = tbmins[axVert] + 1;
			if(fOld != tbmaxs[axVert])
				bNeedsUpdate = TRUE;
			m_Scale[axVert] = double(tbmaxs[axVert] - tbmins[axVert]) /
				double(bmaxs[axVert] - bmins[axVert]);
		}
		if(iTransOrigin & inclLeft)
		{
			float fOld = tbmins[axHorz];
			tbmins[axHorz] = bmins[axHorz] - (m_ptOrigin.x - pt.x);
			if (bSnapToGrid)
			{
				tbmins[axHorz] = m_pDocument->Snap(tbmins[axHorz]);
			}

			if(tbmins[axHorz] > (tbmaxs[axHorz] - 1) && bPreventOverlap)
				tbmins[axHorz] = tbmaxs[axHorz] - 1;
			if(fOld != tbmins[axHorz])
				bNeedsUpdate = TRUE;
			m_Scale[axHorz] = double(tbmaxs[axHorz] - tbmins[axHorz]) /
				double(bmaxs[axHorz] - bmins[axHorz]);
		}

		else if(iTransOrigin & inclRight)
		{
			float fOld = tbmaxs[axHorz];
			
			tbmaxs[axHorz] = bmaxs[axHorz] + (pt.x - m_ptOrigin.x);
			if (bSnapToGrid)
			{
				tbmaxs[axHorz] = m_pDocument->Snap(tbmaxs[axHorz]);
			}
			
			if(tbmaxs[axHorz] < (tbmins[axHorz] + 1) && bPreventOverlap)
				tbmaxs[axHorz] = tbmins[axHorz] + 1;
			if(fOld != tbmaxs[axHorz])
				bNeedsUpdate = TRUE;
			m_Scale[axHorz] = double(tbmaxs[axHorz] - tbmins[axHorz]) /
				double(bmaxs[axHorz] - bmins[axHorz]);
		}
	}

	//
	// Constrain to valid map boundaries.
	//
	for (int nDim = 0; nDim < 3; nDim++)
	{
		if (tbmaxs[nDim] < g_MIN_MAP_COORD)
		{
			tbmaxs[nDim] = g_MIN_MAP_COORD;
		}
		else if (tbmaxs[nDim] > g_MAX_MAP_COORD)
		{
			tbmaxs[nDim] = g_MAX_MAP_COORD;
		}

		if (tbmins[nDim] < g_MIN_MAP_COORD)
		{
			tbmins[nDim] = g_MIN_MAP_COORD;
		}
		else if (tbmins[nDim] > g_MAX_MAP_COORD)
		{
			tbmins[nDim] = g_MAX_MAP_COORD;
		}
	}

	UpdateSavedBounds();

	return bNeedsUpdate;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dwHandleColor - 
//			dwBoxColor - 
//-----------------------------------------------------------------------------
void Box3D::SetDrawColors(COLORREF dwHandleColor, COLORREF dwBoxColor)
{
	if (dwHandleColor != 0xffffffff)
	{
		m_clrHandle = dwHandleColor;
	}

	if (dwBoxColor != 0xffffffff)
	{
		m_clrBox = dwBoxColor;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the angles of rotation around the X, Y, and Z axes.
// Input  : Angles - Receives rotation angles in degrees, as follows:
//				x - Counterclockwise rotation around the X axis.
//				y - Clockwise rotation around the Y axis.
//				z - Counterclockwise rotation around the Z axis.
//-----------------------------------------------------------------------------
void Box3D::GetRotateAngles(QAngle &Angles)
{
	Angles[0] = 0;
	Angles[1] = 0;
	Angles[2] = 0;

	if (stat.TranslateMode == modeRotate)
	{
		float fAngle = stat.fRotateAngle;
		if (bInvertHorz + bInvertVert == 1)
		{
			fAngle = fixang(360.0 - fAngle);
		}

		if (!((m_axFirstHorz + m_axFirstVert) & 0x01))
		{
			Angles[1] = fAngle;
		}
		else if (!((m_axFirstHorz + m_axFirstVert) & 0x02))
		{
			Angles[2] = fAngle;
		}
		else
		{
			Angles[0] = fAngle;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pt - 
//-----------------------------------------------------------------------------
void Box3D::TranslatePoint(Vector& pt)
{
	float & ptHorz = pt[m_axFirstHorz];
	float & ptVert = pt[m_axFirstVert];

	if(iTransOrigin == inclMain)
		goto MoveObjects;

	if(stat.TranslateMode == modeRotate)
	{
		float fX, fY;

		float fAngle = stat.fRotateAngle;
		if(bInvertHorz + bInvertVert == 1)
			fAngle = fixang(360.0 - fAngle);
		rotate_coords(&fX, &fY, ptHorz - stat.rotate_org[m_axFirstHorz], 
			ptVert - stat.rotate_org[m_axFirstVert], fAngle);
		ptHorz = fX + stat.rotate_org[m_axFirstHorz];
		ptVert = fY + stat.rotate_org[m_axFirstVert];
		return;
	}

	if(stat.TranslateMode == modeShear)
	{
		// y = mx+b
		// x = (y - b) / m

		if(iTransOrigin == inclTop)
		{
			float fSlope = (bmaxs[m_axFirstVert] - bmins[m_axFirstVert]) / stat.iShearX;
			ptHorz += (bmaxs[m_axFirstVert] - ptVert) / fSlope;
		}
		else if(iTransOrigin == inclBottom)
		{
			float fSlope = (bmaxs[m_axFirstVert] - bmins[m_axFirstVert]) / stat.iShearX;
			ptHorz += (ptVert - bmins[m_axFirstVert]) / fSlope;
		}
		else if(iTransOrigin == inclLeft)
		{
			float fSlope = (stat.iShearY / (bmaxs[m_axFirstHorz] - bmins[m_axFirstHorz]));
			ptVert += fSlope * (bmaxs[m_axFirstHorz] - ptHorz);
		}
		else if(iTransOrigin == inclRight)
		{
			float fSlope = (stat.iShearY / (bmaxs[m_axFirstHorz] - bmins[m_axFirstHorz]));
			ptVert += fSlope * (ptHorz - bmins[m_axFirstHorz]);
		}

		return;
	}

MoveObjects:
	Vector *pMins, *pMaxs;
	
	if (IsTranslating())
	{
		pMins = &tbmins;
		pMaxs = &tbmaxs;
	}
	else
	{
		pMins = &bmins;
		pMaxs = &bmaxs;
	}

	if(iTransOrigin == inclMain)
	{
		ptHorz += m_Scale[m_axFirstHorz];
		ptVert += m_Scale[m_axFirstVert];
		return;
	}

	double fX, fY;

	if (iTransOrigin & inclLeft)
	{
		fX = (*pMaxs)[m_axFirstHorz] - ptHorz;
		ptHorz = (*pMaxs)[m_axFirstHorz] - (fX * m_Scale[m_axFirstHorz]);
	}
	else if (iTransOrigin & inclRight)
	{
		fX = ptHorz - (*pMins)[m_axFirstHorz];
		ptHorz = (*pMins)[m_axFirstHorz] + (fX * m_Scale[m_axFirstHorz]);
	}

	if (iTransOrigin & inclTop)
	{
		fY = (*pMaxs)[m_axFirstVert] - ptVert;
		ptVert = (*pMaxs)[m_axFirstVert] - (fY * m_Scale[m_axFirstVert]);
	}
	else if (iTransOrigin & inclBottom)
	{
		fY = ptVert - (*pMins)[m_axFirstVert];
		ptVert = (*pMins)[m_axFirstVert] + (fY * m_Scale[m_axFirstVert]);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bSave - 
//-----------------------------------------------------------------------------
void Box3D::FinishTranslation(BOOL bSave)
{
	if(bSave)
	{
		if(iTransOrigin == inclMain)
		{
			// tbmins are offsets from ptmoveref when moving stuff
			// (== inclmain)
			tbmins[axHorz] += ptMoveRef[axHorz];
			tbmins[axVert] += ptMoveRef[axVert];
			tbmaxs[axHorz] += ptMoveRef[axHorz];
			tbmaxs[axVert] += ptMoveRef[axVert];
		}

		bmins = tbmins;
		bmaxs = tbmaxs;

		//
		// Normalize bmins & bmaxs. Although technically we should not need
		// to normalize the axThird mins and maxs, sometimes they are swapped,
		// which creates an invalid selection region and breaks 2D selection.
		// It would be nice to know why that sometimes happens.
		//
		float fTemp;

		if (bmins[axHorz] > bmaxs[axHorz])
		{
			fTemp = bmins[axHorz];
			bmins[axHorz] = bmaxs[axHorz];
			bmaxs[axHorz] = fTemp;
		}

		if (bmins[axVert] > bmaxs[axVert])
		{
			fTemp = bmins[axVert];
			bmins[axVert] = bmaxs[axVert];
			bmaxs[axVert] = fTemp;
		}

		if(bmins[axThird] > bmaxs[axThird])
		{
			fTemp = bmins[axThird];
			bmins[axThird] = bmaxs[axThird];
			bmaxs[axThird] = fTemp;
		}

		bEmpty = FALSE;
	}

	stat.bNewBox = FALSE;
	Tool3D::FinishTranslation(bSave);

	UpdateSavedBounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Box3D::ToggleTranslateMode(void)
{
	stat.TranslateMode++;
	if(stat.TranslateMode == _modeLast)
		stat.TranslateMode = _modeFirst + 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : _bmins[3] - 
//			_bmaxs[3] - 
//-----------------------------------------------------------------------------
void Box3D::Set(Vector& _bmins, Vector& _bmaxs)
{
	if (IsTranslating())
	{
		tbmins = _bmins;
		tbmaxs = _bmaxs;
	}
	else
	{
		bmins = _bmins;
		bmaxs = _bmaxs;
	}

	UpdateSavedBounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL Box3D::IsEmpty(void)
{
	return bEmpty;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Box3D::SetEmpty(void)
{
	bEmpty = TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dwFlags - 
//-----------------------------------------------------------------------------
void Box3D::SetDrawFlags(DWORD dwFlags)
{
	this->dwDrawFlags = dwFlags;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pDC - 
//			bounds - 
//-----------------------------------------------------------------------------
void Box3D::RenderTool2D(CRender2D *pRender)
{
	Vector *pMins, *pMaxs;
	CRect r;

	stat.bTranslating = IsTranslating();

	if (IsTranslating())
	{
		if (iTransOrigin == inclMain)
		{
			static Vector moveMins, moveMaxs;
			moveMins[axHorz] = tbmins[axHorz] + ptMoveRef[axHorz];
			moveMins[axVert] = tbmins[axVert] + ptMoveRef[axVert];
			moveMaxs[axHorz] = tbmaxs[axHorz] + ptMoveRef[axHorz];
			moveMaxs[axVert] = tbmaxs[axVert] + ptMoveRef[axVert];
			pMins = &moveMins;
			pMaxs = &moveMaxs;

			stat.ptRef = ptMoveRef;
		}
		else
		{
			pMins = &tbmins;
			pMaxs = &tbmaxs;
		}
	}
	else
	{
		if (IsEmpty())
		{
			return;
		}

		pMins = &bmins;
		pMaxs = &bmaxs;
	}

	if ((iTransOrigin != inclMain) &&
		(stat.TranslateMode == modeRotate || stat.TranslateMode == modeShear) &&
		stat.bTranslating && axThird != stat.iTranslateAxis)
	{
		return;
	}

	POINT pt1, pt2;
	PointToScreen(*pMins, pt1);
	PointToScreen(*pMaxs, pt2);
	r.SetRect(pt1.x, pt1.y, pt2.x, pt2.y);
	r.NormalizeRect();
	r.bottom += 1;
	r.right += 1;

	if (dwDrawFlags & boundstext)
	{
		DrawBoundsText(pRender, *pMins, *pMaxs, DBT_TOP | DBT_LEFT);
	}

	if (stat.bTranslating)
	{
	    pRender->SetLineType(CRender2D::LINE_DOT, CRender2D::LINE_THIN, GetRValue(Options.colors.clrToolDrag), GetGValue(Options.colors.clrToolDrag), GetBValue(Options.colors.clrToolDrag));

		if (iTransOrigin == inclMain || stat.TranslateMode == modeScale)
		{
			pRender->MoveTo(r.left, r.top);
			pRender->LineTo(r.right, r.top);
			pRender->LineTo(r.right, r.bottom);
			pRender->LineTo(r.left, r.bottom);
			pRender->LineTo(r.left, r.top);

			// draw ptref?
			if (axThird == stat.iTranslateAxis && iTransOrigin == inclMain)
			{
				CPoint pt;
				PointToScreen(stat.ptRef, pt);

			    pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, GetRValue(Options.colors.clrToolDrag), GetGValue(Options.colors.clrToolDrag), GetBValue(Options.colors.clrToolDrag));

				// draw 'X'
				pRender->MoveTo(pt.x - 7, pt.y - 7);
				pRender->LineTo(pt.x + 8, pt.y + 8);
				pRender->MoveTo(pt.x + 7, pt.y - 7);
				pRender->LineTo(pt.x - 8, pt.y + 8);
			}
		}
		else if (stat.TranslateMode == modeRotate)
		{
			r.bottom -= 1;
			r.right -= 1;

			// rotate rect coords and draw
			float fRotateAngle = stat.fRotateAngle;

			CPoint pt;
			PointToScreen(stat.rotate_org, pt);
			pt.x = -pt.x;
			pt.y = -pt.y;
			r.OffsetRect(pt);
			float x, y;
			int x1, y1;
			rotate_coords(&x, &y, r.left, r.top, fRotateAngle);
			x1 = int(x) - pt.x;
			y1 = int(y) - pt.y;

			pRender->MoveTo(x1, y1);

			rotate_coords(&x, &y, r.right, r.top, fRotateAngle);
			pRender->LineTo(int(x) - pt.x, int(y) - pt.y);

			rotate_coords(&x, &y, r.right, r.bottom, fRotateAngle);
			pRender->LineTo(int(x) - pt.x, int(y) - pt.y);

			rotate_coords(&x, &y, r.left, r.bottom, fRotateAngle);
			pRender->LineTo(int(x) - pt.x, int(y) - pt.y);

			// back to the start
			pRender->LineTo(x1, y1);
		}
		else if (stat.TranslateMode == modeShear)
		{
			int iShearX = stat.iShearX * fZoom;
			int iShearY = stat.iShearY * fZoom;

			if (bInvertHorz)
			{
				iShearX = -iShearX;
			}

			if (bInvertVert)
			{
				iShearY = -iShearY;
			}

			if ((iTransOrigin == inclTop && bInvertVert) ||
				(iTransOrigin == inclBottom && !bInvertVert))
			{
				pRender->MoveTo(r.left, r.top);
				pRender->LineTo(r.right, r.top);
				pRender->LineTo(r.right + iShearX, r.bottom);
				pRender->LineTo(r.left + iShearX, r.bottom);
				pRender->LineTo(r.left, r.top);
			}
			else if ((iTransOrigin == inclTop && !bInvertVert) ||
				(iTransOrigin == inclBottom && bInvertVert))
			{
				pRender->MoveTo(r.left + iShearX, r.top);
				pRender->LineTo(r.right + iShearX, r.top);
				pRender->LineTo(r.right, r.bottom);
				pRender->LineTo(r.left, r.bottom);
				pRender->LineTo(r.left + iShearX, r.top);
			}
			else if (iTransOrigin == inclLeft)
			{
				pRender->MoveTo(r.right, r.top);
				pRender->LineTo(r.right, r.bottom);
				pRender->LineTo(r.left, r.bottom + iShearY);
				pRender->LineTo(r.left, r.top + iShearY);
				pRender->LineTo(r.right, r.top);
			}
			else
			{
				pRender->MoveTo(r.left, r.bottom);
				pRender->LineTo(r.left, r.top);
				pRender->LineTo(r.right, r.top + iShearY);
				pRender->LineTo(r.right, r.bottom + iShearY);
				pRender->LineTo(r.left, r.bottom);
			}
		}
	}
	else
	{
		if (!(dwDrawFlags & thicklines))
		{
		    pRender->SetLineType( CRender2D::LINE_DOT, CRender2D::LINE_THIN, GetRValue(m_clrBox), GetGValue(m_clrBox), GetBValue(m_clrBox));
		}
		else
		{
		    pRender->SetLineType( CRender2D::LINE_SOLID, CRender2D::LINE_THICK, GetRValue(m_clrBox), GetGValue(m_clrBox), GetBValue(m_clrBox));
		}

		pRender->Rectangle(r, false);
		
		if (stat.bEnableHandles && !(stat.bNewBox && (dwDrawFlags & newboxnohandles)))
		{
			r.InflateRect(HANDLEOFFSET, HANDLEOFFSET);

			pRender->SetFillColor(GetRValue(m_clrHandle), GetGValue(m_clrHandle), GetBValue(m_clrHandle));
			pRender->SetLineType(CRender2D::LINE_SOLID, CRender2D::LINE_THIN, 0, 0, 0);

			// draw handles
			CRect rh;
			
			// top left
			rh.left = r.left - HANDLERADIUS;
			rh.top = r.top - HANDLERADIUS;
			rh.right = rh.left + HANDLEDIAMETER;
			rh.bottom = rh.top + HANDLEDIAMETER;

			for (int i = 0; i < 2; i++)
			{
				switch (stat.TranslateMode)
				{
					case modeScale:
					{
						pRender->Rectangle(rh, true);

						// bottom
						rh.OffsetRect(0, r.bottom - r.top);
						pRender->Rectangle(rh, true);

						// middle
						rh.OffsetRect(0, -((r.bottom - r.top) / 2));
						pRender->Rectangle(rh, true);
						break;
					}

					case modeRotate:
					{
						pRender->DrawEllipse(rh.CenterPoint(), HANDLERADIUS, HANDLERADIUS, true);

						rh.OffsetRect(0, r.bottom - r.top);
						pRender->DrawEllipse(rh.CenterPoint(), HANDLERADIUS, HANDLERADIUS, true);
						break;
					}

					case modeShear:
					{
						// middle handles only
						rh.OffsetRect(0, ((r.bottom - r.top) / 2));
						pRender->Rectangle(rh, true);
						break;
					}
				}

				// right
				rh.left = r.right - HANDLERADIUS;
				rh.top = r.top - HANDLERADIUS;
				rh.right = rh.left + HANDLEDIAMETER;
				rh.bottom = rh.top + HANDLEDIAMETER;
			}
			
			if (stat.TranslateMode == modeScale || stat.TranslateMode == modeShear)
			{
				// top
				rh.left = (r.left + r.right)/2 - HANDLERADIUS;
				rh.right = rh.left + HANDLEDIAMETER;
				rh.top = r.top - HANDLERADIUS;
				rh.bottom = rh.top + HANDLEDIAMETER;
				pRender->Rectangle(rh, true);

				// bottom
				rh.OffsetRect(0, r.bottom - r.top);
				pRender->Rectangle(rh, true);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Renders this region as a wireframe box.
// Input  : pRender - 3D Renderer.
//-----------------------------------------------------------------------------
void Box3D::RenderTool3D(CRender3D *pRender)
{
	Vector *pMins;
	Vector *pMaxs;

	if (IsTranslating())
	{
		if (iTransOrigin == inclMain)
		{
			static Vector moveMins;
			static Vector moveMaxs;

			moveMins[axHorz] = tbmins[axHorz] + ptMoveRef[axHorz];
			moveMins[axVert] = tbmins[axVert] + ptMoveRef[axVert];
			moveMins[axThird] = tbmins[axThird] + ptMoveRef[axThird];

			moveMaxs[axHorz] = tbmaxs[axHorz] + ptMoveRef[axHorz];
			moveMaxs[axVert] = tbmaxs[axVert] + ptMoveRef[axVert];
			moveMaxs[axThird] = tbmaxs[axThird] + ptMoveRef[axThird];
			
			pMins = &moveMins;
			pMaxs = &moveMaxs;
		}
		else
		{
			pMins = &tbmins;
			pMaxs = &tbmaxs;
		}
	}
	else
	{
		if (IsEmpty())
		{
			return;
		}

		pMins = &bmins;
		pMaxs = &bmaxs;
	}

	pRender->RenderWireframeBox(*pMins, *pMaxs, GetRValue(m_clrBox), GetGValue(m_clrBox), GetBValue(m_clrBox));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vecStart - 
//-----------------------------------------------------------------------------
void Box3D::StartNew(const Vector &vecStart)
{
	CPoint pt(vecStart[axHorz], vecStart[axVert]);
	Tool3D::StartTranslation(pt);

	//
	// Make a 0 x 0 x 64 box to start.
	//	
	Vector vecMins = vecStart;
	Vector vecMaxs = vecStart;

	vecMins[axThird] = m_pDocument->Snap(vecStart[axThird]);
	vecMaxs[axThird] = vecMins[axThird] + 64;

	StartNew(vecStart, vecMins, vecMaxs);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *vecStart - 
//			*mins - 
//			*maxs - 
//-----------------------------------------------------------------------------
void Box3D::StartNew(const Vector &vecStart, const Vector &mins, const Vector &maxs )
{
	CPoint pt(vecStart[axHorz], vecStart[axVert]);

	Tool3D::StartTranslation(pt);
	
	//Setup our info
	bPreventOverlap		= FALSE;
	stat.TranslateMode	= modeScale;
	stat.bNewBox		= TRUE;
	bEmpty				= FALSE;
	iTransOrigin		= inclBottomRight;
	
	//Copy our saved bounds as our starting point
	tbmins = mins;
	tbmaxs = maxs;

	tbmins[axHorz] = tbmaxs[axHorz] = pt.x;
	tbmins[axVert] = tbmaxs[axVert] = pt.y;

	ZeroVector(m_Scale);

	bmins = tbmins;
	bmaxs = tbmaxs;

	UpdateSavedBounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pt1 - 
//			pt2 - 
//-----------------------------------------------------------------------------
void Box3D::Set(CPoint pt1, CPoint pt2)
{
	CRect r(pt1, pt2);
	r.NormalizeRect();

	if (IsTranslating())
	{
		tbmins[axHorz] = r.left;
		tbmins[axVert] = r.top;
		tbmaxs[axHorz] = r.right;
		tbmaxs[axVert] = r.bottom;
	}
	else
	{
		bmins[axHorz] = r.left;
		bmins[axVert] = r.top;
		bmaxs[axHorz] = r.right;
		bmaxs[axVert] = r.bottom;
	}

	UpdateSavedBounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Box3D::UpdateSavedBounds(void)
{
	if (IsTranslating())
	{
		stat.mins = tbmins;
		stat.maxs = tbmaxs;
	}
	else
	{
		stat.mins = bmins;
		stat.maxs = bmaxs;
	}
}
