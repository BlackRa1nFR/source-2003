//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CVARSLIDER_H
#define CVARSLIDER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Slider.h>

class CCvarSlider : public vgui::Slider
{
public:

	CCvarSlider( vgui::Panel *parent, const char *panelName, char const *caption,
		float minValue, float maxValue, char const *cvarname, bool bAllowOutOfRange=false );
	~CCvarSlider();

	virtual void	Paint();

	void			ApplyChanges();
	float			GetSliderValue();
    void            SetSliderValue(float fValue);
    void            Reset();
    bool            HasBeenModified();

private:
	typedef vgui::Slider BaseClass;

	DECLARE_PANELMAP();
	
	void			OnSliderMoved();

    bool        m_bAllowOutOfRange;
    bool        m_bModifiedOnce;
    float       m_fStartValue;
    int         m_iStartValue;
    int         m_iLastSliderValue;
    float       m_fCurrentValue;
	char		m_szCvarName[ 64 ];
};

#endif // CVARSLIDER_H
