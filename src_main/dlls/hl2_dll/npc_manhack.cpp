//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "soundenvelope.h"
#include "npc_manhack.h"
#include "AI_Default.h"
#include "AI_Node.h"
#include "AI_Navigator.h"
#include "AI_Pathfinder.h"
#include "ai_moveprobe.h"
#include "AI_Memory.h"
#include "AI_Squad.h"
#include "explode.h"
#include "basegrenade_shared.h"
#include "ndebugoverlay.h"
#include "decals.h"
#include "gib.h"
#include "splash.h"
#include "game.h"			
#include "AI_Interactions.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"
#include "npcevent.h"
#include "props.h"
#include "te_effect_dispatch.h"
#include "ai_squadslot.h"
#include "world.h"

// When the engine is running and the manhack is operating under power
// we don't let gravity affect him.
#define MANHACK_GRAVITY 0.000

#define MANHACK_GIB_COUNT			5 
#define MANHACK_INGORE_WATER_DIST	384

// Sound stuff
#define MANHACK_PITCH_DIST1		512
#define MANHACK_MIN_PITCH1		(100)
#define MANHACK_MAX_PITCH1		(160)
#define MANHACK_WATER_PITCH1	(85)
#define MANHACK_VOLUME1			0.55

#define MANHACK_PITCH_DIST2		400
#define MANHACK_MIN_PITCH2		(85)
#define MANHACK_MAX_PITCH2		(190)
#define MANHACK_WATER_PITCH2	(90)

#define MANHACK_NOISEMOD_HIDE 5000

#define MANHACK_BODYGROUP_BLADE	1
#define MANHACK_BODYGROUP_BLUR	2
#define MANHACK_BODYGROUP_OFF	0
#define MANHACK_BODYGROUP_ON	1

// ANIMATION EVENTS
#define MANHACK_AE_START_ENGINE			50
#define MANHACK_AE_DONE_UNPACKING		51
#define MANHACK_AE_OPEN_BLADE			52

#define MANHACK_GLOW_SPRITE	"sprites/laserdot.vmt"


ConVar	sk_manhack_health( "sk_manhack_health","0");
ConVar	sk_manhack_melee_dmg( "sk_manhack_melee_dmg","0");

extern void		SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern float	GetFloorZ(const Vector &origin);

//-----------------------------------------------------------------------------
// Private activities.
//-----------------------------------------------------------------------------
int ACT_MANHACK_UNPACK;

//-----------------------------------------------------------------------------
// Manhack Conditions
//-----------------------------------------------------------------------------
enum ManhackConditions
{
	COND_MANHACK_START_ATTACK = LAST_SHARED_CONDITION,	// We are able to do a bombing run on the current enemy.
};

//-----------------------------------------------------------------------------
// Manhack schedules.
//-----------------------------------------------------------------------------
enum ManhackSchedules
{
	SCHED_MANHACK_ATTACK_HOVER = LAST_SHARED_SCHEDULE,
	SCHED_MANHACK_DEPLOY,
	SCHED_MANHACK_REGROUP,
	SCHED_MANHACK_SWARM_IDLE,
	SCHED_MANHACK_SWARM,
	SCHED_MANHACK_SWARM_FAILURE,
};


//-----------------------------------------------------------------------------
// Manhack tasks.
//-----------------------------------------------------------------------------
enum ManhackTasks
{
	TASK_MANHACK_HOVER = LAST_SHARED_TASK,
	TASK_MANHACK_UNPACK,
	TASK_MANHACK_FIND_SQUAD_CENTER,
	TASK_MANHACK_FIND_SQUAD_MEMBER,
	TASK_MANHACK_MOVEAT_SAVEPOSITION,
};

