#include "cbase.h"
#if 0

// KEEPING this file around for reference for a while. 
// The helicopter is now derived from CBaseHelicopter.


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
#include "ndebugoverlay.h"
#include "globals.h"


// -------------------------------------
// Bone controllers
// -------------------------------------
#define CHOPPER_BC_GUN_YAW			0
#define CHOPPER_BC_GUN_PITCH		1
#define CHOPPER_BC_POD_PITCH		2

// -------------------------------------
// Attachment points
// -------------------------------------
#define CHOPPER_AP_GUN_TIP			1
#define CHOPPER_AP_GUN_BASE			2
#define CHOPPER_AP_RR_POD_TIP		3 
#define CHOPPER_AP_RL_POD_TIP		4 
#define CHOPPER_AP_LR_POD_TIP		5 
#define CHOPPER_AP_LL_POD_TIP		6 
#define CHOPPER_AP_RR_POD_BASE		7 
#define CHOPPER_AP_RL_POD_BASE		8 
#define CHOPPER_AP_LR_POD_BASE		9 
#define CHOPPER_AP_LL_POD_BASE		10 


#define CHOPPER_MAX_SPEED			400.0f
#define CHOPPER_MAX_FIRING_SPEED	250.0f
#define CHOPPER_MIN_ROCKET_DIST		1000.0f
#define CHOPPER_MAX_GUN_DIST		2000.0f

class CNPC_Helicopter : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Helicopter, CAI_BaseNPC );
public:

	DECLARE_DATADESC();

	void Spawn( void );
	void Precache( void );
	
	void Event_Killed( const CTakeDamageInfo &info );

	int  BloodColor( void ) { return DONT_BLEED; }
	void GibMonster( void );

	void SetObjectCollisionBox( void )
	{
		SetAbsMins( GetAbsOrigin() + Vector( -300, -300, -172) );
		SetAbsMaxs( GetAbsOrigin() + Vector(300, 300, 8) );
	}

	Class_T Classify ( void ) { return CLASS_COMBINE; }

	void HuntThink( void );
	void FlyTouch( CBaseEntity *pOther );
	void CrashTouch( CBaseEntity *pOther );
	void DyingThink( void );
	void StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void NullThink( void );

	void	ShowDamage( void );
	void	Flight( void );

	void	AimRocketGun(void);
	void	FireRocket(  Vector vLaunchPos, Vector vLaunchDir  );
	bool	FireGun( void );

	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	void	GunOn( CBaseEntity *pActivator, CBaseEntity *pCaller, string_t targetEntity );
	void	GunOff( CBaseEntity *pActivator, CBaseEntity *pCaller, string_t targetEntity );
	void	MissileOn( CBaseEntity *pActivator, CBaseEntity *pCaller, string_t targetEntity );
	void	MissileOff( CBaseEntity *pActivator, CBaseEntity *pCaller, string_t targetEntity );

	void	DrawDebugGeometryOverlays(void);
	
	~CNPC_Helicopter();

	int				m_iRockets;
	float			m_flForce;
	float			m_flNextRocket;
	int				m_fHelicopterFlags;

	Vector			m_vecTarget;
	Vector			m_posTarget;

	Vector			m_vecDesired;
	Vector			m_posDesired;

	Vector			m_vecGoal;

	Vector			m_angGun;
	Vector			m_angPod;

	float			m_flLastSeen;
	float			m_flPrevSeen;

	CSoundPatch*	m_pRotorSound;

	int				m_iSpriteTexture;
	int				m_iExplode;
	int				m_iBodyGibs;
	int				m_iAmmoType;

	float			m_flGoalSpeed;		// Goal speed
	float			m_flInitialSpeed;		// Instanteous initial speed

	int				m_iDoSmokePuff;

	COutputEvent	m_AtTarget;			// Fired when pathcorner has been reached
	COutputEvent	m_LeaveTarget;		// Fired when pathcorner is left

};
LINK_ENTITY_TO_CLASS( znpc_helicopter, CNPC_Helicopter );

