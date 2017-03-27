//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Joystick handling function
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include <windows.h>
#include "cdll_int.h"
#include "cdll_util.h"
#include "kbutton.h"
#include "usercmd.h"
#include "keydefs.h"
#include "input.h"
#include "iviewrender.h"
#include "convar.h"
#include "vstdlib/icommandline.h"

// Control like a joystick
#define JOY_ABSOLUTE_AXIS	0x00000000		
// Control like a mouse, spinner, trackball
#define JOY_RELATIVE_AXIS	0x00000010		

// Joystick info from windows multimedia system.
static JOYINFOEX ji;

static ConVar joy_name( "joyname", "joystick", 0 );
static ConVar joy_advanced( "joyadvanced", "0", 0 );
static ConVar joy_advaxisx( "joyadvaxisx", "0", 0 );
static ConVar joy_advaxisy( "joyadvaxisy", "0", 0 );
static ConVar joy_advaxisz( "joyadvaxisz", "0", 0 );
static ConVar joy_advaxisr( "joyadvaxisr", "0", 0 );
static ConVar joy_advaxisu( "joyadvaxisu", "0", 0 );
static ConVar joy_advaxisv( "joyadvaxisv", "0", 0 );
static ConVar joy_forwardthreshold( "joyforwardthreshold", "0.15", 0 );
static ConVar joy_sidethreshold( "joysidethreshold", "0.15", 0 );
static ConVar joy_pitchthreshold( "joypitchthreshold", "0.15", 0 );
static ConVar joy_yawthreshold( "joyyawthreshold", "0.15", 0 );
static ConVar joy_forwardsensitivity( "joyforwardsensitivity", "-1", 0 );
static ConVar joy_sidesensitivity( "joysidesensitivity", "-1", 0 );
static ConVar joy_pitchsensitivity( "joypitchsensitivity", "1", 0 );
static ConVar joy_yawsensitivity( "joyyawsensitivity", "-1", 0 );
static ConVar joy_wwhack1( "joywwhack1", "0", 0 );
static ConVar joy_wwhack2( "joywwhack2", "0", 0 );

extern ConVar lookspring;
extern ConVar cl_forwardspeed;
extern ConVar lookstrafe;
extern ConVar in_joystick;
extern ConVar m_pitch;
extern ConVar l_pitchspeed;
extern ConVar cl_sidespeed;
extern ConVar cl_yawspeed;
extern ConVar cl_pitchdown;
extern ConVar cl_pitchup;
extern ConVar cl_pitchspeed;
/* 
=============== 
Init_Joystick 
=============== 
*/  
void CInput::Init_Joystick( void ) 
{ 
	int			numdevs;
	JOYCAPS		jc;
	MMRESULT	mmr;
 
	m_rgAxes[ JOY_AXIS_X ].AxisFlags = JOY_RETURNX;
	m_rgAxes[ JOY_AXIS_Y ].AxisFlags = JOY_RETURNY;
	m_rgAxes[ JOY_AXIS_Z ].AxisFlags = JOY_RETURNZ;
	m_rgAxes[ JOY_AXIS_R ].AxisFlags = JOY_RETURNR;
	m_rgAxes[ JOY_AXIS_U ].AxisFlags = JOY_RETURNU;
	m_rgAxes[ JOY_AXIS_V ].AxisFlags = JOY_RETURNV;

 	// assume no joystick
	m_fJoystickAvailable = 0; 

	// abort startup if user requests no joystick
	if ( CommandLine()->FindParm("-nojoy" ) ) 
		return; 
 
	// verify joystick driver is present
	if ((numdevs = joyGetNumDevs ()) == 0)
	{
		// DevMsg( 1, "joystick not found -- driver not present\n\n");
		return;
	}

	// cycle through the joystick ids for the first valid one
	mmr = JOYERR_NOERROR+1; // Error if nothing gets assigned..
	for (m_nJoystickID=0 ; m_nJoystickID<numdevs ; m_nJoystickID++)
	{
		memset (&ji, 0, sizeof(ji));
		ji.dwSize = sizeof(ji);
		ji.dwFlags = JOY_RETURNCENTERED;

		if ((mmr = joyGetPosEx (m_nJoystickID, &ji)) == JOYERR_NOERROR)
			break;
	} 

	// abort startup if we didn't find a valid joystick
	if (mmr != JOYERR_NOERROR)
	{
		return;
	}

	// get the capabilities of the selected joystick
	// abort startup if command fails
	memset (&jc, 0, sizeof(jc));
	if ((mmr = joyGetDevCaps (m_nJoystickID, &jc, sizeof(jc))) != JOYERR_NOERROR)
	{
		DevWarning(1,"joystick not found -- invalid joystick capabilities (%x)\n\n", mmr); 
		return;
	}

	// save the joystick's number of buttons and POV status
	m_nJoystickButtons = (int)jc.wNumButtons;
	m_fJoystickHasPOVControl = jc.wCaps & JOYCAPS_HASPOV;

	// old button and POV states default to no buttons pressed
	m_nJoystickOldButtons	= 0;
	m_nJoystickOldPOVState	= 0;

	// mark the joystick as available and advanced initialization not completed
	// this is needed as cvars are not available during initialization
	Msg ("joystick found\n\n", mmr); 
	m_fJoystickAvailable = 1; 
	m_fJoystickAdvancedInit = 0;
}