BEGIN_DATADESC( CNPC_Manhack )

	DEFINE_FIELD( CNPC_Manhack,	m_vForceVelocity,			FIELD_VECTOR),

	DEFINE_FIELD( CNPC_Manhack,	m_vTargetBanking,			FIELD_VECTOR),
	DEFINE_FIELD( CNPC_Manhack,	m_vForceMoveTarget,			FIELD_POSITION_VECTOR),
	DEFINE_FIELD( CNPC_Manhack,	m_fForceMoveTime,			FIELD_TIME),
	DEFINE_FIELD( CNPC_Manhack,	m_vSwarmMoveTarget,			FIELD_POSITION_VECTOR),
	DEFINE_FIELD( CNPC_Manhack,	m_fSwarmMoveTime,			FIELD_TIME),
	DEFINE_FIELD( CNPC_Manhack,	m_fEnginePowerScale,		FIELD_FLOAT),

	DEFINE_FIELD( CNPC_Manhack,	m_flNextEngineSoundTime,	FIELD_TIME),
	DEFINE_FIELD( CNPC_Manhack,	m_flEngineStallTime,		FIELD_TIME),
	DEFINE_FIELD( CNPC_Manhack, m_flNextBurstTime,			FIELD_TIME ),
	DEFINE_FIELD( CNPC_Manhack, m_flIgnoreDamageTime,		FIELD_TIME),
	DEFINE_FIELD( CNPC_Manhack, m_flWaterSuspendTime,		FIELD_TIME),
	DEFINE_FIELD( CNPC_Manhack, m_nLastSpinSound,			FIELD_INTEGER ),

	// Death
	DEFINE_FIELD( CNPC_Manhack,	m_fSparkTime,				FIELD_TIME),
	DEFINE_FIELD( CNPC_Manhack,	m_fSmokeTime,				FIELD_TIME),

	DEFINE_FIELD( CNPC_Manhack, m_bDirtyPitch, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Manhack, m_bWasClubbed,				FIELD_BOOLEAN),

	DEFINE_SOUNDPATCH( CNPC_Manhack,  m_pEngineSound1 ),
	DEFINE_SOUNDPATCH( CNPC_Manhack,  m_pEngineSound2 ),
	DEFINE_SOUNDPATCH( CNPC_Manhack,  m_pBladeSound ),

	DEFINE_FIELD( CNPC_Manhack, m_bBladesActive,			FIELD_BOOLEAN),
	DEFINE_FIELD( CNPC_Manhack, m_flBladeSpeed,				FIELD_FLOAT),

	DEFINE_FIELD( CNPC_Manhack, m_iEyeAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Manhack, m_pLightGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_Manhack, m_iLightAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Manhack, m_pEyeGlow, FIELD_CLASSPTR ),

	DEFINE_FIELD( CNPC_Manhack, m_iPanel1, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Manhack, m_iPanel2, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Manhack, m_iPanel3, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Manhack, m_iPanel4, FIELD_INTEGER ),

	// Function Pointers
	DEFINE_ENTITYFUNC( CNPC_Manhack, CrashTouch ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( npc_manhack, CNPC_Manhack );

IMPLEMENT_SERVERCLASS_ST(CNPC_Manhack, DT_NPC_Manhack)
END_SEND_TABLE()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CNPC_Manhack::CNPC_Manhack()
{
#ifdef _DEBUG
	m_vForceMoveTarget.Init();
	m_vSwarmMoveTarget.Init();
	m_vTargetBanking.Init();
	m_vForceVelocity.Init();
#endif
	m_bDirtyPitch = true;
	m_pEngineSound1 = NULL;
	m_pEngineSound2 = NULL;
	m_pBladeSound = NULL;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
CNPC_Manhack::~CNPC_Manhack()
{
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this NPC's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Manhack::Classify(void)
{
	return(CLASS_MANHACK);
}



//-----------------------------------------------------------------------------
// Purpose: Turns the manhack into a physics corpse when dying.
//-----------------------------------------------------------------------------
void CNPC_Manhack::Event_Dying(void)
{
	SetHullSizeNormal();
	BaseClass::Event_Dying();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Manhack::StopLoopingSounds(void)
{
	// Kill the engine!
	CSoundEnvelopeController::GetController().SoundDestroy( m_pEngineSound1 );
	m_pEngineSound1 = NULL;

	// Kill the engine!
	CSoundEnvelopeController::GetController().SoundDestroy( m_pEngineSound2 );
	m_pEngineSound2 = NULL;

	// Kill the blade!
	CSoundEnvelopeController::GetController().SoundDestroy( m_pBladeSound );
	m_pBladeSound = NULL;

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	if( m_flWaterSuspendTime > gpGlobals->curtime )
	{
		// Stuck in water!

		// Reduce engine power so that the manhack lifts out of the water slowly.
		m_fEnginePowerScale = 0.2;

		Splash( GetAbsOrigin() );
	}

	// ----------------------------------------
	//	Am I in water?
	// ----------------------------------------
	if ( GetWaterLevel() > 0)
	{
		Splash( GetAbsOrigin() );

		if( IsAlive() )
		{
			// If I've been out of water for 2 seconds or more, I'm eligible to be stuck in water again.
			if( gpGlobals->curtime - m_flWaterSuspendTime > 2.0 )
			{
				m_flWaterSuspendTime = gpGlobals->curtime + 4.0;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	g_vecAttackDir = vecDir;

	if ( info.GetDamageType() & DMG_BULLET)
	{
		g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
	}

	if ( info.GetDamageType() & DMG_CLUB )
	{
		// Clubbed!
		UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);
		g_pEffects->Sparks( ptr->endpos, 1, 1, &ptr->plane.normal );
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::Event_Killed( const CTakeDamageInfo &info )
{
	// turn off the blur!
	SetBodygroup( MANHACK_BODYGROUP_BLUR, MANHACK_BODYGROUP_OFF );

	// Sparks
	for (int i = 0; i < 3; i++)
	{
		Vector sparkPos = GetAbsOrigin();
		sparkPos.x += random->RandomFloat(-12,12);
		sparkPos.y += random->RandomFloat(-12,12);
		sparkPos.z += random->RandomFloat(-12,12);
		g_pEffects->Sparks( sparkPos, 2 );
	}
	// Smoke
	if (GetWaterLevel() == 0)
	{
		UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);
	}

	// Light
	CBroadcastRecipientFilter filter;
	te->DynamicLight( filter, 0.0, &GetAbsOrigin(), 255, 180, 100, 0, 100, 0.1, 0 );

	// Sound
	CPASAttenuationFilter filter2( this, "NPC_Manhack.Die" );
	EmitSound( filter2, entindex(), "NPC_Manhack.Die" );

	StopLoopingSounds();

	if( !m_pEngineSound1 )
	{
		// Probably this manhack was killed immediately after spawning. Turn the sound
		// on right now so that we can pitch it up for the crash!
		SoundInit();
	}

	if( m_bWasClubbed || random->RandomInt( 0, 1 ) == 0 )
	{
		// Always Gib if clubbed.
		// Otherwise, 50% chance to gib.
		CorpseGib( info );
		CleanupOnDeath( info.GetAttacker() );
	}
	else
	{
		BaseClass::Event_Killed( info );
	}
}

void CNPC_Manhack::HitPhysicsObject( CBaseEntity *pOther )
{
	IPhysicsObject *pOtherPhysics = pOther->VPhysicsGetObject();
	Vector pos, posOther;
	// Put the force on the line between the manhack origin and hit object origin
	VPhysicsGetObject()->GetPosition( &pos, NULL );
	pOtherPhysics->GetPosition( &posOther, NULL );
	Vector dir = posOther - pos;
	VectorNormalize(dir);
	// size/2 is approx radius
	pos += dir * WorldAlignSize().x * 0.5;
	Vector cross;

	// UNDONE: Use actual manhack up vector so the fake blade is
	// in the right plane?
	// Get a vector in the x/y plane in the direction of blade spin (clockwise)
	CrossProduct( dir, Vector(0,0,1), cross );
	VectorNormalize( cross );
	// force is a 30kg object going 100 in/s
	pOtherPhysics->ApplyForceOffset( cross * 30 * 100, pos );
}


void CNPC_Manhack::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];

	if ( pOther->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		HitPhysicsObject( pOther );
	}
	BaseClass::VPhysicsShadowCollision( index, pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Manhack is out of control! (dying) Just explode as soon as you touch anything!
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::CrashTouch( CBaseEntity *pOther )
{
	CTakeDamageInfo	info( GetWorldEntity(), GetWorldEntity(), 25, DMG_CRUSH );

	CorpseGib( info );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Manhack::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Hafta make a copy of info cause we might need to scale damage.(sjb)
	CTakeDamageInfo tdInfo = info;

	if (info.GetDamageType() & DMG_PHYSGUN )
	{
		m_flBladeSpeed = 20.0;

		// respond to physics
		// FIXME: shouldn't this happen in a base class?  Anyway to prevent it from happening twice?
		VPhysicsTakeDamage( info );

		// reduce damage to nothing
		tdInfo.SetDamage( 1.0 );
	}
	else if (info.GetDamageType() & DMG_CLUB)
	{
		// Being hit by a club means a couple of things:
		//
		//		-I'm going to be knocked away from the person that clubbed me.
		//		 if fudging this vector a little bit could help me slam into a physics object,
		//		 then make that adjustment. This is a simple heuristic. The manhack will be
		//		 directed towards the physics object that is closest to g_vecAttackDir
		//
		//		-Take only 50% damage from club attacks. This makes crowbar duels last longer.
		
		// tdInfo.ScaleDamage( 0.5 ); Disabled for E3 2003 (SJB)

#define MANHACK_PHYS_SEARCH_SIZE 25

		CBaseEntity *pList[ MANHACK_PHYS_SEARCH_SIZE ];
		Vector vecDelta( 256, 256, 128 );

		int count = UTIL_EntitiesInBox( pList, MANHACK_PHYS_SEARCH_SIZE, GetAbsOrigin() - vecDelta, GetAbsOrigin() + vecDelta, 0 );

		Vector			vecBestDir = g_vecAttackDir;
		float			flBestDot = 0.90;
		IPhysicsObject	*pPhysObj;

		int i;
		for( i = 0 ; i < count ; i++ )
		{
			pPhysObj = pList[ i ]->VPhysicsGetObject();

			if( !pPhysObj || pPhysObj->GetMass() > 200 )
			{
				// Not physics.
				continue;
			}

			Vector center = pList[ i ]->WorldSpaceCenter();

			Vector vecDirToObject;
			VectorSubtract( center, WorldSpaceCenter(), vecDirToObject );
			VectorNormalize( vecDirToObject );

			float flDot;

			flDot = DotProduct( g_vecAttackDir, vecDirToObject );
			

			if( flDot > flBestDot )
			{
				flBestDot = flDot;
				vecBestDir = vecDirToObject;
			}
		}

		tdInfo.SetDamageForce( vecBestDir * info.GetDamage() * 200 );

		// FIXME: shouldn't this happen in a base class?  Anyway to prevent it from happening twice?
		// Msg("hit");
		VPhysicsTakeDamage( tdInfo );

		// SetCurrentVelocity( 650 * vecBestDir * info.GetDamage() );

		// m_vForceVelocity = 650 * vecBestDir * info.GetDamage();
		m_flBladeSpeed = 10.0;

		EmitSound( "NPC_Manhack.Bat" );	

		// tdInfo.SetDamage( 1.0 );

		// Also indicate that the last time we took damage, we were clubbed.
		m_bWasClubbed = true;
	}
	else
	{
		// We were NOT clubbed. Clear this out.
		m_bWasClubbed = false;

		m_flBladeSpeed = 20.0;

		VPhysicsTakeDamage( info );

		// tdInfo.SetDamage( 1.0 );
	}

	return BaseClass::OnTakeDamage_Alive( tdInfo );
}


//------------------------------------------------------------------------------
// Purpose: Become a ragdoll
//------------------------------------------------------------------------------
void CNPC_Manhack::Ragdoll(void)
{
	if ( ShouldFadeOnDeath() )
	{
		// this NPC was created by a NPCmaker... fade the corpse out.
		SUB_StartFadeOut();
	}


	StopLoopingSounds();

	// Long fadeout on the sprites!!
	KillSprites( 5.0 );

	g_pEffects->Sparks( GetAbsOrigin() );

	// Light
	CBroadcastRecipientFilter filter;

	te->DynamicLight( filter, 0.0, &GetAbsOrigin(), 255, 180, 100, 0, 100, 0.1, 0 );

	// Sound
	EmitSound( "NPC_Manhack.Die" );

	BecomeRagdollOnClient( GetCurrentVelocity() );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CNPC_Manhack::CorpseGib( const CTakeDamageInfo &info )
{
	KillSprites( 0.0 );

	Vector			vecGibVelocity;
	AngularImpulse	vecGibAVelocity;

	if( m_bWasClubbed )
	{
		// If clubbed to death, break apart before the attacker's eyes!
		vecGibVelocity = g_vecAttackDir * -150;

		vecGibAVelocity.x = random->RandomFloat( -2000, 2000 );
		vecGibAVelocity.y = random->RandomFloat( -2000, 2000 );
		vecGibAVelocity.z = random->RandomFloat( -2000, 2000 );
	}
	else
	{
		// Shower the pieces with my velocity.
		vecGibVelocity = GetCurrentVelocity();

		vecGibAVelocity.x = random->RandomFloat( -500, 500 );
		vecGibAVelocity.y = random->RandomFloat( -500, 500 );
		vecGibAVelocity.z = random->RandomFloat( -500, 500 );
	}

	PropBreakableCreateAll( GetModelIndex(), NULL, GetAbsOrigin(), GetAbsAngles(), vecGibVelocity, vecGibAVelocity, 1.0, 60, COLLISION_GROUP_DEBRIS );

	AddFlag( EF_NODRAW );
	SetThink( SUB_Remove );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Explode the manhack if it's damaged while crashing
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Manhack::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	// Ignore damage for the first 1 second of crashing behavior.
	// If we don't do this, manhacks always just explode under 
	// sustained fire.

	VPhysicsTakeDamage( info );
	
	if( gpGlobals->curtime > m_flIgnoreDamageTime )
	{
		CorpseGib( info );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Turn on the engine sound if we're gagged!
//-----------------------------------------------------------------------------
void CNPC_Manhack::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	if( m_vNoiseMod.z == MANHACK_NOISEMOD_HIDE && !(m_spawnflags & SF_NPC_WAIT_FOR_SCRIPT) && !(m_spawnflags & SF_MANHACK_PACKED_UP) )
	{
		// This manhack should get a normal noisemod now.
		float flNoiseMod = random->RandomFloat( 1.7, 2.3 );
		
		// Just bob up and down.
		SetNoiseMod( 0, 0, flNoiseMod );
	}

	if( NewState != NPC_STATE_IDLE && (m_spawnflags & SF_NPC_GAG) && !m_pEngineSound1 )
	{
		m_spawnflags &= ~SF_NPC_GAG;
		SoundInit();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
int CNPC_Manhack::SelectSchedule ( void )
{
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::HandleAnimEvent( animevent_t *pEvent )
{
	Vector vecNewVelocity;
	switch( pEvent->event )
	{
	case MANHACK_AE_START_ENGINE:
		StartEngine( true );
		m_spawnflags &= ~SF_MANHACK_PACKED_UP;

		vecNewVelocity = GetAbsVelocity();
		vecNewVelocity.z = 0.0;
		// SetAbsVelocity( vecNewVelocity );

		// No bursts until fully unpacked!
		m_flNextBurstTime = gpGlobals->curtime + FLT_MAX;
		break;

	case MANHACK_AE_DONE_UNPACKING:
		vecNewVelocity = GetAbsVelocity();
		vecNewVelocity.z = 0.0;
		// SetAbsVelocity( vecNewVelocity );

		m_flNextBurstTime = gpGlobals->curtime + 2.0;
		break;

	case MANHACK_AE_OPEN_BLADE:
		m_bBladesActive = true;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether or not the given activity would translate to flying.
//-----------------------------------------------------------------------------
bool CNPC_Manhack::IsFlyingActivity( Activity baseAct )
{
	return ((baseAct == ACT_FLY || baseAct == ACT_IDLE || baseAct == ACT_RUN || baseAct == ACT_WALK) && m_bBladesActive);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
Activity CNPC_Manhack::NPC_TranslateActivity( Activity baseAct )
{
	if (IsFlyingActivity( baseAct ))
	{
		return (Activity)ACT_FLY;
	}

	return BaseClass::NPC_TranslateActivity( baseAct );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
int CNPC_Manhack::TranslateSchedule( int scheduleType ) 
{
	switch ( scheduleType )
	{
	case SCHED_MELEE_ATTACK1:
		{
			return SCHED_MANHACK_ATTACK_HOVER;
			break;
		}
	case SCHED_BACK_AWAY_FROM_ENEMY:
		{
			return SCHED_MANHACK_REGROUP;
			break;
		}
	case SCHED_CHASE_ENEMY:
		{
			if ( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			{
				return SCHED_CHASE_ENEMY;
			}
			else
			{
				return SCHED_MANHACK_SWARM;
			}
		}
	case SCHED_COMBAT_FACE:
		{
			// Don't care about facing enemy, handled automatically
			return TranslateSchedule( SCHED_CHASE_ENEMY );
			break;
		}
	case SCHED_WAKE_ANGRY:
		{
			if( m_spawnflags & SF_MANHACK_PACKED_UP )
			{
				return SCHED_MANHACK_DEPLOY;
			}
			else
			{
				return TranslateSchedule( SCHED_CHASE_ENEMY );
			}
			break;
		}

	case SCHED_IDLE_STAND:
	case SCHED_ALERT_STAND:
	case SCHED_ALERT_FACE:
		{
			if (m_pSquad)
			{
				return SCHED_MANHACK_SWARM_IDLE;
			}
			else
			{
				return BaseClass::TranslateSchedule(scheduleType);
			}
		}

	case SCHED_CHASE_ENEMY_FAILED:
		{
			// Relentless bastard! Doesn't fail (fail not valid anyway)
			return TranslateSchedule( SCHED_CHASE_ENEMY );
			break;
		}

	}
	return BaseClass::TranslateSchedule(scheduleType);
}

//-----------------------------------------------------------------------------
// Purpose: Handles movement towards the last move target.
// Input  : flInterval - 
//-----------------------------------------------------------------------------
bool CNPC_Manhack::OverrideMove(float flInterval)
{
	SpinBlades( flInterval );
	// StudioFrameAdvance( flInterval );
	
	if( m_spawnflags & SF_MANHACK_PACKED_UP )
	{
		// Don't execute any move code if packed up.
		return true;
	}

	// -----------------------------------------------------------------
	//  If I'm being forced to move somewhere
	// ------------------------------------------------------------------
	if (m_fForceMoveTime > gpGlobals->curtime)
	{
		MoveToTarget(flInterval, m_vForceMoveTarget);
	}
	// -----------------------------------------------------------------
	// If I have a route, keep it updated and move toward target
	// ------------------------------------------------------------------
	else if (GetNavigator()->IsGoalActive())
	{
		// NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, 10 ), 255, 0, 0, true, 0.1);
  		if ( ProgressFlyPath( flInterval, GetEnemy(), MoveCollisionMask() ) == AINPP_COMPLETE )
			return true;
	}
	// -----------------------------------------------------------------
	// If I'm supposed to swarm somewhere, try to go there
	// ------------------------------------------------------------------
	else if (m_fSwarmMoveTime > gpGlobals->curtime)
	{
		MoveToTarget(flInterval, m_vSwarmMoveTarget);
	}
	// -----------------------------------------------------------------
	// If I don't have anything better to do, just decelerate
	// -------------------------------------------------------------- ----
	else
	{
		float	myDecay	 = 9.5;
		Decelerate( flInterval, myDecay );

		m_vTargetBanking = vec3_origin;

		// -------------------------------------
		// If I have an enemy turn to face him
		// -------------------------------------
		if (GetEnemy())
		{
			TurnHeadToTarget(flInterval, GetEnemy()->GetAbsOrigin() );
		}
	}

	if ( m_iHealth <= 0 )
	{
		// Crashing!!
		MoveExecute_Dead(flInterval);
	}
	else
	{
		// Alive!
		MoveExecute_Alive(flInterval);
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::TurnHeadRandomly(float flInterval )
{
	float desYaw = random->RandomFloat(0,360);

	float	iRate	 = 0.8;
	// Make frame rate independent
	float timeToUse = flInterval;
	while (timeToUse > 0)
	{
		m_fHeadYaw	   = (iRate * m_fHeadYaw) + (1-iRate)*desYaw;
		timeToUse =- 0.1;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::MoveToTarget(float flInterval, const Vector &MoveTarget)
{
	if (flInterval <= 0)
	{
		return;
	}

	// -----------------------------------------
	// Don't steer if engine's have stalled
	// -----------------------------------------
	if (gpGlobals->curtime < m_flEngineStallTime || m_iHealth <=0 )
	{
		return;
	}

	TurnHeadToTarget( flInterval, MoveTarget );

	// -------------------------------------
	// Move towards our target
	// -------------------------------------
	float	myAccel;
	float	myZAccel = 300.0;
	float	myDecay	 = 0.35;


	Vector vecCurrentDir;

	// Get the relationship between my current velocity and the way I want to be going.
	vecCurrentDir = GetCurrentVelocity();
	VectorNormalize( vecCurrentDir );

	Vector targetDir = MoveTarget - GetAbsOrigin();
	float flDist = VectorNormalize(targetDir);

	float flDot;
	flDot = DotProduct( targetDir, vecCurrentDir );

	if( flDot > 0.25 )
	{
		// If my target is in front of me, my flight model is a bit more accurate.
		//Msg("Fast Accel\n");
		myAccel = 300;
	}
	else
	{
		// Have a harder time correcting my course if I'm currently flying away from my target.
		//Msg("Slow Accel\n");
		myAccel = 200;
	}

	if (myAccel > flDist / flInterval)
	{
		myAccel = flDist / flInterval;
	}

	if( targetDir.z > 0 )
	{
		// Z acceleration is faster when we thrust upwards.
		// This is to help keep manhacks out of water. 
		myZAccel *= 5.0;
	}

	if (myZAccel > flDist / flInterval)
	{
		myAccel = flDist / flInterval;
	}

	myAccel *= m_fEnginePowerScale;
	myZAccel *= m_fEnginePowerScale;

	MoveInDirection( flInterval, targetDir, myAccel, myZAccel, myDecay );

	m_vTargetBanking.x	=  40 * targetDir.x;
	m_vTargetBanking.z	= -40 *	targetDir.y;
	m_vTargetBanking.y	= 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: Ignore water if I'm close to my enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Manhack::MoveCollisionMask(void)
{
	return MASK_NPCSOLID;
}

//-----------------------------------------------------------------------------
// Purpose: Make a splash effect
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::Splash( const Vector &vecSplashPos )
{
	EmitSound( "NPC_Manhack.Splash" );
	
	Vector vSplashDir = GetCurrentVelocity();
	VectorNormalize(vSplashDir);
	vSplashDir = Vector( 0, 0, 1 ) + 2*vSplashDir;

	CSplash *pSplash = CSplash::CreateSplash( vecSplashPos );
	if(pSplash)
	{
		pSplash->m_flSpawnRate = 400;
		pSplash->m_flParticleLifetime = 0.2;
		pSplash->m_flStopEmitTime		= gpGlobals->curtime+0.1;
		pSplash->m_vStartColor.Init(1.0, 1.0, 1.0);
		pSplash->m_vEndColor.Init(0.4,0.5,0.6);
		pSplash->m_flWidthMin	= 1;
		pSplash->m_flWidthMax	= 6;
		pSplash->m_flNoise		= 0.4;
		pSplash->m_flSpeed		= 125;
		pSplash->m_flSpeedRange = 25;
		QAngle angles;
		VectorAngles(vSplashDir,angles);
		pSplash->SetLocalAngles( angles );
		pSplash->SetLifetime(3);
	}
}

//-----------------------------------------------------------------------------
// Purpose: We've touched something that we can hurt. Slice it!
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::Slice( CBaseEntity *pHitEntity, float flInterval, trace_t &tr )
{
	// Damage must be scaled by flInterval so framerate independent
	float flDamage = sk_manhack_melee_dmg.GetFloat()*flInterval;

	if (flDamage < 1)
	{
		flDamage = 1.0;
	}
	
	if( GetWaterLevel() > 0 && pHitEntity->IsPlayer() )
	{
		// Don't hurt the player if I'm in water
	}
	else if( pHitEntity->m_takedamage != DAMAGE_NO )
	{
		CTakeDamageInfo info( this, this, flDamage, DMG_SLASH );
		Vector dir = (tr.endpos - tr.startpos);
		if ( dir == vec3_origin ) 
			dir = tr.m_pEnt->GetAbsOrigin() - GetAbsOrigin();
		CalculateMeleeDamageForce( &info, dir, tr.endpos );
		pHitEntity->TakeDamage( info );

		// Spawn some extra blood where we hit
		SpawnBlood(tr.endpos, pHitEntity->BloodColor(), sk_manhack_melee_dmg.GetFloat()*flInterval);

		EmitSound( "NPC_Manhack.Slice" );

		// Pop back a little bit after hitting the player
		//if( pHitEntity->IsPlayer() )
		{
			CEffectData data;

			Vector vecDir = GetCurrentVelocity();
			
			vecDir.x *= -1.5;
			vecDir.y *= -1.5;
			vecDir.z *= -0.5;

			if( vecDir.LengthSqr() < 100 * 100 )
			{
				// If the manhack isn't bouncing away from whatever he sliced, force it. 
				vecDir = ( GetAbsOrigin() - pHitEntity->WorldSpaceCenter() );

				VectorNormalize( vecDir );

				vecDir *= 200;
			}

			SetCurrentVelocity( vecDir );
	
			/*
			// random impact
			m_vCurrentBanking.x += random->RandomFloat( -60, 60 );
			m_vCurrentBanking.z += random->RandomFloat( -60, 60 );
			m_fHeadYaw += random->RandomFloat( 30, 60 );
			*/
		}

		// Also, delay speed bursts for a while!
		m_flNextBurstTime = gpGlobals->curtime + 3.0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: We've touched something solid. Just bump it.
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::Bump( CBaseEntity *pHitEntity, float flInterval, trace_t &tr )
{
	if ( pHitEntity->GetMoveType() == MOVETYPE_VPHYSICS && pHitEntity->Classify()!=CLASS_MANHACK )
	{
		HitPhysicsObject( pHitEntity );
	}

	// We've hit something so deflect our velocity based on the surface
	// norm of what we've hit
	if (flInterval > 0)
	{
		float moveLen = ( (GetCurrentVelocity() * flInterval) * (1.0 - tr.fraction) ).Length();

		Vector moveVec	= moveLen*tr.plane.normal/flInterval;

		// If I'm totally dead, don't bounce me up
		if (m_iHealth <=0 && moveVec.z > 0)
		{
			moveVec.z = 0;
		}

		// If I'm right over the ground don't push down
		if (moveVec.z < 0)
		{
			float floorZ = GetFloorZ(GetAbsOrigin());
			if (abs(GetAbsOrigin().z - floorZ) < 36)
			{
				moveVec.z = 0;
			}
		}

		if (!(m_spawnflags	& SF_NPC_GAG))
		{
			EmitSound( "NPC_Manhack.Grind" );
		}

		Vector myUp;
		VPhysicsGetObject()->LocalToWorldVector( myUp, Vector( 0.0, 0.0, 1.0 ) );

		// plane must be something that could hit the blades
		if (fabs( DotProduct( myUp, tr.plane.normal ) ) < 0.7 )
		{
			// For decals and sparks we must trace a line in the direction of the surface norm
			// that we hit.
			AI_TraceLine( GetAbsOrigin(), GetAbsOrigin() - (tr.plane.normal * 24),MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

			if (tr.fraction != 1.0)
			{
				//g_pEffects->Sparks( tr.endpos, 3, 1, &tr.plane.normal );

				CEffectData data;
				Vector velocity = GetCurrentVelocity();

				data.m_vOrigin = tr.endpos;
				data.m_vAngles = GetAbsAngles();

				VectorNormalize( velocity );
				
				data.m_vNormal = ( tr.plane.normal + velocity ) * 0.5;;

				DispatchEffect( "ManhackSparks", data );

				CBroadcastRecipientFilter filter;

				te->DynamicLight( filter, 0.0, &GetAbsOrigin(), 255, 180, 100, 0, 50, 0.3, 150 );

				// Leave decal only if colliding horizontally
				if ((DotProduct(Vector(0,0,1),tr.plane.normal)<0.5) && (DotProduct(Vector(0,0,-1),tr.plane.normal)<0.5))
				{
					UTIL_DecalTrace( &tr, "ManhackCut" );
				}

				// Emit smoke if I'm going fast
				if (GetCurrentVelocity().Length() > 200)
				{
					UTIL_Smoke(tr.endpos, random->RandomInt(5, 10), 15);
				}

				// add some spin
				AngularImpulse torque = myUp * 1000;
				VPhysicsGetObject()->ApplyTorqueCenter( torque );
			}
		}

		SetCurrentVelocity( GetCurrentVelocity() + moveVec );
	}

	// -------------------------------------------------------------
	// If I'm on a path check LOS to my next node, and fail on path
	// if I don't have LOS.  Note this is the only place I do this,
	// so the manhack has to collide before failing on a path
	// -------------------------------------------------------------
	if (GetNavigator()->IsGoalActive())
	{
		AIMoveTrace_t moveTrace;
		GetMoveProbe()->MoveLimit( NAV_FLY, GetAbsOrigin(), GetNavigator()->GetCurWaypointPos(), 
			MoveCollisionMask(), GetEnemy(), &moveTrace );

		if (IsMoveBlocked( moveTrace ) && 
			!moveTrace.pObstruction->ClassMatches( GetClassname() ))
		{
			TaskFail(FAIL_NO_ROUTE);
			GetNavigator()->ClearGoal();
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::CheckCollisions(float flInterval)
{
	// ----------------------------------------
	//	Trace forward to see if I hit anything
	// ----------------------------------------
	Vector checkPos = GetAbsOrigin() + (GetCurrentVelocity() * flInterval);
	trace_t			tr;
	CBaseEntity*	pHitEntity = NULL;
	
	AI_TraceHull(	GetAbsOrigin(), 
					checkPos, 
					GetHullMins(), 
					GetHullMaxs(),
					MoveCollisionMask(),
					this,
					COLLISION_GROUP_NONE,
					&tr );

	if (tr.fraction != 1.0 && tr.m_pEnt)
	{
		PhysicsMarkEntitiesAsTouching( tr.m_pEnt, tr );
		pHitEntity = tr.m_pEnt;

		if (pHitEntity && pHitEntity->m_takedamage == DAMAGE_YES && pHitEntity->Classify()!=CLASS_MANHACK && gpGlobals->curtime > m_flWaterSuspendTime )
		{
			// Slice this thing
			Slice( pHitEntity, flInterval, tr );
			m_flBladeSpeed = 20.0;
		}
		else
		{
			// Just bump into this thing.
			Bump( pHitEntity, flInterval, tr );
			m_flBladeSpeed = 20.0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
#define tempTIME_STEP = 0.5;
void CNPC_Manhack::PlayFlySound(void)
{
	float flEnemyDist;

	if( GetEnemy() )
	{
		flEnemyDist = (GetAbsOrigin() - GetEnemy()->GetAbsOrigin()).Length();
	}
	else
	{
		flEnemyDist = FLT_MAX;
	}

	if( m_spawnflags & SF_NPC_GAG )
	{
		// Quiet!
		return;
	}

	if( m_flWaterSuspendTime > gpGlobals->curtime )
	{
		// Just went in water. Slow the motor!!
		if( m_bDirtyPitch )
		{
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound1, MANHACK_WATER_PITCH1, 0.5 );
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound2, MANHACK_WATER_PITCH2, 0.5 );
			m_bDirtyPitch = false;
		}
	}
	// Spin sound based on distance from enemy (unless we're crashing)
	else if (GetEnemy() && IsAlive() )
	{
		if( flEnemyDist < MANHACK_PITCH_DIST1 )
		{
			// recalculate pitch.
			int iPitch1, iPitch2;
			float flDistFactor;

			flDistFactor = min( 1.0, 1 - flEnemyDist / MANHACK_PITCH_DIST1 ); 
			iPitch1 = MANHACK_MIN_PITCH1 + ( ( MANHACK_MAX_PITCH1 - MANHACK_MIN_PITCH1 ) * flDistFactor); 

			// NOTE: MANHACK_PITCH_DIST2 must be < MANHACK_PITCH_DIST1
			flDistFactor = min( 1.0, 1 - flEnemyDist / MANHACK_PITCH_DIST2 ); 
			iPitch2 = MANHACK_MIN_PITCH2 + ( ( MANHACK_MAX_PITCH2 - MANHACK_MIN_PITCH2 ) * flDistFactor); 

			CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound1, iPitch1, 0.1 );
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound2, iPitch2, 0.1 );

			m_bDirtyPitch = true;
		}
		else if( m_bDirtyPitch )
		{
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound1, MANHACK_MIN_PITCH1, 0.1 );
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound2, MANHACK_MIN_PITCH2, 0.2 );
			m_bDirtyPitch = false;
		}
	}
	// If no enemy just play low sound
	else if( IsAlive() && m_bDirtyPitch )
	{
		CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound1, MANHACK_MIN_PITCH1, 0.1 );
		CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound2, MANHACK_MIN_PITCH2, 0.2 );
		m_bDirtyPitch = false;
	}

	// Play special engine every once in a while
	if (gpGlobals->curtime > m_flNextEngineSoundTime && flEnemyDist < 48)
	{
		m_flNextEngineSoundTime	= gpGlobals->curtime + random->RandomFloat( 3.0, 10.0 );

		EmitSound( "NPC_Manhack.EngineNoise" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::MoveExecute_Alive(float flInterval)
{
	Vector	vCurrentVelocity = GetCurrentVelocity();

	// FIXME: move this
	VPhysicsGetObject()->Wake();

	if( m_fEnginePowerScale < 1.0 && gpGlobals->curtime > m_flWaterSuspendTime )
	{
		// Power is low, and we're no longer stuck in water, so bring power up.
		m_fEnginePowerScale += 0.05;
	}

	// ----------------------------------------------------------------------------------------
	// Add time-coherent noise to the current velocity so that it never looks bolted in place.
	// ----------------------------------------------------------------------------------------
	float noiseScale = 7.0;

	if ( (CBaseEntity*)GetEnemy() )
	{
		float flDist = (GetAbsOrigin() - GetEnemy()->GetAbsOrigin()).Length2D();

		if( flDist < 256 )
		{
			// Less noise up close.
			noiseScale = 2.0;
		}

		if( flDist < 256 && gpGlobals->curtime > m_flNextBurstTime )
		{
			// Am I close enough to commit to an attack?
			float flDot;

			Vector vecToEnemy, vecDir;

			vecDir = GetCurrentVelocity();
			VectorNormalize( vecDir );

			vecToEnemy = ( GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() );
			VectorNormalize( vecToEnemy );

			flDot = DotProduct( vecDir, vecToEnemy );

			if( flDot > 0.75 )
			{
				vecDir = ( GetEnemy()->WorldSpaceCenter() - GetAbsOrigin() );
				VectorNormalize( vecDir );
				
				m_vForceVelocity += ( vecDir * 200 );

				// Don't burst attack again for a couple seconds
				m_flNextBurstTime = gpGlobals->curtime + 2.0;

				// Commit to the attack- no steering for about a second
				m_flEngineStallTime = gpGlobals->curtime + 1.0;
			}
		}

		// steve, what does this code do?
		/*
		if( m_translatedActivity == ACT_MANHACK_UNPACK && !m_fAnimate )
		{
			// See if I should continue unpacking!!
			if( ( flDist / m_vCurrentVelocity.Length() ) <= 1.8 )
			{
				// Alright! Let the unpacking continue!!
				Msg("FINISHING UP!");
				m_fAnimate = true;
			}
		}
		*/
	}

	// ----------------------------------------------------------------------------------------
	// Add in any forced velocity
	// ----------------------------------------------------------------------------------------
	// SetCurrentVelocity( vCurrentVelocity + m_vForceVelocity );
	m_vForceVelocity = vec3_origin;

	AddNoiseToVelocity( noiseScale );

	LimitSpeed( 200, ManhackMaxSpeed() );

	if( m_flWaterSuspendTime > gpGlobals->curtime )
	{ 
		if( UTIL_PointContents( GetAbsOrigin() ) & (CONTENTS_WATER|CONTENTS_SLIME) )
		{
			// Ooops, we're submerged somehow. Move upwards until our origin is out of the water.
			m_vCurrentVelocity.z = 20.0;
		}
		else
		{
			// Skimming the surface. Forbid any movement on Z
			m_vCurrentVelocity.z = 0.0;
		}
	}
	else if( GetWaterLevel() > 0 )
	{
		// Allow the manhack to lift off, but not to go deeper.
		m_vCurrentVelocity.z = max( m_vCurrentVelocity.z, 0 );
	}

	CheckCollisions(flInterval);

	QAngle angles = GetLocalAngles();

	// ------------------------------------------
	//  Stalling
	// ------------------------------------------
	if (gpGlobals->curtime < m_flEngineStallTime)
	{
		/*
		// If I'm stalled add random noise
		angles.x += -20+(random->RandomInt(-10,10));
		angles.z += -20+(random->RandomInt(0,40));

		TurnHeadRandomly(flInterval);
		*/
	}
	else
	{
		// Make frame rate independent
		float	iRate	 = 0.5;
		float timeToUse = flInterval;
		while (timeToUse > 0)
		{
			m_vCurrentBanking.x = (iRate * m_vCurrentBanking.x) + (1 - iRate)*(m_vTargetBanking.x);
			m_vCurrentBanking.z = (iRate * m_vCurrentBanking.z) + (1 - iRate)*(m_vTargetBanking.z);
			timeToUse =- 0.1;
		}
		angles.x = m_vCurrentBanking.x;
		angles.z = m_vCurrentBanking.z;
		angles.y = 0;

		{
			Vector delta( 10 * AngleDiff( m_vTargetBanking.x, m_vCurrentBanking.x ), -10 * AngleDiff( m_vTargetBanking.z, m_vCurrentBanking.z ), 0 );
			//Vector delta( 3 * AngleNormalize( m_vCurrentBanking.x ), -4 * AngleNormalize( m_vCurrentBanking.z ), 0 );
			VectorYawRotate( delta, -m_fHeadYaw, delta );

			// Msg("%.0f %.0f\n", delta.x, delta.y );

			SetPoseParameter( m_iPanel1, -delta.x - delta.y * 2);
			SetPoseParameter( m_iPanel2, -delta.x + delta.y * 2);
			SetPoseParameter( m_iPanel3,  delta.x + delta.y * 2);
			SetPoseParameter( m_iPanel4,  delta.x - delta.y * 2);

			//SetPoseParameter( m_iPanel1, -delta.x );
			//SetPoseParameter( m_iPanel2, -delta.x );
			//SetPoseParameter( m_iPanel3, delta.x );
			//SetPoseParameter( m_iPanel4, delta.x );
		}
	}

	// SetLocalAngles( angles );

	if( m_lifeState != LIFE_DEAD )
	{
		PlayFlySound();
		// SpinBlades( flInterval );
		// WalkMove( GetCurrentVelocity() * flInterval, MASK_NPCSOLID );
	}

	// NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, -10 ), 0, 255, 0, true, 0.1);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::SpinBlades(float flInterval)
{
	if (!m_bBladesActive)
	{
		SetBodygroup( MANHACK_BODYGROUP_BLADE, MANHACK_BODYGROUP_OFF );
		SetBodygroup( MANHACK_BODYGROUP_BLUR, MANHACK_BODYGROUP_OFF );
		m_flBladeSpeed = 0.0;
		m_flPlaybackRate = 1.0;
		return;
	}

	if ( IsFlyingActivity( GetActivity() ) )
	{
		if (m_flBladeSpeed < 10)
		{
			m_flBladeSpeed = m_flBladeSpeed * 2 + 1;
		}
		else
		{
			// accelerate engine
			m_flBladeSpeed = m_flBladeSpeed + 80 * flInterval;
		}

		if (m_flBladeSpeed > 100)
		{
			m_flBladeSpeed = 100;
		}

		// blend through blades, blades+blur, blur
		if (m_flBladeSpeed < 20)
		{
			SetBodygroup( MANHACK_BODYGROUP_BLADE, MANHACK_BODYGROUP_ON );
			SetBodygroup( MANHACK_BODYGROUP_BLUR, MANHACK_BODYGROUP_OFF );
		}
		else if (m_flBladeSpeed < 40)
		{
			SetBodygroup( MANHACK_BODYGROUP_BLADE, MANHACK_BODYGROUP_ON );
			SetBodygroup( MANHACK_BODYGROUP_BLUR, MANHACK_BODYGROUP_ON );
		}
		else
		{
			SetBodygroup( MANHACK_BODYGROUP_BLADE, MANHACK_BODYGROUP_OFF );
			SetBodygroup( MANHACK_BODYGROUP_BLUR, MANHACK_BODYGROUP_ON );
		}

		m_flPlaybackRate = m_flBladeSpeed / 100.0;
	}
	else
	{
		m_flBladeSpeed = 0.0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Smokes and sparks, exploding periodically. Eventually it goes away.
//-----------------------------------------------------------------------------
void CNPC_Manhack::MoveExecute_Dead(float flInterval)
{
	if( GetWaterLevel() > 0 )
	{
		// No movement if sinking in water.
		return;
	}

	// Periodically emit smoke.
	if (gpGlobals->curtime > m_fSmokeTime && GetWaterLevel() == 0)
	{
		UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);
		m_fSmokeTime = gpGlobals->curtime + random->RandomFloat( 0.1, 0.3);
	}

	// Periodically emit sparks.
	if (gpGlobals->curtime > m_fSparkTime)
	{
		g_pEffects->Sparks( GetAbsOrigin() );
		m_fSparkTime = gpGlobals->curtime + random->RandomFloat(0.1, 0.3);
	}

	Vector newVelocity = GetCurrentVelocity();

	// accelerate faster and faster when dying
	newVelocity = newVelocity + (newVelocity * 1.5 * flInterval );

	// Lose lift
	newVelocity.z -= 0.02*flInterval*(sv_gravity.GetFloat());

	// ----------------------------------------------------------------------------------------
	// Add in any forced velocity
	// ----------------------------------------------------------------------------------------
	newVelocity += m_vForceVelocity;
	SetCurrentVelocity( newVelocity );
	m_vForceVelocity = vec3_origin;


	// Lots of noise!! Out of control!
	AddNoiseToVelocity( 5.0 );


	// ----------------------
	// Limit overall speed
	// ----------------------
	LimitSpeed( -1, MANHACK_MAX_SPEED * 2.0 );

	QAngle angles = GetLocalAngles();

	// ------------------------------------------
	// If I'm dying, add random banking noise
	// ------------------------------------------
	angles.x += -20+(random->RandomInt(0,40));
	angles.z += -20+(random->RandomInt(0,40));

	CheckCollisions(flInterval);
	PlayFlySound();

	// SetLocalAngles( angles );

	WalkMove( GetCurrentVelocity() * flInterval,MASK_NPCSOLID );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::Precache(void)
{
	//
	// Model.
	//
	engine->PrecacheModel("models/manhack.mdl");
	engine->PrecacheModel( MANHACK_GLOW_SPRITE );
	PropBreakablePrecacheAll( MAKE_STRING("models/manhack.mdl") );
	
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	// The Manhack "regroups" when its in attack range but to
	// far above or below its enemy.  Set the start attack
	// condition if we are far enough away from the enemy
	// or at the correct height
	if ((GetAbsOrigin() - pEnemy->GetAbsOrigin()).Length2D() > 60.0) 
	{
		SetCondition(COND_MANHACK_START_ATTACK);
	}
	else
	{
		float targetZ	= pEnemy->EyePosition().z;
		if (fabs(GetAbsOrigin().z - targetZ) < 12.0)
		{
			SetCondition(COND_MANHACK_START_ATTACK);
		}
	}
	BaseClass::GatherEnemyConditions(pEnemy);
}

//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Manhack::MeleeAttack1Conditions( float flDot, float flDist )
{
	if (flDist > 45)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	if (flDist < 24)
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}

	if (GetEnemy() == NULL)
	{
		return COND_NONE;
	}

	// Assume the this check is in regards to my current enemy
	// for the Manhacks spetial condition
	float targetZ	= GetEnemy()->EyePosition().z;
	if  (((GetAbsOrigin().z - targetZ) >  12.0) ||
		 ((GetAbsOrigin().z - targetZ) < -24.0) )
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	return COND_CAN_MELEE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		// Override this task so we go for the enemy at eye level
	case TASK_MANHACK_HOVER:
		{
			break;
		}
		// If my enemy has moved significantly, update my path
	case TASK_WAIT_FOR_MOVEMENT:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if (pEnemy												&&
				GetCurSchedule()->GetId()	== SCHED_CHASE_ENEMY	&& 
				GetNavigator()->IsGoalActive()										)
			{
				Vector flEnemyLKP = GetEnemyLKP();
				if ((GetNavigator()->GetGoalPos() - pEnemy->EyePosition()).Length() > 40 )
				{
					GetNavigator()->UpdateGoalPos(pEnemy->EyePosition());
				}
			}				
			BaseClass::RunTask(pTask);
			break;
		}
	case TASK_MANHACK_MOVEAT_SAVEPOSITION:
		{
			// do the movement thingy

			// NDebugOverlay::Line( GetAbsOrigin(), m_vSavePosition, 0, 255, 0, true, 0.1);

			Vector dir = (m_vSavePosition - GetAbsOrigin());
			float dist = VectorNormalize( dir );
			float t = m_fSwarmMoveTime - gpGlobals->curtime;

			if (t < 0.1)
			{
				if (dist > 256)
				{
					TaskFail( FAIL_NO_ROUTE );
				}
				else
				{
					TaskComplete();
				}
			}
			else if (dist < 64)
			{
				m_vSwarmMoveTarget = GetAbsOrigin() - Vector( -dir.y, dir.x, 0 ) * 4;
			}
			else
			{
				m_vSwarmMoveTarget = GetAbsOrigin() + dir * 10;
			}
			break;
		}

	default:
		{
			BaseClass::RunTask(pTask);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::Spawn(void)
{
	Precache();

	SetModel( "models/manhack.mdl" );
	SetHullType(HULL_TINY_CENTERED); 
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetMoveType( MOVETYPE_VPHYSICS );

	m_iHealth			= sk_manhack_health.GetFloat();
	SetViewOffset( Vector(0, 0, 10) );		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_NPCState			= NPC_STATE_NONE;
	// SetNavType(NAV_FLY);
	SetBloodColor( DONT_BLEED );
	SetCurrentVelocity( Vector(0, 0, 0) );
	m_vForceVelocity	= Vector(0, 0, 0);
	m_vCurrentBanking	= Vector(0, 0, 0);
	m_vTargetBanking	= Vector(0, 0, 0);

	m_flNextBurstTime	= gpGlobals->curtime;

	CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_MOVE_FLY | bits_CAP_SQUAD );

	m_flNextEngineSoundTime		= gpGlobals->curtime;
	m_flWaterSuspendTime		= gpGlobals->curtime;
	m_flEngineStallTime			= gpGlobals->curtime;
	m_fForceMoveTime			= gpGlobals->curtime;
	m_vForceMoveTarget			= vec3_origin;
	m_fSwarmMoveTime			= gpGlobals->curtime;
	m_vSwarmMoveTarget			= vec3_origin;
	m_nLastSpinSound			= -1;

	m_fSmokeTime		= 0;
	m_fSparkTime		= 0;

	// Set the noise mod to huge numbers right now, in case this manhack starts out waiting for a script
	// for instance, we don't want him to bob whilst he's waiting for a script. This allows designers
	// to 'hide' manhacks in small places. (sjb)
	SetNoiseMod( MANHACK_NOISEMOD_HIDE, MANHACK_NOISEMOD_HIDE, MANHACK_NOISEMOD_HIDE );

	//Create our Eye sprite
	m_iEyeAttachment = LookupAttachment( "Eye" );
	m_pEyeGlow = CSprite::SpriteCreate( MANHACK_GLOW_SPRITE, GetLocalOrigin(), false );
	m_pEyeGlow->SetTransparency( kRenderTransAdd, 255, 0, 0, 128, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( this, m_iEyeAttachment );
	m_pEyeGlow->SetColor( 255, 0, 0 );
	m_pEyeGlow->SetBrightness( 164, 0.1f );
	m_pEyeGlow->SetScale( 0.4f, 0.1f );

	//Create our light sprite
	m_iLightAttachment = LookupAttachment( "Light" );
	m_pLightGlow = CSprite::SpriteCreate( MANHACK_GLOW_SPRITE, GetLocalOrigin(), false );
	m_pLightGlow->SetTransparency( kRenderTransAdd, 255, 0, 0, 128, kRenderFxNoDissipation );
	m_pLightGlow->SetAttachment( this, m_iLightAttachment );
	m_pLightGlow->SetColor( 255, 0, 0 );
	m_pLightGlow->SetBrightness( 255, 0.1f );
	m_pLightGlow->SetScale( 0.25f, 0.1f );

	// Create the glowing light trail
	/*
	// Re-enable for light trails
	m_pLightTrail= CSpriteTrail::SpriteTrailCreate( "sprites/bluelaser1.vmt", GetLocalOrigin(), false );
	m_pLightTrail->SetAttachment( this, m_iLightAttachment );
	m_pLightTrail->SetTransparency( kRenderTransAdd, 255, 0, 0, 128, kRenderFxNone );
	m_pLightTrail->SetScale( 8.0f );
	m_pLightTrail->SetLifeTime( 0.5f );
	*/

	// Start out with full power! 
	m_fEnginePowerScale = 1.0;
	
	// find panels
	m_iPanel1 = LookupPoseParameter( "Panel1" );
	m_iPanel2 = LookupPoseParameter( "Panel2" );
	m_iPanel3 = LookupPoseParameter( "Panel3" );
	m_iPanel4 = LookupPoseParameter( "Panel4" );

	m_fHeadYaw			= 0;

	NPCInit();

	// Manhacks are designed to slam into things, so don't take much damage from it!
	SetImpactEnergyScale( 0.001 );

	// Manhacks get five minutes' worth of free knowledge.
	GetEnemies()->SetFreeKnowledgeDuration( 300.0 );
	
	// don't be an NPC, we want to collide with debris stuff
	SetCollisionGroup( COLLISION_GROUP_NONE );
}



//-----------------------------------------------------------------------------
// Purpose: Get the engine sound started. Unless we're not supposed to have it on yet!
//-----------------------------------------------------------------------------
void CNPC_Manhack::PostNPCInit( void )
{
	// SetAbsVelocity( vec3_origin ); 
	m_bBladesActive = (m_spawnflags & SF_MANHACK_PACKED_UP) ? false : true;
	BladesInit();
}


void CNPC_Manhack::BladesInit()
{
	if( !m_bBladesActive )
	{
		// manhack is packed up, so has no power of its own. 
		// don't start the engine sounds.
		// make us fall a little slower than we should, for visual's sake
		SetGravity( 0.5 );

		SetActivity( ACT_IDLE );
	}
	else
	{
		bool engineSound = (m_spawnflags & SF_NPC_GAG) ? false : true;
		StartEngine( engineSound );
		SetActivity( ACT_FLY );
	}
}


//-----------------------------------------------------------------------------
// Crank up the engine!
//-----------------------------------------------------------------------------
void CNPC_Manhack::StartEngine( bool fStartSound )
{
	if( fStartSound )
	{
		SoundInit();
	}

	// Make the blade appear.
	SetBodygroup( 1, 1 );

	// Pop up a little if falling fast!
	Vector vecVelocity = GetAbsVelocity();
	if( ( m_spawnflags & SF_MANHACK_PACKED_UP ) && vecVelocity.z < 0 )
	{
		// Msg(" POP UP \n" );

		vecVelocity.z *= 0.25;
		// SetAbsVelocity( vecVelocity );
	}

	// Under powered flight now.
	// SetMoveType( MOVETYPE_STEP );
	// SetGravity( MANHACK_GRAVITY );
	AddFlag( FL_FLY );
}

//-----------------------------------------------------------------------------
// Purpose: Start the manhack's engine sound.
//-----------------------------------------------------------------------------
void CNPC_Manhack::SoundInit( void )
{
	CPASAttenuationFilter filter( this );

	// play an engine start sound!!

	// Bring up the engine looping sound.
	if( !m_pEngineSound1 )
	{
		m_pEngineSound1 = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), "NPC_Manhack.EngineSound1" );
		CSoundEnvelopeController::GetController().Play( m_pEngineSound1, 0.0, MANHACK_MIN_PITCH1 );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pEngineSound1, 0.7, 2.0 );
	}

	if( !m_pEngineSound2 )
	{
		m_pEngineSound2 = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), "NPC_Manhack.EngineSound2" );
		CSoundEnvelopeController::GetController().Play( m_pEngineSound2, 0.0, MANHACK_MIN_PITCH2 );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pEngineSound2, 0.7, 2.0 );
	}

	if( !m_pBladeSound )
	{
		m_pBladeSound = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), "NPC_Manhack.BladeSound" );
		CSoundEnvelopeController::GetController().Play( m_pBladeSound, 0.0, MANHACK_MIN_PITCH1 );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pBladeSound, 0.7, 2.0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{	
	case TASK_MANHACK_UNPACK:
		{
			// Just play a sound for now.
			EmitSound( "NPC_Manhack.Unpack" );

			TaskComplete();
		}
		break;

	case TASK_MANHACK_HOVER:
		break;

	case TASK_MOVE_TO_TARGET_RANGE:
	case TASK_GET_PATH_TO_GOAL:
	case TASK_GET_PATH_TO_ENEMY_LKP:
	case TASK_GET_PATH_TO_PLAYER:
		{
			BaseClass::StartTask( pTask );
			/*
			// FIXME: why were these tasks considered bad?
			_asm
			{
				int	3;
				int 5;
			}
			*/
		}
		break;

	case TASK_FACE_IDEAL:
		{
			// this shouldn't ever happen, but if it does, don't choke
			TaskComplete();
		}
		break;

	case TASK_GET_PATH_TO_ENEMY:
		{
			if (IsUnreachable(GetEnemy()))
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}

			CBaseEntity *pEnemy = GetEnemy();

			if ( pEnemy == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
						
			if ( GetNavigator()->SetGoal( GOALTYPE_ENEMY ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =( 
				DevWarning( 2, "GetPathToEnemy failed!!\n" );
				RememberUnreachable(GetEnemy());
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}
		break;

	case TASK_GET_PATH_TO_TARGET:
		// Msg("TARGET\n");
		BaseClass::StartTask( pTask );
		break;

	case TASK_MANHACK_FIND_SQUAD_CENTER:
		{
			if (!m_pSquad)
			{
				m_vSavePosition = GetAbsOrigin();
				TaskComplete();
				break;
			}

			// calc center of squad
			int count = 0;
			m_vSavePosition = Vector( 0, 0, 0 );

			// give attacking members more influence
			AISquadIter_t iter;
			for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
			{
				if (pSquadMember->HasStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ))
				{
					m_vSavePosition += pSquadMember->GetAbsOrigin() * 10;
					count += 10;
				}
				else
				{
					m_vSavePosition += pSquadMember->GetAbsOrigin();
					count++;
				}
			}

			// pull towards enemy
			if (GetEnemy() != NULL)
			{
				m_vSavePosition += GetEnemyLKP() * 4;
				count += 4;
			}



			Assert( count != 0 );
			m_vSavePosition = m_vSavePosition * (1.0 / count);

			TaskComplete();
		}
		break;

	case TASK_MANHACK_FIND_SQUAD_MEMBER:
		{
			if (m_pSquad)
			{
				CAI_BaseNPC *pSquadMember = m_pSquad->GetAnyMember();
				m_vSavePosition = pSquadMember->GetAbsOrigin();

				// find attacking members
				AISquadIter_t iter;
				for (pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
				{
					// are they attacking?
					if (pSquadMember->HasStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ))
					{
						m_vSavePosition = pSquadMember->GetAbsOrigin();
						break;
					}
					// do they have a goal?
					if (pSquadMember->GetNavigator()->IsGoalActive())
					{
						m_vSavePosition = pSquadMember->GetAbsOrigin();
						break;
					}
				}
			}
			else
			{
				m_vSavePosition = GetAbsOrigin();
			}

			TaskComplete();
		}
		break;

	case TASK_MANHACK_MOVEAT_SAVEPOSITION:
		{
			trace_t tr;
			AI_TraceLine( GetAbsOrigin(), m_vSavePosition, MASK_NPCWORLDSTATIC, this, COLLISION_GROUP_NONE, &tr );
			if (tr.DidHitWorld())
			{
				TaskFail( FAIL_NO_ROUTE );
			}
			else
			{
				m_fSwarmMoveTime = gpGlobals->curtime + RandomFloat( pTask->flTaskData * 0.8, pTask->flTaskData * 1.2 );
			}
		}
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::UpdateOnRemove( void )
{
	KillSprites( 1.0 );
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Manhack::HandleInteraction(int interactionType, void* data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionVortigauntClaw)
	{
		// Freeze so vortigaunt and hit me easier

		m_vForceMoveTarget.x = ((Vector *)data)->x;
		m_vForceMoveTarget.y = ((Vector *)data)->y;
		m_vForceMoveTarget.z = ((Vector *)data)->z;
		m_fForceMoveTime   = gpGlobals->curtime + 2.0;
		return false;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_Manhack::ManhackMaxSpeed( void )
{
	if( m_flWaterSuspendTime > gpGlobals->curtime )
	{
		// Slower in water!
		return MANHACK_MAX_SPEED * 0.1;
	}

/*
	if( m_spawnflags & SF_MANHACK_LIMIT_SPEED )
	{
		// Slower if asked to be
		return MANHACK_MAX_SPEED * 0.5;
	}
*/
	
	return MANHACK_MAX_SPEED;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output :
//-----------------------------------------------------------------------------
void CNPC_Manhack::ClampMotorForces( Vector &linear, AngularImpulse &angular )
{
	if (m_flBladeSpeed >= 50)
	{
		// limit reaction forces'
		float scale = (m_flBladeSpeed - 50) * 40;

		linear.x = clamp( linear.x, -scale, scale );
		linear.y = clamp( linear.y, -scale, scale );
		linear.z = clamp( linear.z, -scale, scale < 1200 ? 1200 : scale );

		scale = (m_flBladeSpeed - 50) * 20;
		angular.x = clamp( angular.x, -scale, scale );
		angular.y = clamp( angular.y, -scale, scale );
		angular.z = clamp( angular.z, -scale * 0.5, scale * 0.5 );
	}
	else
	{
		// don't react if the blade is running slowly
		linear.x = 0; // clamp( linear.x, -100, 100 );
		linear.y = 0; // clamp( linear.y, -100, 100 );
		linear.z = clamp( linear.z, 0, 1200 );  // always allow some Z

		angular.Init();
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Manhack::KillSprites( float flDelay )
{
	if( m_pEyeGlow )
	{
		m_pEyeGlow->FadeAndDie( flDelay );
		m_pEyeGlow = NULL;
	}

	if( m_pLightGlow )
	{
		m_pLightGlow->FadeAndDie( flDelay );
		m_pLightGlow = NULL;
	}

	/*
	// Re-enable for light trails
	if ( m_pLightTrail )
	{
		m_pLightTrail->FadeAndDie( flDelay );
		m_pLightTrail = NULL;
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
//			&chasePosition - 
//			&tolerance - 
//-----------------------------------------------------------------------------
void CNPC_Manhack::TranslateEnemyChasePosition( CBaseEntity *pEnemy, Vector &chasePosition, float &tolerance )
{
	Vector offset;
	
	offset.x = 0.0;
	offset.y = 0.0;
	offset.z = pEnemy->WorldAlignSize().z * 0.75;

	chasePosition += offset;

	tolerance = GetHullWidth();
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_manhack, CNPC_Manhack )

	DECLARE_TASK( TASK_MANHACK_HOVER );
	DECLARE_TASK( TASK_MANHACK_UNPACK );
	DECLARE_TASK( TASK_MANHACK_FIND_SQUAD_CENTER );
	DECLARE_TASK( TASK_MANHACK_FIND_SQUAD_MEMBER );
	DECLARE_TASK( TASK_MANHACK_MOVEAT_SAVEPOSITION );

	DECLARE_CONDITION( COND_MANHACK_START_ATTACK );

	DECLARE_ACTIVITY( ACT_MANHACK_UNPACK );

//=========================================================
// > SCHED_MANHACK_ATTACK_HOVER
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_MANHACK_ATTACK_HOVER,

	"	Tasks"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_FLY"
	"		TASK_MANHACK_HOVER		0"
	"	"
	"	Interrupts"
	"		COND_TOO_FAR_TO_ATTACK"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_OCCLUDED"
);


//=========================================================
// > SCHED_MANHACK_ATTACK_HOVER
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_MANHACK_DEPLOY,

	"	Tasks"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_MANHACK_UNPACK"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_FLY"
	"	"
//	"	Interrupts"
);

//=========================================================
// > SCHED_MANHACK_REGROUP
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_MANHACK_REGROUP,

	"	Tasks"
	"		TASK_STOP_MOVING							0"
	"		TASK_SET_TOLERANCE_DISTANCE					24"
	"		TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION	0"
	"		TASK_FIND_BACKAWAY_FROM_SAVEPOSITION		0"
	"		TASK_RUN_PATH								0"
	"		TASK_WAIT_FOR_MOVEMENT						0"
	"	"
	"	Interrupts"
	"		COND_MANHACK_START_ATTACK"
	"		COND_NEW_ENEMY"
	"		COND_CAN_MELEE_ATTACK1"
);



//=========================================================
// > SCHED_MANHACK_SWARN
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_MANHACK_SWARM_IDLE,

	"	Tasks"
	"		TASK_STOP_MOVING							0"
	"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_MANHACK_SWARM_FAILURE"
	"		TASK_MANHACK_FIND_SQUAD_CENTER				0"
	"		TASK_MANHACK_MOVEAT_SAVEPOSITION			5"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
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


DEFINE_SCHEDULE
(
	SCHED_MANHACK_SWARM,

	"	Tasks"
	"		TASK_STOP_MOVING							0"
	"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_MANHACK_SWARM_FAILURE"
	"		TASK_MANHACK_FIND_SQUAD_CENTER				0"
	"		TASK_MANHACK_MOVEAT_SAVEPOSITION			1"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
);

DEFINE_SCHEDULE
(
	SCHED_MANHACK_SWARM_FAILURE,

	"	Tasks"
	"		TASK_STOP_MOVING							0"
	"		TASK_WAIT									2"
	"		TASK_WAIT_RANDOM							2"
	"		TASK_MANHACK_FIND_SQUAD_MEMBER				0"
	"		TASK_GET_PATH_TO_SAVEPOSITION				0"
	"		TASK_WAIT_FOR_MOVEMENT						0"
	"	"
	"	Interrupts"
	"		COND_SEE_ENEMY"
	"		COND_NEW_ENEMY"
);

AI_END_CUSTOM_NPC()


