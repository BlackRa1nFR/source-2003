//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "Taskbar.h"
#include "UtlVector.h"

#include <VGUI_Controls.h>
#include <VGUI_IInput.h>
#include <VGUI_IPanel.h>
#include <VGUI_IScheme.h>

#include <VGUI_PHandle.h>
#include <VGUI_ToggleButton.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Button for a specific window
//-----------------------------------------------------------------------------
class CTaskButton : public vgui::ToggleButton
{
public:
	CTaskButton(Panel *parent, Panel *task) : ToggleButton(parent, "TaskButton", "<unknown>") 
	{
		m_hTask = task;
		m_bHasTitle = false;
	}
	~CTaskButton() {}

	virtual void SetTitle(const char *title)
	{
		setText(title);
		if (title && *title)
		{
			m_bHasTitle = true;
		}
	}

	virtual void setSelected(bool state)
	{
		BaseClass::setSelected(state);

		if (state == m_bSelected)
			return;	// state hasn't changed

		if (state)
		{
			// activate the window
			m_hTask->moveToFront();
			m_hTask->requestFocus();
			m_hTask->setVisible(true);
		}
		else
		{
			// deactivate the window
//			m_hTask->setVisible(false);
		}

		m_bSelected = state;
	}

	bool ShouldDisplay()
	{
		return (m_bHasTitle);
	}

	Panel *GetPanel() { return m_hTask.Get(); }

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);
		setContentAlignment(Label::a_west);
		setTextInset(6, 0);
	}

private:
	PHandle m_hTask;	// panel handle to the task
	bool m_bHasTitle;
	bool m_bSelected;

	typedef vgui::ToggleButton BaseClass;
};

// file variables
CUtlVector<CTaskButton *> g_Tasks;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTaskbar::CTaskbar() : EditablePanel(NULL, "Taskbar")
{
	makePopup();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTaskbar::~CTaskbar()
{
}

//-----------------------------------------------------------------------------
// Purpose: update the taskbar a frame
//-----------------------------------------------------------------------------
void CTaskbar::RunFrame()
{
	invalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the title of a task
// Input  : *panel - 
//			*title - 
//-----------------------------------------------------------------------------
void CTaskbar::SetTitle(VPanel *panel, const char *title)
{
	CTaskButton *task = FindTask(panel);
	if (task)
	{
		task->SetTitle(title);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Lays out the position of the taskbar
//-----------------------------------------------------------------------------
void CTaskbar::PerformLayout()
{
	int wide, tall;
	g_GameUISurface.getScreenSize(wide, tall);

	// position self along bottom of screen
	setPos(0, tall - 32);
	setSize(wide, 32);

	// work out current focus - find the topmost panel
	VPanel *vpanel = input()->getFocus();
	Panel *focus = NULL;
	if (vpanel)
	{
		focus = ipanel()->client(vpanel)->getPanel();
	}

/*
	if (focus->hasParent(g_Tasks[i]))
	{
		g_Tasks[i]->setSelected(true);
	}
	else
	{
		g_Tasks[i]->setSelected(false);
	}
*/

	// validate button list
	int validTasks = 0;
	for (int i = 0; i < g_Tasks.Size(); i++)
	{
		if (!g_Tasks[i]->GetPanel())
		{
			// panel has been deleted, remove the task
			g_Tasks[i]->setVisible(false);
			g_Tasks[i]->markForDeletion();

			// remove from list
			g_Tasks.Remove(i);

			// restart validation
			i = -1;
			continue;
		}
		
		if (g_Tasks[i]->ShouldDisplay())
		{
			validTasks++;
		}
	}

	if (g_Tasks.Size() < 1)
		return;

	// calculate button size
	int buttonWide = (getWide() - 4) / validTasks;
	int buttonTall = (getTall() - 4);

	// cap the size
	if (buttonWide > 128)
		buttonWide = 128;

	int x = 2, y = 2;

	// layout buttons
	for (i = 0; i < g_Tasks.Size(); i++)
	{
		if (!g_Tasks[i]->ShouldDisplay())
		{
			g_Tasks[i]->setVisible(false);
			continue;
		}

		g_Tasks[i]->setVisible(true);
		g_Tasks[i]->setBounds(x, y, buttonWide - 4, buttonTall);
		x += buttonWide;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Loads scheme information
//-----------------------------------------------------------------------------
void CTaskbar::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	setBorder(scheme()->getBorder(scheme()->getDefaultScheme(), "ButtonBorder"));
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new task to the bar
// Input  : *panel - 
//-----------------------------------------------------------------------------
void CTaskbar::AddTask(VPanel *panel)
{
	CTaskButton *task = FindTask(panel);
	if (task)
		return;	// task already exists

	if (panel == vpanel())
		return; // don't put the taskbar on the taskbar!

	// create a new task
	task = new CTaskButton(this, ipanel()->client(panel)->getPanel());

	g_Tasks.AddToTail(task);

	invalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Finds a task that handles the specified panel
//-----------------------------------------------------------------------------
CTaskButton *CTaskbar::FindTask(Panel *panel)
{
	// walk the buttons
	for (int i = 0; i < g_Tasks.Size(); i++)
	{
		if (g_Tasks[i]->GetPanel() == panel)
		{
			return g_Tasks[i];
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a task that handles the specified panel
//-----------------------------------------------------------------------------
CTaskButton *CTaskbar::FindTask(VPanel *panel)
{
	Panel *p = ipanel()->client(panel)->getPanel();
	if (p)
	{
		return FindTask(p);
	}
	return NULL;
}





