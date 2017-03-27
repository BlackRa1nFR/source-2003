//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Default schedules.
//
//=============================================================================

#include "cbase.h"
#include "ai_default.h"
#include "soundent.h"
#include "scripted.h"
#include "ai_schedule.h"
#include "ai_squad.h"
#include "ai_networkmanager.h"
#include "stringregistry.h"
#include "igamesystem.h"
#include "ai_network.h"

CAI_Schedule *CAI_BaseNPC::ScheduleInList( const char *pName, CAI_Schedule **pList, int listCount )
{
	int i;

	if ( !pName )
	{
		Msg( "%s set to unnamed schedule!\n", GetClassname() );
		return NULL;
	}


	for ( i = 0; i < listCount; i++ )
	{
		if ( !pList[i]->GetName() )
		{
			Msg( "Unnamed schedule!\n" );
			continue;
		}
		if ( stricmp( pName, pList[i]->GetName() ) == 0 )
			return pList[i];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Given and schedule name, return the schedule ID
//-----------------------------------------------------------------------------
int CAI_BaseNPC::GetScheduleID(const char* schedName)
{
	return GetSchedulingSymbols()->ScheduleSymbolToId(schedName);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InitDefaultScheduleSR(void)
{
	#define ADD_DEF_SCHEDULE( name, localId ) idSpace.AddSchedule(name, localId, "CAI_BaseNPC" )

	CAI_ClassScheduleIdSpace &idSpace = CAI_BaseNPC::AccessClassScheduleIdSpaceDirect();

	ADD_DEF_SCHEDULE( "SCHED_NONE",							SCHED_NONE);
	ADD_DEF_SCHEDULE( "SCHED_IDLE_STAND",					SCHED_IDLE_STAND);
	ADD_DEF_SCHEDULE( "SCHED_IDLE_WALK",					SCHED_IDLE_WALK);
	ADD_DEF_SCHEDULE( "SCHED_IDLE_WANDER",					SCHED_IDLE_WANDER);
	ADD_DEF_SCHEDULE( "SCHED_WAKE_ANGRY",					SCHED_WAKE_ANGRY);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_FACE",					SCHED_ALERT_FACE);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_SMALL_FLINCH",			SCHED_ALERT_SMALL_FLINCH);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_SCAN",					SCHED_ALERT_SCAN);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_STAND",					SCHED_ALERT_STAND);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_WALK",					SCHED_ALERT_WALK);
	ADD_DEF_SCHEDULE( "SCHED_INVESTIGATE_SOUND",			SCHED_INVESTIGATE_SOUND);
	ADD_DEF_SCHEDULE( "SCHED_COMBAT_FACE",					SCHED_COMBAT_FACE);
	ADD_DEF_SCHEDULE( "SCHED_COMBAT_SWEEP",					SCHED_COMBAT_SWEEP);
	ADD_DEF_SCHEDULE( "SCHED_FEAR_FACE",					SCHED_FEAR_FACE);
	ADD_DEF_SCHEDULE( "SCHED_COMBAT_STAND",					SCHED_COMBAT_STAND);
	ADD_DEF_SCHEDULE( "SCHED_COMBAT_WALK",					SCHED_COMBAT_WALK);
	ADD_DEF_SCHEDULE( "SCHED_CHASE_ENEMY",					SCHED_CHASE_ENEMY);
	ADD_DEF_SCHEDULE( "SCHED_CHASE_ENEMY_FAILED",			SCHED_CHASE_ENEMY_FAILED);
	ADD_DEF_SCHEDULE( "SCHED_VICTORY_DANCE",				SCHED_VICTORY_DANCE);
	ADD_DEF_SCHEDULE( "SCHED_TARGET_FACE",					SCHED_TARGET_FACE);
	ADD_DEF_SCHEDULE( "SCHED_TARGET_CHASE",					SCHED_TARGET_CHASE);
	ADD_DEF_SCHEDULE( "SCHED_SMALL_FLINCH",					SCHED_SMALL_FLINCH);
	ADD_DEF_SCHEDULE( "SCHED_BACK_AWAY_FROM_ENEMY",			SCHED_BACK_AWAY_FROM_ENEMY);
	ADD_DEF_SCHEDULE( "SCHED_BACK_AWAY_FROM_SAVE_POSITION",	SCHED_BACK_AWAY_FROM_SAVE_POSITION);
	ADD_DEF_SCHEDULE( "SCHED_TAKE_COVER_FROM_ENEMY",		SCHED_TAKE_COVER_FROM_ENEMY);
	ADD_DEF_SCHEDULE( "SCHED_TAKE_COVER_FROM_BEST_SOUND",	SCHED_TAKE_COVER_FROM_BEST_SOUND);
	ADD_DEF_SCHEDULE( "SCHED_FLEE_FROM_BEST_SOUND",			SCHED_FLEE_FROM_BEST_SOUND);
	ADD_DEF_SCHEDULE( "SCHED_TAKE_COVER_FROM_ORIGIN",		SCHED_TAKE_COVER_FROM_ORIGIN);
	ADD_DEF_SCHEDULE( "SCHED_FAIL_TAKE_COVER",				SCHED_FAIL_TAKE_COVER);
	ADD_DEF_SCHEDULE( "SCHED_RUN_FROM_ENEMY",				SCHED_RUN_FROM_ENEMY);
	ADD_DEF_SCHEDULE( "SCHED_ESTABLISH_LINE_OF_FIRE",		SCHED_ESTABLISH_LINE_OF_FIRE);
	ADD_DEF_SCHEDULE( "SCHED_FAIL_ESTABLISH_LINE_OF_FIRE",	SCHED_FAIL_ESTABLISH_LINE_OF_FIRE);
	ADD_DEF_SCHEDULE( "SCHED_COWER",						SCHED_COWER);
	ADD_DEF_SCHEDULE( "SCHED_MELEE_ATTACK1",				SCHED_MELEE_ATTACK1);
	ADD_DEF_SCHEDULE( "SCHED_MELEE_ATTACK2",				SCHED_MELEE_ATTACK2);
	ADD_DEF_SCHEDULE( "SCHED_RANGE_ATTACK1",				SCHED_RANGE_ATTACK1);
	ADD_DEF_SCHEDULE( "SCHED_RANGE_ATTACK2",				SCHED_RANGE_ATTACK2);
	ADD_DEF_SCHEDULE( "SCHED_SPECIAL_ATTACK1",				SCHED_SPECIAL_ATTACK1);
	ADD_DEF_SCHEDULE( "SCHED_SPECIAL_ATTACK2",				SCHED_SPECIAL_ATTACK2);
	ADD_DEF_SCHEDULE( "SCHED_STANDOFF",						SCHED_STANDOFF);
	ADD_DEF_SCHEDULE( "SCHED_ARM_WEAPON",					SCHED_ARM_WEAPON);
	ADD_DEF_SCHEDULE( "SCHED_DISARM_WEAPON",				SCHED_DISARM_WEAPON);
	ADD_DEF_SCHEDULE( "SCHED_HIDE_AND_RELOAD",				SCHED_HIDE_AND_RELOAD);
	ADD_DEF_SCHEDULE( "SCHED_RELOAD",						SCHED_RELOAD);
	ADD_DEF_SCHEDULE( "SCHED_AMBUSH",						SCHED_AMBUSH);
	ADD_DEF_SCHEDULE( "SCHED_DIE",							SCHED_DIE);
	ADD_DEF_SCHEDULE( "SCHED_DIE_RAGDOLL",					SCHED_DIE_RAGDOLL);
	ADD_DEF_SCHEDULE( "SCHED_WAIT_FOR_SCRIPT",				SCHED_WAIT_FOR_SCRIPT);
	ADD_DEF_SCHEDULE( "SCHED_AISCRIPT",						SCHED_AISCRIPT);
	ADD_DEF_SCHEDULE( "SCHED_SCRIPTED_WALK",				SCHED_SCRIPTED_WALK);
	ADD_DEF_SCHEDULE( "SCHED_SCRIPTED_RUN",					SCHED_SCRIPTED_RUN);
	ADD_DEF_SCHEDULE( "SCHED_SCRIPTED_CUSTOM_MOVE",			SCHED_SCRIPTED_CUSTOM_MOVE);
	ADD_DEF_SCHEDULE( "SCHED_SCRIPTED_WAIT",				SCHED_SCRIPTED_WAIT);
	ADD_DEF_SCHEDULE( "SCHED_SCRIPTED_FACE",				SCHED_SCRIPTED_FACE);
	ADD_DEF_SCHEDULE( "SCHED_SCENE_SEQUENCE",				SCHED_SCENE_SEQUENCE);
	ADD_DEF_SCHEDULE( "SCHED_SCENE_WALK",					SCHED_SCENE_WALK);
	ADD_DEF_SCHEDULE( "SCHED_SCENE_FACE_TARGET",			SCHED_SCENE_FACE_TARGET);
	ADD_DEF_SCHEDULE( "SCHED_NEW_WEAPON",					SCHED_NEW_WEAPON);
	ADD_DEF_SCHEDULE( "SCHED_GIVE_WAY",						SCHED_GIVE_WAY);
	ADD_DEF_SCHEDULE( "SCHED_FORCED_GO",					SCHED_FORCED_GO);
	ADD_DEF_SCHEDULE( "SCHED_FORCED_GO_RUN",					SCHED_FORCED_GO_RUN);
	ADD_DEF_SCHEDULE( "SCHED_PATROL_WALK",					SCHED_PATROL_WALK);
	ADD_DEF_SCHEDULE( "SCHED_PATROL_RUN",					SCHED_PATROL_RUN);
	ADD_DEF_SCHEDULE( "SCHED_RUN_RANDOM",					SCHED_RUN_RANDOM);
	ADD_DEF_SCHEDULE( "SCHED_FAIL",							SCHED_FAIL);
	ADD_DEF_SCHEDULE( "SCHED_FALL_TO_GROUND",				SCHED_FALL_TO_GROUND);
	ADD_DEF_SCHEDULE( "SCHED_DROPSHIP_DEPLOY_FIRST",		SCHED_DROPSHIP_DEPLOY_FIRST);
	ADD_DEF_SCHEDULE( "SCHED_DROPSHIP_DEPLOY_SECOND",		SCHED_DROPSHIP_DEPLOY_SECOND);
	ADD_DEF_SCHEDULE( "SCHED_DROPSHIP_DEPLOY_THIRD",		SCHED_DROPSHIP_DEPLOY_THIRD);
	ADD_DEF_SCHEDULE( "SCHED_NPC_FREEZE",					SCHED_NPC_FREEZE);

	ADD_DEF_SCHEDULE( "SCHED_FLINCH_PHYSICS",			SCHED_FLINCH_PHYSICS);

	ADD_DEF_SCHEDULE( "SCHED_AWAIT_ORDERS",		SCHED_AWAIT_ORDERS);
	ADD_DEF_SCHEDULE( "SCHED_MOVE_TO_COMMAND_GOAL",		SCHED_MOVE_TO_COMMAND_GOAL);
}

bool CAI_BaseNPC::LoadDefaultSchedules(void)
{
//	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_NONE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_IDLE_STAND);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_IDLE_WALK);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_IDLE_WANDER);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_WAKE_ANGRY);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_ALERT_FACE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_ALERT_SMALL_FLINCH);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_ALERT_SCAN);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_ALERT_STAND);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_ALERT_WALK);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_INVESTIGATE_SOUND);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_COMBAT_FACE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_COMBAT_SWEEP);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_COMBAT_WALK);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_FEAR_FACE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_COMBAT_STAND);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_CHASE_ENEMY);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_CHASE_ENEMY_FAILED);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_VICTORY_DANCE);
//	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_TARGET_FACE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_TARGET_CHASE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SMALL_FLINCH);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_BACK_AWAY_FROM_ENEMY);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_BACK_AWAY_FROM_SAVE_POSITION);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_TAKE_COVER_FROM_ENEMY);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_TAKE_COVER_FROM_BEST_SOUND);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_FLEE_FROM_BEST_SOUND);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_TAKE_COVER_FROM_ORIGIN);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_FAIL_TAKE_COVER);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_RUN_FROM_ENEMY);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_ESTABLISH_LINE_OF_FIRE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_FAIL_ESTABLISH_LINE_OF_FIRE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_COWER);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_MELEE_ATTACK1);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_MELEE_ATTACK2);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_RANGE_ATTACK1);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_RANGE_ATTACK2);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SPECIAL_ATTACK1);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SPECIAL_ATTACK2);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_STANDOFF);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_ARM_WEAPON);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_DISARM_WEAPON);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_HIDE_AND_RELOAD);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_RELOAD);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_AMBUSH);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_DIE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_DIE_RAGDOLL);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_WAIT_FOR_SCRIPT);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SCRIPTED_WALK);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SCRIPTED_RUN);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SCRIPTED_CUSTOM_MOVE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SCRIPTED_WAIT);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SCRIPTED_FACE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SCENE_SEQUENCE);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SCENE_WALK);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_SCENE_FACE_TARGET);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_NEW_WEAPON);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_GIVE_WAY);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_FORCED_GO);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_FORCED_GO_RUN);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_PATROL_WALK);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_PATROL_RUN);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_RUN_RANDOM);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_FAIL);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_FALL_TO_GROUND);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_DROPSHIP_DEPLOY_FIRST);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_DROPSHIP_DEPLOY_SECOND);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_DROPSHIP_DEPLOY_THIRD);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_FLINCH_PHYSICS);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_AWAIT_ORDERS);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_MOVE_TO_COMMAND_GOAL);
	AI_LOAD_DEF_SCHEDULE( CAI_BaseNPC,					SCHED_NPC_FREEZE);

	return true;
}

int CAI_BaseNPC::TranslateSchedule( int scheduleType )
{
	// FIXME: Where should this go now?
#if 0
	if (scheduleType >= LAST_SHARED_SCHEDULE)
	{
		char errMsg[256];
		Q_snprintf(errMsg,sizeof(errMsg),"ERROR: Subclass Schedule (%s) Hitting Base Class!\n",ScheduleName(scheduleType));
		Msg( errMsg );
		AddTimedOverlay( errMsg, 5);
		return SCHED_FAIL;
	}
#endif

	switch( scheduleType )
	{
	// Hande some special cases
	case SCHED_AISCRIPT:
		{
			Assert( m_hCine != NULL );
			if ( !m_hCine )
			{
				DevWarning( 2, "Script failed for %s\n", GetClassname() );
				CineCleanup();
				return SCHED_IDLE_STAND;
			}
//			else
//				DevMsg( 2, "Starting script %s for %s\n", STRING( m_hCine->m_iszPlay ), GetClassname() );

			switch ( m_hCine->m_fMoveTo )
			{
				case CINE_MOVETO_WAIT:
				case CINE_MOVETO_TELEPORT:
				{
					return SCHED_SCRIPTED_WAIT;
				}

				case CINE_MOVETO_WALK:
				{
					return SCHED_SCRIPTED_WALK;
				}

				case CINE_MOVETO_RUN:
				{
					return SCHED_SCRIPTED_RUN;
				}

				case CINE_MOVETO_CUSTOM:
				{
					return SCHED_SCRIPTED_CUSTOM_MOVE;
				}

				case CINE_MOVETO_WAIT_FACING:
				{
					return SCHED_SCRIPTED_FACE;
				}
			}
		}
		break;

	case SCHED_IDLE_STAND:
		{
			// FIXME: crows are set into IDLE_STAND as an failure schedule, not sure if ALERT_STAND or COMBAT_STAND is a better choice
			// Assert( m_NPCState == NPC_STATE_IDLE );
		}
		break;
	case SCHED_IDLE_WANDER:
		{
			// FIXME: citizen interaction only, no idea what the state is.
			// Assert( m_NPCState == NPC_STATE_IDLE );
		}
		break;

	case SCHED_IDLE_WALK:
		{
			switch( m_NPCState )
			{
			case NPC_STATE_ALERT:
				return SCHED_ALERT_WALK;
			case NPC_STATE_COMBAT:
				return SCHED_COMBAT_WALK;
			}
		}
		break;

	case SCHED_ALERT_FACE:
	case SCHED_ALERT_SMALL_FLINCH:
		{
			// FIXME: default AI can pick this when in idle state
			// Assert( m_NPCState == NPC_STATE_ALERT );
		}
		break;
	case SCHED_ALERT_SCAN:
	case SCHED_ALERT_STAND:
		{
			// FIXME: rollermines use this when they're being held
			// Assert( m_NPCState == NPC_STATE_ALERT );
		}
		break;
	case SCHED_ALERT_WALK:
		{
			Assert( m_NPCState == NPC_STATE_ALERT );
		}
		break;
	case SCHED_COMBAT_FACE:
		{
			// FIXME: failure schedule for SCHED_PATROL which can be called when in alert
			// Assert( m_NPCState == NPC_STATE_COMBAT );
		}
		break;
	case SCHED_COMBAT_STAND:
		{
			// FIXME: never used?
		}
		break;
	case SCHED_COMBAT_WALK:
		{
			Assert( m_NPCState == NPC_STATE_COMBAT );
		}
		break;
	}

	return scheduleType;
}

//=========================================================
// GetScheduleOfType - returns a pointer to one of the
// NPC's available schedules of the indicated type.
//=========================================================
CAI_Schedule *CAI_BaseNPC::GetScheduleOfType( int scheduleType )
{
	// allow the derived classes to pick an appropriate version of this schedule or override
	// base schedule types.
	scheduleType = TranslateSchedule( scheduleType );

	// Get a pointer to that schedule
	CAI_Schedule *schedule = GetSchedule(scheduleType);

	if (!schedule)
	{
		Msg( "GetScheduleOfType(): No CASE for Schedule Type %d!\n", scheduleType );
		return GetSchedule(SCHED_IDLE_STAND);
	}
	return schedule;
}

CAI_Schedule *CAI_BaseNPC::GetSchedule(int schedule)
{
	if (!GetClassScheduleIdSpace()->IsGlobalBaseSet())
	{
		Warning("ERROR: %s missing schedule!\n", GetSchedulingErrorName());
		return g_AI_SchedulesManager.GetScheduleFromID(SCHED_IDLE_STAND);
	}
	if ( AI_IdIsLocal( schedule ) )
	{
		schedule = GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedule);
	}

	return g_AI_SchedulesManager.GetScheduleFromID( schedule );
}

