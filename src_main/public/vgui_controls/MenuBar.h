//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MENUBAR_H
#define MENUBAR_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <UtlVector.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class MenuBar : public Panel
{
public:
	MenuBar(Panel *parent, const char *panelName);
	~MenuBar();

	virtual void AddButton(MenuButton *button); // add button to end of menu list
	virtual void AddMenu( const char *pButtonName, Menu *pMenu );

protected:
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void PerformLayout();
	virtual void Paint();
	void OnMenuClose();
	void OnCursorEnteredMenuButton(VPANEL menuButton);

	DECLARE_PANELMAP();

private:
	CUtlVector<MenuButton *> m_pMenuButtons;

	typedef Panel BaseClass;
};

} // namespace vgui

#endif // MENUBAR_H

