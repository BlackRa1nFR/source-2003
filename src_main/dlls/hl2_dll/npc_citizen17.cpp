//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: The downtrodden citizens of City 17.
//
//=============================================================================

#include "cbase.h"
#include "AI_Default.h"
#include "AI_Task.h"
#include "AI_Schedule.h"
#include "AI_Node.h"
#include "AI_Hull.h"
#include "AI_Hint.h"
#include "AI_Interactions.h"
#include "AI_Senses.h"
#include "AI_Motor.h"
#include "ai_squad.h"
#include "npc_citizen17.h"
#include "soundent.h"
#include "game.h"
#include "NPCEvent.h"
#include "EntityList.h"
#include "activitylist.h"
#include "globalstate.h"
#include "bitstring.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "BasePropDoor.h"

#define		CIT_INSPECTED_DELAY_TIME 120  //How often I'm allowed to be inspected

static AI_StandoffParams_t AI_DEFAULT_CITIZEN_STANDOFF_PARAMS = { AIHCR_MOVE_ON_COVER, true, 1.5, 2.5, 1, 3, 25 };

extern ConVar max_command_dist;


ConVar	ai_debug_medic					( "ai_debug_medic",					"0");

ConVar	sk_citizen_heal_any_delay		( "sk_citizen_heal_any_delay",		"8");
ConVar	sk_citizen_heal_player			( "sk_citizen_heal_player",			"25");
ConVar	sk_citizen_heal_player_delay	( "sk_citizen_heal_player_delay",	"30");
ConVar	sk_citizen_heal_player_min_pct	( "sk_citizen_heal_player_min_pct",	"0.60");
ConVar	sk_citizen_heal_ally			( "sk_citizen_heal_ally",			"30");
ConVar	sk_citizen_heal_ally_delay		( "sk_citizen_heal_ally_delay",		"20");
ConVar	sk_citizen_heal_ally_min_pct	( "sk_citizen_heal_ally_min_pct",	"0.90");
const float HEAL_RANGE = 15*12;

//=========================================================
// Citizen activities
//=========================================================
int ACT_CIT_HANDSUP;
int ACT_CIT_HANDSDOWN;
int ACT_CIT_HANDSUPIDLE;
int	ACT_CIT_BLINDED;
int ACT_CIT_SHOWARMBAND;
int ACT_CIT_COWERFLINCH;
int ACT_CIT_COWERIDLE;
int ACT_CIT_COWERSTAND;
int ACT_CIT_COWER;
int ACT_CIT_IDLEWALL;
int ACT_CIT_LEANONWALL;
int ACT_CIT_LEANOFFWALL;
int ACT_CIT_MANHACKFLINCH;
int ACT_CIT_HEAL;

//=========================================================
// Static Data
//=========================================================
ConVar	sk_citizen_health( "sk_citizen_health","0");


LINK_ENTITY_TO_CLASS( npc_citizen, CNPC_Citizen );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------

BEGIN_DATADESC( CNPC_Citizen )

	DEFINE_FIELD( CNPC_Citizen, m_nInspectActivity, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Citizen, m_flNextFearSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Citizen, m_flStopManhackFlinch, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Citizen, m_fNextInspectTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Citizen, m_flHealTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Citizen, m_flPlayerHealTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Citizen, m_flAllyHealTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Citizen, m_hCurLookTarget, FIELD_EHANDLE ),