const char* CAI_BaseNPC::ConditionName(int conditionID)
{
	if ( AI_IdIsLocal( conditionID ) )
		conditionID = GetClassScheduleIdSpace()->ConditionLocalToGlobal(conditionID);
	return GetSchedulingSymbols()->ConditionIdToSymbol(conditionID);
}

const char *CAI_BaseNPC::TaskName(int taskID)
{
	if ( AI_IdIsLocal( taskID ) )
		taskID = GetClassScheduleIdSpace()->TaskLocalToGlobal(taskID);
	return GetSchedulingSymbols()->TaskIdToSymbol( taskID );
}



// This hooks the main game systems callbacks to allow the AI system to manage memory
class CAI_SystemHook : public CAutoGameSystem
{
public:
	// UNDONE: Schedule / strings stuff should probably happen once each GAME, not each level
	void LevelInitPreEntity()
	{
		g_AI_SchedulesManager.CreateStringRegistries();
	}

	void LevelShutdownPostEntity( void )
	{
		g_pAINetworkManager->DeleteAllAINetworks();
		g_AI_SchedulesManager.DeleteAllSchedules();
		g_AI_SquadManager.DeleteAllSquads();
		g_AI_SchedulesManager.DestroyStringRegistries();
	}
};


static CAI_SystemHook g_AISystemHook;


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
// > Fail
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_FAIL,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
	"		TASK_WAIT				1"
	"		TASK_WAIT_PVS			0"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1 "
	"		COND_CAN_RANGE_ATTACK2 "
	"		COND_CAN_MELEE_ATTACK1 "
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_GIVE_WAY"
);

