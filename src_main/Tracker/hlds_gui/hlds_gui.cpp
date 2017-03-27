//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "vinternetdlg.h"
#include "hlds_gui.h"


#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_IPanel.h>
#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>


CHLDS g_ServerSingleton;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CHLDS, IVGuiModule, "VGuiModuleHLDS001", g_ServerSingleton);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHLDS::CHLDS()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHLDS::~CHLDS()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLDS::CreateDialog()
{
	if (!m_hInternetDlg.Get())
	{
		m_hInternetDlg = new VInternetDlg;
		m_hInternetDlg->Initialize();
	}
}

//-----------------------------------------------------------------------------
// Purpose: links to vgui and engine interfaces
//-----------------------------------------------------------------------------
bool CHLDS::Initialize(CreateInterfaceFn *factorylist, int factoryCount)
{
	// load the vgui interfaces
	return vgui::VGui_InitInterfacesList(factorylist, factoryCount);
}

//-----------------------------------------------------------------------------
// Purpose: links to other modules interfaces (tracker)
//-----------------------------------------------------------------------------
bool CHLDS::PostInitialize(CreateInterfaceFn *modules, int factoryCount)
{
	CreateDialog();
	m_hInternetDlg->SetVisible(false);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHLDS::IsValid()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHLDS::Activate()
{
	Open();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLDS::Open()
{
	m_hInternetDlg->Open();
}

//-----------------------------------------------------------------------------
// Purpose: returns direct handle to main server browser dialog
//-----------------------------------------------------------------------------
vgui::VPanel *CHLDS::GetPanel()
{
	return m_hInternetDlg.Get() ? m_hInternetDlg->GetVPanel() : NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Closes down the server browser for good
//-----------------------------------------------------------------------------
void CHLDS::Shutdown()
{
	if (m_hInternetDlg.Get())
	{
		vgui::ivgui()->PostMessage(m_hInternetDlg->GetVPanel(), new vgui::KeyValues("Close"), NULL);
		m_hInternetDlg->MarkForDeletion();
	}
}