//	DEFINE_FIELD( CNPC_Citizen, m_nextHead, FIELD_INTEGER ),
//	DEFINE_FIELD( CNPC_Citizen, m_AssaultBehavior, FIELD_EMBEDDED ),
//	DEFINE_FIELD( CNPC_Citizen, m_FollowBehavior, FIELD_EMBEDDED ),
//	DEFINE_FIELD( CNPC_Citizen, m_StandoffBehavior, FIELD_EMBEDDED ),

	DEFINE_USEFUNC( CNPC_Citizen, Use ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Citizen::Precache( void )
{
	if( !GetModelName() )
	{
		// Level designer has not provided a default
		// Defaulting to Ted makes it most likely the
		// designer will correct the problem promptly.
		SetModelName( MAKE_STRING( "models/Humans/male_01.mdl" ) );
	}

	engine->PrecacheModel( STRING( GetModelName() ) );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
string_t CNPC_Citizen::GetModelName( void ) const
{
	string_t iszModelName = BaseClass::GetModelName();

	//
	// If the model refers to an obsolete model, pretend it was blank
	// so that we pick the new default model.
	//
	if (!strnicmp(STRING(iszModelName), "models/c17_", 11) ||
		!strnicmp(STRING(iszModelName), "models/male", 11) ||
		!strnicmp(STRING(iszModelName), "models/female", 13) ||
		!strnicmp(STRING(iszModelName), "models/citizen", 14))
	{
		return NULL_STRING;
	}

	return iszModelName;
}


//-----------------------------------------------------------------------------
// Not really random. Runs in a circuit.
//-----------------------------------------------------------------------------
void CNPC_Citizen::SelectCitizenModel()
{
	if( HasSpawnFlags( SF_CITIZEN_RANDOM_HEAD ) )
	{
		switch( m_nextHead )
		{
		case 0:
			SetModelName(MAKE_STRING("models/humans/male_01.mdl"));
			break;
		case 1:
			SetModelName(MAKE_STRING("models/humans/male_02.mdl"));
			break;
		case 2:
			SetModelName(MAKE_STRING("models/humans/male_03.mdl"));
			break;
		case 3:
			SetModelName(MAKE_STRING("models/humans/male_07.mdl"));
			break;
		case 4:
			SetModelName(MAKE_STRING("models/humans/male_08.mdl"));
			break;
		case 5:
			SetModelName(MAKE_STRING("models/humans/male_09.mdl"));
			break;
		}

		m_nextHead++;

		if( m_nextHead > 5 )
		{
			m_nextHead = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
int CNPC_Citizen::m_nextHead = 0;
void CNPC_Citizen::Spawn( void )
{
	SelectCitizenModel();

	Precache();

	SetModel( STRING( GetModelName() ) );

	if( m_spawnflags & SF_CITIZEN_MEDIC )
	{
		m_nSkin = 1;
	}

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= sk_citizen_health.GetFloat();
	m_flFieldOfView		= 0.02;
	m_NPCState		= NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_SQUAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_CLIMB );
	CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
	CapabilitiesAdd( bits_CAP_USE_WEAPONS | bits_CAP_AIM_GUN | bits_CAP_MOVE_SHOOT | bits_CAP_NO_HIT_PLAYER );
	CapabilitiesAdd( bits_CAP_DUCK | bits_CAP_DOORS_GROUP );

	m_flStopManhackFlinch = -1;

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_iszIdleExpression = MAKE_STRING("scenes/Expressions/CitizenIdle.vcd");
	m_iszAlertExpression = MAKE_STRING("scenes/Expressions/CitizenAlert.vcd");
	m_iszCombatExpression = MAKE_STRING("scenes/Expressions/CitizenCombat.vcd");

	NPCInit();

	m_szFriends[0] = "npc_citizen";
	m_szFriends[1] = "npc_barney";
	m_szFriends[2] = "npc_alyx";
}

void CNPC_Citizen::PostNPCInit( void )
{
	if( m_spawnflags & SF_CITIZEN_FOLLOW )
	{
		m_FollowBehavior.SetFollowTarget( UTIL_PlayerByIndex( 1 ) );
	}

	BaseClass::PostNPCInit();
}

bool CNPC_Citizen::CreateBehaviors()
{
	AddBehavior( &m_AssaultBehavior );
	AddBehavior( &m_StandoffBehavior );
	AddBehavior( &m_FollowBehavior );
	
	return BaseClass::CreateBehaviors();
}


//-----------------------------------------------------------------------------
// Purpose: Overridden to switch our behavior between passive and rebel. We
//			become combative after Gordon becomes a criminal.
//-----------------------------------------------------------------------------
Class_T	CNPC_Citizen::Classify( void )
{
	if (GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON)
	{
		return CLASS_CITIZEN_PASSIVE;
	}

	return CLASS_PLAYER_ALLY;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Citizen::FCanCheckAttacks( void )
{
	if( GetEnemy() && FClassnameIs( GetEnemy(), "proto_sniper" ) )
	{
		// Don't attack the sniper. Just know about him.
		return false;
	}

	return BaseClass::FCanCheckAttacks();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Citizen::GetSoundInterests ( void )
{
	return	SOUND_WORLD			|
			SOUND_COMBAT		|
			SOUND_PLAYER		|
			SOUND_DANGER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Citizen::LocateEnemySound()
{
#if 0
	if ( !GetEnemy() )
		return;

	float flZDiff = GetLocalOrigin().z - GetEnemy()->GetLocalOrigin().z;

	if( flZDiff < -128 )
	{
		EmitSound( "NPC_Citizen.UpThere" );
	}
	else if( flZDiff > 128 )
	{
		EmitSound( "NPC_Citizen.DownThere" );
	}
	else
	{
		EmitSound( "NPC_Citizen.OverHere" );
	}
#endif
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Citizen::DeathSound ( void )
{
	if ( ai_debug_medic.GetBool() && ( m_spawnflags & SF_CITIZEN_MEDIC ) )
		Msg( "Medic died\n" );

	// Sentences don't play on dead NPCs
	SentenceStop();

	EmitSound( "NPC_Citizen.Die" );

	int lifeState = m_lifeState;
	m_lifeState = LIFE_ALIVE;
	m_lifeState = lifeState;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Citizen::PainSound ( void )
{
	EmitSound( "npc_citizen.die" );
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   : 
// Output  : 
//------------------------------------------------------------------------------
void CNPC_Citizen::FearSound ( void )
{
}


bool CNPC_Citizen::CanHeal( void )
{ 
	if ( !( m_spawnflags & SF_CITIZEN_MEDIC ) )
		return false;

	if ( m_flHealTime > gpGlobals->curtime)
		return false;

	if ( m_NPCState == NPC_STATE_SCRIPT )
		return false;

	return true;
}

bool CNPC_Citizen::ShouldHealTarget( CBaseEntity *pTarget, bool bActiveUse )
{
	if ( pTarget )
	{
		if ( pTarget->IsPlayer() )
		{
			if ( m_flPlayerHealTime > gpGlobals->curtime)
			{
				if ( ai_debug_medic.GetBool() )
					Msg( this, AIMF_IGNORE_SELECTED, "Medic %f sec to heal player\n", gpGlobals->curtime - m_flPlayerHealTime );
				return false;
			}
		}
		else
		{
			if ( m_flAllyHealTime > gpGlobals->curtime)
			{
				if ( ai_debug_medic.GetBool() )
					Msg( this, AIMF_IGNORE_SELECTED, "Medic %f sec to heal player\n", gpGlobals->curtime - m_flPlayerHealTime );
				return false;
			}
		}
		
		float requiredHealthPct = ( pTarget->IsPlayer() ) ? sk_citizen_heal_player_min_pct.GetFloat() : sk_citizen_heal_ally_min_pct.GetFloat();
		int requiredHealth = pTarget->m_iMaxHealth * requiredHealthPct;
		return ( ( bActiveUse || pTarget->m_iHealth <= requiredHealth ) && IRelationType( pTarget ) == D_LI );
	}
	return false;
}

void CNPC_Citizen::Heal( void )
{
	if ( !CanHeal() )
		  return;

	CBaseEntity *pTarget = GetTarget();

	Vector target = pTarget->GetAbsOrigin() - GetAbsOrigin();
	if ( target.Length() > 100 )
		return;

	m_flHealTime = gpGlobals->curtime + sk_citizen_heal_any_delay.GetFloat();
	if ( pTarget->IsPlayer() )
	{
		pTarget->TakeHealth( sk_citizen_heal_player.GetFloat(), DMG_GENERIC );
		m_flPlayerHealTime = gpGlobals->curtime + sk_citizen_heal_player_delay.GetFloat();
		if ( ai_debug_medic.GetBool() )
			Msg( this, AIMF_IGNORE_SELECTED, "Medic healing player for %d points at %f, next at %f\n", sk_citizen_heal_player.GetInt(), gpGlobals->curtime, m_flPlayerHealTime );
	}
	else
	{
		pTarget->TakeHealth( sk_citizen_heal_ally.GetFloat(), DMG_GENERIC );
		pTarget->RemoveAllDecals();
		m_flAllyHealTime = gpGlobals->curtime + sk_citizen_heal_ally_delay.GetFloat();
		if ( ai_debug_medic.GetBool() )
			Msg( this, AIMF_IGNORE_SELECTED, "Medic healing ally NPC for %d points at %f, next at %f\n", sk_citizen_heal_player.GetInt(), gpGlobals->curtime, m_flAllyHealTime );
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Citizen::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case CITIZEN_AE_HEAL:		// Heal my target (if within range)
		Heal();
		break;
	case NPC_EVENT_LEFTFOOT:
		{
			EmitSound( "NPC_Citizen.FootstepLeft" );
		}
		break;
	case NPC_EVENT_RIGHTFOOT:
		{
			EmitSound( "NPC_Citizen.FootstepRight" );
		}
		break;

	case EVENT_WEAPON_RELOAD:
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->WeaponSound( RELOAD_NPC );
			GetActiveWeapon()->m_iClip1 = GetActiveWeapon()->GetMaxClip1(); 
			ClearCondition(COND_LOW_PRIMARY_AMMO);
			ClearCondition(COND_NO_PRIMARY_AMMO);
			ClearCondition(COND_NO_SECONDARY_AMMO);
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Citizen::SelectHealSchedule()
{
	if ( CanHeal() )
	{
		CBaseEntity *pEntity = PlayerInRange( GetLocalOrigin(), HEAL_RANGE );
		if ( pEntity && ShouldHealTarget( pEntity, HasCondition( COND_CIT_PLAYERHEALREQUEST ) ) )
		{
			SetTarget( pEntity );
			return SCHED_CITIZEN_HEAL;
		}
		
		if ( m_pSquad )
		{
			pEntity = NULL;
			float distClosestSq = HEAL_RANGE*HEAL_RANGE;
			float distCurSq;
			
			AISquadIter_t iter;
			CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
			while ( pSquadmate )
			{
				if ( pSquadmate != this )
				{
					distCurSq = ( GetAbsOrigin() - pSquadmate->GetAbsOrigin() ).LengthSqr();
					if ( distCurSq < distClosestSq && ShouldHealTarget( pSquadmate ) )
					{
						distClosestSq = distCurSq;
						pEntity = pSquadmate;
					}
				}

				pSquadmate = m_pSquad->GetNextMember( &iter );
			}
			
			if ( pEntity )
			{
				SetTarget( pEntity );
				return SCHED_CITIZEN_HEAL;
			}
		}
	}
	else
	{
		if ( HasCondition( COND_CIT_PLAYERHEALREQUEST ) )
			Msg( "Would say: sorry, need to recharge\n" );
	}
	
	return SCHED_NONE;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Citizen::SelectSchedule ( void )
{
	if( HasCondition( COND_HEAR_DANGER ) )
	{
		CSound *pSound;
		pSound = GetBestSound();

		ASSERT( pSound != NULL );

		if ( pSound && (pSound->m_iType & SOUND_DANGER) )
		{
			Speak( TLK_DANGER );
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
		}
	}
	
	bool bHaveCommandGoal = (GetCommandGoal() != vec3_invalid);
	
	int schedule = SelectHealSchedule();
	if ( schedule != SCHED_NONE )
		return schedule;

	if ( bHaveCommandGoal && VeryFarFromCommandGoal() )	
		return SCHED_MOVE_TO_COMMAND_GOAL;

	if ( !bHaveCommandGoal && 
		m_FollowBehavior.CanSelectSchedule() && m_FollowBehavior.FarFromFollowTarget() && 
		( !m_StandoffBehavior.CanSelectSchedule() || 
		  m_StandoffBehavior.IsBehindBattleLines( m_FollowBehavior.GetFollowTarget()->GetAbsOrigin() ) ) )
	{
		DeferSchedulingToBehavior( &m_FollowBehavior );
	}
	else if ( !BehaviorSelectSchedule() )
	{
		if( bHaveCommandGoal && !NearCommandGoal() )
		{
			return SCHED_MOVE_TO_COMMAND_GOAL;
		}

		switch ( m_NPCState )
		{
		case NPC_STATE_IDLE:
			break;

		case NPC_STATE_ALERT:
			break;

		case NPC_STATE_COMBAT:

/*
			// If I have an enemy and I'nt scared of him make sure I have a weapon
			if (GetEnemy() )//&& IRelationType( GetEnemy() ) != D_FR)
			{
				if ( !GetActiveWeapon() && !HasCondition( COND_TASK_FAILED ) && HasCondition(COND_BETTER_WEAPON_AVAILABLE) )
				{
					return SCHED_NEW_WEAPON;
				}
			}
*/

			if ( HasCondition ( COND_NO_PRIMARY_AMMO ) )
			{
#if 0
				return SCHED_HIDE_AND_RELOAD;
#else
				// @TODO (toml 05-07-03): e3-03 hack. prevent SW citizens from running inside during closing scene. remove afterwards
				
				return SCHED_RELOAD;
#endif
			}

			break;
		}
	}

	return BaseClass::SelectSchedule();
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Citizen::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		return SCHED_CITIZEN_TAKE_COVER_FROM_BEST_SOUND;

	case SCHED_FLEE_FROM_BEST_SOUND:
		return SCHED_CITIZEN_FLEE_FROM_BEST_SOUND;

	case SCHED_CHASE_ENEMY:
		if( GetCommandGoal() != vec3_invalid )
		{
			return SCHED_STANDOFF;
		}

		if ( GetEnemy() && GetEnemy()->ClassMatches( "npc_turret_*" ) )
			return SCHED_TAKE_COVER_FROM_ENEMY;

		return SCHED_CHASE_ENEMY;

	case SCHED_RANGE_ATTACK1:
		if ( GetEnemy() && GetEnemy()->ClassMatches( "npc_turret_*" ) )
		{
			if ( GetEnemy()->ClassMatches( "npc_turret_floor" ) )
			{
				Vector dir = GetAbsOrigin() - GetEnemy()->EyePosition();
				float dist = VectorNormalize( dir );

				if ( dist > 4.0*12.0 )
				{
					Vector forward;
					GetEnemy()->GetVectors( &forward, NULL, NULL );

					if ( fabs( forward.Dot( dir ) ) < 0.42 )
						break;
				}
			}
			return SCHED_TAKE_COVER_FROM_ENEMY;
		}
	case SCHED_FAIL_TAKE_COVER:
		if ( GetEnemy() && GetEnemy()->ClassMatches( "npc_turret_*" ) )
		{
			return SCHED_CITIZEN_FAIL_TAKE_COVER_TURRET;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Citizen::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if( gpGlobals->curtime > m_flStopManhackFlinch )
	{
		ClearCondition( COND_CIT_HURTBYMANHACK );
	}
	else
	{
		SetCondition( COND_CIT_HURTBYMANHACK );
	}

}

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Citizen::ShouldStandoff( void )
{
	// Short-circuiting this for now, until we have a plan
	// for NPC-initiated behaviour selection. (sjb)
	return false;

	if( HasCondition( COND_ENEMY_UNREACHABLE ) )
	{
		return true;
	}

	return false;
}
#endif

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Citizen::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	if( HasCondition( COND_HEAR_DANGER ) )
	{
		CSound *pBestSound;
		pBestSound = GetBestSound();
		Assert( pBestSound != NULL );
		
		if( pBestSound && !FVisible( pBestSound->GetSoundOrigin() ) )
		{
			// Can't see this sound, so don't worry about it.
			ClearCondition( COND_HEAR_DANGER );
		}
	}

	if( IsSelected() )
	{
		// For now, just draw this box. Done this way so that
		// it works irrespective of how DEVELOPER is set.
		NDebugOverlay::EntityBounds(this, 255, 100, 0, 0 ,0.1);
	}

	if( GetCommandGoal() != vec3_invalid )
	{
		NDebugOverlay::Line( GetAbsOrigin() + Vector( 0, 0, 8 ), GetCommandGoal() + Vector( 0, 0, 8 ), 0,255,0, false, 0.05 );
	}

#if 0
	if( ShouldStandoff() && !m_StandoffBehavior.IsActive() )
	{
		m_StandoffBehavior.SetActive( true );
		m_StandoffBehavior.SetTestNoDamage( false );
		m_StandoffBehavior.SetParameters( AI_DEFAULT_CITIZEN_STANDOFF_PARAMS );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CNPC_Citizen::NPC_TranslateActivity( Activity activity )
{
	if ( activity == ACT_RUN && ( IsCurSchedule( SCHED_TAKE_COVER_FROM_BEST_SOUND ) || IsCurSchedule( SCHED_FLEE_FROM_BEST_SOUND ) ) )
	{
		if ( random->RandomInt( 0, 1 ) && HaveSequenceForActivity( ACT_RUN_PROTECTED ) )
			activity = ACT_RUN_PROTECTED;
	}

	activity = BaseClass::NPC_TranslateActivity( activity );
	if ( activity == ACT_IDLE && (m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT) )
	{
		if (gpGlobals->curtime - m_flLastAttackTime < 3)
		{
			activity = ACT_IDLE_ANGRY;
		}
	}

	return activity;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the activity for the given hint type.
// Input  : sHintType - 
//-----------------------------------------------------------------------------
Activity CNPC_Citizen::GetHintActivity( short sHintType )
{
	if ( sHintType == HINT_HEALTH_KIT )
	{
		// TODO: We don't have an activity for this, so just idle for a bit
		return ACT_IDLE_ANGRY;
	}

	return BaseClass::GetHintActivity( sHintType );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Citizen::SetLookTarget(CBaseEntity *pLookTarget)
{
	m_hCurLookTarget = pLookTarget;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_Citizen::EyeLookTarget()
{
	if (m_hCurLookTarget != NULL)
	{
		return m_hCurLookTarget;
	}

	return BaseClass::EyeLookTarget();
}


//---------------------------------------------------------

bool CNPC_Citizen::gm_bFindingTurretCover;

bool CNPC_Citizen::FindCoverPos( CBaseEntity *pEntity, Vector *pResult)
{
	gm_bFindingTurretCover = pEntity->ClassMatches( "npc_turret_floor" );
	bool result = BaseClass::FindCoverPos( pEntity, pResult );
	gm_bFindingTurretCover = false;
	return result;
}

bool CNPC_Citizen::IsCoverPosition( const Vector &vecThreat, const Vector &vecPosition )
{
	if ( gm_bFindingTurretCover && GetEnemy() )
	{
		Vector dir = vecPosition - vecThreat;
		float dist = VectorNormalize( dir );

		if ( dist > 4.0*12.0 )
		{
			Vector forward;
			GetEnemy()->GetVectors( &forward, NULL, NULL );

			if ( forward.Dot( dir ) < 0.42 )
				return true;
		}
	}

	return BaseClass::IsCoverPosition( vecThreat, vecPosition );
}

bool CNPC_Citizen::ValidateNavGoal()
{
	bool result;
	if ( GetNavigator()->GetGoalType() == GOALTYPE_COVER && GetEnemy() && GetEnemy()->ClassMatches( "npc_turret_floor" ) )
		gm_bFindingTurretCover = true;
	result = BaseClass::ValidateNavGoal();
	gm_bFindingTurretCover = false;
	return result;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Citizen::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SOUND_WAKE:
		LocateEnemySound();
		m_flWaitFinished = gpGlobals->curtime + 0.5;
		break;

	case TASK_CIT_DEFER_MANHACKFLINCH:
		{
			//m_flNextManhackFlinch = gpGlobals->curtime + pTask->flTaskData;
			TaskComplete();
			break;
		}

	case TASK_CIT_GET_LOOK_TARGET:
		{
			// In order, Citizens want to look at 
			// Player
			// Metropolice
			// Other Citizen
			AISightIter_t iter;
			CBaseEntity *pEnt;

			SetLookTarget( NULL );
			pEnt = GetSenses()->GetFirstSeenEntity( &iter );

			while( pEnt )
			{
				// Search all of the NPCs visible to me and find 
				// a metrocop.
				if ( FClassnameIs( pEnt, "NPC_metropolice" ) )
				{
					SetLookTarget(pEnt);
					break;
				}

				pEnt = GetSenses()->GetNextSeenEntity( &iter );
			}
		}
		TaskComplete();
		break;

	case TASK_CIT_TARGET_POLICE:
		{
			AISightIter_t iter;
			CBaseEntity *pEnt;
			CAI_BaseNPC *pNPC;
			CAI_BaseNPC *pNPCLook = NULL;

			pEnt = GetSenses()->GetFirstSeenEntity( &iter );

			while( pEnt )
			{
				// Search all of the NPCs visible to me and find 
				// a metrocop.
				if( FClassnameIs( pEnt, "NPC_metropolice" ) )
				{
					pNPC = pEnt->MyNPCPointer();

					if( pNPC )
					{
						// Now, if this metrocop can't see me, make him my target!
						// Later, target the nearest cop.
						if( !pNPC->FInViewCone( this ) )
						{
							// This officer can't see me. Target him.
							pNPCLook = pNPC;
							break;
						}
						else
						{
							// Oops! This officer can see me. Don't target any
							// officers for taunts/attacks, or I'll be caught.
							break;
						}
					}
				}

				pEnt = GetSenses()->GetNextSeenEntity( &iter );
			}

			SetLookTarget(pNPCLook);

			if (pNPCLook)
			{
				TaskComplete();
			}
			else
			{
				TaskFail(FAIL_NO_TARGET);
			}

			break;
		}

	case TASK_CIT_FACE_GROUP:
		TaskComplete();
		break;

	case TASK_CIT_HEAL:
		Speak( "TLK_HEAL" );
		SetIdealActivity( (Activity)ACT_CIT_HEAL );
		break;
	
	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Citizen::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
		case TASK_SOUND_WAKE:
			if( gpGlobals->curtime > m_flWaitFinished )
			{
				TaskComplete();
			}
			break;

		case TASK_CIT_HEAL:
			if ( IsSequenceFinished() )
			{
				TaskComplete();
			}
			else
			{
				if ( ( GetTarget()->GetAbsOrigin() - GetAbsOrigin() ).Length() > HEAL_RANGE/2 )
					TaskComplete();

				GetMotor()->SetIdealYawToTargetAndUpdate( GetTarget()->GetAbsOrigin() );
			}
			break;
		default:
			BaseClass::RunTask( pTask );
			break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Citizen :: FValidateHintType ( CAI_Hint *pHint )
{
	switch( pHint->HintType() )
	{
	case HINT_URBAN_SHELTER:
		return true;
		break;

	case HINT_FOLLOW_WAIT_POINT:
		return true;
		break;

	case HINT_HEALTH_KIT:
		return true;
		break;

	default:
		return false;
		break;
	}

	return FALSE;
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Citizen::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if( info.GetAttacker() )
	{
		if( FClassnameIs( info.GetAttacker(), "npc_manhack" ) )
		{
				m_flStopManhackFlinch = gpGlobals->curtime + 1.5;
				SetCondition( COND_CIT_HURTBYMANHACK );
		}
	}

	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
bool CNPC_Citizen::PassesDamageFilter( CBaseEntity *pAttacker )
{
#ifdef IMPERVIOUS_TO_PLAYER
	if (pAttacker->IsPlayer())
	{
		m_fNoDamageDecal = true;
		return false;
	}
#endif

	return BaseClass::PassesDamageFilter( pAttacker );
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_Citizen::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if ( IsCurSchedule(SCHED_RANGE_ATTACK1) )
	{
		// Interrupt any schedule except for manhack flinch
		SetCustomInterruptCondition( COND_PLAYER_PUSHING );
	}

	if ( ConditionInterruptsCurSchedule( COND_GIVE_WAY ) )
	{
		SetCustomInterruptCondition( COND_CIT_PLAYERHEALREQUEST );
	}

	SetCustomInterruptCondition( COND_RECEIVED_ORDERS );
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CNPC_Citizen::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

#if 0
	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// Print state
		char tempstr[512];
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;
	}
#endif
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Citizen::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	// TODO:  As citizen gets more complex, we will have to only allow
	//		  these interruptions to happen from certain schedules
	if (interactionType ==	g_interactionScannerInspect)
	{
		if (gpGlobals->curtime > m_fNextInspectTime)
		{
			SetLookTarget(sourceEnt);

			// Don't let anyone else pick me for a couple seconds
			m_fNextInspectTime  = gpGlobals->curtime + 5.0;
			return true;
		}
		return false;
	}
	else if (interactionType ==	g_interactionScannerInspectBegin)
	{
		// Don't inspect me again for a while
		m_fNextInspectTime  = gpGlobals->curtime + CIT_INSPECTED_DELAY_TIME;
		m_nInspectActivity = ACT_CIT_BLINDED;
		SetSchedule(SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY);
		return true;
	}
	else if (interactionType ==	g_interactionScannerInspectHandsUp)
	{
		m_nInspectActivity = ACT_CIT_HANDSUP;
		SetSchedule(SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY);
		return true;
	}
	else if (interactionType ==	g_interactionScannerInspectShowArmband)
	{
		m_nInspectActivity = ACT_CIT_SHOWARMBAND;
		SetSchedule(SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY);
		return true;
	}
	else if (interactionType ==	g_interactionScannerInspectDone)
	{
		SetSchedule(SCHED_IDLE_WANDER);
		return true;
	}
	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Citizen::Touch( CBaseEntity *pOther )
{
	BaseClass::Touch( pOther );

	// Did the player touch me?
	if ( pOther->IsPlayer() )
	{
		// Ignore if pissed at player
		if ( m_afMemory & bits_MEMORY_PROVOKED )
			return;

		// Stay put during speech, UNLESS I'm fighting
		if ( GetExpresser() && GetExpresser()->IsSpeaking() && m_NPCState != NPC_STATE_COMBAT )
			return;
			
		TestPlayerPushing( pOther );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Citizen::FInAimCone( const Vector &vecSpot )
{
	Vector los = ( vecSpot - GetAbsOrigin() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = BodyDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > 0.866 ) //30 degrees
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Turn off following before processing a move order.
//-----------------------------------------------------------------------------
void CNPC_Citizen::MoveOrder( const Vector &vecDest )
{
	if( m_StandoffBehavior.IsRunning() )
	{
		m_StandoffBehavior.SetStandoffGoalPosition( vecDest );
	}

	// If in assault, cancel and move.
	if( m_AssaultBehavior.HasHitRallyPoint() && !m_AssaultBehavior.HasHitAssaultPoint() )
	{
		m_AssaultBehavior.Disable();
		ClearSchedule();
	}

#if 0
	float flDist = ( GetAbsOrigin() - UTIL_PlayerByIndex( 1 )->GetAbsOrigin() ).Length();
	if( flDist >= max_command_dist.GetFloat() )
	{
		// Turn down the orders, too far away.
		NDebugOverlay::EntityBounds(255, 0, 0, 0 ,0.5);
		return;
	}
#endif 0

	if( m_FollowBehavior.GetFollowTarget() )
	{
		m_FollowBehavior.SetFollowTarget( NULL );
	}

	BaseClass::MoveOrder( vecDest );
}

//-----------------------------------------------------------------------------
// Purpose: return TRUE if the commander mode should try to give this order
//			to more people. return FALSE otherwise. For instance, we don't
//			try to send all 3 selectedcitizens to pick up the same gun.
//-----------------------------------------------------------------------------
bool CNPC_Citizen::TargetOrder( CBaseEntity *pTarget )
{
// Forget about any move orders
	SetCommandGoal( vec3_invalid );

	if( m_StandoffBehavior.IsRunning() )
	{
		m_StandoffBehavior.ClearStandoffGoalPosition();
	}

// First, try to pick up a weapon.
	CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon *>( pTarget );
	if( pWeapon && IsSelected() )
	{
		// Player is directing me to retrieve a weapon.

		if( GetActiveWeapon() )
		{
			Weapon_Drop( GetActiveWeapon() );
		}

		SetTarget( pWeapon );
		SetSchedule( SCHED_NEW_WEAPON );
		
		// Return false so this order isn't given to any more people.
		return false;
	}

	CBasePropDoor *pDoor = dynamic_cast<CBasePropDoor *>( pTarget );
	if( pDoor )
	{
		// Target is a door. 
		
		// Interpret as an assault go-code if I'm in assault, even if not selected.
		if( m_AssaultBehavior.HasHitRallyPoint() && !m_AssaultBehavior.HasHitAssaultPoint() )
		{
			// Send whatever cue assault wants.
			m_AssaultBehavior.ReceiveAssaultCue( CUE_COMMANDER );
			return true;
		}

		// Stack up to assault. IF I'm selected.
		// Find a free Rallypoint near the door.
		if( IsSelected() )
		{
			CRallyPoint *pRallyPoint = m_AssaultBehavior.FindBestRallyPointInRadius( pDoor->WorldSpaceCenter(), 256 );

			if( pRallyPoint )
			{
				m_AssaultBehavior.SetParameters( pRallyPoint, CUE_COMMANDER );
			}
			else
			{
				Msg("Can't find rally point near door!\n");
			}
		}

		return true;	
	}

	CAI_BaseNPC *pTargetNpc = pTarget->MyNPCPointer();
	if ( pTargetNpc && IsSelected() )
	{
		CAI_FollowBehavior *pTargetFollow;

		if ( this == pTargetNpc )
		{
			// I'm the target! Toggle follow!
			if( m_FollowBehavior.GetFollowTarget() )
			{
				// Turn follow off!
				Speak( TLK_UNUSE );
				m_FollowBehavior.SetFollowTarget( NULL );
			}
			else
			{
				// Turn follow on!
				m_AssaultBehavior.Disable();

				Speak( TLK_USE );
				m_FollowBehavior.SetFollowTarget( UTIL_PlayerByIndex( 1 ) );
			}
		}
		else if ( pTargetNpc->GetBehavior( &pTargetFollow ) )
		{
			// Target NPC is always called first, so just adopt that guys disposition
			m_FollowBehavior.SetFollowTarget( pTargetFollow->GetFollowTarget() );
		}
	}
	else if( pTarget->IsPlayer() && IsSelected() )
	{
		// Toggle follow!
		if( m_FollowBehavior.GetFollowTarget() )
		{
			// Turn follow off!
			Speak( TLK_UNUSE );
			m_FollowBehavior.SetFollowTarget( NULL );
		}
		else
		{
			// Turn follow on!
			m_AssaultBehavior.Disable();

			Speak( TLK_USE );
			m_FollowBehavior.SetFollowTarget( UTIL_PlayerByIndex( 1 ) );
		}
	}

	return true;
}

void CNPC_Citizen::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( pActivator->IsPlayer() )
	{
		SetCondition( COND_CIT_PLAYERHEALREQUEST );
	}
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_citizen, CNPC_Citizen )

	DECLARE_TASK(		TASK_CIT_TARGET_POLICE)
	DECLARE_TASK(		TASK_CIT_GET_LOOK_TARGET)
	DECLARE_TASK(		TASK_CIT_FACE_GROUP)
	DECLARE_TASK(		TASK_CIT_PLAY_INSPECT_ACT)
	DECLARE_TASK(		TASK_CIT_DEFER_MANHACKFLINCH)
	DECLARE_TASK(		TASK_CIT_HEAL)

	DECLARE_ACTIVITY(	ACT_CIT_HANDSUP)
	DECLARE_ACTIVITY(	ACT_CIT_HANDSDOWN)
	DECLARE_ACTIVITY(	ACT_CIT_HANDSUPIDLE)
	DECLARE_ACTIVITY(	ACT_CIT_BLINDED)
	DECLARE_ACTIVITY(	ACT_CIT_SHOWARMBAND)
	DECLARE_ACTIVITY(	ACT_CIT_COWERFLINCH)
	DECLARE_ACTIVITY(	ACT_CIT_COWERIDLE)
	DECLARE_ACTIVITY(	ACT_CIT_COWERSTAND)
	DECLARE_ACTIVITY(	ACT_CIT_COWER)
	DECLARE_ACTIVITY(	ACT_CIT_IDLEWALL)
	DECLARE_ACTIVITY(	ACT_CIT_LEANONWALL)
	DECLARE_ACTIVITY(	ACT_CIT_LEANOFFWALL)
	DECLARE_ACTIVITY(	ACT_CIT_MANHACKFLINCH)
	DECLARE_ACTIVITY(	ACT_CIT_HEAL)

	DECLARE_CONDITION( COND_CIT_HURTBYMANHACK )
	DECLARE_CONDITION( COND_CIT_PLAYERHEALREQUEST )

	//=========================================================
	// > SCHED_CITIZEN_STAND_LOOK
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_CITIZEN_STAND_LOOK,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_CIT_GET_LOOK_TARGET	0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_WAIT					4"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_GIVE_WAY"
	)

	//=========================================================
	// > SCHED_CITIZEN_COWER
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_CITIZEN_COWER,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_CIT_COWERIDLE"
		"		TASK_WAIT					5"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_CIT_HANDSUPIDLE"
		"		TASK_WAIT					5"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY
	//
	// Face target and play m_nInspectActivity
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_TARGET				0"
		"		TASK_CIT_PLAY_INSPECT_ACT		0"
		"		TASK_WAIT						5"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_CITIZEN_FLIP_OFF
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_CITIZEN_FLIP_OFF,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_CIT_TARGET_POLICE				0"
		"		TASK_FACE_TARGET					0"
		// UNDONE: Disabled - removed hand-to-hand combat
		//"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_HH_CLUBHEAD_BLOCK "
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							1"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_CITIZEN_NAP
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_CITIZEN_NAP,

		"	Tasks"
		"		TASK_FIND_LOCK_HINTNODE		0"
		"		TASK_GET_PATH_TO_HINTNODE	0"
		"		TASK_FACE_PATH				0"
		"		TASK_WALK_PATH				0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_FACE_HINTNODE			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_CIT_IDLEWALL"
		"		TASK_WAIT					30"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_CITIZEN_MANHACKFLINCH
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_CITIZEN_MANHACKFLINCH,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_CIT_MANHACKFLINCH"
		""
		"	Interrupts"
	)

	//=========================================================
	// > TakeCoverFromBestSound
	//
	//			hide from the loudest sound source
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_TAKE_COVER_FROM_BEST_SOUND,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_FLEE_FROM_BEST_SOUND"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_SET_TOLERANCE_DISTANCE		24"
		"		 TASK_FIND_COVER_FROM_BEST_SOUND	0"
		"		 TASK_RUN_PATH_TIMED				1.5"
		"		 TASK_TURN_LEFT						179"
		"		 TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"	// Translated to cover
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)


	//=========================================================
	//
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_FLEE_FROM_BEST_SOUND,

		"	Tasks"
		"		 TASK_MOVE_AWAY_PATH				600"
		"		 TASK_RUN_PATH_TIMED				1.5"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_TURN_LEFT						179"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCI_HEAL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_HEAL,

		"	Tasks"
		"		TASK_GET_PATH_TO_TARGET				0"
		"		TASK_MOVE_TO_TARGET_RANGE			50"
		"		TASK_FACE_IDEAL						0"
//		"		TASK_SAY_HEAL						0"
//		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_ARM"
		"		TASK_CIT_HEAL							0"
//		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_DISARM"
		"	"
		"	Interrupts"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CITIZEN_FAIL_TAKE_COVER_TURRET,

		"	Tasks"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_MOVE_AWAY_PATH				600"
		"		 TASK_RUN_PATH_WITHIN_DIST			100"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_TURN_LEFT						179"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)

AI_END_CUSTOM_NPC()
