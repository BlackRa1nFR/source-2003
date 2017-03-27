//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SPECTATORGUI_H
#define SPECTATORGUI_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include "ISpectatorInterface.h"

class KeyValues;

namespace vgui
{
	class TextEntry;
	class Button;
	class Panel;
	class ImagePanel;
	class ComboBox;
}

#define BLACK_BAR_COLOR	Color(0, 0, 0, 196)

class CBottomBar;
class IBaseFileSystem;

//-----------------------------------------------------------------------------
// Purpose: Spectator UI
//-----------------------------------------------------------------------------
class CSpectatorGUI : public vgui::Frame, public ISpectatorInterface
{
private:
	typedef vgui::Frame BaseClass;

public:
	CSpectatorGUI(vgui::Panel *parent);
	virtual ~CSpectatorGUI();

	virtual void Activate();
	virtual void Deactivate();

	virtual void SetLogoImage(const char *image);

	virtual void Update();
	virtual void UpdateSpectatorPlayerList();
	virtual void UpdateTimer();

	virtual void ShowGUI();
	virtual void HideGUI();
	virtual bool IsGUIVisible();
	
	// both vgui::Frame and ISpectatorInterface define these, so explicitly define them here as passthroughs to vgui
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetVisible(bool state) { BaseClass::SetVisible(state); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

	virtual int GetTopBarHeight() { return m_pTopBar->GetTall(); }
	virtual int GetBottomBarHeight() { return m_pBottomBarBlank->GetTall(); }
protected:

	void SetLabelText(const char *textEntryName, const char *text);
	void SetLabelText(const char *textEntryName, wchar_t *text);
	void MoveLabelToFront(const char *textEntryName);

private:	
	enum { INSET_OFFSET = 2 } ; 

	// vgui overrides
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnCommand( const char *command );

	vgui::Panel *m_pTopBar;
	CBottomBar *m_pBottomBar;
	vgui::Panel *m_pBottomBarBlank;

	vgui::ImagePanel *m_pBannerImage;
	vgui::Label *m_pPlayerLabel;

	vgui::KeyCode m_iDuckKey;
	bool m_bHelpShown;
	bool m_bInsetVisible;
};


#endif // SPECTATORGUI_H
