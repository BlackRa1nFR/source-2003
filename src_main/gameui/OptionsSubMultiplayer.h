//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OPTIONSSUBMULTIPLAYER_H
#define OPTIONSSUBMULTIPLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

class CLabeledCommandComboBox;
class CBitmapImagePanel;

class CCvarToggleCheckButton;
class CCvarTextEntry;
class CCvarSlider;
class CMultiplayerAdvancedDialog;

//-----------------------------------------------------------------------------
// Purpose: multiplayer options property page
//-----------------------------------------------------------------------------
class COptionsSubMultiplayer : public vgui::PropertyPage
{
public:
	COptionsSubMultiplayer(vgui::Panel *parent);
	~COptionsSubMultiplayer();

protected:
	virtual void OnCommand(const char *command);

	// Called when page is loaded.  Data should be reloaded from document into controls.
	virtual void OnResetData();
	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	virtual void OnApplyChanges();

	DECLARE_PANELMAP();

private:
	typedef vgui::PropertyPage BaseClass;

	void InitModelList(CLabeledCommandComboBox *cb);
	void InitLogoList(CLabeledCommandComboBox *cb);

	void RemapModel();
	void RemapLogo();

	void OnTextChanged(vgui::Panel *panel);
	void OnSliderMoved(KeyValues *data);
	void OnApplyButtonEnable();

	void RemapPalette(char *filename, int topcolor, int bottomcolor);
	void RemapLogoPalette(char *filename, int r, int g, int b);

	void ColorForName(char const *pszColorName, int &r, int &g, int &b);

	CBitmapImagePanel *m_pModelImage;
	CLabeledCommandComboBox *m_pModelList;

	CBitmapImagePanel *m_pLogoImage;
	CLabeledCommandComboBox *m_pLogoList;
    char m_LogoName[32];

	CLabeledCommandComboBox	*m_pColorList;

    CCvarTextEntry *m_pNameTextEntry;
    CCvarSlider *m_pPrimaryColorSlider;
    CCvarSlider *m_pSecondaryColorSlider;
	CCvarToggleCheckButton *m_pHighQualityModelCheckBox;

	int	m_nTopColor;
	int	m_nBottomColor;

	int	m_nLogoR;
	int	m_nLogoG;
	int	m_nLogoB;

	vgui::DHANDLE<CMultiplayerAdvancedDialog> m_hMultiplayerAdvancedDialog;
};

#endif // OPTIONSSUBMULTIPLAYER_H