/*
===========
RawValuePointer
===========
*/
unsigned long *CInput::RawValuePointer( int axis )
{
	switch (axis)
	{
	case JOY_AXIS_X:
		return &ji.dwXpos;
	case JOY_AXIS_Y:
		return &ji.dwYpos;
	case JOY_AXIS_Z:
		return &ji.dwZpos;
	case JOY_AXIS_R:
		return &ji.dwRpos;
	case JOY_AXIS_U:
		return &ji.dwUpos;
	case JOY_AXIS_V:
		return &ji.dwVpos;
	}
	// FIX: need to do some kind of error
	return &ji.dwXpos;
}


/*
===========
Joystick_Advanced
===========
*/
void CInput::Joystick_Advanced(void)
{

	// called once by IN_ReadJoystick and by user whenever an update is needed
	// cvars are now available
	int	i;
	DWORD dwTemp;

	// initialize all the maps
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		m_rgAxes[i].AxisMap = AxisNada;
		m_rgAxes[i].ControlMap = JOY_ABSOLUTE_AXIS;
		m_rgAxes[i].pRawValue = RawValuePointer(i);
	}

	if( joy_advanced.GetInt() == 0 )
	{
		// default joystick initialization
		// 2 axes only with joystick control
		m_rgAxes[JOY_AXIS_X].AxisMap = AxisTurn;
		m_rgAxes[JOY_AXIS_Y].AxisMap = AxisForward;
	}
	else
	{
		if ( strcmp ( joy_name.GetString(), "joystick") != 0 )
		{
			// notify user of advanced controller
			Msg ("\n%s configured\n\n", joy_name.GetString());
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwTemp = (DWORD) joy_advaxisx.GetInt();
		m_rgAxes[JOY_AXIS_X].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_X].ControlMap = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisy.GetInt();
		m_rgAxes[JOY_AXIS_Y].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_Y].ControlMap = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisz.GetInt();
		m_rgAxes[JOY_AXIS_Z].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_Z].ControlMap = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisr.GetInt();
		m_rgAxes[JOY_AXIS_R].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_R].ControlMap = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisu.GetInt();
		m_rgAxes[JOY_AXIS_U].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_U].ControlMap = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisv.GetInt();
		m_rgAxes[JOY_AXIS_V].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_V].ControlMap = dwTemp & JOY_RELATIVE_AXIS;
	}

	// compute the axes to collect from DirectInput
	m_nJoystickFlags = JOY_RETURNCENTERED | JOY_RETURNBUTTONS | JOY_RETURNPOV;
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		if (m_rgAxes[i].AxisMap != AxisNada)
		{
			m_nJoystickFlags |= m_rgAxes[i].AxisFlags;
		}
	}
}


