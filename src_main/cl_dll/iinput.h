#if !defined( IINPUT_H )
#define IINPUT_H
#ifdef _WIN32
#pragma once
#endif

class bf_write;
class bf_read;
class CUserCmd;
class C_BaseCombatWeapon;

class IInput
{
public:
	// Initialization/shutdown of the subsystem
	virtual	void		Init_All( void ) = 0;
	virtual void		Shutdown_All( void ) = 0;
	// Latching button states
	virtual int			GetButtonBits( int ) = 0;
	// Create movement command
	virtual void		CreateMove ( int command_number, int totalslots, float tick_frametime, float input_sample_frametime, int active, float packet_loss ) = 0;
	virtual void		ExtraMouseSample( float frametime, int active ) = 0;
	virtual void		WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand ) = 0;
	virtual void		EncodeUserCmdToBuffer( bf_write& buf, int buffersize, int slot ) = 0;
	virtual void		DecodeUserCmdFromBuffer( bf_read& buf, int buffersize, int slot ) = 0;

	virtual CUserCmd	*GetUsercmd( int slot ) = 0;

	virtual void		MakeWeaponSelection( C_BaseCombatWeapon *weapon ) = 0;

	// Retrieve key state
	virtual float		KeyState ( struct kbutton_s *key ) = 0;
	// Issue key event
	virtual int			KeyEvent( int eventcode, int keynum, const char *pszCurrentBinding ) = 0;
	// Look for key
	virtual struct kbutton_s *FindKey( const char *name ) = 0;
	// Issue commands from controllers
	virtual void		ControllerCommands( void ) = 0;
	// Extra initialization for some joysticks
	virtual void		Joystick_Advanced( void ) = 0;
	// Accumulate mouse delta
	virtual void		AccumulateMouse( void ) = 0;
	// Notify about mouse event
	virtual void		MouseEvent( int mstate, bool down ) = 0;
	// Activate/deactivate mouse
	virtual void		ActivateMouse( void ) = 0;
	virtual void		DeactivateMouse( void ) = 0;
	// Clear mouse state data
	virtual void		ClearStates( void ) = 0;
	// Retrieve lookspring setting
	virtual float		GetLookSpring( void ) = 0;
	// Retrieve mouse position
	virtual void		GetFullscreenMousePos( int *mx, int *my, int *unclampedx = 0, int *unclampedy = 0 ) = 0;
	virtual void		SetFullscreenMousePos( int mx, int my ) = 0;
	virtual void		ResetMouse( void ) = 0;

	virtual bool		IsNoClipping( void ) = 0;

	virtual	float		GetLastForwardMove( void ) = 0;

	// Third Person camera ( TODO/FIXME:  Move this to a separate interface? )
	virtual void		CAM_Think( void ) = 0;
	virtual int			CAM_IsThirdPerson( void ) = 0;
	virtual void		CAM_GetCameraOffset( Vector& ofs ) = 0;
	virtual void		CAM_ToThirdPerson(void) = 0;
	virtual void		CAM_ToFirstPerson(void) = 0;
	virtual void		CAM_StartMouseMove(void) = 0;
	virtual void		CAM_EndMouseMove(void) = 0;
	virtual void		CAM_StartDistance(void) = 0;
	virtual void		CAM_EndDistance(void) = 0;
	virtual int			CAM_InterceptingMouse( void ) = 0;

	// orthographic camera info	( TODO/FIXME:  Move this to a separate interface? )
	virtual void		CAM_ToOrthographic() = 0;
	virtual	bool		CAM_IsOrthographic() const = 0;
	virtual	void		CAM_OrthographicSize( float& w, float& h ) const = 0;

	// Causes an input to have to be re-pressed to become active
	virtual void		ClearInputButton( int bits ) = 0;
};

extern ::IInput *input;

#endif // IINPUT_H