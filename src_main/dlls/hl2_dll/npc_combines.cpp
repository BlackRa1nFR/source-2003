//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: This is the soldier version of the combine, analogous to the HL1 grunt.
//
//=============================================================================

#include "cbase.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "npc_talker.h"
#include "npc_CombineS.h"
#include "bitstring.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "npcevent.h"
#include "hl2_dll/hl2_player.h"
#include "game.h"
#include "ammodef.h"
#include "explode.h"
#include "ai_memory.h"
#include "sprite.h"
#include "soundenvelope.h"

ConVar	sk_combine_s_health( "sk_combine_s_health","0");
ConVar	sk_combine_s_kick( "sk_combine_s_kick","0");

//Whether or not the combine should spawn health on death
ConVar	combine_spawn_health( "combine_spawn_health", "1" );

LINK_ENTITY_TO_CLASS( npc_combine_s, CNPC_CombineS );


#define AE_SOLDIER_BLOCK_PHYSICS		20 // trying to block an incoming physics object

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_CombineS::Spawn( void )
{
	Precache();
	SetModel( "models/Combine_Soldier.mdl" );
	m_iHealth = sk_combine_s_health.GetFloat();
	m_nKickDamage = sk_combine_s_kick.GetFloat();

	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK2 );
	CapabilitiesAdd( bits_CAP_MOVE_SHOOT );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_CombineS::Precache()
{
	engine->PrecacheModel("models/combine_soldier.mdl");

	UTIL_PrecacheOther( "item_healthvial" );
	UTIL_PrecacheOther( "weapon_frag" );

	BaseClass::Precache();
}


void CNPC_CombineS::DeathSound( void )
{
	EmitSound( "NPC_CombineS.Die" );
}


