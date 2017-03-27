//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements all the functions exported by the GameUI dll
//
// $NoKeywords: $
//=============================================================================
#include <windows.h>

#include "FileSystem.h"
#include "string.h"
#include <io.h> 
#include "tier0/dbg.h"
#include "winquake.h"
#include "vgui_int.h"
#include "vid.h"
#include "dll_state.h"
#include "keys.h"
#include "console.h"
#include "gl_matsysiface.h"
#include "cdll_engine_int.h"
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
#include "tier0/vprof.h"
#include "cl_demoactionmanager.h"
#include "enginebugreporter.h"
#include "vstdlib/icommandline.h"

#include <vgui/VGUI.h>

// interface to gameui dll
#include <GameUI/IGameUI.h>
#include <GameUI/IGameConsole.h>

// interface to expose vgui root panels
#include <IEngineVGUI.h>
#include "vguimatsurface/IMatSystemSurface.h"

#include <common.h> // COM_*
#include "sysexternal.h" // Sys_Error
#include "enginestats.h" // IClientStats


// vgui2 interface
// note that GameUI project uses ..\public\vgui and ..\public\vgui_controls, not ..\utils\vgui\include
#include <vgui/VGUI.h>
#include <vgui/Cursor.h>
#include <KeyValues.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui_controls/EditablePanel.h>

#include <vgui_controls/MenuButton.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/PHandle.h>

#include "IVGuiModule.h"
#include "vgui_BaseUI_Interface.h"
//#include "BasePanel.h"
#include "vgui_DebugSystemPanel.h"

// top level VGUI2 panel
CStaticPanel *staticPanel = NULL;

// base level panels for other subsystems, rooted on staticPanel
CEnginePanel *staticClientDLLPanel = NULL;
CEnginePanel *staticGameUIPanel = NULL;

// Want engine tools to be on top of other engine panels
CEnginePanel		*staticEngineToolsPanel = NULL;
CDebugSystemPanel *staticDebugSystemPanel = NULL;
extern CFocusOverlayPanel *staticFocusOverlayPanel; // in vgui_int.cpp

extern IVEngineClient *engineClient;

class CBaseUI;
CBaseUI *g_pBaseUI = NULL;
static CBaseUI g_BaseUI;

extern CreateInterfaceFn g_AppSystemFactory;


// functions to reference GameUI and GameConsole functions, from GameUI.dll
IGameUI *staticGameUIFuncs = NULL;
IGameConsole *staticGameConsole = NULL;

bool s_bWindowsInputEnabled = true;



void SCR_CreateDisplayStatsPanel( vgui::Panel *parent );
void Con_CreateConsolePanel( vgui::Panel *parent );
void CL_CreateEntityReportPanel( vgui::Panel *parent );
int VGui_ActivateGameUI();
void ClearIOStates( void );

extern IMatSystemSurface *g_pMatSystemSurface;

