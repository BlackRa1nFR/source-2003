//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Pistol - hand gun
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "NPCEvent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "explode.h"
#include "shake.h"
#include "model_types.h"
#include "studio.h"

#define	PISTOL_FASTEST_REFIRE_TIME		0.1f
#define	PISTOL_FASTEST_DRY_REFIRE_TIME	0.2f

static void LaunchStickyBomb( CBaseCombatCharacter *pOwner, const Vector &origin, const Vector &direction );

//-----------------------------------------------------------------------------
// CWeaponStickyLauncher
//-----------------------------------------------------------------------------

class CWeaponStickyLauncher : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponStickyLauncher, CBaseHLCombatWeapon );
public:

	CWeaponStickyLauncher(void);

	DECLARE_SERVERCLASS();

	void	ItemPostFrame();
	void	PrimaryAttack();
	void	SecondaryAttack();
	void	AddViewKick();
	void	DryFire();
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}
	
	virtual float GetFireRate( void ) { return 1.0f; }

	DECLARE_ACTTABLE();


};

IMPLEMENT_SERVERCLASS_ST(CWeaponStickyLauncher, DT_WeaponStickyLauncher)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_stickylauncher, CWeaponStickyLauncher );
PRECACHE_WEAPON_REGISTER( weapon_stickylauncher );

acttable_t	CWeaponStickyLauncher::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_PISTOL, true },
};

IMPLEMENT_ACTTABLE( CWeaponStickyLauncher );

class CStickyBomb : public CBaseAnimating
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CStickyBomb, CBaseAnimating );
public:
	CStickyBomb();
	~CStickyBomb();
	void Precache();
	void Spawn();
	void SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	void Detonate();
	void Think();
	void NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params );
	void Touch( CBaseEntity *pOther );
	unsigned int PhysicsSolidMaskForEntity( void ) const { return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX; }

	static void DetonateByOperator( CBaseEntity *pOperator );
	void SetBombOrigin();

	DECLARE_SERVERCLASS();

private:
	static CUtlVector<CStickyBomb *> m_stickyList;

	EHANDLE	m_hOperator;
	CNetworkVar( int, m_boneIndexAttached );
	CNetworkVector( m_bonePosition );
	CNetworkQAngle( m_boneAngles );
};

LINK_ENTITY_TO_CLASS( grenade_stickybomb, CStickyBomb );

IMPLEMENT_SERVERCLASS_ST(CStickyBomb, DT_StickyBomb)
	SendPropInt( SENDINFO( m_boneIndexAttached ), MAXSTUDIOBONEBITS, SPROP_UNSIGNED ),
	SendPropVector(SENDINFO(m_bonePosition), -1,  SPROP_COORD ),
	SendPropAngle(SENDINFO_VECTORELEM(m_boneAngles, 0), 13 ),
	SendPropAngle(SENDINFO_VECTORELEM(m_boneAngles, 1), 13 ),
	SendPropAngle(SENDINFO_VECTORELEM(m_boneAngles, 2), 13 ),
