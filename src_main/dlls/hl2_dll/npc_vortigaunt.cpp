//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "beam_shared.h"
#include "game.h"				// For skill levels
#include "npc_talker.h"
#include "AI_Motor.h"
#include "ai_schedule.h"
#include "scripted.h"
#include "basecombatweapon.h"
#include "soundent.h"
#include "NPCEvent.h"
#include "AI_Hull.h"
#include "Animation.h"
#include "AmmoDef.h"				// For DMG_CLUB
#include "Sprite.h"
#include "npc_vortigaunt.h"
#include "activitylist.h"
#include "player.h"
#include "items.h"
#include "basegrenade_shared.h"
#include "AI_Interactions.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "globals.h"

#define		VORTIGAUNT_LIMP_HEALTH				20
#define		VORTIGAUNT_SENTENCE_VOLUME			(float)0.35 // volume of vortigaunt sentences
#define		VORTIGAUNT_VOL						0.35		// volume of vortigaunt sounds
#define		VORTIGAUNT_ATTN						ATTN_NORM	// attenutation of vortigaunt sentences
#define		VORTIGAUNT_MAX_HEAL_AMOUNT			20			// How much armor can heal before resting
#define		VORTIGAUNT_HEAL_RECHARGE			60.0		// How long to rest between heals
#define		VORTIGAUNT_ZAP_GLOWGROW_TIME		0.5			// How long does glow last
#define		VORTIGAUNT_HEAL_GLOWGROW_TIME		1.4			// How long does glow last
#define		VORTIGAUNT_GLOWFADE_TIME			0.5			// How long does it fade
#define		VORTIGAUNT_STOMP_DIST				40
#define		VORTIGAUNT_BEAM_HURT				0
#define		VORTIGAUNT_BEAM_HEAL				1
#define		VORTIGAUNT_STOMP_TURN_OFFSET		20

#define		VORTIGAUNT_LEFT_CLAW				1
#define		VORTIGAUNT_RIGHT_CLAW				2

#define VORT_CURE "VORT_CURE"
#define VORT_CURESTOP "VORT_CURESTOP"
#define VORT_CURE_INTERRUPT "VORT_CURE_INTERRUPT"
#define VORT_ATTACK "VORT_ATTACK"
#define VORT_MAD "VORT_MAD"
#define VORT_SHOT "VORT_SHOT"
#define VORT_PAIN "VORT_PAIN"
#define VORT_DIE "VORT_DIE"
#define VORT_KILL "VORT_KILL"
#define VORT_LINE_FIRE "VORT_LINE_FIRE"
#define VORT_POK "VORT_POK"

ConVar	sk_vortigaunt_health( "sk_vortigaunt_health","0");
ConVar	sk_vortigaunt_dmg_claw( "sk_vortigaunt_dmg_claw","0");
ConVar	sk_vortigaunt_dmg_rake( "sk_vortigaunt_dmg_rake","0");
ConVar	sk_vortigaunt_dmg_zap( "sk_vortigaunt_dmg_zap","0");

//=========================================================
// Vortigaunt activities
//=========================================================
int	ACT_VORTIGAUNT_AIM;
int ACT_VORTIGAUNT_START_HEAL;
int ACT_VORTIGAUNT_HEAL_LOOP;
int ACT_VORTIGAUNT_END_HEAL;
int ACT_VORTIGAUNT_TO_ACTION;
int ACT_VORTIGAUNT_TO_IDLE;
int ACT_VORTIGAUNT_STOMP;
int ACT_VORTIGAUNT_DEFEND;
int ACT_VORTIGAUNT_TO_DEFEND;
int ACT_VORTIGAUNT_FROM_DEFEND;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		VORTIGAUNT_AE_CLAW_LEFT			( 1 )
#define		VORTIGAUNT_AE_CLAW_RIGHT		( 2 )

#define		VORTIGAUNT_AE_ZAP_POWERUP		( 3 )
#define		VORTIGAUNT_AE_ZAP_SHOOT			( 4 )
#define		VORTIGAUNT_AE_ZAP_DONE			( 5 )
#define		VORTIGAUNT_AE_HEAL_STARTGLOW	( 6 )
#define		VORTIGAUNT_AE_HEAL_STARTBEAMS	( 7 )
#define		VORTIGAUNT_AE_KICK				( 8 )
#define		VORTIGAUNT_AE_STOMP				( 9 )
#define		VORTIGAUNT_AE_HEAL_STARTSOUND	( 10 )
#define		VORTIGAUNT_AE_SWING_SOUND		( 11 )
#define		VORTIGAUNT_AE_SHOOT_SOUNDSTART	( 12 )	

#define		VORTIGAUNT_AE_DEFEND_BEAMS		( 13 )  // Short beam on had in grenade defend mode

//=========================================================
// Vortigaunt schedules
//=========================================================
enum
{
	SCHED_VORTIGAUNT_STAND = CNPC_Vortigaunt::BaseClass::NEXT_SCHEDULE,
	SCHED_VORTIGAUNT_RANGE_ATTACK,
	SCHED_VORTIGAUNT_MELEE_ATTACK,
	SCHED_VORTIGAUNT_STOMP_ATTACK,
	SCHED_VORTIGAUNT_HEAL,
	SCHED_VORTIGAUNT_BARNACLE_HIT,
	SCHED_VORTIGAUNT_BARNACLE_PULL,
	SCHED_VORTIGAUNT_BARNACLE_CHOMP,
	SCHED_VORTIGAUNT_BARNACLE_CHEW,
	SCHED_VORTIGAUNT_GRENADE_DEFEND,
	SCHED_VORTIGAUNT_GRENADE_KILL,
};


//=========================================================
// Vortigaunt Tasks 
//=========================================================
enum 
{
	TASK_VORTIGAUNT_HEAL_WARMUP = CNPC_Vortigaunt::BaseClass::NEXT_TASK,
	TASK_VORTIGAUNT_HEAL,
	TASK_VORTIGAUNT_FACE_STOMP,
	TASK_VORTIGAUNT_STOMP_ATTACK,
	TASK_VORTIGAUNT_GRENADE_KILL,
	TASK_VORTIGAUNT_ZAP_GRENADE_OWNER,
};

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionVortigauntStomp		= 0;
int	g_interactionVortigauntStompFail	= 0;
int	g_interactionVortigauntStompHit		= 0;
int	g_interactionVortigauntKick			= 0;
int	g_interactionVortigauntClaw			= 0;

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Vortigaunt )

	DEFINE_FIELD( CNPC_Vortigaunt,	m_flNextNPCThink,		FIELD_TIME),
	DEFINE_ARRAY( CNPC_Vortigaunt,	m_pBeam,				FIELD_CLASSPTR,		VORTIGAUNT_MAX_BEAMS ),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_iBeams,				FIELD_INTEGER),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_nLightningSprite,		FIELD_INTEGER),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_fGlowAge,				FIELD_FLOAT),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_fGlowScale,			FIELD_FLOAT),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_fGlowChangeTime,		FIELD_FLOAT),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_bGlowTurningOn,		FIELD_BOOLEAN),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_nCurGlowIndex,		FIELD_INTEGER),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_pLeftHandGlow,		FIELD_CLASSPTR),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_pRightHandGlow,		FIELD_CLASSPTR),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_flNextHealTime,		FIELD_TIME),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_flNextHealStepTime,	FIELD_FLOAT),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_nCurrentHealAmt,		FIELD_INTEGER),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_nLastArmorAmt,		FIELD_INTEGER),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_iSuitSound,			FIELD_INTEGER),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_flSuitSoundTime,		FIELD_TIME),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_painTime,				FIELD_TIME),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_nextLineFireTime,		FIELD_TIME),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_bInBarnacleMouth,		FIELD_BOOLEAN),
	DEFINE_FIELD( CNPC_Vortigaunt,	m_hVictim,				FIELD_EHANDLE ),
	//								m_StandoffBehavior	(auto saved by AI)
	//								m_AssaultBehavior	(auto saved by AI)

	// Function Pointers
	DEFINE_THINKFUNC( CNPC_Vortigaunt, VortigauntThink ),

