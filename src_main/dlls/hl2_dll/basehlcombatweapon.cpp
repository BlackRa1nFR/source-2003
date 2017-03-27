//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "soundent.h"
#include "ai_basenpc.h"
#include "game.h"

IMPLEMENT_SERVERCLASS_ST( CHLMachineGun, DT_HLMachineGun )
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHLMachineGun::CHLMachineGun( void )
{
}

const Vector &CHLMachineGun::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_3DEGREES;
	return cone;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHLMachineGun::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;
	
	// Abort here to handle burst and auto fire modes
	if ( (UsesClipsForAmmo1() && m_iClip1 == 0) || ( !UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType) ) )
		return;

	// MUST call sound before removing a round from the clip of a CHLMachineGun
	WeaponSound(SINGLE);

	pPlayer->m_fEffects |= EF_MUZZLEFLASH;

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	if (GetSequence() != SelectWeightedSequence( ACT_VM_PRIMARYATTACK ))
	{
		m_flNextPrimaryAttack = gpGlobals->curtime;
	}
	int iBulletsToFire = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		iBulletsToFire++;
	}

	// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
	if ( UsesClipsForAmmo1() )
	{
		if ( iBulletsToFire > m_iClip1 )
			iBulletsToFire = m_iClip1;
		m_iClip1 -= iBulletsToFire;
	}

	// Fire the bullets
	Vector vecSrc	 = pPlayer->Weapon_ShootPosition( );
	Vector vecAiming = pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	FireBullets( iBulletsToFire, vecSrc, vecAiming, pPlayer->GetAttackAccuracy( this ), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );

	//Factor in the view kick
	AddViewKick();

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 400, 0.2, pPlayer );
	
	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Register a muzzleflash for the AI
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
}

