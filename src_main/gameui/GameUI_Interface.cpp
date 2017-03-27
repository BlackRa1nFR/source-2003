//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements all the functions exported by the GameUI dll
//
// $NoKeywords: $
//=============================================================================

#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <io.h>
#include <tier0/dbg.h>
#include <direct.h>

#ifdef SendMessage
#undef SendMessage
#endif

#include "FileSystem.h"
#include "GameUI_Interface.h"
#include "Sys_Utils.h"
#include "string.h"
#include "vstdlib/icommandline.h"

// version check includes
/*/#include "Socket.h"
//#include "proto_oob.h"
//#include "MasterVersionCheck.h"
#include "msgbuffer.h"
#include "version.h"
#include "utlvector.h"
#include "utlsymbol.h"
#include "Random.h"
#include "INetApi.h"
#include "Steam.h"
*/


// interface to engine
#include "EngineInterface.h"

//#include "../common/EngineSurface.h"
//#include "../common/SteamCommon.h"
//#include "../common/ValidateNewValveCDKeyClient.h"
#include "keydefs.h"
#include "VGuiSystemModuleLoader.h"

#include "GameConsole.h"
#include "LoadingDialog.h"
//#include "CDKeyEntryDialog.h"
#include "ModInfo.h"
#include "cl_dll/IGameClientExports.h"

#include "IGameUIFuncs.h"
#include <IEngineVGUI.h>
#include "BaseUI/IBaseUI.h"

// vgui2 interface
// note that GameUI project uses ..\vgui2\include, not ..\utils\vgui\include
#include "BasePanel.h"

#include <vgui/Cursor.h>
#include <KeyValues.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/PHandle.h>

//#include "engine/IEngineSound.h"
#include "tier0/vcrmode.h"


// in-game UI elements
#include "Taskbar.h"
#include "Friends/IFriendsUser.h"
#include "SteamPasswordDialog.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

IGameUIFuncs *gameuifuncs = NULL;
IEngineVGui *enginevguifuncs = NULL;
vgui::ISurface *enginesurfacefuncs = NULL;
IBaseUI *baseuifuncs = NULL;
IFriendsUser *g_pFriendsUser = NULL;

// SRC functions
// interface to the engine
#include "tier0/dbg.h"
#include "engine/IEngineSound.h"


IVEngineClient *engine = NULL;
IEngineSound *enginesound = NULL;
ICvar *cvar = NULL;



// interface to the engine
//cl_enginefunc_t gEngfuncs;
//cl_enginefunc_t *engine = NULL; // a pointer to gEngfuncs, used so gameui in SRC and GoldSrc can be compatible

// interface to the base system used by demo player
//IBaseSystem	* g_pSystemWrapper = NULL;

// interface to the app
CTaskbar *g_pTaskbar = NULL;

static CBasePanel *staticPanel = NULL;

class CGameUI;
CGameUI *g_pGameUI = NULL;

class CLoadingDialog;
vgui::DHANDLE<CLoadingDialog> g_hLoadingDialog;

static CGameUI g_GameUI;

static WHANDLE g_hMutex = NULL;
static WHANDLE g_hWaitMutex = NULL;

static IGameClientExports *g_pGameClientExports = NULL;
IGameClientExports *GameClientExports()
{
	return g_pGameClientExports;
}