//===============================================
//	> Idle_Stand
//===============================================
AI_DEFINE_SCHEDULE
(
	SCHED_IDLE_STAND,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
	"		TASK_WAIT				5"
	"		TASK_WAIT_PVS			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_SMELL"
	"		COND_PROVOKED"
	"		COND_GIVE_WAY"
	"		COND_HEAR_PLAYER"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_BULLET_IMPACT"
);

//===============================================
//	> Wait_For_Script
//===============================================
AI_DEFINE_SCHEDULE
(
	SCHED_WAIT_FOR_SCRIPT,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
	"		TASK_WAIT_INDEFINITE	0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
);

//===============================================
//	> IdleWalk
//===============================================
AI_DEFINE_SCHEDULE
(
	SCHED_IDLE_WALK,

	"	Tasks"
	"		TASK_WALK_PATH			9999"
	"		TASK_WAIT_FOR_MOVEMENT	0"
	"		TASK_WAIT_PVS			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_SMELL"
	"		COND_PROVOKED"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_BULLET_IMPACT"
);

//===============================================
//	> NewWeapon
//===============================================
AI_DEFINE_SCHEDULE
(
	SCHED_NEW_WEAPON,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_GET_PATH_TO_TARGET		0"
	"		TASK_SET_TOLERANCE_DISTANCE	5"
	"		TASK_WEAPON_RUN_PATH		0"
	"		TASK_FACE_TARGET			0"
	"		TASK_WEAPON_PICKUP			0"
	"		TASK_WAIT					0.5"
	""
	"	Interrupts"
);

