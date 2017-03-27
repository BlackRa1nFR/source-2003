//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELCONNECTIONTEST_H
#define SUBPANELCONNECTIONTEST_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_WizardSubPanel.h>

//-----------------------------------------------------------------------------
// Purpose: Panel for option choosing
//-----------------------------------------------------------------------------
class CSubPanelConnectionTest : public vgui::WizardSubPanel
{
public:
	CSubPanelConnectionTest(vgui::Panel *parent, const char *panelName);
	~CSubPanelConnectionTest();

	virtual vgui::WizardSubPanel *GetNextSubPanel();

	virtual void OnDisplayAsNext();
	virtual void PerformLayout();

private:
	void StartServerSearch();
	void OnPingAck();
	void OnPingTimeout(int pingID);
	void OnCommand(const char *command);

	int m_iCurrentPingID;
	bool m_bServerFound;
	bool m_bConnectFailed;

	DECLARE_PANELMAP();
	typedef vgui::WizardSubPanel BaseClass;
};



#endif // SUBPANELCONNECTIONTEST_H
