//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================


#include "stdafx.h"
#include "Axes2.h"
#include "OPTGeneral.h"
#include "Options.h"
#include "worldcraft_mathlib.h"
#include "MapFace.h"
#include "MapGroup.h"
#include "MapSolid.h"
#include "Worldcraft.h"


static CMapSolid * CreateSegment(float fZMin, float fZMax, float fOuterPoints[][2], float fInnerPoints[][2], int iStart, int iEnd, BOOL bCreateSouthFace);


void MakeArc(float x1, float y1, float x2, float y2, int npoints, float start_ang, float fArc, float points[][2])
{
    int point;
    float angle = start_ang;
	float angle_delta;
    float xrad = (x2 - x1) / 2.0f;
	float yrad = (y2 - y1) / 2.0f;

	// make centerpoint for polygon:
    float xCenter = x1 + xrad;
    float yCenter = y1 + yrad;

	angle_delta = fArc / (float)npoints;

	// Add an additional points if we are not doing a full circle
	if (fArc != 360.0)
		++npoints;
	
    for( point = 0; point < npoints; point++ )
    {
        if ( angle > 360 )
           angle -= 360;

        points[point][0] = rint(xCenter + (float)sin(DEG2RAD(angle)) * xrad);
        points[point][1] = rint(yCenter + (float)cos(DEG2RAD(angle)) * yrad);

		angle += angle_delta;
    }

	// Full circle, recopy the first point as the closing point.
	if (fArc == 360.0)
	{
	    points[point][0] = points[0][0];
		points[point][1] = points[0][1];
	}
}

#define ARC_MAX_POINTS 4096


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pBox - 
//			fStartAngle - 
//			iSides - 
//			fArc - 
//			iWallWidth - 
//			iAddHeight - 
//			bPreview - 
// Output : Returns a group containing the arch solids.
//-----------------------------------------------------------------------------
CMapClass *CreateArch(BoundBox *pBox, float fStartAngle, int iSides, float fArc, int iWallWidth, int iAddHeight, BOOL bPreview)
{
	float fOuterPoints[ARC_MAX_POINTS][2];
	float fInnerPoints[ARC_MAX_POINTS][2];

	//
	// create outer points
	//
	MakeArc(pBox->bmins[AXIS_X], pBox->bmins[AXIS_Y],
		pBox->bmaxs[AXIS_X], pBox->bmaxs[AXIS_Y], iSides,
		fStartAngle, fArc, fOuterPoints);

	//
	// create inner points
	//
	MakeArc(pBox->bmins[AXIS_X] + iWallWidth, 
		pBox->bmins[AXIS_Y] + iWallWidth, 
		pBox->bmaxs[AXIS_X] - iWallWidth, 
		pBox->bmaxs[AXIS_Y] - iWallWidth, iSides, 
		fStartAngle, fArc, fInnerPoints);


	//
	// check wall width - if it's half or more of the total,
	//  set the inner poinst to the center point of the box
	//  and turn off the CreateSouthFace flag
	//	
	BOOL bCreateSouthFace = TRUE;
	Vector Center;
	pBox->GetBoundsCenter(Center);
	if((iWallWidth*2+8) >= (pBox->bmaxs[AXIS_X] - pBox->bmins[AXIS_X]) ||
		(iWallWidth*2+8) >= (pBox->bmaxs[AXIS_Y] - pBox->bmins[AXIS_Y]))
	{
		for(int i = 0; i < ARC_MAX_POINTS; i++)
		{
			fInnerPoints[i][AXIS_X] = Center[AXIS_X];
			fInnerPoints[i][AXIS_Y] = Center[AXIS_Y];
		}
		bCreateSouthFace = FALSE;
	}

	// create group for segments
	CMapGroup *pGroup = new CMapGroup;

	Vector MoveAccum( 0.f, 0.f, 0.f );

	float fMinZ, fMaxZ;

	fMinZ = pBox->bmins[2];
	fMaxZ = pBox->bmaxs[2];

	if ((fMaxZ - fMinZ) < 1.0f)
		fMaxZ = fMinZ + 1.0f;

	for (int i = 0; i < iSides; i++)
	{
		int iNextPoint = i+1;
		if (iNextPoint >= iSides + 1)
			iNextPoint = 0;

		CMapSolid *pSolid = CreateSegment(
			fMinZ, fMaxZ,
			fOuterPoints, fInnerPoints,
			i, iNextPoint, bCreateSouthFace);

		pGroup->AddChild(pSolid);

		if (iAddHeight && i)	// don't move first segment
		{
			MoveAccum[2] += iAddHeight;
			pSolid->TransMove(MoveAccum);
		}
	}

	pGroup->CalcBounds(TRUE);

	// make sure size of group's bounds are size of original bounds -
	//  if not, scale up. this can happen when we use rotation.
	Vector objsize, boundsize;
	pBox->GetBoundsSize(boundsize);
	pGroup->GetBoundsSize(objsize);

	if (Options.general.bStretchArches)
	{
		if (boundsize[AXIS_X] > objsize[AXIS_X] || 
			boundsize[AXIS_Y] > objsize[AXIS_Y])
		{
			Vector scale;
			scale[AXIS_X] = boundsize[AXIS_X] / objsize[AXIS_X];
			scale[AXIS_Y] = boundsize[AXIS_Y] / objsize[AXIS_Y];
			scale[AXIS_Z] = 1.0f;  // xxxYWB scaling by 0 causes veneers, so I changed to 1.0
			Vector center;
			pBox->GetBoundsCenter(center);
			pGroup->TransScale(center, scale);
		}
	}

	return pGroup;
}


