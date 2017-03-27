//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "stdwin.h"
#include "resource.h"
#include "vstdlib/icommandline.h"
#include "iengine.h"
#include <stdlib.h>
#include <zmouse.h>
#include "launcher_util.h"
#include "engine_launcher_api.h"
#include "keydefs.h"
#include "dll_state.h"
#include "ivideomode.h"
#include "igame.h"
#include "cd.h"
#include "cd_internal.h"
#include "ithread.h"
#include "ilauncherui.h"
#include "vgui_int.h"
#include "dinput.h"
#include "tier0/vcrmode.h"


//#define USE_DI_MOUSE
//#define USE_DI_KEYBOARD

//-----------------------------------------------------------------------------
// Purpose: Main game interface, including message pump and window creation
//-----------------------------------------------------------------------------
class CGame : public IGame
{
public:
					CGame( void );
	virtual			~CGame( void );

	bool			Init( void *pvInstance );
	bool			Shutdown( void );

	void			HandleDInputEvents( void );
	void			SleepUntilInput( int time );
	HWND			GetMainWindow( void );
	HWND			*GetMainWindowAddress( void );

	void			SetWindowXY( int x, int y );
	void			SetWindowSize( int w, int h );
	void			GetWindowRect( int *x, int *y, int *w, int *h );

	bool			IsActiveApp( void );
	bool			IsMultiplayer( void );

public:
	void			SetMainWindow( HWND window );
	void			SetActiveApp( bool active );
	int				WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

private:
	bool			CreateGameWindow( HINSTANCE hInst );
	void			AppActivate( bool fActive );
#ifndef USE_DI_KEYBOARD
	int				MapKey (int key);
#endif
#ifndef USE_DI_MOUSE
	int				FieldsFromMouseWParam( WPARAM wParam );
	int				FieldsFromMouseUpMsgAndWParam( UINT msg, WPARAM wParam );
#endif

private:
#ifndef USE_DI_MOUSE
	static const int MS_WM_XBUTTONDOWN;
	static const int MS_WM_XBUTTONUP;
	static const int MS_MK_BUTTON4;
	static const int MS_MK_BUTTON5;
#endif
	static const char CLASSNAME[];

	bool			m_bWindowsNT;

	HANDLE			m_hEvent;
	HWND			m_hWindow;

	int				m_x;
	int				m_y;
	int				m_width;
	int				m_height;
	bool			m_bActiveApp;
	bool			m_bMultiplayer;
#ifndef USE_DI_MOUSE
	UINT			m_uiMouseWheel;
#endif

	CDInput			*m_pcdinput;
};

static CGame g_Game;
IGame *game = ( IGame * )&g_Game;

#ifndef USE_DI_MOUSE
const int CGame::MS_WM_XBUTTONDOWN	= 0x020B;
const int CGame::MS_WM_XBUTTONUP	= 0x020C;
const int CGame::MS_MK_BUTTON4		= 0x0020;
const int CGame::MS_MK_BUTTON5		= 0x0040;
#endif
const char CGame::CLASSNAME[] = "Valve001";

void CGame::AppActivate( bool fActive )
{
	if (NULL != m_pcdinput)
		m_pcdinput->Acquire(fActive);

	if ( engine )
	{
		engine->AppActivate( fActive );
	}

	// minimize/restore fulldib windows/mouse-capture normal windows on demand
	if ( !videomode->GetInModeChange() && videomode->GetInitialized() )
	{
		// If we're activing the visible engine window then adjust the mouse
		if ( fActive )
		{
			if ( !launcherui->IsActive() )
			{
				engine->IN_ActivateMouse ();
			}
		}
		else
		{
			engine->IN_DeactivateMouse ();
		}
	}

	if ( !eng->GetInEngineLoad() )
	{
		g_Game.SetActiveApp( fActive );

		if (fActive)
		{
			cdaudio->Resume();
		}
		else
		{
			cdaudio->Pause();
		}
	}

	if ( launcherui->IsActive() )
	{
		launcherui->RequestFocus();
	}
}

