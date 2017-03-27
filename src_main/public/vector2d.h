/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef VECTOR2D_H
#define VECTOR2D_H

#ifdef _WIN32
#pragma once
#endif

#include <math.h>
#include <assert.h>

// For vec_t, put this somewhere else?
#include "basetypes.h"

// For rand(). We really need a library!
#include <stdlib.h>

#ifdef __cplusplus

// HACKHACK: Declare these directly from mathlib.h for now
extern "C" 
{
extern float (*pfSqrt)(float x);
};

#define FastSqrt(x)			(*pfSqrt)(x)


//=========================================================
// 2D Vector2D
//=========================================================

class Vector2D					
{
public:
	// Members
	vec_t x, y;

	// Construction/destruction
	Vector2D(void);
	Vector2D(vec_t X, vec_t Y);
	Vector2D(const float *pFloat);

	// Initialization
	void Init(vec_t ix=0.0f, vec_t iy=0.0f);

	// Got any nasty NAN's?
	bool IsValid() const;

	// array access...
	vec_t operator[](int i) const;
	vec_t& operator[](int i);

	// Base address...
	vec_t* Base();
	vec_t const* Base() const;

	// Initialization methods
	void Random( float minVal, float maxVal );

	// equality
	bool operator==(const Vector2D& v) const;
	bool operator!=(const Vector2D& v) const;	

	// arithmetic operations
	Vector2D&	operator+=(const Vector2D &v);			
	Vector2D&	operator-=(const Vector2D &v);		
	Vector2D&	operator*=(const Vector2D &v);			
	Vector2D&	operator*=(float s);
	Vector2D&	operator/=(const Vector2D &v);		
	Vector2D&	operator/=(float s);					

	// negate the Vector2D components
	void	Negate(); 

	// Get the Vector2D's magnitude.
	vec_t	Length() const;

	// Get the Vector2D's magnitude squared.
	vec_t	LengthSqr(void) const;

	// Get the distance from this Vector2D to the other one.
	vec_t	DistTo(const Vector2D &vOther) const;

	// Get the distance from this Vector2D to the other one squared.
	vec_t	DistToSqr(const Vector2D &vOther) const;		

	// Copy
	void	CopyToArray(float* rgfl) const;	

	// Multiply, add, and assign to this (ie: *this = a + b * scalar). This
	// is about 12% faster than the actual Vector2D equation (because it's done per-component
	// rather than per-Vector2D).
	void	MulAdd(Vector2D const& a, Vector2D const& b, float scalar);	

	// Dot product.
	vec_t	Dot(Vector2D const& vOther) const;			

#ifndef VECTOR_NO_SLOW_OPERATIONS
	// copy constructors
	Vector2D(const Vector2D &vOther);

	// assignment
	Vector2D& operator=(const Vector2D &vOther);

	// arithmetic operations
	Vector2D	operator-(void) const;
				
	Vector2D	operator+(const Vector2D& v) const;	
	Vector2D	operator-(const Vector2D& v) const;	
	Vector2D	operator*(const Vector2D& v) const;	
	Vector2D	operator/(const Vector2D& v) const;	
	Vector2D	operator*(float fl) const;
	Vector2D	operator/(float fl) const;			
	
	// Cross product between two vectors.
	Vector2D	Cross(const Vector2D &vOther) const;		

	// Returns a Vector2D with the min or max in X, Y, and Z.
	Vector2D	Min(const Vector2D &vOther) const;
	Vector2D	Max(const Vector2D &vOther) const;

#else

private:
	// No copy constructors allowed if we're in optimal mode
	Vector2D(Vector2D const& vOther);

	// No assignment operators either...
	Vector2D& operator=( Vector2D const& src );

#endif
};


//-----------------------------------------------------------------------------
// Vector2D related operations
//-----------------------------------------------------------------------------

// Vector2D clear
void Vector2DClear( Vector2D& a );

// Copy
void Vector2DCopy( Vector2D const& src, Vector2D& dst );

