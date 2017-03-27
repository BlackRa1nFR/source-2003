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

#ifndef VECTOR_H
#define VECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include <math.h>
#include <float.h>
#include <assert.h>

// For vec_t, put this somewhere else?
#include "basetypes.h"

// For rand(). We really need a library!
#include <stdlib.h>

#include "vector2d.h"

// Uncomment this to add extra asserts to check for NANs, uninitialized vecs, etc.
//#define VECTOR_PARANOIA	1

// Uncomment this to make sure we don't do anything slow with our vectors
//#define VECTOR_NO_SLOW_OPERATIONS 1


// Used to make certain code easier to read.
#define X_INDEX	0
#define Y_INDEX	1
#define Z_INDEX	2


#ifdef VECTOR_PARANOIA
#define CHECK_VALID( _v)	assert( (_v).IsValid() )
#else
#define CHECK_VALID( _v)	0
#endif

#ifdef __cplusplus

// HACKHACK: Declare these directly from mathlib.h for now
extern "C" 
{
extern float (*pfSqrt)(float x);
};

#define FastSqrt(x)			(*pfSqrt)(x)

//=========================================================
// 3D Vector
//=========================================================

class Vector					
{
public:
	// Members
	vec_t x, y, z;

	// Construction/destruction:
	Vector(void); 
	Vector(vec_t X, vec_t Y, vec_t Z);
	
//	Vector(const float *pFloat);
	// inline Vector(QAngle const &angle);	// evil auto type promotion!!!

	// Initialization
	void Init(vec_t ix=0.0f, vec_t iy=0.0f, vec_t iz=0.0f);

	// Got any nasty NAN's?
	bool IsValid() const;

	// array access...
	vec_t operator[](int i) const;
	vec_t& operator[](int i);

	// Base address...
	vec_t* Base();
	vec_t const* Base() const;

	// Cast to Vector2D...
	Vector2D& AsVector2D();
	Vector2D const& AsVector2D() const;

	// Initialization methods
	void Random( vec_t minVal, vec_t maxVal );

	// equality
	bool operator==(const Vector& v) const;
	bool operator!=(const Vector& v) const;	

	// arithmetic operations
	FORCEINLINE Vector&	operator+=(const Vector &v);			
	FORCEINLINE Vector&	operator-=(const Vector &v);		
	FORCEINLINE Vector&	operator*=(const Vector &v);			
	FORCEINLINE Vector&	operator*=(float s);
	FORCEINLINE Vector&	operator/=(const Vector &v);		
	FORCEINLINE Vector&	operator/=(float s);					

	// negate the vector components
	void	Negate(); 

	// Get the vector's magnitude.
	inline vec_t	Length() const;

	// Get the vector's magnitude squared.
	FORCEINLINE vec_t LengthSqr(void) const
	{ 
		CHECK_VALID(*this);
		return (x*x + y*y + z*z);		
	}


	// Get the distance from this vector to the other one.
	vec_t	DistTo(const Vector &vOther) const;

	// Get the distance from this vector to the other one squared.
	// NJS: note, VC wasn't inlining it correctly in several deeply nested inlines due to being an 'out of line' inline.  
	// may be able to tidy this up after switching to VC7
	FORCEINLINE vec_t DistToSqr(const Vector &vOther) const
	{
		Vector delta;

		delta.x = x - vOther.x;
		delta.y = y - vOther.y;
		delta.z = z - vOther.z;

		return delta.LengthSqr();
	}

	// Copy
	void	CopyToArray(float* rgfl) const;	

	// Multiply, add, and assign to this (ie: *this = a + b * scalar). This
	// is about 12% faster than the actual vector equation (because it's done per-component
	// rather than per-vector).
	void	MulAdd(Vector const& a, Vector const& b, float scalar);	

	// Dot product.
	vec_t	Dot(Vector const& vOther) const;			

#ifndef VECTOR_NO_SLOW_OPERATIONS
	// copy constructors
//	Vector(const Vector &vOther);

	// assignment
	Vector& operator=(const Vector &vOther);

	// arithmetic operations
	Vector	operator-(void) const;
				
	Vector	operator+(const Vector& v) const;	
	Vector	operator-(const Vector& v) const;	
	Vector	operator*(const Vector& v) const;	
	Vector	operator/(const Vector& v) const;	
	Vector	operator*(float fl) const;
	Vector	operator/(float fl) const;			
	
	// Cross product between two vectors.
	Vector	Cross(const Vector &vOther) const;		

	// Returns a vector with the min or max in X, Y, and Z.
	Vector	Min(const Vector &vOther) const;
	Vector	Max(const Vector &vOther) const;

