//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELTRACKERSTATE_H
#define SUBPANELTRACKERSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_EditablePanel.h>

namespace vgui
{
class TextEntry;
class Button;
};

//-----------------------------------------------------------------------------
// Purpose: Displays the current state of tracker when not connected
//-----------------------------------------------------------------------------
class CSubPanelTrackerState : public vgui::EditablePanel
{
public:
	CSubPanelTrackerState(vgui::Panel *parent, const char *panelName);
	~CSubPanelTrackerState();

private:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PerformLayout();
	virtual void OnCommand(const char *command);

	vgui::TextEntry *m_pMessage;
	vgui::Button *m_pSignInButton;

	typedef vgui::EditablePanel BaseClass;
};


#endif // SUBPANELTRACKERSTATE_H
