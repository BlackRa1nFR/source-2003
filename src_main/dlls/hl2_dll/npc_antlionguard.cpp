//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Antlion Guard - nasty bug
//
//=============================================================================

#include "cbase.h"
#include "AI_BaseNPC.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Hint.h"
#include "AI_Senses.h"
#include "AI_Navigator.h"
#include "AI_Motor.h"
#include "bitstring.h"
#include "activitylist.h"
#include "game.h"
#include "NPCEvent.h"
#include "IEffects.h"
#include "Player.h"
#include "EntityList.h"
#include "AI_Interactions.h"
#include "grenade_spit.h"
#include "ndebugoverlay.h"
#include "smoke_trail.h"
#include "shake.h"
#include "soundent.h"
#include "soundenvelope.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "decals.h"
#include "ai_nav_property.h"
#include "iServerVehicle.h"

ConVar	g_debug_antlionguard( "g_debug_antlionguard", "0" );
ConVar	sk_antlionguard_dmg_charge( "sk_antlionguard_dmg_charge", "0" );
ConVar	sk_antlionguard_dmg_shove( "sk_antlionguard_dmg_shove", "0" );

#define	ENVELOPE_CONTROLLER		(CSoundEnvelopeController::GetController())
#define	ANTLIONGUARD_MODEL		"models/antlion_guard.mdl"
#define	MIN_BLAST_DAMAGE		25.0f

class CGrenadeSpore;
class CNPC_AntlionGuard;

//==================================================
// AntlionGuardSchedules
//==================================================

enum
{
	SCHED_ANTLIONGUARD_CHASE_ENEMY = LAST_SHARED_SCHEDULE,
	SCHED_ANTLIONGUARD_COVER,
	SCHED_ANTLIONGUARD_COVER_LOOP,
	SCHED_ANTLIONGUARD_RUN_TO_GUARDSPOT,
	SCHED_ANTLIONGUARD_ATTACK_ENEMY_LKP,
	SCHED_ANTLIONGUARD_RUN_TO_ENEMY_LKP,
	SCHED_ANTLIONGUARD_RUN_TO_ENEMY_LKP_LOS,
	SCHED_ANTLIONGUARD_SEEK_ENEMY_INITIAL,
	SCHED_ANTLIONGUARD_SEEK_ENEMY,
	SCHED_ANTLIONGUARD_SEEK_ENEMY_PEEK,
	SCHED_ANTLIONGUARD_COVER_ADVANCE,
	SCHED_ANTLIONGUARD_PEEK_FLINCH,
	SCHED_ANTLIONGUARD_PAIN,
	SCHED_ANTLIONGUARD_ATTACK_SHOVE_TARGET,
	SCHED_ANTLIONGUARD_LOST,
	SCHED_ANTLIONGUARD_EMOTE_SEEN_ENEMY,
	SCHED_ANTLIONGUARD_EXIT_PEEK,
	SCHED_ANTLIONGUARD_CHARGE,
	SCHED_ANTLIONGUARD_CHARGE_CRASH,
	SCHED_ANTLIONGUARD_CHARGE_CANCEL,
	SCHED_ANTLIONGUARD_PHYSICS_ATTACK,
	SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY,
	SCHED_ANTLIONGUARD_UNBURROW,
	SCHED_ANTLIONGUARD_CHARGE_TARGET,
	SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION,
	SCHED_ANTLIONGUARD_MELEE_ATTACK1,
};


//==================================================
// AntlionGuardTasks
//==================================================

enum
{
	TASK_ANTLIONGUARD_CHECK_UNCOVER = LAST_SHARED_TASK,
	TASK_ANTLIONGUARD_GET_PATH_TO_GUARDSPOT,
	TASK_ANTLIONGUARD_FACE_ENEMY_LKP,
	TASK_ANTLIONGUARD_EMOTE_SEARCH,
	TASK_ANTLIONGUARD_GET_SEARCH_PATH,
	TASK_ANTLIONGUARD_SET_HOP_ACTIVITY,
	TASK_ANTLIONGUARD_SNIFF,
	TASK_ANTLIONGUARD_FACE_SHOVE_TARGET,
	TASK_ANTLIONGUARD_CHARGE,
	TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT,
	TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT,
};

//==================================================
// AntlionGuardConditions
//==================================================

enum
{
	COND_ANTLIONGUARD_PHYSICS_TARGET = LAST_SHARED_CONDITION,
	COND_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY,
	COND_ANTLIONGUARD_PHYSICS_DAMAGE_LIGHT,
	COND_ANTLIONGUARD_HAS_CHARGE_TARGET,
};

Activity ACT_ANTLIONGUARD_SEARCH;
Activity ACT_ANTLIONGUARD_COVER_ENTER;
Activity ACT_ANTLIONGUARD_COVER_LOOP;
Activity ACT_ANTLIONGUARD_COVER_EXIT;
Activity ACT_ANTLIONGUARD_COVER_ADVANCE;
Activity ACT_ANTLIONGUARD_COVER_FLINCH;
Activity ACT_ANTLIONGUARD_PEEK_FLINCH;
Activity ACT_ANTLIONGUARD_PEEK_ENTER;
Activity ACT_ANTLIONGUARD_PEEK_EXIT;
Activity ACT_ANTLIONGUARD_PEEK1;
Activity ACT_ANTLIONGUARD_PAIN;
Activity ACT_ANTLIONGUARD_BARK;
Activity ACT_ANTLIONGUARD_RUN_FULL;
Activity ACT_ANTLIONGUARD_SNEAK;
Activity ACT_ANTLIONGUARD_PEEK_SIGHTED;
Activity ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT;
Activity ACT_ANTLIONGUARD_PHYSICS_FLINCH_HEAVY;
Activity ACT_ANTLIONGUARD_FLINCH_LIGHT;
Activity ACT_ANTLIONGUARD_UNBURROW;
Activity ACT_ANTLIONGUARD_ROAR;
Activity ACT_ANTLIONGUARD_RUN_HURT;

// Charge
Activity ACT_ANTLIONGUARD_CHARGE_START;
Activity ACT_ANTLIONGUARD_CHARGE_CANCEL;
Activity ACT_ANTLIONGUARD_CHARGE_RUN;
Activity ACT_ANTLIONGUARD_CHARGE_CRASH;
Activity ACT_ANTLIONGUARD_CHARGE_STOP;
Activity ACT_ANTLIONGUARD_CHARGE_HIT;

//Animation Events
#define ANTLIONGUARD_AE_CHARGE_HIT			11
#define ANTLIONGUARD_AE_SHOVE_PHYSOBJECT	12
#define ANTLIONGUARD_AE_SHOVE				13
#define	ANTLIONGUARD_AE_FOOTSTEP_LIGHT		14
#define	ANTLIONGUARD_AE_FOOTSTEP_HEAVY		15
#define ANTLIONGUARD_AE_CHARGE_EARLYOUT		16
//#define ANTLIONGUARD_AE_					17
//#define ANTLIONGUARD_AE_					18
//#define ANTLIONGUARD_AE_					19
#define	ANTLIONGUARD_AE_VOICE_GROWL			20
#define	ANTLIONGUARD_AE_VOICE_BARK			21
#define	ANTLIONGUARD_AE_VOICE_PAIN			22
#define	ANTLIONGUARD_AE_VOICE_SQUEEZE		23
#define	ANTLIONGUARD_AE_VOICE_SCRATCH		24
#define	ANTLIONGUARD_AE_VOICE_GRUNT			25

//==================================================
// CNPC_AntlionGuard
//==================================================

class CNPC_AntlionGuard : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_AntlionGuard, CAI_BaseNPC );
	DECLARE_DATADESC();

	CNPC_AntlionGuard( void );

	int		MeleeAttack1Conditions( float flDot, float flDist );
	int		MeleeAttack2Conditions( float flDot, float flDist );
	int		GetSoundInterests( void );

	void	Precache( void );
	void	Spawn( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	void	PrescheduleThink( void );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void	GatherEnemyConditions( CBaseEntity *pEnemy );
	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );

	void	Event_Killed( const CTakeDamageInfo &info );
	int		OnTakeDamage( const CTakeDamageInfo &info );

	// Input handlers.
	void	InputSetShoveTarget( inputdata_t &inputdata );
	void	InputSetChargeTarget( inputdata_t &inputdata );
	void	InputSetCoverFromAttack( inputdata_t &inputdata );
	void	InputUnburrow( inputdata_t &inputdata );
	void	InputRagdoll( inputdata_t &inputdata );

	bool	FValidateHintType( CAI_Hint *pHint );
	bool	FVisible( CBaseEntity *pEntity, int traceMask = MASK_OPAQUE, CBaseEntity **ppBlocker = NULL );
	bool	InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );

	int		TranslateSchedule( int scheduleType );

	float	MaxYawSpeed( void );

	Class_T	Classify( void ) { return CLASS_ANTLION; }

	Activity	NPC_TranslateActivity( Activity baseAct );

	int		SelectSchedule( void );
	Vector	EyePosition( void );

	void	StopLoopingSounds();
	//bool	IsLightDamage( float flDamage, int bitsDamageType );

	DEFINE_CUSTOM_AI;

protected:

	int			SelectCombatSchedule( void );

	bool		DetectedHiddenEnemy( void );
	bool		CheckChargeImpact( void );
	bool		ShouldCharge( const Vector &startPos, const Vector &endPos, bool useTime = true );

	void		BuildScheduleTestBits( void );
	void		RestrictVision( bool restrict = true );			
	void		Shove( void );
	void		FoundEnemy( void );
	void		LostEnemy( void );
	void		UpdateHead( void );
	void		DropBugBait( void );
	void		UpdatePhysicsTarget( void );
	void		SweepPhysicsDebris( void );
	void		ChargeDamage( CBaseEntity *pTarget );
				
	float		ChargeSteer( void );
	void		StartSounds( void );
	void		SoundTest( const char *pSoundName );

	CBaseEntity *FindPhysicsObjectTarget( CBaseEntity *pTarget );

	bool		m_bContinuedSearch;
	bool		m_bWatchEnemy;
	bool		m_bStopped;
	bool		m_bCoverFromAttack;
	bool		m_bStartBurrowed;
	bool		m_bAllowKillDamage;	//Override for damage handling
	
	float		m_flNextGuardSearchTime;

	float		m_flAngerTime;
	float		m_flCoverTime;
	float		m_flSearchNoiseTime;
	float		m_flAngerNoiseTime;
	float		m_flRecoverTime;
	float		m_flBreathTime;
	float		m_flChargeTime;
	float		m_flLastDamageTime;

	Vector		m_vecShove;
	int			m_fLookMask;

	EHANDLE		m_hShoveTarget;
	EHANDLE		m_hChargeTarget;

	CAI_Hint	*m_pGuardHint;
	CAI_Hint	*m_pSearchHint;
	CAI_Hint	*m_pLastSearchHint;

	COutputEvent	m_OnSeeHiddenPlayer;
	COutputEvent	m_OnSmellHiddenPlayer;

	CSoundPatch		*m_pGrowlHighSound;
	CSoundPatch		*m_pGrowlLowSound;
	CSoundPatch		*m_pGrowlIdleSound;
	CSoundPatch		*m_pBreathSound;
	CSoundPatch		*m_pConfusedSound;

	EHANDLE			m_hPhysicsTarget;
	float			m_flPhysicsCheckTime;
};

//==================================================
//
// Antlion Guard
//
//==================================================

#define ANTLIONGUARD_MAX_OBJECTS				128
#define	ANTLIONGUARD_MIN_OBJECT_MASS			16
#define	ANTLIONGUARD_MAX_OBJECT_MASS			1024
#define	ANTLIONGUARD_FARTHEST_PHYSICS_OBJECT	512

