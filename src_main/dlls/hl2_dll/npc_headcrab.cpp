//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Implements the headcrab, a tiny, jumpy alien parasite.
//
// TODO: make poison headcrab hop in response to nearby bullet impacts?
//
//=============================================================================

#include "cbase.h"
#include "game.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Navigator.h"
#include "ai_moveprobe.h"
#include "AI_Memory.h"
#include "bitstring.h"
#include "hl2_shareddefs.h"
#include "NPCEvent.h"
#include "SoundEnt.h"
#include "npc_headcrab.h"
#include "gib.h"
#include "AI_Interactions.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"
#include "world.h"

#define CRAB_ATTN_IDLE				(float)1.5
#define HEADCRAB_GUTS_GIB_COUNT		1
#define HEADCRAB_LEGS_GIB_COUNT		3
#define HEADCRAB_ALL_GIB_COUNT		5

#define HEADCRAB_MAX_JUMP_DIST		256

#define HEADCRAB_RUNMODE_ACCELERATE		1
#define HEADCRAB_RUNMODE_IDLE			2
#define HEADCRAB_RUNMODE_DECELERATE		3
#define HEADCRAB_RUNMODE_FULLSPEED		4
#define HEADCRAB_RUNMODE_PAUSE			5

#define HEADCRAB_RUN_MINSPEED	0.5
#define HEADCRAB_RUN_MAXSPEED	1.0

// Debugging
#define	HEADCRAB_DEBUG_HIDING		1

ConVar g_debug_headcrab( "g_debug_headcrab", "0", FCVAR_CHEAT );

//------------------------------------
// Spawnflags
//------------------------------------
#define SF_HEADCRAB_START_HIDDEN		(1 << 16)

//-----------------------------------------------------------------------------
// Animation events.
//-----------------------------------------------------------------------------
#define	HC_AE_JUMPATTACK				( 2 )
#define	HC_AE_JUMP_TELEGRAPH			( 3 )
#define HC_POISON_AE_FLINCH_HOP			( 4 )
#define HC_POISON_AE_FOOTSTEP			( 5 )
#define HC_POISON_AE_THREAT_SOUND		( 6 )


//-----------------------------------------------------------------------------
// Custom schedules.
//-----------------------------------------------------------------------------
enum
{
	SCHED_HEADCRAB_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,
	SCHED_HEADCRAB_WAKE_ANGRY,
	SCHED_HEADCRAB_DROWN,
	SCHED_HEADCRAB_AMBUSH,
	SCHED_HEADCRAB_DISMOUNT_NPC, // get off the NPC's head!
	SCHED_HEADCRAB_BARNACLED,
	SCHED_HEADCRAB_UNHIDE,

	SCHED_FAST_HEADCRAB_RANGE_ATTACK1,
};


//=========================================================
// tasks
//=========================================================
enum 
{
	TASK_HEADCRAB_HOP_ASIDE = LAST_SHARED_TASK,
	TASK_HEADCRAB_HOP_OFF_NPC,
	TASK_HEADCRAB_DROWN,
	TASK_HEADCRAB_WAIT_FOR_BARNACLE_KILL,
	TASK_HEADCRAB_UNHIDE,
};


//=========================================================
// conditions 
//=========================================================
enum
{
	COND_HEADCRAB_IN_WATER = LAST_SHARED_CONDITION,
	COND_HEADCRAB_ON_NPC,	// The headcrab is on an NPC's head!!
	COND_HEADCRAB_BARNACLED,
	COND_HEADCRAB_UNHIDE,
};

//=========================================================
// private activities
//=========================================================
int ACT_HEADCRAB_THREAT_DISPLAY;
int ACT_HEADCRAB_HOP_LEFT;
int ACT_HEADCRAB_HOP_RIGHT;
int ACT_HEADCRAB_DROWN;


//-----------------------------------------------------------------------------
// Skill settings.
//-----------------------------------------------------------------------------
ConVar	sk_headcrab_health( "sk_headcrab_health","0");
ConVar	sk_headcrab_fast_health( "sk_headcrab_fast_health","0");
ConVar	sk_headcrab_poison_health( "sk_headcrab_poison_health","0");
ConVar	sk_headcrab_melee_dmg( "sk_headcrab_melee_dmg","0");


BEGIN_DATADESC( CBaseHeadcrab )

	// m_nGibCount - don't save
	DEFINE_FIELD( CBaseHeadcrab, m_bHidden, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseHeadcrab, m_flTimeDrown, FIELD_TIME ),
	DEFINE_FIELD( CBaseHeadcrab, m_flNextFlinchTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseHeadcrab, m_vecJumpVel, FIELD_VECTOR ),

	// Function Pointers
	DEFINE_ENTITYFUNC( CBaseHeadcrab, LeapTouch ),

END_DATADESC()




