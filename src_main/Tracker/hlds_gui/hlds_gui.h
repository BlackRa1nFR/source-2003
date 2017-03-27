//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef HLDS_GUI_H
#define HLDS_GUI_H
#ifdef _WIN32
#pragma once
#endif

#include "Server/IServer.h"
#include "IVGuiModule.h"

#include <VGUI_PHandle.h>

class VInternetDlg;

//-----------------------------------------------------------------------------
// Purpose: Handles the UI and pinging of a half-life game server list
//-----------------------------------------------------------------------------
class CHLDS : public IServer, public IVGuiModule
{
public:
	CHLDS();
	~CHLDS();

	// IVGui module implementation
	virtual bool Initialize(CreateInterfaceFn *factorylist, int numFactories);
	virtual bool PostInitialize(CreateInterfaceFn *modules, int factoryCount);
	virtual vgui::VPanel *GetPanel();
	virtual bool Activate();
	virtual bool IsValid();
	virtual void Shutdown();

	virtual void CreateDialog();
	virtual void Open();

private:
	vgui::DHANDLE<VInternetDlg> m_hInternetDlg;

};


#endif // HLDS_GUI_H
