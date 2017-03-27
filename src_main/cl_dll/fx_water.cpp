//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "ClientEffectPrecacheSystem.h"
#include "FX_Sparks.h"
#include "iefx.h"
#include "c_te_effect_dispatch.h"
#include "particles_ez.h"
#include "decals.h"
#include "engine/IEngineSound.h"
#include "fx_quad.h"
#include "tier0/vprof.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectSplash )
CLIENTEFFECT_MATERIAL( "effects/splash1" )
CLIENTEFFECT_MATERIAL( "effects/splash2" )
CLIENTEFFECT_MATERIAL( "effects/splashwake1" )
CLIENTEFFECT_REGISTER_END()

#define	FDECAL_WATER		0x400	// Decal should only be applied to water

#define	SPLASH_MIN_SPEED	50.0f
#define	SPLASH_MAX_SPEED	100.0f

ConVar	cl_show_splashes( "cl_show_splashes", "1" );

class CSplashParticle : public CSimpleEmitter
{
public:
	
	CSplashParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	//Create
	static CSplashParticle *Create( const char *pDebugName )
	{
		return new CSplashParticle( pDebugName );
	}

	//Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -4.0f );

		//Cap the minimum roll
		if ( fabs( pParticle->m_flRollDelta ) < 0.5f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.5f : -0.5f;
		}

		return pParticle->m_flRoll;
	}

	//Velocity
	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		Vector	saveVelocity = pParticle->m_vecVelocity;

		//Decellerate
		static float dtime;
		static float decay;

		if ( dtime != timeDelta )
		{
			dtime = timeDelta;
			float expected = 3.0f;
			decay = exp( log( 0.0001f ) * dtime / expected );
		}

		pParticle->m_vecVelocity = pParticle->m_vecVelocity * decay;
		pParticle->m_vecVelocity[2] -= ( 600.0f * timeDelta );
	}

	//Alpha
	virtual float UpdateAlpha( SimpleParticle *pParticle, float timeDelta )
	{
		float	tLifetime = pParticle->m_flLifetime / pParticle->m_flDieTime;
		float	ramp = 1.0f - tLifetime;

		return ramp;
	}

private:
	CSplashParticle( const CSplashParticle & );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&normal - 
