//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Motor.h"
#include "AI_Navigator.h"
#include "soundent.h"
#include "game.h"
#include "NPCEvent.h"
#include "EntityList.h"
#include "globalstate.h"
#include "basecombatweapon.h"
#include "rope.h"
#include "rope_shared.h"
#include "npc_metropolice.h"
#include "AI_Task.h"
#include "activitylist.h"
#include "AI_Interactions.h"
#include "engine/IEngineSound.h"
#include "AI_SquadSlot.h"
#include "AI_Squad.h"
#include "AI_TacticalServices.h"
#include "soundenvelope.h"
#include "npc_manhack.h"
#include "hl2_player.h"

ConVar	sk_metropolice_health( "sk_metropolice_health","0");

#define METROPOLICE_AE_DRAW_PISTOL		50
#define METROPOLICE_AE_DEPLOY_MANHACK	51

// How many clips of pistol ammo a metropolice carries.
#define METROPOLICE_NUM_CLIPS			5

// -----------------------------------------------
//	> Squad slots
// -----------------------------------------------
enum SquadSlot_T
{
	SQUAD_SLOT_POLICE_CHASE_ENEMY = LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_POLICE_HARASS, // Yell at the player with a megaphone, etc.
	SQUAD_SLOT_POLICE_DEPLOY_MANHACK,
	SQUAD_SLOT_POLICE_POINT,
	SQUAD_SLOT_POLICE_ATTACK_PISTOL1,
	SQUAD_SLOT_POLICE_ATTACK_PISTOL2,
	SQUAD_SLOT_POLICE_OVERWATCH1,
	SQUAD_SLOT_POLICE_OVERWATCH2,
	SQUAD_SLOT_POLICE_OVERWATCH3,
	SQUAD_SLOT_POLICE_ATTACK_WAND1,
	SQUAD_SLOT_POLICE_ATTACK_WAND2,
	SQUAD_SLOT_POLICE_ADVANCE,
};

//=========================================================
// Metro Police  Activities
//=========================================================
int ACT_METROPOLICE_WALK_TORCH;
int ACT_METROPOLICE_IDLE_TORCH;
int ACT_METROPOLICE_SURPRISED;
int ACT_METROPOLICE_RELOAD_PISTOL;
int ACT_METROPOLICE_DRAW_PISTOL;
int ACT_METROPOLICE_OVERWATCH;
int ACT_METROPOLICE_POINT;
int ACT_METROPOLICE_DEPLOY_MANHACK;


//=========================================================
// Metro Police schedules
//=========================================================
enum
{
	SCHED_METROPOLICE_WALK = LAST_SHARED_SCHEDULE,
	SCHED_METROPOLICE_WAKE_ANGRY,
	SCHED_METROPOLICE_SHAKE,
	SCHED_METROPOLICE_RELOAD_WEAPON,
	SCHED_METROPOLICE_OVERWATCH,
	SCHED_METROPOLICE_OVERWATCH_LOS,
	SCHED_METROPOLICE_HARASS,
	SCHED_METROPOLICE_POINT,
	SCHED_METROPOLICE_CHASE_ENEMY,
	SCHED_METROPOLICE_DRAW_PISTOL,
	SCHED_METROPOLICE_DEPLOY_MANHACK,
	SCHED_METROPOLICE_WAIT_DEPLOY_MANHACK,
	SCHED_METROPOLICE_RANGE_ATTACK_PISTOL,
	SCHED_METROPOLICE_RAPPEL_WAIT,
	SCHED_METROPOLICE_RAPPEL,
	SCHED_METROPOLICE_ADVANCE,
	SCHED_METROPOLICE_BURNING_RUN,
	SCHED_METROPOLICE_BURNING_STAND,
};

//-----------------------------------------------------------------------------
// Manhack tasks.
//-----------------------------------------------------------------------------
enum 
{
	TASK_METROPOLICE_RELOAD_WEAPON = LAST_SHARED_TASK,
	TASK_METROPOLICE_HARASS,
	TASK_METROPOLICE_THREATEN_DEPLOY,
	TASK_METROPOLICE_SPEAK_LOCATE,
	TASK_METROPOLICE_RAPPEL,
	TASK_METROPOLICE_DIE_INSTANTLY,
};

