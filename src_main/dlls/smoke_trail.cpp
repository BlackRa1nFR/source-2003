#include "cbase.h"
#include "smoke_trail.h"
#include "dt_send.h"

#define SMOKETRAIL_ENTITYNAME		"env_smoketrail"
#define SPORETRAIL_ENTITYNAME		"env_sporetrail"
#define SPOREEXPLOSION_ENTITYNAME	"env_sporeexplosion"

//Data table
IMPLEMENT_SERVERCLASS_ST(SmokeTrail, DT_SmokeTrail)
	SendPropFloat(SENDINFO(m_SpawnRate), 8, 0, 1, 1024),
	SendPropVector(SENDINFO(m_StartColor), 8, 0, 0, 1),
	SendPropVector(SENDINFO(m_EndColor), 8, 0, 0, 1),
	SendPropFloat(SENDINFO(m_ParticleLifetime), 16, SPROP_ROUNDUP, 0.1, 100),
	SendPropFloat(SENDINFO(m_StopEmitTime), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MinSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MaxSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_StartSize), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_EndSize), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_SpawnRadius), -1, SPROP_NOSCALE),
	SendPropInt	(SENDINFO(m_bEmit), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nAttachment), 32 ),	
	SendPropFloat(SENDINFO(m_Opacity), -1, SPROP_NOSCALE),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(env_smoketrail, SmokeTrail);

