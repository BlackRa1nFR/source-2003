//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SERVERINFOPANEL_H
#define SERVERINFOPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_EditablePanel.h>

namespace vgui
{
class TextEntry;
};

//-----------------------------------------------------------------------------
// Purpose: Displays information about a single server
//-----------------------------------------------------------------------------
class CServerInfoPanel : public vgui::EditablePanel
{
public:
	CServerInfoPanel(vgui::Panel *parent, const char *name);
	~CServerInfoPanel();

	virtual void SetServerID(int serverID);

	virtual void PerformLayout();

private:
	typedef vgui::EditablePanel BaseClass;

	int m_iServerID;
	vgui::TextEntry *m_pText;
};


#endif // SERVERINFOPANEL_H