//-----------------------------------------------------------------------------
// Custom Conditions
//-----------------------------------------------------------------------------
enum MPolice_Conds
{
	COND_METROPOLICE_ZAPPED	= LAST_SHARED_CONDITION,
	COND_METROPOLICE_BEGIN_RAPPEL,
	COND_METROPOLICE_ON_FIRE,
};

LINK_ENTITY_TO_CLASS( npc_metropolice, CNPC_MetroPolice );

BEGIN_DATADESC( CNPC_MetroPolice )

	DEFINE_FIELD( CNPC_MetroPolice, m_iPistolClips, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_MetroPolice, m_fWeaponDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_MetroPolice, m_fCanPoint, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_MetroPolice, m_LastShootSlot, FIELD_INTEGER ),
	DEFINE_EMBEDDED( CNPC_MetroPolice, m_TimeYieldShootSlot ),

	DEFINE_FIELD( CNPC_MetroPolice, m_hRappelLine, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_MetroPolice, m_hManhack, FIELD_EHANDLE ),
	//								m_StandoffBehavior (auto saved by AI)

	DEFINE_KEYFIELD( CNPC_MetroPolice, m_iManhacks, FIELD_INTEGER, "manhacks" ),
	DEFINE_INPUTFUNC( CNPC_MetroPolice, FIELD_VOID, "BeginRappel", InputBeginRappel ),
	DEFINE_OUTPUT( CNPC_MetroPolice, m_OnRappelTouchdown, "OnRappelTouchdown" ),

