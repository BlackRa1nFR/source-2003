//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BUYSUBMENU_H
#define BUYSUBMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/WizardSubPanel.h>
#include <vgui_controls/Button.h>
#include <UtlVector.h>

class BuyMouseOverPanelButton;
class CBuyMenu;

//-----------------------------------------------------------------------------
// Purpose: Draws the class menu
//-----------------------------------------------------------------------------
class CBuySubMenu : public vgui::WizardSubPanel
{
private:
	typedef vgui::WizardSubPanel BaseClass;

public:
	CBuySubMenu(vgui::Panel *parent,const char *name = "BuySubMenu");
	~CBuySubMenu();

	virtual void SetVisible( bool state );
	virtual void DeleteSubPanels();
	
protected:
	CUtlVector<BuyMouseOverPanelButton *> m_pItemButtons;
	vgui::Panel *m_pPanel;
	BuyMouseOverPanelButton *m_pFirstButton;


private:
	virtual vgui::WizardSubPanel *GetNextSubPanel(); // this is the last menu in the list
	virtual vgui::Panel *CreateControlByName(const char *controlName);

	class ClassHelperPanel : public vgui::EditablePanel
	{
	private:
		typedef vgui::EditablePanel BaseClass;

	public:
		ClassHelperPanel( vgui::Panel *parent, const char *panelName ) : vgui::EditablePanel( parent, panelName) {  m_pAssociatedButton = NULL; }

		virtual void SetAssociatedButton( BuyMouseOverPanelButton *button ) { m_pAssociatedButton = button; }
		virtual BuyMouseOverPanelButton *GetAssociatedButton( void ) { return m_pAssociatedButton; }

	private:
		BuyMouseOverPanelButton *m_pAssociatedButton; // used to load class .res and .tga files
		
		virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
		const char *GetClassPage( const char *className );
	};

	// command callbacks
	void OnCommand( const char *command );


	typedef struct
	{
		char filename[_MAX_PATH];
		CBuySubMenu *panel;
	} SubMenuEntry_t;

	CUtlVector<SubMenuEntry_t> m_SubMenus; // a cache of buy submenus, so we don't need to construct them each time

	vgui::WizardSubPanel *_nextPanel;
};

#endif // BUYSUBMENU_H
