//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ENERGYWAVEEFFECT_H
#define ENERGYWAVEEFFECT_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib.h"
#include "Vector.h"
#include "UtlVector.h"
#include "SheetSimulator.h"

// --------------------------------------
//  Energy wave defs
// --------------------------------------
enum
{
	EWAVE_NUM_HORIZONTAL_POINTS = 10,
	EWAVE_NUM_VERTICAL_POINTS = 10,
	EWAVE_NUM_CONTROL_POINTS = EWAVE_NUM_HORIZONTAL_POINTS * EWAVE_NUM_VERTICAL_POINTS,
	EWAVE_INITIAL_THETA = 135,
	EWAVE_INITIAL_PHI = 135,
};

//-----------------------------------------------------------------------------
// Purpose: Energy Wave Effect
//-----------------------------------------------------------------------------

class CEnergyWaveEffect	: public CSheetSimulator
{
public:
	CEnergyWaveEffect( TraceLineFunc_t traceline, TraceHullFunc_t traceHull );

	void	Precache();
	void	Spawn();

	// Computes the opacity....
	float	ComputeOpacity( const Vector& pt, const Vector& center ) const;

	void	DetectCollisions();


private:
	CEnergyWaveEffect( const CEnergyWaveEffect & ); // not defined, not accessible

	// Simulation set up
	void ComputeRestPositions();
	void MakeSpring( int p1, int p2 );
	void ComputeSprings();

	float m_RestDistance;
	float m_EWaveTheta;
	float m_EWavePhi;
};



#endif // ENERGYWAVEEFFECT_H
