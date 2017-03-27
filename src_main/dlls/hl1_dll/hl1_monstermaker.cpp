//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: An entity that creates NPCs in the game. There are two types of NPC
//			makers -- one which creates NPCs using a template NPC, and one which
//			creates an NPC via a classname.
//
//=============================================================================

#include "cbase.h"
#include "entityapi.h"
#include "EntityOutput.h"
#include "AI_BaseNPC.h"
#include "hl1_monstermaker.h"
#include "TemplateEntities.h"
#include "mapentities.h"


BEGIN_DATADESC( CBaseNPCMaker )

	DEFINE_KEYFIELD( CBaseNPCMaker, m_iMaxNumNPCs,			FIELD_INTEGER,	"monstercount" ),
	DEFINE_KEYFIELD( CBaseNPCMaker, m_iMaxLiveChildren,		FIELD_INTEGER,	"MaxLiveChildren" ),
	DEFINE_KEYFIELD( CBaseNPCMaker, m_flSpawnFrequency,		FIELD_FLOAT,	"delay" ),
	DEFINE_KEYFIELD( CBaseNPCMaker, m_bDisabled,			FIELD_BOOLEAN,	"StartDisabled" ),

	DEFINE_FIELD( CBaseNPCMaker, m_cLiveChildren,			FIELD_INTEGER ),
	DEFINE_FIELD( CBaseNPCMaker, m_flGround,				FIELD_FLOAT ),

	// Inputs
	DEFINE_INPUTFUNC( CBaseNPCMaker, FIELD_VOID, "Spawn",	InputSpawnNPC ),
	DEFINE_INPUTFUNC( CBaseNPCMaker, FIELD_VOID, "Enable",	InputEnable ),
	DEFINE_INPUTFUNC( CBaseNPCMaker, FIELD_VOID, "Disable",	InputDisable ),
	DEFINE_INPUTFUNC( CBaseNPCMaker, FIELD_VOID, "Toggle",	InputToggle ),

	// Outputs
	DEFINE_OUTPUT( CBaseNPCMaker, m_OnSpawnNPC, "OnSpawnNPC" ),

	// Function Pointers
	DEFINE_THINKFUNC( CBaseNPCMaker, MakerThink ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CBaseNPCMaker::Spawn( void )
{
	SetSolid( SOLID_NONE );
	m_cLiveChildren		= 0;
	Precache();

	// If I can make an infinite number of NPC, force them to fade
	if ( m_spawnflags & SF_NPCMAKER_INF_CHILD )
	{
		m_spawnflags |= SF_NPCMAKER_FADE;
	}

	//Start on?
	if ( m_bDisabled == false )
	{
		SetThink ( MakerThink );
		SetNextThink( gpGlobals->curtime + m_flSpawnFrequency );
	}
	else
	{
		//wait to be activated.
		SetThink ( SUB_DoNothing );
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
	if ( m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren )
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
	if ( (m_spawnflags & SF_NPCMAKER_INF_CHILD) || m_iMaxNumNPCs > 0 )
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
	SetThink ( MakerThink );
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
	MakeNPC();
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


LINK_ENTITY_TO_CLASS( monstermaker, CNPCMaker );

BEGIN_DATADESC( CNPCMaker )

	DEFINE_KEYFIELD( CNPCMaker, m_iszNPCClassname,		FIELD_STRING,	"monstertype" ),
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

	CBaseEntity	*pent = (CBaseEntity*)CreateEntityByName( STRING(m_iszNPCClassname) );

	if ( !pent )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	// ------------------------------------------------
	//  Intialize spawned NPC's relationships
	// ------------------------------------------------
//	pent->SetRelationshipString( m_RelationshipString );

	m_OnSpawnNPC.FireOutput( this, this );

	pent->SetLocalOrigin( GetAbsOrigin() );
	pent->SetLocalAngles( GetAbsAngles() );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	if ( m_spawnflags & SF_NPCMAKER_FADE )
	{
		pent->AddSpawnFlags( SF_NPC_FADE_CORPSE );
	}

/*	pent->m_spawnEquipment	= m_spawnEquipment;
	pent->SetSquadName( m_SquadName );
	pent->SetHintGroup( m_strHintGroup );*/

//	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );

/*	if ( m_ChildTargetName != NULL_STRING )
	{
		// if I have a netname (overloaded), give the child NPC that name as a targetname
		pent->SetName( m_ChildTargetName );
	}*/

//	ChildPostSpawn( pent );

	m_cLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_iMaxNumNPCs--;

		if ( IsDepleted() )
		{
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
	m_cLiveChildren--;
}


//-----------------------------------------------------------------------------
// Purpose: Creates new NPCs from a template NPC. The template NPC must be marked
//			as a template (spawnflag) and does not spawn.
//-----------------------------------------------------------------------------
class CTemplateNPCMaker : public CBaseNPCMaker
{
public:
	DECLARE_CLASS( CTemplateNPCMaker, CBaseNPCMaker );

	CTemplateNPCMaker( void ) {}

	void Activate( void );

	virtual void MakeNPC( void );

	DECLARE_DATADESC();
	
	string_t m_iszTemplateName;		// The name of the NPC that will be used as the template.
	string_t m_iszTemplateData;		// The keyvalue data blob from the template NPC that will be used to spawn new ones.
};


LINK_ENTITY_TO_CLASS( npc_template_maker, CTemplateNPCMaker );

BEGIN_DATADESC( CTemplateNPCMaker )

	DEFINE_KEYFIELD( CTemplateNPCMaker, m_iszTemplateName, FIELD_STRING, "TemplateName" ),
	DEFINE_FIELD( CTemplateNPCMaker, m_iszTemplateData, FIELD_STRING ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Called after spawning on map load or on a load from save game.
//-----------------------------------------------------------------------------
void CTemplateNPCMaker::Activate( void )
{
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
		}
		else
		{
			m_iszTemplateData = Templates_FindByTargetName(STRING(m_iszTemplateName));
			if ( m_iszTemplateData == NULL_STRING )
			{
				Warning( "npc_template_maker %s: template NPC % not found!\n", GetEntityName(), STRING(m_iszTemplateName) );
				UTIL_Remove( this );
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

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	pent->SetOwnerEntity( this );
	pent->Activate();

	ChildPostSpawn( pent );

	m_cLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_iMaxNumNPCs--;

		if ( IsDepleted() )
		{
			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}