	// 2d
	vec_t	Length2D(void) const;					

#else

private:
	// No copy constructors allowed if we're in optimal mode
	inline Vector(Vector const& vOther);

	// No assignment operators either...
	Vector& operator=( Vector const& src );

#endif
};

//-----------------------------------------------------------------------------
// Utility to simplify table construction. No constructor means can use
// traditional C-style initialization
//-----------------------------------------------------------------------------

class TableVector
{
public:
	vec_t x, y, z;

	operator Vector &()				{ return *((Vector *)(this)); }
	operator const Vector &() const	{ return *((Vector *)(this)); }
};

//-----------------------------------------------------------------------------
// Here's where we add all those lovely SSE optimized routines
//-----------------------------------------------------------------------------

#ifdef _WIN32
class __declspec(align(16)) VectorAligned : public Vector
#elif _LINUX
class __attribute__((aligned(16))) VectorAligned : public Vector
#endif
{
public:
	inline VectorAligned(void) {};
	inline VectorAligned(vec_t X, vec_t Y, vec_t Z) 
	{
		Init(X,Y,Z);
	}

};

//-----------------------------------------------------------------------------
// Vector related operations
//-----------------------------------------------------------------------------

// Vector clear
FORCEINLINE void VectorClear( Vector& a );

// Copy
FORCEINLINE void VectorCopy( Vector const& src, Vector& dst );

// Vector arithmetic
FORCEINLINE void VectorAdd( Vector const& a, Vector const& b, Vector& result );
FORCEINLINE void VectorSubtract( Vector const& a, Vector const& b, Vector& result );
FORCEINLINE void VectorMultiply( Vector const& a, vec_t b, Vector& result );
FORCEINLINE void VectorMultiply( Vector const& a, Vector const& b, Vector& result );
FORCEINLINE void VectorDivide( Vector const& a, vec_t b, Vector& result );
FORCEINLINE void VectorDivide( Vector const& a, Vector const& b, Vector& result );
inline void VectorScale ( Vector const& in, vec_t scale, Vector& result );
// void VectorMA( Vector const& start, float s, Vector const& dir, Vector& result );

// Vector equality with tolerance
bool VectorsAreEqual( Vector const& src1, Vector const& src2, float tolerance = 0.0f );

#define VectorExpand(v) (v).x, (v).y, (v).z


// Normalization
// FIXME: Can't use quite yet
//vec_t VectorNormalize( Vector& v );

// Length
inline vec_t VectorLength( Vector const& v );

// Dot Product
FORCEINLINE vec_t DotProduct(Vector const& a, Vector const& b);

// Cross product
void CrossProduct(Vector const& a, Vector const& b, Vector& result );

// Store the min or max of each of x, y, and z into the result.
void VectorMin( Vector const &a, Vector const &b, Vector &result );
void VectorMax( Vector const &a, Vector const &b, Vector &result );

// Linearly interpolate between two vectors
void VectorLerp(Vector const& src1, Vector const& src2, vec_t t, Vector& dest );

#ifndef VECTOR_NO_SLOW_OPERATIONS

// Cross product
Vector CrossProduct( const Vector& a, const Vector& b );

// Random vector creation
Vector RandomVector( vec_t minVal, vec_t maxVal );

#endif

//-----------------------------------------------------------------------------
//
// Inlined Vector methods
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------
inline Vector::Vector(void)									
{ 
#ifdef _DEBUG
#ifdef VECTOR_PARANOIA
	// Initialize to NAN to catch errors
	x = y = z = VEC_T_NAN;
#endif
#endif
}

inline Vector::Vector(vec_t X, vec_t Y, vec_t Z)						
{ 
	x = X; y = Y; z = Z;
	CHECK_VALID(*this);
}

//inline Vector::Vector(const float *pFloat)					
//{
//	assert( pFloat );
//	x = pFloat[0]; y = pFloat[1]; z = pFloat[2];	
//	CHECK_VALID(*this);
//} 

#if 0
//-----------------------------------------------------------------------------
// copy constructor
//-----------------------------------------------------------------------------

inline Vector::Vector(const Vector &vOther)					
{ 
	CHECK_VALID(vOther);
	x = vOther.x; y = vOther.y; z = vOther.z;
}
#endif

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------

inline void Vector::Init( vec_t ix, vec_t iy, vec_t iz )    
{ 
	x = ix; y = iy; z = iz;
	CHECK_VALID(*this);
}

