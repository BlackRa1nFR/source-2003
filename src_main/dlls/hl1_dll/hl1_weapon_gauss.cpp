//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Gauss
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "NPCEvent.h"
#include "hl1_basecombatweapon_shared.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"
#include "hl1_player.h"
#include "shake.h"


extern ConVar sk_plr_dmg_gauss;


//-----------------------------------------------------------------------------
// CWeaponGauss
//-----------------------------------------------------------------------------


class CWeaponGauss : public CBaseHL1CombatWeapon
{
	DECLARE_CLASS( CWeaponGauss, CBaseHL1CombatWeapon );
public:

	CWeaponGauss( void );

	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	WeaponIdle( void );
	void	AddViewKick( void );
	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	void	StopSpinSound( void );
	float	GetFullChargeTime( void );
	void	StartFire( void );
	void	Fire( Vector vecOrigSrc, Vector vecDir, float flDamage );

private:
	int			m_nAttackState;
	bool		m_bPrimaryFire;
	CSoundPatch	*m_sndCharge;
	int			m_iBeam;
	int			m_iGlow;
};

LINK_ENTITY_TO_CLASS( weapon_gauss, CWeaponGauss );

PRECACHE_WEAPON_REGISTER( weapon_gauss );

IMPLEMENT_SERVERCLASS_ST( CWeaponGauss, DT_WeaponGauss )
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponGauss )
	DEFINE_FIELD( CWeaponGauss, m_nAttackState, FIELD_INTEGER ),
	DEFINE_FIELD( CWeaponGauss, m_bPrimaryFire,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CWeaponGauss, m_iBeam, FIELD_INTEGER ),
	DEFINE_FIELD( CWeaponGauss, m_iGlow, FIELD_INTEGER ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponGauss::CWeaponGauss( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;

	m_bPrimaryFire		= false;
	m_nAttackState		= 0;
	m_sndCharge			= NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGauss::Precache( void )
{
	m_iBeam = engine->PrecacheModel( "sprites/smoke.vmt" );
	m_iGlow = engine->PrecacheModel( "sprites/hotglow.vmt" );

	enginesound->PrecacheSound( "weapons/gauss2.wav" );

	BaseClass::Precache();
}

float CWeaponGauss::GetFullChargeTime( void )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		return 1.5;
	}
	else
	{
		return 4;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGauss::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) < 2 )
	{
		WeaponSound( EMPTY );
		pPlayer->SetNextAttack( gpGlobals->curtime + 0.5 );
		return;
	}

