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

#if !defined( FXLINE_H )
#define FXLINE_H
#ifdef _WIN32
#pragma once
#endif

#include "FX_StaticLine.h"

class CFXLine : public CFXStaticLine
{
public:

	CFXLine( const char *name, Vector& start, Vector& end, Vector& start_vel, 
		Vector& end_vel, float scale, float life, const char *shader, unsigned int flags );
	~CFXLine( void );

	virtual void	Draw( double frametime );
	virtual bool	IsActive( void );
	virtual void	Destroy( void );
	virtual	void	Update( double frametime );

protected:

	Vector	m_vecStartVelocity;
	Vector	m_vecEndVelocity;
};

void FX_DrawLine( const Vector &start, const Vector &end, float scale, IMaterial *pMaterial, const color32 &color );

#endif	//FXLINE_H