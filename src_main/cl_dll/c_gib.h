//========= Copyright © 1996-2001, Valve Corporation, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef C_GIB_H
#define C_GIB_H
#ifdef _WIN32
#pragma once
#endif

// Base client gibs

class C_Gib : public C_BaseAnimating
{
	typedef C_BaseAnimating BaseClass;
public:

	C_Gib::~C_Gib( void );

	static C_Gib	*CreateClientsideGib( const char *pszModelName, Vector vecOrigin, Vector vecForceDir, AngularImpulse vecAngularImp );
	
	bool	InitializeGib( const char *pszModelName, Vector vecOrigin, Vector vecForceDir, AngularImpulse vecAngularImp );
	void	ClientThink( void );
	void	StartTouch( C_BaseEntity *pOther );

	virtual	void HitSurface( C_BaseEntity *pOther );

protected:

	float	m_flTouchDelta;		// Amount of time that must pass before another touch function can be called
};

#endif // C_GIB_H
