//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OPTIONS_SUB_VOICE_H
#define OPTIONS_SUB_VOICE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

typedef struct IVoiceTweak_s IVoiceTweak;

namespace vgui
{
	class CheckButton;
	class Slider;
}

class CCvarSlider;
class CCvarToggleCheckButton;
//-----------------------------------------------------------------------------
// Purpose: Voice Details, Part of OptionsDialog
//-----------------------------------------------------------------------------
class COptionsSubVoice : public vgui::PropertyPage
{
	typedef vgui::PropertyPage BaseClass;

public:
	COptionsSubVoice(vgui::Panel *parent);
	~COptionsSubVoice();

	virtual void OnResetData();
	virtual void OnApplyChanges();

	DECLARE_PANELMAP();

protected:
	virtual void OnThink();							// called every frame before painting, but only if panel is visible

private:
	virtual void	OnPageHide();
    virtual void    OnCommand( const char *command );

    void            OnSliderMoved( int position );
    void            OnCheckButtonChecked( int state );
    void            OnControlModified();

    void            StartTestMicrophone();
    void            EndTestMicrophone();

    void            UseCurrentVoiceParameters();
    void            ResetVoiceParameters();

	typedef vgui::PropertyPage BaseClass;

	IVoiceTweak				*m_pVoiceTweak;		// Engine voice tweak API.
	vgui::CheckButton		*m_pMicBoost;

    vgui::ImagePanel        *m_pMicMeter;
    vgui::ImagePanel        *m_pMicMeter2;
    vgui::Button            *m_pTestMicrophoneButton;
    vgui::Label             *m_pMicrophoneSliderLabel;
	vgui::Slider			*m_pMicrophoneVolume;
    vgui::Label             *m_pReceiveSliderLabel;
    CCvarSlider             *m_pReceiveVolume;
    CCvarToggleCheckButton  *m_pVoiceEnableCheckButton;

    int                     m_nMicVolumeValue;
    bool                    m_bMicBoostSelected;
    float                   m_fReceiveVolume;
    int                     m_nReceiveSliderValue;

    bool            m_bVoiceOn;
};



#endif // OPTIONS_SUB_VOICE_H