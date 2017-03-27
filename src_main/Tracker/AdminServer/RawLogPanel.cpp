//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>

#include "RawLogPanel.h"


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
CRawLogPanel::CRawLogPanel(vgui::Panel *parent, const char *name) : PropertyPage(parent, name)
{
	SetSize(200,100);
	m_pServerRawLogPanel = new TextEntry(this, "ServerChatText");

	m_pServerRawLogPanel->SetMultiline(true);
	m_pServerRawLogPanel->SetEnabled(true);
	m_pServerRawLogPanel->SetEditable(false);
	m_pServerRawLogPanel->SetVerticalScrollbar(true);
	m_pServerRawLogPanel->SetRichEdit(false);
	m_pServerRawLogPanel->setMaximumCharCount(8000); // 100 x 80 char wide lines...	
	m_pServerRawLogPanel->SetWrap(true);


	m_pEnterRconPanel = new TextEntry(this,"RconMessage");
//	m_pEnterRconPanel->SendNewLine(true);

	m_pSendRconButton = new Button(this, "SendRcon", "&Send");
	m_pSendRconButton->SetCommand(new KeyValues("SendRcon"));
	m_pSendRconButton->SetAsDefaultButton(true);
	
	m_pServerRawLogPanel->SetBounds(5,5,GetWide()-5,GetTall()-35);

	m_pEnterRconPanel->SetBounds(5,GetTall()-25,GetWide()-80,20);
	m_pSendRconButton->SetBounds(GetWide()-70,GetTall()-25,60,20);

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CRawLogPanel::~CRawLogPanel()
{

}

//-----------------------------------------------------------------------------
// Purpose: Activates the page
//-----------------------------------------------------------------------------
void CRawLogPanel::OnPageShow()
{
	m_pEnterRconPanel->RequestFocus();

}

//-----------------------------------------------------------------------------
// Purpose: Hides the page
//-----------------------------------------------------------------------------
void CRawLogPanel::OnPageHide()
{
}



//-----------------------------------------------------------------------------
// Purpose: Relayouts the data
//-----------------------------------------------------------------------------
void CRawLogPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	// setup the layout of the panels
	m_pServerRawLogPanel->SetBounds(5,5,GetWide()-10,GetTall()-35);

	m_pEnterRconPanel->SetBounds(5,GetTall()-25,GetWide()-80,20);
	m_pSendRconButton->SetBounds(GetWide()-70,GetTall()-25,60,20);
}

//-----------------------------------------------------------------------------
// Purpose: inserts a new string into the main chat panel
//-----------------------------------------------------------------------------
void CRawLogPanel::DoInsertString(const char *str) 
{
	m_pServerRawLogPanel->DoInsertString(str);
}

//-----------------------------------------------------------------------------
// Purpose: passes the rcon class to use
//-----------------------------------------------------------------------------
void CRawLogPanel::SetRcon(CRcon *rcon) 
{
	m_pRcon=rcon;
}

//-----------------------------------------------------------------------------
// Purpose: run when the send button is pressed, send a rcon "say" to the server
//-----------------------------------------------------------------------------
void CRawLogPanel::OnSendRcon()
{
	if(m_pRcon)
	{
		char chat_text[512];

		m_pEnterRconPanel->GetText(0,chat_text,512);

		if(strlen(chat_text)>1) // check there is something in the text panel
		{
			m_pRcon->SendRcon(chat_text);

			// the message is sent, zero the text
			m_pEnterRconPanel->SetText("");
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CRawLogPanel::m_MessageMap[] =
{
	MAP_MESSAGE( CRawLogPanel, "SendRcon", OnSendRcon ),
	MAP_MESSAGE( CRawLogPanel, "TextNewLine", OnSendRcon ),
	MAP_MESSAGE( CRawLogPanel, "PageShow", OnPageShow ),

};

IMPLEMENT_PANELMAP( CRawLogPanel, Frame );