inline void Vector::Random( vec_t minVal, vec_t maxVal )
{
	x = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	y = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	z = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	CHECK_VALID(*this);
}

inline void VectorClear( Vector& a )
{
	a.x = a.y = a.z = 0.0f;
}

//-----------------------------------------------------------------------------
// assignment
//-----------------------------------------------------------------------------

inline Vector& Vector::operator=(const Vector &vOther)	
{
	CHECK_VALID(vOther);
	x=vOther.x; y=vOther.y; z=vOther.z; 
	return *this; 
}

//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------

inline vec_t& Vector::operator[](int i)
{
	assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}

inline vec_t Vector::operator[](int i) const
{
	assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}

//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------

inline vec_t* Vector::Base()
{
	return (vec_t*)this;
}

inline vec_t const* Vector::Base() const
{
	return (vec_t const*)this;
}

//-----------------------------------------------------------------------------
// Cast to Vector2D...
//-----------------------------------------------------------------------------

inline Vector2D& Vector::AsVector2D()
{
	return *(Vector2D*)this;
}

inline Vector2D const& Vector::AsVector2D() const
{
	return *(Vector2D const*)this;
}

//-----------------------------------------------------------------------------
// IsValid?
//-----------------------------------------------------------------------------

inline bool Vector::IsValid() const
{
	return IsFinite(x) && IsFinite(y) && IsFinite(z);
}

//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------

inline bool Vector::operator==( Vector const& src ) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x == x) && (src.y == y) && (src.z == z);
}

inline bool Vector::operator!=( Vector const& src ) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x != x) || (src.y != y) || (src.z != z);
}


//-----------------------------------------------------------------------------
// Copy
//-----------------------------------------------------------------------------

FORCEINLINE void VectorCopy( Vector const& src, Vector& dst )
{
	CHECK_VALID(src);
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

inline void	Vector::CopyToArray(float* rgfl) const		
{ 
	assert( rgfl );
	CHECK_VALID(*this);
	rgfl[0] = x, rgfl[1] = y, rgfl[2] = z; 
}

//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------

inline void Vector::Negate()
{ 
	CHECK_VALID(*this);
	x = -x; y = -y; z = -z; 
} 

FORCEINLINE  Vector& Vector::operator+=(const Vector& v)	
{ 
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x+=v.x; y+=v.y; z += v.z;	
	return *this;
}

FORCEINLINE  Vector& Vector::operator-=(const Vector& v)	
{ 
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x-=v.x; y-=v.y; z -= v.z;	
	return *this;
}

FORCEINLINE  Vector& Vector::operator*=(float fl)	
{
	x *= fl;
	y *= fl;
	z *= fl;
	CHECK_VALID(*this);
	return *this;
}

FORCEINLINE  Vector& Vector::operator*=(Vector const& v)	
{ 
	CHECK_VALID(v);
	x *= v.x;
	y *= v.y;
	z *= v.z;
	CHECK_VALID(*this);
	return *this;
}

FORCEINLINE  Vector& Vector::operator/=(float fl)	
{
	assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	z *= oofl;
	CHECK_VALID(*this);
	return *this;
}

FORCEINLINE  Vector& Vector::operator/=(Vector const& v)	
{ 
	CHECK_VALID(v);
	assert( v.x != 0.0f && v.y != 0.0f && v.z != 0.0f );
	x /= v.x;
	y /= v.y;
	z /= v.z;
	CHECK_VALID(*this);
	return *this;
}

FORCEINLINE void VectorAdd( Vector const& a, Vector const& b, Vector& c )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	c.z = a.z + b.z;
}

FORCEINLINE void VectorSubtract( Vector const& a, Vector const& b, Vector& c )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	c.z = a.z - b.z;
}

FORCEINLINE void VectorMultiply( Vector const& a, vec_t b, Vector& c )
{
	CHECK_VALID(a);
	assert( IsFinite(b) );
	c.x = a.x * b;
	c.y = a.y * b;
	c.z = a.z * b;
}

FORCEINLINE void VectorMultiply( Vector const& a, Vector const& b, Vector& c )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	c.x = a.x * b.x;
	c.y = a.y * b.y;
	c.z = a.z * b.z;
}

// for backwards compatability
inline void VectorScale ( Vector const& in, vec_t scale, Vector& result )
{
	VectorMultiply( in, scale, result );
}


FORCEINLINE void VectorDivide( Vector const& a, vec_t b, Vector& c )
{
	CHECK_VALID(a);
	assert( b != 0.0f );
	vec_t oob = 1.0f / b;
	c.x = a.x * oob;
	c.y = a.y * oob;
	c.z = a.z * oob;
}

