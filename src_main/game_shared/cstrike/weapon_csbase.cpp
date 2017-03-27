//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Laser Rifle & Shield combo
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "weapon_csbase.h"
#include "ammodef.h"
#include "cs_gamerules.h"


#if defined( CLIENT_DLL )

	#include "vgui/ISurface.h"
	#include "vgui_controls/controls.h"
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


// ----------------------------------------------------------------------------- //
// Global functions.
// ----------------------------------------------------------------------------- //

bool IsAmmoType( int iAmmoType, const char *pAmmoName )
{
	return GetAmmoDef()->Index( pAmmoName ) == iAmmoType;
}



// ----------------------------------------------------------------------------- //
// CWeaponCSBase tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCSBase, DT_WeaponCSBase )

BEGIN_NETWORK_TABLE( CWeaponCSBase, DT_WeaponCSBase )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCSBase )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_cs_base, CWeaponCSBase );


#ifdef GAME_DLL

	BEGIN_DATADESC( CWeaponCSBase )

		//DEFINE_FUNCTION( CWeaponCSBase, DefaultTouch ),
		DEFINE_FUNCTION( CWeaponCSBase, FallThink )

	END_DATADESC()		

#endif


// ----------------------------------------------------------------------------- //
// CWeaponCSBase implementation. 
// ----------------------------------------------------------------------------- //
CWeaponCSBase::CWeaponCSBase()
{
	SetPredictionEligible( true );
	m_flInaccuracy = 0;
	m_flAccuracyTime = 0;
	m_bDelayFire = true;
	m_iShotsFired = 0;
}


bool CWeaponCSBase::IsPredicted() const
{ 
	return true;
}


bool CWeaponCSBase::IsPistol() const
{
	return GetCSWpnData().m_WeaponType == WEAPONTYPE_PISTOL;
}


bool CWeaponCSBase::PlayEmptySound()
{
	//MIKETODO: certain weapons should override this to make it empty:
	//	C4
	//	Flashbang
	//	HE Grenade
	//	Smoke grenade				

	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();

	if ( IsPistol() )
	{
		EmitSound( filter, entindex(), "Default.ClipEmpty_Pistol" );
	}
	else
	{
		EmitSound( filter, entindex(), "Default.ClipEmpty_Rifle" );
	}

	return 0;
}


void CWeaponCSBase::EjectBrassLate()
{
	/*
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150) 
			 				 + gpGlobals->v_forward * 25;

	EjectBrass( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -9
		+ gpGlobals->v_forward * 16 - gpGlobals->v_right * 9 , vecShellVelocity, pev->angles.y, m_iShellId, TE_BOUNCE_SHELL ); 
	*/
}


CCSPlayer* CWeaponCSBase::GetPlayerOwner() const
{
	return dynamic_cast< CCSPlayer* >( GetOwner() );
}


