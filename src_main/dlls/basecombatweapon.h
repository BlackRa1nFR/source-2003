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
// $NoKeywords: $
//=============================================================================

#ifndef COMBATWEAPON_H
#define COMBATWEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "entityoutput.h"
#include "basecombatweapon_shared.h"

//-----------------------------------------------------------------------------
// Bullet types
//-----------------------------------------------------------------------------

// -----------------------------------------
//	Sounds
// -----------------------------------------

//#define NUM_NEARMISSES	7
//extern const char * g_sNearMissSounds[ NUM_NEARMISSES ];

struct animevent_t;

extern void	SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);


// Put this in your derived class definition to declare it's activity table
// UNDONE: Cascade these?
#define DECLARE_ACTTABLE()		static acttable_t m_acttable[];\
								acttable_t *ActivityList( void );\
								int ActivityListCount( void );

// You also need to include the activity table itself in your class' implementation:
// e.g.
//	acttable_t	CWeaponStunstick::m_acttable[] = 
//	{
//		{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, TRUE },
//	};
//
// The stunstick overrides the ACT_MELEE_ATTACK1 activity, replacing it with ACT_MELEE_ATTACK_SWING.
// This animation is required for this weapon's operation.
//

// Put this after your derived class' definition to implement the accessors for the
// activity table.
// UNDONE: Cascade these?
#define IMPLEMENT_ACTTABLE(className) \
								acttable_t *className::ActivityList( void ) { return m_acttable; } \
								int className::ActivityListCount( void ) { return ARRAYSIZE(m_acttable); } \




#endif // COMBATWEAPON_H
