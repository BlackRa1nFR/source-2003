//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "DemoPage.h"

#include <VGUI_IVGui.h>
#include <VGUI_Controls.h>

#include <VGUI_KeyValues.h>
#include <VGUI_ComboBox.h>


using namespace vgui;

// Combo boxes are boxes that display text and have a menu attached.
// Selecting an item from the menu changes the displayed text in the box.

class ComboBoxDemo: public DemoPage
{
public:
	ComboBoxDemo(Panel *parent, const char *name);
	~ComboBoxDemo();
	
private:
	ComboBox *m_pComboBox;
	

};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ComboBoxDemo::ComboBoxDemo(Panel *parent, const char *name) : DemoPage(parent, name)
{
	// Create a new combo box.
	// The first arg is the parent, the second the name
	// The third arg is the number of items that will be in the list
	// The fourth arg is if the box is editable or not.
	m_pComboBox = new ComboBox(this, "Directions", 6, false);

	// Position the box.
	m_pComboBox->SetPos(100, 100);

	// Set the width of the Combo box so any element selected will display nicely.
	m_pComboBox->SetWide(80);

	// Add text selections to the menu list
	m_pComboBox->AddItem("Right");
	m_pComboBox->AddItem("Left");
	m_pComboBox->AddItem("Up");
	m_pComboBox->AddItem("Down");
	m_pComboBox->AddItem("Forward");
	m_pComboBox->AddItem("Backward");

	// Activate the first item in the list, so our box will start out
	// with a default selection. ("Right")
	m_pComboBox->ActivateItem(0);

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
ComboBoxDemo::~ComboBoxDemo()
{
}




Panel* ComboBoxDemo_Create(Panel *parent)
{
	return new ComboBoxDemo(parent, "ComboBoxDemo");
}


