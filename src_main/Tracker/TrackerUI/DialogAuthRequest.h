//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DIALOGAUTHREQUEST_H
#define DIALOGAUTHREQUEST_H
#pragma once


#include <VGUI_Frame.h>

namespace vgui
{
class Label;
class Button;
class RadioButton;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDialogAuthRequest : public vgui::Frame
{
public:
	CDialogAuthRequest();
	~CDialogAuthRequest();

	// runs a read message
	virtual void ReadAuthRequest(int buddyID);

	virtual void AcceptAuth();
	virtual void DenyAuth();

protected:
	// overrides
	virtual void OnClose();	// this dialog deletes itself when closed
	virtual void OnCommand(const char *command);

	vgui::Label *m_pInfoText;
	vgui::Label *m_pNameLabel;

	vgui::RadioButton *m_pAcceptButton;
	vgui::RadioButton *m_pDeclineButton;

	vgui::Button *m_pOKButton;
	vgui::Button *m_pCancelButton;

	int m_iTargetID;

	typedef vgui::Frame BaseClass;
};


#endif // DIALOGAUTHREQUEST_H