BEGIN_DATADESC( CNPC_Helicopter )

	DEFINE_FUNCTION( CNPC_Helicopter, HuntThink ),
	DEFINE_FUNCTION( CNPC_Helicopter, FlyTouch ),

	DEFINE_FIELD( CNPC_Helicopter, m_iRockets,			FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Helicopter, m_flForce,			FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_Helicopter, m_flNextRocket,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_Helicopter, m_fHelicopterFlags,	FIELD_INTEGER),
	DEFINE_FIELD( CNPC_Helicopter, m_vecTarget,			FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Helicopter, m_posTarget,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CNPC_Helicopter, m_vecDesired,		FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Helicopter, m_posDesired,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CNPC_Helicopter, m_vecGoal,			FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Helicopter, m_angGun,			FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Helicopter, m_angPod,			FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Helicopter, m_flLastSeen,		FIELD_TIME ),
	DEFINE_FIELD( CNPC_Helicopter, m_flPrevSeen,		FIELD_TIME ),
	DEFINE_SOUNDPATCH( CNPC_Helicopter, m_pRotorSound ),
//	DEFINE_FIELD( CNPC_Helicopter, m_iSpriteTexture,	FIELD_INTEGER ),
//	DEFINE_FIELD( CNPC_Helicopter, m_iExplode,			FIELD_INTEGER ),
//	DEFINE_FIELD( CNPC_Helicopter, m_iBodyGibs,			FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Helicopter, m_flGoalSpeed,		FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_Helicopter, m_iDoSmokePuff,		FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Helicopter, m_iAmmoType,			FIELD_INTEGER ),

	DEFINE_KEYFIELD( CNPC_Helicopter, m_flInitialSpeed, FIELD_FLOAT, "InitialSpeed" ),

	// Inputs
	DEFINE_INPUTFUNC( CNPC_Helicopter,	FIELD_VOID,		"GunOn",		GunOn		),
	DEFINE_INPUTFUNC( CNPC_Helicopter,	FIELD_VOID,		"GunOff",		GunOff		),
	DEFINE_INPUTFUNC( CNPC_Helicopter,	FIELD_VOID,		"MissileOn",	MissileOn	),
	DEFINE_INPUTFUNC( CNPC_Helicopter,	FIELD_VOID,		"MissileOff",	MissileOff	),

	// Outputs
	DEFINE_OUTPUT(CNPC_Helicopter, m_AtTarget,			"AtPathCorner" ),
	DEFINE_OUTPUT(CNPC_Helicopter, m_LeaveTarget,		"LeavePathCorner" ),//<<TEMP>> Undone

	// Function pointers
	DEFINE_FUNCTION( CNPC_Helicopter, StartupUse ),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose : Inputs
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Helicopter::GunOn( CBaseEntity *pActivator, CBaseEntity *pCaller, string_t targetEntity )
{
	m_fHelicopterFlags |= BITS_HELICOPTER_GUN_ON;
}

void CNPC_Helicopter::GunOff( CBaseEntity *pActivator, CBaseEntity *pCaller, string_t targetEntity )
{
	m_fHelicopterFlags &= ~BITS_HELICOPTER_GUN_ON;
}

void CNPC_Helicopter::MissileOn( CBaseEntity *pActivator, CBaseEntity *pCaller, string_t targetEntity )
{
	m_fHelicopterFlags |= BITS_HELICOPTER_MISSILE_ON;
}

