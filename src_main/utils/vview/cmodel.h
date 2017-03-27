//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
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

#ifndef CMODEL_H
#define CMODEL_H
#pragma once

#include "basetypes.h"

#include "mathlib.h"
#include "bspflags.h"

typedef struct edict_s edict_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/


// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2


typedef struct cmodel_s
{
	Vector		mins, maxs;
	Vector		origin;		// for sounds or lights
	int			headnode;
} cmodel_t;

typedef struct csurface_s
{
	char		name[64];
	int			flags;
	int			value;
} csurface_t;

typedef struct cbrush_s cbrush_t;
// a trace is returned when a box is swept through the world
typedef struct
{
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	Vector		endpos;		// final position
	cplane_t	plane;		// surface normal at impact
	csurface_t	*surface;	// surface hit
	int			contents;	// contents on other side of surface hit
	edict_t		*ent;		// not set by CM_*() functions
	cbrush_t	*brush;		// index of the brush that was hit
	int			sideNumber; // side of the brush we hit
} trace_t;


#endif // CMODEL_H
