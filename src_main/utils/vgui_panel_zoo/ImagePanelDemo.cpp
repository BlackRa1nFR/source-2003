//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "DemoPage.h"

#include <VGUI_IVGui.h>

#include <VGUI_Controls.h>
#include <VGUI_IScheme.h>
#include <VGUI_ImagePanel.h>
#include <VGUI_IImage.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// An ImagePanel is a panel class that handles drawing of Images and gives
// them all kinds of panel features.
//-----------------------------------------------------------------------------
class ImagePanelDemo: public DemoPage
{
	typedef DemoPage BaseClass;

public:
	ImagePanelDemo(Panel *parent, const char *name);
	~ImagePanelDemo();

	virtual void ApplySchemeSettings(IScheme *pScheme);
	
private:
	IImage *m_pImage;
	ImagePanel *m_pImagePanel;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ImagePanelDemo::ImagePanelDemo(Panel *parent, const char *name) : DemoPage(parent, name)
{
	// Create an Image Panel
	m_pImagePanel = new ImagePanel(this, "AnImagePanel");

	// Set the position
	m_pImagePanel->SetPos(100, 100);

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
ImagePanelDemo::~ImagePanelDemo()
{
}

//-----------------------------------------------------------------------------
// Scheme settings
//-----------------------------------------------------------------------------
void ImagePanelDemo::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	// get an image
	m_pImage = pScheme->GetImage("Resource/valve_logo");

	// now insert an image
	m_pImagePanel->SetImage(m_pImage);
}

Panel* ImagePanelDemo_Create(Panel *parent)
{
	return new ImagePanelDemo(parent, "ImagePanelDemo");
}

