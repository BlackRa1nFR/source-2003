//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "baseanimating.h"
#include "AI_Network.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Node.h"
#include "AI_Task.h"
#include "AI_Motor.h"
#include "entitylist.h"
#include "basecombatweapon.h"
#include "soundenvelope.h"
#include "gib.h"
#include "gamerules.h"
#include "ammodef.h"
#include "CBaseHelicopter.h"
#include "npcevent.h"
#include "ndebugoverlay.h"
#include "decals.h"
#include "explode.h" // temp (sjb)
#include "smoke_trail.h" // temp (sjb)
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "ar2_explosion.h"
#include "te_effect_dispatch.h"
#include "effect_dispatch_data.h"
#include "trains.h"
#include "globals.h"
#include "physics_prop_ragdoll.h"
#include "iservervehicle.h"
#include "soundent.h"

#define GUNSHIP_MSG_BIG_SHOT			1
#define GUNSHIP_MSG_STREAKS				2

extern short		g_sModelIndexFireball;		// holds the index for the fireball

#define GUNSHIP_ACCEL_RATE 500

#define SF_GUNSHIP_NO_GROUND_ATTACK	( 1 << 12 )	

ConVar flFlareTime( "hack_flaretime", "6" );
ConVar flRotorVolume( "hack_rotorvolume", "1" );
ConVar flIntakeVolume( "hack_intakevolume", "1" );
ConVar flExhaustVolume( "hack_exhaustvolume", "1" );
ConVar flStitchResetTime( "gunship_hide_time", "3" );
ConVar flGunshipCrashDist("gunship_crashdist", "4000" );

ConVar flGunshipBurstSize("sk_gunship_burst_size", "15" );
ConVar flGunshipBurstMiss("sk_gunship_burst_miss", "15" );
ConVar flGunshipBurstMin("sk_gunship_burst_min", "800" );
ConVar flGunshipBurstDist("sk_gunship_burst_dist", "1048" );
//ConVar flGunshipBurstDelay("gunship_burst_delay", "2" );

ConVar gunshipcrash( "gc", "gunship_crash" ); // E3 2003 (sjb)

/*

Wedge's notes:

  Gunship should move its head according to flight model when the target is behind the gunship,
  or when the target is too far away to shoot at. Otherwise, the head should aim at the target.

	Negative angvelocity.y is a RIGHT turn.
	Negative angvelocity.x is UP

*/

#define GUNSHIP_AP_MUZZLE	5

// * * * * * * * * * * * 
// These aren't working properly yet!!!
// check the base helicopter code.
// what does m_iMaxSpeed really do?
// * * * * * * * * * * * 
#define GUNSHIP_MAX_SPEED			400.0f
#define GUNSHIP_MIN_SPEED			200.0f
#define GUNSHIP_THRUST_SPEED		500.0f
#define GUNSHIP_DECAY_SPEED			5.0f

#define GUNSHIP_MAX_FIRING_SPEED	200.0f
#define GUNSHIP_MIN_ROCKET_DIST		1000.0f
#define GUNSHIP_MAX_GUN_DIST		2000.0f

#define GUNSHIP_HOVER_SPEED			300.0f // play hover animation if moving slower than this.

#define GUNSHIP_AE_THRUST			1

#define GUNSHIP_HEAD_MAX_UP			-65
#define GUNSHIP_HEAD_MAX_DOWN		60
#define GUNSHIP_HEAD_MAX_LEFT		60
#define GUNSHIP_HEAD_MAX_RIGHT		-60

#define	BASE_STITCH_VELOCITY		800		//Units per second
#define	MAX_STITCH_VELOCITY			1000	//Units per second

#define	GUNSHIP_LEAD_DISTANCE		800.0f
#define	GUNSHIP_AVOID_DIST			512.0f
#define	GUNSHIP_STITCH_MIN			512.0f

#define	GUNSHIP_MIN_CHASE_DIST_DIFF	128.0f	// Distance threshold used to determine when a target has moved enough to update our navigation to it

#define	MIN_GROUND_ATTACK_DIST			500.0f // Minimum distance a target has to be for the gunship to consider using the ground attack weapon
#define	MIN_GROUND_ATTACK_HEIGHT_DIFF	128.0f // Target's position and hit position must be within this threshold vertically

#define GUNSHIP_WASH_ALTITUDE		1024.0f

//=====================================
// Custom activities
//=====================================
Activity ACT_GUNSHIP_PATROL;
Activity ACT_GUNSHIP_HOVER;
Activity ACT_GUNSHIP_CRASH;

#define	GUNSHIP_DEBUG_LEADING	1
#define	GUNSHIP_DEBUG_PATH		2
#define	GUNSHIP_DEBUG_STITCHING	3

ConVar g_debug_gunship( "g_debug_gunship", "0", FCVAR_CHEAT );

//=============================================================================
//=============================================================================
class CGunshipCrashFX : public CPointEntity
{
public:
	DECLARE_CLASS( CGunshipCrashFX, CPointEntity );

	void Spawn( void );
	void FXThink( void );

	float m_flStopExplodeTime; // no more explosions after this time.

	DECLARE_DATADESC();
};
LINK_ENTITY_TO_CLASS( gunshipcrashfx, CGunshipCrashFX );

BEGIN_DATADESC( CGunshipCrashFX )
	// Function Pointers
	DEFINE_FIELD( CGunshipCrashFX, m_flStopExplodeTime, FIELD_TIME ),

	DEFINE_ENTITYFUNC( CGunshipCrashFX, FXThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGunshipCrashFX::Spawn( void )
{
	BaseClass::Spawn();

	SetThink( FXThink );

	m_flStopExplodeTime = gpGlobals->curtime + 0.5f;

	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGunshipCrashFX::FXThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.5f );

	if( GetOwnerEntity() )
	{
		AR2Explosion *pExplosion = AR2Explosion::CreateAR2Explosion( GetOwnerEntity()->GetAbsOrigin() );

		if (pExplosion)
		{
			pExplosion->SetLifetime( 10 );
		}

		Vector vecVelocity( 0, 0, 0 );

		if ( GetOwnerEntity()->VPhysicsGetObject() )
		{
			GetOwnerEntity()->VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );

			if( m_flStopExplodeTime > gpGlobals->curtime )
			{
				if( random->RandomInt( 0, 100 ) < 65 )
				{
					Vector vecDelta;

					vecDelta.x = random->RandomFloat( -128, 128 );
					vecDelta.y = random->RandomFloat( -128, 128 );
					vecDelta.z = random->RandomFloat( -128, 128 );

					ExplosionCreate( GetOwnerEntity()->GetAbsOrigin() + vecDelta, QAngle( -90, 0, 0 ), this, 200, 200, false );
				}
			}
			
			if( vecVelocity.Length() < 100.0 )
			{
				SetThink( SUB_Remove );
			}
		}
	}
}

//===================================================================
// Gunship - the combine dugongic like attack vehicle.
//===================================================================
class CNPC_CombineGunship : public CBaseHelicopter
{
public:
	DECLARE_CLASS( CNPC_CombineGunship, CBaseHelicopter );

	CNPC_CombineGunship( void );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	DEFINE_CUSTOM_AI;

	void	PlayPatrolLoop( void );
	void	PlayAngryLoop( void );

	void	Spawn( void );
	void	Precache( void );
	void	PrescheduleThink( void );
	void	HelicopterPostThink( void );
	void	DyingThink( void );
	void	StopLoopingSounds( void );

	bool	IsValidEnemy( CBaseEntity *pEnemy );

	void	Flight( void );

	bool	FVisible( CBaseEntity *pEntity, int traceMask = MASK_OPAQUE, CBaseEntity **ppBlocker = NULL );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );

	void	ExplodeTouch( CBaseEntity *pOther );

	virtual float GetAcceleration( void ) { return 15; }

	void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	void	DoCustomImpactEffect( const trace_t &tr );

	void	MoveHead( void );
	void	DoCombat( void );
	void	Ping( void );

	void	FireCannonRound( void );
	virtual void DoRotorWash( void );

	void	BeginCrash( void ); // start flying towards crash spot.
	
	int		BloodColor( void ) { return DONT_BLEED; }
	void	GibMonster( void );

	void	Event_Killed( const CTakeDamageInfo &info );

	void	UpdateRotorSoundPitch( int iPitch );
	void	InitializeRotorSound( void );

	void	ApplyGeneralDrag( void );
	void	ApplySidewaysDrag( const Vector &vecRight );

	float	GetRotorVolume( void ) { return flRotorVolume.GetFloat(); }
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	void	UpdateEnemyTarget( void );
	void	UpdateVehicularEnemyTarget( void );

	Vector	GetEnemyTarget( void );
	Vector	GetMissileTarget( void );

	float	GroundDistToPosition( const Vector &pos );

	bool	FireGun( void );
	bool	IsTargettingMissile( void );
	
	// Pathing
	virtual void UpdateTrackNavigation( void );
	virtual void UpdateDesiredPosition( void );

	Class_T Classify( void ) { return CLASS_COMBINE_GUNSHIP; } // for now

	~CNPC_CombineGunship();

	// Input functions
	void	InputSetPenetrationDepth( inputdata_t &inputdata );
	void	InputOmniscientOn( inputdata_t &inputdata );
	void	InputOmniscientOff( inputdata_t &inputdata );
	void	InputBlindfireOn( inputdata_t &inputdata );
	void	InputBlindfireOff( inputdata_t &inputdata );
	void	InputSelfDestruct( inputdata_t &inputdata );
	void	InputSetDockingBBox( inputdata_t &inputdata );
	void	InputSetNormalBBox( inputdata_t &inputdata );
	void	InputEnableGroundAttack( inputdata_t &inputdata );
	void	InputDisableGroundAttack( inputdata_t &inputdata );
	void	InputDoGroundAttack( inputdata_t &inputdata );

	void	StartCannonBurst( int iBurstSize );
	void	StopCannonBurst( void );
	void	SelfDestruct( void );

	bool	CheckGroundAttack( void );
	void	StartGroundAttack( void );
	void	StopGroundAttack( void );
	Vector	GetGroundAttackHitPosition( void );

	// Outputs
	COutputEvent	m_OnFireCannon;

	COutputEvent	m_OnFirstDamage;	// First damage tick
	COutputEvent	m_OnSecondDamage;
	COutputEvent	m_OnThirdDamage;
	COutputEvent	m_OnFourthDamage;

	float	m_flNextGroundAttack;	// Time to wait before the next ground attack
	bool	m_bIsGroundAttacking;	// Denotes that we are ground attacking
	bool	m_bCanGroundAttack;		// Denotes whether we can ground attack or not
	float	m_flGroundAttackTime;	// Delay before blast happens from ground attack

	CHandle<SmokeTrail>	m_pSmokeTrail;

	CSoundPatch		*m_pAirIntakeSound;
	CSoundPatch		*m_pAirExhaustSound;
	CSoundPatch		*m_pAirBlastSound;
	CSoundPatch		*m_pCannonSound;

	QAngle			m_vecAngAcceleration;

	float			m_flRotorPitch;
	float			m_flRotorVolume;
	float			m_flDeltaT;
	float			m_flTimeNextAttack;
	float			m_flBeginCrashTime;
	float			m_flNextRocket;
	int				m_iDoSmokePuff;
	int				m_iSpriteTexture;
	int				m_iAmmoType;
	int				m_iBurstSize;
	float			m_flBurstDelay;

	bool			m_fBlindfire;
	bool			m_fOmniscient;
	bool			m_bIsFiring;
	bool			m_bBurstHit;
	bool			m_bPreFire;
	bool			m_bNoiseyShot;

	int				m_nDebrisModel;
	float			m_flTimeNextPing;
	float			m_flPenetrationDepth; 
	float			m_flTargetPersistenceTime;

	Vector			m_vecAttackPosition;
	Vector			m_vecAttackVelocity;

	int				m_iTracerTexture;

	CNetworkVector( m_vecHitPos );

	// If true, playing patrol loop.
	// Else, playing angry.
	bool			m_fPatrolLoopPlaying;
};

