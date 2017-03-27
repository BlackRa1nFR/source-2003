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
// $NoKeywords: $
//=============================================================================

#include "clientsideeffects.h"

#if !defined( FXSTATICLINE_H )
#define FXSTATICLINE_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"

class IMaterial;

#define FXSTATICLINE_FLIP_HORIZONTAL	0x00000001	//Swaps the TC's so the texture is flipped horizontally
#define FXSTATICLINE_FLIP_VERTICAL		0x00000002	//Swaps the TC's so the texture is flipped vertically

class CFXStaticLine : public CClientSideEffect
{
public:

	CFXStaticLine( const char *name, const Vector& start, const Vector& end, float scale, float life, const char *shader, unsigned int flags );
	~CFXStaticLine( void );

	virtual void	Draw( double frametime );
	virtual bool	IsActive( void );
	virtual void	Destroy( void );
	virtual	void	Update( double frametime );

protected:

	IMaterial		*m_pMaterial;
	Vector			m_vecStart, m_vecEnd;
	unsigned int	m_uiFlags;
	float			m_fLife;
	float			m_fScale;
};

#endif
