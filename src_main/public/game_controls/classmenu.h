//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CLASSMENU_H
#define CLASSMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include <UtlVector.h>
#include <vgui/ILocalize.h>
#include <vgui/KeyCode.h>

#include "mouseoverpanelbutton.h"

namespace vgui
{
	class TextEntry;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the class menu
//-----------------------------------------------------------------------------
class CClassMenu : public vgui::Frame
{
private:
	typedef vgui::Frame BaseClass;

public:
	CClassMenu(vgui::Panel *parent);
	virtual ~CClassMenu();

	virtual void Activate( );
	virtual void Update( int *validClasses, int numClasses );

protected:
	MouseOverPanelButton *m_pFirstButton;
	vgui::Panel *m_pPanel;
	vgui::Panel *CreateControlByName(const char *controlName);

	//vgui2 overrides
	virtual void OnKeyCodePressed(vgui::KeyCode code);

private:
	// helper functions
	void SetLabelText(const char *textEntryName, const char *text);


	class ClassHelperPanel : public vgui::EditablePanel
	{
	private:
		typedef vgui::EditablePanel BaseClass;

	public:
		ClassHelperPanel(vgui::Panel *parent, const char *panelName ) : vgui::EditablePanel( parent, panelName) { m_pButton = NULL;}
		
		void SetButton( MouseOverPanelButton *button ) { m_pButton = button; } 

	private:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		const char *GetClassPage( const char *className );

		MouseOverPanelButton *m_pButton;
	};

	// command callbacks
	void OnCommand( const char *command );

	int m_iScoreBoardKey;
	int m_iFirst;

	CUtlVector<MouseOverPanelButton *> m_pClassButtons;

};


#endif // CLASSMENU_H
