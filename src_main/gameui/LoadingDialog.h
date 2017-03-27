//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LOADINGDIALOG_H
#define LOADINGDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>

//-----------------------------------------------------------------------------
// Purpose: Dialog for displaying level loading status
//-----------------------------------------------------------------------------
class CLoadingDialog : public vgui::Frame
{
public:
	CLoadingDialog(vgui::Panel *parent);
	~CLoadingDialog();

	void DisplayProgressBar(const char *resourceType, const char *resourceName);
	void DisplayError(const char *failureReason, const char *extendedReason = NULL);
	void SetProgressPoint(int progressPoint);
	void SetProgressRange(int min, int max);
	void SetStatusText(const char *statusText);
	void SetSecondaryProgress(float progress);
	void SetSecondaryProgressText(const char *statusText);

protected:
	virtual void OnCommand(const char *command);
	virtual void PerformLayout();
	virtual void OnThink();
	virtual void OnClose();
	virtual void OnKeyCodePressed(vgui::KeyCode code);

private:
	vgui::ProgressBar *m_pProgress;
	vgui::ProgressBar *m_pProgress2;
	vgui::Label *m_pInfoLabel;
	vgui::Label *m_pTimeRemainingLabel;
	vgui::Button *m_pCancelButton;
	vgui::HTML *m_pHTML;

	int m_iRangeMin, m_iRangeMax;
	bool m_bShowingSecondaryProgress;
	float m_flSecondaryProgress;
	float m_flLastSecondaryProgressUpdateTime;
	float m_flSecondaryProgressStartTime;

	typedef vgui::Frame BaseClass;
};

// singleton accessor
CLoadingDialog *LoadingDialog();


#endif // LOADINGDIALOG_H
