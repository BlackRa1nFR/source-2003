//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Interface for engine interaction with vgui
//=============================================================================


#include "winquake.h"
#include "vgui_int.h"
#include "vid.h"
#include "dll_state.h"
#include "keys.h"
#include "console.h"
#include "gl_matsysiface.h"
#include "cdll_engine_int.h"
#include "ienginevgui.h"
#include "EngineStats.h"
#include "demo.h"
#include "sys_dll.h"
#include "sound.h"
#include "soundflags.h"
#include "filesystem_engine.h"
#include "igame.h"
#include "dinput.h"
#include "con_nprint.h"



#include "vgui_DebugSystemPanel.h"
#include "vguimatsurface/IMatSystemSurface.h"
#include <vgui/IClientPanel.h>
#include <vgui/ISurface.h>
#include <vgui/Ilocalize.h>
#include <vgui/IPanel.h>
#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>
#include <vgui/IInput.h>


#include <GameUI/IGameUI.h>
#include <BaseUI/IBaseUI.h>
#include <GameUI/IGameConsole.h>

extern IGameUI *staticGameUIFuncs;
extern IGameConsole *staticGameConsole;
IBaseUI *staticUIFuncs;

#include "vgui_BaseUI_Interface.h" // CEnginePanel define
#include "cl_demoactionmanager.h"

#include "tier0/vprof.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern bool con_refreshonprint;
//extern HWND* pmainwindow;
ConVar g_SetEmulatedCursorVisibleCVar(  "vgui_emulatemouse", "0" );
int VGui_ActivateGameUI();
void ClearIOStates( void );
// Mouse callback functions
static void (*s_pfnGetMouse)(int &x, int &y) = NULL;
static void (*s_pfnSetMouse)(int x, int y) = NULL;

extern CEnginePanel	*staticEngineToolsPanel;
extern CDebugSystemPanel *staticDebugSystemPanel;
extern CEnginePanel *staticClientDLLPanel;
extern CStaticPanel *staticPanel;
extern CEnginePanel *staticGameUIPanel;


IMatSystemSurface *g_pMatSystemSurface = NULL;
extern IVEngineClient *engineClient;


// temporary console area so we can store prints before console is loaded
CUtlVector<char> g_TempConsoleBuffer(0, 1024);


static CUtlVector< vgui::VPANEL > g_FocusPanelList;
static ConVar vgui_drawfocus( "vgui_drawfocus", "0", 0, "Report which panel is under the mouse." );


CFocusOverlayPanel::CFocusOverlayPanel( vgui::Panel *pParent, const char *pName ) : vgui::Panel( pParent, pName )
{
	SetCursor( vgui::dc_none );
	
	SetPaintEnabled( false );
	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( false );

	SetPostChildPaintEnabled( true );
}

void CFocusOverlayPanel::PostChildPaint( void )
{
	BaseClass::PostChildPaint();

	if( !vgui_drawfocus.GetBool() )
		return;

	int c = g_FocusPanelList.Size();
	if ( c <= 0 )
		return;

	int slot = 0;
	int fullscreeninset = 0;

	for ( int i = 0; i < c; i++ )
	{
		if ( slot > 31 )
			break;

		VPANEL vpanel = g_FocusPanelList[ i ];
		if ( !vpanel )
			continue;

		if ( !vgui::ipanel()->IsVisible( vpanel ) )
			return;

		// Convert panel bounds to screen space
		int r, g, b;
		GetColorForSlot( slot, r, g, b );

		int x, y, x1, y1;
		vgui::ipanel()->GetClipRect( vpanel, x, y, x1, y1 );

		if ( (unsigned int)(x1 - x) == vid.width && 
			 (unsigned int)(y1 - y) == vid.height )
		{
			x += fullscreeninset;
			y += fullscreeninset;
			x1 -= fullscreeninset;
			y1 -= fullscreeninset;
			fullscreeninset++;
		}
		vgui::surface()->DrawSetColor( Color( r, g, b, 255 ) );
		vgui::surface()->DrawOutlinedRect( x, y, x1, y1 );

		slot++;
	}
}

