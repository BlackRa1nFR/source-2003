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

#ifndef HL2_GAMERULES_H
#define HL2_GAMERULES_H
#pragma once

#include "gamerules.h"
#include "singleplay_gamerules.h"


#ifdef CLIENT_DLL
	#define CHalfLife2 C_HalfLife2
#endif


class CHalfLife2 : public CSingleplayRules
{
public:
	DECLARE_CLASS( CHalfLife2, CSingleplayRules );


	virtual bool			ShouldCollide( int collisionGroup0, int collisionGroup1 );


#ifdef CLIENT_DLL

#else

	CHalfLife2();
	virtual ~CHalfLife2() {}

	virtual bool			ClientCommand( const char *pcmd, CBaseEntity *pEdict );
	virtual void			PlayerSpawn( CBasePlayer *pPlayer );

	virtual void			InitDefaultAIRelationships( void );
	virtual const char*		AIClassText(int classType);
	virtual const char *GetGameDescription( void ) { return "Half-Life 2"; }
	// Ammo
	virtual void			PlayerThink( CBasePlayer *pPlayer );

#endif
};

#endif // HL2_GAMERULES_H
