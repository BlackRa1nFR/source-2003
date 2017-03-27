//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Implementation of the VGUI ISurface interface using the 
// material system to implement it
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#include <windows.h>
#include <zmouse.h>

#include "input.h"
#include "VGuiMatSurface.h"
#include "../vgui2/src/VPanel.h"

#include <vgui/keycode.h>
#include <vgui/mousecode.h>
//#include <vgui/IInput.h>
#include <vgui/IVGui.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui/IClientPanel.h>
#include "Cursor.h"
#include "tier0/dbg.h"
#include "../vgui2/src/vgui_key_translation.h"

#include <vgui/IInputInternal.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Translates actual keys into VGUI ids
//-----------------------------------------------------------------------------
static WNDPROC s_ChainedWindowProc = NULL;
static bool s_bInputEnabled = true;
extern HWND thisWindow;

//-----------------------------------------------------------------------------
// Initializes the input system
//-----------------------------------------------------------------------------
void InitInput()
{
	EnableInput( true );
}

static const int MS_WM_XBUTTONDOWN	= 0x020B;
static const int MS_WM_XBUTTONUP	= 0x020C;
static const int MS_MK_BUTTON4		= 0x0020;
static const int MS_MK_BUTTON5		= 0x0040;

//-----------------------------------------------------------------------------
// Handles input messages
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MatSurfaceWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	VPANEL panel = NULL;
	IClientPanel *client = NULL;

	{
		panel = g_pIVGUI->HandleToPanel(::GetWindowLong(hwnd, GWL_USERDATA));

		if (panel)
		{
			client = ((VPanel *)panel)->Client();
		}
	}

	if (s_bInputEnabled)
	{
		switch(uMsg)
		{
			case MS_WM_XBUTTONDOWN:
			{
				if ( HIWORD( wParam ) == 1 )
				{
					g_pIInput->InternalMousePressed(MOUSE_4);
				}
				else
				{
					g_pIInput->InternalMousePressed(MOUSE_5);
				}
				break;
			}
			case MS_WM_XBUTTONUP:
			{
				if ( HIWORD( wParam ) == 1 )
				{
					g_pIInput->InternalMouseReleased(MOUSE_4);
				}
				else
				{
					g_pIInput->InternalMouseReleased(MOUSE_5);
				}
				break;
			}
			case WM_SETFOCUS:
			{
				// FIXME: pass focus around somehow!!
			//	g_pIPanel->Client(g_pISurface->GetEmbeddedPanel())->RequestFocus();
				break;
			}
			case WM_SETCURSOR:
			{
				ActivateCurrentCursor();
				break;
			}
			case WM_MOUSEMOVE:
			{
				g_pIInput->InternalCursorMoved((short)LOWORD(lParam),(short)HIWORD(lParam));
				break;
			}
			case WM_LBUTTONDOWN:
			{
				g_pIInput->InternalMousePressed(MOUSE_LEFT);
				break;
			}
			case WM_RBUTTONDOWN:
			{
				g_pIInput->InternalMousePressed(MOUSE_RIGHT);
				break;
			}
			case WM_MBUTTONDOWN:
			{
				g_pIInput->InternalMousePressed(MOUSE_MIDDLE);
				break;
			}
			case WM_LBUTTONUP:
			{
				g_pIInput->InternalMouseReleased(MOUSE_LEFT);
				break;
			}
			case WM_RBUTTONUP:
			{
				g_pIInput->InternalMouseReleased(MOUSE_RIGHT);
				break;
			}
			case WM_MBUTTONUP:
			{
				g_pIInput->InternalMouseReleased(MOUSE_MIDDLE);
				break;
			}
			case WM_LBUTTONDBLCLK:
			{
				g_pIInput->InternalMouseDoublePressed(MOUSE_LEFT);
				break;
			}
			case WM_RBUTTONDBLCLK:
			{
				g_pIInput->InternalMouseDoublePressed(MOUSE_RIGHT);
				break;
			}
			case WM_MBUTTONDBLCLK:
			{
				g_pIInput->InternalMouseDoublePressed(MOUSE_MIDDLE);
				break;
			}
			case WM_MOUSEWHEEL:
			{
				g_pIInput->InternalMouseWheeled(((short)HIWORD(wParam))/WHEEL_DELTA);
				break;
			}
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				int code=wParam;
				if(!(lParam&(1<<30)))
				{
					g_pIInput->InternalKeyCodePressed( KeyCode_VirtualKeyToVGUI( code ) );
				}
				g_pIInput->InternalKeyCodeTyped( KeyCode_VirtualKeyToVGUI( code ) );
				break;
			}
			case WM_SYSCHAR:
			case WM_CHAR:
			{
				int unichar = wParam;
				g_pIInput->InternalKeyTyped(unichar);
				break;
			}
			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				int code=wParam;
				g_pIInput->InternalKeyCodeReleased( KeyCode_VirtualKeyToVGUI( code ) );
				break;
			}
		}
	}

	if ( s_ChainedWindowProc )
	{
		Assert( s_ChainedWindowProc );
		return CallWindowProc( s_ChainedWindowProc, hwnd, uMsg, wParam, lParam );
	}
	else
	{
		// This means the application is driving the messages (calling our window procedure manually)
		// rather than us hooking their window procedure. The engine needs to do this in order for VCR 
		// mode to play back properly.
		return 0;	
	}
}


//-----------------------------------------------------------------------------
// Enables/disables input (enabled by default)
//-----------------------------------------------------------------------------
void EnableInput( bool bEnable )
{
#if 0 // #ifdef BENCHMARK
	s_bInputEnabled = false;
#else
	s_bInputEnabled = bEnable;
#endif
}


//-----------------------------------------------------------------------------
// Hooks input listening up to a window
//-----------------------------------------------------------------------------
void InputAttachToWindow(void *hwnd)
{
	s_ChainedWindowProc = (WNDPROC)GetWindowLong( (HWND)hwnd, GWL_WNDPROC );
	SetWindowLong( (HWND)hwnd, GWL_WNDPROC, (LONG)MatSurfaceWindowProc );
}

void InputDetachFromWindow(void *hwnd)
{
	if (!hwnd)
		return;
	Assert( s_ChainedWindowProc ); 
	SetWindowLong( (HWND)hwnd, GWL_WNDPROC, (LONG)s_ChainedWindowProc );
}


void InputHandleWindowMessage( void *hwnd, unsigned int uMsg, unsigned int wParam, long lParam )
{
	MatSurfaceWindowProc( (HWND)hwnd, uMsg, wParam, lParam );
}




