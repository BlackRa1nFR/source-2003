//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TEAMFORTRESSVIEWPORT_H
#define TEAMFORTRESSVIEWPORT_H

// viewport interface for the rest of the dll
#include "IViewPort.h"
#include "IViewPortMsgs.h"
//#include <IClientVGUI.h>
#include "ISpectatorInterface.h"
#include "IScoreBoardInterface.h"
#include <cl_dll/IVGuiClientDll.h>

#include "UtlQueue.h" // a vector based queue template to manage our VGUI menu queue
#include <vgui_controls/Frame.h>
#include "vguitextwindow.h"
#include "vgui/isurface.h"
#include <game_controls/commandmenu.h>

using namespace vgui;

namespace vgui 
{
	class Panel;
	class Menu;
}

class CTeamMenu;
class CClassMenu;
class CClientScoreBoardDialog;
class CClientMOTD;
class CSpectatorGUI;
class CCommandMenu;
class CMapOverview;

#define MAX_SERVERNAME_LENGTH 32
#ifndef MAX_MOTD_LENGTH
#define MAX_MOTD_LENGTH 1536
#endif

class IBaseFileSystem;
class IGameUIFuncs;
class IGameEventManager;
class IMapOverview;

//==============================================================================
class TeamFortressViewport : public IViewPort, public IViewPortMsgs, public vgui::EditablePanel
{
public:
	TeamFortressViewport();
	virtual ~TeamFortressViewport();

	// IClientVGUI interface
	virtual void Start( IGameUIFuncs *pGameUIFuncs, IGameEventManager * pGameEventManager );
	virtual void SetParent(vgui::VPANEL parent);
	virtual bool UseVGUI1() { return false; }
	virtual void HideScoreBoard( void ); //  --
	virtual void HideAllVGUIMenu( void );   // these two are also in IViewPort, as IViewPort has no VGUI2 dependencies in it.
	virtual void ActivateClientUI();
	virtual void HideClientUI();


	// IViewPort interface
	virtual void UpdateScoreBoard( void );
	virtual bool AllowedToPrintText( void );
	virtual void GetAllPlayersInfo( void );
	virtual void DeathMsg( int killer, int victim );
	virtual void ShowScoreBoard( void );
	virtual bool CanShowScoreBoard( void );
	virtual bool IsScoreBoardVisible( void );
	virtual void UpdateSpectatorPanel();
	virtual int	 KeyInput( int down, int keynum, const char *pszCurrentBinding );
	virtual void OnLevelChange(const char * mapname);

	virtual void InputPlayerSpecial( void );


	virtual void ShowVGUIMenu( int iMenu );
	virtual void HideVGUIMenu( int iMenu );

	// just pass interfaces instead of tousend functions
	virtual IMapOverview * GetMapOverviewInterface();		
	virtual ISpectatorInterface * GetSpectatorInterface();

	virtual void ShowCommandMenu();
	virtual void UpdateCommandMenu();
	virtual void HideCommandMenu();
	virtual int IsCommandMenuVisible();

	// Data Handlers
	virtual int GetValidClasses(int iTeam) { return m_iValidClasses[iTeam]; };
	virtual int GetNumberOfTeams() { return m_iNumTeams; };
	virtual void SetNumberOfTeams(int num) { m_iNumTeams = num; };
	virtual bool GetIsFeigning() { return m_bIsFeigning; };
	virtual int GetIsSettingDetpack() { return m_iIsSettingDetpack; }; // 0 if we're not setting and no detpacks left, 1 if we're setting a detpack, 2 if it's possible to set a detpack
	virtual int GetBuildState() { return m_iBuildState; };
	virtual int IsRandomPC() { return m_iRandomPC; };
	virtual char *GetTeamName( int iTeam ) { return m_sTeamNames[iTeam]; };
	virtual int	GetCurrentMenuID() { if( m_PendingDialogs.Count() ) { return m_PendingDialogs.Head(); } else return -1; }
	virtual const char *GetServerName() { return m_szServerName; }


