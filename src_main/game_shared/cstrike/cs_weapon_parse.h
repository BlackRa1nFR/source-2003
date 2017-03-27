//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef CS_WEAPON_PARSE_H
#define CS_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_parse.h"
#include "networkvar.h"


typedef enum
{
	
	WEAPONTYPE_PISTOL=0,
	WEAPONTYPE_RIFLE,
	WEAPONTYPE_C4,
	WEAPONTYPE_GRENADE,
	WEAPONTYPE_UNKNOWN

} CSWeaponType;


class CCSWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CCSWeaponInfo, FileWeaponInfo_t );
	
	CCSWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );


public:

	float m_flMaxSpeed;			// How fast the player can run while this is his primary weapon.
	CSWeaponType m_WeaponType;
	int m_iWeaponPrice;
	int m_iTeam;				// Which team can have this weapon. TEAM_UNASSIGNED if both can have it.
	
	char m_WrongTeamMsg[32];	// Reference to a string describing the error if someone tries to buy
								// this weapon but they're on the wrong team to have it.
								// Zero-length if no specific message for this weapon.
};


#endif // CS_WEAPON_PARSE_H
