//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef COLORSPACE_H
#define COLORSPACE_H

#ifdef _WIN32
#pragma once
#endif

#include <mathlib.h>
#include "bumpvects.h"
#include "tier0/dbg.h"

extern float g_LinearToVertex[4096];	// linear (0..4) to screen corrected vertex space (0..1?)

namespace ColorSpace
{
	void SetGamma( float screenGamma, float texGamma, 
		           float overbright, bool allowCheats, bool linearFrameBuffer );

	// convert texture to linear 0..1 value
	float TextureToLinear( int c );

	// convert texture to linear 0..1 value
	int LinearToTexture( float f );

	float TexLightToLinear( int c, int exponent );

	// assume 0..4 range
	void LinearToLightmap( unsigned char *pDstRGB, const float *pSrcRGB );
	
	// assume 0..4 range
	void LinearToBumpedLightmap( const float *linearColor, const float *linearBumpColor1,
		const float *linearBumpColor2, const float *linearBumpColor3,
		unsigned char *ret, unsigned char *retBump1,
		unsigned char *retBump2, unsigned char *retBump3 );
	
	// converts 0..1 linear value to screen gamma (0..255)
	int LinearToScreenGamma( float f );

	FORCEINLINE void LinearToLightmap( unsigned char *pDstRGB, const float *pSrcRGB )
	{

		Vector tmpVect;
#if 1
		int i, j;
		for( j = 0; j < 3; j++ )
		{
//			i = ( int )( pSrcRGB[j] * 1024 );	// assume 0..4 range
			i = RoundFloatToInt( pSrcRGB[j] * 1024 );	// assume 0..4 range
			if (i < 0)
			{
				i = 0;
			}
			if (i > 4091)
			{
				i = 4091;
			}
			tmpVect[j] = g_LinearToVertex[i];
		}
#else	
		
		tmpVect[0] = LinearToVertexLight( pSrcRGB[0] );
		tmpVect[1] = LinearToVertexLight( pSrcRGB[1] );
		tmpVect[2] = LinearToVertexLight( pSrcRGB[2] );
#endif
		ColorClamp( tmpVect );
		
		pDstRGB[0] = RoundFloatToByte( tmpVect[0] * 255.0f );
		pDstRGB[1] = RoundFloatToByte( tmpVect[1] * 255.0f );
		pDstRGB[2] = RoundFloatToByte( tmpVect[2] * 255.0f );
	}

	FORCEINLINE void 
		LinearToBumpedLightmap( const float *linearColor, const float *linearBumpColor1,
		const float *linearBumpColor2, const float *linearBumpColor3,
		unsigned char *ret, unsigned char *retBump1,
		unsigned char *retBump2, unsigned char *retBump3 )
	{
#if 0
		LinearToLightmap( ret, linearColor );
		LinearToLightmap( retBump1, linearBumpColor1 );
		LinearToLightmap( retBump2, linearBumpColor2 );
		LinearToLightmap( retBump3, linearBumpColor3 );
#else	

		const Vector &linearBump1 = *( ( const Vector * )linearBumpColor1 );
		const Vector &linearBump2 = *( ( const Vector * )linearBumpColor2 );
		const Vector &linearBump3 = *( ( const Vector * )linearBumpColor3 );

		Vector gammaGoal;
		// gammaGoal is premultiplied by 1/overbright, which we want
		gammaGoal[0] = LinearToVertexLight( linearColor[0] );
		gammaGoal[1] = LinearToVertexLight( linearColor[1] );
		gammaGoal[2] = LinearToVertexLight( linearColor[2] );
		ColorClamp( gammaGoal );
		Vector bumpSum = linearBump1;
		bumpSum += linearBump2;
		bumpSum += linearBump3;
		bumpSum *= OO_SQRT_3;
		
		Vector correctionScale;
		if( *( int * )&bumpSum[0] != 0 && *( int * )&bumpSum[1] != 0 && *( int * )&bumpSum[2] != 0 )
		{
			// fast path when we know that we don't have to worry about divide by zero.
			VectorDivide( gammaGoal, bumpSum, correctionScale );
//			correctionScale = gammaGoal / bumpSum;
		}
		else
		{
			correctionScale.Init( 0.0f, 0.0f, 0.0f );
			if( bumpSum[0] != 0.0f )
			{
				correctionScale[0] = gammaGoal[0] / bumpSum[0];
			}
			if( bumpSum[1] != 0.0f )
			{
				correctionScale[1] = gammaGoal[1] / bumpSum[1];
			}
			if( bumpSum[2] != 0.0f )
			{
				correctionScale[2] = gammaGoal[2] / bumpSum[2];
			}
		}
		Vector correctedBumpColor1;
		Vector correctedBumpColor2;
		Vector correctedBumpColor3;
		VectorMultiply( linearBump1, correctionScale, correctedBumpColor1 );
		VectorMultiply( linearBump2, correctionScale, correctedBumpColor2 );
		VectorMultiply( linearBump3, correctionScale, correctedBumpColor3 );

		// FIXME: do I really need to color clamp here, above, or both?
		ColorClamp( correctedBumpColor1 );
		ColorClamp( correctedBumpColor2 );
		ColorClamp( correctedBumpColor3 );

		ret[0] = RoundFloatToByte( gammaGoal[0] * 255.0f );
		ret[1] = RoundFloatToByte( gammaGoal[1] * 255.0f );
		ret[2] = RoundFloatToByte( gammaGoal[2] * 255.0f );
		retBump1[0] = RoundFloatToByte( correctedBumpColor1[0] * 255.0f );
		retBump1[1] = RoundFloatToByte( correctedBumpColor1[1] * 255.0f );
		retBump1[2] = RoundFloatToByte( correctedBumpColor1[2] * 255.0f );
		retBump2[0] = RoundFloatToByte( correctedBumpColor2[0] * 255.0f );
		retBump2[1] = RoundFloatToByte( correctedBumpColor2[1] * 255.0f );
		retBump2[2] = RoundFloatToByte( correctedBumpColor2[2] * 255.0f );
		retBump3[0] = RoundFloatToByte( correctedBumpColor3[0] * 255.0f );
		retBump3[1] = RoundFloatToByte( correctedBumpColor3[1] * 255.0f );
		retBump3[2] = RoundFloatToByte( correctedBumpColor3[2] * 255.0f );
#endif
	}
	

};

#endif // COLORSPACE_H