//-----------------------------------------------------------------------------
// Purpose: singleton accessor
//-----------------------------------------------------------------------------
CBaseUI &BaseUI()
{
	return g_BaseUI;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CBaseUI, IBaseUI, BASEUI_INTERFACE_VERSION, g_BaseUI);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBaseUI::CBaseUI()
{
	g_pBaseUI = this;
	m_hVGuiModule = NULL;
	m_hStaticGameUIModule = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBaseUI::~CBaseUI()
{
	g_pBaseUI = NULL;
}



void CBaseUI::Initialize( CreateInterfaceFn *factories, int count )
{

	TRACE_FUNCTION("CBaseUI::Initialize");

	if (staticGameUIFuncs) // don't initialize twice
		return;

	// load and initialize vgui
	CreateInterfaceFn engineFactory = factories[ 0 ];

	// load the VGUI library
	IFileSystem *fs = ( IFileSystem * )engineFactory( FILESYSTEM_INTERFACE_VERSION, NULL );
	fs->GetLocalCopy("vgui2.dll");
	m_hVGuiModule = Sys_LoadModule("vgui2.dll");

	CreateInterfaceFn vguiFactory = Sys_GetFactory(m_hVGuiModule);

	memset( m_FactoryList, 0x0, sizeof( m_FactoryList ) );
	// setup the factory list
	m_FactoryList[ 0 ] = engineFactory;
	m_FactoryList[ 1 ] = vguiFactory;
	m_iNumFactories = 2; // these 2 factories MUST exist :)

	//-----------------------------------------------------------------------------
	// load the GameUI dll
	char szDllName[512];
	_snprintf(szDllName, sizeof(szDllName), "bin\\gameui.dll" );
	COM_ExpandFilename(szDllName);
	COM_FixSlashes(szDllName);

/*#ifndef _DEBUG
	// only force a local copy in release builds
	FS_GetLocalCopy(szDllName);
#endif*/
	g_pFileSystem->GetLocalCopy(szDllName);

	m_hStaticGameUIModule = Sys_LoadModule(szDllName);
	CreateInterfaceFn gameUIFactory = Sys_GetFactory(m_hStaticGameUIModule);

	if( !gameUIFactory )
	{
		Sys_Error( "Could not load: %s\n", szDllName );
	}
	
	m_FactoryList[ m_iNumFactories ] = gameUIFactory;
	m_iNumFactories++;

	// get the initialization func
	staticGameUIFuncs = (IGameUI *)gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL);
	staticGameConsole = (IGameConsole *)gameUIFactory(GAMECONSOLE_INTERFACE_VERSION, NULL);

	// Check for failure...
	if (!staticGameUIFuncs )
	{
		Sys_Error( "Could not get IGameUI interface %s from %s\n", GAMEUI_INTERFACE_VERSION, szDllName );
	}
	if (!staticGameConsole )
	{
		Sys_Error( "Could not get IGameConsole interface %s from %s\n", GAMECONSOLE_INTERFACE_VERSION, szDllName );
	}
		
	//-----------------------------------------------------------------------------


	vgui::ISurface *pVGuiSurface = (vgui::ISurface*)g_AppSystemFactory( VGUI_SURFACE_INTERFACE_VERSION, NULL );
	if (!pVGuiSurface)
	{
		Sys_Error( "Could not get vgui::ISurface interface %s from %s\n", VGUI_SURFACE_INTERFACE_VERSION, szDllName );
		return;
	}

	vgui::VGui_InitInterfacesList( "BaseUI", &g_AppSystemFactory, 1 );

	pVGuiSurface->Connect( g_AppSystemFactory );

	g_pMatSystemSurface = (IMatSystemSurface *)pVGuiSurface->QueryInterface(MAT_SYSTEM_SURFACE_INTERFACE_VERSION);
	if ( !g_pMatSystemSurface )
	{
		Sys_Error( "Could not get IMatSystemSurface interface %s from %s\n", MAT_SYSTEM_SURFACE_INTERFACE_VERSION, szDllName );
		return;
	}

	if ( pVGuiSurface->Init() != INIT_OK )
	{
		Sys_Error( "IMatSystemSurface Init != INIT_OK\n" );
		return;
	}

	IClientStats* pVGuiMatSurfaceStats = ( IClientStats * )g_AppSystemFactory( INTERFACEVERSION_CLIENTSTATS, NULL );
	if (!pVGuiMatSurfaceStats)
	{
		Con_Printf( "Unable to init vgui materialsystem surface  stats version %s\n", INTERFACEVERSION_CLIENTSTATS );
	}
	else
	{
		g_EngineStats.InstallClientStats( pVGuiMatSurfaceStats );
	}

}