CFocusOverlayPanel *staticFocusOverlayPanel = NULL;


//-----------------------------------------------------------------------------
// Purpose: converts an english string to unicode
//-----------------------------------------------------------------------------
int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSizeInBytes)
{
	return vgui::localize()->ConvertANSIToUnicode( ansi, unicode, unicodeBufferSizeInBytes );
}





//-----------------------------------------------------------------------------
// Purpose: Toggle engine panel active/inactive
// Input  : state - 
//-----------------------------------------------------------------------------
void SetEngineVisible( bool state )
{
	if ( staticClientDLLPanel )
	{
		staticClientDLLPanel->SetVisible( state );
	}
	if ( staticEngineToolsPanel )
	{
		staticEngineToolsPanel->SetVisible( state );
	}
}


void VGui_ConPrintf(const char *fmt, ...)
{
	if (staticGameConsole)
	{
		staticGameConsole->Printf(fmt);
	}
	else
	{
		// write to a temporary buffer so we can write into vgui later
		int len = strlen(fmt);
		int pos = g_TempConsoleBuffer.AddMultipleToTail(len);
		memcpy(&g_TempConsoleBuffer[pos], fmt, len);
	}
}

void VGui_ConDPrintf(const char *fmt, ...)
{
	if (staticGameConsole)
	{
		staticGameConsole->DPrintf(fmt);
	}
	else
	{
		// write to a temporary buffer so we can write into vgui later
		int len = strlen(fmt);
		int pos = g_TempConsoleBuffer.AddMultipleToTail(len);
		memcpy(&g_TempConsoleBuffer[pos], fmt, len);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : clr - 
//			*fmt - 
//			... - 
//-----------------------------------------------------------------------------
void VGui_ColorPrintf( Color& clr, const char *fmt, ...)
{
	if (staticGameConsole)
	{
		staticGameConsole->ColorPrintf( clr, fmt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGui_Startup()
{
	if (staticUIFuncs)
		return;
	
	staticUIFuncs = (IBaseUI *)g_AppSystemFactory(BASEUI_INTERFACE_VERSION, NULL);
	staticUIFuncs->Initialize( &g_AppSystemFactory, 1 );
	staticUIFuncs->Start(NULL, 0); // these two parameters are used in GoldSRC only

	// write contents of temp buffer into console
	g_TempConsoleBuffer.AddToTail('\0');
	VGui_ConPrintf( (const char *)g_TempConsoleBuffer.Base());
	g_TempConsoleBuffer.Purge();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGui_Shutdown()
{

	if (!staticUIFuncs)
		return;

	staticUIFuncs->Shutdown();
	staticUIFuncs=NULL;

}

//-----------------------------------------------------------------------------
// Sets the input trapping mode
//-----------------------------------------------------------------------------
extern bool s_bWindowsInputEnabled;
void VGui_EnableWindowsMessages( bool bEnable )
{
	s_bWindowsInputEnabled = bEnable;
	if (g_pMatSystemSurface)
		g_pMatSystemSurface->EnableWindowsMessages( bEnable );
}


//-----------------------------------------------------------------------------
// Sets the mouse callbacks
//-----------------------------------------------------------------------------
void VGui_SetMouseCallbacks(void (* pfnGetMouse)(int &x, int &y), void (*pfnSetMouse)(int x, int y))
{
	s_pfnGetMouse = pfnGetMouse;
	s_pfnSetMouse = pfnSetMouse;
	if (g_pMatSystemSurface)
		g_pMatSystemSurface->SetMouseCallbacks( s_pfnGetMouse, s_pfnSetMouse );
}

void VGui_HandleWindowMessage( void *hwnd, unsigned int uMsg, unsigned int wParam, long lParam )
{
	if ( g_pMatSystemSurface )
		g_pMatSystemSurface->HandleWindowMessage( hwnd, uMsg, wParam, lParam );
}


// Bunch of vgui debugging stuff
static ConVar vgui_drawtree( "vgui_drawtree", "0", 0, "Draws the vgui panel hiearchy to the specified depth level." );
static ConVar vgui_drawpanel( "vgui_drawpanel", "", 0, "Draws the vgui panel hiearchy starting with the named panel." );
static ConVar vgui_treestart( "vgui_treestart", "0", 0, "Depth level to start printing info out at for vgui_drawtree." );
static ConVar vgui_drawpopups( "vgui_drawpopups", "0", 0, "Draws the vgui popup list in hierarchy(1) or most recently used(2) order." );

static void VGui_RecursiveFindPanels( CUtlVector< vgui::VPANEL  >& panelList, VPANEL check, char const *panelname )
{
	vgui::Panel *panel = vgui::ipanel()->GetPanel( check, "ENGINE" );
	if ( !panel )
		return;

	if ( !Q_strncmp( panel->GetName(), panelname, strlen( panelname ) ) )
	{
		panelList.AddToTail( panel->GetVPanel() );
	}

	int childcount = panel->GetChildCount();
	for ( int i = 0; i < childcount; i++ )
	{
		Panel *child = panel->GetChild( i );
		VGui_RecursiveFindPanels( panelList, child->GetVPanel(), panelname );
	}
}

void VGui_FindNamedPanels( CUtlVector< vgui::VPANEL >& panelList, char const *panelname )
{
	vgui::VPANEL embedded = vgui::surface()->GetEmbeddedPanel();

	// faster version of code below
	// checks through each popup in order, top to bottom windows
	int c = vgui::surface()->GetPopupCount();
	for (int i = c - 1; i >= 0; i--)
	{
		VPANEL popup = vgui::surface()->GetPopup(i);
		if ( !popup )
			continue;

		if ( embedded == embedded )
			continue;

		VGui_RecursiveFindPanels( panelList, popup, panelname );
	}

	VGui_RecursiveFindPanels( panelList, embedded, panelname );
}

static void VGui_TogglePanel_f( void )
{
	if ( Cmd_Argc() < 2 )
	{
		Con_Printf( "Usage:  vgui_showpanel panelname\n" );
		return;
	}

	bool flip = false;
	bool fg = true;
	bool bg = true;

	if ( Cmd_Argc() == 5 )
	{
		flip = atoi( Cmd_Argv( 2 ) ) ? true : false;
		fg = atoi( Cmd_Argv( 3 ) ) ? true : false;
		bg = atoi( Cmd_Argv( 4 ) ) ? true : false;
	}
		
	char const *panelname = Cmd_Argv( 1 );
	if ( !panelname || !panelname[ 0 ] )
		return;

	CUtlVector< vgui::VPANEL > panelList;

	VGui_FindNamedPanels( panelList, panelname );
	if ( !panelList.Size() )
	{
		Con_Printf( "No panels starting with %s\n", panelname );
		return;
	}

	for ( int i = 0; i < panelList.Size(); i++ )
	{
		vgui::VPANEL p = panelList[ i ];
		if ( !p )
			continue;

		vgui::Panel *panel = vgui::ipanel()->GetPanel( p, "ENGINE");
		if ( !panel )
			continue;

		Msg( "Toggling %s\n", panel->GetName() );

		if ( fg )
		{
			panel->SetPaintEnabled( flip );
		}
		if ( bg )
		{
			panel->SetPaintBackgroundEnabled( flip );
		}
	}
}

static ConCommand vgui_togglepanel( "vgui_togglepanel", VGui_TogglePanel_f, "show/hide vgui panel by name." );

static void VGui_RecursePanel( CUtlVector< vgui::VPANEL >& panelList, int x, int y, VPANEL check, bool include_hidden )
{
	vgui::Panel *panel = vgui::ipanel()->GetPanel( check, "ENGINE" );
	if ( !panel )
		return;

	if( !include_hidden && !panel->IsVisible() )
	{
		return;
	}

	if ( panel->IsWithinTraverse( x, y, false ) )
	{
		panelList.AddToTail( check );
	}

	int childcount = panel->GetChildCount();
	for ( int i = 0; i < childcount; i++ )
	{
		Panel *child = panel->GetChild( i );
		VGui_RecursePanel( panelList, x, y, child->GetVPanel(), include_hidden );
	}
}

void VGui_DrawMouseFocus( void )
{
	VPROF( "VGui_DrawMouseFocus" );

	if ( !vgui_drawfocus.GetBool() )
		return;
	
	g_FocusPanelList.RemoveAll();
	staticFocusOverlayPanel->MoveToFront();

	bool include_hidden = vgui_drawfocus.GetInt() > 1 ? true : false;

	int x, y;
	vgui::input()->GetCursorPos( x, y );

	vgui::VPANEL embedded = vgui::surface()->GetEmbeddedPanel();

	if ( vgui::surface()->IsCursorVisible() && vgui::surface()->IsWithin(x, y) )
	{
		// faster version of code below
		// checks through each popup in order, top to bottom windows
		int c = vgui::surface()->GetPopupCount();
		for (int i = c - 1; i >= 0; i--)
		{
			VPANEL popup = vgui::surface()->GetPopup(i);
			if ( !popup )
				continue;

			if ( popup == embedded )
				continue;
			if ( !ipanel()->IsVisible( popup ) )
				continue;

			VGui_RecursePanel( g_FocusPanelList, x, y, popup, include_hidden );
		}

		VGui_RecursePanel( g_FocusPanelList, x, y, embedded, include_hidden );
	}

	// Now draw them
	con_nprint_t np;
	np.time_to_live = 1.0f;
	
	int c = g_FocusPanelList.Size();

	int slot = 0;
	for ( int i = 0; i < c; i++ )
	{
		if ( slot > 31 )
			break;

		VPANEL vpanel = g_FocusPanelList[ i ];
		if ( !vpanel )
			continue;

		np.index = slot;

		int r, g, b;
		CFocusOverlayPanel::GetColorForSlot( slot, r, g, b );

		np.color[ 0 ] = r / 255.0f;
		np.color[ 1 ] = g / 255.0f;
		np.color[ 2 ] = b / 255.0f;

		Con_NXPrintf( &np, "%3i:  %s\n", slot + 1, ipanel()->GetName(vpanel) );

		slot++;
	}

	while ( slot <= 31 )
	{
		Con_NPrintf( slot, "" );
		slot++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : depth - 
//			start - 
//			end - 
//			*current - 
//			totaldrawn - 
//-----------------------------------------------------------------------------
void VGui_RecursivePrintTree( int depth, int start, int end, vgui::Panel *current, int& totaldrawn )
{
	// No more room
	if ( totaldrawn >= 128 )
		return;

	if ( !current )
		return;

	int count = current->GetChildCount();
	for ( int i = 0; i < count ; i++ )
	{
		vgui::Panel *panel = current->GetChild( i );
//		Msg( "%i:  %s : %p, %s %s\n", 
//			i + 1, 
//			panel->GetName(), 
//			panel, 
//			panel->IsVisible() ? "visible" : "hidden",
//			panel->IsPopup() ? "popup" : "" );

		if ( depth >= start && depth <= end )
		{
			Con_NPrintf( totaldrawn++, "%s (%i.%i): %p, %s %s\n", 
				panel->GetName(), 
				depth + 1,
				i + 1, 
				panel, 
				panel->IsVisible() ? "visible" : "hidden",
				panel->IsPopup() ? "popup" : "" );
		}

		VGui_RecursivePrintTree( depth + 1, start, end, panel, totaldrawn );	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGui_DrawPopups( void )
{
	VPROF( "VGui_DrawPopups" );

	if ( vgui_drawpopups.GetInt() <= 0 )
		return;
	
	int type = vgui_drawpopups.GetInt() - 1;
	type = clamp( type, 0, 1 );

	int c = vgui::surface()->GetPopupCount();
	for ( int i = 0; i < c; i++ )
	{
		vgui::VPANEL popup = vgui::surface()->GetPopup( i );
		if ( !popup )
			continue;

		const char *p = vgui::ipanel()->GetName( popup );
		bool visible = vgui::ipanel()->IsVisible( popup );

		Con_NPrintf( i, "%i:  %s : %p, %s\n",
			i,
			p,
			popup,
			visible ? "visible" : "hidden" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGui_DrawHierarchy( void )
{
	VPROF( "VGui_DrawHierarchy" );
	
	if ( vgui_drawtree.GetInt() <= 0 && Q_strlen( vgui_drawpanel.GetString() ) <= 0 )
		return;

	int startlevel = 0;
	int endlevel = 1000;

	bool wholetree = vgui_drawtree.GetInt() > 0 ? true : false;
	
	if ( wholetree )
	{
		startlevel = vgui_treestart.GetInt();
		endlevel = vgui_drawtree.GetInt();
	}

	// Can't start after end
	startlevel = min( endlevel, startlevel );

	int drawn = 0;


	vgui::VPANEL root = vgui::surface()->GetEmbeddedPanel();
	if ( !root )
		return;

	vgui::Panel *base = staticPanel;

	if ( base && !wholetree )
	{
		// Find named panel
		char const *name = vgui_drawpanel.GetString();
		base = base->FindChildByName( name, true );
	}

	if ( !base )
		return;

	VGui_RecursivePrintTree( 0, startlevel, endlevel, base, drawn );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGui_ActivateMouse()
{
	if ( !g_ClientDLL )
		return;

	// Don't mess with mouse if not active
	if ( !game->IsActiveApp() )
	{
		g_ClientDLL->IN_DeactivateMouse ();
		return;
	}
		
	/* 
	//
	// MIKE AND ALFRED: these panels should expose whether they want mouse input or not and 
	// CalculateMouseVisible will take them into account.
	//
	// If showing game ui, make sure nothing else is hooking it
	if ( VGui_IsGameUIVisible() || VGui_IsDebugSystemVisible() )
	{
		g_ClientDLL->IN_DeactivateMouse();
		return;
	}
	*/

	if ( surface()->IsCursorLocked() )
	{
		g_ClientDLL->IN_ActivateMouse ();
	}
	else
	{
		g_ClientDLL->IN_DeactivateMouse ();
	}
}

void VGui_Paint()
{
	VPROF_BUDGET( "VGui_Paint", VPROF_BUDGETGROUP_OTHER_VGUI );

	static ConVar r_drawvgui( "r_drawvgui", "1", 0, "Enable the rendering of vgui panels" );

	con_refreshonprint = false;

	// Don't draw the console at all if vgui is off during a time demo
	if ( demo->IsPlayingBack_TimeDemo() && (r_drawvgui.GetInt() == 0) )
	{
		VPROF( "vgui::ivgui()->RunFrame()" );
		vgui::ivgui()->RunFrame();
		con_refreshonprint = true;
		return;
	}

//	vgui::ivgui()->RunFrame();
	
	SetEngineVisible( r_drawvgui.GetInt() != 0 );

	// Some debugging helpers
	VGui_DrawHierarchy();
	VGui_DrawPopups();
	VGui_DrawMouseFocus();

	if (!staticUIFuncs)
		return;

	/*win::*/POINT pnt = {0, 0};
	/*win::*/RECT rect;
	/*win::*/ClientToScreen(*pmainwindow, &pnt);
	/*win::*/GetClientRect(*pmainwindow, &rect);

	staticUIFuncs->Paint(pnt.x, pnt.y, rect.right, rect.bottom);

	con_refreshonprint = true;

	surface()->CalculateMouseVisible();
	VGui_ActivateMouse();
}

int VGui_IsGameUIVisible()
{
	if (!staticGameUIFuncs)
		return 0;

	return staticGameUIFuncs->HasExclusiveInput();
}

int VGui_NeedSteamPassword(char *szAccountName, char *szUserName)
{
	if (!staticGameUIFuncs)
		return 0;

	staticGameUIFuncs->GetSteamPassword(szAccountName, szUserName);
	return 1;
}


void VGui_NotifyOfServerConnect(const char *game, int IP, int port)
{
	if (!staticGameUIFuncs)
		return;

	staticGameUIFuncs->ConnectToServer(game, IP, port);
}

void VGui_NotifyOfServerDisconnect()
{
	if (!staticGameUIFuncs)
		return;

	// NOTE:  This calls ActivateGameUI internally, so we need to clear IO states below to debounce any keys
	//  that triggered the disconnect
	staticGameUIFuncs->DisconnectFromServer();

	ClearIOStates();
}

int VGui_ActivateGameUI()
{
	/*
	if (!staticGameUIFuncs)
		return 0;

	int iret = staticGameUIFuncs->ActivateGameUI();
	SetEngineVisible( false );
	// clear any keys that might be stuck down
	ClearIOStates();

	return iret;
	*/
	if (staticUIFuncs)
	{
		staticUIFuncs->ActivateGameUI();
	}
	SetEngineVisible( false );
	// clear any keys that might be stuck down
	ClearIOStates();
	return 1;

}

void VGui_HideGameUI()
{
	if (!staticUIFuncs)
		return;

	staticUIFuncs->HideGameUI();
	SetEngineVisible( true );
}

int VGui_IsConsoleVisible()
{
	if (staticGameConsole && staticUIFuncs )
	{
		return ( staticUIFuncs->IsGameUIVisible() && staticGameConsole->IsConsoleVisible() );
	}

	return false;
}


void VGui_ShowConsole()
{
	if (staticUIFuncs)
	{
		staticUIFuncs->ActivateGameUI();
	}

	if (staticUIFuncs)
	{
		staticUIFuncs->ShowConsole();
	}
}

void VGui_HideConsole()
{
	if(staticUIFuncs)
	{
		staticUIFuncs->HideConsole();
	}
}

void VGui_ClearConsole()
{
	if (staticGameConsole)
	{
		staticGameConsole->Clear();
	}
}

int VGui_GameUIKeyPressed()
{
	if (!staticGameUIFuncs)
		return 0;

	// toggle the game UI
	if (staticGameUIFuncs->HasExclusiveInput())
	{
		if (cl.levelname[0])
		{
			VGui_HideGameUI();
		}
		return 1;
	}
	else
	{
		
		return VGui_ActivateGameUI();
	}
}

int VGui_IsDebugSystemVisible( void )
{
	if ( !staticDebugSystemPanel )
		return false;

	return staticDebugSystemPanel->IsVisible();
}

void VGui_ShowDebugSystem( void )
{
	if ( !staticDebugSystemPanel )
		return;

	staticDebugSystemPanel->SetVisible( true );
	SetEngineVisible( false );
	// clear any keys that might be stuck down
	ClearIOStates();

}

void VGui_HideDebugSystem( void )
{
	if ( !staticDebugSystemPanel )
		return;

	staticDebugSystemPanel->SetVisible( false );

	SetEngineVisible( true );
}

void VGui_DebugSystemKeyPressed()
{
	if (!staticDebugSystemPanel)
		return;

	// toggle the game UI
	if ( VGui_IsDebugSystemVisible() )
	{
		VGui_HideDebugSystem();
	}
	else
	{
		VGui_ShowDebugSystem();
	}
}

bool VGui_IsShiftKeyDown( void )
{
	if ( !vgui::input() )
		return false;

	return vgui::input()->IsKeyDown( vgui::KEY_LSHIFT ) ||
		    vgui::input()->IsKeyDown( vgui::KEY_RSHIFT );
}

