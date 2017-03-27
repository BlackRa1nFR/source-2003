//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "movie_explosion.h"
#include "soundent.h"
#include "player.h"
#include "rope.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "explode.h"
#include "util.h"
#include "in_buttons.h"
#include "weapon_rpg.h"
#include "shake.h"
#include "te_effect_dispatch.h"
#include "triggers.h"

BEGIN_DATADESC( CMissile )

	DEFINE_FIELD( CMissile, m_hOwner,			FIELD_EHANDLE ),
	DEFINE_FIELD( CMissile, m_pRocketTrail,		FIELD_CLASSPTR ),
	DEFINE_FIELD( CMissile, m_vecAbsVelocity,	FIELD_VECTOR ),
	DEFINE_FIELD( CMissile, m_flAugerTime,		FIELD_FLOAT ),
	
	// Function Pointers
	DEFINE_FUNCTION( CMissile, StingerTouch ),
	DEFINE_FUNCTION( CMissile, AccelerateThink ),
	DEFINE_FUNCTION( CMissile, AugerThink ),
	DEFINE_FUNCTION( CMissile, IgniteThink ),
	DEFINE_FUNCTION( CMissile, SeekThink ),

END_DATADESC()
LINK_ENTITY_TO_CLASS( rpg_missile, CMissile );

class CWeaponRPG;

