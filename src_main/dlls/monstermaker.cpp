//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: An entity that creates NPCs in the game. There are two types of NPC
//			makers -- one which creates NPCs using a template NPC, and one which
//			creates an NPC via a classname.
//
//=============================================================================

#include "cbase.h"
#include "entityapi.h"
#include "entityoutput.h"
#include "ai_basenpc.h"
#include "monstermaker.h"
#include "TemplateEntities.h"
#include "ndebugoverlay.h"
#include "mapentities.h"


BEGIN_DATADESC( CBaseNPCMaker )

	DEFINE_KEYFIELD( CBaseNPCMaker, m_nMaxNumNPCs,			FIELD_INTEGER,	"MaxNPCCount" ),
	DEFINE_KEYFIELD( CBaseNPCMaker, m_nMaxLiveChildren,		FIELD_INTEGER,	"MaxLiveChildren" ),
	DEFINE_KEYFIELD( CBaseNPCMaker, m_flSpawnFrequency,		FIELD_FLOAT,	"SpawnFrequency" ),
	DEFINE_KEYFIELD( CBaseNPCMaker, m_bDisabled,			FIELD_BOOLEAN,	"StartDisabled" ),

	DEFINE_FIELD(	CBaseNPCMaker,	m_nLiveChildren,		FIELD_INTEGER ),
	DEFINE_FIELD(	CBaseNPCMaker,	m_flGround,				FIELD_FLOAT ),

	// Inputs
	DEFINE_INPUTFUNC( CBaseNPCMaker, FIELD_VOID,	"Spawn",	InputSpawnNPC ),
	DEFINE_INPUTFUNC( CBaseNPCMaker, FIELD_VOID,	"Enable",	InputEnable ),
	DEFINE_INPUTFUNC( CBaseNPCMaker, FIELD_VOID,	"Disable",	InputDisable ),
	DEFINE_INPUTFUNC( CBaseNPCMaker, FIELD_VOID,	"Toggle",	InputToggle ),
	DEFINE_INPUTFUNC( CBaseNPCMaker, FIELD_INTEGER, "SetMaxChildren", InputSetMaxChildren ),

	// Outputs
	DEFINE_OUTPUT( CBaseNPCMaker, m_OnAllSpawned,		"OnAllSpawned" ),
	DEFINE_OUTPUT( CBaseNPCMaker, m_OnAllSpawnedDead,	"OnAllSpawnedDead" ),
	DEFINE_OUTPUT( CBaseNPCMaker, m_OnSpawnNPC,			"OnSpawnNPC" ),

	// Function Pointers
	DEFINE_THINKFUNC( CBaseNPCMaker, MakerThink ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CBaseNPCMaker::Spawn( void )
{
	SetSolid( SOLID_NONE );
	m_nLiveChildren		= 0;
	Precache();

	// If I can make an infinite number of NPC, force them to fade
	if ( m_spawnflags & SF_NPCMAKER_INF_CHILD )
	{
		m_spawnflags |= SF_NPCMAKER_FADE;
	}

	//Start on?
	if ( m_bDisabled == false )
	{
		SetThink ( &CBaseNPCMaker::MakerThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
		//wait to be activated.
		SetThink ( &CBaseNPCMaker::SUB_DoNothing );
	}

	// Make sure absorigin is set
	Relink();

	m_flGround = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not it is OK to make an NPC at this instant.
//-----------------------------------------------------------------------------
bool CBaseNPCMaker::CanMakeNPC( void )
{
	if ( m_nMaxLiveChildren > 0 && m_nLiveChildren >= m_nMaxLiveChildren )
	{// not allowed to make a new one yet. Too many live ones out right now.
		return false;
	}

	if ( !m_flGround )
	{
		// set altitude. Now that I'm activated, any breakables, etc should be out from under me. 
		trace_t tr;

		UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() - Vector ( 0, 0, 2048 ), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		m_flGround = tr.endpos.z;
	}

	Vector mins = GetAbsOrigin() - Vector( 34, 34, 0 );
	Vector maxs = GetAbsOrigin() + Vector( 34, 34, 0 );
	maxs.z = GetAbsOrigin().z;
	
	//Only adjust for the ground if we want it
	if ( ( m_spawnflags & SF_NPCMAKER_NO_DROP ) == false )
	{
		mins.z = m_flGround;
	}

	CBaseEntity *pList[128];
	
	int count = UTIL_EntitiesInBox( pList, 128, mins, maxs, FL_CLIENT|FL_NPC );
	if ( count )
	{
		//Iterate through the list and check the results
		for ( int i = 0; i < count; i++ )
		{
			//Don't build on top of another entity
			if ( pList[i] == NULL )
				continue;

			//If one of the entities is solid, then we can't spawn now
			if ( ( pList[i]->GetSolidFlags() & FSOLID_NOT_SOLID ) == false )
				return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: If this had a finite number of children, return true if they've all
//			been created.
//-----------------------------------------------------------------------------
bool CBaseNPCMaker::IsDepleted()
{
	if ( (m_spawnflags & SF_NPCMAKER_INF_CHILD) || m_nMaxNumNPCs > 0 )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Toggle the spawner's state
//-----------------------------------------------------------------------------
void CBaseNPCMaker::Toggle( void )
{
	if ( m_bDisabled )
	{
		Enable();
	}
	else
	{
		Disable();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Start the spawner
//-----------------------------------------------------------------------------
void CBaseNPCMaker::Enable( void )
{
	// can't be enabled once depleted
	if ( IsDepleted() )
		return;

	m_bDisabled = false;
	SetThink ( &CBaseNPCMaker::MakerThink );
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: Stop the spawner
//-----------------------------------------------------------------------------
void CBaseNPCMaker::Disable( void )
{
	m_bDisabled = true;
	SetThink ( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that spawns an NPC.
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputSpawnNPC( inputdata_t &inputdata )
{
	if( !IsDepleted() )
	{
		MakeNPC();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input hander that starts the spawner
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputEnable( inputdata_t &inputdata )
{
	Enable();
}


//-----------------------------------------------------------------------------
// Purpose: Input hander that stops the spawner
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputDisable( inputdata_t &inputdata )
{
	Disable();
}


//-----------------------------------------------------------------------------
// Purpose: Input hander that toggles the spawner
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputToggle( inputdata_t &inputdata )
{
	Toggle();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CBaseNPCMaker::InputSetMaxChildren( inputdata_t &inputdata )
{
	m_nMaxNumNPCs = inputdata.value.Int();

	// Turn us back on, if we were dormant
	SetThink ( &CBaseNPCMaker::MakerThink );
	SetNextThink( gpGlobals->curtime );
}

LINK_ENTITY_TO_CLASS( npc_maker, CNPCMaker );

BEGIN_DATADESC( CNPCMaker )

	DEFINE_KEYFIELD( CNPCMaker, m_iszNPCClassname,		FIELD_STRING,	"NPCType" ),
	DEFINE_KEYFIELD( CNPCMaker, m_ChildTargetName,		FIELD_STRING,	"NPCTargetname" ),
	DEFINE_KEYFIELD( CNPCMaker, m_SquadName,			FIELD_STRING,	"NPCSquadName" ),
	DEFINE_KEYFIELD( CNPCMaker, m_spawnEquipment,		FIELD_STRING,	"additionalequipment" ),
	DEFINE_KEYFIELD( CNPCMaker, m_strHintGroup,			FIELD_STRING,	"NPCHintGroup" ),
	DEFINE_KEYFIELD( CNPCMaker, m_RelationshipString,	FIELD_STRING,	"Relationship" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CNPCMaker::CNPCMaker( void )
{
	m_spawnEquipment = NULL_STRING;
}


//-----------------------------------------------------------------------------
// Purpose: Precache the target NPC
//-----------------------------------------------------------------------------
void CNPCMaker::Precache( void )
{
	BaseClass::Precache();
	UTIL_PrecacheOther( STRING( m_iszNPCClassname ) );
}


//-----------------------------------------------------------------------------
// Purpose: Creates the NPC.
//-----------------------------------------------------------------------------
void CNPCMaker::MakeNPC( void )
{
	if (!CanMakeNPC())
	{
		return;
	}

	CAI_BaseNPC	*pent = (CAI_BaseNPC*)CreateEntityByName( STRING(m_iszNPCClassname) );

	if ( !pent )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	// ------------------------------------------------
	//  Intialize spawned NPC's relationships
	// ------------------------------------------------
	pent->SetRelationshipString( m_RelationshipString );

	m_OnSpawnNPC.FireOutput( this, this );

	pent->SetLocalOrigin( GetAbsOrigin() );
	pent->SetLocalAngles( GetAbsAngles() );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	if ( m_spawnflags & SF_NPCMAKER_FADE )
	{
		pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );
	}

	pent->m_spawnEquipment	= m_spawnEquipment;
	pent->SetSquadName( m_SquadName );
	pent->SetHintGroup( m_strHintGroup );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );

	if ( m_ChildTargetName != NULL_STRING )
	{
		// if I have a netname (overloaded), give the child NPC that name as a targetname
		pent->SetName( m_ChildTargetName );
	}

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates a new NPC every so often.
//-----------------------------------------------------------------------------
void CBaseNPCMaker::MakerThink ( void )
{
	SetNextThink( gpGlobals->curtime + m_flSpawnFrequency );

	MakeNPC();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//-----------------------------------------------------------------------------
void CBaseNPCMaker::DeathNotice( CBaseEntity *pVictim )
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_nLiveChildren--;

	// If we're here, we're getting erroneous death messages from children we haven't created
	assert( m_nLiveChildren >= 0 );

	if ( m_nLiveChildren <= 0 )
	{
		// See if we've exhausted our supply of NPCs
		if ( ( (m_spawnflags & SF_NPCMAKER_INF_CHILD) == false ) && IsDepleted() )
		{
			// Signal that all our children have been spawned and are now dead
			m_OnAllSpawnedDead.FireOutput( this, this );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates new NPCs from a template NPC. The template NPC must be marked
//			as a template (spawnflag) and does not spawn.
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( npc_template_maker, CTemplateNPCMaker );

BEGIN_DATADESC( CTemplateNPCMaker )

	DEFINE_KEYFIELD( CTemplateNPCMaker, m_iszTemplateName, FIELD_STRING, "TemplateName" ),
	DEFINE_KEYFIELD( CTemplateNPCMaker, m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_FIELD( CTemplateNPCMaker, m_iszTemplateData, FIELD_STRING ),

	DEFINE_INPUTFUNC( CTemplateNPCMaker, FIELD_VOID, "SpawnNPCInRadius", InputSpawnInRadius ),
	DEFINE_INPUTFUNC( CTemplateNPCMaker, FIELD_VOID, "SpawnNPCInLine", InputSpawnInLine ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Called after spawning on map load or on a load from save game.
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::Activate( void )
{
	// Let the base class activate
	BaseClass::Activate();

	if (!m_iszTemplateData)
	{
		//
		// This must be the first time we're activated, not a load from save game.
		// Look up the template in the template database.
		//
		if (!m_iszTemplateName)
		{
			Warning( "npc_template_maker %s has no template NPC!\n", GetEntityName() );
			UTIL_Remove( this );
			return;
		}
		else
		{
			m_iszTemplateData = Templates_FindByTargetName(STRING(m_iszTemplateName));
			if ( m_iszTemplateData == NULL_STRING )
			{
				Warning( "npc_template_maker %s: template NPC %s not found!\n", GetEntityName(), STRING(m_iszTemplateName) );
				UTIL_Remove( this );
				return;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::MakeNPC( void )
{
	if (!CanMakeNPC())
	{
		return;
	}

	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData), NULL );
	if ( pEntity != NULL )
	{
		pent = (CAI_BaseNPC *)pEntity;
	}

	if ( !pent )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	m_OnSpawnNPC.FireOutput( this, this );

	pent->SetLocalOrigin( GetAbsOrigin() );
	pent->SetLocalAngles( GetAbsAngles() );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	if ( m_spawnflags & SF_NPCMAKER_FADE )
	{
		pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );
	}

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	pent->Activate();

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::MakeNPCInLine( void )
{
	if (!CanMakeNPC())
	{
		return;
	}

	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData), NULL );
	if ( pEntity != NULL )
	{
		pent = (CAI_BaseNPC *)pEntity;
	}

	if ( !pent )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	m_OnSpawnNPC.FireOutput( this, this );

	PlaceNPCInLine( pent );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	pent->Activate();

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
bool CTemplateNPCMaker::PlaceNPCInLine( CAI_BaseNPC *pNPC )
{
	Vector vecPlace;
	Vector vecLine;

	GetVectors( &vecLine, NULL, NULL );

	// invert this, line up NPC's BEHIND the maker.
	vecLine *= -1;

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 8192 ), MASK_SHOT, pNPC, COLLISION_GROUP_NONE, &tr );
	vecPlace = tr.endpos;
	float flStepSize = pNPC->GetHullWidth();

	// Try 10 times to place this npc.
	for( int i = 0 ; i < 10 ; i++ )
	{
		UTIL_TraceHull( vecPlace,
						vecPlace + Vector( 0, 0, 10 ),
						pNPC->GetHullMins(),
						pNPC->GetHullMaxs(),
						MASK_SHOT,
						pNPC,
						COLLISION_GROUP_NONE,
						&tr );

		if( tr.fraction == 1.0 )
		{
			pNPC->SetAbsOrigin( tr.endpos );
			return true;
		}

		vecPlace += vecLine * flStepSize;
	}

	Msg("**Failed to place NPC in line! TELL WEDGE!\n");
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Place NPC somewhere on the perimeter of my radius.
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::MakeNPCInRadius( void )
{
	if (!CanMakeNPC())
	{
		return;
	}

	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData), NULL );
	if ( pEntity != NULL )
	{
		pent = (CAI_BaseNPC *)pEntity;
	}

	if ( !pent )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	m_OnSpawnNPC.FireOutput( this, this );

	PlaceNPCInRadius( pent );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	pent->Activate();

	ChildPostSpawn( pent );

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find a place to spawn an npc within my radius.
//			Right now this function tries to place them on the perimeter of radius.
// Output : false if we couldn't find a spot!
//-----------------------------------------------------------------------------
bool CTemplateNPCMaker::PlaceNPCInRadius( CAI_BaseNPC *pNPC )
{
	QAngle fan;

	fan.x = 0;
	fan.z = 0;

	for( fan.y = 0 ; fan.y < 360 ; fan.y += 18.0 )
	{
		Vector vecTest = GetAbsOrigin();
		Vector vecDir;

		AngleVectors( fan, &vecDir );

		vecTest = vecTest + vecDir * m_flRadius;

		trace_t tr;

		UTIL_TraceLine( vecTest, vecTest - Vector( 0, 0, 8192 ), MASK_SHOT, pNPC, COLLISION_GROUP_NONE, &tr );
		if( tr.fraction == 1.0 )
		{
			continue;
		}

		UTIL_TraceHull( tr.endpos,
						tr.endpos + Vector( 0, 0, 10 ),
						pNPC->GetHullMins(),
						pNPC->GetHullMaxs(),
						MASK_SHOT,
						pNPC,
						COLLISION_GROUP_NONE,
						&tr );

		if( tr.fraction == 1.0 )
		{
			pNPC->SetAbsOrigin( tr.endpos );
			return true;
		}
	}

	Msg("**Failed to place NPC in radius! TELL WEDGE!\n");
	return false;
}