FORCEINLINE void VectorDivide( Vector const& a, Vector const& b, Vector& c )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	assert( (b.x != 0.0f) && (b.y != 0.0f) && (b.z != 0.0f) );
	c.x = a.x / b.x;
	c.y = a.y / b.y;
	c.z = a.z / b.z;
}

void VectorMA( Vector const& start, float s, Vector const& dir, Vector& result )
#if 0
{
	CHECK_VALID(start);
	CHECK_VALID(dir);
	assert( IsFinite(s) );
	result.x = start.x + s*dir.x;
	result.y = start.y + s*dir.y;
	result.z = start.z + s*dir.z;
}
#else
;
#endif

// FIXME: Remove
// For backwards compatability
inline void	Vector::MulAdd(Vector const& a, Vector const& b, float scalar)
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	x = a.x + b.x * scalar;
	y = a.y + b.y * scalar;
	z = a.z + b.z * scalar;
}

inline void VectorLerp(const Vector& src1, const Vector& src2, vec_t t, Vector& dest )
{
	CHECK_VALID(src1);
	CHECK_VALID(src2);
	dest[0] = src1[0] + (src2[0] - src1[0]) * t;
	dest[1] = src1[1] + (src2[1] - src1[1]) * t;
	dest[2] = src1[2] + (src2[2] - src1[2]) * t;
}


//-----------------------------------------------------------------------------
// Temporary storage for vector results so const Vector& results can be returned
//-----------------------------------------------------------------------------
inline Vector &AllocTempVector()
{
	static Vector s_vecTemp[32];
	static int s_nIndex = 0;

	s_nIndex = (s_nIndex + 1) & 0x1F;
	return s_vecTemp[s_nIndex];
}


//-----------------------------------------------------------------------------
// dot, cross
//-----------------------------------------------------------------------------
FORCEINLINE vec_t DotProduct(const Vector& a, const Vector& b) 
{ 
	CHECK_VALID(a);
	CHECK_VALID(b);
	return( a.x*b.x + a.y*b.y + a.z*b.z ); 
}

// for backwards compatability
inline vec_t Vector::Dot( Vector const& vOther ) const
{
	CHECK_VALID(vOther);
	return DotProduct( *this, vOther );
}

inline void CrossProduct(Vector const& a, Vector const& b, Vector& result )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	result.x = a.y*b.z - a.z*b.y;
	result.y = a.z*b.x - a.x*b.z;
	result.z = a.x*b.y - a.y*b.x;
}

inline vec_t DotProductAbs( const Vector &v0, const Vector &v1 )
{
	CHECK_VALID(v0);
	CHECK_VALID(v1);
	return FloatMakePositive(v0.x*v1.x) + FloatMakePositive(v0.y*v1.y) + FloatMakePositive(v0.z*v1.z);
}

inline vec_t DotProductAbs( const Vector &v0, const float *v1 )
{
	return FloatMakePositive(v0.x * v1[0]) + FloatMakePositive(v0.y * v1[1]) + FloatMakePositive(v0.z * v1[2]);
}

//-----------------------------------------------------------------------------
// length
//-----------------------------------------------------------------------------

inline vec_t VectorLength( Vector const& v )
{
	CHECK_VALID(v);
	return (vec_t)FastSqrt(v.x*v.x + v.y*v.y + v.z*v.z);		
}


inline vec_t Vector::Length(void) const	
{
	CHECK_VALID(*this);
	return VectorLength( *this );
}


//-----------------------------------------------------------------------------
// Normalization
//-----------------------------------------------------------------------------

/*
// FIXME: Can't use until we're un-macroed in mathlib.h
inline vec_t VectorNormalize( Vector& v )
{
	assert( v.IsValid() );
	vec_t l = v.Length();
	if (l != 0.0f)
	{
		v /= l;
	}
	else
	{
		// FIXME: 
		// Just copying the existing implemenation; shouldn't res.z == 0?
		v.x = v.y = 0.0f; v.z = 1.0f;
	}
	return l;
}
*/

//-----------------------------------------------------------------------------
// Get the distance from this vector to the other one 
//-----------------------------------------------------------------------------

inline vec_t Vector::DistTo(const Vector &vOther) const
{
	Vector delta;
	VectorSubtract( *this, vOther, delta );
	return delta.Length();
}




//-----------------------------------------------------------------------------
// Vector equality with tolerance
//-----------------------------------------------------------------------------
inline bool VectorsAreEqual( Vector const& src1, Vector const& src2, float tolerance )
{
	if (FloatMakePositive(src1.x - src2.x) > tolerance)
		return false;
	if (FloatMakePositive(src1.y - src2.y) > tolerance)
		return false;
	return (FloatMakePositive(src1.z - src2.z) <= tolerance);
}


