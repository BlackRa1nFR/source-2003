/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#include "cbase.h"
#include "AI_Network.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Node.h"
#include "AI_Task.h"
#include "entitylist.h"
#include "basecombatweapon.h"
#include "soundenvelope.h"
#include "gib.h"
#include "gamerules.h"
#include "ammodef.h"
#include "grenade_homer.h"
#include "CBaseHelicopter.h"
#include "engine/IEngineSound.h"
#include "ieffects.h"
#include "globals.h"


// -------------------------------------
// Bone controllers
// -------------------------------------
#define CHOPPER_BC_GUN_YAW			0
#define CHOPPER_BC_GUN_PITCH		1

// -------------------------------------
// Attachment points
// -------------------------------------
#define CHOPPER_AP_GUN_TIP			1
#define CHOPPER_AP_GUN_BASE			2

#define CHOPPER_MAX_SPEED			400.0f
#define CHOPPER_MAX_FIRING_SPEED	250.0f
#define CHOPPER_MAX_GUN_DIST		2000.0f

// -------------------------------------
// Pathing data
#define	CHOPPER_LEAD_DISTANCE			800.0f
#define	CHOPPER_MIN_CHASE_DIST_DIFF		128.0f	// Distance threshold used to determine when a target has moved enough to update our navigation to it
#define	CHOPPER_AVOID_DIST				256.0f

// CVars
ConVar	sk_helicopter_health( "sk_helicopter_health","100");

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_AttackHelicopter : public CBaseHelicopter
{
public:
	DECLARE_CLASS( CNPC_AttackHelicopter, CBaseHelicopter );
	DECLARE_DATADESC();

	~CNPC_AttackHelicopter();

	void	Spawn( void );
	void	Precache( void );
	int		BloodColor( void ) { return DONT_BLEED; }
	Class_T Classify ( void ) { return CLASS_COMBINE_GUNSHIP; }

	void	InitializeRotorSound( void );
	void	DoRotorWash( void );

	// Weaponry
	bool	FireGun( void );

	// Damage handling
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void	Event_Killed( const CTakeDamageInfo &info );

private:
	Vector			m_angGun;
	int				m_iAmmoType;
	float			m_flLastCorpseFall;
};

LINK_ENTITY_TO_CLASS( npc_helicopter, CNPC_AttackHelicopter );

