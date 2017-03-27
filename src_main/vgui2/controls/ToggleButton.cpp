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

#include <vgui/KeyCode.h>

#include <vgui_controls/ToggleButton.h>

#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ToggleButton::ToggleButton(Panel *parent, const char *panelName, const char* text) : Button(parent, panelName, text)
{
	SetButtonActivationType(ACTIVATE_ONPRESSED);
}

//-----------------------------------------------------------------------------
// Purpose: Turns double-click into normal click
//-----------------------------------------------------------------------------
void ToggleButton::OnMouseDoublePressed(MouseCode code)
{
	OnMousePressed(code);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color ToggleButton::GetButtonFgColor()
{
	if (IsSelected())
	{
		// highlight the text when depressed
		return _selectedColor;
	}
	else
	{
		return BaseClass::GetButtonFgColor();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ToggleButton::CanBeDefaultButton(void)
{
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Toggles the state of the button
//-----------------------------------------------------------------------------
void ToggleButton::DoClick()
{
	if (IsSelected())
	{
		ForceDepressed(false);
	}
	else if (!IsSelected())
	{
		ForceDepressed(true);
	}

	SetSelected(!IsSelected());
	FireActionSignal();

	// post a button toggled message
	KeyValues *msg = new KeyValues("ButtonToggled");
	msg->SetInt("state", (int)IsSelected());
	PostActionSignal(msg);
	
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ToggleButton::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	_selectedColor = GetSchemeColor("BrightControlText", pScheme);
}

void ToggleButton::OnKeyCodePressed(KeyCode code)
{
    if (code != KEY_ENTER)
    {
        BaseClass::OnKeyCodePressed(code);
    }
}

