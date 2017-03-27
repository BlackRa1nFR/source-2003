//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELLOGINUSER2_H
#define SUBPANELLOGINUSER2_H
#pragma once

#include <VGUI_WizardSubPanel.h>

namespace vgui
{
class Label;
class ProgressBar;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSubPanelLoginUser2 : public vgui::WizardSubPanel
{
public:
	CSubPanelLoginUser2(vgui::Panel *parent, const char *panelName);
	~CSubPanelLoginUser2();

	virtual void PerformLayout();
	virtual vgui::WizardSubPanel *GetNextSubPanel();

	virtual bool OnFinishButton();
	virtual void OnDisplayAsNext();

	// called on timeout
	virtual void OnTimeout();

	// network message handler
	virtual void OnUserValid(vgui::KeyValues *params);

	DECLARE_PANELMAP();

	// per-frame tick handler
	virtual void OnTick();

private:
	vgui::Label *m_pLabel;
	vgui::ProgressBar *m_pProgressBar;
	vgui::Label *m_pProgressLabel;

	bool m_bGotReply;
	int m_iTimeoutTime;
};




#endif // SUBPANELLOGINUSER2_H
