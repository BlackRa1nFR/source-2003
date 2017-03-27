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

#ifndef RADIOBUTTON_H
#define RADIOBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/ToggleButton.h>

class RadioImage;

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Radio buttons are automatically selected into groups by who their
//			parent is. At most one radio button is active at any time.
//-----------------------------------------------------------------------------
class RadioButton : public ToggleButton
{
public:
	RadioButton(Panel *parent, const char *panelName, const char *text);
	~RadioButton();

	// Set the radio button checked. When a radio button is checked, a 
	// message is sent to all other radio buttons in the same group so
	// they will become unchecked.
	virtual void SetSelected(bool state);

	// Get the tab position of the radio button with the set of radio buttons
	// A group of RadioButtons must have the same TabPosition, with [1, n] subtabpositions
	virtual int GetSubTabPosition();
	virtual void SetSubTabPosition(int position);

	// Return the RadioButton's real tab position (its Panel one changes)
	virtual int GetRadioTabPosition();

protected:
	virtual void DoClick();

	virtual void Paint();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnRadioButtonChecked(Panel *panel, int tabposition);
	virtual void OnKeyCodeTyped(KeyCode code);

	virtual IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus);

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void GetSettings(KeyValues *outResourceData);
	virtual const char *GetDescription();
	virtual void PerformLayout();

	RadioButton *FindBestRadioButton(int direction);

	DECLARE_PANELMAP();

private:


	RadioImage *_radioBoxImage;
	int _oldTabPosition;
	Color _selectedFgColor;

	int _subTabPosition;	// tab position with the radio button list

	typedef ToggleButton BaseClass;
};

}; // namespace vgui

#endif // RADIOBUTTON_H
