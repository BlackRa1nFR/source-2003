//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BUYMENU_H
#define BUYMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/WizardPanel.h>

class CBuyMainMenu;
class CBuySubMenu;
namespace vgui
{
	class Panel;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the class menu
//-----------------------------------------------------------------------------
class CBuyMenu : public vgui::WizardPanel
{
private:
	typedef vgui::WizardPanel BaseClass;

public:
	CBuyMenu(vgui::Panel *parent);
	~CBuyMenu();

	void ActivateBuyMenu();
	void ActivateEquipmentMenu();

	virtual void OnClose();

private:

	CBuySubMenu *_mainMenu;
	CBuySubMenu *_equipMenu;
};


#endif // BUYMENU_H