//FIXME	pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	m_bPrimaryFire = true;

	pPlayer->RemoveAmmo( 2, m_iPrimaryAmmoType );

	StartFire();
	m_nAttackState = 0;
	SetWeaponIdleTime( gpGlobals->curtime + 1.0 );
	pPlayer->SetNextAttack( gpGlobals->curtime + 0.2 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGauss::SecondaryAttack( void )
{
	CHL1_Player *pPlayer = ToHL1Player( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	// don't fire underwater
	if ( pPlayer->GetWaterLevel() == 3 )
	{
		if ( m_nAttackState != 0 )
		{
			EmitSound( "Weapon_Gauss.Zap1" );
			SendWeaponAnim( ACT_VM_IDLE );
			m_nAttackState = 0;
		}
		else
		{
			WeaponSound( EMPTY );
		}

		m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
		return;
	}

	if ( m_nAttackState == 0 )
	{
		if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		{
			WeaponSound( EMPTY );
			pPlayer->SetNextAttack( gpGlobals->curtime + 0.5 );
			return;
		}

		m_bPrimaryFire = false;

		pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );	// take one ammo just to start the spin
		pPlayer->m_flNextAmmoBurn = gpGlobals->curtime;

		// spin up
//FIXME		pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_CHARGE_VOLUME;
		
		SendWeaponAnim( ACT_GAUSS_SPINUP );
		m_nAttackState = 1;
		SetWeaponIdleTime( gpGlobals->curtime + 0.5 );
		pPlayer->m_flStartCharge = gpGlobals->curtime;
		pPlayer->m_flAmmoStartCharge = gpGlobals->curtime + GetFullChargeTime();

		//Start looping sound
		if ( m_sndCharge == NULL )
		{
			CPASAttenuationFilter filter( this );
			m_sndCharge	= (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_WEAPON, "ambience/pulsemachine.wav", ATTN_NORM );
		}

		assert(m_sndCharge!=NULL);
		if ( m_sndCharge != NULL )
		{
			(CSoundEnvelopeController::GetController()).Play( m_sndCharge, 1.0f, 110 );
		}
	}
	else if (m_nAttackState == 1)
	{
		if ( HasWeaponIdleTimeElapsed() )
		{
			SendWeaponAnim( ACT_GAUSS_SPINCYCLE );
			m_nAttackState = 2;
		}
	}
	else
	{
		// during the charging process, eat one bit of ammo every once in a while
		if ( gpGlobals->curtime >= pPlayer->m_flNextAmmoBurn && pPlayer->m_flNextAmmoBurn != 1000 )
		{
			pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );

			if ( g_pGameRules->IsMultiplayer() )
			{
				pPlayer->m_flNextAmmoBurn = gpGlobals->curtime + 0.1;
			}
			else
			{
				pPlayer->m_flNextAmmoBurn = gpGlobals->curtime + 0.3;
			}
		}

		if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		{
			// out of ammo! force the gun to fire
			StartFire();
			m_nAttackState = 0;
			SetWeaponIdleTime( gpGlobals->curtime + 1.0 );
			pPlayer->SetNextAttack( gpGlobals->curtime + 1 );
			return;
		}
		
		if ( gpGlobals->curtime >= pPlayer->m_flAmmoStartCharge )
		{
			// don't eat any more ammo after gun is fully charged.
			pPlayer->m_flNextAmmoBurn = 1000;
		}

		int pitch = ( gpGlobals->curtime - pPlayer->m_flStartCharge ) * ( 150 / GetFullChargeTime() ) + 100;
		if ( pitch > 250 ) 
			 pitch = 250;
		
		// ALERT( at_console, "%d %d %d\n", m_nAttackState, m_iSoundState, pitch );

//		if ( m_iSoundState == 0 )
//			ALERT( at_console, "sound state %d\n", m_iSoundState );

		if ( m_sndCharge != NULL )
		{
			(CSoundEnvelopeController::GetController()).SoundChangePitch( m_sndCharge, pitch, 0 );
		}

//FIXME		m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_CHARGE_VOLUME;
		
		// m_flTimeWeaponIdle = gpGlobals->curtime + 0.1;
		if ( pPlayer->m_flStartCharge < gpGlobals->curtime - 10 )
		{
			// Player charged up too long. Zap him.
			EmitSound( "Weapon_Gauss.Zap1" );
			EmitSound( "Weapon_Gauss.Zap2" );
			
			m_nAttackState = 0;
			SetWeaponIdleTime( gpGlobals->curtime + 1.0 );
			pPlayer->SetNextAttack( gpGlobals->curtime + 1.0 );
				
			// Add DMG_CRUSH because we don't want any physics force
			pPlayer->TakeDamage( CTakeDamageInfo( this, this, 50, DMG_SHOCK | DMG_CRUSH ) );

			color32 gaussDamage = {255,128,0,128};
			UTIL_ScreenFade( pPlayer, gaussDamage, 2, 0.5, FFADE_IN );

			SendWeaponAnim( ACT_VM_IDLE );
			
			StopSpinSound();
			// Player may have been killed and this weapon dropped, don't execute any more code after this!
			return;
		}
	}
}