CMissile::CMissile()
{
	m_pRocketTrail = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CMissile::Precache( void )
{
	engine->PrecacheModel( "models/weapons/w_missile.mdl" );
	engine->PrecacheModel( "models/weapons/w_missile_closed.mdl" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CMissile::Spawn( void )
{
	Precache();

	SetSolid( SOLID_BBOX );
	SetModel("models/weapons/w_missile_closed.mdl");
	UTIL_SetSize( this, -Vector(4,4,4), Vector(4,4,4) );

	SetTouch( StingerTouch );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetThink( IgniteThink );
	
	SetNextThink( gpGlobals->curtime + 0.3f );

	m_takedamage = DAMAGE_YES;
	m_iHealth = m_iMaxHealth = 100;

	AddFlag( FL_OBJECT );
}


//---------------------------------------------------------
//---------------------------------------------------------
void CMissile::Event_Killed( const CTakeDamageInfo &info )
{
	m_takedamage = DAMAGE_NO;

	ShotDown();
}

unsigned int CMissile::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CMissile::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( ( info.GetDamageType() & DMG_MISSILEDEFENSE ) == false )
		return 0;

	bool fIsDamaged;
	int OnTakeDamageReturn;

	if( m_iHealth <= m_iMaxHealth - 20 )
	{
		// This missile isn't damaged enough to wobble in flight yet
		fIsDamaged = false;
	}
	else
	{
		// This missile is already damaged (i.e., already running AugerThink)
		fIsDamaged = true;
	}
	
	OnTakeDamageReturn = BaseClass::OnTakeDamage_Alive( info );

	if( !fIsDamaged  && m_iHealth <= m_iMaxHealth - 20 )
	{
		fIsDamaged = true;
		ShotDown();
	}

	return OnTakeDamageReturn;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CMissile::AccelerateThink( void )
{
	Vector vecForward;

	// !!!UNDONE - make this work exactly the same as HL1 RPG, lest we have looping sound bugs again!
	EmitSound( "Missile.Accelerate" );

	// m_fEffects = EF_LIGHT;

	AngleVectors( GetLocalAngles(), &vecForward );
	SetAbsVelocity( vecForward * 1500.0f );

	SetThink( SeekThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

#define AUGER_YDEVIANCE 20.0f
#define AUGER_XDEVIANCEUP 8.0f
#define AUGER_XDEVIANCEDOWN 1.0f

//---------------------------------------------------------
//---------------------------------------------------------
void CMissile::AugerThink( void )
{
	// If we've augered long enough, then just explode
	if ( m_flAugerTime < gpGlobals->curtime )
	{
		StingerExplode();
		return;
	}

	QAngle angles = GetLocalAngles();

	angles.y += random->RandomFloat( -AUGER_YDEVIANCE, AUGER_YDEVIANCE );
	angles.x += random->RandomFloat( -AUGER_XDEVIANCEDOWN, AUGER_XDEVIANCEUP );

	SetLocalAngles( angles );

	Vector vecForward;

	AngleVectors( GetLocalAngles(), &vecForward );
	
	SetAbsVelocity( vecForward * 1000.0f );

	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: Causes the missile to spiral to the ground and explode, due to damage
//-----------------------------------------------------------------------------
void CMissile::ShotDown( void )
{
	CEffectData	data;
	data.m_vOrigin = GetAbsOrigin();

	DispatchEffect( "RPGShotDown", data );

	if ( m_pRocketTrail != NULL )
	{
		m_pRocketTrail->m_bDamaged = true;
	}

	SetThink( AugerThink );
	SetNextThink( gpGlobals->curtime );
	m_flAugerTime = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMissile::StingerExplode( void )
{
	m_takedamage = DAMAGE_NO;

	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), GetOwnerEntity(), 200, 200, true );

	if( m_pRocketTrail )
	{
		m_pRocketTrail->SetLifetime(0.1f);
		m_pRocketTrail = NULL;
	}

	if ( m_hOwner != NULL )
	{
		m_hOwner->NotifyRocketDied();
	}

	StopSound( "Missile.Ignite" );
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CMissile::StingerTouch( CBaseEntity *pOther )
{
	Assert( pOther );
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		trace_t tr = GetTouchTrace();

		// Do "armor piercing" damage
		CTakeDamageInfo	info( this, GetOwnerEntity(), 200, (DMG_BLAST|DMG_ARMOR_PIERCING) );
		CalculateExplosiveDamageForce( &info, GetAbsVelocity(), tr.endpos );
		pOther->TakeDamage( info );
	}

	//TODO: Play a special effect on the target to denote that it pierced and did damage

	StingerExplode();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMissile::IgniteThink( void )
{
	SetMoveType( MOVETYPE_FLY );
	SetModel("models/weapons/w_missile.mdl");
	UTIL_SetSize( this, vec3_origin, vec3_origin );

	//TODO: Play opening sound

	Vector vecForward;

	EmitSound( "Missile.Ignite" );

	AngleVectors( GetLocalAngles(), &vecForward );
	SetAbsVelocity( vecForward * 1500.0f );

	SetThink( SeekThink );
	SetNextThink( gpGlobals->curtime );

	if ( m_hOwner && m_hOwner->GetOwner() )
	{
		CBasePlayer *pPlayer = ToBasePlayer( m_hOwner->GetOwner() );

		color32 white = { 255,225,205,64 };
		UTIL_ScreenFade( pPlayer, white, 0.1f, 0.0f, FFADE_IN );
	}

	// Smoke trail.
	if( m_pRocketTrail = RocketTrail::CreateRocketTrail())
	{
		m_pRocketTrail->m_Opacity = 0.2f;
		m_pRocketTrail->m_SpawnRate = 100;
		m_pRocketTrail->m_ParticleLifetime = 0.5f;
		m_pRocketTrail->m_StartColor.Init( 0.65f, 0.65f , 0.65f );
		m_pRocketTrail->m_EndColor.Init( 0, 0, 0 );
		m_pRocketTrail->m_StartSize = 8;
		m_pRocketTrail->m_EndSize = 32;
		m_pRocketTrail->m_SpawnRadius = 4;
		m_pRocketTrail->m_MinSpeed = 2;
		m_pRocketTrail->m_MaxSpeed = 16;
		
		m_pRocketTrail->SetLifetime( 30 );
		m_pRocketTrail->FollowEntity( ENTINDEX(pev), 1 );
	}
}

#define	RPG_HOMING_SPEED	0.125f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMissile::SeekThink( void )
{
	CBaseEntity	*pEnt		= NULL;
	CBaseEntity	*pBestDot	= NULL;
	float		flBestDist	= MAX_TRACE_LENGTH;
	float		dotDist;

	//Search for all dots relevant to us
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "env_laserdot" ) ) != NULL )
	{
		if ( pEnt->m_fEffects & EF_NODRAW )
			continue;

		dotDist = (GetAbsOrigin() - pEnt->GetAbsOrigin()).Length();

		//Find closest
		if ( dotDist < flBestDist )
		{
			pBestDot	= pEnt;
			flBestDist	= dotDist;
		}
	}

	//If we have a dot target
	if ( pBestDot != NULL )
	{
		CLaserDot *pLaserDot = (CLaserDot *)pBestDot;
		Vector	targetPos;
		
		CBaseEntity *pLaserTarget = pLaserDot->GetTargetEntity();

		if ( pLaserTarget != NULL )
		{
			targetPos = pLaserTarget->WorldSpaceCenter();
		}
		else
		{
			Vector	vLaserStart;

			if ( pLaserDot->GetOwnerEntity() != NULL )
			{
				//FIXME: Do we care this isn't exactly the muzzle position?
				vLaserStart = pLaserDot->GetOwnerEntity()->WorldSpaceCenter();
			}
			else
			{
				vLaserStart = pLaserDot->GetChasePosition();
			}

			//Get the laser's vector
			Vector vLaserDir = pLaserDot->GetChasePosition() - vLaserStart;
			
			//Find the length of the current laser
			float flLaserLength = VectorNormalize( vLaserDir );
			
			//Find the length from the missile to the laser's owner
			float flMissileLength = ( GetAbsOrigin() - vLaserStart ).Length();

			//Find the length from the missile to the laser's position
			float flTargetLength = ( GetAbsOrigin() - pLaserDot->GetChasePosition() ).Length();

			//See if we should chase the line segment nearest us
			if ( ( flMissileLength < flLaserLength ) || ( flTargetLength <= 512.0f ) )
			{
				targetPos = UTIL_PointOnLineNearestPoint( vLaserStart, pLaserDot->GetChasePosition(), GetAbsOrigin() );
				targetPos += ( vLaserDir * 256.0f );
			}
			else
			{
				//Otherwise chase the dot
				targetPos = pLaserDot->GetChasePosition();
			}

			//NDebugOverlay::Line( pLaserDot->GetChasePosition(), vLaserStart, 0, 255, 0, true, 0.05f );
			//NDebugOverlay::Line( GetAbsOrigin(), targetPos, 255, 0, 0, true, 0.05f );
			//NDebugOverlay::Cross3D( targetPos, -Vector(4,4,4), Vector(4,4,4), 255, 0, 0, true, 0.05f );
		}

		Vector	vTargetDir = targetPos - GetAbsOrigin();
		VectorNormalize( vTargetDir );

		Vector	vDir	= GetAbsVelocity();
		float	flSpeed	= VectorNormalize( vDir );
		float	flTimeToUse = gpGlobals->frametime;
		Vector	vNewVelocity;

		while ( flTimeToUse > 0.0f )
		{
			vNewVelocity = ( RPG_HOMING_SPEED * vTargetDir ) + ( ( 1 - RPG_HOMING_SPEED ) * vDir );
			flTimeToUse =- 0.1f;
		}

		VectorNormalize( vNewVelocity );

		QAngle	finalAngles;
		VectorAngles( vNewVelocity, finalAngles );
		SetAbsAngles( finalAngles );

		vNewVelocity *= flSpeed;
		SetAbsVelocity( vNewVelocity );
	}

	//Think as soon as possible
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : &vecOrigin - 
//			&vecAngles - 
//			NULL - 
//
// Output : CMissile
//-----------------------------------------------------------------------------
CMissile *CMissile::Create( const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner = NULL )
{
	CMissile *pMissile = (CMissile *)CreateEntityByName("rpg_missile" );
	UTIL_SetOrigin( pMissile, vecOrigin );
	Vector vecForward;
	AngleVectors( vecAngles, &vecForward );
	pMissile->SetLocalAngles( vecAngles );
	pMissile->Spawn();
	pMissile->SetOwnerEntity( Instance( pentOwner ) );
	pMissile->SetAbsVelocity( vecForward * 300 + Vector( 0,0, 128 ) );

	return pMissile;
}

//=============================================================================
// RPG
//=============================================================================

BEGIN_DATADESC( CWeaponRPG )

	DEFINE_FIELD( CWeaponRPG,	m_bGuiding,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CWeaponRPG,	m_hLaserDot,	FIELD_EHANDLE ),
	DEFINE_FIELD( CWeaponRPG,	m_hMissile,		FIELD_EHANDLE ),
	DEFINE_FIELD( CWeaponRPG,	m_bInitialStateUpdate, FIELD_BOOLEAN ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CWeaponRPG, DT_WeaponRPG)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_rpg, CWeaponRPG );
PRECACHE_WEAPON_REGISTER(weapon_rpg);

acttable_t	CWeaponRPG::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_ML, true },
};

IMPLEMENT_ACTTABLE(CWeaponRPG);

CWeaponRPG::CWeaponRPG()
{
	m_bReloadsSingly	= true;
	m_bGuiding			= true;
	m_bInitialStateUpdate= false;

	m_fMinRange1		= 65;
	m_fMinRange2		= 65;
	m_fMaxRange1		= 200;
	m_fMaxRange2		= 200;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponRPG::Spawn( void )
{
	BaseClass::Spawn();

	CreateLaserPointer();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponRPG::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "rpg_missile" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponRPG::HasAnyAmmo( void )
{
	if ( m_hMissile != NULL )
		return true;

	return BaseClass::HasAnyAmmo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponRPG::PrimaryAttack( void )
{
	// Can't have an active missile out
	if ( m_hMissile != NULL )
		return;

	// Can't be reloading
	if ( GetActivity() == ACT_VM_RELOAD )
		return;

	Vector vecOrigin;
	Vector vecForward;

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	Vector	vForward, vRight, vUp;

	pOwner->EyeVectors( &vForward, &vRight, &vUp );

	Vector	muzzlePoint = pOwner->Weapon_ShootPosition() + vForward * 12.0f + vRight * 6.0f + vUp * -3.0f;

	QAngle vecAngles;
	VectorAngles( vForward, vecAngles );
	m_hMissile = CMissile::Create( muzzlePoint, vecAngles, GetOwner()->edict() );

	m_hMissile->m_hOwner = this;

	DecrementAmmo( GetOwner() );

	// Register a muzzleflash for the AI
	pOwner->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	WeaponSound( SINGLE );

	// Check to see if we should trigger any RPG firing triggers
	int iCount = g_hWeaponFireTriggers.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		if ( g_hWeaponFireTriggers[i]->IsTouching( pOwner ) )
		{
			if ( FClassnameIs( g_hWeaponFireTriggers[i], "trigger_rpgfire" ) )
			{
				g_hWeaponFireTriggers[i]->ActivateMultiTrigger( pOwner );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponRPG::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	// Take away our primary ammo type
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	
	// Drop the weapon if it's spent and not guiding
	if ( ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 ) && ( m_hMissile == NULL ) )
	{
		pOwner->Weapon_Drop( this );
		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponRPG::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	//If we're pulling the weapon out for the first time, wait to draw the laser
	if ( ( m_bInitialStateUpdate ) && ( GetActivity() != ACT_VM_DRAW ) )
	{
		StartGuiding();
		m_bInitialStateUpdate = false;
	}

	//Player has toggled guidance state
	if ( pPlayer->m_afButtonPressed & IN_ATTACK2 )
	{
		ToggleGuiding();
	}

	//Move the laser
	UpdateLaserPosition();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CWeaponRPG::GetLaserPosition( void )
{
	CreateLaserPointer();

	if ( m_hLaserDot != NULL )
		return m_hLaserDot->GetAbsOrigin();

	//FIXME: The laser dot sprite is not active, this code should not be allowed!
	assert(0);
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true if the rocket is being guided, false if it's dumb
//-----------------------------------------------------------------------------
bool CWeaponRPG::IsGuiding( void )
{
	return m_bGuiding;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponRPG::Deploy( void )
{
	m_bInitialStateUpdate = true;
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponRPG::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	//Can't have an active missile out
	if ( m_hMissile != NULL )
		return false;

	StopGuiding();
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Turn on the guiding laser
//-----------------------------------------------------------------------------
void CWeaponRPG::StartGuiding( void )
{
	WeaponSound(SPECIAL1);

	m_bGuiding = true;

	if ( m_hLaserDot != NULL )
	{
		m_hLaserDot->TurnOn();
	}

	UpdateLaserPosition();
}

//-----------------------------------------------------------------------------
// Purpose: Turn off the guiding laser
//-----------------------------------------------------------------------------
void CWeaponRPG::StopGuiding( void )
{
	WeaponSound( SPECIAL2 );

	m_bGuiding = false;

	if ( m_hLaserDot != NULL )
	{
		m_hLaserDot->TurnOff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the guiding laser
//-----------------------------------------------------------------------------
void CWeaponRPG::ToggleGuiding( void )
{
	if ( IsGuiding() )
	{
		StopGuiding();
	}
	else
	{
		StartGuiding();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponRPG::Drop( const Vector &vecVelocity )
{
	StopGuiding();

	BaseClass::Drop( vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponRPG::UpdateLaserPosition( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;
	
	if ( pPlayer == NULL )
		return;

	//Move the laser dot, if active
	trace_t	tr;
	Vector	muzzlePos = pPlayer->Weapon_ShootPosition();
	
	Vector	forward;
	pPlayer->EyeVectors( &forward );

	Vector	endPos = muzzlePos + ( forward * MAX_TRACE_LENGTH );

	// Trace out for the endpoint
	UTIL_TraceLine( muzzlePos, endPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	// Move the laser sprite
	if ( m_hLaserDot != NULL )
	{
		Vector	laserPos = tr.endpos + ( tr.plane.normal * 2.0f );
		m_hLaserDot->SetLaserPosition( laserPos, tr.plane.normal );
		
		if ( tr.DidHitNonWorldEntity() )
		{
			CBaseEntity *pHit = tr.m_pEnt;

			if ( ( pHit != NULL ) && ( pHit->m_takedamage ) )
			{
				m_hLaserDot->SetTargetEntity( pHit );
			}
			else
			{
				m_hLaserDot->SetTargetEntity( NULL );
			}
		}
		else
		{
			m_hLaserDot->SetTargetEntity( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRPG::CreateLaserPointer( void )
{
	if ( m_hLaserDot != NULL )
		return;

	m_hLaserDot = CLaserDot::Create( GetAbsOrigin(), this );
	m_hLaserDot->TurnOff();

	UpdateLaserPosition();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponRPG::NotifyRocketDied( void )
{
	m_hMissile = NULL;

	Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponRPG::Reload( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return false;

	WeaponSound( RELOAD );
	
	SendWeaponAnim( ACT_VM_RELOAD );

	return true;
}

//=============================================================================
// Laser Dot
//=============================================================================

LINK_ENTITY_TO_CLASS( env_laserdot, CLaserDot );

BEGIN_DATADESC( CLaserDot )
	DEFINE_FIELD( CLaserDot, m_hLaserDot, FIELD_EHANDLE ),
	DEFINE_FIELD( CLaserDot, m_vecSurfaceNormal, FIELD_VECTOR ),
	DEFINE_FIELD( CLaserDot, m_hTargetEnt, FIELD_EHANDLE ),
	DEFINE_FUNCTION( CLaserDot, LaserThink ),
END_DATADESC()

CLaserDot::CLaserDot( void )
{
	m_hTargetEnt	= NULL;
	m_hLaserDot		= NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
// Output : CLaserDot
//-----------------------------------------------------------------------------
CLaserDot *CLaserDot::Create( const Vector &origin, CBaseEntity *pOwner )
{
	CLaserDot *pLaserDot = (CLaserDot *) CBaseEntity::Create( "env_laserdot", origin, QAngle(0,0,0) );

	if ( pLaserDot == NULL )
		return NULL;

	pLaserDot->SetMoveType( MOVETYPE_NONE );
	pLaserDot->AddSolidFlags( FSOLID_NOT_SOLID );

	//Create the graphic
	pLaserDot->m_hLaserDot = CSprite::SpriteCreate( "sprites/redglow1.vmt", origin, false );

	if ( pLaserDot->m_hLaserDot == NULL )
	{
		assert(0);
		return pLaserDot;
	}

	pLaserDot->m_hLaserDot->SetAttachment( pLaserDot, 0 );
	pLaserDot->m_hLaserDot->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
	pLaserDot->m_hLaserDot->SetScale( 0.5f );

	pLaserDot->SetOwnerEntity( pOwner );

	pLaserDot->SetThink( LaserThink );
	pLaserDot->SetNextThink( gpGlobals->curtime );
	pLaserDot->SetSimulatedEveryTick( true );

	return pLaserDot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLaserDot::LaserThink( void )
{
	SetNextThink( gpGlobals->curtime );

	if ( m_hLaserDot == NULL )
		return;

	if ( GetOwnerEntity() == NULL )
		return;

	Vector	viewDir = GetAbsOrigin() - GetOwnerEntity()->GetAbsOrigin();
	float	dist = VectorNormalize( viewDir );

	float	scale = RemapVal( dist, 32, 1024, 0.05f, 1.0f );

	scale = clamp( scale, 0.05f, 1.0f );

	m_hLaserDot->SetScale( scale, 0.1f );
}

void CLaserDot::SetLaserPosition( const Vector &origin, const Vector &normal )
{
	SetAbsOrigin( origin );
	m_vecSurfaceNormal = normal;
}

Vector CLaserDot::GetChasePosition()
{
	return GetAbsOrigin() - m_vecSurfaceNormal * 10;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLaserDot::TurnOn( void )
{
	if ( m_hLaserDot )
	{
		m_hLaserDot->TurnOn();
	}

	m_fEffects &= ~EF_NODRAW;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLaserDot::TurnOff( void )
{
	if ( m_hLaserDot )
	{
		m_hLaserDot->TurnOff();
	}

	m_fEffects |= EF_NODRAW;
}
