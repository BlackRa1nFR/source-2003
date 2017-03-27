//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef SND_FIXEDINT_H
#define SND_FIXEDINT_H

#if defined( _WIN32 )
#pragma once
#endif

// fixed point stuff for real-time resampling
#define FIX_BITS			28
#define FIX_SCALE			(1 << FIX_BITS)
#define FIX_MASK			((1 << FIX_BITS)-1)
#define FIX_FLOAT(a)		((int)((a) * FIX_SCALE))
#define FIX(a)				(((int)(a)) << FIX_BITS)
#define FIX_INTPART(a)		(((int)(a)) >> FIX_BITS)
#define FIX_FRACTION(a,b)	(FIX(a)/(b))
#define FIX_FRACPART(a)		((a) & FIX_MASK)

typedef unsigned int fixedint;

#endif // SND_FIXEDINT_H
