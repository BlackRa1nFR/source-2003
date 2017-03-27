//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Definitions that are shared by the game DLL and the client DLL.
//
// $NoKeywords: $
//=============================================================================

#ifndef SHAREDDEFS_H
#define SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#define ANIMATION_CYCLE_BITS		15
#define ANIMATION_CYCLE_MINFRAC		(1.0f / (1<<ANIMATION_CYCLE_BITS))


// Height above entity position where the viewer's eye is.
#define VEC_VIEW			Vector( 0, 0, 64 )
#define VEC_HULL_MIN		Vector(-16, -16, 0 )
#define VEC_HULL_MAX		Vector( 16,  16,  72 )

#define VEC_DUCK_HULL_MIN		Vector(-16, -16, 0 )
#define VEC_DUCK_HULL_MAX		Vector( 16,  16,  36 )
#define VEC_DUCK_VIEW			Vector( 0, 0, 30 )

#define VEC_DEAD_VIEWHEIGHT		Vector( 0, 0, 14 )

#define WATERJUMP_HEIGHT			8

#define MAX_CLIMB_SPEED		200

#define TIME_TO_DUCK		0.4
#define TIME_TO_UNDUCK		0.2

#define MAX_WEAPON_SLOTS		6	// hud item selection slots
#define MAX_WEAPON_POSITIONS	20	// max number of items within a slot
#define MAX_ITEM_TYPES			6	// hud item selection slots
#define MAX_WEAPONS				48	// Max number of weapons available

#define MAX_ITEMS				5	// hard coded item types

#define WEAPON_NOCLIP			-1	// clip sizes set to this tell the weapon it doesn't use a clip

#define	MAX_AMMO_TYPES	32		// ???
#define MAX_AMMO_SLOTS  32		// not really slots

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

//===================================================================================================================
// Hud Element hiding flags
#define	HIDEHUD_WEAPONS		( 1<<0 )	// Hide viewmodel, ammo count & weapon selection
#define	HIDEHUD_FLASHLIGHT	( 1<<1 )
#define	HIDEHUD_ALL			( 1<<2 )
#define HIDEHUD_HEALTH		( 1<<3 )	// Hide health & armor / suit battery
#define HIDEHUD_PLAYERDEAD  ( 1<<4 )	// Hide when local player's dead
#define HIDEHUD_NEEDSUIT	( 1<<5 )	// Hide when the local player doesn't have the HEV suit
#define HIDEHUD_MISCSTATUS	( 1<<6 )	// Hide miscellaneous status elements (trains, pickup history, death notices, etc)
#define HIDEHUD_CHAT		( 1<<7 )	// Hide all communication elements (saytext, voice icon, etc)

//===================================================================================================================
// Player Defines
#define MAX_PLAYERS				32	// Max number of players in a game
#define MAX_PLAYERNAME_LENGTH	32	// Max length of a player's name

//===================================================================================================================
// Team Defines
#define	TEAM_INVALID			-1
#define TEAM_UNASSIGNED			1	// not assigned to a team
#define TEAM_SPECTATOR			2	// spectator team

#define MAX_TEAMS				32	// Max number of teams in a game
#define MAX_TEAM_NAME_LENGTH	32	// Max length of a team's name

// Weapon m_iState
#define WEAPON_IS_ONTARGET				0x40

#define WEAPON_NOT_CARRIED				0	// Weapon is on the ground
#define WEAPON_IS_CARRIED_BY_PLAYER		1	// This client is carrying this weapon.
#define WEAPON_IS_ACTIVE				2	// This client is carrying this weapon and it's the currently held weapon

// Weapon flags
// -----------------------------------------
//	Flags
// -----------------------------------------
#define ITEM_FLAG_SELECTONEMPTY		(1<<0)
#define ITEM_FLAG_NOAUTORELOAD		(1<<1)
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	(1<<2)
#define ITEM_FLAG_LIMITINWORLD		(1<<3)
#define ITEM_FLAG_EXHAUSTIBLE		(1<<4)	// A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_DOHITLOCATIONDMG	(1<<5)	// This weapon take hit location into account when applying damage
#define ITEM_FLAG_NOAMMOPICKUPS		(1<<6)	// Don't draw ammo pickup sprites/sounds when ammo is received

// Humans only have left and right hands, though we might have aliens with more
//  than two, sigh
#define MAX_VIEWMODELS			2

#define MAX_BEAM_ENTS			10

#define TRACER_TYPE_DEFAULT		0x00000001
#define TRACER_TYPE_GUNSHIP		0x00000002
#define TRACER_TYPE_STRIDER		0x00000004 // Here ya go, Jay!
#define TRACER_TYPE_GAUSS		0x00000008

