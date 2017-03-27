//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef C_BREAKABLEPROP_H
#define C_BREAKABLEPROP_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_BreakableProp : public C_BaseAnimating
{
	typedef C_BaseAnimating BaseClass;
public:
	DECLARE_CLIENTCLASS();

	C_BreakableProp();
	~C_BreakableProp();

	bool		 CreateClientsideProp( const char *pszModelName, Vector vecOrigin, Vector vecForceDir, AngularImpulse vecAngularImp );
	virtual void ClientThink( void );

	virtual CollideType_t	ShouldCollide( void );

protected:
	bool			m_bClientsideOnly;
	double			m_fDeathTime;		// Point at which this object self destructs.  
										// The default of -1 indicates the object shouldn't destruct.
};

#endif // C_BREAKABLEPROP_H