//Melee definitions
#define	ANTLIONGUARD_MELEE1_RANGE	128.0f
#define	ANTLIONGUARD_MELEE1_CONE	0.7f
#define	ANTLIONGUARD_MELEE1_TIME	1.0f

//Shove definitions
#define	ANTLIONGUARD_SHOVE_ATTACK_DIST	128.0f
#define	ANTLIONGUARD_SHOVE_CONE			0.75f
#define	ANTLIONGUARD_SHOVE_HEIGHT		64.0f

#define	ANTLIONGUARD_FOV_NORMAL			-0.4f
#define	ANTLIONGUARD_FOV_RESTRICTED		0.96f

#define	ANTLIONGUARD_COVER_TIME			1.0f
#define	ANTLIONGUARD_ANGER_TIME			2.0f

#define	ANTLIONGUARD_SEARCH_RADIUS		512.0f

#define	ANTLIONGUARD_MAX_SNIFF_DIST		128.0f
#define	ANTLIONGUARD_MAX_SNIFF_DIST_SQR	(ANTLIONGUARD_MAX_SNIFF_DIST*ANTLIONGUARD_MAX_SNIFF_DIST)

#define	ANTLIONGUARD_MIN_PLAYER_GUARD_DIST			512.0f
#define	ANTLIONGUARD_MIN_PLAYER_GUARD_DIST_SQR		(ANTLIONGUARD_MIN_PLAYER_GUARD_DIST*ANTLIONGUARD_MIN_PLAYER_GUARD_DIST)

#define	ANTLIONGUARD_MIN_GUARD_DIST		512.0f
#define	ANTLIONGUARD_MAX_SHOVE_HEIGHT	128.0f

//Neck controllers
#define	ANTLIONGUARD_NECK_YAW			0
#define	ANTLIONGUARD_NECK_PITCH			1

#define	ANTLIONGUARD_CHARGE_MIN			350
#define	ANTLIONGUARD_CHARGE_MAX			1024

ConVar	sk_antlionguard_health( "sk_antlionguard_health", "0" );

//==================================================
// CNPC_AntlionGuard::m_DataDesc
//==================================================

BEGIN_DATADESC( CNPC_AntlionGuard )

	DEFINE_FIELD( CNPC_AntlionGuard, m_bContinuedSearch,	FIELD_BOOLEAN ), 	
	DEFINE_FIELD( CNPC_AntlionGuard, m_bWatchEnemy,			FIELD_BOOLEAN ), 	
	DEFINE_FIELD( CNPC_AntlionGuard, m_bStopped,			FIELD_BOOLEAN ), 	
	DEFINE_FIELD( CNPC_AntlionGuard, m_bCoverFromAttack,	FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( CNPC_AntlionGuard, m_bStartBurrowed,	FIELD_BOOLEAN, "startburrowed" ),
	DEFINE_FIELD( CNPC_AntlionGuard, m_bAllowKillDamage,	FIELD_BOOLEAN ),

	DEFINE_FIELD( CNPC_AntlionGuard, m_flNextGuardSearchTime,FIELD_TIME ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_flAngerTime,			FIELD_TIME ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_flCoverTime,			FIELD_TIME ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_flSearchNoiseTime,	FIELD_TIME ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_flAngerNoiseTime,	FIELD_TIME ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_flRecoverTime,		FIELD_TIME ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_flBreathTime,		FIELD_TIME ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_flChargeTime,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_AntlionGuard, m_flLastDamageTime,	FIELD_TIME ),

	DEFINE_FIELD( CNPC_AntlionGuard, m_vecShove,			FIELD_VECTOR ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_fLookMask,			FIELD_INTEGER ), 
	
	DEFINE_FIELD( CNPC_AntlionGuard, m_pGuardHint,			FIELD_CLASSPTR ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_pSearchHint,			FIELD_CLASSPTR ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_pLastSearchHint,		FIELD_CLASSPTR ), 

	DEFINE_FIELD( CNPC_AntlionGuard, m_hShoveTarget,		FIELD_EHANDLE ), 
	DEFINE_FIELD( CNPC_AntlionGuard, m_hChargeTarget,		FIELD_EHANDLE ), 

	DEFINE_FIELD( CNPC_AntlionGuard, m_hPhysicsTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_AntlionGuard, m_flPhysicsCheckTime, FIELD_TIME ),

	DEFINE_OUTPUT( CNPC_AntlionGuard, m_OnSeeHiddenPlayer, "OnSeeHiddenPlayer" ),
	DEFINE_OUTPUT( CNPC_AntlionGuard, m_OnSmellHiddenPlayer, "OnSmellHiddenPlayer" ),

	DEFINE_SOUNDPATCH( CNPC_AntlionGuard, m_pGrowlHighSound ),
	DEFINE_SOUNDPATCH( CNPC_AntlionGuard, m_pGrowlLowSound ),
	DEFINE_SOUNDPATCH( CNPC_AntlionGuard, m_pGrowlIdleSound ),
	DEFINE_SOUNDPATCH( CNPC_AntlionGuard, m_pBreathSound ),
	DEFINE_SOUNDPATCH( CNPC_AntlionGuard, m_pConfusedSound ),

	DEFINE_INPUTFUNC( CNPC_AntlionGuard, FIELD_STRING, "SetShoveTarget", InputSetShoveTarget ),
	DEFINE_INPUTFUNC( CNPC_AntlionGuard, FIELD_STRING, "SetChargeTarget", InputSetChargeTarget ),
	DEFINE_INPUTFUNC( CNPC_AntlionGuard, FIELD_INTEGER, "SetCoverFromAttack", InputSetCoverFromAttack ),
	DEFINE_INPUTFUNC( CNPC_AntlionGuard, FIELD_VOID, "Unburrow", InputUnburrow ),
	DEFINE_INPUTFUNC( CNPC_AntlionGuard, FIELD_VOID, "Ragdoll", InputRagdoll ),

END_DATADESC()


//Fast Growl (Growl High)
envelopePoint_t envAntlionGuardFastGrowl[] =
{
	{	1.0f, 1.0f,
		0.2f, 0.4f,
	},
	{	0.1f, 0.1f,
		0.8f, 1.0f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.8f,
	},
};


//Bark 1 (Growl High)
envelopePoint_t envAntlionGuardBark1[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.6f,
	},
};

//Bark 2 (Confused)
envelopePoint_t envAntlionGuardBark2[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.2f, 0.3f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.6f,
	},
};

//Pain
envelopePoint_t envAntlionGuardPain1[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	-1.0f, -1.0f,
		0.5f, 0.8f,
	},
	{	0.1f, 0.2f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.5f, 0.75f,
	},
};

//Squeeze (High Growl)
envelopePoint_t envAntlionGuardSqueeze[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		1.0f, 1.5f,
	},
};

//Scratch (Low Growl)
envelopePoint_t envAntlionGuardScratch[] =
{
	{	1.0f, 1.0f,
		0.4f, 0.6f,
	},
	{	0.5f, 0.5f,
		0.4f, 0.6f,
	},
	{	0.0f, 0.0f,
		1.0f, 1.5f,
	},
};

//Grunt
envelopePoint_t envAntlionGuardGrunt[] =
{
	{	0.6f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.1f, 0.2f,
	},
};

envelopePoint_t envAntlionGuardGrunt2[] =
{
	{	0.2f, 0.4f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.6f,
	},
};

//==================================================
// CNPC_AntlionGuard
//==================================================

CNPC_AntlionGuard::CNPC_AntlionGuard( void )
{
}