#ifndef USE_DI_KEYBOARD
static BYTE        scantokey[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6', 
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0 
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i', 
	'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1 
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';', 
	'\'' ,    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2 
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*', 
	K_ALT,' ',   0  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0  , K_HOME, 
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW,K_KP_PLUS,K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,  K_F11, 
	K_F12,	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
}; 

int CGame::MapKey (int key)
{
	int result;
	int modified = ( key >> 16 ) & 255;
	bool is_extended = false;

	if ( modified > 127)
		return 0;

	if ( key & ( 1 << 24 ) )
		is_extended = true;

	result = scantokey[modified];

	if ( !is_extended )
	{
		switch ( result )
		{
		case K_HOME:
			return K_KP_HOME;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_PGUP:
			return K_KP_PGUP;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_END:
			return K_KP_END;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_PGDN:
			return K_KP_PGDN;
		case K_INS:
			return K_KP_INS;
		case K_DEL:
			return K_KP_DEL;
		default:
			break;
		}
	}
	else
	{
		switch ( result )
		{
		case 0x0D:
			return K_KP_ENTER;
		case 0x2F:
			return K_KP_SLASH;
		case 0xAF:
			return K_KP_PLUS;
		}
	}

	return result;
}
#endif

#ifndef USE_DI_MOUSE
int CGame::FieldsFromMouseWParam( WPARAM wParam )
{
	int temp = 0;

	if (wParam & MK_LBUTTON)
	{
		temp |= 1;
	}

	if (wParam & MK_RBUTTON)
	{
		temp |= 2;
	}

	if (wParam & MK_MBUTTON)
	{
		temp |= 4;
	}

	if (wParam & MS_MK_BUTTON4)
	{
		temp |= 8;
	}

	if (wParam & MS_MK_BUTTON5)
	{
		temp |= 16;
	}

	return temp;
}

int CGame::FieldsFromMouseUpMsgAndWParam( UINT msg, WPARAM wParam )
{
	int temp = 0;
	// Fucking windows, if the LBUTTONUP message is received, 
	//  MK_LBUTTON isn't actually set, etc. etc.
	if ( msg == WM_LBUTTONUP )
	{
		temp |= 1;
	}
	if ( msg == WM_RBUTTONUP )
	{
		temp |= 2;
	}
	if ( msg == WM_MBUTTONUP )
	{
		temp |= 4;
	}
	if ( msg == MS_WM_XBUTTONUP )
	{
		if ( HIWORD( wParam ) == 1 )
		{
			temp |= 8;
		}
		else if ( HIWORD( wParam ) == 2 )
		{
			temp |= 16;
		}
	}

	return temp;
}
#endif

int CGame::WindowProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)

{
	LONG			lRet = 0;

	int				temp = 0;
	HDC				hdc;
	PAINTSTRUCT		ps;
	// static int		recursiveflag;
	bool fAltDown;


	g_pVCR->TrapWindowProc( uMsg, wParam, lParam );


	if ( !engine )
	{
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}

#ifndef USE_DI_MOUSE
	if ( uMsg == m_uiMouseWheel )
	{
		eng->TrapKey_Event( ( ( int ) wParam ) > 0 ? K_MWHEELUP : K_MWHEELDOWN, true );
		eng->TrapKey_Event( ( ( int ) wParam ) > 0 ? K_MWHEELUP : K_MWHEELDOWN, false );
        return lRet;
	}
#endif

	switch (uMsg)
	{
		case WM_SETFOCUS:
			if ( launcherui->IsActive() )
			{
				launcherui->RequestFocus();
			}
			break;
		case WM_CREATE:
			::SetForegroundWindow(hWnd);
			break;

#ifndef USE_DI_MOUSE
		case WM_MOUSEWHEEL:
			eng->TrapKey_Event( ( short ) HIWORD( wParam ) > 0 ? K_MWHEELUP : K_MWHEELDOWN, true );
			eng->TrapKey_Event( ( short ) HIWORD( wParam ) > 0 ? K_MWHEELUP : K_MWHEELDOWN, false );
			break;
#endif

		case WM_SYSCOMMAND:

	        if ( ( wParam == SC_SCREENSAVE ) || 
				 ( wParam == SC_CLOSE ) ) 
			{
			    return lRet;
			}
        
			if ( !videomode->GetInModeChange() )
			{
				engine->S_BlockSound ();
				engine->S_ClearBuffer ();
			}

			lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);

			if ( !videomode->GetInModeChange() )
			{
				engine->S_UnblockSound ();
			}
			break;

		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED)
			{
				MoveWindow( hWnd, 0, -20, 0, 20, FALSE );
			}
			break;
			
		case WM_MOVE:
			game->SetWindowXY( (int)LOWORD( lParam ), (int)HIWORD( lParam ) );
			videomode->UpdateWindowPosition();
			break;

		case WM_SYSCHAR:
			// keep Alt-Space from happening
			break;

		case WM_ACTIVATEAPP:
			AppActivate( wParam ? true : false );
			break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

