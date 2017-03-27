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
#include "ifpspanel.h"
#include <vgui_controls/Panel.h>
#include "view.h"
#include <vgui/IVGui.h>
#include "vguimatsurface/imatsystemsurface.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/IPanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar cl_showfps( "cl_showfps", "0", 0, "Draw fps meter at top of screen (1 = fps, 2 = smooth fps)" );
static ConVar cl_showpos( "cl_showpos", "0", 0, "Draw current position at top of screen" );

//-----------------------------------------------------------------------------
// Purpose: Framerate indicator panel
//-----------------------------------------------------------------------------
class CFPSPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
public:
	CFPSPanel( vgui::VPANEL parent );
	virtual			~CFPSPanel( void );

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void	Paint();
	virtual void	OnTick( void );

	virtual bool	ShouldDraw( void );

private:
	vgui::HFont		m_hFont;
};

#define FPS_PANEL_WIDTH 300

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CFPSPanel::CFPSPanel( vgui::VPANEL parent ) : BaseClass( NULL, "CFPSPanel" )
{
	SetParent( parent );
	int wide, tall;
	vgui::ipanel()->GetSize(parent, wide, tall );
	SetPos( wide - FPS_PANEL_WIDTH, 0 );
	SetVisible( false );
	SetCursor( null );

	m_hFont = 0;

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );

	SetSize( FPS_PANEL_WIDTH, vgui::surface()->GetFontTall( m_hFont ) + 10 );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 250 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFPSPanel::~CFPSPanel( void )
{
}

void CFPSPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont( "DefaultFixed" );
	assert( m_hFont );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFPSPanel::OnTick( void )
{
	SetVisible( ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFPSPanel::ShouldDraw( void )
{
	if ( ( !cl_showfps.GetInt() || ( gpGlobals->absoluteframetime <= 0 ) ) &&
		 ( !cl_showpos.GetInt() ) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void CFPSPanel::Paint() 
{
	int i = 0;
	int x = 2;

	if ( cl_showfps.GetInt() && gpGlobals->absoluteframetime > 0.0 )
	{
		i++;


		static float AverageFPS = -1;
		static int high = -1;
		static int low = -1;
		if( cl_showfps.GetInt() == 2 )
		{

			const float NewWeight  = 0.1f;
			float NewFrame = 1.f / gpGlobals->absoluteframetime;

			if( AverageFPS < 0.f )
			{
				AverageFPS = NewFrame;
				high = (int)AverageFPS;
				low = (int)AverageFPS;
			} else
			{				
				AverageFPS *= ( 1.f - NewWeight ) ;
				AverageFPS += ( ( NewFrame ) * NewWeight );
			}
		
			int NewFrameInt = (int)NewFrame;
			if( NewFrameInt < low ) low = NewFrameInt;
			if( NewFrameInt > high ) high = NewFrameInt;	

			
			g_pMatSystemSurface->DrawColoredText( m_hFont, x, 2, 255, 255, 255, 255, "%3i fps (%3i, %3i) on %s", (int)AverageFPS, low, high, engine->GetLevelName() );

		} else
		{
			AverageFPS = -1;
			g_pMatSystemSurface->DrawColoredText( m_hFont, x, 2, 255, 255, 255, 255, "%3i fps on %s", 
				(int)(1.f / gpGlobals->absoluteframetime), engine->GetLevelName() );
		}
	}

	if ( cl_showpos.GetInt() )
	{
		g_pMatSystemSurface->DrawColoredText( m_hFont, x, 2 + i * ( vgui::surface()->GetFontTall( m_hFont ) + 2 ), 
			255, 255, 255, 255, 
			"pos:  %.2f %.2f %.2f", 
			MainViewOrigin().x, MainViewOrigin().y, MainViewOrigin().z );

		i++;

		Vector vel( 0, 0, 0 );
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
		if ( player )
		{
			vel = player->GetLocalVelocity();
		}

		g_pMatSystemSurface->DrawColoredText( m_hFont, x, 2 + i * ( vgui::surface()->GetFontTall( m_hFont ) + 2 ), 
			255, 255, 255, 255, 
			"vel:  %.2f", 
			vel.Length() );
	}
}

class CFPS : public IFPSPanel
{
private:
	CFPSPanel *fpsPanel;
public:
	CFPS( void )
	{
		fpsPanel = NULL;
	}

	void Create( vgui::VPANEL parent )
	{
		fpsPanel = new CFPSPanel( parent );
	}

	void Destroy( void )
	{
		if ( fpsPanel )
		{
			fpsPanel->SetParent( (vgui::Panel *)NULL );
			delete fpsPanel;
		}
	}
};

static CFPS g_FPSPanel;
IFPSPanel *fps = ( IFPSPanel * )&g_FPSPanel;