LINK_ENTITY_TO_CLASS( npc_antlionguard, CNPC_AntlionGuard );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::Precache( void )
{
	engine->PrecacheModel( ANTLIONGUARD_MODEL );

	//Additive sounds
	enginesound->PrecacheSound( "npc/antlion_guard/growl_high.wav" );
	enginesound->PrecacheSound( "npc/antlion_guard/growl_idle.wav" );
	enginesound->PrecacheSound( "npc/antlion_guard/breath.wav" );
	enginesound->PrecacheSound( "npc/antlion_guard/confused1.wav" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::Spawn( void )
{
	Precache();

	SetModel( ANTLIONGUARD_MODEL );

	SetHullType(HULL_LARGE);
	SetHullSizeNormal();
	SetDefaultEyeOffset();
	
	SetNavType(NAV_GROUND);
	m_NPCState				= NPC_STATE_NONE;
	SetBloodColor( BLOOD_COLOR_YELLOW );
	m_iHealth				= sk_antlionguard_health.GetFloat();
	m_iMaxHealth			= m_iHealth;
	m_flFieldOfView			= 0.25f;
	
	m_flNextGuardSearchTime	= 0;
	m_flCoverTime			= 0;
	m_flRecoverTime			= 0;
	m_flPhysicsCheckTime	= 0;
	m_flChargeTime			= 0;
	m_flLastDamageTime		= 0;

	m_pGuardHint			= NULL;
	m_pSearchHint			= NULL;
	m_pLastSearchHint		= NULL;
	ClearHintGroup();
	m_hPhysicsTarget		= NULL;

	m_bContinuedSearch		= false;
	m_bStopped				= false;
	m_bWatchEnemy			= true;
	m_bCoverFromAttack		= true;
	m_bAllowKillDamage		= false;

	m_hShoveTarget			= NULL;
	m_hChargeTarget			= NULL;

	m_HackedGunPos.x		= 10;
	m_HackedGunPos.y		= 0;
	m_HackedGunPos.z		= 30;

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK1 );

	NPCInit();

	BaseClass::Spawn();

	//See if we're supposed to start burrowed
	if ( m_bStartBurrowed )
	{
		m_fEffects	|= EF_NODRAW;
		AddFlag( FL_NOTARGET );

		m_spawnflags |= SF_NPC_GAG;
		
		AddSolidFlags( FSOLID_NOT_SOLID );
		
		m_takedamage = DAMAGE_NO;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::SelectCombatSchedule( void )
{
	//Check for a hidden enemy
	if ( HasCondition( COND_ENEMY_OCCLUDED ) )
	{
		//Do a special event on the first search
		if ( m_bContinuedSearch == false )
		{
			ClearHintGroup();
			m_bContinuedSearch	= true;
			m_bWatchEnemy		= false;
			
			return SCHED_ANTLIONGUARD_RUN_TO_ENEMY_LKP;
		}

		//Continue to search
		return SCHED_ANTLIONGUARD_SEEK_ENEMY;
	}

	if ( GetEnemy() != NULL )
	{
		Vector	predTargetPos;
		UTIL_PredictedPosition( GetEnemy(), 1.5f, &predTargetPos );

		//Charging
		if ( ShouldCharge( GetAbsOrigin(), predTargetPos ) )
			return SCHED_ANTLIONGUARD_CHARGE;
	}

	//Physics target
	if ( HasCondition( COND_ANTLIONGUARD_PHYSICS_TARGET ) )
		return SCHED_ANTLIONGUARD_PHYSICS_ATTACK;

	//Reset our search indicators
	m_bContinuedSearch	= false;
	m_bWatchEnemy		= true;
	ClearHintGroup();
	m_pLastSearchHint	= NULL;
	m_pHintNode			= NULL;

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::ShouldCharge( const Vector &startPos, const Vector &endPos, bool useTime )
{
	if ( GetEnemy() == NULL )
		return false;

	if ( HasCondition( COND_ENEMY_OCCLUDED ) )
		return false;

	if ( ( useTime ) && ( m_flChargeTime > gpGlobals->curtime ) )
		return false;

	if ( UTIL_DistApprox2D( startPos, endPos ) < ANTLIONGUARD_CHARGE_MIN )
		return false;

	trace_t	tr;
	Vector	mins = GetHullMins();

	int StepHeights = StepHeight() * 3;
	mins[2] += StepHeights;

	AI_TraceHull( startPos, endPos, mins, GetHullMaxs(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	CBaseEntity	*pHitEntity = tr.m_pEnt;
	
	// is enemy the player and is the player in a vehicle? then it's not in the way...
	bool bVehicleMatchesHit = false;
	CBaseEntity *pEntity = GetEnemy();
	if ( pEntity->IsPlayer() )
	{
		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer *>(pEntity);
		if ( pPlayer->IsInAVehicle() )
		{
			CBaseEntity *pVehicleEnt = pPlayer->GetVehicle()->GetVehicleEnt();
			if ( pVehicleEnt == pHitEntity )
				bVehicleMatchesHit = true;
		}
	}

	if ( ( tr.fraction < 1.0f ) && ( pHitEntity != GetEnemy() ) && !bVehicleMatchesHit)
	{
		if ( g_debug_antlionguard.GetBool() )
		{
			Vector	enemyDir	= (endPos - startPos);
			float	enemyDist	= VectorNormalize( enemyDir );

			NDebugOverlay::BoxDirection( startPos, mins, GetHullMaxs() + Vector(enemyDist,0,0), enemyDir, 255, 0, 0, 8, 1.0f );
		}

		return false;
	}

	if ( g_debug_antlionguard.GetBool() )
	{
		Vector	enemyDir	= (endPos - startPos);
		float	enemyDist	= VectorNormalize( enemyDir );

		NDebugOverlay::BoxDirection( startPos, mins, GetHullMaxs() + Vector(enemyDist,0,0), enemyDir, 0, 255, 0, 8, 1.0f );
	}

	//Only update this if we've requested it
	if ( useTime )
	{
		m_flChargeTime = gpGlobals->curtime + 4.0f;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::SelectSchedule( void )
{	
	if ( m_bStartBurrowed )
		return SCHED_IDLE_STAND;

	//Physics flinch heavy
	if ( HasCondition( COND_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY ) )
	{
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY );
		UTIL_ScreenShake( GetAbsOrigin(), 32.0f, 8.0f, 0.5f, 512, SHAKE_START );
		return SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY;
	}

	//Charge after a charge target if it's set
	if ( m_hChargeTarget != NULL )
	{
		ClearCondition( COND_ANTLIONGUARD_HAS_CHARGE_TARGET );

		if ( m_hChargeTarget->IsAlive() == false )
		{
			ClearHintGroup();
			m_hChargeTarget	= NULL;
		}
		else
		{
			ClearHintGroup();

			SetEnemy( m_hChargeTarget );

			//If we can't see the target, run to somewhere we can
			if ( ShouldCharge( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), false ) == false )
				return SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION;

			return SCHED_ANTLIONGUARD_CHARGE_TARGET;
		}
	}

	//Only do these in combat states
	if ( m_NPCState == NPC_STATE_COMBAT )
		return SelectCombatSchedule();

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : sHint - 
//			nNodeNum - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::FValidateHintType( CAI_Hint *pHint )
{
	if ( pHint == NULL )
		return false;

	//Valide the disadvantage hint
	if ( pHint->HintType() == HINT_TACTICAL_ENEMY_DISADVANTAGED )
	{
		Vector	vecGoal;
		pHint->GetPosition(this,&vecGoal);

		Vector forward;
		AngleVectors( GetLocalAngles(), &forward );
		Vector	vecDir = ( vecGoal - GetLocalOrigin() );
		VectorNormalize( vecDir );

		if ( vecDir.Dot( forward ) < 0.0f )
			return false;
		
		if ( ( vecGoal[2] - GetLocalOrigin()[2] ) > ANTLIONGUARD_MAX_SHOVE_HEIGHT )
			return false;

		Vector	vecToss = VecCheckThrow( GetEnemy(), GetEnemy()->GetAbsOrigin(), vecGoal, 300, 0.5f );
		
		if ( vecToss[2] > 400.0f )
		{
			vecToss[2] = 400.0f;
		}

		if ( vecToss == vec3_origin )
			return false;

		m_vecShove = vecToss + Vector( 0, 0, 150 );
		
		return true;
	}
	
	//Validate the pinch hint
	if ( pHint->HintType() == HINT_TACTICAL_PINCH )
	{
		//Never reselect our last search node
		if ( ( pHint == m_pLastSearchHint ) || ( pHint == m_pSearchHint ) )
			return false;
		
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > ANTLIONGUARD_MELEE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < ANTLIONGUARD_MELEE1_CONE )
		return COND_NOT_FACING_ATTACK;

	if ( GetEnemy() == NULL )
		return COND_TOO_FAR_TO_ATTACK;

	Vector	offset = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();

	if ( fabs( offset[2] ) > ANTLIONGUARD_SHOVE_HEIGHT )
		return COND_NOT_FACING_ATTACK;

	trace_t	tr;
	AI_TraceHull( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), Vector(-10,-10,-10), Vector(10,10,10), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
		return COND_CAN_MELEE_ATTACK1;

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::MeleeAttack2Conditions( float flDot, float flDist )
{
	if ( HasCondition( COND_ENEMY_OCCLUDED ) )
		return COND_ENEMY_OCCLUDED;

	if ( m_flChargeTime > gpGlobals->curtime )
		return 0;

	if ( UTIL_DistApprox2D( GetAbsOrigin(), GetEnemy()->GetAbsOrigin() ) < 128 )
		return COND_TOO_CLOSE_TO_ATTACK;

	trace_t	tr;
	Vector	mins = GetHullMins();
	mins[2] += StepHeight();

	//FIXME: Slow...
	AI_TraceHull( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), mins, GetHullMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
		return COND_WEAPON_SIGHT_OCCLUDED;

	m_flChargeTime = gpGlobals->curtime + 2.0f;

	return COND_CAN_MELEE_ATTACK2;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_AntlionGuard::MaxYawSpeed( void )
{
	Activity eActivity = GetActivity();

	if (( eActivity == ACT_ANTLIONGUARD_SEARCH ) || 
		( eActivity == ACT_ANTLIONGUARD_COVER_ENTER ) || 
		( eActivity == ACT_ANTLIONGUARD_COVER_LOOP ) || 
		( eActivity == ACT_ANTLIONGUARD_COVER_EXIT ) || 
		( eActivity == ACT_ANTLIONGUARD_COVER_ADVANCE ) || 
		( eActivity == ACT_ANTLIONGUARD_COVER_FLINCH ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK_ENTER ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK_EXIT ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK1 ) || 
		( eActivity == ACT_ANTLIONGUARD_PAIN ) || 
		( eActivity == ACT_ANTLIONGUARD_BARK ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK_SIGHTED ) || 
		( eActivity == ACT_MELEE_ATTACK1 ) )
		return 0.0f;

	//Turn slowly when you're charging
	if ( eActivity == ACT_ANTLIONGUARD_CHARGE_START )
		return 1.0f;

	//FIXME: Alter by skill level
	if ( eActivity == ACT_ANTLIONGUARD_CHARGE_RUN )
		return 2.0f;

	if ( eActivity == ACT_ANTLIONGUARD_CHARGE_STOP )
		return 8.0f;

	switch( eActivity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 40.0f;
		break;
	
	case ACT_RUN:
		return 20.0f;
		break;
	
	default:
		return 20.0f;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::Shove( void )
{
	CBaseEntity *pHurt = NULL;
	CBaseEntity *pTarget;

	pTarget = ( m_hShoveTarget == NULL ) ? GetEnemy() : m_hShoveTarget;

	if ( pTarget == NULL )
	{
		m_hShoveTarget = NULL;
		return;
	}

	//Always damage bullseyes if we're scripted to hit them
	if ( pTarget->Classify() == CLASS_BULLSEYE )
	{
		pTarget->TakeDamage( CTakeDamageInfo( this, this, 1.0f, DMG_CRUSH ) );
		m_hShoveTarget = NULL;
		return;
	}

	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );

	float		flDist	= ( pTarget->GetAbsOrigin() - GetLocalOrigin() ).Length();
	Vector2D	v2LOS	= ( pTarget->GetAbsOrigin() - GetLocalOrigin() ).AsVector2D();
	Vector2DNormalize(v2LOS);
	float		flDot	= DotProduct2D (v2LOS , forward.AsVector2D() );

	float damage = ( pTarget->IsPlayer() ) ? sk_antlionguard_dmg_shove.GetFloat() : 250;

	trace_t tr;

	if ( flDist < ANTLIONGUARD_SHOVE_ATTACK_DIST && flDot >= ANTLIONGUARD_SHOVE_CONE )
	{
		AI_TraceHull( WorldSpaceCenter(), pTarget->WorldSpaceCenter(), Vector(-10,-10,-10), Vector(10,10,10), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );
		pHurt = tr.m_pEnt;

		//pHurt = CheckTraceHullAttack( WorldSpaceCenter(), pTarget->WorldSpaceCenter(), Vector(-10,-10,-10), Vector(10,10,10), damage, DMG_CLUB );
	}
	else
	{
		//FIXME: Trace directly forward
		AI_TraceHull( WorldSpaceCenter(), pTarget->WorldSpaceCenter(), Vector(-10,-10,-10), Vector(10,10,10), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );
		pHurt = tr.m_pEnt;

		//pHurt = CheckTraceHullAttack( ANTLIONGUARD_SHOVE_ATTACK_DIST, Vector(-10,-10,-10), Vector(10,10,10), damage, DMG_CLUB );
	}

	if ( pHurt )
	{
		Vector	traceDir = ( tr.endpos - tr.startpos );
		VectorNormalize( traceDir );

		// Generate enough force to make a 75kg guy move away at 600 in/sec
		Vector vecForce = traceDir * ImpulseScale( 75, 600 );
		CTakeDamageInfo info( this, this, vecForce, tr.endpos, damage, DMG_CLUB );
		pHurt->TakeDamage( info );

		m_hShoveTarget = NULL;

		SoundTest( "NPC_AntlionGuard.Shove" );

		// If the player, throw him around
		if ( pHurt->IsPlayer() )
		{
			//Punch the view
			pHurt->ViewPunch( QAngle(20,0,-20) );
			
			//Shake the screen
			UTIL_ScreenShake( pHurt->GetAbsOrigin(), 100.0, 1.5, 1.0, 2, SHAKE_START );

			//Red damage indicator
			color32 red = {128,0,0,128};
			UTIL_ScreenFade( pHurt, red, 1.0f, 0.1f, FFADE_IN );

			CHintCriteria	hintCriteria;

			hintCriteria.AddIncludePosition( GetLocalOrigin(), 512 );
			hintCriteria.AddExcludePosition( GetLocalOrigin(), 128 );
			hintCriteria.SetHintType( HINT_TACTICAL_ENEMY_DISADVANTAGED );

			CAI_Hint *pThrowHint = CAI_Hint::FindHint( this, &hintCriteria );

			if ( pThrowHint == NULL )
			{
				Vector forward, up;
				AngleVectors( GetLocalAngles(), &forward, NULL, &up );
				pHurt->ApplyAbsVelocityImpulse( forward * 400 + up * 250 );
			}
			else
			{
				pHurt->SetAbsVelocity( m_vecShove );
			}
		}	
		else
		{
			CBaseCombatCharacter *pVictim = ToBaseCombatCharacter( pHurt );
			
			if ( pVictim )
			{
				//if ( pVictim->HandleInteraction( g_interactionAntlionGuardThrow, NULL, this ) )
				{
					pVictim->ApplyAbsVelocityImpulse( BodyDirection2D() * 400 + Vector( 0, 0, 250 ) );
				}
			}

			m_hShoveTarget = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case ANTLIONGUARD_AE_CHARGE_EARLYOUT:
		{
			if ( ShouldCharge( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), false ) == false )
			{
				SetSchedule( SCHED_ANTLIONGUARD_CHARGE_CANCEL );
				return;
			}
		}
		break;

	case ANTLIONGUARD_AE_SHOVE_PHYSOBJECT:
		{
			if ( m_hPhysicsTarget == NULL )
				return;
	
			//Setup the throw velocity
			IPhysicsObject *physObj = m_hPhysicsTarget->VPhysicsGetObject();

			Vector	targetDir = ( GetEnemy()->GetAbsOrigin() - m_hPhysicsTarget->WorldSpaceCenter() );
			float	targetDist = VectorNormalize( targetDir );

			if ( targetDist < 512 )
				targetDist = 512;

			if ( targetDist > 1024 )
				targetDist = 1024;

			targetDir *= targetDist * 3 * physObj->GetMass();	//FIXME: Scale by skill
			targetDir[2] += physObj->GetMass() * 350.0f;
			
			//Display dust
			g_pEffects->Dust( m_hPhysicsTarget->WorldSpaceCenter(), RandomVector( -1, 1), 64.0f, 32 );

			//Play the proper impact sound
			//surfacedata_t *pHit = physprops->GetSurfaceData( m_hPhysicsTarget->entindex() );

			//Send it flying
			AngularImpulse angVel( random->RandomFloat(-180, 180), 100, random->RandomFloat(-360, 360) );
			physObj->ApplyForceCenter( targetDir );
			physObj->AddVelocity( NULL, &angVel );
			
			//Clear the state information, we're done
			ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );

			m_hPhysicsTarget = NULL;
			m_flPhysicsCheckTime = gpGlobals->curtime + 2.0f;
		}
		break;

	case ANTLIONGUARD_AE_CHARGE_HIT:
		{
			UTIL_ScreenShake( GetAbsOrigin(), 32.0f, 4.0f, 1.0f, 512, SHAKE_START );
			SoundTest( "NPC_AntlionGuard.HitHard" );

			trace_t	tr;
			Vector	startPos, endPos;

			startPos = GetAbsOrigin();// + Vector(0,0,20);
			endPos = startPos + ( BodyDirection2D() * 128 );	//FIXME: For now

			Vector	mins, maxs;

			mins = GetHullMins();
			maxs = GetHullMaxs();

			mins[2] += StepHeight();

			AI_TraceHull( startPos, endPos, mins, maxs, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction < 1.0f )
			{			
				//Do damage
				ChargeDamage( tr.m_pEnt );

				tr.endpos[2] += 20.0f;

				Vector offset;
				for ( int i = 0; i < 12; i++ )
				{
					offset.Random( -32.0f, 32.0f );
					g_pEffects->Dust( tr.endpos + offset, -tr.plane.normal, 64.0f, 32 );
				}

				if ( tr.m_pEnt != NULL )
				{
					//Clear our charge target if this was it
					if ( ( tr.m_pEnt == m_hChargeTarget ) && ( tr.m_pEnt->IsAlive() == false ) )
					{
						m_hChargeTarget = NULL;
					}
				}
			}
		}

		break;

	case ANTLIONGUARD_AE_SHOVE:
		SoundTest("NPC_AntlionGuard.StepLight" );
		Shove();
		break;

	case ANTLIONGUARD_AE_FOOTSTEP_LIGHT:
		SoundTest("NPC_AntlionGuard.StepLight" );
		break;

	case ANTLIONGUARD_AE_FOOTSTEP_HEAVY:
		SoundTest( "NPC_AntlionGuard.StepHeavy" );
		break;
	
	case ANTLIONGUARD_AE_VOICE_GROWL:
		{
			StartSounds();

			float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardFastGrowl, ARRAYSIZE(envAntlionGuardFastGrowl) );
			
			m_flAngerNoiseTime = gpGlobals->curtime + duration + random->RandomFloat( 2.0f, 4.0f );

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

			m_flBreathTime = gpGlobals->curtime + duration - 0.2f;

			SoundTest( "NPC_AntlionGuard.Anger" );
		}
		break;

	case ANTLIONGUARD_AE_VOICE_BARK:
		{
			StartSounds();

			float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardBark1, ARRAYSIZE(envAntlionGuardBark1) );
			ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pConfusedSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardBark2, ARRAYSIZE(envAntlionGuardBark2) );
			
			m_flAngerNoiseTime = gpGlobals->curtime + duration + random->RandomFloat( 2.0f, 4.0f );

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );
			
			m_flBreathTime = gpGlobals->curtime + duration - 0.2f;
		}
		break;

	case ANTLIONGUARD_AE_VOICE_PAIN:
		{
			StartSounds();

			float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pConfusedSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardPain1, ARRAYSIZE(envAntlionGuardPain1) );
			ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardBark2, ARRAYSIZE(envAntlionGuardBark2) );
			
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );
			
			m_flBreathTime = gpGlobals->curtime + duration - 0.2f;
		}
		break;

	case ANTLIONGUARD_AE_VOICE_SQUEEZE:
		{	
			StartSounds();

			float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardSqueeze, ARRAYSIZE(envAntlionGuardSqueeze) );
			
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.6f, random->RandomFloat( 2.0f, 4.0f ) );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pBreathSound, random->RandomInt( 60, 80 ), random->RandomFloat( 2.0f, 4.0f ) );

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

			m_flBreathTime = gpGlobals->curtime + ( duration * 0.5f );

			SoundTest( "NPC_AntlionGuard.Anger" );
		}
		break;

	case ANTLIONGUARD_AE_VOICE_SCRATCH:
		{	
			StartSounds();

			float duration = random->RandomFloat( 2.0f, 4.0f );

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.6f, duration );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pBreathSound, random->RandomInt( 60, 80 ), duration );

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

			m_flBreathTime = gpGlobals->curtime + duration;

			SoundTest( "NPC_AntlionGuard.Anger" );
		}
		break;

	case ANTLIONGUARD_AE_VOICE_GRUNT:
		{	
			StartSounds();

			float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pConfusedSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardGrunt, ARRAYSIZE(envAntlionGuardGrunt) );
			ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardGrunt2, ARRAYSIZE(envAntlionGuardGrunt2) );
			
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

			m_flBreathTime = gpGlobals->curtime + duration;
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	BaseClass::GatherEnemyConditions( pEnemy );

	/*
	if ( HasCondition( COND_SEE_ENEMY ) )
	{
		m_flAngerTime = gpGlobals->curtime + ANTLIONGUARD_ANGER_TIME;

		if ( m_NPCState != NPC_STATE_COMBAT )
		{
			FoundEnemy();
		}
	}
	else if ( ( m_flAngerTime < gpGlobals->curtime ) && ( m_NPCState != NPC_STATE_ALERT ) )
	{
		//We're crossing the threshold between combat and alert
		LostEnemy();
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::OnTakeDamage( const CTakeDamageInfo &info )
{
	//Allow any overridden damage to pass unfiltered
	if ( m_bAllowKillDamage )
	{
		return BaseClass::OnTakeDamage( info );
	}

	if ( ( gpGlobals->curtime - m_flLastDamageTime ) < 0.1f )
		return 0;

	//Don't damage on charge
	if ( GetActivity() == ACT_ANTLIONGUARD_CHARGE_RUN )
	{
		RestartGesture( ACT_ANTLIONGUARD_FLINCH_LIGHT );
		return 0;
	}

	// Paff
	if ( info.GetDamageType() & DMG_CRUSH )
	{
		IPhysicsObject *pPhysObject = info.GetInflictor()->VPhysicsGetObject();

		if ( pPhysObject != NULL )
		{
			if ( ( pPhysObject->GetMass() > 32 ) && ( info.GetDamage() > 16 ) )
			{
				CTakeDamageInfo dInfo = info;

				dInfo.SetDamage( m_iMaxHealth / 8.0f );
			
				m_flLastDamageTime = gpGlobals->curtime;

				SetCondition( COND_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY );
				return BaseClass::OnTakeDamage( dInfo );
			}
			else
			{
				RestartGesture( ACT_ANTLIONGUARD_FLINCH_LIGHT );		
				return 0;
			}
		}
	}
	else if ( info.GetDamageType() & DMG_BLAST )
	{
		if ( info.GetDamage() > MIN_BLAST_DAMAGE )
		{
			CTakeDamageInfo dInfo = info;

			dInfo.SetDamage( m_iMaxHealth / 8.0f );
		
			m_flLastDamageTime = gpGlobals->curtime;

			SetCondition( COND_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY );
			return BaseClass::OnTakeDamage( dInfo );
		}
		else
		{
			RestartGesture( ACT_ANTLIONGUARD_FLINCH_LIGHT );		
			return 0;
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAttacker - 
//			flDamage - 
//			&vecDir - 
//			*ptr - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	//E3 HACK: We treat the gunship specially
	if ( info.GetAttacker() != NULL )
	{
		if ( FClassnameIs( info.GetAttacker(), "npc_combinegunship" ) )
		{
			info.SetDamage( m_iMaxHealth / 64.0f );
			BaseClass::TraceAttack( info, vecDir, ptr );
			return;
		}
	}

	//Flinch on light damage
	/*
	if ( IsLightDamage( info.GetDamage(), info.GetDamageType() ) )
	{
		RestartGesture( ACT_ANTLIONGUARD_FLINCH_LIGHT );
	}
	*/

	// Since we don't take damage for anything but crush or >MIN_BLAST_DAMAGE units of
	// blast, reset damage so we don't show blood.
	if ( info.GetDamageType() & DMG_BULLET )
	{	
		// play ricochet sound
		g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
		return;
	}
		
	if( !( info.GetDamageType() & ( DMG_CRUSH | DMG_BLAST ) ) )
	{
		return;
	}


	BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDamage - 
