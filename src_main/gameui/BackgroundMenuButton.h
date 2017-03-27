//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BACKGROUNDMENUBUTTON_H
#define BACKGROUNDMENUBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Button.h>

//-----------------------------------------------------------------------------
// Purpose: Baseclass for the left and right ingame menus that lay on the background
//-----------------------------------------------------------------------------
class CBackgroundMenuButton : public vgui::Button
{
public:
	CBackgroundMenuButton(vgui::Panel *parent, const char *name);
	~CBackgroundMenuButton();

	virtual void OnCommand(const char *command);

	void SetImages(vgui::IImage *image, vgui::IImage *mouseoverImage)
	{
		m_pImage = image;
		m_pMouseOverImage = mouseoverImage;
		SetArmed(IsArmed());
	}

	virtual void SetArmed(bool state)
	{
		if (state)
		{
			SetImageAtIndex(0, m_pMouseOverImage, 0);
		}
		else
		{
			SetImageAtIndex(0, m_pImage, 0);
		}

		BaseClass::SetArmed(state);
	}

protected:
	vgui::Menu *RecursiveLoadGameMenu(KeyValues *datafile);
	vgui::Menu *m_pMenu;

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	vgui::IImage *m_pImage, *m_pMouseOverImage;
	typedef vgui::Button BaseClass;
};


#endif // BACKGROUNDMENUBUTTON_H