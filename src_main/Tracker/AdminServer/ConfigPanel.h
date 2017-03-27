//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CONFIGPANEL_H
#define CONFIGPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>

class KeyValues;

namespace vgui
{
class Button;
class ToggleButton;
class RadioButton;
class Label;
class TextEntry;
class CheckButton;
}

//-----------------------------------------------------------------------------
// Purpose: Dialog for displaying information about a game server
//-----------------------------------------------------------------------------
class CConfigPanel : public vgui::Frame
{
public:
	CConfigPanel(bool autorefresh,bool savercon,int refreshtime,bool graphs, int graphsrefreshtime,bool getlogs);
	~CConfigPanel();

	void Run();
	
protected:
	// message handlers
	void OnButtonToggled(vgui::Panel *toggleButton);


	// vgui overrides
	virtual void OnClose();
	virtual void OnCommand(const char *command);

private:

	void SetControlText(const char *textEntryName, const char *text);

	vgui::Button *m_pOkayButton;
	vgui::Button *m_pCloseButton;
	vgui::CheckButton *m_pRconCheckButton;
	vgui::CheckButton *m_pRefreshCheckButton;
	vgui::TextEntry *m_pRefreshTextEntry;
	vgui::CheckButton *m_pGraphsButton;
	vgui::TextEntry *m_pGraphsRefreshTimeTextEntry;
	vgui::CheckButton *m_pLogsButton;


	DECLARE_PANELMAP();

	typedef vgui::Frame BaseClass;
};

#endif // CONFIGPANEL_H