#ifndef USE_DI_KEYBOARD
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if ( !videomode->GetInModeChange() )
			{
				eng->TrapKey_Event (MapKey(lParam), true );
			}
			break;
#endif

		case WM_SYSKEYUP:
			fAltDown = ( HIWORD(lParam) & KF_ALTDOWN ) ? true : false;

			// Check for ALT-TAB and ALT-ESC
			if (fAltDown && (wParam == VK_TAB || wParam == VK_ESCAPE))
			{
				if (!videomode->IsWindowedMode())
				{
					ShowWindow(hWnd, SW_MINIMIZE);
					break;
				}
			}

#ifndef USE_DI_KEYBOARD
		case WM_KEYUP:
			if ( !videomode->GetInModeChange() )
			{
				eng->TrapKey_Event (MapKey(lParam), false );
			}
			break;
#endif

#ifndef USE_DI_MOUSE
	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
		case WM_MOUSEMOVE:
			if ( !videomode->GetInModeChange() )
			{
				temp |= FieldsFromMouseWParam( wParam );
				eng->TrapMouse_Event ( temp, true );
			}
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case MS_WM_XBUTTONDOWN:
			if ( !videomode->GetInModeChange() )
			{
				temp |= FieldsFromMouseWParam( wParam );
				eng->TrapMouse_Event ( temp, true );
			}
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case MS_WM_XBUTTONUP:
			if ( !videomode->GetInModeChange() )
			{
				temp = FieldsFromMouseUpMsgAndWParam( uMsg, wParam ) | FieldsFromMouseWParam( wParam );
				eng->TrapMouse_Event ( temp, false );
			}
			break;
#endif

   	    case WM_CLOSE:
			if ( !videomode->GetInModeChange() )
			{
				if ( eng->GetState() == DLL_ACTIVE )
				{
					eng->Unload();
					gui->SetQuitting();
				}
			}
			lRet = 0;
			break;

		default:
            /* pass all unhandled messages to DefWindowProc */
            lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);
	        break;
    }

    /* return 0 if handled message, 1 if not */
    return lRet;

}

void CGame::HandleDInputEvents()
{
	if (NULL != m_pcdinput)
		m_pcdinput->HandleEvents();
}

void CGame::SleepUntilInput( int time )
{
	MsgWaitForMultipleObjects(1, &m_hEvent, FALSE, time, QS_KEY );
}

static HINSTANCE g_hDLLInstance = 0;

//-----------------------------------------------------------------------------
// Purpose: This is only here so we can cache of the instance handle of the .dll
//  so we can load icons out of it correctly
// Input  : hInstance - 
//			fdwReason - 
//			pvReserved - 
// Output : int WINAPI DllMain
//-----------------------------------------------------------------------------
int WINAPI DllMain (HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	g_hDLLInstance = hInstance;
    return TRUE;
}


static LONG WINAPI HLEngineWindowProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
	return g_Game.WindowProc( hWnd, uMsg, wParam, lParam );
}

