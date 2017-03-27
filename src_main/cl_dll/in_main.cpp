//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: builds an intended movement command to send to the server
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "kbutton.h"
#include "usercmd.h"
#include "in_buttons.h"
#include "input.h"
#include "movevars_shared.h"
#include "iviewrender.h"
#include "iclientmode.h"
#include "prediction.h"
#include "bitbuf.h"
#include "vgui_BudgetPanel.h"
#include "prediction.h"

extern ConVar in_joystick;

// For showing/hiding the scoreboard
#include "vgui_ScorePanel.h"
#include "vgui_vprofpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// FIXME, tie to entity state parsing for player!!!
int g_iAlive = 1;

static int s_ClearInputState = 0;

// Defined in pm_math.c
float anglemod( float a );

// FIXME void V_Init( void );
static int in_impulse = 0;
static int in_cancel = 0;

ConVar cl_anglespeedkey( "cl_anglespeedkey", "0.67", 0 );
ConVar cl_yawspeed( "cl_yawspeed", "210", 0 );
ConVar cl_pitchspeed( "cl_pitchspeed", "225", 0 );
ConVar cl_pitchdown( "cl_pitchdown", "89", 0 );
ConVar cl_pitchup( "cl_pitchup", "89", 0 );
ConVar cl_sidespeed( "cl_sidespeed", "400", 0 );
ConVar cl_upspeed( "cl_upspeed", "320", FCVAR_ARCHIVE );
ConVar cl_forwardspeed( "cl_forwardspeed", "400", FCVAR_ARCHIVE );
ConVar cl_backspeed( "cl_backspeed", "400", FCVAR_ARCHIVE );
ConVar lookspring( "lookspring", "0", FCVAR_ARCHIVE );
ConVar lookstrafe( "lookstrafe", "0", FCVAR_ARCHIVE );
ConVar in_joystick( "joystick","0", FCVAR_ARCHIVE );


static ConVar cl_lagcomp_errorcheck( "cl_lagcomp_errorcheck", "0", 0, "Player index of other player to check for position errors." );

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/

kbutton_t	in_speed;
kbutton_t	in_walk;
kbutton_t	in_jlook;
kbutton_t	in_strafe;
kbutton_t	in_mlook;
kbutton_t	in_commandermousemove;
// Display the netgraph
kbutton_t	in_graph;  

static	kbutton_t	in_klook;
static	kbutton_t	in_left;
static	kbutton_t	in_right;
static	kbutton_t	in_forward;
static	kbutton_t	in_back;
static	kbutton_t	in_lookup;
static	kbutton_t	in_lookdown;
static	kbutton_t	in_moveleft;
static	kbutton_t	in_moveright;
static	kbutton_t	in_use;
static	kbutton_t	in_jump;
static	kbutton_t	in_attack;
static	kbutton_t	in_attack2;
static	kbutton_t	in_up;
static	kbutton_t	in_down;
static	kbutton_t	in_duck;
static	kbutton_t	in_reload;
static	kbutton_t	in_alt1;
static	kbutton_t	in_score;
static	kbutton_t	in_break;

/*
===========
IN_CenterView_f
===========
*/
void IN_CenterView_f (void)
{
	QAngle viewangles;

	if ( !::input->CAM_InterceptingMouse() )
	{
		engine->GetViewAngles( viewangles );
	    viewangles[PITCH] = 0;
		engine->SetViewAngles( viewangles );
	}
}

/*
===========
IN_Joystick_Advanced_f
===========
*/
void IN_Joystick_Advanced_f (void)
{
	::input->Joystick_Advanced();
}

/*
============
IN_IsDead

Returns 1 if health is <= 0
============
*/
static bool g_bDead = false;