//-----------------------------------------------------------------------------
// Purpose: 
// Input  : NewActivity - 
//-----------------------------------------------------------------------------
void CBaseHeadcrab::OnChangeActivity( Activity NewActivity )
{
	bool fRandomize = false;
	float flRandomRange = 0.0;

	// If this crab is starting to walk or idle, pick a random point within
	// the animation to begin. This prevents lots of crabs being in lockstep.
	if ( NewActivity == ACT_IDLE )
	{
		flRandomRange = 0.75;
		fRandomize = true;
	}
	else if ( NewActivity == ACT_RUN )
	{
		flRandomRange = 0.25;
		fRandomize = true;
	}

	BaseClass::OnChangeActivity( NewActivity );

	if( fRandomize )
	{
		m_flCycle = random->RandomFloat( 0.0, flRandomRange );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this monster's place in the relationship table.
// Output : 
//-----------------------------------------------------------------------------
Class_T	CBaseHeadcrab::Classify( void )
{
	return( CLASS_HEADCRAB ); 
}


//-----------------------------------------------------------------------------
// Purpose: Returns the real center of the monster. The bounding box is much larger
//			than the actual creature so this is needed for targetting.
// Output : Vector
//-----------------------------------------------------------------------------
const Vector &CBaseHeadcrab::WorldSpaceCenter( void ) const
{
	Vector &vecResult = AllocTempVector();
	vecResult = GetAbsOrigin();
	vecResult.z += 6;
	return vecResult;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &posSrc - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CBaseHeadcrab::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{ 
	return WorldSpaceCenter();
}


//-----------------------------------------------------------------------------
// Purpose: Allows each sequence to have a different turn rate associated with it.
// Output : float
//-----------------------------------------------------------------------------
float CBaseHeadcrab::MaxYawSpeed( void )
{
	return BaseClass::MaxYawSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: Does a jump attack at the given position.
// Input  : bRandomJump - Just hop in a random direction.
//			vecPos - Position to jump at, ignored if bRandom is set to true.
//			bThrown - 
//-----------------------------------------------------------------------------
void CBaseHeadcrab::JumpAttack( bool bRandomJump, const Vector &vecPos, bool bThrown )
{
	SetTouch( LeapTouch );

	RemoveFlag( FL_ONGROUND );

	//
	// Take him off ground so engine doesn't instantly reset FL_ONGROUND.
	//
	UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0 , 0 , 1 ));

	Vector vecJumpDir;
	if ( !bRandomJump )
	{
		float gravity = sv_gravity.GetFloat();
		if ( gravity <= 1 )
		{
			gravity = 1;
		}

		//
		// How fast does the headcrab need to travel to reach the position given gravity?
		//
		float height = ( vecPos.z - GetLocalOrigin().z );
		if ( height < 16 )
		{
			height = 16;
		}
		else
		{
			float flMaxHeight = bThrown ? 400 : 120;
			if ( height > flMaxHeight )
			{
				height = flMaxHeight;
			}
		}

		// overshoot the jump by an additional 8 inches
		// NOTE: This calculation jumps at a position INSIDE the box of the enemy (player)
		// so if you make the additional height too high, the crab can land on top of the
		// enemy's head.  If we want to jump high, we'll need to move vecPos to the surface/outside
		// of the enemy's box.
		
		float additionalHeight = 0;
		if ( height < 32 )
		{
			additionalHeight = 8;
		}

		// compute the time to jump the complete height
		height += additionalHeight;

		float speed = sqrt( 2 * gravity * height );
		float time = speed / gravity;

		// add in the time it takes to fall the additional height
		// So the impact takes place on the downward slope at the original height
		time += sqrt(2*gravity*additionalHeight) / gravity;

		//
		// Scale the sideways velocity to get there at the right time
		//
		vecJumpDir = vecPos - GetLocalOrigin();
		vecJumpDir = vecJumpDir / time;

		//
		// Speed to offset gravity at the desired height.
		//
		vecJumpDir.z = speed;

		//
		// Don't jump too far/fast.
		//
		float distance = vecJumpDir.Length();
		float flMaxDist = bThrown ? 1000.0f : 650.0f;
		if ( distance > flMaxDist )
		{
			vecJumpDir = vecJumpDir * ( flMaxDist / distance );
		}
	}
	else
	{
		//
		// Jump hop, don't care where.
		//
		Vector forward, up;
		AngleVectors( GetLocalAngles(), &forward, NULL, &up );
		vecJumpDir = Vector( forward.x, forward.y, up.z ) * 350;
	}

	int iSound = random->RandomInt( 0 , 1 );
	if ( iSound != 0 )
	{
		AttackSound();
	}

	SetAbsVelocity( vecJumpDir );
	m_flNextAttack = gpGlobals->curtime + 2;
}


//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific messages that occur when tagged
//			animation frames are played.
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CBaseHeadcrab::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
		case HC_AE_JUMPATTACK:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if ( pEnemy )
			{
				// Jump at my enemy's eyes.
				JumpAttack( false, pEnemy->EyePosition() );
			}
			else
			{
				// Jump hop, don't care where.
				JumpAttack( true );
			}
			break;
		}

		default:
		{
			CAI_BaseNPC::HandleAnimEvent( pEvent );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHeadcrab::Spawn( void )
{
	//Precache();
	//SetModel( "models/headcrab.mdl" );
	//m_iHealth			= sk_headcrab_health.GetFloat();
	
	SetHullType(HULL_TINY);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	SetCollisionGroup( HL2COLLISION_GROUP_HEADCRAB );

	SetViewOffset( Vector(6, 0, 11) ) ;		// Position of the eyes relative to NPC's origin.

	SetBloodColor( BLOOD_COLOR_GREEN );
	m_flFieldOfView		= 0.5;
	m_NPCState			= NPC_STATE_NONE;
	m_nGibCount			= HEADCRAB_ALL_GIB_COUNT;

	// Are we starting hidden?
	if ( m_spawnflags & SF_HEADCRAB_START_HIDDEN )
	{
		m_bHidden = true;
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetRenderColorA( 0 );
		m_nRenderMode = kRenderTransTexture;
		m_fEffects |= EF_NODRAW;
	}
	else
	{
		m_bHidden = false;
	}
	
	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1 );

	// headcrabs get to cheat for 5 seconds (sjb)
	GetEnemies()->SetFreeKnowledgeDuration( 5.0 );
}


//-----------------------------------------------------------------------------
// Purpose: Precaches all resources this monster needs.
//-----------------------------------------------------------------------------
void CBaseHeadcrab::Precache( void )
{
	BaseClass::Precache();
}	


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CBaseHeadcrab::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_HEADCRAB_WAIT_FOR_BARNACLE_KILL:
			if ( m_flNextFlinchTime < gpGlobals->curtime )
			{
				m_flNextFlinchTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 2.0f );
				PainSound();
			}
			break;

		case TASK_HEADCRAB_HOP_OFF_NPC:
			if( GetFlags() & FL_ONGROUND )
			{
				TaskComplete();
			}
			break;

		case TASK_HEADCRAB_DROWN:
			if( gpGlobals->curtime > m_flTimeDrown )
			{
				OnTakeDamage( CTakeDamageInfo( this, this, m_iHealth * 2, DMG_DROWN ) );
			}
			break;

		case TASK_RANGE_ATTACK1:
		case TASK_RANGE_ATTACK2:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
				SetTouch( NULL );
				SetIdealActivity( ACT_IDLE );
			}
			break;
		}

		default:
		{
			CAI_BaseNPC::RunTask( pTask );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: LeapTouch - this is the headcrab's touch function when it is in the air.
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBaseHeadcrab::LeapTouch( CBaseEntity *pOther )
{
	if ( pOther->Classify() == Classify() )
	{
		return;
	}

	// Don't hit if back on ground
	if ( !( GetFlags() & FL_ONGROUND ) )
	{
		if ( pOther->m_takedamage != DAMAGE_NO )
		{
			BiteSound();
			TouchDamage( pOther );
		}
		else
		{
			ImpactSound();
		}
	}
	else
	{
		ImpactSound();
	}

	SetTouch( NULL );
	SetThink ( CallNPCThink );
}


//-----------------------------------------------------------------------------
// Purpose: Deal the damage from the headcrab's touch attack.
//-----------------------------------------------------------------------------
void CBaseHeadcrab::TouchDamage( CBaseEntity *pOther )
{
	CTakeDamageInfo info( this, this, sk_headcrab_melee_dmg.GetFloat(), DMG_SLASH );
	CalculateMeleeDamageForce( &info, GetAbsVelocity(), GetAbsOrigin() );
	pOther->TakeDamage( info  );
}


//---------------------------------------------------------
//---------------------------------------------------------
void CBaseHeadcrab::GatherConditions( void )
{
	// If we're hidden, just check to see if we should unhide
	if ( m_bHidden )
	{
		// See if there's enough room for our hull to fit here. If so, unhide.
		trace_t tr;
		AI_TraceHull( GetAbsOrigin(), GetAbsOrigin(),GetHullMins(), GetHullMaxs(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction == 1.0 )
		{
			SetCondition( COND_PROVOKED );
			SetCondition( COND_HEADCRAB_UNHIDE );

			if ( g_debug_headcrab.GetInt() == HEADCRAB_DEBUG_HIDING )
			{
				NDebugOverlay::Box( GetAbsOrigin(), GetHullMins(), GetHullMaxs(), 0,255,0, true, 1.0 );
			}
		}
		else if ( g_debug_headcrab.GetInt() == HEADCRAB_DEBUG_HIDING )
		{
			NDebugOverlay::Box( GetAbsOrigin(), GetHullMins(), GetHullMaxs(), 255,0,0, true, 0.1 );
		}

		// Prevent baseclass thinking, so we don't respond to enemy fire, etc.
		return;
	}

	BaseClass::GatherConditions();

	if( m_lifeState == LIFE_ALIVE && GetWaterLevel() > 1 )
	{
		// Start Drowning!
		SetCondition( COND_HEADCRAB_IN_WATER );
	}

	// If it's been a while since we flinched, forget that we flinched so we flinch again when damaged.
	if ( gpGlobals->curtime > m_flNextFlinchTime )
	{
		Forget( bits_MEMORY_FLINCHED );
	}

	// See if I've landed on an NPC!
	CBaseEntity *ground = GetGroundEntity();
	if( ground && ( ground->MyNPCPointer() != NULL ) && GetFlags() & FL_ONGROUND )
	{
		SetCondition( COND_HEADCRAB_ON_NPC );
	}
	else
	{
		ClearCondition( COND_HEADCRAB_ON_NPC );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHeadcrab::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	// Are we fading in after being hidden?
	if ( !m_bHidden && (m_nRenderMode != kRenderNormal) )
	{
		int iNewAlpha = min( 255, GetRenderColor().a + 120 );
		if ( iNewAlpha >= 255 )
		{
			m_nRenderMode = kRenderNormal;
			SetRenderColorA( 0 );
		}
		else
		{
			SetRenderColorA( iNewAlpha );
		}
	}

	//
	// Make the crab coo a little bit in combat state.
	//
	if (( m_NPCState == NPC_STATE_COMBAT ) && ( random->RandomFloat( 0, 5 ) < 0.1 ))
	{
		IdleSound();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CBaseHeadcrab::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_HEADCRAB_WAIT_FOR_BARNACLE_KILL:
		break;

	case TASK_HEADCRAB_HOP_OFF_NPC:
		{
			CBaseEntity *ground = GetGroundEntity();
			if( ground )
			{
				// Jump behind the other NPC so I don't block their path.
				Vector vecJumpDir; 

				ground->GetVectors( &vecJumpDir, NULL, NULL );

				RemoveFlag( FL_ONGROUND );		
				
				// Bump up
				UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0, 0 , 1 ) );
				
				SetAbsVelocity( vecJumpDir * -200 + Vector( 0, 0, 100 ) );
			}
			else
			{
				// *shrug* I guess they're gone now. Or dead.
				TaskComplete();
			}
		}
		break;

		case TASK_HEADCRAB_DROWN:
			// Set the gravity really low here! Sink slowly
			SetGravity(0.1);
			m_flTimeDrown = gpGlobals->curtime + 4;
			break;

		case TASK_RANGE_ATTACK1:
		{
#ifdef WEDGE_FIX_THIS
			CPASAttenuationFilter filter( this, ATTN_IDLE );
			EmitSound( filter, entindex(), CHAN_WEAPON, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
#endif
			SetIdealActivity( ACT_RANGE_ATTACK1 );
			break;
		}

		case TASK_HEADCRAB_UNHIDE:
		{
			m_bHidden = false;
			RemoveSolidFlags( FSOLID_NOT_SOLID );
			m_fEffects &= ~EF_NODRAW;

			TaskComplete();
			break;
		}

		default:
		{
			BaseClass::StartTask( pTask );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CBaseHeadcrab::RangeAttack1Conditions ( float flDot, float flDist )
{
	if ( gpGlobals->curtime < m_flNextAttack )
	{
		return( 0 );
	}

	if ( !( GetFlags() & FL_ONGROUND ) )
	{
		return( 0 );
	}

	if ( flDist > 256 )
	{
		return( COND_TOO_FAR_TO_ATTACK );
	}
	else if ( flDot < 0.65 )
	{
		return( COND_NOT_FACING_ATTACK );
	}

	return( COND_CAN_RANGE_ATTACK1 );
}


//------------------------------------------------------------------------------
// Purpose: Override to do headcrab specific gibs
// Output :
//------------------------------------------------------------------------------
bool CBaseHeadcrab::CorpseGib( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_HeadCrab.Gib" );	

	// Throw headcrab guts 
	CGib::SpawnSpecificGibs( this, m_nGibCount, 300, 400, "models/gibs/hc_gibs.mdl");

	return true;
}

//------------------------------------------------------------------------------
// Purpose:
// Input  :
//------------------------------------------------------------------------------
void CBaseHeadcrab::Touch( CBaseEntity *pOther )
{ 
	// If someone has smacked me into a wall then gib!
	if (m_NPCState == NPC_STATE_DEAD) 
	{
		if (GetAbsVelocity().Length() > 250)
		{
			trace_t tr;
			Vector vecDir = GetAbsVelocity();
			VectorNormalize(vecDir);
			AI_TraceLine(GetAbsOrigin(), GetAbsOrigin() + vecDir * 100, 
				MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr); 
			float dotPr = DotProduct(vecDir,tr.plane.normal);
			if ((tr.fraction						!= 1.0) && 
				(dotPr <  -0.8) )
			{
				CTakeDamageInfo	info( GetWorldEntity(), GetWorldEntity(), 100.0f, DMG_CRUSH );

				info.SetDamagePosition( tr.endpos );

				Event_Gibbed( info );

				// Throw headcrab guts 
				CGib::SpawnSpecificGibs( this, HEADCRAB_GUTS_GIB_COUNT, 300, 400, "models/gibs/hc_gibs.mdl");
			}
		
		}
	}
	BaseClass::Touch(pOther);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pevInflictor - 
//			pevAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : 
//-----------------------------------------------------------------------------
int CBaseHeadcrab::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	//
	// Don't take any acid damage.
	//
	if ( info.GetDamageType() & DMG_ACID )
	{
		info.SetDamage( 0 );
	}

	//
	// Certain death from melee bludgeon weapons!
	//
	if ( info.GetDamageType() & DMG_CLUB )
	{
		info.SetDamage( m_iHealth );
	}

	return CAI_BaseNPC::OnTakeDamage_Alive( info );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
int CBaseHeadcrab::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
		case SCHED_WAKE_ANGRY:
			return SCHED_HEADCRAB_WAKE_ANGRY;
		
		case SCHED_RANGE_ATTACK1:
			return SCHED_HEADCRAB_RANGE_ATTACK1;

		case SCHED_FAIL_TAKE_COVER:
			return SCHED_ALERT_FACE;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
int CBaseHeadcrab::SelectSchedule( void )
{
	// If we're hidden, don't do much at all
	if ( m_bHidden )
	{
		if( HasCondition( COND_HEADCRAB_UNHIDE ) )
		{
			// We've decided to unhide 
			return SCHED_HEADCRAB_UNHIDE;
		}

		return SCHED_IDLE_STAND;
	}

	if( HasCondition( COND_HEADCRAB_IN_WATER ) )
	{
		// No matter what, drown in water
		return SCHED_HEADCRAB_DROWN;
	}

	if( HasCondition( COND_HEADCRAB_ON_NPC ) )
	{
		// You're on an NPC's head. Get off.
		return SCHED_HEADCRAB_DISMOUNT_NPC;
	}

	if ( HasCondition( COND_HEADCRAB_BARNACLED ) )
	{
		// Caught by a barnacle!
		return SCHED_HEADCRAB_BARNACLED;
	}

	switch ( m_NPCState )
	{
		case NPC_STATE_ALERT:
		{
			if (HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ))
			{
				if ( fabs( GetMotor()->DeltaIdealYaw() ) < ( 1.0 - m_flFieldOfView) * 60 ) // roughly in the correct direction
				{
					return SCHED_TAKE_COVER_FROM_ORIGIN;
				}
				else if ( SelectWeightedSequence( ACT_SMALL_FLINCH ) != -1 )
				{
					m_flNextFlinchTime = gpGlobals->curtime + random->RandomFloat( 1, 3 );
					return SCHED_ALERT_SMALL_FLINCH;
				}
			}
			else if (HasCondition( COND_HEAR_DANGER ) ||
					 HasCondition( COND_HEAR_PLAYER ) ||
					 HasCondition( COND_HEAR_WORLD  ) ||
					 HasCondition( COND_HEAR_COMBAT ))
			{
				return SCHED_ALERT_FACE;
			}
			else
			{
				return SCHED_PATROL_WALK;
			}
			break;
		}
	}

	int nSchedule = BaseClass::SelectSchedule();
	if ( nSchedule == SCHED_SMALL_FLINCH )
	{
		 m_flNextFlinchTime = gpGlobals->curtime + random->RandomFloat( 1, 3 );
	}

	return nSchedule;
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CBaseHeadcrab::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionBarnacleVictimDangle)
	{
		// Die instantly
		return false;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		m_IdealNPCState = NPC_STATE_IDLE;

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

		SetSchedule( SCHED_HEADCRAB_BARNACLED );
		SetCondition( COND_HEADCRAB_BARNACLED );
		m_flNextFlinchTime = gpGlobals->curtime - 0.1f;

		// return ideal grab position
		if (data)
		{
			// FIXME: need a good way to ensure this contract
			*((Vector *)data) = GetAbsOrigin() + Vector( 0, 0, 5 );
		}

		m_IdealNPCState = NPC_STATE_PRONE;
		return true;
	}
	else if (interactionType ==	g_interactionVortigauntStomp)
	{
		m_IdealNPCState = NPC_STATE_PRONE;
		return true;
	}
	else if (interactionType ==	g_interactionVortigauntStompFail)
	{
		m_IdealNPCState = NPC_STATE_COMBAT;
		return true;
	}
	else if (interactionType ==	g_interactionVortigauntStompHit)
	{
		// Gib the existing guy, but only with legs and guts
		m_nGibCount = HEADCRAB_LEGS_GIB_COUNT;
		OnTakeDamage ( CTakeDamageInfo( sourceEnt, sourceEnt, m_iHealth, DMG_CRUSH|DMG_ALWAYSGIB ) );
		
		// Create dead headcrab in its place
		CBaseHeadcrab *pEntity = (CBaseHeadcrab*) CreateEntityByName( "npc_headcrab" );
		pEntity->Spawn();
		pEntity->SetModel( "models/HC_squashed01.mdl" );
		pEntity->SetLocalOrigin( GetLocalOrigin() );
		pEntity->SetLocalAngles( GetLocalAngles() );
		pEntity->m_NPCState = NPC_STATE_DEAD;
		return true;
	}
	else if (	(interactionType ==	g_interactionVortigauntKick) ||
				(interactionType ==	g_interactionBullsquidThrow) ||
				(interactionType ==	g_interactionCombineKick)	   )
	{
		m_IdealNPCState = NPC_STATE_PRONE;
		UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0 , 0 , 1 ));

		// Throw headcrab guts 
		CGib::SpawnSpecificGibs( this, HEADCRAB_GUTS_GIB_COUNT, 300, 400, "models/gibs/hc_gibs.mdl");

		trace_t tr;
		Vector vHitDir = GetLocalOrigin() - sourceEnt->GetLocalOrigin();
		VectorNormalize(vHitDir);

		ClearMultiDamage();
		CTakeDamageInfo info( sourceEnt, sourceEnt, m_iHealth+1, DMG_CLUB );
		CalculateMeleeDamageForce( &info, vHitDir, GetAbsOrigin() );
		TraceAttack( info, vHitDir, &tr );
		ApplyMultiDamage();

		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHeadcrab::Precache( void )
{
	engine->PrecacheModel( "models/headcrabclassic.mdl" );
	engine->PrecacheModel( "models/hc_squashed01.mdl" );
	engine->PrecacheModel( "models/gibs/hc_gibs.mdl" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHeadcrab::Spawn( void )
{
	Precache();
	SetModel( "models/headcrabclassic.mdl" );

	BaseClass::Spawn();

	m_iHealth = sk_headcrab_health.GetFloat();

	NPCInit();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHeadcrab::IdleSound( void )
{
	EmitSound( "NPC_HeadCrab.Idle" );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHeadcrab::AlertSound( void )
{
	EmitSound( "NPC_HeadCrab.Alert" );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHeadcrab::PainSound( void )
{
	EmitSound( "NPC_HeadCrab.Pain" );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHeadcrab::DeathSound( void )
{
	EmitSound( "NPC_HeadCrab.Die" );
}


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CFastHeadcrab )

	DEFINE_FIELD( CFastHeadcrab, m_iRunMode,			FIELD_INTEGER ),
	DEFINE_FIELD( CFastHeadcrab, m_flRealGroundSpeed,	FIELD_FLOAT ),
	DEFINE_FIELD( CFastHeadcrab, m_flSlowRunTime,		FIELD_TIME ),
	DEFINE_FIELD( CFastHeadcrab, m_flPauseTime,			FIELD_TIME ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFastHeadcrab::Precache( void )
{
	engine->PrecacheModel( "models/headcrab.mdl" );
	engine->PrecacheModel( "models/hc_squashed01.mdl" );
	engine->PrecacheModel( "models/gibs/hc_gibs.mdl" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFastHeadcrab::Spawn( void )
{
	Precache();
	SetModel( "models/headcrab.mdl" );

	BaseClass::Spawn();

	m_iHealth = sk_headcrab_health.GetFloat();

	m_iRunMode = HEADCRAB_RUNMODE_IDLE;
	m_flPauseTime = 999999;

	NPCInit();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFastHeadcrab::IdleSound( void )
{
	EmitSound( "NPC_FastHeadcrab.Idle" );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFastHeadcrab::AlertSound( void )
{
	EmitSound( "NPC_FastHeadcrab.Alert" );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFastHeadcrab::PainSound( void )
{
	EmitSound( "NPC_FastHeadcrab.Pain" );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFastHeadcrab::DeathSound( void )
{
	EmitSound( "NPC_FastHeadcrab.Die" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFastHeadcrab::PrescheduleThink( void )
{
#if 1 // #IF 0 this to stop the accelrating/decelerating movement.
#define HEADCRAB_ACCELERATION 0.1
	if( IsAlive() && GetNavigator()->IsGoalActive() )
	{
		switch( m_iRunMode )
		{
		case HEADCRAB_RUNMODE_IDLE:
			if ( GetActivity() == ACT_RUN )
			{
				m_flRealGroundSpeed = m_flGroundSpeed;
				m_iRunMode = HEADCRAB_RUNMODE_ACCELERATE;
				m_flPlaybackRate = HEADCRAB_RUN_MINSPEED;
			}
			break;

		case HEADCRAB_RUNMODE_FULLSPEED:
			if( gpGlobals->curtime > m_flSlowRunTime )
			{
				m_iRunMode = HEADCRAB_RUNMODE_DECELERATE;
			}
			break;

		case HEADCRAB_RUNMODE_ACCELERATE:
			if( m_flPlaybackRate < HEADCRAB_RUN_MAXSPEED )
			{
				m_flPlaybackRate += HEADCRAB_ACCELERATION;
			}

			if( m_flPlaybackRate >= HEADCRAB_RUN_MAXSPEED )
			{
				m_flPlaybackRate = HEADCRAB_RUN_MAXSPEED;
				m_iRunMode = HEADCRAB_RUNMODE_FULLSPEED;

				m_flSlowRunTime = gpGlobals->curtime + random->RandomFloat( 0.1, 1.0 );
			}
			break;

		case HEADCRAB_RUNMODE_DECELERATE:
			m_flPlaybackRate -= HEADCRAB_ACCELERATION;

			if( m_flPlaybackRate <= HEADCRAB_RUN_MINSPEED )
			{
				m_flPlaybackRate = HEADCRAB_RUN_MINSPEED;

				// Now stop the crab.
				m_iRunMode = HEADCRAB_RUNMODE_PAUSE;
				SetActivity( ACT_IDLE );
				GetNavigator()->SetMovementActivity(ACT_IDLE);
				m_flPauseTime = gpGlobals->curtime + random->RandomFloat( 0.2, 0.5 );
				m_flRealGroundSpeed = 0.0;
			}
			break;

		case HEADCRAB_RUNMODE_PAUSE:
			{
				if( gpGlobals->curtime > m_flPauseTime )
				{
					m_iRunMode = HEADCRAB_RUNMODE_IDLE;
					SetActivity( ACT_RUN );
					GetNavigator()->SetMovementActivity(ACT_RUN);
					m_flPauseTime = gpGlobals->curtime - 1;
					m_flRealGroundSpeed = m_flGroundSpeed;
				}
			}
			break;

		default:
			Warning( "BIG TIME HEADCRAB ERROR\n" );
			break;
		}

		m_flGroundSpeed = m_flRealGroundSpeed * m_flPlaybackRate;
	}
	else
	{
		m_flPauseTime = gpGlobals->curtime - 1;
	}
#endif

	BaseClass::PrescheduleThink();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scheduleType - 
//-----------------------------------------------------------------------------
int CFastHeadcrab::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_IDLE_STAND:
		return SCHED_PATROL_WALK;
		break;

	case SCHED_RANGE_ATTACK1:
		return SCHED_FAST_HEADCRAB_RANGE_ATTACK1;
		break;

	case SCHED_WAKE_ANGRY:
		return SCHED_HEADCRAB_WAKE_ANGRY;
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CFastHeadcrab::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_RANGE_ATTACK1:
		case TASK_RANGE_ATTACK2:
			
			// Fast headcrab faces the target in flight.
			GetMotor()->SetIdealYawAndUpdate( GetEnemy()->GetAbsOrigin() - GetAbsOrigin(), AI_KEEP_YAW_SPEED );
			
			// Call back up into base headcrab for collision.
			BaseClass::RunTask( pTask );
			break;

		case TASK_HEADCRAB_HOP_ASIDE:
			GetMotor()->SetIdealYawAndUpdate( GetEnemy()->GetAbsOrigin() - GetAbsOrigin(), AI_KEEP_YAW_SPEED );

			if( GetFlags() & FL_ONGROUND )
			{
				SetGravity(1.0);
				SetMoveType( MOVETYPE_STEP );

				if( ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).Length() > HEADCRAB_MAX_JUMP_DIST )
				{
					TaskFail( "");
				}

				TaskComplete();
			}
			break;

		default:
			CAI_BaseNPC::RunTask( pTask );
			break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CFastHeadcrab::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_HEADCRAB_HOP_ASIDE:
		{
			Vector vecDir, vecForward, vecRight;
			bool fJumpIsLeft;
			trace_t tr;

			GetVectors( &vecForward, &vecRight, NULL );

			fJumpIsLeft = false;
			if( random->RandomInt( 0, 100 ) < 50 )
			{
				fJumpIsLeft = true;
				vecRight.Negate();
			}

			vecDir = ( vecRight + ( vecForward * 2 ) );
			VectorNormalize( vecDir );
			vecDir *= 150.0;

			// This could be a problem. Since I'm adjusting the headcrab's gravity for flight, this check actually
			// checks farther ahead than the crab will actually jump. (sjb)
			AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + vecDir,GetHullMins(), GetHullMaxs(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			//NDebugOverlay::Line( tr.startpos, tr.endpos, 0, 255, 0, false, 1.0 );

			if( tr.fraction == 1.0 )
			{
				AIMoveTrace_t moveTrace;
				GetMoveProbe()->MoveLimit( NAV_JUMP, GetAbsOrigin(), tr.endpos, MASK_NPCSOLID, GetEnemy(), &moveTrace );

				// FIXME: Where should this happen?
				m_vecJumpVel = moveTrace.vJumpVelocity;

				if( !IsMoveBlocked( moveTrace ) )
				{
					SetAbsVelocity( m_vecJumpVel );// + 0.5f * Vector(0,0,sv_gravity.GetFloat()) * flInterval;
					SetGravity(2.0);
					RemoveFlag( FL_ONGROUND );
					SetNavType( NAV_JUMP );

					if( fJumpIsLeft )
					{
						SetIdealActivity( (Activity)ACT_HEADCRAB_HOP_LEFT );
						GetNavigator()->SetMovementActivity( (Activity) ACT_HEADCRAB_HOP_LEFT );
					}
					else
					{
						SetIdealActivity( (Activity)ACT_HEADCRAB_HOP_RIGHT );
						GetNavigator()->SetMovementActivity( (Activity) ACT_HEADCRAB_HOP_RIGHT );
					}

					Relink();
				}
				else
				{
					// Can't jump, just fall through.
					TaskComplete();
				}
			}
			else
			{
				// Can't jump, just fall through.
				TaskComplete();
			}
		}
		break;

	default:
		{
			BaseClass::StartTask( pTask );
		}
	}
}


LINK_ENTITY_TO_CLASS( npc_headcrab, CHeadcrab );
LINK_ENTITY_TO_CLASS( npc_headcrab_fast, CFastHeadcrab );


//-----------------------------------------------------------------------------
// Purpose: Make the sound of this headcrab chomping a target.
// Input  : 
//-----------------------------------------------------------------------------
void CHeadcrab::BiteSound( void )
{
	EmitSound( "NPC_HeadCrab.Bite" );
}

void CFastHeadcrab::BiteSound( void )
{
	EmitSound( "NPC_FastHeadcrab.Bite" );
}

void CHeadcrab::AttackSound( void )
{
	EmitSound( "NPC_Headcrab.Attack" );
}

void CFastHeadcrab::AttackSound( void )
{
	EmitSound( "NPC_FastHeadcrab.Attack" );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
float CHeadcrab::MaxYawSpeed ( void )
{
	switch ( GetActivity() )
	{
	case ACT_IDLE:			
		return 30;
		break;

	case ACT_RUN:			
	case ACT_WALK:			
		return 20;
		break;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 15;
		break;

	case ACT_RANGE_ATTACK1:	
		return 30;
		break;

	default:
		return 30;
		break;
	}

	return BaseClass::MaxYawSpeed();
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CBaseHeadcrab::BuildScheduleTestBits( void )
{
	if ( !IsCurSchedule(SCHED_HEADCRAB_DROWN) )
	{
		// Interrupt any schedule unless already drowning.
		SetCustomInterruptCondition( COND_HEADCRAB_IN_WATER );
	}
	else
	{
		// Don't stop drowning just because you're in water!
		ClearCustomInterruptCondition( COND_HEADCRAB_IN_WATER );
	}

	if( !IsCurSchedule(SCHED_HEADCRAB_DISMOUNT_NPC) )
	{
		SetCustomInterruptCondition( COND_HEADCRAB_ON_NPC );
	}
	else
	{
		ClearCustomInterruptCondition( COND_HEADCRAB_ON_NPC );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CFastHeadcrab::MaxYawSpeed( void )
{
	switch ( GetActivity() )
	{
		case ACT_IDLE:
		{
			return( 120 );
		}

		case ACT_RUN:
		case ACT_WALK:
		{
			return( 150 );
		}

		case ACT_TURN_LEFT:
		case ACT_TURN_RIGHT:
		{
			return( 120 );
		}

		case ACT_RANGE_ATTACK1:
		{
			return( 120 );
		}

		default:
		{
			return( 120 );
		}
	}
}


bool CFastHeadcrab::QuerySeeEntity(CBaseEntity *pSightEnt)
{
	if( m_NPCState != NPC_STATE_COMBAT )
	{
		if( fabs( pSightEnt->GetAbsOrigin().z - GetAbsOrigin().z ) >= 150 )
		{
			// Don't see things much higher or lower than me unless
			// I'm already pissed.
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Black headcrab stuff
//-----------------------------------------------------------------------------
int ACT_BLACKHEADCRAB_RUN_PANIC;

BEGIN_DATADESC( CBlackHeadcrab )

	DEFINE_FIELD( CBlackHeadcrab, m_bPanicState, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBlackHeadcrab, m_flPanicStopTime, FIELD_TIME ),
	DEFINE_FIELD( CBlackHeadcrab, m_flNextNPCThink, FIELD_TIME ),

	//DEFINE_FIELD( CBlackHeadcrab, m_bCommittedToJump, FIELD_BOOLEAN ),
	//DEFINE_FIELD( CBlackHeadcrab, m_vecCommittedJumpPos, FIELD_POSITION_VECTOR ),

	DEFINE_THINKFUNC( CBlackHeadcrab, ThrowThink ),

	DEFINE_ENTITYFUNC( CBlackHeadcrab, EjectTouch ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( npc_headcrab_black, CBlackHeadcrab );
LINK_ENTITY_TO_CLASS( npc_headcrab_poison, CBlackHeadcrab );


//-----------------------------------------------------------------------------
// Purpose: Make the sound of this headcrab chomping a target.
//-----------------------------------------------------------------------------
void CBlackHeadcrab::BiteSound( void )
{
	EmitSound( "NPC_BlackHeadcrab.Bite" );
}


//-----------------------------------------------------------------------------
// Purpose: The sound we make when leaping at our enemy.
//-----------------------------------------------------------------------------
void CBlackHeadcrab::AttackSound( void )
{
	EmitSound( "NPC_BlackHeadcrab.Attack" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::Spawn( void )
{
	Precache();
	SetModel( "models/headcrabblack.mdl" );

	BaseClass::Spawn();

	m_bPanicState = false;
	m_iHealth = sk_headcrab_poison_health.GetFloat();

	NPCInit();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::Precache( void )
{
	engine->PrecacheModel( "models/headcrabblack.mdl" );
	engine->PrecacheModel( "models/hc_squashed01.mdl" );
	engine->PrecacheModel( "models/gibs/hc_gibs.mdl" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Returns the max yaw speed for the current activity.
//-----------------------------------------------------------------------------
float CBlackHeadcrab::MaxYawSpeed( void )
{
	// Not a constant, can't be in a switch statement.
	if ( GetActivity() == ACT_BLACKHEADCRAB_RUN_PANIC )
	{
		return 60;
	}

	switch ( GetActivity() )
	{
		case ACT_TURN_LEFT:
		case ACT_TURN_RIGHT:
		{
			return( 30 );
		}

		case ACT_RANGE_ATTACK1:
		{
			return( 30 );
		}

		default:
		{
			return( 30 );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CBlackHeadcrab::NPC_TranslateActivity( Activity eNewActivity )
{
	if ( ( m_bPanicState ) && ( eNewActivity == ACT_RUN ) )
	{
		return ( Activity )ACT_BLACKHEADCRAB_RUN_PANIC;
	}

	return BaseClass::NPC_TranslateActivity( eNewActivity );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBlackHeadcrab::TranslateSchedule( int scheduleType )
{
	switch ( scheduleType )
	{
		// Keep trying to take cover for at least a few seconds.
		case SCHED_FAIL_TAKE_COVER:
		{
			if ( ( m_bPanicState ) && ( gpGlobals->curtime > m_flPanicStopTime ) )
			{
				//Msg( "I'm sick of panicking\n" );
				m_bPanicState = false;
				return SCHED_CHASE_ENEMY;
			}

			break;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CBlackHeadcrab::BuildScheduleTestBits( void )
{
	// Ignore damage if we're attacking or are fleeing and recently flinched.
	if ( IsCurSchedule( SCHED_RANGE_ATTACK1 ) || ( IsCurSchedule( SCHED_TAKE_COVER_FROM_ENEMY ) && HasMemory( bits_MEMORY_FLINCHED ) ) )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}
	else
	{
		SetCustomInterruptCondition( COND_LIGHT_DAMAGE );
		SetCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}

	// If we're committed to jump, carry on even if our enemy hides behind a crate. Or a barrel.
	if ( IsCurSchedule( SCHED_RANGE_ATTACK1 ) && m_bCommittedToJump )
	{
		ClearCustomInterruptCondition( COND_ENEMY_OCCLUDED );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
int CBlackHeadcrab::SelectSchedule( void )
{
	if ( m_bPanicState )
	{
		// We're looking for a place to hide.
		if ( HasMemory( bits_MEMORY_INCOVER ) )
		{
			m_bPanicState = false;
			m_flPanicStopTime = gpGlobals->curtime;

			return SCHED_HEADCRAB_AMBUSH;
		}
		else if (( HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE) ) && !HasMemory( bits_MEMORY_FLINCHED) && SelectWeightedSequence( ACT_SMALL_FLINCH ) != -1 )
		{
			m_flNextFlinchTime = gpGlobals->curtime + random->RandomFloat( 1, 3 );
			return SCHED_SMALL_FLINCH;
		}

		return SCHED_TAKE_COVER_FROM_ENEMY;
	}

	return BaseClass::SelectSchedule();
}


//-----------------------------------------------------------------------------
// Purpose: Overridden so we can reset our commit flag.
// Input  : bRandomJump - 
//			vecPos - 
//			bThrown - 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::JumpAttack( bool bRandomJump, const Vector &vecPos, bool bThrown )
{
	if (m_bCommittedToJump)
	{
		BaseClass::JumpAttack( bRandomJump, m_vecCommittedJumpPos, bThrown );
	}
	else
	{
		BaseClass::JumpAttack( bRandomJump, vecPos, bThrown );
	}

	m_bCommittedToJump = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vecPos - 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::ThrowAt( const Vector &vecPos )
{
	JumpAttack( false, vecPos, true );
	SetThink( ThrowThink );
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::ThrowThink( void )
{
	if (gpGlobals->curtime > m_flNextNPCThink)
	{
		NPCThink();
		m_flNextNPCThink = gpGlobals->curtime + 0.1;
	}

	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: Black headcrab's touch attack damage. Evil!
//-----------------------------------------------------------------------------
void CBlackHeadcrab::TouchDamage( CBaseEntity *pOther )
{
	BaseClass::TouchDamage( pOther );
	if ( pOther->IsAlive() )
	{
		// That didn't finish them. Take them down to one point with poison damage. It'll heal.
		pOther->TakeDamage( CTakeDamageInfo( this, this, pOther->m_iHealth - 1, DMG_POISON ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Bails out of our host zombie, either because he died or was blown
//			into two pieces by an explosion.
// Input  : vecAngles - The yaw direction we should face.
//			flVelocityScale - A multiplier for our ejection velocity.
//			pEnemy - Who we should acquire as our enemy. Usually our zombie host's enemy.
//-----------------------------------------------------------------------------
void CBlackHeadcrab::Eject( const QAngle &vecAngles, float flVelocityScale, CBaseEntity *pEnemy )
{
	RemoveFlag( FL_ONGROUND );
	m_spawnflags |= SF_NPC_FALL_TO_GROUND;

	m_IdealNPCState = NPC_STATE_ALERT;

	if ( pEnemy )
	{
		SetEnemy( pEnemy );
		UpdateEnemyMemory(pEnemy, pEnemy->GetAbsOrigin());
	}

	SetActivity( ACT_RANGE_ATTACK1 );

	SetNextThink( gpGlobals->curtime );
	PhysicsSimulate();

	GetMotor()->SetIdealYaw( vecAngles.y );

	SetAbsVelocity( flVelocityScale * random->RandomInt( 20, 50 ) * 
		Vector( random->RandomFloat( -1.0, 1.0 ), random->RandomFloat( -1.0, 1.0 ), random->RandomFloat( 0.5, 1.0 ) ) );

	SetTouch( EjectTouch );
}


//-----------------------------------------------------------------------------
// Purpose: Touch function for when we are ejected from the poison zombie.
//			Panic when we hit the ground.
//-----------------------------------------------------------------------------
void CBlackHeadcrab::EjectTouch( CBaseEntity *pOther )
{
	LeapTouch( pOther );
	if ( GetFlags() & FL_ONGROUND )
	{
		// Keep trying to take cover for at least a few seconds.
		Panic( random->RandomFloat( 2, 8 ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Puts us in a state in which we just want to hide. We'll stop
//			hiding after the given duration.
//-----------------------------------------------------------------------------
void CBlackHeadcrab::Panic( float flDuration )
{
	m_flPanicStopTime = gpGlobals->curtime + flDuration;
	m_bPanicState = true;
}


//-----------------------------------------------------------------------------
// Purpose: Does a spastic hop in a random or provided direction.
// Input  : pvecDir - 2D direction to hop, NULL picks a random direction.
//-----------------------------------------------------------------------------
void CBlackHeadcrab::JumpFlinch( const Vector *pvecDir )
{
	RemoveFlag( FL_ONGROUND );

	//
	// Take him off ground so engine doesn't instantly reset FL_ONGROUND.
	//
	UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0 , 0 , 1 ));

	//
	// Jump in a random direction.
	//
	Vector up;
	AngleVectors( GetLocalAngles(), NULL, NULL, &up );

	if (pvecDir)
	{
		SetAbsVelocity( Vector( pvecDir->x * 4, pvecDir->y * 4, up.z ) * random->RandomFloat( 40, 80 ) );
	}
	else
	{
		SetAbsVelocity( Vector( random->RandomFloat( -4, 4 ), random->RandomFloat( -4, 4 ), up.z ) * random->RandomFloat( 40, 80 ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific messages that occur when tagged
//			animation frames are played.
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
		case HC_POISON_AE_FOOTSTEP:
		{
			bool walk = ( GetActivity() == ACT_WALK ) ? 1.0 : 0.6;
			if ( walk )
			{
				EmitSound( "NPC_BlackHeadcrab.FootstepWalk" );
			}
			else
			{
				EmitSound( "NPC_BlackHeadcrab.Footstep" );
			}
			break;
		}

		// Just before jumping at our enemy.
		case HC_AE_JUMP_TELEGRAPH:
		{
			EmitSound( "NPC_BlackHeadcrab.Telegraph" );

			CBaseEntity *pEnemy = GetEnemy();
			if ( pEnemy )
			{
				// Once we telegraph, we MUST jump. This is also when commit to what point
				// we jump at. Jump at our enemy's eyes.
				m_vecCommittedJumpPos = pEnemy->EyePosition();
				m_bCommittedToJump = true;
			}

			break;
		}

		// When we wake up angrily.
		case HC_POISON_AE_THREAT_SOUND:
		{
			EmitSound( "NPC_BlackHeadcrab.Threat" );
			EmitSound( "NPC_BlackHeadcrab.Alert" );
			break;
		}

		case HC_POISON_AE_FLINCH_HOP:
		{
			//
			// Hop in a random direction, then run and hide. If we're already running
			// to hide, jump forward -- hopefully that will take us closer to a hiding spot.
			//			
			if (m_bPanicState)
			{
				Vector vecForward;
				AngleVectors( GetLocalAngles(), &vecForward );
				JumpFlinch( &vecForward );
			}
			else
			{
				JumpFlinch( NULL );
			}

			Panic( random->RandomFloat( 2, 5 ) );
			break;
		}

		default:
		{
			BaseClass::HandleAnimEvent( pEvent );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::IdleSound( void )
{
	// TODO: hook up "Marco" / "Polo" talking with nearby buddies
	if ( m_NPCState == NPC_STATE_IDLE )
	{
		EmitSound( "NPC_BlackHeadcrab.Idle" );
	}
	else if ( m_NPCState == NPC_STATE_ALERT )
	{
		EmitSound( "NPC_BlackHeadcrab.Talk" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::AlertSound( void )
{
	EmitSound( "NPC_BlackHeadcrab.AlertVoice" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::PainSound( void )
{
	EmitSound( "NPC_BlackHeadcrab.Pain" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlackHeadcrab::DeathSound( void )
{
	EmitSound( "NPC_BlackHeadcrab.Die" );
}


//-----------------------------------------------------------------------------
// Purpose: Played when we jump and hit something that we can't bite.
//-----------------------------------------------------------------------------
void CBlackHeadcrab::ImpactSound( void )
{
	EmitSound( "NPC_BlackHeadcrab.Impact" );

	if ( !( GetFlags() & FL_ONGROUND ) )
	{
		// Hit a wall - make a pissed off sound.
		EmitSound( "NPC_BlackHeadcrab.ImpactAngry" );
	}
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_headcrab, CBaseHeadcrab )

	DECLARE_TASK( TASK_HEADCRAB_HOP_ASIDE )
	DECLARE_TASK( TASK_HEADCRAB_DROWN )
	DECLARE_TASK( TASK_HEADCRAB_HOP_OFF_NPC )
	DECLARE_TASK( TASK_HEADCRAB_WAIT_FOR_BARNACLE_KILL )
	DECLARE_TASK( TASK_HEADCRAB_UNHIDE )

	DECLARE_ACTIVITY( ACT_HEADCRAB_THREAT_DISPLAY )
	DECLARE_ACTIVITY( ACT_HEADCRAB_HOP_LEFT )
	DECLARE_ACTIVITY( ACT_HEADCRAB_HOP_RIGHT )
	DECLARE_ACTIVITY( ACT_HEADCRAB_DROWN )

	DECLARE_CONDITION( COND_HEADCRAB_IN_WATER )
	DECLARE_CONDITION( COND_HEADCRAB_ON_NPC )
	DECLARE_CONDITION( COND_HEADCRAB_BARNACLED )
	DECLARE_CONDITION( COND_HEADCRAB_UNHIDE );

	//=========================================================
	// > SCHED_HEADCRAB_RANGE_ATTACK1
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HEADCRAB_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_IDEAL				0"
		"		TASK_RANGE_ATTACK1			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_FACE_IDEAL				0"
		"		TASK_WAIT_RANDOM			0.5"
		""
		"	Interrupts"
		"		COND_ENEMY_OCCLUDED"
		"		COND_NO_PRIMARY_AMMO"
	)

	//=========================================================
	//
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HEADCRAB_WAKE_ANGRY,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE "
		"		TASK_FACE_IDEAL					0"
		"		TASK_SOUND_WAKE					0"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY	ACTIVITY:ACT_HEADCRAB_THREAT_DISPLAY"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_FAST_HEADCRAB_RANGE_ATTACK1
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_FAST_HEADCRAB_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_IDEAL				0"
		"		TASK_HEADCRAB_HOP_ASIDE		0"
		"		TASK_RANGE_ATTACK1			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_FACE_IDEAL				0"
		"		TASK_WAIT_RANDOM			0.5"
		""
		"	Interrupts"
		"		COND_ENEMY_OCCLUDED"
		"		COND_NO_PRIMARY_AMMO"
	)

	//=========================================================
	// The irreversible process of drowning
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HEADCRAB_DROWN,

		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_HEADCRAB_DROWN"
		"		TASK_HEADCRAB_DROWN			0"
		""
		"	Interrupts"
	)


	//=========================================================
	// Headcrab lurks in place and waits for a chance to jump on
	// some unfortunate soul.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HEADCRAB_AMBUSH,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_WAIT_INDEFINITE		0"

		"	Interrupts"
		"		COND_SEE_ENEMY"
		"		COND_SEE_HATE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
	)

	//=========================================================
	// Headcrab has landed atop another NPC. Get down!
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HEADCRAB_DISMOUNT_NPC,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_HEADCRAB_HOP_OFF_NPC	0"

		"	Interrupts"
	)

	//=========================================================
	// Headcrab is in the clutches of a barnacle
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HEADCRAB_BARNACLED,

		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_SET_ACTIVITY						ACTIVITY:ACT_HEADCRAB_DROWN"
		"		TASK_HEADCRAB_WAIT_FOR_BARNACLE_KILL	0"

		"	Interrupts"
	)

	//=========================================================
	// Headcrab is unhiding
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HEADCRAB_UNHIDE,

		"	Tasks"
		"		TASK_HEADCRAB_UNHIDE			0"

		"	Interrupts"
	)

AI_END_CUSTOM_NPC()

//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_headcrab_poison, CBlackHeadcrab )

	DECLARE_ACTIVITY( ACT_BLACKHEADCRAB_RUN_PANIC )

AI_END_CUSTOM_NPC()
