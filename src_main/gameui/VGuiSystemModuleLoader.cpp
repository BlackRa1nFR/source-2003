//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include <stdio.h>

#include "VGuiSystemModuleLoader.h"
#include "Sys_Utils.h"
#include "IVGuiModule.h"
#include "ServerBrowser/IServerBrowser.h"

#include <vgui/IPanel.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>

#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>

#include "FileSystem.h"


// instance of class
CVGuiSystemModuleLoader g_VModuleLoader;

#ifdef GAMEUI_EXPORTS

#include "TaskBar.h"
extern CTaskbar *g_pTaskbar; // for SetParent call
#else
#include "..\platform\PlatformMainPanel.h"
extern CPlatformMainPanel *g_pMainPanel;
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CVGuiSystemModuleLoader, IVGuiModuleLoader, VGUIMODULELOADER_INTERFACE_VERSION, g_VModuleLoader);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVGuiSystemModuleLoader::CVGuiSystemModuleLoader()
{
	m_bModulesInitialized = false;
	m_bPlatformShouldRestartAfterExit = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CVGuiSystemModuleLoader::~CVGuiSystemModuleLoader()
{
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the module loader has acquired the platform mutex and loaded the modules
//-----------------------------------------------------------------------------
bool CVGuiSystemModuleLoader::IsPlatformReady()
{
	return m_bModulesInitialized;
}

//-----------------------------------------------------------------------------
// Purpose: sets up all the modules for use
//-----------------------------------------------------------------------------
void CVGuiSystemModuleLoader::InitializeAllModules(CreateInterfaceFn *factorylist, int factorycount)
{
	// Init vgui in the modules
	for (int i = 0; i < m_Modules.Size(); i++)
	{
		if (!m_Modules[i].moduleInterface->Initialize(factorylist, factorycount))
		{
			vgui::ivgui()->DPrintf2("Platform Error: module failed to initialize\n");
		}
	}

	// create a table of all the loaded modules
	CreateInterfaceFn *moduleFactories = (CreateInterfaceFn *)_alloca(sizeof(CreateInterfaceFn) * m_Modules.Size());
	for (i = 0; i < m_Modules.Size(); i++)
	{
		moduleFactories[i] = Sys_GetFactory(m_Modules[i].module);
	}

	// give the modules a chance to link themselves together
	for (i = 0; i < m_Modules.Size(); i++)
	{
		if (!m_Modules[i].moduleInterface->PostInitialize(moduleFactories, m_Modules.Size()))
		{
			vgui::ivgui()->DPrintf2("Platform Error: module failed to initialize\n");
		}
		
#ifdef GAMEUI_EXPORTS
		m_Modules[i].moduleInterface->SetParent(g_pTaskbar->GetVPanel());
		g_pTaskbar->AddTask(m_Modules[i].moduleInterface->GetPanel());
#else
		m_Modules[i].moduleInterface->SetParent(g_pMainPanel->GetVPanel());		
#endif
	}

	m_bModulesInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: Loads and initializes all the modules specified in the platform file
//-----------------------------------------------------------------------------
void CVGuiSystemModuleLoader::LoadPlatformModules(CreateInterfaceFn *factorylist, int factorycount, bool useSteamModules)
{
	// load platform menu
	KeyValues *kv = new KeyValues("Platform");
	if (!kv->LoadFromFile(vgui::filesystem(), "resource/PlatformMenu.vdf", "PLATFORM"))
		return;

	// walk the platform menu loading all the interfaces
	KeyValues *menuKeys = kv->FindKey("Menu", true);
	for (KeyValues *it = menuKeys->GetFirstSubKey(); it != NULL; it = it->GetNextKey())
	{
		// see if we should skip steam modules
		if (!useSteamModules && it->GetInt("SteamApp"))
			continue;

		// get copy out of steam cache
		const char *dllPath = it->GetString("dll");
		vgui::filesystem()->GetLocalCopy(dllPath);

		// load the dll
		char szDir[512];
		if (!vgui::filesystem()->GetLocalPath(dllPath, szDir))
		{
			vgui::ivgui()->DPrintf2("Platform Error: couldn't find %s, not loading\n", it->GetString("dll"));
			continue;
		}

		// make sure it's a valid dll
		CSysModule *mod = Sys_LoadModule(szDir);
		if (!mod)
		{
			vgui::ivgui()->DPrintf2("Platform Error: bad module '%s', not loading\n", it->GetString("dll"));
			continue;
		}

		// make sure we get the right version
		IVGuiModule *moduleInterface = (IVGuiModule *)Sys_GetFactory(mod)(it->GetString("interface"), NULL);
		if (!moduleInterface)
		{
			vgui::ivgui()->DPrintf2("Platform Error: module version ('%s, %s) invalid, not loading\n", it->GetString("dll"), it->GetString("interface"));
			continue;
		}

		// store off the module
		int newIndex = m_Modules.AddToTail();
		m_Modules[newIndex].module = mod;
		m_Modules[newIndex].moduleInterface = moduleInterface;
		m_Modules[newIndex].data = it;
	}
/*
    // find all the AddOns
	FileFindHandle_t findHandle = NULL;
	const char *filename = vgui::filesystem()->FindFirst("AddOns/*.", &findHandle);

    for ( ; filename != NULL ; filename = vgui::filesystem()->FindNext(findHandle))
//    while (filename)
    {
        // skip the . directories
        if (vgui::filesystem()->FindIsDirectory ( findHandle ) && filename[0] != '.')
        {
            // add this to list
            const char *gameName = filename;
            KeyValues* kv = new KeyValues(gameName);
            char fileName[512];
            sprintf(fileName, "AddOns/%s/%s.vdf", gameName, gameName);
            if (!kv->LoadFromFile(vgui::filesystem(), fileName, true, "PLATFORM"))
                continue;

            sprintf(fileName, "AddOns/%s/%s.dll", gameName, gameName);
            vgui::filesystem()->GetLocalCopy(fileName);
            CSysModule *mod = Sys_LoadModule(fileName);
            if (!mod)
                continue;

            // make sure we get the right version
            IVGuiModule *moduleInterface = (IVGuiModule *)Sys_GetFactory(mod)(kv->GetString("interface"), NULL);
            if (!moduleInterface)
                continue;

			// hide it from the Steam Platform Menu
			kv->SetInt("Hidden", 1);

            // store off the module
            int newIndex = m_Modules.AddToTail();
            m_Modules[newIndex].module = mod;
            m_Modules[newIndex].moduleInterface = moduleInterface;
            m_Modules[newIndex].data = kv;
        }
    }
	vgui::filesystem()->FindClose(findHandle);
*/

	InitializeAllModules(factorylist, factorycount);
}

//-----------------------------------------------------------------------------
// Purpose: gives all platform modules a chance to Shutdown gracefully
//-----------------------------------------------------------------------------
void CVGuiSystemModuleLoader::ShutdownPlatformModules()
{
	// static include guard to prevent recursive calls
	static bool runningFunction = false;
	if (runningFunction)
		return;

	runningFunction = true;

	// deactivate all the modules first
	DeactivatePlatformModules();

	// give all the modules notice of quit
	for (int i = 0; i < m_Modules.Size(); i++)
	{
		vgui::ivgui()->PostMessage(m_Modules[i].moduleInterface->GetPanel(), new KeyValues("Command", "command", "Quit"), NULL);
	}

	for (i = 0; i < m_Modules.Size(); i++)
	{
		m_Modules[i].moduleInterface->Shutdown();
	}

	runningFunction = false;
}

//-----------------------------------------------------------------------------
// Purpose: Deactivates all the modules (puts them into in inactive but recoverable state)
//-----------------------------------------------------------------------------
void CVGuiSystemModuleLoader::DeactivatePlatformModules()
{
	for (int i = 0; i < m_Modules.Size(); i++)
	{
		m_Modules[i].moduleInterface->Deactivate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reenables all the deactivated platform modules
//-----------------------------------------------------------------------------
void CVGuiSystemModuleLoader::ReactivatePlatformModules()
{
	for (int i = 0; i < m_Modules.Size(); i++)
	{
		m_Modules[i].moduleInterface->Reactivate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Disables and unloads platform
//-----------------------------------------------------------------------------
void CVGuiSystemModuleLoader::UnloadPlatformModules()
{
	for (int i = 0; i < m_Modules.Count(); i++)
	{
		Sys_UnloadModule(m_Modules[i].module);
	}

	m_Modules.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame
//-----------------------------------------------------------------------------
void CVGuiSystemModuleLoader::RunFrame()
{
}

//-----------------------------------------------------------------------------
// Purpose: returns number of modules loaded
//-----------------------------------------------------------------------------
int CVGuiSystemModuleLoader::GetModuleCount()
{
	return m_Modules.Size();
}

//-----------------------------------------------------------------------------
// Purpose: returns the string menu name (unlocalized) of a module
//			moduleIndex is of the range [0, GetModuleCount())
//-----------------------------------------------------------------------------
const char *CVGuiSystemModuleLoader::GetModuleLabel(int moduleIndex)
{
	return m_Modules[moduleIndex].data->GetString("MenuName", "< unknown >");
}

//-----------------------------------------------------------------------------
// Purpose: brings the specified module to the foreground
//-----------------------------------------------------------------------------
bool CVGuiSystemModuleLoader::IsModuleHidden(int moduleIndex)
{
	return m_Modules[moduleIndex].data->GetInt("Hidden", 0);
}

//-----------------------------------------------------------------------------
// Purpose: brings the specified module to the foreground
//-----------------------------------------------------------------------------
bool CVGuiSystemModuleLoader::ActivateModule(int moduleIndex)
{
	if (!m_Modules.IsValidIndex(moduleIndex))
		return false;

	m_Modules[moduleIndex].moduleInterface->Activate();

#ifdef GAMEUI_EXPORTS
	if (g_pTaskbar)
	{
		wchar_t *wTitle;
		wchar_t w_szTitle[1024];

		wTitle = vgui::localize()->Find(m_Modules[moduleIndex].data->GetName());

		if(!wTitle)
		{
			vgui::localize()->ConvertANSIToUnicode(m_Modules[moduleIndex].data->GetName(),w_szTitle,sizeof(w_szTitle));
			wTitle = w_szTitle;
		}

		g_pTaskbar->SetTitle(m_Modules[moduleIndex].moduleInterface->GetPanel(),wTitle);
	
	}
#endif
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: activates a module by name
//-----------------------------------------------------------------------------
bool CVGuiSystemModuleLoader::ActivateModule(const char *moduleName)
{
	for (int i = 0; i < GetModuleCount(); i++)
	{
		if (!stricmp(GetModuleLabel(i), moduleName) || !stricmp(m_Modules[i].data->GetName(), moduleName))
		{
			ActivateModule(i);
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns a modules interface factory
//-----------------------------------------------------------------------------
CreateInterfaceFn CVGuiSystemModuleLoader::GetModuleFactory(int moduleIndex)
{
	return Sys_GetFactory(m_Modules[moduleIndex].module);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVGuiSystemModuleLoader::PostMessageToAllModules(KeyValues *message)
{
	for (int i = 0; i < m_Modules.Size(); i++)
	{
		vgui::ivgui()->PostMessage(m_Modules[i].moduleInterface->GetPanel(), message->MakeCopy(), NULL);
	}
	message->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: sets the the platform should update and restart when it quits
//-----------------------------------------------------------------------------
void CVGuiSystemModuleLoader::SetPlatformToRestart()
{
	m_bPlatformShouldRestartAfterExit = true;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
bool CVGuiSystemModuleLoader::ShouldPlatformRestart()
{
	return m_bPlatformShouldRestartAfterExit;
}
