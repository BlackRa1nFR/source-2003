//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TASKBAR_H
#define TASKBAR_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_EditablePanel.h>

class CTaskButton;

//-----------------------------------------------------------------------------
// Purpose: Win95-type taskbar for in the game
//-----------------------------------------------------------------------------
class CTaskbar : public vgui::EditablePanel
{
public:
	CTaskbar();
	~CTaskbar();

	// update the taskbar a frame
	void RunFrame();

	// sets the title of a task
	void SetTitle(vgui::VPanel *panel, const char *title);

	// adds a new task to the list
	void AddTask(vgui::VPanel *panel);

private:
	CTaskButton *FindTask(vgui::VPanel *panel);
	CTaskButton *FindTask(vgui::Panel *panel);

	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	typedef vgui::Panel BaseClass;
};

extern CTaskbar *g_Taskbar;


#endif // TASKBAR_H