END_DATADESC()
LINK_ENTITY_TO_CLASS( npc_vortigaunt, CNPC_Vortigaunt );

// for special behavior with rollermines
static bool IsRoller( CBaseEntity *pRoller )
{
	return FClassnameIs( pRoller, "npc_rollermine" );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::IsHealPositionValid(void)
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex( 1 );
	// No player
	if (!pPlayer)
	{
		return false;
	}

	// If player far away
	if ((GetLocalOrigin() - pPlayer->GetLocalOrigin()).LengthSqr() > 40000)
	{
		return false;
	}

	// If not facing player
	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );
	Vector vecSrc = GetLocalOrigin() + forward * 2;
	Vector vecAim = forward;
	trace_t tr;
	AI_TraceLine(vecSrc, vecSrc + vecAim * 1024, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
	if ( tr.m_pEnt != pPlayer)
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
#define VORT_HEAL_SENTENCE			0
#define VORT_DONE_HEAL_SENTENCE		1
void CNPC_Vortigaunt::SpeakSentence( int sentenceType )
{
	if (sentenceType == VORT_HEAL_SENTENCE)
	{
		if (IsHealPositionValid())
		{
			Speak( VORT_CURE );
		}
		else
		{
			TaskFail(FAIL_BAD_POSITION);
		}
	}
	else if (sentenceType == VORT_DONE_HEAL_SENTENCE)
	{
		Speak( VORT_CURESTOP );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask)
	{
	case TASK_VORTIGAUNT_HEAL_WARMUP:
		{
			SetActivity((Activity)ACT_VORTIGAUNT_START_HEAL);
			break;
		}
	case TASK_VORTIGAUNT_HEAL:
		{
			SetActivity((Activity)ACT_VORTIGAUNT_HEAL_LOOP);
			break;
		}
	case TASK_VORTIGAUNT_FACE_STOMP:
		{
			if (GetEnemy()!=NULL)
			{
				GetMotor()->SetIdealYaw( CalcIdealYaw ( GetEnemy()->GetLocalOrigin() ) + VORTIGAUNT_STOMP_TURN_OFFSET );
				SetTurnActivity(); 
			}
			else
			{
				TaskFail(FAIL_NO_ENEMY);
			}
			break;
		}
	case TASK_VORTIGAUNT_STOMP_ATTACK:
		{
			m_flLastAttackTime = gpGlobals->curtime;
			ResetIdealActivity( (Activity)ACT_VORTIGAUNT_STOMP );
			break;
		}
	case TASK_VORTIGAUNT_GRENADE_KILL:
		{
			if (GetTarget() == NULL)
			{
				TaskComplete();
			}
			// Used to set delay between beam shot and damage
			m_flWaitFinished = 0;

			break;
		}
	case TASK_VORTIGAUNT_ZAP_GRENADE_OWNER:
		{
			if (GetEnemy() == NULL)
			{
				TaskComplete();
			}
			// Used to set delay between beam shot and damage
			m_flWaitFinished = 0;

			break;
		}
	default:
		{
			BaseClass::StartTask( pTask );	
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			if (GetEnemy() != NULL)
			{
				if (GetEnemy()->IsPlayer())
				{
					m_flPlaybackRate = 1.5;
				}
				if (!GetEnemy()->IsAlive())
				{
					if( IsActivityFinished() )
					{
						TaskComplete();
						break;
					}
				}
				// This is along attack sequence so if the enemy
				// becomes occluded bail
				if (HasCondition( COND_ENEMY_OCCLUDED ))
				{
					TaskComplete();
					break;
				}
			}
			BaseClass::RunTask( pTask );
			break;
		}
	case TASK_MELEE_ATTACK1:
		{
			if (GetEnemy() == NULL)
			{
				if ( IsActivityFinished() )
				{
					TaskComplete();
				}
			}
			else
			{
				BaseClass::RunTask( pTask );
			}
			break;	
		}
	case TASK_MELEE_ATTACK2:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_VORTIGAUNT_STOMP_ATTACK:
		{
			// Face enemy slightly off center 
			if (GetEnemy() != NULL)
			{
				GetMotor()->SetIdealYawAndUpdate( CalcIdealYaw ( GetEnemy()->GetLocalOrigin() ) + VORTIGAUNT_STOMP_TURN_OFFSET, AI_KEEP_YAW_SPEED );
			}

			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_VORTIGAUNT_FACE_STOMP:
		{
			if (GetEnemy()!=NULL)
			{
				GetMotor()->SetIdealYawAndUpdate( CalcIdealYaw( GetEnemy()->GetLocalOrigin() ) + VORTIGAUNT_STOMP_TURN_OFFSET );

				if ( FacingIdeal() )
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail(FAIL_NO_ENEMY);
			}
			break;
		}
	case TASK_VORTIGAUNT_HEAL_WARMUP:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}

			if (!IsHealPositionValid())
			{
				Speak( VORT_CURE_INTERRUPT );
				ClearBeams();
				EndHandGlow();
				m_flNextHealTime = gpGlobals->curtime + 5.0;
				TaskFail(FAIL_BAD_POSITION);
				return;
			}
			break;
		}
	case TASK_VORTIGAUNT_GRENADE_KILL:
		{
			if (GetTarget() == NULL)
			{
				TaskComplete();
				break;
			}

			// Face the grenade
			GetMotor()->SetIdealYawToTargetAndUpdate( GetTarget()->GetLocalOrigin() );

			// Shoot beam when grenade is close
			float flDist = (GetTarget()->GetLocalOrigin() - GetLocalOrigin()).Length();

			if (flDist < 225)
			{
				ClawBeam(GetTarget(),16,VORTIGAUNT_LEFT_CLAW);
				ClawBeam(GetTarget(),16,VORTIGAUNT_RIGHT_CLAW);
			}
			if (flDist < 150)
			{
				CBaseGrenade* pGrenade = dynamic_cast<CBaseGrenade*>((CBaseEntity*)GetTarget());
				if (!pGrenade)
				{
					TaskComplete();
				}
				pGrenade->Detonate();
				TaskComplete();
			}
			break;
		}
	case TASK_VORTIGAUNT_ZAP_GRENADE_OWNER:
		{
			if (GetEnemy() == NULL)
			{
				TaskComplete();
				break;
			}

			// Face the grenade owner
			GetMotor()->SetIdealYawToTargetAndUpdate( GetEnemy()->GetLocalOrigin() );

			// Shoot beam only if enemy is close enough
			float flDist = (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()).Length();
			if (flDist < 350)
			{
				if (m_flWaitFinished == 0					||
					m_flWaitFinished >  gpGlobals->curtime )
				{
					ClawBeam(GetEnemy(),5,VORTIGAUNT_LEFT_CLAW);
					ClawBeam(GetEnemy(),5,VORTIGAUNT_RIGHT_CLAW);

					// Set a delay and then we do damage.  Store in
					// m_hVictim in case we are interrupted
					if (m_flWaitFinished == 0)
					{
						m_flWaitFinished = gpGlobals->curtime + 0.2;
						m_hVictim		 = GetEnemy();
					}
				}
				else if (m_flWaitFinished < gpGlobals->curtime)
				{
					if (m_hVictim != NULL)
					{
						CTakeDamageInfo info( this, this, 15, DMG_SHOCK );
						CalculateMeleeDamageForce( &info, (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()), GetEnemy()->GetAbsOrigin() );
						m_hVictim->MyNPCPointer()->TakeDamage( info );
					}
					TaskComplete();
				}
			}
			else
			{
				// To far away to shoot
				TaskComplete();
			}
			break;
		}
	case TASK_VORTIGAUNT_HEAL:
		{
			if (gpGlobals->curtime > m_flNextHealStepTime)
			{
				CBasePlayer* pPlayer = UTIL_PlayerByIndex( 1 );

				if (pPlayer)
				{
					// -------------------------
					// Is positioning valid?
					// -------------------------
					if (!IsHealPositionValid())
					{
						Speak( VORT_CURE_INTERRUPT );
						ClearBeams();
						EndHandGlow();
						m_flNextHealTime = gpGlobals->curtime + 5.0;
						TaskFail(FAIL_BAD_POSITION);
						return;
					}

					pPlayer->IncrementArmorValue( 1, 100 );
					m_nCurrentHealAmt	  += 1;
					// ---------------------------------------
					// Only allowed to heal to a maximum
					// amount before I have to rest and can
					// heal again
					// ---------------------------------------
					if (m_nCurrentHealAmt > VORTIGAUNT_MAX_HEAL_AMOUNT)
					{
						m_flNextHealTime	= gpGlobals->curtime + VORTIGAUNT_HEAL_RECHARGE;
						m_nCurrentHealAmt	= 0;

						// Remember players armor value, only heal again
						// if this goes down.  This forces player to have
						// another combat before they can get another healing session
						m_nLastArmorAmt		= pPlayer->ArmorValue();
						ClearBeams();
						EndHandGlow();
						TaskComplete();
						return;
					}

					// ---------------------------------------
					// If player at max quit
					// ---------------------------------------
					if (pPlayer->ArmorValue() >= 100)
					{
						ClearBeams();
						EndHandGlow();
						TaskComplete();
						return;
					}

					// ------------------------------------------
					//  Suit sound
					// ------------------------------------------
					// Play the on sound or the looping charging sound
					if (!m_iSuitSound)
					{
						m_iSuitSound++;
						EmitSound( "NPC_Vortigaunt.SuitOn" );
						m_flSuitSoundTime = 0.56 + gpGlobals->curtime;
					}
					if ((m_iSuitSound == 1) && (m_flSuitSoundTime <= gpGlobals->curtime))
					{
						m_iSuitSound++;

						CPASAttenuationFilter filter( this, "NPC_Vortigaunt.SuitCharge" );
						filter.MakeReliable();

						EmitSound( filter, entindex(), "NPC_Vortigaunt.SuitCharge" );
					}

					// Suit reports new power level
					// For some reason this wasn't working in release build -- round it.
					int pct = (int)( (float)(pPlayer->ArmorValue() * 100.0) * (1.0/MAX_NORMAL_BATTERY) + 0.5);
					pct = (pct / 5);
					if (pct > 0)
						pct--;

					char szcharge[64];
					Q_snprintf( szcharge,sizeof(szcharge),"!HEV_%1dP", pct );					
					pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);
				}
				m_flNextHealStepTime += 0.1;
			}

			break;
		}
	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}