// Vector2D arithmetic
void Vector2DAdd( Vector2D const& a, Vector2D const& b, Vector2D& result );
void Vector2DSubtract( Vector2D const& a, Vector2D const& b, Vector2D& result );
void Vector2DMultiply( Vector2D const& a, vec_t b, Vector2D& result );
void Vector2DMultiply( Vector2D const& a, Vector2D const& b, Vector2D& result );
void Vector2DDivide( Vector2D const& a, vec_t b, Vector2D& result );
void Vector2DDivide( Vector2D const& a, Vector2D const& b, Vector2D& result );
void Vector2DMA( Vector2D const& start, float s, Vector2D const& dir, Vector2D& result );

// Store the min or max of each of x, y, and z into the result.
void Vector2DMin( Vector2D const &a, Vector2D const &b, Vector2D &result );
void Vector2DMax( Vector2D const &a, Vector2D const &b, Vector2D &result );

#define Vector2DExpand( v ) (v).x, (v).y

// Normalization
vec_t Vector2DNormalize( Vector2D& v );

// Length
vec_t Vector2DLength( Vector2D const& v );

// Dot Product
vec_t DotProduct2D(Vector2D const& a, Vector2D const& b);

// Linearly interpolate between two vectors
void Vector2DLerp(Vector2D const& src1, Vector2D const& src2, vec_t t, Vector2D& dest );


//-----------------------------------------------------------------------------
//
// Inlined Vector2D methods
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------

inline Vector2D::Vector2D(void)									
{ 
#ifdef _DEBUG
	// Initialize to NAN to catch errors
	x = y = VEC_T_NAN;
#endif
}

inline Vector2D::Vector2D(vec_t X, vec_t Y)						
{ 
	x = X; y = Y;
	assert( IsValid() );
}

inline Vector2D::Vector2D(const float *pFloat)					
{
	assert( pFloat );
	x = pFloat[0]; y = pFloat[1];	
	assert( IsValid() );
}


//-----------------------------------------------------------------------------
// copy constructor
//-----------------------------------------------------------------------------

inline Vector2D::Vector2D(const Vector2D &vOther)					
{ 
	assert( vOther.IsValid() );
	x = vOther.x; y = vOther.y;
}

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------

inline void Vector2D::Init( vec_t ix, vec_t iy )    
{ 
	x = ix; y = iy;
	assert( IsValid() );
}

inline void Vector2D::Random( float minVal, float maxVal )
{
	x = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	y = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
}

inline void Vector2DClear( Vector2D& a )
{
	a.x = a.y = 0.0f;
}

//-----------------------------------------------------------------------------
// assignment
//-----------------------------------------------------------------------------

inline Vector2D& Vector2D::operator=(const Vector2D &vOther)	
{
	assert( vOther.IsValid() );
	x=vOther.x; y=vOther.y;
	return *this; 
}

//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------

inline vec_t& Vector2D::operator[](int i)
{
	assert( (i >= 0) && (i < 2) );
	return ((vec_t*)this)[i];
}

inline vec_t Vector2D::operator[](int i) const
{
	assert( (i >= 0) && (i < 2) );
	return ((vec_t*)this)[i];
}

//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------

inline vec_t* Vector2D::Base()
{
	return (vec_t*)this;
}

inline vec_t const* Vector2D::Base() const
{
	return (vec_t const*)this;
}

//-----------------------------------------------------------------------------
// IsValid?
//-----------------------------------------------------------------------------

inline bool Vector2D::IsValid() const
{
	return IsFinite(x) && IsFinite(y);
}

//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------

inline bool Vector2D::operator==( Vector2D const& src ) const
{
	assert( src.IsValid() && IsValid() );
	return (src.x == x) && (src.y == y);
}

inline bool Vector2D::operator!=( Vector2D const& src ) const
{
	assert( src.IsValid() && IsValid() );
	return (src.x != x) || (src.y != y);
}


//-----------------------------------------------------------------------------
// Copy
//-----------------------------------------------------------------------------

inline void Vector2DCopy( Vector2D const& src, Vector2D& dst )
{
	assert( src.IsValid() );
	dst.x = src.x;
	dst.y = src.y;
}

