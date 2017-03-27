//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Contains the client-side part of the precipitation class
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_tracer.h"
#include "c_clientstats.h"
#include "view.h"
#include "initializer.h"
#include "particles_simple.h"
#include "env_wind_shared.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"

static ConVar	cl_winddir			( "cl_winddir", "0", 0, "Weather effects wind direction angle" );
static ConVar	cl_windspeed		( "cl_windspeed", "0", 0, "Weather effects wind speed scalar" );

//-----------------------------------------------------------------------------
// Precipitation particle type
//-----------------------------------------------------------------------------

struct PrecipitationParticle_t : public Particle
{
	Vector	m_Velocity;
	float	m_SpawnTime;				// Note: Tweak with this to change lifetime
	float	m_Mass;
	float	m_Ramp;
};
						  

//-----------------------------------------------------------------------------
// Precipitation base entity
//-----------------------------------------------------------------------------

class CClient_Precipitation : public C_BaseEntity, public IParticleEffect
{
class CPrecipitationEffect;
friend class CClient_Precipitation::CPrecipitationEffect;

public:
	DECLARE_CLASS( CClient_Precipitation, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	
	CClient_Precipitation();
	virtual ~CClient_Precipitation();

	// Initializes all the static members..
	static void InitSharedData();

	// Inherited from C_BaseEntity
	void Precache( );

	virtual void	Update( float fTimeDelta );
	virtual bool	SimulateAndRender( Particle *pParticle, ParticleDraw* pParticleDraw, float &sortDist );
	virtual void	GetSortOrigin( Vector &vSortOrigin );

private:
	
	// Types of precipitation
	enum Type_t
	{
		NOT_INITIALIZED = -1,
		SNOW = 0,
		RAIN,
		NUM_TYPES
	};

	CParticleEffectBinding m_ParticleEffect;

	// Creates a single particle
	PrecipitationParticle_t* CreateParticle();

	// Renders the particle
	void RenderParticle( ParticleDraw* pDraw, PrecipitationParticle_t* pParticle, float &sortKey );

	// Emits the actual particles
	void EmitParticles( float fTimeDelta );
	
	// Computes where we're gonna emit
	bool ComputeEmissionArea( Vector& origin, Vector2D& size );

	// Gets the tracer width and speed
	float GetWidth() const;
	float GetLength() const;
	float GetSpeed() const;

	// Gets the remaining lifetime of the particle
	float GetRemainingLifetime( PrecipitationParticle_t* pParticle ) const;

	// Computes the wind vector
	static void ComputeWindVector( );

	// simulation methods
	bool SimulateRain( PrecipitationParticle_t* pParticle );
	bool SimulateSnow( PrecipitationParticle_t* pParticle );

	// Information helpful in creating and rendering particles
	PMaterialHandle	m_MatHandle;	// material used 
	float			m_Color[4];		// precip color
	float			m_Lifetime;		// Precip lifetime
	float			m_InitialRamp;	// Initial ramp value
	float			m_Speed;		// Precip speed
	float			m_Width;		// Tracer width
	float			m_Remainder;	// particles we should render next time
	Type_t			m_Type;			// Precip type

	// Some state used in rendering and simulation
	// Used to modify the rain density and wind from the console
	static ConVar s_raindensity;
	static ConVar s_rainwidth;
	static ConVar s_rainlength;
	static ConVar s_rainspeed;

	static double s_FrameDeltaTime;		// stores the frame dt for simulation
	static Vector s_WindVector;			// Stores the wind speed vector
	static double s_CurrFrameTime;

private:
	CClient_Precipitation( const CClient_Precipitation & ); // not defined, not accessible
};

// Just receive the normal data table stuff
IMPLEMENT_CLIENTCLASS_DT(CClient_Precipitation, DT_Precipitation, CPrecipitation)
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Causes InitSharedData to be called at level init
//-----------------------------------------------------------------------------

void InitPrecipitationData()
{
	CClient_Precipitation::InitSharedData();
}

REGISTER_FUNCTION_INITIALIZER( InitPrecipitationData );


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

ConVar CClient_Precipitation::s_raindensity( "r_raindensity","0.003");
ConVar CClient_Precipitation::s_rainwidth( "r_rainwidth", "0.35f" );
ConVar CClient_Precipitation::s_rainlength( "r_rainlength", "0.075f" );
ConVar CClient_Precipitation::s_rainspeed( "r_rainspeed", "600.0f" );

double CClient_Precipitation::s_FrameDeltaTime;	// stores the frame dt for simulation
Vector CClient_Precipitation::s_WindVector;		// Stores the wind speed vector
double CClient_Precipitation::s_CurrFrameTime = -1;


bool CClient_Precipitation::SimulateAndRender( Particle *pParticle, ParticleDraw* pParticleDraw, float &sortDist )
{
	MEASURE_TIMED_STAT(CS_PRECIPITATION_TIME);

	// Call the appropriate sim and render method
	PrecipitationParticle_t* pPrecip = (PrecipitationParticle_t*)pParticle;
	RenderParticle( pParticleDraw, pPrecip, sortDist );
	if( m_Type == RAIN )
		return SimulateRain( pPrecip );
	else
		return SimulateSnow( pPrecip );
}


void CClient_Precipitation::GetSortOrigin( Vector &vSortOrigin )
{
	vSortOrigin = GetAbsOrigin();
}


//-----------------------------------------------------------------------------
// Initializes all the static members..
//-----------------------------------------------------------------------------

void CClient_Precipitation::InitSharedData()
{
}


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------

CClient_Precipitation::CClient_Precipitation() : m_Type(NOT_INITIALIZED), m_Remainder(0.0f)
{
	g_ParticleMgr.AddEffect( &m_ParticleEffect, this );
}

CClient_Precipitation::~CClient_Precipitation()
{
	g_ParticleMgr.RemoveEffect( &m_ParticleEffect );
}

//-----------------------------------------------------------------------------
// Precache data
//-----------------------------------------------------------------------------

#define SNOW_SPEED	80.0f
#define RAIN_SPEED	425.0f

#define RAIN_TRACER_WIDTH 0.35f
#define SNOW_TRACER_WIDTH 0.7f

void CClient_Precipitation::Precache( )
{
	if (m_Type == NOT_INITIALIZED)
	{
		// Compute precipitation emission speed
		switch( m_nRenderFX )
		{
		case kRenderFxEnvSnow:
			m_Speed	= SNOW_SPEED;
			m_MatHandle = m_ParticleEffect.FindOrAddMaterial( "particle/snow" );
			m_Type = SNOW;
			m_InitialRamp = 0.6f;
			m_Width = SNOW_TRACER_WIDTH;
			break;

		case kRenderFxEnvRain:
			m_Speed	= RAIN_SPEED;
			m_MatHandle = m_ParticleEffect.FindOrAddMaterial( "particle/rain" );
			m_Type = RAIN;
			m_InitialRamp = 1.0f;
			m_Color[3] = 1.0f;	// make translucent
			m_Width = RAIN_TRACER_WIDTH;
			break;
		}

		// calculate the max amount of time it will take this flake to fall.
		// This works if we assume the wind doesn't have a z component

		m_Lifetime = (WorldAlignMaxs()[2] - WorldAlignMins()[2]) / m_Speed;

		// Store off the color
		m_Color[0] = 1.0f;
		m_Color[1] = 1.0f;
		m_Color[2] = 1.0f;
	}
}


//-----------------------------------------------------------------------------
// Gets the tracer width and speed
//-----------------------------------------------------------------------------

inline float CClient_Precipitation::GetWidth() const
{
//	return m_Width;
	return s_rainwidth.GetFloat();
}

inline float CClient_Precipitation::GetLength() const
{
//	return m_Length;
	return s_rainlength.GetFloat();
}

inline float CClient_Precipitation::GetSpeed() const
{
//	return m_Speed;
	return s_rainspeed.GetFloat();
}


//-----------------------------------------------------------------------------
// Gets the remaining lifetime of the particle
//-----------------------------------------------------------------------------

inline float CClient_Precipitation::GetRemainingLifetime( PrecipitationParticle_t* pParticle ) const
{
	float timeSinceSpawn = gpGlobals->curtime - pParticle->m_SpawnTime;
	return m_Lifetime - timeSinceSpawn;
}

//-----------------------------------------------------------------------------
// Creates a particle
//-----------------------------------------------------------------------------

PrecipitationParticle_t* CClient_Precipitation::CreateParticle( )
{
	PrecipitationParticle_t* pParticle = 
		(PrecipitationParticle_t*)m_ParticleEffect.AddParticle( sizeof(PrecipitationParticle_t), m_MatHandle );

	if (pParticle)
	{
		pParticle->m_SpawnTime = gpGlobals->curtime;
		pParticle->m_Ramp = m_InitialRamp;
	}

	return pParticle;
}


//-----------------------------------------------------------------------------
// Compute the emission area
//-----------------------------------------------------------------------------

bool CClient_Precipitation::ComputeEmissionArea( Vector& origin, Vector2D& size )
{
	// FIXME: Compute the precipitation area based on computational power
	float emissionSize = 1000.0f;	// size of box to emit particles in

	// calculate a volume around the player to snow in. Intersect this big magic
	// box around the player with the volume of the current environmental ent.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	// Determine how much time it'll take a falling particle to hit the player
	float emissionHeight = min( WorldAlignMaxs()[2], pPlayer->GetAbsOrigin()[2] + 512 );
	float distToFall = emissionHeight - pPlayer->GetAbsOrigin()[2];
	float fallTime = distToFall / GetSpeed();
	
	// Based on the windspeed, figure out the center point of the emission
	Vector2D center;
	center[0] = pPlayer->GetAbsOrigin()[0] - fallTime * s_WindVector[0];
	center[1] = pPlayer->GetAbsOrigin()[1] - fallTime * s_WindVector[1];

	Vector2D lobound, hibound;
	lobound[0] = center[0] - emissionSize * 0.5f;
	lobound[1] = center[1] - emissionSize * 0.5f;
	hibound[0] = lobound[0] + emissionSize;
	hibound[1] = lobound[1] + emissionSize;

	// Cull non-intersecting.
	if ( ( WorldAlignMaxs()[0] < lobound[0] ) || ( WorldAlignMaxs()[1] < lobound[1] ) ||
		 ( WorldAlignMins()[0] > hibound[0] ) || ( WorldAlignMins()[1] > hibound[1] ) )
		return false;

	origin[0] = max( WorldAlignMins()[0], lobound[0] );
	origin[1] = max( WorldAlignMins()[1], lobound[1] );
	origin[2] = emissionHeight;

	hibound[0] = min( WorldAlignMaxs()[0], hibound[0] );
	hibound[1] = min( WorldAlignMaxs()[1], hibound[1] );

	size[0] = hibound[0] - origin[0];
	size[1] = hibound[1] - origin[1];

	return true;
}


//-----------------------------------------------------------------------------
// emit the precipitation particles
//-----------------------------------------------------------------------------

void CClient_Precipitation::EmitParticles( float fTimeDelta )
{
	Vector2D size;
	Vector vel, org;

	// Compute where to emit
	if (!ComputeEmissionArea( org, size ))
		return;

	// clamp this to prevent creating a bunch of rain or snow at one time.
	if( fTimeDelta > 0.075f )
		fTimeDelta = 0.075f;

	// FIXME: Compute the precipitation density based on computational power
	float density = s_raindensity.GetFloat();
	if (density > 0.01f) 
		density = 0.01f;

	// Compute number of particles to emit based on precip density and emission area and dt
	float fParticles = size[0] * size[1] * density * fTimeDelta + m_Remainder; 
	int cParticles = (int)fParticles;
	m_Remainder = fParticles - cParticles;

#if 0
	Msg("Interval: %f (%d) \n", fTimeDelta, cParticles );
#endif

	// calculate the max amount of time it will take this flake to fall.
	// This works if we assume the wind doesn't have a z component
	VectorCopy( s_WindVector, vel );
	vel[2] -= GetSpeed();

	// Emit all the particles
	for ( int i = 0 ; i < cParticles ; i++ )
	{									 
		// Create the particle
		PrecipitationParticle_t* p = CreateParticle();
		if (!p) 
			return;

		VectorCopy( org, p->m_Pos );
		VectorCopy( vel, p->m_Velocity );

		p->m_Pos[ 0 ] += size[ 0 ] * random->RandomFloat(0, 1);
		p->m_Pos[ 1 ] += size[ 1 ] * random->RandomFloat(0, 1);

		p->m_Velocity[ 0 ] += random->RandomFloat(-100, 100);
		p->m_Velocity[ 1 ] += random->RandomFloat(-100, 100);

		p->m_Mass = random->RandomFloat( 0.5, 1.5 );
	}
}


//-----------------------------------------------------------------------------
// Computes the wind vector
//-----------------------------------------------------------------------------

void CClient_Precipitation::ComputeWindVector( )
{
	// Compute the wind direction
	QAngle windangle;	// used to turn wind yaw direction into a vector

	// turn wind dir and speed values into a vector representing the wind.
	windangle[1] = cl_winddir.GetFloat();

	// Randomize the wind angle and speed slightly to get us a little variation
	windangle[1] = windangle[1] + random->RandomFloat( -10, 10 );
	float windspeed = cl_windspeed.GetFloat() * (1.0 + random->RandomFloat( -0.2, 0.2 ));

	AngleVectors( windangle, &s_WindVector );
	VectorScale( s_WindVector, windspeed, s_WindVector );
}


//-----------------------------------------------------------------------------
// Do per-frame setup
//-----------------------------------------------------------------------------

void CClient_Precipitation::Update( float fTimeDelta )
{
	// NOTE: When client-side prechaching works, we need to remove this
	Precache();

	// Our sim methods needs dt	and wind vector
	if (s_CurrFrameTime != gpGlobals->curtime)
	{
		s_FrameDeltaTime = fTimeDelta;
		ComputeWindVector( );
		s_CurrFrameTime = gpGlobals->curtime; 
	}

	// Emit da particles
	EmitParticles( fTimeDelta );
}



//-----------------------------------------------------------------------------
// tracer rendering
//-----------------------------------------------------------------------------

void CClient_Precipitation::RenderParticle( ParticleDraw* pDraw,
							PrecipitationParticle_t* pParticle, float &sortKey )
{
	float scale;
	Vector start, delta;

	assert( pParticle );

	ClientStats().BeginTimedStat( CS_PRECIPITATION_SETUP );

	// make streaks 0.1 seconds long, but prevent from going past end
	float lifetimeRemaining = GetRemainingLifetime( pParticle );
	if (lifetimeRemaining >= GetLength())
		scale = GetLength() * pParticle->m_Ramp;
	else
		scale = lifetimeRemaining * pParticle->m_Ramp;
	
	// NOTE: We need to do everything in screen space
	TransformParticle(g_ParticleMgr.GetModelView(), pParticle->m_Pos, start);
	sortKey = start.z;

	Vector3DMultiply( CurrentWorldToViewMatrix(), pParticle->m_Velocity, delta );

	// give a spiraling pattern to snow particles
	if ( m_Type == SNOW )
	{
		Vector spiral, camSpiral;
		float s, c;

		if ( pParticle->m_Mass > 1.0f )
		{
			SinCos( gpGlobals->curtime * M_PI * (1+pParticle->m_Mass * 0.1f) + 
					pParticle->m_Mass * 5.0f, &s , &c );

			// only spiral particles with a mass > 1, so some fall straight down
			spiral[0] = 28 * c;
			spiral[1] = 28 * s;
			spiral[2] = 0.0f;

			Vector3DMultiply( CurrentWorldToViewMatrix(), spiral, camSpiral );

			// X and Y are measured in world space; need to convert to camera space
			VectorAdd( start, camSpiral, start );
			VectorAdd( delta, camSpiral, delta );
		}

		// shrink the trails on spiraling flakes.
		pParticle->m_Ramp = 0.3f;
	}

	delta[0] *= scale;
	delta[1] *= scale;
	delta[2] *= scale;

	ClientStats().EndTimedStat( CS_PRECIPITATION_SETUP );

	// See c_tracer.* for this method
	if( pDraw->GetMeshBuilder() )
		Tracer_Draw( pDraw, start, delta, GetWidth(), 0 );
}


//-----------------------------------------------------------------------------
// determines if a weather particle has hit something other than air
//-----------------------------------------------------------------------------
static bool IsInAir( const Vector& position )
{
	int contents = enginetrace->GetPointContents( position ); 	
	return (contents & CONTENTS_SOLID) == 0;
}


//-----------------------------------------------------------------------------
//
// Utility methods for the various simulation functions
//
//-----------------------------------------------------------------------------

bool CClient_Precipitation::SimulateRain( PrecipitationParticle_t* pParticle )
{
	MEASURE_TIMED_STAT(CS_PRECIPITATION_PHYSICS);

	if (GetRemainingLifetime( pParticle ) < 0.0f)
		return false;

	// Update position
	VectorMA( pParticle->m_Pos, s_FrameDeltaTime, pParticle->m_Velocity, 
				pParticle->m_Pos );

	// wind blows rain around
	for ( int i = 0 ; i < 2 ; i++ )
	{
		if ( pParticle->m_Velocity[i] < s_WindVector[i] )
		{
			pParticle->m_Velocity[i] += ( 5 / pParticle->m_Mass );

			// clamp
			if ( pParticle->m_Velocity[i] > s_WindVector[i] )
				pParticle->m_Velocity[i] = s_WindVector[i];
		}
		else if (pParticle->m_Velocity[i] > s_WindVector[i] )
		{
			pParticle->m_Velocity[i] -= ( 5 / pParticle->m_Mass );

			// clamp.
			if ( pParticle->m_Velocity[i] < s_WindVector[i] )
				pParticle->m_Velocity[i] = s_WindVector[i];
		}
	}

	// No longer in the air? punt.
	if ( !IsInAir( pParticle->m_Pos ) )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pPlayer )
			return false;