//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CNPC_Vortigaunt::GetSoundInterests ( void) 
{
	return	SOUND_WORLD	|
			SOUND_COMBAT	|
			SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_DANGER	|
			SOUND_PLAYER;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Class_T	CNPC_Vortigaunt::Classify ( void )
{
	return	CLASS_VORTIGAUNT;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CNPC_Vortigaunt::AlertSound( void )
{
	if ( GetEnemy() != NULL )
	{
		if ( IsOkToCombatSpeak() )
		{
			Speak( VORT_ATTACK );
		}
	}

}
//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_Vortigaunt::MaxYawSpeed ( void )
{
	switch ( GetActivity() )
	{
	case ACT_IDLE:		
		return 100;
		break;
	case ACT_WALK:
		return 100;
		break;
	case ACT_RUN:
		return 120;
		break;
	default:
		return 100;
		break;
	}
}

//------------------------------------------------------------------------------
// Purpose : For innate range attack
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_Vortigaunt::RangeAttack1Conditions( float flDot, float flDist )
{
	if (GetEnemy() == NULL)
	{
		return( COND_NONE );
	}

	if ( gpGlobals->curtime < m_flNextAttack )
	{
		return( COND_NONE );
	}

	// Range attack is ineffective on manhack so never use it
	// Melee attack looks a lot better anyway
	if (GetEnemy()->Classify() == CLASS_MANHACK)
	{
		return( COND_NONE );
	}

	if ( flDist <= 70 )
	{
		return( COND_TOO_CLOSE_TO_ATTACK );
	}
	else if ( flDist > 1500 * 12 )	// 1500ft max
	{
		return( COND_TOO_FAR_TO_ATTACK );
	}
	else if ( flDot < 0.65 )
	{
		return( COND_NOT_FACING_ATTACK );
	}

	return( COND_CAN_RANGE_ATTACK1 );
}

//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > 70)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		// If I'm not facing attack clear TOOFAR which may have been set by range attack
		// Otherwise we may try to back away even when melee attack is valid
		ClearCondition(COND_TOO_FAR_TO_ATTACK);
		return COND_NOT_FACING_ATTACK;
	}

	// no melee attacks against rollermins
	if ( IsRoller(GetEnemy()) )
		return COND_NONE;

	return COND_CAN_MELEE_ATTACK1;
}

