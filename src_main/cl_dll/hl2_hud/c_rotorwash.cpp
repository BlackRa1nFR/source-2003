//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "c_te_particlesystem.h"
#include "fx.h"
#include "fx_quad.h"

#define DUST_STARTSIZE		16
#define DUST_ENDSIZE		80
#define	WASH_RADIUS			128.0f
#define	DUST_STARTALPHA		0.3f
#define	DUST_ENDALPHA		0.0f
#define	DUST_LIFETIME		2.0f

//static Vector g_RotorWashColor( 0.78f, 0.72f, 0.66f );

extern IPhysicsSurfaceProps *physprops;


ConVar wash_r("wash_r", "180" );
ConVar wash_g("wash_g", "162" );
ConVar wash_b("wash_b", "122" );

ConVar wash_test("wash_test", "0", FCVAR_CHEAT );

//==================================================
// C_RotorWash
//==================================================

class C_RotorWash: public C_TEParticleSystem
{
public:

	DECLARE_CLASS( C_RotorWash, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

	class DustParticle : public Particle
	{
	public:
		float				m_RotationSpeed;
		float				m_CurRotation;
		float				m_FadeAlpha;
		unsigned char		m_Color[3];
		Vector				m_Velocity;
		float				m_Lifetime;
	};

	C_RotorWash();
	~C_RotorWash();

//C_BaseEntity
public:
	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	virtual	bool	ShouldDraw( void ) { return true; }

	Vector	m_vecDown;
	float	m_flMaxAltitude;

public:
	CParticleMgr		*m_pParticleMgr;
	PMaterialHandle		m_MaterialHandle;

protected:

	Vector		m_vecPos;
	Vector		m_vecAngles;
	Vector		m_vecVelocity;
	Vector		m_vecDustColor;
	float		m_fLifetime;
};

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT( CTERotorWash, C_RotorWash );

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_RotorWash, DT_TERotorWash, CTERotorWash )
	RecvPropVector( RECVINFO( m_vecDown ) ),
	RecvPropFloat( RECVINFO( m_flMaxAltitude ) ),
END_RECV_TABLE()

//==================================================
// C_RotorWash
//==================================================

C_RotorWash::C_RotorWash()
{
	m_pParticleMgr		= NULL;
	m_MaterialHandle	= INVALID_MATERIAL_HANDLE;
}

C_RotorWash::~C_RotorWash()
{
}

class WashEmitter : public CSimpleEmitter
{
public:
	
	WashEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	static WashEmitter *Create( const char *pDebugName )
	{
		return new WashEmitter( pDebugName );
	}

	void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		float lifetimePercent = pParticle->m_flLifetime / pParticle->m_flDieTime;

		if( lifetimePercent > 0 && pParticle->m_vecVelocity[ 2 ] == 0.0 )
		{
			// Float up when lifetime is half gone.
			pParticle->m_vecVelocity[ 2 ] = random->RandomFloat( 128, 256 );
		}

		// FIXME: optimize this....
		pParticle->m_vecVelocity *= ExponentialDecay( 0.9, 0.03, timeDelta );
	}

	virtual	float UpdateRoll( SimpleParticle *pParticle, float timeDelta )
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;
		
		pParticle->m_flRollDelta += pParticle->m_flRollDelta * ( timeDelta * -2.0f );

		//Cap the minimum roll
		if ( fabs( pParticle->m_flRollDelta ) < 0.5f )
		{
			pParticle->m_flRollDelta = ( pParticle->m_flRollDelta > 0.0f ) ? 0.5f : -0.5f;
		}

		return pParticle->m_flRoll;
	}

	virtual float UpdateAlpha( SimpleParticle *pParticle, float timeDelta )
	{
		return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
	}

private:
	WashEmitter( const WashEmitter & );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bNewEntity - whether or not to start a new entity
//-----------------------------------------------------------------------------

