//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "ammodef.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CBaseCombatCharacter::WeaponCount() const
{
	return MAX_WEAPONS;
}


//-----------------------------------------------------------------------------
// Purpose: Switches to the given weapon (providing it has ammo)
// Input  :
// Output : true is switch suceeded
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex /*=0*/ ) 
{
	if ( pWeapon == NULL )
		return false;

	if (!Weapon_CanSwitchTo(pWeapon))
	{
		return false;
	}

	if ( m_hActiveWeapon )
	{
		if ( !m_hActiveWeapon->Holster( pWeapon ) )
			return false;
	}

	m_hActiveWeapon = pWeapon;
	return pWeapon->Deploy( );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not we can switch to the given weapon.
// Input  : pWeapon - 
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
	if ( !pWeapon->HasAnyAmmo() && !GetAmmoCount( pWeapon->m_iPrimaryAmmoType ) )
		return false;

	if ( !pWeapon->CanDeploy() )
		return false;
	
	if ( m_hActiveWeapon )
	{
		if ( !m_hActiveWeapon->CanHolster() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatWeapon
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseCombatCharacter::GetActiveWeapon() const
{
	return m_hActiveWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iCount - 
//			iAmmoIndex - 
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::RemoveAmmo( int iCount, int iAmmoIndex )
{
	if (iCount <= 0)
		return;

	// Infinite ammo?
	if ( GetAmmoDef()->MaxCarry( iAmmoIndex ) == INFINITE_AMMO )
		return;

	// Ammo pickup sound
	m_iAmmo.Set( iAmmoIndex, max( m_iAmmo[iAmmoIndex] - iCount, 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::RemoveAllAmmo( )
{
	for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		m_iAmmo.Set( i, 0 );
	}
}

//-----------------------------------------------------------------------------
// FIXME: This is a sort of hack back-door only used by physgun!
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetAmmoCount( int iCount, int iAmmoIndex )
{
	// NOTE: No sound, no max check! Seems pretty bogus to me!
	m_iAmmo.Set( iAmmoIndex, iCount );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of a particular type owned
//			owned by the character
// Input  :	Ammo Index
// Output :	The amount of ammo
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::GetAmmoCount( int iAmmoIndex ) const
{
	if ( iAmmoIndex == -1 )
		return 0;

	// Infinite ammo?
	if ( GetAmmoDef()->MaxCarry( iAmmoIndex ) == INFINITE_AMMO )
		return 999;

	return m_iAmmo[ iAmmoIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : i - 
//-----------------------------------------------------------------------------
CBaseCombatWeapon*	CBaseCombatCharacter::GetWeapon( int i )
{
	Assert( (i >= 0) && (i < MAX_WEAPONS) );
	return m_hMyWeapons[i].Get();
}

//-----------------------------------------------------------------------------
// Purpose: Returns weapon if already owns a weapon of this class
//-----------------------------------------------------------------------------
CBaseCombatWeapon* CBaseCombatCharacter::Weapon_OwnsThisType( const char *pszWeapon, int iSubType ) const
{
	// Check for duplicates
	for (int i=0;i<MAX_WEAPONS;i++) 
	{
		if ( m_hMyWeapons[i].Get() && FClassnameIs( m_hMyWeapons[i], pszWeapon ) )
		{
			// Make sure it matches the subtype
			if ( m_hMyWeapons[i]->GetSubType() == iSubType )
				return m_hMyWeapons[i];
		}
	}
	return NULL;
}