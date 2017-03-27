//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Implements the zombie, a horrific once-human headcrab victim.
//
// The zombie has two main states: Full and Torso.
//
// In Full state, the zombie is whole and walks upright as he did in Half-Life.
// He will try to claw the player and swat physics items at him. 
//
// In Torso state, the zombie has been blasted or cut in half, and the Torso will
// drag itself along the ground with its arms. It will try to claw the player.
//
// In either state, a severely injured Zombie will release its headcrab, which
// will immediately go after the player. The Zombie will then die (ragdoll). 
//
//=============================================================================

#include "cbase.h"
#include "NPC_BaseZombie.h"
#include "Player.h"
#include "game.h"
#include "AI_Network.h"
#include "AI_Navigator.h"
#include "AI_Motor.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Node.h"
#include "AI_Memory.h"
#include "AI_Senses.h"
#include "BitString.h"
#include "EntityFlame.h"
#include "hl2_shareddefs.h"
#include "NPCEvent.h"
#include "Activitylist.h"
#include "entitylist.h"
#include "gib.h"
#include "soundenvelope.h"
#include "ndebugoverlay.h"
#include "rope.h"
#include "rope_shared.h"
#include "igamesystem.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

envelopePoint_t envDefaultZombieMoanVolumeFast[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		0.2f, 0.3f,
	},
};

envelopePoint_t envDefaultZombieMoanVolume[] =
{
	{	1.0f, 0.1f,
		0.1f, 0.1f,
	},
	{	1.0f, 1.0f,
		0.2f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.3f, 0.4f,
	},
};


// if the zombie doesn't find anything closer than this, it doesn't swat.
#define ZOMBIE_FARTHEST_PHYSICS_OBJECT	2000

// Don't swat objects unless player is closer than this.
#define ZOMBIE_PLAYER_MAX_SWAT_DIST		1000

//
// How much health a Zombie torso gets when a whole zombie is broken
// It's whole zombie's MAX Health * this value
#define ZOMBIE_TORSO_HEALTH_FACTOR 0.5

//
// When the zombie has health < m_iMaxHealth * this value, it will
// try to release its headcrab.
#define ZOMBIE_RELEASE_HEALTH_FACTOR	0.5

//
// The heaviest physics object that a zombie should try to swat. (kg)
#define ZOMBIE_MAX_PHYSOBJ_MASS		60

//
// Zombie tries to get this close to a physics object's origin to swat it
#define ZOMBIE_PHYSOBJ_SWATDIST		80

//
// Because movement code sometimes doesn't get us QUITE where we
// want to go, the zombie tries to get this close to a physics object
// Zombie will end up somewhere between PHYSOBJ_MOVE_TO_DIST & PHYSOBJ_SWATDIST
#define ZOMBIE_PHYSOBJ_MOVE_TO_DIST	48

//
// How long between physics swat attacks (in seconds). 
#define ZOMBIE_SWAT_DELAY			5


//
// After taking damage, ignore further damage for n seconds. This keeps the zombie
// from being interrupted while.
//
#define ZOMBIE_FLINCH_DELAY			3


//=========================================================
// private activities
//=========================================================
int CNPC_BaseZombie::ACT_ZOM_SWATLEFTMID;
int CNPC_BaseZombie::ACT_ZOM_SWATRIGHTMID;
int CNPC_BaseZombie::ACT_ZOM_SWATLEFTLOW;
int CNPC_BaseZombie::ACT_ZOM_SWATRIGHTLOW;
int CNPC_BaseZombie::ACT_ZOM_RELEASECRAB;
int CNPC_BaseZombie::ACT_ZOM_FALL;

ConVar	sk_zombie_dmg_one_slash( "sk_zombie_dmg_one_slash","0");
ConVar	sk_zombie_dmg_both_slash( "sk_zombie_dmg_both_slash","0");


// When a zombie spawns, he will select a 'base' pitch value
// that's somewhere between basepitchmin & basepitchmax
ConVar zombie_basemin( "zombie_basemin", "100" );
ConVar zombie_basemax( "zombie_basemax", "100" );

ConVar zombie_changemin( "zombie_changemin", "0" );
ConVar zombie_changemax( "zombie_changemax", "0" );

// play a sound once in every zombie_stepfreq steps
ConVar zombie_stepfreq( "zombie_stepfreq", "4" );

ConVar zombie_decaymin( "zombie_decaymin", "0.1" );
ConVar zombie_decaymax( "zombie_decaymax", "0.4" );

ConVar zombie_ambushdist( "zombie_ambushdist", "16000" );

//=========================================================
// For a couple of reasons, we keep a running count of how
// many zombies in the world are angry at any given time.
//=========================================================
static s_iAngryZombies = 0;

//=========================================================
//=========================================================
class CAngryZombieCounter : public CAutoGameSystem
{
public:
	// Level init, shutdown
	virtual void LevelInitPreEntity()
	{
		s_iAngryZombies = 0;
	}
};

CAngryZombieCounter	AngryZombieCounter;


//=========================================================
//=========================================================
BEGIN_DATADESC( CNPC_BaseZombie )

	DEFINE_SOUNDPATCH( CNPC_BaseZombie, m_pMoanSound ),
	DEFINE_FIELD( CNPC_BaseZombie, m_fIsTorso, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_BaseZombie, m_fIsHeadless, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_BaseZombie, m_flNextFlinch, FIELD_TIME ),
	DEFINE_FIELD( CNPC_BaseZombie, m_bHeadShot, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_BaseZombie, m_flBurnDamage, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_BaseZombie, m_flBurnDamageResetTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_BaseZombie, m_hPhysicsEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_BaseZombie, m_flNextMoanSound, FIELD_TIME ),
	DEFINE_FIELD( CNPC_BaseZombie, m_flNextSwat, FIELD_TIME ),
	DEFINE_FIELD( CNPC_BaseZombie, m_flNextSwatScan, FIELD_TIME ),
	DEFINE_FIELD( CNPC_BaseZombie, m_crabHealth, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_BaseZombie, m_flMoanPitch, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_BaseZombie, m_iMoanSound, FIELD_INTEGER ),

