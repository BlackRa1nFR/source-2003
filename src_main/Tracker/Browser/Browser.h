//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BROWSER_H
#define BROWSER_H
#ifdef _WIN32
#pragma once
#endif

#include <IVGuiModule.h>

class VInternetDlg;

//-----------------------------------------------------------------------------
// Purpose: Wrapper for the Browser dialog
//-----------------------------------------------------------------------------
class CBrowser : public IVGuiModule
{
public:
	CBrowser();
	~CBrowser();

	// IVGui module implementation
	virtual bool Initialize(CreateInterfaceFn *factorylist, int numFactories);
	virtual bool PostInitialize(CreateInterfaceFn *modules, int factoryCount);
	virtual vgui::VPanel *GetPanel();
	virtual bool Activate();
	virtual bool IsValid();
	virtual void Shutdown();
	virtual void Deactivate();
	virtual void Reactivate();

	virtual void CreateDialog();
	virtual void Open();

private:
	vgui::DHANDLE<VInternetDlg> m_hInternetDlg;

};


#endif // BROWSER_H
