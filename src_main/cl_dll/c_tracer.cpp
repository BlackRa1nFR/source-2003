//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Contains the client-side part of the precipitation class
//-----------------------------------------------------------------------------
// $Revision: $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_clientstats.h"
#include "particledraw.h"
#include "materialsystem/IMesh.h"

//-----------------------------------------------------------------------------
// Sees if the tracer is behind the camera or should be culled
//-----------------------------------------------------------------------------

static bool ClipTracer( const Vector &start, const Vector &delta, Vector &clippedStart, Vector &clippedDelta )
{
	// dist1 = start dot forward - origin dot forward
	// dist2 = (start + delta ) dot forward - origin dot forward
	// in camera space this is -start[2] since origin = 0 and vecForward = (0, 0, -1)
	float dist1 = -start[2];
	float dist2 = dist1 - delta[2];
	
	// Clipped, skip this tracer
	if ( dist1 <= 0 && dist2 <= 0 )
		return true;

	clippedStart = start;
	clippedDelta = delta;
	
	// Needs to be clipped
	if ( dist1 <= 0 || dist2 <= 0 )
	{
		float fraction = dist2 - dist1;

		// Too close to clipping plane
		if ( fraction < 1e-3 )
			return true;

		fraction = -dist1 / fraction;

		if ( dist1 <= 0 )
		{
			VectorMA( start, fraction, delta, clippedStart );
		}
		else
		{
			VectorMultiply( delta, fraction, clippedDelta );
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Computes the four verts to draw the tracer with
//-----------------------------------------------------------------------------
bool Tracer_ComputeVerts( const Vector &start, const Vector &delta, float width, Vector *pVerts )
{
	Vector clippedStart, clippedDelta;

	// Clip the tracer
	if ( ClipTracer( start, delta, clippedStart, clippedDelta ) )
		return false;

	// Figure out direction in camera space of the normal
	Vector normal;
	CrossProduct( clippedDelta, clippedStart, normal );
					  
	// don't draw if they are parallel
	float sqLength = DotProduct( normal, normal );
	if (sqLength < 1e-3)
		return false;

	// Resize the normal to be appropriate based on the width
	VectorScale( normal, 0.5f * width / sqrt(sqLength), normal );

	VectorSubtract( clippedStart, normal, pVerts[0] );
	VectorAdd( clippedStart, normal, pVerts[1] );

	VectorAdd( pVerts[0], clippedDelta, pVerts[2] );
	VectorAdd( pVerts[1], clippedDelta, pVerts[3] );

	return true;
}


//-----------------------------------------------------------------------------
// draw a tracer.
//-----------------------------------------------------------------------------
void Tracer_Draw( ParticleDraw* pDraw, Vector& start, Vector& delta, float width, float* color, float startV, float endV )
{
	if( !pDraw->GetMeshBuilder() )
		return;
	
	MEASURE_TIMED_STAT( CS_TRACER_RENDER );

	// Take statistics...
	ClientStats().BeginTimedStat( CS_TRACER_TEST );

	// Clip the tracer
	Vector verts[4];
	if (!Tracer_ComputeVerts( start, delta, width, verts ))
		return;

	ClientStats().EndTimedStat( CS_TRACER_TEST );

	// NOTE: Gotta get the winding right so it's not backface culled
	// (we need to turn of backface culling for these bad boys)

	CMeshBuilder* pMeshBuilder = pDraw->GetMeshBuilder();

	pMeshBuilder->Position3f( verts[0].x, verts[0].y, verts[0].z );
	pMeshBuilder->TexCoord2f( 0, 0.0f, startV );
	if (color)
	{
		pMeshBuilder->Color4fv( color );
	}
	else
	{
		pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	}
	pMeshBuilder->AdvanceVertex();

	pMeshBuilder->Position3f( verts[1].x, verts[1].y, verts[1].z );
	pMeshBuilder->TexCoord2f( 0, 1.0f, startV );
	if (color)
	{
		pMeshBuilder->Color4fv( color );
	}
	else
	{
		pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	}
	pMeshBuilder->AdvanceVertex();

	pMeshBuilder->Position3f( verts[3].x, verts[3].y, verts[3].z );
	pMeshBuilder->TexCoord2f( 0, 1.0f, endV );
	if (color)
		pMeshBuilder->Color4fv( color );
	else
		pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	pMeshBuilder->AdvanceVertex();

	pMeshBuilder->Position3f( verts[2].x, verts[2].y, verts[2].z );
	pMeshBuilder->TexCoord2f( 0, 0.0f, endV );
	if (color)
		pMeshBuilder->Color4fv( color );
	else
		pMeshBuilder->Color4ub( 255, 255, 255, 255 );
	pMeshBuilder->AdvanceVertex();

	ClientStats().IncrementCountedStat( CS_TRACER_NUMTRIANGLES, 2 );
}



