//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CL_DEMOUIPANEL_H
#define CL_DEMOUIPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>


namespace vgui
{
class Button;
class CheckButton;
class Label;
class ProgressBar;
class FileOpenDialog;
class Slider;
};

class CDemoEditorPanel;
class CDemoSmootherPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDemoUIPanel : public vgui::Frame
{
	typedef vgui::Frame BaseClass;

public:
	CDemoUIPanel( vgui::Panel *parent );
	~CDemoUIPanel();

	virtual void OnTick();

	// Command issued
	virtual void OnCommand(const char *command);

	virtual void	OnVDMChanged( void );

	virtual float	GetPlaybackScale();

	virtual bool		OverrideView( democmdinfo_t& info, int frame, float elapsed );
	virtual void		DrawDebuggingInfo( int frame, float elapsed );
	DECLARE_PANELMAP();
protected:
	virtual void OnClose();

	void		HandleInput( bool active );

	bool			IsHoldingFastForward();

	void		OnFileSelected( char const *fullpath );

	void		OnEdit();
	void		OnSmooth();
	void		OnLoad();
	void		OnGotoFrame();

	vgui::Label  *m_pCurrentDemo;
	vgui::Button *m_pPlayPauseResume;
	vgui::Button *m_pStop;
	vgui::Button *m_pLoad;
	vgui::Button *m_pEdit;
	vgui::Button *m_pSmooth;
	vgui::Button	*m_pDriveCamera;

	vgui::CheckButton *m_pTimeDemo;
	vgui::Button	*m_pFastForward;
	vgui::Button	*m_pAdvanceFrame;

	vgui::ProgressBar *m_pProgress;
	vgui::Label	*m_pProgressLabelPercent;
	vgui::Label	*m_pProgressLabelFrame;
	vgui::Label	*m_pProgressLabelTime;

	vgui::Slider *m_pSpeedScale;
	vgui::Label	*m_pSpeedScaleLabel;

	vgui::DHANDLE< CDemoEditorPanel > m_hDemoEditor;
	vgui::DHANDLE< CDemoSmootherPanel > m_hDemoSmoother;
	vgui::DHANDLE< vgui::FileOpenDialog > m_hFileOpenDialog;

	vgui::Button	*m_pGo;
	vgui::TextEntry *m_pGotoFrame;

	bool		m_bInputActive;
	int			m_nOldCursor[2];
	Vector		m_ViewOrigin;
	QAngle		m_ViewAngles;
	bool		m_bGrabNewViewSetup;
};

#endif // CL_DEMOUIPANEL_H
