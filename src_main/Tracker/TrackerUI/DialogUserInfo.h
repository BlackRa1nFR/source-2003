//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DIALOGUSERINFO_H
#define DIALOGUSERINFO_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertyDialog.h>

//-----------------------------------------------------------------------------
// Purpose: Displays information about a user
//-----------------------------------------------------------------------------
class CDialogUserInfo : public vgui::PropertyDialog
{
public:
	CDialogUserInfo(int userID);
	~CDialogUserInfo();

	// activates the dialog
	void Open();

	// overrides
	virtual void PerformLayout();
	virtual void OnClose();

	DECLARE_PANELMAP();

private:
	void SetChildText(const char *panelName, const char *valueName);
	void OnReceivedFriendInfo();

	int m_iUserID;

	typedef vgui::PropertyDialog BaseClass;
};


#endif // DIALOGUSERINFO_H
