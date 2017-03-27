//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>

#include "BasePanel.h"
#include "Taskbar.h"
#include "EngineInterface.h"

#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>

#include "GameConsole.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBasePanel::CBasePanel() : Panel(NULL, "BasePanel")
{
	m_eBackgroundState = BACKGROUND_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Notifies the task bar that a new top-level panel has been created
//-----------------------------------------------------------------------------
void CBasePanel::OnChildAdded(VPANEL child)
{
	if (g_pTaskbar)
	{
		g_pTaskbar->AddTask(child);
	}
}

//-----------------------------------------------------------------------------
// Purpose: paints the main background image
//-----------------------------------------------------------------------------
void CBasePanel::PaintBackground()
{
	const char *levelName = engine->GetLevelName();
	if (levelName && levelName[0])
	{
		// render filled background in game
		int swide, stall;
		surface()->GetScreenSize(swide, stall);
		surface()->DrawSetColor(0, 0, 0, 128);
		surface()->DrawFilledRect(0, 0, swide, stall);		
		return;	
	}

	switch (m_eBackgroundState)
	{
	case BACKGROUND_BLACK:
		{
			// if the loading dialog is visible, draw the background black
			int swide, stall;
			surface()->GetScreenSize(swide, stall);
			surface()->DrawSetColor(0, 0, 0, 255);
			surface()->DrawFilledRect(0, 0, swide, stall);		
		}
		break;

	case BACKGROUND_LOADING:
		DrawBackgroundImage();
		break;

	case BACKGROUND_DESKTOPIMAGE:
		DrawBackgroundImage();
		break;
		
	case BACKGROUND_LOADINGTRANSITION:
		{
		}
		break;

	case BACKGROUND_NONE:
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: loads background texture
//-----------------------------------------------------------------------------
void CBasePanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	// turn on hardware filtering if we're scaling the images
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	bool hardwareFilter = false; //(wide != 800);

	bimage_t &bimage = m_ImageID[0][0];
	bimage.imageID = surface()->CreateNewTextureID();

	char filename[512];
	sprintf(filename, "console/console_background" );
	surface()->DrawSetTextureFile(bimage.imageID, filename, hardwareFilter, false);
	surface()->DrawGetTextureSize(bimage.imageID, bimage.width, bimage.height);

/*
	for (int y = 0; y < BACKGROUND_ROWS; y++)
	{
		for (int x = 0; x < BACKGROUND_COLUMNS; x++)
		{
			bimage_t &bimage = m_ImageID[y][x];
			bimage.imageID = surface()->CreateNewTextureID();

			char filename[512];
			sprintf(filename, "resource/background/800_%d_%c_loading", y + 1, 'a' + x);
			surface()->DrawSetTextureFile(bimage.imageID, filename, hardwareFilter, false);
			surface()->DrawGetTextureSize(bimage.imageID, bimage.width, bimage.height);
		}
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: sets how the game background should render
//-----------------------------------------------------------------------------
void CBasePanel::SetBackgroundRenderState(EBackgroundState state)
{
	m_eBackgroundState = state;
}

//-----------------------------------------------------------------------------
// Purpose: draws the background desktop image
//-----------------------------------------------------------------------------
void CBasePanel::DrawBackgroundImage()
{
//	int xpos, ypos;
	int wide, tall;
	GetSize(wide, tall);

	// work out scaling factors
	int swide, stall;
	surface()->GetScreenSize(swide, stall);
	float xScale, yScale;
//	xScale = swide / 800.0f;
//	yScale = stall / 600.0f;
	xScale = 1.0f;
	yScale = 1.0f;
/*
	// iterate and draw all the background pieces
	ypos = 0;
	for (int y = 0; y < BACKGROUND_ROWS; y++)
	{
		xpos = 0;
		for (int x = 0; x < BACKGROUND_COLUMNS; x++)
		{
			bimage_t &bimage = m_ImageID[y][x];

			int dx = (int)ceil(xpos * xScale);
			int dy = (int)ceil(ypos * yScale);
			int dw = (int)ceil((xpos + bimage.width) * xScale);
			int dt = (int)ceil((ypos + bimage.height) * yScale);

			if (x == 0)
			{
				dx = 0;
			}
			if (y == 0)
			{
				dy = 0;
			}

  */
			bimage_t &bimage = m_ImageID[0][0];
			// draw the color image only if the mono image isn't yet fully opaque
			surface()->DrawSetColor(255, 255, 255, 255);
			surface()->DrawSetTexture(bimage.imageID);
			surface()->DrawTexturedRect(0, 0, wide, tall);

		/*	xpos += bimage.width;
		}
		ypos += m_ImageID[y][0].height;
	}
	*/
}
