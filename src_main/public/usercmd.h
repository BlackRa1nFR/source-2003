#if !defined( USERCMD_H )
#define USERCMD_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"
#include "utlvector.h"
#include "imovehelper.h"

#define FRAMETIME_BITPRECISION		8
#define FRAMETIME_ROUNDFACTOR		1000.0f  // 1 msec
#define MAX_USERCMD_FRAMETIME		0.25f

class bf_read;
class bf_write;

class CUserCmd
{
public:
	CUserCmd()
	{
		Reset();
	}

	virtual ~CUserCmd() { };

	void Reset()
	{
		command_number = 0;
		lerp_msec = 0.0f;
		msec = 0;
		frametime = 0.0f;
		viewangles.Init();
		forwardmove = 0.0f;
		sidemove = 0.0f;
		upmove = 0.0f;
		buttons = 0;
		impulse = 0;
		weaponselect = 0;
		weaponsubtype = 0;
		predict_weapons = true;
		lag_compensation = true;
		updaterate = 0;
		commandrate = 0;
		random_seed = 0;
		mousedx = 0;
		mousedy = 0;
	}

	CUserCmd& operator =( const CUserCmd& src )
	{
		if ( this == &src )
			return *this;

		command_number		= src.command_number;
		lerp_msec			= src.lerp_msec;
		msec				= src.msec;
		frametime			= src.frametime;
		viewangles			= src.viewangles;
		forwardmove			= src.forwardmove;
		sidemove			= src.sidemove;
		upmove				= src.upmove;
		buttons				= src.buttons;
		impulse				= src.impulse;
		weaponselect		= src.weaponselect;
		weaponsubtype		= src.weaponsubtype;
		predict_weapons		= src.predict_weapons;
		lag_compensation	= src.lag_compensation;
		updaterate			= src.updaterate;
		commandrate			= src.commandrate;
		random_seed			= src.random_seed;
		mousedx				= src.mousedx;
		mousedy				= src.mousedy;

		return *this;
	}

	// For matching server and client commands for debugging
	int		command_number;
	// Interpolation time on client
	float	lerp_msec;      
	// Duration of command in ms ( this is networked, frametime is not, but is
	//  recomputed on the receiving side from this )
	int		msec;
	// Duration of command ( max MAX_USERCMD_FRAMETIME )
	float	frametime;		
	// Player instantaneous view angles.
	QAngle	viewangles;     
	// Intended velocities
	//	forward velocity.
	float	forwardmove;   
	//  sideways velocity.
	float	sidemove;      
	//  upward velocity.
	float	upmove;         
	// Attack button states
	int		buttons;		
	// Impulse command issued.
	byte    impulse;        
	// Current weapon id
	int		weaponselect;	
	int		weaponsubtype;

	// 
	bool	predict_weapons;
	bool	lag_compensation;

	byte	updaterate;		// Player's packet rate from server (updates per second)
	byte	commandrate;	// Player's packet rate to server (updates per second)

	int		random_seed;	// For shared random functions

	short	mousedx;		// mouse accum in x from create move
	short	mousedy;		// mouse accum in y from create move
};

void ReadUsercmd( bf_read *buf, CUserCmd *move, CUserCmd *from );
void WriteUsercmd( bf_write *buf, CUserCmd *to, CUserCmd *from );

#endif // USERCMD_H