//=========================================================
// StartFire- since all of this code has to run and then 
// call Fire(), it was easier at this point to rip it out 
// of weaponidle() and make its own function then to try to
// merge this into Fire(), which has some identical variable names 
//=========================================================
void CWeaponGauss::StartFire( void )
{
	float flDamage;
	
	CHL1_Player *pPlayer = ToHL1Player( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	Vector vecAiming	= pPlayer->GetAutoaimVector( 0 );
	Vector vecSrc		= pPlayer->Weapon_ShootPosition( );
	
	if ( gpGlobals->curtime - pPlayer->m_flStartCharge > GetFullChargeTime() )
	{
		flDamage = 200;
	}
	else
	{
		flDamage = 200 * (( gpGlobals->curtime - pPlayer->m_flStartCharge) / GetFullChargeTime() );
	}

	if ( m_bPrimaryFire )
	{
		flDamage = sk_plr_dmg_gauss.GetFloat();
	}

	//ALERT ( at_console, "Time:%f Damage:%f\n", gpGlobals->curtime - m_pPlayer->m_flStartCharge, flDamage );
	Vector	vecNewVel	= pPlayer->GetAbsVelocity();
	float	flZVel		= vecNewVel.z;

	if ( !m_bPrimaryFire )
	{
		vecNewVel = vecNewVel - vecAiming * flDamage * 5;
		pPlayer->SetAbsVelocity( vecNewVel );
	}

	if ( !g_pGameRules->IsMultiplayer() )
	{
		// in deathmatch, gauss can pop you up into the air. Not in single play.
		vecNewVel.z = flZVel;
		pPlayer->SetAbsVelocity( vecNewVel );
	}

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// time until aftershock 'static discharge' sound
	pPlayer->m_flPlayAftershock = gpGlobals->curtime + random->RandomFloat( 0.3, 0.8 );

	Fire( vecSrc, vecAiming, flDamage );
}

void CWeaponGauss::Fire( Vector vecOrigSrc, Vector vecDir, float flDamage )
{
	CBaseEntity *pIgnore;
	Vector		vecSrc		= vecOrigSrc;
	Vector		vecDest		= vecSrc + vecDir * MAX_TRACE_LENGTH;
	bool		fFirstBeam	= true;
	bool		fHasPunched = false;
	float		flMaxFrac	= 1.0;
	int			nTotal		= 0;
	int			nMaxHits	= 10;

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

//FIXME	pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;

	StopSpinSound();
	
	pIgnore = pPlayer;

//	ALERT( at_console, "%f %f\n", tr.flFraction, flMaxFrac );

	while ( flDamage > 10 && nMaxHits > 0 )
	{
		trace_t	tr;

		nMaxHits--;

		// ALERT( at_console, "." );
		UTIL_TraceLine( vecSrc, vecDest, MASK_SHOT, pIgnore, COLLISION_GROUP_NONE, &tr );

		if ( tr.allsolid )
			break;

		CBaseEntity *pEntity = tr.m_pEnt;
		if (pEntity == NULL)
			break;

		CBroadcastRecipientFilter filter;

		if ( fFirstBeam )
		{
			pPlayer->m_fEffects |= EF_MUZZLEFLASH;
			fFirstBeam = false;
	
			nTotal += 26;

			te->BeamEntPoint(
				filter,
				0.0,
				pPlayer->GetViewModel()->entindex(),
				NULL,
				0,
				&tr.endpos,
				m_iBeam,
				0,
				0,
				0,
				0.1,
				m_bPrimaryFire ? 1.0 : 2.5,
				m_bPrimaryFire ? 1.0 : 2.5,
				0,
				0.0,
				255,//m_bPrimaryFire ? 255 : 255,
				255,//m_bPrimaryFire ? 255 : 255,
				m_bPrimaryFire ? 0 : 255,
				m_bPrimaryFire ? 200.0 : min( flDamage, 255 ),
				0
			);
		}
		else
		{
			te->BeamPoints(
				filter,
				0.0,
				&vecSrc,
				&tr.endpos,
				m_iBeam,
				0,
				0,
				0,
				0.1,
				m_bPrimaryFire ? 1.0 : 2.5,
				m_bPrimaryFire ? 1.0 : 2.5,
				0,
				0.0,
				255,//m_bPrimaryFire ? 255 : 255,
				255,//m_bPrimaryFire ? 128 : 255,
				m_bPrimaryFire ? 0 : 255,
				m_bPrimaryFire ? 200.0 : min( flDamage, 255 ),
				0
			);
		}
		
		if ( pEntity->m_takedamage != DAMAGE_NO )
		{
			ClearMultiDamage();
			CTakeDamageInfo info( this, pPlayer, flDamage, DMG_ENERGYBEAM );
			CalculateMeleeDamageForce( &info, vecDir, tr.endpos );
			pEntity->DispatchTraceAttack( info, vecDir, &tr );
			ApplyMultiDamage();
		}

		if ( pEntity->IsBSPModel() && (pEntity->m_takedamage == DAMAGE_NO) )
		{
			float n;

			pIgnore = NULL;

			n = -DotProduct( tr.plane.normal, vecDir );

			if ( n < 0.5 ) // 60 degrees
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				Vector r;
			
				r = 2.0 * tr.plane.normal * n + vecDir;
				flMaxFrac = flMaxFrac - tr.fraction;
				vecDir = r;
				vecSrc = tr.endpos;// + vecDir * 8;
				vecDest = vecSrc + vecDir * MAX_TRACE_LENGTH;

				// explode a bit
				RadiusDamage( CTakeDamageInfo( this, pPlayer, flDamage * n, DMG_BLAST ), tr.endpos, flDamage * n * 2.5, CLASS_NONE );

				nTotal += 34;
				
/*
//FIXME
				vec3_t r;
			
				VectorMA( forward, 2.0 * n, tr.plane.normal, r );

				flMaxFrac = flMaxFrac - tr.fraction;
				
				VectorCopy( r, forward );

				VectorMA( tr.endpos, 8.0, forward, vecSrc );
				VectorMA( vecSrc, 8192.0, forward, vecDest );

				gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage * n / 255.0, flDamage * n * 0.5 * 0.1, FTENT_FADEOUT );

				vec3_t fwd;
				VectorAdd( tr.endpos, tr.plane.normal, fwd );

				gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100,
									255, 100 );
*/

				// lose energy
				if (n == 0)
					n = 0.1;

				flDamage = flDamage * (1 - n);
			}
			else
			{
				nTotal += 13;

/*
//FIXME
				// tunnel
				UTIL_ImpactTrace( &tr, DMG_ENERGYBEAM );

				gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 1.0, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT );
*/

				// limit it to one hole punch
				if ( fHasPunched )
					break;

				fHasPunched = true;

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if ( !m_bPrimaryFire )
				{
					trace_t beam_tr;

					UTIL_TraceLine( tr.endpos + vecDir * 8, vecDest, MASK_SHOT, pIgnore, COLLISION_GROUP_NONE, &beam_tr);
					if ( !beam_tr.allsolid )
					{
						// trace backwards to find exit point
						UTIL_TraceLine( beam_tr.endpos, tr.endpos, MASK_SHOT, pIgnore, COLLISION_GROUP_NONE, &beam_tr);

						float n = (beam_tr.endpos - tr.endpos).Length( );

						if ( n < flDamage )
						{
							if (n == 0)
								n = 1;

							flDamage -= n;

/*
//FIXME
							// absorption balls
							{
								vec3_t fwd;
								VectorSubtract( tr.endpos, forward, fwd );
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100,
									255, 100 );
							}
							UTIL_ImpactTrace( &beam_tr, DMG_ENERGYBEAM );
							gEngfuncs.pEfxAPI->R_TempSprite( beam_tr.endpos, vec3_origin, 0.1, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT );
			
							// balls
							{
								vec3_t fwd;
								VectorSubtract( beam_tr.endpos, forward, fwd );
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, beam_tr.endpos, fwd, m_iBalls, (int)(flDamage * 0.3), 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 200,
									255, 40 );
							}
*/

							// ALERT( at_console, "punch %f\n", n );
							nTotal += 21;

							// exit blast damage
							float flDamageRadius;

							if ( g_pGameRules->IsMultiplayer() )
							{
								flDamageRadius = flDamage * 1.75;  // Old code == 2.5
							}
							else
							{
								flDamageRadius = flDamage * 2.5;
							}

							RadiusDamage( CTakeDamageInfo( this, pPlayer, flDamage, DMG_BLAST ), beam_tr.endpos + vecDir * 8, flDamageRadius, CLASS_NONE );

							CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 1024, 3.0 );

							nTotal += 53;

							vecSrc = beam_tr.endpos + vecDir;
						}
					}
					else
					{
						 //ALERT( at_console, "blocked %f\n", n );
						flDamage = 0;
					}
				}
				else
				{
					//ALERT( at_console, "blocked solid\n" );
/*
//FIXME
					if ( m_fPrimaryFire )
					{
						// slug doesn't punch through ever with primary 
						// fire, so leave a little glowy bit and make some balls
						gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, 200.0 / 255.0, 0.3, FTENT_FADEOUT );
			
						{
							vec3_t fwd;
							VectorAdd( tr.endpos, tr.plane.normal, fwd );
							gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 8, 0.6, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100,
								255, 200 );
						}
					}
*/
					
					flDamage = 0;
				}

			}
		}
		else
		{
			vecSrc = tr.endpos + vecDir;
			pIgnore = pEntity;
		}
	}
	// ALERT( at_console, "%d bytes\n", nTotal );

	pPlayer->ViewPunch( QAngle( -2, 0, 0 ) );
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), CHAN_WEAPON, "weapons/gauss2.wav", 0.5 + flDamage * (1.0 / 400.0), ATTN_NORM, 0, 80 + random->RandomInt( 0, 0x1f ) );
}

