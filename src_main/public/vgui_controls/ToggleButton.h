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

#ifndef TOGGLEBUTTON_H
#define TOGGLEBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Button.h>

namespace vgui
{

enum MouseCode;

//-----------------------------------------------------------------------------
// Purpose: Type of button that when pressed stays selected & depressed until pressed again
//-----------------------------------------------------------------------------
class ToggleButton : public Button
{
public:
	ToggleButton(Panel *parent, const char *panelName, const char *text);

	virtual void DoClick();

	/* messages sent (get via AddActionSignalTarget()):
		"ButtonToggled"
			int "state"
	*/

protected:
	// overrides
	virtual void OnMouseDoublePressed(MouseCode code);

	virtual Color GetButtonFgColor();
	virtual void ApplySchemeSettings(IScheme *pScheme);

    virtual bool CanBeDefaultButton(void);
    virtual void OnKeyCodePressed(KeyCode code);

private:
	Color _selectedColor;
	typedef vgui::Button BaseClass;
};

} // namespace vgui

#endif // TOGGLEBUTTON_H
