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

#ifndef FOCUSNAVGROUP_H
#define FOCUSNAVGROUP_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/PHandle.h>

namespace vgui
{

class Panel;

//-----------------------------------------------------------------------------
// Purpose: Handles navigation through a set of panels, with tab order & hotkeys
//-----------------------------------------------------------------------------
class FocusNavGroup
{
public:
	FocusNavGroup(Panel *panel);
	~FocusNavGroup();
	virtual Panel *GetDefaultPanel();	// returns a pointer to the panel with the default focus

	virtual void SetDefaultButton(Panel *panel);	// sets which panel should receive input when ENTER is hit
	virtual Panel *GetDefaultButton();				    // panel which receives default input when ENTER is hit, if current focused item cannot accept ENTER
	virtual Panel *GetCurrentDefaultButton();			// panel which receives input when ENTER is hit
	virtual Panel *FindPanelByHotkey(wchar_t key);		// finds the panel which is activated by the specified key
	virtual bool RequestFocusPrev(Panel *panel = NULL); // if panel is NULL, then the tab increment is based last known panel that had key focus
	virtual bool RequestFocusNext(Panel *panel = NULL);	

	virtual Panel *GetCurrentFocus();
	virtual Panel *SetCurrentFocus(Panel *panel, Panel *defaultPanel);  // returns the Default panel

	// sets the panel that owns this FocusNavGroup to be the root in the focus traversal heirarchy
	// focus change via KEY_TAB will only travel to children of this main panel
	virtual void SetFocusTopLevel(bool state);

    virtual void SetCurrentDefaultButton(Panel *panel);  
private:
	PHandle _defaultButton;
    PHandle _currentDefaultButton;
	Panel *_mainPanel;
	PHandle _currentFocus;
	bool _topLevelFocus;

};

} // namespace vgui

#endif // FOCUSNAVGROUP_H
