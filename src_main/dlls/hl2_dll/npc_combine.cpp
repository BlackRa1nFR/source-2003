//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "AI_Hull.h"
#include "AI_Navigator.h"
#include "AI_Motor.h"
#include "AI_SquadSlot.h"
#include "AI_Squad.h"
#include "soundent.h"
#include "game.h"
#include "NPCEvent.h"
#include "npc_Combine.h"
#include "activitylist.h"
#include "player.h"
#include "basecombatweapon.h"
#include "basegrenade_shared.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "globals.h"
#include "ndebugoverlay.h"

int g_fCombineQuestion;				// true if an idle grunt asked a question. Cleared when someone answers. YUCK old global from grunt code


// Used when only what combine to react to what the spotlight sees
#define SF_COMBINE_NO_LOOK	(1 << 16)

enum COMBINE_SENTENCE_TYPES
{
	COMBINE_SENT_NONE = -1,
	COMBINE_SENT_GREN = 0,
	COMBINE_SENT_ALERT,
	COMBINE_SENT_MONSTER,
	COMBINE_SENT_COVER,
	COMBINE_SENT_THROW,
	COMBINE_SENT_CHARGE,
	COMBINE_SENT_TAUNT,
};

const char *CNPC_Combine::pCombineSentences[] = 
{
	"COMBINE_GREN",		// grenade scared grunt
	"COMBINE_ALERT",	// sees player
	"COMBINE_MONST",  // sees monster
	"COMBINE_COVER",	// running to cover
	"COMBINE_THROW",	// about to throw grenade
	"COMBINE_CHARGE",   // running out to get the enemy
	"COMBINE_TAUNT",	// say rude things
};


#define COMBINE_LIMP_HEALTH				20
#define	COMBINE_SENTENCE_VOLUME			(float)1.0 // volume of grunt sentences
#define COMBINE_VOL						1.0		// volume of grunt sounds
#define COMBINE_ATTN					SNDLVL_70dB	// attenutation of grunt sentences
#define	COMBINE_MIN_GRENADE_CLEAR_DIST	250

#define COMBINE_EYE_STANDING_POSITION	Vector( 0, 0, 66 )
#define COMBINE_GUN_STANDING_POSITION	Vector( 0, 0, 57 )
#define COMBINE_EYE_CROUCHING_POSITION	Vector( 0, 0, 40 )
#define COMBINE_GUN_CROUCHING_POSITION	Vector( 0, 0, 36 )
#define COMBINE_MIN_CROUCH_DISTANCE		256.0

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionCombineKick		= 0;

//=========================================================
// Combines's Anim Events Go Here
//=========================================================
#define COMBINE_AE_RELOAD			( 2 )
#define COMBINE_AE_KICK				( 3 )
#define COMBINE_AE_AIM				( 4 )
#define COMBINE_AE_GREN_TOSS		( 7 )
#define COMBINE_AE_GREN_LAUNCH		( 8 )
#define COMBINE_AE_GREN_DROP		( 9 )
#define COMBINE_AE_CAUGHT_ENEMY		( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.

//=========================================================
// Combine activities
//=========================================================
Activity ACT_COMBINE_STANDING_SMG1;
Activity ACT_COMBINE_CROUCHING_SMG1;
Activity ACT_COMBINE_STANDING_AR2;
Activity ACT_COMBINE_CROUCHING_AR2;
Activity ACT_COMBINE_WALKING_AR2;
Activity ACT_COMBINE_STANDING_SHOTGUN;
Activity ACT_COMBINE_CROUCHING_SHOTGUN;
Activity ACT_COMBINE_THROW_GRENADE;
Activity ACT_COMBINE_LAUNCH_GRENADE;

// -----------------------------------------------
//	> Squad slots
// -----------------------------------------------
enum SquadSlot_T
{	
	SQUAD_SLOT_GRENADE1 = LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_GRENADE2,
	SQUAD_SLOT_ATTACK_OCCLUDER,
};



LINK_ENTITY_TO_CLASS( npc_combine, CNPC_Combine );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Combine )

	DEFINE_FIELD( CNPC_Combine, m_nKickDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Combine, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Combine, m_bStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Combine, m_bCanCrouch, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Combine, m_bFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Combine, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Combine, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Combine, m_lastGrenadeCondition, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Combine, m_flNextPainSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Combine, m_flNextAlertSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Combine, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Combine, m_flNextLostSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Combine, m_flAlertPatrolTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Combine, m_nShots, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Combine, m_flShotDelay, FIELD_FLOAT ),
	DEFINE_KEYFIELD( CNPC_Combine, m_iNumGrenades, FIELD_INTEGER, "NumGrenades" ),

	//							m_AssaultBehavior (auto saved by AI)
	//							m_StandoffBehavior (auto saved by AI)

	DEFINE_INPUTFUNC( CNPC_Combine,	FIELD_VOID,	"LookOff",	InputLookOff ),
	DEFINE_INPUTFUNC( CNPC_Combine,	FIELD_VOID,	"LookOn",	InputLookOn ),

END_DATADESC()

CNPC_Combine::CNPC_Combine()
{
	m_vecTossVelocity.Init();
}

//------------------------------------------------------------------------------
// Purpose: Don't look, only get info from squad.
//------------------------------------------------------------------------------
void CNPC_Combine::InputLookOff( inputdata_t &inputdata )
{
	m_spawnflags |= SF_COMBINE_NO_LOOK;
}

