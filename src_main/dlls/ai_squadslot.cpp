//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "ai_squadslot.h"
#include "ai_basenpc.h"
#include "stringregistry.h"


//=============================================================================
// Init static variables
//=============================================================================
CAI_GlobalNamespace CAI_BaseNPC::gm_SquadSlotNamespace;

//-----------------------------------------------------------------------------
// Purpose: Given and SquadSlot name, return the SquadSlot ID
//-----------------------------------------------------------------------------
int CAI_BaseNPC::GetSquadSlotID(const char* slotName)
{
	return gm_SquadSlotNamespace.SymbolToId(slotName);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InitDefaultSquadSlotSR(void)
{
	gm_SquadSlotNamespace.AddSymbol( "SQUAD_SLOT_ATTACK1", AI_RemapToGlobal(SQUAD_SLOT_ATTACK1) );
	gm_SquadSlotNamespace.AddSymbol( "SQUAD_SLOT_ATTACK2", AI_RemapToGlobal(SQUAD_SLOT_ATTACK2) );
}
