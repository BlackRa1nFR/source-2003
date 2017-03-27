//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"

#include "ai_basenpc.h"
#include "fmtstr.h"
#include "activitylist.h"
#include "animation.h"
#include "basecombatweapon.h"
#include "soundent.h"
#include "decals.h"
#include "entitylist.h"
#include "eventqueue.h"
#include "entityapi.h"
#include "bitstring.h"
#include "gamerules.h"		// For g_pGameRules
#include "scripted.h"
#include "worldsize.h"
#include "game.h"
#include "ai_network.h"
#include "ai_networkmanager.h"
#include "ai_pathfinder.h"
#include "ai_node.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_task.h"
#include "ai_hull.h"
#include "ai_moveprobe.h"
#include "ai_hint.h"
#include "ai_navigator.h"
#include "ai_senses.h"
#include "ai_squadslot.h"
#include "ai_memory.h"
#include "ai_squad.h"
#include "ai_localnavigator.h"
#include "ai_tacticalservices.h"
#include "ai_behavior.h"
#include "basegrenade_shared.h"
#include "ammodef.h"
#include "player.h"
#include "sceneentity.h"
#include "ndebugoverlay.h"
#include "mathlib.h"
#include "bone_setup.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "vstdlib/strtools.h"
#include "doors.h"
#include "BasePropDoor.h"
#include "saverestore_utlvector.h"
#include "npcevent.h"
#include "movevars_shared.h"
#include "te_effect_dispatch.h"
#include "globals.h"
#include "saverestore_bitstring.h"
#include "checksum_crc.h"
#include "iservervehicle.h"
#ifdef HL2_DLL
#include "npc_bullseye.h"
#include "hl2_player.h"
#endif

// dvs: for opening doors -- these should probably not be here
#include "ai_route.h"
#include "ai_waypoint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define DEBUG_LOOK

#define	NPC_GRENADE_FEAR_DIST		200

#define MIN_CORPSE_FADE_TIME		10.0
#define	MIN_CORPSE_FADE_DIST		256.0
#define	MAX_CORPSE_FADE_DIST		1500.0

extern bool			g_fDrawLines;
extern short		g_sModelIndexLaser;		// holds the index for the laser beam
extern short		g_sModelIndexLaserDot;	// holds the index for the laser beam dot

//const char *		g_sNearMissSounds[ NUM_NEARMISSES ];

// NPC damage adjusters
ConVar	sk_npc_head( "sk_npc_head","2" );
ConVar	sk_npc_chest( "sk_npc_chest","1" );
ConVar	sk_npc_stomach( "sk_npc_stomach","1" );
ConVar	sk_npc_arm( "sk_npc_arm","1" );
ConVar	sk_npc_leg( "sk_npc_leg","1" );
ConVar	showhitlocation( "showhitlocation", "0" );

// ConVar for ai_speeds
extern ConVar game_speeds;

// Shoot trajectory
ConVar	ai_lead_time( "ai_lead_time","0.09" );
ConVar	ai_shot_stats( "ai_shot_stats", "0" );
ConVar	ai_shot_stats_term( "ai_shot_stats_term", "300" );

//-----------------------------------------------------------------------------
//
// Crude frame timings
//

CFastTimer g_AIRunTimer;
CFastTimer g_AIPostRunTimer;
CFastTimer g_AIMoveTimer;

CFastTimer g_AIConditionsTimer;
CFastTimer g_AIPrescheduleThinkTimer;
CFastTimer g_AIMaintainScheduleTimer;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CAI_Manager g_AI_Manager;

//-------------------------------------

CAI_Manager::CAI_Manager()
{
	m_AIs.EnsureCapacity( MAX_AIS );
}

//-------------------------------------

CAI_BaseNPC **CAI_Manager::AccessAIs()
{
	if (m_AIs.Count())
		return &m_AIs[0];
	return NULL;
}

//-------------------------------------

int CAI_Manager::NumAIs()
{
	return m_AIs.Count();
}

//-------------------------------------

void CAI_Manager::AddAI( CAI_BaseNPC *pAI )
{
	m_AIs.AddToTail( pAI );
}

//-------------------------------------

void CAI_Manager::RemoveAI( CAI_BaseNPC *pAI )
{
	int i = m_AIs.Find( pAI );

	Assert( i != -1 );

	if ( i != -1 )
		m_AIs.FastRemove( i );
}


//-----------------------------------------------------------------------------

// ================================================================
//  Init static data
// ================================================================
int					CAI_BaseNPC::m_nDebugBits		= 0;
CAI_BaseNPC*		CAI_BaseNPC::m_pDebugNPC		= NULL;
int					CAI_BaseNPC::m_nDebugPauseIndex	= -1;

CAI_ClassScheduleIdSpace	CAI_BaseNPC::gm_ClassScheduleIdSpace( true );
CAI_GlobalScheduleNamespace CAI_BaseNPC::gm_SchedulingSymbols;
CAI_LocalIdSpace			CAI_BaseNPC::gm_SquadSlotIdSpace( true );

// ================================================================
//  Class Methods
// ================================================================

//-----------------------------------------------------------------------------
// Purpose: Static debug function to clear schedules for all NPCS
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ClearAllSchedules(void)
{
	CAI_BaseNPC *npc = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (npc)
	{
		npc->ClearSchedule();
		npc->GetNavigator()->ClearGoal();
		npc = gEntList.NextEntByClass(npc);
	}
}

// ==============================================================================

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::Event_Gibbed( const CTakeDamageInfo &info )
{
	bool gibbed = CorpseGib( info );

	if ( gibbed )
	{
		// don't remove players!
		UTIL_Remove( this );
	}
	else
	{
		CorpseFade();
	}

	return gibbed;
}

//=========================================================
// GetSmallFlinchActivity - determines the best type of flinch
// anim to play.
//=========================================================
Activity CAI_BaseNPC::GetSmallFlinchActivity ( void )
{
	Activity	flinchActivity;
	bool		fTriedDirection;
	float		flDot;

	fTriedDirection = false;
	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );
	flDot = -DotProduct( forward, g_vecAttackDir );

	switch ( LastHitGroup() )
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchActivity = ACT_FLINCH_HEAD;
		break;
	case HITGROUP_STOMACH:
		flinchActivity = ACT_FLINCH_STOMACH;
		break;
	case HITGROUP_LEFTARM:
		flinchActivity = ACT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchActivity = ACT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchActivity = ACT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchActivity = ACT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_GEAR:
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchActivity = ACT_SMALL_FLINCH;
		break;
	}


	// do we have a sequence for the ideal activity?
	if ( SelectWeightedSequence ( flinchActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		flinchActivity = ACT_SMALL_FLINCH;
	}

	return flinchActivity;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::CleanupOnDeath( CBaseEntity *pCulprit )
{
	if ( !m_bDidDeathCleanup )
	{
		m_bDidDeathCleanup = true;

		if( IsSelected() )
		{
			PlayerSelect( false );
		}

		m_OnDeath.FireOutput( pCulprit, this );

		// Vacate any strategy slot I might have
		VacateStrategySlot();

		// Remove from squad if in one
		if (m_pSquad)
		{
			// If I'm in idle it means that I didn't see who killed me
			// and my squad is still in idle state. Tell squad we have
			// an enemy to wake them up and put the enemy position at
			// my death position
			if (m_NPCState == NPC_STATE_IDLE && pCulprit)
			{
				UpdateEnemyMemory( pCulprit, GetAbsOrigin() );
			}

			// Remove from squad
			m_pSquad->RemoveFromSquad(this);
			m_pSquad = NULL;
		}
	}
	else
		Msg( "Unexpected double-death-cleanup\n" );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::Event_Killed( const CTakeDamageInfo &info )
{
	if (IsCurSchedule(SCHED_NPC_FREEZE))
	{
		// We're frozen; don't die.
		return;
	}

	if ( m_NPCState == NPC_STATE_SCRIPT && m_hCine )
	{
		// bail out of this script here
		m_hCine->CancelScript();
		// now keep going with the death code
	}

	CleanupOnDeath( info.GetAttacker() );

	StopLoopingSounds();
	DeathSound();

	if ( ( GetFlags() & FL_NPC ) && ( ShouldGib( info ) == false ) )
	{
		SetTouch( NULL );
	}

	BaseClass::Event_Killed( info );

	// Make sure this condition is fired too (OnTakeDamage breaks out before this happens on death)
	SetCondition( COND_LIGHT_DAMAGE );

	if ( ShouldFadeOnDeath() )
	{
		// this NPC was created by a NPCmaker... fade the corpse out.
		SUB_StartFadeOut();
	}
	else
	{
		// body is gonna be around for a while, so have it stink for a bit.
		CSoundEnt::InsertSound ( SOUND_CARCASS, GetAbsOrigin(), 384, 30 );
	}

	m_IdealNPCState = NPC_STATE_DEAD;

	// Some characters rely on getting a state transition, even to death.
	// zombies, for instance. When a character becomes a ragdoll, their
	// server entity ceases to think, so we have to set the dead state here
	// because the AI code isn't going to pick up the change on the next think
	// for us.

	// Adrian - Only set this if we are going to become a ragdoll. We still want to 
	// select SCHED_DIE or do something special when this NPC dies and we wont 
	// catch the change of state if we set this to whatever the ideal state is.
	if ( CanBecomeRagdoll() || IsRagdoll() )
		 SetState( NPC_STATE_DEAD );

	// If the remove-no-ragdoll flag is set in the damage type, we're being
	// told to remove ourselves immediately on death. This is used when something
	// else has some special reason for us to vanish instead of creating a ragdoll.
	// i.e. The barnacle does this because it's already got a ragdoll for us.
	if ( info.GetDamageType() & DMG_REMOVENORAGDOLL )
	{
		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::PassesDamageFilter( CBaseEntity *pAttacker )
{
	if ( !BaseClass::PassesDamageFilter( pAttacker ) )
	{
		m_fNoDamageDecal = true;
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CAI_BaseNPC::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	PainSound();// "Ouch!"
	Forget( bits_MEMORY_INCOVER );

	if ( !BaseClass::OnTakeDamage_Alive( info ) )
		return 0;

#if 0
	// HACKHACK Don't kill npcs in a script.  Let them break their scripts first
	// THIS is a Half-Life 1 hack that's not cutting the mustard in the scripts
	// that have been authored for Half-Life 2 thus far. (sjb)
	if ( m_NPCState == NPC_STATE_SCRIPT )
	{
		SetCondition( COND_LIGHT_DAMAGE );
	}
#endif

	// -----------------------------------
	//  Fire outputs
	// -----------------------------------
	if ( m_flLastDamageTime != gpGlobals->curtime )
	{
		// only fire once per frame
		m_OnDamaged.FireOutput(this, this);
	}

	if ( m_iHealth <= ( m_iMaxHealth / 2 ) )
	{
		m_OnHalfHealth.FireOutput(this, this);
	}

	// react to the damage (get mad)
	if ( (GetFlags() & FL_NPC) && info.GetAttacker() )
	{
		// ----------------------------------------------------------------
		// If the attacker was an NPC or client update my position memory
		// -----------------------------------------------------------------
		if ( info.GetAttacker()->GetFlags() & (FL_NPC | FL_CLIENT) )
		{
			// ------------------------------------------------------------------
			//				DO NOT CHANGE THIS CODE W/O CONSULTING
			// Only update information about my attacker I don't see my attacker
			// ------------------------------------------------------------------
			if ( !FInViewCone( info.GetAttacker() ) || !FVisible( info.GetAttacker() ) )
			{
				// -------------------------------------------------------------
				//  If I have an inflictor (enemy / grenade) update memory with
				//  position of inflictor, otherwise update with an position
				//  estimate for where the attack came from
				// ------------------------------------------------------
				Vector vAttackPos;
				if (info.GetInflictor())
				{
					vAttackPos = info.GetInflictor()->GetAbsOrigin();
				}
				else
				{
					vAttackPos = (GetAbsOrigin() + ( g_vecAttackDir * 64 ));
				}


				// ----------------------------------------------------------------
				//  If I already have an enemy, assume that the attack
				//  came from the enemy and update my enemy's position
				//  unless I already know about the attacker or I can see my enemy
				// ----------------------------------------------------------------
				if ( GetEnemy() != NULL							&&
					!GetEnemies()->HasMemory( info.GetAttacker() )			&&
					!HasCondition(COND_SEE_ENEMY)	)
				{
					UpdateEnemyMemory(GetEnemy(), vAttackPos);
				}
				// ----------------------------------------------------------------
				//  If I already know about this enemy, update his position
				// ----------------------------------------------------------------
				else if (GetEnemies()->HasMemory( info.GetAttacker() ))
				{
					UpdateEnemyMemory(info.GetAttacker(), vAttackPos);
				}
				// -----------------------------------------------------------------
				//  Otherwise just not the position, but don't add enemy to my list
				// -----------------------------------------------------------------
				else
				{
					UpdateEnemyMemory(NULL, vAttackPos);
				}
			}

			// add pain to the conditions
			if ( IsLightDamage(info.GetDamage(),info.GetDamageType()) )
			{
				SetCondition(COND_LIGHT_DAMAGE);
			}
			if ( IsHeavyDamage(info.GetDamage(),info.GetDamageType()) )
			{
				SetCondition(COND_HEAVY_DAMAGE);
			}

			// Keep track of how much consecutive damage I have recieved
			if ((gpGlobals->curtime - m_flLastDamageTime) < 1.0)
			{
				m_flSumDamage += info.GetDamage();
			}
			else
			{
				m_flSumDamage = info.GetDamage();
			}
			m_flLastDamageTime = gpGlobals->curtime;

			if (m_flSumDamage > m_iMaxHealth*0.3)
			{
				SetCondition(COND_REPEATED_DAMAGE);
			}
		}

		// ---------------------------------------------------------------
		//  Insert a combat sound so that nearby NPCs know I've been hit
		// ---------------------------------------------------------------
		CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 1024, 0.5, this );//<<TODO>>//magic number
	}

	return 1;
}


//=========================================================
// OnTakeDamage_Dying - takedamage function called when a npc's
// corpse is damaged.
//=========================================================
int CAI_BaseNPC::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & DMG_PLASMA )
	{
		if ( m_takedamage != DAMAGE_EVENTS_ONLY )
		{
			m_iHealth -= info.GetDamage();

			if (m_iHealth < -500)
			{
				UTIL_Remove(this);
			}
		}
	}
	return BaseClass::OnTakeDamage_Dying( info );
}

//=========================================================
// OnTakeDamage_Dead - takedamage function called when a npc's
// corpse is damaged.
//=========================================================
int CAI_BaseNPC::OnTakeDamage_Dead( const CTakeDamageInfo &info )
{
	Vector			vecDir;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = vec3_origin;
	if ( info.GetInflictor() )
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
		VectorNormalize( vecDir );
		g_vecAttackDir = vecDir;
	}

#if 0// turn this back on when the bounding box issues are resolved.

	RemoveFlag( FL_ONGROUND );
	GetLocalOrigin().z += 1;

	// let the damage scoot the corpse around a bit.
	if ( info.GetInflictor() && (info.GetAttacker()->GetSolid() != SOLID_TRIGGER) )
	{
		ApplyAbsVelocityImpulse( vecDir * -DamageForce( flDamage ) );
	}

#endif

	// kill the corpse if enough damage was done to destroy the corpse and the damage is of a type that is allowed to destroy the corpse.
	if ( info.GetDamageType() & DMG_GIB_CORPSE )
	{
		// Accumulate corpse gibbing damage, so you can gib with multiple hits
		if ( m_takedamage != DAMAGE_EVENTS_ONLY )
		{
			m_iHealth -= info.GetDamage() * 0.1;
		}
	}

	if ( info.GetDamageType() & DMG_PLASMA )
	{
		if ( m_takedamage != DAMAGE_EVENTS_ONLY )
		{
			m_iHealth -= info.GetDamage();

			if (m_iHealth < -500)
			{
				UTIL_Remove(this);
			}
		}
	}

	return 1;
}

bool CAI_BaseNPC::IsLightDamage( float flDamage, int bitsDamageType )
{
	// ALL nonzero damage is light damage! Mask off COND_LIGHT_DAMAGE if you want to ignore light damage.
	return flDamage >  0;
}

bool CAI_BaseNPC::IsHeavyDamage( float flDamage, int bitsDamageType )
{
	return (flDamage > 20);
}

void CAI_BaseNPC::DoRadiusDamage( const CTakeDamageInfo &info, int iClassIgnore )
{
	RadiusDamage( info, GetAbsOrigin(), info.GetDamage() * 2.5, iClassIgnore );
}


void CAI_BaseNPC::DoRadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, int iClassIgnore )
{
	RadiusDamage( info, vecSrc, info.GetDamage() * 2.5, iClassIgnore );
}


