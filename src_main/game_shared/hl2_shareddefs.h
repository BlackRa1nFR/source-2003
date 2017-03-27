//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef HL2_SHAREDDEFS_H
#define HL2_SHAREDDEFS_H

#ifdef _WIN32
#pragma once
#endif

#include "const.h"


//--------------------------------------------------------------------------
// Collision groups
//--------------------------------------------------------------------------

enum
{
	HL2COLLISION_GROUP_PLASMANODE = LAST_SHARED_COLLISION_GROUP,
	HL2COLLISION_GROUP_SPIT,
	HL2COLLISION_GROUP_HOMING_MISSILE,
	HL2COLLISION_GROUP_FIRST_NPC,
	HL2COLLISION_GROUP_HOUNDEYE,
	HL2COLLISION_GROUP_ZOMBIE, // E3 HACK: so crows can sit on zombies and peck them until they stand up
	HL2COLLISION_GROUP_CROW,
	HL2COLLISION_GROUP_HEADCRAB,
	HL2COLLISION_GROUP_LAST_NPC,
};


#endif // HL2_SHAREDDEFS_H
