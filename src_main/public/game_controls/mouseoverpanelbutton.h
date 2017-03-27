//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MOUSEOVERPANELBUTTON_H
#define MOUSEOVERPANELBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>

extern vgui::Panel *g_lastPanel;

//-----------------------------------------------------------------------------
// Purpose: Triggers a new panel when the mouse goes over the button
//-----------------------------------------------------------------------------
class MouseOverPanelButton : public vgui::Button
{
public:
	MouseOverPanelButton(vgui::Panel *parent, const char *panelName, vgui::Panel *panel ) :
					Button( parent, panelName, "MouseOverPanelButton")
	{
		m_pPanel = panel;
		m_iClass = 0;
	}

	void SetClass(int pClass, int index) { m_iClass = pClass;}
	int GetClass() { return m_iClass; }
	
	virtual void SetPanel( vgui::Panel *panel) 
	{
		m_pPanel = panel;
	}

	void ShowPage()
	{
		if( m_pPanel )
		{
			m_pPanel->SetVisible( true );
			m_pPanel->MoveToFront();
			g_lastPanel = m_pPanel;
		}
	}

private:

	virtual void OnCursorEntered() 
	{
		Button::OnCursorEntered();
		if ( m_pPanel)
		{
			if ( g_lastPanel )
			{
				g_lastPanel->SetVisible( false );
			}
			ShowPage();
		}
	}

	vgui::Panel *m_pPanel;
	int m_iClass;
};


#endif // MOUSEOVERPANELBUTTON_H
