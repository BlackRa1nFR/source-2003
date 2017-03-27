
#include "cbase.h"
#include "baseparticleentity.h"


IMPLEMENT_NETWORKCLASS_ALIASED( BaseParticleEntity, DT_BaseParticleEntity )

BEGIN_NETWORK_TABLE( CBaseParticleEntity, DT_BaseParticleEntity )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(	CBaseParticleEntity )
END_PREDICTION_DATA()

CBaseParticleEntity::CBaseParticleEntity( void )
{
}

#if !defined( CLIENT_DLL )
bool CBaseParticleEntity::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
{
	if ( m_fEffects & EF_NODRAW )
		return false;

	if ( !IsEFlagSet( EFL_IN_SKYBOX ) )
	{
		if ( !engine->CheckAreasConnected( clientArea, pev->areanum) )
		{	
			// doors can legally straddle two areas, so
			// we may need to check another one
			if ( !pev->areanum2 || !engine->CheckAreasConnected( clientArea, pev->areanum2 ) )
			{
				return false;		// blocked by a door
			}
		}

		if ( !IsInPVS( recipient, pvs ) )
			return false;
	}

	return true;
}
#endif

void CBaseParticleEntity::Activate()
{
#if !defined( CLIENT_DLL )
	UTIL_Relink( this );
	BaseClass::Activate();
#endif
}	


void CBaseParticleEntity::Think()
{
	Remove( );
}


void CBaseParticleEntity::FollowEntity(int index)
{
	BaseClass::FollowEntity( CBaseEntity::Instance( index ) );
	SetLocalOrigin( vec3_origin );
}


void CBaseParticleEntity::SetLifetime(float lifetime)
{
	if(lifetime == -1)
		SetNextThink( TICK_NEVER_THINK );
	else
		SetNextThink( gpGlobals->curtime + lifetime );
}

#if defined( CLIENT_DLL )
void CBaseParticleEntity::GetSortOrigin( Vector &vSortOrigin )
{
	vSortOrigin = GetAbsOrigin();
}


bool CBaseParticleEntity::SimulateAndRender( Particle *pParticle, ParticleDraw* pParticleDraw, float &sortKey )
{
	// If you derive from C_BaseParticleEntity, you should always implement this function.
	assert( false );
	return true;
}
#endif