inline void	Vector2D::CopyToArray(float* rgfl) const		
{ 
	assert( IsValid() );
	assert( rgfl );
	rgfl[0] = x; rgfl[1] = y; 
}

//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------

inline void Vector2D::Negate()
{ 
	assert( IsValid() );
	x = -x; y = -y;
} 

inline Vector2D& Vector2D::operator+=(const Vector2D& v)	
{ 
	assert( IsValid() && v.IsValid() );
	x+=v.x; y+=v.y;	
	return *this;
}

inline Vector2D& Vector2D::operator-=(const Vector2D& v)	
{ 
	assert( IsValid() && v.IsValid() );
	x-=v.x; y-=v.y;	
	return *this;
}

inline Vector2D& Vector2D::operator*=(float fl)	
{
	x *= fl;
	y *= fl;
	assert( IsValid() );
	return *this;
}

inline Vector2D& Vector2D::operator*=(Vector2D const& v)	
{ 
	x *= v.x;
	y *= v.y;
	assert( IsValid() );
	return *this;
}

inline Vector2D& Vector2D::operator/=(float fl)	
{
	assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	assert( IsValid() );
	return *this;
}

inline Vector2D& Vector2D::operator/=(Vector2D const& v)	
{ 
	assert( v.x != 0.0f && v.y != 0.0f );
	x /= v.x;
	y /= v.y;
	assert( IsValid() );
	return *this;
}

inline void Vector2DAdd( Vector2D const& a, Vector2D const& b, Vector2D& c )
{
	assert( a.IsValid() && b.IsValid() );
	c.x = a.x + b.x;
	c.y = a.y + b.y;
}

inline void Vector2DSubtract( Vector2D const& a, Vector2D const& b, Vector2D& c )
{
	assert( a.IsValid() && b.IsValid() );
	c.x = a.x - b.x;
	c.y = a.y - b.y;
}

inline void Vector2DMultiply( Vector2D const& a, vec_t b, Vector2D& c )
{
	assert( a.IsValid() && IsFinite(b) );
	c.x = a.x * b;
	c.y = a.y * b;
}

inline void Vector2DMultiply( Vector2D const& a, Vector2D const& b, Vector2D& c )
{				  
	assert( a.IsValid() && b.IsValid() );
	c.x = a.x * b.x;
	c.y = a.y * b.y;
}


inline void Vector2DDivide( Vector2D const& a, vec_t b, Vector2D& c )
{
	assert( a.IsValid() );
	assert( b != 0.0f );
	vec_t oob = 1.0f / b;
	c.x = a.x * oob;
	c.y = a.y * oob;
}

inline void Vector2DDivide( Vector2D const& a, Vector2D const& b, Vector2D& c )
{
	assert( a.IsValid() );
	assert( (b.x != 0.0f) && (b.y != 0.0f) );
	c.x = a.x / b.x;
	c.y = a.y / b.y;
}

inline void Vector2DMA( Vector2D const& start, float s, Vector2D const& dir, Vector2D& result )
{
	assert( start.IsValid() && IsFinite(s) && dir.IsValid() );
	result.x = start.x + s*dir.x;
	result.y = start.y + s*dir.y;
}

// FIXME: Remove
// For backwards compatability
inline void	Vector2D::MulAdd(Vector2D const& a, Vector2D const& b, float scalar)
{
	x = a.x + b.x * scalar;
	y = a.y + b.y * scalar;
}

inline void Vector2DLerp(const Vector2D& src1, const Vector2D& src2, vec_t t, Vector2D& dest )
{
	dest[0] = src1[0] + (src2[0] - src1[0]) * t;
	dest[1] = src1[1] + (src2[1] - src1[1]) * t;
}

//-----------------------------------------------------------------------------
// dot, cross
//-----------------------------------------------------------------------------

inline vec_t DotProduct2D(const Vector2D& a, const Vector2D& b) 
{ 
	assert( a.IsValid() && b.IsValid() );
	return( a.x*b.x + a.y*b.y ); 
}

// for backwards compatability
inline vec_t Vector2D::Dot( Vector2D const& vOther ) const
{
	return DotProduct2D( *this, vOther );
}


