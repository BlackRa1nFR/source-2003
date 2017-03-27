//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: The HL2 Game rules object
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef HL1_GAMERULES_H
#define HL1_GAMERULES_H
#pragma once

#include "gamerules.h"
#include "singleplay_gamerules.h"


#ifdef CLIENT_DLL
	#define CHalfLife1 C_HalfLife1
#endif


class CHalfLife1 : public CSingleplayRules
{
public:

	DECLARE_CLASS( CHalfLife1, CSingleplayRules );
	DECLARE_NETWORKCLASS();

	
	// Client/server shared implementation.
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );


#ifdef CLIENT_DLL

	virtual void OnDataChanged( DataUpdateType_t type );

#else
	
	CHalfLife1();
	virtual ~CHalfLife1() {}

	virtual bool			ClientCommand( const char *pcmd, CBaseEntity *pEdict );
	virtual void			PlayerSpawn( CBasePlayer *pPlayer );

	virtual void			InitDefaultAIRelationships( void );
	virtual const char *	AIClassText(int classType);
	virtual const char *	GetGameDescription( void ) { return "Half-Life 1"; }

	// Ammo
	virtual void			PlayerThink( CBasePlayer *pPlayer );
	bool					CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );

	void					RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore );
#endif

};

#endif // HL1_GAMERULES_H
