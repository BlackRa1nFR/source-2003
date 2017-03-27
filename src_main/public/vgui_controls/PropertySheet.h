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

#ifndef PROPERTYSHEET_H
#define PROPERTYSHEET_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/Dar.h>
#include <vgui_controls/Panel.h>

namespace vgui
{

class Panel;
class PageTab;
class TabButton;
class ComboBox;

typedef enum { LEFT, RIGHT , BOTH, NEITHER } ButtonsToDraw;


//-----------------------------------------------------------------------------
// Purpose: Tabbed property sheet.  Holds and displays a set of Panel's
//-----------------------------------------------------------------------------
class PropertySheet : public Panel
{
public:
	PropertySheet(Panel *parent, const char *panelName);
	PropertySheet(Panel *parent, const char *panelName,ComboBox *combo);
	~PropertySheet();

	// Adds a page to the sheet.  The first page added becomes the active sheet.
	virtual void AddPage(Panel *page, const char *title);

	// sets the current page
	virtual void SetActivePage(Panel *page);

	// sets the width, in pixels, of the page tab buttons.
	virtual void SetTabWidth(int pixels);

	// Gets a pointer to the currently active page.
	virtual Panel *GetActivePage();

	// reloads the data in all the property page
	virtual void ResetAllData();

	// writes out any changed data to the doc
	virtual void ApplyChanges();

	// focus handling - passed on to current active page
	virtual void RequestFocus(int direction = 0);
	virtual bool RequestFocusPrev(Panel *panel = NULL);
	virtual bool RequestFocusNext(Panel *panel = NULL);

	// returns the ith panel 
	virtual Panel *GetPage(int i);

	// deletes this panel from the sheet
	virtual void DeletePage(Panel *panel) ;

	// returns the current activated tab
	virtual Panel *GetActiveTab();

	// returns the title text of the tab
	virtual void GetActiveTabTitle(char *textOut, int bufferLen);

	// returns the title of tab "i"
	virtual bool GetTabTitle(int i,char *textOut, int bufferLen);

	// returns the index of the active page
	virtual int GetActivePageNum();

	// returns the number of pages in the sheet
	virtual int GetNumPages();

	// disable the page with title "title" 
	virtual void DisablePage(const char *title);

	// enable the page with title "title" 
	virtual void EnablePage(const char *title);

	/* MESSAGES SENT TO PAGES
		"PageShow"	- sent when a page is shown
		"PageHide"	- sent when a page is hidden
		"ResetData"	- sent when the data should be reloaded from doc
		"ApplyChanges" - sent when data should be written to doc
	*/

	DECLARE_PANELMAP();

protected:
	virtual void PerformLayout();
	virtual Panel *HasHotkey(wchar_t key);

	// internal message handlers
	virtual void OnTabPressed(Panel *panel);
	virtual void ChangeActiveTab(int index);
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnTextChanged(Panel *panel, const wchar_t *text);

	virtual void OnOpenContextMenu();
	virtual void OnCommand(const char *command);
	
	void OnApplyButtonEnable();

	// called when default button has been set
	virtual void OnDefaultButtonSet(Panel *defaultButton);
	// called when the current default button has been set
	virtual void OnCurrentDefaultButtonSet(Panel *defaultButton);
    virtual void OnFindDefaultButton();

private:
	
	// enable/disable the page with title "title" 
	virtual void EnDisPage(const char *title,bool state);

	Dar<Panel *> _pages;
	Dar<PageTab *> _pageTabs;
	Panel *_activePage;
	PageTab *_activeTab;
	int _tabWidth;
	int _activeTabIndex;
	bool _showTabs;
	ComboBox *_combo;
    bool _tabFocus;
//    int _tabPosition;

	typedef Panel BaseClass;
};

}; // namespace vgui

#endif // PROPERTYSHEET_H
