//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef FX_BLOOD_H
#define FX_BLOOD_H
#ifdef _WIN32
#pragma once
#endif

class CBloodSprayEmitter : public CSimpleEmitter
{
public:
	
	CBloodSprayEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	static CBloodSprayEmitter *Create( const char *pDebugName )
	{
		return new CBloodSprayEmitter( pDebugName );
	}

	inline void SetGravity( float flGravity )
	{
		m_flGravity = flGravity;
	}

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

	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
	{
		Vector	saveVelocity = pParticle->m_vecVelocity;

		//Decellerate
		static float dtime;
		static float decay;

		if ( dtime != timeDelta )
		{
			dtime = timeDelta;
			float expected = 0.5;
			decay = exp( log( 0.0001f ) * dtime / expected );
		}

		pParticle->m_vecVelocity *= decay;
	}

private:

	float m_flGravity;

	CBloodSprayEmitter( const CBloodSprayEmitter & );
};


extern ConVar	cl_show_bloodspray;

#endif // FX_BLOOD_H
