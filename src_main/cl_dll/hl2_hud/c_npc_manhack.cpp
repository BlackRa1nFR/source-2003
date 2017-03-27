//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "c_AI_BaseNPC.h"

class C_NPC_Manhack : public C_AI_BaseNPC
{
public:
	C_NPC_Manhack() {}

	DECLARE_CLASS( C_NPC_Manhack, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

private:
	C_NPC_Manhack( const C_NPC_Manhack & );
};


IMPLEMENT_CLIENTCLASS_DT(C_NPC_Manhack, DT_NPC_Manhack, CNPC_Manhack)
END_RECV_TABLE()