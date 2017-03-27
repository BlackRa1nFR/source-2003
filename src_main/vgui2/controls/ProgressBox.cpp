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

#include <vgui/IInput.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <KeyValues.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/ProgressBox.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ProgressBox::ProgressBox(const char *title, const char *text, const char *pszUnknownTimeString, Panel *parent) : Frame(parent, NULL, false)
{	
	SetTitle(title, true);
	m_pMessageLabel = new Label(this, NULL, pszUnknownTimeString);

	const wchar_t *ws = localize()->Find(text);
	if (ws)
	{
		wcsncpy(m_wcsInfoString, ws, sizeof(m_wcsInfoString) / sizeof(wchar_t));
	}
	else
	{
		m_wcsInfoString[0] = 0;
	}

	ws = localize()->Find(pszUnknownTimeString);
	if (ws)
	{
		wcsncpy(m_wszUnknownTimeString, ws, sizeof(m_wszUnknownTimeString) / sizeof(wchar_t));
	}
	else
	{
		m_wszUnknownTimeString[0] = 0;
	}
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ProgressBox::ProgressBox(const wchar_t *wszTitle, const wchar_t *wszText, const wchar_t *wszUnknownTimeString, Panel *parent) : Frame(parent, NULL, false)
{	
	SetTitle(wszTitle, true);
	m_pMessageLabel = new Label(this, NULL, wszUnknownTimeString);
	wcsncpy(m_wcsInfoString, wszText, sizeof(m_wcsInfoString) / sizeof(wchar_t));
	wcsncpy(m_wszUnknownTimeString, wszUnknownTimeString, sizeof(m_wszUnknownTimeString) / sizeof(wchar_t));
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor Helper
//-----------------------------------------------------------------------------
void ProgressBox::Init()
{
	SetMenuButtonResponsive(false);
	SetMinimizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetSize(384, 128);
	m_flCurrentProgress = 0.0f;
	m_flFirstProgressUpdate = -0.1f;
	m_flLastProgressUpdate = 0.0f;

	m_pProgressBar = new ProgressBar(this, NULL);
	m_pProgressBar->SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
ProgressBox::~ProgressBox()
{
}

//-----------------------------------------------------------------------------
// Purpose: resize the message label
//-----------------------------------------------------------------------------
void ProgressBox::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int wide, tall;
	m_pMessageLabel->GetContentSize(wide, tall);
	SetSize(384, tall + 92);
	m_pMessageLabel->SetSize(344, tall);
}

//-----------------------------------------------------------------------------
// Purpose: Put the message box into a modal state
//			Does not suspend execution - use addActionSignal to get return value
//-----------------------------------------------------------------------------
void ProgressBox::DoModal(Frame *pFrameOver)
{
    ShowWindow(pFrameOver);
	input()->SetAppModalSurface(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: Activates the window
//-----------------------------------------------------------------------------
void ProgressBox::ShowWindow(Frame *pFrameOver)
{
	// move to the middle of the screen
	// get the screen size
	int wide, tall;
	// get our dialog size
	GetSize(wide, tall);

	if (pFrameOver)
	{
		int frameX, frameY;
		int frameWide, frameTall;
		pFrameOver->GetPos(frameX, frameY);
		pFrameOver->GetSize(frameWide, frameTall);

		SetPos((frameWide - wide) / 2 + frameX, (frameTall - tall) / 2 + frameY);
	}
	else
	{
		int swide, stall;
		surface()->GetScreenSize(swide, stall);
		// put the dialog in the middle of the screen
		SetPos((swide - wide) / 2, (stall - tall) / 2);
	}

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Put the text and OK buttons in correct place
//-----------------------------------------------------------------------------
void ProgressBox::PerformLayout()
{	
	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	wide += x;
	tall += y;

	int leftEdge = x + 16;
	m_pMessageLabel->SetPos(leftEdge, y + 12);
	m_pProgressBar->SetPos(leftEdge, y + 14 + m_pMessageLabel->GetTall() + 2);
	m_pProgressBar->SetSize(wide - 44, 24);

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: updates progress bar, range [0, 1]
//-----------------------------------------------------------------------------
void ProgressBox::SetProgress(float progress)
{
	assert(progress >= 0.0f && progress <= 1.0f);
	m_pProgressBar->SetProgress(progress);
	m_pProgressBar->SetVisible(true);

	// store off timings for calculating time remaining
	if (m_flFirstProgressUpdate < 0.0f)
	{
		m_flFirstProgressUpdate = (float)system()->GetFrameTime();
	}
	m_flCurrentProgress = progress;
	m_flLastProgressUpdate = (float)system()->GetFrameTime();

}

//-----------------------------------------------------------------------------
// Purpose: called every render
//-----------------------------------------------------------------------------
void ProgressBox::OnThink()
{
	// calculate the progress made
	if (m_flFirstProgressUpdate >= 0.0f && m_wcsInfoString[0])
	{
		wchar_t timeRemaining[128];
		if (ProgressBar::ConstructTimeRemainingString(timeRemaining, sizeof(timeRemaining), m_flFirstProgressUpdate, (float)system()->GetFrameTime(), m_flCurrentProgress, m_flLastProgressUpdate, true))
		{
			wchar_t unicode[256];
			localize()->ConstructString(unicode, sizeof(unicode), m_wcsInfoString, 1, timeRemaining);
			m_pMessageLabel->SetText(unicode);
		}
		else
		{
			m_pMessageLabel->SetText(m_wszUnknownTimeString);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Deletes self when closed
//-----------------------------------------------------------------------------
void ProgressBox::OnClose()
{
	BaseClass::OnClose();
	// modal surface is released on deletion
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ProgressBox::OnShutdownRequest()
{
	// Shutdown the dialog
	PostMessage(this, new KeyValues("Close"));
}

//-----------------------------------------------------------------------------
// Purpose: Toggles visibility of the close box.
//-----------------------------------------------------------------------------
void ProgressBox::DisableCloseButton(bool state)
{
	BaseClass::SetCloseButtonVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t ProgressBox::m_MessageMap[] =
{
	MAP_MESSAGE( ProgressBox, "ShutdownRequest", OnShutdownRequest ),
};
IMPLEMENT_PANELMAP(ProgressBox, BaseClass);
