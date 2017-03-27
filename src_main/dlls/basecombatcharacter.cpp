//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Base combat character with no AI
//
//=============================================================================

#include "cbase.h"
#include "basecombatcharacter.h"
#include "basecombatweapon.h"
#include "animation.h"
#include "gib.h"
#include "entitylist.h"
#include "gamerules.h"
#include "ai_basenpc.h"
#include "ai_squadslot.h"
#include "ammodef.h"
#include "ndebugoverlay.h"
#include "player.h"
#include "physics.h"
#include "engine/IEngineSound.h"
#include "vstdlib/strtools.h"
#include "sendproxy.h"
#include "EntityFlame.h"
#include "CRagdollMagnet.h"
#include "iservervehicle.h"
#include "igamesystem.h"
#include "globals.h"
#include "physics_prop_ragdoll.h"
#include "physics_impact_damage.h"
#include "saverestore_utlvector.h"
#include "EventQueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar weapon_showproficiency;

// Weapon proficiency table. Keep this in sync with WeaponProficiency_t enum in the header!!
proficiencyinfo_t g_WeaponProficiencyTable[] =
{
	{ 1.50, "VERY LOW"	},
	{ 1.40, "LOW"		},
	{ 1.30, "AVERAGE"	},
	{ 1.20, "HIGH"		},
	{ 1.10, "VERY HIGH" },
	{ 1.00, "PERFECT"	},
};

BEGIN_DATADESC( CBaseCombatCharacter )

#ifdef TF2_DLL
	DEFINE_FIELD( CBaseCombatCharacter, m_iPowerups, FIELD_INTEGER ),
	DEFINE_ARRAY( CBaseCombatCharacter, m_flPowerupAttemptTimes, FIELD_TIME, MAX_POWERUPS ),
	DEFINE_ARRAY( CBaseCombatCharacter, m_flPowerupEndTimes, FIELD_TIME, MAX_POWERUPS ),
	DEFINE_FIELD( CBaseCombatCharacter, m_flFractionalBoost, FIELD_FLOAT ),