		float x = pPlayer->GetAbsOrigin()[0] - pParticle->m_Pos[0];
		float y = pPlayer->GetAbsOrigin()[1] - pParticle->m_Pos[1];
		float impactdist = fabs(x) + fabs(y);

		// Drops that fall quite close have a chance of making a splash sound.
		if( impactdist <= 128.0f )
		{
			if( random->RandomInt( 0, 10 ) < 2 )
			{
//				EmitSound( pParticle->m_Pos );
			}
		}

		// Tell the framework it's time to remove the particle from the list
		return false;
	}

	// We still want this particle
	return true;
}


bool CClient_Precipitation::SimulateSnow( PrecipitationParticle_t* pParticle )
{
	if ( IsInAir( pParticle->m_Pos ) )
	{
		// Update position
		VectorMA( pParticle->m_Pos, s_FrameDeltaTime, pParticle->m_Velocity, 
					pParticle->m_Pos );

		// wind blows rain around
		for ( int i = 0 ; i < 2 ; i++ )
		{
			if ( pParticle->m_Velocity[i] < s_WindVector[i] )
			{
				pParticle->m_Velocity[i] += ( 5.0f / pParticle->m_Mass );

				// accelerating flakes get a trail
				pParticle->m_Ramp = 0.5f;

				// clamp
				if ( pParticle->m_Velocity[i] > s_WindVector[i] )
					pParticle->m_Velocity[i] = s_WindVector[i];
			}
			else if (pParticle->m_Velocity[i] > s_WindVector[i] )
			{
				pParticle->m_Velocity[i] -= ( 5.0f / pParticle->m_Mass );

				// accelerating flakes get a trail
				pParticle->m_Ramp = 0.5f;

				// clamp.
				if ( pParticle->m_Velocity[i] < s_WindVector[i] )
					pParticle->m_Velocity[i] = s_WindVector[i];
			}
		}

		return true;
	}


	// Kill the particle immediately!
	return false;
}


