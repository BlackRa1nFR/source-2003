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

#if !defined( FXDISCREETLINE_H )
#define FXDISCREETLINE_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"
#include "clientsideeffects.h"

class IMaterial;

class CFXDiscreetLine : public CClientSideEffect
{
public:

	CFXDiscreetLine ( const char *name, const Vector& start, const Vector& direction, float velocity, 
		float length, float clipLength, float scale, float life, const char *shader );
	~CFXDiscreetLine ( void );

	virtual void	Draw( double frametime );
	virtual bool	IsActive( void );
	virtual void	Destroy( void );
	virtual	void	Update( double frametime );

protected:

	IMaterial		*m_pMaterial;
	float			m_fLife;
	Vector			m_vecOrigin, m_vecDirection;
	float			m_fVelocity;
	float			m_fStartTime;
	float			m_fClipLength;
	float			m_fScale;
	float			m_fLength;
};

#endif	//FXDISCREETLINE_H