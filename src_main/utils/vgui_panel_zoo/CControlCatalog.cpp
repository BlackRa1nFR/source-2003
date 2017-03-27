//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================


#include "CControlCatalog.h"
#include "stdio.h"

#include <VGUI_ISurface.h>
#include <VGUI_Controls.h>
#include <VGUI_KeyValues.h>

#include <VGUI_Label.h>
#include <VGUI_ComboBox.h>

#include <VGUI_IVGui.h> // for dprinf statements

#include "filesystem.h"


using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CControlCatalog::CControlCatalog(): Frame(NULL, "PanelZoo")
{
	SetTitle("VGUI SDK Sample Application", true);
	// calculate defaults
	int x, y, wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	
	int dwide, dtall;
	dwide = 535;
	dtall = 405;
	x = (int)((wide - dwide) * 0.5);
	y = (int)((tall - dtall) * 0.5);
	SetBounds (x, y, dwide, dtall);

	// Add all demos to the panel list

	// These are SDK control display demos
	m_PanelList.AddToTail(SampleButtons_Create(this));
	m_PanelList.AddToTail(SampleMenus_Create(this));
	m_PanelList.AddToTail(SampleDropDowns_Create(this));
	m_PanelList.AddToTail(SampleListPanelColumns_Create(this));
	m_PanelList.AddToTail(SampleListPanelCats_Create(this));
	m_PanelList.AddToTail(SampleListPanelBoth_Create(this));
	m_PanelList.AddToTail(SampleRadioButtons_Create(this));
	m_PanelList.AddToTail(SampleCheckButtons_Create(this));
	m_PanelList.AddToTail(SampleTabs_Create(this));
	m_PanelList.AddToTail(SampleEditFields_Create(this));
	m_PanelList.AddToTail(SampleSliders_Create(this));
	m_PanelList.AddToTail(DefaultColors_Create(this));

	// These are panel zoo demos
	// These have commented source files with a step by step
	// of how to make and use each control
	// They will have resource file attributes eventually.
	m_PanelList.AddToTail(ImageDemo_Create(this));
	m_PanelList.AddToTail(ImagePanelDemo_Create(this));
	m_PanelList.AddToTail(TextImageDemo_Create(this));
	m_PanelList.AddToTail(LabelDemo_Create(this));
	m_PanelList.AddToTail(Label2Demo_Create(this));
	m_PanelList.AddToTail(TextEntryDemo_Create(this));
	m_PanelList.AddToTail(TextEntryDemo2_Create(this));
	m_PanelList.AddToTail(TextEntryDemo3_Create(this));
	m_PanelList.AddToTail(TextEntryDemo4_Create(this));
	m_PanelList.AddToTail(TextEntryDemo5_Create(this));
	m_PanelList.AddToTail(ButtonDemo_Create(this));
	m_PanelList.AddToTail(ButtonDemo2_Create(this));
	m_PanelList.AddToTail(CheckButtonDemo_Create(this));
	m_PanelList.AddToTail(ToggleButtonDemo_Create(this));
	m_PanelList.AddToTail(RadioButtonDemo_Create(this));
	m_PanelList.AddToTail(MenuDemo_Create(this));
	m_PanelList.AddToTail(MenuDemo2_Create(this));
	m_PanelList.AddToTail(CascadingMenuDemo_Create(this));
	m_PanelList.AddToTail(MessageBoxDemo_Create(this));
	m_PanelList.AddToTail(QueryBoxDemo_Create(this));
	m_PanelList.AddToTail(ComboBoxDemo_Create(this));
	m_PanelList.AddToTail(ComboBox2Demo_Create(this));
	m_PanelList.AddToTail(FrameDemo_Create(this));
	m_PanelList.AddToTail(ProgressBarDemo_Create(this));
	m_PanelList.AddToTail(ScrollBarDemo_Create(this));
	m_PanelList.AddToTail(ScrollBar2Demo_Create(this));
	m_PanelList.AddToTail(EditablePanelDemo_Create(this));
	m_PanelList.AddToTail(EditablePanel2Demo_Create(this));
	m_PanelList.AddToTail(ListPanelDemo_Create(this));
	m_PanelList.AddToTail(TooltipsDemo_Create(this));
	m_PanelList.AddToTail(AnimatingImagePanelDemo_Create(this));
	m_PanelList.AddToTail(WizardPanelDemo_Create(this));
	m_PanelList.AddToTail(FileOpenDemo_Create(this));
	m_PanelList.AddToTail(HTMLDemo_Create(this));
	m_PanelList.AddToTail(HTMLDemo2_Create(this));
	m_PanelList.AddToTail(MenuBarDemo_Create(this));


	m_pSelectControl = new ComboBox(this, "ControlSelect", 10, false);

	// Position the box.
	m_pSelectControl->SetPos(90, 50);

	// Set the width of the Combo box so any element selected will display nicely.
	m_pSelectControl->SetWide(180);

	// Add text selections to the menu list
	// These are the names of the panels in the panel list
	for (int i = 0; i < m_PanelList.Size(); i++)
	{
		m_pSelectControl->AddItem(m_PanelList[i]->GetName());
	}

	m_pSelectControl->ActivateItem(0);
	m_pPrevPanel = m_PanelList[0];

	m_pCategoryLabel = new Label (this, "CategoryLabel", "Category");
	m_pCategoryLabel->GetContentSize(wide, tall);
	m_pCategoryLabel->SetSize(wide + Label::Content/2, tall + Label::Content/2);
	m_pCategoryLabel->SetPos(27, 50);
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CControlCatalog::~CControlCatalog()
{ 
}
 
//-----------------------------------------------------------------------------
// Purpose: Handles closing of the dialog - shuts down the whole app
//-----------------------------------------------------------------------------
void CControlCatalog::OnClose()
{
	Frame::OnClose();

	// stop vgui running
	vgui::ivgui()->Stop();
}


//-----------------------------------------------------------------------------
// Purpose: Checks to see if any text in the combobox has changed
//-----------------------------------------------------------------------------
void CControlCatalog::OnTextChanged()
{
	char buf[40];
	m_pSelectControl->GetText(0, buf, 40);

	m_pPrevPanel->SetVisible(false);

	for (int i = 0; i < m_PanelList.Size(); i++)
	{
		if (!strcmp(buf, m_PanelList[i]->GetName()))
		{
			m_PanelList[i]->SetVisible(true);
			m_pPrevPanel = m_PanelList[i];
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CControlCatalog::m_MessageMap[] =
{
	MAP_MESSAGE( CControlCatalog, "TextChanged", OnTextChanged ),	// message from the text entry
};

IMPLEMENT_PANELMAP(CControlCatalog, Frame);




