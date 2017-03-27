
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


using namespace vgui;


class SampleCheckButtons: public DemoPage
{
	public:
		SampleCheckButtons(Panel *parent, const char *name);
		~SampleCheckButtons();
	
	private:

};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
SampleCheckButtons::SampleCheckButtons(Panel *parent, const char *name) : DemoPage(parent, name)
{
	LoadControlSettings("Demo/SampleCheckButtons.res");
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
SampleCheckButtons::~SampleCheckButtons()
{
}




Panel* SampleCheckButtons_Create(Panel *parent)
{
	return new SampleCheckButtons(parent, "Check buttons");
}


