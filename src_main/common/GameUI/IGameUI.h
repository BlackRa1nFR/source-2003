//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef IGAMEUI_H
#define IGAMEUI_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

struct cl_enginefuncs_s;
class IBaseSystem;	

namespace vgui2
{
class Panel;
}

//-----------------------------------------------------------------------------
// Purpose: contains all the functions that the GameUI dll exports
//			GameUI_GetInterface() is exported via dll export table to get this table
//-----------------------------------------------------------------------------
class IGameUI : public IBaseInterface
{
public:
	virtual void Initialize( CreateInterfaceFn *factories, int count ) = 0;
	virtual void Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion, IBaseSystem *system) = 0;
	virtual void Shutdown() = 0;

	virtual int	ActivateGameUI() = 0;	// activates the menus, returns 0 if it doesn't want to handle it
	virtual int	ActivateDemoUI() = 0;	// activates the demo player, returns 0 if it doesn't want to handle it

	virtual int	HasExclusiveInput() = 0;

	virtual void RunFrame() = 0;

	virtual void ConnectToServer(const char *game, int IP, int port) = 0;
	virtual void DisconnectFromServer() = 0;
	virtual void HideGameUI() = 0;

	virtual bool IsGameUIActive() = 0;
	
	virtual void LoadingStarted(const char *resourceType, const char *resourceName) = 0;
	virtual void LoadingFinished(const char *resourceType, const char *resourceName) = 0;

	virtual void StartProgressBar(const char *progressType, int numProgressPoints) = 0;
	virtual int	 ContinueProgressBar(int progressPoint, float progressFraction) = 0;
	virtual void StopProgressBar(bool bError, const char *failureReasonIfAny, const char *extendedReason) = 0;
	virtual int  SetProgressBarStatusText(const char *statusText) = 0;
 
	virtual void GetSteamPassword( const char *szAccountName, const char *szUserName ) = 0;

	virtual void ValidateCDKey(bool force, bool inConnect) = 0;
};

// the interface version is the number to call GameUI_GetInterface(int interfaceNumber) with
#define GAMEUI_INTERFACE_VERSION "GameUI006"

#endif // IGAMEUI_H
