//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>

#include "ChatPanel.h"


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
CChatPanel::CChatPanel(vgui::Panel *parent, const char *name) : PropertyPage(parent, name)
{
	m_pRcon=NULL;

	m_pServerChatPanel = new TextEntry(this, "ServerChatText");

	m_pServerChatPanel->SetMultiline(true);
	m_pServerChatPanel->SetEnabled(true);
	m_pServerChatPanel->SetEditable(false);
	m_pServerChatPanel->SetVerticalScrollbar(true);
	m_pServerChatPanel->SetRichEdit(false);
	//m_pServerChatPanel->SetMaximumCharCount(-1);
	m_pServerChatPanel->setMaximumCharCount(8000);
	m_pServerChatPanel->SetWrap(true);

	m_pEnterChatPanel = new TextEntry(this,"ChatMessage");
	m_pEnterChatPanel->SetCatchEnterKey(false);
	//m_pServerChatPanel->setMultiline(true);
	//m_pServerChatPanel->setEnabled(true);
	//	m_pEnterChatPanel->SendNewLine(true);
	//m_pServerChatPanel->setEditable(true);

	m_pSendChatButton = new Button(this, "SendChat", "&Send");
	m_pSendChatButton->SetCommand(new KeyValues("SendChat"));
	m_pSendChatButton->SetAsDefaultButton(true);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CChatPanel::~CChatPanel()
{

}

//-----------------------------------------------------------------------------
// Purpose: Activates the page
//-----------------------------------------------------------------------------
void CChatPanel::OnPageShow()
{
	m_pEnterChatPanel->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Hides the page
//-----------------------------------------------------------------------------
void CChatPanel::OnPageHide()
{
}



//-----------------------------------------------------------------------------
// Purpose: Relayouts the data
//-----------------------------------------------------------------------------
void CChatPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	// setup the layout of the panels
	m_pServerChatPanel->SetBounds(5,5,GetWide()-10,GetTall()-35);

	m_pEnterChatPanel->SetBounds(5,GetTall()-25,GetWide()-80,20);
	m_pSendChatButton->SetBounds(GetWide()-70,GetTall()-25,60,20);
}

//-----------------------------------------------------------------------------
// Purpose: inserts a new string into the main chat panel
//-----------------------------------------------------------------------------
void CChatPanel::DoInsertString(const char *str) 
{
	m_pServerChatPanel->DoInsertString(str);
}

//-----------------------------------------------------------------------------
// Purpose: passes the rcon class to use
//-----------------------------------------------------------------------------
void CChatPanel::SetRcon(CRcon *rcon) 
{
	m_pRcon=rcon;
}


//-----------------------------------------------------------------------------
// Purpose: run when the send button is pressed, send a rcon "say" to the server
//-----------------------------------------------------------------------------
void CChatPanel::OnSendChat()
{
	if(m_pRcon)
	{
		char chat_text[512];

		_snprintf(chat_text,512,"say ");
		m_pEnterChatPanel->GetText(0,chat_text+4,512-4);
		if(strlen("say ")!=strlen(chat_text)) // check there is something in the text panel
		{
			m_pRcon->SendRcon(chat_text);

			// the message is sent, zero the text
			m_pEnterChatPanel->SetText("");
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when the game dir combo box is changed
//-----------------------------------------------------------------------------
void CChatPanel::OnTextChanged(Panel *panel, const char *text)
{
// BUG - TextEntry NEVER lets the enter key through... This doesn't work

	if( text[strlen(text)-1]=='\n') // the enter key was just pressed :)
	{
		OnSendChat();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CChatPanel::m_MessageMap[] =
{
	MAP_MESSAGE( CChatPanel, "SendChat", OnSendChat ),
	MAP_MESSAGE( CChatPanel, "TextNewLine", OnSendChat ),

	MAP_MESSAGE( CChatPanel, "PageShow", OnPageShow ),

//	MAP_MESSAGE_PTR_CONSTCHARPTR( CChatPanel, "TextChanged", OnTextChanged, "panel", "text" ),
};

IMPLEMENT_PANELMAP( CChatPanel, Frame );