//------------------------------------------------------------------------------
// Purpose: Enable looking.
//------------------------------------------------------------------------------
void CNPC_Combine::InputLookOn( inputdata_t &inputdata )
{
	m_spawnflags &= ~SF_COMBINE_NO_LOOK;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Combine::Precache()
{
	engine->PrecacheModel("models/Weapons/w_grenade.mdl");
	UTIL_PrecacheOther( "npc_handgrenade" );

	// get voice pitch
	if (random->RandomInt(0,1))
		SetVoicePitch( 109 + random->RandomInt(0,7) );
	else
		SetVoicePitch( 100 );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Combine::Spawn( void )
{
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_flFieldOfView			= -0.2;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState				= NPC_STATE_NONE;
	m_flNextGrenadeCheck	= gpGlobals->curtime + 1;
	m_flNextPainSoundTime	= 0;
	m_flNextAlertSoundTime	= 0;
	m_iSentence				= COMBINE_SENT_NONE;
	m_bStanding				= true;

//	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB);
	// JAY: Disabled jump for now - hard to compare to HL1
	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_CLIMB);

	// Innate range attack for grenade
	// CapabilitiesAdd(bits_CAP_INNATE_RANGE_ATTACK2 );

	// Innate range attack for kicking
	CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1 );

	// Can be in a squad
	CapabilitiesAdd( bits_CAP_SQUAD);
	CapabilitiesAdd( bits_CAP_USE_WEAPONS );

	CapabilitiesAdd( bits_CAP_DUCK );				// In reloading and cover

	m_bFirstEncounter	= true;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_flNextLostSoundTime		= 0;
	m_flAlertPatrolTime			= 0;


	NPCInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Combine::CreateBehaviors()
{
	AddBehavior( &m_AssaultBehavior );
	AddBehavior( &m_StandoffBehavior );
	AddBehavior( &m_FollowBehavior );
	
	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_Combine::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 100;
		break;
	case ACT_RUN:
	case ACT_RUN_HURT:
		return 15;
		break;
	case ACT_WALK:
	case ACT_WALK_CROUCH:
		return 25;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		return 120;
	default:
		return 90;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------
bool CNPC_Combine::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
  	// FIXME: this will break scripted sequences that walk when they have an enemy
  	if (GetEnemy() && (GetNavigator()->GetMovementActivity() == ACT_WALK_AIM || GetNavigator()->GetMovementActivity() == ACT_WALK || GetNavigator()->GetMovementActivity() == ACT_RUN_AIM))
  	{
		Vector vecEnemyLKP = GetEnemyLKP();
		AddFacingTarget( GetEnemy(), vecEnemyLKP, 1.0, 0.2 );
	}

	return BaseClass::OverrideMoveFacing( move, flInterval );
}



	
//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
Class_T	CNPC_Combine::Classify ( void )
{
	return CLASS_COMBINE;
}



//=========================================================
// start task
//=========================================================
void CNPC_Combine::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{

	case TASK_ANNOUNCE_ATTACK:
	{
		// If Primary Attack
		if ((int)pTask->flTaskData == 1)
		{
			// -----------------------------------------------------------
			// If enemy isn't facing me and I haven't attacked in a while
			// annouce my attack before I start wailing away
			// -----------------------------------------------------------
			CBaseCombatCharacter *pBCC = GetEnemyCombatCharacterPointer();

			if	(pBCC && pBCC->IsPlayer() && (!pBCC->FInViewCone ( this )) &&
				 (gpGlobals->curtime - m_flLastAttackTime > 3.0) )
			{
				m_flLastAttackTime = gpGlobals->curtime;

				// High priority so don't check FOkToMakeSound()
				SENTENCEG_PlayRndSz( edict(), "COMBINE_ANNOUNCE", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch() );
				JustMadeSound();

				// Wait two seconds
				m_flWaitFinished = gpGlobals->curtime + 2.0;

				if ( m_bStanding )
				{
					SetActivity(ACT_IDLE);
				}
				else
				{
					SetActivity(ACT_COWER); // This is really crouch idle
				}
			}
			// -------------------------------------------------------------
			//  Otherwise move on
			// -------------------------------------------------------------
			else
			{
				TaskComplete();
			}
		}
		else
		{
			// High priority so don't check FOkToMakeSound()
			SENTENCEG_PlayRndSz( edict(), "COMBINE_THROW_GRENADE", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
			JustMadeSound();
			SetActivity(ACT_IDLE);

			// Wait two seconds
			m_flWaitFinished = gpGlobals->curtime + 2.0;
		}
		break;
	}	
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		BaseClass::StartTask( pTask );
		break;

	case TASK_COMBINE_FACE_TOSS_DIR:
		break;

	case TASK_COMBINE_IGNORE_ATTACKS:
		// must be in a squad
		if (m_pSquad && m_pSquad->NumMembers() > 2)
		{
			// the enemy must be far enough away
			if (GetEnemy() && (GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter()).Length() > 512.0 )
			{
				m_flNextAttack	= gpGlobals->curtime + pTask->flTaskData;
			}
		}
		TaskComplete( );
		break;

/*

  This is handled with an overlay now (sjb)

	case TASK_COMBINE_MOVE_AND_SHOOT:
	case TASK_COMBINE_MOVE_AND_AIM:
		{
			m_nShots = random->RandomInt( 4, 7 );
			ChainStartTask( TASK_WAIT_FOR_MOVEMENT, pTask->flTaskData );
		}
		break;
*/

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		{
			BaseClass::StartTask( pTask );
			bool bIsFlying = (GetMoveType() == MOVETYPE_FLY) || (GetMoveType() == MOVETYPE_FLYGRAVITY);
			if (bIsFlying)
			{
				SetIdealActivity( ACT_GLIDE );
			}

		}
		break;

	case TASK_FIND_COVER_FROM_ENEMY:
		{
			if (GetHintGroup() == NULL_STRING)
			{
				CBaseEntity *pEntity = GetEnemy();
				
				// FIXME: this should be generalized by the schedules that are selected, or in the definition of 
				// what "cover" means (i.e., trace attack vulnerability vs. physical attack vulnerability
				if (pEntity && pEntity->MyNPCPointer())
				{
					if ( !(pEntity->MyNPCPointer()->CapabilitiesGet( ) & bits_CAP_WEAPON_RANGE_ATTACK1))
					{
						TaskComplete();
						return;
					}
				}
			}
			BaseClass::StartTask( pTask );
		}
		break;
	case TASK_RANGE_ATTACK1:
		{
			/*
			// FIXME: should this move into the weapon logic?
			switch( random->RandomInt( 1, 3 ))
			{
			case 1:
				m_nShots = random->RandomInt( 6, 10 );
				m_flShotDelay = 0.0;
				break;
			case 2:
				m_nShots = random->RandomInt( 3, 4 );
				m_flShotDelay = 0.3;
				break;
			case 3:
				m_nShots = random->RandomInt( 1, 2 );
				m_flShotDelay = 0.5;
				break;
			}
			*/

			m_nShots = GetActiveWeapon()->GetRandomBurst();
			m_flShotDelay = GetActiveWeapon()->GetFireRate();

			m_flNextAttack = gpGlobals->curtime + m_flShotDelay - 0.1;
			ResetIdealActivity( ACT_RANGE_ATTACK1 );
			m_flLastAttackTime = gpGlobals->curtime;
		}
		break;
	default: 
		BaseClass:: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CNPC_Combine::RunTask( const Task_t *pTask )
{
	/*
	{
		CBaseEntity *pEnemy = GetEnemy();
		if (pEnemy)
		{
			NDebugOverlay::Line(Center(), pEnemy->Center(), 0,255,255, false, 0.1);
		}
	
	}
	*/

	/*
	if (m_iMySquadSlot != SQUAD_SLOT_NONE)
	{
		char text[64];
		Q_snprintf( text, strlen( text ), "%d", m_iMySquadSlot );

		NDebugOverlay::Text( Center() + Vector( 0, 0, 72 ), text, false, 0.1 );
	}
	*/

	switch ( pTask->iTask )
	{
	case TASK_ANNOUNCE_ATTACK:
	{
		// Stop waiting if enemy facing me or lost enemy
		CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();
		if	(!pBCC || pBCC->FInViewCone( this ))
		{
			TaskComplete();
		}

		if ( gpGlobals->curtime >= m_flWaitFinished )
		{
			TaskComplete();
		}
		break;
	}
	case TASK_COMBINE_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			GetMotor()->SetIdealYawToTargetAndUpdate( GetLocalOrigin() + m_vecTossVelocity * 64, AI_KEEP_YAW_SPEED );

			if ( FacingIdeal() )
			{
				TaskComplete( true );
			}
			break;
		}
#if 0
	case TASK_COMBINE_MOVE_AND_AIM:
		{
			Vector vecEnemyLKP = GetEnemyLKP();
			float flEDist = UTIL_DistApprox2D( Center(), vecEnemyLKP );
			if (GetNavigator()->IsGoalActive())
			{
				float flGDist = GetNavigator()->GetPathDistToGoal( );

				if ((flEDist < 500.0 || flGDist < 60.0) && !HasCondition(COND_TOO_FAR_TO_ATTACK) && !HasCondition(COND_LIGHT_DAMAGE) && !HasCondition(COND_REPEATED_DAMAGE) && !HasCondition(COND_SEE_ENEMY)) // FIXME: use weapon distance?
				{
					GetNavigator()->SetMovementActivity(ACT_WALK_AIM); 	// FIXME: this is probably evil
				}
				else
				{
					GetNavigator()->SetMovementActivity(ACT_RUN_AIM);
				}
			}

			// FIXME: HACK.  Need a clean way to run multiple tasks
			ChainRunTask( TASK_WAIT_FOR_MOVEMENT, pTask->flTaskData );
		}
		break;

	case TASK_COMBINE_MOVE_AND_SHOOT:
		{
			if (GetNavigator()->IsGoalActive())
			{
			// can they shoot?
				bool bShouldShoot = false;
				if (HasCondition( COND_CAN_RANGE_ATTACK1 ) && gpGlobals->curtime >= m_flNextAttack)
				{
					bShouldShoot = true;
				}

				if (bShouldShoot)
				{
					/*
					if (GetNavigator()->GetMovementActivity() != ACT_WALK_AIM)
					{
						// transitioning, wait a bit
						m_flNextAttack = gpGlobals->curtime + 0.3;
					}
					*/
					
					// walk, don't run
					GetNavigator()->SetMovementActivity(ACT_WALK_AIM); 	// FIXME: this is probably evil

					Activity activity = TranslateActivity( ACT_GESTURE_RANGE_ATTACK1 );

					Assert( activity != ACT_INVALID );

					// time to fire?
					if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && gpGlobals->curtime >= m_flNextAttack )
					{
						if (--m_nShots > 0)
						{
							SetLastAttackTime( gpGlobals->curtime );
							AddGesture( activity );
							// FIXME: this seems a bit wacked
							Weapon_SetActivity( Weapon_TranslateActivity( ACT_RANGE_ATTACK1 ), 0 );
							m_flNextAttack = gpGlobals->curtime + GetActiveWeapon()->GetFireRate() - 0.1;
						}
						else
						{
							m_nShots = random->RandomInt( 4, 7 );
							m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 0.3, 0.5 );
						}
					}
				}
				else
				{
					GetNavigator()->SetMovementActivity(ACT_RUN_AIM);
				}

				// keep enemy if dead but try to look for a new one
				if (!GetEnemy() || !GetEnemy()->IsAlive())
				{
					CBaseEntity *pNewEnemy = BestEnemy();

					if( pNewEnemy != NULL )
					{
						//New enemy! Clear the timers and set conditions.
 						SetCondition( COND_NEW_ENEMY );
						SetEnemy( pNewEnemy );
					}
					else
					{
						ClearAttackConditions();
					}
					// SetEnemy( NULL );
				}
			}

			// FIXME: HACK.  Need a clean way to run multiple tasks
			ChainRunTask( TASK_WAIT_FOR_MOVEMENT, pTask->flTaskData );
		}
		break;
#endif

	case TASK_RANGE_ATTACK1:
		{
			AutoMovement( );

			Vector vecEnemyLKP = GetEnemyLKP();
			if (!FInAimCone( vecEnemyLKP ))
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
			}
			else
			{
				GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
			}

			if ( gpGlobals->curtime >= m_flNextAttack )
			{
				if ( IsActivityFinished() )
				{
					if (--m_nShots > 0)
					{
						// Msg("ACT_RANGE_ATTACK1\n");
						ResetIdealActivity( ACT_RANGE_ATTACK1 );
						m_flLastAttackTime = gpGlobals->curtime;
						m_flNextAttack = gpGlobals->curtime + m_flShotDelay - 0.1;
					}
					else
					{
						// Msg("TASK_RANGE_ATTACK1 complete\n");
						TaskComplete();
					}
				}
			}
			else
			{
				// Msg("Wait\n");
			}
		}
		break;

	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//------------------------------------------------------------------------------
// Purpose : Override to always shoot at eyes (for ducking behind things)
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_Combine::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{
	if ( GetFlags() & FL_DUCKING )
	{
		return WorldSpaceCenter() - (Vector(0,0,16));
	}

	return WorldSpaceCenter();
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CNPC_Combine::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	if( m_spawnflags & SF_COMBINE_NO_LOOK )
	{
		// When no look is set, if enemy has eluded the squad, 
		// he's always invisble to me
		if (GetEnemies()->HasEludedMe(pEntity))
		{
			return false;
		}
	}
	return BaseClass::FVisible(pEntity, traceMask, ppBlocker);
}

//-----------------------------------------------------------------------------
// Purpose: Override.  Don't update if I'm not looking
// Input  :
// Output : Returns true is new enemy, false is known enemy
//-----------------------------------------------------------------------------
bool CNPC_Combine::UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, bool firstHand )
{
	if( m_spawnflags & SF_COMBINE_NO_LOOK )
	{
		return false;
	}

	return BaseClass::UpdateEnemyMemory( pEnemy, position, firstHand );
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_Combine::BuildScheduleTestBits( void )
{
	if (gpGlobals->curtime < m_flNextAttack)
	{
		ClearCustomInterruptCondition( COND_CAN_RANGE_ATTACK1 );
		ClearCustomInterruptCondition( COND_CAN_RANGE_ATTACK2 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Translate base class activities into combot activites
//-----------------------------------------------------------------------------
Activity CNPC_Combine::NPC_TranslateActivity( Activity eNewActivity )
{
	if (eNewActivity == ACT_RELOAD_SMG1 || eNewActivity == ACT_RELOAD_PISTOL)
	{
		// Combine doesn't handle these activities
		eNewActivity = ACT_RELOAD;
	}

	if (eNewActivity == ACT_RANGE_ATTACK_SMG1)
	{
		if ( m_bStanding )
		{
			return ( Activity )ACT_COMBINE_STANDING_SMG1;
		}
		else
		{
			return ( Activity )ACT_COMBINE_CROUCHING_SMG1;
		}
	}
	else if (eNewActivity == ACT_RANGE_ATTACK_AR2)
	{
		if ( m_bStanding )
		{
			return ( Activity )ACT_COMBINE_STANDING_AR2;
		}
		else
		{
			return ( Activity )ACT_COMBINE_CROUCHING_AR2;
		}
	}
	else if (eNewActivity == ACT_RANGE_ATTACK_SHOTGUN)
	{
		if ( m_bStanding )
		{
			return ( Activity )ACT_COMBINE_STANDING_SHOTGUN;
		}
		else
		{
			return ( Activity )ACT_COMBINE_CROUCHING_SHOTGUN;
		}
	}
	else if (eNewActivity == ACT_RELOAD)
	{
		eNewActivity = BaseClass::NPC_TranslateActivity( eNewActivity );

		if (eNewActivity == ACT_RELOAD)
		{
			if ( m_bStanding )
			{
				return ( Activity )ACT_RELOAD;
			}
			else
			{
				return ( Activity )ACT_RELOAD_LOW;
			}
		}
	}
	else if (eNewActivity == ACT_RANGE_ATTACK2)
	{
		// grunt is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
		if (Weapon_OwnsThisType( "weapon_grenadelauncher" ) )
		{
			return ( Activity )ACT_COMBINE_LAUNCH_GRENADE;
		}
		else
		{
			return ( Activity )ACT_COMBINE_THROW_GRENADE;
		}
	}
	else if (eNewActivity == ACT_RUN)
	{
		m_bStanding = true;
		if ( m_iHealth <= COMBINE_LIMP_HEALTH )
		{
			// limp!
			//return ACT_RUN_HURT;
		}
	}
	else if (eNewActivity == ACT_WALK)
	{
		m_bStanding = true;
		if ( m_iHealth <= COMBINE_LIMP_HEALTH )
		{
			// limp!
			//return ACT_WALK_HURT;
		}
	}
	else if (eNewActivity == ACT_IDLE)
	{
		if ( !m_bStanding )
		{
			return ACT_CROUCHIDLE;
		}
		if ( m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT )
		{
			return ACT_IDLE_ANGRY;
		}
	}
	return BaseClass::NPC_TranslateActivity( eNewActivity );
}


//-----------------------------------------------------------------------------
// Purpose: Overidden for human grunts because they  hear the DANGER sound
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Combine::GetSoundInterests( void )
{
	return	SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_DANGER | SOUND_PHYSICS_DANGER;
}

//-----------------------------------------------------------------------------
// Purpose: Combine needs to check ammo
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Combine::CheckAmmo ( void )
{
	if (GetActiveWeapon())
	{
		if (GetActiveWeapon()->UsesPrimaryAmmo())
		{
#ifdef INFINITE_AMMO // for testing when don't want interruption by reload schedules
			GetActiveWeapon()->m_iClip1 = GetActiveWeapon()->GetMaxClip1(); 
			GetActiveWeapon()->m_iClip2 = GetActiveWeapon()->GetMaxClip2();  
#endif
			if (!GetActiveWeapon()->HasPrimaryAmmo() )
			{
				SetCondition(COND_NO_PRIMARY_AMMO);
			}
			else if (GetActiveWeapon()->UsesClipsForAmmo1() && GetActiveWeapon()->Clip1() < (GetActiveWeapon()->GetMaxClip1() / 4 + 1))
			{
				// don't check for low ammo if you're near the max range of the weapon

				SetCondition(COND_LOW_PRIMARY_AMMO);
			}
		}
		if (!GetActiveWeapon()->HasSecondaryAmmo() )
		{
			if ( GetActiveWeapon()->UsesClipsForAmmo2() )
				SetCondition(COND_NO_SECONDARY_AMMO);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Announce an assault if the enemy can see me and we are pretty 
//			close to him/her
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Combine::AnnounceAssault(void)
{
	if (random->RandomInt(0,5) > 1)
	{
		return;
	}
	// If enemy can see me make assualt sound
	CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();

	if (!pBCC)
	{
		return;
	}

	if (!FOkToMakeSound())
	{
		return;
	}

	// Make sure we are pretty close
	if ((GetLocalOrigin() - pBCC->GetLocalOrigin()).Length() > 2000)
	{
		return;
	}

	// Make sure we are in view cone of player
	if (!pBCC->FInViewCone ( this ))
	{
		return;
	}

	// Make sure player can see me
	Vector startTrace   = EyePosition();
	Vector endTrace		= pBCC->EyePosition();

	trace_t tr;
	AI_TraceLine( startTrace, endTrace, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction == 1.0 )
	{
		if (FOkToMakeSound())
		{
			SENTENCEG_PlayRndSz( edict(), "COMBINE_ASSAULT", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
			JustMadeSound();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Combine::SelectSchedule( void )
{
	if( BehaviorSelectSchedule() )
	{
		return BaseClass::SelectSchedule();
	}

	// clear old sentence
	m_iSentence = COMBINE_SENT_NONE;

	// These things are done in any state but dead and prone
	if (m_NPCState != NPC_STATE_DEAD && m_NPCState != NPC_STATE_PRONE)
	{
		// flying? It is then assumed I am rapelling. 
		if ( GetMoveType() == MOVETYPE_FLY && GetNavType() != NAV_JUMP && GetNavType() != NAV_CLIMB )
		{
			if (GetFlags() & FL_ONGROUND)
			{
				// just landed
				SetMoveType( MOVETYPE_STEP );
				return SCHED_COMBINE_REPEL_LAND;
			}
			else
			{
				// repel down a rope, 
				if ( m_NPCState == NPC_STATE_COMBAT )
					return SCHED_COMBINE_REPEL_ATTACK;
				else
					return SCHED_COMBINE_REPEL;
			}
		}

		// grunts place HIGH priority on running away from danger sounds.
		if ( HasCondition(COND_HEAR_DANGER) )
		{
			CSound *pSound;
			pSound = GetBestSound();

			Assert( pSound != NULL );
			if ( pSound)
			{
				if (pSound->m_iType & SOUND_DANGER)
				{
					// dangerous sound nearby!
					
					//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
					// and the grunt should find cover from the blast
					// good place for "SHIT!" or some other colorful verbal indicator of dismay.
					// It's not safe to play a verbal order here "Scatter", etc cause 
					// this may only affect a single individual in a squad. 
					
					if (FOkToMakeSound())
					{
						SENTENCEG_PlayRndSz( edict(), "COMBINE_GREN", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
						JustMadeSound();
					}
					return SCHED_TAKE_COVER_FROM_BEST_SOUND;
				}

				// JAY: This was disabled in HL1.  Test?
				if (!HasCondition( COND_SEE_ENEMY ) && ( pSound->m_iType & (SOUND_PLAYER | SOUND_COMBAT) ))
				{
					GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );
				}
			}
		}
	}
	switch	( m_NPCState )
	{
	case NPC_STATE_ALERT:
		{
			if( HasCondition( COND_COMBINE_SHOULD_PATROL ) )
			{
				return SCHED_COMBINE_PATROL;
			}
		}
		break;

	case NPC_STATE_COMBAT:
		{

			// -----------
			// dead enemy
			// -----------
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			// -----------
			// new enemy
			// -----------
			if ( HasCondition(COND_NEW_ENEMY) )
			{
				CBaseEntity *pEnemy = GetEnemy();
				if ( m_pSquad && pEnemy )
				{
					if ( m_pSquad->IsLeader( this ) || m_pSquad->GetLeader()->GetEnemy() != pEnemy)
					{
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a 
						// monster and has made it the squad's enemy. You
						// can check GetFlags() for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight" 
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToMakeSound())
						{
							if ( pEnemy->IsPlayer() )
							{
								// player
								SENTENCEG_PlayRndSz( edict(), "COMBINE_ALERT", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
							}
							else 
							{
								SENTENCEG_PlayRndSz( edict(), "COMBINE_MONST", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());

// JAY: HL1 behavior -- difficult to implement due to too many classifications
#if 0
								int classify = pEnemy->Classify();
								if ( classify != CLASS_PLAYER_ALLY && classify != CLASS_HUMAN_PASSIVE && classify != CLASS_MACHINE )
								{
									// monster
									SENTENCEG_PlayRndSz( edict(), "COMBINE_MONST", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
								}
#endif
							}

							JustMadeSound();
						}
						
						if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
						{
							return SCHED_COMBINE_SUPPRESS;
						}
						else
						{
							return SCHED_COMBINE_ASSAULT;
						}
					}
					else
					{
						return SCHED_TAKE_COVER_FROM_ENEMY;
					}
				}
			}

#if 0
			else if ( !m_hActiveWeapon && !HasCondition( COND_TASK_FAILED ) && Weapon_FindUsable(Vector(500,500,500)) )
			{
				return SCHED_NEW_WEAPON;
			}
#endif

			// ---------------------
			// no ammo
			// ---------------------
			if ( HasCondition ( COND_NO_PRIMARY_AMMO ) || HasCondition ( COND_LOW_PRIMARY_AMMO ))
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return SCHED_HIDE_AND_RELOAD;
			}
			
#if 0 //WEDGE
			if (!HasCondition(COND_CAN_RANGE_ATTACK1) && HasCondition( COND_NO_SECONDARY_AMMO ))
			{
				return SCHED_HIDE_AND_RELOAD;
			}
#endif
			
			// ----------------------
			// LIGHT DAMAGE
			// ----------------------
			if ( HasCondition( COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = random->RandomInt(0,99);

				if ( iPercent <= 90 && GetEnemy() != NULL )
				{
					// only try to take cover if we actually have an enemy!

					// FIXME: need to take cover for enemy dealing the damage

					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToMakeSound()) // && random->RandomInt(0,1))
					{
						//SENTENCEG_PlayRndSz( edict(), "COMBINE_COVER", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
						m_iSentence = COMBINE_SENT_COVER;
						//JustMadeSound();
					}
					return SCHED_TAKE_COVER_FROM_ENEMY;
				}
				else
				{
					// taking damage should have entered a "null" enemy position into memory
					return SCHED_FEAR_FACE;
				}
			}

			int attackSchedule = SelectScheduleAttack();

			if ( attackSchedule != SCHED_NONE )
				return attackSchedule;

			if (HasCondition(COND_ENEMY_OCCLUDED))
			{
				// stand up, just in case
				m_bStanding = true;
				m_bCanCrouch = false;

				if ( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
				{
					// Charge in and break the enemy's cover!
					return SCHED_COMBINE_ASSAULT;
				}

				return SCHED_STANDOFF;
			}

			// --------------------------------------------------------------
			// Enemy not occluded but isn't open to attack
			// --------------------------------------------------------------
			if ( HasCondition( COND_SEE_ENEMY ) && !HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				if (HasCondition( COND_TOO_FAR_TO_ATTACK ) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ))
				{
					return SCHED_COMBINE_PRESS_ATTACK;
				}

				AnnounceAssault(); 
				return SCHED_COMBINE_ASSAULT;
			}
		}
	}

	// no special cases here, call the base class
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------

int CNPC_Combine::SelectScheduleAttack()
{
	// Kick attack?
	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
	{
		return SCHED_MELEE_ATTACK1;
	}

	// Can I shoot?

	if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
	{

// JAY: HL1 behavior missing?
#if 0
		if ( m_pSquad )
		{
			// if the enemy has eluded the squad and a squad member has just located the enemy
			// and the enemy does not see the squad member, issue a call to the squad to waste a 
			// little time and give the player a chance to turn.
			if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
			{
				MySquadLeader()->m_fEnemyEluded = FALSE;
				return SCHED_GRUNT_FOUND_ENEMY;
			}
		}
#endif

		// Engage if allowed
		if ( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
		{
			return SCHED_RANGE_ATTACK1;
		}
		
		// Throw a grenade if not allowed to engage with weapon.
		if ( HasCondition( COND_CAN_RANGE_ATTACK2 ) )
		{
			if ( OccupyStrategySlotRange( SQUAD_SLOT_GRENADE1, SQUAD_SLOT_GRENADE2 ) )
			{
				return SCHED_RANGE_ATTACK2;
			}
		}

		m_bCanCrouch = true;
		return SCHED_TAKE_COVER_FROM_ENEMY;
	}

	if (HasCondition(COND_WEAPON_SIGHT_OCCLUDED))
	{
		// If they are hiding behind something that we can destroy, start shooting at it.
		CBaseEntity *pBlocker = GetEnemyOccluder();
		if ( pBlocker && pBlocker->GetHealth() > 0 && OccupyStrategySlot( SQUAD_SLOT_ATTACK_OCCLUDER ) )
		{
			return SCHED_COMBINE_SHOOT_ENEMY_COVER;
		}
	}

	// can't fire our weapon... can we throw a grenade?
	if ( HasCondition(COND_CAN_RANGE_ATTACK2)  )
	{
		if( OccupyStrategySlotRange( SQUAD_SLOT_GRENADE1, SQUAD_SLOT_GRENADE2 ) )
		{
			if (FOkToMakeSound())
			{	
				SENTENCEG_PlayRndSz( edict(), "COMBINE_THROW_GRENADE", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
				JustMadeSound();
			}

			return SCHED_RANGE_ATTACK2;
		}
	}
	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Combine::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( m_pSquad )
			{
				// Have to explicitly check innate range attack condition as may have weapon with range attack 2
				if (	g_iSkillLevel == SKILL_HARD	&& 
 						HasCondition(COND_CAN_RANGE_ATTACK2) &&
						OccupyStrategySlotRange( SQUAD_SLOT_GRENADE1, SQUAD_SLOT_GRENADE2 ) )
				{
					if (FOkToMakeSound())
					{
						SENTENCEG_PlayRndSz( edict(), "COMBINE_THROW_GRENADE", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
						JustMadeSound();
					}
					return SCHED_COMBINE_TOSS_GRENADE_COVER1;
				}
				else
				{
					return SCHED_COMBINE_TAKE_COVER1;
				}
			}
			else
			{
				// Have to explicitly check innate range attack condition as may have weapon with range attack 2
				if ( random->RandomInt(0,1) && HasCondition(COND_CAN_RANGE_ATTACK2) )
				{
					return SCHED_COMBINE_GRENADE_COVER1;
				}
				else
				{
					return SCHED_COMBINE_TAKE_COVER1;
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return SCHED_COMBINE_TAKE_COVER_FROM_BEST_SOUND;
		}
		break;
	case SCHED_COMBINE_TAKECOVER_FAILED:
		{
			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			{
				return TranslateSchedule( SCHED_RANGE_ATTACK1 );
			}

			// Run somewhere randomly
			return TranslateSchedule( SCHED_FAIL ); 
			break;
		}
		break;
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{
			return TranslateSchedule( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_COMBINE_ASSAULT:
		{
			CBaseEntity *pEntity = GetEnemy();
			
			// FIXME: this should be generalized by the schedules that are selected, or in the definition of 
			// what "cover" means (i.e., trace attack vulnerability vs. physical attack vulnerability
			if (pEntity && pEntity->MyNPCPointer())
			{
				if ( !(pEntity->MyNPCPointer()->CapabilitiesGet( ) & bits_CAP_WEAPON_RANGE_ATTACK1))
				{
					return TranslateSchedule( SCHED_ESTABLISH_LINE_OF_FIRE );
				}
			}
			// don't charge forward if there's a hint group
			if (GetHintGroup() != NULL_STRING)
			{
				return TranslateSchedule( SCHED_ESTABLISH_LINE_OF_FIRE );
			}
			return SCHED_COMBINE_ASSAULT;
		}
	case SCHED_ESTABLISH_LINE_OF_FIRE:
		{
			// always assume standing
			m_bStanding = true;

			return SCHED_COMBINE_ESTABLISH_LINE_OF_FIRE;
		}
		break;
	case SCHED_HIDE_AND_RELOAD:
		{
			// stand up, just in case
			// m_bStanding = true;
			// m_bCanCrouch = true;
			if( HasCondition( COND_CAN_RANGE_ATTACK2 ) && OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) && random->RandomInt( 0, 100 ) < 20 )
			{
				// If I COULD throw a grenade and I need to reload, 20% chance I'll throw a grenade before I hide to reload.
				return SCHED_COMBINE_GRENADE_AND_RELOAD;
			}

			return SCHED_COMBINE_HIDE_AND_RELOAD;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			if ( HasCondition( COND_NO_PRIMARY_AMMO ) || HasCondition( COND_LOW_PRIMARY_AMMO ) )
			{
				return TranslateSchedule( SCHED_HIDE_AND_RELOAD );
			}

			if (m_bCanCrouch && !HasCondition( COND_HEAVY_DAMAGE ) && !HasCondition( COND_REPEATED_DAMAGE ))
			{
				// See if we can crouch and shoot
				if (GetEnemy() != NULL)
				{
					float dist = (GetLocalOrigin() - GetEnemy()->GetLocalOrigin()).Length();

					// only crouch if they are relativly far away
					if (dist > COMBINE_MIN_CROUCH_DISTANCE)
					{
						// try crouching
						m_bStanding = false; 

						Vector targetPos = GetEnemy()->BodyTarget(GetActiveWeapon()->GetLocalOrigin());

						// if we can't see it crouched, stand up
						if (!WeaponLOSCondition(GetLocalOrigin(),targetPos,false))
						{
							m_bStanding = true; 
						}
					}
				}
			}
			else
			{
				// always assume standing
				m_bStanding = true;
			}

			return SCHED_COMBINE_RANGE_ATTACK1;
		}
	case SCHED_RANGE_ATTACK2:
		{
			// If my weapon can range attack 2 use the weapon
			if (GetActiveWeapon() && GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK2)
			{
				return SCHED_RANGE_ATTACK2;
			}
			// Otherwise use innate attack
			else
			{
				return SCHED_COMBINE_RANGE_ATTACK2;
			}
		}
		// SCHED_COMBAT_FACE:
		// SCHED_COMBINE_WAIT_FACE_ENEMY:
		// SCHED_COMBINE_SWEEP:
		// SCHED_COMBINE_COVER_AND_RELOAD:
		// SCHED_COMBINE_FOUND_ENEMY:

	case SCHED_VICTORY_DANCE:
		{
			return SCHED_COMBINE_VICTORY_DANCE;
		}
	case SCHED_COMBINE_SUPPRESS:
		{
			if ( GetEnemy() != NULL && GetEnemy()->IsPlayer() && m_bFirstEncounter )
			{
				m_bFirstEncounter = false;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return SCHED_COMBINE_SIGNAL_SUPPRESS;
			}
			else
			{
				return SCHED_COMBINE_SUPPRESS;
			}
		}
	case SCHED_FAIL:
		{
			if ( GetEnemy() != NULL )
			{
				return SCHED_COMBINE_COMBAT_FAIL;
			}
			return SCHED_FAIL;
		}

 	case SCHED_COMBINE_REPEL:
		{
			Vector vecVelocity = GetAbsVelocity();
			if (vecVelocity.z > -128)
			{
				vecVelocity.z -= 32;
				SetAbsVelocity( vecVelocity );
			}
			return SCHED_COMBINE_REPEL;
		}
	case SCHED_COMBINE_REPEL_ATTACK:
		{
			Vector vecVelocity = GetAbsVelocity();
			if (vecVelocity.z > -128)
			{
				vecVelocity.z -= 32;
				SetAbsVelocity( vecVelocity );
			}
			return SCHED_COMBINE_REPEL_ATTACK;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//=========================================================
//=========================================================
CBaseEntity *CNPC_Combine::Kick( void )
{
	trace_t tr;

	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	Vector vecStart = GetAbsOrigin();
	vecStart.z += WorldAlignSize().z * 0.5;
	Vector vecEnd = vecStart + (forward * 70);

	AI_TraceHull( vecStart, vecEnd, Vector(-16,-16,-18), Vector(16,16,18), 
		MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );
	
	if ( FOkToMakeSound() )
	{
		SENTENCEG_PlayRndSz(edict(), "COMBINE_KICK", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
	}

	if ( tr.m_pEnt )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		return pEntity;
	}
	
	return NULL;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Combine::HandleAnimEvent( animevent_t *pEvent )
{
	Vector vecShootDir;
	Vector vecShootOrigin;

	switch( pEvent->event )
	{
		case COMBINE_AE_AIM:	
			{
				break;
			}
		case COMBINE_AE_RELOAD:

			// We never actually run out of ammo, just need to refill the clip
			if (GetActiveWeapon())
			{
				GetActiveWeapon()->WeaponSound( RELOAD_NPC );
				GetActiveWeapon()->m_iClip1 = GetActiveWeapon()->GetMaxClip1(); 
				GetActiveWeapon()->m_iClip2 = GetActiveWeapon()->GetMaxClip2();  
			}
			ClearCondition(COND_LOW_PRIMARY_AMMO);
			ClearCondition(COND_NO_PRIMARY_AMMO);
			ClearCondition(COND_NO_SECONDARY_AMMO);
			break;

		case COMBINE_AE_GREN_TOSS:
		{
			CBaseGrenade *pGrenade = (CBaseGrenade*)CreateNoSpawn( "npc_handgrenade", GetLocalOrigin() + Vector(0,0,60), vec3_angle, this );
			pGrenade->m_flDetonateTime = gpGlobals->curtime + 3.5;
			pGrenade->Spawn( );
			pGrenade->SetOwner( this );
			pGrenade->SetOwnerEntity( this );

			QAngle vecSpin;

			vecSpin.x = random->RandomFloat( -1000.0, 1000.0 );
			vecSpin.y = random->RandomFloat( -1000.0, 1000.0 );
			vecSpin.z = random->RandomFloat( -1000.0, 1000.0 );

			pGrenade->SetAbsVelocity( m_vecTossVelocity );
			pGrenade->SetLocalAngularVelocity( vecSpin );

			m_iNumGrenades--;

			m_lastGrenadeCondition = COND_NONE;
			ClearCondition( COND_CAN_RANGE_ATTACK2 );
			m_flNextGrenadeCheck = gpGlobals->curtime + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		}
		break;

		case COMBINE_AE_GREN_LAUNCH:
		{
			EmitSound( "NPC_Combine.GrenadeLaunch" );
			
			CBaseEntity *pGrenade = CreateNoSpawn( "npc_contactgrenade", Weapon_ShootPosition(), vec3_angle, this );
			pGrenade->KeyValue( "velocity", m_vecTossVelocity );
			pGrenade->Spawn( );

			m_lastGrenadeCondition = FALSE;
			if (g_iSkillLevel == SKILL_HARD)
				m_flNextGrenadeCheck = gpGlobals->curtime + random->RandomFloat( 2, 5 );// wait a random amount of time before shooting again
			else
				m_flNextGrenadeCheck = gpGlobals->curtime + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		}
		break;

		case COMBINE_AE_GREN_DROP:
		{
			CBaseGrenade *pGrenade = (CBaseGrenade*)CreateNoSpawn( "npc_handgrenade", Weapon_ShootPosition(), vec3_angle, this );
			pGrenade->KeyValue( "velocity", m_vecTossVelocity );
			pGrenade->m_flDetonateTime = gpGlobals->curtime + 3.5;
			pGrenade->Spawn( );
			pGrenade->SetOwner( this );
			pGrenade->SetOwnerEntity( this );

			m_iNumGrenades--;
		}
		break;

		case COMBINE_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();
			CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHurt );
			if (pBCC)
			{
				Vector forward, up;
				AngleVectors( GetLocalAngles(), &forward, NULL, &up );

				if ( pBCC->HandleInteraction( g_interactionCombineKick, NULL, this ) )
				{
					pBCC->ApplyAbsVelocityImpulse( forward * 300 + up * 250 );

					CTakeDamageInfo info( this, this, pBCC->m_iHealth+1, DMG_CLUB );
					CalculateMeleeDamageForce( &info, forward, pBCC->GetAbsOrigin() );
					pBCC->TakeDamage( info );
				}
				else
				{
					pBCC->ViewPunch( QAngle(15,0,0) );
					pHurt->ApplyAbsVelocityImpulse( forward * 100 + up * 50 );

					CTakeDamageInfo info( this, this, m_nKickDamage, DMG_CLUB );
					CalculateMeleeDamageForce( &info, forward, pBCC->GetAbsOrigin() );
					pBCC->TakeDamage( info );
				}
			}
			break;
		}

		case COMBINE_AE_CAUGHT_ENEMY:
		{
			if ( FOkToMakeSound() )
			{
				SENTENCEG_PlayRndSz(edict(), "COMBINE_ALERT", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
				 JustMadeSound();
			}

		}

		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get shoot position of BCC at an arbitrary position
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CNPC_Combine::Weapon_ShootPosition( )
{
 	Vector right;
 	GetVectors( NULL, &right, NULL );

	// FIXME: rename this "estimated" since it's not based on animation
	// FIXME: the orientation won't be correct when testing from arbitary positions for arbitary angles

   	if (m_bStanding )
   	{
 		return GetAbsOrigin() + COMBINE_GUN_STANDING_POSITION + right * 8;
   	}
   	else
   	{
 		return GetAbsOrigin() + COMBINE_GUN_CROUCHING_POSITION + right * 8;
   	}
}


//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
#define FLANK_SENTENCE		0
#define EOL_SENTENCE		1
#define ASSAULT_SENTENCE	2
#define COVER_SENTENCE		3
void CNPC_Combine::SpeakSentence( int sentenceType )
{
	if		(sentenceType == 0)
	{
		if (FOkToMakeSound())
		{
			SENTENCEG_PlayRndSz( edict(), "COMBINE_FLANK", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
			JustMadeSound();
		}
	}
	else if (sentenceType == 1)
	{
		AnnounceAssault();
	}
	else if (sentenceType == 2)
	{
		AnnounceAssault();
	}
}

//=========================================================
// PainSound
//=========================================================
void CNPC_Combine::PainSound ( void )
{
	if ( gpGlobals->curtime > m_flNextPainSoundTime )
	{
#if 0
		if ( random->RandomInt(0,99) < 5 )
		{
			// pain sentences are rare
			if (FOkToMakeSound())
			{
				SENTENCEG_PlayRndSz(edict(), "COMBINE_TAUNT", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, PITCH_NORM);
				JustMadeSound();
				return;
			}
		}
#endif 
		SENTENCEG_PlayRndSz( edict(), "COMBINE_PAIN", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());

		m_flNextPainSoundTime = gpGlobals->curtime + 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: implemented by subclasses to give them an opportunity to make
//			a sound when they lose their enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Combine::LostEnemySound( void)
{
	if (FOkToMakeSound() && gpGlobals->curtime > m_flNextLostSoundTime)
	{
		if (!(CBaseEntity*)GetEnemy() || gpGlobals->curtime - GetEnemyLastTimeSeen() > 10)
		{
			SENTENCEG_PlayRndSz( edict(), "COMBINE_LOST_LONG", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
			JustMadeSound();
		}
		else
		{
			SENTENCEG_PlayRndSz( edict(), "COMBINE_LOST_SHORT", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
			JustMadeSound();
		}
		m_flNextLostSoundTime = gpGlobals->curtime + random->RandomFloat(5.0,15.0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: implemented by subclasses to give them an opportunity to make
//			a sound when they lose their enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Combine::FoundEnemySound( void)
{
	// High priority so don't check FOkToMakeSound()
	SENTENCEG_PlayRndSz( edict(), "COMBINE_REFIND_ENEMY", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
	JustMadeSound();
}

//-----------------------------------------------------------------------------
// Purpose: Implemented by subclasses to give them an opportunity to make
//			a sound before they attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Combine::AlertSound( void)
{
	if ( gpGlobals->curtime > m_flNextAlertSoundTime )
	{
		// High priority so don't check FOkToMakeSound()
		SENTENCEG_PlayRndSz( edict(), "COMBINE_GO_ALERT", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
		JustMadeSound();
		m_flNextAlertSoundTime = gpGlobals->curtime + 10.0f;
	}
}

//=========================================================
// NotifyDeadFriend
//=========================================================
void CNPC_Combine::NotifyDeadFriend ( CBaseEntity* pFriend )
{
	if( FInViewCone( pFriend ) && FVisible( pFriend ) )
	{
		if (FOkToMakeSound())
		{
			SENTENCEG_PlayRndSz( edict(), "COMBINE_MAN_DOWN", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
			JustMadeSound();
		}
	}
	BaseClass::NotifyDeadFriend(pFriend);
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_Combine::DeathSound ( void )
{
	SENTENCEG_PlayRndSz( edict(), "COMBINE_DIE", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
}

//=========================================================
// IdleSound 
//=========================================================
void CNPC_Combine::IdleSound( void )
{
	if (FOkToMakeSound() && (g_fCombineQuestion || random->RandomInt(0,1)))
	{
		if (!g_fCombineQuestion)
		{
			// ask question or make statement
			switch (random->RandomInt(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(edict(), "COMBINE_CHECK", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
				g_fCombineQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(edict(), "COMBINE_QUEST", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
				g_fCombineQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(edict(), "COMBINE_IDLE", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
				break;
			}
		}
		else
		{
			switch (g_fCombineQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(edict(), "COMBINE_CLEAR", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(edict(), "COMBINE_ANSWER", COMBINE_SENTENCE_VOLUME, COMBINE_ATTN, 0, GetVoicePitch());
				break;
			}
			g_fCombineQuestion = 0;
		}
		JustMadeSound();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//			This is for Grenade attacks.  As the test for grenade attacks
//			is expensive we don't want to do it every frame.  Return true
//			if we meet minimum set of requirements and then test for actual
//			throw later if we actually decide to do a grenade attack.
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Combine::RangeAttack2Conditions( float flDot, float flDist ) 
{
	m_lastGrenadeCondition = GetGrenadeConditions( flDot, flDist );
	return m_lastGrenadeCondition;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Combine::GetGrenadeConditions( float flDot, float flDist )
{
	// Clear this condition first. The NPC will earn it again if the 
	// throw is valid.
	ClearCondition( COND_CAN_RANGE_ATTACK2 );
	
	if( m_iNumGrenades < 1 )
	{
		// Out of grenades!
		return COND_NONE;
	}

	if( flDist > 800 || flDist < 128 )
	{
		// Too close or too far!
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return COND_NONE;
	}

	// --------------------------------------------------------
	// Assume things haven't changed too much since last time
	// --------------------------------------------------------
	if (gpGlobals->curtime < m_flNextGrenadeCheck )
		return m_lastGrenadeCondition;
	
	// -----------------------
	// If moving, don't check.
	// -----------------------
	if ( m_flGroundSpeed != 0 )
		return COND_NONE;

	CBaseEntity *pEnemy = GetEnemy();

	if (!pEnemy)
		return COND_NONE;

	Vector vecEnemyLKP = GetEnemyLKP();
	if ( !( GetEnemy()->GetFlags() & FL_ONGROUND ) && GetEnemy()->GetWaterLevel() == 0 && vecEnemyLKP.z > GetAbsMaxs().z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		return COND_NONE;
	}
	
	// --------------------------------------
	//  Get target vector
	// --------------------------------------
	Vector vecTarget;
	if (random->RandomInt(0,1))
	{
		// magically know where they are
		vecTarget = Vector( pEnemy->GetAbsOrigin().x, pEnemy->GetAbsOrigin().y, pEnemy->GetAbsMins().z );
	}
	else
	{
		// toss it to where you last saw them
		vecTarget = vecEnemyLKP;
	}
	// vecTarget = m_vecEnemyLKP + (pEnemy->BodyTarget( GetAbsOrigin() ) - pEnemy->GetAbsOrigin());
	// estimate position
	// vecTarget = vecTarget + pEnemy->GetAbsVelocity() * 2;


	// ---------------------------------------------------------------------
	// Are any of my squad members near the intended grenade impact area?
	// ---------------------------------------------------------------------
	if ( m_pSquad )
	{
		if (m_pSquad->SquadMemberInRange( vecTarget, COMBINE_MIN_GRENADE_CLEAR_DIST ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
			return (COND_WEAPON_BLOCKED_BY_FRIEND);
		}
	}
	
	// Do I have enough headroom?
	trace_t tr;
	AI_TraceLine( WorldSpaceCenter(), WorldSpaceCenter() + Vector( 0, 0, 80 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	if( tr.fraction != 1.0 )
	{
		return COND_NONE;
	}

	// ---------------------------------------------------------------------
	// Check that throw is legal and clear
	// ---------------------------------------------------------------------
	// FIXME: this is only valid for hand grenades, not RPG's
	Vector vecToss = VecCheckToss( this, GetLocalOrigin() + Vector(0,0,60), vecTarget, -1, 1.0, true );

	if ( vecToss != vec3_origin )
	{
		m_vecTossVelocity = vecToss;

		// don't check again for a while.
		// JAY: HL1 keeps checking - test?
		//m_flNextGrenadeCheck = gpGlobals->curtime;
		m_flNextGrenadeCheck = gpGlobals->curtime + 0.3; // 1/3 second.
		return COND_CAN_RANGE_ATTACK2;
	}
	else
	{
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return COND_WEAPON_SIGHT_OCCLUDED;
	}
}

//-----------------------------------------------------------------------------
// Purpose: For combine melee attack (kick/hit)
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Combine::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > 64)
	{
		return COND_NONE; // COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NONE; // COND_NOT_FACING_ATTACK;
	}
	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_Combine::EyePosition( void ) 
{ 
	if ( m_bStanding )
	{
		return GetAbsOrigin() + COMBINE_EYE_STANDING_POSITION;
	}
	else
	{
		return GetAbsOrigin() + COMBINE_EYE_CROUCHING_POSITION;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nActivity - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_Combine::EyeOffset( Activity nActivity )
{
	if (CapabilitiesGet() & bits_CAP_DUCK)
	{
		if ( ( nActivity == ACT_RELOAD_LOW ) || ( nActivity == ACT_COVER_LOW ) )
			return COMBINE_EYE_CROUCHING_POSITION;
		
		if ( nActivity == ACT_COVER_MED )
			return COMBINE_EYE_CROUCHING_POSITION;	//FIXME: Guessing here...

	}
	// if the hint doesn't tell anything, assume current state
	if ( m_bStanding )
	{
		return COMBINE_EYE_STANDING_POSITION;
	}
	else
	{
		return COMBINE_EYE_CROUCHING_POSITION;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
NPC_STATE CNPC_Combine::SelectIdealState ( void )
{
	// If no schedule conditions, the new ideal state is probably the reason we're in here.

	// ---------------------------
	//  Set ideal state
	// ---------------------------
	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		{
			if ( GetEnemy() == NULL && !HasCondition( COND_ENEMY_DEAD ) )
			{
				// Lost track of my enemy. Patrol.
				m_IdealNPCState = NPC_STATE_ALERT;
				SetCondition( COND_COMBINE_SHOULD_PATROL );
				return NPC_STATE_ALERT;
			}
			break;
		}

	default:
		m_IdealNPCState = BaseClass::SelectIdealState();
		break;
	}

	return m_IdealNPCState;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Combine::OnBeginMoveAndShoot()
{
	if ( BaseClass::OnBeginMoveAndShoot() )
	{
		if ( m_iMySquadSlot == SQUAD_SLOT_NONE && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Combine::OnEndMoveAndShoot()
{
	VacateStrategySlot();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Combine::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	if( FClassnameIs( pWeapon, "weapon_ar2" )		|| 
		FClassnameIs( pWeapon, "weapon_shotgun" )	)
	{
		return WEAPON_PROFICIENCY_PERFECT;
	}

	if( FClassnameIs( pWeapon, "weapon_smg1" ) )
	{
		return WEAPON_PROFICIENCY_LOW;
	}

	return BaseClass::CalcWeaponProficiency( pWeapon );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_combine, CNPC_Combine )

	//Tasks
	DECLARE_TASK( TASK_COMBINE_FACE_TOSS_DIR )
	DECLARE_TASK( TASK_COMBINE_IGNORE_ATTACKS )
	//DECLARE_TASK( TASK_COMBINE_MOVE_AND_SHOOT )
	//DECLARE_TASK( TASK_COMBINE_MOVE_AND_AIM )

	//Activities
	DECLARE_ACTIVITY( ACT_COMBINE_STANDING_SMG1 )
	DECLARE_ACTIVITY( ACT_COMBINE_CROUCHING_SMG1) 
	DECLARE_ACTIVITY( ACT_COMBINE_STANDING_AR2 )
	DECLARE_ACTIVITY( ACT_COMBINE_CROUCHING_AR2 )
	DECLARE_ACTIVITY( ACT_COMBINE_CROUCHING_AR2 )
	DECLARE_ACTIVITY( ACT_COMBINE_WALKING_AR2 )
	DECLARE_ACTIVITY( ACT_COMBINE_STANDING_SHOTGUN )
	DECLARE_ACTIVITY( ACT_COMBINE_CROUCHING_SHOTGUN )
	DECLARE_ACTIVITY( ACT_COMBINE_THROW_GRENADE )
	DECLARE_ACTIVITY( ACT_COMBINE_LAUNCH_GRENADE )


	DECLARE_INTERACTION( g_interactionCombineKick );	

	DECLARE_SQUADSLOT( SQUAD_SLOT_GRENADE1 )
	DECLARE_SQUADSLOT( SQUAD_SLOT_GRENADE2 )

	DECLARE_CONDITION( COND_COMBINE_NO_FIRE )
	DECLARE_CONDITION( COND_COMBINE_DEAD_FRIEND )
	DECLARE_CONDITION( COND_COMBINE_SHOULD_PATROL )

	//=========================================================
	// SCHED_COMBINE_TAKE_COVER_FROM_BEST_SOUND
	//
	//	hide from the loudest sound source (to run from grenade)
	//=========================================================
	DEFINE_SCHEDULE	
	(
		SCHED_COMBINE_TAKE_COVER_FROM_BEST_SOUND,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COMBINE_RUN_AWAY_FROM_BEST_SOUND"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_FIND_COVER_FROM_BEST_SOUND	0"
		"		 TASK_RUN_PATH						0"
		"		 TASK_WAIT_FOR_MOVEMENT				0"
		"		 TASK_REMEMBER						MEMORY:INCOVER"
		"		 TASK_TURN_LEFT						179"
		"		 TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"	// Translated to cover
		""
		"	Interrupts"
	)

	DEFINE_SCHEDULE	
	(
		SCHED_COMBINE_RUN_AWAY_FROM_BEST_SOUND,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_COWER"
		"		 TASK_STOP_MOVING					0"
		"		 TASK_STORE_BESTSOUND_IN_SAVEPOSITION 0"
		"		 TASK_FIND_BACKAWAY_FROM_SAVEPOSITION	0"
		"		 TASK_RUN_PATH						0"
		"		 TASK_WAIT_FOR_MOVEMENT				0"
		"		 TASK_TURN_LEFT						179"
		"		 TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"	// Translated to cover
		""
		"	Interrupts"
	)
	//=========================================================
	//	SCHED_COMBINE_COMBAT_FAIL
	//=========================================================
	DEFINE_SCHEDULE	
	(
		SCHED_COMBINE_COMBAT_FAIL,
		
		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE "
		"		TASK_WAIT_FACE_ENEMY		2"
		"		TASK_WAIT_PVS				0"
		""
		"	Interrupts"
		"		COND_CAN_RANGE_ATTACK1 "
		"		COND_CAN_RANGE_ATTACK2 "
	)

	//=========================================================
	// SCHED_COMBINE_VICTORY_DANCE
	//=========================================================
	DEFINE_SCHEDULE	
	(
		SCHED_COMBINE_VICTORY_DANCE,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_FACE_ENEMY						0"
		"		TASK_WAIT							1.5"
		"		TASK_GET_PATH_TO_ENEMY_CORPSE		0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_FACE_ENEMY						0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_VICTORY_DANCE"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// SCHED_COMBINE_ASSAULT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_COMBINE_ASSAULT,

		"	Tasks "
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_COMBINE_ESTABLISH_LINE_OF_FIRE"
		"		TASK_GET_PATH_TO_ENEMY_LKP		0"
		"		TASK_COMBINE_IGNORE_ATTACKS		0.2"
		"		TASK_SPEAK_SENTENCE				2"
		"		TASK_SET_TOLERANCE_DISTANCE		48"
		"		TASK_RUN_PATH					0"
//		"		TASK_COMBINE_MOVE_AND_AIM		0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_COMBINE_IGNORE_ATTACKS		0.0"
		""
		"	Interrupts "
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_FAR_TO_ATTACK"
		"		COND_HEAR_DANGER"
	)

	DEFINE_SCHEDULE 
	(
		SCHED_COMBINE_ESTABLISH_LINE_OF_FIRE,

		"	Tasks "
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_FAIL_ESTABLISH_LINE_OF_FIRE"
		"		TASK_GET_PATH_TO_ENEMY_LKP_LOS	0"
		//"		TASK_COMBINE_IGNORE_ATTACKS		1.5"
		"		TASK_SET_TOLERANCE_DISTANCE		48"
		"		TASK_RUN_PATH					0"
		//"		TASK_COMBINE_MOVE_AND_AIM		0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_COMBINE_IGNORE_ATTACKS		0.0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
		"	"
		"	Interrupts "
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_FAR_TO_ATTACK"
		"		COND_HEAR_DANGER"
		"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// SCHED_COMBINE_ASSAULT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_COMBINE_PRESS_ATTACK,

		"	Tasks "
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_COMBINE_ESTABLISH_LINE_OF_FIRE"
		"		TASK_GET_PATH_TO_ENEMY_LKP		0"
		"		TASK_SET_TOLERANCE_DISTANCE		48"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		""
		"	Interrupts "
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_LOW_PRIMARY_AMMO"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_ENEMY_OCCLUDED"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// SCHED_COMBINE_COMBAT_FACE
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_COMBAT_FACE,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_FACE_ENEMY				0"
		"		 TASK_WAIT					1.5"
		//"		 TASK_SET_SCHEDULE			SCHEDULE:SCHED_COMBINE_SWEEP"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
	)

	//=========================================================
	// 	SCHED_HIDE_AND_RELOAD	
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_HIDE_AND_RELOAD,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_RELOAD"
		"		TASK_FIND_COVER_FROM_ENEMY	0"
		"		TASK_RUN_PATH				0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_REMEMBER				MEMORY:INCOVER"
		"		TASK_FACE_ENEMY				0"
		"		TASK_RELOAD					0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// SCHED_COMBINE_SIGNAL_SUPPRESS
	//	don't stop shooting until the clip is
	//	empty or combine gets hurt.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_SIGNAL_SUPPRESS,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_IDEAL					0"
		//"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_SIGNAL2"
		"		TASK_WAIT						0.5"
		"		TASK_RANGE_ATTACK1				0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_WEAPON_BLOCKED_BY_FRIEND"
		"		COND_WEAPON_SIGHT_OCCLUDED"
		"		COND_HEAR_DANGER"
		"		COND_COMBINE_NO_FIRE"
	)

	//=========================================================
	// SCHED_COMBINE_SUPPRESS
	//=========================================================
	DEFINE_SCHEDULE	
	(
		SCHED_COMBINE_SUPPRESS,
		  
		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_WAIT				0.5"
		"		TASK_RANGE_ATTACK1		0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_HEAR_DANGER"
		"		COND_COMBINE_NO_FIRE"
		"		COND_WEAPON_BLOCKED_BY_FRIEND"
	)

	//=========================================================
	// SCHED_COMBINE_WAIT_IN_COVER
	//	we don't allow danger or the ability
	//	to attack to break a combine's run to cover schedule but
	//	when a combine is in cover we do want them to attack if they can.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_WAIT_IN_COVER,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"	// Translated to cover
		"		TASK_WAIT_FACE_ENEMY			1"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// SCHED_COMBINE_TAKE_COVER1
	//=========================================================
	DEFINE_SCHEDULE	
	(
		SCHED_COMBINE_TAKE_COVER1  ,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_COMBINE_TAKECOVER_FAILED"
		"		TASK_WAIT					0.2"
		"		TASK_FIND_COVER_FROM_ENEMY	0"
		"		TASK_SPEAK_SENTENCE			3"
		"		TASK_RUN_PATH				0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_REMEMBER				MEMORY:INCOVER"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_COMBINE_WAIT_IN_COVER"
		""
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_TAKECOVER_FAILED,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		""
		"	Interrupts"
	)

	//=========================================================
	// SCHED_COMBINE_GRENADE_COVER1
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_GRENADE_COVER1,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_FIND_COVER_FROM_ENEMY			99"
		"		TASK_FIND_FAR_NODE_COVER_FROM_ENEMY	384"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SPECIAL_ATTACK1"
		"		TASK_CLEAR_MOVE_WAIT				0"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_COMBINE_WAIT_IN_COVER"
		""
		"	Interrupts"
	)

	//=========================================================
	// SCHED_COMBINE_TOSS_GRENADE_COVER1
	//
	//	 drop grenade then run to cover.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_TOSS_GRENADE_COVER1,

		"	Tasks"
		"		TASK_FACE_ENEMY						0"
		"		TASK_RANGE_ATTACK2 					0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		""
		"	Interrupts"
	)

	//=========================================================
	// SCHED_COMBINE_RANGE_ATTACK1
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_ENEMY					0"
		"		TASK_ANNOUNCE_ATTACK			1"	// 1 = primary attack
		"		TASK_WAIT_RANDOM				0.2"
		"		TASK_RANGE_ATTACK1				0"
		"		TASK_COMBINE_IGNORE_ATTACKS		0.5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_HEAVY_DAMAGE"
		"		COND_REPEATED_DAMAGE"
		"		COND_LOW_PRIMARY_AMMO"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_WEAPON_BLOCKED_BY_FRIEND"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_GIVE_WAY"
		"		COND_HEAR_DANGER"
		"		COND_COMBINE_NO_FIRE"
		""
		// Enemy_Occluded				Don't interrupt on this.  Means
		//								comibine will fire where player was after
		//								he has moved for a little while.  Good effect!!
		// WEAPON_SIGHT_OCCLUDED		Don't block on this! Looks better for railings, etc.
	)

	//=========================================================
	// 	SCHED_COMBINE_RANGE_ATTACK2	
	//
	//	secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
	//	combines's grenade toss requires the enemy be occluded.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_RANGE_ATTACK2,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_COMBINE_FACE_TOSS_DIR	0"
		"		TASK_ANNOUNCE_ATTACK		2"	// 2 = primary attack
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_RANGE_ATTACK2"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_COMBINE_WAIT_IN_COVER"	// don't run immediately after throwing grenade.
		""
		"	Interrupts"
	)

	
	//=========================================================
	// Throw a grenade, then run off and reload.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_GRENADE_AND_RELOAD,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_COMBINE_FACE_TOSS_DIR	0"
		"		TASK_ANNOUNCE_ATTACK		2"	// 2 = primary attack
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_RANGE_ATTACK2"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_HIDE_AND_RELOAD"	// don't run immediately after throwing grenade.
		""
		"	Interrupts"
	)

	//=========================================================
	// 	SCHED_COMBINE_REPEL	 
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_REPEL,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_IDEAL			0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_GLIDE "
		""
		"	Interrupts"
		"		COND_SEE_ENEMY"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_PLAYER"
	)

	//=========================================================
	// SCHED_COMBINE_REPEL_ATTACK 
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_REPEL_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_FLY "
		""
		"	Interrupts"
		"		COND_ENEMY_OCCLUDED "
	)

	//=========================================================
	// repel land
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_COMBINE_REPEL_LAND,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_LAND"
		"		TASK_GET_PATH_TO_LASTPOSITION	0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_CLEAR_LASTPOSITION			0"
		""
		"	Interrupts"
		"		COND_SEE_ENEMY"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_PLAYER"
	)

	//-----------------------------------------------------------------------------
	// Shoot at my enemy in an attempt to destroy the cover object
	// that they are hiding behind.
	//-----------------------------------------------------------------------------
	DEFINE_SCHEDULE	
	(
		SCHED_COMBINE_SHOOT_ENEMY_COVER,
		  
		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_WAIT				0.5"
		"		TASK_RANGE_ATTACK1		0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_HEAR_DANGER"
		"		COND_COMBINE_NO_FIRE"
		"		COND_WEAPON_BLOCKED_BY_FRIEND"
	)

	DEFINE_SCHEDULE	
	(
		SCHED_COMBINE_PATROL,
		  
		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WANDER						900540" 
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT						3"
		"		TASK_WAIT_RANDOM				3"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBINE_PATROL" // keep doing it
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_NEW_ENEMY"
	)

AI_END_CUSTOM_NPC()
