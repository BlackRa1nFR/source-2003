//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MENU_H
#define MENU_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlVector.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudMenu : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudMenu, vgui::Panel );
public:
	CHudMenu( const char *pElementName );
	void Init( void );
	void VidInit( void );
	void Reset( void );
	virtual bool ShouldDraw( void );
	void MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf );

	bool IsMenuOpen( void );
	void SelectMenuItem( int menu_item );

private:
	virtual void OnThink();
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
private:
	void		ProcessText( void );

	void PaintString( const char *text, int textlen, vgui::HFont& font, int x, int y );

	struct ProcessedLine
	{
		int	menuitem; // -1 for just text
		int startchar;
		int length;
		int pixels;
		int height;
	};

	CUtlVector< ProcessedLine >	m_Processed;

	int				m_nMaxPixels;
	int				m_nHeight;

	int				m_fMenuDisplayed;
	int				m_bitsValidSlots;
	float			m_flShutoffTime;
	int				m_fWaitingForMore;
	int				m_nSelectedItem;

	CPanelAnimationVar( float, m_flOpenCloseTime, "OpenCloseTime", "1" );

	CPanelAnimationVar( float, m_flBlur, "Blur", "0" );
	CPanelAnimationVar( float, m_flTextScan, "TextScane", "1" );

	CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "255.0" );
	CPanelAnimationVar( float, m_flSelectionAlphaOverride, "SelectionAlpha", "255.0" );

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "MenuTextFont" );
	CPanelAnimationVar( vgui::HFont, m_hItemFont, "ItemFont", "MenuItemFont" );
	CPanelAnimationVar( vgui::HFont, m_hItemFontPulsing, "ItemFontPulsing", "MenuItemFontPulsing" );

	CPanelAnimationVar( Color, m_MenuColor, "MenuColor", "MenuColor" );
	CPanelAnimationVar( Color, m_ItemColor, "MenuItemColor", "ItemColor" );
	CPanelAnimationVar( Color, m_BoxColor, "MenuBoxColor", "MenuBoxBg" );
};

#endif // MENU_H