//-----------------------------------------------------------------------------
// Takes the absolute value of a vector
//-----------------------------------------------------------------------------
inline void VectorAbs( Vector const& src, Vector& dst )
{
	dst.x = FloatMakePositive(src.x);
	dst.y = FloatMakePositive(src.y);
	dst.z = FloatMakePositive(src.z);
}


//-----------------------------------------------------------------------------
//
// Slow methods
//
//-----------------------------------------------------------------------------

#ifndef VECTOR_NO_SLOW_OPERATIONS

//-----------------------------------------------------------------------------
// Returns a vector with the min or max in X, Y, and Z.
//-----------------------------------------------------------------------------
inline Vector Vector::Min(const Vector &vOther) const
{
	return Vector(x < vOther.x ? x : vOther.x, 
		y < vOther.y ? y : vOther.y, 
		z < vOther.z ? z : vOther.z);
}

inline Vector Vector::Max(const Vector &vOther) const
{
	return Vector(x > vOther.x ? x : vOther.x, 
		y > vOther.y ? y : vOther.y, 
		z > vOther.z ? z : vOther.z);
}


//-----------------------------------------------------------------------------
// arithmetic operations
//-----------------------------------------------------------------------------

inline Vector Vector::operator-(void) const
{ 
	return Vector(-x,-y,-z);				
}

inline Vector Vector::operator+(const Vector& v) const	
{ 
	Vector res;
	VectorAdd( *this, v, res );
	return res;	
}

inline Vector Vector::operator-(const Vector& v) const	
{ 
	Vector res;
	VectorSubtract( *this, v, res );
	return res;	
}

inline Vector Vector::operator*(float fl) const	
{ 
	Vector res;
	VectorMultiply( *this, fl, res );
	return res;	
}

inline Vector Vector::operator*(Vector const& v) const	
{ 
	Vector res;
	VectorMultiply( *this, v, res );
	return res;	
}

inline Vector Vector::operator/(float fl) const	
{ 
	Vector res;
	VectorDivide( *this, fl, res );
	return res;	
}

inline Vector Vector::operator/(Vector const& v) const	
{ 
	Vector res;
	VectorDivide( *this, v, res );
	return res;	
}

inline Vector operator*(float fl, const Vector& v)	
{ 
	return v * fl; 
}

//-----------------------------------------------------------------------------
// cross product
//-----------------------------------------------------------------------------

inline Vector Vector::Cross(const Vector& vOther) const
{ 
	Vector res;
	CrossProduct( *this, vOther, res );
	return res;
}

//-----------------------------------------------------------------------------
// 2D
//-----------------------------------------------------------------------------

inline vec_t Vector::Length2D(void) const
{ 
	return (vec_t)FastSqrt(x*x + y*y); 
}

inline Vector CrossProduct(const Vector& a, const Vector& b) 
{ 
	return Vector( a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x ); 
}

inline void VectorMin( Vector const &a, Vector const &b, Vector &result )
{
	result.x = (a.x < b.x) ? a.x : b.x;
	result.y = (a.y < b.y) ? a.y : b.y;
	result.z = (a.z < b.z) ? a.z : b.z;
}

inline void VectorMax( Vector const &a, Vector const &b, Vector &result )
{
	result.x = (a.x > b.x) ? a.x : b.x;
	result.y = (a.y > b.y) ? a.y : b.y;
	result.z = (a.z > b.z) ? a.z : b.z;
}

// Get a random vector.
inline Vector RandomVector( float minVal, float maxVal )
{
	Vector random;
	random.Random( minVal, maxVal );
	return random;
}

#endif //slow

//-----------------------------------------------------------------------------
// Helper debugging stuff....
//-----------------------------------------------------------------------------

inline bool operator==( float const* f, Vector const& v )
{
	// AIIIEEEE!!!!
	assert(0);
	return false;
}

inline bool operator==( Vector const& v, float const* f )
{
	// AIIIEEEE!!!!
	assert(0);
	return false;
}

inline bool operator!=( float const* f, Vector const& v )
{
	// AIIIEEEE!!!!
	assert(0);
	return false;
}

inline bool operator!=( Vector const& v, float const* f )
{
	// AIIIEEEE!!!!
	assert(0);
	return false;
}