//-----------------------------------------------------------------------------
// A helper to play sounds through vgui
//-----------------------------------------------------------------------------
static void VGui_PlaySound(const char *pFileName)
{
	// Point at origin if they didn't specify a sound source.
	Vector vDummyOrigin;
	vDummyOrigin.Init();

	CSfxTable *pSound = (CSfxTable*)S_PrecacheSound(pFileName);
	if ( pSound )
	{
		S_StartDynamicSound(cl.viewentity, CHAN_AUTO, pSound, vDummyOrigin, 1.0, SNDLVL_IDLE, 0, PITCH_NORM);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called to setup the base UI
//-----------------------------------------------------------------------------
void CBaseUI::Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion)
{
	TRACE_FUNCTION("CBaseUI::Start");

	// copy the engine interface
//	memcpy(&gEngfuncs, engineFuncs, sizeof(gEngfuncs));

	// Hook the material system into the window
	g_pMatSystemSurface->AttachToWindow( *pmainwindow, true );

	// Hook up any mouse callbacks that may have already been installed
	VGui_SetMouseCallbacks( &CDInputGetMousePos, &CDInputSetMousePos );

	// Are we trapping input?
	g_pMatSystemSurface->EnableWindowsMessages( s_bWindowsInputEnabled );

	// Need to be able to play sounds through vgui
	g_pMatSystemSurface->InstallPlaySoundFunc( VGui_PlaySound );


		// load scheme
	if (!vgui::scheme()->LoadSchemeFromFile("Resource/TrackerScheme.res", "Tracker"))
	{
		Con_Printf("Error loading '%s'\n", "Resource/TrackerScheme.res");
	}


	// Start the App running
	vgui::ivgui()->Start();
	vgui::ivgui()->SetSleep(false);

	// setup base panel for the whole VGUI System
	// The root panel for everything ( NULL parent makes it a child of the embedded panel )

	// Ideal hierarchy:

	// Root -- staticPanel
	//		staticBackgroundImagePanel (from gamui) zpos == 0
	//      staticClientDLLPanel ( zpos == 25 )
	//		staticEngineToolsPanel ( zpos == 75 )
	//		staticGameUIPanel ( GameUI stuff ) ( zpos == 100 )
	//		staticDebugSystemPanel ( Engine debug stuff ) zpos == 125 )
	staticPanel = new CStaticPanel( NULL, "staticPanel" );	
	
	staticPanel->SetBounds( 0,0,vid.width,vid.height);
	staticPanel->SetPaintBorderEnabled(false);
	staticPanel->SetPaintBackgroundEnabled(false);
	staticPanel->SetPaintEnabled(false);
	staticPanel->SetVisible(true);
	staticPanel->SetCursor( vgui::dc_none );
	staticPanel->SetZPos(0);
	staticPanel->SetVisible(true);
	staticPanel->SetParent( vgui::surface()->GetEmbeddedPanel() );

	staticClientDLLPanel = new CEnginePanel( staticPanel, "staticClientDLLPanel" );
	staticClientDLLPanel->SetBounds( 0, 0, vid.width, vid.height );
	staticClientDLLPanel->SetPaintBorderEnabled(false);
	staticClientDLLPanel->SetPaintBackgroundEnabled(false);
	staticClientDLLPanel->SetKeyBoardInputEnabled( false );	// popups in the client DLL can enable this.
	staticClientDLLPanel->SetPaintEnabled(false);
	staticClientDLLPanel->SetVisible( false );
	staticClientDLLPanel->SetCursor( vgui::dc_none );
	staticClientDLLPanel->SetZPos( 25 );

	staticEngineToolsPanel = new CEnginePanel( staticPanel, "Engine Tools" );
	staticEngineToolsPanel->SetBounds( 0, 0, vid.width, vid.height );
	staticEngineToolsPanel->SetPaintBorderEnabled(false);
	staticEngineToolsPanel->SetPaintBackgroundEnabled(false);
	staticEngineToolsPanel->SetPaintEnabled(false);
	staticEngineToolsPanel->SetVisible( true );
	staticEngineToolsPanel->SetCursor( vgui::dc_none );
	staticEngineToolsPanel->SetZPos( 75 );

	staticGameUIPanel = new CEnginePanel( staticPanel, "GameUI Panel" );
	staticGameUIPanel->SetBounds( 0, 0, vid.width, vid.height );
	staticGameUIPanel->SetPaintBorderEnabled(false);
	staticGameUIPanel->SetPaintBackgroundEnabled(false);
	staticGameUIPanel->SetPaintEnabled(false);
	staticGameUIPanel->SetVisible( true );
	staticGameUIPanel->SetCursor( vgui::dc_none );
	staticGameUIPanel->SetZPos( 100 );

	staticDebugSystemPanel = new CDebugSystemPanel( staticPanel, "Engine Debug System" );
	staticDebugSystemPanel->SetZPos( 125 );

	// Install demo playback/editing UI
	demoaction->InstallDemoUI( staticDebugSystemPanel );

	// Create an initialize bug reporting system
	bugreporter->InstallBugReportingUI( staticEngineToolsPanel );
	bugreporter->Init();

	// Make sure this is on top of everything
	staticFocusOverlayPanel = new CFocusOverlayPanel( staticPanel, "FocusOverlayPanel" );
	staticFocusOverlayPanel->SetBounds( 0,0,vid.width,vid.height );
	staticFocusOverlayPanel->SetZPos( 150 );
	staticFocusOverlayPanel->MoveToFront();

	// Create engine vgui panels
	Con_CreateConsolePanel( staticEngineToolsPanel );
	CL_CreateEntityReportPanel( staticEngineToolsPanel );
	SCR_CreateDisplayStatsPanel( staticEngineToolsPanel );

	// Make sure that these materials are in the materials cache
	materialSystemInterface->CacheUsedMaterials();

	// load the base localization file
	vgui::localize()->AddFile( vgui::filesystem(), "Resource/valve_%language%.txt" );

	// don't need to load the "valve" localization file twice
	// Each mod acn have its own language.txt in addition to the valve_%%langauge%%.txt file under defaultgamedir.
	vgui::localize()->AddFile( vgui::filesystem(), "Resource/%language%.txt" );

	if(staticGameUIFuncs)
	{
		staticGameUIFuncs->Initialize( m_FactoryList, m_iNumFactories );
		staticGameUIFuncs->Start(NULL, 0, NULL);
	}

	if(staticGameConsole)
	{
		// setup console
		staticGameConsole->Initialize();
		staticGameConsole->SetParent(staticGameUIPanel->GetVPanel());
	}

	if ( CommandLine()->FindParm( "-toconsole" ) || CommandLine()->FindParm( "-console" ) )
	{
		staticGameUIFuncs->ActivateGameUI();
		staticGameConsole->Activate();
	}
	else
	{
		// show the game UI
		staticGameUIFuncs->ActivateGameUI();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called to Shutdown the game UI system
//-----------------------------------------------------------------------------
void CBaseUI::Shutdown()
{
	bugreporter->Shutdown();

	// This will delete the engine subpanel since it's a child
	delete staticPanel;
	staticPanel = null;
	staticClientDLLPanel	= NULL;
	staticEngineToolsPanel = NULL;
	staticDebugSystemPanel = NULL;
	staticFocusOverlayPanel = NULL;

	// Give panels a chance to settle so things
	//  Marked for deletion will actually get deleted
	vgui::ivgui()->RunFrame();

	// unload the gameUI
	staticGameUIFuncs->Shutdown();
	Sys_UnloadModule(m_hStaticGameUIModule);
	m_hStaticGameUIModule = NULL;

	staticGameUIFuncs = NULL;
	staticGameConsole = NULL;

	// Shutdown vgui
	vgui::system()->SaveUserConfigFile();
	vgui::system()->Shutdown();
	// stop the App running
	vgui::ivgui()->Stop();
	
	// Shutdown
	vgui::surface()->Shutdown();
	vgui::ivgui()->Shutdown();

	Sys_UnloadModule(m_hVGuiModule);
	m_hVGuiModule = 0;

	// unload the dll
	m_hStaticGameUIModule = NULL;
	staticGameUIFuncs = NULL;
	staticGameConsole = NULL;
	g_pMatSystemSurface = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Handles windows messages passed in from the engine
//-----------------------------------------------------------------------------
void CBaseUI::CallEngineSurfaceProc(void *hwnd, unsigned int msg, unsigned int wparam, long lparam)
{
	//staticSurface->proc(hwnd, msg, wparam, lparam);
}


//-----------------------------------------------------------------------------
// Purpose: Hides an Game UI related features (not client UI stuff tho!)
//-----------------------------------------------------------------------------
void CBaseUI::HideGameUI()
{

	staticGameUIFuncs->HideGameUI();
//	staticGameConsole->Hide();
	const char *levelName = engineClient->GetLevelName();

	if (levelName && levelName[0])
	{
		staticGameUIPanel->SetVisible(false);
		staticGameUIPanel->SetPaintBackgroundEnabled(false);

	
		staticClientDLLPanel->SetVisible(true);
		staticClientDLLPanel->SetMouseInputEnabled(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shows any GameUI related panels
//-----------------------------------------------------------------------------
void CBaseUI::ActivateGameUI()
{
	staticGameUIFuncs->ActivateGameUI();
	vgui::surface()->SetCursor( vgui::dc_arrow );
	staticGameUIPanel->SetVisible(true);

	staticClientDLLPanel->SetVisible(false);
	staticClientDLLPanel->SetMouseInputEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Hides the game console (but not the complete GameUI!)
//-----------------------------------------------------------------------------
void CBaseUI::HideConsole()
{
	staticGameConsole->Hide();
}


void CBaseUI::ShowConsole()
{
	staticGameConsole->Activate();

	staticClientDLLPanel->SetVisible(false);
	staticClientDLLPanel->SetMouseInputEnabled(false);

}

bool CBaseUI::IsGameUIVisible() 
{
	return staticGameUIFuncs->IsGameUIActive();
}


//-----------------------------------------------------------------------------
// Purpose: Returns 1 if the key event is handled, 0 if the engine should handle it
//-----------------------------------------------------------------------------

int CBaseUI::Key_Event(int down, int keynum, const char *pszCurrentBinding)
{
	// let the engine handle the console keys
	if (keynum == '~' || keynum == '`')
		return 0;

	// ESCAPE toggles game ui
	if (down && keynum == K_ESCAPE)
	{
		const char *levelName = engineClient->GetLevelName();

		if (staticGameUIFuncs->IsGameUIActive() && levelName && levelName[0])
		{
			HideGameUI();

		}
		else
		{
			ActivateGameUI();
		}
		return 1;
	}
	

	if( vgui::surface()->NeedKBInput() )
	{
		return 1;
	}
	else
	{
		return 0;
	}

}


//-----------------------------------------------------------------------------
// Purpose: paints all the vgui elements
//-----------------------------------------------------------------------------
void CBaseUI::Paint(int x, int y, int right, int bottom)
{
	VPROF( "CBaseUI::Paint" );
	if (!staticPanel)
		return;

	// setup the base panel to cover the screen
	vgui::VPANEL pVPanel = vgui::surface()->GetEmbeddedPanel();
	if (!pVPanel)
		return;


	staticGameUIFuncs->RunFrame();
	vgui::ivgui()->RunFrame();

	//g_BaseUISurface.SetScreenBounds(x, y, right - x, bottom - y);
	vgui::Panel *panel = staticPanel;
	panel->SetBounds(0, 0, right, bottom);
	panel->Repaint();

//TRACE_FUNCTION_STARTTIMER();
	// paint everything 
	vgui::surface()->PaintTraverse(pVPanel);

//TRACE_FUNCTION_ENDTIMER("CBaseUI::Paint", 20)
}




//-----------------------------------------------------------------------------
// Purpose: Implements IEngineVGui interface
//-----------------------------------------------------------------------------
class CEngineVGui : public IEngineVGui
{
public:
	vgui::VPANEL		GetPanel( VGUIPANEL type );
};


//-----------------------------------------------------------------------------
// Purpose: Retrieve specified root panel
// Input  : type - 
// Output : vgui::Panel
//-----------------------------------------------------------------------------
vgui::VPANEL CEngineVGui::GetPanel( VGUIPANEL type )
{
	switch ( type )
	{
	default:
	case PANEL_ROOT:
		return staticPanel->GetVPanel();
	case PANEL_CLIENTDLL:
		return staticClientDLLPanel->GetVPanel();
	case PANEL_GAMEUIDLL:
		return staticGameUIPanel->GetVPanel();
	case PANEL_TOOLS:
		return staticEngineToolsPanel->GetVPanel();
	}
}

EXPOSE_SINGLE_INTERFACE( CEngineVGui, IEngineVGui, VENGINE_VGUI_VERSION );

