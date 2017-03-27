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

#ifndef IMAGEPANEL_H
#define IMAGEPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

namespace vgui
{

class IImage;

//-----------------------------------------------------------------------------
// Purpose: Panel that holds a single image
//-----------------------------------------------------------------------------
class ImagePanel : public Panel
{
public:
	ImagePanel(Panel *parent, const char *name);
	~ImagePanel();

	virtual void SetImage(IImage *image);
	virtual IImage *GetImage();

protected:
	virtual void PaintBackground();
	virtual void GetSettings(KeyValues *outResourceData);
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual const char *GetDescription();
	virtual void OnSizeChanged(int newWide, int newTall);

private:
	DECLARE_PANELMAP();
	IImage *m_pImage;
	char *m_pszImageName;
	char *m_pszColorName;
	bool m_bScaleImage;
	Color m_FillColor;
	typedef vgui::Panel BaseClass;
};

} // namespace vgui

#endif // IMAGEPANEL_H
