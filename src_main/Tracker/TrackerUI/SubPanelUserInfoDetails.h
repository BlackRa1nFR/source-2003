//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SUBPANELUSERINFODETAILS_H
#define SUBPANELUSERINFODETAILS_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_PropertyPage.h>

//-----------------------------------------------------------------------------
// Purpose: Details information about the user (name, email, etc)
//-----------------------------------------------------------------------------
class CSubPanelUserInfoDetails : public vgui::PropertyPage
{
public:
	CSubPanelUserInfoDetails(int userID);
	~CSubPanelUserInfoDetails();

	virtual void PerformLayout();
	virtual void OnCommand(const char *command);

	// Called when page is loaded.  Data should be reloaded from document into controls.
	virtual void OnResetData();

	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	virtual void OnApplyChanges();

private:
	int m_iUserID;
	typedef vgui::PropertyPage BaseClass;
};


#endif // SUBPANELUSERINFODETAILS_H