//-----------------------------------------------------------------------------
// EnvWind - global wind info
//-----------------------------------------------------------------------------
class C_EnvWind : public C_BaseEntity
{
public:
	C_EnvWind();

	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_EnvWind, C_BaseEntity );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void ) { return false; }

	virtual void	ClientThink( );

private:
	C_EnvWind( const C_EnvWind & );

	CEnvWindShared m_EnvWindShared;
};

// Receive datatables
BEGIN_RECV_TABLE_NOBASE(CEnvWindShared, DT_EnvWindShared)
	RecvPropInt		(RECVINFO(m_iMinWind)),
	RecvPropInt		(RECVINFO(m_iMaxWind)),
	RecvPropInt		(RECVINFO(m_iMinGust)),
	RecvPropInt		(RECVINFO(m_iMaxGust)),
	RecvPropFloat	(RECVINFO(m_flMinGustDelay)),
	RecvPropFloat	(RECVINFO(m_flMaxGustDelay)),
	RecvPropInt		(RECVINFO(m_iGustDirChange)),
	RecvPropInt		(RECVINFO(m_iWindSeed)),
	RecvPropInt		(RECVINFO(m_iInitialWindDir)),
	RecvPropFloat	(RECVINFO(m_flInitialWindSpeed)),
	RecvPropFloat	(RECVINFO(m_flStartTime)),
