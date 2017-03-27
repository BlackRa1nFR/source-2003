//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DIALOGREMOVEUSER_H
#define DIALOGREMOVEUSER_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>

//-----------------------------------------------------------------------------
// Purpose: Warning dialog for removing a user
//-----------------------------------------------------------------------------
class CDialogRemoveUser : public vgui::Frame
{
public:
	CDialogRemoveUser(int userID);
	~CDialogRemoveUser();

	// call when dialog is first opened
	void Open();

	virtual void OnCommand(const char *command);
	virtual void OnClose();

private:
	int m_iUserID;
	typedef vgui::Frame BaseClass;
};


#endif // DIALOGREMOVEUSER_H
