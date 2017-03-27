//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELBUDDYLIST_H
#define SUBPANELBUDDYLIST_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Panel.h>
#include <VGUI_PHandle.h>

class CBuddyButton;
class CBuddySectionHeader;
class CDialogChat;

namespace vgui
{
class Label;
class ScrollBar;
};

//-----------------------------------------------------------------------------
// Purpose: Displays the list of buddies
//-----------------------------------------------------------------------------
class CSubPanelBuddyList : public vgui::Panel
{
public:
	CSubPanelBuddyList(vgui::Panel *parent, const char *panelName);
	~CSubPanelBuddyList();

	// sets the buddy list to be only users who are in the chat dialog
	virtual void AssociateWithChatDialog(CDialogChat *chatDialog);

	// clears the buddy list
	void Clear();

	// calls InvalidateLayout() on all the buddy buttons
	void InvalidateAll();

	// vgui overrides
	virtual void PerformLayout();
	virtual void SetSize(int wide, int tall);
	virtual void OnMouseWheeled(int delta);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	void OnSliderMoved();

	typedef vgui::Dar<CBuddyButton*> buddylist_t;
	typedef vgui::Dar<CBuddyButton*> panellist_t;

	buddylist_t m_BuddyButtonDar;

	CBuddySectionHeader *m_pSystemLabel;
	CBuddySectionHeader *m_pIngameLabel;
	CBuddySectionHeader *m_pCurrentGameLabel;
	CBuddySectionHeader *m_pUnknownLabel;
	CBuddySectionHeader *m_pAwaitingAuthLabel;
	CBuddySectionHeader *m_pRequestingAuthLabel;
	CBuddySectionHeader *m_pOnlineLabel;
	CBuddySectionHeader *m_pOfflineLabel;

	vgui::ScrollBar *m_pScrollBar;

	vgui::DHANDLE<CDialogChat> m_hChatDialog;

	int m_iItemHeight;

	// button drawing
	void LayoutButtons(int &x, int &y, int wide, int tall, panellist_t &panels, vgui::Label *label);

	DECLARE_PANELMAP();

	typedef vgui::Panel BaseClass;
};



#endif // SUBPANELBUDDYLIST_H