END_SEND_TABLE()


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CStickyBomb )

	DEFINE_FIELD( CStickyBomb, m_hOperator,			FIELD_EHANDLE ),
	DEFINE_FIELD( CStickyBomb, m_boneIndexAttached,	FIELD_INTEGER ),
	DEFINE_FIELD( CStickyBomb, m_bonePosition,		FIELD_VECTOR ),
	DEFINE_FIELD( CStickyBomb, m_boneAngles,		FIELD_VECTOR ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponStickyLauncher::CWeaponStickyLauncher( void )
{
	m_fMinRange1 = 65;
	m_fMinRange2 = 65;
	m_fMaxRange1 = 512;
	m_fMaxRange2 = 512;

	m_bFiresUnderwater = true;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponStickyLauncher::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition();

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			
			if (npc)
			{
				vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
			}
			else
			{
				AngleVectors( pOperator->GetLocalAngles(), &vecShootDir );
			}

			WeaponSound( SINGLE_NPC );
			LaunchStickyBomb( pOperator, vecShootOrigin, vecShootDir );
			pOperator->m_fEffects |= EF_MUZZLEFLASH;
			m_iClip1 = m_iClip1 - 1;
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

void CWeaponStickyLauncher::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponStickyLauncher::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	WeaponSound(SINGLE);
	pPlayer->m_fEffects |= EF_MUZZLEFLASH;

	SendWeaponAnim( GetPrimaryAttackActivity() );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = pPlayer->Weapon_ShootPosition( );
	Vector vecAiming = pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	m_iClip1 = m_iClip1 - 1;

	LaunchStickyBomb( pPlayer, vecSrc, vecAiming );
	AddViewKick();
}

void CWeaponStickyLauncher::SecondaryAttack()
{
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0;
	CStickyBomb::DetonateByOperator( GetOwner() );
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponStickyLauncher::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	//Allow a refire as fast as the player can click
	if ( !( pOwner->m_nButtons & IN_ATTACK ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
	else if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
	{
		DryFire();
		return;
	}

	BaseClass::ItemPostFrame();
}

/*
==================================================
AddViewKick
==================================================
*/

void CWeaponStickyLauncher::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle viewPunch;

	viewPunch.x = random->RandomFloat( -2.0f, -1.0f );
	viewPunch.y = random->RandomFloat( 0.5f, 1.0f );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}



#define STICKY_BOMB_MODEL	"models/Weapons/glueblob.mdl"

// global list for fast searching
CUtlVector<CStickyBomb *> CStickyBomb::m_stickyList;

//-----------------------------------------------------------------------------
// Purpose: Sticky bomb class
//-----------------------------------------------------------------------------
CStickyBomb::CStickyBomb()
{
	m_stickyList.AddToTail(this);
}

CStickyBomb::~CStickyBomb()
{
	int index = m_stickyList.Find(this);
	if ( m_stickyList.IsValidIndex(index) )
	{
		m_stickyList.FastRemove(index);
	}
}

void CStickyBomb::Precache()
{
	engine->PrecacheModel( STICKY_BOMB_MODEL );
	BaseClass::Precache();
	Assert( (1<<MAXSTUDIOBONEBITS) >= MAXSTUDIOBONES );
}

void CStickyBomb::Spawn()
{
	m_hOperator = GetOwnerEntity();
	Precache();
	SetModel( STICKY_BOMB_MODEL );
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	m_takedamage = DAMAGE_NO;
	SetNextThink( TICK_NEVER_THINK );
	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	m_flCycle = 0;
	UTIL_SetSize( this, vec3_origin, vec3_origin );
	Relink();
	// no shadows, please.
	m_fEffects |= EF_NOSHADOW|EF_NORECEIVESHADOW;
	SetRenderColor( 255, 0, 0 );
}
void CStickyBomb::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	SetAbsVelocity( velocity );
}

void CStickyBomb::DetonateByOperator( CBaseEntity *pOperator )
{
	float delay = 0;
	for ( int i = m_stickyList.Count()-1; i >= 0; --i )
	{
		if ( m_stickyList[i]->m_hOperator == pOperator || m_stickyList[i]->m_hOperator.Get() == NULL )
		{
			if ( m_stickyList[i]->GetNextThink() < gpGlobals->curtime )
			{
				m_stickyList[i]->SetNextThink( gpGlobals->curtime + delay );
				delay += 0.1;
			}
		}
	}
}

void CStickyBomb::SetBombOrigin()
{
	if ( IsFollowingEntity() )
	{
		CBaseAnimating *pOtherAnim = dynamic_cast<CBaseAnimating *>(GetFollowedEntity());
		if ( pOtherAnim )
		{
			Vector origin;
			matrix3x4_t boneToWorld;
			pOtherAnim->GetBoneTransform( m_boneIndexAttached, boneToWorld );
			VectorTransform( m_bonePosition, boneToWorld, origin );
			StopFollowingEntity( );
			SetMoveType(MOVETYPE_NONE);
			SetAbsOrigin( origin );
		}
	}
}


void CStickyBomb::Detonate()
{
	SetBombOrigin();
	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), m_hOperator, 100, 250, true );
	UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 250, SHAKE_START );
	UTIL_Remove( this );
}

void CStickyBomb::Think()
{
	Detonate();
}

void CStickyBomb::NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
	// UNDONE: Do this for client ragdolls too?
	// UNDONE: Add notify event die or client ragdoll?
	if ( eventType == NOTIFY_EVENT_DESTROY )
	{
		if ( pNotify == GetParent() )
		{
			SetBombOrigin();
			SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
		}
	}
}

void CStickyBomb::Touch( CBaseEntity *pOther )
{
	// Don't stick if already stuck
	if ( GetMoveType() == MOVETYPE_FLYGRAVITY )
	{
		trace_t tr = GetTouchTrace();
		// stickies don't stick to each other or sky
		if ( FClassnameIs(pOther, "grenade_stickybomb") || (tr.surface.flags & SURF_SKY) )
		{
			// bounce
			Vector vecNewVelocity;
			PhysicsClipVelocity( GetAbsVelocity(), tr.plane.normal, vecNewVelocity, 1.0 );
			SetAbsVelocity( vecNewVelocity );
		}
		else 
		{
			SetAbsVelocity( vec3_origin );
			SetMoveType( MOVETYPE_NONE );
			if ( pOther->entindex() != 0 )
			{
				// set up notification if the parent is deleted before we explode
				g_pNotify->AddEntity( this, pOther );

				if ( (tr.surface.flags & SURF_HITBOX) && modelinfo->GetModelType( pOther->GetModel() ) == mod_studio )
				{
					CBaseAnimating *pOtherAnim = dynamic_cast<CBaseAnimating *>(pOther);
					if ( pOtherAnim )
					{
						matrix3x4_t bombWorldSpace;
						MatrixCopy( EntityToWorldTransform(), bombWorldSpace );

						// get the bone info so we can follow the bone
						FollowEntity( pOther );
						SetOwnerEntity( pOther );
						m_boneIndexAttached = pOtherAnim->GetHitboxBone( tr.hitbox );
						matrix3x4_t boneToWorld;
						pOtherAnim->GetBoneTransform( m_boneIndexAttached, boneToWorld );

						// transform my current position/orientation into the hit bone's space
						// UNDONE: Eventually we need to intersect with the mesh here
						// REVISIT: maybe do something like the decal code to find a spot on
						//			the mesh.
						matrix3x4_t worldToBone, localMatrix;
						MatrixInvert( boneToWorld, worldToBone );
						ConcatTransforms( worldToBone, bombWorldSpace, localMatrix );
						MatrixAngles( localMatrix, m_boneAngles.GetForModify(), m_bonePosition.GetForModify() );
						return;
					}
				}
				SetParent( pOther );
			}
		}
	}
}

void LaunchStickyBomb( CBaseCombatCharacter *pOwner, const Vector &origin, const Vector &direction )
{
	CStickyBomb *pGrenade = (CStickyBomb *)CBaseEntity::Create( "grenade_stickybomb", origin, vec3_angle, pOwner );
	pGrenade->SetVelocity( direction * 1200, vec3_origin );
}