//===============================================
//	> RangeAttack1
//===============================================
AI_DEFINE_SCHEDULE
(
	SCHED_RANGE_ATTACK1,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_RANGE_ATTACK1		0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_OCCLUDED"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_HEAR_DANGER"
);

//===============================================
//	> RangeAttack2
//===============================================
AI_DEFINE_SCHEDULE
(
	SCHED_RANGE_ATTACK2,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_FACE_ENEMY				0"
	"		TASK_ANNOUNCE_ATTACK		2"	// 2 = secondary attack
	"		TASK_RANGE_ATTACK2			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_OCCLUDED"
	"		COND_NO_SECONDARY_AMMO"
	"		COND_HEAR_DANGER"
);

//=========================================================
//  > Ambush -  monster stands in place and waits for a new
//				enemy or chance to attack an existing enemy.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_AMBUSH,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_WAIT_INDEFINITE		0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_PROVOKED"
);

//=========================================================
//  > Idle_Stand schedule - !!!BUGBUG - if this schedule doesn't
// complete on its own the monster's HintNode will not be
// cleared and the rest of the monster's group will avoid
// that node because they think the group member that was
// previously interrupted is still using that node to active
// idle.
///=========================================================
//AI_DEFINE_SCHEDULE
//	Idle_Stand
//
//Tasks
//	 TASK_FIND_HINTNODE				0
//	 TASK_GET_PATH_TO_HINTNODE		0
//	 TASK_STORE_LASTPOSITION		0
//	 TASK_WALK_PATH					0
//	 TASK_WAIT_FOR_MOVEMENT			0
//	 TASK_FACE_HINTNODE				0
//	 TASK_PLAY_ACTIVE_IDLE			0
//	 TASK_GET_PATH_TO_LASTPOSITION	0
//	 TASK_WALK_PATH					0
//	 TASK_WAIT_FOR_MOVEMENT			0
//	 TASK_CLEAR_LASTPOSITION		0
//	 TASK_CLEAR_HINTNODE			0


