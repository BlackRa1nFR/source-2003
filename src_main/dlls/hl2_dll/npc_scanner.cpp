//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "AI_BaseNPC_Flyer.h"
#include "soundenvelope.h"
#include "AI_Default.h"
#include "AI_Node.h"
#include "AI_Activity.h"  
#include "AI_Hint.h"
#include "AI_Navigator.h"
#include "ai_moveprobe.h"
#include "AI_Squad.h"
#include "beam_shared.h"
#include "Sprite.h"
#include "explode.h"
#include "globalstate.h"
#include "soundent.h"
#include "basecombatweapon.h"
#include "ndebugoverlay.h"
#include "shake.h"		// For screen fade
#include "AI_SquadSlot.h"
#include "entitylist.h"
#include "npc_citizen17.h"
#include "steamjet.h"
#include "gib.h"
#include "smoke_trail.h"
#include "spotlightend.h"
#include "beam_flags.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"

//-----------------------------------------------------------------------------
// Parameters for how the scanner relates to citizens.
//-----------------------------------------------------------------------------
#define SCANNER_CIT_INSPECT_DELAY		1		// Check for citizens this often
#define	SCANNER_CIT_INSPECT_GROUND_DIST	500		// How far to look for citizens to inspect
#define	SCANNER_CIT_INSPECT_FLY_DIST	1500	// How far to look for citizens to inspect

#define SCANNER_CIT_INSPECT_LENGTH		15		// How long does the inspection last
#define SCANNER_HINT_INSPECT_LENGTH		15		// How long does the inspection last
#define SCANNER_SOUND_INSPECT_LENGTH	10		// How long does the inspection last

#define SCANNER_HINT_INSPECT_DELAY		20		// Check for hint nodes this often
	
#define	SCANNER_BANK_RATE				15
#define	SCANNER_MAX_SPEED				400
#define SCANNER_SQUAD_FLY_DIST			500		// How far to scanners stay apart
#define SCANNER_SQUAD_HELP_DIST			4000	// How far will I fly to help

#define SCANNER_SPOTLIGHT_NEAR_DIST		200
#define SCANNER_SPOTLIGHT_FAR_DIST		300
#define SCANNER_SPOTLIGHT_FLY_HEIGHT	200
#define SCANNER_NOSPOTLIGHT_FLY_HEIGHT	60

#define SCANNER_GAS_DAMAGE_DIST			500
#define SCANNER_FLASH_MIN_DIST			900		// How far does flash effect enemy
#define SCANNER_FLASH_MAX_DIST			1200	// How far does flash effect enemy
#define	SCANNER_FLASH_MAX_VALUE			240		// How bright is maximum flash

#define SCANNER_PHOTO_NEAR_DIST			120
#define SCANNER_PHOTO_FAR_DIST			180

#define SCANNER_ATTACK_NEAR_DIST		150		// Fly attack min distance
#define SCANNER_ATTACK_FAR_DIST			300		// Fly attack max distance
#define SCANNER_ATTACK_RANGE			350		// Attack max distance
#define	SCANNER_ATTACK_MIN_DELAY		2		// Min time between attacks
#define	SCANNER_ATTACK_MAX_DELAY		4		// Max time between attacks
#define	SCANNER_EVADE_TIME				1		// How long to evade after take damage

#define	SCANNER_NUM_GIBS				6		// Number of gibs in gib file

//------------------------------------
// Spawnflags
//------------------------------------
#define SF_CSCANNER_NO_DYNAMIC_LIGHT	(1 << 16)


ConVar	sk_scanner_health( "sk_scanner_health","0");
ConVar	sk_scanner_dmg_gas( "sk_scanner_dmg_gas","0");
ConVar	sk_scanner_dmg_dive( "sk_scanner_dmg_dive","0");

//-----------------------------------------------------------------------------
// Private activities.
//-----------------------------------------------------------------------------
static int ACT_SCANNER_SMALL_FLINCH_ALERT = 0;
static int ACT_SCANNER_SMALL_FLINCH_COMBAT = 0;
static int ACT_SCANNER_IDLE_ALERT = 0;
static int ACT_SCANNER_INSPECT = 0;
static int ACT_SCANNER_WALK_ALERT = 0;
static int ACT_SCANNER_WALK_COMBAT = 0;
static int ACT_SCANNER_FLARE = 0;
static int ACT_SCANNER_RETRACT = 0;
static int ACT_SCANNER_FLARE_PRONGS = 0;
static int ACT_SCANNER_RETRACT_PRONGS = 0;

//-----------------------------------------------------------------------------
// Custom interrupt conditions
//-----------------------------------------------------------------------------
enum
{
	COND_SCANNER_FLY_CLEAR = LAST_SHARED_CONDITION,
	COND_SCANNER_FLY_BLOCKED,							
	COND_SCANNER_HAVE_INSPECT_TARGET,							
	COND_SCANNER_INSPECT_DONE,							
	COND_SCANNER_CAN_PHOTOGRAPH,
	COND_SCANNER_SPOT_ON_TARGET,
};


//-----------------------------------------------------------------------------
// Custom schedules.
//-----------------------------------------------------------------------------
enum
{
	SCHED_SCANNER_PATROL = LAST_SHARED_SCHEDULE,
	SCHED_SCANNER_SPOTLIGHT_HOVER,
	SCHED_SCANNER_SPOTLIGHT_INSPECT_POS,
	SCHED_SCANNER_SPOTLIGHT_INSPECT_CIT,
	SCHED_SCANNER_PHOTOGRAPH_HOVER,
	SCHED_SCANNER_PHOTOGRAPH,
	SCHED_SCANNER_ATTACK_HOVER,
	SCHED_SCANNER_ATTACK_FLASH,
	SCHED_SCANNER_ATTACK_GAS,
	SCHED_SCANNER_ATTACK_DIVEBOMB,
	SCHED_SCANNER_CHASE_ENEMY,
	SCHED_SCANNER_CHASE_TARGET,
};


//-----------------------------------------------------------------------------
// Custom tasks.
//-----------------------------------------------------------------------------
enum
{
	TASK_SCANNER_SET_FLY_PHOTO = LAST_SHARED_TASK,
	TASK_SCANNER_SET_FLY_PATROL,
	TASK_SCANNER_SET_FLY_CHASE,
	TASK_SCANNER_SET_FLY_SPOT,
	TASK_SCANNER_SET_FLY_ATTACK,
	TASK_SCANNER_SET_FLY_DIVE,
	TASK_SCANNER_PHOTOGRAPH,
	TASK_SCANNER_ATTACK_PRE_FLASH,
	TASK_SCANNER_ATTACK_FLASH,
	TASK_SCANNER_ATTACK_PRE_GAS,
	TASK_SCANNER_ATTACK_GAS,
	TASK_SCANNER_SPOT_INSPECT_ON,
	TASK_SCANNER_SPOT_INSPECT_WAIT,
	TASK_SCANNER_SPOT_INSPECT_OFF,
	TASK_SCANNER_CLEAR_INSPECT_TARGET,
};


//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionScannerInspect				= 0;
int	g_interactionScannerInspectBegin		= 0;
int g_interactionScannerInspectHandsUp		= 0;
int g_interactionScannerInspectShowArmband	= 0;//<<TEMP>>still to be completed
int	g_interactionScannerInspectDone			= 0;
int g_interactionScannerSupportEntity		= 0;
int g_interactionScannerSupportPosition		= 0;


//-----------------------------------------------------------------------------
// Attachment points.
//-----------------------------------------------------------------------------
#define SCANNER_ATTACHMENT_LIGHT		0
#define SCANNER_ATTACHMENT_FLASH		1
#define SCANNER_ATTACHMENT_LPRONG	2
#define SCANNER_ATTACHMENT_RPRONG	3


//-----------------------------------------------------------------------------
// Other defines.
//-----------------------------------------------------------------------------
#define SCANNER_MAX_BEAMS		4


//-----------------------------------------------------------------------------
// States for the scanner's sound.
//-----------------------------------------------------------------------------
enum ScannerFlyMode_t
{
	SCANNER_FLY_PHOTO = 0,		// Fly close to photograph entity
	SCANNER_FLY_PATROL,			// Fly slowly around the enviroment
	SCANNER_FLY_CHASE,			// Fly quickly around the enviroment
	SCANNER_FLY_SPOT,			// Fly above enity in spotlight position
	SCANNER_FLY_ATTACK,			// Get in my enemies face for spray or flash
	SCANNER_FLY_DIVE,			// Divebomb - only done when dead
};

enum ScannerInspectAct_t
{
	SCANNER_INSACT_HANDS_UP,
	SCANNER_INSACT_SHOWARMBAND,
};

class CBeam;
class CSprite;
class SmokeTrail;
class CSpotlightEnd;


class CNPC_CScanner : public CAI_BaseFlyingBot
{
DECLARE_CLASS( CNPC_CScanner, CAI_BaseFlyingBot );

public:
	CNPC_CScanner();
	Class_T			Classify(void);

	void			Event_Killed( const CTakeDamageInfo &info );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	int				OnTakeDamage_Dying( const CTakeDamageInfo &info );
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void			Gib(void);

	bool			FValidateHintType(CAI_Hint *pHint);

	virtual int		TranslateSchedule( int scheduleType );
	Disposition_t	IRelationType(CBaseEntity *pTarget);
	int				GetSoundInterests( void );

	void			IdleSound(void);
	void			DeathSound(void);
	void			AlertSound(void);
	void			PainSound(void);

	void			NPCThink(void);
	
	void			PrescheduleThink(void);
	int				MeleeAttack1Conditions ( float flDot, float flDist );
	void			Precache(void);
	void			RunTask( const Task_t *pTask );
	virtual int		SelectSchedule(void);
	bool			ShouldPlayIdleSound( void );
	void			Spawn(void);
	void			StartTask( const Task_t *pTask );
	void			OnScheduleChange( void );

	void			InputDisableSpotlight( inputdata_t &inputdata );
	void			InputInspectTargetPhoto( inputdata_t &inputdata );
	void			InputInspectTargetSpotlight( inputdata_t &inputdata );

	void			InspectTarget( inputdata_t &inputdata, ScannerFlyMode_t eFlyMode );

	DEFINE_CUSTOM_AI;

public:
	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
	void			SpeakSentence( int sentenceType );

	// ------------------------------
	//	Inspecting
	// ------------------------------
	Vector			m_vInspectPos;
	float			m_fInspectEndTime;
	float			m_fCheckCitizenTime;	// Time to look for citizens to harass
	float			m_fCheckHintTime;		// Time to look for hints to inspect

	void			SetInspectTargetToEnt(CBaseEntity *pEntity, float fInspectDuration);
	void			SetInspectTargetToPos(const Vector &vInspectPos, float fInspectDuration);
	void			SetInspectTargetToHint(CAI_Hint *pHint, float fInspectDuration);
	void			ClearInspectTarget(void);
	bool			HaveInspectTarget(void);
	Vector			InspectTargetPosition(void);
	bool			IsValidInspectTarget(CBaseEntity *pEntity);
	CBaseEntity*	BestInspectTarget(void);
	void			RequestInspectSupport(void);

	// ------------------------
	//  Photographing
	// ------------------------
	float			m_fNextPhotographTime;
	CSprite*		m_pEyeFlash;

	void			TakePhoto(void);

	// ------------------------------
	//	Spotlight
	// ------------------------------
	Vector			m_vSpotlightTargetPos;
	Vector			m_vSpotlightCurrentPos;
	CBeam*			m_pSpotlight;
	CSpotlightEnd*	m_pSpotlightTarget;
	Vector			m_vSpotlightDir;
	Vector			m_vSpotlightAngVelocity;
	float			m_flSpotlightCurLength;
	float			m_flSpotlightMaxLength;
	float			m_flSpotlightGoalWidth;
	float			m_fNextSpotlightTime;
	int				m_nHaloSprite;
	
	void			SpotlightUpdate(void);
	Vector			SpotlightTargetPos(void);
	Vector			SpotlightCurrentPos(void);
	void			SpotlightCreate(void);
	void			SpotlightDestroy(void);

protected:

	void ClampSteer( Vector &vecSteerAbs, Vector &vecSteerRel, Vector &forward, Vector &right, Vector &up );
	void SteerSeek( Vector &vecSteer, const Vector &vecTarget );

	Vector m_vecAccelMin;
	Vector m_vecAccelMax;
	float m_flFlightSpeed;
	float m_flStoppingDistance;

	virtual void		StopLoopingSounds(void);

	CSoundPatch		*m_pEngineSound;

	// -------------------
	//  Movement
	// -------------------
	float				m_fNextFlySoundTime;
	ScannerFlyMode_t	m_nFlyMode;

	bool				OverrideMove(float flInterval);
	bool				OverridePathMove( float flInterval );
	void				MoveToTarget(float flInterval, const Vector &MoveTarget);
	void				MoveToSpotlight(float flInterval);
	void				MoveToPhotograph(float flInterval);
	void				MoveToAttack(float flInterval);
	void				MoveToDivebomb(float flInterval);
	void				MoveExecute_Alive(float flInterval);
	float				MinGroundDist(void);
	Vector				VelocityToEvade(CBaseCombatCharacter *pEnemy);
	void				SetBanking(float flInterval);
	void				PlayFlySound(void);

	// ---------------------
	//  Attacks
	// ---------------------
	SmokeTrail*			m_pSmokeTrail;
	Vector				m_vGasDirection;

	bool				m_bNoLight;	//FIXME: E3 Hack

