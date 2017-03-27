//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef SPRITETRAIL_H
#define SPRITETRAIL_H
#ifdef _WIN32
#pragma once
#endif

#include "Sprite.h"

#if defined( CLIENT_DLL )
#define CSpriteTrail C_SpriteTrail

typedef	struct
{
	Vector	screenPos;
	float	dieTime;
} trailPoint_t;

#endif

class CSpriteTrail : public CSprite
{
	DECLARE_CLASS( CSpriteTrail, CSprite );

public:

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

// Client only code
#if defined( CLIENT_DLL )

	void	UpdateTrail( void );
	int		DrawModel( int flags );
	
	Vector const &GetRenderOrigin( void );
	bool		ShouldDraw( void );

	CUtlVector< trailPoint_t >	m_vecSteps;
	float						m_flUpdateTime;

#endif	//CLIENT_DLL

// Server only code
#if !defined( CLIENT_DLL )

	static CSpriteTrail *SpriteTrailCreate( const char *pSpriteName, const Vector &origin, bool animate );

#endif	//CLIENT_DLL == false

	CSpriteTrail( void );

	void SetLifeTime( float time ) { m_flLifeTime = time; }

protected:

	CNetworkVar( float, m_flLifeTime );	// Amount of time before a new trail segment fades away
};

#endif // SPRITETRAIL_H