bool IN_IsDead( void )
{
	return g_bDead;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dead - 
//-----------------------------------------------------------------------------
void IN_SetDead( bool dead )
{
	g_bDead = dead;
}

/*
============
KB_ConvertString

Removes references to +use and replaces them with the keyname in the output string.  If
 a binding is unfound, then the original text is retained.
NOTE:  Only works for text with +word in it.
============
*/
int KB_ConvertString( char *in, char **ppout )
{
	char sz[ 4096 ];
	char binding[ 64 ];
	char *p;
	char *pOut;
	char *pEnd;
	const char *pBinding;

	if ( !ppout )
		return 0;

	*ppout = NULL;
	p = in;
	pOut = sz;
	while ( *p )
	{
		if ( *p == '+' )
		{
			pEnd = binding;
			while ( *p && ( isalnum( *p ) || ( pEnd == binding ) ) && ( ( pEnd - binding ) < 63 ) )
			{
				*pEnd++ = *p++;
			}

			*pEnd =  '\0';

			pBinding = NULL;
			if ( strlen( binding + 1 ) > 0 )
			{
				// See if there is a binding for binding?
				pBinding = engine->Key_LookupBinding( binding + 1 );
			}

			if ( pBinding )
			{
				*pOut++ = '[';
				pEnd = (char *)pBinding;
			}
			else
			{
				pEnd = binding;
			}

			while ( *pEnd )
			{
				*pOut++ = *pEnd++;
			}

			if ( pBinding )
			{
				*pOut++ = ']';
			}
		}
		else
		{
			*pOut++ = *p++;
		}
	}

	*pOut = '\0';

	pOut = ( char * )malloc( strlen( sz ) + 1 );
	strcpy( pOut, sz );
	*ppout = pOut;

	return 1;
}

/*
==============================
FindKey

Allows the engine to request a kbutton handler by name, if the key exists.
==============================
*/
struct kbutton_s *CInput::FindKey( const char *name )
{
	CKeyboardKey *p;
	p = m_pKeys;
	while ( p )
	{
		if ( !stricmp( name, p->name ) )
			return p->pkey;

		p = p->next;
	}
	return NULL;
}

/*
============
AddKeyButton

Add a kbutton_t * to the list of pointers the engine can retrieve via KB_Find
============
*/
void CInput::AddKeyButton( const char *name, kbutton_t *pkb )
{
	CKeyboardKey *p;	
	kbutton_t *kb;

	kb = FindKey( name );
	
	if ( kb )
		return;

	p = new CKeyboardKey;

	strcpy( p->name, name );
	p->pkey = pkb;

	p->next = m_pKeys;
	m_pKeys = p;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInput::CInput( void )
{
	m_pCommands = NULL;
	m_pCmdRate = NULL;
	m_pUpdateRate = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInput::~CInput( void )
{
}

/*
============
Init_Keyboard

Add kbutton_t definitions that the engine can query if needed
============
*/
void CInput::Init_Keyboard( void )
{
	m_pKeys = NULL;

	AddKeyButton( "in_graph", &in_graph );
	AddKeyButton( "in_mlook", &in_mlook );
	AddKeyButton( "in_jlook", &in_jlook );
}

/*
============
Shutdown_Keyboard

Clear kblist
============
*/
void CInput::Shutdown_Keyboard( void )
{
	CKeyboardKey *p, *n;
	p = m_pKeys;
	while ( p )
	{
		n = p->next;
		delete p;
		p = n;
	}
	m_pKeys = NULL;
}

/*
============
KeyDown
============
*/
void KeyDown( kbutton_t *b, bool bIgnoreKey = false )
{
	int		k = -1;
	char	*c = NULL;

	if ( !bIgnoreKey )
	{
		c = engine->Cmd_Argv(1);
		if (c[0])
			k = atoi(c);
	}

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key
	
	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		if ( c[0] )
		{
			DevMsg( 1,"Three keys down for a button '%c' '%c' '%c'!\n", b->down[0], b->down[1], c);
		}
		return;
	}
	
	if (b->state & 1)
		return;		// still down
	b->state |= 1 + 2;	// down + impulse down
}

/*
============
KeyUp
============
*/
void KeyUp( kbutton_t *b, bool bIgnoreKey = false )
{
	int		k;
	char	*c;
	
	if ( bIgnoreKey )
	{
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	c = engine->Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
	{
		//Msg ("Keys down for button: '%c' '%c' '%c' (%d,%d,%d)!\n", b->down[0], b->down[1], c, b->down[0], b->down[1], c);
		return;		// some other key is still holding it down
	}

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void IN_CommanderMouseMoveDown() {KeyDown(&in_commandermousemove);}
void IN_CommanderMouseMoveUp() {KeyUp(&in_commandermousemove);}
void IN_BreakDown( void ) { KeyDown( &in_break );};
void IN_BreakUp( void )
{ 
	KeyUp( &in_break ); 
#if defined( _DEBUG )
	_asm
	{
		int 3;
	}
#endif
};
void IN_KLookDown (void) {KeyDown(&in_klook);}
void IN_KLookUp (void) {KeyUp(&in_klook);}
void IN_JLookDown (void) {KeyDown(&in_jlook);}
void IN_JLookUp (void) {KeyUp(&in_jlook);}
void IN_MLookDown (void) {KeyDown(&in_mlook);}
void IN_UpDown(void) {KeyDown(&in_up);}
void IN_UpUp(void) {KeyUp(&in_up);}
void IN_DownDown(void) {KeyDown(&in_down);}
void IN_DownUp(void) {KeyUp(&in_down);}
void IN_LeftDown(void) {KeyDown(&in_left);}
void IN_LeftUp(void) {KeyUp(&in_left);}
void IN_RightDown(void) {KeyDown(&in_right);}
void IN_RightUp(void) {KeyUp(&in_right);}
void IN_ForwardDown(void) {KeyDown(&in_forward);}
void IN_ForwardUp(void) {KeyUp(&in_forward);}
void IN_BackDown(void) {KeyDown(&in_back);}
void IN_BackUp(void) {KeyUp(&in_back);}
void IN_LookupDown(void) {KeyDown(&in_lookup);}
void IN_LookupUp(void) {KeyUp(&in_lookup);}
void IN_LookdownDown(void) {KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {KeyUp(&in_lookdown);}
void IN_MoveleftDown(void) {KeyDown(&in_moveleft);}
void IN_MoveleftUp(void) {KeyUp(&in_moveleft);}
void IN_MoverightDown(void) {KeyDown(&in_moveright);}
void IN_MoverightUp(void) {KeyUp(&in_moveright);}
void IN_WalkDown(void) {KeyDown(&in_walk);}
void IN_WalkUp(void) {KeyUp(&in_walk);}
void IN_SpeedDown(void) {KeyDown(&in_speed);}
void IN_SpeedUp(void) {KeyUp(&in_speed);}
void IN_StrafeDown(void) {KeyDown(&in_strafe);}
void IN_StrafeUp(void) {KeyUp(&in_strafe);}
void IN_Attack2Down(void) { KeyDown(&in_attack2);}
void IN_Attack2Up(void) {KeyUp(&in_attack2);}
void IN_UseDown (void) {KeyDown(&in_use);}
void IN_UseUp (void) {KeyUp(&in_use);}
void IN_JumpDown (void) {KeyDown(&in_jump);}
void IN_JumpUp (void) {KeyUp(&in_jump);}
void IN_DuckDown(void) {KeyDown(&in_duck);}
void IN_DuckUp(void) {KeyUp(&in_duck);}
void IN_ReloadDown(void) {KeyDown(&in_reload);}
void IN_ReloadUp(void) {KeyUp(&in_reload);}
void IN_Alt1Down(void) {KeyDown(&in_alt1);}
void IN_Alt1Up(void) {KeyUp(&in_alt1);}
void IN_GraphDown(void) {KeyDown(&in_graph);}
void IN_GraphUp(void) {KeyUp(&in_graph);}


void IN_AttackDown(void)
{
	KeyDown( &in_attack );
}

void IN_AttackUp(void)
{
	KeyUp( &in_attack );
	in_cancel = 0;
}

// Special handling
void IN_Cancel(void)
{
	in_cancel = 1;
}

void IN_Impulse (void)
{
	in_impulse = atoi( engine->Cmd_Argv(1) );
}

void IN_ScoreDown(void)
{
	KeyDown(&in_score);
	(GET_HUDELEMENT( ScorePanel ))->UserCmd_ShowScores();
}

void IN_ScoreUp(void)
{
	KeyUp(&in_score);
	(GET_HUDELEMENT( ScorePanel ))->UserCmd_HideScores();
}

void IN_VProfDown(void)
{
	(GET_HUDELEMENT( CVProfPanel ))->UserCmd_ShowVProf();
}

void IN_VProfUp(void)
{
	(GET_HUDELEMENT( CVProfPanel ))->UserCmd_HideVProf();
}

void IN_BudgetDown(void)
{
	(GET_HUDELEMENT( CBudgetPanel ))->UserCmd_ShowBudgetPanel();
}

void IN_BudgetUp(void)
{
	(GET_HUDELEMENT( CBudgetPanel ))->UserCmd_HideBudgetPanel();
}

void IN_MLookUp (void)
{
	KeyUp( &in_mlook );
	if ( !( in_mlook.state & 1 ) && ::input->GetLookSpring() )
	{
		view->StartPitchDrift();
	}
}

/*
============
KeyEvent

Return 1 to allow engine to process the key, otherwise, act on it as needed
============
*/
int CInput::KeyEvent( int down, int keynum, const char *pszCurrentBinding )
{
	if ( g_pClientMode )
		return g_pClientMode->KeyInput(down, keynum, pszCurrentBinding);

	return 1;
}



/*
===============
KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CInput::KeyState ( kbutton_t *key )
{
	float		val = 0.0;
	int			impulsedown, impulseup, down;
	
	impulsedown = key->state & 2;
	impulseup	= key->state & 4;
	down		= key->state & 1;
	
	if ( impulsedown && !impulseup )
	{
		// pressed and held this frame?
		val = down ? 0.5 : 0.0;
	}

	if ( impulseup && !impulsedown )
	{
		// released this frame?
		val = down ? 0.0 : 0.0;
	}

	if ( !impulsedown && !impulseup )
	{
		// held the entire frame?
		val = down ? 1.0 : 0.0;
	}

	if ( impulsedown && impulseup )
	{
		if ( down )
		{
			// released and re-pressed this frame
			val = 0.75;	
		}
		else
		{
			// pressed and released this frame
			val = 0.25;	
		}
	}

	// clear impulses
	key->state &= 1;		
	return val;
}

/*
==============================
DetermineKeySpeed

==============================
*/
float CInput::DetermineKeySpeed( float frametime )
{
	float speed;

	speed = frametime;

	if ( in_speed.state & 1 )
	{
		speed *= cl_anglespeedkey.GetFloat();
	}

	return speed;
}

/*
==============================
AdjustYaw

==============================
*/
void CInput::AdjustYaw( float speed, QAngle& viewangles )
{
	if ( !(in_strafe.state & 1) )
	{
		viewangles[YAW] -= speed*cl_yawspeed.GetFloat() * KeyState (&in_right);
		viewangles[YAW] += speed*cl_yawspeed.GetFloat() * KeyState (&in_left);
		viewangles[YAW] = anglemod(viewangles[YAW]);
	}
}

/*
==============================
AdjustPitch

==============================
*/
void CInput::AdjustPitch( float speed, QAngle& viewangles )
{
	float	up, down;

	if ( in_klook.state & 1 )
	{
		view->StopPitchDrift ();
		viewangles[PITCH] -= speed*cl_pitchspeed.GetFloat() * KeyState (&in_forward);
		viewangles[PITCH] += speed*cl_pitchspeed.GetFloat() * KeyState (&in_back);
	}

	up		= KeyState ( &in_lookup );
	down	= KeyState ( &in_lookdown );
	
	viewangles[PITCH] -= speed*cl_pitchspeed.GetFloat() * up;
	viewangles[PITCH] += speed*cl_pitchspeed.GetFloat() * down;

	if ( up || down )
	{
		view->StopPitchDrift ();
	}
}

/*
==============================
ClampAngles

==============================
*/
void CInput::ClampAngles( QAngle& viewangles )
{
	if ( viewangles[PITCH] > cl_pitchdown.GetFloat() )
	{
		viewangles[PITCH] = cl_pitchdown.GetFloat();
	}
	if ( viewangles[PITCH] < -cl_pitchup.GetFloat() )
	{
		viewangles[PITCH] = -cl_pitchup.GetFloat();
	}

	if ( viewangles[ROLL] > 50 )
	{
		viewangles[ROLL] = 50;
	}
	if ( viewangles[ROLL] < -50 )
	{
		viewangles[ROLL] = -50;
	}
}

/*
================
AdjustAngles

Moves the local angle positions
================
*/
void CInput::AdjustAngles ( float frametime )
{
	float	speed;
	QAngle viewangles;
	
	// Determine control scaling factor ( multiplies time )
	speed = DetermineKeySpeed( frametime );

	// Retrieve latest view direction from engine
	engine->GetViewAngles( viewangles );

	// Adjust YAW
	AdjustYaw( speed, viewangles );

	// Adjust PITCH if keyboard looking
	AdjustPitch( speed, viewangles );
	
	// Make sure values are legitimate
	ClampAngles( viewangles );

	// Store new view angles into engine view direction
	engine->SetViewAngles( viewangles );
}

/*
==============================
ComputeSideMove

==============================
*/
void CInput::ComputeSideMove( CUserCmd *cmd )
{
	if ( IsNoClipping() )
	{
		// If strafing, check left and right keys and act like moveleft and moveright keys
		if ( in_strafe.state & 1 )
		{
			cmd->sidemove += sv_noclipspeed.GetFloat() * KeyState (&in_right);
			cmd->sidemove -= sv_noclipspeed.GetFloat() * KeyState (&in_left);
		}

		// Otherwise, check strafe keys
		cmd->sidemove += sv_noclipspeed.GetFloat() * KeyState (&in_moveright);
		cmd->sidemove -= sv_noclipspeed.GetFloat() * KeyState (&in_moveleft);
	}
	else
	{
		// If strafing, check left and right keys and act like moveleft and moveright keys
		if ( in_strafe.state & 1 )
		{
			cmd->sidemove += cl_sidespeed.GetFloat() * KeyState (&in_right);
			cmd->sidemove -= cl_sidespeed.GetFloat() * KeyState (&in_left);
		}

		// Otherwise, check strafe keys
		cmd->sidemove += cl_sidespeed.GetFloat() * KeyState (&in_moveright);
		cmd->sidemove -= cl_sidespeed.GetFloat() * KeyState (&in_moveleft);
	}
}

/*
==============================
ComputeUpwardMove

==============================
*/
void CInput::ComputeUpwardMove( CUserCmd *cmd )
{
	if ( IsNoClipping() )
	{
		cmd->upmove += sv_noclipspeed.GetFloat() * KeyState (&in_up);
		cmd->upmove -= sv_noclipspeed.GetFloat() * KeyState (&in_down);
	}
	else
	{
		cmd->upmove += cl_upspeed.GetFloat() * KeyState (&in_up);
		cmd->upmove -= cl_upspeed.GetFloat() * KeyState (&in_down);
	}
}

/*
==============================
ComputeForwardMove

==============================
*/
void CInput::ComputeForwardMove( CUserCmd *cmd )
{
	if ( !(in_klook.state & 1 ) )
	{	
		if ( IsNoClipping() )
		{
			cmd->forwardmove += sv_noclipspeed.GetFloat() * KeyState (&in_forward);
			cmd->forwardmove -= sv_noclipspeed.GetFloat() * KeyState (&in_back);
		}
		else
		{
			cmd->forwardmove += cl_forwardspeed.GetFloat() * KeyState (&in_forward);
			cmd->forwardmove -= cl_backspeed.GetFloat() * KeyState (&in_back);
		}
	}	
}

/*
==============================
ScaleMovements

==============================
*/
void CInput::ScaleMovements( CUserCmd *cmd )
{
	// float spd;

	// clip to maxspeed
	// FIXME FIXME:  This doesn't work
	return;

	/*
	spd = engine->GetClientMaxspeed();
	if ( spd == 0.0 )
		return;

	// Scale the speed so that the total velocity is not > spd
	float fmov = sqrt( (cmd->forwardmove*cmd->forwardmove) + (cmd->sidemove*cmd->sidemove) + (cmd->upmove*cmd->upmove) );

	if ( fmov > spd && fmov > 0.0 )
	{
		float fratio = spd / fmov;

		if ( !IsNoClipping() ) 
		{
			cmd->forwardmove	*= fratio;
			cmd->sidemove		*= fratio;
			cmd->upmove			*= fratio;
		}
	}
	*/
}


/*
===========
ControllerMove
===========
*/
void CInput::ControllerMove( float frametime, CUserCmd *cmd )
{
	if ( !m_fCameraInterceptingMouse && m_nMouseActive )
	{
		MouseMove ( cmd);
	}

	JoyStickMove ( frametime, cmd);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *weapon - 
//-----------------------------------------------------------------------------
void CInput::MakeWeaponSelection( C_BaseCombatWeapon *weapon )
{
	m_hSelectedWeapon = weapon;
}

/*
================
CreateMove

Send the intended movement message to the server
if active == 1 then we are 1) not playing back demos ( where our commands are ignored ) and
2 ) we have finished signing on to server
================
*/

void CInput::ExtraMouseSample( float frametime, int active )
{
	CUserCmd dummy;
	CUserCmd *cmd = &dummy;

	cmd->Reset();

	// Round off unused bits to msec
	cmd->msec = (int)( frametime * FRAMETIME_ROUNDFACTOR + 0.5f );

	cmd->msec = clamp( cmd->msec, 0, (int)( MAX_USERCMD_FRAMETIME * FRAMETIME_ROUNDFACTOR ) );

	// Convert back to floating point
	cmd->frametime = (float)cmd->msec / FRAMETIME_ROUNDFACTOR;

	// Clamp to range
	cmd->frametime = clamp( cmd->frametime, 0.0f, MAX_USERCMD_FRAMETIME );

	QAngle viewangles;

	if ( active )
	{
		// Determine view angles
		AdjustAngles ( frametime );

		// Determine sideways movement
		ComputeSideMove( cmd );

		// Determine vertical movement
		ComputeUpwardMove( cmd );

		// Determine forward movement
		ComputeForwardMove( cmd );

		// Scale based on holding speed key or having too fast of a velocity based on client maximum
		//  speed.
		ScaleMovements( cmd );

		// Allow mice and other controllers to add their inputs
		ControllerMove( frametime, cmd );
	}

	// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
	engine->GetViewAngles( viewangles );

	// Use new view angles if alive, otherwise user last angles we stored off.
	if ( g_iAlive )
	{
		VectorCopy( viewangles, cmd->viewangles );
		VectorCopy( viewangles, m_angPreviousViewAngles );
	}
	else
	{
		VectorCopy( m_angPreviousViewAngles, cmd->viewangles );
	}

	// Get current view angles after the client mode tweaks with it
	engine->SetViewAngles( cmd->viewangles );

	prediction->SetLocalViewAngles( cmd->viewangles );
}

void CInput::CreateMove ( int command_number, int totalslots, float tick_frametime, float input_sample_frametime, int active, float packet_loss )
{	
	int slot = command_number & ( totalslots - 1 );

	Assert( totalslots <= MULTIPLAYER_BACKUP );
	Assert( slot >= 0 && slot < totalslots );
	Assert( m_pCommands );

	CUserCmd *cmd = &m_pCommands[ slot ];

	cmd->Reset();

	cmd->command_number = command_number;

	// Round off unused bits to msec
	cmd->msec = (int)( tick_frametime * FRAMETIME_ROUNDFACTOR + 0.5f );

	cmd->msec = clamp( cmd->msec, 0, (int)( MAX_USERCMD_FRAMETIME * FRAMETIME_ROUNDFACTOR ) );

	// Convert back to floating point
	cmd->frametime = (float)cmd->msec / FRAMETIME_ROUNDFACTOR;

	// Clamp to range
	cmd->frametime = clamp( cmd->frametime, 0.0f, MAX_USERCMD_FRAMETIME );

	QAngle viewangles;

	if ( active )
	{
		// Determine view angles
		AdjustAngles ( input_sample_frametime );

		// Determine sideways movement
		ComputeSideMove( cmd );

		// Determine vertical movement
		ComputeUpwardMove( cmd );

		// Determine forward movement
		ComputeForwardMove( cmd );

		// Scale based on holding speed key or having too fast of a velocity based on client maximum
		//  speed.
		ScaleMovements( cmd );

		// Allow mice and other controllers to add their inputs
		ControllerMove( input_sample_frametime, cmd );
	}

	// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
	engine->GetViewAngles( viewangles );

	// Latch and clear impulse
	cmd->impulse = in_impulse;
	in_impulse = 0;

	// Latch and clear weapon selection
	if ( m_hSelectedWeapon != NULL )
	{
		C_BaseCombatWeapon *weapon = m_hSelectedWeapon;

		cmd->weaponselect = weapon->entindex();
		cmd->weaponsubtype = weapon->GetSubType();

		// Always clear weapon selection
		m_hSelectedWeapon = NULL;
	}

	// Set button and flag bits
	cmd->buttons = GetButtonBits( 1 );

	// Using joystick?
	if ( in_joystick.GetInt() )
	{
		if ( cmd->forwardmove > 0 )
		{
			cmd->buttons |= IN_FORWARD;
		}
		else if ( cmd->forwardmove < 0 )
		{
			cmd->buttons |= IN_BACK;
		}
	}

	// Use new view angles if alive, otherwise user last angles we stored off.
	if ( g_iAlive )
	{
		VectorCopy( viewangles, cmd->viewangles );
		VectorCopy( viewangles, m_angPreviousViewAngles );
	}
	else
	{
		VectorCopy( m_angPreviousViewAngles, cmd->viewangles );
	}

	// Let the move manager override anything it wants to.
	g_pClientMode->CreateMove( tick_frametime, input_sample_frametime, cmd );

	// Get current view angles after the client mode tweaks with it
	engine->SetViewAngles( cmd->viewangles );

	m_fLastForwardMove = cmd->forwardmove;

	// Get value of cl_interp cvar
	cmd->lerp_msec = ( render->GetInterpolationTime() * 1000.0f );
	cmd->lerp_msec = clamp( cmd->lerp_msec, 0.0f, 250.0f );

	cmd->predict_weapons = cl_predict.GetBool() && cl_predictweapons.GetBool();
	cmd->lag_compensation = cl_predict.GetBool() && cl_lagcompensation.GetBool();

	cmd->commandrate = clamp( m_pCmdRate ? m_pCmdRate->GetInt() : 30, 1, 255 );
	cmd->updaterate = clamp( m_pUpdateRate ? m_pUpdateRate->GetInt() : 20, 1, 255 );

	cmd->random_seed = random->RandomInt( 0, 0x7fffffff );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			buffersize - 
//			slot - 
//-----------------------------------------------------------------------------
void CInput::EncodeUserCmdToBuffer( bf_write& buf, int buffersize, int slot )
{
	CUserCmd nullcmd;
	CUserCmd *cmd = GetUsercmd( slot );

	WriteUsercmd( &buf, cmd, &nullcmd );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			buffersize - 
//			slot - 
//-----------------------------------------------------------------------------
void CInput::DecodeUserCmdFromBuffer( bf_read& buf, int buffersize, int slot )
{
	CUserCmd nullcmd;
	CUserCmd *cmd = GetUsercmd( slot );

	ReadUsercmd( &buf, cmd, &nullcmd );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buf - 
//			from - 
//			to - 
//-----------------------------------------------------------------------------
void CInput::WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand )
{
	Assert( m_pCommands );
	Assert( from == -1 || ( from >= 0 && from < MULTIPLAYER_BACKUP ) );
	Assert( to >= 0 && to < MULTIPLAYER_BACKUP );

	CUserCmd nullcmd;

	CUserCmd *f, *t;

	if ( from == -1 )
	{
		f = &nullcmd;
	}
	else
	{
		f = GetUsercmd( from );
	}

	t = GetUsercmd( to );

	// Write it into the buffer
	WriteUsercmd( buf, t, f );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : slot - 
// Output : CUserCmd
//-----------------------------------------------------------------------------
CUserCmd *CInput::GetUsercmd( int slot )
{
	Assert( m_pCommands );
	Assert( slot >= 0 && slot < MULTIPLAYER_BACKUP );
	return &m_pCommands[ slot ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bits - 
//			in_button - 
//			in_ignore - 
//			*button - 
//			reset - 
// Output : static void
//-----------------------------------------------------------------------------
static void CalcButtonBits( int& bits, int in_button, int in_ignore, kbutton_t *button, bool reset )
{
	// Down or still down?
	if ( button->state & 3 )
	{
		bits |= in_button;
	}

	int clearmask = ~2;
	if ( in_ignore & in_button )
	{
		// This gets taken care of below in the GetButtonBits code
		//bits &= ~in_button;
		// Remove "still down" as well as "just down"
		clearmask = ~3;
	}

	if ( reset )
	{
		button->state &= clearmask;
	}
}

/*
============
GetButtonBits

Returns appropriate button info for keyboard and mouse state
Set bResetState to 1 to clear old state info
============
*/
int CInput::GetButtonBits( int bResetState )
{
	int bits = 0;

	CalcButtonBits( bits, IN_SPEED, s_ClearInputState, &in_speed, bResetState );
	CalcButtonBits( bits, IN_WALK, s_ClearInputState, &in_walk, bResetState );
	CalcButtonBits( bits, IN_ATTACK, s_ClearInputState, &in_attack, bResetState );
	CalcButtonBits( bits, IN_DUCK, s_ClearInputState, &in_duck, bResetState );
	CalcButtonBits( bits, IN_JUMP, s_ClearInputState, &in_jump, bResetState );
	CalcButtonBits( bits, IN_FORWARD, s_ClearInputState, &in_forward, bResetState );
	CalcButtonBits( bits, IN_BACK, s_ClearInputState, &in_back, bResetState );
	CalcButtonBits( bits, IN_USE, s_ClearInputState, &in_use, bResetState );
	CalcButtonBits( bits, IN_LEFT, s_ClearInputState, &in_left, bResetState );
	CalcButtonBits( bits, IN_RIGHT, s_ClearInputState, &in_right, bResetState );
	CalcButtonBits( bits, IN_MOVELEFT, s_ClearInputState, &in_moveleft, bResetState );
	CalcButtonBits( bits, IN_MOVERIGHT, s_ClearInputState, &in_moveright, bResetState );
	CalcButtonBits( bits, IN_ATTACK2, s_ClearInputState, &in_attack2, bResetState );
	CalcButtonBits( bits, IN_RELOAD, s_ClearInputState, &in_reload, bResetState );
	CalcButtonBits( bits, IN_ALT1, s_ClearInputState, &in_alt1, bResetState );
	CalcButtonBits( bits, IN_SCORE, s_ClearInputState, &in_score, bResetState );

	// Cancel is a special flag
	if (in_cancel)
	{
		bits |= IN_CANCEL;
	}

	// Dead? Shore scoreboard, too
	if ( IN_IsDead() )
	{
		bits |= IN_SCORE;
	}

	if ( gHUD.m_iKeyBits & IN_WEAPON1 )
	{
		bits |= IN_WEAPON1;
	}

	if ( gHUD.m_iKeyBits & IN_WEAPON2 )
	{
		bits |= IN_WEAPON2;
	}

	// Clear out any residual
	bits &= ~s_ClearInputState;

	if ( bResetState )
	{
		s_ClearInputState = 0;
	}

	return bits;
}


//-----------------------------------------------------------------------------
// Causes an input to have to be re-pressed to become active
//-----------------------------------------------------------------------------
void CInput::ClearInputButton( int bits )
{
	s_ClearInputState |= bits;
}


/*
==============================
GetLookSpring

==============================
*/
float CInput::GetLookSpring( void )
{
	return lookspring.GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CInput::IsNoClipping( void )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( player )
	{
		if ( player->GetMoveType() == MOVETYPE_NOCLIP )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CInput::GetLastForwardMove( void )
{
	return m_fLastForwardMove;
}

static ConCommand startcommandermousemove("+commandermousemove", IN_CommanderMouseMoveDown);
static ConCommand endcommandermousemove("-commandermousemove", IN_CommanderMouseMoveUp);
static ConCommand startmoveup("+moveup",IN_UpDown);
static ConCommand endmoveup("-moveup",IN_UpUp);
static ConCommand startmovedown("+movedown",IN_DownDown);
static ConCommand endmovedown("-movedown",IN_DownUp);
static ConCommand startleft("+left",IN_LeftDown);
static ConCommand endleft("-left",IN_LeftUp);
static ConCommand startright("+right",IN_RightDown);
static ConCommand endright("-right",IN_RightUp);
static ConCommand startforward("+forward",IN_ForwardDown);
static ConCommand endforward("-forward",IN_ForwardUp);
static ConCommand startback("+back",IN_BackDown);
static ConCommand endback("-back",IN_BackUp);
static ConCommand startlookup("+lookup", IN_LookupDown);
static ConCommand endlookup("-lookup", IN_LookupUp);
static ConCommand startlookdown("+lookdown", IN_LookdownDown);
static ConCommand lookdown("-lookdown", IN_LookdownUp);
static ConCommand startstrafe("+strafe", IN_StrafeDown);
static ConCommand endstrafe("-strafe", IN_StrafeUp);
static ConCommand startmoveleft("+moveleft", IN_MoveleftDown);
static ConCommand endmoveleft("-moveleft", IN_MoveleftUp);
static ConCommand startmoveright("+moveright", IN_MoverightDown);
static ConCommand endmoveright("-moveright", IN_MoverightUp);
static ConCommand startspeed("+speed", IN_SpeedDown);
static ConCommand endspeed("-speed", IN_SpeedUp);
static ConCommand startwalk("+walk", IN_WalkDown);
static ConCommand endwalk("-walk", IN_WalkUp);
static ConCommand startattack("+attack", IN_AttackDown);
static ConCommand endattack("-attack", IN_AttackUp);
static ConCommand startattack2("+attack2", IN_Attack2Down);
static ConCommand endattack2("-attack2", IN_Attack2Up);
static ConCommand startuse("+use", IN_UseDown);
static ConCommand enduse("-use", IN_UseUp);
static ConCommand startjump("+jump", IN_JumpDown);
static ConCommand endjump("-jump", IN_JumpUp);
static ConCommand impulse("impulse", IN_Impulse);
static ConCommand startklook("+klook", IN_KLookDown);
static ConCommand endklook("-klook", IN_KLookUp);
static ConCommand startmlook("+mlook", IN_MLookDown);
static ConCommand endmlook("-mlook", IN_MLookUp);
static ConCommand startjlook("+jlook", IN_JLookDown);
static ConCommand endjlook("-jlook", IN_JLookUp);
static ConCommand startduck("+duck", IN_DuckDown);
static ConCommand endduck("-duck", IN_DuckUp);
static ConCommand startreload("+reload", IN_ReloadDown);
static ConCommand endreload("-reload", IN_ReloadUp);
static ConCommand startalt1("+alt1", IN_Alt1Down);
static ConCommand endalt1("-alt1", IN_Alt1Up);
static ConCommand startscore("+score", IN_ScoreDown);
static ConCommand endscore("-score", IN_ScoreUp);
static ConCommand startshowscores("+showscores", IN_ScoreDown);
static ConCommand endshowscores("-showscores", IN_ScoreUp);
static ConCommand startshowvprof("+showvprof", IN_VProfDown);
static ConCommand endshowvprof("-showvprof", IN_VProfUp);
static ConCommand startshowbudget("+showbudget", IN_BudgetDown);
static ConCommand endshowbudget("-showbudget", IN_BudgetUp);
static ConCommand startgraph("+graph", IN_GraphDown);
static ConCommand endgraph("-graph", IN_GraphUp);
static ConCommand startbreak("+break",IN_BreakDown);
static ConCommand endbreak("-break",IN_BreakUp);
static ConCommand force_centerview("force_centerview", IN_CenterView_f);
static ConCommand joyadvancedupdate("joyadvancedupdate", IN_Joystick_Advanced_f);

/*
============
Init_All
============
*/
void CInput::Init_All (void)
{
	Assert( !m_pCommands );
	m_pCommands = new CUserCmd[ MULTIPLAYER_BACKUP ];

	m_nMouseInitialized	= 0;
	m_fRestoreSPI		= 0;
	m_nMouseActive		= 0;
	memset( m_rgOrigMouseParms, 0, sizeof( m_rgOrigMouseParms ) );
	memset( m_rgNewMouseParms, 0, sizeof( m_rgNewMouseParms ) );
	m_rgNewMouseParms[ 2 ] = 1;
	m_fMouseParmsValid	= 0;
	m_fJoystickAvailable = 0;
	m_fJoystickAdvancedInit = 0;
	m_fJoystickHasPOVControl = 0;
	m_fLastForwardMove = 0.0;

	// Initialize inputs
	Init_Mouse ();
	Init_Joystick ();
	Init_Keyboard();

	// Initialize third person camera controls.
	Init_Camera();

	m_pCmdRate = cvar->FindVar( "cl_cmdrate" );
	m_pUpdateRate = cvar->FindVar( "cl_updaterate" );
}

/*
============
Shutdown_All
============
*/
void CInput::Shutdown_All(void)
{
	DeactivateMouse();
	Shutdown_Keyboard();

	delete[] m_pCommands;
	m_pCommands = NULL;
}