//	RecvPropInt		(RECVINFO(m_iszGustSound)),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_EnvWind, DT_EnvWind, CEnvWind )
	RecvPropDataTable(RECVINFO_DT(m_EnvWindShared), 0, &REFERENCE_RECV_TABLE(DT_EnvWindShared)),
END_RECV_TABLE()


C_EnvWind::C_EnvWind()
{
}

//-----------------------------------------------------------------------------
// Post data update!
//-----------------------------------------------------------------------------
void C_EnvWind::OnDataChanged( DataUpdateType_t updateType )
{
	// Whenever we get an update, reset the entire state.
	// Note that the fields have already been stored by the datatables,
	// but there's still work to be done in the init block
	m_EnvWindShared.Init( entindex(), m_EnvWindShared.m_iWindSeed, 
		m_EnvWindShared.m_flStartTime, m_EnvWindShared.m_iInitialWindDir,
		m_EnvWindShared.m_flInitialWindSpeed );

	SetNextClientThink(0.0f);
}

void C_EnvWind::ClientThink( )
{
	// Update the wind speed
	float flNextThink = m_EnvWindShared.WindThink( gpGlobals->curtime );
	SetNextClientThink(flNextThink);
}



//==================================================
// EmberParticle
//==================================================

class CEmberEmitter : public CSimpleEmitter
{
public:
							CEmberEmitter( const char *pDebugName );
	static CSmartPtr<CEmberEmitter>	Create( const char *pDebugName );
	virtual void			UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	virtual Vector			UpdateColor( SimpleParticle *pParticle, float timeDelta );

private:
							CEmberEmitter( const CEmberEmitter & );
};


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
CEmberEmitter::CEmberEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName )
{
}


