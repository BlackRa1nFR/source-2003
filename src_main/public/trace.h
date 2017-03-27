//========= (C) Copyright 1999-2001 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef TRACE_H
#define TRACE_H
#pragma once

#include "mathlib.h"

// Note: These flags need to match the bspfile.h DISPTRI_TAG_* flags.
#define DISPSURF_FLAG_SURFACE		(1<<0)
#define DISPSURF_FLAG_WALKABLE		(1<<1)
#define DISPSURF_FLAG_BUILDABLE		(1<<2)

//=============================================================================
// Base Trace Structure
// - shared between engine/game dlls and tools (vrad)
//=============================================================================

class CBaseTrace
{
public:

	// Returns true if you hit a displacement surface.
	bool IsDispSurface( void )				{ return ( ( dispFlags & DISPSURF_FLAG_SURFACE ) != 0 ); }

	// Returns true if the displacement surface is walkable.
	bool IsDispSurfaceWalkable( void )		{ return ( ( dispFlags & DISPSURF_FLAG_WALKABLE ) != 0 ); }

	// Returns true if the displacement surface is buildable (currently tf2 specific).
	bool IsDispSurfaceBuildable( void )		{ return ( ( dispFlags & DISPSURF_FLAG_BUILDABLE ) != 0 ); }

public:

	// these members are aligned!!
	Vector			startpos;				// start position
	Vector			endpos;					// final position
	cplane_t		plane;					// surface normal at impact

	float			fraction;				// time completed, 1.0 = didn't hit anything

	int				contents;				// contents on other side of surface hit
	unsigned short	dispFlags;				// displacement flags for marking surfaces with data

	qboolean		allsolid;				// if true, plane is not valid
	qboolean		startsolid;				// if true, the initial point was in a solid area
};

#endif // TRACE_H