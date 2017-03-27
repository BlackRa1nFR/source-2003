//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_HULL_H
#define AI_HULL_H
#pragma once

class Vector;
//=========================================================
// Link Properties. These hulls must correspond to the hulls
// in AI_Hull.cpp!
//=========================================================
enum Hull_t
{
	HULL_HUMAN,				// Combine, Stalker, Zombie...
	HULL_SMALL_CENTERED,	// Scanner
	HULL_WIDE_HUMAN,		// Vortigaunt
	HULL_TINY,				// Headcrab
	HULL_WIDE_SHORT,		// Bullsquid
	HULL_WIDE_TALL,			// Cremator
	HULL_TINY_CENTERED,		// Manhack 
	HULL_LARGE,				// Antlion Guard
	HULL_LARGE_CENTERED,	// Mortar Synth
//--------------------------------------------
	NUM_HULLS,
	HULL_NONE				// No Hull (appears after num hulls as we don't want to count it)
};

enum Hull_Bits_t
{
	bits_HUMAN_HULL				=	0x00000001,
	bits_SMALL_CENTERED_HULL	=	0x00000002,
	bits_WIDE_HUMAN_HULL		=	0x00000004,
	bits_TINY_HULL				=	0x00000008,
	bits_WIDE_SHORT_HULL		=	0x00000010,
	bits_WIDE_TALL_HULL			=	0x00000020,
	bits_TINY_CENTERED_HULL		=	0x00000040,
	bits_LARGE_HULL				=	0x00000080,
	bits_LARGE_CENTERED_HULL	=	0x00000100,
};


//=============================================================================
//	>> CAI_Hull
//=============================================================================
namespace NAI_Hull
{
	const Vector &Mins(int id);
	const Vector &Maxs(int id);

	const Vector &SmallMins(int id);
	const Vector &SmallMaxs(int id);

	float		Length(int id);
	float		Width(int id);
	float		Height(int id);

	int			Bits(int id);
 
	const char*	Name(int id);
};

#endif // AI_HULL_H