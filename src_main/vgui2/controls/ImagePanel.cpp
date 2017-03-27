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

#include <stdio.h>

#include <vgui/IBorder.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <KeyValues.h>

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Image.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ImagePanel::ImagePanel(Panel *parent, const char *name) : Panel(parent, name)
{
	m_pImage = NULL;
	m_pszImageName = NULL;
	m_pszColorName = NULL;
	m_bScaleImage = false;
	m_FillColor = Color(0, 0, 0, 0);

	SetImage(m_pImage);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ImagePanel::~ImagePanel()
{
	delete [] m_pszImageName;
	delete [] m_pszColorName;
}

//-----------------------------------------------------------------------------
// Purpose: handles size changing
//-----------------------------------------------------------------------------
void ImagePanel::OnSizeChanged(int newWide, int newTall)
{
	if (m_bScaleImage)
	{
		// scaling, force the image size to be our size
		m_pImage->SetSize(newWide, newTall);
	}
	BaseClass::OnSizeChanged(newWide, newTall);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ImagePanel::SetImage(IImage *image)
{
	m_pImage = image;

	if (m_pImage)
	{
		int wide, tall;
		if (m_bScaleImage)
		{
			// scaling, force the image size to be our size
			GetSize(wide, tall);
			m_pImage->SetSize(wide, tall);
		}
		else
		{
			// not scaling, so set our size to the image size
			m_pImage->GetSize(wide, tall);
			SetSize(wide, tall);
		}
	}

	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IImage *ImagePanel::GetImage()
{
	return m_pImage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ImagePanel::PaintBackground()
{
	if (m_pImage)
	{
		surface()->DrawSetColor(255, 255, 255, 255);
		m_pImage->SetPos(0, 0);
		m_pImage->Paint();
	}
	else if (m_FillColor[3] > 0)
	{
		// draw the specified fill color
		int wide, tall;
		GetSize(wide, tall);
		surface()->DrawSetColor(m_FillColor);
		surface()->DrawFilledRect(0, 0, wide, tall);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets control settings for editing
//-----------------------------------------------------------------------------
void ImagePanel::GetSettings(KeyValues *outResourceData)
{
	BaseClass::GetSettings(outResourceData);
	if (m_pszImageName)
	{
		outResourceData->SetString("image", m_pszImageName);
	}
	if (m_pszColorName)
	{
		outResourceData->SetString("fillcolor", m_pszColorName);
	}
	if (GetBorder())
	{
		outResourceData->SetString("border", GetBorder()->GetName());
	}

	outResourceData->SetInt("scaleImage", m_bScaleImage);
}

//-----------------------------------------------------------------------------
// Purpose: Applies designer settings from res file
//-----------------------------------------------------------------------------
void ImagePanel::ApplySettings(KeyValues *inResourceData)
{
	delete [] m_pszImageName;
	delete [] m_pszColorName;
	m_pszImageName = NULL;
	m_pszColorName = NULL;

	m_bScaleImage = inResourceData->GetInt("scaleImage", 0);
	const char *imageName = inResourceData->GetString("image", "");
	if (*imageName)
	{
		m_pszImageName = new char[strlen(imageName) + 1];
		strcpy(m_pszImageName, imageName);
		// add in the command
		SetImage(scheme()->GetImage(imageName, m_bScaleImage));
	}

	const char *pszFillColor = inResourceData->GetString("fillcolor", "");
	if (*pszFillColor)
	{
		int r = 0, g = 0, b = 0, a = 255;
		m_pszColorName = new char[strlen(pszFillColor) + 1];
		strcpy(m_pszColorName, pszFillColor);

		if (sscanf(pszFillColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			// it's a direct color
			m_FillColor = Color(r, g, b, a);
		}
		else
		{
			IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
			m_FillColor = pScheme->GetColor(pszFillColor, Color(0, 0, 0, 0));
		}
	}

	const char *pszBorder = inResourceData->GetString("border", "");
	if (*pszBorder)
	{
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		SetBorder(pScheme->GetBorder(pszBorder));
	}

	BaseClass::ApplySettings(inResourceData);
}


//-----------------------------------------------------------------------------
// Purpose: Describes editing details
//-----------------------------------------------------------------------------
const char *ImagePanel::GetDescription()
{
	static char buf[1024];
	_snprintf(buf, sizeof(buf), "%s, string image, string border, string fillcolor, bool scaleImage", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: message map
//-----------------------------------------------------------------------------
MessageMapItem_t ImagePanel::m_MessageMap[] =
{
	MAP_MESSAGE( ImagePanel, "layout", PerformLayout ),
};
IMPLEMENT_PANELMAP(ImagePanel, Panel);