void CHLMachineGun::FireBullets(int cShots, const Vector &vecSrc, const Vector &vecDirShooting, const Vector &vecSpread, float flDistance, int iBulletType, int iTracerFreq)
{
	if(CBasePlayer *pPlayer = ToBasePlayer ( GetOwner() ) )
	{
		pPlayer->FireBullets(cShots, vecSrc, vecDirShooting, vecSpread, flDistance, iBulletType, iTracerFreq);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Weapon firing conditions
//-----------------------------------------------------------------------------
int CHLMachineGun::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	if ( m_iClip1 <=0 )
	{
		return COND_NO_PRIMARY_AMMO;
	}
	else if ( flDist < m_fMinRange1 ) 
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if ( flDist > m_fMaxRange1 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if ( flDot < 0.5f )	// UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLMachineGun::DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime )
{
	#define	KICK_MIN_X			0.2f	//Degrees
	#define	KICK_MIN_Y			0.2f	//Degrees
	#define	KICK_MIN_Z			0.1f	//Degrees

	QAngle vecScratch;
	
	//Find how far into our accuracy degradation we are
	float duration	= ( fireDurationTime > slideLimitTime ) ? slideLimitTime : fireDurationTime;
	float kickPerc = duration / slideLimitTime;

	// do this to get a hard discontinuity, clear out anything under 10 degrees punch
	pPlayer->ViewPunchReset( 10 );

	//Apply this to the view angles as well
	vecScratch.x = -( KICK_MIN_X + ( maxVerticleKickAngle * kickPerc ) );
	vecScratch.y = -( KICK_MIN_Y + ( maxVerticleKickAngle * kickPerc ) ) / 3;
	vecScratch.z = KICK_MIN_Z + ( maxVerticleKickAngle * kickPerc ) / 8;

	//Wibble left and right
	if ( random->RandomInt( -1, 1 ) >= 0 )
		vecScratch.y *= -1;

	//Wobble up and down
	if ( random->RandomInt( -1, 1 ) >= 0 )
		vecScratch.z *= -1;

	//If we're in easy, dampen the effect a bit
	if ( g_iSkillLevel == SKILL_EASY )
	{
		for ( int i = 0; i < 3; i++ )
		{
			vecScratch[i] *= dampEasy;
		}
	}

	//Clip this to our desired min/max
	UTIL_ClipPunchAngleOffset( vecScratch, pPlayer->m_Local.m_vecPunchAngle, QAngle( 24.0f, 3.0f, 1.0f ) );

	//Add it to the view punch
	// NOTE: 0.5 is just tuned to match the old effect before the punch became simulated
	pPlayer->ViewPunch( vecScratch * 0.5 );
}

IMPLEMENT_SERVERCLASS_ST( CHLSelectFireMachineGun, DT_HLSelectFireMachineGun )
END_SEND_TABLE()

//=========================================================
//	>> CHLSelectFireMachineGun
//=========================================================
BEGIN_DATADESC( CHLSelectFireMachineGun )

	DEFINE_FIELD( CHLSelectFireMachineGun, m_iBurstSize, FIELD_INTEGER ),
	DEFINE_FIELD( CHLSelectFireMachineGun, m_iFireMode, FIELD_INTEGER ),
	// Function pinters
	DEFINE_FUNCTION( CHLSelectFireMachineGun, BurstThink ),

END_DATADESC()



float CHLSelectFireMachineGun::GetBurstCycleRate( void )
{
	// this is the time it takes to fire an entire 
	// burst, plus whatever amount of delay we want
	// to have between bursts.
	return 0.5f;
}

float CHLSelectFireMachineGun::GetFireRate( void )
{
	switch( m_iFireMode )
	{
	case FIREMODE_FULLAUTO:
		// the time between rounds fired on full auto
		return 0.1f;	// 600 rounds per minute = 0.1 seconds per bullet
		break;

	case FIREMODE_3RNDBURST:
		// the time between rounds fired within a single burst
		return 0.1f;	// 600 rounds per minute = 0.1 seconds per bullet
		break;

	default:
		return 0.1f;
		break;
	}
}

bool CHLSelectFireMachineGun::Deploy( void )
{
	// Forget about any bursts this weapon was firing when holstered
	m_iBurstSize = 0;
	return BaseClass::Deploy();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHLSelectFireMachineGun::PrimaryAttack( void )
{
	if (m_bFireOnEmpty)
	{
		return;
	}
	switch( m_iFireMode )
	{
	case FIREMODE_FULLAUTO:
		BaseClass::PrimaryAttack();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
		SetWeaponIdleTime( gpGlobals->curtime + 3.0f );
		break;

	case FIREMODE_3RNDBURST:
		m_iBurstSize = GetBurstSize();
		
		// Call the think function directly so that the first round gets fired immediately.
		BurstThink();
		SetThink( BurstThink );
		m_flNextPrimaryAttack = gpGlobals->curtime + GetBurstCycleRate();
		m_flNextSecondaryAttack = gpGlobals->curtime + GetBurstCycleRate();

		// Pick up the rest of the burst through the think function.
		SetNextThink( gpGlobals->curtime + GetFireRate() );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHLSelectFireMachineGun::SecondaryAttack( void )
{
	// change fire modes.

	switch( m_iFireMode )
	{
	case FIREMODE_FULLAUTO:
		//Msg( "Burst\n" );
		m_iFireMode = FIREMODE_3RNDBURST;
		WeaponSound(SPECIAL2);
		break;

	case FIREMODE_3RNDBURST:
		//Msg( "Auto\n" );
		m_iFireMode = FIREMODE_FULLAUTO;
		WeaponSound(SPECIAL1);
		break;
	}
	
	SendWeaponAnim( GetSecondaryAttackActivity() );

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHLSelectFireMachineGun::BurstThink( void )
{
	CHLMachineGun::PrimaryAttack();

	m_iBurstSize--;

	if( m_iBurstSize == 0 )
	{
		// The burst is over!
		SetThink(NULL);

		// idle immediately to stop the firing animation
		SetWeaponIdleTime( gpGlobals->curtime );
		return;
	}

	SetNextThink( gpGlobals->curtime + GetFireRate() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHLSelectFireMachineGun::WeaponSound( WeaponSound_t shoot_type, float soundtime /*= 0.0f*/ )
{
	if (shoot_type == SINGLE)
	{
		switch( m_iFireMode )
		{
		case FIREMODE_FULLAUTO:
			BaseClass::WeaponSound(SINGLE );
			break;

		case FIREMODE_3RNDBURST:
			if( m_iBurstSize == GetBurstSize() && m_iClip1 >= m_iBurstSize )
			{
				// First round of a burst, and enough bullets remaining in the clip to fire the whole burst
				BaseClass::WeaponSound( BURST );
			}
			else if( m_iClip1 < m_iBurstSize )
			{
				// Not enough rounds remaining in the magazine to fire a burst, so play the gunshot
				// sounds individually as each round is fired.
				BaseClass::WeaponSound( SINGLE );
			}

			break;
		}
		return;
	}

	BaseClass::WeaponSound( shoot_type  );
}

// BUGBUG: These need to be rethought
//-----------------------------------------------------------------------------
int CHLSelectFireMachineGun::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	if (m_iClip1 <=0)
	{
		return COND_NO_PRIMARY_AMMO;
	}
	else if ( flDist < m_fMinRange1) 
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDist > m_fMaxRange1) 
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.5)	// UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
int CHLSelectFireMachineGun::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	return COND_NONE; // FIXME: disabled for now

	// m_iClip2 == -1 when no secondary clip is used
	if ( m_iClip2 == 0 && UsesSecondaryAmmo() )
	{
		return COND_NO_SECONDARY_AMMO;
	}
	else if ( flDist < m_fMinRange2 )
	{
		// Don't return	COND_TOO_CLOSE_TO_ATTACK only for primary attack
		return COND_NONE;
	}
	else if (flDist > m_fMaxRange2 )
	{
		// Don't return COND_TOO_FAR_TO_ATTACK only for primary attack
		return COND_NONE;
	}
	else if ( flDot < 0.5 ) // UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK2;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHLSelectFireMachineGun::CHLSelectFireMachineGun( void )
{
	m_fMinRange1	= 65;
	m_fMinRange2	= 65;
	m_fMaxRange1	= 1024;
	m_fMaxRange2	= 1024;
	m_iFireMode		= FIREMODE_FULLAUTO;
}
