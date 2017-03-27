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
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "model_types.h"
#include "view_shared.h"
#include "iviewrender.h"
#include "tempentity.h"
#include "dlight.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "clientsideeffects.h"
#include "cl_animevent.h"
#include "iefx.h"
#include "engine/IEngineSound.h"
#include "env_wind_shared.h"
#include "ClientEffectPrecacheSystem.h"
#include "fx_sparks.h"
#include "fx.h"
#include "movevars_shared.h"
#include "c_clientstats.h"
#include "engine/ivmodelinfo.h"
#include "SoundEmitterSystemBase.h"
#include "view.h"
#include "tier0/vprof.h"

#define TENT_WIND_ACCEL 50

//Precahce the effects
CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectMuzzleFlash )
CLIENTEFFECT_MATERIAL( "sprites/ar2_muzzle1" )
CLIENTEFFECT_MATERIAL( "sprites/ar2_muzzle3" )
CLIENTEFFECT_MATERIAL( "sprites/ar2_muzzle4" )
CLIENTEFFECT_MATERIAL( "sprites/muzzleflash4" )
CLIENTEFFECT_MATERIAL( "particle/fire" )
CLIENTEFFECT_REGISTER_END()

//Whether or not to eject brass from weapons
ConVar cl_ejectbrass( "cl_ejectbrass", "0" );

//-----------------------------------------------------------------------------
// Purpose: Implements temp entity interface
//-----------------------------------------------------------------------------
class CTempEnts : public ITempEnts
{
// Construction
public:
							CTempEnts( void );
	virtual					~CTempEnts( void );
// Exposed interface
public:
	virtual void			Init( void );
	virtual void			Shutdown( void );

	virtual void			LevelShutdown();

	virtual void			Update( void );
	virtual void			Clear( void );

	// Legacy temp entities still supported
	virtual void			BloodSprite( const Vector &org, int r, int g, int b, int a, int modelIndex, int modelIndex2, float size );
	virtual void			RicochetSprite( const Vector &pos, model_t *pmodel, float duration, float scale );
	virtual void			MuzzleFlash( const Vector &pos1, const QAngle &angles, int type, int entityIndex, bool firstPerson = false );
	virtual void			BreakModel(const Vector &pos, const Vector &size, const Vector &dir, float random, float life, int count, int modelIndex, char flags);
	virtual void			Bubbles( const Vector &mins, const Vector &maxs, float height, int modelIndex, int count, float speed );
	virtual void			BubbleTrail( const Vector &start, const Vector &end, float height, int modelIndex, int count, float speed );
	virtual void			Sprite_Explode( C_LocalTempEntity *pTemp, float scale, int flags );
	virtual void			FizzEffect( C_BaseEntity *pent, int modelIndex, int density );
	virtual C_LocalTempEntity		*DefaultSprite( const Vector &pos, int spriteIndex, float framerate );
	virtual void			Sprite_Smoke( C_LocalTempEntity *pTemp, float scale );
	virtual C_LocalTempEntity		*TempSprite( const Vector &pos, const Vector &dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags, const Vector &normal = vec3_origin );
	virtual void			AttachTentToPlayer( int client, int modelIndex, float zoffset, float life );
	virtual void			KillAttachedTents( int client );
	virtual void			Sprite_Spray( const Vector &pos, const Vector &dir, int modelIndex, int count, int speed, int iRand );

	virtual void			PlaySound ( C_LocalTempEntity *pTemp, float damp );
	virtual void			EjectBrass( const Vector &pos1, const QAngle &angles, const QAngle &gunAngles, int type );
	virtual void			SpawnTempModel( model_t *pModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, float flLifeTime, int iFlags );
	void					RocketFlare( const Vector& pos );

// Data
private:
	enum
	{ 
		MAX_TEMP_ENTITIES = 500,
		MAX_TEMP_ENTITY_SPRITES = 200,
		MAX_TEMP_ENTITY_STUDIOMODEL = 50,
	};

	// Global temp entity pool
	C_LocalTempEntity				*m_TempEnts;

	// Free and active temp entity lists
	C_LocalTempEntity				*m_pFreeTempEnts;
	C_LocalTempEntity				*m_pActiveTempEnts;

	// Muzzle flash sprites
	struct model_t			*m_pSpriteMuzzleFlash[10];
	struct model_t			*m_pSpriteAR2Flash[4];
	struct model_t			*m_pShells[3];

// Internal methods
private:
	CTempEnts( const CTempEnts & );

	C_LocalTempEntity		*TempEntAlloc( const Vector& org, model_t *model );
	C_LocalTempEntity		*TempEntAllocHigh( const Vector& org, model_t *model );
	void					TempEntFree( C_LocalTempEntity *pTemp, C_LocalTempEntity *pPrev );
	bool					FreeLowPriorityTempEnt();

	int						AddVisibleTempEntity( C_BaseEntity *pEntity );

	//AR2
	virtual	void			MuzzleFlash_AR2_Player( const Vector &origin, const QAngle &angles, int entityIndex );
	virtual	void			MuzzleFlash_AR2_NPC( const Vector &origin, const QAngle &angles, int entityIndex );
	
	//SMG1
	virtual void			MuzzleFlash_SMG1_Player( const Vector &origin, const QAngle &angles, int entityIndex );
	virtual void			MuzzleFlash_SMG1_NPC( const Vector &origin, const QAngle &angles, int entityIndex );

	//Shotgun
	virtual void			MuzzleFlash_Shotgun_Player( const Vector& origin, const QAngle &angles, int entityIndex );
	virtual void			MuzzleFlash_Shotgun_NPC( const Vector& origin, const QAngle &angles, int entityIndex );

	//Pistol
	virtual	void			MuzzleFlash_Pistol_Player( const Vector& origin, const QAngle &angles, int entityIndex );
	virtual	void			MuzzleFlash_Pistol_NPC( const Vector& origin, const QAngle &angles, int entityIndex );
};

// Temp entity interface
static CTempEnts g_TempEnts;
// Expose to rest of the client .dll
ITempEnts *tempents = ( ITempEnts * )&g_TempEnts;