CSmartPtr<CEmberEmitter> CEmberEmitter::Create( const char *pDebugName )
{
	return new CEmberEmitter( pDebugName );
}


void CEmberEmitter::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	float	speed = VectorNormalize( pParticle->m_vecVelocity );
	Vector	offset;

	speed -= ( 1.0f * timeDelta );

	offset.Random( -0.025f, 0.025f );
	offset[2] = 0.0f;

	pParticle->m_vecVelocity += offset;
	VectorNormalize( pParticle->m_vecVelocity );

	pParticle->m_vecVelocity *= speed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
Vector CEmberEmitter::UpdateColor( SimpleParticle *pParticle, float timeDelta )
{
	Vector	color;
	float	ramp = 1.0f - ( pParticle->m_flLifetime / pParticle->m_flDieTime );

	color[0] = ( (float) pParticle->m_uchColor[0] * ramp ) / 255.0f;
	color[1] = ( (float) pParticle->m_uchColor[1] * ramp ) / 255.0f;
	color[2] = ( (float) pParticle->m_uchColor[2] * ramp ) / 255.0f;

	return color;
}

//==================================================
// C_Embers
//==================================================

class C_Embers : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_Embers, C_BaseEntity );

					C_Embers();
					~C_Embers();

	void	Start( void );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	ShouldDraw( void );
	virtual void	AddEntity( void );

	//Server-side
	int		m_nDensity;
	int		m_nLifetime;
	int		m_nSpeed;
	bool	m_bEmit;

