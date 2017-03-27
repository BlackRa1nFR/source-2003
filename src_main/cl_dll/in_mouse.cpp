//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Mouse input routines
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include <windows.h>

#include "hud.h"
#include "cdll_int.h"
#include "cdll_util.h"
#include "kbutton.h"
#include "usercmd.h"
#include "keydefs.h"
#include "input.h"
#include "iviewrender.h"
#include "iclientmode.h"
#include "vstdlib/icommandline.h"
#include "vgui/isurface.h"
#include "vgui_controls/controls.h"
#include "vgui/cursor.h"

// up / down
#define	PITCH	0
// left / right
#define	YAW		1

extern ConVar lookstrafe;
extern ConVar cl_pitchdown;
extern ConVar cl_pitchup;

ConVar m_pitch( "m_pitch","0.022", FCVAR_ARCHIVE, "Mouse pitch factor." );

static ConVar m_filter( "m_filter","0", FCVAR_ARCHIVE, "Mouse filtering (set this to 1 to average the mouse over 2 frames)." );
ConVar sensitivity( "sensitivity","3", FCVAR_ARCHIVE, "Mouse sensitivity.", true, 0.1f, true, 100.0f );

static ConVar m_side( "m_side","0.8", FCVAR_ARCHIVE, "Mouse side factor." );
static ConVar m_yaw( "m_yaw","0.022", FCVAR_ARCHIVE, "Mouse yaw factor." );
static ConVar m_forward( "m_forward","1", FCVAR_ARCHIVE, "Mouse forward factor." );

/*
===========
ActivateMouse
===========
*/
void CInput::ActivateMouse (void)
{
	if ( m_nMouseActive == 1 )
		return;

	if (m_nMouseInitialized)
	{
		if (m_fMouseParmsValid)
		{
			m_fRestoreSPI = SystemParametersInfo (SPI_SETMOUSE, 0, m_rgNewMouseParms, 0);
		}
		m_nMouseActive = 1;

		ResetMouse();

		// Clear accumulated error, too
		m_nXAccum = 0;
		m_nYAccum = 0;
	}
}

/*
===========
DeactivateMouse
===========
*/
void CInput::DeactivateMouse (void)
{
	// This gets called whenever the mouse should be inactive. We only respond to it if we had 
	// previously activated the mouse. We'll show the cursor in here.
	if ( m_nMouseActive == 0 )
		return;

	if (m_nMouseInitialized)
	{
		if (m_fRestoreSPI)
		{
			SystemParametersInfo (SPI_SETMOUSE, 0, m_rgOrigMouseParms, 0);
		}
		m_nMouseActive = 0;
		vgui::surface()->SetCursor( vgui::dc_arrow );

		// Clear accumulated error, too
		m_nXAccum = 0;
		m_nYAccum = 0;
	}
}

/*
===========
Init_Mouse
===========
*/
void CInput::Init_Mouse (void)
{
	if ( CommandLine()->FindParm("-nomouse" ) ) 
		return; 

	m_nMouseInitialized = 1;

	// TODO:  Do we really want to slam the user's mouse accel setting each time we run?
	m_fMouseParmsValid = 0;

	/*
	m_fMouseParmsValid = SystemParametersInfo (SPI_GETMOUSE, 0, m_rgOrigMouseParms, 0);
	if (m_fMouseParmsValid)
	{
		if ( engine->CheckParm ("-noforcemspd", NULL ) ) 
			m_rgNewMouseParms[2] = m_rgOrigMouseParms[2];

		if ( engine->CheckParm ("-noforcemaccel", NULL ) ) 
		{
			m_rgNewMouseParms[0] = m_rgOrigMouseParms[0];
			m_rgNewMouseParms[1] = m_rgOrigMouseParms[1];
		}

		if ( engine->CheckParm ("-noforcemparms", NULL ) ) 
		{
			m_rgNewMouseParms[0] = m_rgOrigMouseParms[0];
			m_rgNewMouseParms[1] = m_rgOrigMouseParms[1];
			m_rgNewMouseParms[2] = m_rgOrigMouseParms[2];
		}
	}
	*/

	m_nMouseButtons = MOUSE_BUTTON_COUNT;
}

