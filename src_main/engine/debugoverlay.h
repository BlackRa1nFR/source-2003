//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:	Debugging overlay functions
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef DEBUGOVERLAY_H
#define DEBUGOVERLAY_H

#ifdef _WIN32
#pragma once
#endif

namespace CDebugOverlay
{
	void AddEntityTextOverlay(int ent_index, int line_offset, float flDuration, int r, int g, int b, int a, const char *text);
	void AddBoxOverlay(const Vector& origin, const Vector& mins, const Vector& max, const QAngle & angles, int r, int g, int b, int a, float flDuration);
	void AddSweptBoxOverlay(const Vector& start, const Vector& end, const Vector& mins, const Vector& max, const QAngle & angles, int r, int g, int b, int a, float flDuration);
	void AddGridOverlay(const Vector& vPos );
	void AddLineOverlay(const Vector& origin, const Vector& dest, int r, int g, int b, bool noDepthTest, float flDuration);
	void AddTriangleOverlay(const Vector& p1, const Vector& p2, const Vector &p3, int r, int g, int b, int a, bool noDepthTest, float flDuration);
	void AddTextOverlay(const Vector& origin, float flDuration, const char *text);
	void AddTextOverlay(const Vector& origin, int line_offset, float flDuration, const char *text);
	void AddTextOverlay(const Vector& origin, int line_offset, float flDuration, float alpha, const char *text);
	void AddScreenTextOverlay(float flXPos, float flYPos,float flDuration, int r, int g, int b, int a, const char *text);
	void AddTextOverlay(const Vector& textPos, float duration, float alpha, const char *text) ;
	void Draw3DOverlays(void);
}

#endif // DEBUGOVERLAY_H