#define MUZZLEFLASH_TYPE_DEFAULT	0x00000001
#define MUZZLEFLASH_TYPE_GUNSHIP	0x00000002
#define MUZZLEFLASH_TYPE_STRIDER	0x00000004

// Tracer Flags
#define TRACER_FLAG_WHIZ			0x0001
#define TRACER_FLAG_USEATTACHMENT	0x0002

#define TRACER_DONT_USE_ATTACHMENT	-1

//
// Enumerations for setting player animation.
//
enum PLAYER_ANIM
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_IN_VEHICLE,

	// TF Player animations
	PLAYER_RELOAD,
	PLAYER_START_AIMING,
	PLAYER_LEAVE_AIMING,
};

#define PLAYER_FATAL_FALL_SPEED		1024 // approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580 // approx 20 feet
#define DAMAGE_FOR_FALL_SPEED		(float)100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED ) // damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.


#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669
#define AUTOAIM_20DEGREES 0.1736481776669*2	//FIXME: Okay fine, this isn't exactly right

// instant damage

#define DMG_GENERIC			0			// generic damage was done
#define DMG_CRUSH			(1 << 0)	// crushed by falling or moving object. 
										// NOTE: It's assumed crush damage is occurring as a result of physics collision, so no extra physics force is generated by crush damage.
										// DON'T use DMG_CRUSH when damaging entities unless it's the result of a physics collision. You probably want DMG_CLUB instead.
#define DMG_BULLET			(1 << 1)	// shot
#define DMG_SLASH			(1 << 2)	// cut, clawed, stabbed
#define DMG_BURN			(1 << 3)	// heat burned
#define DMG_VEHICLE			(1 << 4)	// hit by a vehicle
#define DMG_FALL			(1 << 5)	// fell too far
#define DMG_BLAST			(1 << 6)	// explosive blast damage
#define DMG_CLUB			(1 << 7)	// crowbar, punch, headbutt
#define DMG_SHOCK			(1 << 8)	// electric shock
#define DMG_SONIC			(1 << 9)	// sound pulse shockwave
#define DMG_ENERGYBEAM		(1 << 10)	// laser or other high energy beam 
#define DMG_NEVERGIB		(1 << 12)	// with this bit OR'd in, no damage type will be able to gib victims upon death
#define DMG_ALWAYSGIB		(1 << 13)	// with this bit OR'd in, any damage type can be made to gib victims upon death.
#define DMG_DROWN			(1 << 14)	// Drowning

// time-based damage
#define DMG_TIMEBASED		(DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN | DMG_SLOWFREEZE)	// mask for time-based damage

#define DMG_PARALYZE		(1 << 15)	// slows affected creature down
#define DMG_NERVEGAS		(1 << 16)	// nerve toxins, very bad
#define DMG_POISON			(1 << 17)	// blood poisoning - heals over time like drowning damage
#define DMG_RADIATION		(1 << 18)	// radiation exposure
#define DMG_DROWNRECOVER	(1 << 19)	// drowning recovery
#define DMG_ACID			(1 << 20)	// toxic chemicals or acid burns
#define DMG_SLOWBURN		(1 << 21)	// in an oven
#define DMG_SLOWFREEZE		(1 << 22)	// in a subzero freezer

//--------------
// HL2 SPECIFIC
//--------------
#define DMG_ARMOR_PIERCING	(1 << 23)	// Hit by an armor piercing round
#define DMG_SNIPER			(1 << 25)	// This is sniper damage
#define DMG_BUCKSHOT		(1 << 26)	// not quite a bullet. Little, rounder, different.
#define DMG_MISSILEDEFENSE	(1 << 27)	// The only kind of damage missiles take. (special missile defense)
#define DMG_PLASMA			(1 << 28)	// Shot by Cremator
#define DMG_PHYSGUN			(1 << 29)	// Hit by manipulator. Usually doesn't do any damage.

#define DMG_REMOVENORAGDOLL	(1 << 30)	// with this bit OR'd in, no ragdoll will be created, and the target will be quietly removed.
										// use this to kill an entity that you've already got a server-side ragdoll for

//--------------
// TF2 SPECIFIC
//--------------
#define DMG_EMP				(1 << 30)	// Hit by EMP
#define DMG_PROBE			(1 << 31)	// Doing a shield-aware probe (heal guns, emp guns)


// these are the damage types that are allowed to gib corpses
#define DMG_GIB_CORPSE		( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB )

// these are the damage types that have client hud art
#define DMG_SHOWNHUD		(DMG_POISON | DMG_ACID | DMG_SLOWFREEZE | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK)

