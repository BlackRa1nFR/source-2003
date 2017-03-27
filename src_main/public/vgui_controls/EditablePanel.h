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

#ifndef EDITABLEPANEL_H
#define EDITABLEPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/FocusNavGroup.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Panel that supports editing via the build dialog
//-----------------------------------------------------------------------------
class EditablePanel : public Panel
{
public:
	EditablePanel(Panel *parent, const char *panelName);
	EditablePanel::EditablePanel(Panel *parent, const char *panelName, HScheme hScheme);

	~EditablePanel();

	// Load the control settings - should be done after all the children are added
	virtual void LoadControlSettings(const char *dialogResourceName, const char *pathID = NULL);

	// sets the name of this dialog so it can be saved in the user config area
	// use dialogID to differentiate multiple instances of the same dialog
	virtual void LoadUserConfig(const char *configName, int dialogID = 0);
	virtual void SaveUserConfig();

	// combines both of the above, LoadControlSettings & LoadUserConfig
	virtual void LoadControlSettingsAndUserConfig(const char *dialogResourceName, int dialogID = 0);

	// Override to change how build mode is activated
	virtual void ActivateBuildMode();

	// Return the buildgroup that this panel is part of.
	virtual BuildGroup *GetBuildGroup();

	// Virtual factory for control creation
	// controlName is a string which is the same as the class name
	virtual Panel *CreateControlByName(const char *controlName);

	// Shortcut function to set data in child controls
	virtual void SetControlString(const char *controlName, const char *string);
	// Shortcut function to set data in child controls
	virtual void SetControlInt(const char *controlName, int state);
	// Shortcut function to get data in child controls
	virtual int GetControlInt(const char *controlName, int defaultState);
	// Shortcut function to get data in child controls
	// Returns a maximum of 511 characters in the string
	virtual const char *GetControlString(const char *controlName, const char *defaultString = "");
	// as above, but copies the result into the specified buffer instead of a static buffer
	virtual void GetControlString(const char *controlName, char *buf, int bufSize, const char *defaultString = "");

	// Focus handling
	// Delegate focus to a sub panel
	virtual void RequestFocus(int direction = 0);
	virtual bool RequestFocusNext(Panel *panel);
	virtual bool RequestFocusPrev(Panel *panel);
	// Pass the focus down onto the last used panel
	virtual void OnSetFocus();
	// Update focus info for navigation
	virtual void OnRequestFocus(Panel *subFocus, Panel *defaultPanel);
	// Get the panel that currently has keyfocus
	virtual VPANEL GetCurrentKeyFocus();
	// Get the panel with the specified hotkey
	virtual Panel *HasHotkey(wchar_t key);

	virtual void OnKeyCodeTyped(enum KeyCode code);

	// Handle information requests
	virtual bool RequestInfo(KeyValues *data);
	/* INFO HANDLING
		"BuildDialog"
			input:
				"BuildGroupPtr" - pointer to the panel/dialog to edit
			returns:
				"PanelPtr" - pointer to a new BuildModeDialog()

		"ControlFactory"
			input:
				"ControlName" - class name of the control to create
			returns:
				"PanelPtr" - pointer to the newly created panel, or NULL if no such class exists
	*/

protected:
	virtual void PaintBackground();

	// nav group access
	virtual FocusNavGroup &GetFocusNavGroup();

	// called when default button has been set
	virtual void OnDefaultButtonSet(Panel *defaultButton);
	// called when the current default button has been set
	virtual void OnCurrentDefaultButtonSet(Panel *defaultButton);
    virtual void OnFindDefaultButton();

	// overrides
	virtual void OnChildAdded(VPANEL child);
	virtual void OnSizeChanged(int wide, int tall);
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void OnClose();

	// optimization for text rendering, returns true if text should be rendered immediately after Paint()
	// disabled for now
	// virtual bool ShouldFlushText();

	DECLARE_PANELMAP();

private:
	BuildGroup *_buildGroup;
	FocusNavGroup m_NavGroup;

	// the wide and tall to which all controls are locked - used for autolayout deltas
	int _baseWide, _baseTall;
	char *m_pszConfigName;
	int m_iConfigID;

	typedef Panel BaseClass;
};

} // namespace vgui

#endif // EDITABLEPANEL_H