//			bitsDamageType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
/*
bool CNPC_AntlionGuard::IsLightDamage( float flDamage, int bitsDamageType )
{
	return true;
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT:
		{
			if ( ( m_hPhysicsTarget == NULL ) || ( GetEnemy() == NULL ) )
			{
				TaskFail( "Tried to shove a NULL physics target!\n" );
				break;
			}
			
			//Get the direction and distance to our thrown object
			Vector	dirToTarget = ( m_hPhysicsTarget->WorldSpaceCenter() - WorldSpaceCenter() );
			float	distToTarget = VectorNormalize( dirToTarget );
			dirToTarget.z = 0;

			//Validate it's still close enough to shove
			//FIXME: Real numbers
			if ( distToTarget > 200.0f )
			{
				m_hPhysicsTarget = NULL;
				m_flPhysicsCheckTime = gpGlobals->curtime + 1.0f;
				TaskFail( "Shove target moved\n" );
				break;
			}

			//Validate its offset from our facing
			float targetYaw = UTIL_VecToYaw( dirToTarget );
			float offset = UTIL_AngleDiff( targetYaw, UTIL_AngleMod( GetLocalAngles().y ) );

			if ( fabs( offset ) > 65 )
			{
				m_hPhysicsTarget = NULL;
				m_flPhysicsCheckTime = gpGlobals->curtime + 1.0f;
				TaskFail( "Shove target off-center\n" );
				break;
			}

			//Blend properly
			SetPoseParameter( "throw", offset );

			//Start playing the animation
			SetActivity( (Activity) ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT );
		}
		
		break;

	case TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT:
		{
			if ( ( m_hPhysicsTarget == NULL ) || ( GetEnemy() == NULL ) )
			{
				TaskFail( "Tried to find a path to NULL physics target!\n" );
				break;
			}

			Vector vecDir = m_hPhysicsTarget->WorldSpaceCenter() - GetEnemy()->WorldSpaceCenter();
			VectorNormalize( vecDir );
			vecDir.z = 0;

			// UNDONE: This goal must be based on the size of the object, not a constant.
			Vector vecGoalPos = m_hPhysicsTarget->WorldSpaceCenter() + (vecDir * 128 );

			if ( g_debug_antlionguard.GetInt() != 0 )
			{
				NDebugOverlay::Cross3D( vecGoalPos, Vector(8,8,8), -Vector(8,8,8), 0, 255, 0, true, 2.0f );
				NDebugOverlay::Line( vecGoalPos, m_hPhysicsTarget->WorldSpaceCenter(), 0, 255, 0, true, 2.0f );
				NDebugOverlay::Line( m_hPhysicsTarget->WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), 0, 255, 0, true, 2.0f );
			}

			if ( GetNavigator()->SetGoal( vecGoalPos ) )
			{
				TaskComplete();
			}
			else
			{
				m_hPhysicsTarget = NULL;
				m_flPhysicsCheckTime = gpGlobals->curtime + 1.0f;
				TaskFail( "Unable to navigate to physics attack target!\n" );
				break;
			}
		}
		break;

	case TASK_ANTLIONGUARD_CHARGE:
		{
			SetActivity( (Activity) ACT_ANTLIONGUARD_CHARGE_START );
		}
		break;

	case TASK_ANTLIONGUARD_FACE_SHOVE_TARGET:
		
		if ( m_hShoveTarget == NULL )
		{
			TaskFail( FAIL_NO_ROUTE );
			return;
		}

		GetMotor()->SetIdealYaw( m_hShoveTarget->GetLocalAngles()[YAW] );

		break;

	case TASK_ANTLIONGUARD_CHECK_UNCOVER:
		{
			if ( GetEnemy() == NULL )
			{
				TaskComplete();
				return;
			}
			
			SetActivity( (Activity) ACT_ANTLIONGUARD_COVER_LOOP );

			if ( ( m_flCoverTime < gpGlobals->curtime ) || ( m_bCoverFromAttack == false ) )
			{
				TaskComplete();
				return;
			}

			float dist = ( GetLocalOrigin() - GetEnemy()->GetAbsOrigin() ).LengthSqr();

			if ( dist < (1024*1024) )
			{
				SetSchedule( SCHED_ANTLIONGUARD_COVER_ADVANCE );
			}
		}

		break;
	
	case TASK_ANTLIONGUARD_GET_PATH_TO_GUARDSPOT:
		{
			if( m_pHintNode )
			{
				Vector vecGoal;
				m_pHintNode->GetPosition(this,&vecGoal);

				AI_NavGoal_t goal( vecGoal );

				goal.pTarget = GetEnemy();
				
				if ( GetNavigator()->SetGoal( goal ) )
				{
					TaskComplete();
				}
				else
				{
					TaskFail( FAIL_NO_ROUTE );
				}
			}
			else
			{
				TaskFail( FAIL_NO_ROUTE );
			}
		}
		break;

	case TASK_ANTLIONGUARD_FACE_ENEMY_LKP:
		{
			Vector	vecFacing = ( GetEnemy()->GetAbsOrigin() - GetLocalOrigin() );
			VectorNormalize( vecFacing );

			GetMotor()->SetIdealYaw( vecFacing );
		}
		break;

	case TASK_ANTLIONGUARD_EMOTE_SEARCH:
		
		if ( ( GetEnemy() == NULL ) || ( FVisible( GetEnemy() ) ) )
		{
			TaskComplete();
			return;
		}
		
		if ( m_pSearchHint )
		{
			GetMotor()->SetIdealYaw( m_pSearchHint->Yaw() );
		}
		
		break;

	case TASK_ANTLIONGUARD_GET_SEARCH_PATH:
		{
			//Must have a valid enemy
			if ( GetEnemy() == NULL )
			{
				TaskFail( FAIL_NO_ENEMY );
				return;
			}

			//Continue or start our search for the enemy
			CHintCriteria	hintCriteria;

			hintCriteria.SetHintType( HINT_TACTICAL_PINCH );
			hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );

			//If we have a group, only search through it
			if ( GetHintGroup() != NULL_STRING )
			{	
				hintCriteria.SetGroup( GetHintGroup() );
			}
			else
			{
				//Search by position
				hintCriteria.AddIncludePosition( GetAbsOrigin(), ANTLIONGUARD_SEARCH_RADIUS );
			}

			//Attempt to find a node that meets our criteria
			CAI_Hint *pSearchHint = CAI_Hint::FindHint( this, &hintCriteria );

			//See if we found one
			if ( pSearchHint != NULL )
			{
				Vector vHintPos;
				pSearchHint->GetPosition( this, &vHintPos );

				AI_NavGoal_t goal( vHintPos, ACT_RUN, AIN_HULL_TOLERANCE, AIN_DEF_FLAGS, GetEnemy());

				//Try and navigate there
				if ( GetNavigator()->SetGoal( goal ) )
				{
					m_pLastSearchHint	= m_pSearchHint;
					m_pSearchHint		= pSearchHint;
					SetHintGroup( pSearchHint->GetGroup() );

					TaskComplete();
					return;
				}
			}

			//TODO: Attempt to just find a random path in the area, looking for the enemy
			SetSchedule( SCHED_PATROL_WALK );
			TaskComplete();
			
			return;
		}
		break;

	case TASK_ANTLIONGUARD_SET_HOP_ACTIVITY:
	{
		
		AI_NavGoal_t goal( GetEnemy()->GetAbsOrigin(), (Activity)ACT_ANTLIONGUARD_COVER_ADVANCE, 
						   AIN_HULL_TOLERANCE, AIN_DEF_FLAGS, GetEnemy());

		if ( !GetNavigator()->SetGoal( goal ) )
		{
			TaskFail( FAIL_NO_ROUTE );
			return;
		}

		TaskComplete();
		break;
	}

	case TASK_ANTLIONGUARD_SNIFF:
		{
			if ( DetectedHiddenEnemy() )
				return;

			ResetIdealActivity( (Activity) ACT_ANTLIONGUARD_PEEK1 );
		}
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::CheckChargeImpact( void )
{
	trace_t	tr;
	Vector	startPos, endPos;

	startPos = GetAbsOrigin();
	endPos = startPos + ( BodyDirection3D() * 85 );	//FIXME: For now

	Vector	mins, maxs;

	mins = GetHullMins();
	maxs = GetHullMaxs();

	// let him climb inclined surfaces
	int StepHeights = StepHeight();
	mins[2] += StepHeights * 3;

	AI_TraceHull( startPos, endPos, mins, maxs, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	
	if ( tr.fraction < 1.0f )
	{
		CBaseEntity *pHit = tr.m_pEnt;

		if ( ( pHit != NULL ) /*&& ( pHit->entindex() != 1 )*/ )
		{
			SoundTest( "NPC_AntlionGuard.Shove" );

			if ( pHit == GetEnemy() )
			{
				RestartGesture( ACT_ANTLIONGUARD_CHARGE_HIT );
				
				ChargeDamage( pHit );
				
				pHit->ApplyAbsVelocityImpulse( ( BodyDirection2D() * 400 ) + Vector( 0, 0, 250 ) );

				if ( pHit->IsAlive() == false )
				{
					SetEnemy( NULL );
				}

				SetActivity( (Activity) ACT_ANTLIONGUARD_CHARGE_STOP );
				return false;
			}
			// don't hit another similar beast??
			else if ( pHit->Classify() == Classify() )
			{
				SetActivity( (Activity) ACT_ANTLIONGUARD_CHARGE_STOP );
				return false;

			}

		}

		SetActivity( (Activity) ACT_ANTLIONGUARD_CHARGE_CRASH );
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_AntlionGuard::ChargeSteer( void )
{
	trace_t	tr;
	Vector	testPos, steer, forward, right;
	QAngle	angles;
	const float	testLength = m_flGroundSpeed * 0.35f;

	//Get our facing
	GetVectors( &forward, &right, NULL );

	steer = forward;

	const float faceYaw	= UTIL_VecToYaw( forward );

	//Offset right
	VectorAngles( forward, angles );
	angles[YAW] += 45.0f;
	AngleVectors( angles, &forward );

	//Probe out
	testPos = WorldSpaceCenter() + ( forward * testLength );

	//Offset by step height
	Vector testHullMins = GetHullMins();
	testHullMins.z += StepHeight();

	//Probe
	AI_TraceHull( WorldSpaceCenter(), testPos, testHullMins, GetHullMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	//Debug info
	if ( g_debug_antlionguard.GetInt() != 0 )
	{
		if ( tr.fraction == 1.0f )
		{
			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 0, 255, 0, 8, 0.1f );
		}
		else
		{
			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 255, 0, 0, 8, 0.1f );
		}
	}

	//Add in this component
	steer += ( right * 0.5f ) * ( 1.0f - tr.fraction );

	//Offset left
	angles[YAW] -= 90.0f;
	AngleVectors( angles, &forward );

	//Probe out
	testPos = WorldSpaceCenter() + ( forward * testLength );

	// Probe
	AI_TraceHull( WorldSpaceCenter(), testPos, testHullMins, GetHullMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	//Debug
	if ( g_debug_antlionguard.GetInt() != 0 )
	{
		if ( tr.fraction == 1.0f )
		{
			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 0, 255, 0, 8, 0.1f );
		}
		else
		{
			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 255, 0, 0, 8, 0.1f );
		}
	}

	//Add in this component
	steer -= ( right * 0.5f ) * ( 1.0f - tr.fraction );

	//Debug
	if ( g_debug_antlionguard.GetInt() != 0 )
	{
		NDebugOverlay::Line( WorldSpaceCenter(), WorldSpaceCenter() + ( steer * 512.0f ), 255, 255, 0, true, 0.1f );
		NDebugOverlay::Cross3D( WorldSpaceCenter() + ( steer * 512.0f ), Vector(2,2,2), -Vector(2,2,2), 255, 255, 0, true, 0.1f );

		NDebugOverlay::Line( WorldSpaceCenter(), WorldSpaceCenter() + ( BodyDirection3D() * 256.0f ), 255, 0, 255, true, 0.1f );
		NDebugOverlay::Cross3D( WorldSpaceCenter() + ( BodyDirection3D() * 256.0f ), Vector(2,2,2), -Vector(2,2,2), 255, 0, 255, true, 0.1f );
	}

	return UTIL_AngleDiff( UTIL_VecToYaw( steer ), faceYaw );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::RunTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT:
	
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLIONGUARD_CHARGE:
		{
			Activity eActivity = GetActivity();

			//See if we're done stopping
			if ( ( eActivity == ACT_ANTLIONGUARD_CHARGE_STOP ) && ( IsActivityFinished() ) )
			{
				TaskComplete();
				return;
			}

			//Check for manual transition
			if ( ( eActivity == ACT_ANTLIONGUARD_CHARGE_START ) && ( IsActivityFinished() ) )
			{
				SetActivity( (Activity) ACT_ANTLIONGUARD_CHARGE_RUN );
			}

			//See if we're still running
			if ( eActivity == ACT_ANTLIONGUARD_CHARGE_RUN ) 
			{
				if ( GetEnemy() == NULL )
				{
					SetActivity( (Activity) ACT_ANTLIONGUARD_CHARGE_STOP );
				}
				else
				{
					Vector	goalDir = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() );
					VectorNormalize( goalDir );

					if ( DotProduct( BodyDirection2D(), goalDir ) < 0.25f )
					{
						SetActivity( (Activity) ACT_ANTLIONGUARD_CHARGE_STOP );
					}
				}
			}

			float	flIntervalIgnored = 0.1f;
			bool	ignored;
			Vector	newPos;
			QAngle	newAngles;
			float	idealYaw;
			
			if ( GetEnemy() == NULL )
			{
				idealYaw = GetMotor()->GetIdealYaw();
			}
			else
			{
				idealYaw = CalcIdealYaw( GetEnemy()->GetAbsOrigin() );
			}

			idealYaw += ChargeSteer();
			
			GetMotor()->SetIdealYawAndUpdate( idealYaw );

			StudioFrameAdvance();
			GetIntervalMovement( flIntervalIgnored, ignored, newPos, newAngles );
			
			bool partial = ( eActivity == ACT_ANTLIONGUARD_CHARGE_STOP );

			//See if we can move
			if ( GetMotor()->MoveGroundStep( newPos, GetNavTargetEntity(), newAngles.y, partial ) == AIM_FAILED )
			{
				if ( partial == false )
				{
					if ( eActivity == ACT_ANTLIONGUARD_CHARGE_START )
					{
						TaskFail( "Unable to make initial movement of charge\n" );
						return;
					}

					SetSchedule( SCHED_ANTLIONGUARD_CHARGE_CRASH );
					TaskComplete();
				}
			}
		}
		break;

	case TASK_ANTLIONGUARD_FACE_SHOVE_TARGET:

		//If we're not facing the target, keep turning
		if ( FacingIdeal() == false )
		{
			GetMotor()->UpdateYaw();
			return;
		}

		TaskComplete();

		break;

	case TASK_ANTLIONGUARD_CHECK_UNCOVER:
		
		if ( ( m_flCoverTime < gpGlobals->curtime ) || ( m_bCoverFromAttack == false ) )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLIONGUARD_FACE_ENEMY_LKP:

		if ( FacingIdeal() == false )
		{
			GetMotor()->UpdateYaw();
			return;
		}

		TaskComplete();

		break;

	case TASK_ANTLIONGUARD_EMOTE_SEARCH:
		{
			//If we're not facing the target, keep turning
			if ( FacingIdeal() == false )
			{
				GetMotor()->UpdateYaw();
				return;
			}

			//Try and detect a hidden enemy
			if ( DetectedHiddenEnemy() )
				return;

			//Start to enter the seeking animation
			if ( GetActivity() != ACT_ANTLIONGUARD_PEEK_ENTER )
			{
				SetActivity( (Activity) ACT_ANTLIONGUARD_PEEK_ENTER );
				return;
			}
			
			//Wait until the animation is finished
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
		}
		
		break;

	case TASK_ANTLIONGUARD_SNIFF:
		{
			//Try and detect a hidden enemy
			if ( DetectedHiddenEnemy() )
				return;
			
			//Wait for the animation to finish
			if ( IsActivityFinished() )
			{
				TaskComplete();
				return;
			}
		}

		break;

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::FoundEnemy( void )
{
	m_flAngerNoiseTime = gpGlobals->curtime + 2.0f;
	SetState( NPC_STATE_COMBAT );

	m_bContinuedSearch	= false;
	m_bWatchEnemy		= true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::LostEnemy( void )
{
	m_flSearchNoiseTime = gpGlobals->curtime + 2.0f;
	SetState( NPC_STATE_ALERT );

	m_OnLostPlayer.FireOutput( this, this );

	//m_bWatchEnemy = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::GetSoundInterests( void )
{
	return (SOUND_WORLD|SOUND_COMBAT|SOUND_PLAYER|SOUND_DANGER);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputSetShoveTarget( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, inputdata.value.String(), inputdata.pActivator );

	if ( pTarget == NULL )
	{
		Warning( "**Guard %s cannot find shove target %s\n", GetClassname(), inputdata.value.String() );
		m_hShoveTarget = NULL;
		return;
	}

	m_hShoveTarget = pTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputSetChargeTarget( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, inputdata.value.String(), inputdata.pActivator );

	if ( pTarget == NULL )
	{
		Warning( "**Guard %s cannot find charge target %s\n", GetClassname(), inputdata.value.String() );
		m_hChargeTarget = NULL;
		return;
	}

	SetCondition( COND_ANTLIONGUARD_HAS_CHARGE_TARGET );
	m_hChargeTarget = pTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	if ( pEntity->GetFlags() & FL_NOTARGET )
		return false;

	// Don't look through water
	if ( ( GetWaterLevel() != 3 && pEntity->GetWaterLevel() == 3 ) || ( GetWaterLevel() == 3 && pEntity->GetWaterLevel() == 0 ) )
		return false;
	
	bool inPeekAnimation;

	if ( ( GetActivity() == ACT_ANTLIONGUARD_PEEK1 ) || ( GetActivity() == ACT_ANTLIONGUARD_PEEK_SIGHTED ) )
		inPeekAnimation = true;
	else
		inPeekAnimation = false;

	//See if we need to restrict the vision of the creature
	RestrictVision( inPeekAnimation );

	trace_t		tr;
	Vector		vecLookerOrigin;
	Vector		vecTargetOrigin;

	//Look at player with special rules
	if ( pEntity->IsPlayer() )
	{
		vecLookerOrigin = EyePosition();
		vecTargetOrigin = pEntity->EyePosition();

		//Must be within range
		if ( ( vecLookerOrigin - vecTargetOrigin ).Length() > GetSenses()->GetDistLook() )
			return false;

		AI_TraceLine( vecLookerOrigin, vecTargetOrigin, m_fLookMask, this, COLLISION_GROUP_NONE, &tr );

		if ((tr.fraction < 1.0f) && (ppBlocker))
		{
			*ppBlocker = tr.m_pEnt;
		}

		return ( tr.fraction == 1.0f );
	}

	return BaseClass::FVisible( pEntity, traceMask, ppBlocker );
}

//-----------------------------------------------------------------------------
// Purpose: Setup the sight restriction information
// Input  : state - restricted or not
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::RestrictVision( bool restrict )
{
	if ( restrict )
	{
		m_flFieldOfView = ANTLIONGUARD_FOV_RESTRICTED;
		SetDistLook( 512 );
		m_fLookMask		= (MASK_OPAQUE);
	}
	else
	{
		m_flFieldOfView = ANTLIONGUARD_FOV_NORMAL;
		SetDistLook( 2048 );
		m_fLookMask		= (MASK_OPAQUE|CONTENTS_MONSTERCLIP);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_AntlionGuard::EyePosition( void ) 
{ 
	if ( ( GetActivity() == ACT_ANTLIONGUARD_PEEK1 ) || ( GetActivity() == ACT_ANTLIONGUARD_PEEK_SIGHTED ) )
	{
		return GetAbsOrigin() + Vector( 0, 0, 16 );
	}

	return BaseClass::EyePosition(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::DetectedHiddenEnemy( void )
{
	if ( GetEnemy() == NULL )
		return false;

	if ( GetEnemy()->GetFlags() & FL_NOTARGET )
		return false;

	//Determine if we can see the enemy (we're pretty myopic when looking)
	if ( GetSenses()->ShouldSeeEntity( GetEnemy() ) && GetSenses()->CanSeeEntity( GetEnemy() ) )
	{
		SoundTest( "NPC_AntlionGuard.SniffFound" );

		m_OnSeeHiddenPlayer.FireOutput( this, this );

		FoundEnemy();

		//Only harass if we're not supposed to fire an output
		if ( m_OnSeeHiddenPlayer.NumberOfElements() != 0 )
		{
			SetSchedule( SCHED_ANTLIONGUARD_EXIT_PEEK );
		}
		else
		{
			m_bWatchEnemy = true;
			SetSchedule( SCHED_ANTLIONGUARD_EMOTE_SEEN_ENEMY );
		}

		return true;
	}

	float length = ( GetEnemy()->GetAbsOrigin() - GetLocalOrigin() ).LengthSqr();

	//If we're within sniffing distance, harass the player
	if ( length < ANTLIONGUARD_MAX_SNIFF_DIST_SQR )
	{
		SoundTest( "NPC_AntlionGuard.SniffFound" );

		m_OnSmellHiddenPlayer.FireOutput( this, this );

		//Only harass if we're not supposed to fire an output
		if ( m_OnSmellHiddenPlayer.NumberOfElements() != 0 )
		{
			SetSchedule( SCHED_ANTLIONGUARD_EXIT_PEEK );
		}
		else
		{
			SetSchedule( SCHED_ANTLIONGUARD_EXIT_PEEK );
		}
		
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : baseAct - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CNPC_AntlionGuard::NPC_TranslateActivity( Activity baseAct )
{
	//See which run to use
	if ( ( baseAct == ACT_RUN ) && IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) )
		return (Activity) ACT_ANTLIONGUARD_CHARGE_RUN;

	if ( ( baseAct == ACT_RUN ) && ( m_iHealth <= (m_iMaxHealth/4) ) )
		return (Activity) ACT_ANTLIONGUARD_RUN_HURT;

	return baseAct;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::UpdateHead( void )
{
	if ( ( m_bWatchEnemy ) && ( GetEnemy() != NULL ) )
	{
		float yaw = VecToYaw( GetEnemy()->GetAbsOrigin() - GetLocalOrigin() ) - GetLocalAngles().y;

		if ( yaw > 180 )	yaw -= 360;
		if ( yaw < -180 )	yaw += 360;

		//Turn towards vector
		SetBoneController( ANTLIONGUARD_NECK_YAW, yaw );
	}
	else
	{
		//Head bobble
		float yaw	= 10.0f * sin( gpGlobals->curtime * 4.0f );
		float pitch	= 15.0f * sin( gpGlobals->curtime * 2.0f );

		SetBoneController( ANTLIONGUARD_NECK_YAW, yaw );
		SetBoneController( ANTLIONGUARD_NECK_PITCH, pitch );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::UpdatePhysicsTarget( void )
{
	if ( GetEnemy() == NULL )
		return;

	if ( m_hPhysicsTarget != NULL )
	{
		SetCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		return;
	}

	if ( m_flPhysicsCheckTime > gpGlobals->curtime )
		return;

	m_hPhysicsTarget		= FindPhysicsObjectTarget( GetEnemy() );
	m_flPhysicsCheckTime	= gpGlobals->curtime + 0.5f;
}

//-----------------------------------------------------------------------------
// Purpose: Push and sweep away small mass items
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::SweepPhysicsDebris( void )
{
	CBaseEntity *pList[ANTLIONGUARD_MAX_OBJECTS];
	CBaseEntity	*pObject;
	IPhysicsObject *pPhysObj;
	Vector vecDelta(128,128,8);
	int	i;

	if ( g_debug_antlionguard.GetInt() != 0 )
	{
		NDebugOverlay::Box( GetAbsOrigin(), vecDelta, -vecDelta, 255, 0, 0, true, 0.1f );
	}

	int count = UTIL_EntitiesInBox( pList, ANTLIONGUARD_MAX_OBJECTS, GetAbsOrigin() - vecDelta, GetAbsOrigin() + vecDelta, 0 );

	for( i = 0, pObject = pList[0]; i < count; i++, pObject = pList[i] )
	{
		if ( pObject == NULL )
			continue;

		pPhysObj = pObject->VPhysicsGetObject();

		if( !pPhysObj || pPhysObj->GetMass() > ANTLIONGUARD_MAX_OBJECT_MASS || pPhysObj->IsAsleep() == false )
			continue;
	
		if ( FClassnameIs( pObject, "prop_physics" ) == false )
			continue;

		NavPropertyAdd( pObject, NAV_IGNORE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::PrescheduleThink( void )
{
	if ( ( m_hPhysicsTarget != NULL ) && ( g_debug_antlionguard.GetInt() != 0 ) )
	{
		NDebugOverlay::Cross3D( m_hPhysicsTarget->WorldSpaceCenter(), Vector(15,15,15), -Vector(15,15,15), 0, 255, 0, true, 0.1f );
	}

	SweepPhysicsDebris();
	UpdatePhysicsTarget();
	UpdateHead();

	//See if we should check for a wall collision
	Activity eActivity = GetActivity();

	if ( ( IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) || IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE_TARGET ) )&& 
		( ( eActivity == ACT_ANTLIONGUARD_CHARGE_START ) ||
		  ( eActivity == ACT_ANTLIONGUARD_CHARGE_RUN ) ) )
	{
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		m_flPhysicsCheckTime = gpGlobals->curtime + 1.0f;
		m_hPhysicsTarget = NULL;

		if ( ( eActivity == ACT_ANTLIONGUARD_CHARGE_RUN ) && ( CheckChargeImpact() ) )
		{
			//FIXME: Do with conditions
			SetSchedule( SCHED_ANTLIONGUARD_CHARGE_CRASH );
		}
	}

	if ( ( m_flGroundSpeed <= 0.0f ) )
	{
		if ( m_bStopped == false )
		{
			StartSounds();

			float duration = random->RandomFloat( 2.0f, 8.0f );

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, duration );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pBreathSound, random->RandomInt( 40, 60 ), duration );
			
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, duration );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pGrowlIdleSound, random->RandomInt( 120, 140 ), duration );

			m_flBreathTime = gpGlobals->curtime + duration - (duration*0.75f);
		}
		
		m_bStopped = true;

		if ( m_flBreathTime < gpGlobals->curtime )
		{
			StartSounds();

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, random->RandomFloat( 0.2f, 0.3f ), random->RandomFloat( 0.5f, 1.0f ) );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pGrowlIdleSound, random->RandomInt( 80, 120 ), random->RandomFloat( 0.5f, 1.0f ) );

			m_flBreathTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 8.0f );
		}
	}
	else
	{
		if ( m_bStopped ) 
		{
			StartSounds();

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.6f, random->RandomFloat( 2.0f, 4.0f ) );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pBreathSound, random->RandomInt( 140, 160 ), random->RandomFloat( 2.0f, 4.0f ) );

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 1.0f );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pGrowlIdleSound, random->RandomInt( 90, 110 ), 0.2f );
		}


		m_bStopped = false;
	}

	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: makes a sound from the script
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::SoundTest( const char *pSoundName )
{
	EmitSound( pSoundName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::DropBugBait( void )
{
	for ( int i = 0; i < 8; i++ )
	{
		CBaseCombatWeapon *pWeapon = Weapon_Create( "weapon_bugbait" );
		
		if ( pWeapon != NULL )
		{
			Weapon_Equip( pWeapon );
			Weapon_Drop( pWeapon );
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInflictor - 
//			*pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::Event_Killed( const CTakeDamageInfo &info )
{
	DropBugBait();

	BaseClass::Event_Killed( info );
}

void CNPC_AntlionGuard::StopLoopingSounds()
{
	//Stop all sounds
	ENVELOPE_CONTROLLER.SoundDestroy( m_pGrowlHighSound );
	ENVELOPE_CONTROLLER.SoundDestroy( m_pGrowlIdleSound );
	ENVELOPE_CONTROLLER.SoundDestroy( m_pBreathSound );
	ENVELOPE_CONTROLLER.SoundDestroy( m_pConfusedSound );
	m_pGrowlHighSound = NULL;
	m_pGrowlIdleSound = NULL;
	m_pBreathSound = NULL;
	m_pConfusedSound = NULL;

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputUnburrow( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	if ( m_bStartBurrowed == false )
		return;

	m_fEffects	&= ~EF_NODRAW;
	RemoveFlag( FL_NOTARGET );

	m_spawnflags &= ~SF_NPC_GAG;
	
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	
	m_takedamage = DAMAGE_YES;

	SetSchedule( SCHED_ANTLIONGUARD_UNBURROW );

	m_bStartBurrowed = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputSetCoverFromAttack( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	m_bCoverFromAttack = ( inputdata.value.Int() == 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_AntlionGuard::FindPhysicsObjectTarget( CBaseEntity *pTarget )
{
	CBaseEntity		*pList[ANTLIONGUARD_MAX_OBJECTS];
	CBaseEntity		*pNearest = NULL;
	CBaseEntity		*pObject;
	float			flNearestDist = ANTLIONGUARD_FARTHEST_PHYSICS_OBJECT;
	IPhysicsObject	*pPhysObj;
	int				i;

	Vector vecDirToTarget = pTarget->GetAbsOrigin() - GetAbsOrigin();
	VectorNormalize( vecDirToTarget );
	vecDirToTarget.z = 0;

	Vector vecDelta( flNearestDist, flNearestDist, GetHullMaxs().z );

	if ( g_debug_antlionguard.GetInt() != 0 )
	{
		NDebugOverlay::Box( GetAbsOrigin(), vecDelta, -vecDelta, 255, 255, 0, true, 1.0f );
	}

	int count = UTIL_EntitiesInBox( pList, ANTLIONGUARD_MAX_OBJECTS, (GetAbsOrigin()-vecDelta), (GetAbsOrigin()+vecDelta), 0 );

	//Consider all the objects near us
	for( i = 0, pObject = pList[0]; i < count; i++, pObject = pList[i] )
	{
		if ( pObject == NULL )
			continue;

		pPhysObj = pObject->VPhysicsGetObject();

		//FIXME: Non-sleeping objects can cause us trouble...
		if( ( pPhysObj == NULL ) || ( pPhysObj->GetMass() > ANTLIONGUARD_MAX_OBJECT_MASS ) || ( pPhysObj->GetMass() < ANTLIONGUARD_MIN_OBJECT_MASS )/*|| ( pPhysObj->IsAsleep() == false )*/ )
			continue;
	
		//FIXME: Need to make sure this is correct
		if ( FClassnameIs( pObject, "prop_physics" ) == false )
			continue;

		Vector center = pObject->WorldSpaceCenter();
		float flDist  = UTIL_DistApprox2D( GetAbsOrigin(), center );

		//Must be closer than a previously valid object
		if ( flDist > flNearestDist )
		{
			if( g_debug_antlionguard.GetInt() != 0 )
			{
				NDebugOverlay::Cross3D( center, Vector(15,15,15), -Vector(15,15,15), 255, 0, 0, true, 1.0f );
			}

			continue;
		}

		Vector vecDirToObject = pObject->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize( vecDirToObject );
		vecDirToObject.z = 0;

		//Validate our cone of sight
		if ( DotProduct( vecDirToTarget, vecDirToObject ) < 0.75f )
		{
			if ( g_debug_antlionguard.GetInt() != 0 )
			{
				NDebugOverlay::Cross3D( center, Vector(15,15,15), -Vector(15,15,15), 255, 0, 255, true, 1.0f );
			}

			continue;
		}

		//Object must be closer than our target
		if ( UTIL_DistApprox2D( GetAbsOrigin(), center ) > UTIL_DistApprox2D( GetAbsOrigin(), GetEnemy()->GetAbsOrigin() ) )
		{
			if ( g_debug_antlionguard.GetInt() != 0 )
			{
				NDebugOverlay::Cross3D( center, Vector(15,15,15), -Vector(15,15,15), 0, 255, 255, true, 1.0f );
			}

			continue;
		}

		trace_t	tr;

		//Check for a "clear" line of sight to our target
		AI_TraceLine( center, GetEnemy()->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( ( tr.fraction < 1.0f ) && ( tr.m_pEnt != GetEnemy() ) )
		{
			if ( g_debug_antlionguard.GetInt() != 0 )
			{
				NDebugOverlay::Line( center, GetEnemy()->WorldSpaceCenter(), 255, 0, 0, true, 1.0f );
			}
			
			continue;
		}

		Vector vecDirTrajectory = GetEnemy()->WorldSpaceCenter() - pObject->WorldSpaceCenter();
		VectorNormalize( vecDirTrajectory );

		//Check for a clear goal position behind the object
		AI_TraceLine( pObject->WorldSpaceCenter(), pObject->WorldSpaceCenter() + ( vecDirTrajectory * -128 ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction < 1.0f )
		{
			if ( g_debug_antlionguard.GetInt() != 0 )
			{
				NDebugOverlay::Line( pObject->WorldSpaceCenter(), pObject->WorldSpaceCenter() + ( vecDirTrajectory * -128 ), 255, 0, 0, true, 1.0f );
			}
		
			continue;
		}

		//Take this as the best object so far
		pNearest = pObject;
		flNearestDist = flDist;
		
		if ( g_debug_antlionguard.GetInt() != 0 )
		{
			NDebugOverlay::Cross3D( center, Vector(15,15,15), -Vector(15,15,15), 255, 255, 0, true, 0.5f );
		}
	}

	return pNearest;
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::BuildScheduleTestBits( void )
{
	if ( IsCurSchedule( SCHED_CHASE_ENEMY ) )
	{
		SetCustomInterruptCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
	}

	if ( IsCurSchedule( SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY ) == false )
	{
		SetCustomInterruptCondition( COND_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY );
	}

	if ( IsCurSchedule( SCHED_IDLE_STAND ) )
	{
		SetCustomInterruptCondition( COND_ANTLIONGUARD_HAS_CHARGE_TARGET );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &ownerPos - 
//			&targetPos - 
//			bSetConditions - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	if ( m_hChargeTarget == NULL )
		return false;

	Vector	targetDir = (targetPos-ownerPos);
	float	targetDist= VectorNormalize( targetDir );

	//Check for being to near
	if ( targetDist < ANTLIONGUARD_CHARGE_MIN )
	{
		if ( bSetConditions )
		{
			SetCondition( COND_TOO_CLOSE_TO_ATTACK );
		}

		return false;
	}

	//Check for being too far
	if ( targetDist > ANTLIONGUARD_CHARGE_MAX )
	{
		if ( bSetConditions )
		{
			SetCondition( COND_TOO_FAR_TO_ATTACK );
		}

		return false;
	}
	
	return ShouldCharge( ownerPos, targetPos, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::ChargeDamage( CBaseEntity *pTarget )
{
	if ( pTarget == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pTarget );

	if ( pPlayer != NULL )
	{
		//Kick the player angles
		pPlayer->ViewPunch( QAngle( 20, 20, -30 ) );	

		Vector	dir = pPlayer->WorldSpaceCenter() - WorldSpaceCenter();
		VectorNormalize( dir );
		dir.z = 0.0f;
		
		Vector vecNewVelocity = dir * 250.0f;
		vecNewVelocity[2] += 128.0f;
		pPlayer->SetAbsVelocity( vecNewVelocity );

		color32 red = {128,0,0,128};
		UTIL_ScreenFade( pPlayer, red, 1.0f, 0.1f, FFADE_IN );
	}
	
	//Player takes less damage
	float	flDamage = ( pPlayer == NULL ) ? 250 : sk_antlionguard_dmg_charge.GetFloat();
	
	// Calculate the physics force
	Vector attackDir = ( pTarget->WorldSpaceCenter() - WorldSpaceCenter() );
	VectorNormalize( attackDir );
	Vector offset = RandomVector( -32, 32 ) + pTarget->WorldSpaceCenter();

	// Generate enough force to make a 75kg guy move away at 700 in/sec
	Vector vecForce = attackDir * ImpulseScale( 75, 700 );

	// Deal the damage
	CTakeDamageInfo	info( this, this, vecForce, offset, flDamage, DMG_CLUB );
	pTarget->TakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputRagdoll( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	//Set us to nearly dead so the velocity from death is minimal
	SetHealth( 1 );
	m_bAllowKillDamage = true;

	CTakeDamageInfo info( this, this, GetHealth(), DMG_CRUSH );
	BaseClass::TakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scheduleType - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_MELEE_ATTACK1:
		return SCHED_ANTLIONGUARD_MELEE_ATTACK1;
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::StartSounds( void )
{
	//Initialize the additive sound channels
	CPASAttenuationFilter filter( this );

	if ( m_pGrowlHighSound == NULL )
	{
		m_pGrowlHighSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_VOICE,	"npc/antlion_guard/growl_high.wav",	ATTN_NORM );
		
		if ( m_pGrowlHighSound )
		{
			ENVELOPE_CONTROLLER.Play( m_pGrowlHighSound,0.0f, 100 );
		}
	}

	if ( m_pGrowlIdleSound == NULL )
	{
		m_pGrowlIdleSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_STATIC,	"npc/antlion_guard/growl_idle.wav",	ATTN_NORM );

		if ( m_pGrowlIdleSound )
		{
			ENVELOPE_CONTROLLER.Play( m_pGrowlIdleSound,0.0f, 100 );
		}
	}

	if ( m_pBreathSound == NULL )
	{
		m_pBreathSound	= ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_ITEM,	"npc/antlion_guard/breath.wav",		ATTN_NORM );
		
		if ( m_pBreathSound )
		{
			ENVELOPE_CONTROLLER.Play( m_pBreathSound,	0.0f, 100 );
		}
	}

	if ( m_pConfusedSound == NULL )
	{
		m_pConfusedSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_WEAPON,	"npc/antlion_guard/confused1.wav",	ATTN_NORM );

		if ( m_pConfusedSound )
		{
			ENVELOPE_CONTROLLER.Play( m_pConfusedSound,	0.0f, 100 );
		}
	}

}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_antlionguard, CNPC_AntlionGuard )

	//Tasks
	DECLARE_TASK( TASK_ANTLIONGUARD_CHECK_UNCOVER )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_PATH_TO_GUARDSPOT )
	DECLARE_TASK( TASK_ANTLIONGUARD_FACE_ENEMY_LKP )
	DECLARE_TASK( TASK_ANTLIONGUARD_EMOTE_SEARCH )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_SEARCH_PATH )
	DECLARE_TASK( TASK_ANTLIONGUARD_SET_HOP_ACTIVITY )
	DECLARE_TASK( TASK_ANTLIONGUARD_SNIFF )
	DECLARE_TASK( TASK_ANTLIONGUARD_FACE_SHOVE_TARGET )
	DECLARE_TASK( TASK_ANTLIONGUARD_CHARGE )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT )
	DECLARE_TASK( TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT )

	//Activities
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_SEARCH )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_COVER_ENTER )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_COVER_LOOP )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_COVER_EXIT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_COVER_ADVANCE )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_COVER_FLINCH )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_FLINCH )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_ENTER )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_EXIT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK1 )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PAIN )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_BARK )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_RUN_FULL )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_SNEAK )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_SIGHTED )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_START )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_CANCEL )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_RUN )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_CRASH )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_STOP )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_HIT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PHYSICS_FLINCH_HEAVY )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_FLINCH_LIGHT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_UNBURROW )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_ROAR )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_RUN_HURT )

	DECLARE_CONDITION( COND_ANTLIONGUARD_PHYSICS_TARGET )
	DECLARE_CONDITION( COND_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY )
	DECLARE_CONDITION( COND_ANTLIONGUARD_PHYSICS_DAMAGE_LIGHT )
	DECLARE_CONDITION( COND_ANTLIONGUARD_HAS_CHARGE_TARGET )

	//==================================================
	// SCHED_ANTLIONGUARD_LOST
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_LOST,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_ANTLIONGUARD_BARK"
		"	"
		"	Interrupts"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_COVER_LOOP
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_COVER_LOOP,

		"	Tasks"
		"		TASK_ANTLIONGUARD_CHECK_UNCOVER		2"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ANTLIONGUARD_COVER_EXIT"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_CAN_MELEE_ATTACK1"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_COVER
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_COVER,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ANTLIONGUARD_COVER_ENTER"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLIONGUARD_COVER_LOOP"
		"	"
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)


	//==================================================
	// SCHED_ANTLIONGUARD_RUN_TO_GUARDSPOT
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_RUN_TO_GUARDSPOT,

		"	Tasks"
		"		TASK_ANTLIONGUARD_GET_PATH_TO_GUARDSPOT	0"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"	"
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
		"		COND_ANTLIONGUARD_HAS_CHARGE_TARGET"
	)


	//==================================================
	// SCHED_ANTLIONGUARD_ATTACK_ENEMY_LKP
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_ATTACK_ENEMY_LKP,

		"	Tasks"
		"		TASK_ANTLIONGUARD_FACE_ENEMY_LKP		0"
		//"		TASK_SET_SCHEDULE						SCHEDULE:SCHED_ANTLIONGUARD_RANGE1"
		"		"
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
	)


	//==================================================
	// SCHED_ANTLIONGUARD_RUN_TO_ENEMY_LKP_LOS
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_RUN_TO_ENEMY_LKP_LOS,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLIONGUARD_SEEK_ENEMY"
		"		TASK_SET_GOAL						GOAL:ENEMY_LKP"
		"		TASK_GET_PATH_TO_GOAL				PATH:LOS"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_ANTLIONGUARD_FACE_ENEMY_LKP	0"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_ANTLIONGUARD_HAS_CHARGE_TARGET"
	)


	//==================================================
	// SCHED_ANTLIONGUARD_RUN_TO_ENEMY_LKP
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_RUN_TO_ENEMY_LKP,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLIONGUARD_RUN_TO_ENEMY_LKP_LOS"
		"		TASK_SET_GOAL						GOAL:ENEMY_LKP"
		"		TASK_GET_PATH_TO_GOAL				PATH:TRAVEL"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_ANTLIONGUARD_FACE_ENEMY_LKP	0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLIONGUARD_SEEK_ENEMY"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
		"		COND_SEE_ENEMY"
		"		COND_NEW_ENEMY"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_ANTLIONGUARD_HAS_CHARGE_TARGET"
	)


	//==================================================
	// SCHED_ANTLIONGUARD_SEEK_ENEMY_PEEK
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_SEEK_ENEMY_PEEK,

		"	Tasks"
		"		TASK_ANTLIONGUARD_EMOTE_SEARCH		0"
		"		TASK_ANTLIONGUARD_SNIFF				0"
		"		TASK_ANTLIONGUARD_SNIFF				0"
		"		TASK_ANTLIONGUARD_SNIFF				0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_ANTLIONGUARD_PEEK_EXIT"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_SEEK_ENEMY_INITIAL
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_SEEK_ENEMY_INITIAL,

		"	Tasks"
		//"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLIONGUARD_ATTACK_ENEMY_LKP"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLIONGUARD_LOST"
		"		TASK_ANTLIONGUARD_GET_SEARCH_PATH	0"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_ANTLIONGUARD_SEARCH"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLIONGUARD_SEEK_ENEMY"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_CAN_MELEE_ATTACK1"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_SEEK_ENEMY
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_SEEK_ENEMY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLIONGUARD_LOST"
		"		TASK_ANTLIONGUARD_GET_SEARCH_PATH	0"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLIONGUARD_SEEK_ENEMY_PEEK"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_ANTLIONGUARD_HAS_CHARGE_TARGET"
	)
		
	//=========================================================
	// SCHED_ANTLIONGUARD_CHASE_ENEMY
	//=========================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHASE_ENEMY,

		"	Tasks"
		"		TASK_SET_GOAL					GOAL:ENEMY"
		"		TASK_GET_PATH_TO_GOAL			PATH:TRAVEL"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_GIVE_WAY"
		"		COND_TASK_FAILED"
		"		COND_ANTLIONGUARD_PHYSICS_TARGET"
		"		COND_ANTLIONGUARD_HAS_CHARGE_TARGET"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_COVER_ADVANCE
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_COVER_ADVANCE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLIONGUARD_COVER_LOOP"
		"		TASK_ANTLIONGUARD_SET_HOP_ACTIVITY	0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLIONGUARD_COVER_LOOP"
		"	"
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_OCCLUDED"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_PEEK_FLINCH
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_PEEK_FLINCH,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_ANTLIONGUARD_PEEK_FLINCH"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ANTLIONGUARD_EXIT_PEEK"
		"	"
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_PAIN
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_PAIN,

		"	Tasks"
		"		TASK_STOP_MOVING	0"
		"		TASK_PLAY_SEQUENCE	ACTIVITY:ACT_ANTLIONGUARD_PAIN"
		"		TASK_SET_SCHEDULE	SCHEDULE:SCHED_ANTLIONGUARD_COVER"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)
		
	//==================================================
	// SCHED_ANTLIONGUARD_ATTACK_SHOVE_TARGET
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_ATTACK_SHOVE_TARGET,

		"	Tasks"
		"		TASK_SET_GOAL						GOAL:SAVED_POSITION"
		"		TASK_GET_PATH_TO_GOAL				PATH:TRAVEL"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_ANTLIONGUARD_FACE_SHOVE_TARGET	0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_MELEE_ATTACK1"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
		//"		COND_ENEMY_DEAD"
		//"		COND_NEW_ENEMY"
		//"		COND_CAN_MELEE_ATTACK1"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_EMOTE_SEEN_ENEMY
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_EMOTE_SEEN_ENEMY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_ANTLIONGUARD_SEEK_ENEMY"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_PEEK_SIGHTED"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_PEEK_EXIT"
		"	"
		"	Interrupts"
		"		COND_ENEMY_OCCLUDED"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_EXIT_PEEK
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_EXIT_PEEK,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_PEEK_EXIT"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY						0"
		"		TASK_ANTLIONGUARD_CHARGE			0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE_TARGET
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE_TARGET,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY						0"
		"		TASK_ANTLIONGUARD_CHARGE			0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE_SMASH
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE_CRASH,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_CHARGE_CRASH"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_PHYSICS_ATTACK
	//==================================================

	DEFINE_SCHEDULE
	( 
		SCHED_ANTLIONGUARD_PHYSICS_ATTACK,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT	0"
		"		TASK_RUN_PATH								0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_FACE_ENEMY								0"
	//	"		TASK_PLAY_SEQUENCE							ACTIVITY:ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT"
		"		TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT			0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
		"		COND_ANTLIONGUARD_HAS_CHARGE_TARGET"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_PHYSICS_FLINCH_HEAVY"
	//	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_ANTLIONGUARD_CHARGE"
		""
		"	Interrupts"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_UNBURROW
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_UNBURROW,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_UNBURROW"
		""
		"	Interrupts"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE_CANCEL
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE_CANCEL,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_CHARGE_CANCEL"
		""
		"	Interrupts"
	)
	
	//==================================================
	// SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION,

		"	Tasks"
		"		TASK_GET_PATH_TO_ENEMY_LOS		0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_GIVE_WAY"
		"		COND_TASK_FAILED"
	)

	//=========================================================
	// > Melee_Attack1
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_MELEE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"		TASK_MELEE_ATTACK1		0"
		""
		"	Interrupts"
		"		COND_ENEMY_OCCLUDED"
	)

AI_END_CUSTOM_NPC()
