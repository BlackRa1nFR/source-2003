//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELFINDBUDDYCOMPLETE_H
#define SUBPANELFINDBUDDYCOMPLETE_H
#pragma once

#include <VGUI_WizardSubPanel.h>

namespace vgui
{
class Label;
};

//-----------------------------------------------------------------------------
// Purpose: Finalizes adding buddy to list
//-----------------------------------------------------------------------------
class CSubPanelFindBuddyComplete : public vgui::WizardSubPanel
{
public:
	CSubPanelFindBuddyComplete(vgui::Panel *parent, const char *panelName);
	~CSubPanelFindBuddyComplete();

	virtual vgui::WizardSubPanel *GetNextSubPanel();
	virtual void OnDisplayAsNext();

private:
	vgui::Label *m_pInfoText;

};



#endif // SUBPANELFINDBUDDYCOMPLETE_H
