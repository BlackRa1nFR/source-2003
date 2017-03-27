//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: An entity that mapmakers can use to spawn instances of templates created with point_template
//=============================================================================

#include "cbase.h"
#include "entityoutput.h"
#include "TemplateEntities.h"
#include "point_template.h"

#define SF_ENTMAKER_AUTOSPAWN				0x0001
#define SF_ENTMAKER_WAITFORDESTRUCTION		0x0002
#define SF_ENTMAKER_IGNOREFACING			0x0004

//-----------------------------------------------------------------------------
// Purpose: An entity that mapmakers can use to ensure there's a required entity never runs out.
//			i.e. physics cannisters that need to be used.
//-----------------------------------------------------------------------------
class CEnvEntityMaker : public CPointEntity
{
	DECLARE_CLASS( CEnvEntityMaker, CPointEntity );
public:
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Activate( void );

	void		 SpawnEntity( void );
	void		 CheckSpawnThink( void );
	void		 InputForceSpawn( inputdata_t &inputdata );

public:
	Vector			m_vecEntityMins;
	Vector			m_vecEntityMaxs;
	EHANDLE			m_hCurrentInstance;
	EHANDLE			m_hCurrentBlocker;	// Last entity that blocked us spawning something
	Vector			m_vecBlockerOrigin;

	string_t		m_iszTemplate;

	COutputEvent	m_pOutputOnSpawned;
};

BEGIN_DATADESC( CEnvEntityMaker )
	// DEFINE_FIELD( CEnvEntityMaker, m_vecEntityMins, FIELD_VECTOR ),
	// DEFINE_FIELD( CEnvEntityMaker, m_vecEntityMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvEntityMaker, m_hCurrentInstance, FIELD_EHANDLE ),
	DEFINE_FIELD( CEnvEntityMaker, m_hCurrentBlocker, FIELD_EHANDLE ),
	DEFINE_FIELD( CEnvEntityMaker, m_vecBlockerOrigin, FIELD_VECTOR ),
	DEFINE_KEYFIELD( CEnvEntityMaker, m_iszTemplate, FIELD_STRING, "EntityTemplate" ),

	// Outputs
	DEFINE_OUTPUT( CEnvEntityMaker, m_pOutputOnSpawned, "OnEntitySpawned" ),

	// Inputs
	DEFINE_INPUTFUNC( CEnvEntityMaker, FIELD_VOID, "ForceSpawn", InputForceSpawn ),

	// Functions
	DEFINE_THINKFUNC( CEnvEntityMaker, CheckSpawnThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_entity_maker, CEnvEntityMaker );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvEntityMaker::Spawn( void )
{
	m_vecEntityMins = vec3_origin;
	m_vecEntityMaxs = vec3_origin;
	m_hCurrentInstance = NULL;
	m_hCurrentBlocker = NULL;
	m_vecBlockerOrigin = vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvEntityMaker::Activate( void )
{
	BaseClass::Activate();

	// check for valid template
	if ( m_iszTemplate == NULL_STRING )
	{
		Warning( "env_entity_maker %s has no template entity!\n", GetEntityName() );
		UTIL_Remove( this );
		return;
	}

	// Spawn an instance
	if ( m_spawnflags & SF_ENTMAKER_AUTOSPAWN )
	{
		SpawnEntity();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn an instance of the entity
//-----------------------------------------------------------------------------
void CEnvEntityMaker::SpawnEntity( void )
{
	// Find our point_template
	CPointTemplate *pTemplate = dynamic_cast<CPointTemplate *>(gEntList.FindEntityByName( NULL, STRING(m_iszTemplate), NULL ));
	if ( !pTemplate )
	{
		Warning( "env_entity_maker %s failed to spawn template %s.\n", GetEntityName(), STRING(m_iszTemplate) );
		return;
	}

	// Spawn our template
	CUtlVector<CBaseEntity*> hNewEntities;
	if ( !pTemplate->CreateInstance( GetAbsOrigin(), GetAbsAngles(), &hNewEntities ) )
		return;
	
	m_hCurrentInstance = hNewEntities[0];

	// Assume it'll block us
	m_hCurrentBlocker = m_hCurrentInstance;
	m_vecBlockerOrigin = m_hCurrentBlocker->GetAbsOrigin();

	// Store off the mins & maxs the first time we spawn
	if ( m_vecEntityMins == vec3_origin )
	{
		m_vecEntityMins = m_hCurrentInstance->WorldAlignMins();
		m_vecEntityMaxs = m_hCurrentInstance->WorldAlignMaxs();
	}

	// Fire our output
	m_pOutputOnSpawned.FireOutput( this, this );

	// Start thinking
	if ( m_spawnflags & SF_ENTMAKER_AUTOSPAWN )
	{
		SetThink( &CEnvEntityMaker::CheckSpawnThink );
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if we should spawn another instance
//-----------------------------------------------------------------------------
void CEnvEntityMaker::CheckSpawnThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.5f );

	// Do we have an instance?
	if ( m_hCurrentInstance )
	{
		// If Wait-For-Destruction is set, abort immediately
		if ( m_spawnflags & SF_ENTMAKER_WAITFORDESTRUCTION )
			return;
	}

	// Do we have a blocker?
	if ( m_hCurrentBlocker )
	{
		// If it hasn't moved, abort immediately
		if ( m_vecBlockerOrigin == m_hCurrentBlocker->GetAbsOrigin() )
			return;
	}

	// Check to see if there's enough room to spawn
	trace_t tr;
	UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin(), m_vecEntityMins, m_vecEntityMaxs, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.m_pEnt || tr.startsolid )
	{
		// Store off our blocker to check later
		m_hCurrentBlocker = tr.m_pEnt;
		if ( m_hCurrentBlocker )
		{
			m_vecBlockerOrigin = m_hCurrentBlocker->GetAbsOrigin();
		}
		return;
	}

	// We're clear, now check to see if the player's looking
	if ( !(m_spawnflags & SF_ENTMAKER_IGNOREFACING) )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
			if ( pPlayer )
			{
				// Only spawn if the player's looking away from me
				Vector vLookDir = pPlayer->EyeDirection3D();
				Vector vTargetDir = GetAbsOrigin() - pPlayer->EyePosition();
				VectorNormalize(vTargetDir);

				float fDotPr = DotProduct(vLookDir,vTargetDir);
				if ( fDotPr > 0 )
					return;
			}
		}
	}

	// Clear, no player watching, so spawn!
	SpawnEntity();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CEnvEntityMaker::InputForceSpawn( inputdata_t &inputdata )
{
	SpawnEntity();
}
