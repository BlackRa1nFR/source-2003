//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "DemoPage.h"

#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Controls.h>
#include <VGUI_ToggleButton.h>

using namespace vgui;


class TooltipsDemo: public DemoPage
{
	public:
		TooltipsDemo(Panel *parent, const char *name);
		~TooltipsDemo();
	
	private:

};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TooltipsDemo::TooltipsDemo(Panel *parent, const char *name) : DemoPage(parent, name)
{
	ToggleButton *pButton = new ToggleButton (this, "RadioDesc5", "");
	pButton->SetTooltipFormatToSingleLine();

	LoadControlSettings("Demo/SampleToolTips.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
TooltipsDemo::~TooltipsDemo()
{
}




Panel* TooltipsDemo_Create(Panel *parent)
{
	return new TooltipsDemo(parent, "TooltipsDemo");
}