//Interrupts
//		New_Enemy
//		Light_Damage
//		Heavy_Damage
//		Provoked
//		HEAR_COMBAT
//		HEAR_WORLD
//		HEAR_PLAYER
//		HEAR_DANGER
//		HEAR_BULLET_IMPACT


//=========================================================
//	> WakeAngry
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_WAKE_ANGRY,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE "
	"		TASK_SOUND_WAKE			0"
	"		TASK_FACE_IDEAL			0"
	""
	"	Interrupts"
);

//=========================================================
//  > AlertFace
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ALERT_FACE,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_FACE_IDEAL				0"
	"		TASK_WAIT					0.5"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_PROVOKED"
);

//=========================================================
//  > AlertSmallFlinch
//					shot but didn't see attacker
//					flinch then face
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ALERT_SMALL_FLINCH,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_REMEMBER				MEMORY:FLINCHED "
	"		TASK_SMALL_FLINCH			0"
	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_ALERT_FACE"
	""
	"	Interrupts"
);

//=========================================================
//  > Alert_Scan
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ALERT_SCAN,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_WAIT				0.5"
	"		TASK_TURN_LEFT			1800"
	"		TASK_WAIT				0.5"
	"		TASK_TURN_LEFT			1800"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
);

//=========================================================
//  > AlertStand
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ALERT_STAND,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_WAIT					20"
	"		TASK_SUGGEST_STATE			STATE:IDLE"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_PROVOKED"
	"		COND_SMELL"
	"		COND_HEAR_COMBAT"		// sound flags
	"		COND_HEAR_WORLD"
	"		COND_HEAR_PLAYER"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_BULLET_IMPACT"
);



//=========================================================
// > AlertWAlk
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ALERT_WALK,

	"	Tasks"
	"		TASK_WALK_PATH			0"
	"		TASK_WAIT_FOR_MOVEMENT	0"
	"		TASK_WAIT_PVS			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
);

//=========================================================
// > InvestigateSound
//
//	sends a monster to the location of the
//	sound that was just heard to check things out.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_INVESTIGATE_SOUND,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_STORE_LASTPOSITION			0"
	"		TASK_GET_PATH_TO_BESTSOUND		0"
	"		TASK_FACE_IDEAL					0"
	"		TASK_SET_TOLERANCE_DISTANCE		5"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_IDLE"
	"		TASK_WAIT						10"
	"		TASK_GET_PATH_TO_LASTPOSITION	0"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_CLEAR_LASTPOSITION			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_SEE_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
);

//=========================================================
// > CombatStand
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_COMBAT_STAND,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_WAIT_INDEFINITE		0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_SEE_ENEMY"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
);

//=========================================================
// > CombatWAlk
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_COMBAT_WALK,

	"	Tasks"
	"		TASK_WALK_PATH			0"
	"		TASK_WAIT_FOR_MOVEMENT	0"
	"		TASK_WAIT_PVS			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
);

//=========================================================
// > CombatFace
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_COMBAT_FACE,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
	"		TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
);

//=========================================================
// 	COMBAT_SWEEP
//
// Do a small sweep of the area
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_COMBAT_SWEEP,

	"	Tasks"
	"		TASK_TURN_LEFT		45"
	"		TASK_WAIT			2"
	"		TASK_TURN_RIGHT		45"
	"		TASK_WAIT			2"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_WORLD"
);

//=========================================================
// > Standoff
//
// Used in combat when a monster is
// hiding in cover or the enemy has moved out of sight.
// Should we look around in this schedule?
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_STANDOFF,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"		// Translated to cover
	"		TASK_WAIT_FACE_ENEMY		2"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_ENEMY_DEAD"
	"		COND_NEW_ENEMY"
	"		COND_HEAR_DANGER"
);

//=========================================================
// > Arm weapon (draw gun)
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ARM_WEAPON,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_FACE_IDEAL				0"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ARM"
	""
	"	Interrupts"
);

//=========================================================
// > Disarm weapon (holster gun)
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_DISARM_WEAPON,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_IDEAL			0"
	"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_DISARM"
	""
	"	Interrupts"
);

//=========================================================
// 	SCHED_HIDE_AND_RELOAD
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_HIDE_AND_RELOAD	,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_RELOAD"
	"		TASK_FIND_COVER_FROM_ENEMY	0"
	"		TASK_RUN_PATH				0"
	"		TASK_WAIT_FOR_MOVEMENT		0"
	"		TASK_REMEMBER				MEMORY:INCOVER"
	"		TASK_FACE_ENEMY				0"
	"		TASK_RELOAD					0"
	""
	"	Interrupts"
	"		COND_HEAVY_DAMAGE"
//	"		COND_HEAR_DANGER"
);

//=========================================================
// > Reload
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RELOAD,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_RELOAD				0"
	""
	"	Interrupts"
	"		COND_HEAVY_DAMAGE"
);

//=========================================================
// > Melee_Attack1
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_MELEE_ATTACK1,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_MELEE_ATTACK1		0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_OCCLUDED"
);

//=========================================================
// > Melee_Attack2
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_MELEE_ATTACK2,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	2"	// 2 = secondary attack
	"		TASK_MELEE_ATTACK2		0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_OCCLUDED"
);

