//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "DemoPage.h"

#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>

#include <VGUI_Controls.h>
#include <VGUI_IScheme.h>
#include <VGUI_IImage.h>
#include <VGUI_TextImage.h>


using namespace vgui;

//-----------------------------------------------------------------------------
// A Label is a panel class to handle the display of images and text strings.
// Here we demonstrate a label that has multiple images and strings in it.
// This demo shows a label with multiple images in it.
//-----------------------------------------------------------------------------

class Label2Demo: public DemoPage
{
	public:
		Label2Demo(Panel *parent, const char *name);
		~Label2Demo();

		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		
	private:
		Label *m_pLabel;
		TextImage *m_pEndText;
		IImage *_statusImage;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
Label2Demo::Label2Demo(Panel *parent, const char *name) : DemoPage(parent, name)
{
	// Create a Label object that displayes the words "LabelText"
	m_pLabel = new Label(this, "Label2Demo", "Beginning Label Text");

	// Create a label holding the ending text
	m_pEndText = new TextImage("Ending Label Text", GetScheme());

	m_pLabel->SetSize( 240, 24 );

	// Set the label position
	m_pLabel->SetPos(100, 100);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
Label2Demo::~Label2Demo()
{
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Label2Demo::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Label's ApplySchemeSettings wipes out the images that were added
	// Re-add them.
	m_pLabel->InvalidateLayout(true);
	_statusImage = pScheme->GetImage("/friends/icon_busy");
	m_pLabel->AddImage(_statusImage, 0);
	m_pLabel->AddImage(m_pEndText, 0);
}


Panel* Label2Demo_Create(Panel *parent)
{
	return new Label2Demo(parent, "Label2Demo");
}


