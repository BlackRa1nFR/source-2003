//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements visual effects entities: sprites, beams, bubbles, etc.
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "Sprite.h"
#include "model_types.h"
#include "engine/ivmodelinfo.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const float MAX_SPRITE_SCALE = 8.0f;

LINK_ENTITY_TO_CLASS( env_sprite, CSprite );
#if !defined( CLIENT_DLL )
LINK_ENTITY_TO_CLASS( env_glow, CSprite ); // For backwards compatibility, remove when no longer needed.
#endif

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CSprite )

	DEFINE_FIELD( CSprite, m_flLastTime, FIELD_TIME ),
	DEFINE_FIELD( CSprite, m_flMaxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CSprite, m_hAttachedToEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( CSprite, m_nAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( CSprite, m_flDieTime, FIELD_TIME ),

	DEFINE_FIELD( CSprite, m_nBrightness,		FIELD_INTEGER ),
	DEFINE_FIELD( CSprite, m_flBrightnessTime,	FIELD_FLOAT ),

	DEFINE_KEYFIELD( CSprite, m_flSpriteScale, FIELD_FLOAT, "scale" ),
	DEFINE_KEYFIELD( CSprite, m_flSpriteFramerate, FIELD_FLOAT, "framerate" ),
	DEFINE_KEYFIELD( CSprite, m_flFrame, FIELD_FLOAT, "frame" ),

	DEFINE_FIELD( CSprite, m_flScaleTime,		FIELD_FLOAT ),
	DEFINE_FIELD( CSprite, m_flStartScale,		FIELD_FLOAT ),
	DEFINE_FIELD( CSprite, m_flDestScale,		FIELD_FLOAT ),
	DEFINE_FIELD( CSprite, m_flScaleTimeStart,	FIELD_TIME ),
	DEFINE_FIELD( CSprite, m_nStartBrightness,	FIELD_INTEGER ),
	DEFINE_FIELD( CSprite, m_nDestBrightness,	FIELD_INTEGER ),
	DEFINE_FIELD( CSprite, m_flBrightnessTimeStart, FIELD_TIME ),

	// Function Pointers
	DEFINE_FUNCTION( CSprite, AnimateThink ),
	DEFINE_FUNCTION( CSprite, ExpandThink ),
	DEFINE_FUNCTION( CSprite, AnimateUntilDead ),

	// Inputs
	DEFINE_INPUT( CSprite, m_flSpriteScale, FIELD_FLOAT, "SetScale" ),
	DEFINE_INPUTFUNC( CSprite, FIELD_VOID, "HideSprite", InputHideSprite ),
	DEFINE_INPUTFUNC( CSprite, FIELD_VOID, "ShowSprite", InputShowSprite ),
	DEFINE_INPUTFUNC( CSprite, FIELD_VOID, "ToggleSprite", InputToggleSprite ),

END_DATADESC()
#endif

BEGIN_PREDICTION_DATA( CSprite )

	// Networked
	DEFINE_PRED_FIELD( CSprite, m_hAttachedToEntity, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CSprite, m_nAttachment, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CSprite, m_flScaleTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CSprite, m_flSpriteScale, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CSprite, m_flSpriteFramerate, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CSprite, m_flFrame, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CSprite, m_flBrightnessTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CSprite, m_nBrightness, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD( CSprite, m_flLastTime, FIELD_FLOAT ),
	DEFINE_FIELD( CSprite, m_flMaxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CSprite, m_flDieTime, FIELD_FLOAT ),

//	DEFINE_FIELD( CSprite, m_flStartScale, FIELD_FLOAT ),			//Starting scale
//	DEFINE_FIELD( CSprite, m_flDestScale, FIELD_FLOAT ),			//Destination scale
//	DEFINE_FIELD( CSprite, m_flScaleTimeStart, FIELD_FLOAT ),		//Real time for start of scale
//	DEFINE_FIELD( CSprite, m_nStartBrightness, FIELD_INTEGER ),		//Starting brightness
//	DEFINE_FIELD( CSprite, m_nDestBrightness, FIELD_INTEGER ),		//Destination brightness
//	DEFINE_FIELD( CSprite, m_flBrightnessTimeStart, FIELD_FLOAT ),	//Real time for brightness

END_PREDICTION_DATA()

IMPLEMENT_NETWORKCLASS_ALIASED( Sprite, DT_Sprite );

BEGIN_NETWORK_TABLE( CSprite, DT_Sprite )
#if !defined( CLIENT_DLL )
	SendPropEHandle( SENDINFO(m_hAttachedToEntity )),
	SendPropInt( SENDINFO(m_nAttachment ), 8 ),
	SendPropFloat( SENDINFO(m_flScaleTime ), 0,	SPROP_NOSCALE ),
	SendPropFloat( SENDINFO(m_flSpriteScale ), 8,	SPROP_ROUNDUP,	0.0f,	MAX_SPRITE_SCALE),
	SendPropFloat( SENDINFO(m_flSpriteFramerate ), 8,	SPROP_ROUNDUP,	0,	60.0f),
	SendPropFloat( SENDINFO(m_flFrame),		20, SPROP_ROUNDDOWN,	0.0f,   256.0f),
	SendPropFloat( SENDINFO(m_flBrightnessTime ), 0,	SPROP_NOSCALE ),
	SendPropInt( SENDINFO(m_nBrightness), 8, SPROP_UNSIGNED ),
#else
	RecvPropEHandle(RECVINFO(m_hAttachedToEntity)),
	RecvPropInt(RECVINFO(m_nAttachment)),
	RecvPropFloat(RECVINFO(m_flScaleTime)),
	RecvPropFloat(RECVINFO(m_flSpriteScale)),
	RecvPropFloat(RECVINFO(m_flSpriteFramerate)),
	RecvPropFloat(RECVINFO(m_flFrame)),
	RecvPropFloat(RECVINFO(m_flBrightnessTime)),
	RecvPropInt(RECVINFO(m_nBrightness)),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	m_flFrame = 0;

	Precache();
	SetModel( STRING( GetModelName() ) );

	m_flMaxFrame = (float)modelinfo->GetModelFrameCount( GetModel() ) - 1;

#if defined( CLIENT_DLL )
	SetNextClientThink( CLIENT_THINK_ALWAYS );
#endif

#if !defined( CLIENT_DLL )
	if ( GetEntityName() != NULL_STRING && !(m_spawnflags & SF_SPRITE_STARTON) )
	{
		TurnOff();
	}
	else
#endif
	{
		TurnOn();
	}
	
	// Worldcraft only sets y rotation, copy to Z
	if ( GetLocalAngles().y != 0 && GetLocalAngles().z == 0 )
	{
		QAngle angles = GetLocalAngles();

		angles.z = angles.y;
		angles.y = 0;

		SetLocalAngles( angles );
	}

	// Clamp our scale if necessary
	float scale = m_flSpriteScale;
	
	if ( scale < 0 || scale > MAX_SPRITE_SCALE )
	{
#if !defined( CLIENT_DLL )
		DevMsg( 1, "LEVEL DESIGN ERROR: Sprite %s with bad scale %.f [0..%.f]\n", GetDebugName(), m_flSpriteScale, MAX_SPRITE_SCALE );
#endif
		scale = clamp( m_flSpriteScale, 0, MAX_SPRITE_SCALE );
	}

	//Set our state
	SetBrightness( m_clrRender->a );
	SetScale( scale );

#if defined( CLIENT_DLL )
	m_flStartScale = m_flDestScale = m_flSpriteScale;
	m_nStartBrightness = m_nDestBrightness = m_nBrightness;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szModelName - 
//-----------------------------------------------------------------------------
void CSprite::SetModel( const char *szModelName )
{
	int index = modelinfo->GetModelIndex( szModelName );
	const model_t *model = modelinfo->GetModel( index );
	if ( model && modelinfo->GetModelType( model ) != mod_sprite )
	{
		Msg( "Setting CSprite to non-sprite model %s\n", szModelName );
	}

#if !defined( CLIENT_DLL )
	UTIL_SetModel( this, szModelName );
#else
	BaseClass::SetModel( szModelName );
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::Precache( void )
{
	if ( GetModelName() != NULL_STRING )
	{
		PrecacheModel( STRING( GetModelName() ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSpriteName - 
//			&origin - 
//-----------------------------------------------------------------------------
void CSprite::SpriteInit( const char *pSpriteName, const Vector &origin )
{
	SetModelName( MAKE_STRING(pSpriteName) );
	SetLocalOrigin( origin );
	Spawn();
}

#if !defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: Fixup parent after restore
//-----------------------------------------------------------------------------
void CSprite::OnRestore()
{
	// Reset attachment after save/restore
	if ( GetFollowedEntity() )
	{
		SetAttachment( GetFollowedEntity(), m_nAttachment );
	}
	else
	{
		// Clear attachment
		m_hAttachedToEntity = NULL;
		m_nAttachment = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSpriteName - 
//			&origin - 
//			animate - 
// Output : CSprite
//-----------------------------------------------------------------------------
CSprite *CSprite::SpriteCreate( const char *pSpriteName, const Vector &origin, bool animate )
{
	CSprite *pSprite = CREATE_ENTITY( CSprite, "env_sprite" );
	pSprite->SpriteInit( pSpriteName, origin );
	pSprite->SetSolid( SOLID_NONE );
	UTIL_SetSize( pSprite, vec3_origin, vec3_origin );
	pSprite->SetMoveType( MOVETYPE_NOCLIP );
	if ( animate )
		pSprite->TurnOn();

	return pSprite;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSpriteName - 
//			&origin - 
//			animate - 
// Output : CSprite
//-----------------------------------------------------------------------------
CSprite *CSprite::SpriteCreatePredictable( const char *module, int line, const char *pSpriteName, const Vector &origin, bool animate )
{
	CSprite *pSprite = ( CSprite * )CBaseEntity::CreatePredictedEntityByName( "env_sprite", module, line );
	if ( pSprite )
	{
		pSprite->SpriteInit( pSpriteName, origin );
		pSprite->SetSolid( SOLID_NONE );
		pSprite->SetSize( vec3_origin, vec3_origin );
		pSprite->SetMoveType( MOVETYPE_NOCLIP );
		if ( animate )
			pSprite->TurnOn();
	}

	return pSprite;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::AnimateThink( void )
{
	Animate( m_flSpriteFramerate * (gpGlobals->curtime - m_flLastTime) );

	SetNextThink( gpGlobals->curtime );
	m_flLastTime			= gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::AnimateUntilDead( void )
{
	if ( gpGlobals->curtime > m_flDieTime )
	{
		Remove( );
	}
	else
	{
		AnimateThink();
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scaleSpeed - 
//			fadeSpeed - 
//-----------------------------------------------------------------------------
void CSprite::Expand( float scaleSpeed, float fadeSpeed )
{
	m_flSpeed = scaleSpeed;
	m_iHealth = fadeSpeed;
	SetThink( &CSprite::ExpandThink );

	SetNextThink( gpGlobals->curtime );
	m_flLastTime	= gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::ExpandThink( void )
{
	float frametime = gpGlobals->curtime - m_flLastTime;
	m_flSpriteScale += m_flSpeed * frametime;

	int sub = (int)(m_iHealth * frametime);
	if ( sub > m_clrRender->a )
	{
		SetRenderColorA( 0 );
		Remove( );
	}
	else
	{
		SetRenderColorA( m_clrRender->a - sub );
		SetNextThink( gpGlobals->curtime );
		m_flLastTime		= gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frames - 
//-----------------------------------------------------------------------------
void CSprite::Animate( float frames )
{ 
	m_flFrame += frames;
	if ( m_flFrame > m_flMaxFrame )
	{
#if !defined( CLIENT_DLL )
		if ( m_spawnflags & SF_SPRITE_ONCE )
		{
			TurnOff();
		}
		else
#endif
		{
			if ( m_flMaxFrame > 0 )
				m_flFrame = fmod( m_flFrame, m_flMaxFrame );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSprite::SetBrightness( int brightness, float time )
{
	m_nBrightness			= brightness;	//Take our current position as our starting position
	m_flBrightnessTime		= time;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::SetScale( float scale, float time )
{
	m_flSpriteScale		= scale;	//Take our current position as our new starting position
	m_flScaleTime		= time;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::TurnOff( void )
{
	m_fEffects |= EF_NODRAW;
	SetNextThink( TICK_NEVER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::TurnOn( void )
{
	m_fEffects &= ~EF_NODRAW;
	if ( (m_flSpriteFramerate && m_flMaxFrame > 1.0)
#if !defined( CLIENT_DLL )
		|| (m_spawnflags & SF_SPRITE_ONCE) 
#endif
		)
	{
		SetThink( &CSprite::AnimateThink );
		SetNextThink( gpGlobals->curtime );
		m_flLastTime = gpGlobals->curtime;
	}
	m_flFrame = 0;
}

#if !defined( CLIENT_DLL )
// DVS TODO: Obsolete Use handler
void CSprite::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int on = m_fEffects != EF_NODRAW;
	if ( ShouldToggle( useType, on ) )
	{
		if ( on )
		{
			TurnOff();
		}
		else
		{
			TurnOn();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that hides the sprite.
//-----------------------------------------------------------------------------
void CSprite::InputHideSprite( inputdata_t &inputdata )
{
	TurnOff();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that hides the sprite.
//-----------------------------------------------------------------------------
void CSprite::InputShowSprite( inputdata_t &inputdata )
{
	TurnOn();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that toggles the sprite between hidden and shown.
//-----------------------------------------------------------------------------
void CSprite::InputToggleSprite( inputdata_t &inputdata )
{
	if ( m_fEffects != EF_NODRAW )
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}
#endif

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CSprite::GetRenderScale( void )
{
	//See if we're done scaling
	if ( ( m_flScaleTime == 0 ) || ( (m_flScaleTimeStart+m_flScaleTime) < gpGlobals->curtime ) )
		return m_flSpriteScale;

	//Get our percentage
	float timeDelta = ( gpGlobals->curtime - m_flScaleTimeStart ) / m_flScaleTime;

	//Return the result
	return ( m_flStartScale + ( ( m_flDestScale - m_flStartScale  ) * timeDelta ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CSprite::GetRenderBrightness( void )
{
	//See if we're done scaling
	if ( ( m_flBrightnessTime == 0 ) || ( (m_flBrightnessTimeStart+m_flBrightnessTime) < gpGlobals->curtime ) )
	{
		return m_nBrightness;
	}

	//Get our percentage
	float timeDelta = ( gpGlobals->curtime - m_flBrightnessTimeStart ) / m_flBrightnessTime;

	float brightness = ( (float) m_nStartBrightness + ( (float) ( m_nDestBrightness - m_nStartBrightness  ) * timeDelta ) );

	//Return the result
	return (int) brightness;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// Only think when sapping
	SetNextClientThink( CLIENT_THINK_ALWAYS );


	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_flStartScale = m_flDestScale = m_flSpriteScale;
		m_nStartBrightness = m_nDestBrightness = m_nBrightness;
	}
}

void CSprite::ClientThink( void )
{
	BaseClass::ClientThink();

	// Module render colors over time
	if ( m_flSpriteScale != m_flDestScale )
	{
		m_flStartScale		= m_flDestScale;
		m_flDestScale		= m_flSpriteScale;
		m_flScaleTimeStart	= gpGlobals->curtime;
	}

	if ( m_nBrightness != m_nDestBrightness )
	{
		m_nStartBrightness		= m_nDestBrightness;
		m_nDestBrightness		= m_nBrightness;
		m_flBrightnessTimeStart = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int CSprite::DrawModel( int flags )
{
	VPROF_BUDGET( "CSprite::DrawModel", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	//See if we should draw
	if ( ( ShouldDraw() == false ) || ( m_bReadyToDraw == false ) )
		return 0;

	//Must be a sprite
	if ( modelinfo->GetModelType( GetModel() ) != mod_sprite )
	{
		assert( 0 );
		return 0;
	}

	float renderscale = GetRenderScale();

	//Draw it
	int drawn = DrawSprite( 
		this,
		GetModel(), 
		GetAbsOrigin(), 
		GetAbsAngles(), 
		m_flFrame,				// sprite frame to render
		m_hAttachedToEntity,	// attach to
		m_nAttachment,			// attachment point
		m_nRenderMode,			// rendermode
		m_nRenderFX,
		GetRenderBrightness(),		// alpha
		m_clrRender->r,
		m_clrRender->g,
		m_clrRender->b,
		renderscale );			// sprite scale

	return drawn;
}


const Vector& CSprite::GetRenderOrigin()
{
	static Vector vOrigin;
	vOrigin = GetAbsOrigin();

	if ( m_hAttachedToEntity )
	{
		C_BaseEntity *ent = m_hAttachedToEntity->GetBaseEntity();
		if ( ent )
		{
			QAngle dummyAngles;
			ent->GetAttachment( m_nAttachment, vOrigin, dummyAngles );
		}
	}

	return vOrigin;
}


#endif
