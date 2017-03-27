//=========== Copyright © 1996-2001, Valve LLC, All rights reserved. ==========
//
// Purpose: Use DInput to process mouse and keyboard input
//
// $NoKeywords: $
//=============================================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "tier0/dbg.h"
#define DIRECTINPUT_VERSION 0x0800
#include "..\dx8sdk\include\dinput.h"
#include "iengine.h"
#include "ivideomode.h"
#include "keydefs.h"
#include "dinput.h"
#include "tier0/vcrmode.h"
//#define USE_DI_MOUSE
//#define USE_DI_KEYBOARD

extern BYTE mpBScanNKey[256];


CDInput *g_pcdinput = NULL;  // We only have a single instance.  We use this
                             // for exposing GetMousePos & SetMousePos to the engine.


//-----------------------------------------------------------------------------
// Purpose: Constructor for CDInput.  Creates the DInput object, the mouse
//          object, and the keyboard object.  Sets m_fInitOK if succesful.
// Input  : hwndMain - hwnd of our main app window
// Output : 
//-----------------------------------------------------------------------------
CDInput::CDInput(HWND hwndMain)
{
	m_fInitOK = false;
	m_pDI = NULL;
	m_pMouse = NULL;
	m_pKeyboard = NULL;
	g_pcdinput = this;

	HMODULE hinst = GetModuleHandle(NULL);

	// Initialize DInput
	HRESULT hr = DirectInput8Create((HINSTANCE)hinst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&m_pDI, NULL); 
	if FAILED(hr)
		return;

#ifdef USE_DI_MOUSE
	// Create the mouse
	if (!FCreateMouseDevice(hwndMain))
		return;
#endif

#ifdef USE_DI_KEYBOARD
	// Create the keyboard
	if (!FCreateKeyboardDevice(hwndMain))
		return;
#endif

	m_fInitOK = true;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor for CDInput.
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
CDInput::~CDInput()
{
	if (NULL != m_pMouse)
	{
		m_pMouse->Unacquire();
		m_pMouse->Release();
		m_pMouse = NULL;
	}

	if (NULL != m_pKeyboard)
	{
		m_pKeyboard->Unacquire();
		m_pKeyboard->Release();
		m_pKeyboard = NULL;
	}

	if (NULL != m_pDI)
	{
		m_pDI->Release();
		m_pDI = NULL;
	}

	m_fInitOK = false;
}


//-----------------------------------------------------------------------------
// Purpose: Initializes the DInput mouse device
// Input  : 
// Output : success or failure
//-----------------------------------------------------------------------------
bool CDInput::FCreateMouseDevice(HWND hwndMain)
{
	HRESULT hr;

	// Create the mouse
	hr = m_pDI->CreateDevice(GUID_SysMouse, &m_pMouse, NULL);
	if (FAILED(hr))
		return(false);

	// Set the data format and cooperative level
	hr = m_pMouse->SetDataFormat(&c_dfDIMouse2);
	if (FAILED(hr))
		return(false);

	hr = m_pMouse->SetCooperativeLevel(hwndMain, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(hr))
		return(false);

	// Use buffered input
	DIPROPDWORD dipdw;
    dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj        = 0;
    dipdw.diph.dwHow        = DIPH_DEVICE;
    dipdw.dwData            = 32;
 
	hr = m_pMouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);
 	if (FAILED(hr))
		return(false);

	// Start out by acquiring the mouse
	m_pMouse->Acquire();

	// Set up our mouse globals
	RECT rectWindow;
	if (GetWindowRect(hwndMain, &rectWindow))
	{
		m_xMouse = (rectWindow.left + rectWindow.right) / 2;
		m_yMouse = (rectWindow.top + rectWindow.bottom) / 2;
	}
	else
	{
		m_xMouse = 0;
		m_yMouse = 0;
	}

	m_nMouseButtons = 0;
	m_fMouseMoved = false;

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Initializes the DInput keyboard device
// Input  : 
// Output : Success or failure
//-----------------------------------------------------------------------------
bool CDInput::FCreateKeyboardDevice(HWND hwndMain)
{
	HRESULT hr;

	// Create the keyboard
	hr = m_pDI->CreateDevice(GUID_SysKeyboard, &m_pKeyboard, NULL);
	if (FAILED(hr))
		return(false);

	// Set the data format and cooperative level
	hr = m_pKeyboard->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr))
		return(false);

	hr = m_pKeyboard->SetCooperativeLevel(hwndMain, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(hr))
		return(false);

	// Use buffered input
	DIPROPDWORD dipdw;
    dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj        = 0;
    dipdw.diph.dwHow        = DIPH_DEVICE;
    dipdw.dwData            = 32;
 
	hr = m_pKeyboard->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);
 	if (FAILED(hr))
		return(false);

	// Start out by acquiring the keyboard
	m_pKeyboard->Acquire();

	// Get system values for key repeat
	GetSystemKeyboardSettings();

	// Set up keyboard globals
	m_nKeyLast = 0;

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Gets system settings for mouse and keyboard behavior
// Input  : 
// Output : success or failure
//-----------------------------------------------------------------------------
void CDInput::GetSystemKeyboardSettings()
{
	// Get the system key repeat delay.  The docs specify that a value of 0 equals a 
	// 250ms delay and a value of 3 equals a 1 sec delay.
	int nT;
	if (SystemParametersInfo(SPI_GETKEYBOARDDELAY, NULL, &nT, 0))
	{
		switch (nT)
		{
		case 0:
			m_nKeyboardDelay = 250;
			break;
		case 1:
			m_nKeyboardDelay = 500;
			break;
		case 2:
			m_nKeyboardDelay = 750;
			break;
		case 3:
		default:
			m_nKeyboardDelay = 1000;
			break;
		}
	}
	else
	{
		m_nKeyboardDelay = 1000;
	}

	// Get the system key repeat rate.  The docs specify that a value of 0 equals a 
	// rate of 2.5 / sec (400 ms delay), and a value of 31 equals a rate of 30/sec
	// (33 ms delay).
	if (SystemParametersInfo(SPI_GETKEYBOARDSPEED, NULL, &nT, 0))
	{
		if (nT < 0)
			nT = 0;
		if (nT > 31)
			nT = 31;
		m_nKeyboardSpeed = 400 + nT * (33 - 400) / 31;
	}
	else
	{
		m_nKeyboardSpeed = 400;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Checks whether any mouse buttons or keyboard keys have been pressed,
//          and if so, sends the appropriate messages.
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CDInput::HandleEvents()
{
	if (!m_fInitOK)
	{
		Assert(false);
		return;
	}

#ifdef USE_DI_MOUSE
	ReadMouseEvents();
	SendMouseEvents();
#endif
#ifdef USE_DI_KEYBOARD
	ReadKeyboardEvents();
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Process all mouse messages up to the current point and
//          update m_xMouse, m_yMouse, and m_nMouseButtons.  Note that we don't
//          send any mouse messages (call SendMouseEvents to do that).  This is
//          because we sometimes need to update the mouse position but don't
//          want to be firing the buttons right away.
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CDInput::ReadMouseEvents()
{
#ifdef USE_DI_MOUSE
	// Loop through all the buffered mouse events
	for (;;)
	{
		HRESULT hr;
		DIDEVICEOBJECTDATA od;
		DWORD dwElements = 1;

		hr = m_pMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);
		if FAILED(hr)
		{
			m_pMouse->Acquire();
			hr = m_pMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);
		}

		if (FAILED(hr) || dwElements == 0) 
			break;

		switch (od.dwOfs) 
		{
		case DIMOFS_X: 
			m_xMouse += od.dwData;
			m_fMouseMoved = true;
			break;

		case DIMOFS_Y: 
			m_yMouse += od.dwData;
			m_fMouseMoved = true;
			break; 

		case DIMOFS_Z:
			if ((int)od.dwData > 0)
			{
				SendKeyboardEvent( K_MWHEELUP, TRUE );
				SendKeyboardEvent( K_MWHEELUP, FALSE );
			}
			else
			{
				SendKeyboardEvent( K_MWHEELDOWN, TRUE );
				SendKeyboardEvent( K_MWHEELDOWN, FALSE );
			}
			break;

		case DIMOFS_BUTTON0:
			if (od.dwData != 0)
				m_nMouseButtons |= 1;
			else
				m_nMouseButtons &= ~1;
			break;

		case DIMOFS_BUTTON1:
			if (od.dwData != 0)
				m_nMouseButtons |= 2;
			else
				m_nMouseButtons &= ~2;
			break;

		case DIMOFS_BUTTON2:
			if (od.dwData != 0)
				m_nMouseButtons |= 4;
			else
				m_nMouseButtons &= ~4;
			break;

		case DIMOFS_BUTTON3:
			if (od.dwData != 0)
				m_nMouseButtons |= 8;
			else
				m_nMouseButtons &= ~8;
			break;

		case DIMOFS_BUTTON4:
			if (od.dwData != 0)
				m_nMouseButtons |= 16;
			else
				m_nMouseButtons &= ~16;
			break;
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Sends a mouse event corresponding to the current state of the
//          mouse buttons.
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CDInput::SendMouseEvents()
{
	//if (!videomode->GetInModeChange())
	{
		eng->TrapMouse_Event(m_nMouseButtons, true);

		if (m_fMouseMoved)
		{
			eng->TrapMouse_Event(m_nMouseButtons, true);
			m_fMouseMoved = false;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Scans the keyboard and emits key up / down messages as necessary
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CDInput::ReadKeyboardEvents()
{
	int nTicksCur = Plat_MSTime();

	// Loop through all the bufferedkeyboard events
	for (;;)
	{
		HRESULT hr;
		DIDEVICEOBJECTDATA od;
		DWORD dwElements = 1;

		hr = m_pKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);
		if FAILED(hr)
		{
			m_pKeyboard->Acquire();
			hr = m_pKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);
		}

		if (FAILED(hr) || dwElements == 0) 
			break;

		// If it's a key we care about, fire a key event
		if (0 != mpBScanNKey[od.dwOfs])
		{
			SendKeyboardEvent(mpBScanNKey[od.dwOfs], 0 != (od.dwData & 0x80));
			if (od.dwData & 0x80)
			{
				m_nKeyLast = od.dwOfs;
				m_nKeyTicksPressed = nTicksCur;
				m_nKeyTicksRepeat = nTicksCur;
			}
			else if (od.dwOfs == m_nKeyLast)
			{
				m_nKeyLast = 0;
			}
		}
	}

	// Auto-repeat the last key pressed
	if (0 != m_nKeyLast)
	{
		// Issue the first repeat message?
		if (m_nKeyTicksRepeat == m_nKeyTicksPressed)
		{
			if (nTicksCur >= m_nKeyTicksPressed + m_nKeyboardDelay)
			{
				SendKeyboardEvent(mpBScanNKey[m_nKeyLast], true);
				m_nKeyTicksRepeat = nTicksCur;
			}
		}
		// Check if we need to repeat again
		else
		{
			if (nTicksCur >= m_nKeyTicksRepeat + m_nKeyboardSpeed)
			{
				SendKeyboardEvent(mpBScanNKey[m_nKeyLast], true);
				m_nKeyTicksRepeat = nTicksCur;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sends a given keystroke to the engine
// Input  : nChar - character code to send
//          fDown - true if it's a key down event
// Output : 
//-----------------------------------------------------------------------------
void CDInput::SendKeyboardEvent(unsigned int nChar, bool fDown)
{
	// if (!videomode->GetInModeChange())
	{
		eng->TrapKey_Event(nChar, fDown);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Acquires or unacquires our DInput devices
// Input  : fAcquire - true if we want to acquire, false for unacquire
// Output : 
//-----------------------------------------------------------------------------
void CDInput::Acquire(bool fAcquire)
{
	if (fAcquire)
	{
#ifdef USE_DI_MOUSE
		m_pMouse->Acquire();
#endif
#ifdef USE_DI_KEYBOARD
		m_pKeyboard->Acquire();
#endif
	}
	else
	{
#ifdef USE_DI_MOUSE
		m_pMouse->Unacquire();
#endif
#ifdef USE_DI_KEYBOARD
		m_pKeyboard->Unacquire();
#endif
	}
}


//-----------------------------------------------------------------------------
// Purpose: Return the current position of the mouse
// Input  : x, y - where to store the mouse pos
// Output : 
//-----------------------------------------------------------------------------
void CDInput::GetMousePos(int &x, int &y)
{
#ifdef USE_DI_MOUSE
	ReadMouseEvents();
	x = m_xMouse;
	y = m_yMouse;
#else
	POINT pt;
	g_pVCR->Hook_GetCursorPos(&pt);
	x = pt.x;
	y = pt.y;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Sets the current mouse position
// Input  : x, y - position to set the mouse to
// Output : 
//-----------------------------------------------------------------------------
void CDInput::SetMousePos(int x, int y)
{
#ifdef USE_DI_MOUSE
	ReadMouseEvents();
	m_xMouse = x;
	m_yMouse = y;
#else
	if ( g_pVCR->GetMode() != VCR_Playback ) // don't mess with the cursor during playback
	{
		SetCursorPos(x, y);
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Calls the global DInput object to get the mouse pos
// Input  : x, y - where to store the mouse pos
// Output : 
//-----------------------------------------------------------------------------
void CDInputGetMousePos(int &x, int &y)
{
	if (NULL != g_pcdinput)
		g_pcdinput->GetMousePos(x, y);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the current mouse position
// Input  : x, y - position to set the mouse to
// Output : 
//-----------------------------------------------------------------------------
void CDInputSetMousePos(int x, int y)
{
	if (NULL != g_pcdinput)
		g_pcdinput->SetMousePos(x, y);
}


// This maps raw key scan codes to our internal key codes
BYTE mpBScanNKey[256] = 
{ 
//  0			1			2			3			4			5			6			7 
//  8			9			A			B			C			D			E			F 
	0,			27,			'1',		'2',		'3',		'4',		'5',		'6', 	// 0 
	'7',		'8',		'9',		'0',		'-',		'=',		K_BACKSPACE,9,
	'q',		'w',		'e',		'r',		't',		'y',		'u',		'i',	// 1  
	'o',		'p',		'[',		']',		13 ,		K_CTRL,		'a',		's',
	'd',		'f',		'g',		'h',		'j',		'k',		'l',		';',	// 2  
	'\'',		'`',		K_SHIFT,	'\\',		'z',		'x',		'c',		'v',
	'b',		'n',		'm',		',',		'.',		'/',		K_SHIFT,	'*', 	// 3 
	K_ALT,		' ',		K_CAPSLOCK,	K_F1,		K_F2,		K_F3,		K_F4,		K_F5,
	K_F6,		K_F7,		K_F8,		K_F9,		K_F10,		K_PAUSE,	0,			K_KP_HOME, // 4 
	K_KP_UPARROW,K_KP_PGUP,	K_KP_MINUS,	K_KP_LEFTARROW,K_KP_5,	K_KP_RIGHTARROW,K_KP_PLUS,K_KP_END,
	K_KP_DOWNARROW,K_KP_PGDN,K_KP_INS,	K_KP_DEL,	0,			0,			0,			K_F11, 	// 5
	K_F12,		0,			0,			0,			0,			0,			0,			0,	
	0,			0,			0,			0,			0,			0,			0,			0,		// 6
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0,		// 7
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0,		// 8
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0,		// 9
	0,			0,			0,			0,			K_KP_ENTER,	K_CTRL,		0,			0,
	0,			0,			0,			0,			0,			0,			0,			0,		// A
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			K_KP_SLASH,	0,			0,		// B
	K_ALT,		0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			K_HOME,	// C
	K_UPARROW,	K_PGUP,		0,			K_LEFTARROW,0,			K_RIGHTARROW,0,			K_END,
	K_DOWNARROW,K_PGDN,		K_INS,		K_DEL,		0,			0,			0,			0,		// D
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0,		// E
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0,		// F
	0,			0,			0,			0,			0,			0,			0,			0,
}; 

