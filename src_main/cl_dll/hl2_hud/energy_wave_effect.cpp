//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:			The EWave effect
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "energy_wave_effect.h"
		  
//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CEnergyWaveEffect::CEnergyWaveEffect( TraceLineFunc_t traceline, 
							 TraceHullFunc_t traceHull ) :
	CSheetSimulator(traceline, traceHull)
{
	Init(EWAVE_NUM_HORIZONTAL_POINTS, EWAVE_NUM_VERTICAL_POINTS, EWAVE_NUM_CONTROL_POINTS);
}


//-----------------------------------------------------------------------------
// compute rest positions of the springs
//-----------------------------------------------------------------------------

void CEnergyWaveEffect::ComputeRestPositions()
{
	int i;

	// Set the initial directions and distances (in ewave space)...
	for	( i = 0; i < EWAVE_NUM_VERTICAL_POINTS; ++i)
	{
		// Choose phi centered at pi/2
		float phi = (M_PI - m_EWavePhi) * 0.5f + m_EWavePhi * 
			(float)i / (float)(EWAVE_NUM_VERTICAL_POINTS - 1);

		for (int j = 0; j < EWAVE_NUM_HORIZONTAL_POINTS; ++j)
		{
			// Choose theta centered at pi/2 also (y, or forward axis)
			float theta = (M_PI - m_EWaveTheta) * 0.5f + m_EWaveTheta * 
				(float)j / (float)(EWAVE_NUM_HORIZONTAL_POINTS - 1);

			int idx = i * EWAVE_NUM_HORIZONTAL_POINTS + j;

			GetFixedPoint(idx).x = cos(theta) * sin(phi) * m_RestDistance;
			GetFixedPoint(idx).y = sin(theta) * sin(phi) * m_RestDistance;
			GetFixedPoint(idx).z = cos(phi) * m_RestDistance;
		}
	}

	
	// Compute box for fake volume testing
	Vector dist = GetFixedPoint(0) - GetFixedPoint(1);
	float l = dist.Length();
	SetBoundingBox( Vector( -l * 0.25f, -l * 0.25f, -l * 0.25f), 
		Vector( l * 0.25f, l * 0.25f, l * 0.25f) );
}

void CEnergyWaveEffect::MakeSpring( int p1, int p2 )
{
	Vector dist = GetFixedPoint(p1) - GetFixedPoint(p2);
    AddSpring( p1, p2, dist.Length() );
}

void CEnergyWaveEffect::ComputeSprings()
{	
	// Do the main springs first
	// Compute springs and rest lengths...
	int i, j;
	for	( i = 0; i < EWAVE_NUM_VERTICAL_POINTS; ++i)
	{
		for ( j = 0; j < EWAVE_NUM_HORIZONTAL_POINTS; ++j)
		{
			// Here's the particle we're making springs for
			int idx = i * EWAVE_NUM_HORIZONTAL_POINTS + j;

			// Make a spring connected to the control point
			AddFixedPointSpring( idx, idx, 0.0f );
		}
	}

	// Add four corner springs
	MakeSpring( 0, 3 );
	MakeSpring( 0, 12 );
	MakeSpring( 12, 15 );
	MakeSpring( 3, 15 );
}

//------------------------------------------------------------------------------
// Purpose : Overwrite.  Energy wave does no collisions
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CEnergyWaveEffect::DetectCollisions()
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void CEnergyWaveEffect::Precache( void )
{
	SetFixedSpringConstant( 200.0f );	
	SetPointSpringConstant( 0.5f );		
	SetSpringDampConstant( 10.0f );			
	SetViscousDrag( 0.25f );			
	m_RestDistance = 500.0;

	m_EWaveTheta = M_PI * EWAVE_INITIAL_THETA / 180.0f;
	m_EWavePhi = M_PI * EWAVE_INITIAL_PHI / 180.0f;

	// Computes the rest positions
	ComputeRestPositions();

	// Computes the springs
	ComputeSprings();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CEnergyWaveEffect::Spawn( void )
{
	Precache();
}


//-----------------------------------------------------------------------------
// Computes the opacity....
//-----------------------------------------------------------------------------

float CEnergyWaveEffect::ComputeOpacity( const Vector& pt, const Vector& center ) const
{
	float dist = pt.DistTo( center ) / m_RestDistance;
	dist = sqrt(dist);
	if (dist > 1.0)
		dist = 1.0f;
	return (1.0 - dist) * 35;
}