/*
===========
ResetMouse

===========
*/
void CInput::ResetMouse( void )
{
	SetMousePos ( engine->GetWindowCenterX(), engine->GetWindowCenterY() );	
}

/*
===========
MouseEvent
===========
*/
void CInput::MouseEvent( int mstate, bool down )
{
	// perform button actions
	for (int i=0 ; i<m_nMouseButtons ; i++)
	{
		// Mouse buttons 1 & 2 are swallowed when the mouse is visible
		if ( (i < 2) && ( m_fCameraInterceptingMouse || vgui::surface()->IsCursorVisible() ) )
			continue;

		// Only fire changed buttons
		if ( (mstate & (1<<i)) && !(m_nMouseOldButtons & (1<<i)) )
		{
			engine->Key_Event (K_MOUSE1 + i, down);
		}
		if ( !(mstate & (1<<i)) && (m_nMouseOldButtons & (1<<i)) )
		{
			// Force 0 instead of down, because MouseMove calls this with down set to true.
			engine->Key_Event (K_MOUSE1 + i, 0);
		}
	}	
	
	m_nMouseOldButtons = mstate;
}

/*
==============================
GetAccumulatedMouse

==============================
*/
void CInput::GetAccumulatedMouse( int *mx, int *my )
{
	*mx = m_nXAccum;
	*my = m_nYAccum;

	m_nXAccum = 0;
	m_nYAccum = 0;
}

/*
==============================
GetMouseDelta

==============================
*/
void CInput::GetMouseDelta( int mx, int my, int *oldx, int *oldy, int *x, int *y )
{
	if (m_filter.GetInt())
	{
		*x = ( mx + *oldx ) * 0.5;
		*y = ( my + *oldy ) * 0.5;
	}
	else
	{
		*x = mx;
		*y = my;
	}

	*oldx = mx;
	*oldy = my;

}

/*
==============================
ScaleMouse

==============================
*/
void CInput::ScaleMouse( int *x, int *y )
{
	if ( gHUD.GetSensitivity() != 0 )
	{
		*x *= gHUD.GetSensitivity();
		*y *= gHUD.GetSensitivity();
	}
	else
	{
		*x *= sensitivity.GetFloat();
		*y *= sensitivity.GetFloat();
	}
}

/*
==============================
ApplyMouse

==============================
*/
void CInput::ApplyMouse( QAngle& viewangles, CUserCmd *cmd, int mouse_x, int mouse_y )
{
	// If holding strafe key or mlooking and have lookstrafe set to true, then apply
	//  horizontal mouse movement to sidemove.
	if ( (in_strafe.state & 1) || (lookstrafe.GetInt() && (in_mlook.state & 1) ))
	{
		cmd->sidemove += m_side.GetFloat() * mouse_x;
	}
	else
	{
		// Otherwize, use mouse to spin around vertical axis
		viewangles[YAW] -= m_yaw.GetFloat() * mouse_x;
	}

	// If mouselooking and not holding strafe key, then use vertical mouse
	//  to adjust view pitch.
	if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
	{
		viewangles[PITCH] += m_pitch.GetFloat() * mouse_y;
		// Check pitch bounds
		if (viewangles[PITCH] > cl_pitchdown.GetFloat())
		{
			viewangles[PITCH] = cl_pitchdown.GetFloat();
		}
		if (viewangles[PITCH] < -cl_pitchup.GetFloat())
		{
			viewangles[PITCH] = -cl_pitchup.GetFloat();
		}
	}
	else
	{
		// Otherwise if holding strafe key and noclipping, then move upward
		if ((in_strafe.state & 1) && IsNoClipping() )
		{
			cmd->upmove -= m_forward.GetFloat() * mouse_y;
		}
		else
		{
			// Default is to apply vertical mouse movement as a forward key press.
			cmd->forwardmove -= m_forward.GetFloat() * mouse_y;
		}
	}

	// Add mouse state to usercmd.
	cmd->mousedx = mouse_x;
	cmd->mousedy = mouse_y;
}