//-----------------------------------------------------------------------------
// Purpose: singleton accessor
//-----------------------------------------------------------------------------
CGameUI &GameUI()
{
	return g_GameUI;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameUI, IGameUI, GAMEUI_INTERFACE_VERSION, g_GameUI);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameUI::CGameUI()
{
	g_pGameUI = this;
	m_bTryingToLoadTracker = false;
	m_iGameIP = 0;
	m_iGamePort = 0;
	m_flProgressStartTime = 0.0f;
	m_pszCurrentProgressType = "";
	m_bActivatedUI = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameUI::~CGameUI()
{
	g_pGameUI = NULL;
}


void CGameUI::Initialize( CreateInterfaceFn *factories, int count )
{
//	TRACE_FUNCTION("CGameUI::Initialize");

/*	CreateInterfaceFn fileSystemFactory = factories[ 2 ];
	CreateInterfaceFn vguiFactory = factories[ 1 ];
	CreateInterfaceFn engineFactory = factories[ 0 ];
	CreateInterfaceFn clientFactory = factories[ 4 ];

	m_FactoryList[ 0 ] = Sys_GetFactoryThis();
	m_FactoryList[ 1 ] = engineFactory;
	m_FactoryList[ 2 ] = vguiFactory;
	m_FactoryList[ 3 ] = fileSystemFactory;
	m_FactoryList[ 4 ] = clientFactory;
	m_iNumFactories = count;

	vgui::VGui_InitInterfacesList("GameUI", m_FactoryList, count); // client factory may not be passed in
*/
	CreateInterfaceFn factory = factories[ 0 ];
	CreateInterfaceFn fileSystemFactory = factories[ 0 ];
	CreateInterfaceFn vguiFactory = factories[ 0 ];
	CreateInterfaceFn engineFactory = factories[ 0 ];
	CreateInterfaceFn clientFactory = factories[ 0 ];


	enginesound = (IEngineSound *)factory(IENGINESOUND_CLIENT_INTERFACE_VERSION, NULL);
	cvar		= (ICvar *)factory( VENGINE_CVAR_INTERFACE_VERSION, NULL );
	engine		= (IVEngineClient *)factory( VENGINE_CLIENT_INTERFACE_VERSION, NULL );

	m_FactoryList[ 0 ] = Sys_GetFactoryThis();
	m_FactoryList[ 1 ] = factory;
	m_iNumFactories = count;

	vgui::VGui_InitInterfacesList( "GameUI", m_FactoryList, 2 );

		// load localization file
	vgui::localize()->AddFile(vgui::filesystem(), "Resource/gameui_%language%.txt");

	// load localization file for kb_act.lst
	vgui::localize()->AddFile(vgui::filesystem(), "Resource/valve_%language%.txt");

	// setup base panel
	staticPanel = new CBasePanel();
	staticPanel->SetBounds(0, 0, 400, 300);
	staticPanel->SetPaintBorderEnabled(false);
	staticPanel->SetPaintBackgroundEnabled(true);
	staticPanel->SetPaintEnabled(false);
	staticPanel->SetVisible( true );
	staticPanel->SetMouseInputEnabled( false );
	staticPanel->SetKeyBoardInputEnabled( false );

	enginevguifuncs = (IEngineVGui * )engineFactory( VENGINE_VGUI_VERSION, NULL);
	if(enginevguifuncs)
	{
		vgui::VPANEL rootpanel = enginevguifuncs->GetPanel(PANEL_GAMEUIDLL);
		staticPanel->SetParent(rootpanel);
	}

	gameuifuncs = (IGameUIFuncs * )engineFactory( VENGINE_GAMEUIFUNCS_VERSION, NULL );
	enginesurfacefuncs = (vgui::ISurface * )engineFactory(VGUI_SURFACE_INTERFACE_VERSION,NULL);
	baseuifuncs = (IBaseUI *)engineFactory( BASEUI_INTERFACE_VERSION, NULL);
	if (clientFactory)
	{
		g_pGameClientExports = (IGameClientExports *)clientFactory(GAMECLIENTEXPORTS_INTERFACE_VERSION, NULL);
	}
}
/*
//-----------------------------------------------------------------------------
// Purpose: sends a query to the master server to check if this client is up to date
//-----------------------------------------------------------------------------
void CGameUI::MasterVersionCheckQuery()
{
	m_pMaster = new CSocket( "version check", -1 );
	// The socket will delete the handler
	int bytecode = M2A_MASTERSERVERS;

	m_pMaster->AddMessageHandler(new CMasterVersionChkMsgHandler( CMsgHandler::MSGHANDLER_BYTECODE, &bytecode));


	// load masters from config file
	CUtlVector<CUtlSymbol> m_MasterServerNames;	// full names of master servers

	KeyValues *kv = new KeyValues("MasterServers");
	if (kv->LoadFromFile(vgui::filesystem(), "Servers/MasterServers.vdf", true))
	{
		// iterate the list loading all the servers
		for (KeyValues *srv = kv->GetFirstSubKey(); srv != NULL; srv = srv->GetNextKey())
		{
			m_MasterServerNames.AddToTail(srv->GetString("addr"));
		}
	}
	else
	{
		assert(!("Could not load file Servers/MasterServers.vdf."));
	}

	// make sure we have at least one master listed
	if (m_MasterServerNames.Count() < 1)
	{
		// add the default master
		m_MasterServerNames.AddToTail("half-life.west.won.net:27010");
	}

	// choose a server at random
	netadr_t m_MasterAddress;	// Address of master server
	int serverIndex = RandomLong(0, m_MasterServerNames.Count() - 1);
	net->StringToAdr(m_MasterServerNames[serverIndex].String(), &m_MasterAddress);

	CMsgBuffer *buffer = m_pMaster->GetSendBuffer();
	assert( buffer );
	if ( !buffer )
		return;

	char version[32];
	char product[32];

	memset( version, 0x0, sizeof(version) );
	memset( product, 0x0, sizeof(product) );

	GetUpdateVersion( product, version );

	if( strlen(product) > 0 && strlen(version) > 0 )
	{
		buffer->Clear();
		// Write query string
		buffer->WriteByte( A2M_GETMASTERSERVERS );
		// Write version string
		buffer->WriteString( version );
		// write product string
		buffer->WriteString( product );

		m_pMaster->SendMessage(&m_MasterAddress);
	}
}


#define VERSION_KEY			"PatchVersion="
#define PRODUCT_KEY			"ShortTitle="
#define PRODUCT_STRING		"HALFLIFE"

//-----------------------------------------------------------------------------
// Purpose: Parses sierra.inf/steam.inf for version info strings
//-----------------------------------------------------------------------------
void CGameUI::GetUpdateVersion( char *pszProd, char *pszVer)
{
	char	szFileName[ MAX_PATH ];
	char	buffer[ 16384 ];
	unsigned int		bufsize = sizeof( buffer );
	DWORD	dwResult;

	// Read it from the .inf file
	memset( buffer, 0, bufsize );

	if ( vgui::filesystem()->FileExists("valve.inf") )
	{
		vgui::filesystem()->GetLocalPath("valve.inf", szFileName, MAX_PATH); // GetPrivateProfileSection() is an OS call...needs the whole file at hand.
		dwResult = GetPrivateProfileSection( "Ident", buffer, bufsize, szFileName ); 
	}
	else
	{
		return; // didn't find a master file to open
	}

	// Get the version number
	strcpy( pszVer, VERSION_STRING );
	if ( dwResult > 0 && dwResult != ( bufsize - 2 ) )
	{
		//
		// Read 
		char *pbuf = buffer;

		while ( 1 )
		{
			if ( ( (int)(pbuf - buffer) - (int)strlen( VERSION_KEY ) ) > (int)dwResult )
				break;

			if ( strnicmp( pbuf, VERSION_KEY, strlen( VERSION_KEY ) ) )
			{
				pbuf++;
				continue;
			}

			pbuf += strlen( VERSION_KEY );

			strcpy( pszVer, pbuf );
			break;
		}
	}

	// Get the product name
	strcpy( pszProd, PRODUCT_STRING );
	if ( dwResult > 0 && dwResult != ( bufsize - 2 ) )
	{
		//
		// Read 
		char *pbuf = buffer;

		while ( 1 )
		{
			if ( ( (int)(pbuf - buffer) - (int)strlen( PRODUCT_KEY ) ) > (int)dwResult )
				break;

			if ( strnicmp( pbuf, PRODUCT_KEY, strlen( PRODUCT_KEY ) ) )
			{
				pbuf++;
				continue;
			}

			pbuf += strlen( PRODUCT_KEY );

			strcpy( pszProd, pbuf );
			break;
		}
	}
}

*/

//-----------------------------------------------------------------------------
// Purpose: Callback function; sends platform Shutdown message to specified window
//-----------------------------------------------------------------------------
int __stdcall SendShutdownMsgFunc(WHANDLE hwnd, int lparam)
{
	Sys_PostMessage(hwnd, Sys_RegisterWindowMessage("ShutdownValvePlatform"), 0, 1);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Called to setup the game UI
//-----------------------------------------------------------------------------
void CGameUI::Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion, IBaseSystem *system)
{
//	TRACE_FUNCTION("CGameUI::Start");
//	m_pMaster = NULL;

	// copy the engine interface
//	memcpy(&gEngfuncs, engineFuncs, sizeof(gEngfuncs));
//	engine = &gEngfuncs;

	// set SystemWrapper for demo player
//	g_pSystemWrapper = system;

	// load mod info
	ModInfo().LoadCurrentGameInfo();


	// Determine Tracker location.
	// ...If running with Steam, Tracker is in a well defined location relative to the game dir.  Use it if there.
	// ...Otherwise get the tracker location from the registry key
	if (FindPlatformDirectory(m_szPlatformDir, sizeof(m_szPlatformDir)))
	{
		// add the tracker directory to the search path
		// add localized version first if we're not in english
		char language[128];
		if (vgui::system()->GetRegistryString("HKEY_LOCAL_MACHINE\\Software\\Valve\\Steam\\Language", language, sizeof(language)))
		{
			if (strlen(language) > 0 && stricmp(language, "english"))
			{
				char path[256];
				sprintf(path, "platform_%s", language);
				vgui::filesystem()->AddSearchPath(path, "PLATFORM");
			}
		}
		vgui::filesystem()->AddSearchPath("platform", "PLATFORM");

		// setup config file directory
		char szConfigDir[512];

		strcpy(szConfigDir, m_szPlatformDir);
		strcat(szConfigDir, "config");

		/*
		// make sure the path exists
		_finddata_t findData;
		long findHandle = _findfirst(steamPath, &findData);
		if (steamPath && findHandle != -1)
		{
			// put the config dir directly under steam
			_snprintf(szConfigDir, sizeof(szConfigDir), "%s/config", steamPath);
			_findclose(findHandle);
		}
		else
		{
			// we're not running steam, so just put the config dir under the platform
			_snprintf(szConfigDir, sizeof(szConfigDir), "%sconfig", m_szPlatformDir);
		}
		*/

		// add the path
		vgui::filesystem()->AddSearchPath(szConfigDir, "CONFIG");
		// make sure the config directory has been created
		_mkdir(szConfigDir);

		vgui::ivgui()->DPrintf("Platform config directory: %s\n", szConfigDir);

		// user dialog configuration
		vgui::system()->SetUserConfigFile("InGameDialogConfig.vdf", "CONFIG");

		// localization
		vgui::localize()->AddFile(vgui::filesystem(), "Resource/platform_%language%.txt");
		vgui::localize()->AddFile(vgui::filesystem(), "Resource/vgui_%language%.txt");


		//!! hack to work around problem with userinfo not being uploaded (and therefore *Tracker field) 
		//!! this is done to make sure the *tracker userinfo field is set before we connect so that it
		//!! will get communicated to the server
		//!! this needs to be changed to a system where it is communicated to server when known but not before
		
		//!! addendum: this may very happen now with the platform changes; needs to be tested before this code
		//!! can be removed
		{
			// get the last known userID from the registry and set it in our userinfo string
			HKEY key;
			DWORD bufSize = sizeof(m_szPlatformDir);
			unsigned int lastUserID = 0;
			bufSize = sizeof(lastUserID);
			if (ERROR_SUCCESS == g_pVCR->Hook_RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Valve\\Tracker", 0, KEY_READ, &key))
			{
				g_pVCR->Hook_RegQueryValueEx(key, "LastUserID", NULL, NULL, (unsigned char *)&lastUserID, &bufSize);

				// close the registry key
				g_pVCR->Hook_RegCloseKey(key);
			}
			if (lastUserID)
			{
				char buf[32];
				sprintf(buf, "%d", lastUserID);
				engine->PlayerInfo_SetValueForKey("*tracker", buf);
			}
		}
	}

	// task bar - needs to be first thing created
	g_pTaskbar = new CTaskbar(staticPanel,"TaskBar");
	g_pTaskbar->SetVisible(false);

// FOR SRC
//	vgui::surface()->SetWorkspaceInsets( 0, 0, 0, g_pTaskbar->GetTall() );

	// Start loading tracker
	if (m_szPlatformDir[0] != 0)
	{
		vgui::ivgui()->DPrintf2("Initializing platform...\n");

		// open a mutex
		Sys_SetLastError(SYS_NO_ERROR);

		// primary mutex is the platform.exe name
		char szExeName[sizeof(m_szPlatformDir) + 32];
		sprintf(szExeName, "%splatform.exe", m_szPlatformDir);
		// convert the backslashes in the path string to be forward slashes so it can be used as a mutex name
		for (char *ch = szExeName; *ch != 0; ch++)
		{
			*ch = tolower(*ch);
			if (*ch == '\\')
			{
				*ch = '/';
			}
		}

		g_hMutex = Sys_CreateMutex("ValvePlatformUIMutex");
		g_hWaitMutex = Sys_CreateMutex("ValvePlatformWaitMutex");
		if (g_hMutex == NULL || g_hWaitMutex == NULL || Sys_GetLastError() == SYS_ERROR_INVALID_HANDLE)
		{
			// error, can't get handle to mutex
			if (g_hMutex)
			{
				Sys_ReleaseMutex(g_hMutex);
			}
			if (g_hWaitMutex)
			{
				Sys_ReleaseMutex(g_hWaitMutex);
			}
			g_hMutex = NULL;
			g_hWaitMutex = NULL;
			Error("Tracker Error: Could not access Tracker, bad mutex\n");
			return;
		}
		unsigned int waitResult = Sys_WaitForSingleObject(g_hMutex, 0);
		if (!(waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED))
		{
			// mutex locked, need to close other tracker

			// get the wait mutex, so that tracker.exe knows that we're trying to acquire ValveTrackerMutex
			waitResult = Sys_WaitForSingleObject(g_hWaitMutex, 0);
			if (waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED)
			{
				Sys_EnumWindows(SendShutdownMsgFunc, 1);
			}
		}
		m_bTryingToLoadTracker = true;
		// now we are set up to check every frame to see if we can Start tracker
	}

	staticPanel->SetBackgroundRenderState(CBasePanel::BACKGROUND_DESKTOPIMAGE);

	// start mp3 playing
	//engine->pfnClientCmd("mp3 loop media/gamestartup.mp3\n");

	// SRC version
	//engine->ClientCmd("loop media/gamestartup.mp3\n");

}

//-----------------------------------------------------------------------------
// Purpose: Finds which directory the platform resides in
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameUI::FindPlatformDirectory(char *platformDir, int bufferSize)
{
	platformDir[0] = '\0';

	// check environment first
	if (getenv("ValvePlatformMutex"))
	{
		// strip the exe name from the end of the mutex string
		char szPlatformPath[MAX_PATH];
		strncpy(szPlatformPath, getenv("ValvePlatformMutex"), sizeof(szPlatformPath) - 1);
		szPlatformPath[sizeof(szPlatformPath) - 1] = 0;
		
		// walk backwards in the string until we find a /
		for (int i = strlen(szPlatformPath); i > 0; i--)
		{
			if (szPlatformPath[i] != '/')
			{
				szPlatformPath[i] = 0;
			}
			else
			{
				break;
			}
		}

		strcpy(platformDir, szPlatformPath);
	}

	// check for ServerBrowser DLL on local area
	if (platformDir[0] == '\0')
	{
		char *pszServerBrowserDLL = "..\\platform\\servers\\serverbrowser.dll";

		// Require that we find the ServerBrowser DLL.
		if (vgui::filesystem()->FileExists(pszServerBrowserDLL))
		{
			char szPlatformPath[MAX_PATH], szFinalPath[MAX_PATH];
			vgui::filesystem()->GetLocalPath(pszServerBrowserDLL, szPlatformPath);
			szPlatformPath[MAX_PATH - 1] = 0;

			// remove any \..\ from the path
			szFinalPath[0] = 0;
			int finalPathPos = 0;
			for (int i = 0; szPlatformPath[i] != 0; i++)
			{
				if (!strncmp(szPlatformPath + i, "\\..\\", 4))
				{
					// skip over the "\\.."
					i += 3;
					
					// walk the final dir back until the previous '\\'
					while (szFinalPath[finalPathPos] != '\\' && finalPathPos)
					{
						finalPathPos--;
					}
				}

				szFinalPath[finalPathPos++] = szPlatformPath[i];
			}
			char *binpos = strstr(szFinalPath, "servers\\serverbrowser.dll");
			if (binpos)
			{
				*binpos = 0;
				strcpy(platformDir, szFinalPath);
			}
		}
	}

	if (platformDir[0] != 0)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the user has a valid CD key; prompts for one to
//			be entered if not
//-----------------------------------------------------------------------------
void CGameUI::ValidateCDKey(bool force, bool inConnect)
{
	return;
	
	// if we're running under steam, we don't need a cd key
	if (CommandLine()->FindParm("-steam"))
		return;

/*	// this function exported by cdkey.obj (we don't have the source code)
	extern int SimpleCDCheck( const char *cdkey );


	char cdkey[255];
	if (!force && (!engine->CheckParm("-forcevalve", NULL) && vgui::system()->GetRegistryString("HKEY_CURRENT_USER\\Software\\Valve\\Half-Life\\Settings\\Key", cdkey, sizeof(cdkey)) 
			&& strlen(cdkey) > 0) )
	{
		if ( SimpleCDCheck(cdkey) == 1 )
		{
			// success
			// vgui::ivgui()->DPrintf2("CD KEY %s VALIDATED\n", cdkey);
			return;
		}
	}
	else if (!force && vgui::system()->GetRegistryString("HKEY_CURRENT_USER\\Software\\Valve\\Half-Life\\Settings\\ValveKey", cdkey, sizeof(cdkey)) 
			&& strlen(cdkey) > 0)
	{
		if ( SteamWeakVerifyNewValveCDKey(cdkey) == eSteamErrorNone )
		{
			// success
			// vgui::ivgui()->DPrintf2("CD KEY %s VALIDATED\n", cdkey);
			return;
		}
	}

	else if (!force && vgui::system()->GetRegistryString("HKEY_CURRENT_USER\\Software\\Valve\\Half-Life\\Settings\\yeK1", cdkey, sizeof(cdkey)))
	{
		// TODO: Add weak encrypted key validation check here!
		if (strlen(cdkey) == 254)
		{
			if(vgui::system()->GetRegistryString("HKEY_CURRENT_USER\\Software\\Valve\\Half-Life\\Settings\\yeK2", cdkey, sizeof(cdkey)))
			{
				if(strlen(cdkey) == 108)
				{
					// success
					// vgui::ivgui()->DPrintf2("CD KEY %s VALIDATED\n", cdkey);
					return;
				}
			}
		}
	}

	// prompt for new cdkey
	vgui::ivgui()->DPrintf2("CD KEY INVALID\n");

	if (!m_hCDKeyEntryDialog.Get())
	{
		m_hCDKeyEntryDialog = new CCDKeyEntryDialog( staticPanel, inConnect );
	}

	m_hCDKeyEntryDialog->Activate();
	m_hCDKeyEntryDialog->MoveToCenterOfScreen();

	enginesurfacefuncs->RestrictPaintToSinglePanel(m_hCDKeyEntryDialog->GetVPanel());
	enginesurfacefuncs->SetModalPanel(m_hCDKeyEntryDialog->GetVPanel());
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Called to Shutdown the game UI system
//-----------------------------------------------------------------------------
void CGameUI::Shutdown()
{
	// notify all the modules of Shutdown
	g_VModuleLoader.ShutdownPlatformModules();

	// unload the modules them from memory
	g_VModuleLoader.UnloadPlatformModules();

	// free mod info
	ModInfo().FreeModInfo();
	
	// release platform mutex
	// close the mutex
	if (g_hMutex)
	{
		Sys_ReleaseMutex(g_hMutex);
	}
	if (g_hWaitMutex)
	{
		Sys_ReleaseMutex(g_hWaitMutex);
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the game UI is currently visible
//-----------------------------------------------------------------------------
bool CGameUI::IsGameUIActive()
{
	if ( m_bActivatedUI )
	{
		return staticPanel->IsVisible();
	}
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: Activate the game UI
//-----------------------------------------------------------------------------
int CGameUI::ActivateGameUI()
{
	if (IsGameUIActive())
		return 1;

	m_bActivatedUI = true;

//	TRACE_FUNCTION("CGameUI::ActivateGameUI");

	// hide/show the main panel to Activate all game ui
	staticPanel->SetVisible(true);
	g_pTaskbar->SetVisible(true);

	// pause the game
	engine->ClientCmd("setpause");

	// notify taskbar
	if (g_pTaskbar)
	{
		g_pTaskbar->OnGameUIActivated();
	}

	// return that things have been handled
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Activate the demo player UI
//-----------------------------------------------------------------------------
int CGameUI::ActivateDemoUI()
{
	if (g_pTaskbar)
	{
		g_pTaskbar->OnOpenDemoDialog();
		return 1;
	}
	else
	{
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hides the game ui, in whatever state it's in
//-----------------------------------------------------------------------------
void CGameUI::HideGameUI()
{
//	TRACE_FUNCTION("CGameUI::HideGameUI");
	// we can't hide the UI if we're not in a level
	const char *levelName = engine->GetLevelName();
	if (levelName && levelName[0])
	{
		//show both the background panel and the taskbar 
		staticPanel->SetVisible(false);
		g_pTaskbar->SetVisible(false);

		// unpause the game
		engine->ClientCmd("unpause");
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns 1 on the game UI having exclusive input, false otherwise
//-----------------------------------------------------------------------------
int CGameUI::HasExclusiveInput()
{
	return IsGameUIActive();
}

//-----------------------------------------------------------------------------
// Purpose: paints all the vgui elements
//-----------------------------------------------------------------------------
void CGameUI::RunFrame()
{
	// resize the background panel to the screen size
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	staticPanel->SetSize(wide,tall);

	// Run frames
	g_VModuleLoader.RunFrame();

	if (g_pTaskbar)
	{
		g_pTaskbar->RunFrame();
	}

	if (m_bTryingToLoadTracker && g_hMutex && g_hWaitMutex)
	{
		// try and load tracker
		unsigned int waitResult = Sys_WaitForSingleObject(g_hMutex, 0);
		if (waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED)
		{
			// we got the tracker mutex, so load tracker
			// clear the loading flag
			m_bTryingToLoadTracker = false;

			g_VModuleLoader.LoadPlatformModules(m_FactoryList, m_iNumFactories, false);

			// get our own interfaces
			for (int i = 0; i < g_VModuleLoader.GetModuleCount(); i++)
			{
				if (!g_pFriendsUser && g_VModuleLoader.GetModuleFactory(i) )
				{
					g_pFriendsUser = (IFriendsUser *)(g_VModuleLoader.GetModuleFactory(i))(FRIENDSUSER_INTERFACE_VERSION, NULL);
				}
			}

			// release the wait mutex
			Sys_ReleaseMutex(g_hWaitMutex);

			// notify the game of our game name
			const char *fullGamePath = engine->GetGameDirectory();
			const char *pathSep = strrchr( fullGamePath, '/' );
			if ( !pathSep )
			{
				pathSep = strrchr( fullGamePath, '\\' );
			}
			if ( pathSep )
			{
				g_VModuleLoader.PostMessageToAllModules(new KeyValues("ActiveGameName", "name", pathSep + 1));
			}

			// notify the ui of a game connect if we're already in a game
			if (m_iGameIP)
			{
				g_VModuleLoader.PostMessageToAllModules(new KeyValues("ConnectedToGame", "ip", m_iGameIP, "port", m_iGamePort));
			}
		}
	}

/*	if( m_pMaster )
	{
		m_pMaster->Frame();
	}
	*/

	if( vgui::surface()->GetModalPanel() )
	{
		vgui::surface()->PaintTraverse( staticPanel->GetVPanel());
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when the game connects to a server
//-----------------------------------------------------------------------------
void CGameUI::ConnectToServer(const char *game, int IP, int port)
{

	//engine->pfnClientCmd("mp3 stop\n");
	// SRC version
//	engine->ClientCmd("stop\n");
	baseuifuncs->HideGameUI();

	// start running our version query if we are not running steam
/*	if( !engine->CheckParm("-steam", NULL) )
	{
		MasterVersionCheckQuery();
	}
*/
	m_iGameIP = IP;
	m_iGamePort = port;

	g_VModuleLoader.PostMessageToAllModules(new KeyValues("ConnectedToGame", "ip", IP, "port", port));
}

//-----------------------------------------------------------------------------
// Purpose: Called when the game disconnects from a server
//-----------------------------------------------------------------------------
void CGameUI::DisconnectFromServer()
{
	ActivateGameUI();

	m_iGameIP = 0;
	m_iGamePort = 0;
	g_VModuleLoader.PostMessageToAllModules(new KeyValues("DisconnectedFromGame"));
}

//-----------------------------------------------------------------------------
// Purpose: activates the loading dialog on level load start
//-----------------------------------------------------------------------------
void CGameUI::LoadingStarted(const char *resourceType, const char *resourceName)
{
	g_VModuleLoader.PostMessageToAllModules(new KeyValues("LoadingStarted", "type", resourceType, "name", resourceName));

	if (!stricmp(resourceType, "transition"))
	{
		// activate the loading image
		staticPanel->SetBackgroundRenderState(CBasePanel::BACKGROUND_LOADINGTRANSITION);
	}
	else
	{
		staticPanel->SetBackgroundRenderState(CBasePanel::BACKGROUND_LOADING);
	}
}

//-----------------------------------------------------------------------------
// Purpose: closes any level load dialog
//-----------------------------------------------------------------------------
void CGameUI::LoadingFinished(const char *resourceType, const char *resourceName)
{
	// notify all the modules
	g_VModuleLoader.PostMessageToAllModules(new KeyValues("LoadingFinished", "type", resourceType, "name", resourceName));

	// stop drawing loading screen
	staticPanel->SetBackgroundRenderState(CBasePanel::BACKGROUND_DESKTOPIMAGE);

	// hide the UI
	baseuifuncs->HideGameUI();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameUI::StartProgressBar(const char *progressType, int progressSteps)
{
//	TRACE_FUNCTION("CGameUI::StartProgressBar");

	if (!g_hLoadingDialog.Get())
	{
		g_hLoadingDialog = new CLoadingDialog(staticPanel);
	}

	// close the start menu
	staticPanel->SetBackgroundRenderState(CBasePanel::BACKGROUND_LOADING);
	m_pszCurrentProgressType = progressType;
	if (m_flProgressStartTime < 0.001f)
	{
		m_flProgressStartTime = (float)vgui::system()->GetCurrentTime();
	}

	// open a loading dialog
	g_hLoadingDialog->SetProgressRange(0 , progressSteps); 
	g_hLoadingDialog->SetProgressPoint(0);
	g_hLoadingDialog->DisplayProgressBar(progressType, "invalid");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CGameUI::ContinueProgressBar(int progressPoint, float progressFraction)
{
	if (!g_hLoadingDialog.Get())
		return 0;

	g_hLoadingDialog->SetProgressPoint(progressPoint);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: stops progress bar, displays error if necessary
//-----------------------------------------------------------------------------
void CGameUI::StopProgressBar(bool bError, const char *failureReason, const char *extendedReason)
{
	if (!g_hLoadingDialog.Get() && bError)
	{
		g_hLoadingDialog = new CLoadingDialog(staticPanel);
	}

	if (!g_hLoadingDialog.Get())
		return;

	if (bError)
	{
		// turn the dialog to error display mode
		g_hLoadingDialog->DisplayError(failureReason,extendedReason);
	}
	else
	{
		// close loading dialog
		g_hLoadingDialog->Close();
		g_hLoadingDialog = NULL;
	}

	// stop drawing loading screen
	staticPanel->SetBackgroundRenderState(CBasePanel::BACKGROUND_DESKTOPIMAGE);
}

//-----------------------------------------------------------------------------
// Purpose: sets loading info text
//-----------------------------------------------------------------------------
int CGameUI::SetProgressBarStatusText(const char *statusText)
{
	if (!g_hLoadingDialog.Get())
		return 0;

	g_hLoadingDialog->SetStatusText(statusText);
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameUI::SetSecondaryProgressBar(float progress /* range [0..1] */)
{
	if (!g_hLoadingDialog.Get())
		return;

	g_hLoadingDialog->SetSecondaryProgress(progress);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameUI::SetSecondaryProgressBarText(const char *statusText)
{
	if (!g_hLoadingDialog.Get())
		return;

	g_hLoadingDialog->SetSecondaryProgressText(statusText);
}

//-----------------------------------------------------------------------------
// Purpose: If the user's Steam subscription expires during play and the user 
//			has not told Steam to cache their password, Steam needs to have 
//			them re-enter it.
//-----------------------------------------------------------------------------
void CGameUI::GetSteamPassword( const char *szAccountName, const char *szUserName )
{
	//TRACE_FUNCTION("CGameUI::GetSteamPassword");
	ActivateGameUI();

	CSteamPasswordDialog *pSteamPasswordDialog = new CSteamPasswordDialog(staticPanel, szAccountName, szUserName);

	// Center it, keeping requested size
	int x, y, ww, wt, wide, tall;
	vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
	pSteamPasswordDialog->GetSize(wide, tall);
	pSteamPasswordDialog->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));

	pSteamPasswordDialog->RequestFocus();

	pSteamPasswordDialog->SetVisible(true);
}

