//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef FIRE_H
#define FIRE_H
#ifdef _WIN32
#pragma once
#endif

#include "entityoutput.h"
#include "fire_smoke.h"
#include "plasma.h"

//Spawnflags
#define	SF_FIRE_INFINITE		0x00000001
#define	SF_FIRE_SMOKELESS		0x00000002
#define	SF_FIRE_START_ON		0x00000004
#define	SF_FIRE_START_FULL		0x00000008
#define SF_FIRE_DONT_DROP		0x00000010
#define	SF_FIRE_NO_GLOW			0x00000020
#define SF_FIRE_DIE_PERMANENT	0x00000080
#define SF_FIRE_START_DISABLED	0x00000100

//==================================================
// CFire
//==================================================

enum fireType_e
{
	FIRE_NATURAL = 0,
	FIRE_PLASMA,
};

//==================================================
// FireSystem
//==================================================
bool FireSystem_StartFire( const Vector &position, float fireHeight, float attack, float fuel, int flags, CBaseEntity *owner, fireType_e type = FIRE_NATURAL);
void FireSystem_ExtinguishInRadius( const Vector &origin, float radius, float rate );
void FireSystem_AddHeatInRadius( const Vector &origin, float radius, float heat );

#endif // FIRE_H