void CWeaponCSBase::ItemPostFrame()
{
	CCSPlayer *pPlayer = GetPlayerOwner();


	//GOOSEMAN : Return zoom level back to previous zoom level before we fired a shot. This is used only for the AWP.
	if ( (m_flNextPrimaryAttack <= gpGlobals->curtime) && (pPlayer->m_bResumeZoom == TRUE) )
	{
		#ifndef CLIENT_DLL
			pPlayer->SetFOV( pPlayer->m_iLastZoom );

			if ( pPlayer->GetFOV() == pPlayer->m_iLastZoom )
			{
				// return the fade level in zoom.
				pPlayer->m_bResumeZoom = false;
			}
		#endif
	}

	//GOOSEMAN : Delayed shell ejection code..
	if ( (pPlayer->m_flEjectBrass != 0.0) && (pPlayer->m_flEjectBrass <= gpGlobals->curtime ) )
	{
		pPlayer->m_flEjectBrass = 0.0;
		EjectBrassLate();
	}


	if ((m_bInReload) && (pPlayer->m_flNextAttack <= gpGlobals->curtime))
	{
		// complete the reload. 
		int j = min( GetMaxClip1() - m_iClip1, pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );	

		// Add them to the clip
		m_iClip1 += j;
		pPlayer->RemoveAmmo( j, m_iPrimaryAmmoType );

		m_bInReload = false;
	}

	if ((pPlayer->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		if ( m_iClip2 != -1 && !pPlayer->GetAmmoCount( GetSecondaryAmmoType() ) )
		{
			m_bFireOnEmpty = TRUE;
		}
		SecondaryAttack();
		pPlayer->m_nButtons &= ~IN_ATTACK2;
	}
	else if ((pPlayer->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime ))
	{
		if ( (m_iClip1 == 0/* && pszAmmo1()*/) || (GetMaxClip1() == -1 && !pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) ) )
		{
			m_bFireOnEmpty = TRUE;
		}

		// Can't shoot during the freeze period
		// Ken: Always allow firing in single player
		//---
		if ( !CSGameRules()->IsFreezePeriod() && 
			!pPlayer->m_bIsDefusing &&
			pPlayer->State_Get() == STATE_JOINED
			)
		{
			PrimaryAttack();
		}
		//---
	}
	else if ( pPlayer->m_nButtons & IN_RELOAD && GetMaxClip1() != WEAPON_NOCLIP && !m_bInReload && m_flNextPrimaryAttack < gpGlobals->curtime) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		
		//MIKETODO: add code for shields...
		//if ( !FBitSet( m_iWeaponState, WPNSTATE_SHIELD_DRAWN ) )
			  Reload();	
	}
	else if ( !(pPlayer->m_nButtons & (IN_ATTACK|IN_ATTACK2) ) )
	{
		// no fire buttons down

		// The following code prevents the player from tapping the firebutton repeatedly 
		// to simulate full auto and retaining the single shot accuracy of single fire
		if (m_bDelayFire == TRUE)
		{
			m_bDelayFire = FALSE;
			if (m_iShotsFired > 15)
				m_iShotsFired = 15;
			m_flDecreaseShotsFired = gpGlobals->curtime + 0.4;
		}

		m_bFireOnEmpty = FALSE;

		// if it's a pistol then set the shots fired to 0 after the player releases a button
		if ( IsPistol() )
		{
			m_iShotsFired = 0;
		}
		else
		{
			if ( (m_iShotsFired > 0) && (m_flDecreaseShotsFired < gpGlobals->curtime)	)
			{
				m_flDecreaseShotsFired = gpGlobals->curtime + 0.0225;
				m_iShotsFired--;
			}
		}

		#ifndef CLIENT_DLL
			if ( !IsUseable() && m_flNextPrimaryAttack < gpGlobals->curtime ) 
			{
				// Intentionally blank -- used to switch weapons here
			}
			else
		#endif
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if ( m_iClip1 == 0 && !(GetFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime )
			{
				Reload();
				return;
			}
		}

		WeaponIdle( );
		return;
	}
}


float CWeaponCSBase::GetMaxSpeed() const
{
	// The weapon should have set this in its constructor.
	float flRet = GetCSWpnData().m_flMaxSpeed;
	Assert( flRet > 1 );
	return flRet;
}


const CCSWeaponInfo &CWeaponCSBase::GetCSWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CCSWeaponInfo *pCSInfo;

	#ifdef _DEBUG
		pCSInfo = dynamic_cast< const CCSWeaponInfo* >( pWeaponInfo );
		Assert( pCSInfo );
	#else
		pCSInfo = static_cast< const CCSWeaponInfo* >( pWeaponInfo );
	#endif

	return *pCSInfo;
}


// GOOSEMAN : Kick the view..
void CWeaponCSBase::KickBack( float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	float flKickUp;
	float flKickLateral;

	if (m_iShotsFired == 1) // This is the first round fired
	{
		flKickUp = up_base;
		flKickLateral = lateral_base;
	}
	else
	{
		flKickUp = up_base + m_iShotsFired*up_modifier;
		flKickLateral = lateral_base + m_iShotsFired*lateral_modifier;
	}


	QAngle angle = pPlayer->GetPunchAngle();

	angle.x -= flKickUp;
	if ( angle.x < -1 * up_max )
		angle.x = -1 * up_max;
	
	if ( m_iDirection == 1 )
	{
		angle.y += flKickLateral;
		if (angle.y > lateral_max)
			angle.y = lateral_max;
	}
	else
	{
		angle.y -= flKickLateral;
		if ( angle.y < -1 * lateral_max )
			angle.y = -1 * lateral_max;
	}

	if ( !SHARED_RANDOMINT( 0, direction_change ) )
		m_iDirection = 1 - m_iDirection;

	pPlayer->SetPunchAngle( angle );
}