END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_MetroPolice::NotifyDeadFriend ( CBaseEntity* pFriend )
{
	if( pFriend == m_hManhack )
	{
		Msg("My manhack died!\n");
		m_hManhack = NULL;
	}

	BaseClass::NotifyDeadFriend(pFriend);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CNPC_MetroPolice::CNPC_MetroPolice()
{
}


void CNPC_MetroPolice::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	if( IsOnFire() )
	{
		SetCondition( COND_METROPOLICE_ON_FIRE );
	}
	else
	{
		ClearCondition( COND_METROPOLICE_ON_FIRE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Precache( void )
{
	engine->PrecacheModel("models/Police.mdl");

	UTIL_PrecacheOther( "npc_manhack" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::Spawn( void )
{
	Precache();

	SetModel( "models/Police.mdl" );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= sk_metropolice_health.GetFloat();
	m_flFieldOfView		= 0.1;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_CLIMB);
	CapabilitiesAdd( bits_CAP_USE_WEAPONS | bits_CAP_AIM_GUN );
	CapabilitiesAdd( bits_CAP_SQUAD );
	CapabilitiesAdd( bits_CAP_DUCK );

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_iPistolClips = METROPOLICE_NUM_CLIPS;

	if( m_spawnflags & METROPOLICE_SF_RAPPEL )
	{
		// If this guy's supposed to rappel, keep him from
		// falling to the ground when he spawns.
		AddFlag( FL_FLY );
	}

	NPCInit();

	m_hManhack = NULL;

	if ( GetActiveWeapon() )
	{
		CBaseCombatWeapon *pWeapon;

		pWeapon = GetActiveWeapon();

		if( FClassnameIs( pWeapon, "weapon_pistol" ) )
		{
			// Pistol starts holstered.
			m_fWeaponDrawn = false;
		}
		else
		{
			m_fWeaponDrawn = true;
		}
	}

	m_TimeYieldShootSlot.Set( 2, 6 );

	GetEnemies()->SetFreeKnowledgeDuration( 6.0 );

	m_fCanPoint = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CreateBehaviors()
{
	AddBehavior( &m_StandoffBehavior );
	
	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::InputBeginRappel( inputdata_t &inputdata )
{
	if( m_spawnflags & METROPOLICE_SF_RAPPEL )
	{
		// Send the message to begin rappeling!
		m_spawnflags &= ~METROPOLICE_SF_RAPPEL;
		SetCondition( COND_METROPOLICE_BEGIN_RAPPEL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::SurpriseSound( void )
{
	EmitSound( "NPC_MetroPolice.Surprise" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::AlertSound( void )
{
	EmitSound( "NPC_MetroPolice.Alert" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::DeathSound( void )
{
	if( !IsOnFire() )
	{
		EmitSound( "NPC_MetroPolice.Die" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::PainSound( void )
{
	// Don't make pain sounds if I'm on fire. The looping sound will take care of that for us.
	if( !( m_spawnflags & METROPOLICE_SF_NOCHATTER ) )
	{
		if( !IsOnFire() )
		{
			EmitSound( "NPC_MetroPolice.Pain" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::GetSoundInterests( void )
{
	switch( m_NPCState )
	{
	case NPC_STATE_IDLE:
		// Metropolice Don't hear the sound of the player moving when in their Idle state.
		return SOUND_COMBAT | SOUND_BULLET_IMPACT | SOUND_DANGER;
		break;

	default:
		return SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_BULLET_IMPACT | SOUND_DANGER;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_MetroPolice::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 120;
		break;
	case ACT_RUN:
	case ACT_RUN_HURT:
		return 15;
		break;
	case ACT_WALK:
	case ACT_WALK_CROUCH:
	case ACT_RUN_CROUCH:
		return 25;
		break;
	default:
		return 45;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
Class_T	CNPC_MetroPolice::Classify ( void )
{
	return CLASS_METROPOLICE;
}


//-----------------------------------------------------------------------------
// Purpose: Overridden because if the player is a criminal, we hate them.
// Input  : pTarget - Entity with which to determine relationship.
// Output : Returns relationship value.
//-----------------------------------------------------------------------------
Disposition_t CNPC_MetroPolice::IRelationType(CBaseEntity *pTarget)
{
	//
	// If it's the player and they are a criminal, we hate them.
	//
	if (pTarget->Classify() == CLASS_PLAYER)
	{
		if (GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON)
		{
			return(D_NU);
		}
	}

	return(BaseClass::IRelationType(pTarget));
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	if( OldState == NPC_STATE_COMBAT )
	{
		// leaving combat. Now we can point again next time.
		m_fCanPoint = true;
	}

	BaseClass::OnStateChange( OldState, NewState );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : *pEvent - 
//
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case METROPOLICE_AE_DRAW_PISTOL:
		m_fWeaponDrawn = true;
		break;

	case METROPOLICE_AE_DEPLOY_MANHACK:
		{
			Vector forward;

			GetVectors( &forward, NULL, NULL );

			m_iManhacks--;

			CNPC_Manhack *pManhack = (CNPC_Manhack *)CreateEntityByName( "npc_manhack" );
			
			pManhack->SetLocalOrigin( EyePosition() + forward * 64 );
			pManhack->SetLocalAngles( GetLocalAngles() );
			pManhack->AddSpawnFlags( SF_MANHACK_PACKED_UP );
			pManhack->Spawn();

			if( m_pSquad )
			{
				m_pSquad->AddToSquad( pManhack );
			}

			if( GetEnemy() )
			{
				pManhack->SetEnemy( GetEnemy() );
				pManhack->SetCondition( COND_NEW_ENEMY );
				pManhack->SetState( NPC_STATE_COMBAT );

				pManhack->UpdateEnemyMemory( GetEnemy(), GetEnemy()->GetAbsOrigin() );
			}

			pManhack->SetAbsVelocity( forward * 200 + Vector( 0, 0, 300 ) );

			m_hManhack = pManhack;
		}
		break;

	case 0:
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Activity CNPC_MetroPolice::NPC_TranslateActivity( Activity newActivity )
{
	bool bWeaponIsPistol = ( Weapon_OwnsThisType( "weapon_pistol" ) != NULL );

	if( IsOnFire() && newActivity == ACT_RUN )
	{
		return ACT_RUN_ON_FIRE;
	}

	newActivity = BaseClass::NPC_TranslateActivity( newActivity );
	
	// Do these after any behaviors have translated
	if ( newActivity == ACT_IDLE && bWeaponIsPistol && GetState() == NPC_STATE_COMBAT )
	{
		return (Activity)ACT_IDLE_ANGRY_PISTOL;
	}

	if ( newActivity == ACT_RELOAD || newActivity == ACT_RELOAD_PISTOL )
	{
		return (Activity)ACT_METROPOLICE_RELOAD_PISTOL;
	}

	return newActivity;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::SelectSchedule( void )
{
	if ( m_spawnflags & METROPOLICE_SF_RAPPEL )
	{
		return SCHED_METROPOLICE_RAPPEL_WAIT;
	}
	else if( HasCondition( COND_METROPOLICE_BEGIN_RAPPEL ) )
	{
		return SCHED_METROPOLICE_RAPPEL;
	}

	if ( HasCondition(COND_METROPOLICE_ON_FIRE) )
	{
		EmitSound( "NPC_MetroPolice.OnFireScream" );

		return SCHED_METROPOLICE_BURNING_STAND;
	}

	// Always shake when zapped
	if (HasCondition(COND_METROPOLICE_ZAPPED))
	{
		PainSound();
		return SCHED_METROPOLICE_SHAKE;
	}

	if( HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		if ( (float)GetHealth() / (float)GetMaxHealth() > .75 )
			return SCHED_RELOAD;
		else
			return SCHED_HIDE_AND_RELOAD;
	}

	// Always run for cover from danger sounds
	if ( HasCondition(COND_HEAR_DANGER))
	{
		CSound *pSound;
		pSound = GetBestSound();

		Assert( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & SOUND_DANGER)
			{
				return SCHED_TAKE_COVER_FROM_BEST_SOUND;
			}
			if (!HasCondition( COND_SEE_ENEMY ) && ( pSound->m_iType & (SOUND_PLAYER | SOUND_COMBAT) ))
			{
				GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );
			}
		}
	}

	if ( !BehaviorSelectSchedule() )
	{
		switch( m_NPCState )
		{
		case NPC_STATE_ALERT:
			break;

		case NPC_STATE_COMBAT:
			{
				float flEnemyDist;

				flEnemyDist = ( GetEnemy()->GetLocalOrigin() - GetLocalOrigin() ).Length();

				if( HasCondition( COND_NEW_ENEMY ) )
				{
					if( m_pSquad ) 
					{
						if( m_fCanPoint && OccupyStrategySlot( SQUAD_SLOT_POLICE_POINT ) )
						{
							// Flip this to false so that this NPC won't point anymore until
							// he leaves combat state and re-enters.
							m_fCanPoint = false;
							return SCHED_METROPOLICE_POINT;
							break;
						}

						if( CanDeployManhack() && OccupyStrategySlot( SQUAD_SLOT_POLICE_DEPLOY_MANHACK ) )
						{
							return SCHED_METROPOLICE_DEPLOY_MANHACK;
						}
					}
					else
					{
						// Not in a squad.
						if( CanDeployManhack() )
						{
							return SCHED_METROPOLICE_DEPLOY_MANHACK;
							break;
						}
					}
				}

				if( !m_fWeaponDrawn )
				{
					return SCHED_METROPOLICE_DRAW_PISTOL;
				}

				if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
				{
					if( ( m_LastShootSlot != SQUAD_SLOT_POLICE_ATTACK_PISTOL1 ||
						  !m_TimeYieldShootSlot.Expired() ) &&
						OccupyStrategySlot( SQUAD_SLOT_POLICE_ATTACK_PISTOL1 ) )
					{
						if ( m_LastShootSlot != SQUAD_SLOT_POLICE_ATTACK_PISTOL1 )
							m_TimeYieldShootSlot.Reset();
						m_LastShootSlot = SQUAD_SLOT_POLICE_ATTACK_PISTOL1;
						return SCHED_RANGE_ATTACK1;
					}
					else if( ( m_LastShootSlot != SQUAD_SLOT_POLICE_ATTACK_PISTOL2 ||
						  !m_TimeYieldShootSlot.Expired() ) &&
						OccupyStrategySlot( SQUAD_SLOT_POLICE_ATTACK_PISTOL2 ) )
					{
						if ( m_LastShootSlot != SQUAD_SLOT_POLICE_ATTACK_PISTOL2 )
							m_TimeYieldShootSlot.Reset();
						m_LastShootSlot = SQUAD_SLOT_POLICE_ATTACK_PISTOL2;
						return SCHED_RANGE_ATTACK1;
					}
					else if( CanDeployManhack() && OccupyStrategySlot( SQUAD_SLOT_POLICE_DEPLOY_MANHACK ) )
					{
						m_LastShootSlot = 0;
						return SCHED_METROPOLICE_DEPLOY_MANHACK;
					}
					else
					{
						m_LastShootSlot = 0;
						return SCHED_METROPOLICE_ADVANCE;
					}
				}
				// If you can't attack, but you can deploy a manhack, do it!
				else if( CanDeployManhack() && OccupyStrategySlot( SQUAD_SLOT_POLICE_DEPLOY_MANHACK ) )
				{
					return SCHED_METROPOLICE_DEPLOY_MANHACK;
				}
				// If you can't see the enemy, but you're close, overwatch
				else if( !HasCondition( COND_SEE_ENEMY ) && flEnemyDist <= 512 )
				{
					return SCHED_METROPOLICE_OVERWATCH_LOS;
				}
				else
				{
	#if 0
					Msg("DIST:%f\n", flEnemyDist );
	#endif
					return SCHED_METROPOLICE_CHASE_ENEMY;
				}
			}
			break;
		}
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY:
		if ( !IsRunningBehavior() )
			return SCHED_METROPOLICE_CHASE_ENEMY;
		break;

	case SCHED_WAKE_ANGRY:
		return SCHED_METROPOLICE_WAKE_ANGRY;
		break;

	case SCHED_FAIL_TAKE_COVER:
		return SCHED_RUN_RANDOM;
		break;

	case SCHED_METROPOLICE_OVERWATCH:
		// If asked to go into overwatch, but ammo is very low, reload! (sjb)
		if( Weapon_OwnsThisType( "weapon_pistol" ) )
		{
			if( GetActiveWeapon()->m_iClip1 <= 3 )
			{
				return SCHED_METROPOLICE_RELOAD_WEAPON;
			}
		}

		if( m_pSquad )
		{
			if( m_pSquad->IsLeader( this ) )
			{
				// I'm the leader.
				if( m_pSquad->NumMembers() == 1 )
				{
					// All alone! Hafta do my own pursuing.
					return SCHED_METROPOLICE_ADVANCE;
				}
				else
				{
					// Just shout at the player. Let a cronie advance.
					return SCHED_METROPOLICE_HARASS;
				}
			}
			else
			{
				// Just a cronie. See if I can advance
				if( OccupyStrategySlot( SQUAD_SLOT_POLICE_ADVANCE ) )
				{
					return SCHED_METROPOLICE_ADVANCE;
				}
				else
				{
					// Overwatch.
					return SCHED_METROPOLICE_OVERWATCH;
				}
			}
		}
		else
		{
			// I'm solo, harass.
			return SCHED_METROPOLICE_HARASS;
		}

		break;

	case SCHED_RELOAD:
		return SCHED_METROPOLICE_RELOAD_WEAPON;
		break;

	case SCHED_RANGE_ATTACK1:
		if( Weapon_OwnsThisType( "weapon_pistol" ) )
		{
			return SCHED_METROPOLICE_RANGE_ATTACK_PISTOL;
		}
		break;
	}
	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::StartTask( const Task_t *pTask )
{
	Vector vecNewVelocity;
	switch (pTask->iTask)
	{	
	case TASK_METROPOLICE_DIE_INSTANTLY:
		{
			CTakeDamageInfo info;

			info.SetAttacker( this );
			info.SetInflictor( this );
			info.SetDamage( m_iHealth );
			info.SetDamageType( pTask->flTaskData );
			info.SetDamageForce( Vector( 0.1, 0.1, 0.1 ) );

			TakeDamage( info );

			TaskComplete();
		}
		break;

	case TASK_METROPOLICE_RAPPEL:
		CreateDropLine();
		vecNewVelocity = GetAbsVelocity();
		vecNewVelocity.z = random->RandomFloat( -300, -400 );
		SetAbsVelocity( vecNewVelocity );
		break;

	case TASK_METROPOLICE_SPEAK_LOCATE:
		{
			if( !( m_spawnflags & METROPOLICE_SF_NOCHATTER ) )
			{
				EmitSound( "NPC_MetroPolice.LocateSpeech" );
			}
			TaskComplete();
		}
		break;

	case TASK_METROPOLICE_THREATEN_DEPLOY:
		{
			if( !( m_spawnflags & METROPOLICE_SF_NOCHATTER ) )
			{
				EmitSound( "NPC_MetroPolice.DeploySpeech" );
			}
			TaskComplete();
		}
		break;

	case TASK_METROPOLICE_HARASS:
		{
			if( !( m_spawnflags & METROPOLICE_SF_NOCHATTER ) )
			{
				
				if( GetEnemy() && GetEnemy()->GetWaterLevel() > 0 )
				{
					EmitSound( "NPC_MetroPolice.WaterSpeech" );
				}
				else
				{
					EmitSound( "NPC_MetroPolice.HidingSpeech" );
				}
			}

			TaskComplete();
		}
		break;

	case TASK_METROPOLICE_RELOAD_WEAPON:
		{
			if (GetActiveWeapon())
			{
				GetActiveWeapon()->WeaponSound( RELOAD_NPC );
				GetActiveWeapon()->m_iClip1 = GetActiveWeapon()->GetMaxClip1(); 
			}

			TaskComplete();
		}
		break;

	case TASK_GET_PATH_TO_ENEMY_LOS:
		{
			if ( GetEnemy() == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
		
			float flMaxRange = 2000;
			float flMinRange = 0;
			if (GetActiveWeapon())
			{
				flMaxRange = max(GetActiveWeapon()->m_fMaxRange1,GetActiveWeapon()->m_fMaxRange2);
				flMinRange = min(GetActiveWeapon()->m_fMinRange1,GetActiveWeapon()->m_fMinRange2);
			}

			// Check against NPC's max range
			if (flMaxRange > m_flDistTooFar)
			{
				flMaxRange = m_flDistTooFar;
			}

			Vector posLos;
			Vector losTarget;

			losTarget = GetEnemy()->WorldSpaceCenter();
			losTarget.z = GetEnemy()->GetAbsMins().z;

			if ( GetTacticalServices()->FindLos(GetEnemy()->GetLocalOrigin(), losTarget, flMinRange, flMaxRange, 1.0, &posLos) )
			{
				AI_NavGoal_t goal( posLos, ACT_RUN, AIN_HULL_TOLERANCE );
				GetNavigator()->SetGoal( goal );
			}
			else
			{
				// no coverwhatsoever.
				TaskFail(FAIL_NO_SHOOT);
			}
			break;
		}

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_METROPOLICE_RAPPEL:
		if( GetAbsVelocity().z == 0.0 )
		{
			// something stopped us, try to get started again
			Vector vecNewVelocity = GetAbsVelocity();
			vecNewVelocity.z = random->RandomFloat( -300, -400 );
			SetAbsVelocity( vecNewVelocity );
		}

		if( GetFlags() & FL_ONGROUND )
		{
			Msg("TOUCHDOWN!\n");
			m_OnRappelTouchdown.FireOutput( this, this, 0 );
			RemoveFlag( FL_FLY );
			TaskComplete();
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pevInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

#if 0
	// Die instantly from a hit in idle/alert states
	if( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT )
	{
		info.SetDamage( m_iHealth );
	}
#endif //0 

	return BaseClass::OnTakeDamage_Alive( info ); 
}

//-----------------------------------------------------------------------------
// Purpose: Create the rope line used to rappel down.
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::CreateDropLine( void )
{
/*
	// Establish the length of the rope by tracing down.
	
	// Create hook for end of tripwire
	CBaseEntity *pHook1 = CBaseEntity::Create( "tripwire_hook", GetLocalOrigin() + Vector( 0, 0, 80 ), GetLocalAngles() );
	CBaseEntity *pHook2 = CBaseEntity::Create( "tripwire_hook", GetLocalOrigin() - Vector( 0, 0, 32 ), GetLocalAngles() );

	if (pHook1 && pHook2)
	{
		pHook1->SetOwnerEntity( this );
		pHook2->SetOwnerEntity( this );

		m_hRappelLine = CRopeKeyframe::Create(pHook1, pHook2,0,0);
		if (m_hRappelLine)
		{
			m_hRappelLine->m_Width		= 1;
			m_hRappelLine->m_RopeLength	= 3;
			m_hRappelLine->m_Slack		= 100;

			//CPASAttenuationFilter filter( this );
			//EmitSound( filter, entindex(), CHAN_BODY, "weapons/tripwire/ropeshoot.wav", 1.0, ATTN_NORM, 1.0, 100 );
		}

		pHook2->m_vecVelocity.z = -200;
	}
*/
}

//-----------------------------------------------------------------------------
// Purpose: Never consider my enemy unreachable if I have a manhack I could throw
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::IsUnreachable( CBaseEntity* pEntity )
{
	if( /*m_iManhacks > 0 &&*/ GetEnemy() == pEntity )
	{
		// My enemy is never unreachable.
		return false;
	}

	return BaseClass::IsUnreachable( pEntity );
}


//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::FInAimCone( const Vector &vecSpot )
{
	Vector los = ( vecSpot - GetAbsOrigin() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = BodyDirection2D();

	float flDot = DotProduct( los, facingDir );

	if ( flDot > 0.866 ) //30 degrees
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: I want to deploy a manhack. Can I?
//-----------------------------------------------------------------------------
bool CNPC_MetroPolice::CanDeployManhack( void )
{
	if( m_hManhack != NULL )
	{
		// Nope, already have one out.
		return false;
	}

	if( m_iManhacks < 1 )
	{
		// Nope, don't have any!
		return false;
	}

	return true;
}
 
//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_MetroPolice::BuildScheduleTestBits( void )
{
	//Always squirm if we're being squashed
	if ( !IsCurSchedule( SCHED_METROPOLICE_BURNING_RUN ) && !IsCurSchedule( SCHED_METROPOLICE_BURNING_STAND ) )
	{
		SetCustomInterruptCondition( COND_METROPOLICE_ON_FIRE );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_MetroPolice::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	if( FClassnameIs( pWeapon, "weapon_pistol" ) )
	{
		return WEAPON_PROFICIENCY_HIGH;
	}

	if( FClassnameIs( pWeapon, "weapon_smg1" ) )
	{
		return WEAPON_PROFICIENCY_VERY_LOW;
	}

	return BaseClass::CalcWeaponProficiency( pWeapon );
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_metropolice, CNPC_MetroPolice )

	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_CHASE_ENEMY );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_HARASS );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_DEPLOY_MANHACK );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_ATTACK_PISTOL1 );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_ATTACK_PISTOL2 );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_OVERWATCH1 );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_OVERWATCH2 );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_OVERWATCH3 );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_ATTACK_WAND1 );
	DECLARE_SQUADSLOT( SQUAD_SLOT_POLICE_ATTACK_WAND2 );

	DECLARE_ACTIVITY( ACT_METROPOLICE_WALK_TORCH );
	DECLARE_ACTIVITY( ACT_METROPOLICE_IDLE_TORCH );
	DECLARE_ACTIVITY( ACT_METROPOLICE_SURPRISED );
	DECLARE_ACTIVITY( ACT_METROPOLICE_RELOAD_PISTOL );
	DECLARE_ACTIVITY( ACT_METROPOLICE_DRAW_PISTOL );
	DECLARE_ACTIVITY( ACT_METROPOLICE_OVERWATCH );
	DECLARE_ACTIVITY( ACT_METROPOLICE_POINT );
	DECLARE_ACTIVITY( ACT_METROPOLICE_DEPLOY_MANHACK );

	DECLARE_TASK( TASK_METROPOLICE_RELOAD_WEAPON );
	DECLARE_TASK( TASK_METROPOLICE_HARASS );
	DECLARE_TASK( TASK_METROPOLICE_THREATEN_DEPLOY );
	DECLARE_TASK( TASK_METROPOLICE_SPEAK_LOCATE );
	DECLARE_TASK( TASK_METROPOLICE_RAPPEL );
	DECLARE_TASK( TASK_METROPOLICE_DIE_INSTANTLY );

	DECLARE_CONDITION( COND_METROPOLICE_ZAPPED );
	DECLARE_CONDITION( COND_METROPOLICE_BEGIN_RAPPEL );
	DECLARE_CONDITION( COND_METROPOLICE_ON_FIRE );


//=========================================================
// > SCHED_METROPOLICE_WALK
//=========================================================
DEFINE_SCHEDULE
( 
	SCHED_METROPOLICE_WALK,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_IDLE"
	"	"
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_NEW_ENEMY"
	"		COND_CAN_MELEE_ATTACK1"
);


//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_WAKE_ANGRY,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
	"		TASK_FACE_ENEMY					0"
	"	"
	"	Interrupts"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE 
(
	SCHED_METROPOLICE_RELOAD_WEAPON,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_RELOAD"
	"		TASK_METROPOLICE_RELOAD_WEAPON	0"
	"	"
	"	Interrupts"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE 
(
	SCHED_METROPOLICE_OVERWATCH_LOS,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_METROPOLICE_OVERWATCH"
	"		TASK_GET_PATH_TO_ENEMY_LKP_LOS	0"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_METROPOLICE_OVERWATCH"
	"	"
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_NEW_ENEMY"
	"		COND_HAVE_ENEMY_LOS"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE 
(
	SCHED_METROPOLICE_OVERWATCH,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE_ANGRY_PISTOL"
	"		TASK_FACE_ENEMY					0"
	"		TASK_WAIT_FACE_ENEMY			0.5"
	"		TASK_WAIT_PVS					0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_HARASS,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_ENEMY					0"
	"		TASK_WAIT_FACE_ENEMY			6"
	"		TASK_METROPOLICE_HARASS			0"
	"		TASK_WAIT_PVS					0"
	"	"
	"	Interrupts"
	"	"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_NEW_ENEMY"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_POINT,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_METROPOLICE_SPEAK_LOCATE	0"
	"		TASK_PLAY_SEQUENCE_FACE_ENEMY	ACTIVITY:ACT_METROPOLICE_POINT"
	"	"
	"	Interrupts"
	"	"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_DRAW_PISTOL,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_PLAY_SEQUENCE_FACE_ENEMY	ACTIVITY:ACT_METROPOLICE_DRAW_PISTOL"
	"		TASK_WAIT_FACE_ENEMY			0.1"
	"	"
	"	Interrupts"
	"	"
);

//=========================================================
// > ChaseEnemy
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_CHASE_ENEMY,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_METROPOLICE_OVERWATCH"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_GET_PATH_TO_ENEMY			0"
//	"		TASK_RUN_PATH_TIMED				1"	
	"		TASK_RUN_PATH_WITHIN_DIST		512"
//	"		TASK_WAIT_FOR_MOVEMENT			0"
	"	"
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
// The uninterruptible portion of this behavior, whereupon 
// the police actually releases the manhack.
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_DEPLOY_MANHACK,

	"	Tasks"
	"		TASK_METROPOLICE_THREATEN_DEPLOY	0"
	"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_METROPOLICE_DEPLOY_MANHACK"
	"	"
	"	Interrupts"
	"	"
);

//=========================================================
// The first portion of manhack release behavior. The leader
// is waiting to see if the player shows himself. if not,
// a manhack will be released. This schedule is interruptible
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_WAIT_DEPLOY_MANHACK,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_WAIT				3"
//	"		TASK_WAIT_FACE_ENEMY				3"
	"		TASK_METROPOLICE_THREATEN_DEPLOY	0"
	"		TASK_WAIT				1"
//	"		TASK_WAIT_FACE_ENEMY				1"
	"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_METROPOLICE_DEPLOY_MANHACK"
	"	"
	"	Interrupts"
	"		COND_SEE_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
);

//===============================================
//	> RangeAttack1
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_RANGE_ATTACK_PISTOL,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_FACE_ENEMY			0"
	"		TASK_RANGE_ATTACK1		0"
	"		TASK_WAIT				.1"
	"		TASK_WAIT_RANDOM		.4"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_RESET"
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
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_RAPPEL_WAIT,

	"	Tasks"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_RAPPEL_LOOP"
	"		TASK_WAIT_INDEFINITE			0"
	""
	"	Interrupts"
	"		COND_METROPOLICE_BEGIN_RAPPEL"
);

//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_RAPPEL,

	"	Tasks"
	"		TASK_WAIT							4" // let the rope fall
	"		TASK_SET_ACTIVITY					ACTIVITY:ACT_RAPPEL_LOOP"
	"		TASK_METROPOLICE_RAPPEL				0"
	""
	"	Interrupts"
	""
);

//===============================================
//===============================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_ADVANCE,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_SET_ACTIVITY					ACTIVITY:ACT_METROPOLICE_OVERWATCH"
	"		TASK_FACE_ENEMY						0"
	"		TASK_WAIT_FACE_ENEMY				1" // give the guy some time to come out on his own
	"		TASK_WAIT_FACE_ENEMY_RANDOM			3" 
	"		TASK_GET_PATH_TO_ENEMY_LOS			0"
	"		TASK_RUN_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE_ANGRY_PISTOL"
	"		TASK_FACE_ENEMY						0"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_ENEMY_DEAD"
	""
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_BURNING_RUN,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_METROPOLICE_BURNING_STAND"
	"		TASK_SET_TOLERANCE_DISTANCE		24"
	"		TASK_GET_PATH_TO_ENEMY			0"
	"		TASK_RUN_PATH_TIMED				10"	
	"		TASK_METROPOLICE_DIE_INSTANTLY	0"
	"	"
	"	Interrupts"
);

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_METROPOLICE_BURNING_STAND,

	"	Tasks"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE_ON_FIRE"
	"		TASK_WAIT						1.5"
	"		TASK_METROPOLICE_DIE_INSTANTLY	DMG_BURN"
	"		TASK_WAIT						1.0"
	"	"
	"	Interrupts"
);


AI_END_CUSTOM_NPC()

