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

#ifndef PROGRESSBOX_H
#define PROGRESSBOX_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Frame.h>

// prevent windows macros from messing with the class
#ifdef ProgressBox
#undef ProgressBox
#endif

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Popup discardable message box
//-----------------------------------------------------------------------------
class ProgressBox : public Frame
{
public:
	// title - Text to be displayed in the title bar of the window
	// text - Text message in the message box
	// parent - parent panel of the message box, by default it has no parent.
	ProgressBox(const char *title, const char *text, const char *pszUnknownTimeString, Panel *parent = NULL);
	ProgressBox(const wchar_t *wszTitle, const wchar_t *wszText, const wchar_t *wszUnknownTimeString, Panel *parent = NULL);
	~ProgressBox();

	// Put the message box into a modal state
	virtual void DoModal(Frame *pFrameOver = NULL);

	// make the message box appear and in a modeless state
	virtual void ShowWindow(Frame *pFrameOver = NULL);

	// updates progress bar, range [0, 1]
	virtual void SetProgress(float progress);

	// toggles visibility of the close box.
	virtual void DisableCloseButton(bool state);

protected:
	virtual void PerformLayout();
	virtual void OnClose();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnThink();

protected:
	Label *m_pMessageLabel;
	ProgressBar *m_pProgressBar;

private:
	virtual void OnShutdownRequest();
	void Init();

	wchar_t m_wcsInfoString[128];
	wchar_t m_wszUnknownTimeString[128];

	float m_flFirstProgressUpdate;
	float m_flLastProgressUpdate;
	float m_flCurrentProgress;

	DECLARE_PANELMAP();

	typedef Frame BaseClass;
};

} // namespace vgui


#endif // PROGRESSBOX_H
