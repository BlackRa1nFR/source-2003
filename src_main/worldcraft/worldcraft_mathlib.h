//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines math functions specific to Worldcraft.
//
// $NoKeywords: $
//=============================================================================

#ifndef WORLDCRAFT_MATHLIB_H
#define WORLDCRAFT_MATHLIB_H

#ifdef _WIN32
#pragma once
#endif

typedef unsigned char byte;

#include "mathlib.h"
#include <math.h>

// disable warning C4244: 'argument' : conversion from 'unsigned long' to 'unsigned short', possible loss of data
#pragma warning(disable:4244)

// disable warning C4305: 'argument' : truncation from 'const double' to 'float'
#pragma warning(disable:4305)

struct matrix4_t
{
	float *operator[]( int i )				{ Assert(( i >= 0 ) && ( i < 4 )); return m_flMatVal[i]; }
	const float *operator[]( int i ) const	{ Assert(( i >= 0 ) && ( i < 4 )); return m_flMatVal[i]; }
	float *Base()							{ return &m_flMatVal[0][0]; }
	const float *Base() const				{ return &m_flMatVal[0][0]; }

	float m_flMatVal[4][4];

	operator matrix3x4_t &()				{ return *(matrix3x4_t*)this; }
	operator const matrix3x4_t &() const	{ return *(const matrix3x4_t*)this; }
};

typedef vec_t vec5_t[5];


//
// Matrix functions:
//
void IdentityMatrix(matrix4_t& Matrix);
void AxisAngleMatrix(matrix4_t& Matrix, const Vector &Axis, float fAngle);
void MatrixMultiply(Vector &Dest, const Vector &Source, const matrix4_t& Matrix);
void MatrixMultiplyPoint(Vector &Dest, const Vector &Source, const matrix4_t& Matrix);
void ProjectPoint(Vector &Dest, const Vector &Source, const matrix4_t& Matrix);
void MatrixInvert(matrix4_t& matrix);
void MultiplyMatrix(const matrix4_t& MatrixIn1, const matrix4_t& MatrixIn2, matrix4_t& MatrixOut);
void TransposeMatrix(matrix4_t& Matrix);
void RotateX(matrix4_t& Matrix, float fDegrees);
void RotateY(matrix4_t& Matrix, float fDegrees);
void RotateZ(matrix4_t& Matrix, float fDegrees);

void VectorIRotate (const Vector& in1, const matrix3x4_t& in2, Vector& out);

float fixang(float a);
float lineangle(float x1, float y1, float x2, float y2);
void rotate_coords(float *dx, float *dy, float sx, float sy, double angle);
void polyMake( float x1, float  y1, float x2, float y2, int npoints, float start_ang, Vector *pmPoints );
float rint(float);

#define ZeroVector(a) { a[0] = 0; a[1]= 0; a[2] = 0; }
#define ZeroVector4(a) { a[0] = 0; a[1]= 0; a[2] = 0; a[3] = 0; }

#endif // WORLDCRAFT_MATHLIB_H
