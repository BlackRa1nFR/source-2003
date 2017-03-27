//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "..\common\winlite.h"
// base vgui interfaces
#include <VGUI_Controls.h>
#include <VGUI_IInput.h>
#include <VGUI_ISurface.h>
#include <VGUI_IScheme.h>
#include <VGUI_IVGui.h>
#include <VGUI_MouseCode.h>
#include "FileSystem.h"

// vgui controls
#include <VGUI_Button.h>
#include <VGUI_CheckButton.h>
#include <VGUI_ComboBox.h>
#include <VGUI_FocusNavGroup.h>
#include <VGUI_Frame.h>
#include <VGUI_KeyValues.h>
#include <VGUI_ListPanel.h>
#include <VGUI_MessageBox.h>
#include <VGUI_Panel.h>
#include <VGUI_TextEntry.h>
#include <VGUI_PropertySheet.h>
#include <VGUI_PropertyPage.h>
#include <VGUI_QueryBox.h>



#include "GameServer.h"
#include "hlds.h"

using namespace vgui;

extern CHLDSServer server;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameServer::CGameServer( vgui::Panel *parent, const char *name) : PropertyPage(parent, name)
{
	server.SetInstance(VInternetDlg::GetInstance());

	start=false;

	m_ConsoleTextEntry = new TextEntry(this,"console");
	m_CommandTextEntry = new TextEntry(this,"commandentry");
	m_SubmitButton = new Button(this,"submit","&Submit");
	m_QuitButton = new Button(this,"quit","&Quit");

	LoadControlSettings("Server/hlds.res");

	m_ConsoleTextEntry->SetMultiline(true);
	m_ConsoleTextEntry->SetEnabled(true);
	m_ConsoleTextEntry->SetVerticalScrollbar(true);
	m_ConsoleTextEntry->SetRichEdit(true);
	m_ConsoleTextEntry->setMaximumCharCount(8000);
	m_ConsoleTextEntry->SetWrap(true);
	m_ConsoleTextEntry->SetEditable(false);

	m_CommandTextEntry->SetMultiline(false);
	m_CommandTextEntry->SendNewLine(true);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameServer::~CGameServer()
{
	Stop();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
void CGameServer::Stop()
{
	if(server.GetEngineAPI())
	{
		server.Stop();
		Sleep( 200 );
		server.SetInstance(NULL);
		server.SetEngineAPI(NULL);
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameServer::Open( void )
{	
	surface()->SetMinimized(GetVPanel(), false);
	SetVisible(true);
	RequestFocus();
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: relayouts the dialogs controls
//-----------------------------------------------------------------------------
void CGameServer::PerformLayout()
{
	BaseClass::PerformLayout();

	int x,y,w,h;
	GetBounds(x,y,w,h);
	
	m_ConsoleTextEntry->SetBounds(2,32,w-5,h-64);
	m_CommandTextEntry->SetBounds(2,y+h-57,w-115,24);
	m_SubmitButton->SetBounds(w+x-110,y+h-57,60,24);
	m_QuitButton->SetBounds(w+x-45,y+h-57,40,24);
}



void CGameServer::ConsoleText(const char *text)
{
	static int i=10;
	m_ConsoleTextEntry->DoInsertColorChange(Color(255-i,i,(i+125)%255));
	i+=10;
	if(i>255) i=0;

	m_ConsoleTextEntry->DoInsertString(text);
}

void CGameServer::OnCommand(const char *command)
{
	if(!stricmp(command,"submit"))
	{
		char command[512];
		m_CommandTextEntry->GetText(0,command,512);
		if(strlen(command)>0)
		{

			strcat(command,"\n");
			if(server.GetEngineAPI())
			{
				server.GetEngineAPI()->AddConsoleText(command);
			}
			m_CommandTextEntry->SetText("");
		}
	} 
	else if (!stricmp(command,"quit"))
	{
		Stop();
		VInternetDlg::GetInstance()->StopServer();
	}
}

void CGameServer::OnSendCommand()
{
	OnCommand("submit");
}

void CGameServer::Start(const char *cmdline,const char *cvars)
{
	server.Start(cmdline,cvars);
	start=true;
}

void CGameServer::UpdateStatus(float fps,const char *map,int maxplayers,int numplayers)
{
	SetLabelText("MapLabel",map);

	char fpstxt[20];
	_snprintf(fpstxt,20,"%5.2f",fps);
	SetLabelText("FpsLabel",fpstxt);

	_snprintf(fpstxt,20,"%i/%i",numplayers,maxplayers);
	SetLabelText("PlayersLabel",fpstxt);

}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CGameServer::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CGameServer::m_MessageMap[] =
{
	MAP_MESSAGE( CGameServer, "TextNewLine", OnSendCommand ),

};
IMPLEMENT_PANELMAP(CGameServer, vgui::Frame);
