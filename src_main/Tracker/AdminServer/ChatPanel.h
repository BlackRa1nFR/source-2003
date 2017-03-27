//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CHATPANEL_H
#define CHATPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>
#include <VGUI_PHandle.h>
#include <VGUI_ListPanel.h>
#include <VGUI_KeyValues.h>
#include <VGUI_PropertyPage.h>

#include "rcon.h"

class KeyValues;

namespace vgui
{
class Button;
class ToggleButton;
class RadioButton;
class Label;
class TextEntry;
class ListPanel;
class MessageBox;
class ComboBox;
class Panel;
class PropertySheet;
};


//-----------------------------------------------------------------------------
// Purpose: Dialog for displaying information about a game server
//-----------------------------------------------------------------------------
class CChatPanel:public vgui::PropertyPage
{
public:
	CChatPanel(vgui::Panel *parent, const char *name);
	~CChatPanel();

	// property page handlers
	virtual void OnPageShow();
	virtual void OnPageHide();
	void DoInsertString(const char *str);

	void SetRcon(CRcon *rcon);

	virtual void PerformLayout();

private:

	void OnSendChat();
	void OnTextChanged(vgui::Panel *panel, const char *text);

	vgui::TextEntry *m_pServerChatPanel;
	vgui::TextEntry *m_pEnterChatPanel;
	vgui::Button *m_pSendChatButton;

	CRcon *m_pRcon;
		
	DECLARE_PANELMAP();
	typedef vgui::PropertyPage BaseClass;
};

#endif // CHATPANEL_H