void CNPC_Helicopter::MissileOff( CBaseEntity *pActivator, CBaseEntity *pCaller, string_t targetEntity )
{
	m_fHelicopterFlags &= ~BITS_HELICOPTER_MISSILE_ON;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Helicopter::Spawn( void )
{
	Precache( );
	// motor
	SetMoveType( MOVETYPE_FLY );
	SetSolid( SOLID_BBOX );

	m_iAmmoType = g_pGameRules->GetAmmoDef()->Index("MediumRound"); 
	
	SetModel( "models/attack_helicopter.mdl" );

	UTIL_SetSize( this, Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	Relink();

	
	m_iHealth = 100;

	m_flFieldOfView = -0.707; // 270 degrees

	m_nSequence = 0;
	ResetSequenceInfo( );
	m_flCycle = 0;

	InitBoneControllers();

	if (m_spawnflags & SF_WAITFORTRIGGER)
	{
		SetUse( StartupUse );
	}
	else
	{
		SetThink( HuntThink );
		SetTouch( FlyTouch );
		SetNextThink( gpGlobals->curtime + 1.0 );
	}

	m_iRockets			= 10;
	m_fHelicopterFlags	= BITS_HELICOPTER_MISSILE_ON | BITS_HELICOPTER_GUN_ON;

	// Get the rotor sound started up.
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	m_takedamage = DAMAGE_AIM;

	m_pRotorSound	= controller.SoundCreate( edict(), CHAN_STATIC, "vehicles/chopper_rotor2.wav", 0.2 );
}


void CNPC_Helicopter::Precache( void )
{
	engine->PrecacheModel("models/attack_helicopter.mdl");
	engine->PrecacheModel("models/combine_soldier.mdl");

	engine->PrecacheSound("vehicles/chopper_rotor1.wav");
	engine->PrecacheSound("vehicles/chopper_rotor2.wav");
	engine->PrecacheSound("vehicles/chopper_rotor3.wav");
	engine->PrecacheSound("vehicles/chopper_whine1.wav");

	engine->PrecacheSound("weapons/mortarhit.wav");

	m_iSpriteTexture = engine->PrecacheModel( "sprites/white.vmt" );

	engine->PrecacheSound("weapons/ar2/ar2_fire2.wav");//<<TEMP>>temp sound

	engine->PrecacheModel("sprites/lgtning.vmt");

	// Missile
	engine->PrecacheSound("weapons/stinger_fire1.wav");
	engine->PrecacheModel("models/weapons/w_missile.mdl");

#if 0 // These don't port yet
	m_iExplode	= engine->PrecacheModel( "sprites/fexplo.vmt" );
	m_iBodyGibs = engine->PrecacheModel( "models/metalplategibs_green.mdl" );
	UTIL_PrecacheOther( "hvr_rocket" );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Override base class to add display of fly direction
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CNPC_Helicopter::DrawDebugGeometryOverlays(void) 
{
	if (m_pfnThink!= NULL)
	{
		// ------------------------------
		// Draw route if requested
		// ------------------------------
		if (m_debugOverlays & OVERLAY_NPC_ROUTE_BIT)
		{
			NDebugOverlay::Line(GetAbsOrigin(), m_posDesired, 0,0,255, true, 0);
		}
	}
	BaseClass::DrawDebugGeometryOverlays();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Helicopter::NullThink( void )
{
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.5 );
}


void CNPC_Helicopter::StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_flGoalSpeed = m_flInitialSpeed;

	SetThink( HuntThink );
	SetTouch( FlyTouch );
	SetNextThink( gpGlobals->curtime + 0.1f );
	SetUse( NULL );
}


void CNPC_Helicopter::Event_Killed( const CTakeDamageInfo &info )
{
	SetMoveType( MOVETYPE_FLYGRAVITY );
	m_flGravity = 0.3;

	UTIL_SetSize( this, Vector( -32, -32, -64), Vector( 32, 32, 0) );
	SetThink( DyingThink );
	SetTouch( CrashTouch );

	SetNextThink( gpGlobals->curtime + 0.1f );
	m_iHealth = 0;
	m_takedamage = DAMAGE_NO;

	if (m_spawnflags & SF_NOWRECKAGE)
	{
		m_flNextRocket = gpGlobals->curtime + 4.0;
	}
	else
	{
		m_flNextRocket = gpGlobals->curtime + 15.0;
	}

	CreateRagGib( "models/combine_soldier.mdl", GetAbsOrigin(), GetAbsAngles(), m_vecVelocity * 50 );
	
	m_OnDeath.FireOutput( pAttacker, this );
}


void CNPC_Helicopter::GibMonster( void )
{
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
#define CHOPPER_MISSILE_HOMING_STRENGTH		0.3
#define CHOPPER_MISSILE_HOMING_DELAY		0.5
#define CHOPPER_MISSILE_HOMING_RAMP_UP		0.5
#define CHOPPER_MISSILE_HOMING_DURATION		1.0
#define CHOPPER_MISSILE_HOMING_RAMP_DOWN	0.5
void CNPC_Helicopter::FireRocket( Vector vLaunchPos, Vector vLaunchDir )
{
	CGrenadeHomer *pGrenade = CGrenadeHomer::CreateGrenadeHomer( MAKE_STRING("models/weapons/w_missile.mdl"), MAKE_STRING("weapons/stinger_fire1.wav"), vLaunchPos, vLaunchDir, edict() );
	pGrenade->Spawn( );
	pGrenade->SetHoming(CHOPPER_MISSILE_HOMING_STRENGTH,
						CHOPPER_MISSILE_HOMING_DELAY,
						CHOPPER_MISSILE_HOMING_RAMP_UP,
						CHOPPER_MISSILE_HOMING_DURATION,
						CHOPPER_MISSILE_HOMING_RAMP_DOWN);
	pGrenade->Launch(this,m_hEnemy,m_vecVelocity+vLaunchDir*500,500,0.5,true);
	UTIL_EmitSoundDyn(pev, CHAN_WEAPON, "weapons/stinger_fire1.wav", 1.0, 0.5, 1,100);

	m_flNextRocket = gpGlobals->curtime + 4.0;

	g_pEffects->Smoke(vLaunchPos,random->RandomInt(10, 15), 10);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Helicopter::AimRocketGun( )
{
	Vector forward, right, up;
	AngleVectors( GetAngles(), &forward, &right, &up );
		
	Vector vBasePos, vBaseAng;
	Vector vTipPos, vTipAng;
	GetAttachment( CHOPPER_AP_LR_POD_BASE, vBasePos, vBaseAng );
	GetAttachment( CHOPPER_AP_LR_POD_TIP, vTipPos, vTipAng );
	
	// ----------------------------
	//  Aim Left
	// ----------------------------
	Vector vTargetDir;
	if (m_hEnemy != NULL)
	{
		vTargetDir = m_posTarget - vTipPos;
	}
	else
	{
		vTargetDir = forward;
	}
	VectorNormalize( vTargetDir );

	Vector vecOut;
	vecOut.x = DotProduct( forward, vTargetDir );
	vecOut.y = -DotProduct( right, vTargetDir );
	vecOut.z = DotProduct( up, vTargetDir );

	Vector angles;
	VectorAngles(vecOut, angles);

	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	if (angles.x > m_angPod.x)
		m_angPod.x = min( angles.x, m_angPod.x + 12 );
	if (angles.x < m_angPod.x)
		m_angPod.x = max( angles.x, m_angPod.x - 12 );
	if (angles.y > m_angPod.y)
		m_angPod.y = min( angles.y, m_angPod.y + 12 );
	if (angles.y < m_angPod.y)
		m_angPod.y = max( angles.y, m_angPod.y - 12 );

	m_angPod.x = SetBoneController( CHOPPER_BC_POD_PITCH,	m_angPod.x );

	// ------------------------
	// If no enemy we're done
	// ------------------------
	if (m_hEnemy == NULL)
	{
		return;
	}

	// --------------------------------
	//  Can I attack yet?
	// --------------------------------
	if (gpGlobals->curtime < m_flNextRocket)
	{
		return;
	}

	// -------------------------------
	// Don't fire when close to enemy
	// -------------------------------
	float flDist = (GetAbsOrigin() - m_hEnemy->GetAbsOrigin()).Length();
	if (flDist < CHOPPER_MIN_ROCKET_DIST)
	{
		return;
	}

	// -------------------------------
	//  Pick a pod to fire from
	//  Is left or right side closer?
	// -------------------------------
	Vector vMyFacing;
	BodyDirection2D(&vMyFacing);
	vMyFacing.z = 0;

	Vector	vEnemyDir   = m_hEnemy->EyePosition() - GetAbsOrigin();
	vEnemyDir.z = 0;

	Vector crossProduct;
	CrossProduct(vMyFacing, vEnemyDir, crossProduct);
	if (crossProduct.z < 0)
	{
		if (random->RandomInt(0,1)==0)
		{
			GetAttachment( CHOPPER_AP_RR_POD_BASE, vBasePos, vBaseAng );
			GetAttachment( CHOPPER_AP_RR_POD_TIP, vTipPos, vTipAng );
		}
		else
		{
			GetAttachment( CHOPPER_AP_RL_POD_BASE, vBasePos, vBaseAng );
			GetAttachment( CHOPPER_AP_RL_POD_TIP, vTipPos, vTipAng );
		}
	}
	else
	{
		if (random->RandomInt(0,1)==0)
		{
			GetAttachment( CHOPPER_AP_LR_POD_BASE, vBasePos, vBaseAng );
			GetAttachment( CHOPPER_AP_LR_POD_TIP, vTipPos, vTipAng );
		}
		else
		{
			GetAttachment( CHOPPER_AP_LL_POD_BASE, vBasePos, vBaseAng );
			GetAttachment( CHOPPER_AP_LL_POD_TIP, vTipPos, vTipAng );
		}
	}

	Vector vGunDir = vTipPos - vBasePos;
	VectorNormalize( vGunDir );
	
	float fDotPr = DotProduct( vGunDir, vTargetDir );

	if (fDotPr > 0.5)
	{
		FireRocket(vBasePos,vGunDir);
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_Helicopter::FireGun( )
{
	Vector forward, right, up;
	AngleVectors( GetAngles(), &forward, &right, &up );
		
	// Get gun attachment points
	Vector vBasePos, vBaseAng;
	GetAttachment( CHOPPER_AP_GUN_BASE, vBasePos, vBaseAng );
	Vector vTipPos, vTipAng;
	GetAttachment( CHOPPER_AP_GUN_TIP, vTipPos, vTipAng );

	Vector vTargetDir = m_posTarget - vBasePos;
	VectorNormalize( vTargetDir );

	Vector vecOut;
	vecOut.x = DotProduct( forward, vTargetDir );
	vecOut.y = -DotProduct( right, vTargetDir );
	vecOut.z = DotProduct( up, vTargetDir );

	Vector angles;
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


	// -----------------------------------------
	//  Don't fire if far away, rockets do that
	// -----------------------------------------
	if (m_hEnemy)
	{
		float flDist = (GetAbsOrigin() - m_hEnemy->GetAbsOrigin()).Length();
		if (flDist > CHOPPER_MAX_GUN_DIST)
		{
			return false;
		}
	}
	Vector vGunDir = vBasePos - vTipPos;
	VectorNormalize( vGunDir );
	
	float fDotPr = DotProduct( vGunDir, vTargetDir );
	if (fDotPr > 0.95)
	{
		UTIL_EmitSound(pev, CHAN_WEAPON, "weapons/ar2/ar2_fire2.wav", 1.0, 0.2);//<<TEMP>>temp sound
		FireBullets( 1, vBasePos, vGunDir, VECTOR_CONE_1DEGREES, 8192, m_iAmmoType, 1 );
		return true;
	}
	return false;
}



void CNPC_Helicopter::ShowDamage( void )
{
	if (m_iDoSmokePuff > 0 || random->RandomInt(0,99) > m_iHealth)
	{
		Vector vSmoke = GetAbsOrigin();
		vSmoke.z -= 32;
		UTIL_Smoke(vSmoke,random->RandomInt(10, 15), 10);
	}
	if (m_iDoSmokePuff > 0)
	{
		m_iDoSmokePuff--;
	}
}


int CNPC_Helicopter::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
#if 0
	// This code didn't port easily. WTF does it do? (sjb)
	if (pevInflictor->m_owner == pev)
		return 0;
#endif

	if (bitsDamageType & DMG_BLAST)
	{
		flDamage *= 2;
	}

	/*
	if ( (bitsDamageType & DMG_BULLET) && flDamage > 50)
	{
		// clip bullet damage at 50
		flDamage = 50;
	}
	*/

	return BaseClass::OnTakeDamage_Alive( info );
}



void CNPC_Helicopter::TraceAttack( edict_t *pevAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType)
{
	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );

#if 0 
	// HITGROUPS don't work currently.
	// ignore blades
	if (ptr->iHitgroup == 6 && (bitsDamageType & (DMG_ENERGYBEAM|DMG_BULLET|DMG_CLUB)))
		return;

	// hit hard, hits cockpit, hits engines
	if (flDamage > 50 || ptr->iHitgroup == 1 || ptr->iHitgroup == 2)
	{
		// ALERT( at_console, "%map .0f\n", flDamage );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		m_iDoSmokePuff = 3 + (flDamage / 5.0);
	}
	else
	{
		// do half damage in the body
		// AddMultiDamage( pevAttacker, this, flDamage / 2.0, bitsDamageType );
		UTIL_Ricochet( ptr->vecEndPos );
	}
#endif
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CNPC_Helicopter::~CNPC_Helicopter(void)
{
}


#endif // 0
