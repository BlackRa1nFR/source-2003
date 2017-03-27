//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_basehlcombatweapon.h"
#include "igamemovement.h"

IMPLEMENT_CLIENTCLASS_DT( C_HLMachineGun, DT_HLMachineGun, CHLMachineGun )
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_HLSelectFireMachineGun, DT_HLSelectFireMachineGun, CHLSelectFireMachineGun )
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_BaseHLBludgeonWeapon, DT_BaseHLBludgeonWeapon, CBaseHLBludgeonWeapon )
END_RECV_TABLE()