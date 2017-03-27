//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>

#include "PlayerPanel.h"
#include "PlayerContextMenu.h"
#include "PlayerListCompare.h"


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

#include "util.h"

using namespace vgui;
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPlayerPanel::CPlayerPanel(vgui::Panel *parent, const char *name) : PropertyPage(parent, name)
{
	m_bHasAdminMod=false;
	m_pParent=parent;
	
	m_pPlayerListPanel = new PlayerListPanel(this,"Players list");

	m_pPlayerListPanel->AddColumnHeader(0, "name", util->GetString("Name"), 200, true, RESIZABLE, RESIZABLE );
	m_pPlayerListPanel->AddColumnHeader(1, "authid", util->GetString("AuthID"), 100, true,  RESIZABLE, NOT_RESIZABLE);
	m_pPlayerListPanel->AddColumnHeader(2, "ping", util->GetString("Ping"), 50, true,  RESIZABLE, NOT_RESIZABLE);
	m_pPlayerListPanel->AddColumnHeader(3, "loss", util->GetString("Loss"), 50, true,  RESIZABLE, NOT_RESIZABLE);
	m_pPlayerListPanel->AddColumnHeader(4, "frags", util->GetString("Frags"), 50, true,  RESIZABLE, NOT_RESIZABLE);
	m_pPlayerListPanel->AddColumnHeader(5, "time", util->GetString("Time"), 75, true,  RESIZABLE, NOT_RESIZABLE);

	m_pPlayerListPanel->SetSortFunc(0, PlayerNameCompare);
	m_pPlayerListPanel->SetSortFunc(1, PlayerAuthCompare);
	m_pPlayerListPanel->SetSortFunc(2, PlayerPingCompare);
	m_pPlayerListPanel->SetSortFunc(3, PlayerLossCompare);
	m_pPlayerListPanel->SetSortFunc(4, PlayerFragsCompare);
	m_pPlayerListPanel->SetSortFunc(5, PlayerTimeCompare);

	// Sort by ping time by default
	m_pPlayerListPanel->SetSortColumn(0);


	m_pKickButton = new Button(this, "Kick", "&Kick");
	m_pChatButton = new Button(this, "Chat", "&Chat");
	m_pSlapButton = new Button(this, "Slap", "&Slap");
	m_pBanButton = new Button(this, "Ban", "&Ban");

	m_pKickButton->SetCommand(new KeyValues("kick"));
	m_pChatButton->SetCommand(new KeyValues("chat"));
	m_pSlapButton->SetCommand(new KeyValues("slap"));
	m_pBanButton->SetCommand(new KeyValues("ban"));

	m_pPlayerContextMenu = new CPlayerContextMenu(this);
	m_pPlayerContextMenu->SetVisible(false);


	LoadControlSettings("Admin\\PlayerPanel.res");
	
	SetAdminMod(false);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CPlayerPanel::~CPlayerPanel()
{

}

//-----------------------------------------------------------------------------
// Purpose: Activates the page
//-----------------------------------------------------------------------------
void CPlayerPanel::OnPageShow()
{
}

//-----------------------------------------------------------------------------
// Purpose: Hides the page
//-----------------------------------------------------------------------------
void CPlayerPanel::OnPageHide()
{
}

vgui::KeyValues *CPlayerPanel::GetSelected()
{
	return m_pPlayerListPanel->GetItem(m_pPlayerListPanel->GetSelectedRow(0));
}

//-----------------------------------------------------------------------------
// Purpose: Relayouts the data
//-----------------------------------------------------------------------------
void CPlayerPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int buttonWidth=40,gap=5;
	int buttonHeight=20;

	// setup the layout of the panels
	m_pPlayerListPanel->SetBounds(5,5,GetWide()-10,GetTall()-buttonHeight-gap-4);

	m_pKickButton->SetBounds(5,GetTall()-buttonHeight-3,buttonWidth,buttonHeight);
	m_pBanButton->SetBounds(5+buttonWidth+gap,GetTall()-buttonHeight-3,buttonWidth,buttonHeight);
	m_pSlapButton->SetBounds(5+(buttonWidth+gap)*2,GetTall()-buttonHeight-3,buttonWidth,buttonHeight);
	m_pChatButton->SetBounds(5+(buttonWidth+gap)*3,GetTall()-buttonHeight-3,buttonWidth+5,buttonHeight);


}


//-----------------------------------------------------------------------------
// Purpose: Called when the game dir combo box is changed
//-----------------------------------------------------------------------------
void CPlayerPanel::OnTextChanged(Panel *panel, const char *text)
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
void CPlayerPanel::PlayerListPanel::OnMouseDoublePressed(vgui::MouseCode code)
{	
	// do nothing, what should the default be??
/*	if (GetNumSelectedRows())
	{
		unsigned int playerID =GetSelectedRow(0);
		PostMessage(m_pParent->GetVPanel(),  new KeyValues("Chat", "playerID", playerID));
	}
	*/
	
}