#ifdef CLIENT_DLL
	
	//-----------------------------------------------------------------------------
	// Purpose: Draw the weapon's crosshair
	//-----------------------------------------------------------------------------
	void CWeaponCSBase::DrawCrosshair()
	{
		BaseClass::DrawCrosshair();
		/*
		// Draw the targeting zone around the crosshair
		int r, g, b, a;
		gHUD.m_clrYellowish.GetColor( r, g, b, a );

		// Check to see if we are in vgui mode
		C_BasePlayer *pPlayer = static_cast<C_BasePlayer*>( GetOwner() );
		if ( !pPlayer || pPlayer->IsInVGuiInputMode() )
			return;

		// Draw a crosshair & accuracy hoodad
		int iBarWidth = XRES(10);
		int iBarHeight = YRES(10);
		int iTotalWidth = (iBarWidth * 2) + (40 * m_flInaccuracy) + XRES(10);
		int iTotalHeight = (iBarHeight * 2) + (40 * m_flInaccuracy) + YRES(10);
		
		// Horizontal bars
		int iLeft = (ScreenWidth() - iTotalWidth) / 2;
		int iMidHeight = (ScreenHeight() / 2);

		vgui::Color dark( r, g, b, 32 );
		vgui::Color light( r, g, b, 160 );

		vgui::surface()->DrawSetColor( dark );

		vgui::surface()->DrawFilledRect( iLeft, iMidHeight-1, iLeft+ iBarWidth, iMidHeight + 2 );
		vgui::surface()->DrawFilledRect( iLeft + iTotalWidth - iBarWidth, iMidHeight-1, iLeft + iTotalWidth, iMidHeight + 2 );

		vgui::surface()->DrawSetColor( light );

		vgui::surface()->DrawFilledRect( iLeft, iMidHeight, iLeft + iBarWidth, iMidHeight + 1 );
		vgui::surface()->DrawFilledRect( iLeft + iTotalWidth - iBarWidth, iMidHeight, iLeft + iTotalWidth, iMidHeight + 1 );
		
		// Vertical bars
		int iTop = (ScreenHeight() - iTotalHeight) / 2;
		int iMidWidth = (ScreenWidth() / 2);

		vgui::surface()->DrawSetColor( dark );
		
		vgui::surface()->DrawFilledRect( iMidWidth-1, iTop, iMidWidth + 2, iTop + iBarHeight );
		vgui::surface()->DrawFilledRect( iMidWidth-1, iTop + iTotalHeight - iBarHeight, iMidWidth + 2, iTop + iTotalHeight );

		vgui::surface()->DrawSetColor( light );

		vgui::surface()->DrawFilledRect( iMidWidth, iTop, iMidWidth + 1, iTop + iBarHeight );
		vgui::surface()->DrawFilledRect( iMidWidth, iTop + iTotalHeight - iBarHeight, iMidWidth + 1, iTop + iTotalHeight );
		*/
	}


	bool CWeaponCSBase::ShouldPredict()
	{
		if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
			return true;

		return BaseClass::ShouldPredict();
	}