//-----------------------------------------------------------------------------
// Purpose: maintains blending states of head, eyes, body, etc. 
//			FIXME: move this to CNPC_Combine?
//			FIXME: this needs to be moved/renamed once normal blended animation control gets added
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_CombineS::MaintainLookTargets ( float flInterval )
{
	// maintain anim vectors
	Vector vecShootOrigin;

	vecShootOrigin = Weapon_ShootPosition();
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

	SetAim( vecShootDir );

	BaseClass::MaintainLookTargets( flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: Soldiers use CAN_RANGE_ATTACK2 to indicate whether they can throw
//			a grenade. Because they check only every half-second or so, this
//			condition must persist until it is updated again by the code
//			that determines whether a grenade can be thrown, so prevent the 
//			base class from clearing it out. (sjb)
//-----------------------------------------------------------------------------
void CNPC_CombineS::ClearAttackConditions( )
{
	bool fCanRangeAttack2 = HasCondition( COND_CAN_RANGE_ATTACK2 );

	// Call the base class.
	BaseClass::ClearAttackConditions();

	if( fCanRangeAttack2 )
	{
		// We don't allow the base class to clear this condition because we
		// don't sense for it every frame.
		SetCondition( COND_CAN_RANGE_ATTACK2 );
	}
}

void CNPC_CombineS::PrescheduleThink( void )
{
	/*//FIXME: This doesn't need to be in here, it's all debug info
	if( HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		// Don't react unless we see the item!!
		CSound *pSound = NULL;

		pSound = GetLoudestSoundOfType( SOUND_PHYSICS_DANGER );

		if( pSound )
		{
			if( FInViewCone( pSound->GetSoundReactOrigin() ) )
			{
				Msg( "OH CRAP!\n" );
				NDebugOverlay::Line( EyePosition(), pSound->GetSoundReactOrigin(), 0, 0, 255, false, 2.0f );
			}
		}
	}
	*/

	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_CombineS::BuildScheduleTestBits( void )
{
	//Interrupt any schedule with physics danger (as long as I'm not moving or already trying to block)
	if ( m_flGroundSpeed == 0.0 && !IsCurSchedule( SCHED_FLINCH_PHYSICS ) )
	{
		SetCustomInterruptCondition( COND_HEAR_PHYSICS_DANGER );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_CombineS::SelectSchedule ( void )
{
	if( HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		return SCHED_FLINCH_PHYSICS;
	}

	return BaseClass::SelectSchedule();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_CombineS::TakeDamage( const CTakeDamageInfo &info )
{
	if( info.GetInflictor() && info.GetInflictor()->VPhysicsGetObject() )
	{
		// Hit by a physics object! Was I blocking?
		if( m_fIsBlocking )
		{
			IPhysicsObject *pPhysObject;

			pPhysObject = info.GetInflictor()->VPhysicsGetObject();

			if( pPhysObject )
			{
				// Only deflect objects of relatively low mass
				//Msg( "MASS: %f\n", pPhysObject->GetMass() );

				if( pPhysObject->GetMass() <= 30.0 )
				{
					// No damage from light objects (tuned for melons)
					return 0;
				}
			}
		}
	}

	BaseClass::TakeDamage( info );
	return 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int	CNPC_CombineS::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	RestartGesture( ACT_GESTURE_SMALL_FLINCH );
	return BaseClass::OnTakeDamage_Alive( info );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CNPC_CombineS::FInAimCone( const Vector &vecSpot )
{
	Vector los = ( vecSpot - GetAbsOrigin() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = BodyDirection2D();

	float flDot = DotProduct( los, facingDir );

	if ( flDot > 0.866 ) //30 degrees
		return true;

	return false;
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombineS::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case AE_SOLDIER_BLOCK_PHYSICS:
		Msg( "BLOCKING!\n" );
		m_fIsBlocking = true;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

void CNPC_CombineS::OnChangeActivity( Activity eNewActivity )
{
	// Any new sequence stops us blocking.
	m_fIsBlocking = false;
	BaseClass::OnChangeActivity( eNewActivity );
}

void CNPC_CombineS::OnListened()
{
	BaseClass::OnListened();

	if ( HasCondition( COND_HEAR_DANGER ) && HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		if ( HasInterruptCondition( COND_HEAR_DANGER ) )
		{
			ClearCondition( COND_HEAR_PHYSICS_DANGER );
		}
	}

	// debugging to find missed schedules
#if 0
	if ( HasCondition( COND_HEAR_DANGER ) && !HasInterruptCondition( COND_HEAR_DANGER ) )
	{
		Msg("Ignore danger in %s\n", GetCurSchedule()->GetName() );
	}
#endif
}

#define MAX_SHOT_DISTANCE	2048

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CNPC_CombineS::Event_Killed( const CTakeDamageInfo &info )
{
	if ( combine_spawn_health.GetBool() == false )
	{
		BaseClass::Event_Killed( info );
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( info.GetAttacker() );

	if ( pPlayer != NULL )
	{
		//Try to throw dynamic health
		float healthPerc = ( (float) pPlayer->m_iHealth / (float) pPlayer->m_iMaxHealth );

		if ( random->RandomFloat( 0.0f, 1.0f ) > healthPerc*1.5f )
		{
			CBaseEntity *pItem = DropItem( "item_healthvial", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );

			if ( pItem )
			{
				IPhysicsObject *pObj = pItem->VPhysicsGetObject();

				if ( pObj )
				{
					Vector			vel		= RandomVector( -64.0f, 64.0f );
					AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

					vel[2] = 0.0f;
					pObj->AddVelocity( &vel, &angImp );
				}
			}
		}

		//Try to throw dynamic grenades
		int grenadeIndex = GetAmmoDef()->Index( "grenade" );
		int numGrenades = pPlayer->GetAmmoCount( grenadeIndex );

		if ( ( numGrenades < GetAmmoDef()->MaxCarry( grenadeIndex ) ) && ( random->RandomInt( 0, 4 ) == 0 ) )
		{
			CBaseEntity *pItem = DropItem( "weapon_frag", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );

			if ( pItem )
			{
				IPhysicsObject *pObj = pItem->VPhysicsGetObject();

				if ( pObj )
				{
					Vector			vel		= RandomVector( -64.0f, 64.0f );
					AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

					vel[2] = 0.0f;
					pObj->AddVelocity( &vel, &angImp );
				}
			}
		}
	}

	BaseClass::Event_Killed( info );
}


//---------------------------------------------------------
//---------------------------------------------------------
class CNPC_BounceBomb : public CBaseAnimating
{
	DECLARE_CLASS( CNPC_BounceBomb, CBaseAnimating );

public:
	CNPC_BounceBomb() { m_flExplosionRadius = -1; m_pWarnSound = NULL; }
	void Precache();
	void Spawn();
	void SearchThink();
	void ExplodeThink();
	bool IsAwake() { return m_bAwake; }
	void Wake( bool bWake );
	float FindNearestBCC();
	void SetNearestBCC( CBaseCombatCharacter *pNearest ) { m_hNearestBCC.Set( pNearest ); }
	int OnTakeDamage( const CTakeDamageInfo &info );

	bool CreateVPhysics()
	{
		VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
		return true;
	}


	DECLARE_DATADESC();

private:
	float	m_flWarnRadius;
	float	m_flDetonateRadius;

	float		m_flExplosionRadius;
	float		m_flExplosionDamage;
	float		m_flExplosionDelay;

	bool	m_bAwake;
	EHANDLE	m_hNearestBCC;
	EHANDLE m_hSprite;

	CSoundPatch	*m_pWarnSound;
};

BEGIN_DATADESC( CNPC_BounceBomb )
	DEFINE_THINKFUNC( CNPC_BounceBomb, ExplodeThink ),
	DEFINE_THINKFUNC( CNPC_BounceBomb, SearchThink ),

	DEFINE_SOUNDPATCH( CNPC_BounceBomb, m_pWarnSound ),

	DEFINE_KEYFIELD( CNPC_BounceBomb, m_flWarnRadius,		FIELD_FLOAT, "WarnRadius" ),
	DEFINE_KEYFIELD( CNPC_BounceBomb, m_flDetonateRadius,	FIELD_FLOAT, "DetonateRadius" ),
	DEFINE_KEYFIELD( CNPC_BounceBomb, m_flExplosionRadius,	FIELD_FLOAT, "ExplosionRadius" ),
	DEFINE_KEYFIELD( CNPC_BounceBomb, m_flExplosionDamage,	FIELD_FLOAT, "ExplosionDamage" ),
	DEFINE_KEYFIELD( CNPC_BounceBomb, m_flExplosionDelay,	FIELD_FLOAT, "ExplosionDelay" ),
	DEFINE_KEYFIELD( CNPC_BounceBomb, m_iHealth,			FIELD_INTEGER, "Health" ),

	DEFINE_FIELD( CNPC_BounceBomb, m_bAwake, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_BounceBomb, m_hNearestBCC, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_BounceBomb, m_hSprite, FIELD_EHANDLE ),
END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BounceBomb::Precache()
{
	engine->PrecacheModel("models/props_c17/pipe03_connector01.mdl");
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BounceBomb::Spawn()
{
	Precache();

	Wake( false );

	SetModel("models/props_c17/pipe03_connector01.mdl");

	CreateVPhysics();

	m_hSprite.Set( NULL );

	m_takedamage = DAMAGE_YES;

	SetThink( SearchThink );
	SetNextThink( gpGlobals->curtime );
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BounceBomb::OnTakeDamage( const CTakeDamageInfo &info )
{
	if( m_takedamage == DAMAGE_NO )
	{
		return false;
	}

	if( info.GetAttacker()->m_iClassname == m_iClassname )
	{
		// Other mines don't blow me up.
		return false;
	}

	m_iHealth -= info.GetDamage();

	if( m_iHealth < 1 )
	{
		ExplodeThink();
	}

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BounceBomb::Wake( bool bAwake )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CEntityMessageFilter filter( this, "CNPC_BounceBomb" );
	filter.MakeReliable();

	if( !m_pWarnSound )
	{
		m_pWarnSound = controller.SoundCreate( filter, entindex(), "Weapon_CombineMine.Ping" );
		controller.Play( m_pWarnSound, 1.0, 0 );
	}

	if( bAwake )
	{
		// Turning on
		EmitSound( "NPC_RollerMine.Tossed" );

		m_hSprite = CSprite::SpriteCreate( "glow01.spr", GetAbsOrigin(), false );

		if( m_hSprite )
		{
			m_hSprite->SetParent( this );		

			CSprite *pSprite = (CSprite *)m_hSprite.Get();

			pSprite->SetTransparency( kRenderTransAdd, 255, 0, 0, 180, kRenderFxNone );
			pSprite->SetScale( 0.25, 0.0 );

			controller.SoundChangeVolume( m_pWarnSound, 1.0, 0.1 );
		}
	}
	else
	{
		// Turning off
		EmitSound( "Weapon_CombineMine.Pong" );
		SetNearestBCC( NULL );

		if( m_hSprite )
		{
			UTIL_Remove( m_hSprite );
			m_hSprite.Set( NULL );
		}

		controller.SoundChangeVolume( m_pWarnSound, 0.0, 0.1 );
	}

	m_bAwake = bAwake;
}

//---------------------------------------------------------
// Returns distance to the nearest BaseCombatCharacter.
//---------------------------------------------------------
#define BOUNCEBOMB_SEARCH_DEPTH	30
float CNPC_BounceBomb::FindNearestBCC()
{
	float		flNearest = FLT_MAX;
	CBaseEntity *pEnts[ BOUNCEBOMB_SEARCH_DEPTH ];

	// Assume this search won't find anyone.
	SetNearestBCC( NULL );
	
	int count = UTIL_EntitiesInSphere( pEnts, BOUNCEBOMB_SEARCH_DEPTH, GetAbsOrigin(), m_flWarnRadius, 0 );

	if( count >= BOUNCEBOMB_SEARCH_DEPTH )
	{
		Msg("**WARNING!!** BounceBomb searching more than %d Ents!\n", BOUNCEBOMB_SEARCH_DEPTH );
	}

	for( int i = 0 ; i < count ; i++ )
	{
		CBaseCombatCharacter *pBCC = NULL;

		pBCC = pEnts[ i ]->MyCombatCharacterPointer();

		if( pBCC && pBCC->IsAlive() )
		{
			float flDist = (GetAbsOrigin() - pBCC->GetAbsOrigin()).Length();

			if( flDist < flNearest && pBCC->Classify() != CLASS_METROPOLICE && pBCC->Classify() != CLASS_COMBINE )
			{
				// Now do a visibility test.
				if( FVisible( pBCC, MASK_SOLID_BRUSHONLY ) )
				{
					flNearest = flDist;
					SetNearestBCC( pBCC );
				}
			}
		}
	}

	return flNearest;
}

//---------------------------------------------------------
// Try to pop up such that I explode when the enemy is very
// near me, assuming they maintain current speed/direction.
// Only works on players right now.
//---------------------------------------------------------
void CNPC_BounceBomb::SearchThink()
{
	SetNextThink( gpGlobals->curtime + 0.1 );

	float flNearestBCCDist = FindNearestBCC();

	if( flNearestBCCDist <= m_flWarnRadius )
	{
		if( !IsAwake() )
		{
			Wake( true );
		}
	}
	else
	{
 		if( IsAwake() )
		{
			Wake( false );
		}

		return;
	}

	// If m_flExplosionRadius is -1, we're in bounce bomb mode.
	// otherwise, we're in proximity mine mode.
	if( m_flExplosionRadius == -1 )
	{
		if( !m_hNearestBCC.Get() )
		{
			Msg("**Warning - BounceBomb lost its target!\n");
			Wake( false );
		}

		Vector vecProjected = m_hNearestBCC->GetAbsOrigin();

		vecProjected += m_hNearestBCC->GetSmoothedVelocity() * 1.0;

		float flDot;

		Vector vecToEnemy = m_hNearestBCC->GetAbsOrigin() - GetAbsOrigin();
		VectorNormalize( vecToEnemy );
		vecToEnemy.z = 0.0;

		Vector vecToProjected = vecProjected - GetAbsOrigin();
		VectorNormalize( vecToProjected );
		vecToProjected.z = 0.0;

		//NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + vecToEnemy * 128, 255, 0, 0, false, 0.1f );
		//NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + vecToProjected * 128, 0, 255, 0, false, 0.1f );

		flDot = DotProduct( vecToProjected, vecToEnemy );

		//Msg( "Dist:%f  Dot:%f\n", flNearestBCCDist, flDot );

		if( flNearestBCCDist <= 100 || flDot < 0.0 )
		{
			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
			if ( pPhysicsObject != NULL )
			{
				pPhysicsObject->Wake();
				pPhysicsObject->ApplyForceCenter( Vector( 0, 0, 25000 ) );
				pPhysicsObject->ApplyTorqueCenter( AngularImpulse( 100, 100, 100 ) );

				// Scare NPC's
				CSoundEnt::InsertSound( SOUND_DANGER, GetAbsOrigin(), 300, 0.3f, this );
			}

			EmitSound( "NPC_RollerMine.Hurt" );
			
			SetThink( ExplodeThink );
			SetNextThink( gpGlobals->curtime + m_flExplosionDelay );
			return;
		}
	}
	else
	{
		// Don't pop up in the air, just explode if the NPC gets closer than explode radius.
		if( flNearestBCCDist <= m_flDetonateRadius )
		{
			EmitSound( "NPC_RollerMine.Hurt" );
			SetThink( ExplodeThink );
			SetNextThink( gpGlobals->curtime + m_flExplosionDelay );
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BounceBomb::ExplodeThink()
{
	// Don't catch self in own explosion!
	m_takedamage = DAMAGE_NO;

	if( m_hSprite )
	{
		UTIL_Remove( m_hSprite );
		m_hSprite.Set( NULL );
	}

	if( m_pWarnSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundDestroy( m_pWarnSound );
	}

	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), this, m_flExplosionDamage, m_flExplosionRadius, true );
	UTIL_Remove( this );
}

LINK_ENTITY_TO_CLASS( bounce_bomb, CNPC_BounceBomb );
LINK_ENTITY_TO_CLASS( combine_bouncemine, CNPC_BounceBomb );
LINK_ENTITY_TO_CLASS( combine_mine, CNPC_BounceBomb );
