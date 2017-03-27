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

#include <vgui/IBorder.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/PropertyPage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

#ifndef min
namespace {
// keep this private to this file
	inline int min(int a,int b)
	{
		if(a<b)
			return a;
		else
			return b;
	}
	inline int max(int a,int b)
	{
		if(a>b)
			return a;
		else
			return b;
	}
}
#endif

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: A single tab
//-----------------------------------------------------------------------------
class PageTab : public Button
{
private:
	bool _active;
	Color _textColor;
	Color _dimTextColor;
	int m_bMaxTabWidth;

public:
	PageTab(Panel *parent, const char *panelName, const char *text, int maxTabWidth) : Button(parent, panelName, text) 
	{
		SetCommand(new KeyValues("TabPressed"));
		_active = false;
		m_bMaxTabWidth = maxTabWidth;
	}

	~PageTab() {}

	virtual void ApplySchemeSettings(IScheme *pScheme)
	{
		// set up the scheme settings
		Button::ApplySchemeSettings(pScheme);

		_textColor = GetSchemeColor("BrightControlText", GetFgColor(), pScheme);
		_dimTextColor = GetSchemeColor("FgColorDim", GetFgColor(), pScheme);

		int wide, tall;
		int contentWide, contentTall;
		GetSize(wide, tall);
		GetContentSize(contentWide, contentTall);

		wide = max(m_bMaxTabWidth, contentWide + 10);  // 10 = 5 pixels margin on each side
		SetSize(wide, tall);
	}

	IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
	{
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		if (_active)
		{
			return pScheme->GetBorder("TabActiveBorder");
		}

		return pScheme->GetBorder("TabBorder");
	}

	virtual Color GetButtonFgColor()
	{
		if (_active)
		{
			return _textColor;
		}
		else
		{
			return _dimTextColor;
		}
	}

	virtual void SetActive(bool state)
	{
		_active = state;
		InvalidateLayout();
		Repaint();
	}

    virtual bool CanBeDefaultButton(void)
    {
        return false;
    }

	//Fire action signal when mouse is pressed down instead  of on release.
	virtual void OnMousePressed(MouseCode code) 
	{
        
		// check for context menu open
		if (code == MOUSE_RIGHT)
		{
			PostActionSignal(new KeyValues("OpenContextMenu", "itemID", -1));
		}
		else
		{
			if (!IsEnabled())
				return;
			
			if (!IsMouseClickEnabled(code))
				return;
			
			if (IsUseCaptureMouseEnabled())
			{
				{
					RequestFocus();
					FireActionSignal();
					SetSelected(true);
					Repaint();
				}
				
				// lock mouse input to going to this button
				input()->SetMouseCapture(GetVPanel());
			}
		}
	}