// these are the damage types that don't have to supply a physics force & position
#define DMG_NO_PHYSICS_FORCE	(DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | DMG_TIMEBASED | DMG_CRUSH | DMG_PHYSGUN)

// settings for m_takedamage
#define	DAMAGE_NO				0
#define DAMAGE_EVENTS_ONLY		1		// Call damage functions, but don't modify health
#define	DAMAGE_YES				2
#define	DAMAGE_AIM				3

// Spectator Movement modes
#define OBS_MODE_NONE				0	// not in spectator mode
#define OBS_MODE_FIXED				1	// view from a fixed camera position
#define OBS_MODE_IN_EYE				2	// follow a player in first perosn view
#define OBS_MODE_CHASE				3	// follow a player in third person view
#define OBS_MODE_ROAMING			4	// free roaming

// Force Camera Restrictions with mp_deadspecmode
#define OBS_ALLOW_ALL			0	// allow all modes, all targets
#define OBS_ALLOW_TEAM			1	// allow only own team & first person, no PIP
#define OBS_ALLOW_NONE			2	// don't allow any spectating after death (fixed & fade to black)

// VGui Screen Flags
enum
{
	VGUI_SCREEN_ACTIVE = 0x1,
	VGUI_SCREEN_VISIBLE_TO_TEAMMATES = 0x2,
	VGUI_SCREEN_ATTACHED_TO_VIEWMODEL=0x4,

	VGUI_SCREEN_MAX_BITS = 3
};

typedef enum
{
	USE_OFF = 0, 
	USE_ON = 1, 
	USE_SET = 2, 
	USE_TOGGLE = 3
} USE_TYPE;

// All NPCs need this data
#define		DONT_BLEED			-1
#define		BLOOD_COLOR_RED		(byte)247
#define		BLOOD_COLOR_YELLOW	(byte)195
#define		BLOOD_COLOR_GREEN	BLOOD_COLOR_YELLOW
#define		BLOOD_COLOR_MECH	(byte)20

//-----------------------------------------------------------------------------
// Vehicles may have more than one passenger.
// This enum may be expanded by derived classes
//-----------------------------------------------------------------------------
enum PassengerRole_t
{
	VEHICLE_DRIVER = 0,		// There can be only one

	VEHICLE_LAST_COMMON_ROLE,
};

enum ExplosionEffect_t
{
	FEXPLOSION_NONE = 0,
	FEXPLOSION_EAR_RINGING = 1,
	FEXPLOSION_SHOCK = 2,
};

// Shared think context stuff
#define	MAX_CONTEXT_LENGTH		32
#define NO_THINK_CONTEXT	-1

// entity flags, CBaseEntity::m_iEFlags
enum
{
	EFL_KILLME	=				(1<<0),	// This entity is marked for death -- This allows the game to actually delete ents at a safe time
	EFL_DORMANT	=				(1<<1),	// Entity is dormant, no updates to client
	EFL_NOCLIP_ACTIVE =			(1<<2),	// Lets us know when the noclip command is active.

	EFL_NOTIFY =				(1<<6),	// Another entity is watching events on this entity (used by teleport)

	// The default behavior in ShouldTransmit is to not send an entity if it doesn't
	// have a model. Certain entities want to be sent anyway because all the drawing logic
	// is in the client DLL. They can set this flag and the engine will transmit them even
	// if they don't have a model.
	EFL_FORCE_CHECK_TRANSMIT =	(1<<7),

	EFL_BOT_FROZEN =			(1<<8),	// This is set on bots that are frozen.
	EFL_SERVER_ONLY =			(1<<9),	// Non-networked entity.

	// Some dirty bits with respect to abs computations
	EFL_DIRTY_ABSTRANSFORM =	(1<<10),
	EFL_DIRTY_ABSVELOCITY =		(1<<11),
	EFL_DIRTY_ABSANGVELOCITY =	(1<<12),

	EFL_IN_SKYBOX =				(1<<13)	// This is set if the entity detects that it's in the skybox.
										// This forces it to pass the "in PVS" for transmission.
};

//-----------------------------------------------------------------------------
// EFFECTS
//-----------------------------------------------------------------------------
const int FX_BLOODSPRAY_DROPS	= 0x01;
const int FX_BLOODSPRAY_GORE	= 0x02;
const int FX_BLOODSPRAY_CLOUD	= 0x04;
const int FX_BLOODSPRAY_ALL		= 0xFF;

//-----------------------------------------------------------------------------
#define MAX_SCREEN_OVERLAYS		10

#endif // SHAREDDEFS_H
