//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include <KeyValues.h>
#include "cs_weapon_parse.h"
#include "cs_shareddefs.h"


FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CCSWeaponInfo;
}



CCSWeaponInfo::CCSWeaponInfo()
{
	m_flMaxSpeed = 1; // This should always be set in the script.
}


void CCSWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_flMaxSpeed = (float)pKeyValuesData->GetInt( "MaxPlayerSpeed", 1 );

	m_iWeaponPrice = pKeyValuesData->GetInt( "WeaponPrice", -1 );
	if ( m_iWeaponPrice == -1 )
	{
		// This weapon should have the price in its script.
		Assert( false );
	}

	// Figure out what team can have this weapon.
	m_iTeam = TEAM_UNASSIGNED;
	const char *pTeam = pKeyValuesData->GetString( "Team", NULL );
	if ( pTeam )
	{
		if ( Q_stricmp( pTeam, "CT" ) == 0 )
		{
			m_iTeam = TEAM_CT;
		}
		else if ( Q_stricmp( pTeam, "TERRORIST" ) == 0 )
		{
			m_iTeam = TEAM_TERRORIST;
		}
		else if ( Q_stricmp( pTeam, "ANY" ) == 0 )
		{
			m_iTeam = TEAM_UNASSIGNED;
		}
		else
		{
			Assert( false );
		}
	}
	else
	{
		Assert( false );
	}

	const char *pWrongTeamMsg = pKeyValuesData->GetString( "WrongTeamMsg", "" );
	Q_strncpy( m_WrongTeamMsg, pWrongTeamMsg, sizeof( m_WrongTeamMsg ) );
	
	
	const char *pTypeString = pKeyValuesData->GetString( "WeaponType", NULL );
	
	m_WeaponType = WEAPONTYPE_UNKNOWN;
	if ( !pTypeString )
	{
		Assert( false );
	}
	else if ( Q_stricmp( pTypeString, "Pistol" ) == 0 )
	{
		m_WeaponType = WEAPONTYPE_PISTOL;
	}
	else if ( Q_stricmp( pTypeString, "Rifle" ) == 0 )
	{
		m_WeaponType = WEAPONTYPE_RIFLE;
	}
	else if ( Q_stricmp( pTypeString, "C4" ) == 0 )
	{
		m_WeaponType = WEAPONTYPE_C4;
	}
	else if ( Q_stricmp( pTypeString, "Grenade" ) == 0 )
	{
		m_WeaponType = WEAPONTYPE_GRENADE;
	}
	else
	{
		Assert( false );
	}
}


