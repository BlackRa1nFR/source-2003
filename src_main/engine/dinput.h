//=========== Copyright © 1996-2001, Valve LLC, All rights reserved. ==========
//
// Purpose: Use DInput to process mouse and keyboard input
//
// $NoKeywords: $
//=============================================================================
#if !defined( CDINPUT_H )
#define CDINPUT_H

typedef class CDInput
{
public:
	CDInput(HWND hwndMain);
	~CDInput(void);

	bool FInitOK(void) { return(m_fInitOK); };

	void HandleEvents(void);
	void GetMousePos(int &x, int &y);
	void SetMousePos(int x, int y);
	void Acquire(bool fAcquire);

private:
	bool FCreateMouseDevice(HWND hwndMain);
	bool FCreateKeyboardDevice(HWND hwndMain);
	void GetSystemKeyboardSettings(void);

	void ReadMouseEvents(void);
	void SendMouseEvents(void);
	void ReadKeyboardEvents(void);
	void SendKeyboardEvent(unsigned int nChar, bool fDown);


private:
	bool m_fInitOK;

	// DirectInput objects
	struct IDirectInput *m_pDI; 
	struct IDirectInputDevice *m_pMouse; 
	struct IDirectInputDevice *m_pKeyboard;

	// Mouse stuff
	int m_xMouse;           // Current position of the mouse
	int m_yMouse;
	int m_nMouseButtons;    // Current button state
	bool m_fMouseMoved;     // True if we need to send a mouse move message

	// Keyboard stuff
	int m_nKeyboardDelay;    // # of ticks before we start repeating keys
	int m_nKeyboardSpeed;    // # of ticks between key repeats
	unsigned int m_nKeyLast; // Last key the user pressed (for auto-repeat)
	int m_nKeyTicksPressed;  // Time the key was pressed
	int	m_nKeyTicksRepeat;   // Last time we repeated the key
} CDInput;


// For exporting to the engine
void CDInputGetMousePos(int &x, int &y);
void CDInputSetMousePos(int x, int y);

#endif // CDINPUT