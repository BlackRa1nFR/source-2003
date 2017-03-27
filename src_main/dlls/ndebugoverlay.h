//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:		Namespace for functions dealing with Debug Overlays
//
// $NoKeywords: $
//=============================================================================
#ifndef NDEBUGOVERLAY_H
#define NDEBUGOVERLAY_H

#ifdef _WIN32
#pragma once
#endif


#include "vector.h"

// ====================================================================================
// > An overlay line.  Used only for debugging
// ====================================================================================
struct OverlayLine_t 
{
	Vector			origin;
	Vector			dest;
	int				r;
	int				g;
	int				b;
	bool			noDepthTest;
	bool			draw;
};

//=============================================================================
//	>> NDebugOverlay
//=============================================================================
namespace NDebugOverlay
{
	void	Box(const Vector &origin, const Vector &mins, const Vector &maxs, int r, int g, int b, int a, float flDuration);
	void	BoxDirection(const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &forward, int r, int g, int b, int a, float flDuration);
	void	BoxAngles(const Vector &origin, const Vector &mins, const Vector &maxs, const QAngle &angles, int r, int g, int b, int a, float flDuration);
	void	EntityBounds( CBaseEntity *pEntity, int r, int g, int b, int a, float flDuration );
	void	Line( const Vector &origin, const Vector &target, int r, int g, int b, bool noDepthTest, float flDuration );
	void	Triangle( const Vector &p1, const Vector &p2, const Vector &p3, int r, int g, int b, int a, bool noDepthTest, float duration );
	void	EntityText( int entityID, int text_offset, const char *text, float flDuration, int r = 255, int g = 255, int b = 255, int a = 255);
	void	Grid( const Vector &vPosition );
	void	Text( const Vector &origin, const char *text, bool bViewCheck, float flDuration );
	void	ScreenText( float fXpos, float fYpos, const char *text, int r, int g, int b, int a, float flDuration);
	void	Cross3D(const Vector &position, const Vector &mins, const Vector &maxs, int r, int g, int b, bool noDepthTest, float flDuration );
	void	DrawOverlayLines(void);
	void	AddDebugLine(const Vector &startPos, const Vector &endPos, bool noDepthTest, bool testLOS);
	void	DrawTickMarkedLine(const Vector &startPos, const Vector &endPos, float tickDist, int tickTextDist, int r, int g, int b, bool noDepthTest, float flDuration );
	void	DrawPositioningOverlay( float flCrossDistance );
	void	DrawGroundCrossHairOverlay();
};


#endif // NDEBUGOVERLAY_H
