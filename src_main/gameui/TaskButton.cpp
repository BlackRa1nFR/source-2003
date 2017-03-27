//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "TaskButton.h"

#include <vgui/MouseCode.h>
#include <vgui_controls/Image.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTaskButton::CTaskButton(Panel *parent, vgui::VPANEL task) : Label(parent, "TaskButton", "<unknown>") 
{
	m_hTask = ivgui()->PanelToHandle(task);
	m_bHasTitle = false;

	// set up the images
	SetTextImageIndex(1);
	SetImageAtIndex(0, &m_WindowStateImage, 4);
	SetImagePreOffset(1, 4);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTaskButton::~CTaskButton()
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets the title of the task button
//-----------------------------------------------------------------------------
void CTaskButton::SetTitle(const wchar_t *title)
{
	SetText(title);
	if (title && *title)
	{
		m_bHasTitle = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the selection state of the task button
//-----------------------------------------------------------------------------
void CTaskButton::SetSelected(bool state)
{
	if (state == m_bSelected)
		return;	// state hasn't changed
	m_bSelected = state;

	VPANEL task = ivgui()->HandleToPanel(m_hTask);
	if (!task)
		return;

	if (m_bSelected)
	{
		// Activate the window
		surface()->SetMinimized(task, false);
		ipanel()->SetVisible(task, true);

		// if a child of the task is in front, then don't move the panel in front
		if (!ipanel()->HasParent(vgui::surface()->GetTopmostPopup(), task))
		{
			ipanel()->MoveToFront(task);
		}

		// set our text color as enabled
		SetFgColor(m_SelectedColor);
		m_WindowStateImage.SetFocused();
	}
	else
	{
		// set our text color as without focus
		SetFgColor(m_LabelColor);

		if (vgui::surface()->IsMinimized(task))
		{
			m_WindowStateImage.SetMinimized();
		}
		else
		{
			m_WindowStateImage.SetOutOfFocus();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the task button should be displayed
//-----------------------------------------------------------------------------
bool CTaskButton::ShouldDisplay()
{
	if (!m_bHasTitle)
		return false;

	VPANEL task = ivgui()->HandleToPanel(m_hTask);
	if (!task)
		return false;

	if (!ipanel()->IsVisible(task))
	{
		// if we're not visible, only display if we're minimized
		if (!vgui::surface()->IsMinimized(task))
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to the associated task
//-----------------------------------------------------------------------------
VPANEL CTaskButton::GetTaskPanel() 
{ 
	return ivgui()->HandleToPanel(m_hTask);
}

//-----------------------------------------------------------------------------
// Purpose: input handling
//-----------------------------------------------------------------------------
void CTaskButton::OnMousePressed(MouseCode mouseCode)
{
	if (m_bSelected)
	{
		// if we're already selected, then minimize the window
		VPANEL task = ivgui()->HandleToPanel(m_hTask);
		if (task)
		{
			surface()->SetMinimized(task, true);
		}
	}
	else
	{
		SetSelected(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: input handling
//-----------------------------------------------------------------------------
void CTaskButton::OnMouseDoublePressed(MouseCode mouseCode)
{
	OnMousePressed(mouseCode);
}

//-----------------------------------------------------------------------------
// Purpose: setup display
//-----------------------------------------------------------------------------
void CTaskButton::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_LabelColor = GetSchemeColor("DisabledText1", pScheme);
	m_SelectedColor = GetSchemeColor("BrightBaseText", pScheme);
	SetContentAlignment(Label::a_west);
	SetBgColor(Color(0, 0, 0, 0));
	SetTextImageIndex(1);
	SetImageAtIndex(0, &m_WindowStateImage, 4);
	SetImagePreOffset(1, 4);

	// set the image height to be the same height as the text
	m_WindowStateImage.SetTall(surface()->GetFontTall(GetFont()));
}

//-----------------------------------------------------------------------------
// Purpose: Image of the window state
//-----------------------------------------------------------------------------
enum 
{
	WINDOW_STATE_FOCUS,
	WINDOW_STATE_OUTOFFOCUS,
	WINDOW_STATE_MINIMIZED,
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWindowStateImage::CWindowStateImage()
{
	SetSize(12, 24);
	m_iState = WINDOW_STATE_OUTOFFOCUS;
}

//-----------------------------------------------------------------------------
// Purpose: sets the height of the image
//-----------------------------------------------------------------------------
void CWindowStateImage::SetTall(int tall)
{
	SetSize(12, tall);
}

//-----------------------------------------------------------------------------
// Purpose: renders the window state image
//-----------------------------------------------------------------------------
void CWindowStateImage::Paint()
{
	int wide, tall;
	GetSize(wide, tall);
	const int xsize = 8;
	int xpad = (wide - xsize) / 2;
	int ypad = (tall - 8);
	int yoffs = 3;	// correction margin for making it sit square with bottom of text

	if (m_iState == WINDOW_STATE_FOCUS)
	{
		DrawSetColor(255, 255, 255, 255);
		DrawFilledRect(xpad, ypad - yoffs, wide - xpad, tall - yoffs);
	}
	else if (m_iState == WINDOW_STATE_OUTOFFOCUS)
	{
		DrawSetColor(100, 120, 100, 255);
		DrawFilledRect(xpad, ypad - yoffs, wide - xpad, tall - yoffs);
	}
	else if (m_iState == WINDOW_STATE_MINIMIZED)
	{
		DrawSetColor(100, 120, 100, 255);
		DrawFilledRect(xpad, tall - 4 - yoffs, wide - xpad, tall - yoffs);
	}
}

//-----------------------------------------------------------------------------
// Purpose: image state
//-----------------------------------------------------------------------------
void CWindowStateImage::SetFocused()
{
	m_iState = WINDOW_STATE_FOCUS;
}

//-----------------------------------------------------------------------------
// Purpose: image state
//-----------------------------------------------------------------------------
void CWindowStateImage::SetOutOfFocus()
{
	m_iState = WINDOW_STATE_OUTOFFOCUS;
}

//-----------------------------------------------------------------------------
// Purpose: image state
//-----------------------------------------------------------------------------
void CWindowStateImage::SetMinimized()
{
	m_iState = WINDOW_STATE_MINIMIZED;
}

