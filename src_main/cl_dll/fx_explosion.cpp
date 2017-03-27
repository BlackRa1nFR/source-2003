//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Base explosion effect
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "fx_explosion.h"
#include "ClientEffectPrecacheSystem.h"
#include "FX_Sparks.h"
#include "dlight.h"
#include "tempentity.h"
#include "IEfx.h"
#include "engine/IEngineSound.h"
#include "engine/IVDebugOverlay.h"
#include "c_te_effect_dispatch.h"
#include "fx.h"

#define	__EXPLOSION_DEBUG	0

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectExplosion )
CLIENTEFFECT_MATERIAL( "effects/fire_cloud1" )
CLIENTEFFECT_MATERIAL( "effects/fire_cloud2" )
CLIENTEFFECT_MATERIAL( "effects/fire_embers1" )
CLIENTEFFECT_MATERIAL( "effects/fire_embers2" )
CLIENTEFFECT_MATERIAL( "effects/fire_embers3" )
CLIENTEFFECT_MATERIAL( "particle/particle_smokegrenade" )
CLIENTEFFECT_REGISTER_END()

//
// CExplosionParticle
//

class CExplosionParticle : public CSimpleEmitter
{
public:
	
	CExplosionParticle( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	//Create
	static CExplosionParticle *Create( const char *pDebugName )
	{
		return new CExplosionParticle( pDebugName );
	}

	//Roll
	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -8.0f );

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
		//pParticle->m_vecVelocity += pParticle->m_vecVelocity * ( timeDelta * -20.0f );
		static float dtime;
		static float decay;

		if ( dtime != timeDelta )
		{
			dtime = timeDelta;
			float expected = 0.5;
			decay = exp( log( 0.0001f ) * dtime / expected );
		}

		pParticle->m_vecVelocity = pParticle->m_vecVelocity * decay;


		//Cap the minimum speed
		if ( pParticle->m_vecVelocity.LengthSqr() < (32.0f*32.0f) )
		{
			VectorNormalize( saveVelocity );
			pParticle->m_vecVelocity = saveVelocity * 32.0f;
		}
	}

	//Alpha
	virtual float UpdateAlpha( SimpleParticle *pParticle, float timeDelta )
	{
		float	tLifetime = pParticle->m_flLifetime / pParticle->m_flDieTime;
		float	ramp = 1.0f - tLifetime;

		//Non-linear fade
		if ( ramp < 0.75f )
			ramp *= ramp;

		return ramp;
	}

	//Color
	virtual Vector UpdateColor( SimpleParticle *pParticle, float timeDelta )
	{
		Vector	color;

		float	tLifetime = pParticle->m_flLifetime / pParticle->m_flDieTime;
		float	ramp = 1.0f - tLifetime;

		color[0] = ( (float) pParticle->m_uchColor[0] * ramp ) / 255.0f;
		color[1] = ( (float) pParticle->m_uchColor[1] * ramp ) / 255.0f;
		color[2] = ( (float) pParticle->m_uchColor[2] * ramp ) / 255.0f;

		return color;
	}

private:
	CExplosionParticle( const CExplosionParticle & );
};

//Singleton static member definition
C_BaseExplosionEffect	C_BaseExplosionEffect::m_instance;

