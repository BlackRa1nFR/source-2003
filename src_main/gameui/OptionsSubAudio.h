//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OPTIONS_SUB_AUDIO_H
#define OPTIONS_SUB_AUDIO_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
class CLabeledCommandComboBox;
class CCvarSlider;
class CCvarToggleCheckButton;
//-----------------------------------------------------------------------------
// Purpose: Audio Details, Part of OptionsDialog
//-----------------------------------------------------------------------------
class COptionsSubAudio : public vgui::PropertyPage
{
public:
	COptionsSubAudio(vgui::Panel *parent);
	~COptionsSubAudio();

	virtual void OnResetData();
	virtual void OnApplyChanges();

private:
	DECLARE_PANELMAP();

	void OnControlModified();

	CCvarToggleCheckButton		*m_pEAXCheckButton;
	CCvarToggleCheckButton		*m_pA3DCheckButton;

	CCvarSlider					*m_pSFXSlider;
	CCvarSlider					*m_pHEVSlider;
	CCvarSlider					*m_pMP3Slider;
	CLabeledCommandComboBox		*m_pSoundQualityCombo;

	typedef vgui::PropertyPage BaseClass;
};



#endif // OPTIONS_SUB_AUDIO_H