bool CGame::CreateGameWindow( HINSTANCE hInst )
{
	WNDCLASSEX		wc;

#ifndef USE_DI_MOUSE
	m_uiMouseWheel = RegisterWindowMessage( "MSWHEEL_ROLLMSG" );
#endif

	memset( &wc, 0, sizeof( wc ) );

	wc.cbSize		 = sizeof( wc );
    wc.style         = CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc   = HLEngineWindowProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASSNAME;
	wc.hIcon		 = LoadIcon( g_hDLLInstance, MAKEINTRESOURCE( IDI_LAUNCHER ) );
	wc.hIconSm		 = wc.hIcon;

    RegisterClassEx( &wc );

	DWORD			style;
	// Note, it's hidden
	style = WS_POPUP | WS_CLIPSIBLINGS;
	
	if ( videomode->IsWindowedMode() )
	{
		// Give it a frame
		style |= WS_OVERLAPPEDWINDOW;
		style &= ~WS_THICKFRAME;
	}

	// Never a max box
	style &= ~WS_MAXIMIZEBOX;

	int w, h;

	// Create a full screen size window by default, it'll get resized later anyway
	w = GetSystemMetrics( SM_CXSCREEN );
	h = GetSystemMetrics( SM_CYSCREEN );

	// Create the window
	HWND hwnd;
	hwnd = CreateWindow( CLASSNAME, util->GetString( "$game" ), style, 
		0, 0, w, h, NULL, NULL, hInst, NULL );

	g_Game.SetMainWindow( hwnd );

	return hwnd ? true : false;
}

CGame::CGame( void )
{
	m_bWindowsNT = false;
	m_hWindow = 0;
	m_x = m_y = 0;
	m_width = m_height = 0;
	m_bActiveApp = true;
	m_bMultiplayer = false;
#ifndef USE_DI_MOUSE
	m_uiMouseWheel = 0;
#endif
}

CGame::~CGame( void )
{
}

bool CGame::Init( void *pvInstance )
{
	m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	if ( !m_hEvent )
	{
		return false;
	}

	OSVERSIONINFO	vinfo;
	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if ( !GetVersionEx( &vinfo ) )
	{
		return false;
	}

	if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32s )
	{
		return false;
	}

	// FIXME:  Any additional OS version enforcement.  Also could check DX for DX 8.1 at least

	thread->Init();
	videomode->Init(); 

	// Create engine window, etc.
	if ( !CreateGameWindow( (HINSTANCE)pvInstance ))
		return(false);
	
	m_pcdinput = new CDInput(m_hWindow);
	return(m_pcdinput->FInitOK());
}

bool CGame::Shutdown( void )
{
	if (NULL != m_pcdinput)
		delete(m_pcdinput);

	CloseHandle( m_hEvent );

	thread->Shutdown();

	if ( m_hWindow )
	{
		DestroyWindow( m_hWindow );
		m_hWindow = (HWND)0;
	}
	return true;
}

HWND CGame::GetMainWindow( void )
{
	return m_hWindow;
}

HWND *CGame::GetMainWindowAddress( void )
{
	return &m_hWindow;
}

void CGame::SetMainWindow( HWND window )
{
	m_hWindow = window;
}

void CGame::SetWindowXY( int x, int y )
{
	m_x = x;
	m_y = y;
}

void CGame::SetWindowSize( int w, int h )
{
	m_width = w;
	m_height = h;
}

void CGame::GetWindowRect( int *x, int *y, int *w, int *h )
{
	if ( x )
	{
		*x = m_x;
	}
	if ( y )
	{
		*y = m_y;
	}
	if ( w )
	{
		*w = m_width;
	}
	if ( h )
	{
		*h = m_height;
	}
}

bool CGame::IsActiveApp( void )
{
	return m_bActiveApp;
}

void CGame::SetActiveApp( bool active )
{
	m_bActiveApp = active;
}

bool CGame::IsMultiplayer( void )
{
	return m_bMultiplayer;
}