//-----------------------------------------------------------------------------
// AngularImpulse
//-----------------------------------------------------------------------------
// AngularImpulse are exponetial maps (an axis scaled by a "twist" angle in degrees)
class AngularImpulse : public Vector
{
public:
	inline AngularImpulse( void ) {}
	inline AngularImpulse( const Vector &init ) : Vector(init) {}
	inline AngularImpulse( float x, float y, float z ) : Vector(x,y,z) {}
};

inline AngularImpulse RandomAngularImpulse( float minVal, float maxVal )
{
	AngularImpulse	angImp;
	angImp.Random( minVal, maxVal );
	return angImp;
}

//-----------------------------------------------------------------------------
// Quaternion
//-----------------------------------------------------------------------------

class RadianEuler;

class Quaternion				// same data-layout as engine's vec4_t,
{								//		which is a vec_t[4]
public:
	inline Quaternion(void)							{ }
	inline Quaternion(RadianEuler const &angle);	// evil auto type promotion!!!

	inline void Init(vec_t ix=0.0f, vec_t iy=0.0f, vec_t iz=0.0f, vec_t iw=0.0f)	{ x = ix; y = iy; z = iz; w = iw; }

	bool IsValid() const;

	// array access...
	vec_t operator[](int i) const;
	vec_t& operator[](int i);

	vec_t x, y, z, w;
};


//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------

inline vec_t& Quaternion::operator[](int i)
{
	assert( (i >= 0) && (i < 4) );
	return ((vec_t*)this)[i];
}

inline vec_t Quaternion::operator[](int i) const
{
	assert( (i >= 0) && (i < 4) );
	return ((vec_t*)this)[i];
}

//-----------------------------------------------------------------------------
// Radian Euler QAngle aligned to axis (NOT ROLL/PITCH/YAW)
//-----------------------------------------------------------------------------

class RadianEuler
{
public:
	inline RadianEuler(void)							{ }
	inline RadianEuler(vec_t X, vec_t Y, vec_t Z)		{ x = X; y = Y; z = Z; }
	inline RadianEuler(Quaternion const &q);	// evil auto type promotion!!!

	// Initialization
	inline void Init(vec_t ix=0.0f, vec_t iy=0.0f, vec_t iz=0.0f)	{ x = ix; y = iy; z = iz; }

	bool IsValid() const;

	// array access...
	vec_t operator[](int i) const;
	vec_t& operator[](int i);

	vec_t x, y, z;
};


extern void AngleQuaternion( RadianEuler const &angles, Quaternion &qt );
extern void QuaternionAngles( Quaternion const &q, RadianEuler &angles );
inline Quaternion::Quaternion(RadianEuler const &angle)
{
	AngleQuaternion( angle, *this );
}

inline bool Quaternion::IsValid() const
{
	return IsFinite(x) && IsFinite(y) && IsFinite(z) && IsFinite(w);
}

inline RadianEuler::RadianEuler(Quaternion const &q)
{
	QuaternionAngles( q, *this );
}