#endif

	DEFINE_FIELD( CBaseCombatCharacter, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CBaseCombatCharacter, m_eHull, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatCharacter, m_bloodColor, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatCharacter, m_bOnFire, FIELD_BOOLEAN ),
	
	DEFINE_FIELD( CBaseCombatCharacter, m_flFieldOfView, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseCombatCharacter, m_HackedGunPos, FIELD_VECTOR ),
	DEFINE_KEYFIELD( CBaseCombatCharacter, m_RelationshipString, FIELD_STRING, "Relationship" ),

	DEFINE_FIELD( CBaseCombatCharacter, m_LastHitGroup, FIELD_INTEGER ),
	DEFINE_INPUT( CBaseCombatCharacter, m_impactEnergyScale, FIELD_FLOAT, "physdamagescale" ),
	DEFINE_FIELD( CBaseCombatCharacter, m_CurrentWeaponProficiency, FIELD_INTEGER),

	DEFINE_UTLVECTOR( CBaseCombatCharacter, m_Relationship,	FIELD_EMBEDDED),

	DEFINE_AUTO_ARRAY( CBaseCombatCharacter, m_iAmmo, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CBaseCombatCharacter, m_hMyWeapons, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseCombatCharacter, m_hActiveWeapon, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( CBaseCombatCharacter, FIELD_VOID, "KilledNPC", InputKilledNPC ),

END_DATADESC()


BEGIN_PREDICTION_DATA( CBaseCombatCharacter )

	DEFINE_FIELD( CBaseCombatCharacter, m_flNextAttack, FIELD_FLOAT ),
	//DEFINE_FIELD( CBaseCombatCharacter, m_eHull, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatCharacter, m_bloodColor, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatCharacter, m_LastHitGroup, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseCombatCharacter, m_impactEnergyScale, FIELD_FLOAT ),
	// DEFINE_FIELD( CBaseCombatCharacter, m_Relationship, Relationship_t ),
	// DEFINE_FIELD( CBaseCombatCharacter, m_RelationshipString, string_t ),
	DEFINE_FIELD( CBaseCombatCharacter, m_flFieldOfView, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseCombatCharacter, m_HackedGunPos, FIELD_VECTOR ),
	// DEFINE_ARRAY( CBaseCombatCharacter, m_hMyWeapons, CHandle < CBaseCombatWeapon >, MAX_WEAPONS ),
	// DEFINE_FIELD( CBaseCombatCharacter, m_hActiveWeapon, CHandle < CBaseCombatWeapon > ),
	// DEFINE_FIELD( CBaseCombatCharacter, m_hLastWeapon, CHandle < CBaseCombatWeapon > ),
	DEFINE_AUTO_ARRAY( CBaseCombatCharacter, m_iAmmo, FIELD_INTEGER ),

END_PREDICTION_DATA()


BEGIN_SIMPLE_DATADESC( Relationship_t )
	DEFINE_FIELD( Relationship_t,		entity,			FIELD_EHANDLE ),
	DEFINE_FIELD( Relationship_t,		classType,		FIELD_INTEGER ),
	DEFINE_FIELD( Relationship_t,		disposition,	FIELD_INTEGER ),
	DEFINE_FIELD( Relationship_t,		priority,	FIELD_INTEGER ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
int					CBaseCombatCharacter::m_lastInteraction   = 0;
Relationship_t**	CBaseCombatCharacter::m_DefaultRelationship	= NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCleanupDefaultRelationShips : public CAutoGameSystem
{
public:
	virtual void Shutdown()
	{
		if ( !CBaseCombatCharacter::m_DefaultRelationship )
			return;

		for ( int i=0; i<NUM_AI_CLASSES; ++i )
		{
			delete[] CBaseCombatCharacter::m_DefaultRelationship[ i ];
		}

		delete[] CBaseCombatCharacter::m_DefaultRelationship;
		CBaseCombatCharacter::m_DefaultRelationship = NULL;
	}
};

static CCleanupDefaultRelationShips g_CleanupDefaultRelationships;

void *SendProxy_SendBaseCombatCharacterLocalDataTable( const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CBaseCombatCharacter *pBCC = ( CBaseCombatCharacter * )pStruct;
	// Only send to local player if this is a player
	pRecipients->ClearAllRecipients();
	if ( pBCC )
	{
		if ( pBCC->IsPlayer() )
		{
			pRecipients->SetOnly( pBCC->entindex() - 1 );
		}
		else
		{
			// If it's a vehicle, send to "driver" (e.g., operator of tf2 manned guns)
			IServerVehicle *v = pBCC->GetServerVehicle();
			if ( v )
			{
				CBasePlayer *driver = v->GetPassenger();
				if ( driver )
				{
					pRecipients->SetOnly( driver->entindex() - 1 );
				}
			}
		}
	}
	return ( void * )pVarData;
}

// Only send active weapon index to local player
BEGIN_SEND_TABLE_NOBASE( CBaseCombatCharacter, DT_BCCLocalPlayerExclusive )
	SendPropTime( SENDINFO( m_flNextAttack ) ),
	SendPropArray( SendPropEHandle( SENDINFO_ARRAY( m_hMyWeapons ) ), m_hMyWeapons ),
END_SEND_TABLE();

//-----------------------------------------------------------------------------
// This table encodes the CBaseCombatCharacter
//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST(CBaseCombatCharacter, DT_BaseCombatCharacter)
	// Data that only gets sent to the local player.
	SendPropDataTable( "bcc_localdata", 0, &REFERENCE_SEND_TABLE(DT_BCCLocalPlayerExclusive), SendProxy_SendBaseCombatCharacterLocalDataTable ),

	SendPropEHandle( SENDINFO( m_hActiveWeapon ) ),

#ifdef TF2_DLL
	SendPropInt( SENDINFO(m_iPowerups), MAX_POWERUPS, SPROP_UNSIGNED ), 
#endif

END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::InitInteractionSystem()
{
	m_lastInteraction = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Return an interaction ID (so we have no collisions)
//-----------------------------------------------------------------------------
int	CBaseCombatCharacter::GetInteractionID(void)
{
	m_lastInteraction++;
	return (m_lastInteraction);
}

// ============================================================================
bool CBaseCombatCharacter::HasHumanGibs( void )
{
	Class_T myClass = Classify();

	if ( myClass == CLASS_CITIZEN_PASSIVE   ||
		 myClass == CLASS_CITIZEN_REBEL		||
		 myClass == CLASS_COMBINE			||
		 myClass == CLASS_CONSCRIPT			||
		 myClass == CLASS_METROPOLICE		||
		 myClass == CLASS_PLAYER )	
		 return true;

#if defined( HL1_DLL )
	if (	myClass == CLASS_HUMAN_MILITARY		||
			myClass == CLASS_PLAYER_ALLY		||
			myClass == CLASS_HUMAN_PASSIVE		||
			myClass == CLASS_PLAYER )
	{
		return true;
	}
#endif	// defined( HL1_DLL )

	return false;
}


bool CBaseCombatCharacter::HasAlienGibs( void )
{
	Class_T myClass = Classify();

	if ( myClass == CLASS_BARNACLE		 || 
		 myClass == CLASS_BULLSQUID		 ||
		 myClass == CLASS_STALKER		 ||
		 myClass == CLASS_ZOMBIE		 ||
		 myClass == CLASS_VORTIGAUNT	 ||
		 myClass == CLASS_HEADCRAB		 ||
		 myClass == CLASS_HOUNDEYE	)
		 return true;

#if defined( HL1_DLL )
	if (	myClass == CLASS_ALIEN_PREY		||
			myClass == CLASS_ALIEN_MILITARY ||
			myClass == CLASS_ALIEN_MONSTER	||
			myClass == CLASS_ALIEN_PREDATOR )
	{
		return true;
	}
#endif	// defined( HL1_DLL )

	return false;
}


void CBaseCombatCharacter::CorpseFade( void )
{
	StopAnimation();
	SetAbsVelocity( vec3_origin );
	SetMoveType( MOVETYPE_NONE );
	SetLocalAngularVelocity( vec3_angle );
	m_flAnimTime = gpGlobals->curtime;
	m_fEffects |= EF_NOINTERP;
	SUB_StartFadeOut();
}

//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool CBaseCombatCharacter::FInViewCone( CBaseEntity *pEntity )
{
	return FInViewCone( pEntity->WorldSpaceCenter() );
}

//=========================================================
// FInViewCone - returns true is the passed Vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool CBaseCombatCharacter::FInViewCone( const Vector &vecSpot )
{
	Vector los = ( vecSpot - EyePosition() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = EyeDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > m_flFieldOfView )
		return true;

	return false;
}


//=========================================================
// FInAimCone - returns true is the passed ent is in
// the caller's forward aim cone. The dot product is performed
// in 2d, making the aim cone infinitely tall. 
//=========================================================
bool CBaseCombatCharacter::FInAimCone( CBaseEntity *pEntity )
{
	return FInAimCone( pEntity->BodyTarget( EyePosition() ) );
}


//=========================================================
// FInAimCone - returns true is the passed Vector is in
// the caller's forward aim cone. The dot product is performed
// in 2d, making the view cone infinitely tall. By default, the
// callers aim cone is assumed to be very narrow
//=========================================================
bool CBaseCombatCharacter::FInAimCone( const Vector &vecSpot )
{
	Vector los = ( vecSpot - GetAbsOrigin() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = BodyDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > 0.994 )//!!!BUGBUG - magic number same as FacingIdeal(), what is this?
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  The type of interaction, extra info pointer, and who started it
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::HandleInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor : Initialize some fields
//-----------------------------------------------------------------------------
CBaseCombatCharacter::CBaseCombatCharacter( void )
{
#ifdef _DEBUG
	// necessary since in debug, we initialize vectors to NAN for debugging
	m_HackedGunPos.Init();
#endif

	// Init weapon and Ammo data
	m_hActiveWeapon			= NULL;

	// reset all ammo values to 0
	RemoveAllAmmo();
}

//------------------------------------------------------------------------------
// Purpose : Destructor
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseCombatCharacter::~CBaseCombatCharacter( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::UpdateOnRemove( void )
{
	int i;
	// Make sure any weapons I didn't drop get removed.
	for (i=0;i<MAX_WEAPONS;i++) 
	{
		if (m_hMyWeapons[i]) 
		{
			UTIL_Remove( m_hMyWeapons[i] );
		}
	}

	// Chain at end to mimic destructor unwind order
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Blood color (see BLOOD_COLOR_* macros in baseentity.h)
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetBloodColor( int nBloodColor )
{
	m_bloodColor = nBloodColor;
}


//=========================================================
// CorpseGib - create some gore and get rid of a character's
// model.
//=========================================================
bool CBaseCombatCharacter::CorpseGib( const CTakeDamageInfo &info )
{
	trace_t		tr;
	bool		gibbed = false;

	EmitSound( "BaseCombatCharacter.CorpseGib" );

	// only humans throw skulls !!!UNDONE - eventually NPCs will have their own sets of gibs
	if ( HasHumanGibs() )
	{
		CGib::SpawnHeadGib( this );
		CGib::SpawnRandomGibs( this, 4, GIB_HUMAN );	// throw some human gibs.
		gibbed = true;
	}
	else if ( HasAlienGibs() )
	{
		CGib::SpawnRandomGibs( this, 4, GIB_ALIEN );	// Throw alien gibs
		gibbed = true;
	}

	return gibbed;
}

//=========================================================
// GetDeathActivity - determines the best type of death
// anim to play.
//=========================================================
Activity CBaseCombatCharacter::GetDeathActivity ( void )
{
	Activity	deathActivity;
	bool		fTriedDirection;
	float		flDot;
	trace_t		tr;
	Vector		vecSrc;

	if (IsPlayer())
	{
		// die in an interesting way
		switch( random->RandomInt(0,7) )
		{
		case 0:	return ACT_DIESIMPLE;
		case 1: return ACT_DIEBACKWARD;
		case 2: return ACT_DIEFORWARD;
		case 3: return ACT_DIEVIOLENT;
		case 4: return ACT_DIE_HEADSHOT;
		case 5: return ACT_DIE_CHESTSHOT;
		case 6: return ACT_DIE_GUTSHOT;
		case 7: return ACT_DIE_BACKSHOT;
		}
	}

	vecSrc = WorldSpaceCenter();

	fTriedDirection = false;
	deathActivity = ACT_DIESIMPLE;// in case we can't find any special deaths to do.

	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );
	flDot = -DotProduct( forward, g_vecAttackDir );

	switch ( m_LastHitGroup )
	{
		// try to pick a region-specific death.
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;

	case HITGROUP_STOMACH:
		deathActivity = ACT_DIE_GUTSHOT;
		break;

	case HITGROUP_GENERIC:
		// try to pick a death based on attack direction
		fTriedDirection = true;

		if ( flDot > 0.3 )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if ( flDot <= -0.3 )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;

	default:
		// try to pick a death based on attack direction
		fTriedDirection = true;

		if ( flDot > 0.3 )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if ( flDot <= -0.3 )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	}


	// can we perform the prescribed death?
	if ( SelectWeightedSequence ( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// no! did we fail to perform a directional death? 
		if ( fTriedDirection )
		{
			// if yes, we're out of options. Go simple.
			deathActivity = ACT_DIESIMPLE;
		}
		else
		{
			// cannot perform the ideal region-specific death, so try a direction.
			if ( flDot > 0.3 )
			{
				deathActivity = ACT_DIEFORWARD;
			}
			else if ( flDot <= -0.3 )
			{
				deathActivity = ACT_DIEBACKWARD;
			}
		}
	}

	if ( SelectWeightedSequence ( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// if we're still invalid, simple is our only option.
		deathActivity = ACT_DIESIMPLE;

		if ( SelectWeightedSequence ( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
		{
			Msg( "ERROR! %s missing ACT_DIESIMPLE\n", STRING(GetModelName()) );
		}
	}

	if ( deathActivity == ACT_DIEFORWARD )
	{
			// make sure there's room to fall forward
			UTIL_TraceHull ( vecSrc, vecSrc + forward * 64, Vector(-16,-16,-18), 
				Vector(16,16,18), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction != 1.0 )
			{
				deathActivity = ACT_DIESIMPLE;
			}
	}

	if ( deathActivity == ACT_DIEBACKWARD )
	{
			// make sure there's room to fall backward
			UTIL_TraceHull ( vecSrc, vecSrc - forward * 64, Vector(-16,-16,-18), 
				Vector(16,16,18), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction != 1.0 )
			{
				deathActivity = ACT_DIESIMPLE;
			}
	}

	return deathActivity;
}


// UNDONE: Should these operate on a list of weapon/items
Activity CBaseCombatCharacter::Weapon_TranslateActivity( Activity baseAct )
{
	Activity translated = baseAct;

	if ( m_hActiveWeapon )
	{
		translated = m_hActiveWeapon->ActivityOverride( baseAct );
	}

	return translated;
}

//-----------------------------------------------------------------------------
// Purpose: NPCs should override this function to translate activities
//			such as ACT_WALK, etc.
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CBaseCombatCharacter::NPC_TranslateActivity( Activity baseAct )
{
	return baseAct;
}


void CBaseCombatCharacter::Weapon_SetActivity( Activity newActivity, float duration )
{
	if ( m_hActiveWeapon )
	{
		m_hActiveWeapon->SetActivity( newActivity, duration );
	}
}

void CBaseCombatCharacter::Weapon_FrameUpdate( void )
{
	if ( m_hActiveWeapon )
	{
		m_hActiveWeapon->Operator_FrameUpdate( this );
	}
}


//------------------------------------------------------------------------------
// Purpose :	expects a length to trace, amount 
//				of damage to do, and damage type. Returns a pointer to
//				the damaged entity in case the NPC wishes to do
//				other stuff to the victim (punchangle, etc)
//
//				Used for many contact-range melee attacks. Bites, claws, etc.
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity *CBaseCombatCharacter::CheckTraceHullAttack( float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType )
{
	// If only a length is given assume we want to trace in our facing direction
	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );
	Vector vStart = GetLocalOrigin();

	// The ideal place to start the trace is in the center of the attacker's bounding box.
	// however, we need to make sure there's enough clearance. Some of the smaller monsters aren't 
	// as big as the hull we try to trace with. (SJB)
	float flVerticalOffset = WorldAlignSize().z * 0.5;

	if( flVerticalOffset < maxs.z )
	{
		// There isn't enough room to trace this hull, it's going to drag the ground.
		// so make the vertical offset just enough to clear the ground.
		flVerticalOffset = maxs.z + 1.0;
	}

	vStart.z += flVerticalOffset;
	Vector vEnd = vStart + (forward * flDist );

	return CheckTraceHullAttack( vStart, vEnd, mins, maxs, iDamage, iDmgType );
}

//------------------------------------------------------------------------------
// Purpose :	start and end trace position, amount 
//				of damage to do, and damage type. Returns a pointer to
//				the damaged entity in case the NPC wishes to do
//				other stuff to the victim (punchangle, etc)
//
//				Used for many contact-range melee attacks. Bites, claws, etc.
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity *CBaseCombatCharacter::CheckTraceHullAttack( const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType )
{
	trace_t tr;
	UTIL_TraceHull( vStart, vEnd, mins, maxs, MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

/*
	// Handy debuging tool to visualize HullAttack trace
	float length	 = (vEnd - vStart ).Length();
	Vector direction = (vEnd - vStart );
	VectorNormalize( direction );
	Vector hullMaxs = maxs;
	hullMaxs.x = length + hullMaxs.x;
	NDebugOverlay::BoxDirection(vStart, mins, hullMaxs, direction, 100,255,255,20,1.0);
	NDebugOverlay::BoxDirection(vStart, mins, maxs, direction, 255,0,0,20,1.0);
*/
	if ( !tr.startsolid && tr.m_pEnt )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if ( !pEntity->m_takedamage )
			return NULL;

		if (IRelationType( pEntity ) == D_HT )
		{
			if ( iDamage > 0 )
			{
				CTakeDamageInfo info( this, this, iDamage, iDmgType );
				CalculateMeleeDamageForce( &info, (vEnd - vStart), vStart );
				pEntity->TakeDamage( info );
			}
		}
		return pEntity;
	}

	return NULL;
}


bool  CBaseCombatCharacter::Event_Gibbed( const CTakeDamageInfo &info )
{
	bool fade = false;

	if ( HasHumanGibs() )
	{
		ConVar const *hgibs = cvar->FindVar( "violence_hgibs" );

		if ( hgibs && hgibs->GetInt() == 0 )
		{
			fade = true;
		}
	}
	else if ( HasAlienGibs() )
	{
		ConVar const *agibs = cvar->FindVar( "violence_agibs" );

		if ( agibs && agibs->GetInt() == 0 )
		{
			fade = true;
		}
	}

	m_takedamage	= DAMAGE_NO;
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_lifeState		= LIFE_DEAD;

	Relink();

	if ( fade )
	{
		CorpseFade();
		return false;
	}
	else
	{
		m_fEffects |= EF_NODRAW; // make the model invisible.
		return CorpseGib( info );
	}
}


Vector CBaseCombatCharacter::CalcDamageForceVector( const CTakeDamageInfo &info )
{
	// Already have a damage force in the data, use that.
	if ( info.GetDamageForce() != vec3_origin )
	{
		if( info.GetDamageType() & DMG_BLAST )
		{
			// Fudge blast forces a little bit, so that each
			// victim gets a slightly different trajectory. 
			// This simulates features that usually vary from
			// person-to-person variables such as bodyweight,
			// which are all indentical for characters using the same model.
			return info.GetDamageForce() * random->RandomFloat( 0.85, 1.15 );
		}

		return info.GetDamageForce();
	}

	CBaseEntity *pForce = info.GetInflictor();
	if ( !pForce )
	{
		pForce = info.GetAttacker();
	}

	if ( pForce )
	{
		// Calculate an impulse large enough to push a 75kg man 4 in/sec per point of damage
		float forceScale = info.GetDamage() * 75 * 4;

		Vector forceVector;
		// If the damage is a blast, point the force vector higher than usual, this gives 
		// the ragdolls a bodacious "really got blowed up" look.
		if( info.GetDamageType() & DMG_BLAST )
		{
			// exaggerate the force from explosions a little (50%)
			forceVector = (GetLocalOrigin() + Vector(0, 0, WorldAlignSize().z) ) - pForce->GetLocalOrigin();
			VectorNormalize(forceVector);
			forceVector *= 1.5f;
		}
		else
		{
			// taking damage from self?  fly backward
			if ( pev == pForce->pev )
			{
				AngleVectors( GetAbsAngles(), &forceVector );
				forceVector *= -1.0f;
			}
			else
			{
				// UNDONE: Collision forces are baked in to CTakeDamageInfo now
				// UNDONE: Is this MOVETYPE_VPHYSICS code still necessary?
				if ( pForce->GetMoveType() == MOVETYPE_VPHYSICS )
				{
					// killed by a physics object
					IPhysicsObject *pPhysics = VPhysicsGetObject();
					if ( !pPhysics )
					{
						pPhysics = pForce->VPhysicsGetObject();
					}
					pPhysics->GetVelocity( &forceVector, NULL );
					forceScale = pPhysics->GetMass();
				}
				else
				{
					forceVector = GetLocalOrigin() - pForce->GetLocalOrigin();
					VectorNormalize(forceVector);
				}
			}
		}
		return forceVector * forceScale;
	}
	return vec3_origin;
}

bool CBaseCombatCharacter::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector )
{
	if ( (info.GetDamageType() & DMG_VEHICLE) && !g_pGameRules->IsMultiplayer() )
	{
		CTakeDamageInfo info2 = info;
		info2.SetDamageForce( forceVector );
		Vector pos = info2.GetDamagePosition();
		if ( (pos.z - GetAbsMins().z) < 24 )
		{
			// HACKHACK: Make sure the vehicle impact is at least 2ft off the ground
			pos.z = GetAbsMins().z + 24;
			info2.SetDamagePosition( pos );
		}

		Vector soundOrigin = info.GetDamagePosition();
		CPASAttenuationFilter filter( soundOrigin );
		EmitSound( filter, SOUND_FROM_WORLD, CHAN_STATIC, "NPC_MetroPolice.HitByVehicle", 1, SNDLVL_NORM, 0, PITCH_NORM, &soundOrigin );
		// in single player create ragdolls on the server when the player hits someone
		// with their vehicle - for more dramatic death/collisions.
		CreateServerRagdoll( this, m_nForceBone, info2, COLLISION_GROUP_INTERACTIVE_DEBRIS );
		UTIL_Remove(this);
		return true;
	}
	else
	{
		return BecomeRagdollOnClient( forceVector );
	}

	return false;
}


/*
============
Killed
============
*/
void CBaseCombatCharacter::Event_Killed( const CTakeDamageInfo &info )
{
	extern ConVar npc_vphysics;

	// Advance life state to dying
	m_lifeState = LIFE_DYING;

	// Calculate death force
	Vector forceVector = CalcDamageForceVector( info );

	// See if there's a ragdoll magnet that should influence our force.
	CRagdollMagnet *pMagnet = CRagdollMagnet::FindBestMagnet( this );
	if( pMagnet )
	{
		forceVector += pMagnet->GetForceVector( this );
	}

	// Drop any weapon that I own
	if ( VPhysicsGetObject() )
	{
		Vector weaponForce = forceVector * VPhysicsGetObject()->GetInvMass();
		Weapon_Drop( m_hActiveWeapon, NULL, &weaponForce );
	}
	else
	{
		Weapon_Drop( m_hActiveWeapon );
	}
	
	// if flagged to drop a health kit
	if (HasSpawnFlags(SF_NPC_DROP_HEALTHKIT))
	{
		CBaseEntity::Create( "item_healthkit", GetAbsOrigin(), GetAbsAngles() );
	}
	// clear the deceased's sound channels.(may have been firing or reloading when killed)
	EmitSound( "BaseCombatCharacter.StopWeaponSounds" );

	// tell owner ( if any ) that we're dead.This is mostly for NPCMaker functionality.
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->DeathNotice( this );
	}

	// Ragdoll unless we've gibbed
	if ( ShouldGib( info ) == false )
	{
		// Tell my killer that he got me!
		if( info.GetAttacker() )
		{
			g_EventQueue.AddEvent( info.GetAttacker(), "KilledNPC", 0.3, this, this );
		}

		BecomeRagdoll( info, forceVector );
	}
}

void CBaseCombatCharacter::Event_Dying( void )
{
}


// ===========================================================================
//  > Weapons
// ===========================================================================
bool CBaseCombatCharacter::Weapon_Detach( CBaseCombatWeapon *pWeapon )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( pWeapon == m_hMyWeapons[i] )
		{
			m_hMyWeapons.Set( i, NULL );
			pWeapon->SetOwner( NULL );

			if ( pWeapon == m_hActiveWeapon )
				ClearActiveWeapon();
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Drop the active weapon, optionally throwing it at the given target position.
// Input  : pWeapon - Weapon to drop/throw.
//			pvecTarget - Position to throw it at, NULL for none.
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget /* = NULL */, const Vector *pVelocity /* = NULL */ )
{
	if ( pWeapon )
	{
		// If I'm an NPC, fill the weapon with ammo before I drop it.
		if ( GetFlags() & FL_NPC )
		{
			pWeapon->m_iClip1 = pWeapon->GetDefaultClip1();
			pWeapon->m_iClip2 = pWeapon->GetDefaultClip2();
		}

		if ( IsPlayer() )
		{
			Vector vThrowPos = Weapon_ShootPosition() - Vector(0,0,12);
			pWeapon->SetAbsOrigin( vThrowPos );

			QAngle gunAngles;
			VectorAngles( BodyDirection2D(), gunAngles );
			pWeapon->SetAbsAngles( gunAngles );
		}
		else
		{
			// If I have a hand, set the weapon position to my hand bone position.
			int iBIndex = LookupBone( "ValveBiped.Weapon_bone" );
			if ( iBIndex != -1)  
			{
				Vector origin;
				QAngle angles;
				GetBonePosition( iBIndex, origin, angles);
				//GetAttachment("gun", origin, angles);
				pWeapon->SetAbsOrigin( origin );
				pWeapon->SetAbsAngles( angles );
				UTIL_Relink(pWeapon);
			}
			// Otherwise just set in front of me.
			else 
			{
				Vector vFacingDir = BodyDirection2D();
				vFacingDir = vFacingDir * 10.0; 
				pWeapon->SetAbsOrigin( Weapon_ShootPosition() + vFacingDir );
			}
		}

		Vector vecThrow;
		if (pvecTarget)
		{
			// I've been told to throw it somewhere specific.
			vecThrow = VecCheckToss( this, pWeapon->GetAbsOrigin(), *pvecTarget, 0.2, 1.0, false );
		}
		else
		{
			if ( pVelocity )
			{
				vecThrow = *pVelocity;
			}
			else
			{
				// Nowhere in particular; just drop it.
				float throwForce = ( IsPlayer() ) ? 400.0f : random->RandomInt( 64, 128 );
				vecThrow = BodyDirection3D() * throwForce;
			}
		}

		pWeapon->Drop( vecThrow );
		Weapon_Detach( pWeapon );
	}
}


//-----------------------------------------------------------------------------
// Purpose:	Add new weapon to the character
// Input  : New weapon
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	// Add the weapon to my weapon inventory
	for (int i=0;i<MAX_WEAPONS;i++) 
	{
		if (!m_hMyWeapons[i]) 
		{
			m_hMyWeapons.Set( i, pWeapon );
			break;
		}
	}

	// Weapon is now on my team
	pWeapon->ChangeTeam( GetTeamNumber() );

	// Players don't automatically holster their current weapon
	if ( IsPlayer() == false )
	{
		if ( m_hActiveWeapon )
		{
			m_hActiveWeapon->Holster();
			m_hActiveWeapon->m_fEffects |= EF_NODRAW;
		}
		SetActiveWeapon( pWeapon );
		m_hActiveWeapon->m_fEffects &= ~EF_NODRAW;

		// If SF_NPC_LONG_RANGE spawn flags is set let weapon work from any distance
		if ( HasSpawnFlags(SF_NPC_LONG_RANGE) )
		{
			m_hActiveWeapon->m_fMaxRange1 = 999999999;
			m_hActiveWeapon->m_fMaxRange2 = 999999999;
		}
	}
	
	// ----------------------
	//  Give Primary Ammo
	// ----------------------
	// If gun doesn't use clips, just give ammo
	if (pWeapon->GetMaxClip1() == -1)
	{
		GiveAmmo(pWeapon->GetDefaultClip1(), pWeapon->m_iPrimaryAmmoType); 
	}
	// If default ammo given is greater than clip
	// size, fill clips and give extra ammo
	else if (pWeapon->GetDefaultClip1() >  pWeapon->GetMaxClip1() )
	{
		pWeapon->m_iClip1 = pWeapon->GetMaxClip1();
		GiveAmmo( (pWeapon->GetDefaultClip1() - pWeapon->GetMaxClip1()), pWeapon->m_iPrimaryAmmoType); 
	}

	// ----------------------
	//  Give Secondary Ammo
	// ----------------------
	// If gun doesn't use clips, just give ammo
	if (pWeapon->GetMaxClip2() == -1)
	{
		GiveAmmo(pWeapon->GetDefaultClip2(), pWeapon->m_iSecondaryAmmoType); 
	}
	// If default ammo given is greater than clip
	// size, fill clips and give extra ammo
	else if ( pWeapon->GetDefaultClip2() > pWeapon->GetMaxClip2() )
	{
		pWeapon->m_iClip2 = pWeapon->GetMaxClip2();
		GiveAmmo( (pWeapon->GetDefaultClip2() - pWeapon->GetMaxClip2()), pWeapon->m_iSecondaryAmmoType); 
	}

	pWeapon->Equip( this );

	if( GetActiveWeapon() == pWeapon )
	{
		int proficiency;
		proficiency = CalcWeaponProficiency( pWeapon );
		
		if( weapon_showproficiency.GetBool() != 0 )
		{
			Msg("%s equipped with %s, proficiency is %s\n", GetClassname(), pWeapon->GetClassname(), g_WeaponProficiencyTable[ proficiency ].pszName );
		}

		SetCurrentWeaponProficiency( proficiency );
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Leaves weapon, giving only ammo to the character
// Input  : Weapon
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_EquipAmmoOnly( CBaseCombatWeapon *pWeapon )
{
	// Check for duplicates
	for (int i=0;i<MAX_WEAPONS;i++) 
	{
		if ( m_hMyWeapons[i].Get() && FClassnameIs(m_hMyWeapons[i], pWeapon->GetClassname()) )
		{
			// Just give the ammo from the clip
			int	primaryGiven	= (pWeapon->UsesClipsForAmmo1()) ? pWeapon->m_iClip1 : pWeapon->GetDefaultClip1();
			int secondaryGiven	= (pWeapon->UsesClipsForAmmo2()) ? pWeapon->m_iClip2 : pWeapon->GetDefaultClip2();

			int takenPrimary   = GiveAmmo( primaryGiven, pWeapon->m_iPrimaryAmmoType); 
			int takenSecondary = GiveAmmo( secondaryGiven, pWeapon->m_iSecondaryAmmoType); 
			
			pWeapon->m_iClip1 -= takenPrimary;
			pWeapon->m_iClip2 -= takenSecondary;
			
			//Only succeed if we've taken ammo from the weapon
			if ( takenPrimary > 0 || takenSecondary > 0 )
				return true;
			
			return false;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the weapon passed in would occupy a slot already occupied by the carrier
// Input  : *pWeapon - weapon to test for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_SlotOccupied( CBaseCombatWeapon *pWeapon )
{
	if ( pWeapon == NULL )
		return false;

	//Check to see if there's a resident weapon already in this slot
	if ( Weapon_GetSlot( pWeapon->GetSlot() ) == NULL )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the weapon (if any) in the requested slot
// Input  : slot - which slot to poll
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseCombatCharacter::Weapon_GetSlot( int slot )
{
	int	targetSlot = slot;

	// Check for that slot being occupied already
	for ( int i=0; i < MAX_WEAPONS; i++ )
	{
		if ( m_hMyWeapons[i].Get() != NULL )
		{
			// If the slots match, it's already occupied
			if ( m_hMyWeapons[i]->GetSlot() == targetSlot )
				return m_hMyWeapons[i];
		}
	}
	
	return NULL;
}



//-----------------------------------------------------------------------------
// Purpose: Can this character operate this weapon?
// Input  : A weapon
// Output :	true or false
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	acttable_t *pTable		= pWeapon->ActivityList();
	int			actCount	= pWeapon->ActivityListCount();

	for ( int i = 0; i < actCount; i++, pTable++ )
	{
		if ( pTable->required )
		{
			// The NPC might translate the weapon activity into another activity
			Activity translatedActivity = NPC_TranslateActivity( (Activity)(pTable->weaponAct) );

			if ( SelectWeightedSequence(translatedActivity) == ACTIVITY_NOT_AVAILABLE )
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
CBaseCombatWeapon *CBaseCombatCharacter::Weapon_Create( const char *pWeaponName )
{
	CBaseCombatWeapon *pWeapon = static_cast<CBaseCombatWeapon *>( Create( pWeaponName, GetLocalOrigin(), GetLocalAngles(), this ) );

	return pWeapon;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_HandleAnimEvent( animevent_t *pEvent )
{
	// UNDONE: Some check to make sure that pEvent->pSource is a weapon I'm holding?
	if ( m_hActiveWeapon )
	{
		// UNDONE: Pass to pEvent->pSource instead?
		m_hActiveWeapon->Operator_HandleAnimEvent( pEvent, this );
	}
}

void CBaseCombatCharacter::RemoveAllWeapons()
{
	ClearActiveWeapon();
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if ( m_hMyWeapons[i] )
		{
			m_hMyWeapons[i]->Delete( );
			m_hMyWeapons.Set( i, NULL );
		}
	}
}


// take health
int CBaseCombatCharacter::TakeHealth (float flHealth, int bitsDamageType)
{
	if (!m_takedamage)
		return 0;
	
	return BaseClass::TakeHealth(flHealth, bitsDamageType);
}


/*
============
OnTakeDamage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
bitsDamageType indicates the type of damage sustained, ie: DMG_SHOCK

Time-based damage: only occurs while the NPC is within the trigger_hurt.
When a NPC is poisoned via an arrow etc it takes all the poison damage at once.



GLOBALS ASSUMED SET:  g_iSkillLevel
============
*/
int CBaseCombatCharacter::OnTakeDamage( const CTakeDamageInfo &info )
{
	int retVal = 0;

	if (!m_takedamage)
		return 0;

	switch( m_lifeState )
	{
	case LIFE_ALIVE:
		retVal = OnTakeDamage_Alive( info );
		if ( m_iHealth <= 0 )
		{
			// int nDeathHealth = m_iHealth;
			IPhysicsObject *pPhysics = VPhysicsGetObject();
			if ( pPhysics )
			{
				pPhysics->EnableCollisions( false );
			}
			
			bool bGibbed = false;

			Event_Killed( info );

			// Only classes that specifically request it are gibbed
			if ( ShouldGib( info ) )
			{
				bGibbed = Event_Gibbed( info );
				retVal = bGibbed;
			}
			
			if ( bGibbed == false )
			{
				Event_Dying();
			}
		}
		return retVal;
		break;

	case LIFE_DYING:
		return OnTakeDamage_Dying( info );
	
	default:
	case LIFE_DEAD:
		retVal = OnTakeDamage_Dead( info );
		if ( m_iHealth <= 0 && (info.GetDamageType() & DMG_GIB_CORPSE) )
		{
			Event_Gibbed( info );
			retVal = 0;
		}
		return retVal;
	}
}


int CBaseCombatCharacter::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	Vector vecDir = vec3_origin;
	if (info.GetInflictor())
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
		VectorNormalize(vecDir);
	}
	g_vecAttackDir = vecDir;

	//!!!LATER - make armor consideration here!
	// do the damage
	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		m_iHealth -= info.GetDamage();
	}

	return 1;
}


int CBaseCombatCharacter::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	return 1;
}

int CBaseCombatCharacter::OnTakeDamage_Dead( const CTakeDamageInfo &info )
{
	// do the damage
	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		m_iHealth -= info.GetDamage();
	}

	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: Catch fire! The flames will burn for the given lifetime (in seconds).
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Ignite( float flFlameLifetime )
{
	CEntityFlame *pFlame = CEntityFlame::Create( this );
	if (pFlame)
	{
		pFlame->SetLifetime( flFlameLifetime );
		m_bOnFire = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Initialize absmin & absmax to the appropriate box
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetObjectCollisionBox( void )
{
	// This is an optimization that takes into account that the bounding boxes
	// for combat chars take yaw into account, but never pitch or roll
	if ( !pev )
		return;

	if ( GetSolid() != SOLID_BBOX )
	{
		BaseClass::SetObjectCollisionBox();
		return;
	}
	SetAbsMins( GetAbsOrigin() + WorldAlignMins() - Vector(1,1,1) );
	SetAbsMaxs( GetAbsOrigin() + WorldAlignMaxs() + Vector(1,1,1) );
}

//-----------------------------------------------------------------------------
// Purpose: Sets vBodyDir to the body direction (2D) of the combat character.  
//			Used as NPC's and players extract facing direction differently
// Input  :
// Output :
//-----------------------------------------------------------------------------
QAngle CBaseCombatCharacter::BodyAngles()
{
	return GetAbsAngles();
}


Vector CBaseCombatCharacter::BodyDirection2D( void )
{
	Vector vBodyDir = BodyDirection3D( );
	vBodyDir.z = 0;
	VectorNormalize( vBodyDir );
	return vBodyDir;
}


Vector CBaseCombatCharacter::BodyDirection3D( void )
{
	QAngle angles = BodyAngles();

	// FIXME: cache this
	Vector vBodyDir;
	AngleVectors( angles, &vBodyDir );
	return vBodyDir;
}


void CBaseCombatCharacter::SetTransmit( CCheckTransmitInfo *pInfo )
{
	// Skip this work if we're already marked for transmission.
	if ( pInfo->WillTransmit( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo );

	bool bLocalPlayer = ( pInfo->m_pClientEnt == edict() );

	if ( bLocalPlayer )
	{
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			CBaseCombatWeapon *pWeapon = m_hMyWeapons[i];
			if ( !pWeapon )
				continue;

			// The local player is sent all of his weapons.
			pWeapon->SetTransmit( pInfo );
		}
	}
	else
	{
		if ( m_hActiveWeapon && !(m_hActiveWeapon->m_fEffects & EF_NODRAW) )
			m_hActiveWeapon->SetTransmit( pInfo );
	}
}



//-----------------------------------------------------------------------------
// Purpose: Add or Change a class relationship for this entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::AddClassRelationship ( Class_T class_type, Disposition_t disposition, int priority )
{
	// First check to see if a relationship has already been declared for this class
	// If so, update it with the new relationship
	for (int i=m_Relationship.Count()-1;i >= 0;i--) 
	{
		if (m_Relationship[i].classType == class_type) 
		{
			m_Relationship[i].disposition = disposition;
			if ( priority != DEF_RELATIONSHIP_PRIORITY )
				m_Relationship[i].priority	  = priority;
			return;
		}
	}

	int index = m_Relationship.AddToTail();
	// Add the new class relationship to our relationship table
	m_Relationship[index].classType		= class_type;
	m_Relationship[index].entity		= NULL;
	m_Relationship[index].disposition	= disposition;
	m_Relationship[index].priority		= ( priority != DEF_RELATIONSHIP_PRIORITY ) ? priority : 0;
}

//-----------------------------------------------------------------------------
// Purpose: Add or Change a entity relationship for this entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::AddEntityRelationship ( CBaseEntity* pEntity, Disposition_t disposition, int priority )
{
	// First check to see if a relationship has already been declared for this entity
	// If so, update it with the new relationship
	for (int i=m_Relationship.Count()-1;i >= 0;i--) 
	{
		if (m_Relationship[i].entity == pEntity) 
		{
			m_Relationship[i].disposition	= disposition;
			if ( priority != DEF_RELATIONSHIP_PRIORITY )
				m_Relationship[i].priority	= priority;
			return;
		}
	}

	int index = m_Relationship.AddToTail();
	// Add the new class relationship to our relationship table
	m_Relationship[index].classType		= CLASS_NONE;
	m_Relationship[index].entity		= pEntity;
	m_Relationship[index].disposition	= disposition;
	m_Relationship[index].priority		= ( priority != DEF_RELATIONSHIP_PRIORITY ) ? priority : 0;
}


//-----------------------------------------------------------------------------
// Allocates default relationships
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::AllocateDefaultRelationships( )
{
	if (!m_DefaultRelationship)
	{
		m_DefaultRelationship = new Relationship_t*[NUM_AI_CLASSES];

		for (int i=0; i<NUM_AI_CLASSES; ++i)
		{
			// Be default all relationships are neutral of priority zero
			m_DefaultRelationship[i] = new Relationship_t[NUM_AI_CLASSES];
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Return an interaction ID (so we have no collisions)
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetDefaultRelationship(Class_T nClass, Class_T nClassTarget, Disposition_t nDisposition, int nPriority)
{
	if (m_DefaultRelationship)
	{
		m_DefaultRelationship[nClass][nClassTarget].disposition	= nDisposition;
		m_DefaultRelationship[nClass][nClassTarget].priority	= nPriority;
	}
}

//-----------------------------------------------------------------------------
// Purpose: describes the relationship between two types of NPC.
// Input  :
// Output :
//-----------------------------------------------------------------------------
Relationship_t *CBaseCombatCharacter::FindEntityRelationship( CBaseEntity *pTarget )
{
	// First check for specific relationship with this edict
	int i;
	for (i=0;i<m_Relationship.Count();i++) 
	{
		if (pTarget == (CBaseEntity *)m_Relationship[i].entity) 
		{
			return &m_Relationship[i];
		}
	}

	if (pTarget->Classify() != CLASS_NONE)
	{
		// Then check for relationship with this edict's class
		for (i=0;i<m_Relationship.Count();i++) 
		{
			if (pTarget->Classify() == m_Relationship[i].classType) 
			{
				return &m_Relationship[i];
			}
		}
	}
	AllocateDefaultRelationships();
	// If none found return the default
	return &m_DefaultRelationship[ Classify() ][ pTarget->Classify() ];
}

Disposition_t CBaseCombatCharacter::IRelationType ( CBaseEntity *pTarget )
{
	return FindEntityRelationship( pTarget )->disposition;
}

//-----------------------------------------------------------------------------
// Purpose: describes the relationship between two types of NPC.
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::IRelationPriority( CBaseEntity *pTarget )
{
	return FindEntityRelationship( pTarget )->priority;
}

//-----------------------------------------------------------------------------
// Purpose: Get shoot position of BCC at current position/orientation
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CBaseCombatCharacter::Weapon_ShootPosition( )
{
	Vector forward, right, up;

	AngleVectors( GetAbsAngles(), &forward, &right, &up );

	Vector vecSrc = GetAbsOrigin() 
					+ forward * m_HackedGunPos.y 
					+ right * m_HackedGunPos.x 
					+ up * m_HackedGunPos.z;

	return vecSrc;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &range - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CBaseCombatCharacter::Weapon_FindUsable( const Vector &range )
{
	CBaseCombatWeapon *weaponList[64];
	CBaseCombatWeapon *pBestWeapon = NULL;

	Vector mins = GetAbsOrigin() - range;
	Vector maxs = GetAbsOrigin() + range;
	int listCount = CBaseCombatWeapon::GetAvailableWeaponsInBox( weaponList, ARRAYSIZE(weaponList), mins, maxs );

	float fBestDist = 1e6;

	for ( int i = 0; i < listCount; i++ )
	{
		// Make sure not moving (ie flying through the air)
		Vector velocity;

		CBaseCombatWeapon *pWeapon = weaponList[i];
		Assert(pWeapon);
		pWeapon->GetVelocity( &velocity, NULL );

		if ( velocity.LengthSqr() > 1 || !Weapon_CanUse(pWeapon) )
			continue;

		float fCurDist = (pWeapon->GetLocalOrigin() - GetLocalOrigin()).Length();

		if ( pBestWeapon )
		{
			// UNDONE: Better heuristic needed here
			//			Need to pick by power of weapons
			//			Don't want to pick a weapon right next to a NPC!

			// choose the last range attack weapon you find or the first available other weapon
			if ( ! (pWeapon->CapabilitiesGet() & bits_CAP_RANGE_ATTACK_GROUP) )
			{
				continue;
			}
			// Don't pick the weapon if its further away
			else if (fCurDist > fBestDist) 
			{
				continue;
			}
		}

		// Make sure this weapon is reachable by tracing out a human sized hull
		// just above the weapon.  If not reject
		trace_t tr;
		Vector	vAboveWeapon = pWeapon->GetAbsOrigin();
		Vector	vAboveWeapon2 = pWeapon->GetAbsOrigin();

		// Take vertical size of the hull into account;
		float	vertOffset = 0.5*(WorldAlignMaxs().z - WorldAlignMins().z);
		vAboveWeapon.z  += + vertOffset;
		vAboveWeapon2.z += + vertOffset + 1.0;

		UTIL_TraceEntity( this, vAboveWeapon, vAboveWeapon2, MASK_SOLID, pWeapon, COLLISION_GROUP_NONE, &tr );

		if ( tr.startsolid || (tr.fraction < 1.0) )
			continue;

		fBestDist   = fCurDist;
		pBestWeapon = pWeapon;
	}

	return pBestWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
// Input  : iCount - Amount of ammo to give.
//			iAmmoIndex - Index of the ammo into the AmmoInfoArray
//			iMax - Max carrying capability of the player
// Output : Amount of ammo actually given
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound)
{
	if (iCount <= 0)
		return 0;

	if ( !g_pGameRules->CanHaveAmmo( this, iAmmoIndex ) )
	{
		// game rules say I can't have any more of this ammo type.
		return 0;
	}

	if ( iAmmoIndex < 0 || iAmmoIndex >= MAX_AMMO_SLOTS )
		return 0;

	int iMax = GetAmmoDef()->MaxCarry(iAmmoIndex);
	int iAdd = min( iCount, iMax - m_iAmmo[iAmmoIndex] );
	if ( iAdd < 1 )
		return 0;

	// Ammo pickup sound
	if ( !bSuppressSound )
	{
		EmitSound( "BaseCombatCharacter.AmmoPickup" );
	}

	m_iAmmo.Set( iAmmoIndex, m_iAmmo[iAmmoIndex] + iAdd );

	return iAdd;
}

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::GiveAmmo( int iCount, const char *szName, bool bSuppressSound )
{
	int iAmmoType = GetAmmoDef()->Index(szName);
	if (iAmmoType == -1)
	{
		Msg("ERROR: Attempting to give unknown ammo type (%s)\n",szName);
		return 0;
	}
	return GiveAmmo( iCount, iAmmoType, bSuppressSound );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of the specified type the character's carrying
//-----------------------------------------------------------------------------
int	CBaseCombatCharacter::GetAmmoCount( char *szName ) const
{
	return GetAmmoCount( GetAmmoDef()->Index(szName) );
}


//-----------------------------------------------------------------------------
// Purpose: Switches to the best weapon that is also better than the given weapon.
// Input  : pCurrent - The current weapon used by the player.
// Output : Returns true if the weapon was switched, false if there was no better
//			weapon to switch to.
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::SwitchToNextBestWeapon(CBaseCombatWeapon *pCurrent)
{
	CBaseCombatWeapon *pNewWeapon = g_pGameRules->GetNextBestWeapon(this, pCurrent);
	
	if ( ( pNewWeapon != NULL ) && ( pNewWeapon != pCurrent ) )
	{
		return Weapon_Switch( pNewWeapon );
	}

	return false;
}


void CBaseCombatCharacter::VPhysicsGetShadowVelocity( IPhysicsObject *pPhysics, Vector &outVel )
{
	IPhysicsShadowController *pController = pPhysics->GetShadowController();
	pController->GetControlVelocity( outVel );
}

// how much to amplify impact forces
// This is to account for the ragdolls responding differently than
// the shadow objects.  Also this makes the impacts more dramatic.
ConVar	phys_impactforcescale( "phys_impactforcescale", "1.0" ); 
ConVar	phys_upimpactforcescale( "phys_upimpactforcescale", "0.5" ); 

void CBaseCombatCharacter::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];
	IPhysicsObject *pOtherPhysics = pEvent->pObjects[otherIndex];
	if ( pOther )
	{
		// Ragdolls are marked as dying.
		if ( pOther->m_lifeState == LIFE_DYING )
			return;

		if ( pOther->GetMoveType() == MOVETYPE_VPHYSICS && pOtherPhysics->IsMoveable() && pOther != GetGroundEntity() )
		{
			int damageType = 0;
			float damage = 0;

			if ( IsPlayer() )
			{
				damage = CalculatePhysicsImpactDamage( index, pEvent, gDefaultPlayerImpactDamageTable, m_impactEnergyScale, false, damageType );
			}
			else
			{
				damage = CalculatePhysicsImpactDamage( index, pEvent, gDefaultNPCImpactDamageTable, m_impactEnergyScale, false, damageType );
			}
			
			if ( damage > 0 )
			{
				// NOTE: We really need some rotational motion for some of these collisions.
				// REVISIT: Maybe resolve this collision on death with a different (not approximately infinite like AABB tensor)
				// inertia tensor to get torque?
				Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass() * phys_impactforcescale.GetFloat();
				
				IServerVehicle *vehicleOther = pOther->GetServerVehicle();
				if ( vehicleOther )
				{
					CBasePlayer *pPlayer = vehicleOther->GetPassenger();
					if ( pPlayer )
					{
						// flag as vehicle damage
						damageType |= DMG_VEHICLE;
						// if hit by vehicle driven by player, add some upward velocity to force
						float len = damageForce.Length();
						damageForce.z += len*phys_upimpactforcescale.GetFloat();
						//Msg("Force %.1f / %.1f\n", damageForce.Length(), damageForce.z );
					}
				}

				Vector damagePos;
				pEvent->pInternalData->GetContactPoint( damagePos );
				CTakeDamageInfo dmgInfo( pOther, pOther, damageForce, damagePos, damage, damageType );
				// UNDONE: Find one near damagePos?
				m_nForceBone = 0;
				PhysCallbackDamage( this, dmgInfo, *pEvent, index );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: this entity is exploding, or otherwise needs to inflict damage upon 
//			entities within a certain range.  only damage ents that can clearly 
//			be seen by the explosion!
// Input  :
// Output :
//-----------------------------------------------------------------------------	
void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore )
{
	// NOTE: I did this this way so I wouldn't have to change a whole bunch of
	// code unnecessarily. We need TF2 specific rules for RadiusDamage, so I moved
	// the implementation of radius damage into gamerules. All existing code calls
	// this method, which calls the game rules method
	g_pGameRules->RadiusDamage( info, vecSrc, flRadius, iClassIgnore );
}

//-----------------------------------------------------------------------------
// Purpose: Change active weapon and notify derived classes
//			
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetActiveWeapon( CBaseCombatWeapon *pNewWeapon )
{
	CBaseCombatWeapon *pOldWeapon = m_hActiveWeapon;
	if ( pNewWeapon != pOldWeapon )
	{
		m_hActiveWeapon = pNewWeapon;
		OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
	}
}

//-----------------------------------------------------------------------------
// Consider the weapon's built-in accuracy, this character's proficiency with
// the weapon, and the status of the target. Use this information to determine
// how accurately to shoot at the target.
//-----------------------------------------------------------------------------
Vector CBaseCombatCharacter::GetAttackAccuracy( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	float flModifier = g_WeaponProficiencyTable[ GetCurrentWeaponProficiency() ].value;
	return pWeapon->GetBulletSpread() * flModifier;
}

//-----------------------------------------------------------------------------
// Assume everyone is average with every weapon. Override this to make exceptions.
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	return WEAPON_PROFICIENCY_AVERAGE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define MAX_MISS_CANDIDATES 16
CBaseEntity *CBaseCombatCharacter::FindMissTarget( void )
{
	CBaseEntity *pMissCandidates[ MAX_MISS_CANDIDATES ];
	int numMissCandidates = 0;

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
	CBaseEntity *pEnts[256];
	Vector		radius( 100, 100, 100);
	Vector		vecSource = GetAbsOrigin();

	int numEnts = UTIL_EntitiesInBox( pEnts, 256, vecSource-radius, vecSource+radius, 0 );

	for ( int i = 0; i < numEnts; i++ )
	{
		if ( pEnts[i] == NULL )
			continue;

		// New rule for this system. Don't shoot what the player won't see.
		if ( pPlayer && !pPlayer->FInViewCone( pEnts[ i ] ) )
			continue;

		if ( numMissCandidates >= MAX_MISS_CANDIDATES )
			break;

		//See if it's a good target candidate
		if ( FClassnameIs( pEnts[i], "prop_dynamic" ) || 
			 FClassnameIs( pEnts[i], "prop_physics" ) || 
			 FClassnameIs( pEnts[i], "physics_prop" ) )
		{
			pMissCandidates[numMissCandidates++] = pEnts[i];
			continue;
		}
	}

	if( numMissCandidates == 0 )
		return NULL;

	return pMissCandidates[ random->RandomInt( 0, numMissCandidates - 1 ) ];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::ShouldShootMissTarget( CBaseCombatCharacter *pAttacker )
{
	// Don't shoot at NPC's right now.
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::InputKilledNPC( inputdata_t &inputdata )
{
	OnKilledNPC( inputdata.pActivator->MyCombatCharacterPointer() );
}

