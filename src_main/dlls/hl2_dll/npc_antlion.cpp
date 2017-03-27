//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Antlion - nasty bug
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "AI_BaseNPC.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Hint.h"
#include "AI_Navigator.h"
#include "AI_Motor.h"
#include "AI_SquadSlot.h"
#include "AI_Squad.h"
#include "ai_moveprobe.h"
#include "activitylist.h"
#include "soundent.h"
#include "game.h"
#include "NPCEvent.h"
#include "Player.h"
#include "gib.h"
#include "EntityList.h"
#include "AI_Interactions.h"
#include "ndebugoverlay.h"
#include "antlion_dust.h"
#include "shake.h"
#include "monstermaker.h"
#include "TemplateEntities.h"
#include "entitymapdata.h"
#include "decals.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "grenade_bugbait.h"
#include "globalstate.h"
#include "movevars_shared.h"
#include "IServerVehicle.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "vehicle_base.h"
#include "mapentities.h"

//FIXME: This should be externalized in the monster maker code!
#define SF_ANTLIONMAKER_SPAWN_FACING_PLAYER 0x00000200

#define	SF_ANTLION_BURROW_ON_ELUDED	1024	//1024

//Debug visualization
ConVar	g_antlion_debug( "g_debug_antlion", "0" );

ConVar	sk_antlion_health( "sk_antlion_health", "0" );
ConVar	sk_antlion_swipe_damage( "sk_antlion_swipe_damage", "0" );
ConVar	sk_antlion_jump_damage( "sk_antlion_jump_damage", "0" );
 
extern ConVar bugbait_radius;

//Animation events
//#define ANTLION_AE_					11
#define	ANTLION_AE_FOOTSTEP_SOFT		12	//Soft footstep
#define	ANTLION_AE_FOOTSTEP				13	//Normal footstep
#define	ANTLION_AE_FOOTSTEP_HEAVY		14	//Hard footstep
#define ANTLION_AE_MELEE_HIT1			15	//Melee hit, one arm
#define ANTLION_AE_MELEE_HIT2			16	//Melee hit, both arms
//#define ANTLION_AE_					17
#define	ANTLION_AE_START_JUMP			18	//Start of the jump
#define	ANTLION_AE_BURROW_IN			19	//Burrowing in
#define	ANTLION_AE_BURROW_OUT			20	//Burrowing out
#define	ANTLION_AE_VANISH				21	//Vanish (invis and not solid)
#define	ANTLION_AE_OPEN_WINGS			22	//Open wings (sound and bodygroup)
#define	ANTLION_AE_CLOSE_WINGS			23  //Close wings (sound and bodygroup)
#define	ANTLION_AE_MELEE1_SOUND			24
#define	ANTLION_AE_MELEE2_SOUND			25

//Attack range definitions
#define	ANTLION_MELEE1_RANGE		100.0f
#define	ANTLION_JUMP_MIN			128.0f
#define	ANTLION_JUMP_MAX			2048.0f

#define	ANTLION_MIN_BUGBAIT_GOAL_TARGET_RADIUS	512

//Interaction IDs
int g_interactionAntlionFoundTarget = 0;

#define	ANTLION_MODEL	"models/antlion.mdl"

#define	ANTLION_BURROW_IN	0
#define	ANTLION_BURROW_OUT	1

#define	NUM_ANTLION_GIBS_SMALL	3
#define	NUM_ANTLION_GIBS_MEDIUM	3
#define	NUM_ANTLION_GIBS_LARGE	3

#define	ANTLION_BUGBAIT_NAV_TOLERANCE	200

//
// Antlion class
//

class CNPC_Antlion : public CAI_BaseNPC
{
public:

	DECLARE_CLASS( CNPC_Antlion, CAI_BaseNPC );

	CNPC_Antlion( void );

	float		MaxYawSpeed( void );
	bool		FInViewCone( CBaseEntity *pEntity );
	bool		FInViewCone( const Vector &vecSpot );
				
	void		HandleAnimEvent( animevent_t *pEvent );
	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );
	void		IdleSound( void );
	void		PainSound( void );
	void		Precache( void );
	void		Spawn( void );
	void		TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void		BuildScheduleTestBits( void );
	void		GatherConditions( void );
	void		PrescheduleThink( void );
	void		BurrowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
				
	bool		IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const;
	bool		HandleInteraction( int interactionType, void *data, CBaseCombatCharacter *sender = NULL );
	bool		QuerySeeEntity( CBaseEntity *pEntity );
	bool		ShouldPlayIdleSound( void );
	bool		OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	bool		IsValidEnemy(CBaseEntity *pEnemy);
	bool		QueryHearSound( CSound *pSound );
	bool		IsLightDamage( float flDamage, int bitsDamageType );
	int			MeleeAttack1Conditions( float flDot, float flDist );
	int			SelectSchedule( void );
	int			GetSoundInterests( void ) { return (BaseClass::GetSoundInterests())|(SOUND_DANGER|SOUND_PHYSICS_DANGER|SOUND_THUMPER|SOUND_BUGBAIT); }
	Class_T		Classify( void ) { return CLASS_ANTLION; }
	void		Event_Killed( const CTakeDamageInfo &info );
	bool		FValidateHintType ( CAI_Hint *pHint );
	void		GatherEnemyConditions( CBaseEntity *pEnemy );
	
	bool		ShouldGib( const CTakeDamageInfo &info );
	bool		CorpseGib( const CTakeDamageInfo &info );

	void		Touch( CBaseEntity *pOther );

	float		GetMaxJumpSpeed() const { return 700.0f; }

	DECLARE_DATADESC();

	bool	m_bStartBurrowed;

private:

	void	InputFightToPosition( inputdata_t &inputdata );

	void	Burrow( void );
	void	Unburrow( void );
	void	InputUnburrow( inputdata_t &inputdata );
	void	InputBurrow( inputdata_t &inputdata );
	bool	FindBurrow( const Vector &origin, float distance, int type, bool excludeNear = true );
	bool	FindBurrowRoute( const Vector &goal );
	void	ClearBurrowPoint( const Vector &origin );
	void	CreateDust( bool placeDecal = true );

	bool	ValidBurrowPoint( const Vector &point );
	bool	CheckLanding( void );
	bool	Alone( void );
	bool	CheckAlertRadius( void );
	bool	ShouldJump( void );

	void	MeleeAttack( float distance, float damage, QAngle& viewPunch, Vector& shove );
	void	SetWings( bool state );
	void	StartJump( void );

	float	m_flIdleDelay;
	float	m_flBurrowTime;
	float	m_flJumpTime;
	float	m_flAlertRadius;

	int		m_iContext;			//for FValidateHintType context

	COutputEvent	m_OnReachFightGoal;	//Reached our scripted destination to fight to

	Vector	m_vecSavedJump;
	Vector	m_vecLastJumpAttempt;

	float	m_flIgnoreSoundTime;//Sound time to ignore if earlier than
	Vector	m_vecHeardSound;
	bool	m_bHasHeardSound;
	bool	m_bAgitatedSound;	//Playing agitated sound?
	bool	m_bWingsOpen;		//Are the wings open?

	bool	m_bFightingToPosition;
	Vector	m_vecFightGoal;
	float	m_flEludeDistance;	//Distance until the antlion will consider himself "eluded" if so flagged

	static const char *m_AntlionGibsSmall[NUM_ANTLION_GIBS_SMALL];
	static const char *m_AntlionGibsMedium[NUM_ANTLION_GIBS_MEDIUM];
	static const char *m_AntlionGibsLarge[NUM_ANTLION_GIBS_LARGE];

	DEFINE_CUSTOM_AI;
};

const char *CNPC_Antlion::m_AntlionGibsSmall[NUM_ANTLION_GIBS_SMALL] = {
"models/gibs/antlion_gib_small_1.mdl",
"models/gibs/antlion_gib_small_2.mdl",
"models/gibs/antlion_gib_small_3.mdl"
};

const char *CNPC_Antlion::m_AntlionGibsMedium[NUM_ANTLION_GIBS_MEDIUM] = {
"models/gibs/antlion_gib_medium_1.mdl",
"models/gibs/antlion_gib_medium_2.mdl",
"models/gibs/antlion_gib_medium_3.mdl",
};

const char *CNPC_Antlion::m_AntlionGibsLarge[NUM_ANTLION_GIBS_LARGE] = {
"models/gibs/antlion_gib_large_1.mdl",
"models/gibs/antlion_gib_large_2.mdl",
"models/gibs/antlion_gib_large_3.mdl",
};

//==================================================
// AntlionConditions
//==================================================

enum AntlionConditions
{
	COND_ANTLION_FLIPPED = LAST_SHARED_CONDITION,
	COND_ANTLION_ON_NPC,
	COND_ANTLION_CAN_JUMP,
};

//==================================================
// AntlionSchedules
//==================================================

enum AntlionSchedules
{
	SCHED_ANTLION_CHASE_ENEMY_BURROW = LAST_SHARED_SCHEDULE,
	SCHED_ANTLION_JUMP,
	SCHED_ANTLION_RUN_TO_BURROW_IN,
	SCHED_ANTLION_BURROW_IN,
	SCHED_ANTLION_BURROW_WAIT,
	SCHED_ANTLION_BURROW_OUT,
	SCHED_ANTLION_WAIT_FOR_UNBORROW_TRIGGER,
	SCHED_ANTLION_WAIT_FOR_CLEAR_UNBORROW,
	SCHED_ANTLION_WAIT_UNBORROW,
	SCHED_ANTLION_FLEE_THUMPER,
	SCHED_ANTLION_CHASE_BUGBAIT,
	SCHED_ANTLION_FLIP,
	SCHED_ANTLION_DISMOUNT_NPC,
	SCHED_ANTLION_RUN_TO_FIGHT_GOAL,
	SCHED_ANTLION_BUGBAIT_IDLE_STAND,
	SCHED_ANTLION_BURROW_AWAY,
	SCHED_ANTLION_FLEE_PHYSICS_DANGER,
};

//==================================================
// AntlionTasks
//==================================================

