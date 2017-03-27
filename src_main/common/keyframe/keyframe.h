//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Keyframed animation header
//			shared between game and tools
//=============================================================================

#ifndef KEYFRAME_H
#define KEYFRAME_H
#pragma once


class IPositionInterpolator
{
public:
	virtual void		Release() = 0;

	virtual void		GetDetails( char **outName, int *outMinKeyReq, int *outMaxKeyReq ) = 0;
	virtual void		SetKeyPosition( int keyNum, Vector const &vPos ) = 0;
	virtual void		InterpolatePosition( float time, Vector &vOut ) = 0;
	
	// Returns true if the key causes a change that changes the interpolated positions.
	virtual bool		ProcessKey( char const *pName, char const *pValue ) = 0;
};


// Time modifiers.
int Motion_GetNumberOfTimeModifiers( void );
bool Motion_GetTimeModifierDetails( int timeInterpNum, char **outName );
bool Motion_CalculateModifiedTime( float time, int timeModifierFuncNum, float *outNewTime );

// Position interpolators.
int Motion_GetNumberOfPositionInterpolators( void );
IPositionInterpolator* Motion_GetPositionInterpolator( int interpNum );

// Rotation interpolators.
int Motion_GetNumberOfRotationInterpolators( void );
bool Motion_GetRotationInterpolatorDetails( int rotInterpNum, char **outName, int *outMinKeyReq, int *outMaxKeyReq );
bool Motion_InterpolateRotation( float time, int interpFuncNum, Quaternion &outQuatRotation );
bool Motion_SetKeyAngles( int keyNum, Quaternion &quatAngles );


#endif // KEYFRAME_H
