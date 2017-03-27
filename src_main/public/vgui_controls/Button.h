//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BUTTON_H
#define BUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/Dar.h>
#include <Color.h>
#include <vgui_controls/Label.h>

namespace vgui
{

enum MouseCode;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class Button : public Label
{
public:
	Button(Panel *parent, const char *panelName, const char *text);
	Button(Panel *parent, const char *panelName, const wchar_t *text);
	~Button();
private:
	void Init();
public:
	// Set armed state.
	virtual void SetArmed(bool state);
	// Check armed state
	virtual bool IsArmed( void );

	// Check depressed state
	virtual bool IsDepressed();
	// Set button force depressed state.
	virtual void ForceDepressed(bool state);
	// Set button depressed state with respect to the force depressed state.
	virtual void RecalculateDepressedState( void );

	// Set button selected state.
	virtual void SetSelected(bool state);
	// Check selected state
	virtual bool IsSelected( void );

	//Set whether or not the button captures all mouse input when depressed.
	virtual void SetUseCaptureMouse( bool state );
	// Check if mouse capture is enabled.
	virtual bool IsUseCaptureMouseEnabled( void );

	// Activate a button click.
	virtual void DoClick( void );

	// Set button to be mouse clickable or not.
	virtual void SetMouseClickEnabled( MouseCode code, bool state );
	// Check if button is mouse clickable
	virtual bool IsMouseClickEnabled( MouseCode code );
	// sets the how this button activates
	enum ActivationType_t
	{
		ACTIVATE_ONPRESSEDANDRELEASED,	// normal button behaviour
		ACTIVATE_ONPRESSED,				// menu buttons, toggle buttons
		ACTIVATE_ONRELEASED,			// menu items
	};
	virtual void SetButtonActivationType(ActivationType_t activationType);

	// Message targets that the button has been pressed
	virtual void FireActionSignal( void );
	// Perform graphical layout of button
	virtual void PerformLayout();

	virtual bool RequestInfo(KeyValues *data);

    virtual bool CanBeDefaultButton(void);

	// Set this button to be the button that is accessed by default when the user hits ENTER or SPACE
	virtual void SetAsDefaultButton(int state);
	// Set this button to be the button that is currently accessed by default when the user hits ENTER or SPACE
	virtual void SetAsCurrentDefaultButton(int state);

	// Respond when key focus is received
	virtual void OnSetFocus();
	// Respond when focus is killed
	virtual void OnKillFocus();

	// Set button border attribute enabled, controls display of button.
	virtual void SetButtonBorderEnabled( bool state );

	// Set default button colors.
	virtual void SetDefaultColor(Color fgColor, Color bgColor);
	// Set armed button colors
	virtual void SetArmedColor(Color fgColor, Color bgColor);
	// Set depressed button colors
	virtual void SetDepressedColor(Color fgColor, Color bgColor);

	// Get button foreground color
	virtual Color GetButtonFgColor();
	// Get button background color
	virtual Color GetButtonBgColor();

	// Set default button border attributes.
	virtual void SetDefaultBorder(IBorder *border);
	// Set depressed button border attributes.
	virtual void SetDepressedBorder(IBorder *border);
	// Set key focused button border attributes.
	virtual void SetKeyFocusBorder(IBorder *border);

	// Set the command to send when the button is pressed
	// Set the panel to send the command to with setActionSignalTarget()
	virtual void SetCommand( const char *command );
	// Set the message to send when the button is pressed
	virtual void SetCommand( KeyValues *message );

	// sound handling
	void SetArmedSound(const char *sound);
	void SetDepressedSound(const char *sound);
	void SetReleasedSound(const char *sound);

	DECLARE_PANELMAP();
	/* CUSTOM MESSAGE HANDLING
		"PressButton"	- makes the button act as if it had just been pressed by the user (just like DoClick())
			input: none		
	*/

	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void SizeToContents();

protected:
	virtual void DrawFocusBorder(int tx0, int ty0, int tx1, int ty1);

	// Paint button on screen
	virtual void Paint(void);
	// Get button border attributes.
	virtual IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus);

	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnSetState(int state);
		
	
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void OnKeyCodePressed(KeyCode code);
	virtual void OnKeyCodeReleased(KeyCode code);
	


	// Get control settings for editing
	virtual void GetSettings( KeyValues *outResourceData );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual const char *GetDescription( void );



private:
	bool               _armed;		// whether the mouse is over the button
	bool		       _depressed;	// whether the button is visual depressed
	bool               _selected;	// whether the mouse is being held down on this button
	bool			   _forceDepressed;
	bool               _buttonBorderEnabled;
	bool			   _useCaptureMouse;
	bool			   _keyDown;
	int                _mouseClickMask;
	KeyValues		  *_actionMessage;
	ActivationType_t   _activationType;

	IBorder			  *_defaultBorder;
	IBorder			  *_depressedBorder;
	IBorder			  *_keyFocusBorder;

	Color			   _defaultFgColor, _defaultBgColor;
	Color			   _armedFgColor, _armedBgColor;
	Color              _depressedFgColor, _depressedBgColor;

	unsigned short	   m_sArmedSoundName, m_sDepressedSoundName, m_sReleasedSoundName;
	bool _defaultButton;	// true if this is the button that gets activated by default when the user hits enter

	typedef Label BaseClass;
};

} // namespace vgui

#endif // BUTTON_H
