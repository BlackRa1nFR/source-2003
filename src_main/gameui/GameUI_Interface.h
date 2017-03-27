//========= Copyright © 1996-2000, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the interface that the GameUI dll exports
//
// $NoKeywords: $
//=============================================================================

#ifndef GAMEUI_INTERFACE_H
#define GAMEUI_INTERFACE_H
#pragma once

#include "GameUI/IGameUI.h"

#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>

class CCDKeyEntryDialog;
class CSocket;
class IGameClientExports;

//-----------------------------------------------------------------------------
// Purpose: Implementation of GameUI's exposed interface 
//-----------------------------------------------------------------------------
class CGameUI : public IGameUI
{
public:
	CGameUI();
	~CGameUI();

	void Initialize( CreateInterfaceFn *factories, int count );
	void Start(struct cl_enginefuncs_s *engineFuncs, int interfaceVersion, IBaseSystem *system);
	void Shutdown();

	int	ActivateGameUI();	// activates the menus, returns 0 if it doesn't want to handle it
	int ActivateDemoUI();	// activates the demo player, returns 0 if it doesn't want to handle it

	int	HasExclusiveInput();

	void RunFrame();

	void ConnectToServer(const char *game, int IP, int port);
	void DisconnectFromServer();
	void HideGameUI();
	// returns true if the game UI is currently visible
	bool IsGameUIActive();
	
	void LoadingStarted(const char *resourceType, const char *resourceName);
	void LoadingFinished(const char *resourceType, const char *resourceName);

	virtual void StartProgressBar(const char *progressType, int progressSteps);
	virtual int	 ContinueProgressBar(int progressPoint, float progressFraction);
	virtual void StopProgressBar(bool bError, const char *failureReason, const char *extendedReason = NULL);
	virtual int SetProgressBarStatusText(const char *statusText);
	virtual void SetSecondaryProgressBar(float progress /* range [0..1] */);
	virtual void SetSecondaryProgressBarText(const char *statusText);

	
	// If the user's Steam subscription expires during play and the user has not told Steam to 
	// cache their password, Steam needs to have them re-enter it.
	void GetSteamPassword( const char *szAccountName, const char *szUserName );

	// prompts user for new key if the current key is invalid, force = true means force it to prompt
	virtual void ValidateCDKey(bool force = false, bool inConnect = false);

private:
	bool FindPlatformDirectory(char *platformDir, int bufferSize);
	void GetUpdateVersion( char *pszProd, char *pszVer);
	void MasterVersionCheckQuery();

	bool m_bTryingToLoadTracker;
	char m_szPlatformDir[260];
	int m_iNumFactories;
	CreateInterfaceFn m_FactoryList[5];

	// details about the game we're currently in
	int m_iGameIP;
	int m_iGamePort;
	bool m_bActivatedUI;

	const char *m_pszCurrentProgressType;
	float m_flProgressStartTime;

	CSocket *m_pMaster;

	vgui::DHANDLE<CCDKeyEntryDialog> m_hCDKeyEntryDialog;
};

// Purpose: singleton accessor
extern CGameUI &GameUI();

// expose client interface
extern IGameClientExports *GameClientExports();


#endif // GAMEUI_INTERFACE_H