LINK_ENTITY_TO_CLASS( npc_combinegunship, CNPC_CombineGunship );

IMPLEMENT_SERVERCLASS_ST( CNPC_CombineGunship, DT_CombineGunship )
	SendPropVector(SENDINFO(m_vecHitPos), -1, SPROP_COORD),
END_SEND_TABLE()

BEGIN_DATADESC( CNPC_CombineGunship )

	DEFINE_ENTITYFUNC( CNPC_CombineGunship, FlyTouch ),
	DEFINE_ENTITYFUNC( CNPC_CombineGunship, ExplodeTouch ),

	DEFINE_FIELD( CNPC_CombineGunship, m_flNextGroundAttack,FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineGunship, m_bIsGroundAttacking,FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_CombineGunship, m_bCanGroundAttack,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_CombineGunship, m_flGroundAttackTime,FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineGunship, m_flTargetPersistenceTime,FIELD_FLOAT ),

	DEFINE_FIELD( CNPC_CombineGunship, m_pSmokeTrail,		FIELD_EHANDLE ),
	DEFINE_SOUNDPATCH( CNPC_CombineGunship, m_pAirIntakeSound ),
	DEFINE_SOUNDPATCH( CNPC_CombineGunship, m_pAirExhaustSound ),
	DEFINE_SOUNDPATCH( CNPC_CombineGunship, m_pAirBlastSound ),
	DEFINE_SOUNDPATCH( CNPC_CombineGunship, m_pCannonSound ),
	DEFINE_FIELD( CNPC_CombineGunship, m_vecAngAcceleration,FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_CombineGunship, m_flRotorPitch,		FIELD_FLOAT),
	DEFINE_FIELD( CNPC_CombineGunship, m_flRotorVolume,		FIELD_FLOAT),
	DEFINE_FIELD( CNPC_CombineGunship, m_flDeltaT,			FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineGunship, m_flTimeNextAttack,	FIELD_TIME ),
	DEFINE_FIELD( CNPC_CombineGunship, m_flBeginCrashTime,	FIELD_TIME ),
	DEFINE_FIELD( CNPC_CombineGunship, m_flNextRocket,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_CombineGunship, m_iDoSmokePuff,		FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineGunship, m_iAmmoType,			FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineGunship, m_iBurstSize,		FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineGunship, m_flBurstDelay,		FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineGunship, m_fBlindfire,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_CombineGunship, m_fOmniscient,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_CombineGunship, m_bIsFiring,			FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_CombineGunship, m_bBurstHit,			FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_CombineGunship, m_nDebrisModel,		FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineGunship, m_flTimeNextPing,	FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineGunship, m_flPenetrationDepth,FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CombineGunship, m_vecAttackPosition,	FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_CombineGunship, m_vecAttackVelocity,	FIELD_VECTOR ),
	// Don't save these, they are model indices
	//DEFINE_FIELD( CNPC_CombineGunship, m_iSpriteTexture,	FIELD_INTEGER ),
	//DEFINE_FIELD( CNPC_CombineGunship, m_iTracerTexture,	FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CombineGunship, m_vecHitPos,			FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_CombineGunship, m_fPatrolLoopPlaying,FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_CombineGunship, m_bNoiseyShot,		FIELD_BOOLEAN ),

	// Function pointers
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_VOID, "OmniscientOn", InputOmniscientOn ),
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_VOID, "OmniscientOff", InputOmniscientOff ),
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_FLOAT, "SetPenetrationDepth", InputSetPenetrationDepth ),
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_VOID, "BlindfireOn", InputBlindfireOn ),
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_VOID, "BlindfireOff", InputBlindfireOff ),
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_VOID, "SelfDestruct", InputSelfDestruct ),
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_VOID, "SetDockingBBox", InputSetDockingBBox ),
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_VOID, "SetNormalBBox", InputSetNormalBBox ),
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_VOID, "EnableGroundAttack", InputEnableGroundAttack ),
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_VOID, "DisableGroundAttack", InputDisableGroundAttack ),
	DEFINE_INPUTFUNC( CNPC_CombineGunship, FIELD_VOID, "DoGroundAttack", InputDoGroundAttack ),

	DEFINE_OUTPUT( CNPC_CombineGunship, m_OnFireCannon,		"OnFireCannon" ),
	DEFINE_OUTPUT( CNPC_CombineGunship, m_OnFirstDamage,	"OnFirstDamage" ),
	DEFINE_OUTPUT( CNPC_CombineGunship, m_OnSecondDamage,	"OnSecondDamage" ),
	DEFINE_OUTPUT( CNPC_CombineGunship, m_OnThirdDamage,	"OnThirdDamage" ),
	DEFINE_OUTPUT( CNPC_CombineGunship, m_OnFourthDamage,	"OnFourthDamage" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CNPC_CombineGunship::CNPC_CombineGunship( void )
{ 
	m_pSmokeTrail	= NULL;
	m_iAmmoType		= -1; 
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CombineGunship::Spawn( void )
{
	Precache( );

	SetModel( "models/gunship.mdl" );
	ExtractBbox( SelectHeaviestSequence( ACT_GUNSHIP_PATROL ), m_cullBoxMins, m_cullBoxMaxs ); 
	BaseClass::Spawn();

	InitPathingData( GUNSHIP_LEAD_DISTANCE, GUNSHIP_MIN_CHASE_DIST_DIFF, flGunshipBurstMin.GetFloat() );

	m_takedamage = DAMAGE_YES;

	SetHullType(HULL_LARGE_CENTERED);
	SetHullSizeNormal();

	m_iMaxHealth = m_iHealth = 100;

	m_flFieldOfView = -0.707; // 270 degrees

	m_fHelicopterFlags |= BITS_HELICOPTER_GUN_ON;

	InitBoneControllers();

	InitCustomSchedules();

	SetActivity( (Activity)ACT_GUNSHIP_PATROL );

	m_flMaxSpeed = BASECHOPPER_MAX_SPEED;
	m_flMaxSpeedFiring = BASECHOPPER_MAX_FIRING_SPEED;

	m_flTimeNextAttack = gpGlobals->curtime;

	// Init the pose parameters
	SetPoseParameter( "flex_horz", 0 );
	SetPoseParameter( "flex_vert", 0 );
	SetPoseParameter( "fin_accel", 0 );
	SetPoseParameter( "fin_sway", 0 );

	if( m_iAmmoType == -1 )
	{
		// Since there's no weapon to index the ammo type,
		// do it manually here.
		m_iAmmoType = GetAmmoDef()->Index("CombineCannon"); 
	}

	//!!!HACKHACK
	// This tricks the AI code that constantly complains that the gunship has no schedule.
	SetSchedule( SCHED_IDLE_STAND );

	AddRelationship( "env_flare D_LI 9",	NULL );
	AddRelationship( "rpg_missile D_HT 99", NULL );

	m_flTimeNextPing = gpGlobals->curtime + 2;

	m_flPenetrationDepth = 24;
	m_flBurstDelay = 2.0f;

	// Blindfire and Omniscience default to off
	m_fBlindfire		= false;
	m_fOmniscient		= false;
	m_bIsFiring			= false;
	m_bPreFire			= false;
	
	// See if we should start being able to attack
	m_bCanGroundAttack	= ( m_spawnflags & SF_GUNSHIP_NO_GROUND_ATTACK ) ? false : true;

	m_flBeginCrashTime = 999999;

	m_iBurstSize = 0;
	m_bBurstHit = false;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CombineGunship::Precache( void )
{
	engine->PrecacheModel("models/gunship.mdl");

	m_iTracerTexture = engine->PrecacheModel( "sprites/laser.vmt" );
	m_iSpriteTexture = engine->PrecacheModel( "sprites/white.vmt" );

	engine->PrecacheModel("sprites/lgtning.vmt");

	m_nDebrisModel = engine->PrecacheModel( "models/gibs/rock_gibs.mdl" );

	BaseClass::Precache();
}

//------------------------------------------------------------------------------
// Purpose : 
//------------------------------------------------------------------------------
CNPC_CombineGunship::~CNPC_CombineGunship(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::Ping( void )
{
	if( IsCrashing() )
	{
		return;
	}

	if( GetEnemy() != NULL )
	{
		if( !HasCondition(COND_SEE_ENEMY) && gpGlobals->curtime > m_flTimeNextPing )
		{
			EmitSound( "NPC_CombineGunship.SearchPing" );
			m_flTimeNextPing = gpGlobals->curtime + 3;
		}
	}
	else
	{
		if( gpGlobals->curtime > m_flTimeNextPing )
		{
			EmitSound( "NPC_CombineGunship.PatrolPing" );
			m_flTimeNextPing = gpGlobals->curtime + 3;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &pos - 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_CombineGunship::GroundDistToPosition( const Vector &pos )
{
	Vector vecDiff;
	VectorSubtract( GetAbsOrigin(), pos, vecDiff );

	// Only interested in the 2d dist
	vecDiff.z = 0;

	return vecDiff.Length();
}

void CNPC_CombineGunship::PlayPatrolLoop( void )
{
	m_fPatrolLoopPlaying = true;
	/*
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundChangeVolume( m_pPatrolSound, 1.0, 1.0 );
	controller.SoundChangeVolume( m_pAngrySound, 0.0, 1.0 );
	*/
}

void CNPC_CombineGunship::PlayAngryLoop( void )
{
	m_fPatrolLoopPlaying = false;
	/*
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundChangeVolume( m_pPatrolSound, 0.0, 1.0 );
	controller.SoundChangeVolume( m_pAngrySound, 1.0, 1.0 );
	*/
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::HelicopterPostThink( void )
{
	// After HelicopterThink()
	if ( HasCondition( COND_ENEMY_DEAD ) )
	{
		if ( m_bIsFiring )
		{
			// Fire more shots at the dead body for effect
			if ( m_iBurstSize > 8 )
			{
				m_iBurstSize = 8;
			}
		}

		// Fade out search sound, fade in patrol sound.
		PlayPatrolLoop();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_CombineGunship::GetGroundAttackHitPosition( void )
{
	trace_t	tr;
	Vector	vecShootPos, vecShootDir;

	GetAttachment( "BellyGun", vecShootPos, &vecShootDir, NULL, NULL );

	AI_TraceLine( vecShootPos, vecShootPos + Vector( 0, 0, -MAX_TRACE_LENGTH ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	return tr.endpos;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::CheckGroundAttack( void )
{
	if ( m_bCanGroundAttack == false )
		return false;

	if ( m_bIsGroundAttacking )
		return false;

	// Must have an enemy
	if ( GetEnemy() == NULL )
		return false;

	// Must not have done it too recently
	if ( m_flNextGroundAttack > gpGlobals->curtime )
		return false;

	Vector	predPos, predDest;
	
	// Find where the enemy is most likely to be in two seconds
	UTIL_PredictedPosition( GetEnemy(), 1.0f, &predPos );
	UTIL_PredictedPosition( this, 1.0f, &predDest );

	Vector	predGap = ( predDest - predPos );
	predGap.z = 0;

	float	predDistance = predGap.Length();

	// Must be within distance
	if ( predDistance > MIN_GROUND_ATTACK_DIST )
		return false;

	// Can't ground attack missiles
	if ( IsTargettingMissile() )
		return false;

	//FIXME: Check to make sure we're not firing too far above or below the target
	if ( fabs( GetGroundAttackHitPosition().z - GetEnemy()->WorldSpaceCenter().z ) > MIN_GROUND_ATTACK_HEIGHT_DIFF )
		return false;

	//FIXME: Check for ground movement capabilities?

	//TODO: Check for friendly-fire

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::StartGroundAttack( void )
{
	// Mark us as attacking
	m_bIsGroundAttacking = true;
	m_flGroundAttackTime = gpGlobals->curtime + 3.0f;

	// Setup the attack effects
	Vector	vecShootPos;

	GetAttachment( "BellyGun", vecShootPos );

	CEntityMessageFilter filter( this, "CNPC_CombineGunship" );
	filter.MakeReliable();

	MessageBegin( filter, 0 );
	WRITE_BYTE( GUNSHIP_MSG_STREAKS );
	WRITE_VEC3COORD( vecShootPos );
	MessageEnd();
	CPASAttenuationFilter filter2( this, "NPC_Strider.Charge" );
	EmitSound( filter2, entindex(), "NPC_Strider.Charge" );

	Vector	endpos = GetGroundAttackHitPosition();
	
	CSoundEnt::InsertSound ( SOUND_DANGER, endpos, 1024, 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::StopGroundAttack( void )
{
	// Mark us as no longer attacking
	m_bIsGroundAttacking = false;
	m_flNextGroundAttack = gpGlobals->curtime + 4.0f;
	m_flTimeNextAttack	 = gpGlobals->curtime + 2.0f;

	Vector	hitPos = GetGroundAttackHitPosition();

	// tell the client side effect to complete
	CEntityMessageFilter filter( this, "CNPC_CombineGunship" );
	filter.MakeReliable();

	MessageBegin( filter, 0 );
		WRITE_BYTE( GUNSHIP_MSG_BIG_SHOT );
		WRITE_VEC3COORD( hitPos );
	MessageEnd();
	
	CPASAttenuationFilter filter2( this, "NPC_Strider.Shoot" );
	EmitSound( filter2, entindex(), "NPC_Strider.Shoot");

	ApplyAbsVelocityImpulse( Vector( 0, 0, 200.0f ) );

	ExplosionCreate( hitPos, QAngle( 0, 0, 1 ), this, 500, 500, true );
}

//-----------------------------------------------------------------------------
// Purpose: do all of the stuff related to having an enemy, attacking, etc.
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::DoCombat( void )
{
	float flDeltaSeen;
	flDeltaSeen = m_flLastSeen - m_flPrevSeen;

	// Check for enemy change-overs
	if ( HasEnemy() )
	{
		if ( HasCondition( COND_NEW_ENEMY ) )
		{
			if( GetEnemy() && GetEnemy()->IsPlayer() )
			{
				EmitSound( "NPC_CombineGunship.SeeEnemy" );
			}

			// If we're shooting at a missile, do it immediately!
			if ( IsTargettingMissile() )
			{
				EmitSound( "NPC_CombineGunship.SeeMissile" );

				m_flTimeNextAttack = gpGlobals->curtime + 0.5f;
				m_iBurstSize = flGunshipBurstSize.GetFloat();
			}

			// Fade in angry sound, fade out patrol sound.
			PlayAngryLoop();

			// Keep this target for at least ten seconds
			m_flTargetPersistenceTime = gpGlobals->curtime + 10.0f;
		}
	}

	// Update our firing
	if ( m_bIsFiring )
	{
		// Move the attack offset closer to the target with each shot.
		int iMissesRemaining;

		// How many more rounds have to miss? 
		iMissesRemaining = m_iBurstSize - ( flGunshipBurstSize.GetFloat() - flGunshipBurstMiss.GetFloat() );

		if ( m_iBurstSize < 1 )
		{
			StopCannonBurst();
			
			if ( IsTargettingMissile() )
			{
				m_flTimeNextAttack = gpGlobals->curtime + 0.5f;
			}
			else
			{
				m_flTimeNextAttack = gpGlobals->curtime + m_flBurstDelay;
			}
		}
		else
		{
			if ( GetEnemy() != NULL )
			{
				float	enemyDist = GroundDistToPosition( GetEnemy()->GetAbsOrigin() );

				// If we're starting a stitch, find our initial position
				if ( ( iMissesRemaining == flGunshipBurstMiss.GetFloat() ) && ( enemyDist > flGunshipBurstMin.GetFloat() ) )
				{
					// Follow mode
					Vector	enemyPos;
					QAngle	offsetAngles;
					Vector	offsetDir;

					//enemyPos = GetEnemy()->WorldSpaceCenter();
					UTIL_PredictedPosition( GetEnemy(), 2.0f, &enemyPos );

					offsetDir = ( WorldSpaceCenter() - enemyPos );
					VectorNormalize( offsetDir );

					VectorAngles( offsetDir, offsetAngles );

					//FIXME: Do we want this from the gunship's facing instead?
					offsetAngles[YAW] += random->RandomInt( -20, 20 );

					AngleVectors( offsetAngles, &offsetDir );
		
					float	stitchOffset;

					if ( enemyDist < ( flGunshipBurstDist.GetFloat() + GUNSHIP_STITCH_MIN ) )
					{
						stitchOffset = GUNSHIP_STITCH_MIN;
					}
					else
					{
						stitchOffset = flGunshipBurstDist.GetFloat();
					}

					//Move out to the start of our stitch run
					m_vecAttackPosition		= enemyPos + ( offsetDir * stitchOffset );
					m_vecAttackPosition.z	= enemyPos.z;

					//Point at our target
					m_vecAttackVelocity		= -offsetDir * BASE_STITCH_VELOCITY;

					CSoundEnt::InsertSound ( SOUND_DANGER, m_vecAttackPosition, 512, 0.2f );
					CSoundEnt::InsertSound ( SOUND_DANGER, enemyPos, 512, 0.2f );
				}
			}
		}
	}
	else
	{
		// If we're not firing, look at the enemy
		if ( GetEnemy() != NULL )
		{
			m_vecAttackPosition = GetEnemy()->WorldSpaceCenter();
		}

		// Check for a ground attack
		if ( CheckGroundAttack() )
		{
			StartGroundAttack();
		}

		// See if we're attacking
		if ( m_bIsGroundAttacking )
		{
			m_vecHitPos = GetGroundAttackHitPosition();

			// If our time is up, fire the blast and be done
			if ( m_flGroundAttackTime < gpGlobals->curtime )
			{
				// Fire!
				StopGroundAttack();
			}
		}
	}

	if( GetEnemy() != NULL && GetEnemy()->Classify() == CLASS_FLARE && flDeltaSeen > flFlareTime.GetFloat() )
	{
		AddEntityRelationship( GetEnemy(), D_NU, 5 );

		PlayPatrolLoop();

		// Forget the flare now.
		SetEnemy( NULL );
	}

	// Fire if we have rounds remaining in this burst
	if ( ( m_iBurstSize > 0 ) && ( gpGlobals->curtime > m_flTimeNextAttack ) )
	{
		UpdateEnemyTarget();
		FireCannonRound();
	}
}


//-----------------------------------------------------------------------------
// Purpose: There's a lot of code in here now. We should consider moving 
//			helicopters and such to scheduled AI. (sjb)
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::MoveHead( void )
{
	float flYaw = GetPoseParameter( "flex_horz" );
	float flPitch = GetPoseParameter( "flex_vert" );

/*
	This head-turning code will cause the head to POP when switching from looking at the enemy
	to looking according to the flight model. I will fix this later. Right now I'm turning
	the code over to Ken for some aiming fixups. (sjb)
*/

	while( 1 )
	{
		if ( GetEnemy() != NULL )
		{
			Vector vecToEnemy, vecAimDir;
			float	flDot;

			Vector vTargetPos, vGunPosition;
			Vector vecTargetOffset;
			QAngle vGunAngles;

			GetAttachment( "muzzle", vGunPosition, vGunAngles );

			vTargetPos = GetEnemyTarget();

			VectorSubtract( vTargetPos, vGunPosition, vecToEnemy );
			VectorNormalize( vecToEnemy );
			
			// get angles relative to body position
			AngleVectors( GetAbsAngles(), &vecAimDir );
			flDot = DotProduct( vecAimDir, vecToEnemy );

			// Look at Enemy!!
			if ( flDot > 0.3f )
			{
				float flDiff;

				float flDesiredYaw = VecToYaw(vTargetPos - vGunPosition);
				flDiff = UTIL_AngleDiff( flDesiredYaw, vGunAngles.y ) * 0.90;
				flYaw = UTIL_Approach( flYaw + flDiff, flYaw, 5.0 );	

				float flDesiredPitch = UTIL_VecToPitch(vTargetPos - vGunPosition);
				flDiff = UTIL_AngleDiff( flDesiredPitch, vGunAngles.x ) * 0.90;
				flPitch = UTIL_Approach( flPitch + flDiff, flPitch, 5.0 );	

				break;
			}
		}

		// Look where going!
#if 1 // old way- look according to rotational velocity
		flYaw = UTIL_Approach( GetLocalAngularVelocity().y, flYaw, 2.0 * 10 * m_flDeltaT );	
		flPitch = UTIL_Approach( GetLocalAngularVelocity().x, flPitch, 2.0 * 10 * m_flDeltaT );	
#else // new way- look towards the next waypoint?
		// !!!UNDONE
#endif
		break;
	}

	// Set the body flexes
	SetPoseParameter( "flex_vert", flPitch );
	SetPoseParameter( "flex_horz", flYaw );
}


//-----------------------------------------------------------------------------
// Purpose: There's a lot of code in here now. We should consider moving 
//			helicopters and such to scheduled AI. (sjb)
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::PrescheduleThink( void )
{
	m_flDeltaT = gpGlobals->curtime - GetLastThink();

	if( gpGlobals->curtime > m_flBeginCrashTime )
	{
		BeginCrash();

		// Do I need to return right here? (sjb)
		return;
	}

	Ping();

	if( m_lifeState == LIFE_ALIVE )
	{
		DoCombat();
		MoveHead();
	}
	else if( m_lifeState == LIFE_DYING )
	{
		// Drop from the sky. Paddles flap, head looking around, smoking.
		Vector vecAttachment;
		Vector vecDir;

		//GetAttachment( "exhaustl", vecAttachment,  &vecDir, NULL, NULL );
		//NDebugOverlay::Line( vecAttachment, vecAttachment + vecDir * 128, 255,0,0, false, 0.1 );

		//GetAttachment( "exhaustr", vecAttachment, &vecDir, NULL, NULL );
		//NDebugOverlay::Line( vecAttachment, vecAttachment + vecDir * 128, 255,0,0, false, 0.1 );
	}
	else
	{
		// Do nothing. You're DEAD!
	}

	BaseClass::PrescheduleThink();

#ifdef JACOBS_GUNSHIP	
	SetPoseParameter( "pitch", random->RandomFloat( GUNSHIP_HEAD_MAX_LEFT, GUNSHIP_HEAD_MAX_RIGHT ) );
	SetPoseParameter( "yaw", random->RandomFloat( GUNSHIP_HEAD_MAX_UP, GUNSHIP_HEAD_MAX_DOWN ) );
#endif

}

//------------------------------------------------------------------------------
// Purpose :	If the enemy is in front of the gun, load up a burst. 
//				Actual gunfire is handled in PrescheduleThink
// Input   : 
// Output  : 
//------------------------------------------------------------------------------
bool CNPC_CombineGunship::FireGun( void )
{
	if ( m_lifeState != LIFE_ALIVE )
		return false;

	if ( m_bIsGroundAttacking )
		return false;

	if ( GetEnemy() != NULL && m_bIsFiring == false && gpGlobals->curtime > m_flTimeNextAttack )
	{
		if ( m_bPreFire == false )
		{
			m_bPreFire = true;
			m_flTimeNextAttack = gpGlobals->curtime + 0.5f;
			
			EmitSound( "NPC_CombineGunship.CannonStartSound" );
			return false;
		}

		//TODO: Emit the danger noise and wait until it's finished

		// Don't fire at an occluded enemy unless blindfire is on.
		if ( HasCondition( COND_ENEMY_OCCLUDED ) && ( m_fBlindfire == false ) )
			return false;

		// Don't shoot if the enemy is too close
		if ( GroundDistToPosition( GetEnemy()->GetAbsOrigin() ) < GUNSHIP_STITCH_MIN )
			return false;

		Vector vecAimDir, vecToEnemy;
		Vector vecMuzzle, vecEnemyTarget;

		GetAttachment( "muzzle", vecMuzzle, &vecAimDir, NULL, NULL );
		vecEnemyTarget = GetEnemyTarget();

		// Aim with the muzzle's attachment point.
		VectorSubtract( vecEnemyTarget, vecMuzzle, vecToEnemy );

		VectorNormalize( vecToEnemy );
		VectorNormalize( vecAimDir );

		if( DotProduct( vecToEnemy, vecAimDir ) > 0.9990 )
		{
			StartCannonBurst( flGunshipBurstSize.GetFloat() );
			return true;
		}

		//FIXME: Not really the case.. but we're trying to aim, so...
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
// Purpose: Fire a round from the cannon
// Notes:	Only call this if you have an enemy.
//------------------------------------------------------------------------------
void CNPC_CombineGunship::FireCannonRound( void )
{
	Vector vecPenetrate;
	trace_t tr;

	QAngle vecAimAngle;
	Vector vecToEnemy, vecEnemyTarget;
	Vector vecMuzzle;
	Vector vecAimDir;

	GetAttachment( "muzzle", vecMuzzle, vecAimAngle );
	vecEnemyTarget = GetEnemyTarget();
	
	// Aim with the muzzle's attachment point.
	VectorSubtract( vecEnemyTarget, vecMuzzle, vecToEnemy );
	VectorNormalize( vecToEnemy );

	Vector	vForward, vRight, vUp;

	// If the gun is wildly off target, stop firing!
	// FIXME  - this should use a vector pointing 
	// to the enemy's location PLUS the stitching 
	// error! (sjb) !!!BUGBUG
	AngleVectors( vecAimAngle, &vecAimDir );
	
	AngleVectors( vecAimAngle, &vForward, &vRight, &vUp );

	if ( g_debug_gunship.GetInt() == GUNSHIP_DEBUG_STITCHING )
	{
		NDebugOverlay::Line( vecMuzzle, vecEnemyTarget, 255, 255, 0, true, 1.0f );

		NDebugOverlay::Line( vecMuzzle, vecMuzzle + ( vForward * 64.0f ), 255, 0, 0, true, 1.0f );
		NDebugOverlay::Line( vecMuzzle, vecMuzzle + ( vRight * 32.0f ), 0, 255, 0, true, 1.0f );
		NDebugOverlay::Line( vecMuzzle, vecMuzzle + ( vUp * 32.0f ), 0, 0, 255, true, 1.0f );
	}

	if ( DotProduct( vecToEnemy, vecAimDir ) < 0.8f )
	{
		StopCannonBurst();
		return;
	}

	//Add a muzzle flash
	g_pEffects->MuzzleFlash( vecMuzzle, vecAimAngle, random->RandomFloat( 5.0f, 7.0f ) , MUZZLEFLASH_TYPE_GUNSHIP );

	m_OnFireCannon.FireOutput( this, this, 0 );

	m_iBurstSize--;

	m_flTimeNextAttack = gpGlobals->curtime + 0.05f;

	float flPrevHealth = 0;
	if ( GetEnemy() )
	{
		flPrevHealth = GetEnemy()->GetHealth();
	}

	// Make sure we hit missiles
	if ( IsTargettingMissile() )
	{
		// Fire a fake shot
		FireBullets( 1, vecMuzzle, vecToEnemy, VECTOR_CONE_5DEGREES, 8192, m_iAmmoType, 1 );

		CBaseEntity *pMissile = GetEnemy();

		Vector	missileDir, threatDir;

		AngleVectors( pMissile->GetAbsAngles(), &missileDir );

		threatDir = ( WorldSpaceCenter() - pMissile->GetAbsOrigin() );
		float	threatDist = VectorNormalize( threatDir );

		// Check that the target is within some threshold
		if ( ( DotProduct( threatDir, missileDir ) > 0.95f ) && ( threatDist < 1024.0f ) )
		{
			if ( random->RandomInt( 0, 1 ) == 0 )
			{
				CTakeDamageInfo info( this, this, 200, DMG_MISSILEDEFENSE );
				CalculateBulletDamageForce( &info, m_iAmmoType, -threatDir, WorldSpaceCenter() );
				GetEnemy()->TakeDamage( info );
			}
		}
		else
		{
			//FIXME: Some other metric
		}
	}
	else
	{
		if ( m_bNoiseyShot )
		{
			// Fire a more random shot
			FireBullets( 1, vecMuzzle, vecToEnemy, VECTOR_CONE_8DEGREES, 8192, m_iAmmoType, 1 );
		}
		else
		{
			// Fire directly at the target
			FireBullets( 1, vecMuzzle, vecToEnemy, Vector(0,0,0), 8192, m_iAmmoType, 1 );
		}

		if ( GetEnemy() && flPrevHealth != GetEnemy()->GetHealth() )
		{
			m_bBurstHit = true;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	bool fReturn = BaseClass::FVisible( pEntity, traceMask, ppBlocker );

	if( m_fOmniscient )
	{
		if( !fReturn )
		{
			// Set this condition so that we can check it later and know that the 
			// enemy truly is occluded, but the gunship regards it as visible due 
			// to omniscience.
			SetCondition( COND_ENEMY_OCCLUDED );
		}
		else
		{
			ClearCondition( COND_ENEMY_OCCLUDED );
		}

		return true;
	}

	if( fReturn )
	{
		ClearCondition( COND_ENEMY_OCCLUDED );
	}
	else
	{
		SetCondition( COND_ENEMY_OCCLUDED );
	}

	return fReturn;
}


//-----------------------------------------------------------------------------
// Purpose: Change the depth that gunship bullets can penetrate through solids
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputSetPenetrationDepth( inputdata_t &inputdata )
{
	m_flPenetrationDepth = inputdata.value.Float();
}


//-----------------------------------------------------------------------------
// Purpose: Allow the gunship to sense its enemy's location even when enemy
//			is hidden from sight.
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputOmniscientOn( inputdata_t &inputdata )
{
	m_fOmniscient = true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the gunship to its default requirement that it see the 
//			enemy to know its current position
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputOmniscientOff( inputdata_t &inputdata )
{
	m_fOmniscient = false;
}


//-----------------------------------------------------------------------------
// Purpose: Allows the gunship to fire at an unseen enemy. The gunship is relying
//			on hitting the target with bullets that will punch through the 
//			cover that the enemy is hiding behind. (Such as the Depot lighthouse)
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputBlindfireOn( inputdata_t &inputdata )
{
	m_fBlindfire = true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the gunship to default rules for attacking the enemy. The
//			enemy must be seen to be fired at.
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputBlindfireOff( inputdata_t &inputdata )
{
	m_fBlindfire = false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::BeginCrash( void )
{
	// Don't call this over and over.
	m_flBeginCrashTime = 9999999;

	Vector vecBBMin;
	Vector vecBBMax;
	
	ExtractBbox( SelectHeaviestSequence( ACT_GUNSHIP_PATROL ), vecBBMin, vecBBMax ); 
	//Trim the bounding box a bit. It's huge.
	vecBBMin.x += 80;
	vecBBMax.x -= 80;
	vecBBMin.y += 80;
	vecBBMax.y -= 80;

	UTIL_SetSize( this, vecBBMin, vecBBMax );

	// Flail
	SetActivity( (Activity)ACT_GUNSHIP_CRASH );
	ResetSequenceInfo();

	//FIXME: The below code needs to be rethought with the new navigation setup (jdw)

	SetTouch( ExplodeTouch );
	m_lifeState = LIFE_DYING;

	/*
	CBaseEntity *pPathCorner;

	pPathCorner = CreateEntityByName( "path_corner" );
	pPathCorner->SetName( MAKE_STRING("gunshipdie") );

/*
	Vector vecForward;
	Vector vecPosition;
	trace_t tr;

	GetVectors( &vecForward, NULL, NULL );

	vecForward.z = 0;

	// Back up, trace down, and then push farther into the ground.
	vecPosition = GetAbsOrigin() + vecForward * -flGunshipCrashDist.GetFloat();

	AI_TraceLine( vecPosition, vecPosition - Vector( 0, 0, 8192 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	vecPosition = tr.endpos;
	vecPosition.z += 64;

	pPathCorner->SetOrigin( vecPosition );
*/

	/*
	CBaseEntity *pCrashEnt = gEntList.FindEntityByName( NULL, gunshipcrash.GetString(), this );

	if( !pCrashEnt )
	{
		Msg( "*** Gunship requires entity named: gunship_crash ***\n" );
		//SetThink(NULL);
		
		// In this case, just have him plummet
		SetTouch( ExplodeTouch );
		m_lifeState = LIFE_DYING;

		return;
	}

	Vector vecDir;
	Vector vecCrashSpot;

	vecCrashSpot = pCrashEnt->GetAbsOrigin();
	vecDir = vecCrashSpot - GetAbsOrigin();

	VectorNormalize( vecDir );

	// push the destination through the thumper so the gunship doesn't slow down.
	vecDir.z = 0;
	vecCrashSpot = pCrashEnt->GetAbsOrigin();

	// but bring the Z back up, since the gunship seems
	// to be trying hard to descend to the new z position more quickly
	// that it's flying toward it.
	// vecCrashSpot.z = pCrashEnt->GetAbsOrigin().z + 100;

	pPathCorner->SetLocalOrigin( vecCrashSpot );

	//NDebugOverlay::Line( GetAbsOrigin(), vecCrashSpot, 255,0,0, false, 10.0 );
	
	SetTouch( ExplodeTouch );

	m_lifeState = LIFE_DYING;
	*/
}


//-----------------------------------------------------------------------------
// Purpose: Set the gunship's paddles flailing!
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::Event_Killed( const CTakeDamageInfo &info )
{
	//BaseClass::Event_Killed( pInflictor, pAttacker, flDamage, bitsDamageType );
	m_takedamage = DAMAGE_NO;

	// no more pings.
	m_flTimeNextPing = gpGlobals->curtime + 9999;

	m_flBeginCrashTime = gpGlobals->curtime;

	StopCannonBurst();

	// Turn the gun off.
	//m_fHelicopterFlags &= ~BITS_HELICOPTER_GUN_ON;

	// Replace the rotor sound with broken engine sound.
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pRotorSound );

	// BUGBUG: Isn't this sound just going to get stomped when the base class calls StopLoopingSounds() ??
	CPASAttenuationFilter filter2( this );
	m_pRotorSound = controller.SoundCreate( filter2, entindex(), "NPC_CombineGunship.DyingSound" );
	controller.Play( m_pRotorSound, 1.0, 100 );

	m_OnDeath.FireOutput( info.GetAttacker(), this );

	///!!!BUGBUG why doesn't this crossfade occur when we lose 
	// our enemy?! (sjb)
	//controller.SoundChangeVolume( m_pPatrolSound, 1.0, 1.0 );
	//controller.SoundChangeVolume( m_pAngrySound, 0.0, 1.0 );

#if 0
	// Find my death node!
	CBaseEntity *pDeathNode = NULL;
	float		flNearestDist = 99999;
	CBaseEntity *pNearestNode = NULL;

	pDeathNode = gEntList.FindEntityByClassname( NULL, "path_corner_crash" );

	

	if( !pDeathNode )
	{
		Msg( "***ERROR*** (DeathNode) Gunship cannot find a path_corner_crash\n" );
		SetThink( NULL );
		return;
	}

	do
	{
		float flDist;

		flDist = (GetDesiredPosition() - pDeathNode->GetAbsOrigin()).Length();

		if( flDist < flNearestDist )
		{
			flNearestDist = flDist;
			pNearestNode = pDeathNode;
		}

		pDeathNode = gEntList.FindEntityByClassname( pDeathNode, "path_corner_crash" );

	} while( pDeathNode != NULL );


	if( !pNearestNode )
	{
		Msg( "***ERROR*** Gunship cannot find a path_corner_crash\n" );
		SetThink( NULL );
		return;
	}

	m_lifeState = LIFE_DYING;

	m_pGoalEnt = pNearestNode;

	// Follow the chain of path_corner_crash's. The one to make the helicopter's new target is the last one in the chain
	// or the last visible one.

	CBaseEntity *pNextPathCorner, *pPreviousPathCorner;

	pPreviousPathCorner = pNextPathCorner = gEntList.FindEntityByName( NULL, pNearestNode->m_target, this );

	do
	{
		//NDebugOverlay::Line( pPreviousPathCorner->GetAbsOrigin(), pNextPathCorner->GetAbsOrigin(), 0, 255, 0, false, 60.0 );

		pPreviousPathCorner = pNextPathCorner;
		pNextPathCorner = gEntList.FindEntityByName( NULL, pPreviousPathCorner->m_target, this );
	} while( pNextPathCorner );
#endif
}


//-----------------------------------------------------------------------------
// Purpose:	Lame, temporary death
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::DyingThink( void )
{
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if( random->RandomInt( 0, 50 ) == 1 )
	{
		Vector vecSize = Vector( 60, 60, 30 );
		CPVSFilter filter( GetAbsOrigin() );
		te->BreakModel( filter, 0.0, &GetAbsOrigin(), &vecSize, &vec3_origin, m_nDebrisModel, 100, 0, 2.5, BREAK_METAL );
	}

	QAngle angVel = GetLocalAngularVelocity();
	if( angVel.y < 400 )
	{
		angVel.y *= 1.1;
		SetLocalAngularVelocity( angVel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::ApplyGeneralDrag( void )
{
	Vector vecNewVelocity = GetAbsVelocity();
	
	// See if we need to stop more quickly
	if ( m_bIsGroundAttacking )
	{
		vecNewVelocity *= 0.95f;
	}
	else
	{
		vecNewVelocity *= 0.995;
	}

	SetAbsVelocity( vecNewVelocity );
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------

void CNPC_CombineGunship::Flight( void )
{
	if( GetFlags() & FL_ONGROUND )
	{
		//This would be really bad.
		RemoveFlag( FL_ONGROUND );
	}

	// NDebugOverlay::Line(GetLocalOrigin(), GetDesiredPosition(), 0,0,255, true, 0.1);

	Vector deltaPos = GetDesiredPosition() - GetLocalOrigin();

	// calc desired acceleration
	float dt = 1.0f;

	Vector	accel;
	float	accelRate = GUNSHIP_ACCEL_RATE;
	float	maxSpeed = 60 * 17.6; // 120 miles per hour.

	if ( m_lifeState == LIFE_DYING )
	{
		accelRate *= 5.0;
		maxSpeed *= 5.0;
	}

	float flDist = min( GetAbsVelocity().Length() + accelRate, maxSpeed );

	// Only decellerate to our goal if we're going to hit it
	if (deltaPos.Length() > flDist * dt)
	{
		float scale = flDist * dt / deltaPos.Length();
		deltaPos = deltaPos * scale;
	}
	
	// calc goal linear accel to hit deltaPos in dt time.
	accel.x = 2.0 * (deltaPos.x - GetAbsVelocity().x * dt) / (dt * dt);
	accel.y = 2.0 * (deltaPos.y - GetAbsVelocity().y * dt) / (dt * dt);
	accel.z = 2.0 * (deltaPos.z - GetAbsVelocity().z * dt + 0.5 * 384 * dt * dt) / (dt * dt);
	
	// NDebugOverlay::Line(GetLocalOrigin(), GetLocalOrigin() + deltaPos, 255,0,0, true, 0.1);

	// don't fall faster than 0.2G or climb faster than 2G
	accel.z = clamp( accel.z, 384 * 0.2, 384 * 2.0 );

	Vector forward, right, up;
	GetVectors( &forward, &right, &up );

	Vector goalUp = accel;
	VectorNormalize( goalUp );

	// calc goal orientation to hit linear accel forces
	float goalPitch = RAD2DEG( asin( DotProduct( forward, goalUp ) ) );
	float goalYaw = UTIL_VecToYaw( m_vecDesiredFaceDir );
	float goalRoll = RAD2DEG( asin( DotProduct( right, goalUp ) ) );

	// clamp goal orientations
	goalPitch = clamp( goalPitch, -45, 60 );
	goalRoll = clamp( goalRoll, -45, 45 );

	// calc angular accel needed to hit goal pitch in dt time.
	dt = 0.6;
	QAngle goalAngAccel;
	goalAngAccel.x = 2.0 * (AngleDiff( goalPitch, AngleNormalize( GetLocalAngles().x ) ) - GetLocalAngularVelocity().x * dt) / (dt * dt);
	goalAngAccel.y = 2.0 * (AngleDiff( goalYaw, AngleNormalize( GetLocalAngles().y ) ) - GetLocalAngularVelocity().y * dt) / (dt * dt);
	goalAngAccel.z = 2.0 * (AngleDiff( goalRoll, AngleNormalize( GetLocalAngles().z ) ) - GetLocalAngularVelocity().z * dt) / (dt * dt);

	goalAngAccel.x = clamp( goalAngAccel.x, -300, 300 );
	//goalAngAccel.y = clamp( goalAngAccel.y, -60, 60 );
	goalAngAccel.y = clamp( goalAngAccel.y, -120, 120 );
	goalAngAccel.z = clamp( goalAngAccel.z, -300, 300 );

	// limit angular accel changes to similate mechanical response times
	dt = 0.1;
	QAngle angAccelAccel;
	angAccelAccel.x = (goalAngAccel.x - m_vecAngAcceleration.x) / dt;
	angAccelAccel.y = (goalAngAccel.y - m_vecAngAcceleration.y) / dt;
	angAccelAccel.z = (goalAngAccel.z - m_vecAngAcceleration.z) / dt;

	angAccelAccel.x = clamp( angAccelAccel.x, -1000, 1000 );
	angAccelAccel.y = clamp( angAccelAccel.y, -1000, 1000 );
	angAccelAccel.z = clamp( angAccelAccel.z, -1000, 1000 );

	m_vecAngAcceleration += angAccelAccel * 0.1;

	// Msg( "pitch %6.1f (%6.1f:%6.1f)  ", goalPitch, GetLocalAngles().x, m_vecAngVelocity.x );
	// Msg( "roll %6.1f (%6.1f:%6.1f) : ", goalRoll, GetLocalAngles().z, m_vecAngVelocity.z );
	// Msg( "%6.1f %6.1f %6.1f  :  ", goalAngAccel.x, goalAngAccel.y, goalAngAccel.z );
	// Msg( "%6.0f %6.0f %6.0f\n", angAccelAccel.x, angAccelAccel.y, angAccelAccel.z );

	ApplySidewaysDrag( right );
	ApplyGeneralDrag();
	
	QAngle angVel = GetLocalAngularVelocity();
	angVel += m_vecAngAcceleration * 0.1;

	//angVel.y = clamp( angVel.y, -60, 60 );
	//angVel.y = clamp( angVel.y, -120, 120 );
	angVel.y = clamp( angVel.y, -120, 120 );

	SetLocalAngularVelocity( angVel );

	m_flForce = m_flForce * 0.8 + (accel.z + fabs( accel.x ) * 0.1 + fabs( accel.y ) * 0.1) * 0.1 * 0.2;

	Vector vecImpulse = m_flForce * up;
	
	if ( m_lifeState == LIFE_DYING )
	{
		vecImpulse.z = -38.4;  // 64ft/sec
	}
	else
	{
		vecImpulse.z -= 38.4;  // 32ft/sec
	}
	
	// Find our acceleration direction
	Vector	vecAccelDir = vecImpulse;
	VectorNormalize( vecAccelDir );

	// Find our current velocity
	Vector	vecVelDir = GetAbsVelocity();
	VectorNormalize( vecVelDir );

	// Level out our plane of movement
	vecAccelDir.z	= 0.0f;
	vecVelDir.z		= 0.0f;
	forward.z		= 0.0f;
	right.z			= 0.0f;

	// Find out how "fast" we're moving in relation to facing and acceleration
	float speed = m_flForce * DotProduct( vecVelDir, vecAccelDir );// * DotProduct( forward, vecVelDir );

	// Apply the acceleration blend to the fins
	float finAccelBlend = SimpleSplineRemapVal( speed, -60, 60, -1, 1 );
	float curFinAccel = GetPoseParameter( "fin_accel" );
	
	curFinAccel = UTIL_Approach( finAccelBlend, curFinAccel, 0.5f );
	SetPoseParameter( "fin_accel", curFinAccel );

	speed = m_flForce * DotProduct( vecVelDir, right );

	// Apply the spin sway to the fins
	float finSwayBlend = SimpleSplineRemapVal( speed, -60, 60, -1, 1 );
	float curFinSway = GetPoseParameter( "fin_sway" );

	curFinSway = UTIL_Approach( finSwayBlend, curFinSway, 0.5f );
	SetPoseParameter( "fin_sway", curFinSway );

	// Add in our velocity pulse for this frame
	ApplyAbsVelocityImpulse( vecImpulse );
}

//-----------------------------------------------------------------------------
// Purpose:
//
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::DoRotorWash( void )
{
	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );
	Vector vecRotorOrigin = GetAbsOrigin() + vecForward * -64;
	DrawRotorWash( GUNSHIP_WASH_ALTITUDE, vecRotorOrigin );
}


//------------------------------------------------------------------------------
// Purpose : Fire up the Gunships 'second' rotor sound. The Search sound.
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineGunship::InitializeRotorSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	
	CPASAttenuationFilter filter( this );

	m_pCannonSound		= controller.SoundCreate( filter, entindex(), "NPC_CombineGunship.CannonSound" );
	m_pRotorSound		= controller.SoundCreate( filter, entindex(), "NPC_CombineGunship.RotorSound" );
	m_pAirIntakeSound	= controller.SoundCreate( filter, entindex(), "NPC_CombineGunship.IntakeSound" );
	m_pAirExhaustSound	= controller.SoundCreate( filter, entindex(), "NPC_CombineGunship.ExhaustSound" );
	m_pAirBlastSound	= controller.SoundCreate( filter, entindex(), "NPC_CombineGunship.RotorBlastSound" );
	
	controller.Play( m_pCannonSound, 0.0, 100 );
	controller.Play( m_pAirIntakeSound, 0.0, 100 );
	controller.Play( m_pAirExhaustSound, 0.0, 100 );
	controller.Play( m_pAirBlastSound, 0.0, 100 );

	BaseClass::InitializeRotorSound();
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineGunship::UpdateRotorSoundPitch( int iPitch )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	// Apply the pitch to both sounds. 
	controller.SoundChangePitch( m_pAirIntakeSound, iPitch, 0.1 );
	controller.SoundChangePitch( m_pAirExhaustSound, iPitch, 0.1 );

	// FIXME: wrong lookup player call
	CBaseEntity *pPlayer = gEntList.FindEntityByName( NULL, "!player", this );
	if (pPlayer)
	{
		Vector pos;
		Vector up;
		GetAttachment( "rotor", pos, NULL, NULL, &up );

		Vector dir = pPlayer->WorldSpaceCenter() - pos;
		VectorNormalize(dir);

		float balance = DotProduct( dir, up );

		float airmovement = m_flForce / 38.4;
		// Msg( "%f\n", airmovement );

		if (balance > -0.2)
		{
			controller.SoundChangeVolume( m_pAirIntakeSound, airmovement * flIntakeVolume.GetFloat() * (balance + 0.2) / 1.2, 0.1 );
		}
		else
		{
			controller.SoundChangeVolume( m_pAirIntakeSound, 0, 0.1 );
		}
		if (balance < 0.5)
		{
			controller.SoundChangeVolume( m_pAirExhaustSound, airmovement * flExhaustVolume.GetFloat() * (0.5 - balance) / 1.5, 0.1 );
		}
		else
		{
			controller.SoundChangeVolume( m_pAirExhaustSound, 0, 0.1 );
		}

		if (balance < 0.0)
		{
			controller.SoundChangeVolume( m_pAirBlastSound, (0.0 - balance) / 1.0, 0.1 );
		}
		else
		{
			controller.SoundChangeVolume( m_pAirBlastSound, 0, 0.1 );
		}
	}

#if 1
	// FIXME: use decay functions
	m_flRotorPitch = m_flRotorPitch * 0.8 + (m_flForce - 30) * 0.2;
	// Msg( "motor %.1f (%d)\n", iPitch + m_flRotorPitch, iPitch );
	controller.SoundChangePitch( m_pRotorSound, iPitch + m_flRotorPitch, 0.1 );

	m_flRotorVolume = m_flRotorVolume * 0.5 + (m_flForce > 30.0 ? (m_flForce - 30) / 20.0 : 0.0) * 0.5;
	controller.SoundChangeVolume( m_pRotorSound, m_flRotorVolume * flRotorVolume.GetFloat(), 0.1 );
	
	// controller.SoundChangeVolume( m_pRotorSound, 0, 0.1 );
	// Msg( "force %.2f : %5.2f  %5.2f\n", m_flForce, p, v );
#else
	BaseClass::UpdateRotorSoundPitch( iPitch );
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::ApplySidewaysDrag( const Vector &vecRight )
{
	Vector vecVelocity = GetAbsVelocity();
	if( m_lifeState == LIFE_ALIVE )
	{
		vecVelocity.x *= (1.0 - fabs( vecRight.x ) * 0.05);
		vecVelocity.y *= (1.0 - fabs( vecRight.y ) * 0.05);
		vecVelocity.z *= (1.0 - fabs( vecRight.z ) * 0.05);
	}
	else
	{
		vecVelocity.x *= (1.0 - fabs( vecRight.x ) * 0.03);
		vecVelocity.y *= (1.0 - fabs( vecRight.y ) * 0.03);
		vecVelocity.z *= (1.0 - fabs( vecRight.z ) * 0.09);
	}
	SetAbsVelocity( vecVelocity );
}

//------------------------------------------------------------------------------
// Purpose: Explode the gunship.
//------------------------------------------------------------------------------
void CNPC_CombineGunship::SelfDestruct( void )
{
	SetThink( NULL );
	
	Vector vecDelta;
	int i;

	for( i = 0 ; i < 3 ; i++ )
	{
		vecDelta.x = random->RandomFloat( -128, 128 );
		vecDelta.y = random->RandomFloat( -128, 128 );
		vecDelta.z = random->RandomFloat( -128, 128 );

		ExplosionCreate( GetAbsOrigin() + vecDelta, QAngle( -90, 0, 0 ), this, 10, 10, false );
	}

	Vector vecSize( 64, 64, 32 );

	Vector vecUp( 0, 0, 100 );

	// CPVSFilter filter( GetAbsOrigin() );
	// te->BreakModel( filter, 0.0, &GetAbsOrigin(), &vecSize, &vecUp, m_nDebrisModel, 100, 0, 2.5, BREAK_METAL );

	StopLoopingSounds();
	StopCannonBurst();
	
	Vector vecVelocity = GetAbsVelocity();
	vecVelocity.z = 0.0; // stop falling.
	SetAbsVelocity( vecVelocity );

	EmitSound( "NPC_CombineGunship.Explode" );

	CTakeDamageInfo info;
	CBaseEntity *pRagdoll;

	info.SetDamageForce( GetAbsVelocity() * 4000 + Vector( 0, 0, 2000 ) );
	info.SetDamagePosition( GetAbsOrigin() );

	pRagdoll = CreateServerRagdoll( this, 0, info, COLLISION_GROUP_NONE );

	/*
	CGunshipCrashFX *pFX = ( CGunshipCrashFX* )CreateEntityByName( "gunshipcrashfx" );
	pFX->Spawn();
	pFX->SetOwnerEntity( pRagdoll );
	*/

	UTIL_Remove(this);
}


//------------------------------------------------------------------------------
// Purpose : Explode the gunship.
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineGunship::InputSelfDestruct( inputdata_t &inputdata )
{
	SelfDestruct();
}

//------------------------------------------------------------------------------
// Purpose : Shrink the gunship's bbox so that it fits in docking bays
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineGunship::InputSetDockingBBox( inputdata_t &inputdata )
{
	Vector vecSize( 32, 32, 32 );

	UTIL_SetSize( this, vecSize * -1, vecSize );
}

//------------------------------------------------------------------------------
// Purpose : Set the gunship BBox to normal size
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CombineGunship::InputSetNormalBBox( inputdata_t &inputdata )
{
	Vector vecBBMin, vecBBMax;

	ExtractBbox( SelectHeaviestSequence( ACT_GUNSHIP_PATROL ), vecBBMin, vecBBMax ); 

	// Trim the bounding box a bit. It's huge.
#define GUNSHIP_TRIM_BOX 38
	vecBBMin.x += GUNSHIP_TRIM_BOX;
	vecBBMax.x -= GUNSHIP_TRIM_BOX;
	vecBBMin.y += GUNSHIP_TRIM_BOX;
	vecBBMax.y -= GUNSHIP_TRIM_BOX;

	UTIL_SetSize( this, vecBBMin, vecBBMax );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputEnableGroundAttack( inputdata_t &inputdata )
{
	m_bCanGroundAttack = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputDisableGroundAttack( inputdata_t &inputdata )
{
	m_bCanGroundAttack = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::InputDoGroundAttack( inputdata_t &inputdata )
{
	StartGroundAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::UpdateVehicularEnemyTarget( void )
{
	// NOTENOTE: jdw -	We need to account for the amount of time a player has been "slow" and "fast" so that
	//					minor mistakes don't result in instant death. The player should be able to do minor
	//					course corrections without penalty.  They should only feel the wrath when they stay still
	//					for too long.

	// Follow mode
	Vector	enemyPos;
	bool	bTargettingPlayer;
	
	enemyPos = GetEnemy()->WorldSpaceCenter();
	bTargettingPlayer = GetEnemy()->IsPlayer();

	// Direction towards the enemy
	Vector	targetDir = enemyPos - m_vecAttackPosition;
	VectorNormalize( targetDir );

	// Direction from the gunship to the enemy
	Vector	enemyDir = enemyPos - WorldSpaceCenter();
	VectorNormalize( enemyDir );

	float	targetDot = DotProduct( enemyDir, targetDir );
	float	lastSpeed = VectorNormalize( m_vecAttackVelocity );

	QAngle	chaseAngles, lastChaseAngles;

	VectorAngles( targetDir, chaseAngles );
	VectorAngles( m_vecAttackVelocity, lastChaseAngles );

	float yawDiff = UTIL_AngleDiff( chaseAngles[YAW], lastChaseAngles[YAW] );

	CBasePlayer *pPlayer = ToBasePlayer( GetEnemy() );

	Vector vecVelocity;
	IPhysicsObject *pVehiclePhysics = pPlayer->GetVehicle()->GetVehicleEnt()->VPhysicsGetObject();
	
	if ( pVehiclePhysics == NULL )
	{
		vecVelocity = GetAbsVelocity();
	}
	else
	{
		AngularImpulse	angVel;
		pVehiclePhysics->GetVelocity( &vecVelocity, &angVel );
	}

	/// The faster the player drives, the slower the gunship tracks.
	float flVehicleSpeed = clamp( vecVelocity.Length(), 0, 512 );

	int	maxYaw = RemapVal( flVehicleSpeed, 512, 0, 2, 30 );

	yawDiff = clamp( yawDiff, -maxYaw, maxYaw );

	chaseAngles[PITCH]	= 0.0f;
	chaseAngles[ROLL]	= 0.0f;
	
	if ( targetDot <= 0.0f )
	{
		// Turn away from our target if we're behind it
		chaseAngles[YAW] = UTIL_AngleMod( lastChaseAngles[YAW] - yawDiff );
	}
	else
	{
		chaseAngles[YAW] = UTIL_AngleMod( lastChaseAngles[YAW] + yawDiff );
	}

	AngleVectors( chaseAngles, &targetDir );

	//Debug info
	if ( g_debug_gunship.GetInt() == GUNSHIP_DEBUG_STITCHING )
	{
		// Final position
		NDebugOverlay::Cross3D( m_vecAttackPosition, -Vector(2,2,2), Vector(2,2,2), 0, 0, 255, true, 4.0f );
		NDebugOverlay::Line( m_vecAttackPosition, m_vecAttackPosition + ( m_vecAttackVelocity * 64 ), 255, 0, 0, true, 4.0f );
	}

	// Update our new velocity
	m_vecAttackVelocity = targetDir * lastSpeed;

	// Move along that velocity for this step in time
	m_vecAttackPosition += ( m_vecAttackVelocity * 0.1f );
	m_vecAttackPosition.z = enemyPos.z;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vGunPosition - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::UpdateEnemyTarget( void )
{
	Vector	vGunPosition;

	GetAttachment( "muzzle", vGunPosition );

	// Follow mode
	Vector	enemyPos;
	bool	bTargettingPlayer;
	
	if ( GetEnemy() != NULL )
	{
		CBasePlayer *pPlayer = ToBasePlayer( GetEnemy() );

		if ( ( pPlayer != NULL ) && ( pPlayer->IsInAVehicle() ) )
		{
			//Update against a driving target
			UpdateVehicularEnemyTarget();
			return;
		}

		enemyPos = GetEnemy()->WorldSpaceCenter();
		bTargettingPlayer = GetEnemy()->IsPlayer();
	}
	else
	{
		enemyPos = m_vecAttackPosition;
		bTargettingPlayer = false;
	}

	// Direction towards the enemy
	Vector	targetDir = enemyPos - m_vecAttackPosition;
	VectorNormalize( targetDir );

	// Direction from the gunship to the enemy
	Vector	enemyDir = enemyPos - WorldSpaceCenter();
	VectorNormalize( enemyDir );

	float	targetDot = DotProduct( enemyDir, targetDir );
	float	lastSpeed = VectorNormalize( m_vecAttackVelocity );

	QAngle	chaseAngles, lastChaseAngles;

	VectorAngles( targetDir, chaseAngles );
	VectorAngles( m_vecAttackVelocity, lastChaseAngles );

	float yawDiff = UTIL_AngleDiff( chaseAngles[YAW], lastChaseAngles[YAW] );

	int	maxYaw;

	if ( bTargettingPlayer )
	{
		//FIXME: Set by skill level
		maxYaw = 6;
	}
	else
	{
		maxYaw = 30;
	}	

	yawDiff = clamp( yawDiff, -maxYaw, maxYaw );

	chaseAngles[PITCH]	= 0.0f;
	chaseAngles[ROLL]	= 0.0f;
	
	if ( ( targetDot <= 0.0f ) && ( bTargettingPlayer ) )
	{
		// Turn away from our target if we're behind it
		chaseAngles[YAW] = UTIL_AngleMod( lastChaseAngles[YAW] - yawDiff );
	}
	else
	{
		chaseAngles[YAW] = UTIL_AngleMod( lastChaseAngles[YAW] + yawDiff );
	}

	AngleVectors( chaseAngles, &targetDir );

	//Debug info
	if ( g_debug_gunship.GetInt() == GUNSHIP_DEBUG_STITCHING )
	{
		// Final position
		NDebugOverlay::Cross3D( m_vecAttackPosition, -Vector(2,2,2), Vector(2,2,2), 0, 0, 255, true, 4.0f );
		NDebugOverlay::Line( m_vecAttackPosition, m_vecAttackPosition + ( m_vecAttackVelocity * 64 ), 255, 0, 0, true, 4.0f );
	}

	// If we're targetting the player, or not close enough, then just stitch along
	if ( ( bTargettingPlayer ) || ( ( m_vecAttackPosition - enemyPos ).Length() > 64.0f ) )
	{
		// Update our new velocity
		m_vecAttackVelocity = targetDir * lastSpeed;

		// Move along that velocity for this step in time
		m_vecAttackPosition += ( m_vecAttackVelocity * 0.1f );
		m_vecAttackPosition.z = enemyPos.z;
		m_bNoiseyShot = false;
	}
	else
	{
		// Otherwise always continue to hit an NPC when close enough
		m_vecAttackPosition = enemyPos + RandomVector( -4, 4 );
		m_bNoiseyShot = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_CombineGunship::GetMissileTarget( void )
{
	return GetEnemy()->GetAbsOrigin();
}

//------------------------------------------------------------------------------
// Purpose : Get the target position for the enemy- the position we fire upon.
//			 this is often modified by m_flAttackOffset to provide the 'stitching'
//			 behavior that's so popular with the kids these days (sjb)
//
// Input   : vGunPosition - location of gunship's muzzle
//		   : pTarget = vector to paste enemy target into.
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_CombineGunship::GetEnemyTarget( void )
{
	// Make sure we have an enemy
	if ( GetEnemy() == NULL )
		return m_vecAttackPosition;

	// If we're locked onto a missile, use special code to try and destroy it
	if ( IsTargettingMissile() )
		return GetMissileTarget();

	return m_vecAttackPosition;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::DoCustomImpactEffect( const trace_t &tr )
{
	// These glow effects don't sort properly, so they're cut for E3 2003 (sjb)
#if 0 
	CEffectData data;

	data.m_vOrigin = tr.endpos;
	data.m_vNormal = vec3_origin;
	data.m_vAngles = vec3_angle;

	DispatchEffect( "GunshipImpact", data );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Make the gunship's signature blue tracer!
// Input  : &vecTracerSrc - 
//			&tr - 
//			iTracerType - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	switch ( iTracerType )
	{
	case TRACER_LINE:
		{
			float flTracerDist;
			Vector vecDir;
			Vector vecEndPos;

			vecDir = tr.endpos - vecTracerSrc;

			flTracerDist = VectorNormalize( vecDir );

			UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 16000, true, "GunshipTracer" );
		}
		break;

	default:
		BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecDir - 
//			*ptr - 
// Output : int
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	// Reflect bullets
	if ( info.GetDamageType() & DMG_BULLET )
	{
		if ( random->RandomInt( 0, 2 ) == 0 )
		{
			m_fEffects |= EF_MUZZLEFLASH;

			Vector vecRicochetDir = vecDir * -1;

			vecRicochetDir.x += random->RandomFloat( -0.5, 0.5 );
			vecRicochetDir.y += random->RandomFloat( -0.5, 0.5 );
			vecRicochetDir.z += random->RandomFloat( -0.5, 0.5 );

			VectorNormalize( vecRicochetDir );

			Vector end = ptr->endpos + vecRicochetDir * 1024;
			UTIL_Tracer( ptr->endpos, end, entindex(), TRACER_DONT_USE_ATTACHMENT, 3500 );
		}

		// If it's time, pick up a new target
		if ( ( m_flTargetPersistenceTime < gpGlobals->curtime ) || ( info.GetAttacker()->IsPlayer() ) )
		{
			if ( ( GetEnemy() != info.GetAttacker() ) && ( m_bIsFiring == false ) )
			{
				SetEnemy( info.GetAttacker() );
			}
		}

		return;
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//------------------------------------------------------------------------------
// Damage filtering
//------------------------------------------------------------------------------
int	CNPC_CombineGunship::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	// Ignore mundane bullet damage.
	if ( ( inputInfo.GetDamageType() & DMG_ARMOR_PIERCING ) == false )
		return 0;

	// Only take blast damage
	CTakeDamageInfo info = inputInfo;

	// Make a pain sound
	EmitSound( "NPC_CombineGunship.Pain" );

	// Take a percentage of our health away
	info.SetDamage( m_iMaxHealth / 5 );
	
	// Find out which "stage" we're at in our health
	int healthIncrement = 5 - ( m_iHealth / ( m_iMaxHealth / 5 ) );

	switch ( healthIncrement )
	{
	case 0:
		m_OnFirstDamage.FireOutput( this, this );
		break;
	
	case 1:
		m_OnSecondDamage.FireOutput( this, this );
		break;
	
	case 2:
		m_OnThirdDamage.FireOutput( this, this );
		break;
	
	case 3:
		m_OnFourthDamage.FireOutput( this, this );
		break;
	}

	if ( m_pSmokeTrail == NULL )
	{
		m_pSmokeTrail = SmokeTrail::CreateSmokeTrail();
	}
	
	if ( m_pSmokeTrail )
	{
		m_pSmokeTrail->m_SpawnRate			= 48;
		m_pSmokeTrail->m_ParticleLifetime	= 2.5f;
		
		m_pSmokeTrail->m_StartColor.Init( 0.25f, 0.25f, 0.25f );
		m_pSmokeTrail->m_EndColor.Init( 0.0, 0.0, 0.0 );
		
		m_pSmokeTrail->m_StartSize		= 24;
		m_pSmokeTrail->m_EndSize		= 128;
		m_pSmokeTrail->m_SpawnRadius	= 4;
		m_pSmokeTrail->m_MinSpeed		= 8;
		m_pSmokeTrail->m_MaxSpeed		= 64;
		m_pSmokeTrail->m_Opacity		= 0.25f;

		m_pSmokeTrail->SetLifetime( 4.0f );
		m_pSmokeTrail->FollowEntity( entindex(), 9 );
	}

	Vector	damageDir = info.GetDamageForce();
	VectorNormalize( damageDir );

	ApplyAbsVelocityImpulse( damageDir * 400.0f );
	
	SetEnemy( info.GetAttacker() );

	return BaseClass::OnTakeDamage_Alive( info );
}


//------------------------------------------------------------------------------
// Purpose : The proper way to begin the gunship cannon firing at the enemy.
// Input   : iBurstSize - the size of the burst, in rounds.
//------------------------------------------------------------------------------
void CNPC_CombineGunship::StartCannonBurst( int iBurstSize )
{
	m_iBurstSize = iBurstSize;
	m_bBurstHit = false;

	m_flTimeNextAttack = gpGlobals->curtime;

	// Start up the cannon sound.
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundChangeVolume( m_pCannonSound, 1.0, 0.1f );

	m_bIsFiring = true;
}


//------------------------------------------------------------------------------
// Purpose : The proper way to cease the gunship cannon firing. 
//------------------------------------------------------------------------------
void CNPC_CombineGunship::StopCannonBurst( void )
{
	m_bIsFiring = false;
	m_bPreFire = false;

	m_iBurstSize = 0;
	m_flTimeNextAttack = gpGlobals->curtime + m_flBurstDelay;

	// Stop the cannon sound.
	if ( m_pCannonSound != NULL )
	{
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pCannonSound, 0.0, 0.1f );
	}

	EmitSound( "NPC_CombineGunship.CannonStopSound" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::StopLoopingSounds( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	controller.SoundDestroy( m_pCannonSound );
	m_pCannonSound = NULL;

	controller.SoundDestroy( m_pRotorSound );
	m_pRotorSound = NULL;

	controller.SoundDestroy( m_pAirIntakeSound );
	m_pAirIntakeSound = NULL;

	controller.SoundDestroy( m_pAirExhaustSound );
	m_pAirExhaustSound = NULL;

	controller.SoundDestroy( m_pAirBlastSound );
	m_pAirBlastSound = NULL;

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::UpdateTrackNavigation( void )
{
	BaseClass::UpdateTrackNavigation();
}

//-----------------------------------------------------------------------------
// Purpose: Figure out our desired position along our current path
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::UpdateDesiredPosition( void )
{
	// See which method to use for attacking
	if ( m_bIsGroundAttacking )
	{
		// Stand still if attacking the ground
		SetDesiredPosition( WorldSpaceCenter() );
	}
	else
	{
		// Get our new point along our path to fly to (leads out ahead of us)
		SetDesiredPosition( GetPathGoalPoint( GUNSHIP_LEAD_DISTANCE + GetPathLeadBias() ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_CombineGunship::ExplodeTouch( CBaseEntity *pOther )
{
	SelfDestruct();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::IsValidEnemy( CBaseEntity *pEnemy )
{
	// Always track missiles
	if ( FClassnameIs( pEnemy, "rpg_missile" ) )
		return true;

	// If we're shooting off a burst, don't pick up a new enemy
	if ( ( m_bIsFiring ) && ( ( GetEnemy() == NULL ) || ( GetEnemy() != pEnemy ) ) )
		return false;

	// Don't allow a new target if we're still looking at the old one
	if ( GetEnemy() && GetEnemy()->IsAlive() && ( pEnemy != GetEnemy() ) && ( m_flTargetPersistenceTime > gpGlobals->curtime ) )
		return false;

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: Tells us whether or not we're targetting an incoming missile
//-----------------------------------------------------------------------------
bool CNPC_CombineGunship::IsTargettingMissile( void )
{
	if ( GetEnemy() == NULL )
		return false;

	if ( FClassnameIs( GetEnemy(), "rpg_missile" ) == false )
		return false;

	return true;
}

AI_BEGIN_CUSTOM_NPC( npc_combinegunship, CNPC_CombineGunship )

//	DECLARE_TASK(  )

	DECLARE_ACTIVITY( ACT_GUNSHIP_PATROL );
	DECLARE_ACTIVITY( ACT_GUNSHIP_HOVER );
	DECLARE_ACTIVITY( ACT_GUNSHIP_CRASH );

	//DECLARE_CONDITION( COND_ )

	//=========================================================
//	DEFINE_SCHEDULE
//	(
//		SCHED_DUMMY,
//
//		"	Tasks"
//		"		TASK_FACE_ENEMY			0"
//		"	"
//		"	Interrupts"
//	)


AI_END_CUSTOM_NPC()

