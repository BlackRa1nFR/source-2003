//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( IVIEWEFFECTS_H )
#define IVIEWEFFECTS_H
#ifdef _WIN32
#pragma once
#endif

class Vector;
class QAngle;

//-----------------------------------------------------------------------------
// Purpose: Apply effects to view origin/angles, etc.  Screen fade and shake
//-----------------------------------------------------------------------------
class IViewEffects
{
public:
	// Initialize subsystem
	virtual void	Init( void ) = 0;
	// Initialize after each level change
	virtual void	LevelInit( void ) = 0;
	// Called each frame to determine the current view fade parameters ( color and alpha )
	virtual void	GetFadeParams( int context, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a, bool *blend ) = 0;
	// Apply directscreen shake
	virtual int		Shake( const char *pszName, int iSize, void *pbuf ) = 0;
	// Apply direct screen fade
	virtual int		Fade( const char *pszName, int iSize, void *pbuf ) = 0;
	// Clear all permanent fades in our fade list
	virtual void	ClearPermanentFades( void ) = 0;
	// Clear all fades in our fade list
	virtual void	ClearAllFades( void ) = 0;
	// Compute screen shake values for this frame
	virtual void	CalcShake( void ) = 0;
	// Apply those values to the passed in vector(s).
	virtual void	ApplyShake( Vector& origin, QAngle& angles, float factor ) = 0;
};

extern IViewEffects *vieweffects;

#endif // IVIEWEFFECTS_H