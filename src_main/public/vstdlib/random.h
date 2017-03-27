//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Random number generator
//
// $Workfile: $
// $NoKeywords: $
//=============================================================================

#ifndef VSTDLIB_RANDOM_H
#define VSTDLIB_RANDOM_H

#include "vstdlib/vstdlib.h"
#include "basetypes.h"
#include "interface.h"

#define NTAB 32

//-----------------------------------------------------------------------------
// A generator of uniformly distributed random numbers
//-----------------------------------------------------------------------------
class IUniformRandomStream
{
public:
	// Sets the seed of the random number generator
	virtual void	SetSeed( int iSeed ) = 0;

	// Generates random numbers
	virtual float	RandomFloat( float flMinVal = 0.0f, float flMaxVal = 1.0f ) = 0;
	virtual int		RandomInt( int iMinVal, int iMaxVal ) = 0;
};


//-----------------------------------------------------------------------------
// The standard generator of uniformly distributed random numbers
//-----------------------------------------------------------------------------
class VSTDLIB_CLASS CUniformRandomStream : public IUniformRandomStream
{
public:
	CUniformRandomStream();

	// Sets the seed of the random number generator
	virtual void	SetSeed( int iSeed );

	// Generates random numbers
	virtual float	RandomFloat( float flMinVal = 0.0f, float flMaxVal = 1.0f );
	virtual int		RandomInt( int iMinVal, int iMaxVal );

private:
	int		GenerateRandomNumber();

	int m_idum;
	int m_iy;
	int m_iv[NTAB];
};


//-----------------------------------------------------------------------------
// A generator of gaussian distributed random numbers
//-----------------------------------------------------------------------------
class VSTDLIB_CLASS CGaussianRandomStream
{
public:
	// Passing in NULL will cause the gaussian stream to use the
	// installed global random number generator
	CGaussianRandomStream( IUniformRandomStream *pUniformStream = NULL );

	// Attaches to a random uniform stream
	void	AttachToStream( IUniformRandomStream *pUniformStream = NULL );

	// Generates random numbers
	float	RandomFloat( float flMean = 0.0f, float flStdDev = 1.0f );

private:
	IUniformRandomStream *m_pUniformStream;
	bool	m_bHaveValue;
	float	m_flRandomValue;
};


//-----------------------------------------------------------------------------
// A couple of convenience functions to access the library's global uniform stream
//-----------------------------------------------------------------------------
VSTDLIB_INTERFACE void	RandomSeed( int iSeed );
VSTDLIB_INTERFACE float	RandomFloat( float flMinVal = 0.0f, float flMaxVal = 1.0f );
VSTDLIB_INTERFACE int	RandomInt( int iMinVal, int iMaxVal );
VSTDLIB_INTERFACE float	RandomGaussianFloat( float flMean = 0.0f, float flStdDev = 1.0f );


//-----------------------------------------------------------------------------
// Installs a global random number generator, which will affect the Random functions above
//-----------------------------------------------------------------------------
VSTDLIB_INTERFACE void	InstallUniformRandomStream( IUniformRandomStream *pStream );


#endif // VSTDLIB_RANDOM_H



