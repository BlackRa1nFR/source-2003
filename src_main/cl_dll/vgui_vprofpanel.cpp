//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: VGUI scoreboard
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

#include "vgui_vprofpanel.h"

#include <Color.h>
#include <vgui/Cursor.h>
#include <vgui_controls/Button.h>
#include <vgui/IScheme.h>
#include "vgui_int.h"
#include <KeyValues.h>
#include "fmtstr.h"

#include "iinput.h"

#include "hud.h"
#include "hud_macros.h"

#include "vgui_BudgetPanel.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// positions
#define VPROF_INDENT_X			XRES(20)
#define VPROF_INDENT_Y			YRES(10)

#define YHEIGHT( x )			( x )

DECLARE_HUDELEMENT( CVProfPanel );

DECLARE_HUD_COMMAND( CVProfPanel, ShowVProf );
DECLARE_HUD_COMMAND( CVProfPanel, HideVProf );

// Scoreboard dimensions
#define VPROF_TITLE_SIZE_Y			YRES(22)

#define X_BORDER					XRES(4)
#define Y_BORDER					YRES(4)

static CVProfPanel *g_pVProfPanel = NULL;

static CVProfPanel *GetVProfPanel( void )
{
	return g_pVProfPanel;
}


void CVProfPanel::ExpandAll( void )
{
	int count = m_Hierarchy.GetHighestItemID();
	int i;
	for( i = 0; i < count; i++ )
	{
		if( m_Hierarchy.IsItemIDValid( i ) )
		{
			m_Hierarchy.ExpandItem( i, true );
		}
	}
}

static void ClearNodeClientData( CVProfNode *pNode )
{
	pNode->SetClientData( -1 );
	if( pNode->GetSibling() )
	{
		ClearNodeClientData( pNode->GetSibling() );
	}
	
	if( pNode->GetChild() )
	{
		ClearNodeClientData( pNode->GetChild() );
	}
}

void CVProfPanel::Reset()
{
	m_Hierarchy.RemoveAll();
	ClearNodeClientData( g_VProfCurrentProfile.GetRoot() );
}


CON_COMMAND( vprof_expand_all, "Expand the whole vprof tree" )
{
	Msg("VProf expand all.\n");
	CVProfPanel *pVProfPanel = ( CVProfPanel * )GET_HUDELEMENT( CVProfPanel );
	pVProfPanel->ExpandAll();
}

static char g_szVProfScope[128];

