//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "ai_hull.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "bitstring.h"
#include "plane.h"
#include "entitylist.h"
#include "ai_hint.h"
#include "IEffects.h"


//-----------------------------------------------------------------------------
// Purpose: If requested slot is available return true and take the slot
//			Otherwise return false
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::OccupyStrategySlot( int squadSlotID )
{
	return OccupyStrategySlotRange( squadSlotID, squadSlotID );
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::OccupyStrategySlotRange( int slotIDStart, int slotIDEnd )
{
	// If I'm not in a squad a I don't fill slots
	return ( !m_pSquad || m_pSquad->OccupyStrategySlotRange( GetEnemy(), slotIDStart, slotIDEnd, &m_iMySquadSlot ) );

}

//=========================================================
// HasStrategySlot 
//=========================================================
bool CAI_BaseNPC::HasStrategySlot( int squadSlotID )
{
	// If I wasn't taking up a squad slot I'm done
	return (m_iMySquadSlot == squadSlotID);
}

bool CAI_BaseNPC::HasStrategySlotRange( int slotIDStart, int slotIDEnd )
{
	// If I wasn't taking up a squad slot I'm done
	if (m_iMySquadSlot < slotIDStart || m_iMySquadSlot > slotIDEnd)
	{
		return false;
	}
	return true;
}

//=========================================================
// VacateSlot 
//=========================================================

void CAI_BaseNPC::VacateStrategySlot(void)
{
	if (m_pSquad)
	{
		m_pSquad->VacateStrategySlot(GetEnemy(), m_iMySquadSlot);
		m_iMySquadSlot = SQUAD_SLOT_NONE;
	}
}


//------------------------------------------------------------------------------
// Purpose :  Is cover node valid
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_BaseNPC::IsValidCover( const Vector &vecCoverLocation, CAI_Hint const *pHint )
{
	// firstly, limit choices to hint groups
	string_t iszHint = GetHintGroup();
	char *pszHint = (char *)STRING(iszHint);
	if ((iszHint != NULL_STRING) && (pszHint[0] != '\0'))
	{
		if (!pHint || pHint->GetGroup() != GetHintGroup())
		{
			return false;
		}
	}

	/*
	// If I'm in a squad don't pick cover node it other squad member
	// is already nearby
	if (m_pSquad)
	{
		return m_pSquad->IsValidCover( vecCoverLocation, pHint );
	}
	*/
	
	// UNDONE: Do we really need this test?
	// ----------------------------------------------------------------
	// Make sure my hull can fit at this node before accepting it. 
	// Could be another NPC there or it could be blocked
	// ----------------------------------------------------------------
	// FIXME: shouldn't this see that if I crouch behind it it'll be safe?
	Vector startPos = vecCoverLocation;
	startPos.z -= GetHullMins().z;  // Move hull bottom up to node
	Vector endPos	= startPos;
	endPos.z += 0.01;
	trace_t tr;
	AI_TraceEntity( this, vecCoverLocation, endPos, MASK_NPCSOLID, &tr );
	if (tr.startsolid)
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Is squad member in my way from shooting here
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsValidShootPosition ( const Vector &vecShootLocation, CAI_Hint const *pHint )
{
	// limit choices to hint groups
	if (GetHintGroup() != NULL_STRING)
	{
		if (!pHint || pHint->GetGroup() != GetHintGroup())
		{
			return false;
		}
	}

	/*
	if ( m_pSquad )
	{
		return m_pSquad->IsValidShootPosition( vecShootLocation, pHint );
	}
	*/

	return true;
}