//=========================================================
// > SpecialAttack1
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SPECIAL_ATTACK1,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_FACE_ENEMY				0"
	"		TASK_SPECIAL_ATTACK1		0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_OCCLUDED"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_HEAR_DANGER"
);

//=========================================================
// > SpecialAttack2
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SPECIAL_ATTACK2,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_SPECIAL_ATTACK2	0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_OCCLUDED"
	"		COND_NO_SECONDARY_AMMO"
	"		COND_HEAR_DANGER"
);

//=========================================================
// > ChaseEnemy
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CHASE_ENEMY,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_GET_PATH_TO_ENEMY			0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_UNREACHABLE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_TASK_FAILED"
	"		COND_LOST_ENEMY"
	"		COND_BETTER_WEAPON_AVAILABLE"
	"		COND_HEAR_DANGER"
);

//=========================================================
// > ChaseEnemy
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_TARGET_CHASE,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_GET_PATH_TO_TARGET			0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_UNREACHABLE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_TASK_FAILED"
	"		COND_LOST_ENEMY"
	"		COND_BETTER_WEAPON_AVAILABLE"
	"		COND_HEAR_DANGER"
);

//=========================================================
// > ChaseEnemyFailed
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CHASE_ENEMY_FAILED,

	"	Tasks"
	"		 TASK_STOP_MOVING					0"
	"		 TASK_WAIT							0.2"
	"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_STANDOFF"
	"		 TASK_SET_TOLERANCE_DISTANCE		24"
	"		 TASK_FIND_COVER_FROM_ENEMY			0"
	"		 TASK_RUN_PATH						0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	"		 TASK_REMEMBER						MEMORY:INCOVER"
	"		 TASK_FACE_ENEMY					0"
	"		 TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"	// Translated to cover
	"		 TASK_WAIT							1"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_BETTER_WEAPON_AVAILABLE"
);

//=========================================================
// > SCHED_BACK_AWAY_FROM_SAVE_POSITION
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_BACK_AWAY_FROM_SAVE_POSITION,

	"	Tasks"
	"		TASK_STOP_MOVING							0"
	"		TASK_SET_TOLERANCE_DISTANCE					24"
	"		TASK_FIND_BACKAWAY_FROM_SAVEPOSITION		0"
	"		TASK_RUN_PATH								0"
	"		TASK_WAIT_FOR_MOVEMENT						0"
	""
	"	Interrupts"
);

//=========================================================
// > BackAwayFromEnemy
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_BACK_AWAY_FROM_ENEMY,

	"	Tasks"
			// If I can't back away from the enemy try to get behind him
	"		TASK_STOP_MOVING							0"
	"		TASK_SET_TOLERANCE_DISTANCE					24"
	"		TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION	0"
	"		TASK_FIND_BACKAWAY_FROM_SAVEPOSITION		0"
	"		TASK_RUN_PATH								0"
	"		TASK_WAIT_FOR_MOVEMENT						0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
);

//=========================================================
// > SmallFlinch
//
//			played when minor damage is taken.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SMALL_FLINCH,

	"	Tasks"
	"		 TASK_REMEMBER				MEMORY:FLINCHED  "
	"		 TASK_STOP_MOVING			0"
	"		 TASK_SMALL_FLINCH			0"
	""
	"	Interrupts"
);

//=========================================================
// > Freeze
//
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_NPC_FREEZE,

	"	Tasks"
	"		 TASK_FREEZE				0"

	"	Interrupts"
	"		COND_NPC_UNFREEZE"
);

//=========================================================
// > Die
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_DIE,

	"	Tasks"
	"		 TASK_STOP_MOVING		0				 "
	"		 TASK_SOUND_DIE			0			 "
	"		 TASK_DIE				0			 "
	""
	"	Interrupts"
);

//=========================================================
// > Die
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_DIE_RAGDOLL,

	"	Tasks"
	"		 TASK_STOP_MOVING		0			 "
	"		 TASK_SOUND_DIE			0			 "
	""
	"	Interrupts"
);


//=========================================================
// > VictoryDance
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_VICTORY_DANCE,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_VICTORY_DANCE"
	"		TASK_WAIT				0"
	""
	"	Interrupts"
);

//=========================================================
// > Error
//=========================================================
//AI_DEFINE_SCHEDULE
//	Error
//
//Tasks
//	TASK_STOP_MOVING			0
//	TASK_WAIT_INDEFINITE		0
//
//Interrupts

//=========================================================
// > ScriptedWalk
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SCRIPTED_WALK,

	"	Tasks"
	"		 TASK_SET_TOLERANCE_DISTANCE		2"
	"		 TASK_PUSH_SCRIPT_ARRIVAL_ACTIVITY	0"
	"		 TASK_WALK_TO_TARGET				0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	"		 TASK_PLANT_ON_SCRIPT				0"
	"		 TASK_FACE_SCRIPT					0"
	"		 TASK_ENABLE_SCRIPT					0"
	"		 TASK_WAIT_FOR_SCRIPT				0"
	"		 TASK_PLAY_SCRIPT					0"
	"		 TASK_PLAY_SCRIPT_POST_IDLE			0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE "
	"		COND_HEAVY_DAMAGE"
);

//=========================================================
// > ScriptedRun
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SCRIPTED_RUN,

	"	Tasks"
	"		 TASK_SET_TOLERANCE_DISTANCE		2"
	"		 TASK_PUSH_SCRIPT_ARRIVAL_ACTIVITY	0"
	"		 TASK_RUN_TO_TARGET					0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	"		 TASK_PLANT_ON_SCRIPT				0"
	"		 TASK_FACE_SCRIPT					0"
	"		 TASK_ENABLE_SCRIPT					0"
	"		 TASK_WAIT_FOR_SCRIPT				0"
	"		 TASK_PLAY_SCRIPT					0"
	"		 TASK_PLAY_SCRIPT_POST_IDLE			0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE "
	"		COND_HEAVY_DAMAGE"
);

