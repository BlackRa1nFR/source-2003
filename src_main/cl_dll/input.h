//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Revision: $
// $NoKeywords: $
//
//=============================================================================

#if !defined( INPUT_H )
#define INPUT_H
#ifdef _WIN32
#pragma once
#endif

#include "iinput.h"
#include "vector.h"
#include "kbutton.h"
#include "ehandle.h"

class CKeyboardKey
{
public:
	// Name for key
	char				name[ 32 ];
	// Pointer to the underlying structure
	struct kbutton_s	*pkey;
	// Next key in key list.
	CKeyboardKey		*next;
};

class ConVar;

class CInput : public IInput
{
// Interface
public:
							CInput( void );
							~CInput( void );

	virtual		void		Init_All( void );
	virtual		void		Shutdown_All( void );
	virtual		int			GetButtonBits( int );
	virtual		void		CreateMove ( int command_number, int totalslots, float tick_frametime, float input_sample_frametime, int active, float packet_loss );
	virtual		void		ExtraMouseSample( float frametime, int active );
	virtual		void		WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand );
	virtual		void		EncodeUserCmdToBuffer( bf_write& buf, int buffersize, int slot );
	virtual		void		DecodeUserCmdFromBuffer( bf_read& buf, int buffersize, int slot );

	virtual		CUserCmd *GetUsercmd( int slot );

	virtual		void		MakeWeaponSelection( C_BaseCombatWeapon *weapon );

	virtual		float		KeyState ( struct kbutton_s *key );
	virtual		int			KeyEvent( int eventcode, int keynum, const char *pszCurrentBinding );
	virtual		struct kbutton_s *FindKey( const char *name );
	virtual		void		ControllerCommands( void );
	virtual		void		Joystick_Advanced( void );
	virtual		void		AccumulateMouse( void );
	virtual		void		MouseEvent( int mstate, bool down );
	virtual		void		ActivateMouse( void );
	virtual		void		DeactivateMouse( void );
	virtual		void		ClearStates( void );
	virtual		float		GetLookSpring( void );
	virtual		void		GetFullscreenMousePos( int *mx, int *my, int *unclampedx = NULL, int *unclampedy = NULL );
	virtual		void		SetFullscreenMousePos( int mx, int my );
	virtual		void		ResetMouse( void );
	virtual		bool		IsNoClipping( void );
	virtual		float		GetLastForwardMove( void );
	virtual		void		ClearInputButton( int bits );

	virtual		void		CAM_Think( void );
	virtual		int			CAM_IsThirdPerson( void );
	virtual		void		CAM_GetCameraOffset( Vector& ofs );
	virtual		void		CAM_ToThirdPerson(void);
	virtual		void		CAM_ToFirstPerson(void);
	virtual		void		CAM_StartMouseMove(void);
	virtual		void		CAM_EndMouseMove(void);
	virtual		void		CAM_StartDistance(void);
	virtual		void		CAM_EndDistance(void);
	virtual		int			CAM_InterceptingMouse( void );

	// orthographic camera info
	virtual		void		CAM_ToOrthographic();
	virtual		bool		CAM_IsOrthographic() const;
	virtual		void		CAM_OrthographicSize( float& w, float& h ) const;

// Private Implementation
private:
	// Implementation specific initialization
	void		Init_Camera( void );
	void		Init_Keyboard( void );
	void		Init_Joystick( void );
	void		Init_Mouse( void );
	void		Shutdown_Keyboard( void );
	// Add a named key to the list queryable by the engine
	void		AddKeyButton( const char *name, struct kbutton_s *pkb );
	// Mouse/keyboard movement input helpers
	void		ScaleMovements( CUserCmd *cmd );
	void		ComputeForwardMove( CUserCmd *cmd );
	void		ComputeUpwardMove( CUserCmd *cmd );
	void		ComputeSideMove( CUserCmd *cmd );
	void		AdjustAngles ( float frametime );
	void		ClampAngles( QAngle& viewangles );
	void		AdjustPitch( float speed, QAngle& viewangles );
	void		AdjustYaw( float speed, QAngle& viewangles );
	float		DetermineKeySpeed( float frametime );
	void		GetAccumulatedMouse( int *mx, int *my );
	void		GetMouseDelta( int mx, int my, int *oldx, int *oldy, int *x, int *y );
	void		ScaleMouse( int *x, int *y );
	void		ApplyMouse( QAngle& viewangles, CUserCmd *cmd, int mouse_x, int mouse_y );
	void		MouseMove ( CUserCmd *cmd );
	// Joystick  movement input helpers
	void		ControllerMove ( float frametime, CUserCmd *cmd );
	void		JoyStickMove ( float frametime, CUserCmd *cmd );
	int			ReadJoystick( void );
	unsigned long *RawValuePointer( int axis );

	// Call this to get the cursor position. The call will be logged in the VCR file if there is one.
	void		GetMousePos(int &x, int &y);
	void		SetMousePos(int x, int y);

