//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "AI_Default.h"
#include "AI_BaseNPC.h"
#include "AmmoDef.h"
#include "AI_Task.h"
#include "AI_Schedule.h"
#include "AI_Node.h"
#include "AI_Hull.h"
#include "AI_Memory.h"
#include "beam_shared.h"
#include "game.h"
#include "NPCEvent.h"
#include "EntityList.h"
#include "activitylist.h"
#include "soundent.h"
#include "gib.h"
#include "ndebugoverlay.h"
#include "smoke_trail.h"
#include "weapon_rpg.h"
#include "player.h"
#include "mathlib.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "ieffects.h"

#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"

extern Vector PointOnLineNearestPoint(const Vector& vStartPos, const Vector& vEndPos, const Vector& vPoint);

ConVar bulletSpeed( "bulletspeed", "6000" );
ConVar sniperLines( "showsniperlines", "0" );
ConVar sniperviewdist("sniperviewdist", "35" );
ConVar showsniperdist("showsniperdist", "0" );

// Moved to HL2_SharedGameRules because these are referenced by shared AmmoDef functions
extern ConVar sk_dmg_sniper_penetrate_plr;
extern ConVar sk_dmg_sniper_penetrate_npc;

// No model, impervious to damage.
#define SF_SNIPER_HIDDEN		(1 << 16)
#define SF_SNIPER_VIEWCONE		(1 << 17) // when set, sniper only sees in a small cone around the laser.
#define SF_SNIPER_NOCORPSE		(1 << 18) // when set, no corpse
#define SF_SNIPER_STARTDISABLED	(1 << 19)

#define SNIPER_PAINT_ENEMY_TIME			1.0f
#define SNIPER_FOG_PAINT_ENEMY_TIME	    0.25f
#define SNIPER_PAINT_DECOY_TIME			2.0f
#define SNIPER_PAINT_FRUSTRATED_TIME	1.0f
#define SNIPER_QUICKAIM_TIME			0.2f
#define SNIPER_PAINT_NO_SHOT_TIME		0.7f


// #def'ing this will turn on heaps of sniper debug messages.
#undef SNIPER_DEBUG


//---------------------------------------------------------
// Like an infotarget, but shares a spawnflag that has
// relevance to the sniper.
//---------------------------------------------------------
#define SF_SNIPERTARGET_SHOOTME		1
#define SF_SNIPERTARGET_NOINTERRUPT 2
#define SF_SNIPERTARGET_SNAPSHOT	4
#define SF_SNIPERTARGET_RESUME		8
#define SF_SNIPERTARGET_SNAPTO		16
#define SF_SNIPERTARGET_FOCUS		32


#define SNIPER_DECOY_RADIUS	256
#define SNIPER_NUM_DECOYS 5

#define NUM_OLDDECOYS	5

#define NUM_PENETRATIONS	3

#define PENETRATION_THICKNESS	5

#define SNIPER_MAX_GROUP_TARGETS	16


//=========================================================
//=========================================================
class CSniperTarget : public CPointEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CSniperTarget, CPointEntity );

	bool KeyValue( const char *szKeyName, const char *szValue );

	string_t m_iszGroupName;
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CSniperTarget )

	DEFINE_FIELD( CSniperTarget, m_iszGroupName, FIELD_STRING ),

END_DATADESC()


//=========================================================
//=========================================================
class CSniperBullet : public CBaseEntity
{
public:
	DECLARE_CLASS( CSniperBullet, CBaseEntity );

	CSniperBullet( void ) { Init(); }

	Vector	m_vecDir;

	Vector		m_vecStart;
	Vector		m_vecEnd;

	float	m_flLastThink;
	float	m_SoundTime;
	int		m_AmmoType;
	int		m_PenetratedAmmoType;
	
	void Precache( void );
	bool IsActive( void ) { return m_fActive; }

	bool Start( const Vector &vecOrigin, const Vector &vecTarget );
	void Stop( void );

	void Think( void );

	void Init( void );

	DECLARE_DATADESC();

private:

	// Only one shot per sniper at a time. If a bullet hasn't
	// hit, the shooter must wait.
	bool	m_fActive;

	// This tracks how many times this single bullet has 
	// struck. This is for penetration, so the bullet can
	// go through things.
	int		m_iImpacts;
};


//=========================================================
//=========================================================
class CProtoSniper : public CAI_BaseNPC
{
	DECLARE_CLASS( CProtoSniper, CAI_BaseNPC );

public:
	CProtoSniper( void );
	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void );
	float	MaxYawSpeed( void );
	Vector EyePosition( void );

	bool IsLaserOn( void ) { return m_pBeam != NULL; }

	void Event_Killed( const CTakeDamageInfo &info );
	int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	bool WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions) {return true;}

	virtual bool FInViewCone( CBaseEntity *pEntity );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	int RangeAttack1Conditions ( float flDot, float flDist );
	bool FireBullet( const Vector &vecTarget, bool fDirectShot );
	Vector	LeadTarget( CBaseEntity *pTarget );

	virtual int SelectSchedule( void );
	virtual int TranslateSchedule( int scheduleType );

	bool KeyValue( const char *szKeyName, const char *szValue );

	void PrescheduleThink( void );

	static const char *pAttackSounds[];

	bool FCanCheckAttacks ( void );
	bool FindDecoyObject( void );

	int GetSoundInterests( void );

	Vector GetBulletOrigin( void );

	virtual int Restore( IRestore &restore );

	virtual void OnScheduleChange( void );

	bool FVisible( CBaseEntity *pEntity, int traceMask = MASK_OPAQUE, CBaseEntity **ppBlocker = NULL );

private:
	
	bool ShouldSnapShot( void );
	void ClearTargetGroup( void );

	float GetPositionParameter( float flTime, bool fLinear );

	void GetPaintAim( const Vector &vecStart, const Vector &vecGoal, float flParameter, Vector *pProgress );

	bool IsSweepingRandomly( void ) { return m_iNumGroupTargets > 0; }

	void ClearOldDecoys( void );
	void AddOldDecoy( CBaseEntity *pDecoy );
	bool HasOldDecoy( CBaseEntity *pDecoy );
	void FindFrustratedShot( float flNoise );

	bool VerifyShot( void );

	// Inputs
	void InputEnableSniper( inputdata_t &inputdata );
	void InputDisableSniper( inputdata_t &inputdata );
	void InputSetDecoyRadius( inputdata_t &inputdata );
	void InputSweepTarget( inputdata_t &inputdata );
	void InputSweepGroupRandomly( inputdata_t &inputdata );

	void LaserOff( void );
	void LaserOn( const Vector &vecTarget, const Vector &vecDeviance );

	void PaintTarget( const Vector &vecTarget, float flPaintTime );

	// This keeps track of the last spot the laser painted. For 
	// continuous sweeping that changes direction.
	Vector m_vecPaintCursor;
	float  m_flPaintTime;

	bool						m_fWeaponLoaded;
	bool						m_fEnabled;
	bool						m_fIsPatient;
	float						m_flPatience;
	int							m_iMisses;
	EHANDLE						m_hDecoyObject;
	EHANDLE						m_hSweepTarget;
	Vector						m_vecDecoyObjectTarget;
	Vector						m_vecFrustratedTarget;
	Vector						m_vecPaintStart; // used to track where a sweep starts for the purpose of interpolating.

	float						m_flFrustration;

	float						m_flThinkInterval;

	float						m_flDecoyRadius;

	CBeam						*m_pBeam;

	bool m_fSnapShot;

	int							m_iNumGroupTargets;
	CBaseEntity					*m_pGroupTarget[ SNIPER_MAX_GROUP_TARGETS ];

	DEFINE_CUSTOM_AI;

	DECLARE_DATADESC();
};


//=========================================================
//=========================================================
// NOTES about the Sniper:
//
// PATIENCE:
// The concept of "patience" is simply a restriction placed
// on how close a target has to be to the sniper before the 
// sniper will take his first shot at the target. This
// distance is referred to as "patience" is set by the `
// designer in Worldcraft. The sniper won't attack unless 
// the target enters this radius. Once the sniper takes 
// this first shot, he will not return to a patient state.
// He will then shoot at any/all targets to which there is 
// a clear shot, regardless of distance. (sjb)
//
//
// TODO: Sniper accumulates frustration while reloading.
//		 probably should subtract reload time from frustration.
//=========================================================
//=========================================================


//=========================================================
//=========================================================
short sFlashSprite;
short sHaloSprite;