//Singleton accessor
C_BaseExplosionEffect &BaseExplosionEffect( void )
{ 
	return C_BaseExplosionEffect::Instance(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &deviant - 
//			&source - 
// Output : float
//-----------------------------------------------------------------------------
float C_BaseExplosionEffect::ScaleForceByDeviation( Vector &deviant, Vector &source, float spread, float *force )
{
	if ( ( deviant == vec3_origin ) || ( source == vec3_origin ) )
		return 1.0f;

	float	dot = source.Dot( deviant );
	
	dot = spread * fabs( dot );	

	if ( force != NULL )
	{
		(*force) *= dot;
	}

	return dot;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : position - 
//			force - 
// Output : virtual void
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::Create( const Vector &position, float force, float scale, int flags )
{
	m_vecOrigin = position;
	m_fFlags	= flags;

	//Find the force of the explosion
	GetForceDirection( m_vecOrigin, force, &m_vecDirection, &m_flForce );

#if __EXPLOSION_DEBUG
	debugoverlay->AddBoxOverlay( m_vecOrigin, -Vector(32,32,32), Vector(32,32,32), vec3_angle, 255, 0, 0, 64, 5.0f );
	debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin+(m_vecDirection*force*m_flForce), 0, 0, 255, false, 3 );
#endif

	PlaySound();

	if ( scale != 0 )
	{
		// UNDONE: Make core size parametric to scale or remove scale?
		CreateCore();
	}

	CreateDebris();
	//FIXME: CreateDynamicLight();
	CreateMisc();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::CreateCore( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOFIREBALL )
		return;

	Vector	offset;
	int		i;

	//Spread constricts as force rises
	float force = m_flForce;

	//Cap our force
	if ( force < EXPLOSION_FORCE_MIN )
		force = EXPLOSION_FORCE_MIN;
	
	if ( force > EXPLOSION_FORCE_MAX )
		force = EXPLOSION_FORCE_MAX;

	float spread = 1.0f - (0.15f*force);

	CSmartPtr<CExplosionParticle> pSimple = CExplosionParticle::Create( "exp_smoke" );
	pSimple->SetSortOrigin( m_vecOrigin );
	pSimple->SetNearClip( 32, 64 );

	SimpleParticle	*pParticle;

	//FIXME: Better sampling area
	offset = m_vecOrigin + ( m_vecDirection * 64.0f );

	//Find area ambient light color and use it to tint smoke
	Vector	worldLight = WorldGetLightForPoint( offset, true );

	//
	// Smoke
	//

	for ( i = 0; i < 4; i++ )
	{
		offset.Random( -32.0f, 32.0f );
		offset += m_vecOrigin;

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "particle/particle_smokegrenade" ), offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;

#ifdef TF2_CLIENT_DLL
			pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );
#else
			pParticle->m_flDieTime	= random->RandomFloat( 3.0f, 4.0f );
#endif

			pParticle->m_vecVelocity.Random( -spread, spread );
			pParticle->m_vecVelocity += ( m_vecDirection * random->RandomFloat( 1.0f, 6.0f ) );
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 1, 750 ) * force;

			//Scale the force down as we fall away from our main direction
			ScaleForceByDeviation( pParticle->m_vecVelocity, m_vecDirection, spread, &fForce );

			pParticle->m_vecVelocity *= fForce;
			
			#if __EXPLOSION_DEBUG
			debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
			#endif

			int nColor = random->RandomInt( 16, 32 );
			pParticle->m_uchColor[0] = nColor + ( worldLight[0] * nColor );
			pParticle->m_uchColor[1] = nColor + ( worldLight[1] * nColor );
			pParticle->m_uchColor[2] = nColor + ( worldLight[2] * nColor );
			
			pParticle->m_uchStartSize	= random->RandomInt( 32, 127 );
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2;
			
			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
		}
	}

	//
	// Inner core
	//

	for ( i = 0; i < 16; i++ )
	{
		offset.Random( -16.0f, 16.0f );
		offset += m_vecOrigin;

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "particle/particle_noisesphere" ), offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;

#ifdef TF2_CLIENT_DLL
			pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 1.0f );
#else
			pParticle->m_flDieTime	= random->RandomFloat( 0.5f, 3.0f );