//-----------------------------------------------------------------------------
// Purpose: sets whether the server has admin mod, true = yes, false = no
//----------------------------------------------------------------------------
void CPlayerPanel::SetAdminMod(bool state)
{
	m_bHasAdminMod=state;

	m_pSlapButton->SetEnabled(state);
	m_pChatButton->SetEnabled(state);
	m_pSlapButton->SetVisible(state);
	m_pChatButton->SetVisible(state);

}

void CPlayerPanel::SetHasRcon(bool state)
{
	m_bHasRcon=state;
	
	m_pKickButton->SetEnabled(state);
	m_pBanButton->SetEnabled(state);
	m_pSlapButton->SetEnabled(state);
	m_pChatButton->SetEnabled(state);

}

void CPlayerPanel::OnCommand(const char *command)
{
	if( m_bHasRcon && m_pPlayerListPanel->GetNumSelectedRows())  // if a user is selected
	{
		int playerID = m_pPlayerListPanel->GetDataItem(m_pPlayerListPanel->GetSelectedRow(0))->userData;
		if (!stricmp(command, "kick"))
		{
			PostMessage(m_pParent->GetVPanel(),new KeyValues("Kick", "playerID",playerID));
		}
		else if (!stricmp(command, "ban"))
		{
			PostMessage(m_pParent->GetVPanel(),new KeyValues("Ban", "playerID",playerID));
		}
		else if (!stricmp(command, "slap"))
		{
			PostMessage(m_pParent->GetVPanel(),new KeyValues("Slap",  "playerID",playerID));
		}
		else if (!stricmp(command, "chat"))
		{
			PostMessage(m_pParent->GetVPanel(),new KeyValues("chat", "playerID",playerID));
		}
	}
}



void CPlayerPanel::OnEffectPlayer(vgui::KeyValues *data)
{
	// you MUST make a copy, if you use the original you get a horrid crash
	KeyValues *kv=data->MakeCopy();
	PostMessage(m_pParent->GetVPanel(),kv);
}

//-----------------------------------------------------------------------------
// Purpose: opens context menu (user right clicked on a server)
//-----------------------------------------------------------------------------
void CPlayerPanel::OnOpenContextMenu(int row)
{
	if (m_bHasRcon && m_pPlayerListPanel->IsVisible() && m_pPlayerListPanel->IsCursorOver() && 
		m_pPlayerListPanel->GetNumSelectedRows()  ) 
		// show the player menus only if its the visible panel and the cursor is over it
		// AND rcon is working
	{
		// get the server
			unsigned int playerID = m_pPlayerListPanel->GetDataItem(m_pPlayerListPanel->GetSelectedRow(0))->userData;
		
		// activate context menu
		m_pPlayerContextMenu->ShowMenu(this, playerID);
		if(m_bHasAdminMod) 
		{
			m_pPlayerContextMenu->AddMenuItem("Slap", "&Slap Player", new KeyValues("Slap", "playerID",playerID), this);
			m_pPlayerContextMenu->AddMenuItem("chat", "&Message Player", new KeyValues("chat", "playerID",playerID), this);
		}
	}
}

void CPlayerPanel::NewPlayerList(CUtlVector<Players_t> *players)
{
	// clear the existing panel
		m_pPlayerListPanel->DeleteAllItems();

		// input the new player list
		KeyValues *kv;

	
		int index;
			
		for (int i = 0; i < players->Count(); i++)
		{
			char timestr[10];

			kv = new KeyValues("PlayerX");
			kv->SetString("name", (*players)[i].name);
			kv->SetInt("frags", (*players)[i].frags);
			_snprintf(timestr,10,"%0.2i:%0.2i:%0.2i",(int)(((*players)[i].time)/3600),  // hours
													  (int)(((*players)[i].time)/60)%60, // minutes
													  (int)((*players)[i].time)%60 // seconds
													);

		//	kv->SetFloat("time", (*players)[i].time);
			kv->SetString("time",timestr);
			kv->SetString("authid",(*players)[i].authid);
			kv->SetInt("ping",(*players)[i].ping);
			kv->SetInt("loss",(*players)[i].loss);
		    index =	m_pPlayerListPanel->AddItem(kv,(*players)[i].userid);
			
		}
		m_pPlayerListPanel->SortList();
		m_pPlayerListPanel->Repaint();	
}





//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CPlayerPanel::m_MessageMap[] =
{	
	MAP_MESSAGE_INT( CPlayerPanel, "OpenContextMenu", OnOpenContextMenu, "row" ),

	MAP_MESSAGE_PARAMS( CPlayerPanel, "Kick", OnEffectPlayer ), // kick a player menu
	MAP_MESSAGE_PARAMS( CPlayerPanel, "Ban", OnEffectPlayer ),
	MAP_MESSAGE_PARAMS( CPlayerPanel, "Slap", OnEffectPlayer ),
	MAP_MESSAGE_PARAMS( CPlayerPanel, "Chat", OnEffectPlayer ), // chat to a player


//	MAP_MESSAGE( CPlayerPanel, "SendChat", OnSendChat ),
//	MAP_MESSAGE_PTR_CONSTCHARPTR( CPlayerPanel, "TextChanged", OnTextChanged, "panel", "text" ),
};

IMPLEMENT_PANELMAP( CPlayerPanel, Frame );