enum AntlionTasks
{
	TASK_ANTLION_SET_CHARGE_GOAL = LAST_SHARED_TASK,
	TASK_ANTLION_FIND_BURROW_IN_POINT,
	TASK_ANTLION_FIND_BURROW_ROUTE,
	TASK_ANTLION_FIND_BURROW_OUT_POINT,
	TASK_ANTLION_BURROW,
	TASK_ANTLION_UNBURROW,
	TASK_ANTLION_VANISH,
	TASK_ANTLION_BURROW_WAIT,
	TASK_ANTLION_CHECK_FOR_UNBORROW,
	TASK_ANTLION_JUMP,
	TASK_ANTLION_WAIT_FOR_TRIGGER,
	TASK_ANTLION_GET_THUMPER_ESCAPE_PATH,
	TASK_ANTLION_GET_PATH_TO_BUGBAIT,
	TASK_ANTLION_FACE_BUGBAIT,
	TASK_ANTLION_DISMOUNT_NPC,
	TASK_ANTLION_REACH_FIGHT_GOAL,
	TASK_ANTLION_GET_PHYSICS_DANGER_ESCAPE_PATH,
	TASK_ANTLION_FACE_JUMP,
};

//==================================================
// AntlionSquadSlots
//==================================================

enum AntlionSquadSlots
{	
	SQUAD_SLOT_ANTLION_JUMP = LAST_SHARED_SQUADSLOT,
};

//==================================================
// Antlion Activities
//==================================================

int ACT_ANTLION_JUMP_START;
int	ACT_ANTLION_DISTRACT;
int ACT_ANTLION_BURROW_IN;
int ACT_ANTLION_BURROW_OUT;
int ACT_ANTLION_BURROW_IDLE;
int	ACT_ANTLION_RUN_AGITATED;
int ACT_ANTLION_FLIP;


//==================================================
// CNPC_Antlion
//==================================================

CNPC_Antlion::CNPC_Antlion( void )
{
	m_flIdleDelay	= 0.0f;
	m_flBurrowTime	= 0.0f;
	m_flJumpTime	= 0.0f;

	m_flAlertRadius	= 256.0f;
	m_flFieldOfView	= -0.5f;

	m_bStartBurrowed	= false;
	m_bAgitatedSound	= false;
	m_bWingsOpen		= false;

	//FIXME: This isn't a sure bet
	m_flIgnoreSoundTime	= 0.0f;
	m_bHasHeardSound	= false;

	m_vecLastJumpAttempt.Init();
	m_vecSavedJump.Init();

	m_bFightingToPosition = false;
	m_vecFightGoal.Init();
}

LINK_ENTITY_TO_CLASS( npc_antlion, CNPC_Antlion );

//==================================================
// CNPC_Antlion::m_DataDesc
//==================================================