//-----------------------------------------------------------------------------
// length
//-----------------------------------------------------------------------------

inline vec_t Vector2DLength( Vector2D const& v )
{
	assert( v.IsValid() );
	return (vec_t)FastSqrt(v.x*v.x + v.y*v.y);		
}

inline vec_t Vector2D::LengthSqr(void) const	
{ 
	assert( IsValid() );
	return (x*x + y*y);		
}

inline vec_t Vector2D::Length(void) const	
{
	return Vector2DLength( *this );
}


inline void Vector2DMin( Vector2D const &a, Vector2D const &b, Vector2D &result )
{
	result.x = (a.x < b.x) ? a.x : b.x;
	result.y = (a.y < b.y) ? a.y : b.y;
}


inline void Vector2DMax( Vector2D const &a, Vector2D const &b, Vector2D &result )
{
	result.x = (a.x > b.x) ? a.x : b.x;
	result.y = (a.y > b.y) ? a.y : b.y;
}


//-----------------------------------------------------------------------------
// Normalization
//-----------------------------------------------------------------------------

inline vec_t Vector2DNormalize( Vector2D& v )
{
	assert( v.IsValid() );
	vec_t l = v.Length();
	if (l != 0.0f)
	{
		v /= l;
	}
	else
	{
		v.x = v.y = 0.0f; 
	}
	return l;
}

//-----------------------------------------------------------------------------
// Get the distance from this Vector2D to the other one 
//-----------------------------------------------------------------------------

inline vec_t Vector2D::DistTo(const Vector2D &vOther) const
{
	Vector2D delta;
	Vector2DSubtract( *this, vOther, delta );
	return delta.Length();
}

inline vec_t Vector2D::DistToSqr(const Vector2D &vOther) const
{
	Vector2D delta;
	Vector2DSubtract( *this, vOther, delta );
	return delta.LengthSqr();
}



//-----------------------------------------------------------------------------
//
// Slow methods
//
//-----------------------------------------------------------------------------

#ifndef VECTOR_NO_SLOW_OPERATIONS

//-----------------------------------------------------------------------------
// Returns a Vector2D with the min or max in X, Y, and Z.
//-----------------------------------------------------------------------------

inline Vector2D Vector2D::Min(const Vector2D &vOther) const
{
	return Vector2D(x < vOther.x ? x : vOther.x, 
		y < vOther.y ? y : vOther.y);
}

inline Vector2D Vector2D::Max(const Vector2D &vOther) const
{
	return Vector2D(x > vOther.x ? x : vOther.x, 
		y > vOther.y ? y : vOther.y);
}


//-----------------------------------------------------------------------------
// arithmetic operations
//-----------------------------------------------------------------------------

inline Vector2D Vector2D::operator-(void) const
{ 
	return Vector2D(-x,-y);				
}

inline Vector2D Vector2D::operator+(const Vector2D& v) const	
{ 
	Vector2D res;
	Vector2DAdd( *this, v, res );
	return res;	
}

inline Vector2D Vector2D::operator-(const Vector2D& v) const	
{ 
	Vector2D res;
	Vector2DSubtract( *this, v, res );
	return res;	
}

inline Vector2D Vector2D::operator*(float fl) const	
{ 
	Vector2D res;
	Vector2DMultiply( *this, fl, res );
	return res;	
}

inline Vector2D Vector2D::operator*(Vector2D const& v) const	
{ 
	Vector2D res;
	Vector2DMultiply( *this, v, res );
	return res;	
}

inline Vector2D Vector2D::operator/(float fl) const	
{ 
	Vector2D res;
	Vector2DDivide( *this, fl, res );
	return res;	
}

inline Vector2D Vector2D::operator/(Vector2D const& v) const	
{ 
	Vector2D res;
	Vector2DDivide( *this, v, res );
	return res;	
}

inline Vector2D operator*(float fl, const Vector2D& v)	
{ 
	return v * fl; 
}

#endif //slow


#endif // _cplusplus

#endif // VECTOR2D_H