//=========================================================
//=========================================================
BEGIN_DATADESC( CProtoSniper )

	DEFINE_FIELD( CProtoSniper, m_fWeaponLoaded, FIELD_BOOLEAN ),
	DEFINE_FIELD( CProtoSniper, m_fEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( CProtoSniper, m_fIsPatient, FIELD_BOOLEAN ),
	DEFINE_FIELD( CProtoSniper, m_flPatience, FIELD_FLOAT ),
	DEFINE_FIELD( CProtoSniper, m_iMisses, FIELD_INTEGER ),
	DEFINE_FIELD( CProtoSniper, m_hDecoyObject, FIELD_EHANDLE ),
	DEFINE_FIELD( CProtoSniper, m_hSweepTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CProtoSniper, m_vecDecoyObjectTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CProtoSniper, m_vecFrustratedTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CProtoSniper, m_vecPaintStart, FIELD_VECTOR ),
	DEFINE_FIELD( CProtoSniper, m_flPaintTime, FIELD_TIME ),
	DEFINE_FIELD( CProtoSniper, m_vecPaintCursor, FIELD_VECTOR ),
	DEFINE_FIELD( CProtoSniper, m_flFrustration, FIELD_TIME ),
	DEFINE_FIELD( CProtoSniper, m_flThinkInterval, FIELD_FLOAT ),
	DEFINE_FIELD( CProtoSniper, m_flDecoyRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CProtoSniper, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CProtoSniper, m_fSnapShot, FIELD_BOOLEAN ),
	DEFINE_FIELD( CProtoSniper, m_iNumGroupTargets, FIELD_INTEGER ),
	DEFINE_ARRAY( CProtoSniper, m_pGroupTarget, FIELD_CLASSPTR, SNIPER_MAX_GROUP_TARGETS  ),

	// Inputs
	DEFINE_INPUTFUNC( CProtoSniper, FIELD_VOID, "EnableSniper", InputEnableSniper ),
	DEFINE_INPUTFUNC( CProtoSniper, FIELD_VOID, "DisableSniper", InputDisableSniper ),
	DEFINE_INPUTFUNC( CProtoSniper, FIELD_INTEGER, "SetDecoyRadius", InputSetDecoyRadius ),
	DEFINE_INPUTFUNC( CProtoSniper, FIELD_STRING, "SweepTarget", InputSweepTarget ),
	DEFINE_INPUTFUNC( CProtoSniper, FIELD_STRING, "SweepGroupRandomly", InputSweepGroupRandomly )

END_DATADESC()



//=========================================================
//=========================================================
BEGIN_DATADESC( CSniperBullet )

	DEFINE_FIELD( CSniperBullet, m_SoundTime, FIELD_TIME ),
	DEFINE_FIELD( CSniperBullet, m_AmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CSniperBullet, m_PenetratedAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CSniperBullet, m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSniperBullet, m_iImpacts, FIELD_INTEGER ),
	DEFINE_FIELD( CSniperBullet, m_vecOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CSniperBullet, m_vecDir, FIELD_VECTOR ),
	DEFINE_FIELD( CSniperBullet, m_flLastThink, FIELD_TIME ),

	DEFINE_FIELD( CSniperBullet, m_vecStart, FIELD_VECTOR ),
	DEFINE_FIELD( CSniperBullet, m_vecEnd, FIELD_VECTOR ),

	DEFINE_FUNCTION( CSniperBullet, Think ),

END_DATADESC()

//=========================================================
// Private conditions
//=========================================================
enum Sniper_Conds
{
	COND_SNIPER_CANATTACKDECOY = LAST_SHARED_CONDITION,
	COND_SNIPER_SUPPRESSED,
	COND_SNIPER_ENABLED,
	COND_SNIPER_DISABLED,
	COND_SNIPER_FRUSTRATED,
	COND_SNIPER_SWEEP_TARGET,
	COND_SNIPER_NO_SHOT,
};


//=========================================================
// schedules
//=========================================================
enum
{
	SCHED_PSNIPER_SCAN = LAST_SHARED_SCHEDULE,
	SCHED_PSNIPER_CAMP,
	SCHED_PSNIPER_ATTACK,
	SCHED_PSNIPER_RELOAD,
	SCHED_PSNIPER_ATTACKDECOY,
	SCHED_PSNIPER_SUPPRESSED,
	SCHED_PSNIPER_DISABLEDWAIT,
	SCHED_PSNIPER_FRUSTRATED_ATTACK,
	SCHED_PSNIPER_SWEEP_TARGET,
	SCHED_PSNIPER_SWEEP_TARGET_NOINTERRUPT,
	SCHED_PSNIPER_SNAPATTACK,
	SCHED_PSNIPER_NO_CLEAR_SHOT,
};

//=========================================================
// tasks
//=========================================================
enum
{
	TASK_SNIPER_FRUSTRATED_ATTACK = LAST_SHARED_TASK,
	TASK_SNIPER_PAINT_ENEMY,
	TASK_SNIPER_PAINT_DECOY,
	TASK_SNIPER_PAINT_FRUSTRATED,
	TASK_SNIPER_PAINT_SWEEP_TARGET,
	TASK_SNIPER_ATTACK_CURSOR,
	TASK_SNIPER_PAINT_NO_SHOT,
};



CProtoSniper::CProtoSniper( void ) 
{ 
#ifdef _DEBUG
	m_vecPaintCursor.Init();
	m_vecDecoyObjectTarget.Init();
	m_vecFrustratedTarget.Init();
	m_vecPaintStart.Init();
#endif

	m_iMisses = 0; 
	m_flDecoyRadius = SNIPER_DECOY_RADIUS; 
	m_fSnapShot = false; 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CProtoSniper::FInViewCone ( CBaseEntity *pEntity )
{
	if( pEntity->GetFlags() & FL_CLIENT )
	{
		CBasePlayer *pPlayer;

		pPlayer = ToBasePlayer( pEntity );

		if( m_spawnflags & SF_SNIPER_VIEWCONE )
		{
			// See how close this spot is to the laser.
			Vector	vecEyes;
			Vector	vecLOS;
			float	flDist;
			Vector	vecNearestPoint;

			vecEyes = EyePosition();
			vecLOS = m_vecPaintCursor - vecEyes;
			VectorNormalize(vecLOS);

			vecNearestPoint = PointOnLineNearestPoint( EyePosition(), EyePosition() + vecLOS * 8192, pPlayer->EyePosition() );

			flDist = ( pPlayer->EyePosition() - vecNearestPoint ).Length();

			if( showsniperdist.GetFloat() != 0 )
			{
				Msg( "Dist from beam: %f\n", flDist );
			}

			if( flDist <= sniperviewdist.GetFloat() )
			{
				return true;
			}

			return false;
		}
	}

	return BaseClass::FInViewCone( pEntity->EyePosition() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CProtoSniper::LaserOff( void )
{
	if( m_pBeam )
	{
		UTIL_Remove( m_pBeam);
		m_pBeam = NULL;
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define LASER_LEAD_DIST	64
void CProtoSniper::LaserOn( const Vector &vecTarget, const Vector &vecDeviance )
{
	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate( "sprites/laserbeam.vmt", 0.3 );
		m_pBeam->SetColor( 0, 100, 255 );
	}
	else
	{
		// Beam seems to be on.
		//return;
	}

	// Don't aim right at the guy right now.
	Vector vecInitialAim;

	if( vecDeviance == vec3_origin )
	{
		// Start the aim where it last left off!
		vecInitialAim = m_vecPaintCursor;
	}
	else
	{
		vecInitialAim = vecTarget;
	}

	vecInitialAim.x += random->RandomFloat( -vecDeviance.x, vecDeviance.x );
	vecInitialAim.y += random->RandomFloat( -vecDeviance.y, vecDeviance.y );
	vecInitialAim.z += random->RandomFloat( -vecDeviance.z, vecDeviance.z );
	
	m_pBeam->PointsInit( GetBulletOrigin(), vecInitialAim );
	m_pBeam->SetBrightness( 75 );
	m_pBeam->SetNoise( 0 );
	m_pBeam->SetWidth( 2.0 );
	m_pBeam->SetScrollRate( 4 );


	m_vecPaintStart = vecInitialAim;

	// Think faster whilst painting. Higher resolution on the 
	// beam movement.
	SetNextThink( gpGlobals->curtime + 0.02 );
}

//-----------------------------------------------------------------------------
// Crikey!
//-----------------------------------------------------------------------------
float CProtoSniper::GetPositionParameter( float flTime, bool fLinear )
{
	float flElapsedTime;
	float flTimeParameter;

	flElapsedTime = flTime - (m_flWaitFinished - gpGlobals->curtime);

	flTimeParameter = ( flElapsedTime / flTime );

	if( fLinear )
	{
		return flTimeParameter;
	}
	else
	{
		return (1 + sin( (M_PI * flTimeParameter) - (M_PI / 2) ) ) / 2;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CProtoSniper::GetPaintAim( const Vector &vecStart, const Vector &vecGoal, float flParameter, Vector *pProgress )
{
#if 0
	Vector vecDelta;

	vecDelta = vecGoal - vecStart;

	float flDist = VectorNormalize( vecDelta );

	vecDelta = vecStart + vecDelta * (flDist * flParameter);

	vecDelta = (vecDelta - GetBulletOrigin() ).Normalize();

	*pProgress = vecDelta;
#else
	// Quaternions
	Vector vecIdealDir;
	QAngle vecIdealAngles;
	QAngle vecCurrentAngles;
	Vector vecCurrentDir;
	Vector vecBulletOrigin = GetBulletOrigin();

	// vecIdealDir is where the gun should be aimed when the painting
	// time is up. This can be approximate. This is only for drawing the
	// laser, not actually aiming the weapon. A large discrepancy will look
	// bad, though.
	vecIdealDir = vecGoal - vecBulletOrigin;
	VectorNormalize(vecIdealDir);

	// Now turn vecIdealDir into angles!
	VectorAngles( vecIdealDir, vecIdealAngles );

	// This is the vector of the beam's current aim.
	vecCurrentDir = m_vecPaintStart - vecBulletOrigin;
	VectorNormalize(vecCurrentDir);

	// Turn this to angles, too.
	VectorAngles( vecCurrentDir, vecCurrentAngles );

	Quaternion idealQuat;
	Quaternion currentQuat;
	Quaternion aimQuat;

	AngleQuaternion( vecIdealAngles, idealQuat );
	AngleQuaternion( vecCurrentAngles, currentQuat );

	QuaternionSlerp( currentQuat, idealQuat, flParameter, aimQuat );

	QuaternionAngles( aimQuat, vecCurrentAngles );

	// Rebuild the current aim vector.
	AngleVectors( vecCurrentAngles, &vecCurrentDir );

	*pProgress = vecCurrentDir;
#endif
}

//-----------------------------------------------------------------------------
// Sweep the laser sight towards the point where the gun should be aimed
//-----------------------------------------------------------------------------
void CProtoSniper::PaintTarget( const Vector &vecTarget, float flPaintTime )
{
	Vector vecCurrentDir;
	Vector vecStart;

	// vecStart is the barrel of the gun (or the laser sight)
	vecStart = GetBulletOrigin();

	float P;

	P = GetPositionParameter( flPaintTime, false );

	GetPaintAim( m_vecPaintStart, vecTarget, P, &vecCurrentDir );

#if 1
#define SNAR 0.8
	float flNoiseScale;
	
	if ( P >= SNAR )
	{
		flNoiseScale = 1 - (1 / (1 - SNAR)) * ( P - SNAR );
	}
	else if ( P <= 1 - SNAR )
	{
		flNoiseScale = P / (1 - SNAR);
	}
	else
	{
		flNoiseScale = 1;
	}

	// mult by P
	vecCurrentDir.x += flNoiseScale * ( sin( 3 * M_PI * gpGlobals->curtime ) * 0.0006 );
	vecCurrentDir.y += flNoiseScale * ( sin( 2 * M_PI * gpGlobals->curtime + 0.5 * M_PI ) * 0.0006 );
	vecCurrentDir.z += flNoiseScale * ( sin( 1.5 * M_PI * gpGlobals->curtime + M_PI ) * 0.0006 );
#endif

	trace_t tr;

	UTIL_TraceLine( vecStart, vecStart + vecCurrentDir * 8192, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	m_pBeam->SetEndPos( tr.endpos );
	m_pBeam->RelinkBeam();

	m_vecPaintCursor = tr.endpos;
}

			
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CProtoSniper::InputSetDecoyRadius( inputdata_t &inputdata )
{
	m_flDecoyRadius = (float)inputdata.value.Int();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CProtoSniper::OnScheduleChange( void )
{
	LaserOff();

	BaseClass::OnScheduleChange();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CProtoSniper::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "radius"))
	{
		m_flPatience = atof(szValue);

		// If the designer specifies a patience radius of 0, the
		// sniper won't have any patience at all. The sniper will 
		// shoot at the first target it sees regardless of distance.
		if( m_flPatience == 0.0 )
		{
			m_fIsPatient = false;
		}
		else
		{
			m_fIsPatient = true;
		}

		return true;
	}
	else if( FStrEq(szKeyName, "misses") )
	{
		m_iMisses = atoi( szValue );
		return true;
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}
}

LINK_ENTITY_TO_CLASS( npc_sniper, CProtoSniper );
LINK_ENTITY_TO_CLASS( proto_sniper, CProtoSniper );
LINK_ENTITY_TO_CLASS( sniperbullet, CSniperBullet );


//---------------------------------------------------------
// Sounds
//---------------------------------------------------------
const char *pProtoSniperSounds[] = 
{
	"npc/sniper/sniper1.wav",
	"npc/sniper/echo1.wav",
	"npc/sniper/sniper2.wav",
	"npc/sniper/reload1.wav",
};


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CProtoSniper::Precache( void )
{
	PRECACHE_SOUND_ARRAY(pProtoSniperSounds);
	engine->PrecacheModel("models/combine_soldier.mdl");
	sHaloSprite = engine->PrecacheModel("sprites/blueflare1.vmt");
	sFlashSprite = engine->PrecacheModel( "sprites/muzzleflash1.vmt" );
	engine->PrecacheModel("sprites/laserbeam.vmt");	

	UTIL_PrecacheOther( "sniperbullet" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CProtoSniper::Spawn( void )
{
	Precache();

	/// HACK:
	SetModel( "models/combine_soldier.mdl" );

	//m_hBullet = (CSniperBullet *)Create( "sniperbullet", GetBulletOrigin(), GetLocalAngles(), NULL );

	//Assert( m_hBullet != NULL );

	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();

	UTIL_SetSize( this, Vector( -16, -16 , 0 ), Vector( 16, 16, 64 ) );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_FLY );
	m_bloodColor		= DONT_BLEED;
	m_iHealth			= 10;
	m_flFieldOfView		= 0.2;
	m_NPCState			= NPC_STATE_NONE;

	if( HasSpawnFlags( SF_SNIPER_STARTDISABLED ) )
	{
		m_fEnabled = false;
	}
	else
	{
		m_fEnabled = true;
	}

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 );

	m_HackedGunPos = Vector ( 0, 0, 0 );

	m_spawnflags |= SF_NPC_LONG_RANGE;
	m_spawnflags |= SF_NPC_ALWAYSTHINK;

	m_pBeam = NULL;

	ClearOldDecoys();

	NPCInit();

	if( m_spawnflags & SF_SNIPER_HIDDEN )
	{
		m_fEffects |= EF_NODRAW;
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	// Point the cursor straight ahead so that the sniper's
	// first sweep of the laser doesn't look weird.
	Vector vecForward;
	AngleVectors( GetLocalAngles(), &vecForward );
	m_vecPaintCursor = GetBulletOrigin() + vecForward * 1024;

	m_fWeaponLoaded = true;

	//m_debugOverlays |= OVERLAY_TEXT_BIT;

	// none!
	GetEnemies()->SetFreeKnowledgeDuration( 0.0 );
}


//-----------------------------------------------------------------------------
// Purpose: Forces an idle sniper to paint the specified target.
//-----------------------------------------------------------------------------
void CProtoSniper::InputSweepTarget( inputdata_t &inputdata )
{
	CBaseEntity *pTarget;

	// In case the sniper was sweeping a random set of targets when asked to sweep a normal chain.
	ClearTargetGroup();

	pTarget = gEntList.FindEntityByName( NULL, inputdata.value.String(), inputdata.pActivator );

	if( !pTarget )
	{
		Msg( "**Sniper %s cannot find sweep target %s\n", GetClassname(), inputdata.value.String() );
		m_hSweepTarget = NULL;
		return;
	}

	m_hSweepTarget = pTarget;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CProtoSniper::ClearTargetGroup( void )
{
	int i;

	for( i = 0 ; i < SNIPER_MAX_GROUP_TARGETS ; i++ )
	{
		m_pGroupTarget[ i ] = NULL;
	}

	m_iNumGroupTargets = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Similar to SweepTarget, but forces the sniper to sweep targets
// in a group (bound by groupname) randomly until interrupted.
//-----------------------------------------------------------------------------
void CProtoSniper::InputSweepGroupRandomly( inputdata_t &inputdata )
{
	ClearTargetGroup();

	CBaseEntity *pEnt;

	// PERFORMANCE
	// Go through the whole ent list? This could hurt. (sjb)
	pEnt = gEntList.FirstEnt();

	do
	{
		CSniperTarget *pTarget;

		pTarget = dynamic_cast<CSniperTarget*>(pEnt);

		// If the pointer is null, this isn't a sniper target.
		if( pTarget )
		{
			if( !strcmp( inputdata.value.String(), STRING( pTarget->m_iszGroupName ) ) )
			{
				m_pGroupTarget[ m_iNumGroupTargets ] = pTarget;
				m_iNumGroupTargets++;
			}
		}

		pEnt = gEntList.NextEnt( pEnt );

	} while( pEnt );

	Msg( "Counted %d targets\n", m_iNumGroupTargets );

	m_hSweepTarget = m_pGroupTarget[ random->RandomInt( 0, m_iNumGroupTargets - 1 ) ];
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CProtoSniper::Classify( void )
{
	return	CLASS_COMBINE;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CProtoSniper::GetBulletOrigin( void )
{
	if( m_spawnflags & SF_SNIPER_HIDDEN )
	{
		return GetLocalOrigin();
	}
	else
	{
		Vector vecForward;
		AngleVectors( GetLocalAngles(), &vecForward );
		return WorldSpaceCenter() + vecForward * 20;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CProtoSniper::ClearOldDecoys( void )
{
#if 0
	int i;

	for( i = 0 ; i < NUM_OLDDECOYS ; i++ )
	{
		m_pOldDecoys[ i ] = NULL;
	}

	m_iOldDecoySlot = 0;
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CProtoSniper::HasOldDecoy( CBaseEntity *pDecoy )
{
#if 0
	int i;

	for( i = 0 ; i < NUM_OLDDECOYS ; i++ )
	{
		if( m_pOldDecoys[ i ] == pDecoy )
		{
			return true;
		}
	}
#endif 

	return false;
}


//-----------------------------------------------------------------------------
// The list of old decoys is just a circular list. We put decoys that we've 
// already fired at in this list. When they've been pushed off the list by others,
// then they are valid targets again.
//-----------------------------------------------------------------------------
void CProtoSniper::AddOldDecoy( CBaseEntity *pDecoy )
{
#if 0
	m_pOldDecoys[ m_iOldDecoySlot ] = pDecoy;
	m_iOldDecoySlot++;

	if( m_iOldDecoySlot == NUM_OLDDECOYS )
	{
		m_iOldDecoySlot = 0;
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Only blast damage can hurt a sniper.
//
//
// Output : 
//-----------------------------------------------------------------------------
int CProtoSniper::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if( !m_fEnabled )
	{
		// As good as not existing.
		return 0;
	}

	if( !(info.GetDamageType() & DMG_BLAST) )
	{
		// Only blasts hurt
		return 0;
	}

	if( info.GetDamage() < m_iHealth )
	{
		// Only blasts powerful enough to kill hurt
		return 0;
	}

	return CAI_BaseNPC::OnTakeDamage_Alive( info );
}


//-----------------------------------------------------------------------------
// Purpose: When a sniper is killed, we launch a fake ragdoll corpse as if the
// sniper was blasted out of his nest.
//
//
// Output : 
//-----------------------------------------------------------------------------
void CProtoSniper::Event_Killed( const CTakeDamageInfo &info )
{
	if( !(m_spawnflags & SF_SNIPER_NOCORPSE) )
	{
		Vector vecForward;

		
		float flForce = random->RandomFloat( 500, 700 ) * 10;

		AngleVectors( GetLocalAngles(), &vecForward );
		CreateRagGib( "models/combine_soldier.mdl", GetLocalOrigin(), GetLocalAngles(), (vecForward * flForce) + Vector(0, 0, 600) );
	}

	m_OnDeath.FireOutput( info.GetAttacker(), this );

	LaserOff();

	UTIL_Remove( this );
}


//---------------------------------------------------------
//---------------------------------------------------------
int CProtoSniper::SelectSchedule ( void )
{
	if( !m_fWeaponLoaded )
	{
		// Reload is absolute priority.
		return SCHED_RELOAD;
	}
	
	if( HasCondition( COND_HEAR_DANGER ) )
	{
		// Next priority is to be suppressed!
		return SCHED_PSNIPER_SUPPRESSED;
	}

	// OK. If you fall through all the cases above, but you're DISABLED,
	// play the schedule that waits a little while and tries again.
	if( !m_fEnabled )
	{
		return SCHED_PSNIPER_DISABLEDWAIT;
	}

	if( HasCondition( COND_SNIPER_SWEEP_TARGET ) )
	{
		// Sweep a target. Scripted by level designers!
		if( m_hSweepTarget->HasSpawnFlags( SF_SNIPERTARGET_NOINTERRUPT ) )
		{
			return SCHED_PSNIPER_SWEEP_TARGET_NOINTERRUPT;
		}
		else
		{
			return SCHED_PSNIPER_SWEEP_TARGET;
		}
	}

	if( GetEnemy() == NULL || HasCondition( COND_ENEMY_DEAD ) )
	{
		// Look for an enemy.
		SetEnemy( NULL );
		return SCHED_PSNIPER_SCAN;
	}

	if( HasCondition( COND_SNIPER_NO_SHOT ) )
	{
		return SCHED_PSNIPER_NO_CLEAR_SHOT;
	}

	if( HasCondition( COND_SNIPER_FRUSTRATED ) )
	{
		return SCHED_PSNIPER_FRUSTRATED_ATTACK;
	}

	if( HasCondition( COND_SNIPER_CANATTACKDECOY ) )
	{
		return SCHED_RANGE_ATTACK2;
	}

	if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
		// shoot!
		return SCHED_RANGE_ATTACK1;
	}
	else
	{
		// Camp on this target
		return SCHED_PSNIPER_CAMP;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
int CProtoSniper::GetSoundInterests( void )
{
	// Suppress when you hear danger sound
	return SOUND_DANGER;
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CProtoSniper::FCanCheckAttacks ( void )
{
	return true;
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CProtoSniper::FindDecoyObject( void )
{
#define SEARCH_DEPTH	50

	CBaseEntity *pDecoys[ SNIPER_NUM_DECOYS ];
	CBaseEntity *pList[ SEARCH_DEPTH ];
	CBaseEntity	*pCurrent;
	int			count;
	int			i;
	Vector vecTarget = GetEnemy()->GetLocalOrigin();
	Vector vecDelta;

	m_hDecoyObject = NULL;
	
	for( i = 0 ; i < SNIPER_NUM_DECOYS ; i++ )
	{
		pDecoys[ i ] = NULL;
	}

	vecDelta.x = m_flDecoyRadius;
	vecDelta.y = m_flDecoyRadius;
	vecDelta.z = m_flDecoyRadius;

	count = UTIL_EntitiesInBox( pList, SEARCH_DEPTH, vecTarget - vecDelta, vecTarget + vecDelta, 0 );

	// Now we have the list of entities near the target. 
	// Dig through that list and build the list of decoys.
	int iIterator = 0;

	for( i = 0 ; i < count ; i++ )
	{
		pCurrent = pList[ i ];

		if( FClassnameIs( pCurrent, "func_breakable" ) || FClassnameIs( pCurrent, "prop_physics" ) || FClassnameIs( pCurrent, "func_physbox" ) )
		{
			// This item meets criteria for a decoy object to shoot at. 

			// But have we shot at this item recently? If we HAVE, don't add it.
#if 0
			if( !HasOldDecoy( pCurrent ) )
#endif
			{
				pDecoys[ iIterator ] = pCurrent;

				if( iIterator == SNIPER_NUM_DECOYS - 1 )
				{
					break;
				}
				else
				{
					iIterator++;
				}
			}
		}
	}

	if( iIterator == 0 )
	{
		return false;
	}

	// try 4 times to pick a random object from the list
	// and trace to it. If the trace goes off, that's the object!
	
	for( i = 0 ; i < 4 ; i++ )
	{
		CBaseEntity *pProspect;
		trace_t		tr;

		// Pick one of the decoys at random.
		pProspect = pDecoys[ random->RandomInt( 0, iIterator - 1 ) ];

		Vector vecDecoyTarget;
		Vector vecDirToDecoy;
		Vector vecBulletOrigin;

		vecBulletOrigin = GetBulletOrigin();
		pProspect->RandomPointInBounds( Vector( .1, .1, .1 ), Vector( .9, .9, .9 ), &vecDecoyTarget );

		// When trying to trace to an object using its absmin + some fraction of its size, it's best 
		// to lengthen the trace a little beyond the object's bounding box in case it's a more complex
		// object, or not axially aligned. 
		vecDirToDecoy = vecDecoyTarget - vecBulletOrigin;
		VectorNormalize(vecDirToDecoy);

		
		// Right now, tracing with MASK_OPAQUE and checking the fraction as well as the object the trace
		// has hit makes it possible for the decoy behavior to shoot through glass. 
		UTIL_TraceLine( vecBulletOrigin, vecDecoyTarget + vecDirToDecoy * 32, 
			MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);

		if( tr.m_pEnt == pProspect || tr.fraction == 1.0 )
		{
			// Great! A shot will hit this object.
			m_hDecoyObject = pProspect;
			m_vecDecoyObjectTarget = tr.endpos;

			// Throw some noise in, don't always hit the center.
			Vector vecNoise;
			pProspect->RandomPointInBounds( Vector( 0.25, 0.25, 0.25 ), Vector( 0.75, 0.75, 0.75 ), &vecNoise );
			m_vecDecoyObjectTarget += vecNoise - pProspect->GetAbsOrigin();
			return true;
		}
	}

	return false;
}


//---------------------------------------------------------
//---------------------------------------------------------
#define SNIPER_SNAP_SHOT_VELOCITY	125
bool CProtoSniper::ShouldSnapShot( void )
{
	if( GetEnemy()->GetFlags() & FL_CLIENT )
	{
		if( GetEnemy()->GetAbsVelocity().Length() >= SNIPER_SNAP_SHOT_VELOCITY )
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	// Right now, always snapshot at NPC's
	return true;
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CProtoSniper::VerifyShot( void )
{
	trace_t tr;

	UTIL_TraceLine( GetBulletOrigin(), GetEnemy()->WorldSpaceCenter(), MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );

	if( tr.fraction != 1.0 )
	{
		return false;
	}
	else
	{
		return true;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
int CProtoSniper::RangeAttack1Conditions ( float flDot, float flDist )
{
	float fFrustration;
	fFrustration = gpGlobals->curtime - m_flFrustration;

	//Msg( "Frustration: %f\n", fFrustration );

	if( HasCondition( COND_SEE_ENEMY ) && !HasCondition( COND_ENEMY_OCCLUDED ) )
	{
		if( VerifyShot() )
		{
			// Can see the player, have a clear shot to his midsection
			ClearCondition( COND_SNIPER_NO_SHOT );
		}
		else
		{
			// Can see the player, but can't take a shot at his midsection
			//Msg( "BOBBA\n" );
			SetCondition( COND_SNIPER_NO_SHOT );
			return COND_NONE;
		}

		if( m_fIsPatient )
		{
			// This sniper has a clear shot at the target, but can not take
			// the shot if he is being patient and the target is outside
			// of the patience radius.

			float flDist;

			flDist = ( GetLocalOrigin() - GetEnemy()->GetLocalOrigin() ).Length2D();

			if( flDist <= m_flPatience )
			{
				// This target is close enough to attack!
				return COND_CAN_RANGE_ATTACK1;
			}
			else
			{
				// Be patient...
				return COND_NONE;
			}
		}
		else
		{
			// Not being patient. Clear for attack.
			return COND_CAN_RANGE_ATTACK1;
		}
	}
	else if( fFrustration >= 2 && !m_fIsPatient && !m_hDecoyObject )
	{
		// Sniper is tired of waiting now. He's going to try to take a shot at
		// a physics object or func_breakable near the target. Or if really
		// frustrated, just take an air shot into the player's line of sight.
		if( FindDecoyObject() )
		{
			return COND_SNIPER_CANATTACKDECOY;
		}
		else if( gpGlobals->curtime - m_flFrustration >= 2.5 )
		{
			// If the sniper doesn't see a decoy for a long time, he's
			// frustrated and takes a wild shot somewhere near the hiding
			// target.
			return COND_SNIPER_FRUSTRATED;
		}
	}

	return COND_NONE;
}


//---------------------------------------------------------
//---------------------------------------------------------
int CProtoSniper::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_RANGE_ATTACK1:
		if( m_hSweepTarget != NULL && m_fSnapShot && ShouldSnapShot() )
		{
			return SCHED_PSNIPER_SNAPATTACK;
		}

		return SCHED_PSNIPER_ATTACK;
		break;

	case SCHED_RANGE_ATTACK2:
		return SCHED_PSNIPER_ATTACKDECOY;
		break;

	case SCHED_RELOAD:
		return SCHED_PSNIPER_RELOAD;
		break;
	}
	return BaseClass::TranslateSchedule( scheduleType );
}


//---------------------------------------------------------
// This starts the bullet state machine.  The actual effects
// of the bullet will happen later.  This function schedules 
// those effects.
//
// fDirectShot indicates whether the bullet is a "direct shot"
// that is - fired with the intent that it will strike the
// enemy. Otherwise, the bullet is intended to strike a 
// decoy object or nothing at all in particular.
//---------------------------------------------------------
bool CProtoSniper::FireBullet( const Vector &vecTarget, bool fDirectShot )
{
	CSniperBullet	*pBullet;
	Vector			vecBulletOrigin;

	vecBulletOrigin = GetBulletOrigin();

	pBullet = (CSniperBullet *)Create( "sniperbullet", GetBulletOrigin(), GetLocalAngles(), NULL );

	Assert( pBullet != NULL );

	if( !pBullet->Start( vecBulletOrigin, vecTarget ) )
	{
		// Bullet must still be active.
		return false;
	}

	pBullet->SetOwnerEntity( this );

	CPASAttenuationFilter filternoatten( this, ATTN_NONE );
	EmitSound( filternoatten, entindex(), CHAN_VOICE, pProtoSniperSounds[ 1 ], 0.8, ATTN_NONE );

	CPVSFilter filter( vecBulletOrigin );
	te->Sprite( filter, 0.0, &vecBulletOrigin, sFlashSprite, 0.3, 255 );
	
	// force a reload when we're done
	 m_fWeaponLoaded = false;

	// Once the sniper takes a shot, turn the patience off!
	m_fIsPatient = false;

	// Alleviate frustration, too!
	m_flFrustration = gpGlobals->curtime;

	// This may have been a snap shot.
	// Don't allow subsequent snap shots.
	m_fSnapShot = false;

	// Sniper had to be aiming here to fire here.
	// Make it the cursor.
	m_vecPaintCursor = vecTarget;

	m_hDecoyObject.Set( NULL );

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CProtoSniper::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SNIPER_ATTACK_CURSOR:
		break;

	case TASK_RANGE_ATTACK1:
		// Start task does nothing here.
		// We fall through to RunTask() which will keep trying to take
		// the shot until the weapon is ready to fire. In some rare cases,
		// the weapon may be ready to fire before the single bullet allocated
		// to the sniper has hit its target. 
		break;

	case TASK_SNIPER_PAINT_SWEEP_TARGET:
		m_flWaitFinished = gpGlobals->curtime + m_hSweepTarget->m_flSpeed;
		
		// Snap directly to this target if this spawnflag is set.
		// Otherwise, sweep from wherever the cursor was.
		if( m_hSweepTarget->HasSpawnFlags( SF_SNIPERTARGET_SNAPTO ) )
		{
			m_vecPaintCursor = m_hSweepTarget->GetLocalOrigin();
		}

		LaserOn( m_hSweepTarget->GetLocalOrigin(), vec3_origin );
		break;

	case TASK_SNIPER_PAINT_ENEMY:
		// If the sniper has a sweep target, clear it, unless it's flagged to resume
		if( m_hSweepTarget != NULL )
		{
			if ( !m_hSweepTarget->HasSpawnFlags( SF_SNIPERTARGET_RESUME) )
			{
				ClearTargetGroup();
				m_hSweepTarget = NULL;
			}
		}

		if( m_spawnflags & SF_SNIPER_VIEWCONE )
		{
			m_flWaitFinished = gpGlobals->curtime + SNIPER_FOG_PAINT_ENEMY_TIME;

			// Just turn it on where it is.
			LaserOn( m_vecPaintCursor, vec3_origin );
		}
		else
		{
			m_flWaitFinished = gpGlobals->curtime + SNIPER_PAINT_ENEMY_TIME;

			Vector vecCursor;

			// Try to start the laser where the player can't miss seeing it!
			AngleVectors( GetEnemy()->GetLocalAngles(), &vecCursor );
			vecCursor = vecCursor * 300;
			vecCursor += GetEnemy()->EyePosition();				
			LaserOn( vecCursor, Vector( 16, 16, 16 ) );
		}

		// Scope glints if shooting at player.
		if( GetEnemy()->IsPlayer() )
		{
			CEffectData data;
			Vector vecNormal;

			vecNormal = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize( vecNormal );

			data.m_vOrigin = GetAbsOrigin();
			data.m_vNormal = vecNormal;
			data.m_vAngles = vec3_angle;

			DispatchEffect( "GunshipImpact", data );
		}

		break;

	case TASK_SNIPER_PAINT_NO_SHOT:
		m_flWaitFinished = gpGlobals->curtime + SNIPER_PAINT_NO_SHOT_TIME;
		FindFrustratedShot( pTask->flTaskData );
		LaserOff();
		LaserOn( m_vecFrustratedTarget, vec3_origin );
		break;

	case TASK_SNIPER_PAINT_FRUSTRATED:
		m_flPaintTime = SNIPER_PAINT_FRUSTRATED_TIME + random->RandomFloat( 0, SNIPER_PAINT_FRUSTRATED_TIME );
		m_flWaitFinished = gpGlobals->curtime + m_flPaintTime;
		FindFrustratedShot( pTask->flTaskData );
		LaserOff();
		LaserOn( m_vecFrustratedTarget, vec3_origin );
		break;

	case TASK_SNIPER_PAINT_DECOY:
		m_flWaitFinished = gpGlobals->curtime + pTask->flTaskData;
		LaserOn( m_vecDecoyObjectTarget, Vector( 64, 64, 64 ) );
		break;

	case TASK_RELOAD:
		{
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), CHAN_WEAPON, pProtoSniperSounds[ 3 ], 1.0, ATTN_NORM );
			m_fWeaponLoaded = true;
			TaskComplete();
		}
		break;

	case TASK_SNIPER_FRUSTRATED_ATTACK:
		//FindFrustratedShot();
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CProtoSniper::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SNIPER_ATTACK_CURSOR:
		if( FireBullet( m_vecPaintCursor, true ) )
		{
			TaskComplete();
		}
		break;

	case TASK_RANGE_ATTACK1:
		// Fire at enemy.
		if( FireBullet( LeadTarget( GetEnemy() ), true ) )
		{
			TaskComplete();
		}
		break;

	case TASK_SNIPER_FRUSTRATED_ATTACK:
		if( FireBullet( m_vecFrustratedTarget, false ) )
		{
			TaskComplete();
		}
		break;

	case TASK_SNIPER_PAINT_SWEEP_TARGET:
		if( gpGlobals->curtime > m_flWaitFinished )
		{
			// Time up! Paint the next target in the chain, or stop.
			CBaseEntity *pNext;
			pNext = gEntList.FindEntityByName( NULL, m_hSweepTarget->m_target, NULL );

			if ( m_hSweepTarget->HasSpawnFlags( SF_SNIPERTARGET_SHOOTME ) )
			{
				FireBullet( m_hSweepTarget->GetLocalOrigin(), false );
				TaskComplete(); // Force a reload.
			}

			if( pNext || IsSweepingRandomly() )
			{
				// Bump the timer up, update the cursor, paint the new target!
				// This is done regardless of whether we just fired at the current target.

				m_vecPaintCursor = m_hSweepTarget->GetLocalOrigin();
				if( IsSweepingRandomly() )
				{
					// If sweeping randomly, just pick another target.
					CBaseEntity *pOldTarget;

					pOldTarget = m_hSweepTarget;

					// Pick another target in the group. Don't shoot at the one we just shot at.
					do
					{
						m_hSweepTarget = m_pGroupTarget[ random->RandomInt( 0, m_iNumGroupTargets - 1 ) ];
					} while( m_hSweepTarget == pOldTarget );
				}
				else
				{
					// If not, go with the next target in the chain.
					m_hSweepTarget = pNext;
				}

				m_vecPaintStart = m_vecPaintCursor;
				m_flWaitFinished = gpGlobals->curtime + m_hSweepTarget->m_flSpeed;
			}
			else
			{
				m_hSweepTarget = NULL;
				LaserOff();
				TaskComplete();
			} 

#if 0
			NDebugOverlay::Line(GetBulletOrigin(), m_hSweepTarget->GetLocalOrigin(), 0,255,0, true, 20 );
#endif
		}
		else
		{
			if ( m_hSweepTarget->HasSpawnFlags( SF_SNIPERTARGET_SNAPSHOT ) )
			{
				m_fSnapShot = true;
			}

			PaintTarget( m_hSweepTarget->GetLocalOrigin(), m_hSweepTarget->m_flSpeed );
		}

		break;

	case TASK_SNIPER_PAINT_ENEMY:
		if( gpGlobals->curtime > m_flWaitFinished )
		{
			TaskComplete();
		}

		PaintTarget( LeadTarget( GetEnemy() ), SNIPER_PAINT_ENEMY_TIME );
		break;

	case TASK_SNIPER_PAINT_DECOY:
		if( gpGlobals->curtime > m_flWaitFinished )
		{
			TaskComplete();
		}

		PaintTarget( m_vecDecoyObjectTarget, pTask->flTaskData );
		break;

	case TASK_SNIPER_PAINT_NO_SHOT:
		if( gpGlobals->curtime > m_flWaitFinished )
		{
			//HACKHACK(sjb)
			// This condition should be turned off 
			// by a task.
			ClearCondition( COND_SNIPER_NO_SHOT );
			TaskComplete();
		}

		PaintTarget( m_vecFrustratedTarget, SNIPER_PAINT_NO_SHOT_TIME );
		break;

	case TASK_SNIPER_PAINT_FRUSTRATED:
		if( gpGlobals->curtime > m_flWaitFinished )
		{
			TaskComplete();
		}

		PaintTarget( m_vecFrustratedTarget, m_flPaintTime );
		break;

	case TASK_RANGE_ATTACK2:
		// Fire at decoy
		if( m_hDecoyObject == NULL )
		{
			TaskFail("sniper: bad decoy");
			break;
		}

		if( FireBullet( m_vecDecoyObjectTarget, false ) )
		{
			//Msg( "Fired at decoy\n" );
			AddOldDecoy( m_hDecoyObject );
			TaskComplete();
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// The sniper throws away the circular list of old decoys when we restore.
//-----------------------------------------------------------------------------
int CProtoSniper::Restore( IRestore &restore )
{
	ClearOldDecoys();

	return BaseClass::Restore( restore );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
float CProtoSniper::MaxYawSpeed( void )
{
	return 60;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CProtoSniper::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	// If a sweep target is set, keep asking the AI to sweep the target
	if( m_hSweepTarget != NULL && !HasCondition( COND_CAN_RANGE_ATTACK1 ) && !HasCondition( COND_SNIPER_NO_SHOT ) )
	{
		SetCondition( COND_SNIPER_SWEEP_TARGET );
	}
	else
	{
		ClearCondition( COND_SNIPER_SWEEP_TARGET );
	}

	// Think faster if the beam is on, this gives the beam higher resolution.
	if( m_pBeam )
	{
		SetNextThink( gpGlobals->curtime + 0.03 );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	// If the enemy has just stepped into view, or we've acquired a new enemy,
	// Record the last time we've seen the enemy as right now.
	//
	// If the enemy has been out of sight for a full second, mark him eluded.
	if( GetEnemy() != NULL )
	{
		if( gpGlobals->curtime - GetEnemies()->LastTimeSeen( GetEnemy() ) > 30 )
		{
			// Stop pestering enemies after 30 seconds of frustration.
			GetEnemies()->ClearMemory( GetEnemy() );
			SetEnemy(NULL);
		}
	}

	// Suppress at the sound of danger. Incoming missiles, for example.
	if( HasCondition( COND_HEAR_DANGER ) )
	{
		SetCondition( COND_SNIPER_SUPPRESSED );
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
Vector CProtoSniper::EyePosition( void )
{
	if( m_spawnflags & SF_SNIPER_HIDDEN )
	{
		return GetLocalOrigin();
	}
	else
	{
		return BaseClass::EyePosition();
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
Vector CProtoSniper::LeadTarget( CBaseEntity *pTarget )
{
	float targetTime;
	float targetDist;
	//float adjustedShotDist;
	//float actualShotDist;
	Vector vecAdjustedShot;
	Vector vecTarget;
	trace_t tr;

	if( pTarget == NULL )
	{
		// no target
		return vec3_origin;
	}

	// Get target
	if( pTarget->GetFlags() & FL_CLIENT )
	{
		vecTarget = pTarget->WorldSpaceCenter();
	}
	else
	{
		// Shoot NPCs in the head
		vecTarget = pTarget->EyePosition() - Vector( 0, 0, 8 );
	}

	// Get bullet time to target
	targetDist = (vecTarget - GetBulletOrigin() ).Length();
	targetTime = targetDist / bulletSpeed.GetFloat();
	
	// project target's velocity over that time. 
	Vector vecVelocity;

	if( pTarget->GetFlags() & FL_CLIENT || pTarget->Classify() == CLASS_MISSILE )
	{
		// This target is a client, who has an actual velocity.
		//vecVelocity = GetSmoothedVelocity();

		vecVelocity = pTarget->GetAbsVelocity();

		// Slow the vertical velocity down a lot, or the sniper will
		// lead a jumping player by firing several feet above his head.
		// THIS may affect the sniper hitting a player that's ascending/descending
		// ladders. If so, we'll have to check for the player's ladder flag.
		if( pTarget->GetFlags() & FL_CLIENT )
		{
			vecVelocity.z *= 0.25;
		}
	}
	else
	{
		// This target is an NPC. Have to build a velocity vector
		// using the character's current groundspeed.
		CBaseAnimating *pAnimating;

		pAnimating = (CBaseAnimating *)pTarget;

		Assert( pAnimating != NULL );

		QAngle vecAngle;
		vecAngle.y = pAnimating->GetSequenceMoveYaw( pAnimating->GetSequence() );
		vecAngle.x = 0;
		vecAngle.z = 0;

		vecAngle.y += pTarget->GetLocalAngles().y;

		AngleVectors( vecAngle, &vecVelocity );

		vecVelocity = vecVelocity * pAnimating->m_flGroundSpeed;
	}

	if( m_iMisses > 0 && !FClassnameIs( pTarget, "npc_bullseye" ) )
	{
		// I'm supposed to miss this shot, so aim above the target's head.
		// BUT DON'T miss bullseyes, and don't count the shot.
		vecAdjustedShot = vecTarget; 
		vecAdjustedShot.z += 16;

		m_iMisses--;

		return vecAdjustedShot;
	}

	vecAdjustedShot = vecTarget + ( vecVelocity * targetTime );

	// if the adjusted shot falls well short of the target, take the straight shot.
	// it's not very interesting for the bullet to hit something far away from the 
	// target. (for instance, if a sign or ledge or something is between the player
	// and the sniper, and the sniper would hit this object if he tries to lead the player)
	Vector vecBulletOrigin;
	vecBulletOrigin = GetBulletOrigin();


	if( sniperLines.GetFloat() == 1.0f )
	{
		CPVSFilter filter( GetLocalOrigin() );
		te->ShowLine( filter, 0.0, &vecBulletOrigin, &vecAdjustedShot );
	}



/*
	UTIL_TraceLine( vecBulletOrigin, vecAdjustedShot, MASK_SHOT, this, &tr );

	actualShotDist = (tr.endpos - vecBulletOrigin ).Length();
	adjustedShotDist = ( vecAdjustedShot - vecBulletOrigin ).Length();

	/////////////////////////////////////////////
	// the shot taken should hit within 10% of the sniper's distance to projected target.
	// else, shoot straight. (there's some object in the way of the adjusted shot)
	/////////////////////////////////////////////
	if( actualShotDist <= adjustedShotDist * 0.9 )
	{
		vecAdjustedShot = vecTarget;
	}
*/
	return vecAdjustedShot;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CProtoSniper::InputEnableSniper( inputdata_t &inputdata )
{
	ClearCondition( COND_SNIPER_DISABLED );
	SetCondition( COND_SNIPER_ENABLED );

	m_fEnabled = true;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CProtoSniper::InputDisableSniper( inputdata_t &inputdata )
{
	ClearCondition( COND_SNIPER_ENABLED );
	SetCondition( COND_SNIPER_DISABLED );

	m_fEnabled = false;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CProtoSniper::FindFrustratedShot( float flNoise )
{
	Vector vecForward;
	Vector vecStart;
	Vector vecAimAt;
	Vector vecAim;

#ifdef SNIPER_DEBUG
	Msg( "Frustrated!\n" );
#endif //SNIPER_DEBUG

	// Just pick a spot somewhere around the target.
	// Try a handful of times to pick a spot that guarantees the 
	// target will see the laser.
#define MAX_TRIES	15
	for( int i = 0 ; i < MAX_TRIES ; i++ )
	{
		Vector vecSpot = GetEnemyLKP();

		vecSpot.x += random->RandomFloat( -64, 64 );
		vecSpot.y += random->RandomFloat( -64, 64 );
		vecSpot.z += random->RandomFloat( -40, 40 );

		// Help move the frustrated spot off the target's BBOX in X/Y space.
		if( vecSpot.x < 0 )
			vecSpot.x -= 32;
		else
			vecSpot.x += 32;

		if( vecSpot.y < 0 )
			vecSpot.y -= 32;
		else
			vecSpot.y += 32;

		Vector vecSrc, vecDir;

		vecSrc = GetAbsOrigin(); 
		vecDir = vecSpot - vecSrc;
		VectorNormalize( vecDir );

		if( GetEnemy()->FVisible( vecSpot ) || i == MAX_TRIES - 1 )
		{
			trace_t tr;
			AI_TraceLine(vecSrc, vecSrc + vecDir * 8192, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

			if( !GetEnemy()->FVisible( tr.endpos ) )
			{
				// Dont accept this point unless we are out of tries!
				if( i != MAX_TRIES - 1 )
				{
					continue;
				}
/*
				else
				{
					Msg("Taking sub-optimal spot.\n");
				}
*/
			}
/*
			else
			{
				Msg("Found spot in %d tries.\n", i );
			}
*/
			m_vecFrustratedTarget = tr.endpos;
			break;
		}
	}


/*
	// Project the target's line of sight. Fire a shot
	// to intersect that.
	if( GetEnemy()->GetFlags() & FL_CLIENT )
	{
		// Use viewangle for clients
		CBasePlayer *player = ( CBasePlayer * )GetEnemy();
		player->EyeVectors( &vecForward );
	}
	else
	{
		// Just angles for NPCs
		AngleVectors( GetEnemy()->GetLocalAngles(), &vecForward );
	}

	vecStart = GetEnemy()->EyePosition() + Vector( 0, 0, 48 );
	vecAimAt = vecStart + vecForward * 64;

	vecStart = GetBulletOrigin();

	vecAim = vecAimAt - vecStart;
	VectorNormalize(vecAim);

	if( flNoise != 0 )
	{
		vecAim.x += random->RandomFloat( -flNoise, flNoise );
		vecAim.y += random->RandomFloat( -flNoise, flNoise );
	}

	trace_t tr;
	UTIL_TraceLine( vecStart, vecStart + vecAim * 8192, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
*/

#if 0
	NDebugOverlay::Line(vecStart, tr.endpos, 0,255,0, true, 20 );
#endif

}


//---------------------------------------------------------
// See all NPC's easily.
//
// Only see the player if you can trace to both of his 
// eyeballs. That is, allow the player to peek around corners.
// This is a little more expensive than the base class' check!
//---------------------------------------------------------
#define SNIPER_EYE_DIST 0.75
bool CProtoSniper::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	if( m_spawnflags & SF_SNIPER_VIEWCONE )
	{
		// Viewcone snipers are blind with their laser off.
		if( !IsLaserOn() )
		{
			return false;
		}
	}

	if( !pEntity->IsPlayer() )
	{
		// NPC
		return BaseClass::FVisible( pEntity, traceMask, ppBlocker );
	}

	if ( pEntity->GetFlags() & FL_NOTARGET )
	{
		return false;
	}

	Vector	vecRight;
	Vector	vecEye;
	trace_t	tr;

	AngleVectors( pEntity->GetLocalAngles(), NULL, &vecRight, NULL );

	vecEye = vecRight * SNIPER_EYE_DIST - Vector( 0, 0, 5 );
	UTIL_TraceLine( EyePosition(), pEntity->EyePosition() + vecEye, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );

#if 0
	NDebugOverlay::Line(EyePosition(), tr.endpos, 0,255,0, true, 0.1);
#endif

	bool fCheckFailed = false;

	if( tr.fraction != 1.0 )
	{
		fCheckFailed = true;
	}

	// Don't check the other eye if the first eye failed.
	if( !fCheckFailed )
	{
		vecEye = -vecRight * SNIPER_EYE_DIST - Vector( 0, 0, 5 );
		UTIL_TraceLine( EyePosition(), pEntity->EyePosition() + vecEye, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );

#if 0
		NDebugOverlay::Line(EyePosition(), tr.endpos, 0,255,0, true, 0.1);
#endif 

		if( tr.fraction != 1.0 )
		{
			fCheckFailed = true;
		}
	}

	if( !fCheckFailed )
	{
		// Can see the player.
		return true;
	}
	
	// Now, if the check failed, see if the player is ducking and has recently
	// fired a muzzleflash. If yes, see if you'd be able to see the player if 
	// they were standing in their current position instead of ducking. Since
	// the sniper doesn't have a clear shot in this situation, he will harrass
	// near the player.
	CBasePlayer *pPlayer;

	pPlayer = ToBasePlayer( pEntity );

	if( (pPlayer->GetFlags() & FL_DUCKING) && pPlayer->MuzzleFlashTime() > gpGlobals->curtime )
	{
		vecEye = pPlayer->EyePosition() + Vector( 0, 0, 32 );
		UTIL_TraceLine( EyePosition(), vecEye, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );

		if( tr.fraction != 1.0 )
		{
			// Everything failed.
			if (ppBlocker)
			{
				*ppBlocker = tr.m_pEnt;
			}
			return false;
		}
		else
		{
			// Fake being able to see the player.
			return true;
		}
	}

	if (ppBlocker)
	{
		*ppBlocker = tr.m_pEnt;
	}

	return false;
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( proto_sniper, CProtoSniper )

	DECLARE_CONDITION( COND_SNIPER_CANATTACKDECOY );
	DECLARE_CONDITION( COND_SNIPER_SUPPRESSED );
	DECLARE_CONDITION( COND_SNIPER_ENABLED );
	DECLARE_CONDITION( COND_SNIPER_DISABLED );
	DECLARE_CONDITION( COND_SNIPER_FRUSTRATED );
	DECLARE_CONDITION( COND_SNIPER_SWEEP_TARGET );	
	DECLARE_CONDITION( COND_SNIPER_NO_SHOT );	

	DECLARE_TASK( TASK_SNIPER_FRUSTRATED_ATTACK );
	DECLARE_TASK( TASK_SNIPER_PAINT_ENEMY );
	DECLARE_TASK( TASK_SNIPER_PAINT_DECOY );
	DECLARE_TASK( TASK_SNIPER_PAINT_FRUSTRATED );
	DECLARE_TASK( TASK_SNIPER_PAINT_SWEEP_TARGET );
	DECLARE_TASK( TASK_SNIPER_ATTACK_CURSOR );
	DECLARE_TASK( TASK_SNIPER_PAINT_NO_SHOT );

	//=========================================================
	// SCAN
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_SCAN,

		"	Tasks"
		"		TASK_WAIT_INDEFINITE		0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_HEAR_DANGER"
		"		COND_SNIPER_DISABLED"
		"		COND_SNIPER_SWEEP_TARGET"
	)

	//=========================================================
	// CAMP
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_CAMP,

		"	Tasks"
		"		TASK_WAIT_INDEFINITE		0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_SNIPER_CANATTACKDECOY"
		"		COND_SNIPER_SUPPRESSED"
		"		COND_HEAR_DANGER"
		"		COND_SNIPER_DISABLED"
		"		COND_SNIPER_FRUSTRATED"
		"		COND_SNIPER_SWEEP_TARGET"
	)

	//=========================================================
	// ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_ATTACK,

		"	Tasks"
		"		TASK_SNIPER_PAINT_ENEMY		0"
		"		TASK_RANGE_ATTACK1			0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_OCCLUDED"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
		"		COND_SNIPER_DISABLED"
	)
		
	//=========================================================
	// ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_SNAPATTACK,

		"	Tasks"
		"		TASK_SNIPER_ATTACK_CURSOR	0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_OCCLUDED"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
		"		COND_SNIPER_DISABLED"
	)

	//=========================================================
	// RELOAD
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_RELOAD,

		"	Tasks"
		"		TASK_RELOAD				0"
		"		TASK_WAIT				2"
		"	"
		"	Interrupts"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// Attack decoy
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_ATTACKDECOY,

		"	Tasks"
		"		TASK_SNIPER_PAINT_DECOY		2.0"
		"		TASK_RANGE_ATTACK2			0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_HEAR_DANGER"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_SNIPER_DISABLED"
		"		COND_SNIPER_SWEEP_TARGET"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_SUPPRESSED,

		"	Tasks"
		"		TASK_WAIT			2.0"
		"	"
		"	Interrupts"
	)

	//=========================================================
	// Sniper is allowed to process a couple conditions while
	// disabled, but mostly he waits until he's enabled.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_DISABLEDWAIT,

		"	Tasks"
		"		TASK_WAIT			0.5"
		"	"
		"	Interrupts"
		"		COND_SNIPER_ENABLED"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_FRUSTRATED_ATTACK,

		"	Tasks"
		"		TASK_WAIT						2.0"
		"		TASK_SNIPER_PAINT_FRUSTRATED	0.05"
		"		TASK_SNIPER_PAINT_FRUSTRATED	0.025"
		"		TASK_SNIPER_PAINT_FRUSTRATED	0.0"
		"		TASK_SNIPER_FRUSTRATED_ATTACK	0.0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_SNIPER_DISABLED"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_SEE_ENEMY"
		"		COND_HEAR_DANGER"
		"		COND_SNIPER_SWEEP_TARGET"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_SWEEP_TARGET,

		"	Tasks"
		"		TASK_SNIPER_PAINT_SWEEP_TARGET	0.0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SNIPER_DISABLED"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_HEAR_DANGER"
		"		COND_SNIPER_NO_SHOT"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_SWEEP_TARGET_NOINTERRUPT,

		"	Tasks"
		"		TASK_SNIPER_PAINT_SWEEP_TARGET	0.0"
		"	"
		"	Interrupts"
		"		COND_SNIPER_DISABLED"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_PSNIPER_NO_CLEAR_SHOT,

		"	Tasks"
		"		TASK_SNIPER_PAINT_NO_SHOT	0.0"
		"		TASK_SNIPER_PAINT_NO_SHOT	0.075"
		"		TASK_SNIPER_PAINT_NO_SHOT	0.05"
		"		TASK_SNIPER_PAINT_NO_SHOT	0.0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_SNIPER_DISABLED"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_HEAR_DANGER"
	)

AI_END_CUSTOM_NPC()

//-----------------------------------------------------------------------------
//
// Sniper Bullet
//
//-----------------------------------------------------------------------------


//---------------------------------------------------------
//---------------------------------------------------------
void CSniperBullet::Precache()
{
}


//---------------------------------------------------------
//---------------------------------------------------------
void CSniperBullet::Think( void )
{
	// Set the bullet up to think again.
	SetNextThink( gpGlobals->curtime + 0.05 );

	if( gpGlobals->curtime >= m_SoundTime )
	{
		// See if it's time to make the sonic boom.
		CPASAttenuationFilter filter( this, ATTN_NONE );
		EmitSound( filter, entindex(), CHAN_WEAPON, pProtoSniperSounds[ 0 ], 1.0, ATTN_NONE );

		if( GetOwnerEntity() )
		{
			CAI_BaseNPC *pSniper;
			CAI_BaseNPC *pEnemyNPC;
			pSniper = GetOwnerEntity()->MyNPCPointer();

			if( pSniper && pSniper->GetEnemy() )
			{
				pEnemyNPC = pSniper->GetEnemy()->MyNPCPointer();

				// Warn my enemy if they can see me.
				if( pEnemyNPC && pEnemyNPC->FInViewCone( pSniper ) && pEnemyNPC->FVisible( pSniper ) )
				{
					CSoundEnt::InsertSound( SOUND_DANGER, pSniper->GetEnemy()->GetAbsOrigin(), 80, 0.2f, this );
				}
			}
		}

		// No way the bullet will live this long.
		m_SoundTime = 1e9;
	}

	// Trace this timeslice of the bullet.
	Vector vecStart;
	Vector vecEnd;
	float flInterval;

	flInterval = gpGlobals->curtime - GetLastThink();
	vecStart = GetAbsOrigin();
	vecEnd = vecStart + ( m_vecDir * (bulletSpeed.GetFloat() * flInterval) );
	float flDist = (vecStart - vecEnd).Length();

	//Msg(".");

	trace_t tr;
	AI_TraceLine( vecStart, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	if( tr.fraction != 1.0 )
	{
		// This slice of bullet will hit something.
		FireBullets( 1, vecStart, m_vecDir, vec3_origin, flDist, m_AmmoType, 0 );
		m_iImpacts++;

		if( tr.m_pEnt->m_takedamage == DAMAGE_YES || m_iImpacts == NUM_PENETRATIONS )
		{
			// Bullet stops when it hits an NPC, or when it's penetrated enough times.
			//Msg("*\n");
			Stop();
			return;
		}
		else
		{
			#define STEP_SIZE	2
			#define NUM_STEPS	6
			// Try to slide a 'cursor' through the object that was hit. 
			Vector vecCursor = tr.endpos;
			
			for( int i = 0 ; i < NUM_STEPS ; i++ )
			{
				//Msg("-");
				vecCursor += m_vecDir * STEP_SIZE;

				if( UTIL_PointContents( vecCursor ) != CONTENTS_SOLID )
				{
					// Passed out of a solid! 
					SetAbsOrigin( vecCursor );

					// Fire another tracer.
					AI_TraceLine( vecCursor, vecCursor + m_vecDir * 8192, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
					UTIL_Tracer( vecCursor, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, bulletSpeed.GetFloat(), true, "StriderTracer" );
					return;
				}
			}

			// Bullet also stops when it fails to exit material after penetrating this far.
			//Msg("#\n");
			Stop();
			return;
		}
	}
	else
	{
		SetAbsOrigin( vecEnd );
	}
}


//=========================================================
//=========================================================
bool CSniperBullet::Start( const Vector &vecOrigin, const Vector &vecTarget )
{
	m_flLastThink = gpGlobals->curtime;

	if( m_AmmoType == -1 )
	{
		// This guy doesn't have a REAL weapon, per say, but he does fire
		// sniper rounds. Since there's no weapon to index the ammo type,
		// do it manually here.
		m_AmmoType = GetAmmoDef()->Index("SniperRound"); 

		// This is the bullet that is used for all subsequent FireBullets() calls after the first
		// call penetrates a surface and keeps going.
		m_PenetratedAmmoType = GetAmmoDef()->Index("SniperPenetratedRound");
	}
	
	if( m_fActive )
	{
		return false;
	}

	UTIL_SetOrigin( this, vecOrigin );

	m_vecDir = vecTarget - vecOrigin;
	VectorNormalize( m_vecDir );

	// Start the tracer here, and tell it to end at the end of the last trace
	// the trace comes from the loop above that does penetration.
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + m_vecDir * 8192, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	UTIL_Tracer( vecOrigin, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, bulletSpeed.GetFloat(), true, "StriderTracer" );

	float flElapsedTime = ( (tr.startpos - tr.endpos).Length() / bulletSpeed.GetFloat() );
	m_SoundTime = gpGlobals->curtime + flElapsedTime * 0.5;
	
	SetThink( Think );
	SetNextThink( gpGlobals->curtime );
	m_fActive = true;
	return true;

/*
	int i;

	// Try to find all of the things the bullet can go through along the way.
	//-------------------------------
	//-------------------------------
	m_vecDir = vecTarget - vecOrigin;
	VectorNormalize( m_vecDir );

	trace_t	tr;

	
	// Elapsed time counts how long the bullet is in motion through this simulation.
	float flElapsedTime = 0;

	for( i = 0 ; i < NUM_PENETRATIONS ; i++ )
	{
		// Trace to the target. 
		UTIL_TraceLine( GetAbsOrigin(), vecTarget, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		flShotDist = (tr.endpos - GetAbsOrigin()).Length();

		// Record the two endpoints of the segment and the time at which this bullet hits,
		// and the time at which it's supposed to hit its mark.
		m_ImpactTime[ i ] = flElapsedTime + ( flShotDist / bulletSpeed.GetFloat() );
		m_vecStart[ i ] = tr.startpos;
		m_vecEnd[ i ] = tr.endpos;

		// The elapsed time is now pushed forward by how long it takes the bullet
		// to travel through this segment.
		flElapsedTime += ( flShotDist / bulletSpeed.GetFloat() );

		// Never let gpGlobals->curtime get added to the elapsed time!
		m_ImpactTime[ i ] += gpGlobals->curtime;

		CBaseEntity *pEnt;

		pEnt = tr.m_pEnt;

		if( !pEnt												||
			pEnt->MyNPCPointer()								|| 
			UTIL_DistApprox2D( tr.endpos, vecTarget ) <= 4		||
			FClassnameIs( pEnt, "prop_physics" ) )
		{
			// If we're close to the target, assume the shot is going to hit
			// the target and stop penetrating.
			//
			// If we're going to hit an NPC, stop penetrating.
			//
			// If we hit a physics prop, stop penetrating.
			//
			// Otherwise, keep looping.
			break;
		}

		// We're going to try to penetrate whatever the bullet has hit. 

		// Push through the object by the penetration distance, then trace back.
		Vector vecCursor;

		vecCursor = tr.endpos;
		vecCursor += m_vecDir * PENETRATION_THICKNESS;

		UTIL_TraceLine( vecCursor, vecCursor + m_vecDir * -2, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

#if 1
		if( tr.startsolid )
		{
			// The cursor is inside the solid. Solid is too thick to penetrate.
#ifdef SNIPER_DEBUG
			Msg( "SNIPER STARTSOLID\n" );
#endif
			break;
		}
#endif
		
		// Now put the bullet at this point and continue.
		UTIL_SetOrigin( this, vecCursor );
	}
	//-------------------------------
	//-------------------------------
*/	
	

/*
#ifdef SNIPER_DEBUG
	Msg( "PENETRATING %d items", i );
#endif // SNIPER_DEBUG

#ifdef SNIPER_DEBUG
	Msg( "Dist: %f Travel Time: %f\n", flShotDist, m_ImpactTime );
#endif // SNIPER_DEBUG
*/
}


//---------------------------------------------------------
//---------------------------------------------------------
void CSniperBullet::Init( void )
{
#ifdef SNIPER_DEBUG
	Msg( "Bullet stopped\n" );
#endif // SNIPER_DEBUG

	m_fActive = false;
	m_vecDir.Init(); 
	m_AmmoType = -1; 
	m_SoundTime = 1e9;
	m_iImpacts = 0;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CSniperBullet::Stop( void )
{
	UTIL_Remove(this);
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CSniperTarget::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "groupname"))
	{
		Msg( "Got my group name!\n" );
		m_iszGroupName = AllocPooledString( szValue );
		return true;
	}
	else
	{
		return CPointEntity::KeyValue( szKeyName, szValue );
	}
}

LINK_ENTITY_TO_CLASS( info_snipertarget, CSniperTarget );


