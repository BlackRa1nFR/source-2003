//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#define PROTECTED_THINGS_DISABLE

#include "..\common\winlite.h"
#include <stdio.h>

#include "ModList.h"

#include <vgui/VGUI.h>
#include <vgui/ISystem.h>
#include <KeyValues.h>
#include <vgui_controls/Controls.h>

//-----------------------------------------------------------------------------
// Purpose: Singleton accessor
//-----------------------------------------------------------------------------
CModList &ModList()
{
	static CModList s_ModList;
	return s_ModList;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CModList::CModList()
{

	ParseInstalledMods();
}

//-----------------------------------------------------------------------------
// Purpose: returns number of mods 
//-----------------------------------------------------------------------------
int CModList::ModCount()
{
	return m_ModList.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CModList::GetModDir(int index)
{
	return m_ModList[index].modName;
}


//-----------------------------------------------------------------------------
// Purpose: Searches half-life directory for mods
//-----------------------------------------------------------------------------
void CModList::ParseInstalledMods()
{
	char szGameDirectory[MAX_PATH];
	char szSearchPath[MAX_PATH + 5];

	// if we're running under steam, use it's game list
	ParseSteamMods();
	
	// get half-life directory
	if (!vgui::system()->GetRegistryString("HKEY_LOCAL_MACHINE\\Software\\Valve\\Half-life\\InstallPath", szGameDirectory, sizeof(szGameDirectory)))
	{
		// fallback to getting the cstrike retail directory
		if (!vgui::system()->GetRegistryString("HKEY_LOCAL_MACHINE\\Software\\Sierra OnLine\\Setup\\CSTRIKE\\Directory", szGameDirectory, sizeof(szGameDirectory)))
		{
			return;
		}
	}

	strcpy(szSearchPath, szGameDirectory);
	strcat(szSearchPath, "\\*.*");

	WIN32_FIND_DATA wfd;
	HANDLE hResult;
	memset(&wfd, 0, sizeof(WIN32_FIND_DATA));
	
	hResult = FindFirstFile( szSearchPath, &wfd);
	if (hResult != INVALID_HANDLE_VALUE)
	{
		BOOL bMoreFiles;
		while (1)
		{
			if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
				 (strnicmp(wfd.cFileName, ".",1) ) )
			{
				// Check for dlls\*.dll
				char	szDllDirectory[MAX_PATH + 16];
				sprintf(szDllDirectory, "%s\\%s\\liblist.gam", szGameDirectory, wfd.cFileName);

				WIN32_FIND_DATA wfd2;
				HANDLE hResult2;
				memset(&wfd2, 0, sizeof(WIN32_FIND_DATA));

				hResult2 = FindFirstFile(szDllDirectory, &wfd2);
				if (hResult2 != INVALID_HANDLE_VALUE)
				{
					// Add the game directory.
					strlwr(wfd.cFileName);

					mod_t mod;

					strncpy(mod.modName, wfd.cFileName, sizeof(mod.modName) - 1);
					mod.modName[sizeof(mod.modName) - 1] = 0;

					m_ModList.AddToTail(mod);
					
/*
					CMod *pnew = ParseFromLibList( wfd.cFileName, szDllDirectory );
					if ( pnew )
					{
						pnew->next = g_pModList;
						g_pModList = pnew;

						if ( !stricmp( wfd.cFileName, "VALVE" ) )
							g_pHalfLife = pnew;
					}
					FindClose( hResult2 );
*/
				}
			}
			bMoreFiles = FindNextFile(hResult, &wfd);
			if (!bMoreFiles)
				break;
		}
		
		FindClose(hResult);
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets list of steam games we can filter for
//-----------------------------------------------------------------------------
void CModList::ParseSteamMods()
{
	KeyValues *file = new KeyValues("GameInfo");
	if (file->LoadFromFile((IBaseFileSystem*)vgui::filesystem(), "resource/games/ClientGameInfo.vdf", "PLATFORM"))
	{
		for (KeyValues *kv = file->FindKey("Apps", true)->GetFirstSubKey(); kv != NULL; kv = kv->GetNextKey())
		{
			if (!kv->GetInt("NoServers"))
			{
				// add the game directory to the list
				mod_t mod;
				strncpy(mod.modName, kv->GetString("gamedir"), sizeof(mod.modName) - 1);
				mod.modName[sizeof(mod.modName) - 1] = 0;
				strlwr(mod.modName);
				m_ModList.AddToTail(mod);
			}
		}
	}

	file->deleteThis();
}