//=========================================================
// > ScriptedMoveCustom
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SCRIPTED_CUSTOM_MOVE,

	"	Tasks"
	"		 TASK_SET_TOLERANCE_DISTANCE		2"
	"		 TASK_PUSH_SCRIPT_ARRIVAL_ACTIVITY	0"
	"		 TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET	0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	"		 TASK_PLANT_ON_SCRIPT				0"
	"		 TASK_FACE_SCRIPT					0"
	"		 TASK_ENABLE_SCRIPT					0"
	"		 TASK_WAIT_FOR_SCRIPT				0"
	"		 TASK_PLAY_SCRIPT					0"
	"		 TASK_PLAY_SCRIPT_POST_IDLE			0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE "
	"		COND_HEAVY_DAMAGE"
);

//=========================================================
// > ScriptedWait
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SCRIPTED_WAIT,

	"	Tasks"
	"		 TASK_STOP_MOVING				0"
	"		 TASK_WAIT_FOR_SCRIPT			0"
	"		 TASK_PLAY_SCRIPT				0"
	"		 TASK_PLAY_SCRIPT_POST_IDLE		0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE "
	"		COND_HEAVY_DAMAGE"
);

//=========================================================
// > ScriptedFace
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SCRIPTED_FACE,

	"	Tasks"
	"		 TASK_STOP_MOVING				0"
	"		 TASK_FACE_SCRIPT				0"
	"		 TASK_WAIT_FOR_SCRIPT			0"
	"		 TASK_PLAY_SCRIPT				0"
	"		 TASK_PLAY_SCRIPT_POST_IDLE		0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE "
	"		COND_HEAVY_DAMAGE"
);


//=========================================================
// > SCENE_SEQUENCE
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SCENE_SEQUENCE,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_PLAY_SCRIPT				0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE "
	"		COND_HEAVY_DAMAGE"
);


//=========================================================
// > SCENE_WALK
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SCENE_WALK,

	"	Tasks"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_SET_ROUTE_SEARCH_TIME		3"	// Spend 3 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_TARGET			0"
	"		TASK_WALK_TO_TARGET				0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
);


//=========================================================
// > SCENE_FACE_TARGET
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SCENE_FACE_TARGET,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
	"		TASK_FACE_TARGET		0"
	""
	"	Interrupts"
);


//=========================================================
// > Cower
//
//		This is what is usually done when attempts
//		to escape danger fail.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_COWER,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_COWER"
	""
	"	Interrupts"
);

//=========================================================
// > TakeCoverFromOrigin
//
//			move away from where you're currently standing.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_TAKE_COVER_FROM_ORIGIN,

	"	Tasks"
	"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_FAIL_TAKE_COVER"
	"		 TASK_STOP_MOVING					0"
	"		 TASK_SET_TOLERANCE_DISTANCE		24"
	"		 TASK_FIND_COVER_FROM_ORIGIN		0"
	"		 TASK_RUN_PATH						0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	"		 TASK_REMEMBER						MEMORY:INCOVER"
	"		 TASK_TURN_LEFT						179"
	"		 TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"	// Translated to cover
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
);

//=========================================================
// > TakeCoverFromBestSound
//
//			hide from the loudest sound source
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_TAKE_COVER_FROM_BEST_SOUND,

	"	Tasks"
	"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_FLEE_FROM_BEST_SOUND"
	"		 TASK_STOP_MOVING					0"
	"		 TASK_SET_TOLERANCE_DISTANCE		24"
	"		 TASK_FIND_COVER_FROM_BEST_SOUND	0"
	"		 TASK_RUN_PATH						0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	"		 TASK_REMEMBER						MEMORY:INCOVER"
	"		 TASK_TURN_LEFT						179"
	"		 TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"	// Translated to cover
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
);


//=========================================================
//
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_FLEE_FROM_BEST_SOUND,

	"	Tasks"
	"		 TASK_MOVE_AWAY_PATH				600"
	"		 TASK_RUN_PATH_WITHIN_DIST			100"
	"		 TASK_STOP_MOVING					0"
	"		 TASK_TURN_LEFT						179"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
);


//=========================================================
// > TakeCoverFromEnemy
//
//		Take cover from enemy!
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_TAKE_COVER_FROM_ENEMY,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_FAIL_TAKE_COVER"
	"		TASK_STOP_MOVING				0"
	"		TASK_WAIT						0.2"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_FIND_COVER_FROM_ENEMY		0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_REMEMBER					MEMORY:INCOVER"
	"		TASK_FACE_ENEMY					0"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"	// Translated to cover
	"		TASK_WAIT						1"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_HEAR_DANGER"
);

//=========================================================
// FAIL_TAKE_COVER
//
//  Default case.  Overwritten by subclasses for behavior
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_FAIL_TAKE_COVER,

	"	Tasks "
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
);

//=========================================================
// > RunFromEnemy
//
//	Run to cover, but don't turn to face enemy and upon
//  fail run around randomly
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RUN_FROM_ENEMY,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_RUN_RANDOM"
	"		TASK_STOP_MOVING				0"
	"		TASK_WAIT						0.2"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_FIND_COVER_FROM_ENEMY		0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
);

//=========================================================
// > Fear_Face
//
// Face an enemy that I'm scared of, until I see it.  Used
// after I run to cover from a feared enemy
// UNDONE: Add a special ACT_IDLE_FEAR
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_FEAR_FACE,

	"	Tasks"
	"		 TASK_STOP_MOVING			0"
	"		 TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		 TASK_FACE_ENEMY			0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_SEE_ENEMY"
);