C_LocalTempEntity::C_LocalTempEntity()
{
#ifdef _DEBUG
	tentOffset.Init();
	m_vecVelocity.Init();
	tempent_angles.Init();
	m_vecNormal.Init();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Prepare a temp entity for creation
// Input  : time - 
//			*model - 
//-----------------------------------------------------------------------------
void C_LocalTempEntity::Prepare( model_t *pmodel, float time )
{
	index = -1;
	Clear();

	// Use these to set per-frame and termination conditions / actions
	flags = FTENT_NONE;		
	die = time + 0.75;
	SetModelPointer( pmodel );
	m_nRenderMode = kRenderNormal;
	m_nRenderFX = kRenderFxNone;
	m_nBody = 0;
	m_nSkin = 0;
	fadeSpeed = 0.5;
	hitSound = 0;
	clientIndex = -1;
	bounceFactor = 1;
	m_nFlickerFrame = 0;
}

//-----------------------------------------------------------------------------
// Sets the velocity
//-----------------------------------------------------------------------------
void C_LocalTempEntity::SetVelocity( const Vector &vecVelocity )
{
	m_vecVelocity = vecVelocity;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_LocalTempEntity::DrawStudioModel( int flags )
{
	VPROF_BUDGET( "C_LocalTempEntity::DrawStudioModel", VPROF_BUDGETGROUP_MODEL_RENDERING );
	int drawn = 0;

	if ( !GetModel() || modelinfo->GetModelType( GetModel() ) != mod_studio )
		return drawn;
	
	// Make sure m_pstudiohdr is valid for drawing
	if ( !GetModelPtr() )
		return drawn;

	drawn = modelrender->DrawModel( 
		flags, 
		this,
		MODEL_INSTANCE_INVALID,
		index, 
		GetModel(),
		GetAbsOrigin(),
		GetAbsAngles(),
		GetSequence(),
		m_nSkin,
		m_nBody,
		m_nHitboxSet );
	return drawn;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
//-----------------------------------------------------------------------------
int	C_LocalTempEntity::DrawModel( int flags )
{
	int drawn = 0;

	if ( !GetModel() )
	{
		return drawn;
	}

	if ( this->flags & FTENT_BEOCCLUDED )
	{
		// Check normal
		Vector vecDelta = (GetAbsOrigin() - MainViewOrigin());
		VectorNormalize( vecDelta );
		float flDot = DotProduct( m_vecNormal, vecDelta );
		if ( flDot > 0 )
		{
			float flAlpha = RemapVal( min(flDot,0.3), 0, 0.3, 0, 1 );
			flAlpha = max( 1.0, tempent_renderamt - (tempent_renderamt * flAlpha) );
			SetRenderColorA( flAlpha );
		}
	}

	switch ( modelinfo->GetModelType( GetModel() ) )
	{
	case mod_sprite:
		drawn = DrawSprite( 
			this,
			GetModel(), 
			GetAbsOrigin(), 
			GetAbsAngles(), 
			m_flFrame,  // sprite frame to render
			m_nBody > 0 ? cl_entitylist->GetBaseEntity( m_nBody ) : NULL,  // attach to
			m_nSkin,  // attachment point
			m_nRenderMode, // rendermode
			m_nRenderFX, // renderfx
			m_clrRender->a, // alpha
			m_clrRender->r,
			m_clrRender->g,
			m_clrRender->b,
			m_flSpriteScale		  // sprite scale
			);
		break;
	case mod_studio:
		drawn = DrawStudioModel( flags );
		break;
	default:
		break;
	}

	return drawn;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_LocalTempEntity::IsActive( void )
{
	bool active = true;

	float life = die - gpGlobals->curtime;
	
	if ( life < 0 )
	{
		if ( flags & FTENT_FADEOUT )
		{
			int alpha;
			if (m_nRenderMode == kRenderNormal)
				m_nRenderMode = kRenderTransTexture;
			
			alpha = tempent_renderamt * ( 1 + life * fadeSpeed );
			
			if ( alpha <= 0 )
			{
				active = false;
				alpha = 0;
			}

			SetRenderColorA( alpha );
		}
		else 
		{
			active = false;
		}
	}

	// Never die tempents only die when their die is cleared
	if ( flags & FTENT_NEVERDIE )
	{
		active = (die != 0);
	}

	return active;
}

bool C_LocalTempEntity::Frame( float frametime, int framenumber )
{
	float		gravity, gravitySlow, fastFreq;

	fastFreq = gpGlobals->curtime * 5.5;
	gravity = -frametime * sv_gravity.GetFloat();
	gravitySlow = gravity * 0.5;

	Assert( !GetMoveParent() );

	m_vecPrevLocalOrigin = GetLocalOrigin();

	if ( flags & FTENT_PLYRATTACHMENT )
	{
		if ( IClientEntity *pClient = cl_entitylist->GetClientEntity( clientIndex ) )
		{
			SetLocalOrigin( pClient->GetAbsOrigin() + tentOffset );
		}
	}
	else if ( flags & FTENT_SINEWAVE )
	{
		x += m_vecVelocity[0] * frametime;
		y += m_vecVelocity[1] * frametime;

		SetLocalOrigin( Vector(
			x + sin( m_vecVelocity[2] + gpGlobals->curtime /* * anim.prevframe */ ) * (10*m_flSpriteScale),
			y + sin( m_vecVelocity[2] + fastFreq + 0.7 ) * (8*m_flSpriteScale),
			GetLocalOriginDim( Z_INDEX ) + m_vecVelocity[2] * frametime ) );
	}
	else if ( flags & FTENT_SPIRAL )
	{
		float s, c;
		SinCos( m_vecVelocity[2] + fastFreq, &s, &c );

		SetLocalOrigin( GetLocalOrigin() + Vector(
			m_vecVelocity[0] * frametime + 8 * sin( gpGlobals->curtime * 20 ),
			m_vecVelocity[1] * frametime + 4 * sin( gpGlobals->curtime * 30 ),
			m_vecVelocity[2] * frametime ) );
	}
	else
	{
		SetLocalOrigin( GetLocalOrigin() + m_vecVelocity * frametime );
	}
	
	if ( flags & FTENT_SPRANIMATE )
	{
		m_flFrame += frametime * m_flFrameRate;
		if ( m_flFrame >= m_flFrameMax )
		{
			m_flFrame = m_flFrame - (int)(m_flFrame);

			if ( !(flags & FTENT_SPRANIMATELOOP) )
			{
				// this animating sprite isn't set to loop, so destroy it.
				die = 0.0f;
				return false;
			}
		}
	}
	else if ( flags & FTENT_SPRCYCLE )
	{
		m_flFrame += frametime * 10;
		if ( m_flFrame >= m_flFrameMax )
		{
			m_flFrame = m_flFrame - (int)(m_flFrame);
		}
	}

	if ( flags & FTENT_SMOKEGROWANDFADE )
	{
		m_flSpriteScale += frametime * 0.5f;
		//m_clrRender.a -= frametime * 1500;
	}

	if ( flags & FTENT_ROTATE )
	{
		SetLocalAngles( GetLocalAngles() + tempent_angles * frametime );
	}

	if ( flags & (FTENT_COLLIDEALL | FTENT_COLLIDEWORLD) )
	{
		Vector	traceNormal;
		float	traceFraction = 1;

		traceNormal.Init();

		if ( flags & FTENT_COLLIDEALL )
		{
			trace_t pm;		
			UTIL_TraceLine( m_vecPrevLocalOrigin, GetLocalOrigin(), MASK_SOLID, NULL, COLLISION_GROUP_NONE, &pm );

			// Make sure it didn't bump into itself... (?!?)
			if  ( 
				(pm.fraction != 1) && 
					( (pm.DidHitWorld()) || 
					  (pm.m_pEnt != ClientEntityList().GetEnt(clientIndex)) ) 
				)
			{
				traceFraction = pm.fraction;
				VectorCopy( pm.plane.normal, traceNormal );
			}
		}
		else if ( flags & FTENT_COLLIDEWORLD )
		{
			trace_t trace;
			CTraceFilterWorldOnly traceFilter;
			UTIL_TraceLine( m_vecPrevLocalOrigin, GetLocalOrigin(), MASK_SOLID, &traceFilter, &trace );
			if ( trace.fraction != 1 )
			{
				traceFraction = trace.fraction;
				VectorCopy( trace.plane.normal, traceNormal );
			}
		}
		
		if ( traceFraction != 1 )	// Decent collision now, and damping works
		{
			float  proj, damp;

			// Place at contact point
			Vector newOrigin;
			VectorMA( m_vecPrevLocalOrigin, traceFraction*frametime, m_vecVelocity, newOrigin );
			SetLocalOrigin( newOrigin );
			
			// Damp velocity
			damp = bounceFactor;
			if ( flags & (FTENT_GRAVITY|FTENT_SLOWGRAVITY) )
			{
				damp *= 0.5;
				if ( traceNormal[2] > 0.9 )		// Hit floor?
				{
					if ( m_vecVelocity[2] <= 0 && m_vecVelocity[2] >= gravity*3 )
					{
						damp = 0;		// Stop
						flags &= ~(FTENT_ROTATE|FTENT_GRAVITY|FTENT_SLOWGRAVITY|FTENT_COLLIDEWORLD|FTENT_SMOKETRAIL);
						SetLocalAnglesDim( X_INDEX, 0 );
						SetLocalAnglesDim( Z_INDEX, 0 );
					}
				}
			}

			if (hitSound)
			{
				tempents->PlaySound(this, damp);
			}

			if (flags & FTENT_COLLIDEKILL)
			{
				// die on impact
				flags &= ~FTENT_FADEOUT;	
				die = gpGlobals->curtime;			
			}
			else
			{
				// Reflect velocity
				if ( damp != 0 )
				{
					proj = ((Vector)m_vecVelocity).Dot(traceNormal);
					VectorMA( m_vecVelocity, -proj*2, traceNormal, m_vecVelocity );
					// Reflect rotation (fake)
					SetLocalAnglesDim( Y_INDEX, -GetLocalAnglesDim( Y_INDEX ) );
				}
				
				if ( damp != 1 )
				{
					VectorScale( m_vecVelocity, damp, m_vecVelocity );
					SetLocalAngles( GetLocalAngles() * 0.9 );
				}
			}
		}
	}


	if ( (flags & FTENT_FLICKER) && framenumber == m_nFlickerFrame )
	{
		dlight_t *dl = effects->CL_AllocDlight (LIGHT_INDEX_TE_DYNAMIC);
		VectorCopy (GetLocalOrigin(), dl->origin);
		dl->radius = 60;
		dl->color.r = 255;
		dl->color.g = 120;
		dl->color.b = 0;
		dl->die = gpGlobals->curtime + 0.01;
	}

	if ( flags & FTENT_SMOKETRAIL )
	{
		Warning( "FIXME:  Reworkd smoketrail to be client side" );
	}

	if ( flags & FTENT_GRAVITY )
		m_vecVelocity[2] += gravity;
	else if ( flags & FTENT_SLOWGRAVITY )
		m_vecVelocity[2] += gravitySlow;

	if ( flags & FTENT_WINDBLOWN )
	{
		Vector vecWind;
		GetWindspeedAtTime( gpGlobals->curtime, vecWind );

		for ( int i = 0 ; i < 2 ; i++ )
		{
			if ( m_vecVelocity[i] < vecWind[i] )
			{
				m_vecVelocity[i] += ( frametime * TENT_WIND_ACCEL );

				// clamp
				if ( m_vecVelocity[i] > vecWind[i] )
					m_vecVelocity[i] = vecWind[i];
			}
			else if (m_vecVelocity[i] > vecWind[i] )
			{
				m_vecVelocity[i] -= ( frametime * TENT_WIND_ACCEL );

				// clamp.
				if ( m_vecVelocity[i] < vecWind[i] )
					m_vecVelocity[i] = vecWind[i];
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTempEnts::CTempEnts( void )
{
	m_TempEnts = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTempEnts::~CTempEnts( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Create a fizz effect
// Input  : *pent - 
//			modelIndex - 
//			density - 
//-----------------------------------------------------------------------------
void CTempEnts::FizzEffect( C_BaseEntity *pent, int modelIndex, int density )
{
	C_LocalTempEntity		*pTemp;
	const model_t	*model;
	int				i, width, depth, count, frameCount;
	float			maxHeight, speed, xspeed, yspeed;
	Vector			origin;
	Vector			mins, maxs;

	if ( !pent->GetModel() || !modelIndex ) 
		return;

	model = modelinfo->GetModel( modelIndex );
	if ( !model )
		return;

	count = density + 1;
	density = count * 3 + 6;

	modelinfo->GetModelBounds( pent->GetModel(), mins, maxs );

	maxHeight = maxs[2] - mins[2];
	width = maxs[0] - mins[0];
	depth = maxs[1] - mins[1];
	speed = (float)pent->GetRenderColor().r * 256.0 + (float)pent->GetRenderColor().g;
	if ( pent->GetRenderColor().b )
		speed = -speed;

	SinCos( pent->GetLocalAngles()[1]*M_PI/180, &yspeed, &xspeed );
	xspeed *= speed;
	yspeed *= speed;
	frameCount = modelinfo->GetModelFrameCount( model );

	for (i=0 ; i<count ; i++)
	{
		origin[0] = mins[0] + random->RandomInt(0,width-1);
		origin[1] = mins[1] + random->RandomInt(0,depth-1);
		origin[2] = mins[2];
		pTemp = TempEntAlloc( origin, (model_t *)model );
		if (!pTemp)
			return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];

		float zspeed = random->RandomInt(80,140);
		pTemp->SetVelocity( Vector(xspeed, yspeed, zspeed) );
		pTemp->die = gpGlobals->curtime + (maxHeight / zspeed) - 0.1;
		pTemp->m_flFrame = random->RandomInt(0,frameCount-1);
		// Set sprite scale
		pTemp->m_flSpriteScale = 1.0 / random->RandomFloat(2,5);
		pTemp->m_nRenderMode = kRenderTransAlpha;
		pTemp->SetRenderColorA( 255 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create bubbles
// Input  : *mins - 
//			*maxs - 
//			height - 
//			modelIndex - 
//			count - 
//			speed - 
//-----------------------------------------------------------------------------
void CTempEnts::Bubbles( const Vector &mins, const Vector &maxs, float height, int modelIndex, int count, float speed )
{
	C_LocalTempEntity			*pTemp;
	const model_t		*model;
	int					i, frameCount;
	float				sine, cosine;
	Vector				origin;

	if ( !modelIndex ) 
		return;

	model = modelinfo->GetModel( modelIndex );
	if ( !model )
		return;

	frameCount = modelinfo->GetModelFrameCount( model );

	for (i=0 ; i<count ; i++)
	{
		origin[0] = random->RandomInt( mins[0], maxs[0] );
		origin[1] = random->RandomInt( mins[1], maxs[1] );
		origin[2] = random->RandomInt( mins[2], maxs[2] );
		pTemp = TempEntAlloc( origin, ( model_t * )model );
		if (!pTemp)
			return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];
		SinCos( random->RandomInt( -M_PI, M_PI ), &sine, &cosine );
		
		float zspeed = random->RandomInt(80,140);
		pTemp->SetVelocity( Vector(speed * cosine, speed * sine, zspeed) );
		pTemp->die = gpGlobals->curtime + ((height - (origin[2] - mins[2])) / zspeed) - 0.1;
		pTemp->m_flFrame = random->RandomInt( 0, frameCount-1 );
		
		// Set sprite scale
		pTemp->m_flSpriteScale = 1.0 / random->RandomFloat(4,16);
		pTemp->m_nRenderMode = kRenderTransAlpha;
		
		pTemp->SetRenderColor( 255, 255, 255, 192 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create bubble trail
// Input  : *start - 
//			*end - 
//			height - 
//			modelIndex - 
//			count - 
//			speed - 
//-----------------------------------------------------------------------------
void CTempEnts::BubbleTrail( const Vector &start, const Vector &end, float height, int modelIndex, int count, float speed )
{
	C_LocalTempEntity			*pTemp;
	const model_t		*model;
	int					i, frameCount;
	float				dist, angle;
	Vector				origin;

	if ( !modelIndex ) 
		return;

	model = modelinfo->GetModel( modelIndex );
	if ( !model )
		return;

	frameCount = modelinfo->GetModelFrameCount( model );

	for (i=0 ; i<count ; i++)
	{
		dist = random->RandomFloat( 0, 1.0 );
		origin[0] = start[0] + dist * (end[0] - start[0]);
		origin[1] = start[1] + dist * (end[1] - start[1]);
		origin[2] = start[2] + dist * (end[2] - start[2]);
		pTemp = TempEntAlloc( origin, ( model_t * )model );
		if (!pTemp)
			return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];
		angle = random->RandomInt( -M_PI, M_PI );

		float zspeed = random->RandomInt(80,140);
		pTemp->SetVelocity( Vector(speed * cos(angle), speed * sin(angle), zspeed) );
		pTemp->die = gpGlobals->curtime + ((height - (origin[2] - start[2])) / zspeed) - 0.1;
		pTemp->m_flFrame = random->RandomInt(0,frameCount-1);
		// Set sprite scale
		pTemp->m_flSpriteScale = 1.0 / random->RandomFloat(4,8);
		pTemp->m_nRenderMode = kRenderTransAlpha;
		
		pTemp->SetRenderColor( 255, 255, 255, 192 );
	}
}

#define SHARD_VOLUME 12.0	// on shard ever n^3 units

//-----------------------------------------------------------------------------
// Purpose: Create model shattering shards
// Input  : *pos - 
//			*size - 
//			*dir - 
//			random - 
//			life - 
//			count - 
//			modelIndex - 
//			flags - 
//-----------------------------------------------------------------------------
void CTempEnts::BreakModel( const Vector &pos, const Vector &size, const Vector &dir, 
						   float randRange, float life, int count, int modelIndex, char flags)
{
	int					i, frameCount;
	C_LocalTempEntity			*pTemp;
	const model_t		*pModel;
	char				type;

	if (!modelIndex) 
		return;

	type = flags & BREAK_TYPEMASK;

	pModel = modelinfo->GetModel( modelIndex );
	if ( !pModel )
		return;

	frameCount = modelinfo->GetModelFrameCount( pModel );

	if (count == 0)
		// assume surface (not volume)
		count = (size[0] * size[1] + size[1] * size[2] + size[2] * size[0])/(3 * SHARD_VOLUME * SHARD_VOLUME);

	// Limit to 100 pieces
	if ( count > 100 )
		count = 100;

	for ( i = 0; i < count; i++ ) 
	{
		Vector vecSpot;

		// fill up the box with stuff

		vecSpot[0] = pos[0] + random->RandomFloat(-0.5,0.5) * size[0];
		vecSpot[1] = pos[1] + random->RandomFloat(-0.5,0.5) * size[1];
		vecSpot[2] = pos[2] + random->RandomFloat(-0.5,0.5) * size[2];

		pTemp = TempEntAlloc(vecSpot, ( model_t * )pModel);
		
		if (!pTemp)
			return;

		// keep track of break_type, so we know how to play sound on collision
		pTemp->hitSound = type;
		
		if ( modelinfo->GetModelType( pModel ) == mod_sprite )
			pTemp->m_flFrame = random->RandomInt(0,frameCount-1);
		else if ( modelinfo->GetModelType( pModel ) == mod_studio )
			pTemp->m_nBody = random->RandomInt(0,frameCount-1);

		pTemp->flags |= FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_SLOWGRAVITY;

		if ( random->RandomInt(0,255) < 200 ) 
		{
			pTemp->flags |= FTENT_ROTATE;
			pTemp->tempent_angles[0] = random->RandomFloat(-256,255);
			pTemp->tempent_angles[1] = random->RandomFloat(-256,255);
			pTemp->tempent_angles[2] = random->RandomFloat(-256,255);
		}

		if ( (random->RandomInt(0,255) < 100 ) && (flags & BREAK_SMOKE) )
			pTemp->flags |= FTENT_SMOKETRAIL;

		if ((type == BREAK_GLASS) || (flags & BREAK_TRANS))
		{
			pTemp->m_nRenderMode = kRenderTransTexture;
			pTemp->SetRenderColorA( 128 );
			pTemp->tempent_renderamt = 128;
		}
		else
		{
			pTemp->m_nRenderMode = kRenderNormal;
			pTemp->tempent_renderamt = 255;		// Set this for fadeout
		}

		pTemp->SetVelocity( Vector( dir[0] + random->RandomFloat(-randRange,randRange),
							dir[1] + random->RandomFloat(-randRange,randRange),
							dir[2] + random->RandomFloat(   0,randRange) ) );

		pTemp->die = gpGlobals->curtime + life + random->RandomFloat(0,1);	// Add an extra 0-1 secs of life
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create sprite TE
// Input  : *pos - 
//			*dir - 
//			scale - 
//			modelIndex - 
//			rendermode - 
//			renderfx - 
//			a - 
//			life - 
//			flags - 
// Output : C_LocalTempEntity
//-----------------------------------------------------------------------------
C_LocalTempEntity *CTempEnts::TempSprite( const Vector &pos, const Vector &dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags, const Vector &normal )
{
	C_LocalTempEntity			*pTemp;
	const model_t		*model;
	int					frameCount;

	if ( !modelIndex ) 
		return NULL;

	model = modelinfo->GetModel( modelIndex );
	if ( !model )
	{
		Warning("No model %d!\n", modelIndex);
		return NULL;
	}

	frameCount = modelinfo->GetModelFrameCount( model );

	pTemp = TempEntAlloc( pos, ( model_t * )model );
	if (!pTemp)
		return NULL;

	pTemp->m_flFrameMax = frameCount - 1;
	pTemp->m_flFrameRate = 10;
	pTemp->m_nRenderMode = rendermode;
	pTemp->m_nRenderFX = renderfx;
	pTemp->m_flSpriteScale = scale;
	pTemp->tempent_renderamt = a * 255;
	pTemp->m_vecNormal = normal;
	pTemp->SetRenderColor( 255, 255, 255, a * 255 );

	pTemp->flags |= flags;

	pTemp->SetVelocity( dir );
	pTemp->SetLocalOrigin( pos );
	if ( life )
		pTemp->die = gpGlobals->curtime + life;
	else
		pTemp->die = gpGlobals->curtime + (frameCount * 0.1) + 1;

	pTemp->m_flFrame = 0;
	return pTemp;
}

//-----------------------------------------------------------------------------
// Purpose: Spray sprite
// Input  : *pos - 
//			*dir - 
//			modelIndex - 
//			count - 
//			speed - 
//			iRand - 
//-----------------------------------------------------------------------------
void CTempEnts::Sprite_Spray( const Vector &pos, const Vector &dir, int modelIndex, int count, int speed, int iRand )
{
	C_LocalTempEntity			*pTemp;
	const model_t		*pModel;
	float				noise;
	float				znoise;
	int					frameCount;
	int					i;

	noise = (float)iRand / 100;

	// more vertical displacement
	znoise = noise * 1.5;
	
	if ( znoise > 1 )
	{
		znoise = 1;
	}

	pModel = modelinfo->GetModel( modelIndex );
	
	if ( !pModel )
	{
		Warning("No model %d!\n", modelIndex);
		return;
	}

	frameCount = modelinfo->GetModelFrameCount( pModel ) - 1;

	for ( i = 0; i < count; i++ )
	{
		pTemp = TempEntAlloc( pos, ( model_t * )pModel );
		if (!pTemp)
			return;

		pTemp->m_nRenderMode = kRenderTransAlpha;
		pTemp->SetRenderColor( 255, 255, 255, 255 );
		pTemp->tempent_renderamt = 255;
		pTemp->m_nRenderFX = kRenderFxNoDissipation;
		//pTemp->scale = random->RandomFloat( 0.1, 0.25 );
		pTemp->m_flSpriteScale = 0.5;
		pTemp->flags |= FTENT_FADEOUT | FTENT_SLOWGRAVITY;
		pTemp->fadeSpeed = 2.0;

		// make the spittle fly the direction indicated, but mix in some noise.
		Vector velocity;
		velocity.x = dir[ 0 ] + random->RandomFloat ( -noise, noise );
		velocity.y = dir[ 1 ] + random->RandomFloat ( -noise, noise );
		velocity.z = dir[ 2 ] + random->RandomFloat ( 0, znoise );
		velocity *= random->RandomFloat( (speed * 0.8), (speed * 1.2) );
		pTemp->SetVelocity( velocity );

		pTemp->SetLocalOrigin( pos );

		pTemp->die = gpGlobals->curtime + 0.35;

		pTemp->m_flFrame = random->RandomInt( 0, frameCount );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Attaches entity to player
// Input  : client - 
//			modelIndex - 
//			zoffset - 
//			life - 
//-----------------------------------------------------------------------------
void CTempEnts::AttachTentToPlayer( int client, int modelIndex, float zoffset, float life )
{
	C_LocalTempEntity			*pTemp;
	const model_t		*pModel;
	Vector				position;
	int					frameCount;

	if ( client < 0 || client > engine->GetMaxClients() )
	{
		Warning("Bad client in AttachTentToPlayer()!\n");
		return;
	}

	IClientEntity *clientClass = cl_entitylist->GetClientEntity( client );
	if ( !clientClass )
	{
		Warning("Couldn't get IClientEntity for %i\n", client );
		return;
	}

	pModel = modelinfo->GetModel( modelIndex );
	
	if ( !pModel )
	{
		Warning("No model %d!\n", modelIndex);
		return;
	}

	VectorCopy( clientClass->GetAbsOrigin(), position );
	position[ 2 ] += zoffset;

	pTemp = TempEntAllocHigh( position, ( model_t * )pModel );
	if (!pTemp)
	{
		Warning("No temp ent.\n");
		return;
	}

	pTemp->m_nRenderMode = kRenderNormal;
	pTemp->SetRenderColorA( 255 );
	pTemp->tempent_renderamt = 255;
	pTemp->m_nRenderFX = kRenderFxNoDissipation;
	pTemp->m_flModelScale = 1;
	
	pTemp->clientIndex = client;
	pTemp->tentOffset[ 0 ] = 0;
	pTemp->tentOffset[ 1 ] = 0;
	pTemp->tentOffset[ 2 ] = zoffset;
	pTemp->die = gpGlobals->curtime + life;
	pTemp->flags |= FTENT_PLYRATTACHMENT | FTENT_PERSIST;

	// is the model a sprite?
	if ( modelinfo->GetModelType( pTemp->GetModel() ) == mod_sprite )
	{
		frameCount = modelinfo->GetModelFrameCount( pModel );
		pTemp->m_flFrameMax = frameCount - 1;
		pTemp->flags |= FTENT_SPRANIMATE | FTENT_SPRANIMATELOOP;
		pTemp->m_flFrameRate = 10;
	}
	else
	{
		// no animation support for attached clientside studio models.
		pTemp->m_flFrameMax = 0;
	}

	pTemp->m_flFrame = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Detach entity from player
//-----------------------------------------------------------------------------
void CTempEnts::KillAttachedTents( int client )
{
	C_LocalTempEntity *pTemp;

	if ( client < 0 || client > engine->GetMaxClients() )
	{
		Warning("Bad client in KillAttachedTents()!\n");
		return;
	}

	pTemp = m_pActiveTempEnts;

	while ( pTemp )
	{
		if ( pTemp->flags & FTENT_PLYRATTACHMENT )
		{
			// this TENT is player attached.
			// if it is attached to this client, set it to die instantly.
			if ( pTemp->clientIndex == client )
			{
				pTemp->die = gpGlobals->curtime;// good enough, it will die on next tent update. 
			}
		}

		pTemp = pTemp->next;
	}
}



//-----------------------------------------------------------------------------
// Purpose: Create ricochet sprite
// Input  : *pos - 
//			*pmodel - 
//			duration - 
//			scale - 
//-----------------------------------------------------------------------------
void CTempEnts::RicochetSprite( const Vector &pos, model_t *pmodel, float duration, float scale )
{
	C_LocalTempEntity	*pTemp;

	pTemp = TempEntAlloc( pos, ( model_t * )pmodel );
	if (!pTemp)
		return;

	pTemp->m_nRenderMode = kRenderGlow;
	pTemp->m_nRenderFX = kRenderFxNoDissipation;
	pTemp->SetRenderColorA( 200 );
	pTemp->tempent_renderamt = 200;
	pTemp->m_flSpriteScale = scale;
	pTemp->flags = FTENT_FADEOUT;

	pTemp->SetVelocity( vec3_origin );

	pTemp->SetLocalOrigin( pos );
	
	pTemp->fadeSpeed = 8;
	pTemp->die = gpGlobals->curtime;

	pTemp->m_flFrame = 0;
	pTemp->SetLocalAnglesDim( Z_INDEX, 45 * random->RandomInt( 0, 7 ) );
}

//-----------------------------------------------------------------------------
// Purpose: Create blood sprite
// Input  : *org - 
//			colorindex - 
//			modelIndex - 
//			modelIndex2 - 
//			size - 
//-----------------------------------------------------------------------------
void CTempEnts::BloodSprite( const Vector &org, int r, int g, int b, int a, int modelIndex, int modelIndex2, float size )
{
	const model_t			*model;

	//Validate the model first
	if ( modelIndex && (model = modelinfo->GetModel( modelIndex ) ) )
	{
		C_LocalTempEntity		*pTemp;
		int						frameCount = modelinfo->GetModelFrameCount( model );
		color32					impactcolor = { r, g, b, a };

		//Large, single blood sprite is a high-priority tent
		if ( pTemp = TempEntAllocHigh( org, ( model_t * )model ) )
		{
			pTemp->m_nRenderMode	= kRenderTransTexture;
			pTemp->m_nRenderFX		= kRenderFxClampMinScale;
			pTemp->m_flSpriteScale	= random->RandomFloat( size / 25, size / 35);
			pTemp->flags			= FTENT_SPRANIMATE;
 
			pTemp->m_clrRender		= impactcolor;
			pTemp->tempent_renderamt= pTemp->m_clrRender->a;

			pTemp->SetVelocity( vec3_origin );

			pTemp->m_flFrameRate	= frameCount * 4; // Finish in 0.250 seconds
			pTemp->die				= gpGlobals->curtime + (frameCount / pTemp->m_flFrameRate); // Play the whole thing Once

			pTemp->m_flFrame		= 0;
			pTemp->m_flFrameMax		= frameCount - 1;
			pTemp->bounceFactor		= 0;
			pTemp->SetLocalAnglesDim( Z_INDEX, random->RandomInt( 0, 360 ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create default sprite TE
// Input  : *pos - 
//			spriteIndex - 
//			framerate - 
// Output : C_LocalTempEntity
//-----------------------------------------------------------------------------
C_LocalTempEntity *CTempEnts::DefaultSprite( const Vector &pos, int spriteIndex, float framerate )
{
	C_LocalTempEntity		*pTemp;
	int				frameCount;
	const model_t	*pSprite;

	// don't spawn while paused
	if ( gpGlobals->frametime == 0.0 )
		return NULL;

	pSprite = modelinfo->GetModel( spriteIndex );
	if ( !spriteIndex || !pSprite || modelinfo->GetModelType( pSprite ) != mod_sprite )
	{
		DevWarning( 1,"No Sprite %d!\n", spriteIndex);
		return NULL;
	}

	frameCount = modelinfo->GetModelFrameCount( pSprite );

	pTemp = TempEntAlloc( pos, ( model_t * )pSprite );
	if (!pTemp)
		return NULL;

	pTemp->m_flFrameMax = frameCount - 1;
	pTemp->m_flSpriteScale = 1.0;
	pTemp->flags |= FTENT_SPRANIMATE;
	if ( framerate == 0 )
		framerate = 10;

	pTemp->m_flFrameRate = framerate;
	pTemp->die = gpGlobals->curtime + (float)frameCount / framerate;
	pTemp->m_flFrame = 0;
	pTemp->SetLocalOrigin( pos );

	return pTemp;
}

//-----------------------------------------------------------------------------
// Purpose: Create sprite smoke
// Input  : *pTemp - 
//			scale - 
//-----------------------------------------------------------------------------
void CTempEnts::Sprite_Smoke( C_LocalTempEntity *pTemp, float scale )
{
	if ( !pTemp )
		return;

	pTemp->m_nRenderMode = kRenderTransAlpha;
	pTemp->m_nRenderFX = kRenderFxNone;
	pTemp->SetVelocity( Vector( 0, 0, 30 ) );
	int iColor = random->RandomInt(20,35);
	pTemp->SetRenderColor( iColor,
		iColor,
		iColor,
		255 );
	pTemp->SetLocalOriginDim( Z_INDEX, pTemp->GetLocalOriginDim( Z_INDEX ) + 20 );
	pTemp->m_flSpriteScale = scale;
	pTemp->flags = FTENT_WINDBLOWN;

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pos1 - 
//			angles - 
//			type - 
//-----------------------------------------------------------------------------
void CTempEnts::EjectBrass( const Vector &pos1, const QAngle &angles, const QAngle &gunAngles, int type )
{
	if ( cl_ejectbrass.GetBool() == false )
		return;

	const model_t *pModel = m_pShells[type];
	
	if ( pModel == NULL )
		return;

	C_LocalTempEntity	*pTemp = TempEntAlloc( pos1, ( model_t * ) pModel );

	if ( pTemp == NULL )
		return;

	//Keep track of shell type
	if ( type == 2 )
	{
		pTemp->hitSound = BOUNCE_SHOTSHELL;
	}
	else
	{
		pTemp->hitSound = BOUNCE_SHELL;
	}

	pTemp->m_nBody	= 0;

	pTemp->flags |= ( FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_GRAVITY | FTENT_ROTATE );

	pTemp->tempent_angles[0] = random->RandomFloat(-256,255);
	pTemp->tempent_angles[1] = random->RandomFloat(-256,255);
	pTemp->tempent_angles[2] = random->RandomFloat(-256,255);

	//Face forward
	pTemp->SetAbsAngles( gunAngles );

	pTemp->m_nRenderMode = kRenderNormal;
	pTemp->tempent_renderamt = 255;		// Set this for fadeout

	Vector	dir;

	AngleVectors( angles, &dir );

	dir *= random->RandomFloat( 150.0f, 200.0f );

	pTemp->SetVelocity( Vector(dir[0] + random->RandomFloat(-64,64),
						dir[1] + random->RandomFloat(-64,64),
						dir[2] + random->RandomFloat(  0,64) ) );

	pTemp->die = gpGlobals->curtime + 1.0f + random->RandomFloat( 0.0f, 1.0f );	// Add an extra 0-1 secs of life	
}

//-----------------------------------------------------------------------------
// Purpose: Create some simple physically simulated models
//-----------------------------------------------------------------------------
void CTempEnts::SpawnTempModel( model_t *pModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, float flLifeTime, int iFlags )
{
	Assert( pModel );

	// Alloc a new tempent
	C_LocalTempEntity *pTemp = TempEntAlloc( vecOrigin, ( model_t * ) pModel );
	if ( !pTemp )
		return;

	/*
	// Keep track of shell type
	if ( type == 2 )
	{
		pTemp->hitSound = BOUNCE_SHOTSHELL;
	}
	else
	{
		pTemp->hitSound = BOUNCE_SHELL;
	}
	*/

	pTemp->SetAbsAngles( vecAngles );
	pTemp->m_nBody	= 0;
	pTemp->flags |= iFlags;
	pTemp->tempent_angles[0] = random->RandomFloat(-255,255);
	pTemp->tempent_angles[1] = random->RandomFloat(-255,255);
	pTemp->tempent_angles[2] = random->RandomFloat(-255,255);
	pTemp->m_nRenderMode = kRenderNormal;
	pTemp->tempent_renderamt = 255;
	pTemp->SetVelocity( vecVelocity );
	pTemp->die = gpGlobals->curtime + flLifeTime;
}

//-----------------------------------------------------------------------------
// Purpose: Play muzzle flash
// Input  : *pos1 - 
//			type - 
//-----------------------------------------------------------------------------
void CTempEnts::MuzzleFlash( const Vector& pos1, const QAngle& angles, int type, int entityIndex, bool firstPerson )
{
	switch ( type )
	{
	//
	// AR2
	//
	case MUZZLEFLASH_AR2:
		if ( firstPerson )
		{
			MuzzleFlash_AR2_Player( pos1, angles, entityIndex );
		}
		else
		{
			MuzzleFlash_AR2_NPC( pos1, angles, entityIndex );
		}
		break;

	//
	// Shotgun
	//
	case MUZZLEFLASH_SHOTGUN:
		if ( firstPerson )
		{
			MuzzleFlash_Shotgun_Player( pos1, angles, entityIndex );
		}
		else
		{
			MuzzleFlash_Shotgun_NPC( pos1, angles, entityIndex );
		}
		break;

	// UNDONE: These need their own effects/sprites.  For now use the pistol
	// SMG1
	case MUZZLEFLASH_SMG1:
		if ( firstPerson )
		{
			MuzzleFlash_SMG1_Player( pos1, angles, entityIndex );
		}
		else
		{
			MuzzleFlash_SMG1_NPC( pos1, angles, entityIndex );
		}
		break;

	// SMG2
	case MUZZLEFLASH_SMG2:
	case MUZZLEFLASH_PISTOL:
		if ( firstPerson )
		{
			MuzzleFlash_Pistol_Player( pos1, angles, entityIndex );
		}
		else
		{
			MuzzleFlash_Pistol_NPC( pos1, angles, entityIndex );
		}
		break;
	
	default:
// Old code
#if 0
		{
			C_LocalTempEntity	*pTemp;
			int index;
			float scale;
			int			frameCount;

			index = type % 10;
			scale = (type / 10) * 0.1;
			if (scale == 0)
				scale = 0.5;

			frameCount = modelinfo->GetModelFrameCount( m_pSpriteMuzzleFlash[index] );

			// DevMsg( 1,"%d %f\n", index, scale );
			pTemp = TempEntAlloc( pos1, ( model_t * )m_pSpriteMuzzleFlash[index] );
			if (!pTemp)
				return;
			pTemp->m_nRenderMode = kRenderTransAdd;
			pTemp->m_clrRender.a = 255;
			pTemp->m_nRenderFX = 0;
			pTemp->m_clrRender.r = 255;
			pTemp->m_clrRender.g = 255;
			pTemp->m_clrRender.b = 255;
			VectorCopy( pos1, pTemp->origin );
			pTemp->die = gpGlobals->curtime + 0.01;
			pTemp->m_flFrame = random->RandomInt(0, frameCount-1);
			pTemp->m_flFrameMax = frameCount - 1;
			VectorCopy( vec3_origin, pTemp->angles );


			if (index == 0)
			{
				// Rifle flash
				pTemp->scale = scale * random->RandomFloat( 0.5, 0.6 );
				pTemp->angles[2] = 90 * random->RandomInt(0, 3);
			}
			else
			{
				pTemp->scale = scale;
				pTemp->angles[2] = random->RandomInt(0,359);
			}
		
			view->AddVisibleEntity( pTemp );
		}
#endif
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create explosion sprite
// Input  : *pTemp - 
//			scale - 
//			flags - 
//-----------------------------------------------------------------------------
void CTempEnts::Sprite_Explode( C_LocalTempEntity *pTemp, float scale, int flags )
{
	if ( !pTemp )
		return;

	if ( flags & TE_EXPLFLAG_NOADDITIVE )
	{
		// solid sprite
		pTemp->m_nRenderMode = kRenderNormal;
		pTemp->SetRenderColorA( 255 ); 
	}
	else if( flags & TE_EXPLFLAG_DRAWALPHA )
	{
		// alpha sprite
		pTemp->m_nRenderMode = kRenderTransAlpha;
		pTemp->SetRenderColorA( 180 );
	}
	else
	{
		// additive sprite
		pTemp->m_nRenderMode = kRenderTransAdd;
		pTemp->SetRenderColorA( 180 );
	}

	if ( flags & TE_EXPLFLAG_ROTATE )
	{
		pTemp->SetLocalAnglesDim( Z_INDEX, random->RandomInt( 0, 360 ) );
	}

	pTemp->m_nRenderFX = kRenderFxNone;
	pTemp->SetVelocity( Vector( 0, 0, 8 ) );
	pTemp->SetRenderColor( 255, 255, 255 );
	pTemp->SetLocalOriginDim( Z_INDEX, pTemp->GetLocalOriginDim( Z_INDEX ) + 10 );
	pTemp->m_flSpriteScale = scale;
}

enum
{
	SHELL_NONE = 0,
	SHELL_SMALL,
	SHELL_BIG,
	SHELL_SHOTGUN,
};

//-----------------------------------------------------------------------------
// Purpose: Clear existing temp entities
//-----------------------------------------------------------------------------
void CTempEnts::Clear( void )
{
	if ( !m_TempEnts )
	{
		return;
	}
	int i;

	for ( i = 0; i < MAX_TEMP_ENTITIES-1; i++ )
	{
		m_TempEnts[i].next = &m_TempEnts[i+1];
		m_TempEnts[ i ].Prepare( NULL, 0.0 );
	}
	m_TempEnts[ MAX_TEMP_ENTITIES-1 ].next = NULL;
	m_pFreeTempEnts = m_TempEnts;
	m_pActiveTempEnts = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Allocate temp entity ( normal/low priority )
// Input  : *org - 
//			*model - 
// Output : C_LocalTempEntity
//-----------------------------------------------------------------------------
C_LocalTempEntity *CTempEnts::TempEntAlloc( const Vector& org, model_t *model )
{
	C_LocalTempEntity		*pTemp;

	if ( !model )
	{
		DevWarning( 1, "Can't create temporary entity with NULL model!\n" );
		return NULL;
	}

	if ( !m_pFreeTempEnts )
	{
		DevWarning( 1, "Overflow %d temporary ents!\n", MAX_TEMP_ENTITIES );
		return NULL;
	}

	pTemp = m_pFreeTempEnts;
	m_pFreeTempEnts = pTemp->next;

	pTemp->Prepare( model, gpGlobals->curtime );

	pTemp->priority = TENTPRIORITY_LOW;
	pTemp->SetLocalOrigin( org );

	pTemp->next = m_pActiveTempEnts;
	m_pActiveTempEnts = pTemp;

	pTemp->SetupEntityRenderHandle( RENDER_GROUP_OTHER );

	return pTemp;
}

void CTempEnts::TempEntFree( C_LocalTempEntity *pTemp, C_LocalTempEntity *pPrev )
{
	// Remove from the active list.
	if( pPrev )	
	{
		pPrev->next = pTemp->next;
	}
	else
	{
		m_pActiveTempEnts = pTemp->next;
	}

	// Cleanup its data.
	pTemp->RemoveFromLeafSystem();

	// Add to the free list.
	pTemp->next = m_pFreeTempEnts;
	m_pFreeTempEnts = pTemp;
}


// Free the first low priority tempent it finds.
bool CTempEnts::FreeLowPriorityTempEnt()
{
	C_LocalTempEntity *pActive = m_pActiveTempEnts;
	C_LocalTempEntity *pPrev = NULL;

	while ( pActive )
	{
		if ( pActive->priority == TENTPRIORITY_LOW )
		{
			TempEntFree( pActive, pPrev );
			return true;
		}
		
		pPrev = pActive;
		pActive = pActive->next;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Allocate a temp entity, if there are no slots, kick out a low priority
//  one if possible
// Input  : *org - 
//			*model - 
// Output : C_LocalTempEntity
//-----------------------------------------------------------------------------
C_LocalTempEntity *CTempEnts::TempEntAllocHigh( const Vector& org, model_t *model )
{
	C_LocalTempEntity		*pTemp;

	if ( !model )
	{
		DevWarning( 1, "temporary ent model invalid\n" );
		return NULL;
	}

	if ( !m_pFreeTempEnts )
	{
		// no temporary ents free, so find the first active low-priority temp ent 
		// and overwrite it.
		FreeLowPriorityTempEnt();
	}

	if ( !m_pFreeTempEnts )
	{
		// didn't find anything? The tent list is either full of high-priority tents
		// or all tents in the list are still due to live for > 10 seconds. 
		DevWarning( 1,"Couldn't alloc a high priority TENT!\n" );
		return NULL;
	}

	// Move out of the free list and into the active list.
	pTemp = m_pFreeTempEnts;
	m_pFreeTempEnts = pTemp->next;

	pTemp->next = m_pActiveTempEnts;
	m_pActiveTempEnts = pTemp;


	pTemp->Prepare( model, gpGlobals->curtime );

	pTemp->priority = TENTPRIORITY_HIGH;
	pTemp->SetLocalOrigin( org );

	pTemp->SetupEntityRenderHandle( RENDER_GROUP_OTHER );

	return pTemp;
}

//-----------------------------------------------------------------------------
// Purpose: Play sound when temp ent collides with something
// Input  : *pTemp - 
//			damp - 
//-----------------------------------------------------------------------------
void CTempEnts::PlaySound ( C_LocalTempEntity *pTemp, float damp )
{
	const char	*soundname = NULL;
	float fvol;
	bool isshellcasing = false;
	int zvel;

	switch ( pTemp->hitSound )
	{
	default:
		return;	// null sound

	case BOUNCE_GLASS:
		{
			soundname = "Bounce.Glass";
		}
		break;

	case BOUNCE_METAL:
		{
			soundname = "Bounce.Metal";
		}
		break;

	case BOUNCE_FLESH:
		{
			soundname = "Bounce.Flesh";
		}
		break;

	case BOUNCE_WOOD:
		{
			soundname = "Bounce.Wood";
		}
		break;

	case BOUNCE_SHRAP:
		{
			soundname = "Bounce.Shrapnel";
		}
		break;

	case BOUNCE_SHOTSHELL:
		{
			soundname = "Bounce.ShotgunShell";
			isshellcasing = true; // shell casings have different playback parameters
		}
		break;

	case BOUNCE_SHELL:
		{
			soundname = "Bounce.Shell";
			isshellcasing = true; // shell casings have different playback parameters
		}
		break;

	case BOUNCE_CONCRETE:
		{
			soundname = "Bounce.Concrete";
		}
		break;
	}

	zvel = abs( pTemp->GetVelocity()[2] );
		
	// only play one out of every n

	if ( isshellcasing )
	{	
		// play first bounce, then 1 out of 3		
		if ( zvel < 200 && random->RandomInt(0,3) )
			return;
	}
	else
	{
		if ( random->RandomInt(0,5) ) 
			return;
	}

	CSoundParameters params;
	if ( !C_BaseEntity::GetParametersForSound( soundname, params ) )
		return;

	fvol = params.volume;

	if ( damp > 0.0 )
	{
		int pitch;
		
		if ( isshellcasing )
		{
			fvol *= min (1.0, ((float)zvel) / 350.0); 
		}
		else
		{
			fvol *= min (1.0, ((float)zvel) / 450.0); 
		}
		
		if ( !random->RandomInt(0,3) && !isshellcasing )
		{
			pitch = random->RandomInt( params.pitchlow, params.pitchhigh );
		}
		else
		{
			pitch = params.pitch;
		}

		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, CHAN_STATIC, params.soundname,
			fvol, params.soundlevel, 0, pitch, &pTemp->GetAbsOrigin() );
	}
}
					
//-----------------------------------------------------------------------------
// Purpose: Add temp entity to visible entities list of it's in PVS
// Input  : *pEntity - 
// Output : int
//-----------------------------------------------------------------------------
int CTempEnts::AddVisibleTempEntity( C_BaseEntity *pEntity )
{
	int i;
	Vector mins, maxs;
	Vector model_mins, model_maxs;

	if ( !pEntity->GetModel() )
		return 0;

	modelinfo->GetModelBounds( pEntity->GetModel(), model_mins, model_maxs );

	for (i=0 ; i<3 ; i++)
	{
		mins[i] = pEntity->GetAbsOrigin()[i] + model_mins[i];
		maxs[i] = pEntity->GetAbsOrigin()[i] + model_maxs[i];
	}

	// FIXME: Vis isn't setup by the time we get here, so this call fails if 
	//		  you try to add a tempent before the first frame is drawn, and it's
	//		  one frame behind the rest of the time. Fix this.
	// does the box intersect a visible leaf?
	//if ( engine->IsBoxInViewCluster( mins, maxs ) )
	{
		// Temporary entities have no corresponding element in cl_entitylist
		pEntity->index = -1;
		
		// Add to list
		view->AddVisibleEntity( pEntity );
		return 1;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Runs Temp Ent simulation routines
//-----------------------------------------------------------------------------
void CTempEnts::Update(void)
{
	static int gTempEntFrame = 0;
	C_LocalTempEntity	*current, *pnext, *pprev;
	float		frametime;

	// Don't simulate while loading
	if ( !m_pActiveTempEnts || !engine->IsInGame() )		
	{
		return;
	}

	// !!!BUGBUG	-- This needs to be time based
	gTempEntFrame = (gTempEntFrame+1) & 31;

	current = m_pActiveTempEnts;
	frametime = gpGlobals->frametime;

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL 
	// tent, then set this BOOL to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).

	// !!! Don't simulate while paused....  This is sort of a hack, revisit.
	if ( frametime == 0 )
	{
		while ( current )
		{
			AddVisibleTempEntity( current );
			current = current->next;
		}
	}
	else
	{
		pprev = NULL;

		while ( current )
		{
			bool fTempEntFreed = false;

			pnext = current->next;

			// Kill it
			if ( !current->IsActive() || !current->Frame( frametime, gTempEntFrame ) )
			{
				TempEntFree( current, pprev );
				fTempEntFreed = true;
			}
			else
			{
				// Cull to PVS (not frustum cull, just PVS)
				if ( !AddVisibleTempEntity( current ) )
				{
					if ( !(current->flags & FTENT_PERSIST) ) 
					{
						// If we can't draw it this frame, just dump it.
						current->die = gpGlobals->curtime;
						// Don't fade out, just die
						current->flags &= ~FTENT_FADEOUT;

						TempEntFree( current, pprev );
						fTempEntFreed = true;
					}
				}
			}

			if ( fTempEntFreed == false )
			{
				pprev = current;
			}
			current = pnext;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initialize TE system
//-----------------------------------------------------------------------------
void CTempEnts::Init (void)
{
	m_TempEnts = new C_LocalTempEntity[ MAX_TEMP_ENTITIES ];

	m_pSpriteMuzzleFlash[0] = (model_t *)engine->LoadModel( "sprites/ar2_muzzle1.vmt" );
	m_pSpriteMuzzleFlash[1] = (model_t *)engine->LoadModel( "sprites/muzzleflash4.vmt" );
	m_pSpriteMuzzleFlash[2] = (model_t *)engine->LoadModel( "sprites/muzzleflash4.vmt" );

	m_pSpriteAR2Flash[0] = (model_t *)engine->LoadModel( "sprites/ar2_muzzle1b.vmt" );
	m_pSpriteAR2Flash[1] = (model_t *)engine->LoadModel( "sprites/ar2_muzzle2b.vmt" );
	m_pSpriteAR2Flash[2] = (model_t *)engine->LoadModel( "sprites/ar2_muzzle3b.vmt" );
	m_pSpriteAR2Flash[3] = (model_t *)engine->LoadModel( "sprites/ar2_muzzle4b.vmt" );

	m_pShells[0] = (model_t *) engine->LoadModel( "models/weapons/shell.mdl" );
	m_pShells[1] = (model_t *) engine->LoadModel( "models/weapons/rifleshell.mdl" );
	m_pShells[2] = (model_t *) engine->LoadModel( "models/weapons/shotgun_shell.mdl" );

	// Clear out lists to start
	Clear();

	for ( int i = 0; i < MAX_TEMP_ENTITIES; i++ )
	{
		m_TempEnts[ i ].Interp_SetupMappings( m_TempEnts[ i ].GetVarMapping() );
	}
}


void CTempEnts::LevelShutdown()
{
	// Free all active tempents.
	while( m_pActiveTempEnts )
	{
		TempEntFree( m_pActiveTempEnts, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTempEnts::Shutdown()
{
	LevelShutdown();
	Clear();
	delete[] m_TempEnts;
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_AR2_Player( const Vector &origin, const QAngle &angles, int entityIndex )
{
#if 1

	float	scale = 1.5f;
	Vector	forward, offset;

	AngleVectors( angles, &forward );
	float flScale = random->RandomFloat( scale-0.25f, scale );

	if ( flScale < 0.5f )
	{
		flScale = 0.5f;
	}

	//
	// Flash
	//
	C_LocalTempEntity *pTemp;

	for ( int i = 1; i < 9; i++ )
	{
		offset = origin + (forward * (i*8.0f*flScale));

		pTemp = TempEntAllocHigh( offset, (model_t *) m_pSpriteAR2Flash[random->RandomInt(0,3)] );
		
		if ( pTemp == NULL )
			return;

		//Setup our colors and type
		pTemp->m_nRenderMode	= kRenderTransAdd;
		pTemp->SetRenderColor( 164, 164, 164+random->RandomInt(0,64), 255 );
		pTemp->tempent_renderamt= 255;
		pTemp->m_nRenderFX		= kRenderFxNone;
		pTemp->flags			= FTENT_PERSIST;
		pTemp->die				= gpGlobals->curtime + 0.025f;
		pTemp->m_flFrame		= 0;
		pTemp->m_flFrameMax		= 0;
		
		pTemp->SetLocalOrigin( offset );
		pTemp->SetLocalAngles( vec3_angle );
		
		pTemp->SetVelocity( vec3_origin );
		
		//Scale and rotate
		pTemp->m_flSpriteScale	= ( (random->RandomFloat( 6.0f, 8.0f ) * (12-(i))/9) * flScale ) / 32;
		pTemp->SetLocalAnglesDim( Z_INDEX, random->RandomInt( 0, 360 ) );

		view->AddVisibleEntity( pTemp );
	}

	//Flare
	pTemp = TempEntAlloc( origin, (model_t *) m_pSpriteMuzzleFlash[0] );
	
	if ( pTemp == NULL )
		return;

	//Setup our colors and type
	pTemp->m_nRenderMode	= kRenderTransAdd;
	pTemp->SetRenderColor( 255, 255, 255, 255 );
	pTemp->tempent_renderamt= 255;
	pTemp->m_nRenderFX		= kRenderFxNone;
	pTemp->flags			= FTENT_PERSIST;
	pTemp->die				= gpGlobals->curtime + 0.025f;
	pTemp->m_flFrame		= 0;
	pTemp->m_flFrameMax		= 0;
	
	pTemp->SetLocalOrigin( origin );
	pTemp->SetLocalAngles( vec3_angle );
	
	pTemp->SetVelocity( vec3_origin );
	
	//Scale and rotate
	pTemp->m_flSpriteScale	= random->RandomFloat( 0.05f, 0.1f );
	pTemp->SetLocalAnglesDim( Z_INDEX, random->RandomInt( 0, 360 ) );

	view->AddVisibleEntity( pTemp );

#else

	C_LocalTempEntity *pTemp = TempEntAlloc( origin, (model_t *) m_pSpriteMuzzleFlash[0] );
	
	if ( pTemp == NULL )
		return;

	//Setup our colors and type
	pTemp->m_nRenderMode	= kRenderTransAdd;
	pTemp->m_clrRender.r	= 255;
	pTemp->m_clrRender.g	= 255;
	pTemp->m_clrRender.b	= 255;
	pTemp->m_clrRender.a	= 255;
	pTemp->tempent_renderamt= 255;
	pTemp->m_nRenderFX		= kRenderFxNone;
	pTemp->flags			= FTENT_NONE;
	pTemp->die				= gpGlobals->curtime + 0.025f;
	pTemp->m_flFrame		= 0;
	pTemp->m_flFrameMax		= 0;
	
	pTemp->SetLocalOrigin( origin );
	pTemp->SetLocalAngles( vec3_angle );
	
	pTemp->SetVelocity( vec3_origin );
	
	//Scale and rotate
	pTemp->m_flSpriteScale		= random->RandomFloat( 0.1f, 0.15f );
	pTemp->SetAnglesDim( Z_INDEX, (360.0f/6.0f) * random->RandomInt( 0, 5 ) );

	view->AddVisibleEntity( pTemp );

	//
	// Smoke
	//

	Vector forward, start, end;

	AngleVectors( angles, &forward );

	start   = (Vector) origin - ( forward * 8 );
	end		= (Vector) origin + ( forward * random->RandomFloat( 48, 128 ) );

	unsigned int	flags = 0;

	if ( random->RandomInt( 0, 1 ) )
	{
		flags |= FXSTATICLINE_FLIP_HORIZONTAL;
	}

	const char *text = ( random->RandomInt( 0, 1 ) ) ? "sprites/ar2_muzzle3" : "sprites/ar2_muzzle4";

	FX_AddStaticLine( start, end, random->RandomFloat( 8.0f, 14.0f ), 0.01f, text, flags );

	// 
	// Dynamic light
	//

	/*
	if ( muzzleflash_light.GetInt() )
	{
		dlight_t *dl = effects->CL_AllocDlight( LIGHT_INDEX_MUZZLEFLASH + entityIndex );

		dl->origin	= origin;

		dl->color.r = 255;
		dl->color.g = 225;
		dl->color.b = 164;
		dl->color.exponent = 5;

		dl->radius	= 100;
		dl->decay	= dl->radius / 0.05f;
		dl->die		= gpGlobals->curtime + 0.05f;
	}
	*/

#endif
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_AR2_NPC( const Vector &origin, const QAngle &angles, int entityIndex )
{
	//Draw the cloud of fire
	FX_MuzzleEffect( origin, angles, 1.0f, entityIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//-----------------------------------------------------------------------------
void CTempEnts::MuzzleFlash_SMG1_NPC( const Vector &origin, const QAngle &angles, int entityIndex )
{
	//Draw the cloud of fire
	FX_MuzzleEffect( origin, angles, 0.75f, entityIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//-----------------------------------------------------------------------------
void CTempEnts::MuzzleFlash_SMG1_Player( const Vector &origin, const QAngle &angles, int entityIndex )
{
	float	scale = 1.0f;
	Vector	forward, offset;

	AngleVectors( angles, &forward );
	float flScale = random->RandomFloat( scale-0.25f, scale );

	if ( flScale < 0.5f )
	{
		flScale = 0.5f;
	}

	//
	// Flash
	//
	C_LocalTempEntity *pTemp;

	for ( int i = 1; i < 9; i++ )
	{
		offset = origin + (forward * (i*8.0f*flScale));

		pTemp = TempEntAllocHigh( offset, (model_t *) m_pSpriteAR2Flash[random->RandomInt(0,3)] );
		
		if ( pTemp == NULL )
			return;

		//Setup our colors and type
		pTemp->m_nRenderMode	= kRenderTransAdd;
		pTemp->SetRenderColor( 164, 164, 164+random->RandomInt(0,64), 255 );
		pTemp->tempent_renderamt= 255;
		pTemp->m_nRenderFX		= kRenderFxNone;
		pTemp->flags			= FTENT_PERSIST;
		pTemp->die				= gpGlobals->curtime + 0.025f;
		pTemp->m_flFrame		= 0;
		pTemp->m_flFrameMax		= 0;
		
		pTemp->SetLocalOrigin( offset );
		pTemp->SetLocalAngles( vec3_angle );
		
		pTemp->SetVelocity( vec3_origin );
		
		//Scale and rotate
		pTemp->m_flSpriteScale	= ( (random->RandomFloat( 6.0f, 8.0f ) * (12-(i))/9) * flScale ) / 32;
		pTemp->SetLocalAnglesDim( Z_INDEX, random->RandomInt( 0, 360 ) );

		view->AddVisibleEntity( pTemp );
	}

	//Flare
	pTemp = TempEntAlloc( origin, (model_t *) m_pSpriteMuzzleFlash[0] );
	
	if ( pTemp == NULL )
		return;

	//Setup our colors and type
	pTemp->m_nRenderMode	= kRenderTransAdd;
	pTemp->SetRenderColor( 255, 255, 255, 255 );
	pTemp->tempent_renderamt= 255;
	pTemp->m_nRenderFX		= kRenderFxNone;
	pTemp->flags			= FTENT_PERSIST;
	pTemp->die				= gpGlobals->curtime + 0.025f;
	pTemp->m_flFrame		= 0;
	pTemp->m_flFrameMax		= 0;
	
	pTemp->SetLocalOrigin( origin );
	pTemp->SetLocalAngles( vec3_angle );
	
	pTemp->SetVelocity( vec3_origin );
	
	//Scale and rotate
	pTemp->m_flSpriteScale	= random->RandomFloat( 0.05f, 0.1f );
	pTemp->SetLocalAnglesDim( Z_INDEX, random->RandomInt( 0, 360 ) );

	view->AddVisibleEntity( pTemp );
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_Shotgun_Player( const Vector& origin, const QAngle& angles, int entityIndex )
{
	C_LocalTempEntity	*pTemp = TempEntAlloc( origin, (model_t *) m_pSpriteMuzzleFlash[1] );
	
	if ( pTemp == NULL )
		return;

	//Setup our colors and type
	pTemp->m_nRenderMode		= kRenderTransAdd;
	pTemp->SetRenderColor( 255, 255, 255, 255 );
	pTemp->m_nRenderFX		= 0;

	//Setup other information
	pTemp->die				= gpGlobals->curtime + 0.01f;
	pTemp->m_flFrame		= 0;
	pTemp->m_flFrameMax		= 0;
	
	pTemp->SetLocalOrigin( origin );
	pTemp->SetLocalAngles( vec3_angle );

	//Scale and rotate
	pTemp->m_flSpriteScale = random->RandomFloat( 0.2f, 0.3f );
	pTemp->SetLocalAnglesDim( Z_INDEX, random->RandomInt( 0, 360 ) );

	view->AddVisibleEntity( pTemp );

#if 0

	pTemp = TempEntAlloc( origin, (model_t *) m_pSpriteMuzzleFlash[0] );
	
	if ( pTemp == NULL )
		return;

	//Setup our colors and type
	pTemp->m_nRenderMode	= kRenderTransAlpha;
	pTemp->m_clrRender.r	= 255;
	pTemp->m_clrRender.g	= 255;
	pTemp->m_clrRender.b	= 255;
	pTemp->tempent_renderamt= random->RandomInt( 128, 164 );
	pTemp->m_clrRender.a	= pTemp->tempent_renderamt;
	pTemp->m_nRenderFX		= kRenderFxNone;
	pTemp->flags			= FTENT_ROTATE | FTENT_SMOKEGROWANDFADE;
	pTemp->fadeSpeed		= 1.0f;
	pTemp->die				= gpGlobals->curtime + 0.1f;
	pTemp->tempent_angles[2]= random->RandomFloat( 0, 360 );

	//Setup other information
	pTemp->m_flFrame		= 0;
	pTemp->m_flFrameMax		= 0;
	
	VectorCopy( origin, pTemp->origin );
	VectorCopy( vec3_origin, pTemp->angles );

	//Scale and rotate
	pTemp->scale = random->RandomFloat( 0.05f, 0.08f );
	pTemp->angles[2] = random->RandomInt( 0, 360 );

	view->AddVisibleEntity( pTemp );

#endif
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_Shotgun_NPC( const Vector& origin, const QAngle& angles, int entityIndex )
{
	//Draw the cloud of fire
	FX_MuzzleEffect( origin, angles, 0.75f, entityIndex );

	Vector	forward;
	int		i;

	AngleVectors( angles, &forward );

	//Embers less often
	if ( random->RandomInt( 0, 2 ) == 0 )
	{
		//Embers
		CSmartPtr<CEmberEffect> pEmbers = CEmberEffect::Create( "muzzle_embers" );
		pEmbers->SetSortOrigin( origin );

		SimpleParticle	*pParticle;

		int	numEmbers = random->RandomInt( 0, 4 );

		for ( int i = 0; i < numEmbers; i++ )
		{
			pParticle = (SimpleParticle *) pEmbers->AddParticle( sizeof( SimpleParticle ), pEmbers->GetPMaterial( "effects/muzzleflash1" ), origin );
				
			if ( pParticle == NULL )
				return;

			pParticle->m_flLifetime		= 0.0f;
			pParticle->m_flDieTime		= random->RandomFloat( 0.2f, 0.4f );

			pParticle->m_vecVelocity.Random( -0.05f, 0.05f );
			pParticle->m_vecVelocity += forward;
			VectorNormalize( pParticle->m_vecVelocity );

			pParticle->m_vecVelocity	*= random->RandomFloat( 64.0f, 256.0f );

			pParticle->m_uchColor[0]	= 255;
			pParticle->m_uchColor[1]	= 128;
			pParticle->m_uchColor[2]	= 64;

			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;

			pParticle->m_uchStartSize	= 1;
			pParticle->m_uchEndSize		= 0;

			pParticle->m_flRoll			= 0;
			pParticle->m_flRollDelta	= 0;
		}
	}

	//
	// Trails
	//
	
	CSmartPtr<CTrailParticles> pTrails = CTrailParticles::Create( "MuzzleFlash_Shotgun_NPC" );
	pTrails->SetSortOrigin( origin );

	TrailParticle	*pTrailParticle;

	pTrails->SetFlag( bitsPARTICLE_TRAIL_FADE );
	pTrails->m_ParticleCollision.SetGravity( 0.0f );

	int	numEmbers = random->RandomInt( 4, 8 );

	for ( i = 0; i < numEmbers; i++ )
	{
		pTrailParticle = (TrailParticle *) pTrails->AddParticle( sizeof( TrailParticle ), pTrails->GetPMaterial( "effects/muzzleflash1" ), origin );
			
		if ( pTrailParticle == NULL )
			return;

		pTrailParticle->m_flLifetime		= 0.0f;
		pTrailParticle->m_flDieTime		= random->RandomFloat( 0.1f, 0.2f );

		float spread = 0.05f;

		pTrailParticle->m_vecVelocity.Random( -spread, spread );
		pTrailParticle->m_vecVelocity += forward;
		
		VectorNormalize( pTrailParticle->m_vecVelocity );
		VectorNormalize( forward );

		float dot = forward.Dot( pTrailParticle->m_vecVelocity );

		dot = (1.0f-fabs(dot)) / spread;
		pTrailParticle->m_vecVelocity *= (random->RandomFloat( 256.0f, 1024.0f ) * (1.0f-dot));

		pTrailParticle->m_flColor[0]	= 1.0f;
		pTrailParticle->m_flColor[1]	= 0.95f;
		pTrailParticle->m_flColor[2]	= 0.75f;
		
		pTrailParticle->m_flColor[3]	= 1.0f;

		pTrailParticle->m_flLength	= 0.05f;
		pTrailParticle->m_flWidth	= random->RandomFloat( 0.25f, 0.5f );
	}
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_Pistol_Player( const Vector& origin, const QAngle& angles, int entityIndex )
{
	C_LocalTempEntity	*pTemp = TempEntAlloc( origin, (model_t *) m_pSpriteMuzzleFlash[1] );
	
	if ( pTemp == NULL )
		return;

	//Setup our colors and type
	pTemp->m_nRenderMode	= kRenderTransAdd;
	pTemp->SetRenderColor( 255, 255, 255, 255 );
	pTemp->m_nRenderFX		= 0;

	//Setup other information
	pTemp->die			= gpGlobals->curtime + 0.01f;
	pTemp->m_flFrame	= 0;
	pTemp->m_flFrameMax	= 0;
	
	pTemp->SetLocalOrigin( origin );
	pTemp->SetLocalAngles( vec3_angle );

	//Scale and rotate
	pTemp->m_flSpriteScale = random->RandomFloat( 0.2f, 0.3f );
	pTemp->SetLocalAnglesDim( Z_INDEX, random->RandomInt( 0, 360 ) );

	view->AddVisibleEntity( pTemp );

	// Smoke

	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "MuzzleFlash_Pistol_Player" );
	pSimple->SetSortOrigin( origin );
	
	SimpleParticle *pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "particle/particle_noisesphere" ), origin );
		
	if ( pParticle == NULL )
		return;

	int	alpha = random->RandomInt( 64, 128 );

	pParticle->m_flLifetime		= 0.0f;
	pParticle->m_flDieTime		= 0.25f;

	pParticle->m_vecVelocity.Init();

	pParticle->m_uchColor[0]	= alpha;
	pParticle->m_uchColor[1]	= alpha;
	pParticle->m_uchColor[2]	= alpha;
	pParticle->m_uchStartAlpha	= alpha;
	pParticle->m_uchEndAlpha	= 0;
	pParticle->m_uchStartSize	= random->RandomInt( 4, 8 );;
	pParticle->m_uchEndSize		= pParticle->m_uchStartSize*2;
	pParticle->m_flRoll			= random->RandomInt( 0, 360 );
	pParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
	pParticle->m_vecVelocity[2] = 16;
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_Pistol_NPC( const Vector& origin, const QAngle &angles, int entityIndex )
{
	C_LocalTempEntity	*pTemp = TempEntAlloc( origin, (model_t *) m_pSpriteMuzzleFlash[1] );
	
	if ( pTemp == NULL )
		return;

	//Setup our colors and type
	pTemp->m_nRenderMode	= kRenderTransAdd;
	pTemp->SetRenderColor( 255, 255, 255, 255 );
	pTemp->m_nRenderFX		= 0;

	//Setup other information
	pTemp->die			= gpGlobals->curtime + 0.01f;
	pTemp->m_flFrame	= 0;
	pTemp->m_flFrameMax	= 0;
	
	pTemp->SetLocalOrigin( origin );
	pTemp->SetLocalAngles( vec3_angle );

	//Scale and rotate
	pTemp->m_flSpriteScale = random->RandomFloat( 0.1f, 0.2f );
	pTemp->SetLocalAnglesDim( Z_INDEX, random->RandomInt( 0, 360 ) );

	view->AddVisibleEntity( pTemp );
}


void CTempEnts::RocketFlare( const Vector& pos )
{
	C_LocalTempEntity	*pTemp;
	const model_t		*model;
	int					nframeCount;

	model = (model_t *)engine->LoadModel( "sprites/animglow01.vmt" );
	if ( !model )
	{
		return;
	}

	nframeCount = modelinfo->GetModelFrameCount( model );

	pTemp = TempEntAlloc( pos, (model_t *)model );
	if ( !pTemp )
		return;

	pTemp->m_flFrameMax = nframeCount - 1;
	pTemp->m_nRenderMode = kRenderGlow;
	pTemp->m_nRenderFX = kRenderFxNoDissipation;
	pTemp->tempent_renderamt = 255;
	pTemp->m_flFrameRate = 1.0;
	pTemp->m_flFrame = random->RandomInt( 0, nframeCount - 1);
	pTemp->m_flSpriteScale = 1.0;
	pTemp->SetAbsOrigin( pos );
	pTemp->die = gpGlobals->curtime + 0.01;
}