//			scale - 
//-----------------------------------------------------------------------------
void FX_WaterRipple( const Vector &origin, float scale )
{
	VPROF_BUDGET( "FX_WaterRipple", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	trace_t	tr;

	//FIXME: Pass in?
	Vector	color = Vector( 0.8f, 0.8f, 0.75f );

	Vector startPos = origin + Vector(0,0,8);
	Vector endPos = origin + Vector(0,0,-64);

	UTIL_TraceLine( startPos, endPos, MASK_WATER, NULL, COLLISION_GROUP_NONE, &tr );
	
	if ( tr.fraction < 1.0f )
	{
		//Add a ripple quad to the surface
		FX_AddQuad( tr.endpos + ( tr.plane.normal * 0.5f ), 
					tr.plane.normal, 
					16.0f*scale, 
					128.0f*scale, 
					0.7f,
					1.0f, 
					0.0f,
					0.25f,
					random->RandomFloat( 0, 360 ),
					random->RandomFloat( -16.0f, 16.0f ),
					color, 
					1.5f, 
					"effects/splashwake1", 
					(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&normal - 
//-----------------------------------------------------------------------------
void FX_GunshotSplash( const Vector &origin, const Vector &normal, float scale )
{
	VPROF_BUDGET( "FX_GunshotSplash", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	Vector	offset;
	float	spread	= 0.2f;
	
	if ( cl_show_splashes.GetBool() == false )
		return;

	CSmartPtr<CSplashParticle> pSimple = CSplashParticle::Create( "splish" );
	pSimple->SetSortOrigin( origin );

	SimpleParticle	*pParticle;

	//FIXME: Blood color! 
	//Vector	color = Vector( 0.25f, 0.0f, 0.0f );

	Vector	color = Vector( 0.8f, 0.8f, 0.75f );
	float	colorRamp;

	int i;
	float	flScale = scale / 8.0f;

	PMaterialHandle	hMaterial = g_ParticleMgr.GetPMaterial( "effects/splash2" );

#if 1

	int		numDrops = 32;
	float	length = 0.1f;
	Vector	vForward, vRight, vUp;
	Vector	offDir;

	TrailParticle	*tParticle;

	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "splash" );

	if ( !sparkEmitter )
		return;

	sparkEmitter->SetSortOrigin( origin );
	sparkEmitter->m_ParticleCollision.SetGravity( 800.0f );
	sparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	sparkEmitter->SetVelocityDampen( 2.0f );

	//Dump out drops
	for ( i = 0; i < numDrops; i++ )
	{
		offset = origin;
		offset[0] += random->RandomFloat( -16.0f, 16.0f ) * flScale;
		offset[1] += random->RandomFloat( -16.0f, 16.0f ) * flScale;

		tParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;
		tParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );

		offDir = normal + RandomVector( -1.0f, 1.0f );

		tParticle->m_vecVelocity = offDir * random->RandomFloat( SPLASH_MIN_SPEED * flScale * 2.0f, SPLASH_MAX_SPEED * flScale * 2.0f );
		tParticle->m_vecVelocity[2] += random->RandomFloat( 32.0f, 128.0f ) * flScale;

		tParticle->m_flWidth		= random->RandomFloat( 1.0f, 3.0f ) * flScale;
		tParticle->m_flLength		= random->RandomFloat( length*0.25f, length ) * flScale;

		colorRamp = random->RandomFloat( 1.5f, 2.0f );

		tParticle->m_flColor[0]	= min( 1.0f, color[0] * colorRamp );
		tParticle->m_flColor[1]	= min( 1.0f, color[1] * colorRamp );
		tParticle->m_flColor[2]	= min( 1.0f, color[2] * colorRamp );
		tParticle->m_flColor[3]	= 1.0f;
	}

	//Dump out drops
	for ( i = 0; i < 8; i++ )
	{
		offset = origin;
		offset[0] += random->RandomFloat( -16.0f, 16.0f ) * flScale;
		offset[1] += random->RandomFloat( -16.0f, 16.0f ) * flScale;

		tParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;
		tParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );

		offDir = normal + RandomVector( -0.2f, 0.2f );

		tParticle->m_vecVelocity = offDir * random->RandomFloat( SPLASH_MIN_SPEED * flScale * 3.0f, SPLASH_MAX_SPEED * flScale * 3.0f );
		tParticle->m_vecVelocity[2] += random->RandomFloat( 32.0f, 128.0f ) * flScale;

		tParticle->m_flWidth		= random->RandomFloat( 2.0f, 3.0f ) * flScale;
		tParticle->m_flLength		= random->RandomFloat( length*0.25f, length ) * flScale;

		colorRamp = random->RandomFloat( 1.5f, 2.0f );

		tParticle->m_flColor[0]	= min( 1.0f, color[0] * colorRamp );
		tParticle->m_flColor[1]	= min( 1.0f, color[1] * colorRamp );
		tParticle->m_flColor[2]	= min( 1.0f, color[2] * colorRamp );
		tParticle->m_flColor[3]	= 1.0f;
	}

#endif

#if 1

	//Main gout
	for ( i = 0; i < 8; i++ )
	{
		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, origin );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= 0.75f * flScale;

			pParticle->m_vecVelocity.Random( -spread, spread );
			pParticle->m_vecVelocity += ( normal * random->RandomFloat( 1.0f, 6.0f ) );
			
			VectorNormalize( pParticle->m_vecVelocity );

			pParticle->m_vecVelocity *= 50 * flScale * i;
			
			colorRamp = random->RandomFloat( 0.75f, 1.25f );

			pParticle->m_uchColor[0]	= min( 1.0f, color[0] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[1]	= min( 1.0f, color[1] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[2]	= min( 1.0f, color[2] * colorRamp ) * 255.0f;
			
			pParticle->m_uchStartSize	= 2 * flScale * (i+1);
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2;
			
			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
		}
	}

#endif 

	//Impact hit
	for ( i = 0; i < 4; i++ )
	{
		offset = origin;
		offset[0] += random->RandomFloat( -8.0f, 8.0f ) * flScale;
		offset[1] += random->RandomFloat( -8.0f, 8.0f ) * flScale;

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= 0.75f * flScale;

			spread = 1.0f;

			pParticle->m_vecVelocity.Random( -spread, spread );
			pParticle->m_vecVelocity += normal;
			
			VectorNormalize( pParticle->m_vecVelocity );

			pParticle->m_vecVelocity *= random->RandomFloat( 16, 64 ) * flScale;
			pParticle->m_vecVelocity[2] += 150.0f * flScale;
			
			colorRamp = random->RandomFloat( 0.75f, 1.25f );

			pParticle->m_uchColor[0]	= min( 1.0f, color[0] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[1]	= min( 1.0f, color[1] * colorRamp ) * 255.0f;
			pParticle->m_uchColor[2]	= min( 1.0f, color[2] * colorRamp ) * 255.0f;
			
			pParticle->m_uchStartSize	= 16 * flScale;
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2;
			
			pParticle->m_uchStartAlpha	= random->RandomInt( 32, 64 );
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
		}
	}			

	FX_WaterRipple( origin, flScale );

	//Play a sound
	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, CHAN_VOICE, "Physics.WaterSplash", 1.0, ATTN_NORM, 0, 100, &origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SplashCallback( const CEffectData &data )
{
	Vector	normal;

	AngleVectors( data.m_vAngles, &normal );

	FX_GunshotSplash( data.m_vOrigin, Vector(0,0,1), data.m_flScale );
}

DECLARE_CLIENT_EFFECT( "watersplash", SplashCallback );


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void GunshotSplashCallback( const CEffectData &data )
{
	FX_GunshotSplash( data.m_vOrigin, data.m_vNormal, data.m_flScale );
}

DECLARE_CLIENT_EFFECT( "gunshotsplash", GunshotSplashCallback );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void RippleCallback( const CEffectData &data )
{
	float	flScale = data.m_flScale / 8.0f;

	FX_WaterRipple( data.m_vOrigin, flScale );
}

DECLARE_CLIENT_EFFECT( "waterripple", RippleCallback );
