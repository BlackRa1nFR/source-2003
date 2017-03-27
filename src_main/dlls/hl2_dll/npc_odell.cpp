//========= Copyright © 2002, Valve LLC, All rights reserved. ============
//
// Purpose: Odell
//
//=============================================================================

#include "cbase.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Navigator.h"
#include "AI_Motor.h"
#include "npc_leader.h"
#include "npc_talker.h"
#include "soundent.h"
#include "game.h"
#include "NPCEvent.h"
#include "EntityList.h"
#include "npc_odell.h"
#include "activitylist.h"
#include "soundflags.h"
#include "ai_goalentity.h"
#include "scripted.h"

//-----------------------------------------------------------------------------
//
// CNPC_Odell
//

LINK_ENTITY_TO_CLASS( npc_odell, CNPC_Odell );

//=========================================================
// Odell schedules
//=========================================================
enum
{
	// Lead
	SCHED_ODELL_STAND_LOOK = LAST_SHARED_SCHEDULE,

	LAST_ODELL_SCHEDULE,

};

//=========================================================
// Anim Events	
//=========================================================
#define ODELL_AE_REDUCE_BBOX		11	// shrink bounding box (for squishing against walls)
#define ODELL_AE_RESTORE_BBOX		12	// restore bounding box

//-----------------------------------------------------------------------------

CNPC_Odell::CNPC_Odell()
{
}

bool CNPC_Odell::CreateBehaviors()
{
	AddBehavior( &m_LeadBehavior );
	AddBehavior( &m_FollowBehavior );
	
	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
//-----------------------------------------------------------------------------

void CNPC_Odell::Precache( void )
{
	engine->PrecacheModel( "models/odell.mdl" );
	
	BaseClass::Precache();
}
 
//-----------------------------------------------------------------------------
void CNPC_Odell::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	GetExpresser()->SetOuter( this );

	SetModel( "models/odell.mdl" );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_RED;
	m_fEffects			= 0;
	m_iHealth			= 20;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_CLIMB );
	CapabilitiesAdd( bits_CAP_USE_WEAPONS );
	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );

	NPCInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------

float CNPC_Odell::MaxYawSpeed( void )
{
	if( IsMoving() )
	{
		return 20;
	}

	switch( GetActivity() )
	{
	case ACT_180_LEFT:
		return 30;
		break;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 30;
		break;
	default:
		return 15;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : int	CNPC_Odell::Classify
//-----------------------------------------------------------------------------

Class_T	CNPC_Odell::Classify ( void )
{
	return	CLASS_PLAYER_ALLY_VITAL;
}

//-----------------------------------------------------------------------------

int CNPC_Odell::TranslateSchedule( int scheduleType ) 
{
#if 0
	switch( scheduleType )
	{
	case SCHED_IDLE_STAND:
		return SCHED_ODELL_STAND_LOOK;
		break;
	}
#endif
	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------

void CNPC_Odell::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ODELL_AE_REDUCE_BBOX:
		// for now...
		AddSolidFlags( FSOLID_NOT_SOLID );
		break;

	case ODELL_AE_RESTORE_BBOX:
		// for now...
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//-----------------------------------------------------------------------------

int CNPC_Odell::SelectSchedule( void )
{
	BehaviorSelectSchedule();
		
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------

bool CNPC_Odell::OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule )
{
	if ( pBehavior->CanSelectSchedule() && pBehavior != GetRunningBehavior() )
	{
		if ( pBehavior == &m_LeadBehavior || ( !IsLeading() && pBehavior == &m_FollowBehavior ) )
			return true;
	}
	
	return BaseClass::OnBehaviorChangeStatus( pBehavior, fCanFinishSchedule );
}

//-----------------------------------------------------------------------------

bool CNPC_OdellExpresser::Speak( AIConcept_t concept, const char *pszModifiers )
{
	// @Note (toml 09-12-02): the following code is placeholder until both tag support and scene groups are supported
	if ( pszModifiers && strcmp( concept, CONCEPT_LEAD_CATCHUP ) == 0 )
	{
		if ( strcmp( pszModifiers, "look" ) == 0 )
		{
			const char *ppszValidLooks[] =
			{
				"scene:scenes/borealis/Odell_lookit_01.vcd",
				"scene:scenes/borealis/Odell_lookit_02.vcd",
			};
			
			pszModifiers = ppszValidLooks[ random->RandomInt( 0, ARRAYSIZE( ppszValidLooks ) - 1 ) ];
		}
	}

	if ( !CanSpeakConcept( concept ) )
		return false;
	
	return CAI_Expresser::Speak( concept, pszModifiers );
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_odell, CNPC_Odell )

	DECLARE_USES_SCHEDULE_PROVIDER( CAI_LeadBehavior )
	DECLARE_USES_SCHEDULE_PROVIDER( CAI_FollowBehavior )

#if 0
	// This schedule makes Odell's idle unbreakable by the sound
	// of the player moving around nearby. Simple hack that lets
	// us continue working on the current crop of hard problems with
	// the tour guide (sjb)
	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	( 
		SCHED_ODELL_STAND_LOOK,

		"	Tasks"
		"		TASK_STOP_MOVING			0			"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE		"
		"		TASK_WAIT					4			"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)
#endif
	
AI_END_CUSTOM_NPC()

//-----------------------------------------------------------------------------

