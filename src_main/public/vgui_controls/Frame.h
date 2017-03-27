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

#ifndef VGUI_FRAME_H
#define VGUI_FRAME_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/Dar.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/FocusNavGroup.h>

namespace vgui
{

class Button;
class FrameButton;
class Menu;
class TextImage;
class MenuButton;
class FrameSystemButton;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class Frame : public EditablePanel
{
public:
	Frame(Panel *parent, const char *panelName, bool showTaskbarIcon = true);
	~Frame();

	// Set the text in the title bar.  Set surfaceTitle=true if you want this to be the taskbar text as well.
	virtual void SetTitle(const char *title, bool surfaceTitle);
	virtual void SetTitle(const wchar_t *title, bool surfaceTitle);

	// Bring the frame to the front and requests focus, ensures it's not minimized
	virtual void Activate();

	// activates the dialog; if dialog is not currently visible it starts it minimized and flashing in the taskbar
	virtual void ActivateMinimized();

	// closes the dialog
	virtual void Close();

	// Move the dialog to the center of the screen 
	virtual void MoveToCenterOfScreen();

	// Set the movability of the panel
	virtual void SetMoveable(bool state);
	// Check the movability of the panel
	virtual bool IsMoveable();

	// Set the resizability of the panel
	virtual void SetSizeable(bool state);
	// Check the resizability of the panel
	virtual bool IsSizeable();
	// Toggle visibility of the system menu button
	virtual void SetMenuButtonVisible(bool state);
	void SetMenuButtonResponsive(bool state);

	// Toggle visibility of the minimize button
	virtual void SetMinimizeButtonVisible(bool state);
	// Toggle visibility of the maximize button
	virtual void SetMaximizeButtonVisible(bool state);
	// Toggles visibility of the minimize-to-systray icon (defaults to false)
	virtual void SetMinimizeToSysTrayButtonVisible(bool state);

	// Toggle visibility of the close button
	virtual void SetCloseButtonVisible(bool state);

	// returns true if the dialog is currently minimized
	virtual bool IsMinimized();
	// Flash the window system tray button until the frame gets focus
	virtual void FlashWindow();
	// Stops any window flashing
	virtual void FlashWindowStop();
	// command handling
	virtual void OnCommand(const char *command);

	// Get the system menu 
	virtual Menu *GetSysMenu();
	// Set the system menu 
	virtual void SetSysMenu(Menu *menu);

	// set whether the title bar should be rendered
	virtual void SetTitleBarVisible( bool state );

	DECLARE_PANELMAP();
	/* CUSTOM MESSAGE HANDLING
		"SetTitle"
			input:	"text"	- string to set the title to be
	*/

protected:
	// Respond to mouse presses
	virtual void OnMousePressed(MouseCode code);
	// Respond to Key presses.
	virtual void OnKeyCodePressed(KeyCode code);
	// Respond to Key typing
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	// Respond to Key releases
	virtual void OnKeyCodeReleased(KeyCode code);
	// Respond to Key focus ticks
	virtual void OnKeyFocusTicked();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	// Recalculate the position of all items
	virtual void PerformLayout();
	// Respond when a close message is recieved.  Can be called directly to close a frame.
	virtual void OnClose();
	// Minimize the window on the taskbar.
	virtual void OnMinimize();
	// Called when minimize-to-systray button is pressed (does nothing by default)
	virtual void OnMinimizeToSysTray();
	// Add the child to the focus nav group
	virtual void OnChildAdded(VPANEL child);
	// Load the control settings 
	virtual void LoadControlSettings(const char *dialogResourceName, const char *pathID = NULL);
	// settings
	virtual void ApplySettings(KeyValues *inResourceData);
	// records the settings into the resource data
	virtual void GetSettings(KeyValues *outResourceData);

	// gets the default position and size on the screen to appear the first time (defaults to centered)
	virtual bool GetDefaultScreenPosition(int &x, int &y, int &wide, int &tall);

	// painting
	virtual void PaintBackground();

	// Get the size of the panel inside the frame edges.
	virtual void GetClientArea(int &x, int &y, int &wide, int &tall);

	// user configuration settings
	// this is used for any control details the user wants saved between sessions
	// eg. dialog positions, last directory opened, list column width
	virtual void ApplyUserConfigSettings(KeyValues *userConfig);

	// returns user config settings for this control
	virtual void GetUserConfigSettings(KeyValues *userConfig);

private:
	void InternalSetTitle(const char *title);
	void InternalFlashWindow();
	void SetupResizeCursors();
	void LayoutProportional( FrameButton *bt);

	Color _titleBarBgColor;
	Color _titleBarDisabledBgColor;
	Color _titleBarFgColor;
	Color _titleBarDisabledFgColor;
	TextImage *_title;
	bool _sizeable;
	bool _moveable;
	bool _hasFocus;
	Panel * _topGrip;
	Panel *_bottomGrip;
	Panel *_leftGrip;
	Panel *_rightGrip;
	Panel *_topLeftGrip;
	Panel *_topRightGrip;
	Panel *_bottomLeftGrip;
	Panel *_bottomRightGrip;
	Panel *_captionGrip;

	FrameButton *_minimizeButton;
	FrameButton	*_maximizeButton;
	FrameButton *_minimizeToSysTrayButton;
	FrameButton	*_closeButton;
	FrameSystemButton *_menuButton;
	Frame *_resizeable;
	bool _flashWindow;
	bool _nextFlashState;
	Menu *_sysMenu;
	bool _drawTitleBar;

	typedef EditablePanel BaseClass;
};

} // namespace vgui

#endif // VGUI_FRAME_H
