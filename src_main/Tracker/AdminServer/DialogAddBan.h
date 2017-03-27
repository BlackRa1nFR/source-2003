//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DIALOGADDBAN_H
#define DIALOGADDBAN_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>


namespace vgui
{
class TextEntry;
class Label;
class Button;
};

//-----------------------------------------------------------------------------
// Purpose: Prompt for user to enter a password to be able to connect to the server
//-----------------------------------------------------------------------------
class CDialogAddBan : public vgui::Frame
{
public:
	CDialogAddBan();
	~CDialogAddBan();

	// initializes the dialog and brings it to the foreground
	void Activate(const char *type);

	/* message returned:
		"AddBanValue" 
			"id"
			"time"
			"type"
			"ipcheck"
	*/
	
	// make the input stars, ala a password entry dialog
	void MakePassword();
	
	// set the text in a certain label name
	void SetLabelText(const char *textEntryName, const char *text);

	// returns if the IPCheck check button is checked
	bool IsIPCheck();

private:
	virtual void PerformLayout();
	virtual void OnCommand(const char *command);
	virtual void OnClose();

	vgui::TextEntry *m_pTimeTextEntry;
	vgui::TextEntry *m_pIDTextEntry;
	vgui::Button *m_pOkayButton;

	typedef vgui::Frame BaseClass;
	const char *m_cType;

};


#endif // DIALOGADDBAN_H