	void				AttackPreFlash(void);
	void				AttackFlash(void);
	void				AttackFlashBlind(void);
	void				AttackPreGas(void);
	void				AttackGas(void);
	void				AttackGasDamage(void);
	void				AttackDivebomb(void);
	void				AttackDivebombCollide(float flInterval);

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CNPC_CScanner )

	DEFINE_FIELD( CNPC_CScanner, m_vecAccelMin,				FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_CScanner, m_vecAccelMax,				FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_CScanner, m_flFlightSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CScanner, m_flStoppingDistance,		FIELD_FLOAT ),
	DEFINE_SOUNDPATCH( CNPC_CScanner, m_pEngineSound ),
	DEFINE_FIELD( CNPC_CScanner, m_vInspectPos,				FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_CScanner, m_fInspectEndTime,			FIELD_TIME ),
	DEFINE_FIELD( CNPC_CScanner, m_fCheckCitizenTime,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_CScanner, m_fCheckHintTime,			FIELD_TIME ),
	DEFINE_FIELD( CNPC_CScanner, m_fNextPhotographTime,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_CScanner, m_pEyeFlash,				FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_CScanner, m_vSpotlightTargetPos,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CNPC_CScanner, m_vSpotlightCurrentPos,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CNPC_CScanner, m_pSpotlight,				FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_CScanner, m_pSpotlightTarget,		FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_CScanner, m_vSpotlightDir,			FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_CScanner, m_vSpotlightAngVelocity,	FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_CScanner, m_flSpotlightCurLength,	FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_CScanner, m_fNextSpotlightTime,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_CScanner, m_nHaloSprite,				FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CScanner, m_fNextFlySoundTime,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_CScanner, m_nFlyMode,				FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_CScanner, m_pSmokeTrail,				FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_CScanner, m_vGasDirection,			FIELD_VECTOR ),

	DEFINE_KEYFIELD( CNPC_CScanner, m_flSpotlightMaxLength,	FIELD_FLOAT,	"SpotlightLength"),
	DEFINE_KEYFIELD( CNPC_CScanner, m_flSpotlightGoalWidth,	FIELD_FLOAT,	"SpotlightWidth"),

	DEFINE_KEYFIELD( CNPC_CScanner, m_bNoLight, FIELD_BOOLEAN, "SpotlightDisabled" ),

	DEFINE_INPUTFUNC( CNPC_CScanner, FIELD_VOID, "DisableSpotlight", InputDisableSpotlight ),
	DEFINE_INPUTFUNC( CNPC_CScanner, FIELD_STRING, "InspectTargetPhoto", InputInspectTargetPhoto ),
	DEFINE_INPUTFUNC( CNPC_CScanner, FIELD_STRING, "InspectTargetSpotlight", InputInspectTargetSpotlight ),

END_DATADESC()


LINK_ENTITY_TO_CLASS(npc_cscanner, CNPC_CScanner);

