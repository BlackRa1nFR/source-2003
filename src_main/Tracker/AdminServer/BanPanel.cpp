//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>

#include "BanPanel.h"
#include "PlayerContextMenu.h"


#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_ISurface.h>
#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>
#include <VGUI_TextEntry.h>
#include <VGUI_Button.h>
#include <VGUI_ToggleButton.h>
#include <VGUI_RadioButton.h>
#include <VGUI_ListPanel.h>
#include <VGUI_ComboBox.h>
#include <VGUI_PHandle.h>
#include <VGUI_PropertySheet.h>

using namespace vgui;
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBanPanel::CBanPanel(vgui::Panel *parent, const char *name) : PropertyPage(parent, name)
{
	m_pParent=parent;
	
	m_pBanListPanel = new BanListPanel(this,"Ban list");

	m_pAddButton = new Button(this, "Add", "&Add");
	m_pRemoveButton = new Button(this, "Remove", "&Remove");
	m_pChangeButton = new Button(this, "Change", "&Change");

	m_pAddButton->SetCommand(new KeyValues("addban"));
	m_pRemoveButton->SetCommand(new KeyValues("removeban"));
	m_pChangeButton->SetCommand(new KeyValues("changeban"));

	m_pBanContextMenu = new CBanContextMenu(this);
	m_pBanContextMenu->SetVisible(false);


	LoadControlSettings("Admin\\BanPanel.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBanPanel::~CBanPanel()
{

}

//-----------------------------------------------------------------------------
// Purpose: Activates the page
//-----------------------------------------------------------------------------
void CBanPanel::OnPageShow()
{
}

//-----------------------------------------------------------------------------
// Purpose: Hides the page
//-----------------------------------------------------------------------------
void CBanPanel::OnPageHide()
{
}



//-----------------------------------------------------------------------------
// Purpose: Relayouts the data
//-----------------------------------------------------------------------------
void CBanPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int buttonWidth=65,gap=5;
	int buttonHeight=20;

	// setup the layout of the panels
	m_pBanListPanel->SetBounds(5,5,GetWide()-10,GetTall()-buttonHeight-gap-4);

	m_pAddButton->SetBounds(5,GetTall()-buttonHeight-3,buttonWidth,buttonHeight);
	m_pRemoveButton->SetBounds(5+buttonWidth+gap,GetTall()-buttonHeight-3,buttonWidth,buttonHeight);
	m_pChangeButton->SetBounds(5+(buttonWidth+gap)*2,GetTall()-buttonHeight-3,buttonWidth,buttonHeight);


}


//-----------------------------------------------------------------------------
// Purpose: Called when the game dir combo box is changed
//-----------------------------------------------------------------------------
void CBanPanel::OnTextChanged(Panel *panel, const char *text)
{
// BUG - TextEntry NEVER lets the enter key through... This doesn't work

	if( text[strlen(text)-1]=='\n') // the enter key was just pressed :)
	{
	//	OnSendChat();
	}
}


//-----------------------------------------------------------------------------
// Purpose: double click handler for the player list, doesn't do anything currently
//----------------------------------------------------------------------------
void CBanPanel::BanListPanel::OnMouseDoublePressed(vgui::MouseCode code)
{	
	// do nothing, what should the default be??
/*	if (GetNumSelectedRows())
	{
		unsigned int banD =GetSelectedRow(0);
		PostMessage(m_pParent->GetVPanel(),  new KeyValues("addban", "banID", playerID));
	}
	*/
	
}



//-----------------------------------------------------------------------------
// Purpose: this is currently unused, we go straight to the message map :)
//----------------------------------------------------------------------------
void CBanPanel::OnCommand(const char *command)
{
	if (!stricmp(command, "addban"))
	{
			PostMessage(m_pParent->GetVPanel(),new KeyValues("addban", "banID",0));
	}

	if( m_pBanListPanel->GetNumSelectedRows())  // if a user is selected
	{
		int playerID = m_pBanListPanel->GetDataItem(m_pBanListPanel->GetSelectedRow(0))->userData;
		if (!stricmp(command, "removeban"))
		{
			PostMessage(m_pParent->GetVPanel(),new KeyValues("removeban", "playerID",playerID));
		}
		else if (!stricmp(command, "changeban"))
		{
			PostMessage(m_pParent->GetVPanel(),new KeyValues("changeban",  "playerID",playerID));
		}
	}
	
}



void CBanPanel::OnEffectPlayer(vgui::KeyValues *data)
{
	// you MUST make a copy, if you use the original you get a horrid crash
	KeyValues *kv=data->MakeCopy();
	PostMessage(m_pParent->GetVPanel(),kv);
}

//-----------------------------------------------------------------------------
// Purpose: opens context menu (user right clicked on a server)
//-----------------------------------------------------------------------------
void CBanPanel::OnOpenContextMenu(int row)
{
	if (m_pBanListPanel->IsVisible() && m_pBanListPanel->IsCursorOver()
		&& m_pBanListPanel->GetNumSelectedRows())
	// show the ban changing menu IF its the visible panel and the cursor is
	// over it 
	{
	
		unsigned int banID =m_pBanListPanel->GetSelectedRow(0);
			
		// activate context menu
		m_pBanContextMenu->ShowMenu(this, banID);
	} 
	else
	{
		m_pBanContextMenu->ShowMenu(this, -1);
	}
}


//-----------------------------------------------------------------------------
// Purpose: various function passthroughts for the list panel
//----------------------------------------------------------------------------

void CBanPanel::AddColumnHeader(int index, const char *columnName, const char *columnText, 
		int width, bool isTextImage , bool sliderResizable , bool windowResizable )
{
	m_pBanListPanel->AddColumnHeader(index,columnName,columnText,width,isTextImage,sliderResizable,windowResizable);	
}

void CBanPanel::SetSortFunc(int column, SortFunc *func) 
{
	m_pBanListPanel->SetSortFunc(column,func);
}

void CBanPanel::SetSortColumn(int column) 
{
	m_pBanListPanel->SetSortColumn(column);
}

int CBanPanel::GetNumSelectedRows()
{
	return m_pBanListPanel->GetNumSelectedRows();
}

int CBanPanel::GetSelectedRow(int selectionIndex)
{
	return m_pBanListPanel->GetSelectedRow(selectionIndex);
}

void CBanPanel::DeleteAllItems()
{
	m_pBanListPanel->DeleteAllItems();
}

int CBanPanel:: AddItem(KeyValues *data, unsigned int userData  )
{
	return m_pBanListPanel->AddItem(data,userData);
}

void CBanPanel::SortList( void )
{
	m_pBanListPanel->SortList();
}

vgui::ListPanel::DATAITEM *CBanPanel::GetDataItem( int itemIndex )
{
	return m_pBanListPanel->GetDataItem(itemIndex);
}

KeyValues *CBanPanel::GetItem(int itemIndex)
{
	return m_pBanListPanel->GetItem(itemIndex);
}




//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CBanPanel::m_MessageMap[] =
{	
	MAP_MESSAGE_INT( CBanPanel, "OpenContextMenu", OnOpenContextMenu, "row" ),

	MAP_MESSAGE_PARAMS( CBanPanel, "Addban", OnEffectPlayer ), // kick a player menu
	MAP_MESSAGE_PARAMS( CBanPanel, "Removeban", OnEffectPlayer ),
	MAP_MESSAGE_PARAMS( CBanPanel, "Changeban", OnEffectPlayer ),

};

IMPLEMENT_PANELMAP( CBanPanel, Frame );
