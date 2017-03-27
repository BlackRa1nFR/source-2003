//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "vgui_BudgetHistoryPanel.h"
#include <vgui/ISurface.h>
#include "tier0/vprof.h"
#include "materialsystem/imaterialsystem.h"

ConVar budget_show_history( "budget_show_history", "1", FCVAR_ARCHIVE, "turn history graph off and on. . good to turn off on low end" );

CBudgetHistoryPanel::CBudgetHistoryPanel( vgui::Panel *pParent, const char *pPanelName ) : 
	BaseClass( pParent, pPanelName )
{
	m_nSamplesPerGroup = 0;
	SetProportional( false );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
//	SetSizeable( false ); 
	SetVisible( true );
	SetPaintBackgroundEnabled( false );
	SetBgColor( Color( 0, 0, 0, 255 ) );
	
	// hide the system buttons
	SetTitleBarVisible( false );
}

CBudgetHistoryPanel::~CBudgetHistoryPanel()
{
}

void CBudgetHistoryPanel::Paint()
{
	if( m_nSamplesPerGroup == 0 )
	{
		// SetData hasn't been called yet.
		return;
	}
	if( !budget_show_history.GetBool() )
	{
		return;
	}
	materials->Flush();
	g_VProfCurrentProfile.Pause();
	int width, height;
	GetSize( width, height );
//	vgui::surface()->DrawSetColor( 0, 0, 0, 255 );
	// DrawFilledRect is panel relative
//	vgui::surface()->DrawFilledRect( 0, 0, width, height );

	int startID = m_nSampleOffset - width;
	while( startID < 0 )
	{
		startID += m_nSamplesPerGroup;
	}
	int endID = startID + width;
	int i;
	for( i = startID; i < endID; i++ )
	{
		int sampleOffset = i % m_nSamplesPerGroup;
		int j;
		double curHeight = 0.0;
		for( j = 0; j < m_nGroups; j++ )
		{
			int red, green, blue, alpha;
			g_VProfCurrentProfile.GetBudgetGroupColor( j, red, green, blue, alpha );
			vgui::surface()->DrawSetColor( red, green, blue, alpha );
			int left = i - startID;
			int right = left + 1;
			int bottom = ( curHeight - m_fRangeMin ) * ( 1.0f / ( m_fRangeMax - m_fRangeMin ) ) * height;
			curHeight += m_pData[sampleOffset + m_nSamplesPerGroup * j];
			int top = ( curHeight - m_fRangeMin ) * ( 1.0f / ( m_fRangeMax - m_fRangeMin ) ) * height;
			bottom = height - bottom - 1;
			top = height - top - 1;
			// DrawFilledRect is panel relative
			vgui::surface()->DrawFilledRect( left, top, right, bottom );
		}
	}
	DrawBudgetLine( 1000.0 / 60.0 );
	DrawBudgetLine( 1000.0 / 30.0 );
	DrawBudgetLine( 1000.0 / 20.0 );
	materials->Flush();
	g_VProfCurrentProfile.Resume();
}

// Only call this from Paint!!!!!
void CBudgetHistoryPanel::DrawBudgetLine( float val )
{
	int width, height;
	GetSize( width, height );
	double y = ( val - m_fRangeMin ) * ( 1.0f / ( m_fRangeMax - m_fRangeMin ) ) * height;
	int bottom = ( int )( height - y - 1 + .5 );
	int top = ( int )( height - y - 1 - .5 );
	vgui::surface()->DrawSetColor( 0, 0, 0, 255 );
	vgui::surface()->DrawFilledRect( 0, top-1, width, bottom+1 );
	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
	vgui::surface()->DrawFilledRect( 0, top, width, bottom );
}


void CBudgetHistoryPanel::SetData( double *pData, int nGroups, int nSamplesPerGroup, int nSampleOffset )
{
	m_pData = pData;
	m_nGroups = nGroups;
	m_nSamplesPerGroup = nSamplesPerGroup;
	m_nSampleOffset = nSampleOffset;
}

void CBudgetHistoryPanel::SetRange( float fMin, float fMax )
{
	m_fRangeMin = fMin;
	m_fRangeMax = fMax;
}
