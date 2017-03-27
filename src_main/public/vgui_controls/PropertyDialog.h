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

#ifndef PROPERTYDIALOG_H
#define PROPERTYDIALOG_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Frame.h>

namespace vgui
{

class Button;
class PropertySheet;
enum KeyCode;

//-----------------------------------------------------------------------------
// Purpose: Simple frame that holds a property sheet
//-----------------------------------------------------------------------------
class PropertyDialog : public Frame
{
public:
	PropertyDialog(Panel *parent, const char *panelName);
	~PropertyDialog();

	// returns a pointer to the PropertySheet this dialog encapsulates 
	virtual PropertySheet *GetPropertySheet();

	// wrapper for PropertySheet interface
	virtual void AddPage(Panel *page, const char *title);
	virtual Panel *GetActivePage();
	virtual void ResetAllData();

	// sets the text on the OK/Cancel buttons, overriding the default
	void SetOKButtonText(const char *text);
	void SetCancelButtonText(const char *text);
	void SetApplyButtonText(const char *text);

	// changes the visibility of the buttons
	void SetOKButtonVisible(bool state);
	void SetCancelButtonVisible(bool state);
	void SetApplyButtonVisible(bool state);

	/* MESSAGES SENT
		"ResetData"			- sent when page is loaded.  Data should be reloaded from document into controls.
		"ApplyChanges"		- sent when the OK / Apply button is pressed.  Changed data should be written into document.
	*/

	DECLARE_PANELMAP();

protected:
	// Called when the OK button is pressed.  Simply closes the dialog.
	virtual void OnOK();

	// vgui overrides
	virtual void PerformLayout();
	virtual void OnCommand(const char *command);
	virtual void ActivateBuildMode();
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void RequestFocus(int direction = 0);

	void OnApplyButtonEnable();
	void EnableApplyButton(bool bEnable);
	
private:
	PropertySheet *_propertySheet;
	Button *_okButton;
	Button *_cancelButton;
	Button *_applyButton;
	typedef Frame BaseClass;
};

}; // vgui

#endif // PROPERTYDIALOG_H
