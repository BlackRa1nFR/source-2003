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

#include <assert.h>

#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/VGUI.h>
#include <KeyValues.h>
#include <tier0/dbg.h>

#include <vgui_controls/Controls.h>
#include <vgui_controls/FocusNavGroup.h>
#include <vgui_controls/Panel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *panel - parent panel
//-----------------------------------------------------------------------------
FocusNavGroup::FocusNavGroup(Panel *panel) : _mainPanel(panel)
{
	_currentFocus = NULL;
	_topLevelFocus = false;
    _currentDefaultButton = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
FocusNavGroup::~FocusNavGroup()
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets the focus to the previous panel in the tab order
// Input  : *panel - panel currently with focus
//-----------------------------------------------------------------------------
bool FocusNavGroup::RequestFocusPrev(Panel *panel)
{
	if(panel==NULL)
		return false;

	_currentFocus = NULL;
	int newPosition = 9999999;
	if (panel)
	{
		newPosition = panel->GetTabPosition();
	}

	bool bFound = false;
	bool bRepeat = true;
	Panel *best = NULL;
	while (1)
	{
		newPosition--;
		if (newPosition > 0)
		{
			int bestPosition = 0;

			// look for the next tab position
			for (int i = 0; i < _mainPanel->GetChildCount(); i++)
			{
				Panel *child = _mainPanel->GetChild(i);
				if (child->IsVisible() && child->IsEnabled() && child->GetTabPosition())
				{
					int tabPosition = child->GetTabPosition();
					if (tabPosition == newPosition)
					{
						// we've found the right tab
						best = child;
						bestPosition = newPosition;

						// don't loop anymore since we've found the correct panel
						break;
					}
					else if (tabPosition < newPosition && tabPosition > bestPosition)
					{
						// record the match since this is the closest so far
						bestPosition = tabPosition;
						best = child;
					}
				}
			}

			if (!bRepeat)
				break;

			if (best)
				break;
		}
		else
		{
			// reset new position for next loop
			newPosition = 9999999;
		}

		// haven't found an item

		if (!_topLevelFocus)
		{
			// check to see if we should push the focus request up
			if (_mainPanel->GetParent() && _mainPanel->GetParent()->GetVPanel() != surface()->GetEmbeddedPanel())
			{
				// we're not a top level panel, so forward up the request instead of looping
				if (_mainPanel->GetParent()->RequestFocusPrev(_mainPanel))
				{
					bFound = true;
					SetCurrentDefaultButton(NULL);
					break;
				}
			}
		}

		// not found an item, loop back
		newPosition = 9999999;
		bRepeat = false;
	}

	if (best)
	{
		_currentFocus = best;
		best->RequestFocus(-1);
		bFound = true;

        if (!best->CanBeDefaultButton())
        {
            if (_defaultButton)
            {
                SetCurrentDefaultButton(_defaultButton);
            }
			else
			{
				SetCurrentDefaultButton(NULL);

				// we need to ask the parent to set its default button
				if (_mainPanel->GetParent())
				{
					ivgui()->PostMessage(_mainPanel->GetParent()->GetVPanel(), new KeyValues("FindDefaultButton"), NULL);
				}
			}
        }
        else
        {
            SetCurrentDefaultButton(best);
        }
	}
	return bFound;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the focus to the previous panel in the tab order
// Input  : *panel - panel currently with focus
//-----------------------------------------------------------------------------
bool FocusNavGroup::RequestFocusNext(Panel *panel)
{
	// basic recursion guard, in case user has set up a bad focus hierarchy
	static int stack_depth = 0;
	stack_depth++;

	_currentFocus = NULL;
	int newPosition = 0;
	if (panel)
	{
		newPosition = panel->GetTabPosition();
	}

	bool bFound = false;
	bool bRepeat = true;
	Panel *best = NULL;
	while (1)
	{
		newPosition++;
		int bestPosition = 999999;

		// look for the next tab position
		for (int i = 0; i < _mainPanel->GetChildCount(); i++)
		{
			Panel *child = _mainPanel->GetChild(i);
			if (child->IsVisible() && child->IsEnabled() && child->GetTabPosition())
			{
				int tabPosition = child->GetTabPosition();
				if (tabPosition == newPosition)
				{
					// we've found the right tab
					best = child;
					bestPosition = newPosition;

					// don't loop anymore since we've found the correct panel
					break;
				}
				else if (tabPosition > newPosition && tabPosition < bestPosition)
				{
					// record the match since this is the closest so far
					bestPosition = tabPosition;
					best = child;
				}
			}
		}

		if (!bRepeat)
			break;

		if (best)
			break;

		// haven't found an item

		// check to see if we should push the focus request up
		if (!_topLevelFocus)
		{
			if (_mainPanel->GetParent() && _mainPanel->GetParent()->GetVPanel() != surface()->GetEmbeddedPanel())
			{
				// we're not a top level panel, so forward up the request instead of looping
				if (stack_depth < 15)
				{
					if (_mainPanel->GetParent()->RequestFocusNext(_mainPanel))
					{
						bFound = true;
						SetCurrentDefaultButton(NULL);
						break;
					}

					// if we find one then we break, otherwise we loop
				}
			}
		}
		
		// loop back
		newPosition = 0;
		bRepeat = false;
	}

	if (best)
	{
		_currentFocus = best;
		best->RequestFocus(1);
		bFound = true;

        if (!best->CanBeDefaultButton())
        {
            if (_defaultButton)
			{
                SetCurrentDefaultButton(_defaultButton);
			}
			else
			{
				SetCurrentDefaultButton(NULL);

				// we need to ask the parent to set its default button
				if (_mainPanel->GetParent())
				{
					ivgui()->PostMessage(_mainPanel->GetParent()->GetVPanel(), new KeyValues("FindDefaultButton"), NULL);
				}
			}
        }
        else
        {
            SetCurrentDefaultButton(best);
        }
	}

	stack_depth--;
	return bFound;
}

//-----------------------------------------------------------------------------
// Purpose: sets the panel that owns this FocusNavGroup to be the root in the focus traversal heirarchy
//-----------------------------------------------------------------------------
void FocusNavGroup::SetFocusTopLevel(bool state)
{
	_topLevelFocus = state;
}

//-----------------------------------------------------------------------------
// Purpose: sets panel which receives input when ENTER is hit
// Input  : *panel - 
//-----------------------------------------------------------------------------
void FocusNavGroup::SetDefaultButton(Panel *panel)
{
	if (panel == _defaultButton)
		return;

    Assert(panel->CanBeDefaultButton());

	_defaultButton = panel;
    SetCurrentDefaultButton(panel);
}

//-----------------------------------------------------------------------------
// Purpose: sets panel which receives input when ENTER is hit
// Input  : *panel - 
//-----------------------------------------------------------------------------
void FocusNavGroup::SetCurrentDefaultButton(Panel *panel)
{
	if (panel == _currentDefaultButton)
		return;

	if (_currentDefaultButton != NULL && _currentDefaultButton.Get())
	{
		ivgui()->PostMessage(_currentDefaultButton->GetVPanel(), new KeyValues("SetAsCurrentDefaultButton", "state", 0), NULL);
	}

	_currentDefaultButton = panel;

	if (_currentDefaultButton != NULL && _currentDefaultButton.Get())
	{
		ivgui()->PostMessage(_currentDefaultButton->GetVPanel(), new KeyValues("SetAsCurrentDefaultButton", "state", 1), NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets panel which receives input when ENTER is hit
// Input  : *panel - 
//-----------------------------------------------------------------------------
Panel *FocusNavGroup::GetCurrentDefaultButton()
{
	return _currentDefaultButton;
}

//-----------------------------------------------------------------------------
// Purpose: sets panel which receives input when ENTER is hit
// Input  : *panel - 
//-----------------------------------------------------------------------------
Panel *FocusNavGroup::GetDefaultButton()
{
	return _defaultButton;
}

//-----------------------------------------------------------------------------
// Purpose: finds the panel which is activated by the specified key
// Input  : code - the keycode of the hotkey
// Output : Panel * - NULL if no panel found
//-----------------------------------------------------------------------------
Panel *FocusNavGroup::FindPanelByHotkey(wchar_t key)
{
	for (int i = 0; i < _mainPanel->GetChildCount(); i++)
	{
		Panel *hot = _mainPanel->GetChild(i)->HasHotkey(key);
		if (hot && hot->IsVisible() && hot->IsEnabled())
		{
			return hot;
		}
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Panel
//-----------------------------------------------------------------------------
Panel *FocusNavGroup::GetDefaultPanel()
{
	for (int i = 0; i < _mainPanel->GetChildCount(); i++)
	{
		if (_mainPanel->GetChild(i)->GetTabPosition() == 1)
		{
			return _mainPanel->GetChild(i);
		}
	}

	return NULL;	// no specific panel set
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Panel
//-----------------------------------------------------------------------------
Panel *FocusNavGroup::GetCurrentFocus()
{
	return _currentFocus.Get();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the current focus
// Output : Panel
//-----------------------------------------------------------------------------
Panel *FocusNavGroup::SetCurrentFocus(Panel *focus, Panel *defaultPanel)
{
	_currentFocus = focus;

    // if we haven't found a default panel yet, let's see if we know of one
    if (defaultPanel == NULL)
    {
        // can this focus itself by the default
        if (focus->CanBeDefaultButton())
        {
            defaultPanel = focus;
        }
        else if (_defaultButton)        // do we know of a default button
        {
            defaultPanel = _defaultButton;
        }
    }

    SetCurrentDefaultButton(defaultPanel);
    return defaultPanel;
}
