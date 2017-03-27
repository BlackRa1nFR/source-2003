//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Alyx, the female sidekick and love interest that's taking the world by storm!
//
//			Try the new Alyx Brite toothpaste!
//			Alyx lederhosen!
//
//			FIXME: need a better comment block
//
//=============================================================================

#include	"cbase.h"
#include	"NPCevent.h"
#include	"AI_BaseNPC.h"
#include	"ai_hull.h"
#include	"ai_basehumanoid.h"
#include	"npc_alyx.h"


LINK_ENTITY_TO_CLASS( npc_alyx, CNPC_Alyx );

BEGIN_DATADESC( CNPC_Alyx )

//	DEFINE_FIELD( CNPC_Citizen, m_FollowBehavior, FIELD_EMBEDDED ),

END_DATADESC()

//=========================================================
// Classify - indicates this NPC's place in the 
// relationship table.
//=========================================================
Class_T	CNPC_Alyx :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY_VITAL;
}


//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Alyx :: HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// GetSoundInterests - generic NPC can't hear.
//=========================================================
int CNPC_Alyx :: GetSoundInterests ( void )
{
	return	NULL;
}

//=========================================================
// 
//=========================================================
bool CNPC_Alyx :: CreateBehaviors()
{
	AddBehavior( &m_FollowBehavior );
	
	return BaseClass::CreateBehaviors();
}


//=========================================================
// Spawn
//=========================================================
void CNPC_Alyx :: Spawn()
{
	Precache();

	BaseClass::Spawn();

	SetModel( "models/alyx.mdl" );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 30;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_USE_WEAPONS | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD);

	NPCInit();

	Relink();
}

//=========================================================
// Precache - precaches all resources this NPC needs
//=========================================================
void CNPC_Alyx :: Precache()
{
	engine->PrecacheModel( "models/alyx.mdl" );
}	


//=========================================================
//=========================================================
int CNPC_Alyx :: SelectSchedule ( void )
{
	if ( !BehaviorSelectSchedule() )
	{
	}

	return BaseClass::SelectSchedule();
}
//=========================================================
// AI Schedules Specific to this NPC
//=========================================================

AI_BEGIN_CUSTOM_NPC( npc_alyx, CNPC_Alyx )
AI_END_CUSTOM_NPC()

