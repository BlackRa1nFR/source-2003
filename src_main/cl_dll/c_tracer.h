//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Contains the tracer particle renderer
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#ifndef C_TRACER_H
#define C_TRACER_H

class Vector;
class ParticleDraw;

//-----------------------------------------------------------------------------
// Tracer_Draw(): draws a tracer, assuming the modelview matrix is identity
// This function accepts all arguments in CAMERA (pre-projected) space
//
// arguments
//		[in] Vector& : The origin of the tracer (CAMERA space)
//		[in] Vector& : The direction and length of the tracer (CAMERA space)
//		[in] float  : The tracer width (CAMERA space)
//		[in] float* : r, g, b, a (0 - 1)
//-----------------------------------------------------------------------------
void Tracer_Draw( ParticleDraw* pDraw, Vector& start, Vector& delta, 
				 float width, float* color, float startV = 0.0, float endV = 1.0 );


//-----------------------------------------------------------------------------
// Computes the four verts to draw the tracer with, in the following order:
//	start vertex left side, start vertex right side 
//	end vertex left side, end vertex right side 
// returne false if the tracer is offscreen
//-----------------------------------------------------------------------------
bool Tracer_ComputeVerts( const Vector &start, const Vector &delta, float width, Vector *pVerts );

#endif // C_TRACER_H