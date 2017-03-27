//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Does anyone ever read this?
//
//=============================================================================

#include "cbase.h"
#include "vgui_BudgetPanel.h"
#include "vgui_controls/label.h"
#include "vgui_int.h"
#include "tier0/vprof.h"
#include "tier0/fasttimer.h"
#include "materialsystem/imaterialsystem.h"
#include "vguimatsurface/imatsystemsurface.h"
#include "vgui/ischeme.h"

#include "IVRenderView.h"
#include "engine/ivmodelinfo.h"

DECLARE_HUDELEMENT( CBudgetPanel );

void __CmdFunc_ShowBudgetPanel( void )
{
	CHudElement *pElement = gHUD.FindElement( "CBudgetPanel" );
	{
			((CBudgetPanel*)pElement)->UserCmd_ShowBudgetPanel( );
	}	
}
void __CmdFunc_HideBudgetPanel( void )
{
	CHudElement *pElement = gHUD.FindElement( "CBudgetPanel" );
	{
			((CBudgetPanel*)pElement)->UserCmd_HideBudgetPanel( );
	}	
}

CON_COMMAND( vprof_adddebuggroup1, "add a new budget group dynamically for debugging" )
{
	VPROF_BUDGET( "vprof_adddebuggroup1", "vprof_adddebuggroup1" );
}

static CBudgetPanel *g_pBudgetPanel = NULL;

CBudgetPanel *GetBudgetPanel( void )
{
	return g_pBudgetPanel;
}

static void PanelGeometryChangedCallBack( ConVar *var, const char *pOldString )
{
	GetBudgetPanel()->InvalidateLayout();
}

static ConVar budget_history_range( "budget_history_range", "15.0", FCVAR_ARCHIVE, "1/val seconds is the range to show in the budget history", PanelGeometryChangedCallBack );
static ConVar budget_bargraph_range( "budget_bargraph_range", "30.0", FCVAR_ARCHIVE, "1/val seconds is the range to show in the budget history", PanelGeometryChangedCallBack );
static ConVar budget_panel_x( "budget_panel_x", "0", FCVAR_ARCHIVE, "number of pixels from the left side of the game screen to draw the budget panel", PanelGeometryChangedCallBack );
static ConVar budget_panel_y( "budget_panel_y", "20", FCVAR_ARCHIVE, "number of pixels from the top side of the game screen to draw the budget panel", PanelGeometryChangedCallBack );
static ConVar budget_panel_width( "budget_panel_width", "512", FCVAR_ARCHIVE, "width in pixels of the budget panel", PanelGeometryChangedCallBack );
static ConVar budget_panel_height( "budget_panel_height", "384", FCVAR_ARCHIVE, "height in pixels of the budget panel", PanelGeometryChangedCallBack );
static ConVar budget_panel_bottom_of_history_fraction( "budget_panel_bottom_of_history_fraction", ".25", FCVAR_ARCHIVE, "number between 0 and 1", PanelGeometryChangedCallBack );

void CBudgetPanel::ClearTimesForAllGroupsForThisFrame( void )
{
	int i;
	for( i = 0; i < m_CachedNumBudgetGroups; i++ )
	{
		m_BudgetGroupTimes[i].m_Time[m_BudgetHistoryOffset] = 0.0;
	}
}

void CBudgetPanel::ClearAllTimesForGroup( int groupID )
{
	int i;
	for( i = 0; i < VPROF_HISTORY_COUNT; i++ )
	{
		m_BudgetGroupTimes[groupID].m_Time[i] = 0.0;
	}
}

static ConVar budget_old( "budget_old", "0" );

void CBudgetPanel::CalculateBudgetGroupTimes_Recursive( CVProfNode *pNode )
{
	int groupID = pNode->GetBudgetGroupID();
	Assert( groupID >= 0 && groupID < m_CachedNumBudgetGroups );

	double nodeTime;

	if( budget_old.GetBool() )
	{
		nodeTime = pNode->GetCurTimeLessChildren();
	} 
	else
	{
		nodeTime = pNode->GetPrevTimeLessChildren();
	}
//	Assert( nodeTime >= 0.0 );
	m_BudgetGroupTimes[groupID].m_Time[m_BudgetHistoryOffset] += nodeTime;
	if( pNode->GetSibling() )
	{
		CalculateBudgetGroupTimes_Recursive( pNode->GetSibling() );
	}
	if( pNode->GetChild() )
	{
		CalculateBudgetGroupTimes_Recursive( pNode->GetChild() );
	}
}