protected:

	void	SpawnEmber( void );

	PMaterialHandle		m_hMaterial;
	TimedEvent			m_tParticleSpawn;
	CSmartPtr<CEmberEmitter> m_pEmitter;

};

//Receive datatable
IMPLEMENT_CLIENTCLASS_DT( C_Embers, DT_Embers, CEmbers )
	RecvPropInt( RECVINFO( m_nDensity ) ),
	RecvPropInt( RECVINFO( m_nLifetime ) ),
	RecvPropInt( RECVINFO( m_nSpeed ) ),
	RecvPropInt( RECVINFO( m_bEmit ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
C_Embers::C_Embers()
{
	m_pEmitter = CEmberEmitter::Create( "C_Embers" );

	m_pEmitter->SetSortOrigin( GetAbsOrigin() );
}

C_Embers::~C_Embers()
{
}

void C_Embers::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_Embers::ShouldDraw()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Embers::Start( void )
{
	//Various setup info
	m_tParticleSpawn.Init( m_nDensity );
	
	m_hMaterial	= m_pEmitter->GetPMaterial( "particle/fire" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Embers::AddEntity( void ) 
{
	if ( m_bEmit == false )
		return;

	float tempDelta = gpGlobals->frametime;

	while( m_tParticleSpawn.NextEvent( tempDelta ) )
	{
		SpawnEmber();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Embers::SpawnEmber( void )
{
	Vector	offset, mins, maxs;
	
	modelinfo->GetModelBounds( GetModel(), mins, maxs );

	//Setup our spawn position
	offset[0] = random->RandomFloat( mins[0], maxs[0] );
	offset[1] = random->RandomFloat( mins[1], maxs[1] );
	offset[2] = random->RandomFloat( mins[2], maxs[2] );

	//Spawn the particle
	SimpleParticle	*sParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof( SimpleParticle ), m_hMaterial, offset );

	float	cScale = random->RandomFloat( 0.75f, 1.0f );

	//Set it up
	sParticle->m_flLifetime = 0.0f;
	sParticle->m_flDieTime	= m_nLifetime;

	sParticle->m_uchColor[0]	= m_clrRender->r * cScale;
	sParticle->m_uchColor[1]	= m_clrRender->g * cScale;
	sParticle->m_uchColor[2]	= m_clrRender->b * cScale;
	sParticle->m_uchStartAlpha	= 255;
	sParticle->m_uchEndAlpha	= 0;
	sParticle->m_uchStartSize	= 1;
	sParticle->m_uchEndSize		= 0;
	
	//Set the velocity
	Vector	velocity;

	AngleVectors( GetAbsAngles(), &velocity );

	sParticle->m_vecVelocity = velocity * m_nSpeed;

	sParticle->m_vecVelocity[0]	+= random->RandomFloat( -(m_nSpeed/8), (m_nSpeed/8) );
	sParticle->m_vecVelocity[1]	+= random->RandomFloat( -(m_nSpeed/8), (m_nSpeed/8) );
	sParticle->m_vecVelocity[2]	+= random->RandomFloat( -(m_nSpeed/8), (m_nSpeed/8) );
}