/*
===========
ControllerCommands
===========
*/
void CInput::ControllerCommands( void )
{
	int		i, key_index;
	DWORD	buttonstate, povstate;

	if (!m_fJoystickAvailable)
	{
		return;
	}

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	buttonstate = ji.dwButtons;
	for (i=0 ; i < m_nJoystickButtons ; i++)
	{
		if ( (buttonstate & (1<<i)) && !(m_nJoystickOldButtons & (1<<i)) )
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			engine->Key_Event (key_index + i, 1);
		}

		if ( !(buttonstate & (1<<i)) && (m_nJoystickOldButtons & (1<<i)) )
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			engine->Key_Event (key_index + i, 0);
		}
	}
	m_nJoystickOldButtons = (unsigned int)buttonstate;

	if (m_fJoystickHasPOVControl)
	{
		// convert POV information into 4 bits of state information
		// this avoids any potential problems related to moving from one
		// direction to another without going through the center position
		povstate = 0;
		if(ji.dwPOV != JOY_POVCENTERED)
		{
			if (ji.dwPOV == JOY_POVFORWARD)
				povstate |= 0x01;
			if (ji.dwPOV == JOY_POVRIGHT)
				povstate |= 0x02;
			if (ji.dwPOV == JOY_POVBACKWARD)
				povstate |= 0x04;
			if (ji.dwPOV == JOY_POVLEFT)
				povstate |= 0x08;
		}
		// determine which bits have changed and key an auxillary event for each change
		for (i=0 ; i < 4 ; i++)
		{
			if ( (povstate & (1<<i)) && !(m_nJoystickOldPOVState & (1<<i)) )
			{
				engine->Key_Event (K_AUX29 + i, 1);
			}

			if ( !(povstate & (1<<i)) && (m_nJoystickOldPOVState & (1<<i)) )
			{
				engine->Key_Event (K_AUX29 + i, 0);
			}
		}
		m_nJoystickOldPOVState = (unsigned int)povstate;
	}
}


/* 
=============== 
ReadJoystick
=============== 
*/  
int CInput::ReadJoystick (void)
{
	memset (&ji, 0, sizeof(ji));
	ji.dwSize = sizeof(ji);
	ji.dwFlags = (DWORD)m_nJoystickFlags;

	if (joyGetPosEx (m_nJoystickID, &ji) == JOYERR_NOERROR)
	{
		// this is a hack -- there is a bug in the Logitech WingMan Warrior DirectInput Driver
		// rather than having 32768 be the zero point, they have the zero point at 32668
		// go figure -- anyway, now we get the full resolution out of the device
		if (joy_wwhack1.GetInt() != 0 )
		{
			ji.dwUpos += 100;
		}
		return 1;
	}
	else
	{
		// read error occurred
		// turning off the joystick seems too harsh for 1 read error,\
		// but what should be done?
		// Warning ("IN_ReadJoystick: no response\n");
		return 0;
	}
}