BEGIN_DATADESC( SmokeTrail )

	DEFINE_FIELD( SmokeTrail, m_StartColor, FIELD_VECTOR ),
	DEFINE_FIELD( SmokeTrail, m_EndColor, FIELD_VECTOR ),
	DEFINE_FIELD( SmokeTrail, m_Opacity, FIELD_FLOAT ),
	DEFINE_FIELD( SmokeTrail, m_SpawnRate, FIELD_FLOAT ),
	DEFINE_FIELD( SmokeTrail, m_ParticleLifetime, FIELD_FLOAT ),
	DEFINE_FIELD( SmokeTrail, m_StopEmitTime, FIELD_TIME ),
	DEFINE_FIELD( SmokeTrail, m_MinSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( SmokeTrail, m_MaxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( SmokeTrail, m_StartSize, FIELD_FLOAT ),
	DEFINE_FIELD( SmokeTrail, m_EndSize, FIELD_FLOAT ),
	DEFINE_FIELD( SmokeTrail, m_SpawnRadius, FIELD_FLOAT ),
	DEFINE_FIELD( SmokeTrail, m_bEmit, FIELD_BOOLEAN ),
	DEFINE_FIELD( SmokeTrail, m_nAttachment, FIELD_INTEGER ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
SmokeTrail::SmokeTrail()
{
	m_SpawnRate = 10;
	m_StartColor.GetForModify().Init(0.5, 0.5, 0.5);
	m_EndColor.GetForModify().Init(0,0,0);
	m_ParticleLifetime = 5;
	m_StopEmitTime = 0; // Don't stop emitting particles
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_bEmit = true;
	m_nAttachment	= 0;
	m_Opacity = 0.5f;
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void SmokeTrail::SetEmit(bool bVal)
{
	m_bEmit = bVal;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : SmokeTrail*
//-----------------------------------------------------------------------------
SmokeTrail* SmokeTrail::CreateSmokeTrail()
{
	CBaseEntity *pEnt = CreateEntityByName(SMOKETRAIL_ENTITYNAME);
	if(pEnt)
	{
		SmokeTrail *pSmoke = dynamic_cast<SmokeTrail*>(pEnt);
		if(pSmoke)
		{
			pSmoke->Activate();
			return pSmoke;
		}
		else
		{
			UTIL_Remove(pEnt);
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Attach the smoke trail to an entity or point 
// Input  : index - entity that has the attachment
//			attachment - point to attach to
//-----------------------------------------------------------------------------
void SmokeTrail::FollowEntity( int index, int attachment )
{
	//For attachments
	m_nAttachment = attachment;

	BaseClass::FollowEntity( index );
}

//==================================================
// RocketTrail
//==================================================

//Data table
IMPLEMENT_SERVERCLASS_ST(RocketTrail, DT_RocketTrail)
	SendPropFloat(SENDINFO(m_SpawnRate), 8, 0, 1, 1024),
	SendPropVector(SENDINFO(m_StartColor), 8, 0, 0, 1),
	SendPropVector(SENDINFO(m_EndColor), 8, 0, 0, 1),
	SendPropFloat(SENDINFO(m_ParticleLifetime), 16, SPROP_ROUNDUP, 0.1, 100),
	SendPropFloat(SENDINFO(m_StopEmitTime), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MinSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_MaxSpeed), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_StartSize), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_EndSize), -1, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_SpawnRadius), -1, SPROP_NOSCALE),
	SendPropInt	(SENDINFO(m_bEmit), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nAttachment), 32 ),	
	SendPropFloat(SENDINFO(m_Opacity), -1, SPROP_NOSCALE),
	SendPropInt	(SENDINFO(m_bDamaged), 1, SPROP_UNSIGNED),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_rockettrail, RocketTrail );

BEGIN_DATADESC( RocketTrail )

	DEFINE_FIELD( RocketTrail, m_StartColor, FIELD_VECTOR ),
	DEFINE_FIELD( RocketTrail, m_EndColor, FIELD_VECTOR ),
	DEFINE_FIELD( RocketTrail, m_Opacity, FIELD_FLOAT ),
	DEFINE_FIELD( RocketTrail, m_SpawnRate, FIELD_FLOAT ),
	DEFINE_FIELD( RocketTrail, m_ParticleLifetime, FIELD_FLOAT ),
	DEFINE_FIELD( RocketTrail, m_StopEmitTime, FIELD_TIME ),
	DEFINE_FIELD( RocketTrail, m_MinSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( RocketTrail, m_MaxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( RocketTrail, m_StartSize, FIELD_FLOAT ),
	DEFINE_FIELD( RocketTrail, m_EndSize, FIELD_FLOAT ),
	DEFINE_FIELD( RocketTrail, m_SpawnRadius, FIELD_FLOAT ),
	DEFINE_FIELD( RocketTrail, m_bEmit, FIELD_BOOLEAN ),
	DEFINE_FIELD( RocketTrail, m_nAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( RocketTrail, m_bDamaged, FIELD_BOOLEAN ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
RocketTrail::RocketTrail()
{
	m_SpawnRate = 10;
	m_StartColor.GetForModify().Init(0.5, 0.5, 0.5);
	m_EndColor.GetForModify().Init(0,0,0);
	m_ParticleLifetime = 5;
	m_StopEmitTime = 0; // Don't stop emitting particles
	m_MinSpeed = 2;
	m_MaxSpeed = 4;
	m_StartSize = 35;
	m_EndSize = 55;
	m_SpawnRadius = 2;
	m_bEmit = true;
	m_nAttachment	= 0;
	m_Opacity = 0.5f;
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void RocketTrail::SetEmit(bool bVal)
{
	m_bEmit = bVal;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : SmokeTrail*
//-----------------------------------------------------------------------------
RocketTrail* RocketTrail::CreateRocketTrail()
{
	CBaseEntity *pEnt = CreateEntityByName( "env_rockettrail" );
	
	if( pEnt != NULL )
	{
		RocketTrail *pTrail = dynamic_cast<RocketTrail*>(pEnt);
		
		if( pTrail != NULL )
		{
			pTrail->Activate();
			return pTrail;
		}
		else
		{
			UTIL_Remove( pEnt );
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Attach the smoke trail to an entity or point 
// Input  : index - entity that has the attachment
//			attachment - point to attach to
//-----------------------------------------------------------------------------
void RocketTrail::FollowEntity( int index, int attachment )
{
	//For attachments
	m_nAttachment = attachment;

	BaseClass::FollowEntity( index );
}

//==================================================
// SporeTrail
//==================================================

IMPLEMENT_SERVERCLASS_ST( SporeTrail, DT_SporeTrail )
	SendPropFloat	(SENDINFO(m_flSpawnRate), 8, 0, 1, 1024),
	SendPropVector	(SENDINFO(m_vecEndColor), 8, 0, 0, 1),
	SendPropFloat	(SENDINFO(m_flParticleLifetime), 16, SPROP_ROUNDUP, 0.1, 100),
	SendPropFloat	(SENDINFO(m_flStartSize), -1, SPROP_NOSCALE),
	SendPropFloat	(SENDINFO(m_flEndSize), -1, SPROP_NOSCALE),
	SendPropFloat	(SENDINFO(m_flSpawnRadius), -1, SPROP_NOSCALE),
	SendPropInt		(SENDINFO(m_bEmit), 1, SPROP_UNSIGNED),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(env_sporetrail, SporeTrail);

BEGIN_DATADESC( SporeTrail )

	DEFINE_FIELD( SporeTrail, m_vecEndColor, FIELD_VECTOR ),
	DEFINE_FIELD( SporeTrail, m_flSpawnRate, FIELD_FLOAT ),
	DEFINE_FIELD( SporeTrail, m_flParticleLifetime, FIELD_FLOAT ),
	DEFINE_FIELD( SporeTrail, m_flStartSize, FIELD_FLOAT ),
	DEFINE_FIELD( SporeTrail, m_flEndSize, FIELD_FLOAT ),
	DEFINE_FIELD( SporeTrail, m_flSpawnRadius, FIELD_FLOAT ),
	DEFINE_FIELD( SporeTrail, m_bEmit, FIELD_BOOLEAN ),

END_DATADESC()

SporeTrail::SporeTrail( void )
{
	m_vecEndColor.GetForModify().Init();

	m_flSpawnRate			= 100.0f;
	m_flParticleLifetime	= 1.0f;
	m_flStartSize			= 1.0f;
	m_flEndSize				= 0.0f;
	m_flSpawnRadius			= 16.0f;
	SetRenderColor( 255, 255, 255, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : SporeTrail*
//-----------------------------------------------------------------------------
SporeTrail* SporeTrail::CreateSporeTrail()
{
	CBaseEntity *pEnt = CreateEntityByName( SPORETRAIL_ENTITYNAME );
	
	if(pEnt)
	{
		SporeTrail *pSpore = dynamic_cast<SporeTrail*>(pEnt);
		
		if ( pSpore )
		{
			pSpore->Activate();
			return pSpore;
		}
		else
		{
			UTIL_Remove( pEnt );
		}
	}

	return NULL;
}

//==================================================
// SporeExplosion
//==================================================

IMPLEMENT_SERVERCLASS_ST( SporeExplosion, DT_SporeExplosion )
	SendPropFloat	(SENDINFO(m_flSpawnRate), 8, 0, 1, 1024),
	SendPropFloat	(SENDINFO(m_flParticleLifetime), 16, SPROP_ROUNDUP, 0.1, 100),
	SendPropFloat	(SENDINFO(m_flStartSize), -1, SPROP_NOSCALE),
	SendPropFloat	(SENDINFO(m_flEndSize), -1, SPROP_NOSCALE),
	SendPropFloat	(SENDINFO(m_flSpawnRadius), -1, SPROP_NOSCALE),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_sporeexplosion, SporeExplosion );

BEGIN_DATADESC( SporeExplosion )

	DEFINE_FIELD( SporeExplosion, m_flSpawnRate, FIELD_FLOAT ),
	DEFINE_FIELD( SporeExplosion, m_flParticleLifetime, FIELD_FLOAT ),
	DEFINE_FIELD( SporeExplosion, m_flStartSize, FIELD_FLOAT ),
	DEFINE_FIELD( SporeExplosion, m_flEndSize, FIELD_FLOAT ),
	DEFINE_FIELD( SporeExplosion, m_flSpawnRadius, FIELD_FLOAT ),
	DEFINE_FIELD( SporeExplosion, m_bEmit, FIELD_BOOLEAN ),

END_DATADESC()

SporeExplosion::SporeExplosion( void )
{
	m_flSpawnRate			= 100.0f;
	m_flParticleLifetime	= 1.0f;
	m_flStartSize			= 1.0f;
	m_flEndSize				= 0.0f;
	m_flSpawnRadius			= 16.0f;
	SetRenderColor( 255, 255, 255, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : SporeExplosion*
//-----------------------------------------------------------------------------
SporeExplosion *SporeExplosion::CreateSporeExplosion()
{
	CBaseEntity *pEnt = CreateEntityByName( SPOREEXPLOSION_ENTITYNAME );
	
	if ( pEnt )
	{
		SporeExplosion *pSpore = dynamic_cast<SporeExplosion*>(pEnt);
		
		if ( pSpore )
		{
			pSpore->Activate();
			return pSpore;
		}
		else
		{
			UTIL_Remove( pEnt );
		}
	}

	return NULL;
}

