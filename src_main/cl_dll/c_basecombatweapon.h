//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Client's CBaseCombatWeapon entity
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#ifndef C_BASECOMBATWEAPON_H
#define C_BASECOMBATWEAPON_H

#ifdef _WIN32
#pragma once
#endif

#include "basecombatweapon_shared.h"
#include "weapons_resource.h"

class CViewSetup;
class C_BaseViewModel;

// Accessors for local weapons
C_BaseCombatWeapon *GetActiveWeapon( void );


#endif // C_BASECOMBATWEAPON