CNPC_CScanner::CNPC_CScanner()
{
#ifdef _DEBUG
	m_vInspectPos.Init();
	m_vSpotlightTargetPos.Init();
	m_vSpotlightCurrentPos.Init();
	m_vSpotlightDir.Init();
	m_vSpotlightAngVelocity.Init();
	m_vCurrentBanking.Init();
	m_vGasDirection.Init();
#endif

	m_pEngineSound = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this NPC's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_CScanner::Classify(void)
{
	return(CLASS_SCANNER);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_CScanner::GetSoundInterests( void )
{
	return SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_DANGER;
}

//------------------------------------------------------------------------------
// Purpose : Override to split in two when attacked
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_CScanner::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Turn off my spotlight when shot
	SpotlightDestroy();
	m_fNextSpotlightTime = gpGlobals->curtime + 2.0;

	return (BaseClass::OnTakeDamage_Alive( info ));
}

//------------------------------------------------------------------------------
// Purpose : Override to split in two when attacked
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_CScanner::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	// do the damage
	m_iHealth -= info.GetDamage();
	
	if (m_iHealth < -50)
	{
		Gib();
		return 1;
	}
	else
	{
		return VPhysicsTakeDamage( info );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_CScanner::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( info.GetDamageType() & DMG_BULLET)
	{
		g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
	}
	BaseClass::TraceAttack( info, vecDir, ptr );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::Gib(void)
{
	// Sparks
	for (int i = 0; i < 4; i++)
	{
		Vector sparkPos = GetAbsOrigin();
		sparkPos.x += random->RandomFloat(-12,12);
		sparkPos.y += random->RandomFloat(-12,12);
		sparkPos.z += random->RandomFloat(-12,12);
		g_pEffects->Sparks(sparkPos);
	}
	// Smoke
	UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);

	// Light
	CBroadcastRecipientFilter filter;
	te->DynamicLight( filter, 0.0,
			&GetAbsOrigin(), 255, 180, 100, 0, 100, 0.1, 0 );

	// Blow up sound
	CPASAttenuationFilter filter2( this, "NPC_CScanner.Gib" );
	EmitSound( filter2, entindex(), "NPC_CScanner.Gib" );

	CGib::SpawnSpecificGibs( this, SCANNER_NUM_GIBS, 300, 400, "models/gibs/combot_gibs.mdl");

	if (m_pSmokeTrail)
	{
		m_pSmokeTrail->m_ParticleLifetime = 0;
		UTIL_RemoveImmediate(m_pSmokeTrail);
		m_pSmokeTrail = NULL;
	}

	UTIL_Remove(this);
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::StopLoopingSounds(void)
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pEngineSound );
	m_pEngineSound = NULL;

	BaseClass::StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::Event_Killed( const CTakeDamageInfo &info )
{
	m_OnDeath.FireOutput( info.GetAttacker(), this );

	// Interrupt whatever schedule I'm on
	SetCondition(COND_SCHEDULE_DONE);

	// Remove spotlight
	SpotlightDestroy();

	// Remove sprite
	UTIL_Remove(m_pEyeFlash);
	m_pEyeFlash = NULL;

	// If I have an enemy and I'm up high, do a dive bomb
	if (GetEnemy() != NULL)
	{
		float fDist = GetLocalOrigin().z - GetEnemy()->GetLocalOrigin().z;
		if (fDist > 200)
		{	
			AttackDivebomb();
			return;
		}
	}

	// Otherwise, turn into a physics object and fall to the ground
	BaseClass::Event_Killed( info );

#if 0
	// Create the object in the physics system
	VPhysicsDestroyObject(); // remove AI shadow

	// this model doesn't have a .PHY file anymore!!!
	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal( GetSolid(), false );

	Vector vPhysVel = vec3_origin;
	AngularImpulse vPhysAVel;
	vPhysAVel.x  = random->RandomFloat( -5,5 );
	vPhysAVel.y  = random->RandomFloat( -5,5 );
	vPhysAVel.z  = random->RandomFloat( -5,5 );

	// Calculate death velocity
	CBaseEntity *pForce = info.GetInflictor();
	Vector forceVector(0,0,0);
	if ( !pForce )
	{
		pForce = info.GetAttacker();
	}
	
	if ( pForce )
	{
		// If the damage is a blast, point the force vector higher than usual, this gives 
		// the ragdolls a bodacious "really got blowed up" look.
		if( info.GetDamageType() & DMG_BLAST )
		{
			vPhysVel = GetLocalOrigin() + Vector(0, 0, GetSize().z) - pForce->GetLocalOrigin();
			VectorNormalize(vPhysVel);
			vPhysVel *= info.GetDamage() * 40;
		}
		else
		{
			vPhysVel = GetLocalOrigin() - pForce->GetLocalOrigin();
			VectorNormalize(vPhysVel);
			vPhysVel *= info.GetDamage();
		}
	}

	Vector vel = vPhysVel+GetCurrentVelocity();
	if ( !pPhysicsObject )
	{
		DevMsg(1, "Can't create physics for scanner!\n");
		return;
	}
	pPhysicsObject->AddVelocity(&vel,&vPhysAVel);
#endif
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackDivebombCollide(float flInterval)
{
	// ----------------------------------------
	//	Trace forward to see if I hit anything
	// ----------------------------------------
	Vector			checkPos = GetAbsOrigin() + (GetCurrentVelocity() * flInterval);
	trace_t			tr;
	CBaseEntity*	pHitEntity = NULL;
	AI_TraceHull( GetAbsOrigin(), checkPos, GetHullMins(), 
		GetHullMaxs(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if (tr.m_pEnt)
	{
		pHitEntity = tr.m_pEnt;

		// Did I hit an entity that isn't another manhack?
		if (pHitEntity && pHitEntity->Classify()!=CLASS_SCANNER)
		{
			CTakeDamageInfo info( this, this, sk_scanner_dmg_dive.GetFloat(), DMG_CLUB );
			CalculateMeleeDamageForce( &info, (tr.endpos - tr.startpos), tr.endpos );
			pHitEntity->TakeDamage( info );
			Gib();
		}
	}
	if (tr.fraction != 1.0)
	{
		// We've hit something so deflect our velocity based on the surface
		// norm of what we've hit
		if (flInterval > 0)
		{
			float moveLen	= (1.0 - tr.fraction)*(GetAbsOrigin() - checkPos).Length();
			Vector vBounceVel	= moveLen*tr.plane.normal/flInterval;

			// If I'm right over the ground don't push down
			if (vBounceVel.z < 0)
			{
				float floorZ = GetFloorZ(GetAbsOrigin());
				if (abs(GetAbsOrigin().z - floorZ) < 36)
				{
					vBounceVel.z = 0;
				}
			}
			SetCurrentVelocity( GetCurrentVelocity() + vBounceVel );
		}
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHitEntity );

		if (pBCC)
		{
			// Spawn some extra blood where we hit
			SpawnBlood(tr.endpos, pBCC->BloodColor(), sk_scanner_dmg_dive.GetFloat());
		}
		else
		{
			if (!(m_spawnflags	& SF_NPC_GAG))
			{
				// <<TEMP>> need better sound here...
				EmitSound( "NPC_CScanner.Shoot" );
			}
			// For sparks we must trace a line in the direction of the surface norm
			// that we hit.
			checkPos = GetAbsOrigin() - (tr.plane.normal * 24);

			AI_TraceLine( GetAbsOrigin(), checkPos,MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			if (tr.fraction != 1.0)
			{
				g_pEffects->Sparks( tr.endpos );

				CBroadcastRecipientFilter filter;
				te->DynamicLight( filter, 0.0,
					&GetAbsOrigin(), 255, 180, 100, 0, 50, 0.1, 0 );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Tells use whether or not the NPC cares about a given type of hint node.
// Input  : sHint - 
// Output : TRUE if the NPC is interested in this hint type, FALSE if not.
//-----------------------------------------------------------------------------
bool CNPC_CScanner::FValidateHintType(CAI_Hint *pHint)
{
	return(pHint->HintType() == HINT_WORLD_WINDOW);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
int CNPC_CScanner::TranslateSchedule( int scheduleType ) 
{
	switch ( scheduleType )
	{
		case SCHED_IDLE_STAND:
		{
			return SCHED_SCANNER_PATROL;
		}
	}
	return BaseClass::TranslateSchedule(scheduleType);
}


//-----------------------------------------------------------------------------
// Purpose: Emit sounds specific to the NPC's state.
//-----------------------------------------------------------------------------
void CNPC_CScanner::AlertSound(void)
{
	EmitSound( "NPC_CScanner.Alert" );
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::DeathSound(void)
{
	EmitSound( "NPC_CScanner.Die" );
}


//-----------------------------------------------------------------------------
// Purpose: Plays sounds while idle or in combat.
//-----------------------------------------------------------------------------
void CNPC_CScanner::IdleSound(void)
{
	if ( m_NPCState == NPC_STATE_COMBAT )
	{
		// dvs: the combat sounds should be related to what is happening, rather than random
		EmitSound( "NPC_CScanner.Combat" );
	}
	else
	{
		EmitSound( "NPC_CScanner.Idle" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Plays a sound when hurt.
//-----------------------------------------------------------------------------
void CNPC_CScanner::PainSound(void)
{
	EmitSound( "NPC_CScanner.Pain" );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CScanner::PlayFlySound(void)
{
	if ( IsMarkedForDeletion() )
	{
		return;
	}

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	//Setup the sound if we're not already
	if ( m_pEngineSound == NULL )
	{
		// Create the sound
		CPASAttenuationFilter filter( this );
		m_pEngineSound = controller.SoundCreate( filter, entindex(), CHAN_STATIC, "npc/scanner/cbot_fly_loop.wav", ATTN_NORM );

		Assert(m_pEngineSound);

		// Start the engine sound
		controller.Play( m_pEngineSound, 0.0f, 100.0f );
		controller.SoundChangeVolume( m_pEngineSound, 1.0f, 2.0f );
	}

	float	speed	 = GetCurrentVelocity().Length();
	float	flVolume = 0.25f + (0.75f*(speed/SCANNER_MAX_SPEED));
	int		iPitch	 = 80 + (20*(speed/SCANNER_MAX_SPEED));

	//Update our pitch and volume based on our speed
	controller.SoundChangePitch( m_pEngineSound, iPitch, 0.1f );
	controller.SoundChangeVolume( m_pEngineSound, flVolume, 0.1f );
}


//-----------------------------------------------------------------------------
// Purpose: Plays the engine sound.
//-----------------------------------------------------------------------------
void CNPC_CScanner::NPCThink(void)
{
	if (!IsAlive())
	{
		SetActivity((Activity)ACT_SCANNER_RETRACT_PRONGS);
		StudioFrameAdvance( );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
		BaseClass::NPCThink();
		SpotlightUpdate();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::Precache(void)
{
	//
	// Model.
	//
	engine->PrecacheModel("models/combot.mdl");
	engine->PrecacheModel("models/gibs/combot_gibs.mdl");

	//
	// Sprites.
	//
	engine->PrecacheModel("sprites/spotlight.vmt");
	m_nHaloSprite		= engine->PrecacheModel("sprites/blueflare1.vmt");

	enginesound->PrecacheSound( "npc/scanner/cbot_fly_loop.wav" );

	BaseClass::Precache();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
#define SCANNER_SENTENCE_ATTENTION	0
#define SCANNER_SENTENCE_HANDSUP	1
#define SCANNER_SENTENCE_PROCEED	2
#define SCANNER_SENTENCE_CURIOUS	3
void CNPC_CScanner::SpeakSentence( int sentenceType )
{
	

	if		(sentenceType == SCANNER_SENTENCE_ATTENTION)
	{
		EmitSound( "NPC_CScanner.Attention" );
	}
	else if (sentenceType == SCANNER_SENTENCE_HANDSUP)
	{
		EmitSound( "NPC_CScanner.Scan" );
	}
	else if (sentenceType == SCANNER_SENTENCE_PROCEED)
	{
		EmitSound( "NPC_CScanner.Proceed" );
	}
	else if (sentenceType == SCANNER_SENTENCE_CURIOUS)
	{
		EmitSound( "NPC_CScanner.Curious" );
	}
}


//------------------------------------------------------------------------------
// Purpose: Request help inspecting from other squad members
//------------------------------------------------------------------------------
void CNPC_CScanner::RequestInspectSupport(void)
{
	if (m_pSquad)
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			if (pSquadMember != this)
			{
				if (GetTarget())
				{
					pSquadMember->HandleInteraction(g_interactionScannerSupportEntity,((void *)((CBaseEntity*)GetTarget())),this);
				}
				else
				{
					pSquadMember->HandleInteraction(g_interactionScannerSupportPosition,((void *)m_vInspectPos.Base()),this);
				}
			}
		}
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CNPC_CScanner::IsValidInspectTarget(CBaseEntity *pEntity)
{
	// If a citizen, make sure he can be inspected again
	if (pEntity->Classify() == CLASS_CITIZEN_PASSIVE)
	{
		if (((CNPC_Citizen*)pEntity)->m_fNextInspectTime > gpGlobals->curtime)
		{
			return false;
		}
	}

	// Make sure no other squad member has already chosen to 
	// inspect this entity
	if (m_pSquad)
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			if (pSquadMember->GetTarget() == pEntity)
			{
				return false;
			}
		}
	}
	return true;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
CBaseEntity* CNPC_CScanner::BestInspectTarget(void)
{

	CBaseEntity*	pBestEntity = NULL;
	float			fBestDist	= MAX_COORD_RANGE;
	float			fTestDist;

	CBaseEntity *pEntity = NULL;

	// If I have a spotlight, search from the spotlight position
	// otherwise search from my position
	Vector	vSearchOrigin;
	float	fSearchDist;
	if (m_pSpotlightTarget != NULL)
	{
		vSearchOrigin	= m_pSpotlightTarget->GetLocalOrigin();
		fSearchDist		= SCANNER_CIT_INSPECT_GROUND_DIST;
	}
	else
	{
		vSearchOrigin	= GetLocalOrigin();
		fSearchDist		= SCANNER_CIT_INSPECT_FLY_DIST;
	}

	for ( CEntitySphereQuery sphere( vSearchOrigin, fSearchDist ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if ((pNPC && pNPC->Classify() == CLASS_CITIZEN_PASSIVE) || 
			(pEntity->GetFlags() & FL_CLIENT)			)
		{
			if (pEntity->GetFlags() & FL_NOTARGET)
			{
				continue;
			}

			if (!pEntity->IsAlive())
			{
				continue;
			}

			fTestDist = (GetLocalOrigin() - pEntity->EyePosition()).Length();
			if (fTestDist < fBestDist)
			{
				if( IsValidInspectTarget(pEntity) )
				{
					fBestDist	= fTestDist;
					pBestEntity	= pEntity; 
				}
			}
		}
	}
	return pBestEntity;
}


//------------------------------------------------------------------------------
// Purpose : Clears any previous inspect target and set inspect target to
//			 the given entity and set the durection of the inspection
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::SetInspectTargetToEnt(CBaseEntity *pEntity, float fInspectDuration)
{
	ClearInspectTarget();
	SetTarget(pEntity);
	
	m_fInspectEndTime = gpGlobals->curtime + fInspectDuration;
}


//------------------------------------------------------------------------------
// Purpose : Clears any previous inspect target and set inspect target to
//			 the given hint node and set the durection of the inspection
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::SetInspectTargetToHint(CAI_Hint *pHint, float fInspectDuration)
{
	ClearInspectTarget();

	// --------------------------------------------
	// Figure out the location that the hint hits
	// --------------------------------------------
	Vector vHintDir	= UTIL_YawToVector( pHint->Yaw() );

	Vector vHintOrigin;
	pHint->GetPosition(this,&vHintOrigin);

	Vector vHintEnd		= vHintOrigin + (vHintDir * 500);
	trace_t tr;
	AI_TraceLine ( vHintOrigin, vHintEnd, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction == 1.0)
	{
		Msg("ERROR: Scanner hint node not facing a surface!\n");
	}
	else
	{
		m_pHintNode		= pHint;
		m_vInspectPos	= tr.endpos;
		m_pHintNode->Lock(this);

		m_fInspectEndTime = gpGlobals->curtime + fInspectDuration;
	}
}


//------------------------------------------------------------------------------
// Purpose : Clears any previous inspect target and set inspect target to
//			 the given position and set the durection of the inspection
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::SetInspectTargetToPos(const Vector &vInspectPos, float fInspectDuration)
{
	ClearInspectTarget();
	m_vInspectPos		= vInspectPos;

	m_fInspectEndTime	= gpGlobals->curtime + fInspectDuration;
}


//------------------------------------------------------------------------------
// Purpose: Clears out any previous inspection targets
//------------------------------------------------------------------------------
void CNPC_CScanner::ClearInspectTarget(void)
{
	// Unlock hint node
	if (m_pHintNode)
	{
		m_pHintNode->Unlock(SCANNER_HINT_INSPECT_LENGTH);
	}

	if (m_IdealNPCState != NPC_STATE_SCRIPT)
	{
		SetTarget( NULL );
	}
	m_pHintNode		= NULL;
	m_vInspectPos	= vec3_origin;
}


//------------------------------------------------------------------------------
// Purpose : Returns true if there is a position to be inspected.
//------------------------------------------------------------------------------
bool CNPC_CScanner::HaveInspectTarget(void)
{
	if (GetTarget() != NULL)
	{
		return true;
	}
	if (m_vInspectPos != vec3_origin)
	{
		return true;
	}
	return false;
}


//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
Vector CNPC_CScanner::InspectTargetPosition(void)
{
	if (GetTarget() != NULL)
	{
		// If in spotlight mode, aim for ground below target unless is client
		if (m_nFlyMode	== SCANNER_FLY_SPOT && !(GetTarget()->GetFlags() & FL_CLIENT))
		{
			Vector vInspectPos;
			vInspectPos.x	= GetTarget()->GetLocalOrigin().x;
			vInspectPos.y	= GetTarget()->GetLocalOrigin().y;
			vInspectPos.z	= GetFloorZ(GetTarget()->GetLocalOrigin());
			return vInspectPos;
		}
		// Otherwise aim for eyes
		else
		{
			return GetTarget()->EyePosition();
		}
	}
	else if (m_vInspectPos != vec3_origin)
	{
		return m_vInspectPos;
	}
	else
	{
		Msg("InspectTargetPosition called with no target!\n");
		return m_vInspectPos;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Tells the scanner to go photograph an entity.
// Input  : String name or classname of the entity to inspect.
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputInspectTargetPhoto(inputdata_t &inputdata)
{
	InspectTarget(inputdata, SCANNER_FLY_PHOTO);
}


//-----------------------------------------------------------------------------
// Purpose: Tells the scanner to go spotlight an entity.
// Input  : String name or classname of the entity to inspect.
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputInspectTargetSpotlight(inputdata_t &inputdata)
{
	InspectTarget(inputdata, SCANNER_FLY_SPOT);
}


//-----------------------------------------------------------------------------
// Purpose: Tells the scanner to go photo or spotlight an entity.
// Input  : String name or classname of the entity to inspect.
//-----------------------------------------------------------------------------
void CNPC_CScanner::InspectTarget(inputdata_t &inputdata, ScannerFlyMode_t eFlyMode)
{
	CBaseEntity *pEnt = gEntList.FindEntityGeneric(NULL, inputdata.value.String(), this, inputdata.pActivator);
	if ((pEnt != NULL) && (pEnt->pev != NULL))
	{
		SetInspectTargetToEnt(pEnt, SCANNER_CIT_INSPECT_LENGTH);
		m_nFlyMode = eFlyMode;
		SetCondition(COND_SCANNER_HAVE_INSPECT_TARGET);
	}
	else
	{
		if (pEnt == NULL)
		{
			Msg("InspectTarget: target %s not found!\n", inputdata.value.String());
		}
		else
		{
			Msg("InspectTarget: target %s has no origin!\n", inputdata.value.String());
		}
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();
	
	// --------------------------------------
	//  COND_SCANNER_INSPECT_DONE
	//
	// If my inspection over 
	// ---------------------------------------------------------
	ClearCondition ( COND_SCANNER_INSPECT_DONE );

	if (HaveInspectTarget() &&
		gpGlobals->curtime > m_fInspectEndTime)
	{
		SetCondition ( COND_SCANNER_INSPECT_DONE );

		m_fCheckCitizenTime	= gpGlobals->curtime + SCANNER_CIT_INSPECT_DELAY;
		m_fCheckHintTime	= gpGlobals->curtime + SCANNER_HINT_INSPECT_DELAY;
		ClearInspectTarget();
	}


	// ----------------------------------------------------------
	//  If I heard a sound and I don't have an enemy, inspect it
	// ----------------------------------------------------------
	if (GetEnemy() == NULL &&
		(HasCondition(COND_HEAR_COMBAT) ||
		HasCondition(COND_HEAR_DANGER)) )
	{
		CSound *pSound = GetBestSound();
		if (pSound)
		{
			Vector vSoundPos = pSound->GetSoundOrigin();
			SetInspectTargetToPos(vSoundPos,SCANNER_SOUND_INSPECT_LENGTH);

			m_nFlyMode = (random->RandomInt(0,1)==0) ? SCANNER_FLY_SPOT : SCANNER_FLY_PHOTO;
		}
	}

	// --------------------------------------
	//  COND_SCANNER_HAVE_INSPECT_TARGET
	//
	// Look for a nearby citizen or player to hassle. 
	// ---------------------------------------------------------
	ClearCondition ( COND_SCANNER_HAVE_INSPECT_TARGET );

	// Check for citizens to inspect
	if (GetEnemy()			==	NULL					&&
		gpGlobals->curtime		>	m_fCheckCitizenTime		&&
		!HaveInspectTarget()							)
	{
		CBaseEntity *pBestEntity = BestInspectTarget();
		if (pBestEntity)
		{
			// Check that this guy will accept....
			CAI_BaseNPC* pNPC = pBestEntity->MyNPCPointer();
			if (pNPC && pNPC->HandleInteraction(g_interactionScannerInspect,NULL,this))
			{
				SetInspectTargetToEnt(pBestEntity,SCANNER_CIT_INSPECT_LENGTH);
				m_nFlyMode = (random->RandomInt(0,1)==0) ? SCANNER_FLY_SPOT : SCANNER_FLY_PHOTO;
				SetCondition ( COND_SCANNER_HAVE_INSPECT_TARGET );
			}
		}
	}

	// Check for hints to inspect
	if (GetEnemy()			==	NULL				&&
		gpGlobals->curtime		>	m_fCheckHintTime	&&
		!HaveInspectTarget()						)
	{
		m_pHintNode = CAI_Hint::FindHint( this, HINT_NONE, 0, SCANNER_CIT_INSPECT_FLY_DIST );

		if (m_pHintNode)
		{
			m_fCheckHintTime	= gpGlobals->curtime + SCANNER_HINT_INSPECT_DELAY;

			m_nFlyMode = (random->RandomInt(0,2)==0) ? SCANNER_FLY_SPOT : SCANNER_FLY_PHOTO;

			SetInspectTargetToHint(m_pHintNode,SCANNER_HINT_INSPECT_LENGTH);

			SetCondition ( COND_SCANNER_HAVE_INSPECT_TARGET );
		}
	}

	// --------------------------------------
	//  COND_SCANNER_SPOT_ON_TARGET
	//
	//  True when spotlight is on target ent
	// --------------------------------------
	ClearCondition( COND_SCANNER_SPOT_ON_TARGET );

	if ( GetEnemy()			==	NULL				&&
		 m_pSpotlightTarget != NULL					&&
		 HaveInspectTarget()						&& 
		 m_pSpotlightTarget->GetAbsVelocity().Length() < 25 )
	{
		// If I have a target entity, check my spotlight against the
		// actual position of the entity
		if (GetTarget())
		{
			float fInspectDist = (m_vSpotlightTargetPos - m_vSpotlightCurrentPos).Length();
			if (fInspectDist < 100  )
			{
				SetCondition( COND_SCANNER_SPOT_ON_TARGET );
			}
		}
		// Otherwise just check by beam direction
		else
		{
			Vector vTargetDir		= SpotlightTargetPos() - GetLocalOrigin();
			VectorNormalize(vTargetDir);
			float dotpr = DotProduct(vTargetDir, m_vSpotlightDir);
			if (dotpr > 0.95)
			{
				SetCondition( COND_SCANNER_SPOT_ON_TARGET );
			}
		}
	}

	// --------------------------------------------
	//  COND_SCANNER_CAN_PHOTOGRAPH
	//
	//  True when can photograph target ent
	// --------------------------------------------
	ClearCondition(COND_SCANNER_CAN_PHOTOGRAPH);

	if (m_nFlyMode ==  SCANNER_FLY_PHOTO)
	{
		// Make sure I have something to photograph and I'm ready to photograph
		// and I'm not moving to fast
		if (gpGlobals->curtime			>	m_fNextPhotographTime	&&
			HaveInspectTarget()									&&
			GetCurrentVelocity().Length()	<	100					)
		{
			// Check that I'm in the right distance range
			float  fInspectDist = (InspectTargetPosition() - GetAbsOrigin()).Length2D();
			if (fInspectDist > SCANNER_PHOTO_NEAR_DIST &&
				fInspectDist < SCANNER_PHOTO_FAR_DIST )
			{
				trace_t tr;
				AI_TraceLine ( GetAbsOrigin(), InspectTargetPosition(), MASK_OPAQUE, 
					GetTarget(), COLLISION_GROUP_NONE, &tr);
				if (tr.fraction == 1.0)
				{
					SetCondition(COND_SCANNER_CAN_PHOTOGRAPH);
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_CScanner::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (GetEnemy() == NULL)
	{
		return COND_NONE;
	}

	// Check too far to attack with 2D distance
	float vEnemyDist2D = (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()).Length2D();

	if (m_flNextAttack > gpGlobals->curtime)
	{
		return COND_NONE;
	}
	else if (vEnemyDist2D > SCANNER_ATTACK_RANGE)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}
	return COND_CAN_MELEE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose: Overridden because if the player is a criminal, we hate them.
// Input  : pTarget - Entity with which to determine relationship.
// Output : Returns relationship value.
//-----------------------------------------------------------------------------
Disposition_t CNPC_CScanner::IRelationType(CBaseEntity *pTarget)
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
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_SCANNER_PHOTOGRAPH:
		{
			if (gpGlobals->curtime > m_flWaitFinished)
			{	
				// If light was on turn it off
				if (m_pEyeFlash->m_clrRender->a > 0)
				{
					m_pEyeFlash->SetBrightness( 0 );

					// I'm done with this target
					if (gpGlobals->curtime > m_fInspectEndTime)
					{
						ClearInspectTarget();
						TaskComplete();
					}
					// Otherwise take another picture
					else
					{
						m_flWaitFinished = gpGlobals->curtime + random->RandomFloat(1.5,2.5);
					}
				}
				// If light was off, take another picture
				else
				{
					TakePhoto();
					m_flWaitFinished = gpGlobals->curtime + 0.1;
				}
			}
			break;
		}
		case TASK_SCANNER_ATTACK_PRE_FLASH:
		{
			AttackPreFlash();
			if (gpGlobals->curtime > m_flWaitFinished)
			{
				TaskComplete();
			}
			break;
		}
		case TASK_SCANNER_ATTACK_FLASH:
		{
			if (gpGlobals->curtime > m_flWaitFinished)
			{
				AttackFlashBlind();
				TaskComplete();
			}
			break;
		}
		case TASK_SCANNER_ATTACK_PRE_GAS:
		{
			if (gpGlobals->curtime > m_flWaitFinished)
			{
				m_pEyeFlash->SetBrightness( 200 );
				TaskComplete();
			}
			break;
		}
		case TASK_SCANNER_ATTACK_GAS:
		{
			if (gpGlobals->curtime > m_flWaitFinished)
			{
				m_pEyeFlash->SetBrightness( 0 );
				AttackGasDamage();
				TaskComplete();
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
// Purpose: Gets the appropriate next schedule based on current condition
//			bits.
//-----------------------------------------------------------------------------
int CNPC_CScanner::SelectSchedule(void)
{
	//
	// Turn our flash off in case we were interrupted while it was on.
	//
	if (m_pEyeFlash)
	{
		m_pEyeFlash->SetBrightness( 0 );
	}

	// ----------------------------------------------------
	//  If I'm dead, go into a dive bomb
	// ----------------------------------------------------
	if (m_iHealth < 0)
	{
		return SCHED_SCANNER_ATTACK_DIVEBOMB;
	}

	// -------------------------------
	// If I'm in a script sequence
	// -------------------------------
	if (m_NPCState == NPC_STATE_SCRIPT)
	{
		return(BaseClass::SelectSchedule());
	}

	//
	// Flinch.
	//
	if ( HasCondition(COND_LIGHT_DAMAGE) || 
		 HasCondition(COND_HEAVY_DAMAGE) )
	{
		if ( m_NPCState == NPC_STATE_IDLE )
		{
			return SCHED_ALERT_SMALL_FLINCH;
		}
		else if ( m_NPCState == NPC_STATE_ALERT )
		{
			if ( m_iHealth < ( 3 * sk_scanner_health.GetFloat() / 4 ))
			{
				return SCHED_TAKE_COVER_FROM_ORIGIN;
			}
			else if ( SelectWeightedSequence( ACT_SMALL_FLINCH ) != -1 )
			{
				return SCHED_ALERT_SMALL_FLINCH;
			}
		}
		else
		{
			if (random->RandomInt( 0, 10 ) < 4 )
			{
				return SCHED_SMALL_FLINCH;
			}
		}
	}

	// ----------------------------------------------------------
	//  If I have an enemy
	// ----------------------------------------------------------
	if (GetEnemy() != NULL	&& 
		GetEnemy()->IsAlive()	)
	{
		if (HasCondition( COND_LOST_ENEMY))
		{
			return SCHED_SCANNER_PATROL;
		}
		else if (HasCondition( COND_SCANNER_FLY_BLOCKED ))
		{  
			return SCHED_SCANNER_CHASE_ENEMY;
		}
		else if (gpGlobals->curtime < m_flNextAttack)
		{
			return SCHED_SCANNER_SPOTLIGHT_HOVER;
		}
		else
		{
			if (HasCondition( COND_CAN_MELEE_ATTACK1 ))
			{ 
				if (random->RandomInt(0,1))
				{
					return SCHED_SCANNER_ATTACK_FLASH;
				}
				else
				{
					return SCHED_SCANNER_ATTACK_GAS;
				}
			}
			else
			{
				// -------------------------------------------------
				// If I'm far from the enemy, stay up high and 
				// approach in spotlight mode
				// -------------------------------------------------
				float  fAttack2DDist = (GetEnemyLKP() - GetLocalOrigin()).Length2D();
				if (fAttack2DDist > SCANNER_ATTACK_FAR_DIST)
				{
					return SCHED_SCANNER_SPOTLIGHT_HOVER;
				}
				// ----------------------------------------
				//  Otherwise fly in low for attack
				// ----------------------------------------
				else
				{
					return SCHED_SCANNER_ATTACK_HOVER;
				}
			}
		}
	}

	// ----------------------------------------------------------
	//  If I have something to inspect
	// ----------------------------------------------------------
	if (HaveInspectTarget())
	{
		if (HasCondition( COND_SCANNER_FLY_BLOCKED ))
		{  
			return SCHED_SCANNER_CHASE_TARGET;
		}

		// If I was chasing, pick with photographing or spotlighting 
		if (m_nFlyMode == SCANNER_FLY_CHASE)
		{
			m_nFlyMode = (random->RandomInt(0,1)==0) ? SCANNER_FLY_SPOT : SCANNER_FLY_PHOTO;
		}

		if (m_nFlyMode == SCANNER_FLY_SPOT)
		{
			if (HasCondition( COND_SCANNER_SPOT_ON_TARGET ))
			{
				if (GetTarget())
				{
					RequestInspectSupport();

					CAI_BaseNPC *pNPC = GetTarget()->MyNPCPointer();
					// If I'm leading the inspection, so verbal inspection
					if (pNPC && pNPC->GetTarget() == this)
					{
						return SCHED_SCANNER_SPOTLIGHT_INSPECT_CIT;
					}
					// Otherwise just spotlight
					else
					{
						return SCHED_SCANNER_SPOTLIGHT_HOVER;
					}
				}
				else
				{
					return SCHED_SCANNER_SPOTLIGHT_INSPECT_POS;
				}
			}
			else
			{
				return SCHED_SCANNER_SPOTLIGHT_HOVER;
			}	
		}
		else if (m_nFlyMode == SCANNER_FLY_PHOTO)
		{
			if ( HasCondition( COND_SCANNER_CAN_PHOTOGRAPH ))
			{ 
				return SCHED_SCANNER_PHOTOGRAPH;
			}
			else
			{			
				return SCHED_SCANNER_PHOTOGRAPH_HOVER;
			}
		}
		else if (m_nFlyMode == SCANNER_FLY_PATROL)
		{
			return SCHED_SCANNER_PATROL;
		}
	}
	else
	{
		return SCHED_SCANNER_PATROL;
	}
	return(BaseClass::SelectSchedule());
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::SpotlightDestroy(void)
{
	if (m_pSpotlight)
	{
		UTIL_RemoveImmediate(m_pSpotlight);
		m_pSpotlight = NULL;
		
		UTIL_RemoveImmediate(m_pSpotlightTarget);
		m_pSpotlightTarget = NULL;
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::SpotlightCreate(void)
{
	// -------------------------------------------------
	//  Make sure we don't already have one
	// -------------------------------------------------
	if (m_pSpotlight)
	{
		return;
	}

	// -------------------------------------------------
	//  Can we create a spotlight yet?
	// -------------------------------------------------
	if (gpGlobals->curtime < m_fNextSpotlightTime)
	{
		return;
	}

	// If I have an enemy, start spotlight on my enemy
	if (GetEnemy() != NULL)
	{
		Vector vEnemyPos	= GetEnemyLKP();
		Vector vTargetPos	= vEnemyPos;
		vTargetPos.z		= GetFloorZ(vEnemyPos);
		m_vSpotlightDir = vTargetPos - GetLocalOrigin();
		VectorNormalize(m_vSpotlightDir);
	}
	// If I have an target, start spotlight on my target
	else if (GetTarget() != NULL)
	{
		Vector vTargetPos	= GetTarget()->GetLocalOrigin();
		vTargetPos.z		= GetFloorZ(GetTarget()->GetLocalOrigin());
		m_vSpotlightDir = vTargetPos - GetLocalOrigin();
		VectorNormalize(m_vSpotlightDir);
	}
	// Other wise just start looking down
	else
	{
		m_vSpotlightDir	= Vector(0,0,-1); 
	}

	trace_t tr;
	AI_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + m_vSpotlightDir * 2024, 
		MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);

	m_pSpotlightTarget				= (CSpotlightEnd*)CreateEntityByName( "spotlight_end" );
	m_pSpotlightTarget->Spawn();
	m_pSpotlightTarget->SetLocalOrigin( tr.endpos );
	m_pSpotlightTarget->SetOwnerEntity( this );

	// Using the same color as the beam...
	m_pSpotlightTarget->SetRenderColor( 255, 255, 255 );
	m_pSpotlightTarget->m_Radius = m_flSpotlightMaxLength;
	UTIL_Relink(m_pSpotlightTarget);

	m_pSpotlight = CBeam::BeamCreate( "sprites/spotlight.vmt", 2.0 );
	m_pSpotlight->SetColor( 255, 255, 255 ); 
	m_pSpotlight->SetHaloTexture(m_nHaloSprite);
	m_pSpotlight->SetHaloScale(40);
	m_pSpotlight->SetEndWidth(m_flSpotlightGoalWidth);
	m_pSpotlight->SetBeamFlags(FBEAM_SHADEOUT);
	m_pSpotlight->SetBrightness( 80 );
	m_pSpotlight->SetNoise( 0 );
	m_pSpotlight->EntsInit( this, m_pSpotlightTarget );

	// attach to light
	m_pSpotlight->SetStartAttachment( SCANNER_ATTACHMENT_LIGHT );

	m_vSpotlightAngVelocity = vec3_origin;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
Vector CNPC_CScanner::SpotlightTargetPos(void)
{
	// ----------------------------------------------
	//  If I have an enemy 
	// ----------------------------------------------
	if (GetEnemy() != NULL)
	{
		// If I can see my enemy aim for him
		if (HasCondition(COND_SEE_ENEMY))
		{
			// If its client aim for his eyes
			if (GetEnemy()->GetFlags() & FL_CLIENT)
			{
				m_vSpotlightTargetPos = GetEnemy()->EyePosition();
			}
			// Otherwise same for his feet
			else
			{
				m_vSpotlightTargetPos	= GetEnemy()->GetLocalOrigin();
				m_vSpotlightTargetPos.z	= GetFloorZ(GetEnemy()->GetLocalOrigin());
			}
		}
		// Otherwise aim for last known position if I can see LKP
		else
		{
			Vector vLKP				= GetEnemyLKP();
			m_vSpotlightTargetPos.x	= vLKP.x;
			m_vSpotlightTargetPos.y	= vLKP.y;
			m_vSpotlightTargetPos.z	= GetFloorZ(vLKP);
		}
	}
	// ----------------------------------------------
	//  If I have an inspect target
	// ----------------------------------------------
	else if (HaveInspectTarget())
	{
		m_vSpotlightTargetPos = InspectTargetPosition();
	}
	else
	{
		// This creates a nice patrol spotlight sweep
		// in the direction that I'm travelling
		m_vSpotlightTargetPos	= GetCurrentVelocity();
		m_vSpotlightTargetPos.z = 0;
		VectorNormalize( m_vSpotlightTargetPos );
		m_vSpotlightTargetPos   *= 5;

		float noiseScale = 2.5;
		const Vector &noiseMod = GetNoiseMod();
		m_vSpotlightTargetPos.x += noiseScale*sin(noiseMod.x * gpGlobals->curtime + noiseMod.x);
		m_vSpotlightTargetPos.y += noiseScale*cos(noiseMod.y* gpGlobals->curtime + noiseMod.y);
		m_vSpotlightTargetPos.z -= fabs(noiseScale*cos(noiseMod.z* gpGlobals->curtime + noiseMod.z) );
		m_vSpotlightTargetPos   = GetLocalOrigin()+m_vSpotlightTargetPos * 2024;
	}

	return m_vSpotlightTargetPos;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
Vector CNPC_CScanner::SpotlightCurrentPos(void)
{
	Vector vTargetDir		= SpotlightTargetPos() - GetLocalOrigin();
	VectorNormalize(vTargetDir);

	if (!m_pSpotlight)
	{
		Msg("Spotlight pos. called w/o spotlight!\n");
		return vec3_origin;
	}
	// -------------------------------------------------
	//  Beam has momentum relative to it's ground speed
	//  so sclae the turn rate based on its distance
	//  from the beam source
	// -------------------------------------------------
	float	fBeamDist		= (m_pSpotlightTarget->GetLocalOrigin() - GetLocalOrigin()).Length();

	float	fBeamTurnRate	= atan(50/fBeamDist);
	Vector  vNewAngVelocity = fBeamTurnRate * (vTargetDir - m_vSpotlightDir);

	float	myDecay	 = 0.4;
	m_vSpotlightAngVelocity = (myDecay * m_vSpotlightAngVelocity + (1-myDecay) * vNewAngVelocity);

	// ------------------------------
	//  Limit overall angular speed
	// -----------------------------
	if (m_vSpotlightAngVelocity.Length() > 1)
	{

		Vector velDir = m_vSpotlightAngVelocity;
		VectorNormalize(velDir);
		m_vSpotlightAngVelocity = velDir * 1;
	}

	// ------------------------------
	//  Calculate new beam direction
	// ------------------------------
	m_vSpotlightDir = m_vSpotlightDir + m_vSpotlightAngVelocity;
	m_vSpotlightDir = m_vSpotlightDir;
	VectorNormalize(m_vSpotlightDir);


	// ---------------------------------------------
	//	Get beam end point.  Only collide with
	//  solid objects, not npcs
	// ---------------------------------------------
	trace_t tr;
	Vector vTraceEnd = GetAbsOrigin() + (m_vSpotlightDir * 2 * m_flSpotlightMaxLength);
	AI_TraceLine ( GetAbsOrigin(), vTraceEnd, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);

	return (tr.endpos);
}


//------------------------------------------------------------------------------
// Purpose : Update the direction and position of my spotlight
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::SpotlightUpdate(void)
{
	//FIXME: JDW - E3 Hack
	if ( m_bNoLight )
	{
		if ( m_pSpotlight )
		{
			SpotlightDestroy();
		}

		return;
	}

	if (m_nFlyMode != SCANNER_FLY_SPOT		&&
		m_nFlyMode != SCANNER_FLY_PATROL	)
	{
		if (m_pSpotlight)
		{	
			SpotlightDestroy();
		}
		return;
	}
	// ---------------------------------------------------
	//  If I don't have a spotlight attempt to create one
	// ---------------------------------------------------
	if (!m_pSpotlight)
	{
		SpotlightCreate();
	}
	if (!m_pSpotlight)
	{
		return;
	}

	// -------------------------------------------
	//  Calculate the new homing target position
	// --------------------------------------------
	m_vSpotlightCurrentPos = SpotlightCurrentPos();

	// ------------------------------------------------------------------
	//  If I'm not facing the spotlight turn it off 
	// ------------------------------------------------------------------
	Vector vSpotDir = m_vSpotlightCurrentPos - GetLocalOrigin();
	VectorNormalize(vSpotDir);
	Vector vFacingDir = BodyDirection2D( );
	float dotpr = DotProduct(vFacingDir,vSpotDir);
	if (dotpr < 0.0)
	{
		// Leave spotlight off for a while
		m_fNextSpotlightTime = gpGlobals->curtime + 1.0;

		SpotlightDestroy();
		return;
	}

	// --------------------------------------------------------------
	//  Update spotlight target velocity
	// --------------------------------------------------------------
	Vector vTargetDir  = (m_vSpotlightCurrentPos - m_pSpotlightTarget->GetLocalOrigin());
	float  vTargetDist = vTargetDir.Length();

	Vector vecNewVelocity = vTargetDir;
	VectorNormalize(vecNewVelocity);
	vecNewVelocity *= (10 * vTargetDist);

	// If a large move is requested, just jump to final spot as we
	// probably hit a discontinuity
	if (vecNewVelocity.Length() > 200)
	{
		VectorNormalize(vecNewVelocity);
		vecNewVelocity *= 200;
		m_pSpotlightTarget->SetLocalOrigin( m_vSpotlightCurrentPos );
	}
	m_pSpotlightTarget->SetAbsVelocity( vecNewVelocity );

	m_pSpotlightTarget->m_vSpotlightOrg = GetAbsOrigin();

	// Avoid sudden change in where beam fades out when cross disconinuities
	m_pSpotlightTarget->m_vSpotlightDir = m_pSpotlightTarget->GetLocalOrigin() - m_pSpotlightTarget->m_vSpotlightOrg;
	float flBeamLength	= VectorNormalize( m_pSpotlightTarget->m_vSpotlightDir.GetForModify() );
	m_flSpotlightCurLength = (0.80*m_flSpotlightCurLength) + (0.2*flBeamLength);

	// Fade out spotlight end if past max length.  
	if (m_flSpotlightCurLength > 2*m_flSpotlightMaxLength)
	{
		m_pSpotlightTarget->SetRenderColorA( 0 );
		m_pSpotlight->SetFadeLength(m_flSpotlightMaxLength);
	}
	else if (m_flSpotlightCurLength > m_flSpotlightMaxLength)		
	{
		m_pSpotlightTarget->SetRenderColorA( (1-((m_flSpotlightCurLength-m_flSpotlightMaxLength)/m_flSpotlightMaxLength)) );
		m_pSpotlight->SetFadeLength(m_flSpotlightMaxLength);
	}
	else
	{
		m_pSpotlightTarget->SetRenderColorA( 1.0 );
		m_pSpotlight->SetFadeLength(m_flSpotlightCurLength);
	}

	// Adjust end width to keep beam width constant
	float flNewWidth		= m_flSpotlightGoalWidth*(flBeamLength/m_flSpotlightMaxLength);
	m_pSpotlight->SetEndWidth(flNewWidth);

	// Adjust width of light on the end.  
	if ( FBitSet (m_spawnflags, SF_CSCANNER_NO_DYNAMIC_LIGHT) )
	{
		m_pSpotlightTarget->m_flLightScale = 0.0;
	}
	else
	{
		m_pSpotlightTarget->m_flLightScale = flNewWidth;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::Spawn(void)
{
	// Check for user error
	if (m_flSpotlightMaxLength <= 0)
	{
		Msg("ERROR: Invalid spotlight length <= 0, setting to 500\n");
		m_flSpotlightMaxLength = 500;
	}
	if (m_flSpotlightGoalWidth <= 0)
	{
		Msg("ERROR: Invalid spotlight width <= 0, setting to 100\n");
		m_flSpotlightGoalWidth = 100;
	}

	Precache();

	SetModel( "models/combot.mdl");

	SetHullType(HULL_SMALL_CENTERED); 
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );			
	m_bloodColor		= DONT_BLEED;
	m_iHealth			= sk_scanner_health.GetFloat();
	SetViewOffset( Vector(0, 0, 10) );		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView		= 0.2;
	m_NPCState			= NPC_STATE_NONE;
	SetNavType( NAV_FLY );
	AddFlag( FL_FLY );


	// ------------------------------------
	//	Init all class vars 
	// ------------------------------------
	m_vInspectPos			= vec3_origin;
	m_fInspectEndTime		= 0;
	m_fCheckCitizenTime		= gpGlobals->curtime + SCANNER_CIT_INSPECT_DELAY;
	m_fCheckHintTime		= gpGlobals->curtime + SCANNER_HINT_INSPECT_DELAY;
	m_fNextPhotographTime	= 0;

	m_pEyeFlash = CSprite::SpriteCreate( "sprites/blueflare1.vmt", GetLocalOrigin(), FALSE );
	m_pEyeFlash->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
	m_pEyeFlash->SetAttachment( this, SCANNER_ATTACHMENT_LIGHT + 1 );
	m_pEyeFlash->SetBrightness( 0 );
	m_pEyeFlash->SetScale( 1.4 );

	m_vSpotlightTargetPos	= vec3_origin;
	m_vSpotlightCurrentPos	= vec3_origin;

	m_pSpotlight			= NULL;
	m_pSpotlightTarget		= NULL;

	AngleVectors( GetLocalAngles(), &m_vSpotlightDir );
	m_vSpotlightAngVelocity = vec3_origin;

	m_fNextSpotlightTime	= 0;
	//m_nHaloSprite			// Set in precache
	m_fNextFlySoundTime		= 0;
	m_nFlyMode				= SCANNER_FLY_PATROL;
	SetCurrentVelocity( vec3_origin );
	m_vCurrentBanking		= m_vSpotlightDir;
	SetNoiseMod( random->RandomFloat( -3, 3 ), random->RandomFloat( -3, 3 ), random->RandomFloat( -3, 3 ) );
	m_fHeadYaw				= 0;
	m_pSmokeTrail			= NULL;
	m_vGasDirection			= m_vSpotlightDir;
	m_flSpotlightCurLength	= m_flSpotlightMaxLength;

	// set flight speed
	m_flSpeed = SCANNER_MAX_SPEED;
	// --------------------------------------------


	CapabilitiesAdd( bits_CAP_MOVE_FLY | bits_CAP_SQUAD);
	CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1);

	NPCInit();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::TakePhoto(void)
{
	EmitSound( "NPC_CScanner.TakePhoto" );
	m_pEyeFlash->SetScale( 1.4 );
	m_pEyeFlash->SetBrightness( 255 );
	m_pEyeFlash->SetColor(255,255,255);

	Vector vRawPos		= InspectTargetPosition();
	Vector vLightPos	= vRawPos;

	// If taking picture of entity, aim at feet
	if (GetTarget())
	{
		vLightPos.z			= GetFloorZ(vRawPos);
	}

	Vector pos = vLightPos+Vector(0,0,30);
	CBroadcastRecipientFilter filter2;
	te->DynamicLight( filter2, 0.0, &pos, 200, 200, 255, 0, 90, 0.2, 50 );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackPreFlash(void)
{
	EmitSound( "NPC_CScanner.TakePhoto" );

	// If off turn on, if on turn off
	if (m_pEyeFlash->m_clrRender->a == 0)
	{
		m_pEyeFlash->SetScale( 0.5 );
		m_pEyeFlash->SetBrightness( 255 );
		m_pEyeFlash->SetColor(255,0,0);
	}
	else
	{
		m_pEyeFlash->SetBrightness( 0 );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackPreGas(void)
{
	EmitSound( "NPC_CScanner.TakePhoto" );

	m_pEyeFlash->SetScale( 0.5 );
	m_pEyeFlash->SetBrightness( 255 );
	m_pEyeFlash->SetColor(234,191,17);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackFlash(void)
{
	EmitSound( "NPC_CScanner.AttackFlash" );
	m_pEyeFlash->SetScale( 1.8 );
	m_pEyeFlash->SetBrightness( 255 );
	m_pEyeFlash->SetColor(255,255,255);

	if (GetEnemy() != NULL)
	{
		Vector pos = GetEnemyLKP();
		CBroadcastRecipientFilter filter;
		te->DynamicLight( filter, 0.0, &pos, 200, 200, 255, 0, 300, 0.2, 50 );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackFlashBlind(void)
{
	// -------------------------------------------------------
	//  If close to my enemy (and a client) make screen	white
	// -------------------------------------------------------
	if (GetEnemy() != NULL					&&
		GetEnemy()->GetFlags() & FL_CLIENT	)
	{
		CBaseCombatCharacter *pBCC = (CBaseCombatCharacter*)(CBaseEntity*)(GetEnemy());
		Vector vEnemyPos = pBCC->GetLocalOrigin();
		vEnemyPos.z		= GetFloorZ(pBCC->GetLocalOrigin());

		float fFlashValue	= 0;
		float fFlashDist	= (vEnemyPos - GetLocalOrigin()).Length();
		if (fFlashDist < SCANNER_FLASH_MIN_DIST)
		{
			fFlashValue = SCANNER_FLASH_MAX_VALUE;
		}
		else if (fFlashDist < SCANNER_FLASH_MAX_DIST)
		{
			fFlashValue = SCANNER_FLASH_MAX_VALUE*(1-(fFlashDist/SCANNER_FLASH_MAX_DIST));
		}

		if (fFlashValue)
		{
			// Now scale the flash value by how closely the player is looking at me
			Vector vFlashDir = GetAbsOrigin() - pBCC->GetAbsOrigin();
			VectorNormalize(vFlashDir);
			Vector vBCCFacing = pBCC->BodyDirection2D( );
			float  dotPr	  = DotProduct(vFlashDir,vBCCFacing);
			if (dotPr > 0.5)
			{
				// Make sure nothing in the way
				trace_t tr;
				AI_TraceLine ( GetAbsOrigin(), pBCC->GetAbsOrigin(), MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);
				if (!tr.startsolid && tr.fraction == 1.0)
				{
					color32 white = {255,255,255,fFlashValue*dotPr};
					UTIL_ScreenFade( GetEnemy(), white, 0.5, 1.5, FFADE_MODULATE );
				}

			
			}
		}
	}
	m_pEyeFlash->SetBrightness( 0 );
	float fAttackDelay = random->RandomInt(SCANNER_ATTACK_MIN_DELAY,SCANNER_ATTACK_MAX_DELAY);
	m_flNextAttack			= gpGlobals->curtime + fAttackDelay;
	m_fNextSpotlightTime	= gpGlobals->curtime + 1.0;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackGas(void)
{
	if (GetEnemy())
	{
		EmitSound( "NPC_CScanner.AttackGas" );

		CSteamJet *pJet= (CSteamJet *)CreateEntityByName("env_steamjet");
		pJet->Spawn();
		pJet->SetLocalOrigin( GetLocalOrigin() );
		pJet->m_bEmit		= true;
		pJet->m_SpreadSpeed	= 30;
		pJet->m_Speed		= 400;
		pJet->m_StartSize	= 5;
		pJet->m_EndSize		= 100;
		pJet->m_JetLength	= 700;
		//pJet->m_Color		= Vector(110/255.0,82/255.0,9/255.0); // Mustard gas orange
		pJet->SetRenderColor( 110, 82, 9, 255 );
		//NOTENOTE: Set pJet->m_clrRender->a here for translucency

		pJet->SetParent(this);
		pJet->SetLifetime(1.0);

		Vector	vEnemyPos	= GetEnemy()->EyePosition() + GetEnemy()->GetAbsVelocity();
		m_vGasDirection		= GetLocalOrigin() - vEnemyPos;

		QAngle angles;
		VectorAngles(m_vGasDirection, angles );
		angles.y += 90;

		pJet->SetLocalAngles( angles );

		float fAttackDelay		= random->RandomInt(SCANNER_ATTACK_MIN_DELAY,SCANNER_ATTACK_MAX_DELAY);
		m_flNextAttack			= gpGlobals->curtime + fAttackDelay;
		m_fNextSpotlightTime	= gpGlobals->curtime + 1.0;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackGasDamage(void)
{
	// Note: Don't use CheckTraceHullAttack as other scanners get in the way
	if (GetEnemy() == NULL)
	{
		return;
	}

	float fEnemyDist = (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()).Length();
	if (fEnemyDist > SCANNER_GAS_DAMAGE_DIST)
	{
		return;
	}

	Vector	vEnemyDir	= GetLocalOrigin() - GetEnemy()->GetLocalOrigin();
	Vector gasDir = m_vGasDirection;
	VectorNormalize(vEnemyDir);
	VectorNormalize(gasDir);
	float	dotPr		= DotProduct(vEnemyDir,gasDir);
	if (dotPr > 0.90)
	{
		GetEnemy()->TakeDamage( CTakeDamageInfo( this, this, sk_scanner_dmg_gas.GetFloat(), DMG_NERVEGAS ) );
	}

	if (GetEnemy()->GetFlags() & FL_CLIENT)
	{
		GetEnemy()->ViewPunch(QAngle(random->RandomInt(-10,10), 0, random->RandomInt(-10,10)));
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackDivebomb(void)
{
	EmitSound( "NPC_CScanner.DiveBomb" );
	// -------------
	// Smoke trail.
	// -------------
	m_pSmokeTrail = SmokeTrail::CreateSmokeTrail();
	if(m_pSmokeTrail)
	{
		m_pSmokeTrail->m_SpawnRate = 10;
		m_pSmokeTrail->m_ParticleLifetime = 1;
		m_pSmokeTrail->m_StartColor.Init(0.5, 0.5, 0.5);
		m_pSmokeTrail->m_EndColor.Init(0,0,0);
		m_pSmokeTrail->m_StartSize		= 8;
		m_pSmokeTrail->m_EndSize		= 50;
		m_pSmokeTrail->m_SpawnRadius	= 10;
		m_pSmokeTrail->m_MinSpeed		= 15;
		m_pSmokeTrail->m_MaxSpeed		= 25;
		m_pSmokeTrail->SetLifetime(10.0f);
		m_pSmokeTrail->FollowEntity(ENTINDEX(pev));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
		case TASK_SCANNER_SPOT_INSPECT_ON:
		{
			if (GetTarget() == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				CAI_BaseNPC* pNPC = GetTarget()->MyNPCPointer();
				if (!pNPC)
				{
					TaskFail(FAIL_NO_TARGET);
				}
				else
				{
					pNPC->HandleInteraction(g_interactionScannerInspectBegin,NULL,this);
					
					// Now we need some time to inspect
					m_fInspectEndTime = gpGlobals->curtime + SCANNER_CIT_INSPECT_LENGTH;
					TaskComplete();
				}
			}
			break;
		}
		case TASK_SCANNER_SPOT_INSPECT_WAIT:
		{
			if (GetTarget() == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				CAI_BaseNPC* pNPC = GetTarget()->MyNPCPointer();
				if (!pNPC)
				{
					SetTarget( NULL );
					TaskFail(FAIL_NO_TARGET);
				}
				else
				{
					//<<TEMP>>//<<TEMP>> armband too!
					pNPC->HandleInteraction(g_interactionScannerInspectHandsUp,NULL,this);
				}
				TaskComplete();
			}
			break;
		}
		case TASK_SCANNER_SPOT_INSPECT_OFF:
		{
			if (GetTarget() == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				CAI_BaseNPC* pNPC = GetTarget()->MyNPCPointer();
				if (!pNPC)
				{
					TaskFail(FAIL_NO_TARGET);
				}
				else
				{
					pNPC->HandleInteraction(g_interactionScannerInspectDone,NULL,this);

					// Clear target entity and don't inspect again for a while
					SetTarget( NULL );
					m_fCheckCitizenTime = gpGlobals->curtime + SCANNER_CIT_INSPECT_DELAY;
					TaskComplete();
				}
			}
			break;
		}
		case TASK_SCANNER_CLEAR_INSPECT_TARGET:
		{
			ClearInspectTarget();
		}
		case TASK_SCANNER_SET_FLY_PHOTO:
		{
			m_nFlyMode = SCANNER_FLY_PHOTO;

			// Leave spotlight off for a while
			m_fNextSpotlightTime = gpGlobals->curtime + 2.0;

			TaskComplete();
			break;
		}
		case TASK_SCANNER_SET_FLY_PATROL:
		{
			// Fly in partol mode and clear any
			// remaining target entity
			m_nFlyMode		= SCANNER_FLY_PATROL;
			TaskComplete();
			break;
		}
		case TASK_SCANNER_SET_FLY_CHASE:
		{
			m_nFlyMode = SCANNER_FLY_CHASE;
			TaskComplete();
			break;
		}
		case TASK_SCANNER_SET_FLY_SPOT:
		{
			m_nFlyMode = SCANNER_FLY_SPOT;
			TaskComplete();
			break;
		}
		case TASK_SCANNER_SET_FLY_ATTACK:
		{
			m_nFlyMode = SCANNER_FLY_ATTACK;
			TaskComplete();
			break;
		}
		case TASK_SCANNER_SET_FLY_DIVE:
		{
			m_nFlyMode = SCANNER_FLY_DIVE;
			TaskComplete();
			break;
		}

		case TASK_SCANNER_PHOTOGRAPH:
		{
			TakePhoto();
			m_flWaitFinished = gpGlobals->curtime + 0.1;
			break;
		}

		case TASK_SCANNER_ATTACK_PRE_FLASH:
		{
			if (m_pEyeFlash)
			{
				AttackPreFlash();
				// Flash red for a while
				m_flWaitFinished = gpGlobals->curtime + 0.2;
			}
			else
			{
				TaskFail("No Flash");
			}
			break;
		}

		case TASK_SCANNER_ATTACK_FLASH:
		{
			AttackFlash();
			// Blinding occurs slightly later
			m_flWaitFinished = gpGlobals->curtime + 0.05;
			break;
		}

		case TASK_SCANNER_ATTACK_PRE_GAS:
		{
			if (m_pEyeFlash)
			{
				m_pEyeFlash->SetBrightness( 150 );
				AttackPreGas();
				m_flWaitFinished = gpGlobals->curtime + 0.2;
			}
			break;
		}

		case TASK_SCANNER_ATTACK_GAS:
		{
			AttackGas();
			// Damage occurs slightly later, so player can avoid it
			m_flWaitFinished = gpGlobals->curtime + 0.65;
			break;
		}
		// Override to go to inspect target position whether or not is an entity
		case TASK_GET_PATH_TO_TARGET:
		{
			if (!HaveInspectTarget())
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else if (m_pHintNode)
			{
				Vector vNodePos;
				m_pHintNode->GetPosition(this,&vNodePos);

				GetNavigator()->SetGoal( vNodePos );
			}
			else 
			{
				AI_NavGoal_t goal( InspectTargetPosition() );
				goal.pTarget = GetTarget();
				GetNavigator()->SetGoal( goal );
			}
			break;
		}
		default:
		{
			BaseClass::StartTask(pTask);
		}
	}
}


void CNPC_CScanner::OnScheduleChange( void )
{
	m_flSpeed = SCANNER_MAX_SPEED;

	BaseClass::OnScheduleChange();
}

//-----------------------------------------------------------------------------
// Purpose: Overridden so that scanners play battle sounds while fighting.
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
bool CNPC_CScanner::ShouldPlayIdleSound( void )
{
	if (( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT ) && 
		   random->RandomInt(0,99) == 0 && !(m_spawnflags & SF_NPC_GAG ))
	{
		return( TRUE );
	}

	return( FALSE );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CNPC_CScanner::MinGroundDist(void)
{
	if (m_nFlyMode	== SCANNER_FLY_SPOT && !m_pHintNode)
	{
		return SCANNER_SPOTLIGHT_FLY_HEIGHT;
	}
	else
	{
		return SCANNER_NOSPOTLIGHT_FLY_HEIGHT;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CScanner::SetBanking(float flInterval)
{
	// Make frame rate independent
	float	iRate	 = 0.5;
	float timeToUse = flInterval;
	while (timeToUse > 0)
	{
		Vector vFlyDirection	= GetCurrentVelocity();
		VectorNormalize(vFlyDirection);
		vFlyDirection *= SCANNER_BANK_RATE;

		Vector vBankDir;
		
		if (m_nFlyMode == SCANNER_FLY_PATROL && m_pSpotlight)
		{
			// Bank to face spotlight
			Vector vRawDir = m_vSpotlightCurrentPos - GetLocalOrigin();
			VectorNormalize(vRawDir);
			vRawDir *= 50;

			// Account for head rotation
			vBankDir.x = vRawDir.z * -cos(m_fHeadYaw*M_PI/180);
			vBankDir.z = vRawDir.z *  sin(m_fHeadYaw*M_PI/180);
		}
		else if ( GetEnemy()	!= NULL	)
		{
			// Bank to face enemy
			Vector vRawDir = GetEnemy()->EyePosition() - GetLocalOrigin();
			VectorNormalize(vRawDir);
			vRawDir *= 50;

			// Account for head rotation
			vBankDir.x = vRawDir.z * -cos(m_fHeadYaw*M_PI/180);
			vBankDir.z = vRawDir.z *  sin(m_fHeadYaw*M_PI/180);
		}
		else if (HaveInspectTarget())
		{
			// Bank to face inspect target
			Vector vRawDir = InspectTargetPosition() - GetLocalOrigin();
			VectorNormalize( vRawDir );
			vRawDir *= 50;

			// Account for head rotation
			vBankDir.x = vRawDir.z * -cos(m_fHeadYaw*M_PI/180);
			vBankDir.z = vRawDir.z *  sin(m_fHeadYaw*M_PI/180);
		}
		else
		{
			// Bank based on fly direction
			vBankDir	= vFlyDirection;
		}

		// If I'm diveboming add some banking noise
		if (m_nFlyMode == SCANNER_FLY_DIVE)
		{
			float fNoiseScale = 30.0;
			const Vector &noiseMod = GetNoiseMod();
			vBankDir.x += fNoiseScale*sin(noiseMod.x * gpGlobals->curtime + noiseMod.x);
			vBankDir.z += fNoiseScale*cos(noiseMod.y * gpGlobals->curtime + noiseMod.y);
		}

		m_vCurrentBanking.x		= (iRate * m_vCurrentBanking.x) + (1 - iRate)*(vBankDir.x);
		m_vCurrentBanking.z		= (iRate * m_vCurrentBanking.z) + (1 - iRate)*(vBankDir.z);
		timeToUse =- 0.1;
	}
	SetLocalAngles( QAngle( m_vCurrentBanking.x, 0, m_vCurrentBanking.z ) );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveExecute_Alive(float flInterval)
{
	// Amount of noise to add to flying
	float noiseScale = 4.0;

	// -------------------------------------------
	//  Avoid obstacles, unless I'm dive bombing
	// -------------------------------------------
	if (m_nFlyMode != SCANNER_FLY_DIVE)
	{
		SetCurrentVelocity( GetCurrentVelocity() + VelocityToAvoidObstacles(flInterval) );
	}
	// If I am dive bomving add more noise to my flying
	else
	{
		AttackDivebombCollide(flInterval);
		noiseScale *= 4;
	}

	// ----------------------------------------------------------------------------------------
	// Add time-coherent noise to the current velocity so that it never looks bolted in place.
	// ----------------------------------------------------------------------------------------

	AddNoiseToVelocity( noiseScale );

	// Limit fall speed
	LimitSpeed( 200 );

	SetBanking(flInterval);

	PlayFlySound();
	WalkMove( GetCurrentVelocity() * flInterval, MASK_NPCSOLID );
}

//-----------------------------------------------------------------------------
// Purpose: Handles movement towards the last move target.
// Input  : flInterval - 
//-----------------------------------------------------------------------------
bool CNPC_CScanner::OverridePathMove( float flInterval )
{
	CBaseEntity *pMoveTarget = (GetTarget()) ? GetTarget() : GetEnemy();
	Vector waypointDir = GetNavigator()->GetCurWaypointPos() - GetLocalOrigin();
	VectorNormalize(waypointDir);

	// -----------------------------------------------------------------
	// Check route is blocked
	// ------------------------------------------------------------------
	Vector checkPos = GetLocalOrigin() + (waypointDir * (m_flSpeed * flInterval));

	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( NAV_FLY, GetLocalOrigin(), checkPos, MASK_NPCSOLID|CONTENTS_WATER,
		pMoveTarget, &moveTrace);
	if (IsMoveBlocked( moveTrace ))
	{
		TaskFail(FAIL_NO_ROUTE);
		GetNavigator()->ClearGoal();
		return true;
	}

	// ----------------------------------------------
	
	Vector lastPatrolDir = GetNavigator()->GetCurWaypointPos() - GetLocalOrigin();
	
	if ( ProgressFlyPath( flInterval, pMoveTarget, MASK_NPCSOLID,
						  !IsCurSchedule( SCHED_SCANNER_PATROL ), 64 ) == AINPP_COMPLETE )
	{
		if (IsCurSchedule( SCHED_SCANNER_PATROL ))
		{
			m_vLastPatrolDir = lastPatrolDir;
			VectorNormalize(m_vLastPatrolDir);
		}
		return true;
	}
	return false;
}

bool CNPC_CScanner::OverrideMove(float flInterval)
{
	// ----------------------------------------------
	//	Select move target 
	// ----------------------------------------------
	CBaseEntity *pMoveTarget = NULL;
	if (GetTarget() != NULL )
	{
		pMoveTarget = GetTarget();
	}
	else if (GetEnemy() != NULL )
	{
		pMoveTarget		= GetEnemy();
	}

	// ----------------------------------------------
	//	Select move target position 
	// ----------------------------------------------
	Vector vMoveTargetPos(0,0,0);
	if (HaveInspectTarget())
	{
		vMoveTargetPos = InspectTargetPosition(); 
	}
	else if (GetEnemy() != NULL)
	{
		vMoveTargetPos = GetEnemy()->GetAbsOrigin();
	}

	// -----------------------------------------
	//  See if we can fly there directly
	// -----------------------------------------
	if (pMoveTarget || HaveInspectTarget())
	{
		trace_t tr;

		if (pMoveTarget)
		{
			AI_TraceHull(GetAbsOrigin(), vMoveTargetPos, GetHullMins(), 
				GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, pMoveTarget, COLLISION_GROUP_NONE, &tr);
		}
		else
		{
			AI_TraceHull(GetAbsOrigin(), vMoveTargetPos, GetHullMins(), 
				GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr);
		}

		float fTargetDist = (1-tr.fraction)*(GetAbsOrigin() - vMoveTargetPos).Length();
		if (fTargetDist > 50)
		{
			/* HANDY DEBUG TOOL
			NDebugOverlay::Line(GetLocalOrigin(), vMoveTargetPos, 255,0,255, true, 0);
			NDebugOverlay::Cross3D(tr.endpos,Vector(-15,-15,-15),Vector(5,5,5),0,255,255,true,0.1);
			*/

			SetCondition( COND_SCANNER_FLY_BLOCKED );
		}
		else
		{
			SetCondition( COND_SCANNER_FLY_CLEAR );
		}
	}


	// -----------------------------------------------------------------
	// If I have a route, keep it updated and move toward target
	// ------------------------------------------------------------------
	if (GetNavigator()->IsGoalActive())
	{
		if ( OverridePathMove( flInterval ) )
			return true;
	}
	
	// ----------------------------------------------
	//	If spotlighting
	// ----------------------------------------------
	else if (m_nFlyMode == SCANNER_FLY_SPOT)
	{
		MoveToSpotlight(flInterval);

		if (m_pSpotlight)
		{
			TurnHeadToTarget(flInterval, m_vSpotlightCurrentPos );
		}
		else if (HaveInspectTarget())
		{
			TurnHeadToTarget(flInterval, InspectTargetPosition() );
		}
		else if (GetEnemy() != NULL)
		{
			TurnHeadToTarget(flInterval, GetEnemy()->GetLocalOrigin() );
		}
	}
	// ----------------------------------------------
	//	If photographing
	// ----------------------------------------------
	else if (m_nFlyMode == SCANNER_FLY_PHOTO)
	{
		MoveToPhotograph(flInterval);

		// -------------------------------------
		// If my beam is on, face it
		// -------------------------------------
		if (HaveInspectTarget())
		{
			TurnHeadToTarget(flInterval, InspectTargetPosition() );
		}
	}
	// ----------------------------------------------
	//	If attacking
	// ----------------------------------------------
	else if (m_nFlyMode == SCANNER_FLY_ATTACK)
	{
		if (m_pSpotlight)
		{
			SpotlightDestroy();
		}
		MoveToAttack(flInterval);

		// -------------------------------------
		// Face my enemy
		// -------------------------------------
		if (GetEnemy() != NULL)
		{
			TurnHeadToTarget(flInterval, GetEnemy()->GetLocalOrigin() );
		}
	}
	// ----------------------------------------------
	//	If dive bombing
	// ----------------------------------------------
	else if (m_nFlyMode == SCANNER_FLY_DIVE)
	{
		if (m_pSpotlight)
		{
			SpotlightDestroy();
		}
		MoveToDivebomb(flInterval);

		// -------------------------------------
		// Face my enemy
		// -------------------------------------
		if (GetEnemy() != NULL)
		{
			TurnHeadToTarget(flInterval, GetEnemy()->GetLocalOrigin() );
		}
	}
	// -----------------------------------------------------------------
	// If I don't have a route, just decelerate
	// -----------------------------------------------------------------
	else if (!GetNavigator()->IsGoalActive())
	{
		float	myDecay	 = 9.5;
		Decelerate( flInterval, myDecay);

		// -------------------------------------
		// If I have an enemy turn to face him
		// -------------------------------------
		if (GetEnemy())
		{
			TurnHeadToTarget(flInterval, GetEnemy()->GetLocalOrigin() );
		}
	}
	MoveExecute_Alive(flInterval);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Clamps the desired steering vector based on the limitations of this NPC.
// Input  : vecSteerAbs - The vector indicating our ideal steering vector. Receives
//				the clamped steering vector in absolute (x,y,z) coordinates.
//			vecSteerRel - Receives the clamped steering vector in relative (forward, right, up)
//				coordinates.
//			forward - Our current forward vector.
//			right - Our current right vector.
//			up - Our current up vector.
//-----------------------------------------------------------------------------
void CNPC_CScanner::ClampSteer( Vector &vecSteerAbs, Vector &vecSteerRel, Vector &forward, Vector &right, Vector &up )
{
	float flForwardSteer = clamp( DotProduct( vecSteerAbs, forward ), m_vecAccelMin.x, m_vecAccelMax.x );
	float flRightSteer = clamp( DotProduct( vecSteerAbs, right ), m_vecAccelMin.y, m_vecAccelMax.y );
	float flUpSteer = clamp( DotProduct( vecSteerAbs, up ), m_vecAccelMin.z, m_vecAccelMax.z );

	Vector vecSteerOld = vecSteerAbs;

	vecSteerAbs = ( flForwardSteer * forward ) + ( flRightSteer * right ) + ( flUpSteer * up );

	vecSteerRel.x = flForwardSteer;
	vecSteerRel.y = flRightSteer;
	vecSteerRel.z = flUpSteer;
}


//-----------------------------------------------------------------------------
// Purpose: Gets a steering vector to move towards a target position.
// Input  : vecTarget - Target position to seek.
//			vecSteer - Receives the ideal steering vector.
//-----------------------------------------------------------------------------
void CNPC_CScanner::SteerSeek(Vector &vecSteer, const Vector &vecTarget)
{
	Vector vecOffset = vecTarget - GetLocalOrigin();

	// Figure out how fast we want to go.
	float flDistance = VectorNormalize( vecOffset );
	float flCurSpeed = GetCurrentVelocity().Length();
	float flDesiredSpeed = clamp( m_flFlightSpeed * flDistance / m_flStoppingDistance, flCurSpeed, m_flFlightSpeed );

	Vector vecDesiredVelocity = flDesiredSpeed * vecOffset;
	vecSteer = vecDesiredVelocity - GetCurrentVelocity();
}


//-----------------------------------------------------------------------------
// Purpose: Accelerates toward a given position.
// Input  : flInterval - Time interval over which to move.
//			vecMoveTarget - Position to move toward.
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveToTarget( float flInterval, const Vector &vecMoveTarget )
{
	if (m_pSpotlight)
	{
		TurnHeadToTarget(flInterval, m_vSpotlightCurrentPos);
	}
	else
	{
		TurnHeadToTarget( flInterval, vecMoveTarget );
	}

	// HACK: seperate movement vars for E3 path following
	// TODO: set this up elsewhere?
	m_vecAccelMin = Vector( -300, -300, -400 );
	m_vecAccelMax = Vector( 300, 300, 400 );

	if (m_nFlyMode == SCANNER_FLY_PATROL  )
	{
		m_flFlightSpeed = 100;
		m_flStoppingDistance = 30;
	}
	else
	{
		m_flFlightSpeed = 300;
		m_flStoppingDistance = 100;
	}


	Vector vecSteerAbs;
	SteerSeek( vecSteerAbs, vecMoveTarget );

	Vector vecForward;
	Vector vecRight;
	Vector vecUp;

	AngleVectors( GetLocalAngles(), &vecForward, &vecRight, &vecUp );

	Vector vecSteerRel;
	ClampSteer( vecSteerAbs, vecSteerRel, vecForward, vecRight, vecUp );

	Vector newVel;
	VectorMA( GetCurrentVelocity(), flInterval, vecSteerAbs, newVel );
	SetCurrentVelocity( newVel );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveToSpotlight(float flInterval)
{
	Vector vTargetPos;
	Vector vFlyDirection = vec3_origin;
	if (HaveInspectTarget())
	{
		vTargetPos = InspectTargetPosition();
	}
	else if (GetEnemy() != NULL)
	{
		vTargetPos = GetEnemyLKP();
	}
	else
	{
		return;
	}

	// -----------------------------------------
	//  Keep within range of enemy
	// -----------------------------------------
	float	fTargetDist = (vTargetPos - GetLocalOrigin()).Length2D();
	Vector	vTargetDir  = (vTargetPos - GetLocalOrigin());
	vTargetDir.z = 0;
	VectorNormalize( vTargetDir );
	if (fTargetDist < SCANNER_SPOTLIGHT_NEAR_DIST)
	{
		vFlyDirection = -vTargetDir;
	}
	else if (fTargetDist > SCANNER_SPOTLIGHT_FAR_DIST)
	{
		vFlyDirection = vTargetDir;
	}

	// ------------------------------------------------
	//  Also keep my distance from other squad members
	//  unless I'm inspecting
	// ------------------------------------------------
	if (m_pSquad &&
		gpGlobals->curtime > m_fInspectEndTime)
	{
		CBaseEntity*	pNearest	= m_pSquad->NearestSquadMember(this);
		if (pNearest)
		{
			Vector			vNearestDir = (pNearest->GetLocalOrigin() - GetLocalOrigin());
			if (vNearestDir.Length() < SCANNER_SQUAD_FLY_DIST) 
			{
				vNearestDir		= pNearest->GetLocalOrigin() - GetLocalOrigin();
				VectorNormalize(vNearestDir);
				vFlyDirection  -= 0.5*vNearestDir;
			}
		}
	}

	// ---------------------------------------------------------
	//  Add evasion if I have taken damage recently
	// ---------------------------------------------------------
	if ((m_flLastDamageTime + SCANNER_EVADE_TIME) > gpGlobals->curtime)
	{
		vFlyDirection = vFlyDirection + VelocityToEvade(GetEnemyCombatCharacterPointer());
	}

	// -------------------------------------
	// Set net velocity, accelerate faster
	// if I have an enemy
	// -------------------------------------
	float	myAccel;
	if (GetEnemy() != NULL)
	{
		myAccel	 = 650.0;
	}
	else
	{
		myAccel	 = 300.0;
	}
	float	myDecay	 = 0.35; // decay to 35% in 1 second
	MoveInDirection( flInterval, vFlyDirection, myAccel, 2 * myAccel, myDecay);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveToAttack(float flInterval)
{
	if (GetEnemy() == NULL)
	{
		return;
	}
	Vector vEnemyPos	 = GetEnemyLKP();
	Vector vFlyDirection = vec3_origin;

	// -----------------------------------------
	//  Keep within range of enemy
	// -----------------------------------------
	float  fAttack2DDist = (vEnemyPos - GetLocalOrigin()).Length2D();
	float  fAttackVtDist = fabs(vEnemyPos.z - GetLocalOrigin().z);
	Vector vAttackDir  = vEnemyPos - GetLocalOrigin();
	VectorNormalize(vAttackDir);

	if (fAttack2DDist < SCANNER_ATTACK_NEAR_DIST)
	{
		vFlyDirection.x = -vAttackDir.x;
		vFlyDirection.y = -vAttackDir.y;
		vFlyDirection.z = -vAttackDir.z;
	}
	else if (fAttack2DDist > SCANNER_ATTACK_FAR_DIST)
	{
		vFlyDirection.x = vAttackDir.x;
		vFlyDirection.y = vAttackDir.y;
	}

	if (fAttackVtDist > 100)
	{
		vFlyDirection.z = vAttackDir.z;
	}

	// ---------------------------------------------------------
	//  Add evasion if I have taken damage recently
	// ---------------------------------------------------------
	if ((m_flLastDamageTime + SCANNER_EVADE_TIME) > gpGlobals->curtime)
	{
		vFlyDirection = vFlyDirection + VelocityToEvade(GetEnemyCombatCharacterPointer());
	}

	// -------------------------------------
	// Set net velocity 
	// -------------------------------------
	float	myAccel = 500; 
	float	myDecay	 = 0.35; // decay to 35% in 1 second
	MoveInDirection( flInterval, vFlyDirection, myAccel, 2 * myAccel, myDecay);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveToDivebomb(float flInterval)
{
	float	myAccel = 800; 
	float	myDecay	 = 0.35; // decay to 35% in 1 second

	// -----------------------------------------
	//  Fly towards my enemy
	// -----------------------------------------
	if (GetEnemy() != NULL)
	{
		Vector vEnemyPos	 = GetEnemyLKP();
		Vector vFlyDirection  = vEnemyPos - GetLocalOrigin();
		VectorNormalize(vFlyDirection);

		// Add some downward force so I crash
		vFlyDirection.z = -0.3;

		// -------------------------------------
		// Set net velocity 
		// -------------------------------------
		MoveInDirection( flInterval, vFlyDirection, myAccel, 2 * myAccel, myDecay);
	}
	else
	{
		// Slowly crash into ground
		Vector newVel = GetCurrentVelocity();
		newVel.z = flInterval*(myDecay * newVel.z + (-0.3 * myAccel));
		SetCurrentVelocity( newVel );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveToPhotograph(float flInterval)
{
	if (!HaveInspectTarget())
	{
		return;
	}
	Vector vTargetPos = InspectTargetPosition();
	Vector vFlyDirection = vec3_origin;

	// -----------------------------------------
	//  Keep within range of enemy
	// -----------------------------------------
	float  fInspect2DDist = (vTargetPos - GetLocalOrigin()).Length2D();
	float  fInspectVtDist = fabs(vTargetPos.z - GetLocalOrigin().z);
	Vector vInspectDir  = vTargetPos - GetLocalOrigin();
	VectorNormalize(vInspectDir);

	if (fInspect2DDist < SCANNER_PHOTO_NEAR_DIST)
	{
		vFlyDirection.x = -vInspectDir.x;
		vFlyDirection.y = -vInspectDir.y;
		vFlyDirection.z = -vInspectDir.z;
	}
	else if (fInspect2DDist > SCANNER_PHOTO_FAR_DIST)
	{
		vFlyDirection.x = vInspectDir.x;
		vFlyDirection.y = vInspectDir.y;
	}

	if (fInspectVtDist > 100)
	{
		vFlyDirection.z = vInspectDir.z;
	}

	// ------------------------------------------------
	//  Also keep my distance from other squad members
	//  unless I'm inspecting
	// ------------------------------------------------
	if (m_pSquad &&
		gpGlobals->curtime > m_fInspectEndTime)
	{
		CBaseEntity*	pNearest	= m_pSquad->NearestSquadMember(this);
		if (pNearest)
		{
			Vector			vNearestDir = (pNearest->GetLocalOrigin() - GetLocalOrigin());
			if (vNearestDir.Length() < SCANNER_SQUAD_FLY_DIST) 
			{
				vNearestDir		= pNearest->GetLocalOrigin() - GetLocalOrigin();
				VectorNormalize(vNearestDir);
				vFlyDirection  -= 0.5*vNearestDir;
			}
		}
	}


	// -------------------------------------
	// Set net velocity 
	// -------------------------------------
	float	myAccel = 300; 
	float	myDecay	 = 0.35; // decay to 35% in 1 second
	MoveInDirection( flInterval, vFlyDirection, myAccel, 2 * myAccel, myDecay);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector CNPC_CScanner::VelocityToEvade(CBaseCombatCharacter *pEnemy)
{
	if (pEnemy)
	{
		// -----------------------------------------
		//  Keep out of enemy's shooting position
		// -----------------------------------------
		Vector vEnemyFacing = pEnemy->BodyDirection2D( );
		Vector	vEnemyDir   = pEnemy->EyePosition() - GetLocalOrigin();
		VectorNormalize(vEnemyDir);
		float  fDotPr		= DotProduct(vEnemyFacing,vEnemyDir);

		if (fDotPr < -0.9)
		{
			Vector vDirUp(0,0,1);
			Vector vDir;
			CrossProduct( vEnemyFacing, vDirUp, vDir);

			Vector crossProduct;
			CrossProduct(vEnemyFacing, vEnemyDir, crossProduct);
			if (crossProduct.y < 0)
			{
				vDir = vDir * -1;
			}
			return (vDir);
		}
		else if (fDotPr < -0.85)
		{
			Vector vDirUp(0,0,1);
			Vector vDir;
			CrossProduct( vEnemyFacing, vDirUp, vDir);

			Vector crossProduct;
			CrossProduct(vEnemyFacing, vEnemyDir, crossProduct);
			if (random->RandomInt(0,1))
			{
				vDir = vDir * -1;
			}
			return (vDir);
		}
	}
	return vec3_origin;
}


//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_CScanner::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* pSourceEnt)
{
	//	TODO:: - doing this by just an interrupt contition would be a lot better!
	if (interactionType ==	g_interactionScannerSupportEntity)
	{
		// Only accept help request if I'm not already busy
		if (GetEnemy() == NULL && !HaveInspectTarget())
		{
			// Only accept if target is a reasonable distance away
			CBaseEntity* pTarget = (CBaseEntity*)data;
			float fTargetDist = (pTarget->GetLocalOrigin() - GetLocalOrigin()).Length();

			if (fTargetDist < SCANNER_SQUAD_HELP_DIST)
			{
				float fInspectTime = (((CNPC_CScanner*)pSourceEnt)->m_fInspectEndTime - gpGlobals->curtime);
				SetInspectTargetToEnt(pTarget,fInspectTime);

				if (random->RandomInt(0,2)==0)
				{
					SetSchedule(SCHED_SCANNER_PHOTOGRAPH_HOVER);
				}
				else
				{
					SetSchedule(SCHED_SCANNER_SPOTLIGHT_HOVER);
				}
				return true;
			}
		}
	}
	else if (interactionType ==	g_interactionScannerSupportPosition)
	{
		// Only accept help request if I'm not already busy
		if (GetEnemy() == NULL && !HaveInspectTarget())
		{
			// Only accept if target is a reasonable distance away
			Vector vInspectPos;
			vInspectPos.x = ((Vector *)data)->x;
			vInspectPos.y = ((Vector *)data)->y;
			vInspectPos.z = ((Vector *)data)->z;

			float fTargetDist = (vInspectPos - GetLocalOrigin()).Length();

			if (fTargetDist < SCANNER_SQUAD_HELP_DIST)
			{
				float fInspectTime = (((CNPC_CScanner*)pSourceEnt)->m_fInspectEndTime - gpGlobals->curtime);
				SetInspectTargetToPos(vInspectPos,fInspectTime);

				if (random->RandomInt(0,2)==0)
				{
					SetSchedule(SCHED_SCANNER_PHOTOGRAPH_HOVER);
				}
				else
				{
					SetSchedule(SCHED_SCANNER_SPOTLIGHT_HOVER);
				}
				return true;
			}
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputDisableSpotlight( inputdata_t &inputdata )
{
	m_bNoLight = true;
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_cscanner, CNPC_CScanner )

	DECLARE_TASK(TASK_SCANNER_SET_FLY_PHOTO)
	DECLARE_TASK(TASK_SCANNER_SET_FLY_PATROL)
	DECLARE_TASK(TASK_SCANNER_SET_FLY_CHASE)
	DECLARE_TASK(TASK_SCANNER_SET_FLY_SPOT)
	DECLARE_TASK(TASK_SCANNER_SET_FLY_ATTACK)
	DECLARE_TASK(TASK_SCANNER_SET_FLY_DIVE)
	DECLARE_TASK(TASK_SCANNER_PHOTOGRAPH)
	DECLARE_TASK(TASK_SCANNER_ATTACK_PRE_FLASH)
	DECLARE_TASK(TASK_SCANNER_ATTACK_FLASH)
	DECLARE_TASK(TASK_SCANNER_ATTACK_PRE_GAS)
	DECLARE_TASK(TASK_SCANNER_ATTACK_GAS)
	DECLARE_TASK(TASK_SCANNER_SPOT_INSPECT_ON)
	DECLARE_TASK(TASK_SCANNER_SPOT_INSPECT_WAIT)
	DECLARE_TASK(TASK_SCANNER_SPOT_INSPECT_OFF)
	DECLARE_TASK(TASK_SCANNER_CLEAR_INSPECT_TARGET)

	DECLARE_CONDITION(COND_SCANNER_FLY_CLEAR)
	DECLARE_CONDITION(COND_SCANNER_FLY_BLOCKED)
	DECLARE_CONDITION(COND_SCANNER_HAVE_INSPECT_TARGET)
	DECLARE_CONDITION(COND_SCANNER_INSPECT_DONE)
	DECLARE_CONDITION(COND_SCANNER_CAN_PHOTOGRAPH)
	DECLARE_CONDITION(COND_SCANNER_SPOT_ON_TARGET)

	DECLARE_ACTIVITY(ACT_SCANNER_SMALL_FLINCH_ALERT)
	DECLARE_ACTIVITY(ACT_SCANNER_SMALL_FLINCH_COMBAT)
	DECLARE_ACTIVITY(ACT_SCANNER_IDLE_ALERT)
	DECLARE_ACTIVITY(ACT_SCANNER_INSPECT)
	DECLARE_ACTIVITY(ACT_SCANNER_WALK_ALERT)
	DECLARE_ACTIVITY(ACT_SCANNER_WALK_COMBAT)
	DECLARE_ACTIVITY(ACT_SCANNER_FLARE)
	DECLARE_ACTIVITY(ACT_SCANNER_RETRACT)
	DECLARE_ACTIVITY(ACT_SCANNER_FLARE_PRONGS)
	DECLARE_ACTIVITY(ACT_SCANNER_RETRACT_PRONGS)

	DECLARE_INTERACTION(g_interactionScannerInspect)
	DECLARE_INTERACTION(g_interactionScannerInspectBegin)
	DECLARE_INTERACTION(g_interactionScannerInspectDone)
	DECLARE_INTERACTION(g_interactionScannerInspectHandsUp)
	DECLARE_INTERACTION(g_interactionScannerInspectShowArmband)
	DECLARE_INTERACTION(g_interactionScannerSupportEntity)
	DECLARE_INTERACTION(g_interactionScannerSupportPosition)

	//=========================================================
	// > SCHED_SCANNER_PATROL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_PATROL,

		"	Tasks"
		"		TASK_SCANNER_CLEAR_INSPECT_TARGET	0"
		"		TASK_SCANNER_SET_FLY_PATROL			0"
		"		TASK_SET_TOLERANCE_DISTANCE			48"
		"		TASK_SET_ROUTE_SEARCH_TIME			5"	// Spend 5 seconds trying to build a path if stuck
		"		TASK_GET_PATH_TO_RANDOM_NODE		2000"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		""
		"	Interrupts"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_PLAYER"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_SCANNER_HAVE_INSPECT_TARGET"
	)

	//=========================================================
	// > SCHED_SCANNER_SPOTLIGHT_HOVER
	//
	// Hover above target entity, trying to get spotlight
	// on my target
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_SPOTLIGHT_HOVER,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_SPOT			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_WALK  "
		"		TASK_WAIT							1"
		""
		"	Interrupts"
		"		COND_SCANNER_SPOT_ON_TARGET"
		"		COND_SCANNER_INSPECT_DONE"
		"		COND_SCANNER_FLY_BLOCKED"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCANNER_SPOTLIGHT_INSPECT_POS
	//
	// Inspect a position once spotlight is on it
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_SPOTLIGHT_INSPECT_POS,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_SPOT			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_SCANNER_INSPECT"
		"		TASK_SPEAK_SENTENCE					3"	// Curious sound
		"		TASK_WAIT							5"
		"		TASK_SCANNER_CLEAR_INSPECT_TARGET	0"
		""
		"	Interrupts"
		"		COND_SCANNER_INSPECT_DONE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCANNER_SPOTLIGHT_INSPECT_CIT
	//
	// Inspect a citizen once spotlight is on it
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_SPOTLIGHT_INSPECT_CIT,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_SPOT			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_SCANNER_INSPECT"
		"		TASK_SPEAK_SENTENCE					0"	// Stop!
		"		TASK_WAIT							1"
		"		TASK_SCANNER_SPOT_INSPECT_ON		0"
		"		TASK_WAIT							2"
		"		TASK_SPEAK_SENTENCE					1"	// Hands on head or Show Armband!
		"		TASK_WAIT							1"
		"		TASK_SCANNER_SPOT_INSPECT_WAIT		0"
		"		TASK_WAIT							5"
		"		TASK_SPEAK_SENTENCE					2"	// Free to go!
		"		TASK_WAIT							1"
		"		TASK_SCANNER_SPOT_INSPECT_OFF		0"
		"		TASK_SCANNER_CLEAR_INSPECT_TARGET	0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCANNER_PHOTOGRAPH_HOVER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_PHOTOGRAPH_HOVER,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_PHOTO			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_WALK"
		"		TASK_WAIT							3"
		""
		"	Interrupts"
		"		COND_SCANNER_INSPECT_DONE"
		"		COND_SCANNER_CAN_PHOTOGRAPH"
		"		COND_SCANNER_FLY_BLOCKED"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCANNER_ATTACK_HOVER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_ATTACK_HOVER,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_ATTACK			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_SCANNER_IDLE_ALERT"
		"		TASK_WAIT							0.1"
		""
		"	Interrupts"
		"		COND_TOO_FAR_TO_ATTACK"
		"		COND_SCANNER_FLY_BLOCKED"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCANNER_PHOTOGRAPH
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_PHOTOGRAPH,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_PHOTO			0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SCANNER_IDLE_ALERT"
		"		TASK_SCANNER_PHOTOGRAPH				0"
		""
		"	Interrupts"
		"		COND_SCANNER_INSPECT_DONE"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCANNER_ATTACK_FLASH
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_ATTACK_FLASH,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_ATTACK			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE_ANGRY"
		"		TASK_SCANNER_ATTACK_PRE_FLASH		0 "
		"		TASK_SCANNER_ATTACK_FLASH			0"
		"		TASK_WAIT							0.5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCANNER_ATTACK_GAS
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_ATTACK_GAS,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_ATTACK			0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_IDLE_ANGRY  "
		"		TASK_SCANNER_ATTACK_PRE_GAS			0"
		"		TASK_SCANNER_ATTACK_GAS				0"
		"		TASK_WAIT							0.5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// > SCHED_SCANNER_ATTACK_DIVEBOMB
	//
	// Only done when scanner is dead
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_ATTACK_DIVEBOMB,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_DIVE			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE_ANGRY  "
		"		TASK_WAIT							10"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_SCANNER_CHASE_ENEMY
	//
	//  Different interrupts than normal chase enemy.  
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_CHASE_ENEMY,

		"	Tasks"
		"		 TASK_SCANNER_SET_FLY_CHASE			0"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_SCANNER_PATROL"
		"		 TASK_SET_TOLERANCE_DISTANCE		120"
		"		 TASK_GET_PATH_TO_ENEMY				0"
		"		 TASK_RUN_PATH						0"
		"		 TASK_WAIT_FOR_MOVEMENT				0"
		""
		""
		"	Interrupts"
		"		COND_SCANNER_FLY_CLEAR"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_LOST_ENEMY"
	)

	//=========================================================
	// > SCHED_SCANNER_CHASE_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_CHASE_TARGET,

		"	Tasks"
		"		 TASK_SCANNER_SET_FLY_CHASE			0"
		"		 TASK_SET_TOLERANCE_DISTANCE		120"
		"		 TASK_GET_PATH_TO_TARGET			0"
		"		 TASK_RUN_PATH						0"
		"		 TASK_WAIT_FOR_MOVEMENT				0"
		""
		"	Interrupts"
		"		COND_SCANNER_FLY_CLEAR"
		"		COND_NEW_ENEMY"
	)

AI_END_CUSTOM_NPC()