/*
===========
JoyStickMove
===========
*/
void CInput::JoyStickMove( float frametime, CUserCmd *cmd )
{
	float	speed, aspeed;
	float	fAxisValue, fTemp;
	int		i;
	QAngle viewangles;

	engine->GetViewAngles( viewangles );

	// complete initialization if first time in
	// this is needed as cvars are not available at initialization time
	if( m_fJoystickAdvancedInit != 1 )
	{
		Joystick_Advanced();
		m_fJoystickAdvancedInit = 1;
	}

	// verify joystick is available and that the user wants to use it
	if (!m_fJoystickAvailable || !in_joystick.GetInt() )
	{
		return; 
	}
 
	// collect the joystick data, if possible
	if ( ReadJoystick () != 1)
	{
		return;
	}

	speed = 1.0;
	aspeed = speed * frametime;

	// loop through the axes
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		// get the floating point zero-centered, potentially-inverted data for the current axis
		fAxisValue = (float) *m_rgAxes[i].pRawValue;
		// move centerpoint to zero
		fAxisValue -= 32768.0;

		if (joy_wwhack2.GetInt() != 0 )
		{
			if (m_rgAxes[i].AxisMap == AxisTurn)
			{
				// this is a special formula for the Logitech WingMan Warrior
				// y=ax^b; where a = 300 and b = 1.3
				// also x values are in increments of 800 (so this is factored out)
				// then bounds check result to level out excessively high spin rates
				fTemp = 300.0 * pow(abs(fAxisValue) / 800.0, 1.3);
				if (fTemp > 14000.0)
					fTemp = 14000.0;
				// restore direction information
				fAxisValue = (fAxisValue > 0.0) ? fTemp : -fTemp;
			}
		}

		// convert range from -32768..32767 to -1..1 
		fAxisValue /= 32768.0;

		switch ( m_rgAxes[i].AxisMap )
		{
		case AxisForward:
			if ((joy_advanced.GetInt() == 0) && (in_jlook.state & 1))
			{
				// user wants forward control to become look control
				if (fabs(fAxisValue) > joy_pitchthreshold.GetFloat())
				{		
					// if mouse invert is on, invert the joystick pitch value
					// only absolute control support here (joy_advanced is 0)
					if (m_pitch.GetFloat() < 0.0)
					{
						viewangles[PITCH] -= (fAxisValue * joy_pitchsensitivity.GetFloat()) * aspeed * cl_pitchspeed.GetFloat();
					}
					else
					{
						viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.GetFloat()) * aspeed * cl_pitchspeed.GetFloat();
					}
					view->StopPitchDrift();
				}
				else
				{
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if(lookspring.GetFloat() == 0.0)
					{
						view->StopPitchDrift();
					}
				}
			}
			else
			{
				// user wants forward control to be forward control
				if (fabs(fAxisValue) > joy_forwardthreshold.GetFloat())
				{
					cmd->forwardmove += (fAxisValue * joy_forwardsensitivity.GetFloat()) * speed * cl_forwardspeed.GetFloat();
				}
			}
			break;

		case AxisSide:
			if (fabs(fAxisValue) > joy_sidethreshold.GetFloat())
			{
				cmd->sidemove += (fAxisValue * joy_sidesensitivity.GetFloat()) * speed * cl_sidespeed.GetFloat();
			}
			break;

		case AxisTurn:
			if ((in_strafe.state & 1) || (lookstrafe.GetFloat() && (in_jlook.state & 1)))
			{
				// user wants turn control to become side control
				if (fabs(fAxisValue) > joy_sidethreshold.GetFloat())
				{
					cmd->sidemove -= (fAxisValue * joy_sidesensitivity.GetFloat()) * speed * cl_sidespeed.GetFloat();
				}
			}
			else
			{
				// user wants turn control to be turn control
				if (fabs(fAxisValue) > joy_yawthreshold.GetFloat())
				{
					if(m_rgAxes[i].ControlMap == JOY_ABSOLUTE_AXIS)
					{
						viewangles[YAW] += (fAxisValue * joy_yawsensitivity.GetFloat()) * aspeed * cl_yawspeed.GetFloat();
					}
					else
					{
						viewangles[YAW] += (fAxisValue * joy_yawsensitivity.GetFloat()) * speed * 180.0;
					}

				}
			}
			break;

		case AxisLook:
			if (in_jlook.state & 1)
			{
				if (fabs(fAxisValue) > joy_pitchthreshold.GetFloat())
				{
					// pitch movement detected and pitch movement desired by user
					if(m_rgAxes[i].ControlMap == JOY_ABSOLUTE_AXIS)
					{
						viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.GetFloat()) * aspeed * cl_pitchspeed.GetFloat();
					}
					else
					{
						viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity.GetFloat()) * speed * 180.0;
					}
					view->StopPitchDrift();
				}
				else
				{
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if( lookspring.GetFloat() == 0.0 )
					{
						view->StopPitchDrift();
					}
				}
			}
			break;

		default:
			break;
		}
	}

	// bounds check pitch
	if (viewangles[PITCH] > cl_pitchdown.GetFloat())
		viewangles[PITCH] = cl_pitchdown.GetFloat();
	if (viewangles[PITCH] < -cl_pitchup.GetFloat())
		viewangles[PITCH] = -cl_pitchup.GetFloat();

	engine->SetViewAngles( viewangles );

}

