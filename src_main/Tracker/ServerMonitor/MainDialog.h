//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAINDIALOG_H
#define MAINDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>

namespace vgui
{
class PropertySheet;
};

//-----------------------------------------------------------------------------
// Purpose: Main dialog for server monitor
//-----------------------------------------------------------------------------
class CMainDialog : public vgui::Frame
{
public:
	CMainDialog(vgui::Panel *parent);
	~CMainDialog();

	// vgui overrides
	virtual void PerformLayout();
	virtual void OnClose();

private:
	virtual void OnRefreshServers();

	DECLARE_PANELMAP();

	vgui::PropertySheet *m_pSheet;

	typedef vgui::Frame BaseClass;
};


#endif // MAINDIALOG_H