#endif

			pParticle->m_vecVelocity.Random( -spread, spread );
			pParticle->m_vecVelocity += ( m_vecDirection * random->RandomFloat( 1.0f, 6.0f ) );
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 1, 2000 ) * force;

			//Scale the force down as we fall away from our main direction
			ScaleForceByDeviation( pParticle->m_vecVelocity, m_vecDirection, spread, &fForce );

			pParticle->m_vecVelocity *= fForce;
			
			#if __EXPLOSION_DEBUG
			debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
			#endif

			int nColor = random->RandomInt( 16, 64 );
			pParticle->m_uchColor[0] = nColor + ( worldLight[0] * nColor );
			pParticle->m_uchColor[1] = nColor + ( worldLight[1] * nColor );
			pParticle->m_uchColor[2] = nColor + ( worldLight[2] * nColor );
					
			pParticle->m_uchStartSize	= random->RandomInt( 16, 32 );
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4;

			//pParticle->m_uchStartAlpha	= random->RandomFloat( 32, 64 );
			pParticle->m_uchStartAlpha	= 255;//random->RandomFloat( 128, 255 );
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
		}
	}

	//
	// Ground ring
	//

	Vector	vRight, vUp;
	VectorVectors( m_vecDirection, vRight, vUp );

	vUp = CrossProduct( m_vecDirection, vRight );

	Vector	forward;

#ifndef TF2_CLIENT_DLL
	float	yaw;

	int	numRingSprites = 32;

	for ( i = 0; i < numRingSprites; i++ )
	{
		yaw = ( (float) i / (float) numRingSprites ) * 360.0f;
		forward = ( vRight * sin( DEG2RAD( yaw) ) ) + ( vUp * cos( DEG2RAD( yaw ) ) );
		VectorNormalize( forward );

		offset = ( RandomVector( -4.0f, 4.0f ) + m_vecOrigin ) + ( forward * random->RandomFloat( 8.0f, 16.0f ) );

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "particle/particle_noisesphere" ), offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 1.5f, 3.0f );

			pParticle->m_vecVelocity = forward;
		
			float	fForce = random->RandomFloat( 1, 1500 ) * force;

			//Scale the force down as we fall away from our main direction
			ScaleForceByDeviation( pParticle->m_vecVelocity, pParticle->m_vecVelocity, spread, &fForce );

			pParticle->m_vecVelocity *= fForce;
			
			#if __EXPLOSION_DEBUG
			debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
			#endif

			int nColor = random->RandomInt( 16, 64 );
			pParticle->m_uchColor[0] = nColor + ( worldLight[0] * nColor );
			pParticle->m_uchColor[1] = nColor + ( worldLight[1] * nColor );
			pParticle->m_uchColor[2] = nColor + ( worldLight[2] * nColor );

			pParticle->m_uchStartSize	= random->RandomInt( 16, 32 );
			pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4;

			pParticle->m_uchStartAlpha	= random->RandomFloat( 16, 32 );
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -8.0f, 8.0f );
		}
	}
