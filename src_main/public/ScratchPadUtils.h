//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: This module contains helper functions for use with scratch pads.
//
// $NoKeywords: $
//=============================================================================

#ifndef SCRATCHPADUTILS_H
#define SCRATCHPADUTILS_H
#ifdef _WIN32
#pragma once
#endif


#include "iscratchpad3d.h"


// Draw a cone.
void ScratchPad_DrawLitCone( 
	IScratchPad3D *pPad,
	const Vector &vBaseCenter,
	const Vector &vTip,
	const Vector &vBrightColor,
	const Vector &vDarkColor,
	const Vector &vLightDir,
	float baseWidth,
	int nSegments );


// Draw a cylinder.
void ScratchPad_DrawLitCylinder( 
	IScratchPad3D *pPad,
	const Vector &v1,
	const Vector &v2,
	const Vector &vBrightColor,
	const Vector &vDarkColor,
	const Vector &vLightDir,
	float width,
	int nSegments );


// Draw an arrow.
void ScratchPad_DrawArrow( 
	IScratchPad3D *pPad,
	const Vector &vPos, 
	const Vector &vDirection,
	const Vector &vColor, 
	float flLength=20, 
	float flLineWidth=3,
	float flHeadWidth=8,
	int nCylinderSegments=5,
	int nHeadSegments=8,
	float flArrowHeadPercentage = 0.3f	// How much of the line is the arrow head.
	);


// Draw an arrow with less parameters.. it generates parameters based on length
// automatically to make the arrow look good.
void ScratchPad_DrawArrowSimple( 
	IScratchPad3D *pPad,
	const Vector &vPos, 
	const Vector &vDirection,
	const Vector &vColor, 
	float flLength );


#endif // SCRATCHPADUTILS_H