//------------------------------------------------------------------------------
// Purpose : Returns who I hit on a kick (or NULL)
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity*	CNPC_Vortigaunt::Kick( void )
{
	trace_t tr;

	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	Vector vecStart = GetAbsOrigin();
	vecStart.z += WorldAlignSize().z * 0.5;
	Vector vecEnd = vecStart + (forward * 70);

	AI_TraceHull( vecStart, vecEnd, Vector(-16,-16,-18), Vector(16,16,18), 
		MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

	return tr.m_pEnt;
}

//------------------------------------------------------------------------------
// Purpose : Vortigaunt claws at his enemy
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::Claw( int nHand )
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 40, Vector(-10,-10,-10), Vector(10,10,10),sk_vortigaunt_dmg_claw.GetFloat(), DMG_SLASH );
	if ( pHurt )
	{
		pHurt->ViewPunch( QAngle(5,0,-18) );
		// If hit manhack make temp glow
		if (pHurt->Classify() == CLASS_MANHACK)
		{
			ClawBeam(pHurt,16,nHand);
		}
		// Play a random attack hit sound
		EmitSound( "NPC_Vortigaunt.Claw" );
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_Vortigaunt::HandleAnimEvent( animevent_t *pEvent )
{
	
	switch( pEvent->event )
	{
		case NPC_EVENT_LEFTFOOT:
			{
				EmitSound( "NPC_Vortigaunt.FootstepLeft" );
			}
			break;
		case NPC_EVENT_RIGHTFOOT:
			{
				EmitSound( "NPC_Vortigaunt.FootstepRight" );
			}
			break;

		case VORTIGAUNT_AE_CLAW_LEFT:
		{ 
			Claw(VORTIGAUNT_LEFT_CLAW);
		}
		break;

		case VORTIGAUNT_AE_CLAW_RIGHT:
		{
			Claw(VORTIGAUNT_RIGHT_CLAW);
		}
		break;

		case VORTIGAUNT_AE_ZAP_POWERUP:
		{
			// speed up attack when on hard
			if (g_iSkillLevel == SKILL_HARD)
				m_flPlaybackRate = 1.5;

			ArmBeam( -1 ,VORTIGAUNT_BEAM_HURT);
			ArmBeam( 1	,VORTIGAUNT_BEAM_HURT);
			BeamGlow( );

			// Make hands glow if not already glowing
			if (m_fGlowAge == 0)
			{
				StartHandGlow(VORTIGAUNT_BEAM_HURT);
				SetThink(VortigauntThink);
			}

			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), CHAN_WEAPON, "npc/vort/attack_charge.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
			//m_nSkin = m_iBeams / 2;
		}
		break;

		case VORTIGAUNT_AE_ZAP_SHOOT:
		{
			ClearBeams( );

			ClearMultiDamage();

			ZapBeam( -1 );
			ZapBeam( 1 );
			EndHandGlow();

			EmitSound( "NPC_Vortigaunt.Shoot" );
			ApplyMultiDamage();

			m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 0.5, 4.0 );
		}
		break;

		case VORTIGAUNT_AE_SWING_SOUND:
		{
			EmitSound( "NPC_Vortigaunt.Swing" );
			break;
		}
		case VORTIGAUNT_AE_SHOOT_SOUNDSTART:
		{
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), CHAN_WEAPON, "npc/vort/attack_shoot.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
			break;
		}
		case VORTIGAUNT_AE_HEAL_STARTSOUND:
		{
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), CHAN_WEAPON, "npc/vort/health_charge.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
			break;
		}
		case VORTIGAUNT_AE_HEAL_STARTGLOW:
		{
			// Make hands glow
			StartHandGlow(VORTIGAUNT_BEAM_HEAL);
			SetThink(VortigauntThink);
			break;
		}
		case VORTIGAUNT_AE_HEAL_STARTBEAMS:
		{
			// speed up heal when on easy
			//if (g_iSkillLevel == SKILL_EASY)
			//	m_flPlaybackRate = 1.5;

			// --------------------------
			// If positioning invalid, quit
			// --------------------------
			if (!IsHealPositionValid())
			{
				Speak( VORT_CURE_INTERRUPT );
				ClearBeams();
				EndHandGlow();
				m_flNextHealTime = gpGlobals->curtime + 5.0;
				TaskFail(FAIL_BAD_POSITION);
				return;
			}

			HealBeam(-1);

			//m_nSkin = m_iBeams / 2;
			break;
		}

		case VORTIGAUNT_AE_ZAP_DONE:
		{
			ClearBeams( );
			break;
		}

		case VORTIGAUNT_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();
			CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHurt );
			if (pBCC)
			{
				Vector forward, up;
				AngleVectors( GetLocalAngles(), &forward, NULL, &up );

				if ( pBCC->HandleInteraction( g_interactionVortigauntKick, NULL, this ) )
				{
					pBCC->ApplyAbsVelocityImpulse( forward * 300 + up * 250 );

					CTakeDamageInfo info( this, this, pBCC->m_iHealth+1, DMG_CLUB );
					CalculateMeleeDamageForce( &info, forward, pHurt->GetAbsOrigin() );
					pBCC->TakeDamage( info );
				}
				else
				{
					pBCC->ViewPunch( QAngle(15,0,0) );

					CTakeDamageInfo info( this, this, sk_vortigaunt_dmg_claw.GetFloat(), DMG_CLUB );
					CalculateMeleeDamageForce( &info, forward, pHurt->GetAbsOrigin() );
					pBCC->TakeDamage( info );
				}
				EmitSound( "NPC_Vortigaunt.Kick" );
			}
			break;
		}
		case VORTIGAUNT_AE_STOMP:
		{
			CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();

			if ( pBCC )
			{
				float fDist = (pBCC->GetLocalOrigin() - GetLocalOrigin()).Length();
				if (fDist > VORTIGAUNT_STOMP_DIST)
				{
					pBCC->HandleInteraction( g_interactionVortigauntStompFail, NULL, this );
					return;
				}
				else
				{
					pBCC->HandleInteraction( g_interactionVortigauntStompHit, NULL, this );
				}
				EmitSound( "NPC_Vortigaunt.Kick" );
			}
			break;
		}
		case VORTIGAUNT_AE_DEFEND_BEAMS:
		{
			DefendBeams();
			break;
		}
		default:
			BaseClass::HandleAnimEvent( pEvent );
	}
	
}

//------------------------------------------------------------------------------
// Purpose : Returnst true if entity is stompable
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_Vortigaunt::IsStompable(CBaseEntity *pEntity)
{
	if (!pEntity)
		return false;

	float fDist = (pEntity->GetLocalOrigin() - GetLocalOrigin()).Length();
	if (fDist > VORTIGAUNT_STOMP_DIST)
		return false;

	CBaseCombatCharacter* pBCC = (CBaseCombatCharacter *)pEntity;
	
	if ( pBCC && pBCC->HandleInteraction( g_interactionVortigauntStomp, NULL, this ) )
		return true;

	return false;
}

//------------------------------------------------------------------------------
// Purpose : Translate some activites for the Vortigaunt
// Input   :
// Output  :
//------------------------------------------------------------------------------
Activity CNPC_Vortigaunt::NPC_TranslateActivity( Activity eNewActivity )
{
	if ((eNewActivity == ACT_SIGNAL3)									&& 
		(SelectWeightedSequence ( ACT_SIGNAL3 ) == ACTIVITY_NOT_AVAILABLE)	)
	{
		eNewActivity = ACT_IDLE;
	}

	if (eNewActivity == ACT_IDLE)
	{
		if ( m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT)
		{
			return ACT_IDLE_ANGRY;
		}
	}
	else if (eNewActivity == ACT_MELEE_ATTACK1)
	{
		// If enemy is low pick ATTACK2 (kick)
		if (GetEnemy() != NULL && (GetEnemy()->EyePosition().z - GetLocalOrigin().z) < 20)
		{
			return ACT_MELEE_ATTACK2;
		}
	}
	return BaseClass::NPC_TranslateActivity( eNewActivity );
}