BEGIN_DATADESC( CNPC_AttackHelicopter )

	DEFINE_ENTITYFUNC( CNPC_AttackHelicopter, FlyTouch ),

	DEFINE_FIELD( CNPC_AttackHelicopter, m_flForce,				FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_AttackHelicopter, m_fHelicopterFlags,	FIELD_INTEGER),
	DEFINE_FIELD( CNPC_AttackHelicopter, m_vecTarget,			FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_AttackHelicopter, m_angGun,				FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_AttackHelicopter, m_flLastSeen,			FIELD_TIME ),
	DEFINE_FIELD( CNPC_AttackHelicopter, m_flPrevSeen,			FIELD_TIME ),
	DEFINE_FIELD( CNPC_AttackHelicopter, m_flGoalSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_AttackHelicopter, m_iAmmoType,			FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_AttackHelicopter, m_flLastCorpseFall,	FIELD_TIME ),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::Spawn( void )
{
	Precache( );

	SetModel( "models/attack_helicopter.mdl" );
	ExtractBbox( SelectHeaviestSequence( ACT_IDLE ), m_cullBoxMins, m_cullBoxMaxs ); 
	BaseClass::Spawn();

	InitPathingData( CHOPPER_LEAD_DISTANCE, CHOPPER_MIN_CHASE_DIST_DIFF, CHOPPER_AVOID_DIST );

	m_takedamage = DAMAGE_YES;

	SetHullType(HULL_LARGE_CENTERED);
	SetHullSizeNormal();

	m_iMaxHealth = m_iHealth = sk_helicopter_health.GetInt();

	m_flFieldOfView = -0.707; // 270 degrees

	m_iAmmoType = GetAmmoDef()->Index("MediumRound"); 

	InitBoneControllers();

	m_fHelicopterFlags = BITS_HELICOPTER_GUN_ON;
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
CNPC_AttackHelicopter::~CNPC_AttackHelicopter(void)
{
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::Precache( void )
{
	engine->PrecacheModel("models/attack_helicopter.mdl");
	engine->PrecacheModel("models/combine_soldier.mdl");

	BaseClass::Precache();
}

//------------------------------------------------------------------------------
// Purpose : Create our rotor sound
//------------------------------------------------------------------------------
void CNPC_AttackHelicopter::InitializeRotorSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CPASAttenuationFilter filter( this );
	m_pRotorSound = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.Rotors" );
	m_pRotorBlast = controller.SoundCreate( filter, entindex(), "NPC_AttackHelicopter.RotorBlast" );

	BaseClass::InitializeRotorSound();
}

//-----------------------------------------------------------------------------
// Purpose: Play a different sound when I'm over water
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::DoRotorWash( void )
{
	BaseClass::DoRotorWash();
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
bool CNPC_AttackHelicopter::FireGun( void )
{
	// HACK: CBaseHelicopter ignores this, and fire forever at the last place it saw the player. Why?
	if ( !((m_flLastSeen + 1 > gpGlobals->curtime) && (m_flPrevSeen + 2 < gpGlobals->curtime)) )
		return false;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );
		
	// Get gun attachment points
	Vector vBasePos;
	QAngle vBaseAng;
	GetAttachment( CHOPPER_AP_GUN_BASE, vBasePos, vBaseAng );
	Vector vTipPos;
	QAngle vTipAng;
	GetAttachment( CHOPPER_AP_GUN_TIP, vTipPos, vTipAng );

	Vector vTargetDir = m_vecTargetPosition - vBasePos;
	VectorNormalize( vTargetDir );

	Vector vecOut;
	vecOut.x = DotProduct( forward, vTargetDir );
	vecOut.y = -DotProduct( right, vTargetDir );
	vecOut.z = DotProduct( up, vTargetDir );

	QAngle angles;
	VectorAngles(vecOut, angles);

	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	if (angles.x > m_angGun.x)
		m_angGun.x = min( angles.x, m_angGun.x + 12 );
	if (angles.x < m_angGun.x)
		m_angGun.x = max( angles.x, m_angGun.x - 12 );
	if (angles.y > m_angGun.y)
		m_angGun.y = min( angles.y, m_angGun.y + 12 );
	if (angles.y < m_angGun.y)
		m_angGun.y = max( angles.y, m_angGun.y - 12 );

	m_angGun.y = SetBoneController( CHOPPER_BC_GUN_YAW,		m_angGun.y );
	m_angGun.x = SetBoneController( CHOPPER_BC_GUN_PITCH,	m_angGun.x );

	// FIXME: hack to make attack chopper shoot like a guy with a pistol
	if (m_flNextAttack > gpGlobals->curtime)
	{
		return false;
	}

	//  Don't fire if we're too far away
	if (GetEnemy())
	{
		float flDist = (GetAbsOrigin() - GetEnemy()->GetAbsOrigin()).Length();
		if (flDist > CHOPPER_MAX_GUN_DIST)
			return false;
	}
	Vector vGunDir = vBasePos - vTipPos;
	VectorNormalize( vGunDir );
	
	float fDotPr = DotProduct( vGunDir, vTargetDir );
	if (fDotPr > 0.95)
	{
		// FIXME: hack to make attack chopper shoot like a guy with a pistol
		m_flNextAttack = gpGlobals->curtime + random->RandomFloat(0.3,0.8);
		EmitSound( "NPC_AttackHelicopter.FireGun" );
		
		FireBullets( 1, vBasePos, vGunDir, VECTOR_CONE_1DEGREES, 8192, m_iAmmoType, 1 );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Take damage from trace attacks if they hit the gunner
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	//Msg( "%d %.0f\n", ptr->hitgroup, info.GetDamage() );

	// Only hits to the gunner do damage
	if ( ptr->hitgroup == 1 )
	{
		AddMultiDamage( info, this );
	}
	else
	{
		g_pEffects->Ricochet( ptr->endpos, ptr->plane.normal );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Come back to life!
//-----------------------------------------------------------------------------
void CNPC_AttackHelicopter::Event_Killed( const CTakeDamageInfo &info )
{
	// Don't drop another corpse if the next guy's not out on the gun yet
	if ( m_flLastCorpseFall > gpGlobals->curtime )
	{
		m_iHealth = 1;
		return;
	}

	m_flLastCorpseFall = gpGlobals->curtime + 3.0;

	// Revert back to full health
	m_iHealth = m_iMaxHealth;

	// Spawn a ragdoll combine guard
	float forceScale = info.GetDamage() * 75 * 4;
	Vector vecForceVector = RandomVector(-1,1);
	vecForceVector.z = 0.5;
	vecForceVector *= forceScale;

	CBaseEntity *pGib = CreateRagGib( "models/combine_soldier.mdl", GetAbsOrigin(), GetAbsAngles(), vecForceVector );
	if ( pGib )
	{
		pGib->SetOwnerEntity( this );
	}

	// Start the "new guy taking over the gunner spot" animation
}