CON_COMMAND( vprof_set_scope, "Set a specific scope to start showing vprof tree" )
{
	if ( engine->Cmd_Argc() != 2 )
	{
		Msg( "Clearing\n" );
		g_szVProfScope[0] = 0;
	}
	else
	{
		Q_strncpy(g_szVProfScope, engine->Cmd_Argv( 1 ), sizeof(g_szVProfScope) );
		Msg("VProf setting scope to %s\n", g_szVProfScope);
	}
	g_pVProfPanel->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Create the VProf panel
//-----------------------------------------------------------------------------
CVProfPanel::CVProfPanel( const char *pElementName ) 
 :	CHudElement( pElementName ),
	vgui::Frame( NULL, pElementName ),
	m_Hierarchy( this, "Hierarchy" )
{
	m_fShowVprofHeld = false;

	int x = VPROF_INDENT_X;
	int y = VPROF_INDENT_Y;
	int wide = ScreenWidth() - x * 2;
	int tall = ScreenHeight() - y * 2;

	SetBounds( x, y, wide, tall );
	SetBgColor(Color(0, 0, 0, 175));

	// Initialize the top title.
#ifdef VPROF_ENABLED
	SetTitle("VProf", false);
#else
	SetTitle("** VProf is not enabled **", false);
#endif
}


CVProfPanel::~CVProfPanel( void )
{
	g_pVProfPanel = NULL;
}

//-----------------------------------------------------------------------------
// Scheme settings!
//-----------------------------------------------------------------------------
void CVProfPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	IBorder *border = pScheme->GetBorder( "ToolTipBorder" );
	SetBorder(border);

	SetBgColor(Color(0, 0, 0, 175));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CVProfPanel::Close()
{
	UserCmd_HideVProf();
	BaseClass::Close();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CVProfPanel::Initialize( void )
{

	int x = VPROF_INDENT_X;
	int y = VPROF_INDENT_Y;
	int wide = ScreenWidth() - x * 2;
	int tall = ScreenHeight() - y * 2;

	m_Hierarchy.SetBounds(X_BORDER, VPROF_TITLE_SIZE_Y, wide - X_BORDER*2, tall - Y_BORDER*2 - VPROF_TITLE_SIZE_Y);
	m_Hierarchy.SetPaintBackgroundEnabled( false );
	m_Hierarchy.SetParent(this);
	m_Hierarchy.SetPaintBorderEnabled( false );

	g_pVProfPanel = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVProfPanel::Init(void)
{
	vgui::VPANEL pParent = VGui_GetClientDLLRootPanel();
	SetParent( pParent );
	SetZPos( 1002 );

	Initialize();
}

bool CVProfPanel::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && m_fShowVprofHeld );
}



void CVProfPanel::FillTree( KeyValues *pKeyValues, CVProfNode *pNode, int parent )
{
#ifdef VPROF_ENABLED

	CFmtStrN<1024> msg;

	bool fIsRoot = ( pNode == g_VProfCurrentProfile.GetRoot() );

	if ( fIsRoot )
	{
		if( pNode->GetChild() )
		{
			FillTree( pKeyValues, pNode->GetChild(), parent );
		}
	}
	else
	{
		double avgCalls = ( g_VProfCurrentProfile.NumFramesSampled() > 0 ) ? (double)pNode->GetTotalCalls() / (double)g_VProfCurrentProfile.NumFramesSampled() : 0;
		double avg = ( pNode->GetTotalCalls() > 0 ) ? pNode->GetTotalTime() / (double)pNode->GetTotalCalls() : 0;
		double avgLessChildren = ( pNode->GetTotalCalls() > 0 ) ? pNode->GetTotalTimeLessChildren() / (double)pNode->GetTotalCalls() : 0;
		
		msg.sprintf( "%s: %s Frame[%3dc, %5.2f/%5.2f] Avg[%5.2fc, %5.2f/%5.2f] Sum[%3dc, %5.2f/%5.2f] Peak[%5.2f]", 
					 pNode->GetName(), 
					 g_VProfCurrentProfile.GetBudgetGroupName( pNode->GetBudgetGroupID() ),
					 pNode->GetCurCalls(), pNode->GetCurTime(), pNode->GetCurTimeLessChildren(),
					 avgCalls, avg, avgLessChildren,
					 pNode->GetTotalCalls(), pNode->GetTotalTime(), pNode->GetTotalTimeLessChildren(),
					 pNode->GetPeakTime() );
		pKeyValues->SetString( "Text", msg );

		int id = pNode->GetClientData();
		if ( id  == -1 )
		{
			id = m_Hierarchy.AddItem( pKeyValues, parent ) ;
			int r,g,b,a;
			
			g_VProfCurrentProfile.GetBudgetGroupColor( pNode->GetBudgetGroupID(), r, g, b, a );
			m_Hierarchy.SetItemFgColor( id, Color( r, g, b, a ) );

			pNode->SetClientData( id );
		}
		else
		{
			m_Hierarchy.ModifyItem( id, pKeyValues );
		}
		
		if( pNode->GetSibling() )
		{
			FillTree( pKeyValues, pNode->GetSibling(), parent );
		}
		
		if( pNode->GetChild() )
		{
			FillTree( pKeyValues, pNode->GetChild(), id );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
//extern void ExecuteDeferredOp();

void CVProfPanel::UpdateProfile()
{
#ifdef VPROF_ENABLED
	//ExecuteDeferredOp();
	if (IsVisible())
	{
		SetTitle( CFmtStr( "VProf (%s) --  %d frames sampled", 
									   g_VProfCurrentProfile.IsEnabled() ?  "running" : "not running",
										   g_VProfCurrentProfile.NumFramesSampled() ), false );

		// It's important to cache bEnabled since calling pause can disable.
		bool bEnabled = g_VProfCurrentProfile.IsEnabled();
		if( bEnabled )
		{
			g_VProfCurrentProfile.Pause();
		}

		KeyValues * pVal = new KeyValues("");
		
		if ( !m_Hierarchy.GetItemCount() )
		{
			pVal->SetString( "Text", "Call tree" );
			m_Hierarchy.AddItem( pVal, -1 );
			m_Hierarchy.ExpandItem( 0, true );
		}

		CVProfNode *pStartNode = ( g_szVProfScope[0] == 0 ) ? g_VProfCurrentProfile.GetRoot()  : g_VProfCurrentProfile.FindNode(g_VProfCurrentProfile.GetRoot(), g_szVProfScope );
		
		if ( pStartNode )
		{
			FillTree( pVal, pStartNode, 0 );
		}
		
		pVal->deleteThis();

		if( bEnabled )
		{
			g_VProfCurrentProfile.Resume();
		}
	}
	if ( g_VProfCurrentProfile.IsEnabled() )
	{
		Assert( g_VProfCurrentProfile.AtRoot() );
		GetBudgetPanel()->SnapshotVProfHistory();
//		g_VProfCurrentProfile.MarkFrame();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVProfPanel::UserCmd_ShowVProf( void )
{
	m_fShowVprofHeld = true;

	SetVisible( true );
	// This is hacky . . need to at least remember the previous value to set it back.
	ConVar *cl_mouseenable = ( ConVar * )cvar->FindVar( "cl_mouseenable" );
	if( cl_mouseenable )
	{
		cl_mouseenable->SetValue( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVProfPanel::UserCmd_HideVProf( void )
{
	m_fShowVprofHeld = false;

	SetVisible( false );
	// This is hacky . . need to at least remember the previous value to set it back.
	ConVar *cl_mouseenable = ( ConVar * )cvar->FindVar( "cl_mouseenable" );
	if( cl_mouseenable )
	{
		cl_mouseenable->SetValue( 1 );
	}
}


//-----------------------------------------------------------------------------

void CVProfPanel::Paint()
{
	g_VProfCurrentProfile.Pause();
	BaseClass::Paint();
	g_VProfCurrentProfile.Resume();
}


