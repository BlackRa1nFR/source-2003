//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( IVIEWPORT_H )
#define IVIEWPORT_H
#ifdef _WIN32
#pragma once
#endif

namespace vgui
{
	class Panel;
}

#include <cl_dll/IVGuiClientDll.h>

// for ShowVGUIMenu
#define MENU_DEFAULT				1
#define MENU_TEAM 					2
#define MENU_CLASS 					3
#define MENU_MAPBRIEFING			4
#define MENU_INTRO 					5
#define MENU_CLASSHELP				6
#define MENU_CLASSHELP2 			7
#define MENU_REPEATHELP 			8
#define MENU_SPECHELP				9



#define MENU_SPY					12
#define MENU_SPY_SKIN				13
#define MENU_SPY_COLOR				14
#define MENU_ENGINEER				15
#define MENU_ENGINEER_FIX_DISPENSER	16
#define MENU_ENGINEER_FIX_SENTRYGUN	17
#define MENU_ENGINEER_FIX_MORTAR	18
#define MENU_DISPENSER				19
#define MENU_CLASS_CHANGE			20
#define MENU_TEAM_CHANGE			21

class IMapOverview;
class ISpectatorInterface;

class IViewPort
{
public:

	virtual VGuiLibraryInterface_t *GetClientDllInterface() = 0;
	virtual void SetClientDllInterface(VGuiLibraryInterface_t *clientInterface) = 0;

	//death.cpp
	virtual void UpdateScoreBoard() = 0;
	virtual bool AllowedToPrintText() = 0; // also saytext.cpp and text_message.cpp
	virtual void GetAllPlayersInfo( void ) = 0;
	virtual void DeathMsg( int killer, int victim ) = 0;
		
	// hud_redraw.cpp
	virtual void ShowScoreBoard( void ) = 0; // also input.cpp
	virtual bool CanShowScoreBoard( void ) = 0;
	virtual void HideAllVGUIMenu( void ) = 0;
	
	// input.cpp
	virtual bool IsScoreBoardVisible() = 0; // also menu.cpp
	virtual void HideScoreBoard( void ) = 0;
	virtual int	 KeyInput( int down, int keynum, const char *pszCurrentBinding ) = 0;	

	virtual void ShowVGUIMenu( int Menu ) = 0;

	virtual void ShowCommandMenu() = 0;
	virtual void UpdateCommandMenu() = 0;	
	virtual void HideCommandMenu() = 0;
	virtual int IsCommandMenuVisible() = 0;

	virtual int GetValidClasses(int iTeam) = 0;
	virtual int GetNumberOfTeams() = 0;
	virtual bool GetIsFeigning() = 0;
	virtual int GetIsSettingDetpack() = 0;
	virtual int GetBuildState() = 0;
	virtual int IsRandomPC() = 0;
	virtual char *GetTeamName( int iTeam ) = 0;
	virtual int	GetCurrentMenuID() = 0;
	virtual const char *GetServerName() = 0;

	virtual void InputPlayerSpecial() = 0;

	virtual void OnTick() = 0;

	virtual int GetViewPortScheme() = 0;

	virtual vgui::Panel *GetViewPortPanel() = 0;
	virtual IMapOverview * GetMapOverviewInterface() = 0;
	virtual ISpectatorInterface * GetSpectatorInterface() = 0;	

	virtual void OnLevelChange(const char * mapname) = 0;

	virtual void HideBackGround() = 0;

	virtual void ChatInputPosition( int *x, int *y ) = 0; 

	// Input
	virtual bool SlotInput( int iSlot ) = 0;

	// viewport
	virtual VGuiLibraryTeamInfo_t GetPlayerTeamInfo( int playerIndex ) = 0;
};

extern IViewPort *gViewPortInterface;

#endif // IVIEWPORT_H