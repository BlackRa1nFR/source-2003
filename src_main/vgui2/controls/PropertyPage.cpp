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
#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
PropertyPage::PropertyPage(Panel *parent, const char *panelName, bool PaintBorder) : EditablePanel(parent, panelName)
{
	_paintRaised=PaintBorder;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
PropertyPage::~PropertyPage()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called when page is loaded.  Data should be reloaded from document into controls.
//-----------------------------------------------------------------------------
void PropertyPage::OnResetData()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called when the OK / Apply button is pressed.  Changed data should be written into document.
//-----------------------------------------------------------------------------
void PropertyPage::OnApplyChanges()
{
}

//-----------------------------------------------------------------------------
// Purpose: Designed to be overriden
//-----------------------------------------------------------------------------
void PropertyPage::OnPageShow()
{
}

//-----------------------------------------------------------------------------
// Purpose: Designed to be overriden
//-----------------------------------------------------------------------------
void PropertyPage::OnPageHide()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pageTab - 
//-----------------------------------------------------------------------------
void PropertyPage::OnPageTabActivated(Panel *pageTab)
{
	_pageTab = pageTab;
}

//-----------------------------------------------------------------------------
// Purpose: Paints the border, but with the required breaks for the page tab
//-----------------------------------------------------------------------------
void PropertyPage::PaintBorder()
{
	IBorder *border = GetBorder();

	// setup border break
	if (_paintRaised==true && border && _pageTab.Get())
	{
		int px, py, pwide, ptall;
		_pageTab->GetBounds(px, py, pwide, ptall);
	
		int wide, tall;
		GetSize(wide, tall);
		border->Paint(0, 0, wide, tall, IBorder::SIDE_TOP, px+1, px + pwide -1);
	}
	else
	{
		// Paint the border
		BaseClass::PaintBorder();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PropertyPage::OnKeyCodeTyped(KeyCode code)
{
	switch (code)
	{
        // left and right only get propogated to parents if our tab has focus
	case KEY_RIGHT:
		{
            if (_pageTab != NULL && _pageTab->HasFocus())
                BaseClass::OnKeyCodeTyped(code);
			break;
		}
	case KEY_LEFT:
		{
            if (_pageTab != NULL && _pageTab->HasFocus())
                BaseClass::OnKeyCodeTyped(code);
			break;
		}
	default:
		BaseClass::OnKeyCodeTyped(code);
		break;
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *inResourceData - 
//-----------------------------------------------------------------------------
void PropertyPage::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	if(_paintRaised)
	{
		SetBorder(pScheme->GetBorder("ButtonBorder"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PropertyPage::SetVisible(bool state)
{
    if (IsVisible() && !state)
    {
        // if we're going away and we have a current button, get rid of it
        if (GetFocusNavGroup().GetCurrentDefaultButton())
        {
            GetFocusNavGroup().SetCurrentDefaultButton(NULL);
        }
    }

    BaseClass::SetVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t PropertyPage::m_MessageMap[] =
{
	MAP_MESSAGE( PropertyPage, "ResetData", OnResetData ),
	MAP_MESSAGE( PropertyPage, "ApplyChanges", OnApplyChanges ),
	MAP_MESSAGE( PropertyPage, "PageShow", OnPageShow),
	MAP_MESSAGE( PropertyPage, "PageHide", OnPageHide),
	MAP_MESSAGE_PTR( PropertyPage, "PageTabActivated", OnPageTabActivated, "Panel"),
};
IMPLEMENT_PANELMAP( PropertyPage, BaseClass );