inline void VectorCopy( RadianEuler const& src, RadianEuler &dst )
{
	CHECK_VALID(src);
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

inline void VectorScale( RadianEuler const& src, float b, RadianEuler &dst )
{
	CHECK_VALID(src);
	assert( IsFinite(b) );
	dst.x = src.x * b;
	dst.y = src.y * b;
	dst.z = src.z * b;
}

inline bool RadianEuler::IsValid() const
{
	return IsFinite(x) && IsFinite(y) && IsFinite(z);
}

//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------

inline vec_t& RadianEuler::operator[](int i)
{
	assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}

inline vec_t RadianEuler::operator[](int i) const
{
	assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}

//-----------------------------------------------------------------------------
// Degree Euler QAngle pitch, yaw, roll
//-----------------------------------------------------------------------------
class QAngle					
{
public:
	// Members
	vec_t x, y, z;

	// Construction/destruction
	QAngle(void);
	QAngle(vec_t X, vec_t Y, vec_t Z);

	// Initialization
	void Init(vec_t ix=0.0f, vec_t iy=0.0f, vec_t iz=0.0f);
	void Random( vec_t minVal, vec_t maxVal );

	// Got any nasty NAN's?
	bool IsValid() const;

	// array access...
	vec_t operator[](int i) const;
	vec_t& operator[](int i);

	// Base address...
	vec_t* Base();
	vec_t const* Base() const;
	
	// equality
	bool operator==(const QAngle& v) const;
	bool operator!=(const QAngle& v) const;	

	// arithmetic operations
	QAngle&	operator+=(const QAngle &v);
	QAngle&	operator-=(const QAngle &v);
	QAngle&	operator*=(float s);
	QAngle&	operator/=(float s);

	// Get the vector's magnitude.
	vec_t	Length() const;
	vec_t	LengthSqr() const;

	// negate the QAngle components
	//void	Negate(); 

#ifndef VECTOR_NO_SLOW_OPERATIONS
	// copy constructors
	// QAngle(const QAngle &vOther);

	// assignment
	QAngle& operator=(const QAngle &vOther);

	// arithmetic operations
	QAngle	operator-(void) const;

	QAngle	operator+(const QAngle& v) const;
	QAngle	operator-(const QAngle& v) const;
	QAngle	operator*(float fl) const;
	QAngle	operator/(float fl) const;
#else

private:
	// No copy constructors allowed if we're in optimal mode
	QAngle(QAngle const& vOther);

	// No assignment operators either...
	QAngle& operator=( QAngle const& src );

#endif
};

inline void VectorAdd( QAngle const& a, QAngle const& b, QAngle& result )
{
	CHECK_VALID(a);
	CHECK_VALID(b);
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
}

inline void VectorMA( const QAngle &start, float scale, const QAngle &direction, QAngle &dest )
{
	CHECK_VALID(start);
	CHECK_VALID(direction);
	dest.x = start.x + scale * direction.x;
	dest.y = start.y + scale * direction.y;
	dest.z = start.z + scale * direction.z;
}

//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------

inline QAngle::QAngle(void)									
{ 
#ifdef _DEBUG
#ifdef VECTOR_PARANOIA
	// Initialize to NAN to catch errors
	x = y = z = VEC_T_NAN;
#endif
#endif
}

inline QAngle::QAngle(vec_t X, vec_t Y, vec_t Z)						
{ 
	x = X; y = Y; z = Z;
	CHECK_VALID(*this);
}

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------

inline void QAngle::Init( vec_t ix, vec_t iy, vec_t iz )    
{ 
	x = ix; y = iy; z = iz;
	CHECK_VALID(*this);
}


inline void QAngle::Random( vec_t minVal, vec_t maxVal )
{
	x = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	y = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	z = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	CHECK_VALID(*this);
}

inline QAngle RandomAngle( float minVal, float maxVal )
{
	Vector random;
	random.Random( minVal, maxVal );
	return QAngle( random.x, random.y, random.z );
}

//-----------------------------------------------------------------------------
// assignment
//-----------------------------------------------------------------------------

inline QAngle& QAngle::operator=(const QAngle &vOther)	
{
	CHECK_VALID(vOther);
	x=vOther.x; y=vOther.y; z=vOther.z; 
	return *this; 
}


//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------

inline vec_t& QAngle::operator[](int i)
{
	assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}

inline vec_t QAngle::operator[](int i) const
{
	assert( (i >= 0) && (i < 3) );
	return ((vec_t*)this)[i];
}


//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------

inline vec_t* QAngle::Base()
{
	return (vec_t*)this;
}

inline vec_t const* QAngle::Base() const
{
	return (vec_t const*)this;
}

//-----------------------------------------------------------------------------
// IsValid?
//-----------------------------------------------------------------------------

inline bool QAngle::IsValid() const
{
	return IsFinite(x) && IsFinite(y) && IsFinite(z);
}


//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------

inline bool QAngle::operator==( QAngle const& src ) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x == x) && (src.y == y) && (src.z == z);
}

inline bool QAngle::operator!=( QAngle const& src ) const
{
	CHECK_VALID(src);
	CHECK_VALID(*this);
	return (src.x != x) || (src.y != y) || (src.z != z);
}

//-----------------------------------------------------------------------------
// Copy
//-----------------------------------------------------------------------------

inline void VectorCopy( QAngle const& src, QAngle& dst )
{
	CHECK_VALID(src);
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}


//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------


inline QAngle& QAngle::operator+=(const QAngle& v)	
{ 
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x+=v.x; y+=v.y; z += v.z;	
	return *this;
}

inline QAngle& QAngle::operator-=(const QAngle& v)	
{ 
	CHECK_VALID(*this);
	CHECK_VALID(v);
	x-=v.x; y-=v.y; z -= v.z;	
	return *this;
}

inline QAngle& QAngle::operator*=(float fl)	
{
	x *= fl;
	y *= fl;
	z *= fl;
	CHECK_VALID(*this);
	return *this;
}

inline QAngle& QAngle::operator/=(float fl)	
{
	assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	z *= oofl;
	CHECK_VALID(*this);
	return *this;
}


//-----------------------------------------------------------------------------
// length
//-----------------------------------------------------------------------------

