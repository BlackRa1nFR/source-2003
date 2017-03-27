//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <VGUI_PropertyPage.h>
#include "filesystem.h"
#include <VGUI_Controls.h>

#include <VGUI_IVGui.h> // for dprinf statements

using namespace vgui;

//-----------------------------------------------------------------------------
// This class contains the basic layout for every demo panel.
//-----------------------------------------------------------------------------
class DemoPage: public PropertyPage
{
	public:
		DemoPage(Panel *parent, const char *name);
		~DemoPage();
		
	private:
};