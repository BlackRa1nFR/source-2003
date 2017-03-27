//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: Shared CS definitions.
//
//=============================================================================

#ifndef CS_SHAREDDEFS_H
#define CS_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif


/*======================*/
//      Menu stuff      //
/*======================*/

#include "game_controls/iviewport.h"

// CS-specific menus.
#define MENU_TERRORIST				26
#define MENU_CT						27
#define MENU_BUY					28
#define MENU_PISTOL					29
#define MENU_SHOTGUN				30
#define MENU_RIFLE					31
#define MENU_SMG					32
#define MENU_MACHINEGUN				33
#define MENU_EQUIPMENT				34


// CS Team IDs.
#define TEAM_TERRORIST	3
#define TEAM_CT			4
#define TEAM_MAXCOUNT	4	// update this if we ever add teams (unlikely)


// The various states the player can be in during the join game process.
enum CSPlayerState
{
	// Happily running around in the game.
	// You can't move though if CSGameRules()->IsFreezePeriod() returns true.
	// This state can jump to a bunch of other states like STATE_PICKINGCLASS or STATE_DEATH_ANIM.
	STATE_JOINED=0,
	
	// This is the state you're in when you first enter the server.
	// It's switching between intro cameras every few seconds, and there's a level info 
	// screen up.
	STATE_SHOWLTEXT,			// Show the level intro screen.
	
	// During these states, you can either be a new player waiting to join, or
	// you can be a live player in the game who wants to change teams.
	// Either way, you can't move while choosing team or class (or while any menu is up).
	STATE_PICKINGTEAM,			// Choosing team.
	STATE_PICKINGCLASS,			// Choosing class.
	
	STATE_DEATH_ANIM,			// Playing death anim, waiting for that to finish.
	STATE_DEATH_WAIT_FOR_KEY,	// Done playing death anim. Waiting for keypress to go into observer mode.
	STATE_OBSERVER_MODE,		// Noclipping around, watching players, etc.
	NUM_PLAYER_STATES
};


enum
{
	CS_CLASS_NONE=0,

	// Terrorist classes (keep in sync with FIRST_T_CLASS/LAST_T_CLASS).
	CS_CLASS_PHOENIX_CONNNECTION,
	CS_CLASS_L337_KREW,
	CS_CLASS_ARCTIC_AVENGERS,
	CS_CLASS_GUERILLA_WARFARE,

	// CT classes (keep in sync with FIRST_CT_CLASS/LAST_CT_CLASS).
	CS_CLASS_SEAL_TEAM_6,
	CS_CLASS_GSG_9,
	CS_CLASS_SAS,
	CS_CLASS_GIGN,

	CS_NUM_CLASSES
};


// Keep these in sync with CSClasses.
#define FIRST_T_CLASS	CS_CLASS_PHOENIX_CONNNECTION
#define LAST_T_CLASS	CS_CLASS_GUERILLA_WARFARE

#define FIRST_CT_CLASS	CS_CLASS_SEAL_TEAM_6
#define LAST_CT_CLASS	CS_CLASS_GIGN


class CCSClassInfo
{
public:
	char		*m_pClassName;
};

const CCSClassInfo* GetCSClassInfo( int i );



#endif // CS_SHAREDDEFS_H