	virtual void OnMouseReleased(MouseCode code)
	{
		// ensure mouse capture gets released
		if (IsUseCaptureMouseEnabled())
		{
			input()->SetMouseCapture(NULL);
		}

		// make sure the button gets unselected
		SetSelected(false);
		Repaint();
	}
};

}; // namespace vgui



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
PropertySheet::PropertySheet(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
	_activePage = NULL;
	_activeTab = NULL;
	_tabWidth = 64;
	_activeTabIndex = 0;
	_showTabs = true;
	_combo = NULL;
    _tabFocus = false;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
PropertySheet::PropertySheet(Panel *parent, const char *panelName,ComboBox *combo) : Panel(parent, panelName)
{
	_activePage = NULL;
	_activeTab = NULL;
	_tabWidth = 64;
	_activeTabIndex = 0;
	_combo=combo;
	_combo->AddActionSignalTarget(this);
	_showTabs = false;
    _tabFocus = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
PropertySheet::~PropertySheet()
{
}

//-----------------------------------------------------------------------------
// Purpose: adds a page to the sheet
// Input  : *page - 
//			*title - 
//-----------------------------------------------------------------------------
void PropertySheet::AddPage(Panel *page, const char *title)
{
	if(page==NULL)
	{
		return;
	}
	// don't add the page if we already have it
	if (_pages.HasElement(page))
		return;

	PageTab *tab = new PageTab(this, "tab", title, _tabWidth);
	if(_showTabs)
	{
		tab->AddActionSignalTarget(this);
	}
	else if (_combo)
	{
		_combo->AddItem(title, NULL);
	}
	_pageTabs.AddElement(tab);

	_pages.AddElement(page);

	page->SetParent(this);
	page->AddActionSignalTarget(this);
	PostMessage(page, new KeyValues("ResetData"));

	page->SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *page - 
//-----------------------------------------------------------------------------
void PropertySheet::SetActivePage(Panel *page)
{
	// walk the list looking for this page
	int index = _pages.FindElement(page);
	if (index < 0)
		return;

	ChangeActiveTab(index);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pixels - 
//-----------------------------------------------------------------------------
void PropertySheet::SetTabWidth(int pixels)
{
	_tabWidth = pixels;
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: reloads the data in all the property page
//-----------------------------------------------------------------------------
void PropertySheet::ResetAllData()
{
	// iterate all the dialogs resetting them
	for (int i = 0; i < _pages.GetCount(); i++)
	{
		ivgui()->PostMessage(_pages[i]->GetVPanel(), new KeyValues("ResetData"), GetVPanel());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies any changes made by the dialog
//-----------------------------------------------------------------------------
void PropertySheet::ApplyChanges()
{
	// iterate all the dialogs resetting them
	for (int i = 0; i < _pages.GetCount(); i++)
	{
		ivgui()->PostMessage(_pages[i]->GetVPanel(), new KeyValues("ApplyChanges"), GetVPanel());
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets a pointer to the currently active page
// Output : Panel
//-----------------------------------------------------------------------------
Panel *PropertySheet::GetActivePage()
{
	return _activePage;
}

//-----------------------------------------------------------------------------
// Purpose: gets a pointer to the currently active tab
// Output : Panel
//-----------------------------------------------------------------------------
Panel *PropertySheet::GetActiveTab()
{
	return _activeTab;
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of panels in the sheet
// Output : Panel
//-----------------------------------------------------------------------------
int PropertySheet::GetNumPages()
{
	return _pages.GetCount();
}

//-----------------------------------------------------------------------------
// Purpose: returns the name contained in the active tab
// Input  : a text buffer to contain the output 
//-----------------------------------------------------------------------------
void PropertySheet::GetActiveTabTitle(char *textOut, int bufferLen)
{
	if(_activeTab) _activeTab->GetText(textOut, bufferLen);
}

//-----------------------------------------------------------------------------
// Purpose: returns the name contained in the active tab
// Input  : a text buffer to contain the output 
//-----------------------------------------------------------------------------
bool PropertySheet::GetTabTitle(int i,char *textOut, int bufferLen)
{
	if(i<0 && i>_pageTabs.GetCount()) 
	{
		return false;
	}

	_pageTabs[i]->GetText(textOut, bufferLen);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the index of the currently active page
//-----------------------------------------------------------------------------
int PropertySheet::GetActivePageNum()
{
	for (int i = 0; i < _pages.GetCount(); i++)
	{
		if(_pages[i]==_activePage) 
		{
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Forwards focus requests to current active page
// Input  : direction - 
//-----------------------------------------------------------------------------
void PropertySheet::RequestFocus(int direction)
{
    if (direction == -1 || direction == 0)
    {
    	if (_activePage)
    	{
    		_activePage->RequestFocus(direction);
            _tabFocus = false;
    	}
    }
    else 
    {
        if (_showTabs && _activeTab)
        {
            _activeTab->RequestFocus(direction);
            _tabFocus = true;
        }
		else if (_activePage)
    	{
    		_activePage->RequestFocus(direction);
            _tabFocus = false;
    	}
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool PropertySheet::RequestFocusPrev(Panel *panel)
{
    if (_tabFocus || !_showTabs || !_activeTab)
    {
        _tabFocus = false;
        return BaseClass::RequestFocusPrev(panel);
    }
    else
    {
        if (GetParent())
        {
            PostMessage(GetParent(), new KeyValues("FindDefaultButton"));
        }
        _activeTab->RequestFocus(-1);
        _tabFocus = true;
        return true;
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool PropertySheet::RequestFocusNext(Panel *panel)
{
    if (!_tabFocus || !_activePage)
    {
        return BaseClass::RequestFocusNext(panel);
    }
    else
    {
        if (!_activeTab)
        {
            return BaseClass::RequestFocusNext(panel);
        }
        else
        {
            _activePage->RequestFocus(1);
            _tabFocus = false;
            return true;
        }
    }
}
//-----------------------------------------------------------------------------
// Purpose: Lays out the dialog
//-----------------------------------------------------------------------------
void PropertySheet::PerformLayout()
{
	BaseClass::PerformLayout();
	int i;

	if (!_activePage)
	{
		// first page becomes the active page
		ChangeActiveTab(0);
		if (_activePage)
			_activePage->RequestFocus(0);
	}

	int x, y, wide, tall;
	GetBounds(x, y, wide, tall);
	if (_activePage)
	{
		if(_showTabs)
		{
			_activePage->SetBounds(0, 28, wide, tall - 28);
		}
		else
		{
			_activePage->SetBounds(0, 0, wide, tall );
		}
		_activePage->InvalidateLayout();
	}

	
	int xtab;
	int limit=_pageTabs.GetCount();

	xtab=0;

	// draw the visible tabs
	if(_showTabs)
	{
		for (i = 0; i < limit; i++)
		{
            int width, tall;
            _pageTabs[i]->GetSize(width, tall);
			if (_pageTabs[i] == _activeTab)
			{
				// active tab is taller
				_activeTab->SetBounds(xtab, 2, width, 27);
			}
			else
			{
				_pageTabs[i]->SetBounds(xtab, 4, width, 25);
			}
			_pageTabs[i]->SetVisible(true);
			xtab += (width + 1);
		}
	}
	else
	{
		for (i = 0; i < limit; i++)
		{
			_pageTabs[i]->SetVisible(false);
		}
	}

	// ensure draw order (page drawing over all the tabs except one)
	if (_activePage)
	{
		_activePage->MoveToFront();
		_activePage->Repaint();
	}
	if (_activeTab)
	{
		_activeTab->MoveToFront();
		_activeTab->Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Switches the active panel
// Input  : *panel - 
//-----------------------------------------------------------------------------
void PropertySheet::OnTabPressed(Panel *panel)
{
	// look for the tab in the list
	for (int i = 0; i < _pageTabs.GetCount(); i++)
	{
		if (_pageTabs[i] == panel)
		{
			// flip to the new tab
			ChangeActiveTab(i);
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the panel associated with index i
// Input  : the index of the panel to return 
//-----------------------------------------------------------------------------
Panel *PropertySheet::GetPage(int i) 
{
	if(i<0 && i>_pages.GetCount()) 
	{
		return NULL;
	}

	return _pages[i];
}


//-----------------------------------------------------------------------------
// Purpose: disables page "i"
// Input  : the tab title of the page to disable
//-----------------------------------------------------------------------------
void PropertySheet::DisablePage(const char *title) 
{
	EnDisPage(title,false);
}

//-----------------------------------------------------------------------------
// Purpose: enables page "i"
// Input  : the tab title of the page to enabled
//-----------------------------------------------------------------------------
void PropertySheet::EnablePage(const char *title) 
{
	EnDisPage(title,true);
}

//-----------------------------------------------------------------------------
// Purpose: disables page "i"
// Input  : the tab title of the page to disable
//-----------------------------------------------------------------------------
void PropertySheet::EnDisPage(const char *title,bool state) 
{
	for (int i = 0; i < _pageTabs.GetCount(); i++)
	{
		if(_showTabs)
		{
			char tmp[50];
			_pageTabs[i]->GetText(tmp,50);
			if (!strnicmp(title,tmp,strlen(tmp)))
			{	
				_pageTabs[i]->SetEnabled(state);
			}
		}
		else
		{
			_combo->SetItemEnabled(title,state);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: deletes the page associated with panel
// Input  : *panel - the panel of the page to remove
//-----------------------------------------------------------------------------
void PropertySheet::DeletePage(Panel *panel) 
{
	if (! _pages.HasElement(panel))
		return;

	int location = _pages.FindElement(panel);

	if(_activePage == panel) 
	{
		// if this page is currently active, backup to the page before this.
		ChangeActiveTab( location - 1 ); 
	}
	//	changeActiveTab( 0 ); 

	// ASSUMPTION = that the number of pages equals number of tabs
	if(_showTabs)
	{
		_pageTabs[location]->RemoveActionSignalTarget(this);
	}
	// now remove the panels
	PageTab *tab  = _pageTabs[location];
	_pageTabs.RemoveElement(_pageTabs[location]);
	tab->MarkForDeletion();
	
	_pages.RemoveElement(panel);
	panel->MarkForDeletion();

	PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: flips to the new tab, sending out all the right notifications
//  flipping to a tab activates the tab.
// Input  : index - 
//-----------------------------------------------------------------------------
void PropertySheet::ChangeActiveTab(int index)
{
	if (_pages[index] == NULL)
	{
		return;
	}

	if (_pages[index] == _activePage)
	{
		_activeTab->RequestFocus();
		_tabFocus = true;
		return;
	}

	// notify old page
	if (_activePage)
	{
		ivgui()->PostMessage(_activePage->GetVPanel(), new KeyValues("PageHide"), GetVPanel());
		KeyValues *msg = new KeyValues("PageTabActivated");
		msg->SetPtr("panel", (Panel *)NULL);
		ivgui()->PostMessage(_activePage->GetVPanel(), msg, GetVPanel());

		_activePage->SetVisible(false);
	}
	if (_activeTab)
	{
		//_activeTabIndex=index;
		_activeTab->SetActive(false);

		// does the old tab have the focus?
		_tabFocus = _activeTab->HasFocus();
	}
	else
	{
		_tabFocus = false;
	}

	// flip page
	_activePage = _pages[index];
	_activeTab = _pageTabs[index];
	_activeTabIndex = index;

	_activePage->SetVisible(true);
	_activePage->MoveToFront();
	
	_activeTab->SetVisible(true);
	_activeTab->MoveToFront();
	_activeTab->SetActive(true);

	if (_tabFocus)
	{
		// if a tab already has focused,give the new tab the focus
		_activeTab->RequestFocus();
	}
	else
	{
		// otherwise, give the focus to the page
		_activePage->RequestFocus();
	}

	if(!_showTabs)
	{
		_combo->ActivateItemByRow(index);
	}

	// notify
	ivgui()->PostMessage(_activePage->GetVPanel(), new KeyValues("PageShow"), GetVPanel());

	KeyValues *msg = new KeyValues("PageTabActivated");
	msg->SetPtr("panel", (Panel *)_activeTab);
	ivgui()->PostMessage(_activePage->GetVPanel(), msg, GetVPanel());

	// tell parent
	PostActionSignal(new KeyValues("PageChanged"));

	// Repaint
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Gets the panel with the specified hotkey, from the current page
//-----------------------------------------------------------------------------
Panel *PropertySheet::HasHotkey(wchar_t key)
{
	if (!_activePage)
		return NULL;

	for (int i = 0; i < _activePage->GetChildCount(); i++)
	{
		Panel *hot = _activePage->GetChild(i)->HasHotkey(key);
		if (hot)
		{
			return hot;
		}
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: catches the opencontextmenu event
//-----------------------------------------------------------------------------
void PropertySheet::OnOpenContextMenu()
{
	// tell parent
	PostActionSignal(new KeyValues("OpenContextMenu"));
}

//-----------------------------------------------------------------------------
// Purpose: Handle key presses, through tabs.
//-----------------------------------------------------------------------------
void PropertySheet::OnKeyCodeTyped(KeyCode code)
{
	switch (code)
	{
		// for now left and right arrows just open or close submenus if they are there.
	case KEY_RIGHT:
		{
			ChangeActiveTab(_activeTabIndex+1);
			break;
		}
	case KEY_LEFT:
		{
			ChangeActiveTab(_activeTabIndex-1);
			break;
		}
	default:
		BaseClass::OnKeyCodeTyped(code);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called by the associated combo box (if in that mode), changes the current panel
//-----------------------------------------------------------------------------
void PropertySheet::OnTextChanged(Panel *panel,const wchar_t *wszText)
{
	if ( panel == _combo )
	{
		wchar_t tabText[30];
		for(int i = 0 ; i < _pageTabs.GetCount() ; i++ )
		{
			tabText[0] = 0;
			_pageTabs[i]->GetText(tabText,30);
			if ( !wcsicmp(wszText,tabText) )
			{
				ChangeActiveTab(i);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PropertySheet::OnCommand(const char *command)
{
    // propogate the close command to our parent
	if (!stricmp(command, "Close") && GetParent())
    {
        GetParent()->OnCommand(command);
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PropertySheet::OnApplyButtonEnable()
{
	// tell parent
	PostActionSignal(new KeyValues("ApplyButtonEnable"));	
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *defaultButton - 
//-----------------------------------------------------------------------------
void PropertySheet::OnCurrentDefaultButtonSet(Panel *defaultButton)
{
	// forward the message up
	if (GetParent())
	{
		KeyValues *msg = new KeyValues("CurrentDefaultButtonSet");
		msg->SetPtr("button", defaultButton);
		PostMessage(GetParent(), msg);
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *defaultButton - 
//-----------------------------------------------------------------------------
void PropertySheet::OnDefaultButtonSet(Panel *defaultButton)
{
	// forward the message up
	if (GetParent())
	{
		KeyValues *msg = new KeyValues("DefaultButtonSet");
		msg->SetPtr("button", defaultButton);
		PostMessage(GetParent(), msg);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PropertySheet::OnFindDefaultButton()
{
    if (GetParent())
    {
        PostMessage(GetParent(), new KeyValues("FindDefaultButton"));
    }
}


//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t PropertySheet::m_MessageMap[] =
{
	MAP_MESSAGE_PTR( PropertySheet, "TabPressed", OnTabPressed, "panel" ),
	MAP_MESSAGE( PropertySheet, "OpenContextMenu", OnOpenContextMenu ),
	MAP_MESSAGE_PTR_CONSTWCHARPTR( PropertySheet, "TextChanged", OnTextChanged, "panel", "text" ),
	MAP_MESSAGE( PropertySheet, "ApplyButtonEnable", OnApplyButtonEnable ),
	MAP_MESSAGE( PropertySheet, "FindDefaultButton", OnFindDefaultButton ),
	MAP_MESSAGE_PTR( PropertySheet, "DefaultButtonSet", OnDefaultButtonSet, "button" ),
	MAP_MESSAGE_PTR( PropertySheet, "CurrentDefaultButtonSet", OnCurrentDefaultButtonSet, "button" ),
};
IMPLEMENT_PANELMAP(PropertySheet, BaseClass);