END_DATADESC()


//LINK_ENTITY_TO_CLASS( base_zombie, CNPC_BaseZombie );

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::g_numZombies = 0;


//---------------------------------------------------------
//---------------------------------------------------------
CNPC_BaseZombie::CNPC_BaseZombie()
{
	// Gotta select which sound we're going to play, right here!
	// Because everyone's constructed before they spawn.
	//
	// Assign moan sounds in order, over and over.
	// This means if 3 or so zombies spawn near each
	// other, they will definitely not pick the same
	// moan loop.
	m_iMoanSound = g_numZombies;

	g_numZombies++;
}


//---------------------------------------------------------
//---------------------------------------------------------
CNPC_BaseZombie::~CNPC_BaseZombie()
{
	g_numZombies--;
}


//---------------------------------------------------------
// The closest physics object is chosen that is:
// <= MaxMass in Mass
// Between the zombie and the enemy
// not too far from a direct line to the enemy.
//---------------------------------------------------------
bool CNPC_BaseZombie::FindNearestPhysicsObject( int iMaxMass )
{
	CBaseEntity		*pList[ 100 ];
	CBaseEntity		*pNearest = NULL;
	float			flNearestDist = ZOMBIE_FARTHEST_PHYSICS_OBJECT;
	float			flDist;
	IPhysicsObject	*pPhysObj;
	int				i;
	Vector			vecDirToEnemy;
	Vector			vecDirToObject;

	if (GetEnemy() == NULL)
	{
		// No enemy, no swat.
		m_hPhysicsEnt = NULL;
		return false;
	}

	vecDirToEnemy = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
	float dist = VectorNormalize(vecDirToEnemy);
	vecDirToEnemy.z = 0;

	if( dist > ZOMBIE_PLAYER_MAX_SWAT_DIST )
	{
		// Player is too far away. Don't bother 
		// trying to swat anything at them until
		// they are closer.
		return false;
	}

	Vector vecDelta( dist, dist, dist );

	int count = UTIL_EntitiesInBox( pList, 100, GetAbsOrigin() - vecDelta, GetAbsOrigin() + vecDelta, 0 );

	for( i = 0 ; i < count ; i++ )
	{
		pPhysObj = pList[ i ]->VPhysicsGetObject();

		if( !pPhysObj || pPhysObj->GetMass() > iMaxMass || !pPhysObj->IsAsleep() )
		{
			continue;
		}

		Vector center = pList[ i ]->WorldSpaceCenter();
		flDist = UTIL_DistApprox2D( GetAbsOrigin(), center );

		if( flDist < flNearestDist )
		{
			// This object is closer... but is it between the player and the zombie?
			vecDirToObject = pList[ i ]->WorldSpaceCenter() - GetAbsOrigin();
			VectorNormalize(vecDirToObject);
			vecDirToObject.z = 0;

			if( DotProduct( vecDirToEnemy, vecDirToObject ) >= 0.8 )
			{
				if( UTIL_DistApprox2D( GetAbsOrigin(), center ) < UTIL_DistApprox2D( center, GetEnemy()->GetAbsOrigin() ) )
				{
					float down = GetAbsMins().z * 0.75 + GetAbsMaxs().z * 0.25;
					if ( pList[i]->GetAbsMaxs().z < down )
					{
						// don't swat things where the highest point is under my knees
						continue;
					}

					if( center.z > EyePosition().z )
					{
						// don't swat things that are over my head.
						continue;
					}

					vcollide_t *pCollide = modelinfo->GetVCollide( pList[i]->GetModelIndex() );
					
					Vector objMins, objMaxs;
					physcollision->CollideGetAABB( objMins, objMaxs, pCollide->solids[0], pList[i]->GetAbsOrigin(), pList[i]->GetAbsAngles() );

					if ( objMaxs.z < down )
						continue;

					if ( !FVisible( pList[i] ) )
						continue;

					// Make this the last check, since it makes a string.
					// Don't swat server ragdolls!
					if ( FClassnameIs( pList[ i ], "physics_prop_ragdoll" ) )
					{
						continue;
					}
						
					if ( FClassnameIs( pList[ i ], "prop_ragdoll" ) )
					{
						continue;
					}

					// The object must also be closer to the zombie than it is to the enemy
					pNearest = pList[ i ];
					flNearestDist = flDist;
				}
			}
		}
	}

	m_hPhysicsEnt = pNearest;

	if( m_hPhysicsEnt == NULL )
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
bool CNPC_BaseZombie :: FValidateHintType ( CAI_Hint *pHint )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns this monster's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_BaseZombie::Classify( void )
{
	return( CLASS_ZOMBIE ); 
}


//-----------------------------------------------------------------------------
// Purpose: Returns the maximum yaw speed based on the monster's current activity.
//-----------------------------------------------------------------------------
float CNPC_BaseZombie::MaxYawSpeed( void )
{
	if( m_fIsTorso )
	{
		return( 60 );
	}
	else if (IsMoving() && HasPoseParameter( GetSequence(), "move_yaw" ))
	{
		return( 15 );
	}
	else
	{
		return( 120 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	if (!HasPoseParameter( GetSequence(), "move_yaw" ))
	{
		return BaseClass::OverrideMoveFacing( move, flInterval );
	}

	// required movement direction
	float flMoveYaw = UTIL_VecToYaw( move.dir );
	float idealYaw = UTIL_AngleMod( flMoveYaw );

	if (GetEnemy())
	{
		float flEDist = UTIL_DistApprox2D( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter() );

		if (flEDist < 256.0)
		{
			float flEYaw = UTIL_VecToYaw( GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() );

			if (flEDist < 128.0)
			{
				idealYaw = flEYaw;
			}
			else
			{
				idealYaw = flMoveYaw + UTIL_AngleDiff( flEYaw, flMoveYaw ) * (2 - flEDist / 128.0);
			}

			//Msg("was %.0f now %.0f\n", flMoveYaw, idealYaw );
		}
	}

	GetMotor()->SetIdealYawAndUpdate( idealYaw );

	// find movement direction to compensate for not being turned far enough
	float fSequenceMoveYaw = GetSequenceMoveYaw( GetSequence() );
	float flDiff = UTIL_AngleDiff( flMoveYaw, GetLocalAngles().y + fSequenceMoveYaw );
	SetPoseParameter( "move_yaw", GetPoseParameter( "move_yaw" ) + flDiff );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BaseZombie::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > ZOMBIE_MELEE_REACH )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	BaseClass::TraceAttack( info, vecDir, ptr );

	// Keep track of headshots so we can determine whether to pop off our headcrab.
	if (ptr->hitgroup == HITGROUP_HEAD)
	{
		m_bHeadShot = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: A zombie has taken damage. Determine whether he should split in half
// Input  : 
// Output : bool, true if yes.
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold )
{
	if( m_fIsTorso )
	{
		// Already split.
		return false;
	}

	// Break in half IF:
	// 
	// Take half or more of max health in DMG_BLAST
	if( (info.GetDamageType() & DMG_BLAST) && flDamageThreshold >= 0.5 )
	{
		return true;
	}

#if 0
	if( info.GetDamageType() & DMG_BUCKSHOT )
	{
		if( m_iHealth <= 0 || flDamageThreshold >= 0.5 )
		{
			return true;
		}
	}
#endif 
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: A zombie has taken damage. Determine whether he release his headcrab.
// Output : YES, IMMEDIATE, or SCHEDULED (see HeadcrabRelease_t)
//-----------------------------------------------------------------------------
HeadcrabRelease_t CNPC_BaseZombie::ShouldReleaseHeadcrab( const CTakeDamageInfo &info, float flDamageThreshold )
{
	if ( m_iHealth <= 0 )
	{
		// If I was killed by a bullet, and wasn't shot in the head, release the crab. 
		if ( (info.GetDamageType() & DMG_BULLET) && (!m_bHeadShot) )
		{
			return RELEASE_IMMEDIATE;
		}

		// If I was killed by an explosion, release the crab.
		if ( info.GetDamageType() & DMG_BLAST )
		{
			return RELEASE_IMMEDIATE;
		}
	}

	return RELEASE_NO;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_BaseZombie::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	// Take 30% damage from bullets, take full buckshot damage
	if ( (info.GetDamageType() & DMG_BULLET) && !(info.GetDamageType() & DMG_BUCKSHOT) )
	{
		if( !(info.GetDamageType() & DMG_SNIPER) )
		{
			// Zombie is resistant to bullets, but not sniper bullets!
			info.ScaleDamage( 0.3 );
		}
	}

	if ( ShouldIgnite( info ) )
	{
		Ignite( 100.0f );
	}

	int tookDamage = BaseClass::OnTakeDamage_Alive( info );

	// flDamageThreshold is what percentage of the creature's max health
	// this amount of damage represents. (clips at 1.0)
	float flDamageThreshold = min( 1, info.GetDamage() / m_iMaxHealth );
	
	// Being chopped up by a sharp physics object is a pretty special case
	// so we handle it with some special code. Mainly for 
	// Ravenholm's helicopter traps right now (sjb).
	if( IsChopped( info ) )
	{
		DieChopped( info );
	}
	else
	{
		if( ShouldBecomeTorso( info, flDamageThreshold ) )
		{
			BecomeTorso( vec3_origin, vec3_origin );
			if (flDamageThreshold >= 1.0)
			{
				Vector forceVector( vec3_origin );
				forceVector += CalcDamageForceVector( info );
				BecomeRagdollOnClient( forceVector );
			}
		}

		HeadcrabRelease_t release = ShouldReleaseHeadcrab( info, flDamageThreshold );
		
		if( release == RELEASE_IMMEDIATE )
		{
			ReleaseHeadcrab( EyePosition(), vec3_origin, true, true );
		}
		else if( release == RELEASE_SCHEDULED )
		{
			SetCondition( COND_ZOMBIE_RELEASECRAB );
		}
	}

	// IMPORTANT: always clear the headshot flag after applying damage. No early outs!
	m_bHeadShot = false;

	return tookDamage;
}


//-----------------------------------------------------------------------------
// Purpose: Handle the special case of a zombie killed by a physics chopper.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::DieChopped( const CTakeDamageInfo &info )
{
	Vector forceVector( vec3_origin );

	forceVector += CalcDamageForceVector( info );

	EmitSound( "E3_Phystown.Slicer" );

	forceVector *= 0.2;
	BecomeTorso( forceVector, vec3_origin );
	ReleaseHeadcrab( EyePosition(), forceVector * 0.05, true, false );

	// is this 100kg at 10 inches per second? (sjb)
	forceVector.z = ( 100 * 12 * 10 );

	BecomeRagdollOnClient( forceVector );

	int i;

	for ( i = 0 ; i < 10 ; i++ )
	{
		Vector vecSpot = WorldSpaceCenter();
		
		vecSpot.x += random->RandomFloat( -30, 30 ); 
		vecSpot.y += random->RandomFloat( -30, 30 ); 
		vecSpot.z += random->RandomFloat( -10, 10 ); 

		UTIL_BloodDrips( vecSpot, vec3_origin, BLOOD_COLOR_RED, 50 );
	}

	for ( i = 0 ; i < 5 ; i++ )
	{
		Vector vecSpot = WorldSpaceCenter();
		
		vecSpot.x += random->RandomFloat( -15, 15 ); 
		vecSpot.y += random->RandomFloat( -15, 15 ); 
		vecSpot.z += random->RandomFloat( -8, 8 );

		Vector vecDir;
		vecDir.x = random->RandomFloat(-1, 1);
		vecDir.y = random->RandomFloat(-1, 1);
		vecDir.z = 0;

		UTIL_BloodSpray( vecSpot, vecDir, BLOOD_COLOR_RED, 4, FX_BLOODSPRAY_DROPS | FX_BLOODSPRAY_CLOUD );
	}

	UTIL_BloodSpray( WorldSpaceCenter(), Vector(0, 0, 1), BLOOD_COLOR_RED, 4, FX_BLOODSPRAY_DROPS | FX_BLOODSPRAY_CLOUD );
}

//-----------------------------------------------------------------------------
// Purpose: Open a window and let a little bit of the looping moan sound
//			come through.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize )
{
	if( !m_pMoanSound )
	{
		// Don't set this up until the code calls for it.
		const char *pszSound = GetMoanSound( m_iMoanSound );
		m_flMoanPitch = random->RandomInt( zombie_basemin.GetInt(), zombie_basemax.GetInt() );

		//m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( entindex(), CHAN_STATIC, pszSound, ATTN_NORM );
		CPASAttenuationFilter filter( this );
		m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_STATIC, pszSound, ATTN_NORM );

		ENVELOPE_CONTROLLER.Play( m_pMoanSound, 1.0, m_flMoanPitch );
	}

	//HACKHACK get these from chia chin's console vars.
	envDefaultZombieMoanVolumeFast[ 1 ].durationMin = zombie_decaymin.GetFloat();
	envDefaultZombieMoanVolumeFast[ 1 ].durationMax = zombie_decaymax.GetFloat();

	if( random->RandomInt( 1, 2 ) == 1 )
	{
		IdleSound();
	}

	float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pMoanSound, SOUNDCTRL_CHANGE_VOLUME, pEnvelope, iEnvelopeSize );

	float flPitch = random->RandomInt( m_flMoanPitch + zombie_changemin.GetInt(), m_flMoanPitch + zombie_changemax.GetInt() );
	ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, flPitch, 0.3 );

	m_flNextMoanSound = gpGlobals->curtime + duration + 9999;
}

//-----------------------------------------------------------------------------
// Purpose: Determine whether the zombie is chopped up by some physics item
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::IsChopped( const CTakeDamageInfo &info )
{
	float flDamageThreshold = min( 1, info.GetDamage() / m_iMaxHealth );

	if ( ( m_iHealth <= 0 || flDamageThreshold > 0.5 ) && ( info.GetDamageType() & DMG_SLASH) && ( info.GetDamageType() & DMG_CRUSH) )
	{
		// If you take crush and slash damage, you're hit by a sharp physics item.
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: damage has been done. Should the zombie ignite?
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldIgnite( const CTakeDamageInfo &info )
{
 	if ( IsOnFire() )
	{
		// Already burning!
		return false;
	}

	if ( info.GetDamageType() & DMG_BURN )
	{
		//
		// If we take more than ten percent of our health in burn damage within a five
		// second interval, we should catch on fire.
		//
		m_flBurnDamage += info.GetDamage();
		m_flBurnDamageResetTime = gpGlobals->curtime + 5;

		if ( m_flBurnDamage >= m_iMaxHealth * 0.1 )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sufficient fire damage has been done. Zombie ignites!
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Ignite( float flFlameLifetime )
{
	BaseClass::Ignite( flFlameLifetime );

	// FIXME: use overlays when they come online
	//AddOverlay( ACT_ZOM_WALK_ON_FIRE, false );

	Activity activity = GetActivity();
	if ( activity == ACT_WALK )
	{
		SetActivity( ( Activity )ACT_WALK_ON_FIRE );
	}
	else if ( activity == ACT_IDLE )
	{
		SetActivity( ( Activity )ACT_IDLE_ON_FIRE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Look in front and see if the claw hit anything.
//
// Input  :	flDist				distance to trace		
//			iDamage				damage to do if attack hits
//			vecViewPunch		camera punch (if attack hits player)
//			vecVelocityPunch	velocity punch (if attack hits player)
//
// Output : The entity hit by claws. NULL if nothing.
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_BaseZombie::ClawAttack( float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch )
{
	//
	// Trace out a cubic section of our hull and see what we hit.
	//
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	CBaseEntity *pHurt = CheckTraceHullAttack( flDist, vecMins, vecMaxs, iDamage, DMG_SLASH );

	if ( pHurt )
	{
		AttackHitSound();

		CBasePlayer *pPlayer = ToBasePlayer( pHurt );

		if ( pPlayer != NULL )
		{
			pPlayer->ViewPunch( qaViewPunch );
			
			pPlayer->VelocityPunch( vecVelocityPunch );
		}
	}
	else 
	{
		AttackMissSound();
	}

	return pHurt;
}

//-----------------------------------------------------------------------------
// Purpose: The zombie is frustrated and pounding walls/doors. Make an appropriate noise
// Input  : 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::PoundSound()
{
	trace_t		tr;
	Vector		forward;

	GetVectors( &forward, NULL, NULL );

	AI_TraceLine( EyePosition(), EyePosition() + forward * 128, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if( tr.fraction == 1.0 )
	{
		// Didn't hit anything!
		return;
	}

	// All we have for the moment is this one sound.
	CPASAttenuationFilter filter( this,"NPC_BaseZombie.PoundDoor" );
	EmitSound( filter, entindex(),"NPC_BaseZombie.PoundDoor" );
}

//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific events that occur when tagged animation
//			frames are played.
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case ZOMBIE_AE_POUND:
		PoundSound();
		break;

	case ZOMBIE_AE_STEP_LEFT:
		FootstepSound( false );
		break;
	
	case ZOMBIE_AE_STEP_RIGHT:
		FootstepSound( true );
		break;

	case ZOMBIE_AE_GET_UP:
		m_flNextMoanSound = gpGlobals->curtime;
		MoanSound( envDefaultZombieMoanVolumeFast, ARRAYSIZE( envDefaultZombieMoanVolumeFast ) );
		break;

	case ZOMBIE_AE_SCUFF_LEFT:
		FootscuffSound( false );
		break;

	case ZOMBIE_AE_SCUFF_RIGHT:
		FootscuffSound( true );
		break;

	// all swat animations are handled as a single case.
	case ZOMBIE_AE_STARTSWAT:
		AttackSound();
		break;

	case ZOMBIE_AE_ATTACK_SCREAM:
		AttackSound();
		break;

	case ZOMBIE_AE_SWATITEM:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if ( pEnemy )
			{
				Vector v;
				CBaseEntity *pPhysicsEntity = m_hPhysicsEnt;
				if( !pPhysicsEntity )
				{
					Msg( "**Zombie: Missing my physics ent!!" );
					return;
				}
				
				IPhysicsObject *pPhysObj = pPhysicsEntity->VPhysicsGetObject();

				if( !pPhysObj )
				{
					Msg( "**Zombie: No Physics Object for physics Ent!" );
					return;
				}

				EmitSound( "NPC_BaseZombie.Swat" );
				PhysicsImpactSound( pEnemy, pPhysObj, CHAN_BODY, pPhysObj->GetMaterialIndex(), 0.5 );

				Vector physicsCenter = pPhysicsEntity->WorldSpaceCenter();
				v = pEnemy->WorldSpaceCenter() - physicsCenter;
				VectorNormalize(v);

				// Send the object at 800 in/sec toward the enemy.  Add 200 in/sec up velocity to keep it
				// in the air for a second or so.
				v = v * 800;
				v.z += 200;

				// add some spin so the object doesn't appear to just fly in a straight line
				// Also this spin will move the object slightly as it will press on whatever the object
				// is resting on.
				AngularImpulse angVelocity( random->RandomFloat(-180, 180), 20, random->RandomFloat(-360, 360) );

				pPhysObj->AddVelocity( &v, &angVelocity );

				// If we don't put the object scan time well into the future, the zombie
				// will re-select the object he just hit as it is flying away from him.
				// It will likely always be the nearest object because the zombie moved
				// close enough to it to hit it.
				m_hPhysicsEnt = NULL;

				m_flNextSwatScan = gpGlobals->curtime + ZOMBIE_SWAT_DELAY;
			}
		}
		break;

		case ZOMBIE_AE_ATTACK_RIGHT:
		{
			Vector right, forward;
			AngleVectors( GetLocalAngles(), &forward, &right, NULL );
			
			right = right * 100;
			forward = forward * 200;

			ClawAttack( ZOMBIE_MELEE_REACH, sk_zombie_dmg_one_slash.GetFloat(), QAngle( -15, -20, -10 ), right + forward );
			break;
		}

		case ZOMBIE_AE_ATTACK_LEFT:
		{
			Vector right, forward;
			AngleVectors( GetLocalAngles(), &forward, &right, NULL );

			right = right * -100;
			forward = forward * 200;

			ClawAttack( ZOMBIE_MELEE_REACH, sk_zombie_dmg_one_slash.GetFloat(), QAngle( -15, 20, -10 ), right + forward );
			break;
		}

		case ZOMBIE_AE_ATTACK_BOTH:
		{
			Vector forward;
			QAngle qaPunch( 45, random->RandomInt(-5,5), random->RandomInt(-5,5) );
			AngleVectors( GetLocalAngles(), &forward );
			forward = forward * 200;
			ClawAttack( ZOMBIE_MELEE_REACH, sk_zombie_dmg_one_slash.GetFloat(), qaPunch, forward );
			break;
		}

		default:
		{
			BaseClass::HandleAnimEvent( pEvent );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the base zombie.
//
// !!!IMPORTANT!!! YOUR DERIVED CLASS'S SPAWN() RESPONSIBILITIES:
//
//		Call Precache();
//		Set status for m_fIsTorso & m_fIsHeadless
//		Set blood color
//		Set health
//		Set field of view
//		Call CapabilitiesClear() & then set relevant capabilities
//		THEN Call BaseClass::Spawn()
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Spawn( void )
{
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );

	m_NPCState			= NPC_STATE_NONE;

	SetCollisionGroup( HL2COLLISION_GROUP_ZOMBIE );

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_DOORS_GROUP );

	m_flNextSwat = gpGlobals->curtime;
	m_flNextSwatScan = gpGlobals->curtime;
	m_pMoanSound = NULL;

	m_flNextMoanSound = gpGlobals->curtime + 9999;

	SetZombieModel();

	NPCInit();

	// Zombies get to cheat for 6 seconds (sjb)
	GetEnemies()->SetFreeKnowledgeDuration( 6.0 );
}


//-----------------------------------------------------------------------------
// Purpose: Pecaches all resources this NPC needs.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Precache( void )
{
	UTIL_PrecacheOther( GetHeadcrabClassname() );

	BaseClass::Precache();
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY:
		return SCHED_ZOMBIE_CHASE_ENEMY;
		break;

	case SCHED_ZOMBIE_SWATITEM:
		// If the object is far away, move and swat it. If it's close, just swat it.
		if( DistToPhysicsEnt() > ZOMBIE_PHYSOBJ_SWATDIST )
		{
			return SCHED_ZOMBIE_MOVE_SWATITEM;
		}
		else
		{
			return SCHED_ZOMBIE_SWATITEM;
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::BuildScheduleTestBits( void )
{
	// Ignore damage if we were recently damaged or we're attacking.
	if ((GetActivity() == ACT_MELEE_ATTACK1) || (m_flNextFlinch >= gpGlobals->curtime))
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}

	// Everything should be interrupted if we get killed.
	SetCustomInterruptCondition( COND_ZOMBIE_RELEASECRAB );
}


//-----------------------------------------------------------------------------
// Purpose: Called when we change schedules.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::OnScheduleChange( void )
{
	//
	// If we took damage and changed schedules, ignore further damage for a few seconds.
	//
	if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ))
	{
		m_flNextFlinch = gpGlobals->curtime + ZOMBIE_FLINCH_DELAY;
	} 
}


//---------------------------------------------------------
//---------------------------------------------------------
int	CNPC_BaseZombie::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if( failedSchedule == SCHED_ZOMBIE_WANDER_MEDIUM )
	{
		return SCHED_ZOMBIE_WANDER_FAIL;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::SelectSchedule ( void )
{
	if ( HasCondition( COND_ZOMBIE_RELEASECRAB ) )
	{
		// Death waits for no man. Or zombie. Or something.
		return SCHED_ZOMBIE_RELEASECRAB;
	}

	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if ( HasCondition( COND_NEW_ENEMY ) && GetEnemy() )
		{
			float flDist;

			flDist = ( GetLocalOrigin() - GetEnemy()->GetLocalOrigin() ).Length();

			// If this is a new enemy that's far away, ambush!!
			if (flDist >= zombie_ambushdist.GetFloat() && MustCloseToAttack() )
			{
				return SCHED_ZOMBIE_MOVE_TO_AMBUSH;
			}
		}

		if ( HasCondition( COND_LOST_ENEMY ) || ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() ) )
		{
			return SCHED_ZOMBIE_WANDER_MEDIUM;
		}

		if( HasCondition( COND_ZOMBIE_CAN_SWAT_ATTACK ) )
		{
			return SCHED_ZOMBIE_SWATITEM;
		}
		break;

	case NPC_STATE_ALERT:
		if ( HasCondition( COND_LOST_ENEMY ) || ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() ) )
		{
			ClearCondition( COND_LOST_ENEMY );
			ClearCondition( COND_ENEMY_UNREACHABLE );

#ifdef DEBUG_ZOMBIES
			DevMsg("Wandering\n");
#endif+

			// Just lost track of our enemy. 
			// Wander around a bit so we don't look like a dingus.
			return SCHED_ZOMBIE_WANDER_MEDIUM;
		}
		break;
	}

	return BaseClass::SelectSchedule();
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::GetSwatActivity( void )
{
	// Hafta figure out whether to swat with left or right arm.
	// Also hafta figure out whether to swat high or low. (later)
	float		flDot;
	Vector		vecRight, vecDirToObj;

	AngleVectors( GetLocalAngles(), NULL, &vecRight, NULL );
	
	vecDirToObj = m_hPhysicsEnt->GetLocalOrigin() - GetLocalOrigin();
	VectorNormalize(vecDirToObj);

	// compare in 2D.
	vecRight.z = 0.0;
	vecDirToObj.z = 0.0;

	flDot = DotProduct( vecRight, vecDirToObj );

	Vector vecMyCenter;
	Vector vecObjCenter;

	vecMyCenter = WorldSpaceCenter();
	vecObjCenter = m_hPhysicsEnt->WorldSpaceCenter();
	float flZDiff;

	flZDiff = vecMyCenter.z - vecObjCenter.z;

	if( flDot >= 0 )
	{
		// Right
		if( flZDiff < 0 )
		{
			return ACT_ZOM_SWATRIGHTMID;
		}

		return ACT_ZOM_SWATRIGHTLOW;
	}
	else
	{
		// Left
		if( flZDiff < 0 )
		{
			return ACT_ZOM_SWATLEFTMID;
		}

		return ACT_ZOM_SWATLEFTLOW;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if( m_NPCState == NPC_STATE_COMBAT && !m_fIsTorso )
	{
		// This check for !m_pPhysicsEnt prevents a crashing bug, but also
		// eliminates the zombie picking a better physics object if one happens to fall
		// between him and the object he's heading for already. 
		if( gpGlobals->curtime >= m_flNextSwatScan && (m_hPhysicsEnt == NULL) )
		{
			FindNearestPhysicsObject( ZOMBIE_MAX_PHYSOBJ_MASS );
			m_flNextSwatScan = gpGlobals->curtime + 2.0;
		}
	}

	if( (m_hPhysicsEnt != NULL) && gpGlobals->curtime >= m_flNextSwat && HasCondition( COND_SEE_ENEMY ) && !HasCondition( COND_ZOMBIE_RELEASECRAB ) )
	{
		SetCondition( COND_ZOMBIE_CAN_SWAT_ATTACK );
	}
	else
	{
		ClearCondition( COND_ZOMBIE_CAN_SWAT_ATTACK );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
#if 0
	Msg(" ** %d Angry Zombies **\n", s_iAngryZombies );
#endif

#if 0
	if( m_NPCState == NPC_STATE_COMBAT )
	{
		// Zombies should make idle sounds in combat
		if( random->RandomInt( 0, 30 ) == 0 )
		{
			IdleSound();
		}
	}	
#endif 

	//
	// Cool off if we aren't burned for five seconds or so. 
	//
	if ( ( m_flBurnDamageResetTime ) && ( gpGlobals->curtime >= m_flBurnDamageResetTime ) )
	{
		m_flBurnDamage = 0;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_DIE:
		// Go to ragdoll
		KillMe();
		TaskComplete();
		break;

	case TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ:
		{
			Vector vecGoalPos;
			Vector vecDir;

			vecDir = GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin();
			VectorNormalize(vecDir);
			vecDir.z = 0;

			// UNDONE: This goal must be based on the size of the object, not a constant.
			vecGoalPos = m_hPhysicsEnt->WorldSpaceCenter() + (vecDir * ZOMBIE_PHYSOBJ_MOVE_TO_DIST );

			GetNavigator()->SetGoal( vecGoalPos, AIN_CLEAR_TARGET );

			TaskComplete();
		}
		break;

	case TASK_ZOMBIE_SWAT_ITEM:
		{
			if( m_hPhysicsEnt == NULL )
			{
				// Physics Object is gone! Probably was an explosive 
				// or something else broke it.
				TaskFail("Physics ent NULL");
			}
			else if ( DistToPhysicsEnt() > ZOMBIE_PHYSOBJ_SWATDIST )
			{
				// Physics ent is no longer in range! Probably another zombie swatted it or it moved
				// for some other reason.
				TaskFail( "Physics swat item has moved" );
			}
			else
			{
				SetIdealActivity( (Activity)GetSwatActivity() );
			}
			break;
		}
		break;

	case TASK_ZOMBIE_DELAY_SWAT:
		m_flNextSwat = gpGlobals->curtime + pTask->flTaskData;
		TaskComplete();
		break;

	case TASK_ZOMBIE_RELEASE_HEADCRAB:
		{
			// make the crab look like it's pushing off the body
			Vector vecForward;
			Vector vecVelocity;

			AngleVectors( GetAbsAngles(), &vecForward );
			
			vecVelocity = vecForward * 30;
			vecVelocity.z += 100;

			ReleaseHeadcrab( EyePosition(), vecVelocity, true, true );
			TaskComplete();
		}
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_SWAT_ITEM:
		if( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//---------------------------------------------------------
// Make the necessary changes to a zombie to make him a 
// torso!
//---------------------------------------------------------
void CNPC_BaseZombie::BecomeTorso( const Vector &vecTorsoForce, const Vector &vecLegsForce )
{
	if( m_fIsTorso )
	{
		Msg( "*** Zombie is already a torso!\n" );
		return;
	}

	m_iMaxHealth = ZOMBIE_TORSO_HEALTH_FACTOR * m_iMaxHealth;
	m_iHealth = m_iMaxHealth;

	// No more opening doors!
	CapabilitiesRemove( bits_CAP_DOORS_GROUP );
	
	ClearSchedule();
	GetNavigator()->ClearGoal();
	m_hPhysicsEnt = NULL;

	// Put the zombie in a TOSS / fall schedule
	// Otherwise he fails and sits on the ground for a sec.
	SetSchedule( SCHED_FALL_TO_GROUND );

	m_fIsTorso = true;

	CBaseEntity *pGib = CreateRagGib( GetLegsModel(), GetLocalOrigin(), GetLocalAngles(), vecLegsForce );

	// don't collide with this thing ever
	if ( pGib )
	{
		pGib->SetOwnerEntity( this );
	}

	// Put the torso up where the torso was when the zombie
	// was whole.
	Vector origin = GetLocalOrigin();
	origin.z += 48;
	
	UTIL_SetOrigin( this, origin );

	RemoveFlag( FL_ONGROUND );
	// assume zombie mass ~ 100 kg
	ApplyAbsVelocityImpulse( vecTorsoForce * (1.0 / 100.0) );

	SetZombieModel();
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );
}

void CNPC_BaseZombie::StopLoopingSounds()
{
	ENVELOPE_CONTROLLER.SoundDestroy( m_pMoanSound );
	m_pMoanSound = NULL;

	BaseClass::StopLoopingSounds();
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::RemoveHead( void )
{
	m_fIsHeadless = true;
	SetZombieModel();
}


bool CNPC_BaseZombie::ShouldPlayFootstepMoan( void )
{
	if( random->RandomInt( 1, zombie_stepfreq.GetInt() * s_iAngryZombies ) == 1 )
	{
		return true;
	}

	return false;
}


#define ZOMBIE_CRAB_INHERITED_SPAWNFLAGS	(SF_NPC_GAG|SF_NPC_LONG_RANGE|SF_NPC_FADE_CORPSE|SF_NPC_ALWAYSTHINK)

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//			&vecVelocity - 
//			fRemoveHead - 
//			fRagdollBody - 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::ReleaseHeadcrab( const Vector &vecOrigin, const Vector &vecVelocity, bool fRemoveHead, bool fRagdollBody )
{
	CAI_BaseNPC		*pCrab;
	Vector vecSpot = vecOrigin;

	// Until the headcrab is a bodygroup, we have to approximate the
	// location of the head with magic numbers.
	if( !m_fIsTorso )
	{
		vecSpot.z -= 16;
	}

	pCrab = (CAI_BaseNPC*)CreateEntityByName( GetHeadcrabClassname() );

	if ( !pCrab )
	{
		Warning( "**%s: Can't make %s!\n", GetClassname(), GetHeadcrabClassname() );
		return;
	}

	// don't pop to floor, fall
	pCrab->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
	
	// add on the parent flags
	pCrab->AddSpawnFlags( m_spawnflags & ZOMBIE_CRAB_INHERITED_SPAWNFLAGS );
	
	// make me the crab's owner to avoid collision issues
	pCrab->SetOwnerEntity( this );

	pCrab->SetLocalOrigin( vecSpot );
	pCrab->SetLocalAngles( GetAbsAngles() );
	DispatchSpawn( pCrab );
	pCrab->SetActivity( ACT_RANGE_ATTACK1 );
	pCrab->SetNextThink( gpGlobals->curtime );
	pCrab->PhysicsSimulate();
	pCrab->SetAbsVelocity( vecVelocity );

	pCrab->GetMotor()->SetIdealYaw( GetAbsAngles().y );


	// if I have an enemy, stuff that to the headcrab.
	CBaseEntity *pEnemy;
	pEnemy = GetEnemy();

	if( pEnemy )
	{
		pCrab->SetEnemy( pEnemy );
		pCrab->SetCondition( COND_NEW_ENEMY );
	}

	if( fRemoveHead )
	{
		RemoveHead();
	}

	if( fRagdollBody )
	{
		BecomeRagdollOnClient( vec3_origin );
	}
}


//---------------------------------------------------------
// Provides a standard way for the zombie to get the 
// distance to a physics ent. Since the code to find physics 
// objects uses a fast dis approx, we have to use that here
// as well.
//---------------------------------------------------------
float CNPC_BaseZombie::DistToPhysicsEnt( void )
{
	//return ( GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin() ).Length();
	if ( m_hPhysicsEnt != NULL )
		return UTIL_DistApprox2D( GetAbsOrigin(), m_hPhysicsEnt->WorldSpaceCenter() );
	return ZOMBIE_PHYSOBJ_SWATDIST + 1;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	switch( NewState )
	{
	case NPC_STATE_COMBAT:
		s_iAngryZombies++;
		break;

	default:
		if( OldState == NPC_STATE_COMBAT )
		{
			// Only decrement if coming OUT of combat state.
			s_iAngryZombies--;
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Refines a base activity into something more specific to our internal state.
//-----------------------------------------------------------------------------
Activity CNPC_BaseZombie::NPC_TranslateActivity( Activity baseAct )
{
	if ( IsOnFire() )
	{
		switch ( baseAct )
		{
			case ACT_WALK:
			{
				// I'm on fire. Put ME out.
				return ( Activity )ACT_WALK_ON_FIRE;
			}

			case ACT_IDLE:
			{
				// I'm on fire. Put ME out.
				return ( Activity )ACT_IDLE_ON_FIRE;
			}
		}
	}

	return BaseClass::NPC_TranslateActivity( baseAct );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( base_zombie, CNPC_BaseZombie )

	DECLARE_TASK( TASK_ZOMBIE_DELAY_SWAT )
	DECLARE_TASK( TASK_ZOMBIE_SWAT_ITEM )
	DECLARE_TASK( TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ )
	DECLARE_TASK( TASK_ZOMBIE_DIE )
	DECLARE_TASK( TASK_ZOMBIE_RELEASE_HEADCRAB )

	DECLARE_ACTIVITY( ACT_ZOM_SWATLEFTMID )
	DECLARE_ACTIVITY( ACT_ZOM_SWATRIGHTMID )
	DECLARE_ACTIVITY( ACT_ZOM_SWATLEFTLOW )
	DECLARE_ACTIVITY( ACT_ZOM_SWATRIGHTLOW )
	DECLARE_ACTIVITY( ACT_ZOM_RELEASECRAB )
	DECLARE_ACTIVITY( ACT_ZOM_FALL )

	DECLARE_CONDITION( COND_ZOMBIE_CAN_SWAT_ATTACK )
	DECLARE_CONDITION( COND_ZOMBIE_RELEASECRAB )

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_MOVE_SWATITEM,

		"	Tasks"
		"		TASK_ZOMBIE_DELAY_SWAT			3"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ	0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY					0"
		"		TASK_ZOMBIE_SWAT_ITEM			0"
		"	"
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// SwatItem
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_SWATITEM,

		"	Tasks"
		"		TASK_ZOMBIE_DELAY_SWAT			3"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY					0"
		"		TASK_ZOMBIE_SWAT_ITEM			0"
		"	"
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// ChaseEnemy
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_CHASE_ENEMY,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		 TASK_SET_TOLERANCE_DISTANCE	24"
		"		 TASK_GET_PATH_TO_ENEMY			0"
		"		 TASK_RUN_PATH					0"
		"		 TASK_WAIT_FOR_MOVEMENT			0"
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
		"		COND_BETTER_WEAPON_AVAILABLE"
		"		COND_ZOMBIE_CAN_SWAT_ATTACK"
		"		COND_ZOMBIE_RELEASECRAB"
	)


	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_RELEASECRAB,

		"	Tasks"
		"		TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ZOM_RELEASECRAB"
		"		TASK_ZOMBIE_RELEASE_HEADCRAB				0"
		"		TASK_ZOMBIE_DIE								0"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
	)


	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_MOVE_TO_AMBUSH,

		"	Tasks"
		"		TASK_WAIT						1.0" // don't react as soon as you see the player.
		"		TASK_FIND_COVER_FROM_ENEMY		0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_TURN_LEFT					180"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WAIT_AMBUSH"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_NEW_ENEMY"
	)


	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WAIT_AMBUSH,

		"	Tasks"
		"		TASK_WAIT_FACE_ENEMY	99999"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
	)

	//=========================================================
	// Wander around for a while so we don't look stupid. 
	// this is done if we ever lose track of our enemy.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_MEDIUM,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WANDER						480384" // 4 feet to 32 feet
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM" // keep doing it
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// If you fail to wander, wait just a bit and try again.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_FAIL,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_WAIT				1"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_ZOMBIE_RELEASECRAB"
	)
AI_END_CUSTOM_NPC()