void C_RotorWash::PostDataUpdate( DataUpdateType_t updateType )
{
	trace_t	tr;

	UTIL_TraceLine( m_vecOrigin, m_vecOrigin+Vector(0,0,-m_flMaxAltitude), (MASK_SOLID_BRUSHONLY|CONTENTS_WATER), NULL, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction == 1.0f )
		return;

	bool bWaterWash = false;
	if ( wash_test.GetInt() )
	{
		bWaterWash = true;
	}

	if ( tr.contents & CONTENTS_WATER || bWaterWash )
	{
		m_vecDustColor.x = 0.8f;
		m_vecDustColor.y = 0.8f;
		m_vecDustColor.z = 0.75f;
	}
	else
	{
		m_vecDustColor.x = 0.45f;
		m_vecDustColor.y = 0.4f;
		m_vecDustColor.z = 0.3f;
	}

	CSmartPtr<WashEmitter> pSimple = WashEmitter::Create( "wash" );
	pSimple->SetSortOrigin( m_vecOrigin );
	pSimple->SetNearClip( 128, 256 );

	SimpleParticle	*pParticle;

	Vector	offset;

	int		numPuffs = 64;

	PMaterialHandle	hMaterial[2];
	
	if ( tr.contents & CONTENTS_WATER || bWaterWash )
	{
		hMaterial[0] = pSimple->GetPMaterial("effects/splash1");
		hMaterial[1] = pSimple->GetPMaterial("particle/particle_noisesphere");
	}
	else
	{
		hMaterial[0] = pSimple->GetPMaterial("particle/particle_smokegrenade");
		hMaterial[1] = pSimple->GetPMaterial("particle/particle_noisesphere");
	}

	Vector	color;

	// If we're above water, make ripples
	if ( bWaterWash )
	{
		float flScale = 8.0;

		trace_t	watertrace;
		Vector	color = Vector( 0.8f, 0.8f, 0.75f );
		Vector startPos = tr.endpos + Vector(0,0,8);
		Vector endPos = tr.endpos + Vector(0,0,-64);
		UTIL_TraceLine( startPos, endPos, MASK_WATER, NULL, COLLISION_GROUP_NONE, &watertrace );

		if ( watertrace.fraction < 1.0f )
		{
			//Add a ripple quad to the surface
			FX_AddQuad( watertrace.endpos + ( watertrace.plane.normal * 0.5f ), 
						watertrace.plane.normal, 
						64.0f * flScale, 
						128.0f * flScale, 
						0.8f,
						1.0f, 
						0.0f,
						0.25f,
						0, //random->RandomFloat( 0, 360 ),
						random->RandomFloat( -16.0f, 16.0f ),
						color, 
						1.5f,  
						"effects/splashwake3",  
						(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );
		}
	}

	//Throw smaller puffs
	for ( int i = 0; i < numPuffs; i++ )
	{
		offset[0] = random->RandomFloat( -WASH_RADIUS, WASH_RADIUS );
		offset[1] = random->RandomFloat( -WASH_RADIUS, WASH_RADIUS );
		offset[2] = random->RandomFloat(  -16, 8 );
		offset += tr.endpos + Vector( 0, 0, 8.0f );

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof(SimpleParticle), hMaterial[random->RandomInt(0,1)], offset );

		if ( pParticle != NULL )
		{			
			pParticle->m_flLifetime		= 0.0f;
			pParticle->m_flDieTime		= DUST_LIFETIME/2.0f;
			
			Vector dir = offset - tr.endpos;
			float length = VectorNormalize( dir );

			// Water particles move faster
			if ( bWaterWash )
			{
				pParticle->m_vecVelocity	= dir * ( length * RandomFloat(7.0f, 8.0f) );
			}
			else
			{
				pParticle->m_vecVelocity	= dir * ( length * 4.0f );
			}
			pParticle->m_vecVelocity[2]	= 0.0;

			color = m_vecDustColor * RandomFloat( 0.8f, 1.25f );

			pParticle->m_uchColor[0]	= color[0]*255;
			pParticle->m_uchColor[1]	= color[1]*255;
			pParticle->m_uchColor[2]	= color[2]*255;

			pParticle->m_uchStartAlpha	= random->RandomInt( 32, 128 );
			pParticle->m_uchEndAlpha	= 0;

			// Water particles are smaller
			if ( bWaterWash )
			{
				pParticle->m_uchStartSize	= 16;
				pParticle->m_uchEndSize		= 32;
			}
			else
			{
				pParticle->m_uchStartSize	= 16;
				pParticle->m_uchEndSize		= 64;
			}
			pParticle->m_flRoll			= random->RandomInt( 0, 360 );
			pParticle->m_flRollDelta	= random->RandomFloat( -16.0f, 16.0f );
		}
	}
}
