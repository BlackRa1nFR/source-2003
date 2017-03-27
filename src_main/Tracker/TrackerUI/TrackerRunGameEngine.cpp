//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "winlite.h"
#include <process.h>

#include "IRunGameEngine.h"
#include "TrackerDialog.h"

#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_IVGui.h>
#include <VGUI_MessageBox.h>
#include <VGUI_KeyValues.h>

using namespace vgui;

class CTrackerRunGameEngine : public IRunGameEngine
{
public:
	// Returns true if the engine is running, false otherwise.
	virtual bool IsRunning()
	{
		return false;
	}

	// Adds text to the engine command buffer. Only works if IsRunning()
	// returns true on success, false on failure
	virtual bool AddTextCommand(const char *text)
	{
		return false;
	}

	// runs the engine with the specified command line parameters.  Only works if !IsRunning()
	// returns true on success, false on failure
	virtual bool RunEngine(const char *gameDir, const char *commandLineParams)
	{
		// Start half-life
		bool success = false;

		// quick hack, gamedir -> steam game name mapping
		const char *gameName = "0";
		if (!stricmp(gameDir, "cstrike"))
		{
			gameName = "1";
		}
		else if (!stricmp(gameDir, "tfc"))
		{
			gameName = "2";
		}
		else if (!stricmp(gameDir, "dmc"))
		{
			gameName = "3";
		}
		else if (!stricmp(gameDir, "dod"))
		{
			gameName = "4";
		}

		char gameParam[256];
		_snprintf(gameParam, sizeof(gameParam), "%s 0 \"%s\"", gameName, commandLineParams);

		const char *steamPath = getenv("SteamInstallPath");
		char path[1024];
		
		// save settings before we go in, this will save the window we are chatting in if any.
		CTrackerDialog::GetInstance()->PostMessage(CTrackerDialog::GetInstance(), new KeyValues("SaveUserData"), NULL);

		if (steamPath)
		{
			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory( &si, sizeof(si) );
			si.cb = sizeof(si);
			ZeroMemory( &pi, sizeof(pi) );
			_snprintf(path, sizeof(path), "\"%s\\steam.exe\" %s", steamPath, gameParam);

			success = ::CreateProcess(NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

			// err = ::_spawnl(_P_NOWAIT, path, path, gameParam, commandLineParams, NULL);
			
			// Close process and thread handles. 
			CloseHandle( pi.hProcess );
			CloseHandle( pi.hThread );
		}
		else
		{
			// we're not running in steam, look in the directory below us
			// Run the game
			strcpy(path, "..\\hl.exe");
			success = ::_spawnl(_P_NOWAIT, path, path, " -console", commandLineParams, NULL);
		}

		if (!success)
		{
			char errorString[1024];
			strcpy(path, "<install path not found>");
			_snprintf(errorString, sizeof(errorString), "Could not Run %s", path);

			vgui::MessageBox *box = new vgui::MessageBox("Tracker - Error joining game", errorString, false);
			box->DoModal();
			return false;
		}

		return true;
	}

	// returns true if the player is currently in a game
	virtual bool IsInGame()
	{
		return false;
	}

	// gets information about the server the engine is currently connected to
	// returns true on success, false on failure
	virtual bool GetGameInfo(char *infoBuffer, int bufferSize)
	{
		return false;
	}

	// we already know our userID so ignore
	virtual void SetTrackerUserID(int userID)
	{
	}

	virtual const char *GetUserName(int userID)
	{
		return NULL;
	}

};

EXPOSE_SINGLE_INTERFACE(CTrackerRunGameEngine, IRunGameEngine, RUNGAMEENGINE_INTERFACE_VERSION);
