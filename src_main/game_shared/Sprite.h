//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SPRITE_H
#define SPRITE_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "baseentity_shared.h"

#define SF_SPRITE_STARTON		0x0001
#define SF_SPRITE_ONCE			0x0002
#define SF_SPRITE_TEMPORARY		0x8000

class CBasePlayer;

#if defined( CLIENT_DLL )
#define CSprite C_Sprite

class C_SpriteRenderer
{
public:
	//-----------------------------------------------------------------------------
	// Purpose: Sprite orientations
	// WARNING!  Change these in common/MaterialSystem/Sprite.cpp if you change them here!
	//-----------------------------------------------------------------------------
	typedef enum
	{
		SPR_VP_PARALLEL_UPRIGHT		= 0,
		SPR_FACING_UPRIGHT			= 1,
		SPR_VP_PARALLEL				= 2,
		SPR_ORIENTED				= 3,
		SPR_VP_PARALLEL_ORIENTED	= 4
	} SPRITETYPE;
	
	// Determine sprite orientation
	void							GetSpriteAxes( SPRITETYPE type, 
										const Vector& origin,
										const QAngle& angles,
										Vector& forward, 
										Vector& right, 
										Vector& up );

	// Sprites can alter blending amount
	float							GlowBlend( Vector& entorigin, int rendermode, int renderfx, int alpha, float *scale );

	// Draws tempent as a sprite
	int								DrawSprite( 
										IClientEntity *entity,
										const model_t *model, 
										const Vector& origin, 
										const QAngle& angles,
										float frame,
										IClientEntity *attachedto,
										int attachmentindex,
										int rendermode,
										int renderfx,
										int alpha,
										int r, 
										int g, 
										int b,
										float scale
										);
};

#endif

class CSprite : public CBaseEntity
#if defined( CLIENT_DLL )
	, public C_SpriteRenderer
#endif
{
	DECLARE_CLASS( CSprite, CBaseEntity );
public:
	DECLARE_PREDICTABLE();
	DECLARE_NETWORKCLASS();

	virtual void SetModel( const char *szModelName );

	void Spawn( void );
	void Precache( void );

#if !defined( CLIENT_DLL )
	int	ObjectCaps( void )
	{ 
		int flags = 0;
		if ( HasSpawnFlags( SF_SPRITE_TEMPORARY ) )
			flags = FCAP_DONT_SAVE;
		return (BaseClass :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | flags; 
	}
	void OnRestore();
#endif

	void AnimateThink( void );
	void ExpandThink( void );
	void Animate( float frames );
	void Expand( float scaleSpeed, float fadeSpeed );
	void SpriteInit( const char *pSpriteName, const Vector &origin );

#if !defined( CLIENT_DLL )
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	// Input handlers
	void InputHideSprite( inputdata_t &inputdata );
	void InputShowSprite( inputdata_t &inputdata );
	void InputToggleSprite( inputdata_t &inputdata );
#endif

	inline void SetAttachment( CBaseEntity *pEntity, int attachment )
	{
		if ( pEntity )
		{
			m_hAttachedToEntity = pEntity;
			m_nAttachment = attachment;
			FollowEntity( pEntity );
		}
	}

	void TurnOff( void );
	void TurnOn( void );
	inline float Frames( void ) { return m_flMaxFrame; }
	inline void SetTransparency( int rendermode, int r, int g, int b, int a, int fx )
	{
		m_nRenderMode = rendermode;
		SetColor( r, g, b );
		SetBrightness( a );
		m_nRenderFX = fx;
	}
	inline void SetTexture( int spriteIndex ) { SetModelIndex( spriteIndex ); }
	inline void SetColor( int r, int g, int b ) { SetRenderColor( r, g, b, GetRenderColor().a ); }
	
	void SetBrightness( int brightness, float duration = 0.0f );
	void SetScale( float scale, float duration = 0.0f );

	float GetScale( void ) { return m_flSpriteScale; }
	int	GetBrightness( void ) { return m_nBrightness; }

	inline void FadeAndDie( float duration ) 
	{ 
		SetBrightness( 0, duration );
		SetThink(&CSprite::AnimateUntilDead); 
		m_flDieTime = gpGlobals->curtime + duration; 
		SetNextThink( gpGlobals->curtime );  
	}

	inline void AnimateAndDie( float framerate ) 
	{ 
		SetThink(&CSprite::AnimateUntilDead); 
		m_flSpriteFramerate = framerate;
		m_flDieTime = gpGlobals->curtime + (m_flMaxFrame / m_flSpriteFramerate); 
		SetNextThink( gpGlobals->curtime ); 
	}

	inline void AnimateForTime( float framerate, float time ) 
	{ 
		SetThink(&CSprite::AnimateUntilDead); 
		m_flSpriteFramerate = framerate;
		m_flDieTime = gpGlobals->curtime + time;
		SetNextThink( gpGlobals->curtime ); 
	}
	void AnimateUntilDead( void );
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();

	static CSprite *SpriteCreate( const char *pSpriteName, const Vector &origin, bool animate );
#endif
	static CSprite *SpriteCreatePredictable( const char *module, int line, const char *pSpriteName, const Vector &origin, bool animate );

#if defined( CLIENT_DLL )
	virtual float	GetRenderScale( void );
	virtual int		GetRenderBrightness( void );

	virtual int		DrawModel( int flags );
	virtual const Vector& GetRenderOrigin();


// Only supported in TF2 right now
#if defined( TF2_CLIENT_DLL )
	virtual bool	ShouldPredict( void )
	{
		return true;
	}
#endif

	virtual void	ClientThink( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

#endif
public:
	CNetworkHandle( CBaseEntity, m_hAttachedToEntity );
	CNetworkVar( int, m_nAttachment );
	CNetworkVar( float, m_flSpriteFramerate );
	CNetworkVar( float, m_flFrame );

	float		m_flDieTime;

private:

	CNetworkVar( int, m_nBrightness );
	CNetworkVar( float, m_flBrightnessTime );
	
	CNetworkVar( float, m_flSpriteScale );
	CNetworkVar( float, m_flScaleTime );

	float		m_flLastTime;
	float		m_flMaxFrame;


	float		m_flStartScale;
	float		m_flDestScale;		//Destination scale
	float		m_flScaleTimeStart;	//Real time for start of scale
	int			m_nStartBrightness;
	int			m_nDestBrightness;		//Destination brightness
	float		m_flBrightnessTimeStart;//Real time for brightness
};

// Macro to wrap creation
#define SPRITE_CREATE_PREDICTABLE( name, origin, animate ) \
	CSprite::SpriteCreatePredictable( __FILE__, __LINE__, name, origin, animate )

#endif // SPRITE_H