inline vec_t QAngle::Length( ) const
{
	CHECK_VALID(*this);
	return (vec_t)FastSqrt( LengthSqr( ) );		
}


inline vec_t QAngle::LengthSqr( ) const
{
	CHECK_VALID(*this);
	return x * x + y * y + z * z;
}
	

//-----------------------------------------------------------------------------
// Vector equality with tolerance
//-----------------------------------------------------------------------------
inline bool QAnglesAreEqual( QAngle const& src1, QAngle const& src2, float tolerance = 0.0f )
{
	if (FloatMakePositive(src1.x - src2.x) > tolerance)
		return false;
	if (FloatMakePositive(src1.y - src2.y) > tolerance)
		return false;
	return (FloatMakePositive(src1.z - src2.z) <= tolerance);
}



//-----------------------------------------------------------------------------
// arithmetic operations
//-----------------------------------------------------------------------------

inline QAngle QAngle::operator-(void) const
{ 
	return QAngle(-x,-y,-z);				
}

inline QAngle QAngle::operator+(const QAngle& v) const	
{ 
	QAngle res;
	res.x = x + v.x;
	res.y = y + v.y;
	res.z = z + v.z;
	return res;	
}

inline QAngle QAngle::operator-(const QAngle& v) const	
{ 
	QAngle res;
	res.x = x - v.x;
	res.y = y - v.y;
	res.z = z - v.z;
	return res;	
}

inline QAngle QAngle::operator*(float fl) const	
{ 
	QAngle res;
	res.x = x * fl;
	res.y = y * fl;
	res.z = z * fl;
	return res;	
}

inline QAngle QAngle::operator/(float fl) const	
{ 
	QAngle res;
	res.x = x / fl;
	res.y = y / fl;
	res.z = z / fl;
	return res;	
}

inline QAngle operator*(float fl, const QAngle& v)	
{ 
	return v * fl; 
}


//-----------------------------------------------------------------------------
// NOTE: These are not completely correct.  The representations are not equivalent
// unless the QAngle represents a rotational impulse along a coordinate axis (x,y,z)
inline void QAngleToAngularImpulse( const QAngle &angles, AngularImpulse &impulse )
{
	impulse.x = angles.z;
	impulse.y = angles.x;
	impulse.z = angles.y;
}

inline void AngularImpulseToQAngle( const AngularImpulse &impulse, QAngle &angles )
{
	angles.x = impulse.y;
	angles.y = impulse.z;
	angles.z = impulse.x;
}



//-----------------------------------------------------------------------------

////////////////////////////////////////////////
// Various implementations of VectorNormalize //
////////////////////////////////////////////////

float FASTCALL _VectorNormalize (Vector& v);
float FASTCALL _SSE_VectorNormalize (Vector& v);
float FASTCALL _3DNow_VectorNormalize (Vector& v);

void FASTCALL _VectorNormalizeFast (Vector& v);
void FASTCALL _SSE_VectorNormalizeFast (Vector& v);
void FASTCALL _3DNow_VectorNormalizeFast (Vector& v);

extern float (FASTCALL *pfVectorNormalize)(Vector& v);
extern void (FASTCALL *pfVectorNormalizeFast)(Vector& v);

// FIXME: Change this back to a #define once we get rid of the vec_t version
FORCEINLINE float VectorNormalize( Vector& v )
{
	return (*pfVectorNormalize)(v);
}

// FIXME: Obsolete version of VectorNormalize, once we remove all the friggin float*s
FORCEINLINE float VectorNormalize( float * v )
{
	return VectorNormalize(*(reinterpret_cast<Vector *>(v)));
}

FORCEINLINE void VectorNormalizeFast( Vector& v )
{
	(*pfVectorNormalizeFast)(v);
}

#ifdef PFN_VECTORMA
/////////////////////////////////////////
// Various implementations of VectorMA //
/////////////////////////////////////////

void _VectorMA ( const Vector &start, float scale, const Vector &direction, Vector &dest );
void _SSE_VectorMA ( const Vector &start, float scale, const Vector &direction, Vector &dest );
void _3DNow_VectorMA ( const Vector &start, float scale, const Vector &direction, Vector &dest );

extern void (*pfVectorMA)( const Vector &start, float scale, const Vector &direction, Vector &dest );

// FIXME: Change this back to a #define once we get rid of the vec_t version
inline void VectorMA( const Vector &start, float scale, const Vector &direction, Vector &dest )
{
	(*pfVectorMA)(start,scale,direction,dest);
}
#endif

#endif // _cplusplus

#endif