//------------------------------------------------------------------------------
// Purpose : If I've been in alert for a while and nothing's happened, 
//			 go back to idle
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_Vortigaunt::ShouldGoToIdleState( void )
{
	if (m_flLastStateChangeTime + 10 < gpGlobals->curtime)
	{
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::Event_Killed( const CTakeDamageInfo &info )
{
	ClearBeams();
	ClearHandGlow();

	StopSound( "NPC_Vortigaunt.SuitCharge" );
	StopSound( entindex(), CHAN_WEAPON, "npc/vort/health_charge.wav" );
	StopSound( entindex(), CHAN_WEAPON, "npc/vort/attack_charge.wav" );
	StopSound( "NPC_Vortigaunt.Shoot" );

	BaseClass::Event_Killed( info );
}

bool CNPC_Vortigaunt::CreateBehaviors()
{
	AddBehavior( &m_AssaultBehavior );
	AddBehavior( &m_StandoffBehavior );
	//AddBehavior( &m_LeadBehavior );
	//AddBehavior( &m_FollowBehavior );
	
	return BaseClass::CreateBehaviors();
}
//=========================================================
// Spawn
//=========================================================
void CNPC_Vortigaunt::Spawn()
{
	Precache( );

	SetModel( "models/vortigaunt.mdl" );	
	SetHullType(HULL_WIDE_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_GREEN;
	m_iHealth			= sk_vortigaunt_health.GetFloat();
	SetViewOffset( Vector ( 0, 0, 64 ) );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_NPCState			= NPC_STATE_NONE;

	GetExpresser()->SetVoicePitch( random->RandomInt( 85, 110 ) );

	CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_NO_HIT_PLAYER );
	CapabilitiesAdd	( bits_CAP_INNATE_RANGE_ATTACK1 );
	CapabilitiesAdd	( bits_CAP_INNATE_MELEE_ATTACK1 );
	
	m_flNextHealTime		= 0;		// Next time allowed to heal player
	m_flNextHealStepTime	= 0;		// Healing increment
	m_nCurrentHealAmt		= 0;		// How much healed this session
	m_nLastArmorAmt			= 100;		
	m_flEyeIntegRate		= 0.6;		// Got a big eyeball so turn it slower
	m_hVictim				= NULL;
	
	m_nCurGlowIndex			= 0;
	m_pLeftHandGlow			= NULL;
	m_pRightHandGlow		= NULL;

	NPCInit();
	SetUse( FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Vortigaunt::Precache()
{

	engine->PrecacheModel("models/vortigaunt.mdl");

	m_nLightningSprite = engine->PrecacheModel("sprites/lgtning.vmt");

	enginesound->PrecacheSound("npc/vort/attack_charge.wav");
	enginesound->PrecacheSound("npc/vort/attack_shoot.wav");
	enginesound->PrecacheSound("npc/vort/health_charge.wav");
	enginesound->PrecacheSound("items/suitcharge1.wav");

	engine->PrecacheModel("sprites/blueglow1.vmt");
	engine->PrecacheModel("sprites/greenglow1.vmt");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	BaseClass::Precache();
}	

// Init talk data
void CNPC_Vortigaunt::TalkInit()
{
	
	BaseClass::TalkInit();

	// vortigaunt will try to talk to friends in this order:
	m_szFriends[0] = "npc_vortigaunt";
//	m_szFriends[1] = "npc_conscript";  disable for now

	// get voice for head - just one barney voice for now
	GetExpresser()->SetVoicePitch( 100 );
}


static bool IsFacing( CBaseCombatCharacter *pBCC, const Vector &reference )
{
	Vector vecDir = (reference - pBCC->GetLocalOrigin());
	vecDir.z = 0;
	VectorNormalize( vecDir );
	Vector vecForward = pBCC->BodyDirection2D( );

	// He's facing me, he meant it
	if ( DotProduct( vecForward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Vortigaunt::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = BaseClass::OnTakeDamage_Alive( info );
	if (!IsAlive())
	{
		return ret;
	}

	if ( m_NPCState != NPC_STATE_PRONE && (info.GetAttacker()->GetFlags() & FL_CLIENT) )
	{

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( GetEnemy() == NULL )
		{
			// If I'm already suspicious, get mad
			if (m_afMemory & bits_MEMORY_SUSPICIOUS)
			{
				// Alright, now I'm pissed!
				Speak( VORT_MAD );

				Remember( bits_MEMORY_PROVOKED );

				// Allowed to hit the player now!
				CapabilitiesRemove(bits_CAP_NO_HIT_PLAYER);

				StopFollowing();
			}
			else
			{
				// Hey, be careful with that
				Speak( VORT_SHOT );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(GetEnemy()->IsPlayer()) && (m_lifeState != LIFE_DEAD ))
		{
			Speak( VORT_SHOT );
		}
	}
	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void CNPC_Vortigaunt::PainSound ( void )
{
	if (gpGlobals->curtime < m_painTime)
		return;
	
	m_painTime = gpGlobals->curtime + random->RandomFloat(0.5, 0.75);

	Speak( VORT_PAIN );
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_Vortigaunt::DeathSound ( void )
{
	Speak( VORT_DIE );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	if ( (info.GetDamageType() & DMG_SHOCK) && FClassnameIs( info.GetAttacker(), GetClassname() ) )
	{
		// mask off damage from other vorts for now
		info.SetDamage( 0.01 );
	}

	switch( ptr->hitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			info.ScaleDamage( 0.5f );
		}
		break;
	case 10:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			info.SetDamage( info.GetDamage() - 20 );
			if (info.GetDamage() <= 0)
			{
				g_pEffects->Ricochet( ptr->endpos, (vecDir*-1.0f) );
				info.SetDamage( 0.01 );
			}
		}
		// always a head shot
		ptr->hitgroup = HITGROUP_HEAD;
		break;
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::TranslateSchedule( int scheduleType )
{
	int baseType;

	switch( scheduleType )
	{

	// JAY: Removed because this interferes with combat behavior during follow
	// Hook these to make a looping schedule
#if 0
	case SCHED_TARGET_FACE:
		{
			// call base class default so that barney will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule(scheduleType);

			if (baseType != SCHED_TALKER_IDLE_SPEAK_WAIT)
			{
				// Try healing if player is ready
				if (gpGlobals->curtime > m_flNextHealTime && !GetExpresser()->IsSpeaking())
				{
					// Only heal player if they have been in a battle since that last time they were healed
					CBasePlayer* pPlayer = UTIL_PlayerByIndex( 1 );
					if (pPlayer && pPlayer->ArmorValue() < m_nLastArmorAmt)
					{
						return SCHED_VORTIGAUNT_HEAL;
					}
				}
			}
			return baseType;
		}
		break;
#endif

	case SCHED_RANGE_ATTACK1:
		return SCHED_VORTIGAUNT_RANGE_ATTACK;
		break;

	case SCHED_MELEE_ATTACK1:
		if (IsStompable(GetEnemy()))
		{
			return SCHED_VORTIGAUNT_STOMP_ATTACK;
		}
		else
		{
			// Tell my enemy I'm about to punch them so they can do something
			// special if they want to
			if ((GetEnemy() != NULL) && (GetEnemy()->MyNPCPointer() != NULL))
			{
				Vector vFacingDir = BodyDirection2D( );
				Vector vClawPos = EyePosition() + vFacingDir*30;
				GetEnemy()->MyNPCPointer()->HandleInteraction( g_interactionVortigauntClaw, &vClawPos, this );
			}
			return SCHED_VORTIGAUNT_MELEE_ATTACK;
		}
		break;

	case SCHED_IDLE_STAND:
		{
			// call base class default so that scientist will talk
			// when standing during idle
			baseType = BaseClass::TranslateSchedule(scheduleType);

			if (baseType == SCHED_IDLE_STAND)
			{
				// just look straight ahead
				return SCHED_VORTIGAUNT_STAND;
			}
			else
				return baseType;	
			break;

		}
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{
			// Fail schedule doesn't go through SelectSchedule()
			// So we have to clear beams and glow here
			ClearBeams();
			EndHandGlow();

			return SCHED_COMBAT_FACE;
			break;
		}
	case SCHED_CHASE_ENEMY_FAILED:
		{
			baseType = BaseClass::TranslateSchedule(scheduleType);
			if ( baseType != SCHED_CHASE_ENEMY_FAILED )
				return baseType;

			if (HasMemory(bits_MEMORY_INCOVER))
			{
				// Occasionally run somewhere so I don't just
				// stand around forever if my enemy is stationary
				if (random->RandomInt(0,5) == 5)
				{
					return SCHED_PATROL_RUN;
				}
				else
				{
					return SCHED_VORTIGAUNT_STAND;
				}
			}
			break;
		}
	case SCHED_FAIL_TAKE_COVER:
		{
			// Fail schedule doesn't go through SelectSchedule()
			// So we have to clear beams and glow here
			ClearBeams();
			EndHandGlow();

			return SCHED_RUN_RANDOM;
			break;
		}
	}
	
	return BaseClass::TranslateSchedule( scheduleType );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_Vortigaunt::SelectSchedule( void )
{
	ClearBeams();
	EndHandGlow();

	if ( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();

	switch( m_NPCState )
	{
	case NPC_STATE_PRONE:
		{
			if (m_bInBarnacleMouth)
			{
				return SCHED_VORTIGAUNT_BARNACLE_CHOMP;
			}
			else
			{
				return SCHED_VORTIGAUNT_BARNACLE_HIT;
			}
		}
	case NPC_STATE_COMBAT:
		{
			// dead enemy
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			if ( HasCondition( COND_HEAVY_DAMAGE ) )
			{
				return SCHED_TAKE_COVER_FROM_ENEMY;
			}

			// ----------------------------------------------
			// If fighting a scanner and in range of scanner
			// go into defensive stance
			// ----------------------------------------------
			if (GetEnemy()			 != NULL			&& 
				GetEnemy()->Classify() == CLASS_WASTE_SCANNER	)
			{
				// If I've taken damage while in defensive position go for cover
				// about some the time
				if ( HasCondition( COND_LIGHT_DAMAGE )    &&
					 GetActivity() == ACT_VORTIGAUNT_DEFEND &&
					 random->RandomInt(0,1) == 0		)
				{
					return SCHED_TAKE_COVER_FROM_ENEMY;
				}
				// Otherwise get in grendade defend stance
				else
				{
					float flDist = (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()).Length();
					if (flDist < 300)
					{
						return SCHED_VORTIGAUNT_GRENADE_DEFEND;
					}
				}
				ClearCondition( COND_HEAR_DANGER );
			}

			if ( HasCondition( COND_HEAR_DANGER ) )
			{
				return SCHED_TAKE_COVER_FROM_BEST_SOUND;
			}

			if ( HasCondition( COND_ENEMY_DEAD ) && IsOkToCombatSpeak() )
			{
				Speak( VORT_KILL );
			}

			// If I might hit the player shooting...
			else if ( HasCondition( COND_WEAPON_PLAYER_IN_SPREAD ))
			{
				if ( IsOkToCombatSpeak() && m_nextLineFireTime	< gpGlobals->curtime)
				{
					Speak( VORT_LINE_FIRE );
					m_nextLineFireTime = gpGlobals->curtime + 3.0f;
				}

				// Run to a new location or stand and aim
				if (random->RandomInt(0,2) == 0)
				{
					return SCHED_ESTABLISH_LINE_OF_FIRE;
				}
				else
				{
					return SCHED_COMBAT_FACE;
				}
			}
		}
		break;

	case NPC_STATE_ALERT:	
	case NPC_STATE_IDLE:

		if ( HasCondition( COND_HEAR_DANGER ) )
		{
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
		}

		if ( HasCondition( COND_ENEMY_DEAD ) && IsOkToCombatSpeak() )
		{
			Speak( VORT_KILL );
		}

		break;
	}
	
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionBarnacleVictimDangle)
	{
		// Force choosing of a new schedule
		ClearSchedule();
		m_bInBarnacleMouth	= true;
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		m_IdealNPCState = NPC_STATE_IDLE;

		m_bInBarnacleMouth	= false;
		SetAbsVelocity( vec3_origin );
		SetMoveType( MOVETYPE_STEP );
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimGrab )
	{
		if ( GetFlags() & FL_ONGROUND )
		{
			RemoveFlag( FL_ONGROUND );
		}
		m_IdealNPCState = NPC_STATE_PRONE;
		PainSound();
		return true;
	}
	else if ( interactionType == g_interactionWScannerBomb )
	{
		// If I'm running grenade defend schedule, kill this grenade
		// unless I'm already killing another
		if (GetActivity() == ACT_VORTIGAUNT_DEFEND)
		{
			// If was interrupted with a m_hVictim, make sure
			// I do damage before switching enemies
			if (m_hVictim != NULL)
			{
				CTakeDamageInfo info( this, this, 15, DMG_SHOCK );
				CalculateMeleeDamageForce( &info, (m_hVictim->GetAbsOrigin() - GetAbsOrigin()), m_hVictim->GetAbsOrigin() );
				m_hVictim->TakeDamage( info );
			}

			SetTarget( (CBaseEntity*)data );
			SetEnemy( sourceEnt );
			SetSchedule(SCHED_VORTIGAUNT_GRENADE_KILL);
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

void CNPC_Vortigaunt::DeclineFollowing( void )
{
	Speak( VORT_POK );
}

//------------------------------------------------------------------------------
// Purpose : Update beam more often then regular NPC think so it doesn't
//			 move so jumpily over the ground
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::VortigauntThink(void)
{
	SetNextThink( gpGlobals->curtime + 0.025 );
	m_fGlowAge	+= 0.025;

	float fScale = 1.0;

	if (m_bGlowTurningOn)
	{
		fScale = m_fGlowAge/m_fGlowChangeTime;
	}
	else
	{
		fScale = 1.0 - (m_fGlowAge/m_fGlowChangeTime);
	}

	// Kill glow if
	if ((fScale < 1.0) && (fScale > 0.0))
	{
		m_pLeftHandGlow->SetBrightness( 230 * fScale );
		m_pLeftHandGlow->SetScale( m_fGlowScale * fScale );
		m_pRightHandGlow->SetBrightness( 230 * fScale );
		m_pRightHandGlow->SetScale( m_fGlowScale * fScale );
	}

	if (fScale < 0.0)
	{
		ClearHandGlow();
		SetThink(CallNPCThink);
	}

	if (gpGlobals->curtime > m_flNextNPCThink)
	{
		NPCThink();
		m_flNextNPCThink = gpGlobals->curtime + 0.1;
	}
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================
void CNPC_Vortigaunt::ArmBeam( int side, int beamType )
{
	trace_t tr;
	float flDist = 1.0;
	
	if (m_iBeams >= VORTIGAUNT_MAX_BEAMS)
		return;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );
	Vector vecSrc = GetLocalOrigin() + up * 36 + right * side * 16 + forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = right * side * random->RandomFloat( 0, 1 ) + up * random->RandomFloat( -1, 1 );
		trace_t tr1;
		AI_TraceLine ( vecSrc, vecSrc + vecAim * 512, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr1);
		if (flDist > tr1.fraction)
		{
			tr = tr1;
			flDist = tr.fraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	UTIL_ImpactTrace( &tr, DMG_CLUB );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.endpos, this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );

	if (beamType == VORTIGAUNT_BEAM_HEAL)
	{
		m_pBeam[m_iBeams]->SetColor( 0, 0, 255 );
	}
	else
	{
		m_pBeam[m_iBeams]->SetColor( 96, 128, 16 );
	}
	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 12.5 );
	m_iBeams++;
}

//------------------------------------------------------------------------------
// Purpose : Short beam on had in grenade defend mode
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::DefendBeams( void )
{
	Vector vBeamStart;
	QAngle vBeamAng;

	// -----------
	// Left hand
	// -----------
	int i;
	GetAttachment( 1, vBeamStart, vBeamAng );
	for (i=0;i<4;i++)
	{
		Vector vBeamPos = vBeamStart;
		vBeamPos.x += random->RandomFloat(-8,8);
		vBeamPos.y += random->RandomFloat(-8,8);
		vBeamPos.z += random->RandomFloat(-8,8);

		CBeam  *pBeam = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0 );
		pBeam->PointEntInit( vBeamPos, this );
		pBeam->SetEndAttachment( 1 );
		pBeam->SetBrightness( 255 );
		pBeam->SetNoise( 2 );
		pBeam->SetColor( 96, 128, 16 );
		pBeam->LiveForTime( 0.1 );
	}

	// -----------
	// Right hand
	// -----------
	GetAttachment( 2, vBeamStart, vBeamAng );
	for (i=4;i<VORTIGAUNT_MAX_BEAMS;i++)
	{
		Vector vBeamPos = vBeamStart;
		vBeamPos.x += random->RandomFloat(-8,8);
		vBeamPos.y += random->RandomFloat(-8,8);
		vBeamPos.z += random->RandomFloat(-8,8);

		CBeam  *pBeam = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0 );
		pBeam->PointEntInit( vBeamPos, this );
		pBeam->SetEndAttachment( 2 );
		pBeam->SetBrightness( 255 );
		pBeam->SetNoise( 2 );
		pBeam->SetColor( 96, 128, 16 );
		pBeam->LiveForTime( 0.1 );
	}
}

//------------------------------------------------------------------------------
// Purpose : Glow for when vortiguant punches manhack
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::ClawBeam( CBaseEntity *pHurt, int nNoise, int nHand )
{
	for (int i=0;i<2;i++)
	{
		CBeam* pBeam = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0 );
		pBeam->EntsInit( this, pHurt );
		pBeam->SetStartAttachment( nHand );
		pBeam->SetBrightness( 255 );
		pBeam->SetNoise( nNoise );
		pBeam->SetColor( 96, 128, 16 );
		pBeam->LiveForTime( 0.2 );
	}
	EmitSound( "NPC_Vortigaunt.ClawBeam" );
}
//------------------------------------------------------------------------------
// Purpose : Put glowing sprites on hands
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::StartHandGlow( int beamType )
{
	m_bGlowTurningOn	= true;
	m_fGlowAge		= 0;

	if (beamType == VORTIGAUNT_BEAM_HEAL)
	{
		m_fGlowChangeTime = VORTIGAUNT_HEAL_GLOWGROW_TIME;
		m_fGlowScale		= 0.8;
	}
	else 
	{
		m_fGlowChangeTime = VORTIGAUNT_ZAP_GLOWGROW_TIME;
		m_fGlowScale		= 1.3;
	}

	if (!m_pLeftHandGlow)
	{

		if (beamType == VORTIGAUNT_BEAM_HEAL)
		{
			m_pLeftHandGlow = CSprite::SpriteCreate( "sprites/blueglow1.vmt", GetLocalOrigin(), FALSE );
			m_pLeftHandGlow->SetAttachment( this, 2 );
		}
		else
		{
			m_pLeftHandGlow = CSprite::SpriteCreate( "sprites/greenglow1.vmt", GetLocalOrigin(), FALSE );
			m_pLeftHandGlow->SetAttachment( this, 1 );
		}
		m_pLeftHandGlow->SetTransparency( kRenderGlow, 255, 200, 200, 0, kRenderFxNoDissipation );
		m_pLeftHandGlow->SetBrightness( 0 );
		m_pLeftHandGlow->SetScale( 0 );
	}
	if (!m_pRightHandGlow)
	{
		if (beamType == VORTIGAUNT_BEAM_HEAL)
		{
			m_pRightHandGlow = CSprite::SpriteCreate( "sprites/blueglow1.vmt", GetLocalOrigin(), FALSE );
		}
		else
		{
			m_pRightHandGlow = CSprite::SpriteCreate( "sprites/greenglow1.vmt", GetLocalOrigin(), FALSE );
		}
			m_pRightHandGlow->SetAttachment( this, 2 );
		m_pRightHandGlow->SetTransparency( kRenderGlow, 255, 200, 200, 0, kRenderFxNoDissipation );
		m_pRightHandGlow->SetBrightness( 0 );
		m_pRightHandGlow->SetScale( 0 );
	}
}


//------------------------------------------------------------------------------
// Purpose : Fade glow from hands
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::EndHandGlow( void )
{
	m_bGlowTurningOn	= false;
	m_fGlowChangeTime = VORTIGAUNT_GLOWFADE_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: vortigaunt shoots at noisy body target (fixes problems in cover, etc)
//			NOTE: His weapon doesn't have any error
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CNPC_Vortigaunt::GetShootEnemyDir( const Vector &shootOrigin )
{
	CBaseEntity *pEnemy = GetEnemy();

	if ( pEnemy )
	{
		return (pEnemy->BodyTarget(shootOrigin, true)-shootOrigin);
	}
	else
	{
		Vector forward;
		AngleVectors( GetLocalAngles(), &forward );
		return forward;
	}
}

bool CNPC_Vortigaunt::IsValidEnemy( CBaseEntity *pEnemy )
{
	if ( IsRoller( pEnemy ) )
	{
		CAI_BaseNPC *pNPC = pEnemy->MyNPCPointer();
		if ( pNPC && pNPC->GetEnemy() != NULL )
			return true;
		return false;
	}

	return BaseClass::IsValidEnemy( pEnemy );
}

//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CNPC_Vortigaunt::BeamGlow( )
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255) 
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}


//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CNPC_Vortigaunt::WackBeam( int side, CBaseEntity *pEntity )
{
	Vector vecDest;
	
	if (m_iBeams >= VORTIGAUNT_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->WorldSpaceCenter(), this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 12 );
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CNPC_Vortigaunt::ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	trace_t tr;
	CBaseEntity *pEntity;

	if (m_iBeams >= VORTIGAUNT_MAX_BEAMS)
		return;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );

	vecSrc = GetLocalOrigin() + up * 36;
	vecAim = GetShootEnemyDir( vecSrc );
	float deflection = 0.01;
	vecAim = vecAim + side * right * random->RandomFloat( 0, deflection ) + up * random->RandomFloat( -deflection, deflection );
	AI_TraceLine ( vecSrc, vecSrc + vecAim * 1024, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 5.0 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.endpos, this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 3 );
	m_iBeams++;

	pEntity = tr.m_pEnt;
	if (pEntity != NULL && m_takedamage)
	{
		CTakeDamageInfo dmgInfo( this, this, sk_vortigaunt_dmg_zap.GetFloat(), DMG_SHOCK );
		dmgInfo.SetDamagePosition( tr.endpos );
		VectorNormalize( vecAim );// not a unit vec yet
		// hit like a 5kg object flying 400 ft/s
		dmgInfo.SetDamageForce( 5 * 400 * 12 * vecAim );
		pEntity->DispatchTraceAttack( dmgInfo, vecAim, &tr );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::HealBeam( int side )
{
	if (m_iBeams >= VORTIGAUNT_MAX_BEAMS)
		return;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );

	Vector vecSrc		= GetLocalOrigin() + up * 36;
	Vector vecAim		= BodyDirection2D( );
	float  deflection	= 0.01;

	vecAim = vecAim + side * right * random->RandomFloat( 0, deflection ) + up * random->RandomFloat( -deflection, deflection );

	trace_t tr;
	AI_TraceLine ( vecSrc, vecSrc + vecAim * 1024, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 5.0 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.endpos, this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 0, 0, 255 );

	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 6.5 );
	m_iBeams++;
}

//------------------------------------------------------------------------------
// Purpose : Clear Glow
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::ClearHandGlow( )
{
	if (m_pLeftHandGlow)
	{
		UTIL_Remove( m_pLeftHandGlow );
		m_pLeftHandGlow = NULL;
	}
	if (m_pRightHandGlow)
	{
		UTIL_Remove( m_pRightHandGlow );
		m_pRightHandGlow = NULL;
	}
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CNPC_Vortigaunt::ClearBeams( )
{
	for (int i = 0; i < VORTIGAUNT_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}

	m_iBeams = 0;
	m_nSkin = 0;

	// Stop looping suit charge sound.
	if (m_iSuitSound > 1) 
	{
		StopSound( entindex(), CHAN_STATIC, "items/suitcharge1.wav" );
		m_iSuitSound = 0;
	}

	StopSound( entindex(), CHAN_WEAPON, "npc/vort/health_charge.wav" );
	StopSound( entindex(), CHAN_WEAPON, "npc/vort/attack_charge.wav" );
}


//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_vortigaunt, CNPC_Vortigaunt )

	DECLARE_USES_SCHEDULE_PROVIDER( CAI_LeadBehavior )

	DECLARE_TASK(TASK_VORTIGAUNT_HEAL_WARMUP)
	DECLARE_TASK(TASK_VORTIGAUNT_HEAL)
	DECLARE_TASK(TASK_VORTIGAUNT_FACE_STOMP)
	DECLARE_TASK(TASK_VORTIGAUNT_STOMP_ATTACK)
	DECLARE_TASK(TASK_VORTIGAUNT_GRENADE_KILL)
	DECLARE_TASK(TASK_VORTIGAUNT_ZAP_GRENADE_OWNER)
	
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_AIM)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_START_HEAL)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_HEAL_LOOP)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_END_HEAL)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_TO_ACTION)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_TO_IDLE)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_STOMP)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_DEFEND)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_TO_DEFEND)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_FROM_DEFEND)

	DECLARE_INTERACTION( g_interactionVortigauntStomp )
	DECLARE_INTERACTION( g_interactionVortigauntStompFail )
	DECLARE_INTERACTION( g_interactionVortigauntStompHit )
	DECLARE_INTERACTION( g_interactionVortigauntKick )
	DECLARE_INTERACTION( g_interactionVortigauntClaw )

	//=========================================================
	// > SCHED_VORTIGAUNT_RANGE_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_RANGE_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_IDEAL					0"
		"		TASK_RANGE_ATTACK1				0"
		"		TASK_WAIT						0.2" // Wait a sec before killing beams
		""
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_MELEE_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_MELEE_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_FACE_ENEMY						0"
		"		TASK_MELEE_ATTACK1					0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_STOMP_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_STOMP_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_VORTIGAUNT_FACE_STOMP			0"
		"		TASK_VORTIGAUNT_STOMP_ATTACK		0"
		""
		"	Interrupts"
				// New_Enemy	Don't interrupt, finish current attack first
		"		COND_ENEMY_DEAD"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_GRENADE_DEFEND
	//
	// Zap an incoming grenade (GetTarget())
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_GRENADE_DEFEND,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_FACE_ENEMY						0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_VORTIGAUNT_DEFEND"
		""
		"	Interrupts"
		"		COND_ENEMY_OCCLUDED"
		"		COND_LIGHT_DAMAGE"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_GRENADE_KILL
	//
	// Zap an incoming grenade (GetTarget())
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_GRENADE_KILL,

		"	Tasks"
		"		TASK_VORTIGAUNT_GRENADE_KILL		0"
		"		TASK_WAIT							0.3"
		"		TASK_VORTIGAUNT_ZAP_GRENADE_OWNER	0"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
	);
	//=========================================================
	// > SCHED_VORTIGAUNT_HEAL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_HEAL,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_VORTIGAUNT_STAND"
		"		TASK_GET_PATH_TO_TARGET			0"
		"		TASK_MOVE_TO_TARGET_RANGE		128"				// Move within 128 of target ent (client)
		"		TASK_FACE_PLAYER				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_SPEAK_SENTENCE				0"					// VORT_HEAL_SENTENCE
		"		TASK_WAIT						2"					// Wait to finish sentence
		"		TASK_VORTIGAUNT_HEAL_WARMUP		0"
		"		TASK_VORTIGAUNT_HEAL			0"
		"		TASK_SPEAK_SENTENCE				1"					// VORT_DONE_HEAL_SENTENCE
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_STAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							2"					// repick IDLESTAND every two seconds."
		"		TASK_TALKER_HEADRESET				0"					// reset head position"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SMELL"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_DANGER"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_BARNACLE_HIT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_BARNACLE_HIT,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_HIT"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_VORTIGAUNT_BARNACLE_PULL"
		""
		"	Interrupts"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_BARNACLE_PULL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_BARNACLE_PULL,

		"	Tasks"
		"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_PULL"
		""
		"	Interrupts"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_BARNACLE_CHOMP
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_BARNACLE_CHOMP,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHOMP"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_VORTIGAUNT_BARNACLE_CHEW"
		""
		"	Interrupts"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_BARNACLE_CHEW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_BARNACLE_CHEW,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHEW"
	)

AI_END_CUSTOM_NPC()
