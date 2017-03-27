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
#include "winquake.h"
#include "iengine.h"
#include <stdlib.h>
#include <zmouse.h>
#include "engine_launcher_api.h"
#include "keydefs.h"
#include "dll_state.h"
#include "ivideomode.h"
#include "igame.h"
#include "cd.h"
#include "cd_internal.h"
#include "cdll_engine_int.h"
#include "ithread.h"
#include "host.h"
#include "profile.h"
#include "cdll_int.h"
#include "vgui_int.h"
#include "sound.h"
#include "dinput.h"
#include "conprint.h"
#include "utllinkedlist.h"
#include "tier0/vcrmode.h"
#include "gl_matsysiface.h"
#include "filesystem_engine.h"
#include "keydefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void S_BlockSound (void);
void S_UnblockSound (void);
void ClearIOStates( void );


class CStoredGameMessage
{
public:
	UINT	uMsg;
	WPARAM	wParam;
	LPARAM	lParam;
};


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

	bool			CreateGameWindow( void );

	void			HandleDInputEvents();
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


// Message handlers.
public:

	void	HandleMsg_UIMouseWheel( UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	HandleMsg_MouseWheel( UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	HandleMsg_WMMove( UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	HandleMsg_ActivateApp( UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	HandleMsg_KeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	HandleMsg_KeyUp( UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	HandleMsg_MouseMove( UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	HandleMsg_ButtonDown( UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	HandleMsg_ButtonUp( UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	HandleMsg_Close( UINT uMsg, WPARAM wParam, LPARAM lParam );
	void	HandleMsg_Quit( UINT uMsg, WPARAM wParam, LPARAM lParam );

	// Call the appropriate HandleMsg_ function.
	void	DispatchGameMsg( UINT uMsg, WPARAM wParam, LPARAM lParam );

	// Dispatch all the queued up messages.
	virtual void	DispatchAllStoredGameMessages();
	
	static const int MS_WM_XBUTTONDOWN;
	static const int MS_WM_XBUTTONUP;

private:
	void			AppActivate( bool fActive );
	int				MapVirtualKeyToEngineKey (WPARAM wParam); // Maps a windows wParam key message to engien key code
	int				MapEngineKeyToVirtualKey(int keyCode); // and the other way round
	int				MapScanCodeToEngineKey (LPARAM lParam); // map non-translated keyboard scan code to engine code
	int				FieldsFromMouseWParam( WPARAM wParam );
	int				FieldsFromMouseUpMsgAndWParam( UINT msg, WPARAM wParam );

private:
	static const int MS_MK_BUTTON4;
	static const int MS_MK_BUTTON5;
	static const char CLASSNAME[];

	bool			m_bWindowsNT;

	HANDLE			m_hEvent;
	HWND			m_hWindow;
	HINSTANCE		m_hInstance;

	int				m_x;
	int				m_y;
	int				m_width;
	int				m_height;
	bool			m_bActiveApp;
	bool			m_bMultiplayer;
	UINT			m_uiMouseWheel;

	CUtlLinkedList<CStoredGameMessage, int>	m_StoredGameMessages;

private:
#ifndef SWDS
	CDInput			*m_pcdinput;
#endif
};




static CGame g_Game;
IGame *game = ( IGame * )&g_Game;

const int CGame::MS_WM_XBUTTONDOWN	= 0x020B;
const int CGame::MS_WM_XBUTTONUP	= 0x020C;
const int CGame::MS_MK_BUTTON4		= 0x0020;
const int CGame::MS_MK_BUTTON5		= 0x0040;
const char CGame::CLASSNAME[] = "Valve001";


// These are all the windows messages that can change game state.
// See CGame::WindowProc for a description of how they work.
class CGameMessageHandler
{
public:
	UINT		uMsg;
	void		(CGame::*pFn)( UINT uMsg, WPARAM wParam, LPARAM lParam );
};

CGameMessageHandler g_GameMessageHandlers[] = 
{
	{ 0xFFFF /*m_uiMouseWheel*/,&CGame::HandleMsg_UIMouseWheel }, // First entry MUST be the UIMouseWheel one because we use RegisterMessage.
	{ WM_MOUSEWHEEL,			&CGame::HandleMsg_MouseWheel },
	{ WM_MOVE,					&CGame::HandleMsg_WMMove },
	{ WM_ACTIVATEAPP,			&CGame::HandleMsg_ActivateApp },
	{ WM_KEYDOWN,				&CGame::HandleMsg_KeyDown },
	{ WM_SYSKEYDOWN,			&CGame::HandleMsg_KeyDown },
	{ WM_SYSKEYUP,				&CGame::HandleMsg_KeyUp },
	{ WM_KEYUP,					&CGame::HandleMsg_KeyUp },
	{ WM_MOUSEMOVE,				&CGame::HandleMsg_MouseMove },

	{ WM_LBUTTONDOWN,			&CGame::HandleMsg_ButtonDown },
	{ WM_RBUTTONDOWN,			&CGame::HandleMsg_ButtonDown },
	{ WM_MBUTTONDOWN,			&CGame::HandleMsg_ButtonDown },
	{ CGame::MS_WM_XBUTTONDOWN,	&CGame::HandleMsg_ButtonDown },

	{ WM_LBUTTONUP,				&CGame::HandleMsg_ButtonUp },
	{ WM_RBUTTONUP,				&CGame::HandleMsg_ButtonUp },
	{ WM_MBUTTONUP,				&CGame::HandleMsg_ButtonUp },
	{ CGame::MS_WM_XBUTTONUP,	&CGame::HandleMsg_ButtonUp },

	{ WM_CLOSE,					&CGame::HandleMsg_Close },
	{ WM_QUIT,					&CGame::HandleMsg_Close }
};




void CGame::AppActivate( bool fActive )
{
#ifndef SWDS
	if (NULL != m_pcdinput)
		m_pcdinput->Acquire(fActive);

	if ( host_initialized )
	{
		if ( fActive )
		{
			videomode->RestoreVideo();

			ClearIOStates();
			UpdateMaterialSystemConfig();

			cdaudio->Resume();
		}
		else
		{
			cdaudio->Pause();

			videomode->ReleaseVideo();
		}
	}
#endif

	SetActiveApp( fActive );
}

// this maps non-translated keyboard scan codes to engine key codes
// Google for 'Keyboard Scan Code Specification'
static BYTE s_pVirtualToEngine[256];
static BYTE s_pEngineToVirtual[256];
static bool s_bInitted = false; // true if keytoscan is filled

static BYTE s_pScanToEngine[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    K_ESCAPE, '1',    '2',    '3',    '4',    '5',    '6', 
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0 
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i', 
	'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1 
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';', 
	'\'' ,  '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2 
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*', 
	K_ALT,' ',   K_CAPSLOCK  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0  , K_HOME, 
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW,K_KP_PLUS,K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,  K_F11, 
	K_F12,	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
}; 

void InitKeyTranslationTables()
{
	int i;

	if ( s_bInitted )
		return;

	memset( s_pVirtualToEngine, 0, sizeof(s_pVirtualToEngine) );	

	for (i=0; i<128;i++)
		s_pVirtualToEngine[i] = i;	// set default ascii chars

	for (i=65; i<=90;i++)
		s_pVirtualToEngine[i] = i+32;	// A -> a

	s_pVirtualToEngine[VK_NUMPAD0]	=K_KP_INS;
	s_pVirtualToEngine[VK_NUMPAD1]	=K_KP_END;
	s_pVirtualToEngine[VK_NUMPAD2]	=K_KP_DOWNARROW;
	s_pVirtualToEngine[VK_NUMPAD3]	=K_KP_PGDN;
	s_pVirtualToEngine[VK_NUMPAD4]	=K_KP_LEFTARROW;
	s_pVirtualToEngine[VK_NUMPAD5]	=K_KP_5;
	s_pVirtualToEngine[VK_NUMPAD6]	=K_KP_RIGHTARROW;
	s_pVirtualToEngine[VK_NUMPAD7]	=K_KP_HOME;
	s_pVirtualToEngine[VK_NUMPAD8]	=K_KP_UPARROW;
	s_pVirtualToEngine[VK_NUMPAD9]	=K_KP_PGUP;
	s_pVirtualToEngine[VK_DIVIDE]	=K_KP_SLASH;
	s_pVirtualToEngine[VK_MULTIPLY]	=0;
	s_pVirtualToEngine[VK_SUBTRACT]	=K_KP_MINUS;
	s_pVirtualToEngine[VK_ADD]		=K_KP_PLUS;
	s_pVirtualToEngine[VK_RETURN]	=K_KP_ENTER;
	s_pVirtualToEngine[VK_DECIMAL]	=K_KP_DEL;
	s_pVirtualToEngine[VK_RETURN]	=K_ENTER;
	s_pVirtualToEngine[VK_SPACE]	=K_SPACE;
	s_pVirtualToEngine[VK_BACK]		=K_BACKSPACE;
	s_pVirtualToEngine[VK_TAB]		=K_TAB;
	s_pVirtualToEngine[VK_CAPITAL]	=K_CAPSLOCK;
	s_pVirtualToEngine[VK_NUMLOCK]	=0;
	s_pVirtualToEngine[VK_ESCAPE]	=K_ESCAPE;
	s_pVirtualToEngine[VK_SCROLL]	=0;
	s_pVirtualToEngine[VK_INSERT]	=K_INS;
	s_pVirtualToEngine[VK_DELETE]	=K_DEL;
	s_pVirtualToEngine[VK_HOME]		=K_HOME;
	s_pVirtualToEngine[VK_END]		=K_END;
	s_pVirtualToEngine[VK_PRIOR]	=K_PGUP;
	s_pVirtualToEngine[VK_NEXT]		=K_PGDN;
	s_pVirtualToEngine[VK_PAUSE]	=K_PAUSE;
	s_pVirtualToEngine[VK_SHIFT]	=K_SHIFT;
	s_pVirtualToEngine[VK_MENU]		=K_ALT;
	s_pVirtualToEngine[VK_CONTROL]	=K_CTRL;
	s_pVirtualToEngine[VK_LWIN]		=0;
	s_pVirtualToEngine[VK_RWIN]		=0;
	s_pVirtualToEngine[VK_APPS]		=0;
	s_pVirtualToEngine[VK_UP]		=K_UPARROW;
	s_pVirtualToEngine[VK_LEFT]		=K_LEFTARROW;
	s_pVirtualToEngine[VK_DOWN]		=K_DOWNARROW;
	s_pVirtualToEngine[VK_RIGHT]	=K_RIGHTARROW;	
	s_pVirtualToEngine[VK_F1]		=K_F1;
	s_pVirtualToEngine[VK_F2]		=K_F2;
	s_pVirtualToEngine[VK_F3]		=K_F3;
	s_pVirtualToEngine[VK_F4]		=K_F4;
	s_pVirtualToEngine[VK_F5]		=K_F5;
	s_pVirtualToEngine[VK_F6]		=K_F6;
	s_pVirtualToEngine[VK_F7]		=K_F7;
	s_pVirtualToEngine[VK_F8]		=K_F8;
	s_pVirtualToEngine[VK_F9]		=K_F9;
	s_pVirtualToEngine[VK_F10]		=K_F10;
	s_pVirtualToEngine[VK_F11]		=K_F11;
	s_pVirtualToEngine[VK_F12]		=K_F12;

	s_pVirtualToEngine[192]			='`';	// console HACK

	// create revers table engine to virtual

	for ( i=0; i < 256; i++ )
	{
		s_pEngineToVirtual[ s_pVirtualToEngine[i] ] = i;
	}

	s_pEngineToVirtual[0] = 0;

	s_bInitted = true;
}

int MapEngineKeyToVirtualKey( int keyCode )
{				
	InitKeyTranslationTables();

	if ( keyCode < 0 || keyCode >= ARRAYSIZE( s_pEngineToVirtual ) )
	{
		Warning( "CGame::KeyToScan: invalid engine key code (%d)\n", keyCode );
		return 0;
	}

	return s_pEngineToVirtual[keyCode];
}

int CGame::MapEngineKeyToVirtualKey(int keyCode)
{
	return ::MapEngineKeyToVirtualKey(keyCode);
}

int CGame::MapScanCodeToEngineKey (LPARAM lParam)
{
	int result;
	int scanCode = ( lParam >> 16 ) & 255;
	bool is_extended = (lParam & ( 1 << 24 )) != 0;

	if ( scanCode > 127)
		return 0;

	if ( lParam & ( 1 << 24 ) )
		is_extended = true;

	result = s_pScanToEngine[scanCode];

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

// Maps windows scan code to engine key code
int CGame::MapVirtualKeyToEngineKey (WPARAM wParam)
{
	InitKeyTranslationTables();

	if ( wParam < 0 || wParam >= ARRAYSIZE( s_pVirtualToEngine ) )
	{
		Warning( "CGame::KeyToScan: invalid virtual key code (%d)\n", wParam );
		return 0;
	}

	return s_pVirtualToEngine[wParam];
}

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


void CGame::HandleMsg_UIMouseWheel( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	eng->TrapKey_Event( ( ( int ) wParam ) > 0 ? K_MWHEELUP : K_MWHEELDOWN, true );
	eng->TrapKey_Event( ( ( int ) wParam ) > 0 ? K_MWHEELUP : K_MWHEELDOWN, false );
}


void CGame::HandleMsg_MouseWheel( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	eng->TrapKey_Event( ( short ) HIWORD( wParam ) > 0 ? K_MWHEELUP : K_MWHEELDOWN, true );
	eng->TrapKey_Event( ( short ) HIWORD( wParam ) > 0 ? K_MWHEELUP : K_MWHEELDOWN, false );
}


void CGame::HandleMsg_WMMove( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	game->SetWindowXY( (int)LOWORD( lParam ), (int)HIWORD( lParam ) );
#ifndef SWDS
	videomode->UpdateWindowPosition();
#endif
}


void CGame::HandleMsg_ActivateApp( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	AppActivate( wParam ? true : false );
}


void CGame::HandleMsg_KeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	eng->TrapKey_Event (MapScanCodeToEngineKey(lParam), true );
}

void CGame::HandleMsg_KeyUp( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	eng->TrapKey_Event (MapScanCodeToEngineKey(lParam), false );
}


void CGame::HandleMsg_MouseMove( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	eng->TrapMouse_Event ( FieldsFromMouseWParam( wParam ), true );
}


void CGame::HandleMsg_ButtonDown( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	eng->TrapMouse_Event ( FieldsFromMouseWParam( wParam ), true );
}


void CGame::HandleMsg_ButtonUp( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	int temp = FieldsFromMouseUpMsgAndWParam( uMsg, wParam ) | FieldsFromMouseWParam( wParam );
	eng->TrapMouse_Event ( temp, false );
}


void CGame::HandleMsg_Close( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if ( eng->GetState() == DLL_ACTIVE )
	{
		eng->SetQuitting( IEngine::QUIT_TODESKTOP );
	}
}


void CGame::DispatchGameMsg( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
#ifndef SWDS
	// Let VGUI handle the message.
	VGui_HandleWindowMessage( game->GetMainWindow(), uMsg, wParam, lParam );
#endif

	for ( int i=0; i < ARRAYSIZE( g_GameMessageHandlers ); i++ )
	{
		if ( g_GameMessageHandlers[i].uMsg == uMsg )
		{
			(this->*g_GameMessageHandlers[i].pFn)( uMsg, wParam, lParam );
			break;
		}
	}
}


void CGame::DispatchAllStoredGameMessages()
{
	if ( g_pVCR->GetMode() == VCR_Playback )
	{
		UINT uMsg;
		WPARAM wParam;
		LPARAM lParam;
		while ( g_pVCR->Hook_PlaybackGameMsg( uMsg, wParam, lParam ) )
		{
			DispatchGameMsg( uMsg, wParam, lParam );
		}
	}
	else
	{
		while ( m_StoredGameMessages.Count() > 0 )
		{
			CStoredGameMessage msg = m_StoredGameMessages[ m_StoredGameMessages.Head() ];
			m_StoredGameMessages.Remove( m_StoredGameMessages.Head() );

			g_pVCR->Hook_RecordGameMsg( msg.uMsg, msg.wParam, msg.lParam );
			DispatchGameMsg( msg.uMsg, msg.wParam, msg.lParam );
		}

		g_pVCR->Hook_RecordEndGameMsg();
	}
}


int CGame::WindowProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)

{
	LONG			lRet = 0;
	HDC				hdc;
	PAINTSTRUCT		ps;

	
	//
	// NOTE: the way this function works is to handle all messages that just call through to
	// Windows or provide data to it.
	//
	// Any messages that change the engine's internal state (like key events) are stored in a list
	// and processed at the end of the frame. This is necessary for VCR mode to work correctly because
	// Windows likes to pump messages during some of its API calls like SetWindowPos, and unless we add
	// custom code around every Windows API call so VCR mode can trap the wndproc calls, VCR mode can't 
	// reproduce the calls to the wndproc.
	//


	if ( eng->GetQuitting() != IEngine::QUIT_NOTQUITTING )
	{
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}

	// If we're playing back, then ignore the message because DispatchAllStoredGameMessages 
	// will get the messages from VCR mode.
	if ( g_pVCR->GetMode() != VCR_Playback )
	{
		CStoredGameMessage stored;
		stored.uMsg = uMsg;
		stored.wParam = wParam;
		stored.lParam = lParam;

		m_StoredGameMessages.AddToTail( stored );
	}

	//
	// Note: NO engine state should be changed in here while in VCR record or playback. 
	// We can send whatever we want to Windows, but if we change its state in here instead of 
	// in DispatchAllStoredGameMessages, the playback may not work because Windows messages 
	// are not deterministic, so you might get different messages during playback than you did during record.
	//
	switch (uMsg)
	{
		case WM_CREATE:
			::SetForegroundWindow(hWnd);
			break;

		case WM_SYSCOMMAND:

	        if (wParam == SC_SCREENSAVE ) 
			{
			    return lRet;
			}
        
			if ( wParam == SC_CLOSE ) 
			{
				return lRet;
			}

			if ( wParam == SC_KEYMENU )
			{
				return lRet;
			}


#ifndef SWDS
			if ( g_pVCR->GetMode() == VCR_Disabled )
			{
				S_BlockSound();
				S_ClearBuffer();
			}
#endif

			lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);

#ifndef SWDS
			if ( g_pVCR->GetMode() == VCR_Disabled )
			{
				S_UnblockSound();
			}
#endif
			break;

		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED)
			{
				MoveWindow( hWnd, 0, -20, 0, 20, FALSE );
			}
			break;
			
		case WM_SYSCHAR:
			// keep Alt-Space from happening
			break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
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
#ifndef SWDS
	if (NULL != m_pcdinput)
		m_pcdinput->HandleEvents();
#endif
}

void CGame::SleepUntilInput( int time )
{
	MsgWaitForMultipleObjects(1, &m_hEvent, FALSE, time, QS_ALLEVENTS );
}

static LONG WINAPI HLEngineWindowProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
	return g_Game.WindowProc( hWnd, uMsg, wParam, lParam );
}

#define DEFAULT_EXE_ICON		101

bool CGame::CreateGameWindow( void )
{
#ifndef SWDS
	WNDCLASS		wc;
	
	m_uiMouseWheel = RegisterWindowMessage( "MSWHEEL_ROLLMSG" );
	Assert( g_GameMessageHandlers[0].uMsg == 0xFFFF && g_GameMessageHandlers[0].pFn == &CGame::HandleMsg_UIMouseWheel );
	g_GameMessageHandlers[0].uMsg = m_uiMouseWheel;

	memset( &wc, 0, sizeof( wc ) );

    wc.style         = CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc   = HLEngineWindowProc;
    wc.hInstance     = m_hInstance;
    wc.lpszClassName = CLASSNAME;

	// find the icon file in the filesystem
	char localPath[ MAX_PATH ];
	if ( g_pFileSystem->GetLocalPath( "resource/game.ico", localPath ) )
	{
		g_pFileSystem->GetLocalCopy( localPath );
		wc.hIcon = (HICON)::LoadImage(NULL, localPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
	}
	else
	{
		wc.hIcon = (HICON)LoadIcon( GetModuleHandle( 0 ), MAKEINTRESOURCE( DEFAULT_EXE_ICON ) );
	}

	// Oops, we didn't clean up the class registration from last cycle which
	//  might mean that the wndproc pointer is bogus
	UnregisterClass( CLASSNAME, m_hInstance );

	// Register it again
    RegisterClass( &wc );

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
	HWND hwnd = CreateWindow( CLASSNAME, "Half-Life", style, 
		0, 0, w, h, NULL, NULL, m_hInstance, NULL );

	g_Game.SetMainWindow( hwnd );

	if ( !hwnd )
		return false;

	m_pcdinput = new CDInput(m_hWindow);
	return(m_pcdinput->FInitOK());
#else
	return true;
#endif
}

CGame::CGame( void )
{
	m_bWindowsNT = false;
	m_hWindow = 0;
	m_x = m_y = 0;
	m_width = m_height = 0;
	m_bActiveApp = true;
	m_bMultiplayer = false;
	m_uiMouseWheel = 0;
	m_hInstance = 0;
}

CGame::~CGame( void )
{
}

bool CGame::Init( void *pvInstance )
{
	m_hInstance = (HINSTANCE)pvInstance;

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

#ifndef SWDS
	thread->Init();
#endif
	// Create engine window, etc.
	return true;
}

bool CGame::Shutdown( void )
{
#ifndef SWDS
	if (NULL != m_pcdinput)
		delete(m_pcdinput);

	thread->Shutdown();
#endif

	CloseHandle( m_hEvent );

	if ( m_hWindow )
	{
		DestroyWindow( m_hWindow );
		m_hWindow = (HWND)0;
	}

	UnregisterClass( CLASSNAME, m_hInstance );
	m_hInstance = 0;
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