//-----------------------------------------------------------------------------
// Purpose: Create a segment using two polygons and a start and end position in
//			those polygons.
// Input  : fZMin - 
//			fZMax - 
//			fOuterPoints - 
//			fInnerPoints - 
//			iStart - 
//			iEnd - 
//			bCreateSouthFace - 
// Output : 
//-----------------------------------------------------------------------------
static CMapSolid *CreateSegment(float fZMin, float fZMax, float fOuterPoints[][2], float fInnerPoints[][2], int iStart, int iEnd, BOOL bCreateSouthFace)
{
	CMapFace Face;
	Vector points[4];	// all sides have four vertices

	CMapSolid *pSolid = new CMapSolid;

	int iNorthSouthPoints = 3 + (bCreateSouthFace ? 1 : 0);

	// create top face
	points[0][0] = fOuterPoints[iStart][0];
	points[0][1] = fOuterPoints[iStart][1];
	points[0][2] = fZMin;

	points[1][0] = fOuterPoints[iEnd][0];
	points[1][1] = fOuterPoints[iEnd][1];
	points[1][2] = fZMin;

	points[2][0] = fInnerPoints[iEnd][0];
	points[2][1] = fInnerPoints[iEnd][1];
	points[2][2] = fZMin;

	points[3][0] = fInnerPoints[iStart][0];
	points[3][1] = fInnerPoints[iStart][1];
	points[3][2] = fZMin;

	Face.CreateFace(points, -iNorthSouthPoints);
	pSolid->AddFace(&Face);

	// bottom face - set other z value and reverse order
	for (int i = 0; i < 4; i++)
	{
		points[i][2] = fZMax;
	}

	Face.CreateFace(points, iNorthSouthPoints);
	pSolid->AddFace(&Face);

	// left side
	points[0][0] = fOuterPoints[iStart][0];
	points[0][1] = fOuterPoints[iStart][1];
	points[0][2] = fZMax;

	points[1][0] = fOuterPoints[iStart][0];
	points[1][1] = fOuterPoints[iStart][1];
	points[1][2] = fZMin;

	points[2][0] = fInnerPoints[iStart][0];
	points[2][1] = fInnerPoints[iStart][1];
	points[2][2] = fZMin;

	points[3][0] = fInnerPoints[iStart][0];
	points[3][1] = fInnerPoints[iStart][1];
	points[3][2] = fZMax;

	Face.CreateFace(points, -4);
	pSolid->AddFace(&Face);

	// right side
	points[0][0] = fOuterPoints[iEnd][0];
	points[0][1] = fOuterPoints[iEnd][1];
	points[0][2] = fZMin;

	points[1][0] = fOuterPoints[iEnd][0];
	points[1][1] = fOuterPoints[iEnd][1];
	points[1][2] = fZMax;

	points[2][0] = fInnerPoints[iEnd][0];
	points[2][1] = fInnerPoints[iEnd][1];
	points[2][2] = fZMax;

	points[3][0] = fInnerPoints[iEnd][0];
	points[3][1] = fInnerPoints[iEnd][1];
	points[3][2] = fZMin;

	Face.CreateFace(points, -4);
	pSolid->AddFace(&Face);

	// north face
	points[0][0] = fOuterPoints[iEnd][0];
	points[0][1] = fOuterPoints[iEnd][1];
	points[0][2] = fZMin;

	points[1][0] = fOuterPoints[iStart][0];
	points[1][1] = fOuterPoints[iStart][1];
	points[1][2] = fZMin;

	points[2][0] = fOuterPoints[iStart][0];
	points[2][1] = fOuterPoints[iStart][1];
	points[2][2] = fZMax;

	points[3][0] = fOuterPoints[iEnd][0];
	points[3][1] = fOuterPoints[iEnd][1];
	points[3][2] = fZMax;

	Face.CreateFace(points, -4);
	pSolid->AddFace(&Face);

	// south face
	if (bCreateSouthFace)
	{
		points[0][0] = fInnerPoints[iStart][0];
		points[0][1] = fInnerPoints[iStart][1];
		points[0][2] = fZMin;

		points[1][0] = fInnerPoints[iEnd][0];
		points[1][1] = fInnerPoints[iEnd][1];
		points[1][2] = fZMin;

		points[2][0] = fInnerPoints[iEnd][0];
		points[2][1] = fInnerPoints[iEnd][1];
		points[2][2] = fZMax;

		points[3][0] = fInnerPoints[iStart][0];
		points[3][1] = fInnerPoints[iStart][1];
		points[3][2] = fZMax;

		Face.CreateFace(points, -4);
		pSolid->AddFace(&Face);
	}

	pSolid->InitializeTextureAxes(Options.GetTextureAlignment(), INIT_TEXTURE_ALL | INIT_TEXTURE_FORCE);

	return(pSolid);
}
