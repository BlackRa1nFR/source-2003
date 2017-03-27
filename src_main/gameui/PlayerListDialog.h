//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PLAYERLISTDIALOG_H
#define PLAYERLISTDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>

//-----------------------------------------------------------------------------
// Purpose: List of players, their ingame-name and their tracker-name
//-----------------------------------------------------------------------------
class CPlayerListDialog : public vgui::Frame
{
public:
	CPlayerListDialog(vgui::Panel *parent);
	~CPlayerListDialog();

	virtual void Activate();

private:
  	virtual void OnItemSelected();
	virtual void OnCommand(const char *command);

	void RequestAuthorizationFromSelectedUser();
	void ToggleMuteStateOfSelectedUser();
	void RefreshPlayerProperties();

	DECLARE_PANELMAP();

	vgui::ListPanel *m_pPlayerList;
	vgui::Button *m_pAddFriendButton;
	vgui::Button *m_pMuteButton;

	typedef vgui::Frame BaseClass;
};


#endif // PLAYERLISTDIALOG_H
