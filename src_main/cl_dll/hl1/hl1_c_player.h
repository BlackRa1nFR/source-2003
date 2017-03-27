//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef C_HL1_PLAYER_H
#define C_HL1_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseplayer.h"

class C_HL1_Player : public CBasePlayer
{
public:
	DECLARE_CLASS( C_HL1_Player, C_BasePlayer );
	DECLARE_CLIENTCLASS();

	C_HL1_Player();


	CNetworkVar( bool, m_bHasLongJump );

private:
	C_HL1_Player( const C_HL1_Player & );
	
};

inline C_HL1_Player *ToHL1Player( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<C_HL1_Player*>( pEntity );
}

#endif //C_HL1_PLAYER_H