//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		The default shared conditions
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	SQUADSLOT_H
#define	SQUADSLOT_H

#define	MAX_SQUADSLOTS 32

//=========================================================
// These are the default shared conditions
//=========================================================
enum SQUAD_SLOT_t {

	// Currently there are no shared squad slots
	SQUAD_SLOT_NONE = -1,

	SQUAD_SLOT_ATTACK1 = 0,		// reserve 2 attack slots for most squads
	SQUAD_SLOT_ATTACK2,
	// ======================================
	// IMPORTANT: This must be the last enum
	// ======================================
	LAST_SHARED_SQUADSLOT,		
};

#endif	//SQUADSLOT_H