void CBudgetPanel::SnapshotVProfHistory( void )
{
	m_BudgetHistoryOffset = ( m_BudgetHistoryOffset + 1 ) % VPROF_HISTORY_COUNT;
	ClearTimesForAllGroupsForThisFrame();
	CVProfNode *pNode = g_VProfCurrentProfile.GetRoot();
	if( pNode && pNode->GetChild() )
	{
		CalculateBudgetGroupTimes_Recursive( pNode->GetChild() );
	}
}

static void NumBudgetGroupsChangedCallBack( void )
{
	GetBudgetPanel()->OnNumBudgetGroupsChanged();
}

void CBudgetPanel::OnNumBudgetGroupsChanged( void )
{
	int oldNumGroups = m_CachedNumBudgetGroups;
	// Rebuild updates m_CachedNumBudgetGroups
	Rebuild();
	m_BudgetGroupTimes.EnsureCount( m_CachedNumBudgetGroups );
	int i;
	for( i = oldNumGroups; i < m_CachedNumBudgetGroups; i++ )
	{
		ClearAllTimesForGroup( i );
	}
	InvalidateLayout( false, true );
}


#define RIGHT_OF_HISTORY_PERCENTAGE .9
#define RIGHT_OF_LABELS_PERCENTAGE .2

void CBudgetPanel::Rebuild()
{
	int oldNumBudgetGroups = m_CachedNumBudgetGroups;
	m_CachedNumBudgetGroups = g_VProfCurrentProfile.GetNumBudgetGroups();
	if( m_pBudgetHistoryPanel )
	{
		m_pBudgetHistoryPanel->MarkForDeletion();
	}
	m_pBudgetHistoryPanel = new CBudgetHistoryPanel( this, "FrametimeHistory" );

	if( m_pBudgetBarGraphPanel )
	{
		m_pBudgetBarGraphPanel->MarkForDeletion();
	}
	m_pBudgetBarGraphPanel = new CBudgetBarGraphPanel( this, "BudgetBarGraph" );

	m_GraphLabels.AddMultipleToTail( m_CachedNumBudgetGroups - oldNumBudgetGroups );

	int i;
	for( i = oldNumBudgetGroups; i < m_CachedNumBudgetGroups; i++ )
	{
		const char *pBudgetGroupName = g_VProfCurrentProfile.GetBudgetGroupName( i );
		m_GraphLabels[i] = new vgui::Label( this, pBudgetGroupName, pBudgetGroupName );
	}
	Assert( m_GraphLabels.Count() == m_CachedNumBudgetGroups );
}

CBudgetPanel::CBudgetPanel( const char *pElementName )
	 :	CHudElement( pElementName ),
		vgui::Frame( NULL, pElementName )
{
	g_VProfCurrentProfile.RegisterNumBudgetGroupsChangedCallBack( &NumBudgetGroupsChangedCallBack );
	vgui::VPANEL pParent = VGui_GetClientDLLRootPanel();
	SetParent( pParent );

	m_CachedNumBudgetGroups = 0;
	m_bShowBudgetPanelHeld = false;
	m_BudgetHistoryOffset = 0;
//	memset( m_BudgetGroupTimes, 0, sizeof( m_BudgetGroupTimes ) );

	SetProportional( false );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
	SetSizeable( false ); 
	SetVisible( true );
	SetPaintBackgroundEnabled( false );

	// hide the system buttons
	SetTitleBarVisible( false );
	
	// set the scheme before any child control is created
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile("resource/ClientScheme.res", "Client");
	SetScheme( scheme );

	m_pBudgetHistoryPanel = NULL;
	m_pBudgetBarGraphPanel = NULL;
	OnNumBudgetGroupsChanged();

//	Rebuild();
//	LoadControlSettings( "resource/BudgetPanel.res" );
}

void CBudgetPanel::UpdateWindowGeometry()
{
	int width = budget_panel_width.GetInt();
	int height = budget_panel_height.GetInt();
	int x = budget_panel_x.GetInt();
	int y = budget_panel_y.GetInt();
	if( width > VPROF_HISTORY_COUNT )
	{
		width = VPROF_HISTORY_COUNT;
		budget_panel_width.SetValue( width );
	}
	if( width > ScreenWidth() - x )
	{
		width = ScreenWidth() - x;
		budget_panel_width.SetValue( width );
	}
	if( height > ScreenHeight() - y )
	{
		height = ScreenHeight() - y;
		budget_panel_height.SetValue( height );
	}
	SetPos( x, y );
	SetSize( width, height );
}