BEGIN_DATADESC( CNPC_Antlion )

	DEFINE_KEYFIELD( CNPC_Antlion, m_bStartBurrowed,FIELD_BOOLEAN, "startburrowed" ),

	DEFINE_FIELD( CNPC_Antlion, m_flIdleDelay,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_Antlion, m_flBurrowTime,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_Antlion, m_flJumpTime,		FIELD_TIME ),
	DEFINE_KEYFIELD( CNPC_Antlion, m_flAlertRadius,	FIELD_FLOAT, "radius" ),
	DEFINE_FIELD( CNPC_Antlion, m_iContext,			FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Antlion, m_vecSavedJump,		FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Antlion, m_vecLastJumpAttempt,FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Antlion, m_flIgnoreSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Antlion, m_vecHeardSound,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CNPC_Antlion, m_bHasHeardSound,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Antlion, m_bAgitatedSound,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Antlion, m_bWingsOpen,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Antlion, m_bFightingToPosition, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Antlion, m_vecFightGoal,		FIELD_VECTOR ),
	DEFINE_KEYFIELD( CNPC_Antlion, m_flEludeDistance,FIELD_FLOAT, "eludedist" ),

	DEFINE_INPUTFUNC( CNPC_Antlion, FIELD_VOID,	"Unburrow", InputUnburrow ),
	DEFINE_INPUTFUNC( CNPC_Antlion, FIELD_VOID,	"Burrow",	InputBurrow ),
	DEFINE_INPUTFUNC( CNPC_Antlion, FIELD_STRING, "FightToPosition", InputFightToPosition ),

	DEFINE_OUTPUT( CNPC_Antlion, m_OnReachFightGoal, "OnReachFightGoal" ),

	// Function Pointers
	DEFINE_ENTITYFUNC( CNPC_Antlion, Touch ),
	DEFINE_USEFUNC( CNPC_Antlion, BurrowUse ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Spawn( void )
{
	Precache();

	SetModel( ANTLION_MODEL );

	SetHullType(HULL_WIDE_HUMAN);
	SetHullSizeNormal();
	SetDefaultEyeOffset();
	
	SetNavType( NAV_GROUND );

	m_NPCState		= NPC_STATE_NONE;
	SetBloodColor( BLOOD_COLOR_YELLOW );
	m_iHealth		= sk_antlion_health.GetFloat();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetMoveType( MOVETYPE_STEP );

	//Only do this if a squadname appears in the entity
	if ( m_SquadName != NULL_STRING )
	{
		CapabilitiesAdd( bits_CAP_SQUAD );
	}

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_INNATE_MELEE_ATTACK1 );

	NPCInit();

	//See if we're supposed to start burrowed
	if ( m_bStartBurrowed )
	{
		m_fEffects		|= EF_NODRAW;
		AddFlag( FL_NOTARGET );
		m_spawnflags |= SF_NPC_GAG;
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_takedamage	= DAMAGE_NO;

		SetState( NPC_STATE_IDLE );
		SetActivity( (Activity) ACT_ANTLION_BURROW_IDLE );
		SetSchedule( SCHED_ANTLION_WAIT_FOR_UNBORROW_TRIGGER );

		SetUse( BurrowUse );
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Precache( void )
{
	engine->PrecacheModel( ANTLION_MODEL );

	for ( int i = 0; i < NUM_ANTLION_GIBS_SMALL; i++ )
	{
		engine->PrecacheModel( m_AntlionGibsSmall[i] );
	}

	for ( i = 0; i < NUM_ANTLION_GIBS_MEDIUM; i++ )
	{
		engine->PrecacheModel( m_AntlionGibsMedium[i] );
	}

	for ( i = 0; i < NUM_ANTLION_GIBS_LARGE; i++ )
	{
		engine->PrecacheModel( m_AntlionGibsLarge[i] );
	}

	BaseClass::Precache();
}

#define	ANTLION_VIEW_FIELD_NARROW	0.85f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::FInViewCone( CBaseEntity *pEntity )
{
	m_flFieldOfView = ( GetEnemy() != NULL ) ? ANTLION_VIEW_FIELD_NARROW : VIEW_FIELD_WIDE;

	return BaseClass::FInViewCone( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecSpot - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::FInViewCone( const Vector &vecSpot )
{
	m_flFieldOfView = ( GetEnemy() != NULL ) ? ANTLION_VIEW_FIELD_NARROW : VIEW_FIELD_WIDE;

	return BaseClass::FInViewCone( vecSpot );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Event_Killed( const CTakeDamageInfo &info )
{
	//Turn off wings
	SetWings( false );
	VacateStrategySlot();
	
	if ( info.GetDamageType() & DMG_CRUSH )
	{
		CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, GetAbsOrigin(), 256, 0.5f, this );
	}

	BaseClass::Event_Killed( info );

	CBaseEntity *pAttacker = info.GetInflictor();
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
	if ( pPlayer && pPlayer->IsInAVehicle() )
	{
		CBaseEntity *pVehicleEnt = pPlayer->GetVehicle()->GetVehicleEnt();
		if ( pVehicleEnt == pAttacker )
		{
			SpawnBlood( GetAbsOrigin(), BloodColor(), info.GetDamage() );
			CGib::SpawnSpecificGibs( this, 12, 300, 400, m_AntlionGibsSmall[random->RandomInt(0,NUM_ANTLION_GIBS_SMALL)], 3 );

			CPASAttenuationFilter filter( this );
			enginesound->EmitSound( filter, entindex(), CHAN_VOICE, "npc/antlion_grub/squashed.wav", 1.0f, ATTN_NORM );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input: 
//==================================================

void CNPC_Antlion::MeleeAttack( float distance, float damage, QAngle &viewPunch, Vector &shove )
{
	if ( GetEnemy() == NULL )
	{
		AssertMsg( 0, "antlion in melee with no enemy!\n" );
		return;
	}

	Vector vecForceDir = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin());

	//FIXME: Always hurt bullseyes for now
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->Classify() == CLASS_BULLSEYE ) )
	{
		CTakeDamageInfo info( this, this, damage, DMG_SLASH );
		CalculateMeleeDamageForce( &info, vecForceDir, GetEnemy()->GetAbsOrigin() );
		GetEnemy()->TakeDamage( info );
		return;
	}

	Vector vecWorldSpaceCenter = WorldSpaceCenter();
	Vector	attackPos = ( GetEnemy()->WorldSpaceCenter() - vecWorldSpaceCenter );
	VectorNormalize( attackPos );

	attackPos = vecWorldSpaceCenter + ( attackPos * distance );

	//CBaseEntity *pHurt = CheckTraceHullAttack( Center(), attackPos, Vector(-8,-8,-8), Vector(8,8,8), damage, DMG_SLASH );
	trace_t tr;
	AI_TraceHull( vecWorldSpaceCenter, attackPos, -Vector(8,8,8), Vector(8,8,8), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );
	
	CBaseEntity *pHurt = tr.m_pEnt;

	if ( pHurt )
	{
		//FIXME: Until the interaction is setup, kill combine soldiers in one hit -- jdw
		if ( FClassnameIs( pHurt, "npc_combine_s" ) )
		{
			CTakeDamageInfo	dmgInfo( this, this, pHurt->m_iHealth+25, DMG_SLASH );
			CalculateMeleeDamageForce( &dmgInfo, vecForceDir, pHurt->GetAbsOrigin() );
			pHurt->TakeDamage( dmgInfo );
			return;
		}

		//Take damage
		CTakeDamageInfo info( this, this, damage, DMG_SLASH );
		CalculateMeleeDamageForce( &info, vecForceDir, pHurt->GetAbsOrigin() );
		pHurt->TakeDamage( info );

		CBasePlayer *pPlayer = ToBasePlayer( pHurt );

		if ( pPlayer != NULL )
		{
			//Kick the player angles
			pPlayer->ViewPunch( viewPunch );

			Vector	dir = pHurt->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize(dir);

			QAngle angles;
			VectorAngles( dir, angles );
			Vector forward, right;
			AngleVectors( angles, &forward, &right, NULL );

			//Push the target back
			pHurt->ApplyAbsVelocityImpulse( - right * shove[1] - forward * shove[0] );
		}

		// Play a random attack hit sound
		EmitSound( "NPC_Antlion.MeleeAttack" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case ANTLION_AE_OPEN_WINGS:
		SetWings( true );
		return;
		break;

	case ANTLION_AE_CLOSE_WINGS:
		SetWings( false );
		return;
		break;

	case ANTLION_AE_VANISH:
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_takedamage	= DAMAGE_NO;
		m_fEffects		|= EF_NODRAW;
		
		Relink();
		SetWings( false );

		return;
		break;

	case ANTLION_AE_BURROW_IN:
		{
		//Burrowing sound
		EmitSound( "NPC_Antlion.BurrowIn" );

		//Shake the screen
		UTIL_ScreenShake( GetAbsOrigin(), 0.5f, 80.0f, 1.0f, 256.0f, SHAKE_START );

		//Throw dust up
		CreateDust();

		if ( m_pHintNode )
		{
			m_pHintNode->Unlock( 2.0f );
		}

		return;
		}
		break;

	case ANTLION_AE_BURROW_OUT:	
		{
		EmitSound( "NPC_Antlion.BurrowOut" );

		//Shake the screen
		UTIL_ScreenShake( GetAbsOrigin(), 0.5f, 80.0f, 1.0f, 256.0f, SHAKE_START );

		//Throw dust up
		//CreateDust();

		m_fEffects &= ~EF_NODRAW;
		RemoveFlag( FL_NOTARGET );

		return;
		}
		break;

	case ANTLION_AE_FOOTSTEP_SOFT:
		{
		EmitSound( "NPC_Antlion.FootstepSoft" );
		return;
		}
		break;
	
	case ANTLION_AE_FOOTSTEP:
		{
		EmitSound( "NPC_Antlion.Footstep" );return;
		}
		break;

	case ANTLION_AE_FOOTSTEP_HEAVY:
		{
			EmitSound( "NPC_Antlion.FootstepHeavy" );
			return;
		}
		break;

	case ANTLION_AE_MELEE_HIT1:
		MeleeAttack( ANTLION_MELEE1_RANGE, sk_antlion_swipe_damage.GetFloat(), QAngle( 20.0f, 0.0f, -12.0f ), Vector( -250.0f, 1.0f, 1.0f ) );
		return;
		break;

	case ANTLION_AE_MELEE_HIT2:
		MeleeAttack( ANTLION_MELEE1_RANGE, sk_antlion_swipe_damage.GetFloat(), QAngle( 20.0f, 0.0f, 0.0f ), Vector( -350.0f, 1.0f, 1.0f ) );
		return;
		break;

	case ANTLION_AE_MELEE1_SOUND:
		{
			EmitSound( "NPC_Antlion.MeleeAttackSingle" );
			return;
		}
		break;

	case ANTLION_AE_MELEE2_SOUND:
		{
			EmitSound( "NPC_Antlion.MeleeAttackDouble" );
			return;
		}
		break;

	case ANTLION_AE_START_JUMP:
		StartJump();
		return;
		break;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::FindBurrowRoute( const Vector &goal )
{
	//TODO: Search for a near burrow point

	//TODO: Search for a target burrow point

	if ( FindBurrow( GetAbsOrigin(), 512, ANTLION_BURROW_IN, false ) )
		return true;

	//if ( FindBurrow( GetEnemy()->GetAbsOrigin(), 512, ANTLION_BURROW_OUT, false ) )
	//{
	//}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
void CNPC_Antlion::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask ) 
	{
	case TASK_ANTLION_FACE_JUMP:
		break;

	case TASK_ANTLION_REACH_FIGHT_GOAL:

		m_OnReachFightGoal.FireOutput( this, this );
		m_bFightingToPosition = false;
		TaskComplete();
		break;

	case TASK_ANTLION_DISMOUNT_NPC:
		{
			CBaseEntity *pGroundEnt = GetGroundEntity();
			
			if( pGroundEnt != NULL )
			{
				// Jump behind the other NPC so I don't block their path.
				Vector vecJumpDir; 

				pGroundEnt->GetVectors( &vecJumpDir, NULL, NULL );

				RemoveFlag( FL_ONGROUND );		
				
				// Bump up
				UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0, 0 , 1 ) );
				
				SetAbsVelocity( vecJumpDir * -200 + Vector( 0, 0, 100 ) );

				SetActivity( (Activity) ACT_ANTLION_FLIP );
			}
			else
			{
				// Dead or gone now
				TaskComplete();
			}
		}

		break;

	case TASK_ANTLION_FACE_BUGBAIT:
			
		//Must have a saved sound
		//FIXME: This isn't assured to be still pointing to the right place, need to protect this
		if ( !m_bHasHeardSound )
		{
			TaskFail( "No remembered bug bait sound to run to!" );
			return;
		}

		GetMotor()->SetIdealYawToTargetAndUpdate( m_vecHeardSound );
		SetTurnActivity();

		break;
	
	case TASK_ANTLION_GET_PATH_TO_BUGBAIT:
		{
			//Must have a saved sound
			//FIXME: This isn't assured to be still pointing to the right place, need to protect this
			if ( !m_bHasHeardSound )
			{
				TaskFail( "No remembered bug bait sound to run to!" );
				return;
			}
			
			Vector soundOrigin = m_vecHeardSound;

			AI_NavGoal_t goal( soundOrigin, (Activity) ACT_ANTLION_RUN_AGITATED, ANTLION_BUGBAIT_NAV_TOLERANCE );
			
			//Try to run directly there
			if ( GetNavigator()->SetGoal( goal, AIN_DISCARD_IF_FAIL ) == false )
			{
				//Try and get as close as possible otherwise
				AI_NavGoal_t nearGoal( soundOrigin, (Activity) ACT_ANTLION_RUN_AGITATED, ANTLION_BUGBAIT_NAV_TOLERANCE, (AIN_NEAREST_NODE|AIN_CLEAR_PREVIOUS_STATE) );

				if ( GetNavigator()->SetGoal( nearGoal ) )
				{
					//FIXME: HACK! The internal pathfinding is setting this without our consent, so override it!
					ClearCondition( COND_TASK_FAILED );
					TaskComplete();
				}
				else
				{
					TaskFail( "Antlion failed to find path to bugbait position\n" );
					return;
				}
			}

			TaskComplete();

			break;
		}

	case TASK_ANTLION_FIND_BURROW_ROUTE:
		
		//Try to find a burrow route
		if ( FindBurrowRoute( GetEnemy()->GetAbsOrigin() ) )
		{
			TaskComplete();
			return;
		}
		
		TaskFail( "TASK_ANTLION_FIND_BURROW_ROUTE: No burrow route available\n" );

		break;

	case TASK_ANTLION_WAIT_FOR_TRIGGER:
		m_flIdleDelay = gpGlobals->curtime + 1.0f;

		break;

	case TASK_ANTLION_JUMP:
		
		if ( CheckLanding() )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_CHECK_FOR_UNBORROW:
		
		if ( ValidBurrowPoint( GetAbsOrigin() ) )
		{
			m_spawnflags &= ~SF_NPC_GAG;
			RemoveSolidFlags( FSOLID_NOT_SOLID );
			Relink();
			
			TaskComplete();
		}

		break;

	case TASK_ANTLION_BURROW_WAIT:
		
		if ( pTask->flTaskData == 1.0f )
		{
			//Set our next burrow time
			m_flBurrowTime = gpGlobals->curtime + random->RandomFloat( 1, 6 );
		}

		break;

	case TASK_ANTLION_FIND_BURROW_IN_POINT:
		
		if ( FindBurrow( GetAbsOrigin(), pTask->flTaskData, ANTLION_BURROW_IN ) == false )
		{
			TaskFail( "TASK_ANTLION_FIND_BURROW_IN_POINT: Unable to find burrow in position\n" );
		}
		else
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_FIND_BURROW_OUT_POINT:
		
		if ( FindBurrow( GetAbsOrigin(), pTask->flTaskData, ANTLION_BURROW_OUT ) == false )
		{
			TaskFail( "TASK_ANTLION_FIND_BURROW_OUT_POINT: Unable to find burrow out position\n" );
		}
		else
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_BURROW:
		Burrow();
		TaskComplete();

		break;

	case TASK_ANTLION_UNBURROW:
		Unburrow();
		TaskComplete();

		break;

	case TASK_ANTLION_VANISH:
		m_fEffects	|= EF_NODRAW;
		AddFlag( FL_NOTARGET );
		m_spawnflags |= SF_NPC_GAG;
		
		// If the task parameter is non-zero, remove us when we vanish
		if ( pTask->flTaskData )
		{
			CBaseEntity *pOwner = GetOwnerEntity();
			
			if( pOwner != NULL )
			{
				pOwner->DeathNotice( this );
				SetOwnerEntity( NULL );
			}

			// NOTE: We can't UTIL_Remove here, because we're in the middle of running our AI, and
			//		 we'll crash later in the bowels of the AI. Remove ourselves next frame.
			SetThink( SUB_Remove );
			SetNextThink( gpGlobals->curtime + 0.1 );
		}

		TaskComplete();

		break;

	case TASK_ANTLION_GET_THUMPER_ESCAPE_PATH:
		{
			Vector vecFleeGoal;
			Vector vecFleeDir;
			Vector vecSoundPos;

			CSound *pSound = GetLoudestSoundOfType( SOUND_THUMPER );

			if ( pSound == NULL  )
			{
				//NOTENOTE: If you're here, there's a disparity between Listen() and GetLoudestSoundOfType() - jdw
				TaskFail( "Unable to find thumper sound!" );
			}

			vecSoundPos = pSound->GetSoundOrigin();

			// Put the sound location on the same plane as the antlion.
			vecSoundPos.z = GetAbsOrigin().z;

			vecFleeDir = GetAbsOrigin() - vecSoundPos;
			VectorNormalize( vecFleeDir );

			vecFleeGoal = vecSoundPos + vecFleeDir * pTask->flTaskData;
			
			AI_NavGoal_t goal( vecFleeGoal, AIN_DEF_ACTIVITY, AIN_DEF_TOLERANCE, AIN_NEAREST_NODE );
			
			if ( GetNavigator()->SetGoal( goal ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail(FAIL_NO_REACHABLE_NODE);
			}
		}
		
		break;

	//FIXME: This is redundant to the thumper implementation

	case TASK_ANTLION_GET_PHYSICS_DANGER_ESCAPE_PATH:
		{
			Vector vecFleeGoal;
			Vector vecFleeDir;
			Vector vecSoundPos;

			CSound *pSound = GetLoudestSoundOfType( SOUND_PHYSICS_DANGER );

			if ( pSound == NULL  )
			{
				//NOTENOTE: If you're here, there's a disparity between Listen() and GetLoudestSoundOfType() - jdw
				TaskFail( "Unable to find physics danger sound!" );
				break;
			}

			vecSoundPos = pSound->GetSoundOrigin();

			// Put the sound location on the same plane as the antlion.
			vecSoundPos.z = GetAbsOrigin().z;

			vecFleeDir = GetAbsOrigin() - vecSoundPos;
			VectorNormalize( vecFleeDir );

			vecFleeGoal = vecSoundPos + ( vecFleeDir * pTask->flTaskData );
			
			AI_NavGoal_t goal( vecFleeGoal, (Activity) ACT_ANTLION_RUN_AGITATED, AIN_DEF_TOLERANCE, AIN_DEF_FLAGS );
			
			if ( GetNavigator()->SetGoal( goal ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail(FAIL_NO_REACHABLE_NODE);
			}
		}
		
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANTLION_FACE_JUMP:
		{
			Vector	jumpDir = m_vecSavedJump;
			VectorNormalize( jumpDir );
			
			QAngle	jumpAngles;
			VectorAngles( jumpDir, jumpAngles );

			GetMotor()->SetIdealYawAndUpdate( jumpAngles[YAW], AI_KEEP_YAW_SPEED );
			SetTurnActivity();
			
			if ( GetMotor()->DeltaIdealYaw() < 2 )
			{
				TaskComplete();
			}
		}

		break;

	case TASK_ANTLION_REACH_FIGHT_GOAL:
		break;

	case TASK_ANTLION_DISMOUNT_NPC:
		
		if ( GetFlags() & FL_ONGROUND )
		{
			CBaseEntity *pGroundEnt = GetGroundEntity();

			if ( ( pGroundEnt != NULL ) && ( ( pGroundEnt->MyNPCPointer() != NULL ) || pGroundEnt->GetSolidFlags() & FSOLID_NOT_STANDABLE ) )
			{
				// Jump behind the other NPC so I don't block their path.
				Vector vecJumpDir; 

				pGroundEnt->GetVectors( &vecJumpDir, NULL, NULL );

				RemoveFlag( FL_ONGROUND );		
				
				// Bump up
				UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0, 0 , 1 ) );
				
				SetAbsVelocity( vecJumpDir * -200 + Vector( 0, 0, 100 ) );

				SetActivity( (Activity) ACT_ANTLION_FLIP );
			}
			else if ( IsActivityFinished() )
			{
				TaskComplete();
			}
		}
		
		break;

	case TASK_ANTLION_FACE_BUGBAIT:
			
		//Must have a saved sound
		//FIXME: This isn't assured to be still pointing to the right place, need to protect this
		if ( !m_bHasHeardSound )
		{
			TaskFail( "No remembered bug bait sound to run to!" );
			return;
		}

		GetMotor()->SetIdealYawToTargetAndUpdate( m_vecHeardSound );

		if ( FacingIdeal() )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_WAIT_FOR_TRIGGER:
		
		if ( ( m_flIdleDelay > gpGlobals->curtime ) || GetEntityName() != NULL_STRING )
			return;

		if ( CheckAlertRadius() )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_JUMP:

		if ( CheckLanding() )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_CHECK_FOR_UNBORROW:
		
		//Must wait for our next check time
		if ( m_flBurrowTime > gpGlobals->curtime )
			return;

		//See if we can pop up
		if ( ValidBurrowPoint( GetAbsOrigin() ) )
		{
			m_spawnflags &= ~SF_NPC_GAG;
			RemoveSolidFlags( FSOLID_NOT_SOLID );

			TaskComplete();
			return;
		}

		//Try again in a couple of seconds
		m_flBurrowTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 1.0f );

		break;

	case TASK_ANTLION_BURROW_WAIT:
		
		//See if enough time has passed
		if ( m_flBurrowTime < gpGlobals->curtime )
		{
			TaskComplete();
		}
		
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if a reasonable jumping distance
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CNPC_Antlion::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const
{
	const float MAX_JUMP_RISE		= ANTLION_JUMP_MAX;
	const float MAX_JUMP_DROP		= ANTLION_JUMP_MAX;
	const float MAX_JUMP_DISTANCE	= ANTLION_JUMP_MAX;

	return BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::ShouldJump( void )
{
	if ( GetEnemy() == NULL )
		return false;

	//Too soon to try to jump
	if ( m_flJumpTime > gpGlobals->curtime )
		return false;

	// only jump if you're on the ground
  	if (!(GetFlags() & FL_ONGROUND))
		return false;

	Vector vecPredictedPos;

	//Get our likely position in two seconds
	UTIL_PredictedPosition( GetEnemy(), 2.0f, &vecPredictedPos );

	// Don't jump if we're already near the target
	if ( ( GetAbsOrigin() - vecPredictedPos ).LengthSqr() < (512*512) )
		return false;

	//Don't retest if the target hasn't moved enough
	//FIXME: Check your own distance from last attempt as well
	if ( ( ( m_vecLastJumpAttempt - vecPredictedPos ).LengthSqr() ) < (128*128) )
	{
		m_flJumpTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 2.0f );		
		return false;
	}

	Vector	targetDir = ( vecPredictedPos - GetAbsOrigin() );

	float flDist = VectorNormalize( targetDir );

	// don't jump at target it it's very close
	if (flDist < ANTLION_JUMP_MIN)
		return false;

	Vector	targetPos = vecPredictedPos + ( targetDir * (GetHullWidth()*4.0f) );

	// Try the jump
	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( NAV_JUMP, GetAbsOrigin(), targetPos, MASK_NPCSOLID, GetNavTargetEntity(), &moveTrace );

	//See if it succeeded
	if ( IsMoveBlocked( moveTrace.fStatus ) )
	{
		if ( g_antlion_debug.GetInt() )
		{
			NDebugOverlay::Box( targetPos, GetHullMins(), GetHullMaxs(), 255, 0, 0, 0, 5 );
			NDebugOverlay::Line( GetAbsOrigin(), targetPos, 255, 0, 0, 0, 5 );
		}

		m_flJumpTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 2.0f );
		return false;
	}

	if ( g_antlion_debug.GetInt() )
	{
		NDebugOverlay::Box( targetPos, GetHullMins(), GetHullMaxs(), 0, 255, 0, 0, 5 );
		NDebugOverlay::Line( GetAbsOrigin(), targetPos, 0, 255, 0, 0, 5 );
	}

	//Save this jump in case the next time fails
	m_vecSavedJump = moveTrace.vJumpVelocity;
	m_vecLastJumpAttempt = targetPos;

	//Okay to jump
	if ( Alone() )
		return true;

	//Only jump if no one else is already in the air
	//if ( OccupyStrategySlot( SQUAD_SLOT_ANTLION_JUMP ) )
		return true;

	//Don't try again for a few seconds
	//m_flJumpTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 2.0f );

	//return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Antlion::SelectSchedule( void )
{
	//Flipped?
	if ( HasCondition( COND_ANTLION_FLIPPED ) )
	{
		ClearCondition( COND_ANTLION_FLIPPED );
		return SCHED_ANTLION_FLIP;
	}

	//Hear a thumper?
	if( HasCondition( COND_HEAR_THUMPER ) )
	{
		PainSound();
		return SCHED_ANTLION_FLEE_THUMPER;
	}

	//Hear a physics danger sound?
	if( HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		PainSound();
		return SCHED_ANTLION_FLEE_PHYSICS_DANGER;
	}

	//On another NPC's head?
	if( HasCondition( COND_ANTLION_ON_NPC ) )
	{
		// You're on an NPC's head. Get off.
		return SCHED_ANTLION_DISMOUNT_NPC;
	}

	//Hear bug bait splattered?
	if ( HasCondition( COND_HEAR_BUGBAIT ) )
	{
		//Play a special sound
		EmitSound( "NPC_Antlion.Distracted" );
		m_flIdleDelay = gpGlobals->curtime + 4.0f;
		
		//If the sound is valid, act upon it
		if ( m_bHasHeardSound )
		{		
			//Mark anything in the area as more interesting
			CBaseEntity *pTarget = NULL;
			CBaseEntity *pNewEnemy = NULL;
			Vector		soundOrg = m_vecHeardSound;

			//Find all entities within that sphere
			while ( ( pTarget = gEntList.FindEntityInSphere( pTarget, soundOrg, bugbait_radius.GetInt() ) ) != NULL )
			{
				CAI_BaseNPC *pNPC = dynamic_cast<CAI_BaseNPC*>((CBaseEntity*)pTarget);

				if ( pNPC == NULL )
					continue;

				//Check to see if the default relationship is hatred, and if so intensify that
				if ( ( IRelationType( pTarget ) == D_HT ) || ( pNPC->Classify() == CLASS_BULLSEYE ) )
				{
					AddEntityRelationship( pTarget, D_HT, 99 );			
					
					//Try to spread out the enemy distribution
					if ( ( pNewEnemy == NULL ) || ( random->RandomInt( 0, 1 ) ) )
					{
						pNewEnemy = pTarget;
						continue;
					}
				}
			}
			
			//Setup our ignore info
			SetEnemy( pNewEnemy );
			
			return SCHED_ANTLION_CHASE_BUGBAIT;
		}
	}

	// If we're flagged to burrow away when eluded, do so
	if ( ( m_spawnflags & SF_ANTLION_BURROW_ON_ELUDED ) && HasCondition( COND_ENEMY_UNREACHABLE ) )
		return SCHED_ANTLION_BURROW_IN;

	//Otherwise do basic state schedule selection
	switch ( m_NPCState )
	{	
	case NPC_STATE_COMBAT:
		{
			//Try to jump
			if ( HasCondition( COND_ANTLION_CAN_JUMP ) )
				return SCHED_ANTLION_JUMP;
		}
		break;

	default:
		
		if ( m_bFightingToPosition )
		{
			m_vSavePosition	= m_vecFightGoal;
			return SCHED_ANTLION_RUN_TO_FIGHT_GOAL;
		}
		break;
	}

	int retSched = BaseClass::SelectSchedule();
	
	//Must be idling
	if ( ( retSched == SCHED_IDLE_STAND ) || ( retSched == SCHED_ALERT_STAND ) )
	{
		//Must be on a bugbait controlled map
		if ( GlobalEntity_GetState( "antlion_allied" ) == GLOBAL_ON )
		{
			//Player must be witihin range
			if ( PlayerInRange( GetAbsOrigin(), 512 ) )
				return SCHED_ANTLION_BUGBAIT_IDLE_STAND;
		}
	}

	return retSched;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo newInfo = info;

	Vector	vecShoveDir = vecDir;
	vecShoveDir.z = 0.0f;

	//Are we already flipped?
	if ( GetActivity() == ACT_ANTLION_FLIP )
	{
		//If we were hit by physics damage, move with it
		if ( newInfo.GetDamageType() & DMG_CRUSH )
		{
			PainSound();
			ApplyAbsVelocityImpulse( ( vecShoveDir * random->RandomInt( 500.0f, 1000.0f ) ) + Vector(0,0,64.0f) );
			RemoveFlag( FL_ONGROUND );

			newInfo.SetDamage( 5.0f );
		}
		else
		{
			//More vulnerable when flipped
			newInfo.ScaleDamage( 4.0f );
		}
	}
	else if ( newInfo.GetDamageType() & (DMG_BLAST|DMG_CRUSH) )
	{
		//Grenades make us fuh-lip!
		if ( newInfo.GetDamage() >= 15.0f )
		{
			//Don't flip off the deck
			if ( GetFlags() & FL_ONGROUND )
			{
				PainSound();

				SetCondition( COND_ANTLION_FLIPPED );

				//Get tossed!
				ApplyAbsVelocityImpulse( ( vecShoveDir * random->RandomInt( 150.0f, 450.0f ) ) + Vector(0,0,64.0f) );
				RemoveFlag( FL_ONGROUND );
			}
		}
	}

	// Send down the custom antlion impact
	// FIXME: This wouldn't be needed if the antlion's model had its material set to CHAR_TEX_ANTLION
	CEffectData data;
	data.m_vOrigin = ptr->endpos;
	data.m_vStart = ptr->startpos;
	data.m_nMaterial = CHAR_TEX_ANTLION;
	data.m_nDamageType = newInfo.GetDamageType();
	data.m_nHitBox = ptr->hitbox;
	data.m_nEntIndex = ptr->m_pEnt->entindex();
	DispatchEffect( "Impact", data );

	
	if ( ( random->RandomInt( 0, 1 ) == 0 ) && ( newInfo.GetDamage() > 2.0f ) )
	{
		CGib::SpawnSpecificGibs( this, 1, 256, 64, m_AntlionGibsSmall[random->RandomInt(0,NUM_ANTLION_GIBS_SMALL)], 3 );
	}

	BaseClass::TraceAttack( newInfo, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::IdleSound( void )
{
	EmitSound( "NPC_Antlion.Idle" );
	m_flIdleDelay = gpGlobals->curtime + 4.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::PainSound( void )
{
	EmitSound( "NPC_Antlion.Pain" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_Antlion::MaxYawSpeed( void )
{
	switch ( GetActivity() )
	{
	case ACT_IDLE:		
		return 32.0f;
		break;
	
	case ACT_WALK:
		return 16.0f;
		break;
	
	default:
	case ACT_RUN:
		return 32.0f;
		break;
	}

	return 32.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::ShouldPlayIdleSound( void )
{
	//Only do idles in the right states
	if ( ( m_NPCState != NPC_STATE_IDLE && m_NPCState != NPC_STATE_ALERT ) )
		return false;

	//Gagged monsters don't talk
	if ( m_spawnflags & SF_NPC_GAG )
		return false;

	//Don't cut off another sound or play again too soon
	if ( m_flIdleDelay > gpGlobals->curtime )
		return false;

	//Randomize it a bit
	if ( random->RandomInt( 0, 20 ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Antlion::MeleeAttack1Conditions( float flDot, float flDist )
{
#if 0 //NOTENOTE: Use predicted position melee attacks

	float		flPrDist, flPrDot;
	Vector		vecPrPos;
	Vector2D	vec2DPrDir;

	//Get our likely position in one half second
	UTIL_PredictedPosition( GetEnemy(), 0.5f, &vecPrPos );

	//Get the predicted distance and direction
	flPrDist	= ( vecPrPos - GetAbsOrigin() ).Length();
	vec2DPrDir	= ( vecPrPos - GetAbsOrigin() ).AsVector2D();

	Vector vecBodyDir = BodyDirection2D();

	Vector2D vec2DBodyDir = vecBodyDir.AsVector2D();
	
	flPrDot	= DotProduct2D ( vec2DPrDir, vec2DBodyDir );

	if ( flPrDist > ANTLION_MELEE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;

	if ( flPrDot < 0.5f )
		return COND_NOT_FACING_ATTACK;

#else

	if ( flDist > ANTLION_MELEE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;

	if ( flDot < 0.5f )
		return COND_NOT_FACING_ATTACK;

#endif

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : interactionType - 
//			*data - 
//			*sender - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::HandleInteraction( int interactionType, void *data, CBaseCombatCharacter *sender )
{
	//Check for a target found while burrowed
	if ( interactionType == g_interactionAntlionFoundTarget )
	{
		CBaseEntity	*pOther = (CBaseEntity *) data;
		
		//Randomly delay
		m_flBurrowTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 1.0f );
		BurrowUse( pOther, pOther, USE_ON, 0.0f );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::Alone( void )
{
	if ( m_pSquad == NULL )
		return true;

	if ( m_pSquad->NumMembers() <= 1 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::StartJump( void )
{
	//Must be jumping at an enemy
	if ( GetEnemy() == NULL )
		return;

	//Don't jump if we're not on the ground
	if ( ( GetFlags() & FL_ONGROUND ) == false )
		return;

	//Take us off the ground
	RemoveFlag( FL_ONGROUND );
	SetAbsVelocity( m_vecSavedJump );

	//Setup our jump time so that we don't try it again too soon
	m_flJumpTime = gpGlobals->curtime + random->RandomInt( 2, 6 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : sHint - 
//			nNodeNum - 
// Output : bool CAI_BaseNPC::FValidateHintType
//-----------------------------------------------------------------------------
bool CNPC_Antlion::FValidateHintType(CAI_Hint *pHint)
{
	switch ( m_iContext )
	{
	case ANTLION_BURROW_OUT:
		{			
			//See if this is a valid point
			Vector vHintPos;
			pHint->GetPosition(this,&vHintPos);

			if ( ValidBurrowPoint( vHintPos ) == false )
				return false;
		}
		break;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::ClearBurrowPoint( const Vector &origin )
{
	CBaseEntity *pEntity = NULL;
	float		flDist;
	Vector		vecSpot, vecCenter, vecForce;

	//Cause a ruckus
	UTIL_ScreenShake( origin, 1.0f, 80.0f, 1.0f, 256.0f, SHAKE_START );

	//Iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( origin, 128 ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		if ( pEntity->m_takedamage != DAMAGE_NO && pEntity->Classify() != CLASS_PLAYER && pEntity->VPhysicsGetObject() )
		{
			vecSpot	 = pEntity->BodyTarget( origin );
			vecForce = ( vecSpot - origin ) + Vector( 0, 0, 16 );

			// decrease damage for an ent that's farther from the bomb.
			flDist = VectorNormalize( vecForce );

			//float mass = pEntity->VPhysicsGetObject()->GetMass();
			RandomPointInBounds( vec3_origin, Vector( 1.0f, 1.0f, 1.0f ), &vecCenter );

			if ( flDist <= 128.0f )
			{
				pEntity->VPhysicsGetObject()->Wake();
				pEntity->VPhysicsGetObject()->ApplyForceOffset( vecForce * 250.0f, vecCenter );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Determine whether a point is valid or not for burrowing up into
// Input  : &point - point to test for validity
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::ValidBurrowPoint( const Vector &point )
{
	trace_t	tr;

	AI_TraceHull( point, point+Vector(0,0,1), GetHullMins(), GetHullMaxs(), 
		MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	//See if we were able to get there
	if ( ( tr.startsolid ) || ( tr.allsolid ) || ( tr.fraction < 1.0f ) )
	{
		CBaseEntity *pEntity = tr.m_pEnt;

		//If it's a physics object, attempt to knock is away
		if ( ( pEntity ) && ( pEntity->VPhysicsGetObject() ) )
		{
			ClearBurrowPoint( point );
		}

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a burrow point for the antlion
// Input  : distance - radius to search for burrow spot in
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::FindBurrow( const Vector &origin, float distance, int type, bool excludeNear )
{
	//Burrowing in?
	if ( type == ANTLION_BURROW_IN )
	{
		//Attempt to find a burrowing point
		CHintCriteria	hCriteria;

		hCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );
		hCriteria.SetFlag( bits_HINT_NODE_NEAREST );

		hCriteria.AddIncludePosition( origin, distance );
		
		if ( excludeNear )
		{
			hCriteria.AddExcludePosition( origin, 128 );
		}

		CAI_Hint *pHint = CAI_Hint::FindHint( this, &hCriteria );

		if ( pHint == NULL )
			return false;

		//Free up the node for use
		if ( m_pHintNode )
		{
			m_pHintNode->Unlock(0);
		}

		m_pHintNode = pHint;

		//Lock the node
		m_pHintNode->Lock(this);

		//Setup our path and attempt to run there
		Vector vHintPos;
		m_pHintNode->GetPosition( this, &vHintPos );

		AI_NavGoal_t goal( vHintPos, ACT_RUN );

		return GetNavigator()->SetGoal( goal );
	}

	//Burrow out
	m_iContext = ANTLION_BURROW_OUT;

	CHintCriteria	hCriteria;

	hCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );
	hCriteria.SetFlag( bits_HINT_NODE_NEAREST );

	if ( GetEnemy() != NULL )
	{
		hCriteria.AddIncludePosition( GetEnemy()->GetAbsOrigin(), distance );
	}

	//Attempt to find an open burrow point
	CAI_Hint *pHint = CAI_Hint::FindHint( this, &hCriteria );

	m_iContext = -1;

	if ( pHint == NULL )
		return false;

	//Free up the node for use
	if (m_pHintNode)
	{
		m_pHintNode->Unlock(0);
	}

	m_pHintNode = pHint;
	m_pHintNode->Lock(this);

	Vector burrowPoint;
	m_pHintNode->GetPosition(this,&burrowPoint);

	UTIL_SetOrigin( this, burrowPoint );

	//Burrowing out
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:	Cause the antlion to unborrow
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------

void CNPC_Antlion::BurrowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//Don't allow us to do this again
	SetUse( NULL );
	
	//Allow idle sounds again
	m_spawnflags &= ~SF_NPC_GAG;

	//If the player activated this, then take them as an enemy
	if ( ( pCaller != NULL ) && ( pCaller->IsPlayer() ) )
	{
		SetEnemy( pActivator );
	}

	//Start trying to surface
	SetSchedule( SCHED_ANTLION_WAIT_UNBORROW );
}

//-----------------------------------------------------------------------------
// Purpose: Monitor the antlion's jump to play the proper landing sequence
//-----------------------------------------------------------------------------
bool CNPC_Antlion::CheckLanding( void )
{
	trace_t	tr;
	Vector	testPos;

	//Amount of time to predict forward
	const float	timeStep = 0.1f;

	//Roughly looks one second into the future
	testPos = GetAbsOrigin() + ( GetAbsVelocity() * timeStep );
	testPos[2] -= ( 0.5 * sv_gravity.GetFloat() * GetGravity() * timeStep * timeStep);

	if ( g_antlion_debug.GetInt() == 1 )
	{
		NDebugOverlay::Line( GetAbsOrigin(), testPos, 255, 0, 0, 0, 0.5f );
		NDebugOverlay::Cross3D( m_vecSavedJump, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, true, 0.5f );
	}
	
	AI_TraceLine( GetAbsOrigin(), testPos, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	//See if we're about to contact, or have already contacted the ground
	if ( ( tr.fraction != 1.0f ) || ( GetFlags() & FL_ONGROUND ) )
	{
		int	sequence = SelectWeightedSequence( ACT_LAND );

		if ( GetSequence() != sequence )
		{
			SetWings( false );
			VacateStrategySlot();
			SetActivity( (Activity) ACT_LAND );

			CreateDust( false );
			EmitSound( "NPC_Antlion.Land" );

			SetAbsVelocity( GetAbsVelocity() * 0.33f );
			return false;
		}

		return IsActivityFinished();
	}

	return false;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::QuerySeeEntity( CBaseEntity *pEntity )
{
	//If we're under the ground, don't look at enemies
	if ( m_fEffects & EF_NODRAW )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Turns the antlion's wings on or off
// Input  : state - on or off
//-----------------------------------------------------------------------------
void CNPC_Antlion::SetWings( bool state )
{
	if (m_bWingsOpen == state)
	{
		return;
	}

	m_bWingsOpen = state;

	if ( m_bWingsOpen )
	{
		CPASAttenuationFilter filter( this, "NPC_Antlion.WingsOpen" );
		filter.MakeReliable();

		EmitSound( filter, entindex(), "NPC_Antlion.WingsOpen" );
		SetBodygroup( 1, 1 );
	}
	else
	{
		StopSound( "NPC_Antlion.WingsOpen" );
		SetBodygroup( 1, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check in a radius around the burrowed antlion for a valid target
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::CheckAlertRadius( void )
{
	m_flIdleDelay = gpGlobals->curtime + random->RandomFloat( 0.2f, 1.0f );

	CBaseEntity *pEntity = NULL;

	for ( CEntitySphereQuery sphere( GetAbsOrigin(), m_flAlertRadius ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		//Must be mad at it
		if ( IRelationType( pEntity ) != D_HT )
			continue;

		//Cannot be in NOTARGET
		if ( pEntity->GetFlags() & FL_NOTARGET )
			continue;

		//Act as though we're triggered
		BurrowUse( pEntity, pEntity, USE_ON, 0 );

		//If we're in a squad, tell them
		if ( Alone() == false )
		{
			m_pSquad->SquadNewEnemy( pEntity );
			m_pSquad->BroadcastInteraction( g_interactionAntlionFoundTarget, (void *) pEntity, this );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Burrow( void )
{
	SetWings( false );

	//Stop us from taking damage and being solid
	m_spawnflags |= SF_NPC_GAG;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Unburrow( void )
{
	SetWings( false );

	//Become solid again and visible
	m_spawnflags &= ~SF_NPC_GAG;
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage	= DAMAGE_YES;

	RemoveFlag( FL_ONGROUND );

	//If we have an enemy, come out facing them
	if ( GetEnemy() )
	{
		Vector	dir = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
		VectorNormalize(dir);

		QAngle angles = GetLocalAngles();
		angles[ YAW ] = UTIL_VecToYaw( dir );
		SetLocalAngles( angles );
	}

	Relink();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputUnburrow( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	SetSchedule( SCHED_ANTLION_WAIT_UNBORROW );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputBurrow( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	SetSchedule( SCHED_ANTLION_BURROW_IN );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecDir - 
//			flDistance - 
//-----------------------------------------------------------------------------
bool CNPC_Antlion::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
#if 0 //NOTENOTE: This looks a little weird still...
	if ( HasCondition( COND_SEE_ENEMY ) )
	{
		float idealYaw = UTIL_VecToYaw( GetNavigator()->GetGoalPos() - Center() );
		GetMotor()->SetIdealYawAndUpdate( idealYaw );
		return;
	}
#endif
		
	return BaseClass::OverrideMoveFacing( move, flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::CreateDust( bool placeDecal )
{
	trace_t	tr;
	AI_TraceLine( GetAbsOrigin()+Vector(0,0,1), GetAbsOrigin()-Vector(0,0,64), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
	{
		surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );

		if ( ( (char) pdata->gameMaterial == 'C' ) || ( (char) pdata->gameMaterial == 'D' ) )
		{
			UTIL_CreateAntlionDust( tr.endpos + Vector(0,0,24), GetLocalAngles() );
			
			if ( placeDecal )
			{
				UTIL_DecalTrace( &tr, "Antion.Unburrow" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSound - 
//-----------------------------------------------------------------------------
bool CNPC_Antlion::QueryHearSound( CSound *pSound )
{
	if ( !BaseClass::QueryHearSound( pSound ) )
		return false;
		
	if ( pSound->m_iType == SOUND_BUGBAIT )
	{
		//Must be more recent than the current
		if ( pSound->SoundExpirationTime() <= m_flIgnoreSoundTime )
			return false;

		//If we can hear it, store it
		m_bHasHeardSound = (pSound != NULL);
		if ( m_bHasHeardSound )
		{
			m_vecHeardSound = pSound->GetSoundOrigin();
			m_flIgnoreSoundTime	= pSound->SoundExpirationTime();
		}
	}

	//Do the normal behavior at this point
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_Antlion::BuildScheduleTestBits( void )
{
	//Don't allow any modifications when scripted
	if ( m_NPCState == NPC_STATE_SCRIPT )
		return;

	//Make sure we interrupt a run schedule if we can jump
	if ( IsCurSchedule(SCHED_CHASE_ENEMY) )
	{
		SetCustomInterruptCondition( COND_ANTLION_CAN_JUMP );
		SetCustomInterruptCondition( COND_ENEMY_UNREACHABLE );
	}

	//Interrupt any schedule unless already fleeing, burrowing, burrowed, or unburrowing.
	if( !IsCurSchedule(SCHED_ANTLION_FLEE_THUMPER)				&& 		
		!IsCurSchedule(SCHED_ANTLION_FLEE_PHYSICS_DANGER)		&& 		
		!IsCurSchedule(SCHED_ANTLION_BURROW_IN)					&& 		
		!IsCurSchedule(SCHED_ANTLION_WAIT_UNBORROW)				&& 		
		!IsCurSchedule(SCHED_ANTLION_BURROW_OUT)				&&
		!IsCurSchedule(SCHED_ANTLION_BURROW_WAIT)				&&
		!IsCurSchedule(SCHED_ANTLION_WAIT_FOR_UNBORROW_TRIGGER)	&&
		!IsCurSchedule(SCHED_ANTLION_WAIT_FOR_CLEAR_UNBORROW)	&&
		!IsCurSchedule(SCHED_ANTLION_WAIT_UNBORROW)				&&
		!IsCurSchedule(SCHED_ANTLION_JUMP)						&&
		!IsCurSchedule(SCHED_ANTLION_FLIP)						&&
		!IsCurSchedule(SCHED_ANTLION_DISMOUNT_NPC)				&& 
		( GetFlags() & FL_ONGROUND ) )
	{
		// Only do these if not jumping as well
		if ( !IsCurSchedule(SCHED_ANTLION_JUMP)	)
		{
			SetCustomInterruptCondition( COND_HEAR_PHYSICS_DANGER );
			SetCustomInterruptCondition( COND_HEAR_THUMPER );
			SetCustomInterruptCondition( COND_HEAR_BUGBAIT );
			SetCustomInterruptCondition( COND_ANTLION_FLIPPED );
		}

		SetCustomInterruptCondition( COND_ANTLION_ON_NPC );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::IsValidEnemy( CBaseEntity *pEnemy )
{
	//See if antlions are friendly to the player in this map
	if ( ( GlobalEntity_GetState( "antlion_allied" ) == GLOBAL_ON ) && pEnemy->IsPlayer() )
		return false;

	//If we're chasing bugbait, close to within a certain radius before picking up enemies
	if ( IsCurSchedule( SCHED_ANTLION_CHASE_BUGBAIT ) && ( GetNavigator() != NULL ) )
	{
		//If the enemy is without the target radius, then don't allow them
		if ( ( GetNavigator()->IsGoalActive() ) && ( GetNavigator()->GetGoalPos() - pEnemy->GetAbsOrigin() ).Length() > bugbait_radius.GetFloat() )
			return false;
	}

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::GatherConditions( void )
{
	BaseClass::GatherConditions();

	// See if I've landed on an NPC!
	CBaseEntity *pGroundEnt = GetGroundEntity();
	
	if ( ( ( pGroundEnt != NULL ) && ( pGroundEnt->GetSolidFlags() & FSOLID_NOT_STANDABLE ) ) && ( GetFlags() & FL_ONGROUND ) )
	{
		SetCondition( COND_ANTLION_ON_NPC );
	}
	else
	{
		ClearCondition( COND_ANTLION_ON_NPC );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::PrescheduleThink( void )
{
	Activity eActivity = GetActivity();

	//See if we need to play their agitated sound
	if ( ( eActivity == ACT_ANTLION_RUN_AGITATED ) && ( m_bAgitatedSound == false ) )
	{
		//Start sound
		CPASAttenuationFilter filter( this, "NPC_Antlion.LoopingAgitated" );
		filter.MakeReliable();

		EmitSound( filter, entindex(), "NPC_Antlion.LoopingAgitated" );
		m_bAgitatedSound = true;
	}
	else if ( ( eActivity != ACT_ANTLION_RUN_AGITATED ) && ( m_bAgitatedSound == true ) )
	{
		//Stop sound
		StopSound( "NPC_Antlion.LoopingAgitated" );
		m_bAgitatedSound = false;
	}

	//See if our wings got interrupted from being turned off
	if (    ( m_bWingsOpen ) &&
			( eActivity != ACT_ANTLION_JUMP_START ) && 
			( eActivity != ACT_JUMP ) && 
			( eActivity != ACT_GLIDE ) && 
			( eActivity != ACT_LAND ) && 
			( eActivity != ACT_ANTLION_DISTRACT ) && 
			( eActivity != ACT_JUMP ) &&
			( eActivity != ACT_GLIDE ) &&
			( eActivity != ACT_LAND ) )
	{
		SetWings( false );
	}

	if ( ShouldJump() )
	{
		SetCondition( COND_ANTLION_CAN_JUMP );
	}
	else
	{
		ClearCondition( COND_ANTLION_CAN_JUMP );
	}

	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDamage - 
//			bitsDamageType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::IsLightDamage( float flDamage, int bitsDamageType )
{
	if ( ( random->RandomInt( 0, 1 ) ) && ( flDamage > 3 ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputFightToPosition( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.String(), this );

	if ( pEntity != NULL )
	{
		m_bFightingToPosition = true;
		m_vecFightGoal = pEntity->GetAbsOrigin();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	// Do the base class
	BaseClass::GatherEnemyConditions( pEnemy );

	// Only continue if we burrow when eluded
	if ( ( m_spawnflags & SF_ANTLION_BURROW_ON_ELUDED ) == false )
		return;

	// If we're not already too far away, check again
	//TODO: Check to make sure we don't already have a condition set that removes the need for this
	if ( HasCondition( COND_ENEMY_UNREACHABLE ) == false )
	{
		Vector	predPosition;
		UTIL_PredictedPosition( GetEnemy(), 1.0f, &predPosition );

		Vector	predDir = ( predPosition - GetAbsOrigin() );
		float	predLength = VectorNormalize( predDir );

		// See if we'll be outside our effective target range
		if ( predLength > m_flEludeDistance )
		{
			Vector	predVelDir = ( predPosition - GetEnemy()->GetAbsOrigin() );
			float	predSpeed  = VectorNormalize( predVelDir );

			// See if the enemy is moving mostly away from us
			if ( ( predSpeed > 512.0f ) && ( DotProduct( predVelDir, predDir ) > 0.0f ) )
			{
				// Mark the enemy as eluded and burrow away
				ClearEnemyMemory();
				SetEnemy( NULL );
				m_IdealNPCState = NPC_STATE_ALERT;
				SetCondition( COND_ENEMY_UNREACHABLE );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::ShouldGib( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & DMG_NEVERGIB )
		return false;

	if ( info.GetDamageType() & DMG_ALWAYSGIB )
		return true;

	if ( info.GetDamageType() & DMG_BLAST )
		return true;

	if ( m_iHealth < -20 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::CorpseGib( const CTakeDamageInfo &info )
{
	CEffectData	data;
	
	data.m_vOrigin = WorldSpaceCenter();
	data.m_vNormal = data.m_vOrigin - info.GetDamagePosition();
	VectorNormalize( data.m_vNormal );
	
	data.m_flScale = RemapVal( m_iHealth, 0, -500, 1, 3 );
	data.m_flScale = clamp( data.m_flScale, 1, 3 );

	DispatchEffect( "AntlionGib", data );

	CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, GetAbsOrigin(), 256, 0.5f, this );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Touch( CBaseEntity *pOther )
{
	//See if the touching entity is a vehicle
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( 1 ) );
	
	if ( pPlayer && pPlayer->IsInAVehicle() )
	{
		IServerVehicle	*pVehicle = pPlayer->GetVehicle();
		CBaseEntity *pVehicleEnt = pVehicle->GetVehicleEnt();

		if ( pVehicleEnt == pOther )
		{
			CPropVehicleDriveable	*pDrivableVehicle = dynamic_cast<CPropVehicleDriveable *>( pVehicleEnt );

			if ( pDrivableVehicle != NULL )
			{
				//Get tossed!
				Vector	vecShoveDir = pOther->GetAbsVelocity();
				Vector	vecTargetDir = GetAbsOrigin() - pOther->GetAbsOrigin();
				
				VectorNormalize( vecShoveDir );
				VectorNormalize( vecTargetDir );

				if ( ( pDrivableVehicle->m_nRPM > 100 ) && DotProduct( vecShoveDir, vecTargetDir ) <= 0 )
				{
					// We're being shoved
					PainSound();

					SetCondition( COND_ANTLION_FLIPPED );

					vecTargetDir[2] = 0.0f;

					ApplyAbsVelocityImpulse( ( vecTargetDir * 250.0f ) + Vector(0,0,64.0f) );
					RemoveFlag( FL_ONGROUND );

					CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, GetAbsOrigin(), 256, 0.5f, this );
				}
			}
		}
	}

	BaseClass::Touch( pOther );
}


//
// Antlion spawner
//

//FIXME: This class is now obsolete with the introduction of the CAntlionTemplateMaker! (jdw)

class CAntlionMaker : public CNPCMaker
{
public:
	
	DECLARE_CLASS( CAntlionMaker, CNPCMaker );

	CAntlionMaker( void)
	{
		m_iszNPCClassname = MAKE_STRING( "npc_antlion" );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Hook to allow antlions to pack specific data into their known class fields
	// Input  : pChild - pointer to the spawned entity to work on
	//-----------------------------------------------------------------------------
	virtual	void ChildPreSpawn( CAI_BaseNPC *pChild )
	{
		CNPC_Antlion *pAntlion = dynamic_cast<CNPC_Antlion*>((CAI_BaseNPC*)pChild);

		ASSERT( pAntlion != NULL );

		if ( pAntlion == NULL )
			return;

		pAntlion->m_bStartBurrowed	= m_bStartBurrowed;

		// Spawn near the player if asked to.
		if ( m_strSpawnGroup != NULL_STRING )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);

			if ( pPlayer )
			{
				CHintCriteria	hintCriteria;

				hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );
				hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );

				hintCriteria.SetGroup( m_strSpawnGroup );

				// Spawn the dudes in the player's way.
				hintCriteria.AddIncludePosition( pPlayer->GetAbsOrigin(), 512 );	

				CAI_Hint *pSearchHint = CAI_Hint::FindHint( pChild, &hintCriteria );

				if ( pSearchHint )
				{
					Vector vecOrigin;

					//pSearchHint->Lock( pChild );
					//pSearchHint->Unlock( 1 );
					pSearchHint->GetPosition( pChild, &vecOrigin );
					UTIL_SetOrigin( pChild, vecOrigin );
				}
			}
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: Hook to allow antlions to pack specific data into their known class fields
	// Input  : *pChild - pointer to the spawned entity to work on
	//-----------------------------------------------------------------------------
	virtual	void ChildPostSpawn( CAI_BaseNPC *pChild )
	{
		CNPC_Antlion *pAntlion = dynamic_cast<CNPC_Antlion*>((CAI_BaseNPC*)pChild);

		ASSERT( pAntlion != NULL );

		if ( pAntlion == NULL )
			return;

		if ( m_bStartBurrowed )
		{
			pAntlion->BurrowUse( this, this, USE_ON, 0.0f );
		}

		if ( m_spawnflags & SF_NPC_LONG_RANGE )
		{
			pAntlion->m_flDistTooFar = 1e9f;
			pAntlion->SetDistLook( 6000.0 );
		}

		// Face the child to the player if asked to.
		if ( m_spawnflags & SF_ANTLIONMAKER_SPAWN_FACING_PLAYER )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);

			if( pPlayer )
			{
				QAngle Angles;
				Vector vecDir;

				VectorSubtract( pPlayer->GetAbsOrigin(), pChild->GetAbsOrigin(), vecDir );

				VectorAngles( vecDir, Angles );

				Angles.x = 0;
				Angles.z = 0;

				pChild->SetLocalAngles( Angles );
			}
		}
	}

	DECLARE_DATADESC();

	bool		m_bStartBurrowed;
	string_t	m_strSpawnGroup;	// if present, spawn children on the nearest node of this group (to the player)
};

LINK_ENTITY_TO_CLASS( npc_antlionmaker, CAntlionMaker );

//DT Definition
BEGIN_DATADESC( CAntlionMaker )

	DEFINE_KEYFIELD( CAntlionMaker, m_bStartBurrowed,	FIELD_BOOLEAN,	"startburrowed" ),
	DEFINE_KEYFIELD( CAntlionMaker, m_strSpawnGroup,	FIELD_STRING,	"spawngroup" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Antlion template maker
//-----------------------------------------------------------------------------

class CAntlionTemplateMaker : public CTemplateNPCMaker
{
	public:

	DECLARE_CLASS( CAntlionTemplateMaker, CTemplateNPCMaker );

	void	MakeNPC( void );
	void	ChildPostSpawn( CAI_BaseNPC *pChild );

protected:

	bool	FindHintSpawnPosition( const Vector &origin, float radius, string_t hintGroupName, Vector *ret );

	string_t	m_strSpawnGroup;	// if present, spawn children on the nearest node of this group (to the player)
	string_t	m_strSpawnTarget;	// name of target to spawn near
	float		m_flSpawnRadius;	// radius around target to attempt to spawn in

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( npc_antlion_template_maker, CAntlionTemplateMaker );

//DT Definition
BEGIN_DATADESC( CAntlionTemplateMaker )

	DEFINE_KEYFIELD( CAntlionTemplateMaker,	m_strSpawnGroup,	FIELD_STRING,	"spawngroup" ),
	DEFINE_KEYFIELD( CAntlionTemplateMaker,	m_strSpawnTarget,	FIELD_STRING,	"spawntarget" ),
	DEFINE_KEYFIELD( CAntlionTemplateMaker,	m_flSpawnRadius,	FIELD_FLOAT,	"spawnradius" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::MakeNPC( void )
{
	// If we're not restricting to hint groups, spawn as normal
	if ( m_strSpawnGroup == NULL_STRING )
	{
		BaseClass::MakeNPC();
		return;
	}

	if ( CanMakeNPC() == false )
		return;

	// Set our defaults
	Vector	targetOrigin = GetAbsOrigin();
	QAngle	targetAngles = GetAbsAngles();

	// Look for our target entity
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_strSpawnTarget, this );

	// Take its position if it exists
	if ( pTarget != NULL )
	{
		UTIL_PredictedPosition( pTarget, 1.5f, &targetOrigin );
	}

	Vector	spawnOrigin;

	// If we can't find a spawn position, then we can't spawn this time
	if ( FindHintSpawnPosition( targetOrigin, m_flSpawnRadius, m_strSpawnGroup, &spawnOrigin ) == false )
		return;

	// Point at the current position of the enemy
	if ( pTarget != NULL )
	{
		targetOrigin = pTarget->GetAbsOrigin();
	}

	// Create the entity via a template
	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData), NULL );
	
	if ( pEntity != NULL )
	{
		pent = (CAI_BaseNPC *) pEntity;
	}

	if ( pent == NULL )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	m_OnSpawnNPC.FireOutput( this, this );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	ChildPreSpawn( pent );

	DispatchSpawn( pent );
	
	// Put us at the desired location
	pent->SetLocalOrigin( spawnOrigin );
	
	// Face our spawning direction
	Vector	spawnDir = ( targetOrigin - spawnOrigin );
	VectorNormalize( spawnDir );

	QAngle	spawnAngles;
	VectorAngles( spawnDir, spawnAngles );
	spawnAngles[PITCH] = 0.0f;
	spawnAngles[ROLL] = 0.0f;

	pent->SetLocalAngles( spawnAngles );
	
	pent->SetOwnerEntity( this );
	pent->Activate();

	ChildPostSpawn( pent );

	m_nLiveChildren++; // count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find a hint position to spawn the new antlion at
// Input  : &origin - search origin
//			radius - search radius
//			hintGroupName - search hint group name
//			*retOrigin - found origin (if any)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAntlionTemplateMaker::FindHintSpawnPosition( const Vector &origin, float radius, string_t hintGroupName, Vector *retOrigin )
{
	CHintCriteria	hintCriteria;

	hintCriteria.SetGroup( hintGroupName );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );
	hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );
	hintCriteria.AddIncludePosition( origin, radius );

	CAI_Hint *pHint = CAI_Hint::FindHint( origin, &hintCriteria );

	if ( pHint != NULL )
	{
		pHint->GetPosition( HULL_WIDE_HUMAN, retOrigin );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Makes the antlion immediatley unburrow if it started burrowed
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::ChildPostSpawn( CAI_BaseNPC *pChild )
{
	CNPC_Antlion *pAntlion = dynamic_cast<CNPC_Antlion*>((CAI_BaseNPC*)pChild);

	ASSERT( pAntlion != NULL );

	if ( pAntlion == NULL )
		return;

	// Unburrow the spawned antlion immediately
	if ( pAntlion->m_bStartBurrowed )
	{
		pAntlion->BurrowUse( this, this, USE_ON, 0.0f );
	}

	BaseClass::ChildPostSpawn( pChild );
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_antlion, CNPC_Antlion )

	//Register our interactions
	DECLARE_INTERACTION( g_interactionAntlionFoundTarget )

	//Conditions
	DECLARE_CONDITION( COND_ANTLION_FLIPPED )
	DECLARE_CONDITION( COND_ANTLION_ON_NPC )
	DECLARE_CONDITION( COND_ANTLION_CAN_JUMP )
		
	//Squad slots
	DECLARE_SQUADSLOT( SQUAD_SLOT_ANTLION_JUMP )

	//Tasks
	DECLARE_TASK( TASK_ANTLION_SET_CHARGE_GOAL )
	DECLARE_TASK( TASK_ANTLION_BURROW )
	DECLARE_TASK( TASK_ANTLION_UNBURROW )
	DECLARE_TASK( TASK_ANTLION_VANISH )
	DECLARE_TASK( TASK_ANTLION_FIND_BURROW_IN_POINT )
	DECLARE_TASK( TASK_ANTLION_FIND_BURROW_ROUTE )
	DECLARE_TASK( TASK_ANTLION_FIND_BURROW_OUT_POINT )
	DECLARE_TASK( TASK_ANTLION_BURROW_WAIT )
	DECLARE_TASK( TASK_ANTLION_CHECK_FOR_UNBORROW )
	DECLARE_TASK( TASK_ANTLION_JUMP )
	DECLARE_TASK( TASK_ANTLION_WAIT_FOR_TRIGGER )
	DECLARE_TASK( TASK_ANTLION_GET_THUMPER_ESCAPE_PATH )
	DECLARE_TASK( TASK_ANTLION_GET_PATH_TO_BUGBAIT )
	DECLARE_TASK( TASK_ANTLION_FACE_BUGBAIT )
	DECLARE_TASK( TASK_ANTLION_DISMOUNT_NPC )
	DECLARE_TASK( TASK_ANTLION_REACH_FIGHT_GOAL )
	DECLARE_TASK( TASK_ANTLION_GET_PHYSICS_DANGER_ESCAPE_PATH )
	DECLARE_TASK( TASK_ANTLION_FACE_JUMP )

	//Activities
	DECLARE_ACTIVITY( ACT_ANTLION_DISTRACT )
	DECLARE_ACTIVITY( ACT_ANTLION_JUMP_START )
	DECLARE_ACTIVITY( ACT_ANTLION_BURROW_IN )
	DECLARE_ACTIVITY( ACT_ANTLION_BURROW_OUT )
	DECLARE_ACTIVITY( ACT_ANTLION_BURROW_IDLE )
	DECLARE_ACTIVITY( ACT_ANTLION_RUN_AGITATED )
	DECLARE_ACTIVITY( ACT_ANTLION_FLIP )


	//Schedules
	//==================================================
	// SCHED_ANTLION_CHASE_ENEMY_BURROW
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_CHASE_ENEMY_BURROW,

		"	Tasks								"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		TASK_ANTLION_FIND_BURROW_ROUTE	0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ANTLION_RUN_TO_BURROW_IN"
		" "
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_GIVE_WAY"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Jump
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_JUMP,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_ANTLION_FACE_JUMP			0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_ANTLION_JUMP_START"
		"		TASK_ANTLION_JUMP				0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Wait for unborrow (once burrow has been triggered)
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_WAIT_UNBORROW,

		"	Tasks"
		"		TASK_ANTLION_BURROW_WAIT		0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ANTLION_WAIT_FOR_CLEAR_UNBORROW"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Burrow Wait
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_BURROW_WAIT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLION_BURROW_WAIT"
		"		TASK_ANTLION_BURROW_WAIT			1"
		"		TASK_ANTLION_FIND_BURROW_OUT_POINT	1024"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLION_WAIT_FOR_CLEAR_UNBORROW"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Burrow In
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_BURROW_IN,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLION_BURROW_WAIT"
		"		TASK_ANTLION_BURROW					0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_ANTLION_BURROW_IN"
		"		TASK_ANTLION_VANISH					0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLION_BURROW_WAIT"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Run to burrow in
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_RUN_TO_BURROW_IN,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		TASK_ANTLION_FIND_BURROW_IN_POINT	512"
		"		TASK_SET_TOLERANCE_DISTANCE			8"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLION_BURROW_IN"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_GIVE_WAY"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
	)

	//==================================================
	// Burrow Out
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_BURROW_OUT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ANTLION_BURROW_WAIT"
		"		TASK_ANTLION_UNBURROW			0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_ANTLION_BURROW_OUT"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Wait for unborrow (triggered)
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_WAIT_FOR_UNBORROW_TRIGGER,

		"	Tasks"
		"		TASK_ANTLION_WAIT_FOR_TRIGGER	0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Wait for clear burrow spot (triggered)
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_WAIT_FOR_CLEAR_UNBORROW,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLION_BURROW_WAIT"
		"		TASK_ANTLION_CHECK_FOR_UNBORROW		1"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLION_BURROW_OUT"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Run from the sound of a thumper!
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_FLEE_THUMPER,
		
		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_ANTLION_BURROW_IN"
		"		TASK_ANTLION_GET_THUMPER_ESCAPE_PATH	1200"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"		TASK_STOP_MOVING						0"
		"		TASK_WAIT								1.0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// SCHED_ANTLION_CHASE_BUGBAIT
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_CHASE_BUGBAIT,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_ANTLION_GET_PATH_TO_BUGBAIT	0"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_STOP_MOVING					0"
		"		TASK_ANTLION_FACE_BUGBAIT			0"
		""
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	//==================================================
	// SCHED_ANTLION_FLIP
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_FLIP,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_ANTLION_FLIP"

		"	Interrupts"
		"		COND_TASK_FAILED"
	)
	
	//=========================================================
	// Headcrab has landed atop another NPC. Get down!
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_DISMOUNT_NPC,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_ANTLION_DISMOUNT_NPC	0"

		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_RUN_TO_FIGHT_GOAL,

		"	Tasks"
		"		TASK_GET_PATH_TO_SAVEPOSITION	0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_ANTLION_REACH_FIGHT_GOAL	0"

		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_HEAVY_DAMAGE"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_ANTLION_CAN_JUMP"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_BUGBAIT_IDLE_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING	0"
		"		TASK_FACE_PLAYER	2"
		"		TASK_PLAY_SEQUENCE	ACTIVITY:ACT_IDLE"

		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_HEAVY_DAMAGE"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_ANTLION_CAN_JUMP"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_BURROW_AWAY,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_ANTLION_BURROW		0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_ANTLION_BURROW_IN"
		"		TASK_ANTLION_VANISH		1"

		"	Interrupts"
	)

	//==================================================
	// Run from the sound of a physics crash
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_FLEE_PHYSICS_DANGER,
		
		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ANTLION_GET_PHYSICS_DANGER_ESCAPE_PATH	1024"
		"		TASK_RUN_PATH								0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_STOP_MOVING							0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

AI_END_CUSTOM_NPC()