/*
===========
AccumulateMouse
===========
*/
static ConVar cl_mouseenable( "cl_mouseenable", "1" );
void CInput::AccumulateMouse( void )
{
	if( !cl_mouseenable.GetBool() )
	{
		return;
	}
	//only accumulate mouse if we are not moving the camera with the mouse
	if ( !m_fCameraInterceptingMouse && vgui::surface()->IsCursorLocked() )
	{
		//Assert( !vgui::surface()->IsCursorVisible() );

		int current_posx, current_posy;

		GetMousePos(current_posx, current_posy);

		m_nXAccum += current_posx - engine->GetWindowCenterX();
		m_nYAccum += current_posy - engine->GetWindowCenterY();

		// force the mouse to the center, so there's room to move
		ResetMouse();
	}
}

void GetVGUICursorPos( int& x, int& y );
void SetVGUICursorPos( int x, int y );

void CInput::GetMousePos(int &ox, int &oy)
{
	GetVGUICursorPos( ox, oy );
}

void CInput::SetMousePos(int x, int y)
{
	SetVGUICursorPos(x, y);
}

/*
===========
MouseMove
===========
*/
void CInput::MouseMove( CUserCmd *cmd)
{
	int		mouse_x, mouse_y;
	int		mx, my;
	QAngle viewangles;

	static	int old_mouse_x, old_mouse_y;

	// Get view angles from engine
	engine->GetViewAngles( viewangles );

	// Don't dript pitch at all if mouselooking.
	if ( in_mlook.state & 1)
	{
		view->StopPitchDrift ();
	}

	//jjb - this disbles normal mouse control if the user is trying to 
	//      move the camera, or if the mouse cursor is visible 
	if ( !m_fCameraInterceptingMouse && !vgui::surface()->IsCursorVisible() )
	{
		// Sample mouse one more time
		AccumulateMouse();

		// Latch accumulated mouse movements
		GetAccumulatedMouse( &mx, &my );

		// Filter, etc. the delta values and place into mouse_x and mouse_y
		GetMouseDelta( mx, my, &old_mouse_x, &old_mouse_y, &mouse_x, &mouse_y );

		// Apply scaling factor
		ScaleMouse( &mouse_x, &mouse_y );

		// Let the client mode at the mouse input before it's used
		g_pClientMode->OverrideMouseInput( &mouse_x, &mouse_y );

		// Add mouse X/Y movement to cmd
		ApplyMouse( viewangles, cmd, mouse_x, mouse_y );

		// Re-center the mouse.
		ResetMouse();
	}

	// Store out the new viewangles.
	engine->SetViewAngles( viewangles );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mx - 
//			*my - 
//			*unclampedx - 
//			*unclampedy - 
//-----------------------------------------------------------------------------
void CInput::GetFullscreenMousePos( int *mx, int *my, int *unclampedx /*=NULL*/, int *unclampedy /*=NULL*/ )
{
	assert( mx );
	assert( my );

	if ( vgui::surface()->IsCursorVisible() )
	{
		int		current_posx, current_posy;
		GetMousePos(current_posx, current_posy);

		current_posx -= engine->GetWindowCenterX();
		current_posy -= engine->GetWindowCenterY();

		// Now need to add back in mid point of viewport
		//

		current_posx += ScreenWidth()  / 2;
		current_posy += ScreenHeight() / 2;

		if ( unclampedx )
		{
			*unclampedx = current_posx;
		}

		if ( unclampedy )
		{
			*unclampedy = current_posy;
		}

		// Clamp
		current_posx = max( 0, current_posx );
		current_posx = min( ScreenWidth(), current_posx );

		current_posy = max( 0, current_posy );
		current_posy = min( ScreenHeight(), current_posy );

		*mx = current_posx;
		*my = current_posy;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mx - 
//			my - 
//-----------------------------------------------------------------------------
void CInput::SetFullscreenMousePos( int mx, int my )
{
	SetMousePos
	( 
		mx - ScreenWidth() / 2 + engine->GetWindowCenterX(), 
		my - ScreenHeight() / 2 + engine->GetWindowCenterY() 
	);
}

/*
===================
ClearStates
===================
*/
void CInput::ClearStates (void)
{
	if ( !m_nMouseActive )
		return;

	m_nXAccum = 0;
	m_nYAccum = 0;
	m_nMouseOldButtons = 0;
}
