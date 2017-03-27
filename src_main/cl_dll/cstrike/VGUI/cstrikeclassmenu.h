//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CSCLASSMENU_H
#define CSCLASSMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <game_controls/ClassMenu.h>
#include <vgui_controls/EditablePanel.h>
#include <FileSystem.h>

//-----------------------------------------------------------------------------
// Purpose: Draws the class menu
//-----------------------------------------------------------------------------
class CCSClassMenu : public CClassMenu
{
private:
	typedef CClassMenu BaseClass;

public:
	CCSClassMenu(vgui::Panel *parent);
	~CCSClassMenu();
	// override the base class
	virtual void Update( int *validClasses, int team ); // we override the defn of the 2nd param

	// VGUI2 overrides
	vgui::Panel *CreateControlByName(const char *controlName);

private:
	// helper functions

	class ClassHelperPanel : public vgui::EditablePanel
	{
	private:
		typedef vgui::EditablePanel BaseClass;

	public:
		ClassHelperPanel( vgui::Panel *parent, const char *panelName ) : vgui::EditablePanel( parent, panelName ) { m_pAssociatedButton = NULL; }
		
		virtual void SetAssociatedButton( vgui::Panel *button ) { m_pAssociatedButton = button; }
		virtual vgui::Panel *GetAssociatedButton( void ) { return m_pAssociatedButton; }

	private:
		vgui::Panel *m_pAssociatedButton; // used to load class .res and .tga files

		virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
		const char *GetClassPage( const char *className );
	};

	int m_iFirst;
	void SetVisibleButton(const char *textEntryName, bool state);
};
#endif // CSCLASSMENU_H