void CBudgetPanel::PerformLayout()
{
	float bottomOfHistoryPercentage = budget_panel_bottom_of_history_fraction.GetFloat();
	UpdateWindowGeometry();
	int x, y, width, height;
	GetPos( x, y );
	GetSize( width, height );
	m_pBudgetHistoryPanel->SetPos( x, y );
	m_pBudgetHistoryPanel->SetSize( width * RIGHT_OF_HISTORY_PERCENTAGE, height * bottomOfHistoryPercentage );


	int maxLabelWidth = 0;
	int i;
	for( i = 0; i < m_CachedNumBudgetGroups; i++ )
	{
		int width, height;
		m_GraphLabels[i]->GetContentSize( width, height );
		if( maxLabelWidth < width )
		{
			maxLabelWidth = width;
		}
	}
	
	m_pBudgetBarGraphPanel->SetPos( x + maxLabelWidth, y + height * bottomOfHistoryPercentage );
	m_pBudgetBarGraphPanel->SetSize( width - maxLabelWidth, height * ( 1 - bottomOfHistoryPercentage ) );

	for( i = 0; i < m_CachedNumBudgetGroups; i++ )
	{
		m_GraphLabels[i]->SetPos( 0, ( bottomOfHistoryPercentage * height ) + ( i * height * ( 1 - bottomOfHistoryPercentage ) ) / m_CachedNumBudgetGroups );
		// fudge height by 1 for rounding 
		m_GraphLabels[i]->SetSize( maxLabelWidth, 1 + ( height * ( 1 - bottomOfHistoryPercentage ) ) / m_CachedNumBudgetGroups );
		m_GraphLabels[i]->SetContentAlignment( vgui::Label::a_east );
	}

}

void CBudgetPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
//	SetBgColor( GetSchemeColor( "PerfBackground", pScheme ) );
	int i;
	for( i = 0; i < m_CachedNumBudgetGroups; i++ )
	{
		int red, green, blue, alpha;
		g_VProfCurrentProfile.GetBudgetGroupColor( i, red, green, blue, alpha );
		m_GraphLabels[i]->SetFgColor( Color( red, green, blue, alpha ) );
		m_GraphLabels[i]->SetBgColor( Color( 0, 0, 0, 255 ) );
		m_GraphLabels[i]->SetPaintBackgroundEnabled( false );
		m_GraphLabels[i]->SetFont( pScheme->GetFont( "BudgetLabel", IsProportional() ) );
	}

	m_hFont = pScheme->GetFont( "Default" );

}

CBudgetPanel::~CBudgetPanel()
{
}

void CBudgetPanel::Paint()
{

	if( m_BudgetGroupTimes.Count() == 0 )
	{
		return;
	}

	static CFastTimer timer;
	static bool TimerInitialized = false;
	if( !TimerInitialized )
	{
		timer.Start();
		TimerInitialized=true;
	}

	timer.End();
	

	// Subtract out the time spent drawing the panel:
	//materials->Flush(false);
	g_VProfCurrentProfile.Pause();
	m_pBudgetHistoryPanel->SetData( &m_BudgetGroupTimes[0].m_Time[0], m_CachedNumBudgetGroups, VPROF_HISTORY_COUNT, m_BudgetHistoryOffset );
	m_pBudgetHistoryPanel->SetRange( 0.0, 1000.0f / budget_history_range.GetFloat() );
	m_pBudgetBarGraphPanel->SetRange( 0.0, 1000.0f / budget_bargraph_range.GetFloat() );
	BaseClass::Paint();
	//materials->Flush(false);
	g_VProfCurrentProfile.Resume();
	
	//g_VProfCurrentProfile.GetRoot()->GetChild()->GetPrevTime();
	double fFrameTimeLessBudget = timer.GetDuration().GetSeconds();
	double fFrameRate = 1.0 / fFrameTimeLessBudget;

	//g_VProfCurrentProfile.GetRoot()->GetPrevTime();
	int r = 255;
	int g = 0; 

	int nDXSupportLevel = engine->GetDXSupportLevel();
	if( ( fFrameRate >= 60 )
	 || ( nDXSupportLevel <= 70 && fFrameRate >= 30 )
	 || ( nDXSupportLevel <= 60 && fFrameRate >= 20 ) )
	{
		r = 0;
		g = 255;
	}

	g_pMatSystemSurface->DrawColoredText( m_hFont, 600, 2, r, g, 0, 255, "%i fps (without showbudget)", RoundFloatToInt(fFrameRate) );

		
	timer.Start();

}

bool CBudgetPanel::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && m_bShowBudgetPanelHeld );
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CBudgetPanel::Initialize( void )
{
	g_pBudgetPanel = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBudgetPanel::Init(void)
{
	SetZPos( 1001 );

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBudgetPanel::UserCmd_ShowBudgetPanel( void )
{
	engine->ClientCmd( "vprof_on\n" );
	m_bShowBudgetPanelHeld = true;
	SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBudgetPanel::UserCmd_HideBudgetPanel( void )
{
	engine->ClientCmd( "vprof_off\n" );
	m_bShowBudgetPanelHeld = false;
	SetVisible( false );
}

