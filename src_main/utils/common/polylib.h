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

#ifndef POLYLIB_H
#define POLYLIB_H
#pragma once

#ifndef MATHLIB_H
#include "mathlib.h"
#endif

// We have to explicitly set the packing of this struct.  When using doubles, the compiler will align p[] 
// in some of the modules.  Force a specific 8-byte packing so that this works out to be the same struct
// in every module even when vec3_t is double precision.

// #pragma pack(8)
//struct winding_t;
typedef struct winding_s
{
	int		numpoints;
	Vector	*p;		// variable sized
	int		maxpoints;
	struct  winding_s *next;
} winding_t;
// #pragma pack()

#define	MAX_POINTS_ON_WINDING	64

// you can define on_epsilon in the makefile as tighter
// point on plane side epsilon
// todo: need a world-space epsilon, a lightmap-space epsilon, and a texture space epsilon
// or at least convert from a world-space epsilon to lightmap and texture space epsilons
#ifndef	ON_EPSILON
#define	ON_EPSILON	0.1
#endif


winding_t	*AllocWinding (int points);
vec_t	WindingArea (winding_t *w);
void	WindingCenter (winding_t *w, Vector& center);
vec_t	WindingAreaAndBalancePoint( winding_t *w, Vector& center );
void	ClipWindingEpsilon (winding_t *in, Vector& normal, vec_t dist, 
				vec_t epsilon, winding_t **front, winding_t **back);

void	ClassifyWindingEpsilon( winding_t *in, Vector& normal, vec_t dist, 
				vec_t epsilon, winding_t **front, winding_t **back, winding_t **on);

winding_t	*ChopWinding (winding_t *in, Vector& normal, vec_t dist);
winding_t	*CopyWinding (winding_t *w);
winding_t	*ReverseWinding (winding_t *w);
winding_t	*BaseWindingForPlane (Vector& normal, vec_t dist);
void	CheckWinding (winding_t *w);
void	WindingPlane (winding_t *w, Vector& normal, vec_t *dist);
void	RemoveColinearPoints (winding_t *w);
int		WindingOnPlaneSide (winding_t *w, Vector& normal, vec_t dist);
void	FreeWinding (winding_t *w);
void	WindingBounds (winding_t *w, Vector& mins, Vector& maxs);

void	ChopWindingInPlace (winding_t **w, Vector& normal, vec_t dist, vec_t epsilon);
// frees the original if clipped

bool PointInWinding( Vector const &pt, winding_t *pWinding );

void pw(winding_t *w);


#endif // POLYLIB_H
