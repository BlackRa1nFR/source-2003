//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef BUDDYBUTTON_H
#define BUDDYBUTTON_H
#pragma once

#include <VGUI_MenuButton.h>
#include <VGUI_Color.h>

class KeyValues;

namespace vgui
{
class TextImage;
class Menu;
enum MouseCode;
};

class CBuddy;

//-----------------------------------------------------------------------------
// Purpose: BuddyButton
//-----------------------------------------------------------------------------
class CBuddyButton : public vgui::MenuButton
{
public:
	CBuddyButton(vgui::Panel *parent, int buddyID);
	~CBuddyButton();

	virtual unsigned int GetBuddyID() { return m_iBuddyID; }
	virtual int GetBuddyStatus() { return m_iBuddyStatus; }
	virtual bool HasMessage() { return m_bHasMessage; }

	virtual void RefreshBuddyStatus();

protected:
	// overrides
	virtual void OnThink();
	virtual void PerformLayout();
	virtual void PaintBackground();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual vgui::IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus);

	virtual void OnShowMenu(vgui::Menu *menu);

	virtual void OnCommand(const char *command);

	virtual void OnMouseDoublePressed(vgui::MouseCode code);

	// drag-drop support
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void OnMouseCaptureLost();
	virtual void OnCursorMoved(int x, int y);

	// methods
	virtual void recalculateMenuItems();

	// returns a pointer to the right-click context menu for this button
	virtual vgui::Menu *GetContextMenu();

	// sets the current text
	virtual void setNameText(const char *text);
	virtual void setStatusText(const char *text);

private:
	vgui::Menu *m_pMenu;
	CBuddy *m_pBuddy;
	const char *m_pDefaultCommand;

	int m_iBuddyID;
	int m_iBuddyStatus;
	int m_iImageOffset;

	int m_iGameRefreshTime;		// time (in millis) at which we should refresh the game list

	// user states
	bool m_bHasMessage;

	// drag/drop support
	bool m_bDragging;

	class vgui::KeyValues *m_pBuddyData;

	vgui::IImage *_statusImage;
	vgui::TextImage *_nameText;
	vgui::TextImage *_statusText;

	vgui::Color m_FgColor1;
	vgui::Color m_FgColor2;
	vgui::Color m_BgColor;
	vgui::Color m_ArmedFgColor1;
	vgui::Color m_ArmedFgColor2;
	vgui::Color m_ArmedBgColor;
	vgui::Color m_GameColor;
	vgui::Color m_GameNameColor;

	typedef vgui::MenuButton BaseClass;
};

#endif // BUDDYBUTTON_H