	// Message Handlers
	virtual int MsgFunc_ValClass(const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_TeamNames(const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_Feign(const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_Detpack(const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_VGUIMenu(const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_BuildSt( const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_RandomPC( const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_ServerName( const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_Spectator( const char *pszName, int iSize, void *pbuf );
	virtual int MsgFunc_SpecFade( const char *pszName, int iSize, void *pbuf );	
	virtual int MsgFunc_ResetFade( const char *pszName, int iSize, void *pbuf );	

	virtual int GetViewPortScheme() { return m_pBackGround->GetScheme(); }
	virtual vgui::Panel *GetViewPortPanel() { return m_pBackGround->GetParent(); }

	virtual void HideBackGround() { m_pBackGround->SetVisible( false ); }

	virtual void ChatInputPosition( int *x, int *y );
	
	// Input
	virtual bool SlotInput( int iSlot );

	VGuiLibraryInterface_t *GetClientDllInterface();
	void SetClientDllInterface(VGuiLibraryInterface_t *clientInterface);
	virtual VGuiLibraryTeamInfo_t GetPlayerTeamInfo( int playerIndex ) { return m_pClientScoreBoard->GetPlayerTeamInfo(playerIndex); }

protected:
	// derived classes can access these
	virtual bool IsVGUIMenuActive( int iMenu );
	virtual bool IsAnyVGUIMenuActive() { return m_pBackGround->IsVisible(); }
	virtual void DisplayVGUIMenu( int iMenu );

	char	*m_sTeamNames[5];
	// main panels
	CClientMOTD *m_pMOTD;
	ISpectatorInterface *m_pSpectatorGUI; // spectator is an interface so you can subclass it without having to override all the spectator function calls in TeamFortressViewport
	IScoreBoardInterface *m_pClientScoreBoard;
	CTeamMenu *m_pTeamMenu;
	CClassMenu *m_pClassMenu;
	CommandMenu *m_pCommandMenu;
	CVGUITextWindow *m_pMapBriefing;
	CVGUITextWindow *m_pClassHelp;
	CMapOverview * m_pMapOverview;

	class CSpecHelpWindow: public CVGUITextWindow
	{
	public:
		CSpecHelpWindow(vgui::Panel *parent, ISpectatorInterface *specGUI): CVGUITextWindow(parent) 
		{
			m_SpecGUI = specGUI;
		}
		
	private:
		virtual void OnClose()
		{
			m_SpecGUI->Activate();
			gViewPortInterface->HideBackGround();
			CVGUITextWindow::OnClose();
		}
		ISpectatorInterface *m_SpecGUI;
	};
	CSpecHelpWindow *m_pSpecHelp;

	class CBackGroundPanel : public vgui::Frame
	{
	private:
		typedef vgui::Frame BaseClass;
	public:
		CBackGroundPanel( vgui::Panel *parent) : Frame( parent, "ViewPortBackGround" ) 
		{
			SetScheme("ClientScheme");

			SetTitleBarVisible( false );
			SetMoveable(false);
			SetSizeable(false);
			SetProportional(true);
		}
	private:

		virtual void ApplySchemeSettings(IScheme *pScheme)
		{
			BaseClass::ApplySchemeSettings(pScheme);
			SetBgColor(pScheme->GetColor("ViewportBG", Color( 0,0,0,0 ) )); 
		}

		virtual void PerformLayout() 
		{
			int w,h;
			surface()->GetScreenSize(w, h);

			// fill the screen
			SetBounds(0,0,w,h);

			BaseClass::PerformLayout();
		}

		virtual void OnMousePressed(MouseCode code) { }// don't respond to mouse clicks
		virtual vgui::VPANEL IsWithinTraverse( int x, int y, bool traversePopups )
		{
			return NULL;
		}

	};
	CBackGroundPanel *m_pBackGround;

	virtual void OnTick(); // per frame think function


protected:
	IGameUIFuncs*		m_GameuiFuncs; // for key binding details
	IGameEventManager*  m_GameEventManager;

	
private:
	bool			 m_bInitialized;
	CSpectatorGUI *m_pPrivateSpectatorGUI;
	CClientScoreBoardDialog *m_pPrivateClientScoreBoard;

	CUtlQueue<int> m_PendingDialogs; // a queue of menus to display, the top of the stack is the 
									// menu that is currently displayed

	// server state
	char	m_szServerName[ MAX_SERVERNAME_LENGTH ];
	char	m_szMOTD[ MAX_MOTD_LENGTH ];	// MOTD
	int		m_iNumTeams; // the number of teams in the game
	int		m_iGotAllMOTD;
//	int		m_iAllowSpectators;
//	char	m_sMapName[64];
	int		m_iValidClasses[5];
	bool	m_bIsFeigning;
	int		m_iIsSettingDetpack;
	int		m_iBuildState;
	int		m_iRandomPC;


	// viewport state
	float		 m_flScoreBoardLastUpdated;
	int			 m_iCurrentTeamNumber;	// used to detect team change
	char		 m_szCurrentMap[256];	// used to detect a level change
	
	VGuiLibraryInterface_t *m_pClientDllInterface;

};


bool Helper_LoadFile( IBaseFileSystem *pFileSystem, const char *pFilename, CUtlVector<char> &buf );


#endif
