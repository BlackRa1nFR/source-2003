//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "MainDialog.h"
#include "ServersPage.h"
#include "UsersPage.h"

#include <VGUI_Controls.h>
#include <VGUI_ISurface.h>
#include <VGUI_IVGui.h>

#include <VGUI_PropertySheet.h>

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMainDialog::CMainDialog(vgui::Panel *parent) : Frame(parent, "ServerMonitor")
{
	// make us a popup
	MakePopup();

	// create the controls
	SetTitle("Tracker - Server Monitor", true);

	m_pSheet = new PropertySheet(this, "ServerSheet");
	m_pSheet->AddPage(new CServersPage(), "Servers");
	m_pSheet->AddPage(new CUsersPage(), "Status");
	
	int x, y, wide, tall;
	surface()->GetWorkspaceBounds(x, y, wide, tall);

	SetBounds(x + 100, y + 100, 640, tall - 200);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMainDialog::~CMainDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: forces everything to redraw
//-----------------------------------------------------------------------------
void CMainDialog::OnRefreshServers()
{
	// make all our children relayout
	for (int i = 0; i < GetChildCount(); i++)
	{
		Panel *child = GetChild(i);
		child->InvalidateLayout();
	}

	Repaint();
}


//-----------------------------------------------------------------------------
// Purpose: lays out the controls in the dialog
//-----------------------------------------------------------------------------
void CMainDialog::PerformLayout()
{
	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);

	m_pSheet->SetBounds(x, y, wide, tall);
	
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Handles closing of the dialog - shuts down the whole app
//-----------------------------------------------------------------------------
void CMainDialog::OnClose()
{
	BaseClass::OnClose();

	// Stop vgui running
	vgui::ivgui()->Stop();
}



//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CMainDialog::m_MessageMap[] =
{
	MAP_MESSAGE( CMainDialog, "RefreshServers", OnRefreshServers ),
};
IMPLEMENT_PANELMAP(CMainDialog, BaseClass);