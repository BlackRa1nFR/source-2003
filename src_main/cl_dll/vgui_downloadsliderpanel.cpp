//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "idownloadslider.h"
#include "ivrenderview.h"
#include <vgui/IVgui.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Shows download progress
//-----------------------------------------------------------------------------
class CDownloadSliderPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
public:
					CDownloadSliderPanel( vgui::VPANEL parent );
	virtual			~CDownloadSliderPanel( void );

	virtual void	Paint();
	virtual void	OnTick( void );

	virtual bool	ShouldDraw( void );

	virtual void	DrawDownloadText(void);

private:
	ConVar			*scr_downloading;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CDownloadSliderPanel::CDownloadSliderPanel( vgui::VPANEL parent ) :
	BaseClass( NULL , "CDownloadSliderPanel" )
{
	SetParent( parent );
	SetSize( ScreenWidth(), ScreenHeight() );
	SetPos( 0, 0 );
	SetVisible( false );
	SetCursor( null );

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );

	scr_downloading = (ConVar *)cvar->FindVar( "scr_downloading" );
	assert( scr_downloading );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 250 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDownloadSliderPanel::~CDownloadSliderPanel( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDownloadSliderPanel::OnTick( void )
{
	SetVisible( ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDownloadSliderPanel::ShouldDraw( void )
{
	if ( !scr_downloading )
		return false;

	if ( scr_downloading->GetInt() < 0 )
		return false;

	if ( render->IsPlayingDemo() || !engine->IsConnected() )
	{
		scr_downloading->SetValue( -1 );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDownloadSliderPanel::DrawDownloadText(void)
{
	// FIXME: Reenable using ftp?
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CDownloadSliderPanel::Paint() 
{
	int nPercent;
	int w, h, x, y;
	vrect_t rcFill;

	nPercent = scr_downloading->GetInt();
	nPercent = max(0,nPercent);
	nPercent = min(100,nPercent);

	if ( engine->IsInGame() )
	{
		if (ScreenWidth() <= 100 )
			w = ScreenWidth()-10;
		else
			w = 100;

		if (ScreenHeight() <= 6)
			h = ScreenHeight() -2;
		else
			h = 6;

		x = 3;  // Lower left corner
		y = ScreenHeight() - 3 - h;
	}
	else
	{
		if (ScreenWidth() <= 250)
			w = ScreenWidth()-2;
		else
			w = 250;

		if (ScreenHeight() <= 10)
			h = ScreenHeight() -2;
		else
			h = 10;

		x = (ScreenWidth() - w)/2 + 1;
		y = ScreenHeight() -2 - 100;
	}

	if (y <= 2)
		y = 2;

	rcFill.x      = x;
	rcFill.y      = y;
	rcFill.width  = w;
	rcFill.height = h;
	
	vgui::surface()->DrawSetColor( 63, 63, 63, 255 );
	vgui::surface()->DrawFilledRect( rcFill.x, rcFill.y, rcFill.x + rcFill.width, rcFill.y + rcFill.height );

	// now draw bar;
	rcFill.x += 2;
	rcFill.y += 2;
	rcFill.height -= 4;
	rcFill.width = (int)(( (float)nPercent/100.0f ) * (w-4) + 0.5f);

	vgui::surface()->DrawSetColor( 180, 170, 160, 255 );
	vgui::surface()->DrawFilledRect( rcFill.x, rcFill.y, rcFill.x + rcFill.width, rcFill.y + rcFill.height );

	rcFill.x += rcFill.width;
	rcFill.width = ( w - 4 ) - (int)(( (float)nPercent/100.0f ) * (w-4) + 0.5f);

	vgui::surface()->DrawSetColor( 0, 0, 0, 255 );
	vgui::surface()->DrawFilledRect( rcFill.x, rcFill.y, rcFill.x + rcFill.width, rcFill.y + rcFill.height );
	
	DrawDownloadText();
}

class CDownloadSlider : public IDownloadSlider
{
private:
	CDownloadSliderPanel *downloadSliderPanel;
public:
	CDownloadSlider( void )
	{
		downloadSliderPanel = NULL;
	}

	void Create( vgui::VPANEL parent )
	{
		downloadSliderPanel = new CDownloadSliderPanel( parent );
	}

	void Destroy( void )
	{
		if ( downloadSliderPanel )
		{
			downloadSliderPanel->SetParent( (vgui::Panel *)NULL );
			delete downloadSliderPanel;
		}
	}
};

static CDownloadSlider g_DownloadSlider;
IDownloadSlider *downloadslider = ( IDownloadSlider * )&g_DownloadSlider;
