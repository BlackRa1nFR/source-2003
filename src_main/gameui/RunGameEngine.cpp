//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "IRunGameEngine.h"
#include "EngineInterface.h"
#include <string.h>
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: Interface to running the engine from the UI dlls
//-----------------------------------------------------------------------------
class CRunGameEngine : public IRunGameEngine
{
public:
	// Returns true if the engine is running, false otherwise.
	virtual bool IsRunning()
	{
		return true;
	}

	// Adds text to the engine command buffer. Only works if IsRunning()
	// returns true on success, false on failure
	virtual bool AddTextCommand(const char *text)
	{
		engine->ClientCmd((char *)text);
		return true;
	}

	// runs the engine with the specified command line parameters.  Only works if !IsRunning()
	// returns true on success, false on failure
	virtual bool RunEngine(const char *gameName, const char *commandLineParams)
	{
		return false;
	}

	// returns true if the player is currently connected to a game server
	virtual bool IsInGame()
	{
		return engine->GetLevelName() && strlen(engine->GetLevelName()) > 0;
	}

	// gets information about the server the engine is currently connected to
	// returns true on success, false on failure
	virtual bool GetGameInfo(char *infoBuffer, int bufferSize)
	{
		//!! need to implement
		return false;
	}

	virtual void SetTrackerUserID(int trackerID)
	{
		char buf[32];
		sprintf(buf, "%d", trackerID);
		engine->PlayerInfo_SetValueForKey("*tracker", buf);
	}

	// iterates users
	// returns the number of user
	virtual int GetPlayerCount()
	{
		return engine->GetMaxClients();
	}

	// returns a playerID for a player
	// playerIndex is in the range [0, GetPlayerCount)
	virtual unsigned int GetPlayerUserID(int playerIndex)
	{
		int userID = engine->GetTrackerIDForPlayer(playerIndex);
		if (userID > 0)
			return userID;
		return 0;
	}

	// gets the in-game name of another user, returns NULL if that user doesn't exists
	virtual const char *GetPlayerName(int trackerID)
	{
		int playerSlot = engine->GetPlayerForTrackerID(trackerID);
		if (!playerSlot)
			return NULL;

		hud_player_info_t playerInfo;
		engine->GetPlayerInfo(playerSlot, &playerInfo);
		return playerInfo.name;
	}
};

EXPOSE_SINGLE_INTERFACE(CRunGameEngine, IRunGameEngine, RUNGAMEENGINE_INTERFACE_VERSION);
