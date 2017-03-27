//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( C_SPRITE_H )
#define C_SPRITE_H
#ifdef _WIN32
#pragma once
#endif

#include "Sprite.h"

float GlowSightDistance( const Vector &glowOrigin );

#if 0 
#include "c_baseentity.h"
class C_Sprite : public C_BaseEntity, public C_SpriteRenderer
{
public:
	DECLARE_CLASS( C_Sprite, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_Sprite( void );
	~C_Sprite( void );

	virtual float	GetScale( void );
	virtual int		GetBrightness( void );

	virtual	float	GetScaleTime( void )		{ return m_flScaleTime;			}
	virtual float	GetBrightnessTime( void )	{ return m_flBrightnessTime;	}
	
	virtual	void	SetBrightness( int brightness, float time = 0.0f );	//NOTENOTE: Use these calls ONLY for setting sprite values!
	virtual void	SetScale( float scale, float time = 0.0f );
	
	virtual int		DrawModel( int flags );

public:

	int			m_nAttachedToEntity;
	int			m_nAttachment;
	float		m_flSpriteFramerate;
	float		m_flFrame;

protected:

	float		m_flSpriteScale;	//Starting scale
	float		m_flDestScale;		//Destination scale
	float		m_flScaleTime;		//Scale time
	float		m_flScaleTimeStart;	//Real time for start of scale

	int			m_nBrightness;			//Starting brightness
	int			m_nDestBrightness;		//Destination brightness
	float		m_flBrightnessTime;		//Brightness time
	float		m_flBrightnessTimeStart;//Real time for brightness
};
#endif

#endif // C_SPRITE_H
