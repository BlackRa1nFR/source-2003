//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "DemoPage.h"

#include <VGUI_IVGui.h>
#include <VGUI_Controls.h>

#include <VGUI_Menu.h> 
#include <VGUI_MenuButton.h>
#include <VGUI_KeyValues.h>

using namespace vgui;


class MenuDemo: public DemoPage
{
	public:
		MenuDemo(Panel *parent, const char *name);
		~MenuDemo();
		void InitMenus();
		
		void OnMaggie();
		
	protected:
		// Menu that opens when button is pressed
		Menu *m_pMenu;

		// Button to trigger the menu
		MenuButton *m_pMenuButton;
		
	private:
		// explain this
		DECLARE_PANELMAP();
				
};