void CWeaponGauss::WeaponIdle( void )
{
	CHL1_Player *pPlayer = ToHL1Player( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	// play aftershock static discharge
	if ( pPlayer->m_flPlayAftershock && pPlayer->m_flPlayAftershock < gpGlobals->curtime )
	{
		EmitSound( "Weapon_Gauss.StaticDischarge" );
		pPlayer->m_flPlayAftershock = 0.0;
	}

	if ( !HasWeaponIdleTimeElapsed() )
		return;

	if ( m_nAttackState != 0 )
	{
		StartFire();
		m_nAttackState = 0;
		SetWeaponIdleTime( gpGlobals->curtime + 2.0 );
	}
	else
	{
		float flRand = random->RandomFloat( 0, 1 );
		if ( flRand <= 0.75 )
		{
			SendWeaponAnim( ACT_VM_IDLE );
			SetWeaponIdleTime( gpGlobals->curtime + random->RandomFloat( 10, 15 ) );
		}
		else
		{
			SendWeaponAnim( ACT_VM_FIDGET );
			SetWeaponIdleTime( gpGlobals->curtime + 3 );
		}
	}
}

/*
==================================================
AddViewKick
==================================================
*/

void CWeaponGauss::AddViewKick( void )
{
}

bool CWeaponGauss::Deploy( void )
{
	if ( DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_VM_DRAW, (char*)GetAnimPrefix() ) )
	{
		CHL1_Player *pPlayer = ToHL1Player( GetOwner() );
		if ( pPlayer )
		{
			pPlayer->m_flPlayAftershock = 0.0;
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool CWeaponGauss::Holster( CBaseCombatWeapon *pSwitchingTo )
{

	StopSpinSound();
	m_nAttackState = 0;

	return BaseClass::Holster(pSwitchingTo);
}

void CWeaponGauss::StopSpinSound( void )
{
	if ( m_sndCharge != NULL )
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndCharge, 0.1f );
	}
}
