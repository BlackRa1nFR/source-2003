//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hl1_c_player.h"

IMPLEMENT_CLIENTCLASS_DT( C_HL1_Player, DT_HL1Player, CHL1_Player )
	RecvPropInt( RECVINFO( m_bHasLongJump ) ),
END_RECV_TABLE()

C_HL1_Player::C_HL1_Player()
{
	
}