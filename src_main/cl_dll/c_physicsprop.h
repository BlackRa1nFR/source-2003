//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef C_PHYSICSPROP_H
#define C_PHYSICSPROP_H
#ifdef _WIN32
#pragma once
#endif

#include "c_breakableprop.h"
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PhysicsProp : public C_BreakableProp
{
	typedef C_BreakableProp BaseClass;
public:
	DECLARE_CLIENTCLASS();

	C_PhysicsProp();
	~C_PhysicsProp();

	virtual void ComputeFxBlend( void );

	float m_fadeMinDist;
	float m_fadeMaxDist;

	// Computes the physics fade.  Returns true if the object is fadable, false otherwise.
	bool		    ComputeFade( void );
	bool			ShouldFade( void );

};

#endif // C_PHYSICSPROP_H 