//=========================================================
// > Give_Way
//
// Get out of the way of someone that requested a move
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_GIVE_WAY,

	"	Tasks"
		//  Disable use of small hull for now.  Introduces movement bugs"
	//"		TASK_USE_SMALL_HULL				0"
	//"		TASK_REMEMBER					MEMORY:IN_SMALL_HULL"
	""
	"		TASK_SET_ROUTE_SEARCH_TIME		0.5"	// Spend 1/2 seconds trying to build a path if stuck
	"		TASK_SET_TOLERANCE_DISTANCE		5"
	"		TASK_GET_PATH_TO_SAVEPOSITION	2"
	"		TASK_RUN_PATH_TIMED				2.0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_GIVE_WAY"
	"		COND_WAY_CLEAR"
	"		COND_NEW_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_SMELL"
	"		COND_PROVOKED"
);

//=========================================================
// > Forced_Go (Used for debug only)
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_FORCED_GO,

	"	Tasks"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_SET_ROUTE_SEARCH_TIME		3"	// Spend 3 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_LASTPOSITION	0"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
);

//=========================================================
// > Forced_Go (Used for debug only)
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_FORCED_GO_RUN,

	"	Tasks"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_SET_ROUTE_SEARCH_TIME		3"	// Spend 3 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_LASTPOSITION	0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
);

//=========================================================
// ESTABLISH_LINE_OF_FIRE
//
//  Go to a location from which I can shoot my enemy
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ESTABLISH_LINE_OF_FIRE  ,

	"	Tasks "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_FAIL_ESTABLISH_LINE_OF_FIRE"
	"		TASK_GET_PATH_TO_ENEMY_LOS		0"
	"		TASK_SPEAK_SENTENCE				1"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
	""
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LOST_ENEMY"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
);

//=========================================================
// FAIL_ESTABLISH_LINE_OF_FIRE
//
//  Default case.  Overwritten by subclasses for behavior
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_FAIL_ESTABLISH_LINE_OF_FIRE  ,

	"	Tasks "
	""
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
	""
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LOST_ENEMY"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
);

//=========================================================
// > PATROL_RUN
//
// Run around randomly until we detect an enemy
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_PATROL_RUN,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_COMBAT_FACE"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_SET_ROUTE_SEARCH_TIME		5"	// Spend 5 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_RANDOM_NODE	200"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1 "
	"		COND_CAN_RANGE_ATTACK2 "
	"		COND_CAN_MELEE_ATTACK1 "
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_GIVE_WAY"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_PLAYER"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_SMELL"
	"		COND_PROVOKED"
);

//=========================================================
// > IDLE_WANDER
//
// Walk around randomly
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_IDLE_WANDER,

	"	Tasks"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_SET_ROUTE_SEARCH_TIME		5"	// Spend 5 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_RANDOM_NODE	200"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_WAIT_PVS					0"
	""
	"	Interrupts"
	"		COND_GIVE_WAY"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
);

//=========================================================
// > PATROL_WALK
//
// Walk around randomly until we detect an enemy
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_PATROL_WALK,

	"	Tasks"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_SET_ROUTE_SEARCH_TIME		5"	// Spend 5 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_RANDOM_NODE	200"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1 "
	"		COND_CAN_RANGE_ATTACK2 "
	"		COND_CAN_MELEE_ATTACK1 "
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_GIVE_WAY"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_PLAYER"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_SMELL"
	"		COND_PROVOKED"
);

//=========================================================
// > RUN_RANDOM
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_RUN_RANDOM,

	"	Tasks"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_SET_ROUTE_SEARCH_TIME		1"	// Spend 1 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_RANDOM_NODE	500"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
);

//=========================================================
// > FALL_TO_GROUND
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_FALL_TO_GROUND,

	"	Tasks"
	"		TASK_FALL_TO_GROUND				0"
	""
	"	Interrupts"
);

//=========================================================
// > DROPSHIP_DEPLOY
// Wait for a few seconds whilst the dropship door opens,
// then run down the ramp!
// nothing may interrupt this.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_DROPSHIP_DEPLOY_FIRST,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
	"		TASK_WAIT						1.0"
	"		TASK_WAIT_RANDOM				0.5"
	"		TASK_GET_DROPSHIP_DEPLOY_PATH	0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
);

//=========================================================
// Deploy second row
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_DROPSHIP_DEPLOY_SECOND,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
	"		TASK_WAIT						3.5"
	"		TASK_WAIT_RANDOM				0.5"
	"		TASK_GET_DROPSHIP_DEPLOY_PATH	0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
);

//=========================================================
// Deploy third row
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_DROPSHIP_DEPLOY_THIRD,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
	"		TASK_WAIT						5.0"
	"		TASK_WAIT_RANDOM				0.5"
	"		TASK_GET_DROPSHIP_DEPLOY_PATH	0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
);

//=========================================================
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_AWAIT_ORDERS,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE_ANGRY"
	"		TASK_WAIT_INDEFINITE		0"
	""
	"	Interrupts"
	"		COND_PLAYER_UNSELECTED"
	"		COND_RECEIVED_ORDERS"
);

//=========================================================
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_MOVE_TO_COMMAND_GOAL,

	"	Tasks"
	"		TASK_GET_PATH_TO_COMMAND_GOAL		0"
	"		TASK_RUN_PATH						ACTIVITY:ACT_FLINCH_PHYSICS"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_IGNORE_OLD_ENEMIES				0"
//	"		TASK_CLEAR_COMMAND_GOAL				0"
	""
	"	Interrupts"
	"		COND_HEAR_DANGER"
	"		COND_SEE_FEAR"
);

//=========================================================
// Flinch to protect self from incoming physics object
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_FLINCH_PHYSICS,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_FLINCH_PHYSICS"
	""
	"	Interrupts"
);

