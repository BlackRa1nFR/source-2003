//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef COMBOBOX_H
#define COMBOBOX_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Menu.h>

namespace vgui
{

class ComboBoxButton;

//-----------------------------------------------------------------------------
// Purpose: Text entry with drop down options list
//-----------------------------------------------------------------------------
class ComboBox : public TextEntry
{
public:
	ComboBox(Panel *parent, const char *panelName, int numLines, bool allowEdit);
	~ComboBox();

	// functions designed to be overriden
	virtual void OnShowMenu(Menu *menu) {}
	virtual void OnHideMenu(Menu *menu) {}

	// Set the number of items in the drop down menu.
	virtual void SetNumberOfEditLines( int numLines );

	//  Add an item to the drop down
	virtual int AddItem(const char *itemText, KeyValues *userData);
	virtual int AddItem(const wchar_t *itemText, KeyValues *userData);

	virtual int GetItemCount();

	// update the item
	virtual bool UpdateItem(int itemID, const char *itemText, KeyValues *userData);
	virtual bool UpdateItem(int itemID, const wchar_t *itemText, KeyValues *userData);
	virtual bool IsItemIDValid( int itemID );

	// set the enabled state of an item
	virtual void SetItemEnabled(const char *itemText, bool state);
	virtual void SetItemEnabled(int itemID, bool state);
	
	// Remove all items from the drop down menu
	virtual void DeleteAllItems();

	// Sorts the items in the list - FIXME does nothing
	virtual void SortItems();

	// Set the visiblity of the drop down menu button.
	virtual void SetDropdownButtonVisible(bool state);

	// Return true if the combobox current has the dropdown menu open
	virtual bool IsDropdownVisible();

	// Activate the item in the menu list,as if that 
	// menu item had been selected by the user
	virtual void ActivateItem(int itemID);
	virtual void ActivateItemByRow(int row);

	virtual int GetActiveItem();
	virtual KeyValues *GetActiveItemUserData();
	virtual KeyValues *GetItemUserData(int itemID);

	// sets a custom menu to use for the dropdown
	virtual void SetMenu( Menu *menu );
	virtual Menu *GetMenu() { return m_pDropDown; }

	// Layout the format of the combo box for drawing on screen
	virtual void PerformLayout();

	virtual void ShowMenu();
	virtual void HideMenu();
	virtual void OnKillFocus();
	virtual void OnMenuClose();
	virtual void DoClick();
	virtual void OnSizeChanged(int wide, int tall);

	enum MenuDirection_e
	{
		LEFT,
		RIGHT,
		UP,
		DOWN,
		CURSOR,	// make the menu appear under the mouse cursor
		ALIGN_WITH_PARENT, // make the menu appear under the parent
	};

	virtual void SetOpenDirection(MenuDirection_e direction);

protected:
	// overrides
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMenuItemSelected();
	virtual void OnCommand( const char *command );
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	// custom message handlers
	virtual void OnSetText( const wchar_t *newtext );
	virtual void OnSetFocus();						// called after the panel receives the keyboard focus
    virtual void OnKeyCodeTyped(KeyCode code);

    void MoveAlongMenuItemList(int direction);

	DECLARE_PANELMAP();

private:
	void DoMenuLayout();

	Menu 				*m_pDropDown;
	ComboBoxButton 		*m_pButton;

	bool 				m_bAllowEdit;
	bool 				m_bHighlight;
	MenuDirection_e 	m_iDirection;
	int 				m_iOpenOffsetY;


	typedef TextEntry BaseClass;
};

} // namespace vgui

#endif // COMBOBOX_H
