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
#include <VGUI_SectionedListPanel.h>


using namespace vgui;


class SampleListPanelBoth: public DemoPage
{
	public:
		SampleListPanelBoth(Panel *parent, const char *name);
		~SampleListPanelBoth();

		void onButtonClicked();
	
	private:
		SectionedListPanel *m_pSectionedListPanel;
		
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
SampleListPanelBoth::SampleListPanelBoth(Panel *parent, const char *name) : DemoPage(parent, name)
{
	// Create a list panel.
	m_pSectionedListPanel = new SectionedListPanel(this, "AListPanel");

	// Add a new section
	m_pSectionedListPanel->AddSection(0, "LIST ITEMS");
	m_pSectionedListPanel->AddColumnToSection(0, "items", SectionedListPanel::COLUMN_TEXT);

	// Add items to the list
	KeyValues *data = new KeyValues ("items");
	data->SetString("items", "Many actions");
	m_pSectionedListPanel->AddItem(0, 0, data);

	data->SetString("items", "Performed on");
	m_pSectionedListPanel->AddItem(1, 0, data);

	data->SetString("items", "List items can");
	m_pSectionedListPanel->AddItem(2, 0, data);

	data->SetString("items", "Only be accessed");
	m_pSectionedListPanel->AddItem(3, 0, data);

	data->SetString("items", "Using the right-");
	m_pSectionedListPanel->AddItem(4, 0, data);

	data->SetString("items", "Click menu");
	m_pSectionedListPanel->AddItem(5, 0, data);

	// Add a new section
	//m_pSectionedListPanel->AddSection(1, "RIGHT CLICK");
	m_pSectionedListPanel->AddColumnToSection(0, "items", SectionedListPanel::COLUMN_TEXT);

	// Add items to the list
	data->SetString("items", "Right-click the item");
	m_pSectionedListPanel->AddItem(10, 1, data);

	data->SetString("items", "To access its");
	m_pSectionedListPanel->AddItem(11, 1, data);

	data->SetString("items", "List of associated");
	m_pSectionedListPanel->AddItem(12, 1, data);

	data->SetString("items", "Commands");
	m_pSectionedListPanel->AddItem(13, 1, data);


	// Add a new section
	m_pSectionedListPanel->AddSection(2, "RIGHT CLICK");
	m_pSectionedListPanel->AddColumnToSection(2, "items", SectionedListPanel::COLUMN_TEXT);

	// Add items to the list
	data->SetString("items", "Right-click the item");
	m_pSectionedListPanel->AddItem(20, 2, data);

	data->SetString("items", "To access its");
	m_pSectionedListPanel->AddItem(21, 2, data);

	data->SetString("items", "List of associated");
	m_pSectionedListPanel->AddItem(22, 2, data);

	data->SetString("items", "Commands");
	m_pSectionedListPanel->AddItem(23, 2, data);


	// Set its position.
	m_pSectionedListPanel->SetPos(90, 25);
	m_pSectionedListPanel->SetSize(200, 150);
	
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
SampleListPanelBoth::~SampleListPanelBoth()
{
}




Panel* SampleListPanelBoth_Create(Panel *parent)
{
	return new SampleListPanelBoth(parent, "List Panel - both");
}