//=========================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_BaseNPC::DecalTrace( trace_t *pTrace, char const *decalName )
{
	if ( m_fNoDamageDecal )
	{
		m_fNoDamageDecal = false;
		// @Note (toml 04-23-03): e3, don't decal face on damage if still alive
		return;
	}
	BaseClass::DecalTrace( pTrace, decalName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
{
	if ( m_fNoDamageDecal )
	{
		m_fNoDamageDecal = false;
		// @Note (toml 04-23-03): e3, don't decal face on damage if still alive
		return;
	}
	BaseClass::ImpactTrace( pTrace, iDamageType, pCustomImpactName );
}

//=========================================================
// TraceAttack
//=========================================================
void CAI_BaseNPC::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	m_fNoDamageDecal = false;
	if ( m_takedamage )
	{
		CTakeDamageInfo subInfo = info;

		SetLastHitGroup( ptr->hitgroup );
		m_nForceBone = ptr->physicsbone;		// save this bone for physics forces

		Assert( m_nForceBone > -255 && m_nForceBone < 256 );

		bool bDebug = showhitlocation.GetBool();

		switch ( ptr->hitgroup )
		{
		case HITGROUP_GENERIC:
			if( bDebug ) Msg("Hit Location: Generic\n");
			break;

		// hit gear, react but don't bleed
		case HITGROUP_GEAR:
			subInfo.SetDamage( 0.01 );
			ptr->hitgroup = HITGROUP_GENERIC;
			if( bDebug ) Msg("Hit Location: Gear\n");
			break;

		case HITGROUP_HEAD:
			subInfo.ScaleDamage( sk_npc_head.GetFloat() );
			if( bDebug ) Msg("Hit Location: Head\n");
			break;
		case HITGROUP_CHEST:
			subInfo.ScaleDamage( sk_npc_chest.GetFloat() );
			if( bDebug ) Msg("Hit Location: Chest\n");
			break;
		case HITGROUP_STOMACH:
			subInfo.ScaleDamage( sk_npc_stomach.GetFloat() );
			if( bDebug ) Msg("Hit Location: Stomach\n");
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			subInfo.ScaleDamage( sk_npc_arm.GetFloat() );
			if( bDebug ) Msg("Hit Location: Left/Right Arm\n");
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			subInfo.ScaleDamage( sk_npc_leg.GetFloat() );
			if( bDebug ) Msg("Hit Location: Left/Right Leg\n");
			break;
		default:
			if( bDebug ) Msg("Hit Location: UNKNOWN\n");
			break;
		}

		if ( subInfo.GetDamage() >= 1.0 && !(subInfo.GetDamageType() & DMG_SHOCK ) )
		{
			// Bleed, but don't bleed if shocked.
			if ( ptr->hitgroup != HITGROUP_HEAD || m_iHealth - subInfo.GetDamage() <= 0 )
			{
				SpawnBlood( ptr->endpos, BloodColor(), subInfo.GetDamage() );// a little surface blood.
				TraceBleed( subInfo.GetDamage(), vecDir, ptr, subInfo.GetDamageType() );
			}
			else
				m_fNoDamageDecal = true;
		}

		subInfo.SetInflictor( info.GetAttacker() );
		AddMultiDamage( subInfo, this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Checks if player is in spread angle between source and target Pos
//			Used to prevent friendly fire
// Input  : Source of attack, target position, spread angle
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::PlayerInSpread( const Vector &sourcePos, const Vector &targetPos, float flSpread, bool ignoreHatedPlayers )
{
	Vector toTarget		= targetPos - sourcePos;
	float  distTarget	= VectorNormalize(toTarget);

	// loop through all players
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !ignoreHatedPlayers || IRelationType( pPlayer ) != D_HT )
		{
			Vector toPlayer   = pPlayer->WorldSpaceCenter() - sourcePos;
			float  distPlayer = VectorNormalize(toPlayer);
			float  dotProduct = DotProduct(toTarget,toPlayer);
			if (dotProduct > flSpread)
			{
				// Only reject if target is on other side of player
				if (distTarget > distPlayer)
				{
					return true;
				}
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if player is in range of given location.  Used by NPCs
//			to prevent friendly fire
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *CAI_BaseNPC::PlayerInRange( const Vector &vecLocation, float flDist )
{
	// loop through all players
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );



		if (pPlayer && (vecLocation - pPlayer->WorldSpaceCenter() ).Length2D() <= flDist)
		{
			return pPlayer;
		}
	}
	return NULL;
}


#define BULLET_WIZZDIST	80.0
#define SLOPE ( -1.0 / BULLET_WIZZDIST )

void BulletWizz( Vector vecSrc, Vector vecEndPos, edict_t *pShooter, bool isTracer )
{
	CBasePlayer *pPlayer;
	Vector vecBulletPath;
	Vector vecPlayerPath;
	Vector vecBulletDir;
	Vector vecNearestPoint;
	float flDist;
	float flBulletDist;

	vecBulletPath = vecEndPos - vecSrc;
	vecBulletDir = vecBulletPath;
	VectorNormalize(vecBulletDir);

	// see how near this bullet passed by player in a single player game
	// for multiplayer, we need to go through the list of clients.
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		// Don't hear one's own bullets
		if( pPlayer->pev == pShooter )
			continue;

		vecPlayerPath = pPlayer->EarPosition() - vecSrc;
		flDist = DotProduct( vecPlayerPath, vecBulletDir );
		vecNearestPoint = vecSrc + vecBulletDir * flDist;
		// FIXME: minus m_vecViewOffset?
		flBulletDist = ( vecNearestPoint - pPlayer->EarPosition() ).Length();

		if( flBulletDist <= BULLET_WIZZDIST )
		{
#ifdef OLD_BULLET_WIZZ
			float Volume = SLOPE * flBulletDist + 1.0;

			CSingleUserRecipientFilter filter( pPlayer );

			CBaseEntity::EmitSound( filter, pPlayer->entindex(), CHAN_STATIC,
				g_sNearMissSounds[random->RandomInt(0, NUM_NEARMISSES - 1) ], Volume, ATTN_IDLE,
				0, random->RandomInt( 95, 105 ), &vecNearestPoint );
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Hits triggers with raycasts
//-----------------------------------------------------------------------------
class CTriggerTraceEnum : public IEntityEnumerator
{
public:
	CTriggerTraceEnum( Ray_t *pRay, const CTakeDamageInfo &info, const Vector& dir, int contentsMask ) :
		m_info( info ),	m_VecDir(dir), m_ContentsMask(contentsMask), m_pRay(pRay)
	{
	}

	virtual bool EnumEntity( IHandleEntity *pHandleEntity )
	{
		trace_t tr;

		CBaseEntity *pEnt = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );

		// Done to avoid hitting an entity that's both solid & a trigger.
		if ( pEnt->IsSolid() )
			return true;

		enginetrace->ClipRayToEntity( *m_pRay, m_ContentsMask, pHandleEntity, &tr );
		if (tr.fraction < 1.0f)
		{
			pEnt->DispatchTraceAttack( m_info, m_VecDir, &tr );
			ApplyMultiDamage();
		}

		return true;
	}

private:
	Vector m_VecDir;
	int m_ContentsMask;
	Ray_t *m_pRay;
	CTakeDamageInfo m_info;
};

void CBaseEntity::TraceAttackToTriggers( const CTakeDamageInfo &info, const Vector& start, const Vector& end, const Vector& dir )
{
	Ray_t ray;
	ray.Init( start, end );

	CTriggerTraceEnum triggerTraceEnum( &ray, info, dir, MASK_SHOT );
	enginetrace->EnumerateEntities( ray, true, &triggerTraceEnum );
}



//-----------------------------------------------------------------------------
// Purpose: Virtual function allows entities to handle tracer presentation
//			as they see fit.
//
// Input  : vecTracerSrc - the point at which to start the tracer (not always the
//			same spot as the traceline!
//
//			tr - the entire trace result for the shot.
//
// Output :
//-----------------------------------------------------------------------------
void CBaseEntity::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	CPASFilter filter( vecTracerSrc );

	switch ( iTracerType )
	{
	case TRACER_LINE:
		{
			UTIL_Tracer( vecTracerSrc, tr.endpos, ENTINDEX( edict() ) );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_BaseNPC::FireBullets( int cShots, const Vector &vecSrc, const Vector &vecDirShooting, const Vector &vecSpread, float flDistance, int iBulletType, int iTracerFreq, int firingEntID, int attachmentID,int iDamage, CBaseEntity *pAttacker, bool bFirstShotAccurate )
{
#ifdef HL2_DLL
	// If we're shooting at a bullseye, become perfectly accurate if the bullseye demands it
	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
	{
		CNPC_Bullseye *pBullseye = dynamic_cast<CNPC_Bullseye*>(GetEnemy()); 
		if ( pBullseye && pBullseye->UsePerfectAccuracy() )
		{
			BaseClass::FireBullets( cShots, vecSrc, vecDirShooting, vec3_origin, flDistance, iBulletType, iTracerFreq, firingEntID, attachmentID,iDamage, pAttacker, bFirstShotAccurate );
			return;
		}
	}
#endif

	BaseClass::FireBullets( cShots, vecSrc, vecDirShooting, vecSpread, flDistance, iBulletType, iTracerFreq, firingEntID, attachmentID,iDamage, pAttacker, bFirstShotAccurate );
}

//---------------------------------------------------------
// Caches off a shot direction and allows you to perform
// various operations on it without having to recalculate
// vecRight and vecUp each time. 
//---------------------------------------------------------
class CShotManipulator
{
public:
	CShotManipulator( const Vector &vecForward )
	{
		m_vecShotDirection = vecForward;
		VectorVectors( m_vecShotDirection, m_vecRight, m_vecUp );
	};

	const Vector &ApplySpread( const Vector &vecSpread );

	const Vector &GetShotDirection()	{ return m_vecShotDirection; }
	const Vector &GetResult()			{ return m_vecResult; }
	const Vector &GetRightVector()		{ return m_vecRight; }
	const Vector &GetUpVector()			{ return m_vecUp;}

private:
	Vector m_vecShotDirection;
	Vector m_vecRight;
	Vector m_vecUp;
	Vector m_vecResult;
};

//---------------------------------------------------------
// Take a vector (direction) and another vector (spread) 
// and modify the direction to point somewhere within the 
// spread. This used to live inside FireBullets.
//---------------------------------------------------------
const Vector &CShotManipulator::ApplySpread( const Vector &vecSpread )
{
	// get circular gaussian spread
	float x, y, z;

	do
	{
		x = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
		y = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
		z = x*x+y*y;
	} while (z > 1);

	m_vecResult = m_vecShotDirection + x * vecSpread.x * m_vecRight + y * vecSpread.y * m_vecUp;

	return m_vecResult;
}


/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/

void CBaseEntity::FireBullets(int cShots, const Vector &vecSrc,
	const Vector &vecDirShooting, const Vector &vecSpread, float flDistance,
	int iAmmoType, int iTracerFreq, int firingEntID, int attachmentID,
	int iDamage, CBaseEntity *pAttacker, bool bFirstShotAccurate )
{
	static int	tracerCount;
	bool		tracer;
	trace_t		tr;
	CAmmoDef*	pAmmoDef	= GetAmmoDef();
	int			nDamageType	= pAmmoDef->DamageType(iAmmoType);
	bool		bHitWater = false;

	if ( pAttacker == NULL )
	{
		pAttacker = this;  // the default attacker is ourselves
	}

	ClearMultiDamage();
	g_MultiDamage.SetDamageType( nDamageType | DMG_NEVERGIB );

	//-----------------------------------------------------
	// Set up our shot manipulator.
	//-----------------------------------------------------
	CShotManipulator Manipulator( vecDirShooting );

	for (int iShot = 0; iShot < cShots; iShot++)
	{
		Vector vecDir;
		Vector vecEnd;

		// If we're firing multiple shots, and the first shot has to be bang on target, ignore spread
		if ( iShot == 0 && cShots > 1 && bFirstShotAccurate )
		{
			vecDir = Manipulator.GetShotDirection();
		}
		else
		{
			vecDir = Manipulator.ApplySpread( vecSpread );
		}

		vecEnd = vecSrc + vecDir * flDistance;

		AI_TraceLine(vecSrc, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

#if 0
		if( Classify() == CLASS_COMBINE || Classify() == CLASS_MILITARY || Classify() == CLASS_METROPOLICE )
		{
			NDebugOverlay::Line( vecSrc, tr.endpos, 255, 0, 0, false, 0.5 );
		}
		else if( IsPlayer() )
		{
			NDebugOverlay::Line( vecSrc, tr.endpos, 0, 0, 255, false, 0.5 );
		}
		else
		{
			NDebugOverlay::Line( vecSrc, tr.endpos, 0, 255, 0, false, 0.5 );
		}
#endif

		// Now hit all triggers along the ray that respond to shots...
		// Clip the ray to the first collided solid returned from traceline
		CTakeDamageInfo triggerInfo( pAttacker, pAttacker, iDamage, nDamageType );
		CalculateBulletDamageForce( &triggerInfo, iAmmoType, vecDir, tr.endpos );
		TraceAttackToTriggers( triggerInfo, tr.startpos, tr.endpos, vecDir );

		tracer = false;
		if (iTracerFreq != 0 && (tracerCount++ % iTracerFreq) == 0)
		{
			Vector vecTracerSrc = vec3_origin;

			if ( IsPlayer() )
			{// adjust tracer position for player
				Vector forward, right;
				CBasePlayer *pPlayer = ToBasePlayer( this );
				pPlayer->EyeVectors( &forward, &right, NULL );
				vecTracerSrc = vecSrc + Vector ( 0 , 0 , -4 ) + right * 2 + forward * 16;
			}
			else
			{
				vecTracerSrc = vecSrc;
			}

			if ( iTracerFreq != 1 )		// guns that always trace also always decal
			{
				tracer = true;
			}

			MakeTracer( vecTracerSrc, tr, pAmmoDef->TracerType(iAmmoType) );
		}

		// Make sure given a valid bullet type
		if (iAmmoType == -1)
		{
			Msg("ERROR: Undefined ammo type!\n");
			return;
		}

		// do damage, paint decals
		if (tr.fraction != 1.0)
		{
			CBaseEntity *pEntity = tr.m_pEnt;

			CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, tr.endpos, 200, 0.2, this );

			//See if the bullet ended up underwater
			if ( enginetrace->GetPointContents( tr.endpos ) & CONTENTS_WATER )
			{
				trace_t	waterTrace;

				//Trace again with water enabled
				AI_TraceLine( vecSrc, vecEnd, (MASK_SHOT|CONTENTS_WATER), this, COLLISION_GROUP_NONE, &waterTrace );
				
				//See if this is the point we entered
				if ( enginetrace->GetPointContents( waterTrace.endpos - Vector(0,0,0.1f) ) & CONTENTS_WATER )
				{
					CEffectData	data;

					data.m_vOrigin = waterTrace.endpos;
					data.m_vNormal = waterTrace.plane.normal;
					data.m_flScale = random->RandomFloat( 4.0f, 8.0f );
		
					DispatchEffect( "gunshotsplash", data );
					bHitWater = true;
				}
			}

			if ( iDamage )
			{
				// Damage specified by function parameter
				CTakeDamageInfo dmgInfo( this, pAttacker, iDamage, nDamageType | ((iDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB ) );
				CalculateBulletDamageForce( &dmgInfo, iAmmoType, vecDir, tr.endpos );
				pEntity->DispatchTraceAttack( dmgInfo, vecDir, &tr );
				
				if ( bHitWater == false )
				{
					UTIL_ImpactTrace( &tr, pAmmoDef->DamageType(iAmmoType) );
				}
			}
			else
			{
				// Damage is taken from the ammo type.
				float flDamage;

				if( pAmmoDef->DamageType(iAmmoType) & DMG_SNIPER )
				{
					// If this damage is from a SNIPER, we do damage based on what the bullet
					// HITS, not who fired it. All other bullets have their damage values
					// arranged according to the owner of the bullet, not the recipient.
					if( pEntity->IsPlayer() )
					{
						// Player
						flDamage = pAmmoDef->PlrDamage(iAmmoType);
					}
					else
					{
						// NPC or breakable
						flDamage = pAmmoDef->NPCDamage(iAmmoType);
					}
				}
				else
				{
					if (IsPlayer())
					{
						flDamage = pAmmoDef->PlrDamage(iAmmoType);
					}
					else
					{
						flDamage = pAmmoDef->NPCDamage(iAmmoType);
					}
				}

				CTakeDamageInfo dmgInfo( pAttacker, pAttacker, flDamage, nDamageType );
				CalculateBulletDamageForce( &dmgInfo, iAmmoType, vecDir, tr.endpos );
				pEntity->DispatchTraceAttack( dmgInfo, vecDir, &tr );

				if ( bHitWater == false )
				{
					//!!!HACKHACK E3 2002 (sjb)
					// The gunship wants a large impact effect.
					if( Classify() == CLASS_COMBINE_GUNSHIP )
					{
						UTIL_ImpactTrace( &tr, pAmmoDef->DamageType(iAmmoType), "ImpactGunship" );
					}
					else
					{
						UTIL_ImpactTrace( &tr, pAmmoDef->DamageType(iAmmoType) );
					}

					DoCustomImpactEffect( tr );
				}
			}
		}
		// make bullet trails
		//FIXME: Removed for E3 - jdw
		//UTIL_BubbleTrail( vecSrc, tr.endpos, (flDistance * tr.fraction) / 64.0 );
	}

	ApplyMultiDamage();
}


void CBaseEntity::TraceBleed( float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType )
{
	if ((BloodColor() == DONT_BLEED) || (BloodColor() == BLOOD_COLOR_MECH))
	{
		return;
	}

	if (flDamage == 0)
		return;

	if (! (bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		return;

	// make blood decal on the wall!
	trace_t Bloodtr;
	Vector vecTraceDir;
	float flNoise;
	int cCount;
	int i;


	if ( !IsAlive() )
	{
		// dealing with a dead npc.
		if ( m_iMaxHealth <= 0 )
		{
			// no blood decal for a npc that has already decalled its limit.
			return;
		}
		else
		{
			m_iMaxHealth -= 1;
		}
	}

	if (flDamage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (flDamage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	for ( i = 0 ; i < cCount ; i++ )
	{
		vecTraceDir = vecDir * -1;// trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += random->RandomFloat( -flNoise, flNoise );
		vecTraceDir.y += random->RandomFloat( -flNoise, flNoise );
		vecTraceDir.z += random->RandomFloat( -flNoise, flNoise );

		AI_TraceLine( ptr->endpos, ptr->endpos + vecTraceDir * -172, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &Bloodtr);

		if ( Bloodtr.fraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}

//=========================================================
//=========================================================
void CAI_BaseNPC::MakeDamageBloodDecal ( int cCount, float flNoise, trace_t *ptr, Vector vecDir )
{
	// make blood decal on the wall!
	trace_t Bloodtr;
	Vector vecTraceDir;
	int i;

	if ( !IsAlive() )
	{
		// dealing with a dead npc.
		if ( m_iMaxHealth <= 0 )
		{
			// no blood decal for a npc that has already decalled its limit.
			return;
		}
		else
		{
			m_iMaxHealth -= 1;
		}
	}

	for ( i = 0 ; i < cCount ; i++ )
	{
		vecTraceDir = vecDir;

		vecTraceDir.x += random->RandomFloat( -flNoise, flNoise );
		vecTraceDir.y += random->RandomFloat( -flNoise, flNoise );
		vecTraceDir.z += random->RandomFloat( -flNoise, flNoise );

		AI_TraceLine( ptr->endpos, ptr->endpos + vecTraceDir * 172, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &Bloodtr);

		if ( Bloodtr.fraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}

//
// fade out - slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER!
// SET A FUTURE THINK AND A RENDERMODE!!
void CBaseEntity::SUB_StartFadeOut ( void )
{
	if (m_nRenderMode == kRenderNormal)
	{
		SetRenderColorA( 255 );
		m_nRenderMode = kRenderTransTexture;
	}

	AddSolidFlags( FSOLID_NOT_SOLID );
	SetLocalAngularVelocity( vec3_angle );
	Relink();

	SetNextThink( gpGlobals->curtime + MIN_CORPSE_FADE_TIME );
	SetThink ( &CBaseEntity::SUB_FadeOut );
}

//-----------------------------------------------------------------------------
// Purpose: Vanish when players aren't looking
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_Vanish( void )
{
	//Always think again next frame
	SetNextThink( gpGlobals->curtime + 0.1f );

	CBasePlayer *pPlayer;

	//Get all players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		//Get the next client
		if ( ( pPlayer = UTIL_PlayerByIndex( i ) ) != NULL )
		{
			Vector corpseDir = (GetAbsOrigin() - pPlayer->WorldSpaceCenter() );

			float flDistSqr = corpseDir.LengthSqr();
			//If the player is close enough, don't fade out
			if ( flDistSqr < (MIN_CORPSE_FADE_DIST*MIN_CORPSE_FADE_DIST) )
				return;

			// If the player's far enough away, we don't care about looking at it
			if ( flDistSqr < (MAX_CORPSE_FADE_DIST*MAX_CORPSE_FADE_DIST) )
			{
				VectorNormalize( corpseDir );

				Vector	plForward;
				pPlayer->EyeVectors( &plForward );

				float dot = plForward.Dot( corpseDir );

				if ( dot > 0.0f )
					return;
			}
		}
	}

	//If we're here, then we can vanish safely
	SetThink( &CBaseEntity::SUB_Remove );
}

//-----------------------------------------------------------------------------
// Purpose: Fade out slowly
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_FadeOut( void  )
{
	if ( m_clrRender->a > 7 )
	{
		SetRenderColorA( GetRenderColor().a - 7 );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
		SetRenderColorA( 0 );
		SetNextThink( gpGlobals->curtime + 0.2 );
		SetThink ( &CBaseEntity::SUB_Remove );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn some blood particles
//-----------------------------------------------------------------------------
void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage)
{
	UTIL_BloodDrips( vecSpot, g_vecAttackDir, bloodColor, (int)flDamage );
}


//---------------------------------------------------------
//---------------------------------------------------------
#define InterruptFromCondition( iCondition ) \
	AI_RemapFromGlobal( ( AI_IdIsLocal( iCondition ) ? GetClassScheduleIdSpace()->ConditionLocalToGlobal( iCondition ) : iCondition ) )
	
void CAI_BaseNPC::SetCondition( int iCondition )
{
	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return;
	}
	
	m_Conditions.SetBit( interrupt );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_BaseNPC::HasCondition( int iCondition )
{
	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return false;
	}
	
	bool bReturn = m_Conditions.GetBit(interrupt);
	return (bReturn);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_BaseNPC::ClearCondition( int iCondition )
{
	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return;
	}
	
	m_Conditions.ClearBit(interrupt);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_BaseNPC::ClearConditions( int *pConditions, int nConditions )
{
	for ( int i = 0; i < nConditions; ++i )
	{
		int iCondition = pConditions[i];
		int interrupt = InterruptFromCondition( iCondition );
		
		if ( interrupt == -1 )
		{
			Assert(0);
			continue;
		}
		
		m_Conditions.ClearBit( interrupt );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_BaseNPC::SetIgnoreConditions( int *pConditions, int nConditions )
{
	for ( int i = 0; i < nConditions; ++i )
	{
		int iCondition = pConditions[i];
		int interrupt = InterruptFromCondition( iCondition );
		
		if ( interrupt == -1 )
		{
			Assert(0);
			continue;
		}
		
		m_InverseIgnoreConditions.ClearBit( interrupt ); // clear means ignore
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_BaseNPC::HasInterruptCondition( int iCondition )
{
	if( !GetCurSchedule() )
	{
		return false;
	}

	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return false;
	}
	return ( m_Conditions.GetBit( interrupt ) && GetCurSchedule()->HasInterrupt( interrupt ) );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_BaseNPC::ConditionInterruptsCurSchedule( int iCondition )
{
	if( !GetCurSchedule() )
	{
		return false;
	}

	int interrupt = InterruptFromCondition( iCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return false;
	}
	return ( GetCurSchedule()->HasInterrupt( interrupt ) );
}


//-----------------------------------------------------------------------------
// Purpose: Sets a flag in the custom interrupt flags, translating the condition
//			to the proper global space, if necessary
//-----------------------------------------------------------------------------
void CAI_BaseNPC::SetCustomInterruptCondition( int nCondition )
{
	int interrupt = InterruptFromCondition( nCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return;
	}
	
	m_CustomInterruptConditions.SetBit( interrupt );
}

//-----------------------------------------------------------------------------
// Purpose: Clears a flag in the custom interrupt flags, translating the condition
//			to the proper global space, if necessary
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ClearCustomInterruptCondition( int nCondition )
{
	int interrupt = InterruptFromCondition( nCondition );
	
	if ( interrupt == -1 )
	{
		Assert(0);
		return;
	}
	
	m_CustomInterruptConditions.ClearBit( interrupt );
}

//-----------------------------------------------------------------------------

void CAI_BaseNPC::SetDistLook( float flDistLook )
{
	m_pSenses->SetDistLook( flDistLook );
}

//-----------------------------------------------------------------------------

void CAI_BaseNPC::OnLooked( int iDistance )
{
	// DON'T let visibility information from last frame sit around!
	static int conditionsToClear[] =
	{
		COND_SEE_HATE,
		COND_SEE_DISLIKE,
		COND_SEE_ENEMY,
		COND_SEE_FEAR,
		COND_SEE_NEMESIS,
		COND_SEE_PLAYER,
	};

	ClearConditions( conditionsToClear, ARRAYSIZE( conditionsToClear ) );

	AISightIter_t iter;
	CBaseEntity *pSightEnt;

	pSightEnt = GetSenses()->GetFirstSeenEntity( &iter );

	while( pSightEnt )
	{
		if ( pSightEnt->IsPlayer() )
		{
			// if we see a client, remember that (mostly for scripted AI)
			SetCondition(COND_SEE_PLAYER);
		}

		Disposition_t relation = IRelationType( pSightEnt );

		// the looker will want to consider this entity
		// don't check anything else about an entity that can't be seen, or an entity that you don't care about.
		if ( relation != D_NU )
		{
			if ( pSightEnt == GetEnemy() )
			{
				// we know this ent is visible, so if it also happens to be our enemy, store that now.
				SetCondition(COND_SEE_ENEMY);
			}

			// don't add the Enemy's relationship to the conditions. We only want to worry about conditions when
			// we see npcs other than the Enemy.
			switch ( relation )
			{
			case D_HT:
				{
					int priority = IRelationPriority( pSightEnt );
					if (priority < 0)
					{
						SetCondition(COND_SEE_DISLIKE);
					}
					else if (priority > 10)
					{
						SetCondition(COND_SEE_NEMESIS);
					}
					else
					{
						SetCondition(COND_SEE_HATE);
					}
					UpdateEnemyMemory(pSightEnt,pSightEnt->GetAbsOrigin());
					break;

				}
			case D_FR:
				UpdateEnemyMemory(pSightEnt,pSightEnt->GetAbsOrigin());
				SetCondition(COND_SEE_FEAR);
				break;
			case D_LI:
			case D_NU:
				break;
			default:
				DevWarning( 2, "%s can't assess %s\n", GetClassname(), pSightEnt->GetClassname() );
				break;
			}
		}

		pSightEnt = GetSenses()->GetNextSeenEntity( &iter );
	}
}

//-----------------------------------------------------------------------------

void CAI_BaseNPC::OnListened()
{
	AISoundIter_t iter;

	CSound *pCurrentSound;

	static int conditionsToClear[] =
	{
		COND_HEAR_DANGER,
		COND_HEAR_COMBAT,
		COND_HEAR_WORLD,
		COND_HEAR_PLAYER,
		COND_HEAR_THUMPER,
		COND_HEAR_BUGBAIT,
		COND_HEAR_PHYSICS_DANGER,
		COND_HEAR_BULLET_IMPACT,
		COND_SMELL,
	};

	ClearConditions( conditionsToClear, ARRAYSIZE( conditionsToClear ) );

	pCurrentSound = GetSenses()->GetFirstHeardSound( &iter );

	while ( pCurrentSound )
	{
		// the npc cares about this sound, and it's close enough to hear.
		int condition = COND_NONE;

		if ( pCurrentSound->FIsSound() )
		{
			// this is an audible sound.
			switch( pCurrentSound->SoundType() )
			{
				case SOUND_DANGER:			condition = COND_HEAR_DANGER;			break;
				case SOUND_THUMPER:			condition = COND_HEAR_THUMPER;			break;
				case SOUND_BUGBAIT:			condition = COND_HEAR_BUGBAIT;			break;
				case SOUND_COMBAT:			condition = COND_HEAR_COMBAT;			break;
				case SOUND_WORLD:			condition = COND_HEAR_WORLD;			break;
				case SOUND_PLAYER:			condition = COND_HEAR_PLAYER;			break;
				case SOUND_BULLET_IMPACT:	condition = COND_HEAR_BULLET_IMPACT;	break;
				case SOUND_PHYSICS_DANGER:	condition = COND_HEAR_PHYSICS_DANGER;	break;

				default:
					Msg( "**ERROR: Monster %s hearing sound of unknown type %d!\n", GetClassname(), pCurrentSound->SoundType() );
					break;
			}
		}
		else
		{
			// if not a sound, must be a smell - determine if it's just a scent, or if it's a food scent
			condition = COND_SMELL;
		}

		if ( condition != COND_NONE )
			SetCondition( condition );

		pCurrentSound = GetSenses()->GetNextHeardSound( &iter );
	}

	// Sound outputs
	if ( HasCondition( COND_HEAR_WORLD ) )
	{
		m_OnHearWorld.FireOutput(this, this);
	}

	if ( HasCondition( COND_HEAR_PLAYER ) )
	{
		m_OnHearPlayer.FireOutput(this, this);
	}

	if ( HasCondition( COND_HEAR_COMBAT ) ||
		 HasCondition( COND_HEAR_BULLET_IMPACT ) ||
		 HasCondition( COND_HEAR_DANGER ) )
	{
		m_OnHearCombat.FireOutput(this, this);
	}
}

//=========================================================
// FValidateHintType - tells use whether or not the npc cares
// about the type of Hint Node given
//=========================================================
bool CAI_BaseNPC::FValidateHintType ( CAI_Hint *pHint )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Override in subclasses to associate specific hint types
//			with activities
//-----------------------------------------------------------------------------
Activity CAI_BaseNPC::GetHintActivity( short sHintType )
{
	return ACT_IDLE;
}

//-----------------------------------------------------------------------------
// Purpose: Override in subclasses to give specific hint types delays
//			before they can be used again
// Input  :
// Output :
//-----------------------------------------------------------------------------
float	CAI_BaseNPC::GetHintDelay( short sHintType )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:	Return incoming grenade if spotted
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseGrenade* CAI_BaseNPC::IncomingGrenade(void)
{
	int				iDist;

	AIEnemiesIter_t iter;
	for( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter) )
	{
		CBaseGrenade* pBG = dynamic_cast<CBaseGrenade*>((CBaseEntity*)pEMemory->hEnemy);

		// Make sure this memory is for a grenade and grenade is not dead
		if (!pBG || pBG->m_lifeState == LIFE_DEAD)
			continue;

		// Make sure it's visible
		if (!FVisible(pBG))
			continue;

		// Check if it's near me
		iDist = ( pBG->GetAbsOrigin() - GetAbsOrigin() ).Length();
		if ( iDist <= NPC_GRENADE_FEAR_DIST )
			return pBG;

		// Check if it's headed towards me
		Vector	vGrenadeDir = GetAbsOrigin() - pBG->GetAbsOrigin();
		Vector  vGrenadeVel;
		pBG->GetVelocity( &vGrenadeVel, NULL );
		VectorNormalize(vGrenadeDir);
		VectorNormalize(vGrenadeVel);
		float	flDotPr		= DotProduct(vGrenadeDir, vGrenadeVel);
		if (flDotPr > 0.85)
			return pBG;
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Check my physical state with the environment
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::TryRestoreHull(void)
{
	if ( GetCurSchedule() &&
		GetCurSchedule()->GetId() != SCHED_GIVE_WAY &&
		IsUsingSmallHull() )
	{

		trace_t tr;
		Vector	vUpBit = GetAbsOrigin();
		vUpBit.z += 1;

		AI_TraceHull( GetAbsOrigin(), vUpBit, GetHullMins(),
			GetHullMaxs(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
		if ( !tr.startsolid && (tr.fraction == 1.0) )
		{
			SetHullSizeNormal();
		}
	}
}

//=========================================================
//=========================================================
int CAI_BaseNPC::GetSoundInterests( void )
{
	return SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER;
}

//---------------------------------------------------------
// Return the loudest sound of this type in the sound list
//---------------------------------------------------------
CSound *CAI_BaseNPC::GetLoudestSoundOfType( int iType )
{
	return CSoundEnt::GetLoudestSoundOfType( iType, EarPosition() );
}

//=========================================================
// GetBestSound - returns a pointer to the sound the npc
// should react to. Right now responds only to nearest sound.
//=========================================================
CSound* CAI_BaseNPC::GetBestSound( void )
{
	CSound *pResult = GetSenses()->GetClosestSound();
	if ( pResult == NULL)
		Msg( "Warning: NULL Return from GetBestSound\n" ); // condition previously set now no longer true. Have seen this when play too many sounds...
	return pResult;
}

//=========================================================
// PBestScent - returns a pointer to the scent the npc
// should react to. Right now responds only to nearest scent
//=========================================================
CSound* CAI_BaseNPC::GetBestScent( void )
{
	CSound *pResult = GetSenses()->GetClosestSound( true );
	if ( pResult == NULL)
		Msg("Warning: NULL Return from GetBestScent\n" );
	return pResult;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if target is in legal range of eye movements
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ValidEyeTarget(const Vector &lookTargetPos)
{
	Vector vHeadDir = HeadDirection3D( );
	Vector lookTargetDir	= lookTargetPos - EyePosition();
	VectorNormalize(lookTargetDir);

	// Only look if it doesn't crank my eyeballs too far
	float dotPr = DotProduct(lookTargetDir, vHeadDir);
	if (dotPr > 0.7)
	{
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Integrate head turn over time
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::SetHeadDirection( const Vector &vTargetPos, float flInterval)
{
	if (!(CapabilitiesGet() & bits_CAP_TURN_HEAD))
		return;

#ifdef DEBUG_LOOK
	// Draw line in body, head, and eye directions
	Vector vEyePos = EyePosition();
	Vector vHeadDir;
	HeadDirection3D(&vHeadDir);
	Vector vBodyDir;
	BodyDirection2D(&vBodyDir);

	//UNDONE <<TODO>>
	// currently eye dir just returns head dir, so use vTargetPos for now
	//Vector vEyeDir;	w
	//EyeDirection3D(&vEyeDir);
	NDebugOverlay::Line( vEyePos, vEyePos+(50*vHeadDir), 255, 0, 0, false, 0.1 );
	NDebugOverlay::Line( vEyePos, vEyePos+(50*vBodyDir), 0, 255, 0, false, 0.1 );
	NDebugOverlay::Line( vEyePos, vTargetPos, 0, 0, 255, false, 0.1 );
#endif

	//--------------------------------------
	// Set head yaw
	//--------------------------------------
	float flDesiredYaw = VecToYaw(vTargetPos - GetLocalOrigin()) - GetLocalAngles().y;
	if (flDesiredYaw > 180)
		flDesiredYaw -= 360;
	if (flDesiredYaw < -180)
		flDesiredYaw += 360;

	float	iRate	 = 0.8;

	// Make frame rate independent
	float timeToUse = flInterval;
	while (timeToUse > 0)
	{
		m_flHeadYaw	   = (iRate * m_flHeadYaw) + (1-iRate)*flDesiredYaw;
		timeToUse -= 0.1;
	}
	if (m_flHeadYaw > 360) m_flHeadYaw = 0;

	m_flHeadYaw = SetBoneController( 0, m_flHeadYaw );


	//--------------------------------------
	// Set head pitch
	//--------------------------------------
	Vector	vEyePosition	= EyePosition();
	float	fTargetDist		= (vTargetPos - vEyePosition).Length();
	float	fVertDist		= vTargetPos.z - vEyePosition.z;
	float	flDesiredPitch	= -RAD2DEG(atan(fVertDist/fTargetDist));

	// Make frame rate independent
	timeToUse = flInterval;
	while (timeToUse > 0)
	{
		m_flHeadPitch	   = (iRate * m_flHeadPitch) + (1-iRate)*flDesiredPitch;
		timeToUse -= 0.1;
	}
	if (m_flHeadPitch > 360) m_flHeadPitch = 0;

	SetBoneController( 1, m_flHeadPitch );

}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's eye direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseNPC::EyeDirection2D( void )
{
	// UNDONE
	// For now just return head direction....
	return HeadDirection2D( );
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's eye direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseNPC::EyeDirection3D( void )
{
	// UNDONE //<<TODO>>
	// For now just return head direction....
	return HeadDirection3D( );
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's head direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseNPC::HeadDirection2D( void )
{
	// UNDONE <<TODO>>
	// This does not account for head rotations in the animation,
	// only those done via bone controllers

	// Head yaw is in local cooridnate so it must be added to the body's yaw
	QAngle bodyAngles = BodyAngles();
	float flWorldHeadYaw	= m_flHeadYaw + bodyAngles.y;

	// Convert head yaw into facing direction
	return UTIL_YawToVector( flWorldHeadYaw );
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's head direction in 3D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseNPC::HeadDirection3D( void )
{
	Vector vHeadDirection;

	// UNDONE <<TODO>>
	// This does not account for head rotations in the animation,
	// only those done via bone controllers

	// Head yaw is in local cooridnate so it must be added to the body's yaw
	QAngle bodyAngles = BodyAngles();
	float	flWorldHeadYaw	= m_flHeadYaw + bodyAngles.y;

	// Convert head yaw into facing direction
	AngleVectors( QAngle( m_flHeadPitch, flWorldHeadYaw, 0), &vHeadDirection );
	return vHeadDirection;
}


//-----------------------------------------------------------------------------
// Purpose: Look at other NPCs and clients from time to time
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *CAI_BaseNPC::EyeLookTarget( void )
{
	if (m_flNextEyeLookTime < gpGlobals->curtime)
	{
		CBaseEntity*	pBestEntity = NULL;
		float			fBestDist	= MAX_COORD_RANGE;
		float			fTestDist;

		CBaseEntity *pEntity = NULL;

		for ( CEntitySphereQuery sphere( GetAbsOrigin(), 1024, 0 ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
		{
			if (pEntity == this)
			{
				continue;
			}
			CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
			if (pNPC || (pEntity->GetFlags() & FL_CLIENT))
			{
				fTestDist = (GetAbsOrigin() - pEntity->EyePosition()).Length();
				if (fTestDist < fBestDist)
				{
					if (ValidEyeTarget(pEntity->EyePosition()))
					{
						fBestDist	= fTestDist;
						pBestEntity	= pEntity;
					}
				}
			}
		}
		if (pBestEntity)
		{
			m_flNextEyeLookTime = gpGlobals->curtime + random->RandomInt(1,5);
			m_hEyeLookTarget = pBestEntity;
		}
	}
	return m_hEyeLookTarget;
}

//-----------------------------------------------------------------------------
// Purpose: Set direction that the NPC aiming thier gun
//-----------------------------------------------------------------------------

void CAI_BaseNPC::SetAim( const Vector &aimDir )
{
	QAngle angDir;
	VectorAngles( aimDir, angDir );

	float curPitch = GetPoseParameter( "aim_pitch" );
	float curYaw = GetPoseParameter( "aim_yaw" );

	// clamp and dampen movement
	float newPitch = curPitch + 0.5 * UTIL_AngleDiff( UTIL_ApproachAngle( angDir.x, curPitch, 60 ), curPitch );
	float newYaw = curYaw + 0.5 * UTIL_AngleDiff( UTIL_ApproachAngle( UTIL_AngleDiff( angDir.y, GetMotor()->GetIdealYaw( )), curYaw, 60 ), curYaw );

	SetPoseParameter( "aim_pitch", newPitch );
	SetPoseParameter( "aim_yaw", newYaw );
	// Msg("aim %.0f %0.f\n", newPitch, newYaw ); 
}


void CAI_BaseNPC::RelaxAim( )
{
	float curPitch = GetPoseParameter( "aim_pitch" );
	float curYaw = GetPoseParameter( "aim_yaw" );

	// dampen existing aim
	float newPitch = UTIL_ApproachAngle( 0, curPitch, 3 );
	float newYaw = UTIL_ApproachAngle( 0, curYaw, 2 );

	SetPoseParameter( "aim_pitch", newPitch );
	SetPoseParameter( "aim_yaw", newYaw );
	// Msg("aim %.0f %0.f\n", newPitch, newYaw ); 
}

//-----------------------------------------------------------------------------
void CAI_BaseNPC::AimGun()
{
	if (GetEnemy())
	{
		Vector vecShootOrigin;

		vecShootOrigin = Weapon_ShootPosition();
		Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

		SetAim( vecShootDir );
	}
	else
	{
		RelaxAim( );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set direction that the NPC is looking
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::MaintainLookTargets ( float flInterval )
{
	// --------------------------------------------------------
	// Try to look at enemy if I have one
	// --------------------------------------------------------
	if ((CBaseEntity*)GetEnemy())
	{
		if ( ValidEyeTarget(GetEnemy()->EyePosition()) )
		{
			SetHeadDirection(GetEnemy()->EyePosition(),flInterval);
			SetViewtarget( GetEnemy()->EyePosition() );
			return;
		}
	}

#if 0
	// --------------------------------------------------------
	// First check if I've been assigned to look at an entity
	// --------------------------------------------------------
	CBaseEntity *lookTarget = EyeLookTarget();
	if (lookTarget && ValidEyeTarget(lookTarget->EyePosition()))
	{
		SetHeadDirection(lookTarget->EyePosition(),flInterval);
		SetViewtarget( lookTarget->EyePosition() );
		return;
	}
#endif

	// --------------------------------------------------------
	// If I'm moving, look at my target position
	// --------------------------------------------------------
	if (GetNavigator()->IsGoalActive() && ValidEyeTarget(GetNavigator()->GetCurWaypointPos()))
	{
		SetHeadDirection(GetNavigator()->GetCurWaypointPos(),flInterval);
		SetViewtarget( GetNavigator()->GetCurWaypointPos() );
		return;
	}


	// -------------------------------------
	// If I hear a combat sounds look there
	// -------------------------------------
	if ( HasCondition(COND_HEAR_COMBAT) || HasCondition(COND_HEAR_DANGER) )
	{
		CSound *pSound = GetBestSound();

		if ( pSound && pSound->IsSoundType(SOUND_COMBAT | SOUND_DANGER) )
		{
			if (ValidEyeTarget( pSound->GetSoundOrigin() ))
			{
				SetHeadDirection(pSound->GetSoundOrigin(),flInterval);
				SetViewtarget( pSound->GetSoundOrigin() );
				return;
			}
		}
	}

	// -------------------------------------
	// Otherwise just look around randomly
	// -------------------------------------

	// Check that look target position is still valid
	if (m_flNextEyeLookTime > gpGlobals->curtime)
	{
		if (!ValidEyeTarget(m_vEyeLookTarget))
		{
			// Force choosing of new look target
			m_flNextEyeLookTime = 0;
		}
	}

	if (m_flNextEyeLookTime < gpGlobals->curtime)
	{
		Vector	vBodyDir = BodyDirection2D( );

		/*
		Vector  lookSpread	= Vector(0.82,0.82,0.22);
		float	x			= random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
		float	y			= random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
		float	z			= random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);

		QAngle angles;
		VectorAngles( vBodyDir, angles );
		Vector forward, right, up;
		AngleVectors( angles, &forward, &right, &up );

		Vector vecDir		= vBodyDir + (x * lookSpread.x * forward) + (y * lookSpread.y * right) + (z * lookSpread.z * up);
		float  lookDist		= random->RandomInt(50,2000);
		m_vEyeLookTarget	= EyePosition() + lookDist*vecDir;
		*/
		m_vEyeLookTarget	= EyePosition() + 500*vBodyDir;
		m_flNextEyeLookTime = gpGlobals->curtime + 0.5; // random->RandomInt(1,5);
	}
	SetHeadDirection(m_vEyeLookTarget,flInterval);

	// ----------------------------------------------------
	// Integrate eye turn in frame rate independent manner
	// ----------------------------------------------------
	float timeToUse = flInterval;
	while (timeToUse > 0)
	{
		m_vCurEyeTarget = ((1 - m_flEyeIntegRate) * m_vCurEyeTarget + m_flEyeIntegRate * m_vEyeLookTarget);
		timeToUse -= 0.1;
	}
	SetViewtarget( m_vCurEyeTarget );
}

//-----------------------------------------------------------------------------
// Here's where all motion occurs
//-----------------------------------------------------------------------------
void CAI_BaseNPC::PerformMovement()
{
	AI_PROFILE_SCOPE(CAI_BaseNPC_PerformMovement);
	g_AIMoveTimer.Start();

	m_pNavigator->Move();

	g_AIMoveTimer.End();

}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::PreThink( void )
{
	// ----------------------------------------------------------
	// Skip AI if its been disabled or networks haven't been
	// loaded, and put a warning message on the screen
	// ----------------------------------------------------------
	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI ||
		!g_pAINetworkManager->NetworksLoaded()						   )
	{
		static float next_message_time = 0.0f;

		if ( gpGlobals->curtime >= next_message_time )
		{
			next_message_time = gpGlobals->curtime + 5.0f;

			hudtextparms_s tTextParam;
			tTextParam.x			= 0.7;
			tTextParam.y			= 0.65;
			tTextParam.effect		= 0;
			tTextParam.r1			= 255;
			tTextParam.g1			= 255;
			tTextParam.b1			= 255;
			tTextParam.a1			= 255;
			tTextParam.r2			= 255;
			tTextParam.g2			= 255;
			tTextParam.b2			= 255;
			tTextParam.a2			= 255;
			tTextParam.fadeinTime	= 0;
			tTextParam.fadeoutTime	= 0;
			tTextParam.holdTime		= 5.1;
			tTextParam.fxTime		= 0;
			tTextParam.channel		= 1;
			UTIL_HudMessageAll( tTextParam, "A.I. Disabled...\n" );
		}
		SetActivity( ACT_IDLE );
		return false;
	}

	// --------------------------------------------------------
	//	If debug stepping
	// --------------------------------------------------------
	if (CAI_BaseNPC::m_nDebugBits & bits_debugStepAI)
	{
		if (m_nDebugCurIndex >= CAI_BaseNPC::m_nDebugPauseIndex)
		{
			if (!GetNavigator()->IsGoalActive())
			{
				m_flPlaybackRate = 0;
			}
			return false;
		}
		else
		{
			m_flPlaybackRate = 1;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------

void CAI_BaseNPC::RunAnimation( void )
{
	VPROF_BUDGET( "CAI_BaseNPC_RunAnimation", VPROF_BUDGETGROUP_NPC_ANIM );

	float flInterval = GetAnimTimeInterval();
		
	StudioFrameAdvance( ); // animate

	if ((CAI_BaseNPC::m_nDebugBits & bits_debugStepAI) && !GetNavigator()->IsGoalActive())
	{
		flInterval = 0;
	}

	if ( CapabilitiesGet() & bits_CAP_AIM_GUN )
	{
		AimGun();
	}

	// set look targets for npcs with animated faces
	if ( CapabilitiesGet() & bits_CAP_ANIMATEDFACE )
	{
		MaintainLookTargets(flInterval);
	}

	// start or end a fidget
	// This needs a better home -- switching animations over time should be encapsulated on a per-activity basis
	// perhaps MaintainActivity() or a ShiftAnimationOverTime() or something.
	if ( m_NPCState != NPC_STATE_SCRIPT && m_NPCState != NPC_STATE_DEAD && m_Activity == ACT_IDLE && IsActivityFinished() )
	{
		int iSequence;

		if ( SequenceLoops() )
		{
			// animation does loop, which means we're playing subtle idle. Might need to fidget.
			iSequence = SelectWeightedSequence ( m_translatedActivity );
		}
		else
		{
			// animation that just ended doesn't loop! That means we just finished a fidget
			// and should return to our heaviest weighted idle (the subtle one)
			iSequence = SelectHeaviestSequence ( m_translatedActivity );
		}
		if ( iSequence != ACTIVITY_NOT_AVAILABLE )
		{
			ResetSequence( iSequence ); // Set to new anim (if it's there)
		}
	}
}

//-----------------------------------------------------------------------------

void CAI_BaseNPC::PostRun( void )
{
	AI_PROFILE_SCOPE(CAI_BaseNPC_PostRun);

	g_AIPostRunTimer.Start();

	RunAnimation();

	DispatchAnimEvents( this );

	// update slave items (weapons)
	Weapon_FrameUpdate();

	g_AIPostRunTimer.End();
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::ShouldAlwaysThink()
{
	// @TODO (toml 07-08-03): This needs to be beefed up. 
	// There are simple cases already seen where an AI can briefly leave
	// the PVS while navigating to the player. Perhaps should incorporate a heuristic taking into
	// account mode, enemy, last time saw player, player range etc. For example, if enemy is player,
	// and player is within 100 feet, and saw the player within the past 15 seconds, keep running...
	return HasSpawnFlags(SF_NPC_ALWAYSTHINK);
}

//-----------------------------------------------------------------------------
// NPC Think - calls out to core AI functions and handles this
// npc's specific animation events
//

void CAI_BaseNPC::NPCThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );// keep npc thinking.

	if ( g_pAINetworkManager && g_pAINetworkManager->IsInitialized() )
	{
		VPROF_BUDGET( "NPCs", VPROF_BUDGETGROUP_NPCS );

		if ( !PreThink() )
			return;

		RunAI();

		PostRun();

		PerformMovement();

		SetSimulationTime( gpGlobals->curtime );
	}
}

//=========================================================
// CAI_BaseNPC - USE - will make a npc angry at whomever
// activated it.
//=========================================================
void CAI_BaseNPC::NPCUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	return;

	// Can't +USE NPCs running scripts
	if ( GetState() == NPC_STATE_SCRIPT )
		return;

	m_IdealNPCState = NPC_STATE_ALERT;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual function that allows us to have any npc ignore a set of
// shared conditions.
//
//-----------------------------------------------------------------------------
void CAI_BaseNPC::RemoveIgnoredConditions ( void )
{
	m_Conditions.And( m_InverseIgnoreConditions, &m_Conditions );

	if ( m_NPCState == NPC_STATE_SCRIPT && m_hCine )
		m_hCine->RemoveIgnoredConditions();
}

//=========================================================
// RangeAttack1Conditions
//=========================================================
int CAI_BaseNPC::RangeAttack1Conditions ( float flDot, float flDist )
{
	if ( flDist < 64)
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDist > 784)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.5)
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}

//=========================================================
// RangeAttack2Conditions
//=========================================================
int CAI_BaseNPC::RangeAttack2Conditions ( float flDot, float flDist )
{
	if ( flDist < 64)
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDist > 512)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.5)
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK2;
}

//=========================================================
// MeleeAttack1Conditions
//=========================================================
int CAI_BaseNPC::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if ( flDist > 64)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return 0;
	}
	else if (GetEnemy() == NULL)
	{
		return 0;
	}

	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( GetEnemy()->GetFlags() & FL_ONGROUND )
	{
		return COND_CAN_MELEE_ATTACK1;
	}
	return 0;
}

//=========================================================
// MeleeAttack2Conditions
//=========================================================
int CAI_BaseNPC::MeleeAttack2Conditions ( float flDot, float flDist )
{
	if ( flDist > 64)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return 0;
	}
	return COND_CAN_MELEE_ATTACK2;
}

// Get capability mask
int CAI_BaseNPC::CapabilitiesGet( void )
{
	int capability = m_afCapability;
	if ( GetActiveWeapon() )
	{
		capability |= GetActiveWeapon()->CapabilitiesGet();
	}
	return capability;
}

// Set capability mask
int CAI_BaseNPC::CapabilitiesAdd( int capability )
{
	m_afCapability |= capability;

	return m_afCapability;
}

// Set capability mask
int CAI_BaseNPC::CapabilitiesRemove( int capability )
{
	m_afCapability &= ~capability;

	return m_afCapability;
}

// Clear capability mask
void CAI_BaseNPC::CapabilitiesClear( void )
{
	m_afCapability = 0;
}


//=========================================================
// ClearAttacks - clear out all attack conditions
//=========================================================
void CAI_BaseNPC::ClearAttackConditions( )
{
	// Clear all attack conditions
	ClearCondition( COND_CAN_RANGE_ATTACK1 );
	ClearCondition( COND_CAN_RANGE_ATTACK2 );
	ClearCondition( COND_CAN_MELEE_ATTACK1 );
	ClearCondition( COND_CAN_MELEE_ATTACK2 );
	ClearCondition( COND_WEAPON_HAS_LOS );
	ClearCondition( COND_WEAPON_BLOCKED_BY_FRIEND );
	ClearCondition( COND_WEAPON_PLAYER_IN_SPREAD );		// Player in shooting direction
	ClearCondition( COND_WEAPON_PLAYER_NEAR_TARGET );	// Player near shooting position
	ClearCondition( COND_WEAPON_SIGHT_OCCLUDED );
}

//=========================================================
// GatherAttackConditions - sets all of the bits for attacks that the
// npc is capable of carrying out on the passed entity.
//=========================================================

void CAI_BaseNPC::GatherAttackConditions( CBaseEntity *pTarget, float flDist )
{
	Vector vecLOS = ( pTarget->GetAbsOrigin() - GetAbsOrigin() );
	vecLOS.z = 0;
	VectorNormalize( vecLOS );

	Vector vBodyDir = BodyDirection2D( );
	float  flDot	= DotProduct( vecLOS, vBodyDir );

	// we know the enemy is in front now. We'll find which attacks the npc is capable of by
	// checking for corresponding Activities in the model file, then do the simple checks to validate
	// those attack types.

	// Clear all attack conditions
	ClearAttackConditions( );

	int		capability		= CapabilitiesGet();
	Vector  targetPos		= pTarget->BodyTarget( GetAbsOrigin() );
	bool	bWeaponHasLOS	= WeaponLOSCondition( GetAbsOrigin(), targetPos, true );
	int		condition;

	if ( !bWeaponHasLOS )
	{
		// @TODO (toml 06-15-03):  There are simple cases where
		// the upper torso of the enemy is visible, and the NPC is at an angle below
		// them, but the above test fails because BodyTarget returns the center of
		// the target. This needs some better handling/closer evaluation
		
		// Try the eyes
		ClearAttackConditions();
		targetPos = pTarget->EyePosition();
		bWeaponHasLOS = WeaponLOSCondition( GetAbsOrigin(), targetPos, true );
	}

	// FIXME: move this out of here
	if ( (capability & bits_CAP_WEAPON_RANGE_ATTACK1) && GetActiveWeapon())
	{
		condition = GetActiveWeapon()->WeaponRangeAttack1Condition(flDot, flDist);

		if ( condition == COND_NOT_FACING_ATTACK && FInAimCone( targetPos ) )
			Msg( "Warning: COND_NOT_FACING_ATTACK set but FInAimCone is true\n" );

		if (condition != COND_CAN_RANGE_ATTACK1 || bWeaponHasLOS)
		{
			SetCondition(condition);
		}
	}
	else if ( capability & bits_CAP_INNATE_RANGE_ATTACK1 )
	{
		condition = RangeAttack1Conditions( flDot, flDist );
		if (condition != COND_CAN_RANGE_ATTACK1 || bWeaponHasLOS)
		{
			SetCondition(condition);
		}
	}

	if ( (capability & bits_CAP_WEAPON_RANGE_ATTACK2) && GetActiveWeapon() && ( GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK2 ) )
	{
		condition = GetActiveWeapon()->WeaponRangeAttack2Condition(flDot, flDist);
		if (condition != COND_CAN_RANGE_ATTACK2 || bWeaponHasLOS)
		{
			SetCondition(condition);
		}
	}
	else if ( capability & bits_CAP_INNATE_RANGE_ATTACK2 )
	{
		condition = RangeAttack2Conditions( flDot, flDist );
		if (condition != COND_CAN_RANGE_ATTACK2 || bWeaponHasLOS)
		{
			SetCondition(condition);
		}
	}

	if ( (capability & bits_CAP_WEAPON_MELEE_ATTACK1) && GetActiveWeapon())
	{
		SetCondition(GetActiveWeapon()->WeaponMeleeAttack1Condition(flDot, flDist));
	}
	else if ( capability & bits_CAP_INNATE_MELEE_ATTACK1 )
	{
		SetCondition(MeleeAttack1Conditions ( flDot, flDist ));
	}

	if ( (capability & bits_CAP_WEAPON_MELEE_ATTACK2) && GetActiveWeapon())
	{
		SetCondition(GetActiveWeapon()->WeaponMeleeAttack2Condition(flDot, flDist));
	}
	else if ( capability & bits_CAP_INNATE_MELEE_ATTACK2 )
	{
		SetCondition(MeleeAttack2Conditions ( flDot, flDist ));
	}

	// -----------------------------------------------------------------
	// If any attacks are possible clear attack specific bits
	// -----------------------------------------------------------------
	if (HasCondition(COND_CAN_RANGE_ATTACK2) ||
		HasCondition(COND_CAN_RANGE_ATTACK1) ||
		HasCondition(COND_CAN_MELEE_ATTACK2) ||
		HasCondition(COND_CAN_MELEE_ATTACK1) )
	{

		ClearCondition(COND_TOO_CLOSE_TO_ATTACK);
		ClearCondition(COND_TOO_FAR_TO_ATTACK);
		ClearCondition(COND_WEAPON_BLOCKED_BY_FRIEND);
	}
}


//=========================================================
// SetState
//=========================================================
void CAI_BaseNPC::SetState ( NPC_STATE State )
{
	NPC_STATE OldState;

	OldState = m_NPCState;

	if ( State != m_NPCState )
	{
		m_flLastStateChangeTime = gpGlobals->curtime;
	}

	switch( State )
	{
	// Drop enemy pointers when going to idle
	case NPC_STATE_IDLE:

		if ( GetEnemy() != NULL )
		{
			SetEnemy( NULL ); // not allowed to have an enemy anymore.
			DevMsg( 2, "Stripped\n" );
		}
		break;
	}

	bool fNotifyChange = false;

	if( m_NPCState != State )
	{
		// Don't notify if we're changing to a state we're already in!
		fNotifyChange = true;
	}

	m_NPCState = State;
	m_IdealNPCState = State;

	// Notify the character that its state has changed.
	if( fNotifyChange )
	{
		OnStateChange( OldState, m_NPCState );
	}
}


//-----------------------------------------------------------------------------
// Sets all sensing-related conditions
//-----------------------------------------------------------------------------
void CAI_BaseNPC::PerformSensing( void )
{
	GetSenses()->PerformSensing();
}


//-----------------------------------------------------------------------------

void CAI_BaseNPC::ClearSenseConditions( void )
{
	static int conditionsToClear[] =
	{
		COND_SEE_HATE,
		COND_SEE_DISLIKE,
		COND_SEE_ENEMY,
		COND_SEE_FEAR,
		COND_SEE_NEMESIS,
		COND_SEE_PLAYER,
		COND_HEAR_DANGER,
		COND_HEAR_COMBAT,
		COND_HEAR_WORLD,
		COND_HEAR_PLAYER,
		COND_HEAR_THUMPER,
		COND_HEAR_BUGBAIT,
		COND_HEAR_PHYSICS_DANGER,
		COND_SMELL,
	};

	ClearConditions( conditionsToClear, ARRAYSIZE( conditionsToClear ) );
}

//-----------------------------------------------------------------------------

void CAI_BaseNPC::GatherConditions( void )
{
	m_bConditionsGathered = true;
	g_AIConditionsTimer.Start();

	if ( m_NPCState != NPC_STATE_NONE	&&
		 m_NPCState != NPC_STATE_DEAD )
	{
		// Sample the environment. Do this unconditionally if there is a player in this
		// npc's PVS. NPCs in COMBAT state are allowed to simulate when there is no player in
		// the same PVS. This is so that any fights in progress will continue even if the player leaves the PVS.
		if ( ( GetEfficiency() == AIE_NORMAL ) && 
			 ( HasSpawnFlags(SF_NPC_ALWAYSTHINK) || UTIL_FindClientInPVS( edict() ) || ( m_NPCState == NPC_STATE_COMBAT ) ) )
		{
			if ( ShouldPlayIdleSound() )
			{
				AI_PROFILE_SCOPE(CAI_BaseNPC_IdleSound);
				IdleSound();
			}

			PerformSensing();

			GetEnemies()->RefreshMemories();
			ChooseEnemy();
		}
		else
			ClearSenseConditions(); // if not done, can have problems if leave PVS in same frame heard/saw things, since only PerformSensing clears conditions

		// do these calculations if npc has an enemy.
		if ( GetEnemy() != NULL )
		{
			if ( GetEfficiency() == AIE_NORMAL )
				GatherEnemyConditions( GetEnemy() );
			else
				SetEnemy( NULL );
		}

		// do these calculations if npc has a target
		if ( GetTarget() != NULL )
		{
			CheckTarget( GetTarget() );
		}

		CheckAmmo();
	}
	
	RemoveIgnoredConditions();

	g_AIConditionsTimer.End();
}

//-----------------------------------------------------------------------------
// Main entry point for processing AI
//-----------------------------------------------------------------------------

void CAI_BaseNPC::RunAI( void )
{
	AI_PROFILE_SCOPE(CAI_BaseNPC_RunAI);
	g_AIRunTimer.Start();

	m_bConditionsGathered = false;

	if ( g_pDeveloper->GetInt() && !GetNavigator()->IsOnNetwork() )
	{
		AddTimedOverlay( "NPC w/no reachable nodes!", 5 );
	}

	{
	AI_PROFILE_SCOPE(CAI_BaseNPC_RunAI_GatherConditions);
	GatherConditions();
	if ( !m_bConditionsGathered )
		m_bConditionsGathered = true; // derived class didn't call to base
	}

	TryRestoreHull();


	{
	AI_PROFILE_SCOPE(CAI_RunAI_PrescheduleThink);
	g_AIPrescheduleThinkTimer.Start();

	PrescheduleThink();

	g_AIPrescheduleThinkTimer.End();
	}

	MaintainSchedule();

	// if the npc didn't use these conditions during the above call to MaintainSchedule()
	// we throw them out cause we don't want them sitting around through the lifespan of a schedule
	// that doesn't use them.
	ClearCondition( COND_LIGHT_DAMAGE  );
	ClearCondition( COND_HEAVY_DAMAGE );

	g_AIRunTimer.End();
}

//-----------------------------------------------------------------------------
// Purpose: Surveys the Conditions information available and finds the best
// new state for a npc.
//
// NOTICE that m_IdealNPCState is actually set in this function, not merely
// suggested.
//
// NOTICE the CAI_BaseNPC implementation of this function does not care about
// private conditions!
//
// Output : NPC_STATE - the state that m_IdealNPCState has been set to.
//-----------------------------------------------------------------------------
NPC_STATE CAI_BaseNPC::SelectIdealState ( void )
{
	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	// ---------------------------
	// Do some squad stuff first
	// ---------------------------
	if (m_pSquad)
	{
		switch( m_NPCState )
		{
		case NPC_STATE_IDLE:
		case NPC_STATE_ALERT:
			if ( HasCondition ( COND_NEW_ENEMY ) )
			{
				m_pSquad->SquadNewEnemy ( GetEnemy() );
			}
			break;
		}
	}

	// ---------------------------
	//  Set ideal state
	// ---------------------------
	switch ( m_NPCState )
	{
	case NPC_STATE_IDLE:

		/*
		IDLE goes to ALERT upon hearing a sound
		IDLE goes to ALERT upon being injured
		IDLE goes to ALERT upon seeing food
		IDLE goes to COMBAT upon sighting an enemy
		*/
		{
			if ( HasInterruptCondition(COND_NEW_ENEMY) ||
				 HasInterruptCondition(COND_SEE_ENEMY) )

			{
				// new enemy! This means an idle npc has seen someone it dislikes, or
				// that a npc in combat has found a more suitable target to attack
				m_IdealNPCState = NPC_STATE_COMBAT;
			}
			else if ( HasInterruptCondition(COND_LIGHT_DAMAGE) || HasInterruptCondition(COND_HEAVY_DAMAGE) )
			{
				if( GetEnemy() )
				{
					Vector flEnemyLKP = GetEnemyLKP();
					GetMotor()->SetIdealYawToTarget( flEnemyLKP );
				}
				else
				{
					// Don't have an enemy, so face the direction the last attack came
					// from. Don't face north.
					GetMotor()->SetIdealYawToTarget( WorldSpaceCenter() + g_vecAttackDir * 128 );
				}

				m_IdealNPCState = NPC_STATE_ALERT;
			}
			else if ( HasInterruptCondition(COND_HEAR_DANGER)  ||
					  HasInterruptCondition(COND_HEAR_COMBAT)  ||
					  HasInterruptCondition(COND_HEAR_WORLD)   ||
					  HasInterruptCondition(COND_HEAR_PLAYER)  ||
					  HasInterruptCondition(COND_HEAR_THUMPER) ||
					  HasInterruptCondition(COND_HEAR_BULLET_IMPACT) )
			{
				// Interrupted by a sound. So make our ideal yaw the
				// source of the sound!
				CSound *pSound;

				pSound = GetBestSound();
				Assert( pSound != NULL );
				if ( pSound )
				{
					// Build a yaw to the sound or source of the sound,
					// whichever is appropriate
					GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );

					if ( pSound->IsSoundType( SOUND_COMBAT | SOUND_DANGER | SOUND_BULLET_IMPACT ) )
					{
						m_IdealNPCState = NPC_STATE_ALERT;
					}
				}
			}
			else if (	HasInterruptCondition(COND_SMELL))
			{
				m_IdealNPCState = NPC_STATE_ALERT;
			}

			break;
		}
	case NPC_STATE_ALERT:
		/*
		ALERT goes to IDLE upon becoming bored
		ALERT goes to COMBAT upon sighting an enemy
		*/
		{
			if ( HasInterruptCondition(COND_NEW_ENEMY) ||
				 HasInterruptCondition(COND_SEE_ENEMY) )
			{
				m_IdealNPCState = NPC_STATE_COMBAT;
			}
			else if ( HasInterruptCondition(COND_HEAR_DANGER) ||
					  HasInterruptCondition(COND_HEAR_COMBAT) )
			{
				m_IdealNPCState = NPC_STATE_ALERT;
				CSound *pSound = GetBestSound();
				Assert( pSound != NULL );

				if ( pSound )
					GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );
			}
			else if ( ShouldGoToIdleState() )
			{
				m_IdealNPCState = NPC_STATE_IDLE;
			}
			break;
		}
	case NPC_STATE_COMBAT:
		/*
		COMBAT goes to ALERT upon death of enemy
		*/
		{
			if ( GetEnemy() == NULL )
			{
				m_IdealNPCState = NPC_STATE_ALERT;
				// m_fEffects = EF_BRIGHTFIELD;
				DevWarning( 2, "***Combat state with no enemy!\n" );
			}
			break;
		}
	case NPC_STATE_SCRIPT:
		if (	HasInterruptCondition(COND_TASK_FAILED)  ||
				HasInterruptCondition(COND_LIGHT_DAMAGE) ||
				HasInterruptCondition(COND_HEAVY_DAMAGE))
		{
			ExitScriptedSequence();	// This will set the ideal state
		}
		break;

	case NPC_STATE_DEAD:
		m_IdealNPCState = NPC_STATE_DEAD;
		break;
	}

	return m_IdealNPCState;
}

//-----------------------------------------------------------------------------
// Purpose:  Check if a better weapon is available.
//			 For now check if there is a weapon and I don't have one.  In
//			 the future
// UNDONE: actually rate the weapons based on there strength
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::Weapon_IsBetterAvailable()
{
	if (CapabilitiesGet() & bits_CAP_USE_WEAPONS)
	{
		if ( m_flNextWeaponSearchTime < gpGlobals->curtime )
		{
			m_flNextWeaponSearchTime = gpGlobals->curtime + 2;
			if (!GetActiveWeapon() && Weapon_FindUsable(Vector(300,300,100)))
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true is weapon has a line of sight.  If bSetConditions is
//			true, also sets LOC conditions
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	bool bHaveLOS;

	if (GetActiveWeapon())
	{
		bHaveLOS = GetActiveWeapon()->WeaponLOSCondition(ownerPos, targetPos, bSetConditions);
	}
	else if (CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1)
	{
		bHaveLOS = InnateWeaponLOSCondition(ownerPos, targetPos, bSetConditions);
	}
	else
	{
		if (bSetConditions)
		{
			SetCondition( COND_NO_WEAPON );
		}
		bHaveLOS = false;
	}
	// -------------------------------------------
	//  Check for friendly fire with the player
	// -------------------------------------------
	if (CapabilitiesGet() & bits_CAP_NO_HIT_PLAYER)
	{
		// Check shoot direction relative to player
		if (PlayerInSpread( ownerPos, targetPos, 0.92 ))
		{
			if (bSetConditions)
			{
				SetCondition( COND_WEAPON_PLAYER_IN_SPREAD );
			}
			bHaveLOS = false;
		}
		/* For grenades etc. check that player is clear?
		// Check player position also
		if (PlayerInRange( targetPos, 100 ))
		{
			SetCondition( COND_WEAPON_PLAYER_NEAR_TARGET );
		}
		*/
	}
	return bHaveLOS;
}

//-----------------------------------------------------------------------------
// Purpose: Check the innate weapon LOS for an owner at an arbitrary position
//			If bSetConditions is true, LOS related conditions will also be set
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	// --------------------
	// Check for occlusion
	// --------------------
	// Base class version assumes innate weapon position is at eye level
	Vector barrelPos		= ownerPos + GetViewOffset();
	trace_t tr;
	AI_TraceLine( barrelPos, targetPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	if ( tr.fraction == 1.0 )
	{
		return true;
	}
	CBaseEntity	*pHitEntity = tr.m_pEnt;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( 1 ) );
	// Is player in a vehicle? if so, verify vehicle is target and return if so (so npc shoots at vehicle)
	if ( pPlayer && pPlayer->IsInAVehicle() )
	{
		// Ok, player in vehicle, check if vehicle is target we're looking at, fire if it is
		// Also, check to see if the owner of the entity is the vehicle, in which case it's valid too.
		// This catches vehicles that use bone followers.
		CBaseEntity	*pVehicle  = pPlayer->GetVehicle()->GetVehicleEnt();
		if ( pHitEntity == pVehicle || pHitEntity->GetOwnerEntity() == pVehicle )
			return true;
	}

	if ( pHitEntity == GetEnemy() )
	{
		return true;
	}
	else if ( pHitEntity && pHitEntity->MyCombatCharacterPointer() )
	{
		if (IRelationType( pHitEntity ) == D_HT)
		{
			return true;
		}
		else if (bSetConditions)
		{
			SetCondition(COND_WEAPON_BLOCKED_BY_FRIEND);
		}
	}
	else if (bSetConditions)
	{
		SetCondition(COND_WEAPON_SIGHT_OCCLUDED);
		SetEnemyOccluder(tr.m_pEnt);
	}

	return false;
}

//=========================================================
// CanCheckAttacks - prequalifies a npc to do more fine
// checking of potential attacks.
//=========================================================
bool CAI_BaseNPC::FCanCheckAttacks( void )
{
	// Not allowed to check attacks while climbing or jumping
	// Otherwise schedule is interrupted while on ladder/etc
	// which is NOT a legal place to attack from
	if ( GetNavType() == NAV_CLIMB || GetNavType() == NAV_JUMP )
		return false;

	if ( HasCondition(COND_SEE_ENEMY) && !HasCondition( COND_ENEMY_TOO_FAR))
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return dist. to enemy (closest of origin/head/feet)
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CAI_BaseNPC::EnemyDistance( CBaseEntity *pEnemy )
{
	Vector enemyDelta = pEnemy->GetAbsOrigin() - GetAbsOrigin();

	// see if the enemy is closer to my head, feet or in between
	if ( pEnemy->GetAbsMins().z >= GetAbsMaxs().z )
	{
		// closest to head
		enemyDelta.z = pEnemy->GetAbsMins().z - GetAbsMaxs().z;
	}
	else if ( pEnemy->GetAbsMaxs().z <= GetAbsMins().z )
	{
		// closest to feet
		enemyDelta.z = pEnemy->GetAbsMaxs().z - GetAbsMins().z;
	}
	else
	{
		// in between, no delta
		enemyDelta.z = 0;
	}

	return enemyDelta.Length();
}

//-----------------------------------------------------------------------------
// Purpose: Update information on my enemy
// Input  :
// Output : Returns true is new enemy, false is known enemy
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, bool firstHand )
{
	if ( GetEnemies() )
	{
		// If the was eluding me and allow the NPC to play a sound
		if (GetEnemies()->HasEludedMe(pEnemy))
		{
			FoundEnemySound();
		}
		bool result = GetEnemies()->UpdateMemory(GetNavigator()->GetNetwork(), pEnemy, position);
		if ( firstHand && pEnemy && m_pSquad )
		{
			m_pSquad->UpdateEnemyMemory( this, pEnemy, position );
		}
		return result;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Remembers the thing my enemy is hiding behind. Called when either
//			COND_ENEMY_OCCLUDED or COND_WEAPON_SIGHT_OCCLUDED is set.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::SetEnemyOccluder(CBaseEntity *pBlocker)
{
	m_hEnemyOccluder = pBlocker;
}


//-----------------------------------------------------------------------------
// Purpose: Gets the thing my enemy is hiding behind (assuming they are hiding).
//-----------------------------------------------------------------------------
CBaseEntity *CAI_BaseNPC::GetEnemyOccluder(void)
{
	return m_hEnemyOccluder.Get();
}


//-----------------------------------------------------------------------------
// Purpose: part of the Condition collection process
//			gets and stores data and conditions pertaining to a npc's
//			enemy.
// 			@TODO (toml 07-27-03): this should become subservient to the senses. right
// 			now, it yields different result
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	AI_PROFILE_SCOPE(CAI_Enemies_GatherEnemyConditions);

	ClearCondition( COND_ENEMY_FACING_ME  );
	ClearCondition( COND_BEHIND_ENEMY   );
	ClearCondition( COND_HAVE_ENEMY_LOS );
	ClearCondition( COND_ENEMY_OCCLUDED  );

	// ---------------------------
	//  Set visibility conditions
	// ---------------------------
	CBaseEntity *pBlocker = NULL;
	SetEnemyOccluder(NULL);

	if ( !FVisible( pEnemy, MASK_OPAQUE, &pBlocker ) )
	{
		// No LOS to enemy
		SetEnemyOccluder(pBlocker);
		SetCondition( COND_ENEMY_OCCLUDED );

		if (HasMemory( bits_MEMORY_HAD_LOS ))
		{
			// Send output event
			if (GetEnemy()->IsPlayer())
			{
				m_OnLostPlayerLOS.FireOutput(this, this);
			}
			else
			{
				m_OnLostEnemyLOS.FireOutput(this, this);
			}
		}
		Forget( bits_MEMORY_HAD_LOS );
	}
	else
	{
		// Have LOS but may not be in view cone
		SetCondition( COND_HAVE_ENEMY_LOS );

		if (FInViewCone( pEnemy ) && QuerySeeEntity( pEnemy ))
		{
			// Have LOS and in view cone
			SetCondition( COND_SEE_ENEMY );
		}

		if (!HasMemory( bits_MEMORY_HAD_LOS ))
		{
			// Send output event
			EHANDLE hEnemy;
			hEnemy.Set( GetEnemy() );

			if (GetEnemy()->IsPlayer())
			{
				m_OnFoundPlayer.Set(hEnemy, this, this);
				m_OnFoundEnemy.Set(hEnemy, this, this);
			}
			else
			{
				m_OnFoundEnemy.Set(hEnemy, this, this);
			}
		}
		Remember( bits_MEMORY_HAD_LOS );
	}

  	// -------------------
  	// If enemy is dead
  	// -------------------
  	if ( !pEnemy->IsAlive() )
  	{
  		SetCondition( COND_ENEMY_DEAD );
  		ClearCondition( COND_SEE_ENEMY );
  		ClearCondition( COND_ENEMY_OCCLUDED );
  		return;
  	}	
	
	float flDistToEnemy = EnemyDistance(pEnemy);

	if ( HasCondition( COND_SEE_ENEMY ) )
	{
		// Trail the enemy a bit if he's moving
		if (pEnemy->GetAbsVelocity() != vec3_origin)
		{
			Vector vTrailPos = pEnemy->GetAbsOrigin() - pEnemy->GetAbsVelocity() * random->RandomFloat( -0.05, 0 );
			UpdateEnemyMemory(pEnemy,vTrailPos);
		}
		else
		{
			UpdateEnemyMemory(pEnemy,pEnemy->GetAbsOrigin());
		}

		// If it's not an NPC, assume it can't see me
		if ( pEnemy->MyCombatCharacterPointer() && pEnemy->MyCombatCharacterPointer()->FInViewCone ( this ) )
		{
			SetCondition ( COND_ENEMY_FACING_ME );
			ClearCondition ( COND_BEHIND_ENEMY );
		}
		else
		{
			ClearCondition( COND_ENEMY_FACING_ME );
			SetCondition ( COND_BEHIND_ENEMY );
		}
	}
	else if ( (!HasCondition(COND_ENEMY_OCCLUDED) && !HasCondition(COND_SEE_ENEMY)) && ( flDistToEnemy <= 256 ) )
	{
		// if the enemy is not occluded, and unseen, that means it is behind or beside the npc.
		// if the enemy is near enough the npc, we go ahead and let the npc know where the
		// enemy is.
		UpdateEnemyMemory( pEnemy, pEnemy->GetAbsOrigin());
	}
	if ( flDistToEnemy >= m_flDistTooFar )
	{
		// enemy is very far away from npc
		SetCondition( COND_ENEMY_TOO_FAR );
	}
	else
	{
		ClearCondition( COND_ENEMY_TOO_FAR );
	}

#if 0
	// Don't do this check, actually. It's evil. (sjb)
	// Check to see if there is a better weapon available
	if (Weapon_IsBetterAvailable())
	{
		SetCondition(COND_BETTER_WEAPON_AVAILABLE);
	}
#endif 

	if ( FCanCheckAttacks() )
	{
		// This may also call SetEnemyOccluder!
		GatherAttackConditions ( GetEnemy(), flDistToEnemy );
	}

	// If my enemy has moved significantly, or if the enemy has changed update my path
	UpdateEnemyPos();

	// If my target entity has moved significantly, update my path
	// This is an odd place to put this, but where else should it go?
	UpdateTargetPos();

	// ----------------------------------------------------------------------------
	// Check if enemy is reachable via the node graph unless I'm not on a network
	// ----------------------------------------------------------------------------
	if (GetNavigator()->IsOnNetwork())
	{
		// Note that unreachablity times out
		if (IsUnreachable(GetEnemy()))
		{
			SetCondition(COND_ENEMY_UNREACHABLE);
		}
	}

	//-----------------------------------------------------------------------
	// If I haven't seen the enemy in a while he may have eluded me
	//-----------------------------------------------------------------------
	if (gpGlobals->curtime - GetEnemyLastTimeSeen() > 8)
	{
		//-----------------------------------------------------------------------
		// I'm at last known position at enemy isn't in sight then has eluded me
		// ----------------------------------------------------------------------
		Vector flEnemyLKP = GetEnemyLKP();
		if (((flEnemyLKP - GetAbsOrigin()).Length2D() < 48) &&
			!HasCondition(COND_SEE_ENEMY))
		{
			MarkEnemyAsEluded();
		}
		//-------------------------------------------------------------------
		// If enemy isn't reachable, I can see last known position and enemy
		// isn't there, then he has eluded me
		// ------------------------------------------------------------------
		if (!HasCondition(COND_SEE_ENEMY) && HasCondition(COND_ENEMY_UNREACHABLE))
		{

			trace_t tr;
			AI_TraceLine( EyePosition(), flEnemyLKP, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction != 1.0 )
			{
				MarkEnemyAsEluded();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// In the case of goaltype enemy, update the goal position
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UpdateEnemyPos()
{
	// BRJ 10/7/02
	// FIXME: make this check time based instead of distance based!

	// Don't perform path recomputations during a climb or a jump
	if ( !GetNavigator()->IsInterruptable() )
		return;

	// If my enemy has moved significantly, or if the enemy has changed update my path
	if ( GetNavigator()->GetGoalType() == GOALTYPE_ENEMY )
	{
		if (m_hEnemy != GetNavigator()->GetGoalTarget())
		{
			GetNavigator()->SetGoalTarget( m_hEnemy, vec3_origin );
		}
		else
		{
			// UNDONE: Should we allow npcs to override this distance (80?)
			Vector flEnemyLKP = GetEnemyLKP();
			float ignored;
			TranslateEnemyChasePosition( GetEnemy(), flEnemyLKP, ignored );
			if ( (GetNavigator()->GetGoalPos() - flEnemyLKP).Length() > 80 )
			{
				GetNavigator()->RefindPathToGoal();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// In the case of goaltype targetent, update the goal position
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UpdateTargetPos()
{
	// BRJ 10/7/02
	// FIXME: make this check time based instead of distance based!

	// Don't perform path recomputations during a climb or a jump
	if ( !GetNavigator()->IsInterruptable() )
		return;

	// If my target entity has moved significantly, or has changed, update my path
	// This is an odd place to put this, but where else should it go?
	if ( GetNavigator()->GetGoalType() == GOALTYPE_TARGETENT )
	{
		if (m_hTargetEnt != GetNavigator()->GetGoalTarget())
		{
			GetNavigator()->SetGoalTarget( m_hTargetEnt, vec3_origin );
		}
		else if ( GetNavigator()->GetGoalFlags() & AIN_UPDATE_TARGET_POS )
		{
			if ( GetTarget() == NULL || (GetNavigator()->GetGoalPos() - GetTarget()->GetAbsOrigin()).Length() > 80 )
			{
				GetNavigator()->RefindPathToGoal();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: part of the Condition collection process
//			gets and stores data and conditions pertaining to a npc's
//			enemy.
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::CheckTarget( CBaseEntity *pTarget )
{
	AI_PROFILE_SCOPE(CAI_Enemies_CheckTarget);

	ClearCondition ( COND_HAVE_TARGET_LOS );
	ClearCondition ( COND_TARGET_OCCLUDED  );

	// ---------------------------
	//  Set visibility conditions
	// ---------------------------
	if ( !FVisible( pTarget ) )
	{
		// No LOS to target
		SetCondition( COND_TARGET_OCCLUDED );
	}
	else
	{
		// Have LOS (may not be in view cone)
		SetCondition( COND_HAVE_TARGET_LOS );
	}

	UpdateTargetPos();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eNewActivity - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CAI_BaseNPC::NPC_TranslateActivity( Activity eNewActivity )
{
	Assert( eNewActivity != ACT_INVALID );

	if (CapabilitiesGet() & bits_CAP_DUCK)
	{
		if (eNewActivity == ACT_RELOAD)
		{
			return GetReloadActivity(m_pHintNode);
		}
		else if ((eNewActivity == ACT_COVER	)								 ||
				 (eNewActivity == ACT_IDLE && HasMemory(bits_MEMORY_INCOVER)))
		{
			return GetCoverActivity(m_pHintNode);
		}
	}
	return eNewActivity;
}


//-----------------------------------------------------------------------------

Activity CAI_BaseNPC::TranslateActivity( Activity idealActivity, Activity *pIdealWeaponActivity )
{
	const int MAX_TRIES = 5;
	int count = 0;

	Activity idealWeaponActivity;
	Activity baseTranslation;
	Activity weaponTranslation;
	Activity last;
	Activity current;

	idealWeaponActivity = Weapon_TranslateActivity( idealActivity );
	if ( pIdealWeaponActivity )
		*pIdealWeaponActivity = idealWeaponActivity;

	baseTranslation	  = idealActivity;
	weaponTranslation = idealActivity;
	last			  = idealActivity;
	while ( count++ < MAX_TRIES )
	{
		current = NPC_TranslateActivity( last );
		if ( current != last )
			baseTranslation = current;

		weaponTranslation = Weapon_TranslateActivity( current );

		if ( weaponTranslation == last )
			break;

		last = weaponTranslation;
	}
	AssertMsg( count < MAX_TRIES, "Circular activity translation!" );

	if ( last == ACT_SCRIPT_CUSTOM_MOVE )
		return ACT_SCRIPT_CUSTOM_MOVE;
	
	if ( HaveSequenceForActivity( weaponTranslation ) )
		return weaponTranslation;
	
	if ( baseTranslation != weaponTranslation && HaveSequenceForActivity( baseTranslation ) )
		return baseTranslation;
	
	if ( idealWeaponActivity != baseTranslation && HaveSequenceForActivity( idealWeaponActivity ) )
		return idealActivity;
	
	if ( idealActivity != idealWeaponActivity && HaveSequenceForActivity( idealActivity ) )
		return idealActivity;

	if ( idealActivity == ACT_RUN )
	{
		Assert( !HaveSequenceForActivity( idealActivity ) );
		idealActivity = ACT_WALK;
	}

	return idealActivity;
}
	
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : NewActivity - 
//			iSequence - 
//			translatedActivity - 
//			weaponActivity - 
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ResolveActivityToSequence(Activity NewActivity, int &iSequence, Activity &translatedActivity, Activity &weaponActivity)
{
	iSequence = ACTIVITY_NOT_AVAILABLE;

	translatedActivity = TranslateActivity( NewActivity, &weaponActivity );

	if ( ( NewActivity == ACT_SCRIPT_CUSTOM_MOVE ) && ( m_hCine != NULL ) )
	{
		iSequence = LookupSequence( STRING( m_hCine->m_iszCustomMove ) );

		if ( iSequence == ACT_INVALID )
		{
			DevMsg( "SCRIPT_CUSTOM_MOVE: %s has no sequence:%s\n", GetClassname(), m_hCine->m_iszCustomMove );
			iSequence = SelectWeightedSequence( ACT_WALK );
		}
	}
	else
	{
		iSequence = SelectWeightedSequence( translatedActivity );

		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			static CAI_BaseNPC *pLastWarn;
			static Activity lastWarnActivity;
			static float timeLastWarn;

			if ( ( pLastWarn != this && lastWarnActivity != translatedActivity ) || gpGlobals->curtime - timeLastWarn > 5.0 )
			{
				DevMsg( "%s has no sequence for act:%s\n", GetClassname(), ActivityList_NameForIndex(translatedActivity) );
				pLastWarn = this;
				lastWarnActivity = translatedActivity;
				timeLastWarn = gpGlobals->curtime;
			}

			if ( translatedActivity == ACT_RUN )
			{
				translatedActivity = ACT_WALK;
				iSequence = SelectWeightedSequence( translatedActivity );
			}
		}
	}

	if ( iSequence == ACT_INVALID )
	{
		// Abject failure. Use sequence zero.
		iSequence = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : NewActivity - 
//			iSequence - 
//			translatedActivity - 
//			weaponActivity - 
//-----------------------------------------------------------------------------
void CAI_BaseNPC::SetActivityAndSequence(Activity NewActivity, int iSequence, Activity translatedActivity, Activity weaponActivity)
{
	m_translatedActivity = translatedActivity;

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( GetSequence() != iSequence || !SequenceLoops() )
		{
			//
			// Don't reset frame between movement phased animations
			if (!IsActivityMovementPhased( m_Activity ) || 
				!IsActivityMovementPhased( NewActivity ))
			{
				m_flCycle = 0;
			}
		}

		ResetSequence( iSequence );
		Weapon_SetActivity( weaponActivity, SequenceDuration( iSequence ) );
	}
	else
	{
		// Not available try to get default anim
		ResetSequence( 0 );
	}

	// Set the view position based on the current activity
	SetViewOffset( EyeOffset(m_translatedActivity) );

	if (m_Activity != NewActivity)
	{
		OnChangeActivity(NewActivity);
	}

	// NOTE: We DO NOT write the translated activity here.
	// This is to abstract the activity translation from the AI code.
	// As far as the code is concerned, a translation is merely a new set of sequences
	// that should be regarded as the activity in question.

	// Go ahead and set this so it doesn't keep trying when the anim is not present
	m_Activity = NewActivity;

	// this cannot be called until m_Activity stores NewActivity!
	GetMotor()->RecalculateYawSpeed();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the activity to the desired activity immediately, skipping any
//			transition sequences.
// Input  : NewActivity - 
//-----------------------------------------------------------------------------
void CAI_BaseNPC::SetActivity( Activity NewActivity )
{
	// If I'm already doing the NewActivity I can bail.
	// FIXME: Should this be based on the current translated activity and ideal translated activity (calculated below)?
	//		  The old code only cared about the logical activity, not translated.
	if (m_Activity == NewActivity)
	{
		return;
	}

	// Don't do this if I'm playing a transition, unless it's ACT_RESET.
	if ((NewActivity != ACT_RESET) && (m_Activity == ACT_TRANSITION))
	{
		return;
	}

	// In case someone calls this with something other than the ideal activity.
	m_IdealActivity = NewActivity;

	// Resolve to ideals and apply directly, skipping transitions.
	ResolveActivityToSequence(m_IdealActivity, m_nIdealSequence, m_IdealTranslatedActivity, m_IdealWeaponActivity);

	//Msg("%s: SLAM %s -> %s\n", GetClassname(), GetSequenceName(GetSequence()), GetSequenceName(m_nIdealSequence));

	SetActivityAndSequence(m_IdealActivity, m_nIdealSequence, m_IdealTranslatedActivity, m_IdealWeaponActivity);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the activity that we would like to transition toward.
// Input  : NewActivity - 
//-----------------------------------------------------------------------------
void CAI_BaseNPC::SetIdealActivity( Activity NewActivity )
{
	// FIXME: Don't do this if I'm playing a transition?
	//if (m_Activity == ACT_TRANSITION)
	//{
	//	return;
	//}

	if (NewActivity == ACT_RESET)
	{
		// They probably meant to call SetActivity(ACT_RESET)... we'll fix it for them.
		SetActivity(ACT_RESET);
		return;
	}

	m_IdealActivity = NewActivity;

	if( NewActivity == ACT_DO_NOT_DISTURB )
	{
		// Don't resolve anything! Leave it the way the user has it right now.
		return;
	}

	// Perform translation in case we need to change sequences within a single activity,
	// such as between a standing idle and a crouching idle.
	ResolveActivityToSequence(m_IdealActivity, m_nIdealSequence, m_IdealTranslatedActivity, m_IdealWeaponActivity);
}


//-----------------------------------------------------------------------------
// Purpose: Moves toward the ideal activity through any transition sequences.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::AdvanceToIdealActivity(void)
{
	// If there is a transition sequence between the current sequence and the ideal sequence...
	int nNextSequence = FindTransitionSequence(GetSequence(), m_nIdealSequence, NULL);
	if (nNextSequence != -1)
	{
		// We found a transition sequence or possibly went straight to
		// the ideal sequence.
		if (nNextSequence != m_nIdealSequence)
		{
			//Msg("%s: TRANSITION %s -> %s -> %s\n", GetClassname(), GetSequenceName(GetSequence()), GetSequenceName(nNextSequence), GetSequenceName(m_nIdealSequence));

			Activity eWeaponActivity = ACT_TRANSITION;
			Activity eTranslatedActivity = ACT_TRANSITION;

			// Figure out if the transition sequence has an associated activity that
			// we can use for our weapon. Do activity translation also.
			Activity eTransitionActivity = GetSequenceActivity(nNextSequence);
			if (eTransitionActivity != ACT_INVALID)
			{
				int nDiscard;
				ResolveActivityToSequence(eTransitionActivity, nDiscard, eTranslatedActivity, eWeaponActivity);
			}

			// Set activity and sequence to the transition stuff. Set the activity to ACT_TRANSITION
			// so we know we're in a transition.
			SetActivityAndSequence(ACT_TRANSITION, nNextSequence, eTranslatedActivity, eWeaponActivity);
		}
		else
		{
			//Msg("%s: IDEAL %s -> %s\n", GetClassname(), GetSequenceName(GetSequence()), GetSequenceName(m_nIdealSequence));

			// Set activity and sequence to the ideal stuff that was set up in MaintainActivity.
			SetActivityAndSequence(m_IdealActivity, m_nIdealSequence, m_IdealTranslatedActivity, m_IdealWeaponActivity);
		}
	}
	// Else go straight there to the ideal activity.
	else
	{
		//Msg("%s: Unable to get from sequence %s to %s!\n", GetClassname(), GetSequenceName(GetSequence()), GetSequenceName(m_nIdealSequence));
		SetActivity(m_IdealActivity);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Tries to achieve our ideal animation state, playing any transition
//			sequences that we need to play to get there.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::MaintainActivity(void)
{
	if ( m_lifeState == LIFE_DEAD )
	{
		// Don't maintain activities if we're daid.
		// Blame Speyrer
		return;
	}

	if ((GetState() == NPC_STATE_SCRIPT))
	{
		// HACK: finish any transitions we might be playing before we yield control to the script
		if (GetActivity() != ACT_TRANSITION)
		{
			// Our animation state is being controlled by a script.
			return;
		}
	}

	if( m_IdealActivity == ACT_DO_NOT_DISTURB )
	{
		return;
	}

	// We may have work to do if we aren't playing our ideal activity OR if we
	// aren't playing our ideal sequence.
	if ((GetActivity() != m_IdealActivity) || (GetSequence() != m_nIdealSequence))
	{
		bool bAdvance = false;

		// If we're in a transition activity, see if we are done with the transition.
		if (GetActivity() == ACT_TRANSITION)
		{
			// If the current sequence is finished, try to go to the next one
			// closer to our ideal sequence.
			if (IsSequenceFinished())
			{
				bAdvance = true;
			}
			// Else a transition sequence is in progress, do nothing.
		}
		// Else get a specific sequence for the activity and try to transition to that.
		else
		{
			// Save off a target sequence and translated activities to apply when we finish
			// playing all the transitions and finally arrive at our ideal activity.
			ResolveActivityToSequence(m_IdealActivity, m_nIdealSequence, m_IdealTranslatedActivity, m_IdealWeaponActivity);
			bAdvance = true;
		}
		
		if (bAdvance)
		{
			// Try to go to the next sequence closer to our ideal sequence.
			AdvanceToIdealActivity();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if our ideal activity has finished playing.
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsActivityFinished( void )
{
	return (IsSequenceFinished() && (GetSequence() == m_nIdealSequence));
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the activity is one of the standard phase-matched movement activities
// Input  : activity
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsActivityMovementPhased( Activity activity )
{
	switch( activity )
	{
	case ACT_WALK:
	case ACT_WALK_AIM:
	case ACT_WALK_CROUCH:
	case ACT_WALK_CROUCH_AIM:
	case ACT_RUN:
	case ACT_RUN_AIM:
	case ACT_RUN_CROUCH:
	case ACT_RUN_CROUCH_AIM:
	case ACT_RUN_PROTECTED:
		return true;
	}
	return false;
}

//=========================================================
// SetSequenceByName
//=========================================================
void CAI_BaseNPC::SetSequenceByName ( char *szSequence )
{
	int	iSequence;

	iSequence = LookupSequence ( szSequence );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( GetSequence() != iSequence || !SequenceLoops() )
		{
			m_flCycle = 0;
		}

		ResetSequence( iSequence );	// Set to the reset anim (if it's there)
		GetMotor()->RecalculateYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		DevWarning( 2, "%s has no sequence named:%f\n", GetClassname(), szSequence );
		SetSequence( 0 );	// Set to the reset anim (if it's there)
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the target entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *CAI_BaseNPC::GetNavTargetEntity(void)
{
	if ( GetNavigator()->GetGoalType() == GOALTYPE_ENEMY )
		return m_hEnemy;
	else if ( GetNavigator()->GetGoalType() == GOALTYPE_TARGETENT )
		return m_hTargetEnt;
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: returns zero if the caller can jump from
//			vecStart to vecEnd ignoring collisions with pTarget
//
//			if the throw fails, returns the distance
//			that can be travelled before an obstacle is hit
//-----------------------------------------------------------------------------
#include "ai_initutils.h"
//#define _THROWDEBUG
float CAI_BaseNPC::ThrowLimit(	const Vector &vecStart,
								const Vector &vecEnd,
								float		fGravity,
								float		fArcSize,
								const Vector &mins,
								const Vector &maxs,
								CBaseEntity *pTarget,
								Vector		*jumpVel,
								CBaseEntity **pBlocker)
{
	// Get my jump velocity
	Vector rawJumpVel	= CalcThrowVelocity(vecStart, vecEnd, fGravity, fArcSize);
	*jumpVel			= rawJumpVel;
	Vector vecFrom		= vecStart;

	// Calculate the total time of the jump minus a tiny fraction
	float jumpTime		= (vecStart - vecEnd).Length2D()/rawJumpVel.Length2D();
	float timeStep		= jumpTime / 10.0;

	Vector gravity = Vector(0,0,fGravity);

	// this loop takes single steps to the goal.
	for (float flTime = 0 ; flTime < jumpTime-0.1 ; flTime += timeStep )
	{
		// Calculate my position after the time step (average velocity over this time step)
		Vector nextPos = vecFrom + (rawJumpVel - 0.5 * gravity * timeStep) * timeStep;

		// If last time step make next position the target position
		if ((flTime + timeStep) > jumpTime)
		{
			nextPos = vecEnd;
		}

		trace_t tr;
		AI_TraceHull( vecFrom, nextPos, mins, maxs, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

		if (tr.startsolid || tr.fraction < 1.0)
		{
			CBaseEntity *pEntity = tr.m_pEnt;

			// If we hit the target we are good to go!
			if (pEntity == pTarget)
			{
				return 0;
			}

#ifdef _THROWDEBUG
			NDebugOverlay::Line( vecFrom, nextPos, 255, 0, 0, true, 1.0 );
#endif
			// ----------------------------------------------------------
			// If blocked by an npc remember
			// ----------------------------------------------------------
			*pBlocker = pEntity;

			// Return distance sucessfully traveled before block encountered
			return ((tr.endpos - vecStart).Length());
		}
#ifdef _THROWDEBUG
		else
		{
			NDebugOverlay::Line( vecFrom, nextPos, 255, 255, 255, true, 1.0 );
		}
#endif


		rawJumpVel  = rawJumpVel - gravity * timeStep;
		vecFrom		= nextPos;
	}
	return 0;
}



//-----------------------------------------------------------------------------
// Purpose: Called to initialize or re-initialize the vphysics hull when the size
//			of the NPC changes
//-----------------------------------------------------------------------------
void CAI_BaseNPC::SetupVPhysicsHull()
{
	if ( GetMoveType() == MOVETYPE_VPHYSICS )
		return;

	if ( VPhysicsGetObject() )
	{
		VPhysicsDestroyObject();
	}
	VPhysicsInitShadow( true, false );
	IPhysicsObject *pPhysObj = VPhysicsGetObject();
	if ( pPhysObj )
	{
		float mass = Studio_GetMass(GetModelPtr());
		if ( mass > 0 )
		{
			pPhysObj->SetMass( mass );
		}
#if _DEBUG
		else
		{
			Msg("Warning: %s has no physical mass\n", STRING(GetModelName()));
		}
#endif
		IPhysicsShadowController *pController = pPhysObj->GetShadowController();
		float avgsize = (WorldAlignSize().x + WorldAlignSize().y) * 0.5;
		pController->SetTeleportDistance( avgsize * 0.5 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: To be called instead of UTIL_SetSize, so pathfinding hull
//			and actual hull agree
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::SetHullSizeNormal( bool force )
{
	if ( m_fIsUsingSmallHull || force )
	{
		UTIL_SetSize(this, GetHullMins(),GetHullMaxs());
		m_fIsUsingSmallHull = false;
		if ( VPhysicsGetObject() )
		{
			SetupVPhysicsHull();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: To be called instead of UTIL_SetSize, so pathfinding hull
//			and actual hull agree
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::SetHullSizeSmall( bool force )
{
	if ( !m_fIsUsingSmallHull || force )
	{
		UTIL_SetSize(this, NAI_Hull::SmallMins(GetHullType()),NAI_Hull::SmallMaxs(GetHullType()));
		m_fIsUsingSmallHull = true;
		if ( VPhysicsGetObject() )
		{
			SetupVPhysicsHull();
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Checks to see that the nav hull is valid for the NPC
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsNavHullValid() const
{
	Assert( GetSolid() != SOLID_BSP );

	Vector hullMin = GetHullMins();
	Vector hullMax = GetHullMaxs();
	Vector vecMins, vecMaxs;
	if ( GetSolid() == SOLID_BBOX )
	{
		vecMins = WorldAlignMins();
		vecMaxs = WorldAlignMaxs();
	}
	else if ( GetSolid() == SOLID_VPHYSICS )
	{
		Assert( VPhysicsGetObject() );
		const CPhysCollide *pPhysCollide = VPhysicsGetObject()->GetCollide();
		physcollision->CollideGetAABB( vecMins, vecMaxs, pPhysCollide, GetAbsOrigin(), GetAbsAngles() ); 
		vecMins -= GetAbsOrigin();
		vecMaxs -= GetAbsOrigin();
	}
	else
	{
		vecMins = hullMin;
		vecMaxs = hullMax;
	}

	if ( (hullMin.x > vecMins.x) || (hullMax.x < vecMaxs.x) ||
		(hullMin.y > vecMins.y) || (hullMax.y < vecMaxs.y) ||
		(hullMin.z > vecMins.z) || (hullMax.z < vecMaxs.z) )
	{
		return false;
	}

	return true;
}


//=========================================================
// NPCInit - after a npc is spawned, it needs to
// be dropped into the world, checked for mobility problems,
// and put on the proper path, if any. This function does
// all of those things after the npc spawns. Any
// initialization that should take place for all npcs
// goes here.
//=========================================================
void CAI_BaseNPC::NPCInit ( void )
{
	if (!g_pGameRules->FAllowNPCs())
	{
		UTIL_Remove( this );
		return;
	}

#ifdef _DEBUG
	// Make sure that the bounding box is appropriate for the hull size...
	// FIXME: We can't test vphysics objects because NPCInit occurs before VPhysics is set up
	if ( GetSolid() != SOLID_VPHYSICS )
	{
		if ( !IsNavHullValid() )
		{
			Warning("NPC Entity %s (%d) has a bounding box which extends outside its nav box!\n",
				STRING(m_iClassname), entindex() );
		}
	}
#endif

	// Set fields common to all npcs
	AddFlag( FL_AIMTARGET | FL_NPC );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetGravity(1.0);	// Don't change
	m_takedamage		= DAMAGE_YES;
	GetMotor()->SetIdealYaw( GetLocalAngles().y );
	m_iMaxHealth		= m_iHealth;
	m_lifeState			= LIFE_ALIVE;
	m_IdealNPCState		= NPC_STATE_IDLE;// Assume npc will be idle, until proven otherwise
	SetIdealActivity( ACT_IDLE );

	ClearCommandGoal();
	m_hCommandTarget.Set( NULL );

	ClearSchedule();
	GetNavigator()->ClearGoal();
	InitBoneControllers( ); // FIX: should be done in Spawn
	ResetActivityIndexes();

	m_pHintNode			= NULL;

	m_afMemory			= MEMORY_CLEAR;

	SetEnemy( NULL );

	m_flDistTooFar		= 1024.0;
	SetDistLook( 2048.0 );

	if ( HasSpawnFlags( SF_NPC_LONG_RANGE ) )
	{
		m_flDistTooFar	= 1e9f;
		SetDistLook( 6000.0 );
	}

	// Clear conditions
	m_Conditions.ClearAllBits();

	// set eye position
	SetDefaultEyeOffset();

	// Only give weapon of allowed to have one
	if (CapabilitiesGet() & bits_CAP_USE_WEAPONS)
	{	// Does this npc spawn with a weapon
		if ( m_spawnEquipment != NULL_STRING && strcmp(STRING(m_spawnEquipment), "0"))
		{
			CBaseCombatWeapon *pWeapon = Weapon_Create( STRING(m_spawnEquipment) );
			if ( pWeapon )
			{
				Weapon_Equip( pWeapon );
			}
		}
	}

	SetUse ( &CAI_BaseNPC::NPCUse );

	if( gpGlobals->curtime > 1.0 )
	{
		// After this much time, we can be sure that everything in the World
		// is spawned, so finish all initialization immediately by calling
		// the init think directly. (sjb)
		NPCInitThink();
	}
	else
	{
		// We're at worldspawn, so we must put off the rest of our initialization
		// until we're sure everything else has had a chance to spawn. Otherwise
		// we may try to reference entities that haven't spawned yet.(sjb)
		SetThink( &CAI_BaseNPC::NPCInitThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	CreateVPhysics();

	if ( HasSpawnFlags( SF_NPC_START_EFFICIENT ) )
	{
		SetEfficiency( AIE_EFFICIENT );
	}

	m_GiveUpOnDeadEnemyTimer.Set( 0.75, 2.0 );
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::CreateVPhysics()
{
	if ( IsAlive() && !VPhysicsGetObject() )
	{
		SetupVPhysicsHull();
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Initialze the relationship table from the keyvalues
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InitRelationshipTable(void)
{
	AddRelationship( STRING( m_RelationshipString ), NULL );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_BaseNPC::AddRelationship( const char *pszRelationship, CBaseEntity *pActivator )
{
	// Parse the keyvalue data
	char parseString[1000];
	Q_strncpy(parseString, pszRelationship, sizeof(parseString));

	// Look for an entity string
	char *entityString = strtok(parseString," ");
	while (entityString)
	{
		// Get the disposition
		char *dispositionString = strtok(NULL," ");
		Disposition_t disposition = D_NU;
		if ( dispositionString )
		{
			if (!stricmp(dispositionString,"D_HT"))
			{
				disposition = D_HT;
			}
			else if (!stricmp(dispositionString,"D_FR"))
			{
				disposition = D_FR;
			}
			else if (!stricmp(dispositionString,"D_LI"))
			{
				disposition = D_LI;
			}
			else if (!stricmp(dispositionString,"D_NU"))
			{
				disposition = D_NU;
			}
			else
			{
				disposition = D_NU;
				Warning( "***ERROR***\nBad relationship type (%s) to unknown entity (%s)!\n", dispositionString,entityString );
				Assert( 0 );
				return;
			}
		}
		else
		{
			Warning("Can't parse relationship info (%s)\n", pszRelationship );
			Assert(0);
			return;
		}

		// Get the priority
		char *priorityString	= strtok(NULL," ");
		int	priority = ( priorityString ) ? atoi(priorityString) : DEF_RELATIONSHIP_PRIORITY;

		// Try to get pointer to an entity of this name
		CBaseEntity *entity = gEntList.FindEntityByName( NULL, entityString, NULL );
		if (entity)
		{
			AddEntityRelationship(entity, disposition, priority );
		}
		else
		{
			// Need special condition for player as we can only have one
			if (!stricmp("player",entityString))
			{
				AddClassRelationship( CLASS_PLAYER, disposition, priority );
			}
			// Otherwise try to create one too see if a valid classname and get class type
			else
			{
				// HACKHACK:
				CBaseEntity *pEntity = CreateEntityByName( entityString );
				if (pEntity)
				{
					AddClassRelationship( pEntity->Classify(), disposition, priority );
					UTIL_RemoveImmediate(pEntity);
				}
				else
				{
					Warning( "***ERROR***\nCouldn't set relationship to unknown entity or class (%s)!\n", entityString );
				}
			}
		}
		// Check for another entity in the list
		entityString		= strtok(NULL," ");
	}
}

//=========================================================
// NPCInitThink - Calls StartNPC. Startnpc is
// virtual, but this function cannot be
//=========================================================
void CAI_BaseNPC::NPCInitThink ( void )
{
	// Initialize the relationship table
	InitRelationshipTable();

	StartNPC();

	PostNPCInit();
}

//=========================================================
// StartNPC - final bit of initization before a npc
// is turned over to the AI.
//=========================================================
void CAI_BaseNPC::StartNPC ( void )
{
	// Raise npc off the floor one unit, then drop to floor
	if ( (GetMoveType() != MOVETYPE_FLY) && (GetMoveType() != MOVETYPE_FLYGRAVITY) &&
		 !(CapabilitiesGet() & bits_CAP_MOVE_FLY) &&
		 !HasSpawnFlags( SF_NPC_FALL_TO_GROUND ) )
	{
		Vector origin = GetLocalOrigin();

		if (!GetMoveProbe()->FloorPoint( origin, MASK_NPCSOLID, 0, -256, &origin ))
		{
			Warning( "NPC %s stuck in wall--level design error\n", GetClassname());
		}

		SetLocalOrigin( origin );
	}
	else
	{
		RemoveFlag( FL_ONGROUND );
	}

	if ( m_target != NULL_STRING )// this npc has a target
	{
		// Find the npc's initial target entity, stash it
		m_pGoalEnt = gEntList.FindEntityByName( NULL, m_target, NULL );

		if ( !m_pGoalEnt )
		{
			Warning( "ReadyNPC()--%s couldn't find target %s", GetClassname(), STRING(m_target));
		}
		else
		{
			StartTargetHandling( m_pGoalEnt );
		}
	}

	//SetState ( m_IdealNPCState );
	//SetActivity ( m_IdealActivity );

	InitSquad();

	if( gpGlobals->curtime > 2.0 )
	{
		// If this NPC is spawning late in the game, just push through the rest of the initialization
		// start thinking right now.
		SetThink ( &CAI_BaseNPC::CallNPCThink );
		SetNextThink( gpGlobals->curtime );
		//CallNPCThink();
	}
	else
	{
		// Delay drop to floor to make sure each door in the level has had its chance to spawn
		// Spread think times so that they don't all happen at the same time (Carmack)
		SetThink ( &CAI_BaseNPC::CallNPCThink );
		SetNextThink( gpGlobals->curtime + random->RandomFloat(0.1, 1.0) ); // spread think times
	}

	m_ScriptArrivalActivity = AIN_DEF_ACTIVITY;
	m_strScriptArrivalSequence = NULL_STRING;

	if ( HasSpawnFlags(SF_NPC_WAIT_FOR_SCRIPT) )
	{
		SetState( NPC_STATE_IDLE );
		// UNDONE: Some scripted sequence npcs don't have an idle?
		SetActivity( ACT_IDLE );
		SetSchedule( SCHED_WAIT_FOR_SCRIPT );
	}
}

//-----------------------------------------------------------------------------

void CAI_BaseNPC::StartTargetHandling( CBaseEntity *pTargetEnt )
{
	// set the npc up to walk a path corner path.
	// !!!BUGBUG - this is a minor bit of a hack.
	// JAYJAY

	// NPC will start turning towards his destination
	bool bIsFlying = (GetMoveType() == MOVETYPE_FLY) || (GetMoveType() == MOVETYPE_FLYGRAVITY);
	AI_NavGoal_t goal( GOALTYPE_PATHCORNER, pTargetEnt->GetLocalOrigin(),
					   bIsFlying ? ACT_FLY : ACT_WALK,
					   AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST);

	SetState( NPC_STATE_IDLE );
	SetSchedule( SCHED_IDLE_WALK );

	if ( !GetNavigator()->SetGoal( goal ) )
	{
		DevWarning( 2, "Can't Create Route!\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Connect my memory to the squad's
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::InitSquad( void )
{
	// -------------------------------------------------------
	//  If I form squads add me to a squad
	// -------------------------------------------------------
	if (!m_pSquad && ( CapabilitiesGet() & bits_CAP_SQUAD ))
	{
		if ( !m_SquadName )
		{
			Msg("WARNING: Found %s that isn't in a squad\n",GetClassname());
		}
		else
		{
			m_pSquad = g_AI_SquadManager.FindCreateSquad(this, m_SquadName);
		}
	}

	return ( m_pSquad != NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Get the memory for this NPC
//-----------------------------------------------------------------------------
CAI_Enemies *CAI_BaseNPC::GetEnemies( void )
{
	return m_pEnemies;
}

//-----------------------------------------------------------------------------
// Purpose: Remove this NPC's memory
//-----------------------------------------------------------------------------
void CAI_BaseNPC::RemoveMemory( void )
{
	delete m_pEnemies;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_BaseNPC::TaskComplete(  bool fIgnoreSetFailedCondition )
{
	EndTaskOverlay();

	// Handy thing to use for debugging
	//if (IsCurSchedule(SCHED_PUT_HERE) &&
	//	GetTask()->iTask == TASK_PUT_HERE)
	//{
	//	int put_breakpoint_here = 5;
	//}

	if ( fIgnoreSetFailedCondition || !HasCondition(COND_TASK_FAILED) )
	{
		SetTaskStatus( TASKSTATUS_COMPLETE );
	}
}

void CAI_BaseNPC::TaskMovementComplete( void )
{
	switch( GetTaskStatus() )
	{
	case TASKSTATUS_NEW:
	case TASKSTATUS_RUN_MOVE_AND_TASK:
		SetTaskStatus( TASKSTATUS_RUN_TASK );
		break;

	case TASKSTATUS_RUN_MOVE:
		TaskComplete();
		break;

	case TASKSTATUS_RUN_TASK:
		Warning( "Movement completed twice!\n" );
		Assert( 0 );
		break;

	case TASKSTATUS_COMPLETE:
		break;
	}

	// JAY: Put this back in.
	// UNDONE: Figure out how much of the timestep was consumed by movement
	// this frame and restart the movement/schedule engine if necessary
	if ( m_scriptState != SCRIPT_CUSTOM_MOVE_TO_MARK )
	{
		SetIdealActivity( GetStoppedActivity() );
	}

	// Advance past the last node (in case there is some event at this node)
	if ( GetNavigator()->IsGoalActive() )
	{
		GetNavigator()->AdvancePath();
	}

	// Now clear the path, it's done.
	GetNavigator()->ClearGoal();
}


int CAI_BaseNPC::TaskIsRunning( void )
{
	if ( GetTaskStatus() != TASKSTATUS_COMPLETE &&
		 GetTaskStatus() != TASKSTATUS_RUN_MOVE )
		 return 1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::TaskFail( AI_TaskFailureCode_t code )
{
	EndTaskOverlay();

	// Handy tool for debugging
	//if (IsCurSchedule(SCHED_PUT_NAME_HERE))
	//{
	//	int put_breakpoint_here = 5;
	//}

	// If in developer mode save the fail text for debug output
	if (g_pDeveloper->GetInt())
	{
		m_failText = TaskFailureToString( code );

		m_interuptSchedule	= NULL;
		m_failedSchedule    = GetCurSchedule();

		if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
		{
			Msg(this, AIMF_IGNORE_SELECTED, "      TaskFail -> %s\n", m_failText );
		}

		//AddTimedOverlay( fail_text, 5);
	}

	m_ScheduleState.taskFailureCode = code;
	SetCondition(COND_TASK_FAILED);
}

//------------------------------------------------------------------------------
// Purpose : Remember that this entity wasn't reachable
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_BaseNPC::RememberUnreachable(CBaseEntity *pEntity)
{
	const float NPC_UNREACHABLE_TIMEOUT = 3;
	// Only add to list if not already on it
	for (int i=m_UnreachableEnts.Size()-1;i>=0;i--)
	{
		// If record already exists just update mark time
		if (pEntity == m_UnreachableEnts[i].hUnreachableEnt)
		{
			m_UnreachableEnts[i].fExpireTime	 = gpGlobals->curtime + NPC_UNREACHABLE_TIMEOUT;
			m_UnreachableEnts[i].vLocationWhenUnreachable = pEntity->GetAbsOrigin();
			return;
		}
	}

	// Add new unreachabe entity to list
	int nNewIndex = m_UnreachableEnts.AddToTail();
	m_UnreachableEnts[nNewIndex].hUnreachableEnt = pEntity;
	m_UnreachableEnts[nNewIndex].fExpireTime	 = gpGlobals->curtime + NPC_UNREACHABLE_TIMEOUT;
	m_UnreachableEnts[nNewIndex].vLocationWhenUnreachable = pEntity->GetAbsOrigin();
}

//------------------------------------------------------------------------------
// Purpose : Returns true is entity was remembered as unreachable.
//			 After a time delay reachability is checked
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_BaseNPC::IsUnreachable(CBaseEntity *pEntity)
{
	float UNREACHABLE_DIST_TOLERANCE_SQ = (120*120);

	// Note that it's ok to remove elements while I'm iterating
	// as long as I iterate backwards and remove them using FastRemove
	for (int i=m_UnreachableEnts.Size()-1;i>=0;i--)
	{
		// Remove any dead elements
		if (m_UnreachableEnts[i].hUnreachableEnt == NULL)
		{
			m_UnreachableEnts.FastRemove(i);
		}
		else if (pEntity == m_UnreachableEnts[i].hUnreachableEnt)
		{
			// Test for reachablility on any elements that have timed out
			if ( gpGlobals->curtime > m_UnreachableEnts[i].fExpireTime ||
				  pEntity->GetAbsOrigin().DistToSqr(m_UnreachableEnts[i].vLocationWhenUnreachable) > UNREACHABLE_DIST_TOLERANCE_SQ)
			{
				m_UnreachableEnts.FastRemove(i);
				return false;
			}
			return true;
		}
	}
	return false;
}

bool CAI_BaseNPC::IsValidEnemy( CBaseEntity *pEnemy )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Picks best enemy from my list of enemies
//			Prefers reachable enemies over enemies that are unreachable,
//			regardless of priority.  For enemies that are both reachable or
//			unreachable picks by priority.  If priority is the same, picks
//			by distance.
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *CAI_BaseNPC::BestEnemy ( void )
{
	// TODO - may want to consider distance, attack types, back turned, etc.

	CBaseEntity*	pBestEnemy			= NULL;
	int				iBestDistSq			= MAX_COORD_RANGE * MAX_COORD_RANGE;// so first visible entity will become the closest.
	int				iBestPriority		= -1000;
	bool			bBestUnreachable	= true;			  // Forces initial check
	bool			bBestSeen			= false;
	int				iDistSq;
	bool			bUnreachable		= false;

	AIEnemiesIter_t iter;
	for( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter) )
	{
		CBaseEntity *pEnemy = pEMemory->hEnemy;

		if (!pEnemy || !pEnemy->IsAlive())
			continue;
		
		// UNDONE: Move relationship checks into IsValidEnemy?
		if ( (pEnemy->GetFlags() & FL_NOTARGET) || 
			(IRelationType( pEnemy ) != D_HT && IRelationType( pEnemy ) != D_FR) ||
			!IsValidEnemy( pEnemy ) )
		{
			continue;
		}

		if ( pEMemory->flLastTimeSeen < m_flAcceptableTimeSeenEnemy )
			continue;

		// Skip enemies that have eluded me to prevent infinite loops
		if (GetEnemies()->HasEludedMe(pEnemy))
			continue;

		// establish the reachability of this enemy
		bUnreachable = IsUnreachable(pEnemy);

		// If best is reachable and current is unreachable, skip the unreachable enemy regardless of priority
		if (!bBestUnreachable && bUnreachable)
			continue;

		//  If best is unreachable and current is reachable, always pick the current regardless of priority
		if (bBestUnreachable && !bUnreachable)
		{
			bBestSeen 		 = ( GetSenses()->DidSeeEntity( pEnemy ) || FVisible( pEnemy ) ); // @TODO (toml 04-02-03): Need to optimize CanSeeEntity() so multiple calls in frame do not recalculate, rather cache
			iBestPriority	 = IRelationPriority ( pEnemy );
			iBestDistSq		 = (pEnemy->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
			pBestEnemy		 = pEnemy;
			bBestUnreachable = bUnreachable;
		}
		// If both are unreachable or both are reachable, chose enemy based on priority and distance
		else if ( IRelationPriority( pEnemy ) > iBestPriority )
		{
			// this entity is disliked MORE than the entity that we
			// currently think is the best visible enemy. No need to do
			// a distance check, just get mad at this one for now.
			iBestPriority	 = IRelationPriority ( pEnemy );
			iBestDistSq		 = ( pEnemy->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
			pBestEnemy		 = pEnemy;
			bBestUnreachable = bUnreachable;
		}
		else if ( IRelationPriority( pEnemy ) == iBestPriority )
		{
			// this entity is disliked just as much as the entity that
			// we currently think is the best visible enemy, so we only
			// get mad at it if it is closer.
			iDistSq = ( pEnemy->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();

			bool bCloser = ( iDistSq < iBestDistSq ) ;

			if ( bCloser || !bBestSeen )
			{
				// @TODO (toml 04-02-03): Need to optimize FVisible() so multiple calls in frame do not recalculate, rather cache
				bool fSeen = ( GetSenses()->DidSeeEntity( pEnemy ) || FVisible( pEnemy ) );
				if ( ( bCloser && ( fSeen || !bBestSeen ) ) || ( !bCloser && !bBestSeen && fSeen ) )
				{
					bBestSeen		 = fSeen;
					iBestDistSq		 = iDistSq;
					iBestPriority	 = IRelationPriority ( pEnemy );
					pBestEnemy		 = pEnemy;
					bBestUnreachable = bUnreachable;
				}
			}
		}
	}

	return pBestEnemy;
}

//-----------------------------------------------------------------------------
// Purpose: Given a node returns the appropriate reload activity
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CAI_BaseNPC::GetReloadActivity( CAI_Hint* pHint )
{
	Activity nReloadActivity = ACT_RELOAD;

	if (pHint && GetEnemy()!=NULL)
	{
		switch (pHint->HintType())
		{
			case HINT_TACTICAL_COVER_LOW:
			case HINT_TACTICAL_COVER_MED:
			{
				if (SelectWeightedSequence( ACT_RELOAD_LOW ) != ACTIVITY_NOT_AVAILABLE)
				{
					Vector vEyePos = GetAbsOrigin() + EyeOffset(ACT_RELOAD_LOW);
					// Check if this location will block the threat's line of sight to me
					trace_t tr;
					AI_TraceLine ( vEyePos, GetEnemy()->EyePosition(), MASK_OPAQUE,  this, COLLISION_GROUP_NONE, &tr );
					if (tr.fraction != 1.0)
					{
						nReloadActivity = ACT_RELOAD_LOW;
					}
				}
				break;
			}
		}
	}
	return nReloadActivity;
}

//-----------------------------------------------------------------------------
// Purpose: Given a node returns the appropriate cover activity
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CAI_BaseNPC::GetCoverActivity( CAI_Hint *pHint )
{
	Activity nCoverActivity;

	// ---------------------------------------------------------------
	// Some NPCs don't have a cover activity defined so just use idle
	// ---------------------------------------------------------------
	if (SelectWeightedSequence( ACT_COVER ) == ACTIVITY_NOT_AVAILABLE)
	{
		nCoverActivity = ACT_IDLE;
	}
	else
	{
		nCoverActivity = ACT_COVER;
	}

	// ---------------------------------------------------------------
	//  Check if hint node specifies different cover type
	// ---------------------------------------------------------------
	if (pHint)
	{
		switch (pHint->HintType())
		{
			case HINT_TACTICAL_COVER_MED:
			{
				if (SelectWeightedSequence( ACT_COVER_MED ) != ACTIVITY_NOT_AVAILABLE)
				{
					nCoverActivity = ACT_COVER_MED;
				}
				break;
			}
			case HINT_TACTICAL_COVER_LOW:
			{
				if (SelectWeightedSequence( ACT_COVER_LOW ) != ACTIVITY_NOT_AVAILABLE)
				{
					nCoverActivity = ACT_COVER_LOW;
				}
				break;
			}
		}
	}

	return nCoverActivity;
}

//=========================================================
// CalcIdealYaw - gets a yaw value for the caller that would
// face the supplied vector. Value is stuffed into the npc's
// ideal_yaw
//=========================================================
float CAI_BaseNPC::CalcIdealYaw( const Vector &vecTarget )
{
	Vector	vecProjection;

	// strafing npc needs to face 90 degrees away from its goal
	if ( GetNavigator()->GetMovementActivity() == ACT_STRAFE_LEFT )
	{
		vecProjection.x = -vecTarget.y;
		vecProjection.y = vecTarget.x;

		return UTIL_VecToYaw( vecProjection - GetLocalOrigin() );
	}
	else if ( GetNavigator()->GetMovementActivity() == ACT_STRAFE_RIGHT )
	{
		vecProjection.x = vecTarget.y;
		vecProjection.y = vecTarget.x;

		return UTIL_VecToYaw( vecProjection - GetLocalOrigin() );
	}
	else
	{
		return UTIL_VecToYaw ( vecTarget - GetLocalOrigin() );
	}
}

//=========================================================
// SetEyePosition
//
// queries the npc's model for $eyeposition and copies
// that vector to the npc's m_vDefaultEyeOffset and m_vecViewOffset
//
//=========================================================
void CAI_BaseNPC::SetDefaultEyeOffset ( void )
{
	GetEyePosition( GetModelPtr(), m_vDefaultEyeOffset );

	if ( m_vDefaultEyeOffset == vec3_origin )
	{
		Msg( "WARNING: %s has no eye offset in .qc!\n", GetClassname() );
		VectorAdd( WorldAlignMins(), WorldAlignMaxs(), m_vDefaultEyeOffset );
		m_vDefaultEyeOffset *= 0.75;
	}

	SetViewOffset( m_vDefaultEyeOffset );

}

//------------------------------------------------------------------------------
// Purpose : Returns eye offset for an NPC for the given activity
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseNPC::EyeOffset( Activity nActivity )
{
	if (CapabilitiesGet() & bits_CAP_DUCK)
	{
		if		((nActivity == ACT_RELOAD_LOW	) ||
				 (nActivity == ACT_COVER_LOW	) )
		{
			return (Vector(0,0,24));
		}
	}
	return ( m_vDefaultEyeOffset );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_BaseNPC::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case SCRIPT_EVENT_DEAD:
		if ( m_NPCState == NPC_STATE_SCRIPT )
		{
			m_lifeState = LIFE_DYING;
			// Kill me now! (and fade out when CineCleanup() is called)
#if _DEBUG
			DevMsg( 2, "Death event: %s\n", GetClassname() );
#endif
			m_iHealth = 0;
		}
#if _DEBUG
		else
			DevWarning( 2, "INVALID death event:%s\n", GetClassname() );
#endif
		break;
	case SCRIPT_EVENT_NOT_DEAD:
		if ( m_NPCState == NPC_STATE_SCRIPT )
		{
			m_lifeState = LIFE_ALIVE;
			// This is for life/death sequences where the player can determine whether a character is dead or alive after the script
			m_iHealth = m_iMaxHealth;
		}
		break;

	case SCRIPT_EVENT_SOUND:			// Play a named wave file
		{
			EmitSound( pEvent->options );
		}
		break;

	case SCRIPT_EVENT_SOUND_VOICE:
		{
			EmitSound( pEvent->options );
		}
		break;

	case SCRIPT_EVENT_SENTENCE_RND1:		// Play a named sentence group 33% of the time
		if (random->RandomInt(0,2) == 0)
			break;
		// fall through...
	case SCRIPT_EVENT_SENTENCE:			// Play a named sentence group
		SENTENCEG_PlayRndSz( edict(), pEvent->options, 1.0, SNDLVL_TALKING, 0, 100 );
		break;

	case SCRIPT_EVENT_FIREEVENT:
	{
		//
		// Fire a script event. The number of the script event to fire is in the options string.
		//
		if ( m_hCine != NULL )
		{
			m_hCine->FireScriptEvent( atoi( pEvent->options ) );
		}
		break;
	}

	case SCRIPT_EVENT_NOINTERRUPT:		// Can't be interrupted from now on
		if ( m_hCine )
			m_hCine->AllowInterrupt( false );
		break;

	case SCRIPT_EVENT_CANINTERRUPT:		// OK to interrupt now
		if ( m_hCine )
			m_hCine->AllowInterrupt( true );
		break;

#if 0
	case SCRIPT_EVENT_INAIR:			// Don't engine->DropToFloor()
	case SCRIPT_EVENT_ENDANIMATION:		// Set ending animation sequence to
		break;
#endif
	case SCRIPT_EVENT_BODYGROUPON:
	case SCRIPT_EVENT_BODYGROUPOFF:
	case SCRIPT_EVENT_BODYGROUPTEMP:
			Msg( "Bodygroup!\n" );
		break;

	case NPC_EVENT_BODYDROP_HEAVY:
		if ( GetFlags() & FL_ONGROUND )
		{
			EmitSound( "AI_BaseNPC.BodyDrop_Heavy" );
		}
		break;

	case NPC_EVENT_BODYDROP_LIGHT:
		if ( GetFlags() & FL_ONGROUND )
		{
			EmitSound( "AI_BaseNPC.BodyDrop_Light" );
		}
		break;

	case NPC_EVENT_SWISHSOUND:
		{
			// NO NPC may use this anim event unless that npc's precache precaches this sound!!!
			EmitSound( "AI_BaseNPC.SwishSound" );
			break;
		}


	case NPC_EVENT_180TURN:
		{
			//Msg( "Turned!\n" );
			SetIdealActivity( ACT_IDLE );
			Forget( bits_MEMORY_TURNING );
			SetBoneController( 0, GetLocalAngles().y );
			m_fEffects |= EF_NOINTERP;
			break;
		}

	case NPC_EVENT_WEAPON_PICKUP:
		{
			//
			// Pick up a weapon that we found earlier or that was specified by the animation.
			//
			CBaseCombatWeapon *pWeapon = NULL;
			if (pEvent->options && strlen( pEvent->options ) > 0 )
			{
				// Pick up the weapon that was specified in the anim event.
				CBaseEntity *pFound = gEntList.FindEntityGenericNearest(pEvent->options, GetAbsOrigin(), 256, this);
				pWeapon = dynamic_cast<CBaseCombatWeapon *>(pFound);
			}
			else
			{
				// Pick up the weapon that was found earlier and cached in our target pointer.
				pWeapon = dynamic_cast<CBaseCombatWeapon *>((CBaseEntity *)GetTarget());
			}

			if (pWeapon)
			{
				CBaseCombatCharacter *pOwner  = pWeapon->GetOwner();
				if (pOwner)
				{
					TaskFail( "Weapon in use by someone else" );
				}
				else if (!pWeapon)
				{
					TaskFail( "Weapon doesn't exist" );
				}
				else if (!Weapon_CanUse( pWeapon ))
				{
					TaskFail( "Can't use this weapon type" );
				}
				else
				{
					pWeapon->OnPickedUp( this );
					Weapon_Equip( pWeapon );
					TaskComplete();
				}
			}
			else
			{
				TaskFail( "Weapon stolen by someone else" );
			}

			break;
		}

	case NPC_EVENT_WEAPON_SET_SEQUENCE_NUMBER:
	{
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ((pWeapon) && (pEvent->options))
		{
			int nSequence = atoi(pEvent->options);
			if (nSequence != -1)
			{
				pWeapon->ResetSequence(nSequence);
			}
		}
		break;
	}

	case NPC_EVENT_WEAPON_SET_SEQUENCE_NAME:
	{
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ((pWeapon) && (pEvent->options))
		{
			int nSequence = pWeapon->LookupSequence(pEvent->options);
			if (nSequence != -1)
			{
				pWeapon->ResetSequence(nSequence);
			}
		}
		break;
	}

	case NPC_EVENT_WEAPON_SET_ACTIVITY:
	{
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ((pWeapon) && (pEvent->options))
		{
			Activity act = (Activity)pWeapon->LookupActivity(pEvent->options);
			if (act != ACT_INVALID)
			{
				// FIXME: where should the duration come from? normally it would come from the current sequence
				Weapon_SetActivity(act, 0);
			}
		}
		break;
	}

	case NPC_EVENT_WEAPON_DROP:
		{
			//
			// Drop our active weapon (or throw it at the specified target entity).
			//
			CBaseEntity *pTarget = NULL;
			if (pEvent->options)
			{
				pTarget = gEntList.FindEntityGeneric(NULL, pEvent->options, this);
			}

			if (pTarget)
			{
				Vector vecTargetPos = pTarget->WorldSpaceCenter();
				Weapon_Drop(GetActiveWeapon(), &vecTargetPos);
			}
			else
			{
				Weapon_Drop(GetActiveWeapon());
			}

			break;
		}

	case NPC_EVENT_LEFTFOOT:
	case NPC_EVENT_RIGHTFOOT:
		// For right now, do nothing. All functionality for this lives in individual npcs.
		break;

	default:
		// Came from my weapon?
		if ( pEvent->pSource != this || (pEvent->event >= EVENT_WEAPON && pEvent->event <= EVENT_WEAPON_LAST) )
		{
			Weapon_HandleAnimEvent( pEvent );
		}
		else
		{
			DevWarning( 2, "Unhandled animation event %d for %s\n", pEvent->event, GetClassname() );
		}
		break;

	}
}


//-----------------------------------------------------------------------------
// Purpose: Override base class to add display of routes
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
void CAI_BaseNPC::DrawDebugGeometryOverlays(void)
{
	// Handy for debug
	//NDebugOverlay::Cross3D(EyePosition(),Vector(-2,-2,-2),Vector(2,2,2),0,255,0,true);

	// ------------------------------
	// Remove me if requested
	// ------------------------------
	if (m_debugOverlays & OVERLAY_NPC_ZAP_BIT)
	{
		VacateStrategySlot();
		Weapon_Drop( GetActiveWeapon() );
		SetThink( &CAI_BaseNPC::SUB_Remove );
	}

	// ------------------------------
	// properly kill an NPC.
	// ------------------------------
	if (m_debugOverlays & OVERLAY_NPC_KILL_BIT)
	{
		CTakeDamageInfo info;

		info.SetDamage( m_iHealth );
		info.SetAttacker( UTIL_PlayerByIndex( 1 ) );
		info.SetInflictor( UTIL_PlayerByIndex( 1 ) );
		info.SetDamageType( DMG_GENERIC );

		TakeDamage( info );
		return;
	}


	// ------------------------------
	// Draw route if requested
	// ------------------------------
	if ((m_debugOverlays & OVERLAY_NPC_ROUTE_BIT))
	{
		GetNavigator()->DrawDebugRouteOverlay();
	}

	if (!(CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI) && (IsCurSchedule(SCHED_FORCED_GO) || IsCurSchedule(SCHED_FORCED_GO_RUN)))
	{
		NDebugOverlay::Box(m_vecLastPosition, Vector(-5,-5,-5),Vector(5,5,5), 255, 0, 255, 0, 0);
	}

	// ------------------------------
	// Draw red box around if selected
	// ------------------------------
	if ((m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
	{
		NDebugOverlay::EntityBounds(this, 255, 0, 0, 20, 0);
	}

	// ------------------------------
	// Draw nearest node if selected
	// ------------------------------
	if ((m_debugOverlays & OVERLAY_NPC_NEAREST_BIT))
	{
		int iNodeID = GetNavigator()->GetNetwork()->NearestNodeToNPCAtPoint( this, GetLocalOrigin() );
		if (iNodeID != NO_NODE)
		{
			NDebugOverlay::Box(GetNavigator()->GetNetwork()->AccessNodes()[iNodeID]->GetPosition(GetHullType()), Vector(-10,-10,-10),Vector(10,10,10), 255, 255, 255, 0, 0);
		}
	}

	// ------------------------------
	// Draw viewcone if selected
	// ------------------------------
	if ((m_debugOverlays & OVERLAY_NPC_VIEWCONE_BIT))
	{
		float flViewRange	= acos(m_flFieldOfView);
		Vector vEyeDir = EyeDirection2D( );
		Vector vLeftDir, vRightDir;
		float fSin, fCos;
		SinCos( flViewRange, &fSin, &fCos );

		vLeftDir.x			= vEyeDir.x * fCos - vEyeDir.y * fSin;
		vLeftDir.y			= vEyeDir.x * fSin + vEyeDir.y * fCos;
		vLeftDir.z			=  vEyeDir.z;
		fSin				= sin(-flViewRange);
		fCos				= cos(-flViewRange);
		vRightDir.x			= vEyeDir.x * fCos - vEyeDir.y * fSin;
		vRightDir.y			= vEyeDir.x * fSin + vEyeDir.y * fCos;
		vRightDir.z			=  vEyeDir.z;

		NDebugOverlay::BoxDirection(EyePosition(), Vector(0,0,-40), Vector(200,0,40), vLeftDir, 255, 0, 0, 50, 0 );
		NDebugOverlay::BoxDirection(EyePosition(), Vector(0,0,-40), Vector(200,0,40), vRightDir, 255, 0, 0, 50, 0 );
		NDebugOverlay::BoxDirection(EyePosition(), Vector(0,0,-40), Vector(200,0,40), vEyeDir, 0, 255, 0, 50, 0 );
		NDebugOverlay::Box(EyePosition(), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, 128, 0 );
	}

	// ------------------------------
	// Draw enemies if selected
	// ------------------------------
	if ((m_debugOverlays & OVERLAY_NPC_ENEMIES_BIT))
	{
		AIEnemiesIter_t iter;
		for( AI_EnemyInfo_t *eMemory = GetEnemies()->GetFirst(&iter); eMemory != NULL; eMemory = GetEnemies()->GetNext(&iter) )
		{
			if (eMemory->hEnemy)
			{
				CBaseCombatCharacter *npcEnemy = (eMemory->hEnemy)->MyCombatCharacterPointer();
				if (npcEnemy)
				{
					float	r,g,b;
					char	debugText[255];
					debugText[0] = NULL;

					if (npcEnemy == GetEnemy())
					{
						strcat(debugText,"Current Enemy");
					}
					else if (npcEnemy == GetTarget())
					{
						strcat(debugText,"Current Target");
					}
					else
					{
						strcat(debugText,"Other Memory");
					}
					if (IsUnreachable(npcEnemy))
					{
						strcat(debugText," (Unreachable)");
					}
					if (eMemory->bEludedMe)
					{
						strcat(debugText," (Eluded)");
					}
					// Unreachable enemy drawn in green
					if (IsUnreachable(npcEnemy))
					{
						r = 0;
						g = 255;
						b = 0;
					}
					// Eluded enemy drawn in blue
					else if (eMemory->bEludedMe)
					{
						r = 0;
						g = 0;
						b = 255;
					}
					// Current enemy drawn in red
					else if (npcEnemy == GetEnemy())
					{
						r = 255;
						g = 0;
						b = 0;
					}
					// Current traget drawn in magenta
					else if (npcEnemy == GetTarget())
					{
						r = 255;
						g = 0;
						b = 255;
					}
					// All other enemies drawn in pink
					else
					{
						r = 255;
						g = 100;
						b = 100;
					}


					Vector drawPos = eMemory->vLastKnownLocation;
					NDebugOverlay::Text( drawPos, debugText, false, 0.0 );

					// If has a line on the player draw cross slightly in front so player can see
					if (npcEnemy->IsPlayer() &&
						(eMemory->vLastKnownLocation - npcEnemy->GetAbsOrigin()).Length()<10 )
					{
						Vector vEnemyFacing = npcEnemy->BodyDirection2D( );
						Vector eyePos = npcEnemy->EyePosition() + vEnemyFacing*10.0;
						Vector upVec	= Vector(0,0,2);
						Vector sideVec;
						CrossProduct( vEnemyFacing, upVec, sideVec);
						NDebugOverlay::Line(eyePos+sideVec+upVec, eyePos-sideVec-upVec, r,g,b, false,0);
						NDebugOverlay::Line(eyePos+sideVec-upVec, eyePos-sideVec+upVec, r,g,b, false,0);

						NDebugOverlay::Text( eyePos, debugText, false, 0.0 );
					}
					else
					{
						NDebugOverlay::Cross3D(drawPos,NAI_Hull::Mins(npcEnemy->GetHullType()),NAI_Hull::Maxs(npcEnemy->GetHullType()),r,g,b,false,0);
					}
				}
			}
		}
	}

	// ----------------------------------------------
	// Draw line to target and enemy entity if exist
	// ----------------------------------------------
	if ((m_debugOverlays & OVERLAY_NPC_FOCUS_BIT))
	{
		if (GetEnemy() != NULL)
		{
			NDebugOverlay::Line(EyePosition(),GetEnemy()->EyePosition(),255,0,0,true,0.0);
		}
		if (GetTarget() != NULL)
		{
			NDebugOverlay::Line(EyePosition(),GetTarget()->EyePosition(),0,0,255,true,0.0);
		}
	}


	GetPathfinder()->DrawDebugGeometryOverlays(m_debugOverlays);

	CBaseEntity::DrawDebugGeometryOverlays();
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CAI_BaseNPC::DrawDebugTextOverlays(void)
{
	int text_offset = 0;

	// ---------------------
	// Print Baseclass text
	// ---------------------
	text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_NPC_SQUAD_BIT)
	{
		// Print health
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"Health: %i",m_iHealth);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		// Print squad name
		Q_strncpy(tempstr,"Squad: ",sizeof(tempstr));
		if (m_pSquad)
		{
			strncat(tempstr,m_pSquad->GetName(),sizeof(tempstr));

			if( m_pSquad->GetLeader() == this )
			{
				strncat(tempstr," (LEADER)",sizeof(tempstr));
			}

			strncat(tempstr,"\n",sizeof(tempstr));
		}
		else
		{
			strncat(tempstr," - \n",sizeof(tempstr));
		}
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		// Print enemy name
		Q_strncpy(tempstr,"Enemy: ",sizeof(tempstr));
		if (GetEnemy())
		{
			if (GetEnemy()->GetEntityName() != NULL_STRING)
			{
				strcat(tempstr,STRING(GetEnemy()->GetEntityName()));
				strcat(tempstr,"\n");
			}
			else
			{
				strcat(tempstr,STRING(GetEnemy()->m_iClassname));
				strcat(tempstr,"\n");
			}
		}
		else
		{
			strcat(tempstr," - \n");
		}
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		// Print slot
		Q_snprintf(tempstr,sizeof(tempstr),"Slot:  %s \n",
			SquadSlotName(m_iMySquadSlot));
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

	}

	if (m_debugOverlays & OVERLAY_TEXT_BIT)
	{
		char tempstr[512];
		// --------------
		// Print Health
		// --------------
		Q_snprintf(tempstr,sizeof(tempstr),"Health: %i",m_iHealth);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		// --------------
		// Print State
		// --------------
		static const char *pStateNames[] = { "None", "Idle", "Alert", "Combat", "Scripted", "PlayDead", "Dead" };
		if ( (int)m_NPCState < ARRAYSIZE(pStateNames) )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Stat: %s, ", pStateNames[m_NPCState] );
			NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
			text_offset++;
		}

		// -----------------
		// Print MotionType
		// -----------------
		int navTypeIndex = (int)GetNavType() + 1;
		static const char *pMoveNames[] = { "None", "Ground", "Jump", "Fly", "Climb" };
		Assert( navTypeIndex >= 0 && navTypeIndex < ARRAYSIZE(pMoveNames) );
		if ( navTypeIndex < ARRAYSIZE(pMoveNames) )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Move: %s, ", pMoveNames[navTypeIndex] );
			NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
			text_offset++;
		}

		// --------------
		// Print Schedule
		// --------------
		if ( GetCurSchedule() )
		{
			CAI_BehaviorBase *pBehavior = GetRunningBehavior();
			if ( pBehavior )
			{
				Q_snprintf(tempstr,sizeof(tempstr),"Behv: %s, ", pBehavior->GetName() );
				NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
				text_offset++;
			}

			const char *pName = NULL;
			pName = GetCurSchedule()->GetName();
			if ( !pName )
			{
				pName = "Unknown";
			}
			Q_snprintf(tempstr,sizeof(tempstr),"Schd: %s, ", pName );
			NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
			text_offset++;

			if (m_debugOverlays & OVERLAY_NPC_TASK_BIT)
			{
				for (int i = 0 ; i < GetCurSchedule()->NumTasks(); i++)
				{
					Q_snprintf(tempstr,sizeof(tempstr),"%s%s%s%s",
						((i==0)					? "Task:":"       "),
						((i==GetScheduleCurTaskIndex())	? "->"   :"   "),
						TaskName(GetCurSchedule()->GetTaskList()[i].iTask),
						((i==GetScheduleCurTaskIndex())	? "<-"   :""));

					NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
					text_offset++;
				}
			}
			else
			{
				const Task_t *pTask = GetTask();
				if ( pTask )
				{
					Q_snprintf(tempstr,sizeof(tempstr),"Task: %s (#%d), ", TaskName(pTask->iTask), GetScheduleCurTaskIndex() );
				}
				else
				{
					Q_strncpy(tempstr,"Task: None",sizeof(tempstr));
				}
				NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
				text_offset++;
			}
		}

		// --------------
		// Print Acitivity
		// --------------
		if( m_Activity != ACT_INVALID && m_IdealActivity != ACT_INVALID && m_Activity != ACT_RESET)
		{
			Activity iActivity		= TranslateActivity( m_Activity );

			Activity iIdealActivity	= Weapon_TranslateActivity( m_IdealActivity );
			iIdealActivity			= NPC_TranslateActivity( iIdealActivity );

			const char *pszActivity = GetSequenceActivityName( SelectWeightedSequence(iActivity) );
			const char *pszIdealActivity = GetSequenceActivityName( SelectWeightedSequence(iIdealActivity) );

			Q_snprintf(tempstr,sizeof(tempstr),"Actv: %s (%s)\n", pszActivity, pszIdealActivity );
		}
		else if (m_Activity == ACT_RESET)
		{
			Q_strncpy(tempstr,"Actv: RESET",sizeof(tempstr) );
		}
		else
		{
			Q_strncpy(tempstr,"Actv: INVALID", sizeof(tempstr) );
		}
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		//
		// Print all the current conditions.
		//
		if (m_debugOverlays & OVERLAY_NPC_CONDITIONS_BIT)
		{
			bool bHasConditions = false;
			for (int i = 0; i < MAX_CONDITIONS; i++)
			{
				if (m_Conditions.GetBit(i))
				{
					Q_snprintf(tempstr, sizeof(tempstr), "Cond: %s\n", ConditionName(AI_RemapToGlobal(i)));
					NDebugOverlay::EntityText(entindex(), text_offset, tempstr, 0);
					text_offset++;
					bHasConditions = true;
				}
			}
			if (!bHasConditions)
			{
				Q_snprintf(tempstr,sizeof(tempstr),"(no conditions)",m_iHealth);
				NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
				text_offset++;
			}
		}

		// --------------
		// Print Interrupte
		// --------------
		if (m_interuptSchedule)
		{
			const char *pName = NULL;
			pName = m_interuptSchedule->GetName();
			if ( !pName )
			{
				pName = "Unknown";
			}

			Q_snprintf(tempstr,sizeof(tempstr),"Intr: %s (%s)\n", pName, m_interruptText );
			NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
			text_offset++;
		}

		// --------------
		// Print Failure
		// --------------
		if (m_failedSchedule)
		{
			const char *pName = NULL;
			pName = m_failedSchedule->GetName();
			if ( !pName )
			{
				pName = "Unknown";
			}
			Q_snprintf(tempstr,sizeof(tempstr),"Fail: %s (%s)\n", pName,m_failText );
			NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
			text_offset++;
		}


		// -------------------------------
		// Print any important condtions
		// -------------------------------
		if (HasCondition(COND_ENEMY_TOO_FAR))
		{
			NDebugOverlay::EntityText(entindex(),text_offset,"Enemy too far to attack",0);
			text_offset++;
		}
		if ( GetAbsVelocity() != vec3_origin || GetLocalAngularVelocity() != vec3_angle )
		{
			char tmp[512];
			Q_snprintf( tmp, sizeof(tmp), "Vel %.1f %.1f %.1f   Ang: %.1f %.1f %.1f\n", 
				GetAbsVelocity().x, GetAbsVelocity().y, GetAbsVelocity().z, 
				GetLocalAngularVelocity().x, GetLocalAngularVelocity().y, GetLocalAngularVelocity().z );
			NDebugOverlay::EntityText(entindex(),text_offset,tmp,0);
			text_offset++;
		}
	}
	return text_offset;
}


//-----------------------------------------------------------------------------
// Purpose: Displays information in the console about the state of this npc.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ReportAIState( void )
{
	static const char *pStateNames[] = { "None", "Idle", "Alert", "Combat", "Scripted", "PlayDead", "Dead" };

	Msg( "%s: ", GetClassname() );
	if ( (int)m_NPCState < ARRAYSIZE(pStateNames) )
		Msg( "State: %s, ", pStateNames[m_NPCState] );

	if( m_Activity != ACT_INVALID && m_IdealActivity != ACT_INVALID )
	{
		const char *pszActivity = GetSequenceActivityName( SelectWeightedSequence(m_Activity) );
		const char *pszIdealActivity = GetSequenceActivityName( SelectWeightedSequence(m_IdealActivity) );

		Msg( "Activity: %s  -  Ideal Activity: %s\n", pszActivity, pszIdealActivity );
	}

	if ( GetCurSchedule() )
	{
		const char *pName = NULL;
		pName = GetCurSchedule()->GetName();
		if ( !pName )
			pName = "Unknown";
		Msg( "Schedule %s, ", pName );
		const Task_t *pTask = GetTask();
		if ( pTask )
			Msg( "Task %d (#%d), ", pTask->iTask, GetScheduleCurTaskIndex() );
	}
	else
		Msg( "No Schedule, " );

	if ( GetEnemy() != NULL )
	{
		g_pEffects->Sparks( GetEnemy()->GetAbsOrigin() + Vector( 0, 0, 64 ) );
		Msg( "\nEnemy is %s", GetEnemy()->GetClassname() );
	}
	else
		Msg( "No enemy " );

	if ( IsMoving() )
	{
		Msg( " Moving " );
		if ( m_flMoveWaitFinished > gpGlobals->curtime )
			Msg( ": Stopped for %.2f. ", m_flMoveWaitFinished - gpGlobals->curtime );
		else if ( m_IdealActivity == GetStoppedActivity() )
			Msg( ": In stopped anim. " );
	}

	Msg( "Leader." );

	Msg( "\n" );
	Msg( "Yaw speed:%3.1f,Health: %3d\n", GetMotor()->GetYawSpeed(), m_iHealth );

	if ( GetGroundEntity() )
	{
		Msg( "Groundent:%s\n\n", GetGroundEntity()->GetClassname() );
	}
	else
	{
		Msg( "Groundent: NULL\n\n" );
	}
}

//-----------------------------------------------------------------------------

ConVar ai_report_task_timings_on_limit( "ai_report_task_timings_on_limit", "0", FCVAR_ARCHIVE );
ConVar ai_think_limit_label( "ai_think_limit_label", "0", FCVAR_ARCHIVE );

void CAI_BaseNPC::ReportOverThinkLimit( float time )
{
	Msg( "%s thinking for %.02fms!!! (%s); r%.2f (c%.2f, pst%.2f, ms%.2f), p-r%.2f, m%.2f\n",
		 GetDebugName(), time, GetCurSchedule()->GetName(),
		 g_AIRunTimer.GetDuration().GetMillisecondsF(),
		 g_AIConditionsTimer.GetDuration().GetMillisecondsF(),
		 g_AIPrescheduleThinkTimer.GetDuration().GetMillisecondsF(),
		 g_AIMaintainScheduleTimer.GetDuration().GetMillisecondsF(),
		 g_AIPostRunTimer.GetDuration().GetMillisecondsF(),
		 g_AIMoveTimer.GetDuration().GetMillisecondsF() );

	Vector tmp = GetLocalOrigin();
	tmp.z = GetAbsMaxs().z + 16;

	if (ai_think_limit_label.GetBool()) 
	{
		float max = -1;
		const char *pszMax = "unknown";

		if ( g_AIConditionsTimer.GetDuration().GetMillisecondsF() > max )
		{
			max = g_AIConditionsTimer.GetDuration().GetMillisecondsF();
			pszMax = "Conditions";
		}
		if ( g_AIPrescheduleThinkTimer.GetDuration().GetMillisecondsF() > max )
		{
			max = g_AIPrescheduleThinkTimer.GetDuration().GetMillisecondsF();
			pszMax = "Pre-think";
		}
		if ( g_AIMaintainScheduleTimer.GetDuration().GetMillisecondsF() > max )
		{
			max = g_AIMaintainScheduleTimer.GetDuration().GetMillisecondsF();
			pszMax = "Schedule";
		}
		if ( g_AIPostRunTimer.GetDuration().GetMillisecondsF() > max )
		{
			max = g_AIPostRunTimer.GetDuration().GetMillisecondsF();
			pszMax = "Post-run";
		}
		if ( g_AIMoveTimer.GetDuration().GetMillisecondsF() > max )
		{
			max = g_AIMoveTimer.GetDuration().GetMillisecondsF();
			pszMax = "Move";
		}
		NDebugOverlay::Text( tmp, (char*)(const char *)CFmtStr( "Slow %.1f, %s %.1f ", time, pszMax, max ), false, 1 );
	}

	if ( ai_report_task_timings_on_limit.GetBool() )
		DumpTaskTimings();
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not this npc can play the scripted sequence or AI
//			sequence that is trying to possess it. If DisregardState is set, the npc
//			will be sucked into the script no matter what state it is in. ONLY
//			Scripted AI ents should allow this.
// Input  : fDisregardNPCState -
//			interruptLevel -
//			eMode - If the function returns true, eMode will be one of the following values:
//				CAN_PLAY_NOW
//				CAN_PLAY_ENQUEUED
// Output :
//-----------------------------------------------------------------------------
CanPlaySequence_t CAI_BaseNPC::CanPlaySequence( bool fDisregardNPCState, int interruptLevel )
{
	CanPlaySequence_t eReturn = CAN_PLAY_NOW;

	if ( m_hCine )
	{
		if ( !m_hCine->CanEnqueueAfter() )
		{
			// npc is already running one scripted sequence and has an important script to play next
			return CANNOT_PLAY;
		}

		eReturn = CAN_PLAY_ENQUEUED;
	}

	if ( !IsAlive() )
	{
		// npc is dead!
		return CANNOT_PLAY;
	}

	if ( fDisregardNPCState )
	{
		// ok to go, no matter what the npc state. (scripted AI)
		return eReturn;
	}

	if ( m_NPCState == NPC_STATE_NONE || m_NPCState == NPC_STATE_IDLE || m_IdealNPCState == NPC_STATE_IDLE )
	{
		// ok to go, but only in these states
		return eReturn;
	}

	if ( m_NPCState == NPC_STATE_ALERT && interruptLevel >= SS_INTERRUPT_BY_NAME )
	{
		return eReturn;
	}

	// unknown situation
	return CANNOT_PLAY;
}


//-----------------------------------------------------------------------------

void CAI_BaseNPC::SetHintGroup( string_t newGroup, bool bHintGroupNavLimiting )	
{ 
	string_t oldGroup = m_strHintGroup;
	m_strHintGroup = newGroup;
	m_bHintGroupNavLimiting = bHintGroupNavLimiting;

	if ( oldGroup != newGroup )
		OnChangeHintGroup( oldGroup, newGroup );

}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CAI_BaseNPC::GetShootEnemyDir( const Vector &shootOrigin )
{
	CBaseEntity *pEnemy = GetEnemy();

	if ( pEnemy )
	{
		Vector vecEnemyLKP = GetEnemyLKP();

		Vector retval = (pEnemy->BodyTarget( shootOrigin ) - pEnemy->GetAbsOrigin()) + vecEnemyLKP - shootOrigin;
		VectorNormalize( retval );
		return retval;
	}
	else
	{
		Vector forward;
		AngleVectors( GetLocalAngles(), &forward );
		return forward;
	}
}

//-----------------------------------------------------------------------------
// Simulates many times and reports statistical accuracy. 
//-----------------------------------------------------------------------------
void CAI_BaseNPC::CollectShotStats( const Vector &vecShootOrigin, const Vector &vecShootDir )
{
	int iterations = ai_shot_stats_term.GetInt();
	int iHits = 0;

	CShotManipulator manipulator( vecShootDir );

	for( int i = 0 ; i < iterations ; i++ )
	{
		// Apply appropriate accuracy.
		manipulator.ApplySpread( GetAttackAccuracy( GetActiveWeapon(), GetEnemy() ) );
		Vector shotDir = manipulator.GetResult();

		Vector vecEnd = vecShootOrigin + shotDir * 8192;

		trace_t tr;
		AI_TraceLine( vecShootOrigin, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

		if( tr.m_pEnt && tr.m_pEnt == GetEnemy() )
		{
			iHits++;
		}
	}

	float flHitPercent = ((float)iHits / (float)iterations) * 100.0;
	Msg("Shots:%d   Hits:%d   Percentage:%f\n", iterations, iHits, flHitPercent);
}

#ifdef HL2_DLL
//-----------------------------------------------------------------------------
// Similar to calling GetShootEnemyDir, but returns the exact trajectory to 
// fire the bullet along, after calculating for target speed, location, 
// concealment, etc. (sjb)
//-----------------------------------------------------------------------------
Vector CAI_BaseNPC::GetActualShootTrajectory( const Vector &shootOrigin )
{
	// FIRSTLY, project the target's location into the future.
	Vector vecProjectedPosition;

	if( GetEnemy() )
	{
		Vector vecEnemyLKP = GetEnemyLKP();
		Vector vecTargetPosition = (GetEnemy()->BodyTarget( shootOrigin ) - GetEnemy()->GetAbsOrigin()) + vecEnemyLKP;

		// lead for some fraction of a second.
		vecProjectedPosition = vecTargetPosition + ( GetEnemy()->GetAbsVelocity() * ai_lead_time.GetFloat() );
	}
	else
	{
		return GetShootEnemyDir( shootOrigin );
	}

	Vector shotDir = vecProjectedPosition - shootOrigin;
	VectorNormalize( shotDir );

	if( ai_shot_stats.GetBool() != 0 && GetEnemy()->IsPlayer() )
	{
		CollectShotStats( shootOrigin, shotDir );
	}

	// NOW we have a shoot direction. Where a 100% accurate bullet should go.
	// Modify it by weapon proficiency.
	// construct a manipulator 
	CShotManipulator manipulator( shotDir );

	// Apply appropriate accuracy.
	manipulator.ApplySpread( GetAttackAccuracy( GetActiveWeapon(), GetEnemy() ) );
	shotDir = manipulator.GetResult();

	// Look for an opportunity to make misses hit interesting things.
	CBaseCombatCharacter *pEnemy;

	pEnemy = GetEnemy()->MyCombatCharacterPointer();

	if( pEnemy && pEnemy->ShouldShootMissTarget( this ) )
	{
		Vector vecEnd = shootOrigin + shotDir * 8192;
		trace_t tr;

		AI_TraceLine(shootOrigin, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

		if( tr.fraction != 1.0 && tr.m_pEnt && tr.m_pEnt->m_takedamage != DAMAGE_NO )
		{
			// Hit something we can harm. Just shoot it.
			return manipulator.GetResult();
		}

		// Find something interesting around the enemy to shoot instead of just missing.
		CBaseEntity *pMissTarget = pEnemy->FindMissTarget();
		
		if( pMissTarget )
		{
			shotDir = pMissTarget->WorldSpaceCenter() - shootOrigin;
			VectorNormalize( shotDir );
		}
	}

	return shotDir;
}
#endif HL2_DLL

//-----------------------------------------------------------------------------

Vector CAI_BaseNPC::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{ 
	Vector low = WorldSpaceCenter() - ( WorldSpaceCenter() - GetAbsOrigin() ) * .25;
	Vector high = EyePosition();
	Vector delta = high - low;
	Vector result;
	if ( bNoisy )
	{
		// bell curve
		float rand1 = random->RandomFloat( 0.0, 0.5 );
		float rand2 = random->RandomFloat( 0.0, 0.5 );
		result = low + delta * rand1 + delta * rand2;
	}
	else
		result = low + delta * 0.5; 

	return result;
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::ShouldMoveAndShoot()
{ 
	return ( ( CapabilitiesGet() & bits_CAP_MOVE_SHOOT ) != 0 ); 
}


//=========================================================
// FacingIdeal - tells us if a npc is facing its ideal
// yaw. Created this function because many spots in the
// code were checking the yawdiff against this magic
// number. Nicer to have it in one place if we're gonna
// be stuck with it.
//=========================================================
bool CAI_BaseNPC::FacingIdeal( void )
{
	if ( fabs( GetMotor()->DeltaIdealYaw() ) <= 0.006 )//!!!BUGBUG - no magic numbers!!!
	{
		return true;
	}

	return false;
}

//---------------------------------

void CAI_BaseNPC::AddFacingTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp )
{
	GetMotor()->AddFacingTarget( pTarget, flImportance, flDuration, flRamp );
}

void CAI_BaseNPC::AddFacingTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	GetMotor()->AddFacingTarget( vecPosition, flImportance, flDuration, flRamp );
}

void CAI_BaseNPC::AddFacingTarget( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	GetMotor()->AddFacingTarget( pTarget, vecPosition, flImportance, flDuration, flRamp );
}

float CAI_BaseNPC::GetFacingDirection( Vector &vecDir )
{
	return (GetMotor()->GetFacingDirection( vecDir ));
}


//---------------------------------


int CAI_BaseNPC::PlaySentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, CBaseEntity *pListener )
{
	int sentenceIndex = -1;

	if ( pszSentence && IsAlive() )
	{

		if ( pszSentence[0] == '!' )
		{
			sentenceIndex = SENTENCEG_Lookup( pszSentence );
			CPASAttenuationFilter filter( this, soundlevel );
			enginesound->EmitSentenceByIndex( filter, entindex(), CHAN_VOICE, sentenceIndex, volume, soundlevel, 0, PITCH_NORM );
		}
		else
		{
			sentenceIndex = SENTENCEG_PlayRndSz( edict(), pszSentence, volume, soundlevel, 0, PITCH_NORM );
		}
	}

	return sentenceIndex;
}


int CAI_BaseNPC::PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener )
{
	return PlaySentence( pszSentence, delay, volume, soundlevel, pListener );
}


void CAI_BaseNPC::SentenceStop( void )
{
	EmitSound( "AI_BaseNPC.SentenceStop" );
}




//-----------------------------------------------------------------------------
// Purpose: Play a one-shot scene
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CAI_BaseNPC::PlayScene( const char *pszScene )
{
	return InstancedScriptedScene( this, pszScene );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *CAI_BaseNPC::FindNamedEntity( const char *name )
{
	if ( !stricmp( name, "!player" ))
	{
		return ( CBaseEntity * )UTIL_PlayerByIndex( 1 );
	}
	else if ( !stricmp( name, "!enemy" ) )
	{
		if (GetEnemy() != NULL)
			return GetEnemy();
	}
	else if ( !stricmp( name, "!self" ) || !stricmp( name, "!target1" ) )
	{
		return this;
	}
	else if ( !stricmp( name, "!nearestfriend" ) || !stricmp( name, "!friend" ) )
	{
		// FIXME: look at CBaseEntity *CNPCSimpleTalker :: FindNearestFriend(bool fPlayer)
		// punt for now
		return ( CBaseEntity * )UTIL_PlayerByIndex( 1 );
	}
	else if (!stricmp( name, "self" ))
	{
		static int selfwarningcount = 0;

		// fix the vcd, the reserved names have changed
		if ( ++selfwarningcount < 5 )
		{
			Msg( "ERROR: \"self\" is no longer used, use \"!self\" in vcd instead!\n" );
		}
		return this;
	}
	else if ( !stricmp( name, "Player" ))
	{
		static int playerwarningcount = 0;
		if ( ++playerwarningcount < 5 )
		{
			Msg( "ERROR: \"player\" is no longer used, use \"!player\" in vcd instead!\n" );
		}
		return ( CBaseEntity * )UTIL_PlayerByIndex( 1 );
	}
	else
	{
		CBaseEntity *entity = gEntList.FindEntityByName( NULL, name, NULL );
		if (entity)
			return entity;
	}

	// huh, punt
	return this;
}


void CAI_BaseNPC::CorpseFallThink( void )
{
	if ( GetFlags() & FL_ONGROUND )
	{
		SetThink ( NULL );

		SetSequenceBox( );
		Relink();
	}
	else
		SetNextThink( gpGlobals->curtime + 0.1f );
}

// Call after animation/pose is set up
void CAI_BaseNPC::NPCInitDead( void )
{
	InitBoneControllers();

	RemoveSolidFlags( FSOLID_NOT_SOLID );

	// so he'll fall to ground
	SetMoveType( MOVETYPE_FLYGRAVITY );

	m_flCycle = 0;
	ResetSequenceInfo( );
	m_flPlaybackRate = 0;

	// Copy health
	m_iMaxHealth		= m_iHealth;
	m_lifeState		= LIFE_DEAD;

	UTIL_SetSize(this, vec3_origin, vec3_origin );
	Relink();

	// Setup health counters, etc.
	SetThink( &CAI_BaseNPC::CorpseFallThink );

	SetNextThink( gpGlobals->curtime + 0.5f );
}

//=========================================================
// BBoxIsFlat - check to see if the npc's bounding box
// is lying flat on a surface (traces from all four corners
// are same length.)
//=========================================================
bool CAI_BaseNPC::BBoxFlat ( void )
{
	trace_t	tr;
	Vector		vecPoint;
	float		flXSize, flYSize;
	float		flLength;
	float		flLength2;

	flXSize = WorldAlignSize().x / 2;
	flYSize = WorldAlignSize().y / 2;

	vecPoint.x = GetAbsOrigin().x + flXSize;
	vecPoint.y = GetAbsOrigin().y + flYSize;
	vecPoint.z = GetAbsOrigin().z;

	AI_TraceLine ( vecPoint, vecPoint - Vector ( 0, 0, 100 ), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	flLength = (vecPoint - tr.endpos).Length();

	vecPoint.x = GetAbsOrigin().x - flXSize;
	vecPoint.y = GetAbsOrigin().y - flYSize;

	AI_TraceLine ( vecPoint, vecPoint - Vector ( 0, 0, 100 ), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	flLength2 = (vecPoint - tr.endpos).Length();
	if ( flLength2 > flLength )
	{
		return false;
	}
	flLength = flLength2;

	vecPoint.x = GetAbsOrigin().x - flXSize;
	vecPoint.y = GetAbsOrigin().y + flYSize;
	AI_TraceLine ( vecPoint, vecPoint - Vector ( 0, 0, 100 ), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	flLength2 = (vecPoint - tr.endpos).Length();
	if ( flLength2 > flLength )
	{
		return false;
	}
	flLength = flLength2;

	vecPoint.x = GetAbsOrigin().x + flXSize;
	vecPoint.y = GetAbsOrigin().y - flYSize;
	AI_TraceLine ( vecPoint, vecPoint - Vector ( 0, 0, 100 ), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	flLength2 = (vecPoint - tr.endpos).Length();
	if ( flLength2 > flLength )
	{
		return false;
	}
	flLength = flLength2;

	return true;
}


void CAI_BaseNPC::SetEnemy( CBaseEntity *pEnemy )
{
	if (m_hEnemy != pEnemy)
	{
		ClearAttackConditions( );
		VacateStrategySlot();
		m_GiveUpOnDeadEnemyTimer.Stop();
	}

	// Assert( (pEnemy == NULL) || (m_NPCState == NPC_STATE_COMBAT) );

	m_hEnemy = pEnemy;
}

const Vector &CAI_BaseNPC::GetEnemyLKP() const
{
	return (const_cast<CAI_BaseNPC *>(this))->GetEnemies()->LastKnownPosition( GetEnemy() );
}

float CAI_BaseNPC::GetEnemyLastTimeSeen() const
{
	return (const_cast<CAI_BaseNPC *>(this))->GetEnemies()->LastTimeSeen( GetEnemy() );
}

void CAI_BaseNPC::MarkEnemyAsEluded()
{
	GetEnemies()->MarkAsEluded( GetEnemy() );
}

void CAI_BaseNPC::ClearEnemyMemory()
{
	GetEnemies()->ClearMemory( GetEnemy() );
}

bool CAI_BaseNPC::EnemyHasEludedMe() const
{
	return (const_cast<CAI_BaseNPC *>(this))->GetEnemies()->HasEludedMe( GetEnemy() );
}

void CAI_BaseNPC::SetTarget( CBaseEntity *pTarget )
{
	m_hTargetEnt = pTarget;
}


//=========================================================
// Choose Enemy - tries to find the best suitable enemy for the npc.
//=========================================================

bool CAI_BaseNPC::ShouldChooseNewEnemy()
{
	CBaseEntity *pEnemy = GetEnemy();
	if ( pEnemy )
	{
		if ( EnemyHasEludedMe() || (IRelationType( pEnemy ) != D_HT && IRelationType( pEnemy ) != D_FR) || !IsValidEnemy( pEnemy ) )
			return true;
		if ( HasCondition(COND_SEE_HATE) || HasCondition(COND_SEE_DISLIKE) || HasCondition(COND_SEE_NEMESIS) )
			return true;
		if ( !pEnemy->IsAlive() )
		{
			if ( m_GiveUpOnDeadEnemyTimer.IsRunning() )
			{
				if ( m_GiveUpOnDeadEnemyTimer.Expired() )
					return true;
			}
			else
				m_GiveUpOnDeadEnemyTimer.Start();
		}
		if( GetEnemies()->HasMemory( pEnemy ) )
			return false;
	}

	return true;
}

//-------------------------------------

bool CAI_BaseNPC::ChooseEnemy ( void )
{
	AI_PROFILE_SCOPE(CAI_Enemies_ChooseEnemy);

	//---------------------------------
	//
	// Gather initial conditions
	//

	CBaseEntity *pInitialEnemy = GetEnemy();
	CBaseEntity *pChosenEnemy  = pInitialEnemy;

	// Use memory bits in case enemy pointer altered outside this function, (e.g., ehandle goes NULL)
	bool fHadEnemy  	 = ( HasMemory( bits_MEMORY_HAD_ENEMY | bits_MEMORY_HAD_PLAYER ) );
	bool fEnemyWasPlayer = HasMemory( bits_MEMORY_HAD_PLAYER );
	bool fEnemyWentNull  = ( fHadEnemy && !pInitialEnemy );
	bool fEnemyEluded	 = ( fEnemyWentNull || ( pInitialEnemy && GetEnemies()->HasEludedMe( pInitialEnemy ) ) );

	//---------------------------------
	//
	// Establish suitability of choosing a new enemy
	//

	bool fHaveCondNewEnemy;
	bool fHaveCondLostEnemy;

	if ( GetCurSchedule() )
	{
		Assert( InterruptFromCondition( COND_NEW_ENEMY ) == COND_NEW_ENEMY && InterruptFromCondition( COND_LOST_ENEMY ) == COND_LOST_ENEMY );
		fHaveCondNewEnemy  = GetCurSchedule()->HasInterrupt( COND_NEW_ENEMY );
		fHaveCondLostEnemy = GetCurSchedule()->HasInterrupt( COND_LOST_ENEMY );
	}
	else
	{
		fHaveCondNewEnemy  = true; // not having a schedule is the same as being interruptable by any condition
		fHaveCondLostEnemy = true;
	}

	if ( !fEnemyWentNull )
	{
		if ( !fHaveCondNewEnemy && !( fHaveCondLostEnemy && fEnemyEluded ) )
		{
			// DO NOT mess with the npc's enemy pointer unless the schedule the npc is currently
			// running will be interrupted by COND_NEW_ENEMY or COND_LOST_ENEMY. This will
			// eliminate the problem of npcs getting a new enemy while they are in a schedule
			// that doesn't care, and then not realizing it by the time they get to a schedule
			// that does. I don't feel this is a good permanent fix.
			return ( pChosenEnemy != NULL );
		}
	}
	else if ( !fHaveCondNewEnemy && !fHaveCondLostEnemy )
	{
		DevMsg( 2, "WARNING: AI enemy went NULL but schedule is not interested\n" );
	}

	//---------------------------------
	//
	// Select a target
	//

	if ( ShouldChooseNewEnemy()	)
	{
		pChosenEnemy = BestEnemy();
	}

	//---------------------------------
	//
	// React to result of selection
	//

	bool fChangingEnemy = ( pChosenEnemy != pInitialEnemy );

	if ( fChangingEnemy || fEnemyWentNull )
	{
		Forget( bits_MEMORY_HAD_ENEMY | bits_MEMORY_HAD_PLAYER );

		// Did our old enemy snuff it?
		if ( pInitialEnemy && !pInitialEnemy->IsAlive() )
		{
			SetCondition( COND_ENEMY_DEAD );
		}

		SetEnemy( pChosenEnemy );
		SetCondition(COND_NEW_ENEMY);

		if ( fHadEnemy )
		{
			// Vacate any strategy slot on old enemy
			VacateStrategySlot();

			// Force output event for establishing LOS
			Forget( bits_MEMORY_HAD_LOS );
			// m_flLastAttackTime	= 0;
		}

		if ( !pChosenEnemy )
		{
			if ( fEnemyEluded )
			{
				SetCondition( COND_LOST_ENEMY );
				LostEnemySound();
			}

			if ( fEnemyWasPlayer )
			{
				m_OnLostPlayer.FireOutput(this, this);
			}
			else
			{
				m_OnLostEnemy.FireOutput(this, this);
			}
		}
		else
		{
			Remember( ( pChosenEnemy->IsPlayer() ) ? bits_MEMORY_HAD_PLAYER : bits_MEMORY_HAD_ENEMY );
		}
	}

	//---------------------------------

	return ( pChosenEnemy != NULL );
}


//=========================================================
// DropItem - dead npc drops named item
//=========================================================
CBaseEntity *CAI_BaseNPC::DropItem ( char *pszItemName, Vector vecPos, QAngle vecAng )
{
	if ( !pszItemName )
	{
		Msg( "DropItem() - No item name!\n" );
		return NULL;
	}

	CBaseEntity *pItem = CBaseEntity::Create( pszItemName, vecPos, vecAng, this );

	if ( pItem )
	{
		// do we want this behavior to be default?! (sjb)
		pItem->SetAbsVelocity( GetAbsVelocity() );
		pItem->SetLocalAngularVelocity( QAngle ( 0, random->RandomFloat( 0, 100 ), 0 ) );
		return pItem;
	}
	else
	{
		Msg( "DropItem() - Didn't create!\n" );
		return false;
	}

}


bool CAI_BaseNPC::ShouldFadeOnDeath( void )
{
	// if flagged to fade out
	return HasSpawnFlags(SF_NPC_FADE_CORPSE);
}

//-----------------------------------------------------------------------------
// Purpose: Indicates whether or not this npc should play an idle sound now.
//
//
// Output : Returns true if yes, false if no.
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ShouldPlayIdleSound( void )
{
	if ( ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT ) &&
		   random->RandomInt(0,99) == 0 && !HasSpawnFlags(SF_NPC_GAG) )
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
bool CAI_BaseNPC::FOkToMakeSound( void )
{
	// Am I making uninterruptable sound
	if (gpGlobals->curtime <= m_flSoundWaitTime)
	{
		return false;
	}

	// Check other's in squad to see if they making uninterrupable sounds
	if (m_pSquad)
	{
		if (gpGlobals->curtime <= m_pSquad->GetSquadSoundWaitTime())
		{
			return false;
		}
	}


	if ( HasSpawnFlags(SF_NPC_GAG) )
	{
		if ( m_NPCState != NPC_STATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::JustMadeSound( void )
{
	m_flSoundWaitTime = gpGlobals->curtime + random->RandomFloat(1.5, 2.0);

	if (m_pSquad)
	{
		m_pSquad->SetSquadSoundWaitTime( gpGlobals->curtime + random->RandomFloat(1.5, 2.0) );
	}
}

Activity CAI_BaseNPC::GetStoppedActivity( void )
{
	if (GetNavigator()->IsGoalActive())
	{
		Activity activity = GetNavigator()->GetArrivalActivity();

		if (activity != ACT_INVALID)
		{
			return activity;
		}
	}

	return ACT_IDLE;
}


//=========================================================
//=========================================================
void CAI_BaseNPC::OnScheduleChange ( void )
{
	EndTaskOverlay();

	m_pNavigator->OnScheduleChange();

	m_flMoveWaitFinished = 0;

	VacateStrategySlot();
	// If I still have have a route, clear it
	GetNavigator()->ClearGoal();

	// If I locked a hint node clear it
	if ( HasMemory(bits_MEMORY_LOCKED_HINT)	&& m_pHintNode != NULL)
	{
		float hintDelay = GetHintDelay(m_pHintNode->HintType());
		m_pHintNode->Unlock(hintDelay);
		m_pHintNode = NULL;
	}
}



CBaseCombatCharacter* CAI_BaseNPC::GetEnemyCombatCharacterPointer()
{
	if ( GetEnemy() == NULL )
		return NULL;

	return GetEnemy()->MyCombatCharacterPointer();
}


// Global Savedata for npc
// UNDONE: Save schedule data?  Can this be done?  We may
// lose our enemy pointer or other data (goal ent, target, etc)
// that make the current schedule invalid, perhaps it's best
// to just pick a new one when we start up again.
//
// This should be an exact copy of the var's in the header.  Fields
// that aren't save/restored are commented out

BEGIN_DATADESC( CAI_BaseNPC )

	//								m_pSchedule  (reacquired on restore)
	DEFINE_EMBEDDED( CAI_BaseNPC,	m_ScheduleState ),
	DEFINE_FIELD( CAI_BaseNPC,	m_failSchedule,				FIELD_INTEGER ),
	//								m_bDoPostRestoreRefindPath (not saved)
	DEFINE_BITSTRING(CAI_BaseNPC, m_Conditions),
	DEFINE_BITSTRING(CAI_BaseNPC, m_CustomInterruptConditions),
	DEFINE_BITSTRING(CAI_BaseNPC, m_InverseIgnoreConditions),
	DEFINE_FIELD( CAI_BaseNPC,	m_bConditionsGathered,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_BaseNPC,	m_NPCState,					FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_IdealNPCState,			FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_flLastStateChangeTime,	FIELD_TIME ),
	DEFINE_FIELD( CAI_BaseNPC,	m_Efficiency,				FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_Activity,					FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_translatedActivity,		FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_IdealActivity,			FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_nIdealSequence,			FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_IdealTranslatedActivity,	FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_IdealWeaponActivity,		FIELD_INTEGER ),
	DEFINE_EMBEDDEDBYREF( CAI_BaseNPC,	m_pSenses ),
  	DEFINE_FIELD( CAI_BaseNPC,	m_hEnemy,					FIELD_EHANDLE ),
	DEFINE_FIELD( CAI_BaseNPC,	m_hTargetEnt,				FIELD_EHANDLE ),
	DEFINE_EMBEDDED( CAI_BaseNPC,	m_GiveUpOnDeadEnemyTimer ),
	DEFINE_FIELD( CAI_BaseNPC,	m_flAcceptableTimeSeenEnemy,	FIELD_TIME ),
	DEFINE_FIELD( CAI_BaseNPC,	m_bSelectedByPlayer,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_BaseNPC,	m_vecCommandGoal,			FIELD_VECTOR ),
	DEFINE_FIELD( CAI_BaseNPC,	m_hCommandTarget,			FIELD_EHANDLE ),
	DEFINE_FIELD( CAI_BaseNPC,	m_flSoundWaitTime,			FIELD_TIME ),
	DEFINE_FIELD( CAI_BaseNPC,	m_afCapability,				FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_flMoveWaitFinished,		FIELD_TIME ),
	DEFINE_FIELD( CAI_BaseNPC,	m_hOpeningDoor,				FIELD_EHANDLE ),
	DEFINE_EMBEDDEDBYREF( CAI_BaseNPC,	m_pNavigator ),
	DEFINE_EMBEDDEDBYREF( CAI_BaseNPC,	m_pLocalNavigator ),
	DEFINE_EMBEDDEDBYREF( CAI_BaseNPC,	m_pPathfinder ),
	DEFINE_EMBEDDEDBYREF( CAI_BaseNPC,	m_pMoveProbe ),
	DEFINE_EMBEDDEDBYREF( CAI_BaseNPC,	m_pMotor ),
	DEFINE_UTLVECTOR(CAI_BaseNPC, m_UnreachableEnts,		FIELD_EMBEDDED),
	DEFINE_FIELD( CAI_BaseNPC,	m_vDefaultEyeOffset,		FIELD_VECTOR ),
  	DEFINE_FIELD( CAI_BaseNPC,	m_flNextEyeLookTime,		FIELD_TIME ),
    DEFINE_FIELD( CAI_BaseNPC,	m_flEyeIntegRate,			FIELD_FLOAT ),
    DEFINE_FIELD( CAI_BaseNPC,	m_vEyeLookTarget,			FIELD_POSITION_VECTOR ),
    DEFINE_FIELD( CAI_BaseNPC,	m_vCurEyeTarget,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CAI_BaseNPC,	m_hEyeLookTarget,			FIELD_EHANDLE ),
    DEFINE_FIELD( CAI_BaseNPC,	m_flHeadYaw,				FIELD_FLOAT ),
    DEFINE_FIELD( CAI_BaseNPC,	m_flHeadPitch,				FIELD_FLOAT ),
    DEFINE_FIELD( CAI_BaseNPC,	m_scriptState,				FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_hCine,					FIELD_EHANDLE ),
	DEFINE_FIELD( CAI_BaseNPC,	m_ScriptArrivalActivity,	FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_strScriptArrivalSequence,	FIELD_STRING ),
	// TODO						m_pScene,				
	// 							m_pEnemies					Saved specially in ai_saverestore.cpp
	DEFINE_FIELD( CAI_BaseNPC,	m_afMemory,					FIELD_INTEGER ),
  	DEFINE_FIELD( CAI_BaseNPC,	m_hEnemyOccluder,			FIELD_EHANDLE ),
  	DEFINE_FIELD( CAI_BaseNPC,	m_flSumDamage,				FIELD_FLOAT ),
  	DEFINE_FIELD( CAI_BaseNPC,	m_flLastDamageTime,			FIELD_TIME ),
  	DEFINE_FIELD( CAI_BaseNPC,	m_flLastAttackTime,			FIELD_TIME ),
  	DEFINE_FIELD( CAI_BaseNPC,	m_flNextWeaponSearchTime,	FIELD_TIME ),
	// 							m_pSquad					Saved specially in ai_saverestore.cpp
	DEFINE_KEYFIELD(CAI_BaseNPC,m_SquadName,				FIELD_STRING, "squadname" ),
    DEFINE_FIELD( CAI_BaseNPC,	m_iMySquadSlot,				FIELD_INTEGER ),
	DEFINE_KEYFIELD( CAI_BaseNPC,m_strHintGroup,			FIELD_STRING, "hintgroup" ),
	DEFINE_KEYFIELD( CAI_BaseNPC,m_bHintGroupNavLimiting,	FIELD_BOOLEAN, "hintlimiting" ),
 	DEFINE_EMBEDDEDBYREF( CAI_BaseNPC,	m_pTacticalServices ),
 	DEFINE_FIELD( CAI_BaseNPC,	m_flWaitFinished,			FIELD_TIME ),
	DEFINE_EMBEDDED( CAI_BaseNPC,	m_MoveAndShootOverlay ),
	DEFINE_FIELD( CAI_BaseNPC,	m_vecLastPosition,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CAI_BaseNPC,	m_vSavePosition,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CAI_BaseNPC,	m_pHintNode,				FIELD_CLASSPTR),
	DEFINE_FIELD( CAI_BaseNPC,	m_cAmmoLoaded,				FIELD_INTEGER ),
    DEFINE_FIELD( CAI_BaseNPC,	m_flDistTooFar,				FIELD_FLOAT ),
	DEFINE_FIELD( CAI_BaseNPC,	m_pGoalEnt,					FIELD_CLASSPTR ),
	DEFINE_KEYFIELD(CAI_BaseNPC,m_spawnEquipment,			FIELD_STRING, "additionalequipment" ),
  	DEFINE_FIELD( CAI_BaseNPC,	m_fNoDamageDecal,			FIELD_BOOLEAN ),
  	DEFINE_FIELD( CAI_BaseNPC,	m_hStoredPathTarget,			FIELD_EHANDLE ),
	DEFINE_FIELD( CAI_BaseNPC,	m_vecStoredPathGoal,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CAI_BaseNPC,	m_nStoredPathType,			FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_fStoredPathFlags,			FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseNPC,	m_bDidDeathCleanup,			FIELD_BOOLEAN ),
	//							m_fIsUsingSmallHull			TODO -- This needs more consideration than simple save/load
	// 							m_failText					DEBUG
	// 							m_interruptText				DEBUG
	// 							m_failedSchedule			DEBUG
	// 							m_interuptSchedule			DEBUG
	// 							m_nDebugCurIndex			DEBUG

	// Outputs
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnDamaged,				"OnDamaged" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnDeath,					"OnDeath" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnHalfHealth,				"OnHalfHealth" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnFoundEnemy,				"OnFoundEnemy" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnLostEnemyLOS,			"OnLostEnemyLOS" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnLostEnemy,				"OnLostEnemy" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnFoundPlayer,			"OnFoundPlayer" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnLostPlayerLOS,			"OnLostPlayerLOS" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnLostPlayer,				"OnLostPlayer" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnHearWorld,				"OnHearWorld" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnHearPlayer,				"OnHearPlayer" ),
	DEFINE_OUTPUT( CAI_BaseNPC, m_OnHearCombat,				"OnHearCombat" ),

	// Inputs
	DEFINE_INPUTFUNC( CAI_BaseNPC, FIELD_STRING, "SetRelationship", InputSetRelationship ),
	DEFINE_INPUTFUNC( CAI_BaseNPC, FIELD_INTEGER, "SetHealth", InputSetHealth ),

	// Function pointers
	DEFINE_USEFUNC( CAI_BaseNPC, NPCUse ),
	DEFINE_THINKFUNC( CAI_BaseNPC, CallNPCThink ),
	DEFINE_THINKFUNC( CAI_BaseNPC, CorpseFallThink ),
	DEFINE_THINKFUNC( CAI_BaseNPC, NPCInitThink ),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( AIScheduleState_t )
	DEFINE_FIELD( AIScheduleState_t,	iCurTask,				FIELD_INTEGER ),
	DEFINE_FIELD( AIScheduleState_t,	fTaskStatus,			FIELD_INTEGER ),
	DEFINE_FIELD( AIScheduleState_t,	timeStarted,			FIELD_TIME ),
	DEFINE_FIELD( AIScheduleState_t,	timeCurTaskStarted,		FIELD_TIME ),
	DEFINE_FIELD( AIScheduleState_t,	taskFailureCode,		FIELD_INTEGER ),
END_DATADESC()


IMPLEMENT_SERVERCLASS_ST( CAI_BaseNPC, DT_AI_BaseNPC )
	SendPropInt( SENDINFO( m_lifeState ), 3, SPROP_UNSIGNED ),
END_SEND_TABLE()

//-------------------------------------

BEGIN_SIMPLE_DATADESC( UnreachableEnt_t )

	DEFINE_FIELD( UnreachableEnt_t,		hUnreachableEnt,			FIELD_EHANDLE	),
	DEFINE_FIELD( UnreachableEnt_t,		fExpireTime,				FIELD_TIME		),
	DEFINE_FIELD( UnreachableEnt_t,		vLocationWhenUnreachable,	FIELD_POSITION_VECTOR	),

END_DATADESC()

//-------------------------------------

void CAI_BaseNPC::PostConstructor( const char *szClassname )
{
	BaseClass::PostConstructor( szClassname );
	CreateComponents();
}

void CAI_BaseNPC::Precache( void )
{
	if ( m_spawnEquipment != NULL_STRING && strcmp(STRING(m_spawnEquipment), "0") )
	{
		UTIL_PrecacheOther( STRING(m_spawnEquipment) );
	}

	// Make sure schedules are loaded for this NPC type
	if (!LoadedSchedules())
	{
		Msg("ERROR: Rejecting spawn of %s as error in NPC's schedules.\n",GetDebugName());
		UTIL_Remove(this);
		return;
	}

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------

const short AI_EXTENDED_SAVE_HEADER_VERSION = 1;

struct AIExtendedSaveHeader_t
{
	AIExtendedSaveHeader_t()
	 : version(AI_EXTENDED_SAVE_HEADER_VERSION)
	{
	}

	short version;
	unsigned flags;
	char szSchedule[128];
	CRC32_t scheduleCrc;
	
	DECLARE_SIMPLE_DATADESC();
};

enum AIExtendedSaveHeaderFlags_t
{
	AIESH_HAD_ENEMY		= 0x01,
	AIESH_HAD_TARGET	= 0x02,
	AIESH_HAD_NAVGOAL	= 0x04,
};

//-------------------------------------

BEGIN_SIMPLE_DATADESC( AIExtendedSaveHeader_t )
	DEFINE_FIELD( 		AIExtendedSaveHeader_t,	version,		FIELD_SHORT ),
	DEFINE_FIELD( 		AIExtendedSaveHeader_t,	flags,			FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY(	AIExtendedSaveHeader_t,	szSchedule,		FIELD_CHARACTER ),
	DEFINE_FIELD( 		AIExtendedSaveHeader_t,	scheduleCrc,	FIELD_INTEGER ),
END_DATADESC()

//-------------------------------------

int CAI_BaseNPC::Save( ISave &save )
{
	AIExtendedSaveHeader_t saveHeader;
	
	if ( GetEnemy() )
		saveHeader.flags |= AIESH_HAD_ENEMY;
	if ( GetTarget() )
		saveHeader.flags |= AIESH_HAD_TARGET;
	if ( GetNavigator()->IsGoalActive() )
		saveHeader.flags |= AIESH_HAD_NAVGOAL;
	
	if ( m_pSchedule )
	{
		const char *pszSchedule = m_pSchedule->GetName();

		Assert( strlen( pszSchedule ) < sizeof( saveHeader.szSchedule ) - 1 );
		Q_strncpy( saveHeader.szSchedule, pszSchedule, sizeof( saveHeader.szSchedule ) - 1 );
		saveHeader.szSchedule[sizeof(saveHeader.szSchedule) - 1] = 0;

		CRC32_Init( &saveHeader.scheduleCrc );
		CRC32_ProcessBuffer( &saveHeader.scheduleCrc, (void *)m_pSchedule->GetTaskList(), m_pSchedule->NumTasks() * sizeof(Task_t) );
		CRC32_Final( &saveHeader.scheduleCrc );
	}
	else
	{
		saveHeader.szSchedule[0] = 0;
		saveHeader.scheduleCrc = 0;
	}

	save.WriteAll( &saveHeader );
	return BaseClass::Save(save);
}

//-------------------------------------

void CAI_BaseNPC::DiscardScheduleState()
{
	// We don't save/restore routes yet
	GetNavigator()->ClearGoal();

	// We don't save/restore schedules yet
	ClearSchedule();

	// Reset animation
	m_Activity = ACT_RESET;

	// If we don't have an enemy, clear conditions like see enemy, etc.
	if ( GetEnemy() == NULL )
	{
		m_Conditions.ClearAllBits();
	}

	// went across a transition and lost my m_hCine
	bool bLostScript = ( m_NPCState == NPC_STATE_SCRIPT && m_hCine == NULL );
	if ( bLostScript )
	{
		// UNDONE: Do something better here?
		// for now, just go back to idle and let the AI figure out what to do.
		SetState( NPC_STATE_IDLE );
		m_IdealNPCState = NPC_STATE_IDLE;
		DevMsg(1, "Scripted Sequence stripped on level transition for %s\n", GetDebugName() );
	}
}

//-------------------------------------

void CAI_BaseNPC::OnRestore()
{
	if ( m_bDoPostRestoreRefindPath )
	{
		if ( !GetNavigator()->RefindPathToGoal( false ) )
			DiscardScheduleState();
	}
	else
		GetNavigator()->ClearGoal();
	BaseClass::OnRestore();
}

//-------------------------------------

int CAI_BaseNPC::Restore( IRestore &restore )
{
	AIExtendedSaveHeader_t saveHeader;
	restore.ReadAll( &saveHeader );
	
	// do a normal restore
	int status = BaseClass::Restore(restore);
	if ( !status )
		return 0;

	bool bLostScript = ( m_NPCState == NPC_STATE_SCRIPT && m_hCine == NULL );
	bool bDiscardScheduleState = ( bLostScript || 
								   saveHeader.szSchedule[0] == 0 ||
								   saveHeader.version != AI_EXTENDED_SAVE_HEADER_VERSION ||
								   ( (saveHeader.flags & AIESH_HAD_ENEMY) && !GetEnemy() ) ||
								   ( (saveHeader.flags & AIESH_HAD_TARGET) && !GetTarget() ) );

	if ( m_ScheduleState.taskFailureCode >= NUM_FAIL_CODES )
		m_ScheduleState.taskFailureCode = FAIL_NO_TARGET; // must have been a string, gotta punt

	if ( !bDiscardScheduleState )
	{
		m_pSchedule = g_AI_SchedulesManager.GetScheduleByName( saveHeader.szSchedule );
		if ( m_pSchedule )
		{
			CRC32_t scheduleCrc;
			CRC32_Init( &scheduleCrc );
			CRC32_ProcessBuffer( &scheduleCrc, (void *)m_pSchedule->GetTaskList(), m_pSchedule->NumTasks() * sizeof(Task_t) );
			CRC32_Final( &scheduleCrc );

			if ( scheduleCrc != saveHeader.scheduleCrc )
			{
				m_pSchedule = NULL;
			}
		}
	}

	if ( !m_pSchedule )
		bDiscardScheduleState = true;

	if ( !bDiscardScheduleState )
		m_bDoPostRestoreRefindPath = ( ( saveHeader.flags & AIESH_HAD_NAVGOAL) != 0 );
	else 
	{
		m_bDoPostRestoreRefindPath = false;
		DiscardScheduleState();
	}

	return status;
}


//-----------------------------------------------------------------------------
// Purpose: Debug function to make this NPC freeze in place (or unfreeze).
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ToggleFreeze(void) 
{
	if (!IsCurSchedule(SCHED_NPC_FREEZE))
	{
		// Freeze them.
		SetCondition(COND_NPC_FREEZE);
		SetMoveType(MOVETYPE_NONE);
		SetGravity(0);
		SetLocalAngularVelocity(vec3_angle);
		SetAbsVelocity( vec3_origin );
	}
	else
	{
		// Unfreeze them.
		SetCondition(COND_NPC_UNFREEZE);
		m_Activity = ACT_RESET;

		// BUGBUG: this might not be the correct movetype!
		SetMoveType( MOVETYPE_STEP );

		// Doesn't restore gravity to the original value, but who cares?
		SetGravity(1);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Written by subclasses macro to load schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::LoadSchedules(void)
{
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::LoadedSchedules(void)
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_BaseNPC::CAI_BaseNPC(void)
 :	m_UnreachableEnts( 0, 4 )
{
	m_pMotor = NULL;
	m_pMoveProbe = NULL;
	m_pNavigator = NULL;
	m_pSenses = NULL;
	m_pPathfinder = NULL;
	m_pLocalNavigator = NULL;

#ifdef _DEBUG
	// necessary since in debug, we initialize vectors to NAN for debugging
	m_vecLastPosition.Init();
	m_vSavePosition.Init();
	m_vEyeLookTarget.Init();
	m_vCurEyeTarget.Init();
	m_vDefaultEyeOffset.Init();

#endif
	m_bDidDeathCleanup = false;

	m_afCapability				= 0;		// Make sure this is cleared in the base class

	SetHullType(HULL_HUMAN);  // Give human hull by default, subclasses should override

	m_iMySquadSlot				= SQUAD_SLOT_NONE;
	m_flSumDamage				= 0;
	m_flLastDamageTime			= 0;
	m_flLastAttackTime			= 0;
	m_flSoundWaitTime			= 0;
	m_flNextEyeLookTime			= 0;
	m_flHeadYaw					= 0;
	m_flHeadPitch				= 0;
	m_spawnEquipment			= NULL_STRING;
	m_pEnemies					= new CAI_Enemies;
	m_flEyeIntegRate			= 0.95;
	SetTarget( NULL );

	m_pSquad					= NULL;

	m_flMoveWaitFinished		= 0;

	m_fIsUsingSmallHull			= true;

	m_bHintGroupNavLimiting		= false;

	m_fNoDamageDecal			= false;

	// ----------------------------
	//  Debugging fields
	// ----------------------------
	m_interruptText				= NULL;
	m_failText					= NULL;
	m_failedSchedule			= NULL;
	m_interuptSchedule			= NULL;
	m_nDebugPauseIndex			= 0;

	// Player command
	PlayerSelect( false );

	g_AI_Manager.AddAI( this );
	SetCollisionGroup( COLLISION_GROUP_NPC );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_BaseNPC::~CAI_BaseNPC(void)
{
	g_AI_Manager.RemoveAI( this );

	RemoveMemory();

	delete m_pPathfinder;
	delete m_pNavigator;
	delete m_pMotor;
	delete m_pLocalNavigator;
	delete m_pMoveProbe;
	delete m_pSenses;
	delete m_pTacticalServices;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UpdateOnRemove(void)
{
	if ( !m_bDidDeathCleanup && m_NPCState == NPC_STATE_DEAD )
		Msg( "May not have cleaned up on NPC death\n");

	if (m_pSquad)
	{
		// Remove from squad if not already removed
		if (m_pSquad->SquadIsMember(this))
		{
			m_pSquad->RemoveFromSquad(this);
		}
	}

	if ( m_pHintNode )
	{
		m_pHintNode->Unlock();
		m_pHintNode = NULL;
	}

	// Virtual call to shut down any looping sounds.
	StopLoopingSounds();

	// Chain at end to mimic destructor unwind order
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::CreateComponents()
{
	m_pSenses = CreateSenses();
	if ( !m_pSenses )
		return false;

	m_pMotor = CreateMotor();
	if ( !m_pMotor )
		return false;

	m_pLocalNavigator = CreateLocalNavigator();
	if ( !m_pLocalNavigator )
		return false;

	m_pMoveProbe = CreateMoveProbe();
	if ( !m_pMoveProbe )
		return false;

	m_pNavigator = CreateNavigator();
	if ( !m_pNavigator )
		return false;

	m_pPathfinder = CreatePathfinder();
	if ( !m_pPathfinder )
		return false;
		
	m_pTacticalServices = CreateTacticalServices();
	if ( !m_pTacticalServices )
		return false;
		
	m_MoveAndShootOverlay.SetOuter( this );

	m_pMotor->Init( m_pLocalNavigator );
	m_pLocalNavigator->Init( m_pNavigator );
	m_pNavigator->Init( g_pBigAINet );
	m_pPathfinder->Init( g_pBigAINet );
	m_pTacticalServices->Init( g_pBigAINet );
	
	return true;
}

//-----------------------------------------------------------------------------

CAI_Senses *CAI_BaseNPC::CreateSenses()
{
	CAI_Senses *pSenses = new CAI_Senses;
	pSenses->SetOuter( this );
	return pSenses;
}

//-----------------------------------------------------------------------------

CAI_Motor *CAI_BaseNPC::CreateMotor()
{
	return new CAI_Motor( this );
}

//-----------------------------------------------------------------------------

CAI_MoveProbe *CAI_BaseNPC::CreateMoveProbe()
{
	return new CAI_MoveProbe( this );
}

//-----------------------------------------------------------------------------

CAI_LocalNavigator *CAI_BaseNPC::CreateLocalNavigator()
{
	return new CAI_LocalNavigator( this );
}

//-----------------------------------------------------------------------------

CAI_TacticalServices *CAI_BaseNPC::CreateTacticalServices()
{
	return new CAI_TacticalServices( this );
}

//-----------------------------------------------------------------------------

CAI_Navigator *CAI_BaseNPC::CreateNavigator()
{
	return new CAI_Navigator( this );
}

//-----------------------------------------------------------------------------

CAI_Pathfinder *CAI_BaseNPC::CreatePathfinder()
{
	return new CAI_Pathfinder( this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InputSetRelationship( inputdata_t &inputdata )
{
	AddRelationship( inputdata.value.String(), inputdata.pActivator );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InputSetHealth( inputdata_t &inputdata )
{
	int iNewHealth = inputdata.value.Int();
	int iDelta = abs(GetHealth() - iNewHealth);
	if ( iNewHealth > GetHealth() )
	{
		TakeHealth( iDelta, DMG_GENERIC );
	}
	else if ( iNewHealth < GetHealth() )
	{
		TakeDamage( CTakeDamageInfo( this, this, iDelta, DMG_GENERIC ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when exiting a scripted sequence.
// Output : Returns true if alive, false if dead.
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ExitScriptedSequence( )
{
	if ( m_lifeState == LIFE_DYING )
	{
		// is this legal?
		// BUGBUG -- This doesn't call Killed()
		m_IdealNPCState = NPC_STATE_DEAD;
		return false;
	}

	if (m_hCine)
	{
		m_hCine->CancelScript( );
	}

	return true;
}


bool CAI_BaseNPC::CineCleanup()
{
	CAI_ScriptedSequence *pOldCine = m_hCine;

	// am I linked to a cinematic?
	if (m_hCine)
	{
		// okay, reset me to what it thought I was before
		m_hCine->SetTarget( NULL );
		SetMoveType( m_hCine->m_saved_movetype, m_hCine->m_saved_movecollide );
		SetSolidFlags( m_hCine->m_saved_solidflags );
		m_fEffects = m_hCine->m_saved_effects;
	}
	else
	{
		// arg, punt
		SetMoveType( MOVETYPE_STEP );// this is evil
		SetSolid( SOLID_BBOX );
		SetSolidFlags( FSOLID_NOT_STANDABLE );
	}
	m_hCine = NULL;
	SetTarget( NULL );
	m_pGoalEnt = NULL;
	if (m_lifeState == LIFE_DYING)
	{
		// last frame of death animation?
		m_iHealth			= 0;
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetState( NPC_STATE_DEAD );
		m_lifeState = LIFE_DEAD;
		UTIL_SetSize( this, WorldAlignMins(), Vector(WorldAlignMaxs().x, WorldAlignMaxs().y, WorldAlignMins().z + 2) );
		Relink();

		if ( pOldCine && pOldCine->HasSpawnFlags( SF_SCRIPT_LEAVECORPSE ) )
		{
			SetUse( NULL );		// BUGBUG -- This doesn't call Killed()
			SetThink( NULL );	// This will probably break some stuff
			SetTouch( NULL );
		}
		else
			SUB_StartFadeOut(); // SetThink( SUB_DoNothing );


		//Not becoming a ragdoll, so set the NOINTERP flag on.
		if ( CanBecomeRagdoll() == false )
		{
			StopAnimation();
  			m_fEffects |= EF_NOINTERP;	// Don't interpolate either, assume the corpse is positioned in its final resting place
		}

		SetMoveType( MOVETYPE_NONE );
		return false;
	}

	// If we actually played a sequence
	if ( pOldCine && pOldCine->m_iszPlay != NULL_STRING && pOldCine->PlayedSequence() )
	{
		if ( !pOldCine->HasSpawnFlags(SF_SCRIPT_NOSCRIPTMOVEMENT) )
		{
			// reset position
			Vector new_origin;
			QAngle new_angle;
			GetBonePosition( 0, new_origin, new_angle );

			// Figure out how far they have moved
			// We can't really solve this problem because we can't query the movement of the origin relative
			// to the sequence.  We can get the root bone's position as we do here, but there are
			// cases where the root bone is in a different relative position to the entity's origin
			// before/after the sequence plays.  So we are stuck doing this:

			// !!!HACKHACK: Float the origin up and drop to floor because some sequences have
			// irregular motion that can't be properly accounted for.

			// UNDONE: THIS SHOULD ONLY HAPPEN IF WE ACTUALLY PLAYED THE SEQUENCE.
			Vector oldOrigin = GetLocalOrigin();

			// UNDONE: ugly hack.  Don't move NPC if they don't "seem" to move
			// this really needs to be done with the AX,AY,etc. flags, but that aren't consistantly
			// being set, so animations that really do move won't be caught.
			if ((oldOrigin - new_origin).Length2D() < 8.0)
				new_origin = oldOrigin;

			Vector origin = GetLocalOrigin();

			origin.x = new_origin.x;
			origin.y = new_origin.y;
			origin.z += 1;

			if ( GetFlags() & FL_FLY )
			{
				origin.z = new_origin.z;
				SetLocalOrigin( origin );
			}
			else
			{
				SetLocalOrigin( origin );

				AddFlag(  FL_ONGROUND );
				int drop = UTIL_DropToFloor( this, MASK_NPCSOLID );

				// Origin in solid?  Set to org at the end of the sequence
				if ( drop < 0 )
				{
					SetLocalOrigin( oldOrigin );
				}
				else if ( drop == 0 ) // Hanging in air?
				{
					Vector origin = GetLocalOrigin();
					origin.z = new_origin.z;
					SetLocalOrigin( origin );
					RemoveFlag( FL_ONGROUND );
				}
			}

			// Call teleport to notify
			origin = GetLocalOrigin();
			Teleport( &origin, NULL, NULL );
			SetLocalOrigin( origin );

			m_fEffects |= EF_NOINTERP;

			if ( m_iHealth <= 0 )
			{
				// Dropping out because he got killed
				m_IdealNPCState = NPC_STATE_DEAD;
				SetCondition( COND_LIGHT_DAMAGE );
				m_lifeState = LIFE_DYING;
				m_lifeState = LIFE_ALIVE;
			}
		}

		// We should have some animation to put these guys in, but for now it's idle.
		// Due to NOINTERP above, there won't be any blending between this anim & the sequence
		m_Activity = ACT_RESET;
	}
	// set them back into a normal state
	if ( m_iHealth > 0 )
		m_IdealNPCState = NPC_STATE_IDLE; // m_previousState;
	else
	{
		// Dropping out because he got killed
		m_IdealNPCState = NPC_STATE_DEAD;
		SetCondition( COND_LIGHT_DAMAGE );
	}


	//	SetAnimation( m_NPCState );
	CLEARBITS(m_spawnflags, SF_NPC_WAIT_FOR_SCRIPT );

	return true;
}

//-----------------------------------------------------------------------------

void CAI_BaseNPC::TestPlayerPushing( CBaseEntity *pPlayer )
{
	// Heuristic for determining if the player is pushing me away
	float speed = fabs(pPlayer->GetAbsVelocity().x) + fabs(pPlayer->GetAbsVelocity().y);
	if ( speed > 50 )
	{
		SetCondition( COND_PLAYER_PUSHING );
		GetMotor()->SetIdealYawToTarget( pPlayer->GetLocalOrigin() ); // kind of intrusive.
	}
}

//-----------------------------------------------------------------------------

Navigation_t CAI_BaseNPC::GetNavType() const
{
	return m_pNavigator->GetNavType();
}

void CAI_BaseNPC::SetNavType( Navigation_t navType )
{
	m_pNavigator->SetNavType( navType );
}

//-----------------------------------------------------------------------------
// NPCs can override this to tweak with how costly particular movements are
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost )
{
	// We have nothing to say on the matter, but derived classes might
	return false;
}

bool CAI_BaseNPC::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	return false;
}

bool CAI_BaseNPC::OverrideMove( float flInterval )
{
	return false;
}


//=========================================================
// VecToYaw - turns a directional vector into a yaw value
// that points down that vector.
//=========================================================
float CAI_BaseNPC::VecToYaw( const Vector &vecDir )
{
	if (vecDir.x == 0 && vecDir.y == 0 && vecDir.z == 0)
		return GetLocalAngles().y;

	return UTIL_VecToYaw( vecDir );
}

//-----------------------------------------------------------------------------
// Inherited from IAI_MotorMovementServices
//-----------------------------------------------------------------------------
float CAI_BaseNPC::CalcYawSpeed( void )
{
	// Negative values are invalud
	return -1.0f;
}

bool CAI_BaseNPC::OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal,
										float distClear,
										AIMoveResult_t *pResult )
{
	if ( pMoveGoal->directTrace.pObstruction )
	{
		CBaseDoor *pDoor = dynamic_cast<CBaseDoor *>( pMoveGoal->directTrace.pObstruction );
		if ( pDoor && OnUpcomingDoor( pMoveGoal, pDoor, distClear, pResult ) )
		{
			return true;
		}

		CBasePropDoor *pPropDoor = dynamic_cast<CBasePropDoor *>( pMoveGoal->directTrace.pObstruction );
		if ( pPropDoor && OnUpcomingPropDoor( pMoveGoal, pPropDoor, distClear, pResult ) )
		{
			return true;
		}
	}

	return false;
}


bool CAI_BaseNPC::OnUpcomingDoor( AILocalMoveGoal_t *pMoveGoal,
 									 CBaseDoor *pDoor,
									 float distClear,
									 AIMoveResult_t *pResult )
{
	if ( pMoveGoal->maxDist < distClear )
		return false;

	// By default, NPCs don't know how to open doors
	if ( pDoor->m_toggle_state ==  TS_AT_BOTTOM || pDoor->m_toggle_state == TS_GOING_DOWN )
	{
		if ( distClear < 0.1 )
		{
			*pResult = AIMR_BLOCKED_ENTITY;
		}
		else
		{
			pMoveGoal->maxDist = distClear;
			*pResult = AIMR_OK;
		}

		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pMoveGoal - 
//			pDoor - 
//			distClear - 
//			default - 
//			spawn - 
//			oldorg - 
//			pfPosition - 
//			neworg - 
// Output : Returns true if movement is blocked, false otherwise.
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::OnUpcomingPropDoor( AILocalMoveGoal_t *pMoveGoal,
 										CBasePropDoor *pDoor,
										float distClear,
										AIMoveResult_t *pResult )
{
	if ( pMoveGoal->maxDist < distClear )
		return false;

	if (pDoor == m_hOpeningDoor)
	{
		// We're in the process of opening the door, don't be blocked by it.
		return false;
	}

	if (CapabilitiesGet() & bits_CAP_DOORS_GROUP)
	{
		AI_Waypoint_t *pOpenDoorRoute = NULL;

		opendata_t opendata;
		pDoor->GetNPCOpenData(this, opendata);
		
		// dvs: FIXME: local route might not be sufficient
		pOpenDoorRoute = GetPathfinder()->BuildLocalRoute(
			GetLocalOrigin(), 
			opendata.vecStandPos,
			NULL, 
			bits_WP_TO_DOOR | bits_WP_DONT_SIMPLIFY, 
			NO_NODE,
			bits_BUILD_GROUND,
			0.0);
		
		if ( !pOpenDoorRoute )
			return true;

		// Attach the door to the waypoint so we open it when we get there.
		// dvs: FIXME: this is kind of bullshit, I need to find the exact waypoint to open the door
		//		should I just walk the path until I find it?
		pOpenDoorRoute->m_hData = pDoor;

		if (GetNavigator()->GetPath()->PrependWaypoints( pOpenDoorRoute ))
		{
			m_hOpeningDoor = pDoor;
			pMoveGoal->maxDist = distClear;
			*pResult = AIMR_OK;
				
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Called when the door we were trying to open becomes fully open.
// Input  : pDoor - 
//-----------------------------------------------------------------------------
void CAI_BaseNPC::OnDoorFullyOpen(CBasePropDoor *pDoor)
{
	// We're done with the door.
	m_hOpeningDoor = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Called when the door we were trying to open becomes blocked before opening.
// Input  : pDoor - 
//-----------------------------------------------------------------------------
void CAI_BaseNPC::OnDoorBlocked(CBasePropDoor *pDoor)
{
	// dvs: FIXME: do something so that we don't loop forever trying to open this door
	//		not clearing out the door handle will cause the NPC to invalidate the connection
	// We're done with the door.
	//m_hOpeningDoor = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Template NPCs are marked as templates by the level designer. They
//			do not spawn, but their keyvalues are saved for use by a template
//			spawner.
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsTemplate( void )
{
	return HasSpawnFlags( SF_NPC_TEMPLATE );
}



//-----------------------------------------------------------------------------
//
// Movement code for walking + flying
//
//-----------------------------------------------------------------------------
int CAI_BaseNPC::FlyMove( const Vector& pfPosition, unsigned int mask )
{
	Vector		oldorg, neworg;
	trace_t		trace;

	// try the move	
	VectorCopy( GetAbsOrigin(), oldorg );
	VectorAdd( oldorg, pfPosition, neworg );
	UTIL_TraceEntity( this, oldorg, neworg, mask, &trace );				
	if (trace.fraction == 1)
	{
		if ( (GetFlags() & FL_SWIM) && enginetrace->GetPointContents(trace.endpos) == CONTENTS_EMPTY )
			return false;	// swim monster left water

		SetAbsOrigin( trace.endpos );
		engine->RelinkEntity( GetEdict(), true );
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ent - 
//			Dir - Normalized direction vector for movement.
//			dist - Distance along 'Dir' to move.
//			iMode - 
// Output : Returns nonzero on success, zero on failure.
//-----------------------------------------------------------------------------
int CAI_BaseNPC::WalkMove( const Vector& vecPosition, unsigned int mask )
{	
	if ( GetFlags() & (FL_FLY | FL_SWIM) )
	{
		return FlyMove( vecPosition, mask );
	}

	if ( (GetFlags() & FL_ONGROUND) == 0 )
	{
		return 0;
	}

	trace_t	trace;
	Vector oldorg, neworg, end;
	Vector move( vecPosition[0], vecPosition[1], 0.0f );
	VectorCopy( GetAbsOrigin(), oldorg );
	VectorAdd( oldorg, move, neworg );

	// push down from a step height above the wished position
	float flStepSize = sv_stepsize.GetFloat();
	neworg[2] += flStepSize;
	VectorCopy(neworg, end);
	end[2] -= flStepSize*2;

	UTIL_TraceEntity( this, neworg, end, mask, &trace );
	if ( trace.allsolid )
		return false;

	if (trace.startsolid)
	{
		neworg[2] -= flStepSize;
		UTIL_TraceEntity( this, neworg, end, mask, &trace );
		if ( trace.allsolid || trace.startsolid )
			return false;
	}

	if (trace.fraction == 1)
	{
		// if monster had the ground pulled out, go ahead and fall
		if ( GetFlags() & FL_PARTIALGROUND )
		{
			SetAbsOrigin( oldorg + move );
			engine->RelinkEntity( GetEdict(), true );
			RemoveFlag( FL_ONGROUND );
			return true;
		}
	
		return false;		// walked off an edge
	}

	// check point traces down for dangling corners
	SetAbsOrigin( trace.endpos );

	if (UTIL_CheckBottom( this, NULL, flStepSize ) == 0)
	{
		if ( GetFlags() & FL_PARTIALGROUND )
		{	
			// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			engine->RelinkEntity( GetEdict(), true );
			return true;
		}

		// Reset to original position
		SetAbsOrigin( oldorg );
		return false;
	}

	if ( GetFlags() & FL_PARTIALGROUND )
	{
		// Con_Printf ("back on ground\n"); 
		RemoveFlag( FL_PARTIALGROUND );
	}

	// the move is ok
	SetGroundEntity( trace.m_pEnt );
	engine->RelinkEntity( GetEdict(), true );
	return true;
}

//-----------------------------------------------------------------------------

static void AIMsgGuts( CAI_BaseNPC *pAI, unsigned flags, const char *pszMsg )
{
	static float lastTime;
	static int counter;

	int			len		= strlen( pszMsg );
	const char *pszFmt2 = NULL;

	if ( lastTime != gpGlobals->curtime )
	{
		lastTime = gpGlobals->curtime;
		if ( ++counter > 100 )
			counter = 1;	 
	}

	if ( len && pszMsg[len-1] == '\n' )
	{
		(const_cast<char *>(pszMsg))[len-1] = 0;
		pszFmt2 = "%s (%s: %s) [%d]\n";
	}
	else
		pszFmt2 = "%s (%s: %s) [%d]";
	
	Msg( pszFmt2, 
		 pszMsg, 
		 pAI->GetClassname(),
		 ( pAI->GetEntityName() == NULL_STRING ) ? "<unnamed>" : STRING(pAI->GetEntityName()),
		 counter );
}

void Msg( CAI_BaseNPC *pAI, unsigned flags, const char *pszFormat, ... )
{
	if ( (flags & AIMF_IGNORE_SELECTED) || (pAI->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		AIMsgGuts( pAI, flags, CFmtStr( &pszFormat ) );
	}
}

//-----------------------------------------------------------------------------

void Msg( CAI_BaseNPC *pAI, const char *pszFormat, ... )
{
	if ( (pAI->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		AIMsgGuts( pAI, 0, CFmtStr( &pszFormat ) );
	}
}
//-----------------------------------------------------------------------------

void CAI_BaseNPC::PlayerSelect( bool select )
{
	m_bSelectedByPlayer = select;

	if( select )
	{
		SetCondition( COND_PLAYER_SELECTED );
		OnPlayerSelect();
	}
	else
	{
		SetCondition( COND_PLAYER_UNSELECTED );
	}
}

//-----------------------------------------------------------------------------

Vector CAI_BaseNPC::GetSmoothedVelocity( void )
{
	if( GetMoveType() == MOVETYPE_STEP )
	{
		return m_pMotor->GetCurVel();
	}

	return BaseClass::GetSmoothedVelocity();
}


//-----------------------------------------------------------------------------

bool CAI_BaseNPC::IsCoverPosition( const Vector &vecThreat, const Vector &vecPosition )
{
	trace_t	tr;
	AI_TraceLine( vecThreat, vecPosition, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);
	return (tr.fraction != 1.0);
}



//=========================================================
//=========================================================
class CAI_Relationship : public CBaseEntity
{
	DECLARE_CLASS( CAI_Relationship, CBaseEntity );

public:
	CAI_Relationship() { m_iPreviousDisposition = -1; }
	void Spawn();
	void ChangeRelationships( int disposition, bool fReverting );

private:
	string_t	m_iszSubject;
	string_t	m_iszTarget;
	int			m_iDisposition;
	int			m_iRank;
	int			m_iPreviousDisposition;

public:
	// Input functions
	void InputApplyRelationship( inputdata_t &inputdata );
	void InputRevertRelationship( inputdata_t &inputdata );

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( ai_relationship, CAI_Relationship );

BEGIN_DATADESC( CAI_Relationship )
	DEFINE_KEYFIELD( CAI_Relationship, m_iszSubject, FIELD_STRING, "subject" ),
	DEFINE_KEYFIELD( CAI_Relationship, m_iszTarget, FIELD_STRING, "target" ),
	DEFINE_KEYFIELD( CAI_Relationship, m_iDisposition, FIELD_INTEGER, "disposition" ),
	DEFINE_KEYFIELD( CAI_Relationship, m_iRank, FIELD_INTEGER, "rank" ),
	DEFINE_FIELD( CAI_Relationship, m_iPreviousDisposition, FIELD_INTEGER ),


	// Inputs
	DEFINE_INPUTFUNC( CAI_Relationship, FIELD_VOID, "ApplyRelationship", InputApplyRelationship ),
	DEFINE_INPUTFUNC( CAI_Relationship, FIELD_VOID, "RevertRelationship", InputRevertRelationship ),
END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::Spawn()
{
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::InputApplyRelationship( inputdata_t &inputdata )
{
	ChangeRelationships( m_iDisposition, false );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::InputRevertRelationship( inputdata_t &inputdata )
{
	ChangeRelationships( m_iPreviousDisposition, true );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::ChangeRelationships( int disposition, bool fReverting )
{
	if( fReverting && m_iPreviousDisposition == -1 )
	{
		// Trying to revert without having ever set the relationships!
		Msg("**ERROR! ai_relationship cannot revert changes before they are applied!\n");
		return;
	}

	// Assume the same thing for the target.
	CBaseEntity *pTarget;
	pTarget = gEntList.FindEntityByName( NULL, m_iszTarget, this );

	CBaseEntity *pSubjectList[ 100 ];
	CBaseEntity *pTargetList[ 100 ];
	int numSubjects = 0;
	int numTargets = 0;

	// Populate the subject list. Try targetname first.
	CBaseEntity *pSearch = NULL;
	do
	{
		pSearch = gEntList.FindEntityByName( pSearch, m_iszSubject, this );
		if( pSearch )
		{
			pSubjectList[ numSubjects ] = pSearch;
			numSubjects++;
		}
	} while( pSearch );

	if( numSubjects == 0 )
	{
		// No subjects found by targetname! Assume classname.
		do
		{
			pSearch = gEntList.FindEntityByClassname( pSearch, STRING(m_iszSubject) );
			if( pSearch )
			{
				pSubjectList[ numSubjects ] = pSearch;
				numSubjects++;
			}
		} while( pSearch );
	}
	// If the subject list is still empty, we have an error!
	if( numSubjects == 0 )
	{
		Msg("**ERROR! ai_relationship finds no subject(s) called: %s\n", STRING( m_iszSubject ) );
		return;
	}


	// Now populate the target list.
	bool fTargetIsClass = false;
	do
	{
		pSearch = gEntList.FindEntityByName( pSearch, m_iszTarget, this );
		if( pSearch )
		{
			pTargetList[ numTargets ] = pSearch;
			numTargets++;
		}
	} while( pSearch );

	if( numTargets == 0 )
	{
		// No subjects found by targetname! Assume classname.
		fTargetIsClass = true;
		do
		{
			pSearch = gEntList.FindEntityByClassname( pSearch, STRING(m_iszTarget) );
			if( pSearch )
			{
				pTargetList[ numTargets ] = pSearch;
				numTargets++;
			}
		} while( pSearch );
	}
	// If the subject list is still empty, we have an error!
	if( numTargets == 0 )
	{
		Msg("**ERROR! ai_relationship finds no target(s) called: %s\n", STRING( m_iszTarget ) );
		return;
	}

	// Ok, lists are populated. Apply all relationships.
	if( fTargetIsClass )
	{
		int i;
		for( i = 0 ; i < numSubjects ; i++ )
		{
			CAI_BaseNPC *pSubject = pSubjectList[ i ]->MyNPCPointer();
			if( !pSubject )
			{
				Msg("Skipping non-npc Subject\n" );
				continue;
			}

			// Since the target list is populated by indentical creatures, just use
			// the first one to get the class's AI classification for all subjects.
			CAI_BaseNPC *pTarget = pTargetList[ 0 ]->MyNPCPointer();
			if( !pTarget )
			{
				Msg("Skipping non-npc Target\n" );
				continue;
			}

			if( m_iPreviousDisposition == -1 )
			{
				// Set previous disposition.
				m_iPreviousDisposition = pSubject->IRelationType( pTarget );
			}

			pSubject->AddClassRelationship( pTarget->Classify(), (Disposition_t)disposition, m_iRank );
		}
	}
	else
	{
		int i,j;
		for( i = 0 ; i < numSubjects ; i++ )
		{
			CAI_BaseNPC *pSubject = pSubjectList[ i ]->MyNPCPointer();

			if( !pSubject )
			{
				Msg("Skipping non-npc Subject\n" );
				continue;
			}

			for( j = 0 ; j < numTargets ; j++ )
			{
				CAI_BaseNPC *pTarget = pTargetList[ j ]->MyNPCPointer();

				if( pTarget )
				{
					if( m_iPreviousDisposition == -1 )
					{
						// Set previous disposition.
						m_iPreviousDisposition = pSubject->IRelationType( pTarget );
					}

					pSubject->AddEntityRelationship( pTarget, (Disposition_t)disposition, m_iRank );
				}
				else
				{
					// Not an NPC. Is it a player?
					if( pTargetList[ j ]->IsPlayer() )
					{
						if( m_iPreviousDisposition == -1 )
						{
							// Set previous disposition.
							m_iPreviousDisposition = pSubject->IRelationType( pTargetList[ j ] );
						}

						pSubject->AddEntityRelationship( pTargetList[ j ], (Disposition_t)disposition, m_iRank );
					}
					else
					{
						Msg("Skipping non-npc Target\n" );
						continue;
					}
				}
			}
		}
	}
}

