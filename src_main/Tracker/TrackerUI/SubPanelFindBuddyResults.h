//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELFINDBUDDYRESULTS_H
#define SUBPANELFINDBUDDYRESULTS_H
#pragma once

#include <VGUI_WizardSubPanel.h>

namespace vgui
{
class ListPanel;
class Label;
};

//-----------------------------------------------------------------------------
// Purpose: Displays results from a buddy search
//-----------------------------------------------------------------------------
class CSubPanelFindBuddyResults : public vgui::WizardSubPanel
{
public:
	CSubPanelFindBuddyResults(vgui::Panel *parent, const char *panelName);
	~CSubPanelFindBuddyResults();

	virtual vgui::WizardSubPanel *GetNextSubPanel();
	virtual void OnDisplayAsNext();
	virtual void OnDisplay();
	virtual void PerformLayout();
	virtual bool OnNextButton();

	DECLARE_PANELMAP();
	
private:
	virtual void OnFriendFound(vgui::KeyValues *friendData);
	virtual void OnNoFriends(int attemptID);
	
	virtual void OnRowSelected(int startIndex, int endIndex);

	vgui::ListPanel *m_pTable;
	vgui::Label *m_pInfoText;
	int m_iFound;
	int m_iAttemptID;
};


#endif // SUBPANELFINDBUDDYRESULTS_H
