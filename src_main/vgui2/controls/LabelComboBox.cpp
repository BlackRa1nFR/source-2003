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

#include <vgui/IScheme.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>

#include <vgui_controls/LabelComboBox.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/Menu.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: This MenuButton holds the menu. The parent is a button, who's
//			mouse click events shoulc be redirected to this menu's events in order
//			for the menu to open/close in response to both buttons.
//			This class overrides the GetColor functions so that the button is only
//			highlighted if you put the mouse over it or click on it.
//-----------------------------------------------------------------------------
class LabelComboBoxButton : public MenuButton
{
public:
	LabelComboBoxButton(Button *parent, const char *panelName, const char *text);
	virtual void ApplySchemeSettings(IScheme *pScheme);
	IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus);
	virtual Color GetButtonFgColor();
	virtual Color GetButtonBgColor();
private:
	Button *m_pParent;
	Color _defaultFgColor, _defaultBgColor;
};


LabelComboBoxButton::LabelComboBoxButton(Button *parent, const char *panelName, const char *text) : MenuButton(parent, panelName, text)
{
	m_pParent = parent;
}

void LabelComboBoxButton::ApplySchemeSettings(IScheme *pScheme)
{
	MenuButton::ApplySchemeSettings(pScheme);
	
	SetFont(pScheme->GetFont( "Marlett", IsProportional()));
	SetContentAlignment(Label::a_west);
	SetTextInset(3, 0);
	SetDefaultBorder(pScheme->GetBorder( "ScrollBarButtonBorder"));
	
	SetArmedColor(GetSchemeColor("MenuButton/ArmedArrowColor", pScheme), GetSchemeColor("MenuButton/ArmedBgColor", pScheme));
	SetDepressedColor(GetSchemeColor("MenuButton/ArmedArrowColor", pScheme), GetSchemeColor("MenuButton/ArmedBgColor", pScheme));
	
	_defaultFgColor = GetSchemeColor("MenuButton/ButtonArrowColor", pScheme);
	_defaultBgColor = GetSchemeColor("MenuButton/ButtonBgColor", pScheme);
	SetDefaultColor(_defaultFgColor, _defaultBgColor);
}

IBorder *LabelComboBoxButton::GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
{
	return NULL;
	//		return Button::GetBorder(depressed, armed, selected, keyfocus);
}

// Only highlight if this button is armed
Color LabelComboBoxButton::GetButtonFgColor()
{
	if ( IsArmed() )
	{
		return Button::GetButtonFgColor();	
	}
	else 
	{
		return _defaultFgColor;	
	}
}

// Only highlight if this button is armed
Color LabelComboBoxButton::GetButtonBgColor()
{
	if ( IsArmed() )
	{
		return Button::GetButtonBgColor();
	}
	else 
	{
		return _defaultBgColor;	
	}
}

} // end using namespace vgui


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
LabelComboBox::LabelComboBox(Panel *parent, const char *panelName) : vgui::Button(parent, panelName, "")
{
	_statusImage = NULL;	// will update status image later

	SetMouseClickEnabled(MOUSE_RIGHT, true);

	SetContentAlignment(Label::a_west);

	// create the drop-down menu
	m_pDropDown = new Menu(this, NULL);
	m_pDropDown->SetVisible(false);
	m_pDropDown->AddActionSignalTarget(this);

	// button to Activate menu
	m_pButton = new LabelComboBoxButton(this, NULL, "u");	
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetMenu(m_pDropDown);

	m_pButton->SetOpenDirection(MenuButton::ALIGN_WITH_PARENT);
	m_pButton->SetOpenOffsetY(2); // want menu to open 2 pixels below the combobox
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
LabelComboBox::~LabelComboBox()
{
	delete m_pDropDown;
	delete m_pButton;
}
  
//-----------------------------------------------------------------------------
// Purpose: Return the appropriate box border colors
//-----------------------------------------------------------------------------
IBorder *LabelComboBox::GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
{
	return BaseClass::GetBorder(true, false, true, false);
}

//-----------------------------------------------------------------------------
// Purpose: Adds an item to the drop down menu
// Input  : char *itemText - name of the menu item
//          message - message to be sent when menu item is selected
//          target - object to send the message to 
//-----------------------------------------------------------------------------
void LabelComboBox::AddItem( const char *itemText, KeyValues *message, Panel *target )
{
	// add the element to the menu
	m_pDropDown->AddMenuItem( itemText, message, target );
}

//-----------------------------------------------------------------------------
// Purpose: Set up the graphics for display
//-----------------------------------------------------------------------------
void LabelComboBox::PerformLayout()
{
	m_pDropDown->SetFixedWidth(GetWide());

	int wide, tall;
	GetPaintSize(wide, tall);

	m_pButton->SetBounds( (wide - tall) + 4, 0, tall , tall );

	if (!m_pDropDown->IsPopup())
	{
		m_pDropDown->MakePopup(false);
	}
	
	m_pButton->SetEnabled(IsEnabled());

	// chain back
	BaseClass::PerformLayout();

}

//-----------------------------------------------------------------------------
// Purpose: Here we override the Button's function so that if we press
//          this button we do a click in the scroll bar button, triggering the 
//          dropdown menu.
//-----------------------------------------------------------------------------
void LabelComboBox::OnMousePressed(MouseCode code)
{
   m_pButton->DoClick();
}

//-----------------------------------------------------------------------------
// Purpose: Must have this function.to handle hiding the menu if we click
//			on this button.  
//-----------------------------------------------------------------------------
void LabelComboBox::HideMenu()
{
  	m_pButton->HideMenu();
}