// Private Data
private:
	typedef struct
	{
		unsigned int AxisFlags;
		unsigned int AxisMap;
		unsigned int ControlMap;
		unsigned long *pRawValue;
	} joy_axis_t;

	enum
	{
		JOY_AXIS_X = 0,
		JOY_AXIS_Y,
		JOY_AXIS_Z,
		JOY_AXIS_R,
		JOY_AXIS_U,
		JOY_AXIS_V,
	};

	enum
	{
		MOUSE_BUTTON_COUNT = 5
	};

	enum
	{
		JOY_MAX_AXES = 6,
	};

	enum
	{
		AxisNada = 0,
		AxisForward,
		AxisLook,
		AxisSide,
		AxisTurn
	};

	enum
	{
		CAM_COMMAND_NONE = 0,
		CAM_COMMAND_TOTHIRDPERSON = 1,
		CAM_COMMAND_TOFIRSTPERSON = 2
	};

	// Has the mouse been initialized?
	int			m_nMouseInitialized;
	// Is the mosue active?
	int			m_nMouseActive;
	// Is there a joystick?
	int			m_fJoystickAvailable;
	// Has the joystick advanced initialization been run?
	int			m_fJoystickAdvancedInit;
	// Does the joystick have a POV control?
	int			m_fJoystickHasPOVControl;
	// Number of buttons on joystick
	int			m_nJoystickButtons;
	// Which joystick are we using?
	int			m_nJoystickID;
	unsigned int m_nJoystickFlags;
	// Old Joystick button states
	unsigned int m_nJoystickOldButtons;
	unsigned int m_nJoystickOldPOVState;
	// Number of mouse buttons
	int			m_nMouseButtons;
	// Old button states
	int			m_nMouseOldButtons;
	// Accumulated mouse deltas
	int			m_nXAccum;
	int			m_nYAccum;
	// Flag to restore systemparameters when exiting
	int			m_fRestoreSPI;
	// Original mouse parameters
	int			m_rgOrigMouseParms[ 3 ];
	// Current mouse parameters.
	int			m_rgNewMouseParms[ 3 ];
	// Are the parameters valid
	int			m_fMouseParmsValid;
	// Joystick Axis data
	joy_axis_t m_rgAxes[ JOY_MAX_AXES ];
	// List of queryable keys
	CKeyboardKey *m_pKeys;
	
	// Is the 3rd person camera using the mouse?
	int			m_fCameraInterceptingMouse;
	// Are we in 3rd person view?
	int			m_fCameraInThirdPerson;
	// Should we move view along with mouse?
	int			m_fCameraMovingWithMouse;
	// What is the current camera offset from the view origin?
	Vector		m_vecCameraOffset;
	// Is the camera in distance moving mode?
	int			m_fCameraDistanceMove;
	// Old and current mouse position readings.
	int			m_nCameraOldX;
	int			m_nCameraOldY;
	int			m_nCameraX;
	int			m_nCameraY;

	// orthographic camera settings
	bool		m_CameraIsOrthographic;

	QAngle		m_angPreviousViewAngles;

	int			m_fLastForwardMove;

	CUserCmd	*m_pCommands;

	const ConVar *m_pCmdRate;
	const ConVar *m_pUpdateRate;

	// Set until polled by CreateMove and cleared
	CHandle< C_BaseCombatWeapon > m_hSelectedWeapon;
};

extern kbutton_t in_strafe;
extern kbutton_t in_mlook;
extern kbutton_t in_speed;
extern kbutton_t in_jlook;
extern kbutton_t in_graph;  

#endif // INPUT_H