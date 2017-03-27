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

#ifndef CHECKBUTTON_H
#define CHECKBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/ToggleButton.h>

class CheckImage;

namespace vgui
{

class TextImage;

//-----------------------------------------------------------------------------
// Purpose: Tick-box button
//-----------------------------------------------------------------------------
class CheckButton : public ToggleButton
{
public:
	CheckButton(Panel *parent, const char *panelName, const char *text);
	~CheckButton();

	// Check the button
	virtual void SetSelected(bool state);

	// Set text label next to the checkbox.
	virtual void SetText(const char *text);
	virtual void SetText(const wchar_t *text);

protected:
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnCheckButtonChecked(Panel *panel);
	virtual Color GetButtonFgColor();

	virtual IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus);

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void GetSettings(KeyValues *outResourceData);

	/* MESSAGES SENT
		"CheckButtonChecked" - sent when the check button state is changed
			"state"	- button state: 1 is checked, 0 is unchecked
	*/

	DECLARE_PANELMAP();

private:
	enum { CHECK_INSET = 6 };
	CheckImage *_checkBoxImage;
	Color _selectedFgColor;

	typedef vgui::ToggleButton BaseClass;
};

} // namespace vgui

#endif // CHECKBUTTON_H
