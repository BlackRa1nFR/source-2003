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
#include <VGUI_QueryBox.h>



#include "vinternetdlg.h"
#include "hlds.h"

using namespace vgui;

static VInternetDlg *s_InternetDlg = NULL;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
VInternetDlg::VInternetDlg( void ) : Frame(NULL, "VInternetDlg")
{
	MakePopup();
	s_InternetDlg=this;

//	start=false;

	SetMinimumSize(480, 400);
	SetSize(480, 400);

	m_pDetailsSheet = new PropertySheet(this, "ServerTabs");
	m_pGameServer = new CGameServer(this, "Server");
	m_pConfigPage = new CCreateMultiplayerGameServerPage(this,"Config");

	LoadControlSettings("Server/dlg.res");

	m_pDetailsSheet->AddPage(m_pConfigPage,"Config");
	m_pDetailsSheet->AddPage(m_pGameServer,"Server");
	m_pDetailsSheet->DisablePage("Server");


}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
VInternetDlg::~VInternetDlg()
{
	m_pGameServer->Stop();
}

//-----------------------------------------------------------------------------
// Purpose: Called once to set up
//-----------------------------------------------------------------------------
void VInternetDlg::Initialize()
{
	SetTitle("Servers", true);
	SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VInternetDlg::Open( void )
{	
//	m_pTabPanel->RequestFocus();
	
	surface()->SetMinimized(GetVPanel(), false);
	SetVisible(true);
	RequestFocus();
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: relayouts the dialogs controls
//-----------------------------------------------------------------------------
void VInternetDlg::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	
	// game list in middle
	m_pDetailsSheet->SetBounds(8, y + 8, GetWide() - 16, tall - (28));
	m_pConfigPage->SetBounds(2,2,wide-30,tall-50);

	// status text along bottom
	Repaint();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VInternetDlg::OnClose()
{
	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a static instance of this dialog
// Output : VInternetDlg
//-----------------------------------------------------------------------------
VInternetDlg *VInternetDlg::GetInstance()
{
	return s_InternetDlg;
}

void VInternetDlg::ConsoleText(const char *text)
{
	m_pGameServer->ConsoleText(text);
}

void VInternetDlg::UpdateStatus(float fps,const char *map,int maxplayers,int numplayers)
{
	m_pGameServer->UpdateStatus(fps,map,maxplayers,numplayers);
}

void VInternetDlg::StartServer(const char *cmdline,const char *cvars)
{
	m_pDetailsSheet->EnablePage("Server");
	m_pDetailsSheet->DisablePage("Config");
	m_pDetailsSheet->SetActivePage(m_pGameServer);

	m_pGameServer->Start(cmdline,cvars);
}
void VInternetDlg::StopServer()
{
	m_pDetailsSheet->EnablePage("Config");
	m_pDetailsSheet->DisablePage("Server");
	m_pDetailsSheet->SetActivePage(m_pConfigPage);
}