#else


	//-----------------------------------------------------------------------------
	// Purpose: Get the accuracy derived from weapon and player, and return it
	//-----------------------------------------------------------------------------
	const Vector& CWeaponCSBase::GetBulletSpread()
	{
		static Vector cone = VECTOR_CONE_8DEGREES;
		return cone;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void CWeaponCSBase::ItemBusyFrame()
	{
		BaseClass::ItemBusyFrame();

		RecalculateAccuracy();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	float CWeaponCSBase::GetFireRate()
	{	
		// Derived weapons must implement this.
		Assert( false );
		return 1;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void CWeaponCSBase::RecalculateAccuracy()
	{
		CBasePlayer *pPlayer = (CBasePlayer*)GetOwner();
		if (!pPlayer)
			return;

		m_flAccuracyTime += gpGlobals->frametime;

		while ( m_flAccuracyTime > 0.05 )
		{
			if ( !(pPlayer->GetFlags() & FL_ONGROUND) )
			{
				m_flInaccuracy += 0.05;
			}
			else if ( pPlayer->GetFlags() & FL_DUCKING )
			{
				m_flInaccuracy -= 0.08;
			}
			else
			{
				m_flInaccuracy -= 0.04;
			}

			// Crouching prevents accuracy ever going beyond a point
			if ( pPlayer->GetFlags() & FL_DUCKING )
			{
				m_flInaccuracy = clamp(m_flInaccuracy, 0, 0.8);
			}
			else
			{
				m_flInaccuracy = clamp(m_flInaccuracy, 0, 1);
			}

			m_flAccuracyTime -= 0.05;
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: Match the anim speed to the weapon speed while crouching
	//-----------------------------------------------------------------------------
	float CWeaponCSBase::GetDefaultAnimSpeed()
	{
		return 1.0;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Draw the laser rifle effect
	//-----------------------------------------------------------------------------
	void CWeaponCSBase::BulletWasFired( const Vector &vecStart, const Vector &vecEnd )
	{
	}


	bool CWeaponCSBase::ShouldRemoveOnRoundRestart()
	{
		if ( GetPlayerOwner() )
			return false;
		else
			return true;
	}


	//=========================================================
	// Materialize - make a CWeaponCSBase visible and tangible
	//=========================================================
	void CWeaponCSBase::Materialize()
	{
		if ( m_fEffects & EF_NODRAW )
		{
			// changing from invisible state to visible.
			//Ken: Suppress the default sound
			//---
			if( g_pGameRules->IsMultiplayer() )
			{
				//EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
			}

			//---
			m_fEffects &= ~EF_NODRAW;
			m_fEffects |= EF_MUZZLEFLASH;
		}

		AddSolidFlags( FSOLID_TRIGGER );

		Relink();
		//SetTouch( &CWeaponCSBase::DefaultTouch );

		//Ken: Leave guns & items lying around in single-player
		//---
		if( g_pGameRules->IsMultiplayer() ) 
		{
			SetThink (&CWeaponCSBase::SUB_Remove);
			SetNextThink( gpGlobals->curtime + 1 );
		}
		else
		{
			SetThink( NULL );
		}
	}

	//=========================================================
	// AttemptToMaterialize - the item is trying to rematerialize,
	// should it do so now or wait longer?
	//=========================================================
	void CWeaponCSBase::AttemptToMaterialize()
	{
		float time = g_pGameRules->FlWeaponTryRespawn( this );

		if ( time == 0 )
		{
			Materialize();
			return;
		}

		SetNextThink( gpGlobals->curtime + time );
	}

	//=========================================================
	// CheckRespawn - a player is taking this weapon, should 
	// it respawn?
	//=========================================================
	void CWeaponCSBase::CheckRespawn()
	{
		//GOOSEMAN : Do not respawn weapons!
		return;
	}

		
	//=========================================================
	// Respawn- this item is already in the world, but it is
	// invisible and intangible. Make it visible and tangible.
	//=========================================================
	CBaseEntity* CWeaponCSBase::Respawn()
	{
		// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
		// will decide when to make the weapon visible and touchable.
		CBaseEntity *pNewWeapon = CBaseEntity::Create( (char *)STRING( pev->classname ), g_pGameRules->VecWeaponRespawnSpot( this ), GetAbsAngles(), GetOwner() );

		if ( pNewWeapon )
		{
			pNewWeapon->m_fEffects |= EF_NODRAW;// invisible for now
			pNewWeapon->SetTouch( NULL );// no touch
			pNewWeapon->SetThink( &CWeaponCSBase::AttemptToMaterialize );

			UTIL_DropToFloor( this, MASK_SOLID );

			// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
			// but when it should respawn is based on conditions belonging to the weapon that was taken.
			pNewWeapon->SetNextThink( gpGlobals->curtime + g_pGameRules->FlWeaponRespawnTime( this ) );
		}
		else
		{
			Msg( "Respawn failed to create %s!\n", STRING( pev->classname ) );
		}

		return pNewWeapon;
	}

	bool CWeaponCSBase::Reload()
	{
		m_iShotsFired = 0;
		return BaseClass::Reload();
	}
	
	bool CWeaponCSBase::Deploy()
	{
		m_iShotsFired = 0;
		m_flDecreaseShotsFired = gpGlobals->curtime;
		
		CCSPlayer *pPlayer = GetPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->m_bResumeZoom = false;
			pPlayer->m_iLastZoom = 90;
		}
		
		return BaseClass::Deploy();
	}

	
	bool CWeaponCSBase::IsUseable()
	{
		CCSPlayer *pPlayer = GetPlayerOwner();

		if ( Clip1() <= 0 )
		{
			if ( pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) <= 0 && GetMaxClip1() != -1 )			
			{
				// clip is empty (or nonexistant) and the player has no more ammo of this type. 
				return false;
			}
		}

		return true;
	}

#endif

