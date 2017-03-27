/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef GAME_H
#define GAME_H


#include "globals.h"

extern void GameDLLInit( void );

// Skill Level
#define SKILL_EASY		1
#define SKILL_MEDIUM	2
#define SKILL_HARD		3



extern ConVar	displaysoundlist;
extern ConVar	mapcyclefile;
extern ConVar	servercfgfile;
extern ConVar	lservercfgfile;

// multiplayer server rules
extern ConVar	teamplay;
extern ConVar	fraglimit;
extern ConVar	timelimit;
extern ConVar	friendlyfire;
extern ConVar	falldamage;
extern ConVar	weaponstay;
extern ConVar	forcerespawn;
extern ConVar	footsteps;
extern ConVar	flashlight;
extern ConVar	aimcrosshair;
extern ConVar	decalfrequency;
extern ConVar	teamlist;
extern ConVar	teamoverride;
extern ConVar	defaultteam;
extern ConVar	allowNPCs;

extern ConVar	suitvolume;
extern ConVar	timelimit;

// Engine Cvars
extern const ConVar *g_pDeveloper;
#endif		// GAME_H