#endif

	//
	// Embers
	//

	for ( i = 0; i < 64; i++ )
	{
		offset.Random( -48.0f, 48.0f );
		offset += m_vecOrigin;

		static char	text[64];
		Q_snprintf( text, sizeof( text ), "effects/fire_embers%d", random->RandomInt( 1, 2 ) );

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( text ), offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 1.0f, 1.5f );

			pParticle->m_vecVelocity.Random( -spread*2, spread*2 );
			pParticle->m_vecVelocity += m_vecDirection;
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 1.0f, 800.0f );

			//Scale the force down as we fall away from our main direction
			float	vDev = ScaleForceByDeviation( pParticle->m_vecVelocity, m_vecDirection, spread );

			pParticle->m_vecVelocity *= fForce * ( 16.0f * (vDev*vDev*0.5f) );
			
			#if __EXPLOSION_DEBUG
			debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
			#endif

			int nColor = random->RandomInt( 192, 255 );
			pParticle->m_uchColor[0]	= pParticle->m_uchColor[1] = pParticle->m_uchColor[2] = nColor;
			
			pParticle->m_uchStartSize	= random->RandomInt( 8, 16 ) * vDev;

			pParticle->m_uchStartSize	= clamp( pParticle->m_uchStartSize, 4, 32 );

			pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
			
			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -32.0f, 32.0f );
		}
	}

	//
	// Fireballs
	//

	for ( i = 0; i < 32; i++ )
	{
		offset.Random( -48.0f, 48.0f );
		offset += m_vecOrigin;

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "effects/fire_cloud2" ), offset );

		if ( pParticle != NULL )
		{
			pParticle->m_flLifetime = 0.0f;
			pParticle->m_flDieTime	= random->RandomFloat( 0.2f, 0.4f );

			pParticle->m_vecVelocity.Random( -spread*0.75f, spread*0.75f );
			pParticle->m_vecVelocity += m_vecDirection;
			
			VectorNormalize( pParticle->m_vecVelocity );

			float	fForce = random->RandomFloat( 400.0f, 800.0f );

			//Scale the force down as we fall away from our main direction
			float	vDev = ScaleForceByDeviation( pParticle->m_vecVelocity, m_vecDirection, spread );

			pParticle->m_vecVelocity *= fForce * ( 16.0f * (vDev*vDev*0.5f) );

			#if __EXPLOSION_DEBUG
			debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + pParticle->m_vecVelocity, 255, 0, 0, false, 3 );
			#endif

			int nColor = random->RandomInt( 128, 255 );
			pParticle->m_uchColor[0]	= pParticle->m_uchColor[1] = pParticle->m_uchColor[2] = nColor;
			
			pParticle->m_uchStartSize	= random->RandomInt( 32, 85 ) * vDev;

			pParticle->m_uchStartSize	= clamp( pParticle->m_uchStartSize, 32, 85 );

			pParticle->m_uchEndSize		= (int)((float)pParticle->m_uchStartSize * 1.5f);
			
			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;
			
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -16.0f, 16.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::CreateDebris( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOPARTICLES )
		return;

	//
	// Sparks
	//

	CSmartPtr<CTrailParticles> pSparkEmitter	= CTrailParticles::Create( "CreateDebris 1" );
	TrailParticle	*tParticle;

	if ( !pSparkEmitter )
	{
		assert(0);
		return;
	}

	PMaterialHandle hMaterial = pSparkEmitter->GetPMaterial( "effects/fire_cloud2" );

	pSparkEmitter->SetSortOrigin( m_vecOrigin );
	
	pSparkEmitter->m_ParticleCollision.SetGravity( 200.0f );
	pSparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	pSparkEmitter->SetVelocityDampen( 8.0f );
	
	int		numSparks = random->RandomInt( 16, 32 );
	Vector	dir;
	float	spread = 1.0f;

	// Dump out sparks
	int i;
	for ( i = 0; i < numSparks; i++ )
	{
		tParticle = (TrailParticle *) pSparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, m_vecOrigin );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;
		tParticle->m_flDieTime	= random->RandomFloat( 0.1f, 0.15f );

		dir.Random( -spread, spread );
		dir += m_vecDirection;
		VectorNormalize( dir );
		
		tParticle->m_flWidth		= random->RandomFloat( 2.0f, 16.0f );
		tParticle->m_flLength		= random->RandomFloat( 0.05f, 0.1f );
		
		tParticle->m_vecVelocity	= dir * random->RandomFloat( 1500, 2500 );
		
		//float color = random->RandomFloat( 0.5f, 1.0f );
		float color = 1.0f;
		tParticle->m_flColor[0]		= color;
		tParticle->m_flColor[1]		= color;
		tParticle->m_flColor[2]		= color;
		tParticle->m_flColor[3]		= color;
	}

	//
	// Chunks
	//

	Vector	offset;

	CSmartPtr<CFleckParticles> fleckEmitter = CFleckParticles::Create( "CreateDebris 2" );
	if ( !fleckEmitter )
		return;

	fleckEmitter->SetSortOrigin( m_vecOrigin );

	// Setup our collision information
	fleckEmitter->m_ParticleCollision.Setup( m_vecOrigin, &m_vecDirection, 0.9f, 512, 1024, 800, 0.5f );

	PMaterialHandle	hMaterialArray[2];
	
	hMaterialArray[0] = fleckEmitter->GetPMaterial( "effects/fleck_cement1" );
	hMaterialArray[1] = fleckEmitter->GetPMaterial( "effects/fleck_cement2" );

	int	numFlecks = random->RandomInt( 50, 100 );

	// Dump out flecks
	for ( i = 0; i < numFlecks; i++ )
	{
		offset = m_vecOrigin + ( m_vecDirection * 16.0f );

		offset[0] += random->RandomFloat( -8.0f, 8.0f );
		offset[1] += random->RandomFloat( -8.0f, 8.0f );
		offset[2] += random->RandomFloat( -8.0f, 8.0f );

		FleckParticle *pParticle = (FleckParticle *) fleckEmitter->AddParticle( sizeof(FleckParticle), hMaterialArray[random->RandomInt(0,1)], offset );

		if ( pParticle == NULL )
			break;

		pParticle->m_flLifetime	= 0.0f;
		pParticle->m_flDieTime	= 3.0f;
		
		dir[0] = m_vecDirection[0] + random->RandomFloat( -1.0f, 1.0f );
		dir[1] = m_vecDirection[1] + random->RandomFloat( -1.0f, 1.0f );
		dir[2] = m_vecDirection[2] + random->RandomFloat( -1.0f, 1.0f );

		pParticle->m_uchSize		= random->RandomInt( 1, 3 );

		VectorNormalize( dir );

		float	fForce = ( random->RandomFloat( 64, 256 ) * ( 4 - pParticle->m_uchSize ) );

		float	fDev = ScaleForceByDeviation( dir, m_vecDirection, 0.8f );

		pParticle->m_vecVelocity = dir * ( fForce * ( 16.0f * (fDev*fDev*0.5f) ) );

		pParticle->m_flRoll			= random->RandomFloat( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( 0, 360 );

		float colorRamp = random->RandomFloat( 0.5f, 1.5f );
		pParticle->m_uchColor[0] = min( 1.0f, 0.25f*colorRamp )*255.0f;
		pParticle->m_uchColor[1] = min( 1.0f, 0.25f*colorRamp )*255.0f;
		pParticle->m_uchColor[2] = min( 1.0f, 0.25f*colorRamp )*255.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::CreateMisc( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::CreateDynamicLight( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NODLIGHTS )
		return;

	dlight_t *dl = effects->CL_AllocDlight( 0 );
	
	VectorCopy (m_vecOrigin, dl->origin);
	
	dl->decay	= 200;
	dl->radius	= 255;
	dl->color.r = 255;
	dl->color.g = 220;
	dl->color.b = 128;
	dl->die		= gpGlobals->curtime + 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::PlaySound( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOSOUND )
		return;

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "BaseExplosionEffect.Sound", &m_vecOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			&m_vecDirection - 
//			strength - 
// Output : float
//-----------------------------------------------------------------------------
float C_BaseExplosionEffect::Probe( const Vector &origin, Vector *vecDirection, float strength )
{
	//Press out
	Vector endpos = origin + ( (*vecDirection) * strength );

	//Trace into the world
	trace_t	tr;
	UTIL_TraceLine( origin, endpos, CONTENTS_SOLID, NULL, COLLISION_GROUP_NONE, &tr );

	//Push back a proportional amount to the probe
	(*vecDirection) = -(*vecDirection) * (1.0f-tr.fraction);

#if __EXPLOSION_DEBUG
	debugoverlay->AddLineOverlay( m_vecOrigin, endpos, (255*(1.0f-tr.fraction)), (255*tr.fraction), 0, false, 3 );
#endif

	assert(( 1.0f - tr.fraction ) >= 0.0f );

	//Return the impacted proportion of the probe
	return (1.0f-tr.fraction);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			&m_vecDirection - 
//			&m_flForce - 
//-----------------------------------------------------------------------------
void C_BaseExplosionEffect::GetForceDirection( const Vector &origin, float magnitude, Vector *resultDirection, float *resultForce )
{
	Vector	d[6];

	//All cardinal directions
	d[0] = Vector(  1,  0,  0 );
	d[1] = Vector( -1,  0,  0 );
	d[2] = Vector(  0,  1,  0 );
	d[3] = Vector(  0, -1,  0 );
	d[4] = Vector(  0,  0,  1 );
	d[5] = Vector(  0,  0, -1 );

	//Init the results
	(*resultDirection).Init();
	(*resultForce) = 1.0f;
	
	//Get the aggregate force vector
	for ( int i = 0; i < 6; i++ )
	{
		(*resultForce) += Probe( origin, &d[i], magnitude );
		(*resultDirection) += d[i];
	}

	//If we've hit nothing, then point up
	if ( (*resultDirection) == vec3_origin )
	{
		(*resultDirection) = Vector( 0, 0, 1 );
		(*resultForce) = EXPLOSION_FORCE_MIN;
	}

	//Just return the direction
	VectorNormalize( (*resultDirection) );
}

//-----------------------------------------------------------------------------
// Purpose: Intercepts the water explosion dispatch effect
//-----------------------------------------------------------------------------
void ExplosionCallback( const CEffectData &data )
{
	BaseExplosionEffect().Create( data.m_vOrigin, data.m_flMagnitude, data.m_flScale, data.m_fFlags );
}

DECLARE_CLIENT_EFFECT( "Explosion", ExplosionCallback );


//===============================================================================================================
// Water Explosion
//===============================================================================================================
//Singleton static member definition
C_WaterExplosionEffect	C_WaterExplosionEffect::m_waterinstance;

//Singleton accessor
C_WaterExplosionEffect &WaterExplosionEffect( void )
{ 
	return C_WaterExplosionEffect::Instance(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WaterExplosionEffect::Create( const Vector &position, float force, float scale, int flags )
{
	m_vecOrigin = position;

	// Find our water surface by tracing up till we're out of the water
	trace_t tr;
	Vector vecTrace(0,0,1024);
	UTIL_TraceLine( m_vecOrigin, m_vecOrigin + vecTrace, MASK_WATER, NULL, COLLISION_GROUP_NONE, &tr );
	if ( tr.fractionleftsolid )
	{
		m_vecWaterSurface = m_vecOrigin + (vecTrace * tr.fractionleftsolid);
		debugoverlay->AddBoxOverlay( tr.endpos, -Vector(32,32,32), Vector(32,32,32), vec3_angle, 255, 0, 0, 64, 5.0f );
	}
	else
	{
		m_vecWaterSurface = m_vecOrigin;
	}

	BaseClass::Create( position, force, scale, flags );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WaterExplosionEffect::CreateCore( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOFIREBALL )
		return;

	FX_GunshotSplash( m_vecWaterSurface, Vector(0,0,1), m_flForce * 20 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WaterExplosionEffect::CreateDebris( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOPARTICLES )
		return;

	// ?
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WaterExplosionEffect::CreateMisc( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WaterExplosionEffect::PlaySound( void )
{
	if ( m_fFlags & TE_EXPLFLAG_NOSOUND )
		return;

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "Physics.WaterSplash", &m_vecOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: Intercepts the water explosion dispatch effect
//-----------------------------------------------------------------------------
void WaterSurfaceExplosionCallback( const CEffectData &data )
{
	WaterExplosionEffect().Create( data.m_vOrigin, data.m_flMagnitude, data.m_flScale, data.m_fFlags );
}

DECLARE_CLIENT_EFFECT( "WaterSurfaceExplosion", WaterSurfaceExplosionCallback );
