//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef GAMEPANELINFO_H
#define GAMEPANELINFO_H
#ifdef _WIN32
#pragma once
#endif

//#include <string.h>

#include <VGUI_Frame.h>
#include <VGUI_PHandle.h>
#include <VGUI_ListPanel.h>
#include <VGUI_Image.h>
#include <VGUI_KeyValues.h>
#include <VGUI_PropertyPage.h>
#include "serverinfo.h"
#include "playerlist.h"
#include "rcon.h"
#include "mapslist.h"
#include "Iresponse.h"
#include "RulesContextMenu.h"
#include "ChatContextMenu.h"
#include "BanContextMenu.h"
#include "logmsghandler.h"
#include "chatpanel.h"
#include "rawlogpanel.h"
#include "serverconfigpanel.h"
#include "playerpanel.h"
#include "motdpanel.h"
#include "banpanel.h"
#include "graphpanel.h"
#include "banlist.h"
#include "cmdlist.h"
#include "serverping.h"
#include "HelpText.h"


#define PASSWORD_LEN 64
#define MOD_LEN 64

// make sure we define it our way..
#undef PropertySheet

class KeyValues;

namespace vgui
{
class Button;
class ToggleButton;
class RadioButton;
class Label;
class TextEntry;
class ListPanel;
class MessageBox;
class ComboBox;
class Panel;
class PropertySheet;
class Image;
class ImagePanel;
};

#include "rulesinfo.h"
#include "point.h"



//-----------------------------------------------------------------------------
// Purpose: Dialog for displaying information about a game server
//-----------------------------------------------------------------------------
class CGamePanelInfo :public vgui::Frame, public IResponse
{
public:
	CGamePanelInfo(vgui::Panel *parent, const char *name,const char *mod,int refreshtime,int graphsrefreshtime,bool dolog);
	~CGamePanelInfo();

	// starts the servers refreshing
	virtual void StartRefresh();

	// stops current refresh/GetNewServerList()
	virtual void StopRefresh();

	// returns true if the list is currently refreshing servers
	virtual bool IsRefreshing();

	// changes the game that the screen is monitoring
	void ChangeGame(int serverIP, int serverPort,const char *pass);
	void ChangeGame(serveritem_t serverIn,const char *pass);

	// forces the dialog to attempt to connect to the server
	void Connect();

	// implementation of IRconResponse interface
	// called when the server has successfully responded to an rcon command
	virtual void ServerResponded();

	// called when a server response has timed out
	virtual void ServerFailedToRespond();


protected:
	// message handlers
	void OnConnect();
	void OnRefresh();
	void OnPassword();
	void OnTextChanged(vgui::Panel *panel, const char *text);
	void OnOpenContextMenu(int row);
	void OnKickPlayer(int playerID);
	void OnEffectPlayer(int playerID,const char *question, const char *type);
	void OnBanPlayer(int playerID);
	void OnStatusPlayer(int playerID);
	void OnSlapPlayer(int playerID);
	void OnChatPlayer(int playerID);
	void OnBanChange(int banID);
	void OnBanRemove(int banID);
	void OnBanAdd(int banID);
	void OnStop();
	void OnHelp();

	void OnCvarChange(int cvarID);
	
	// response from the get password dialog
	void OnJoinServerWithPassword(const char *password);

	// response from player dialog
	void OnPlayerDialog(vgui::KeyValues *data);


	// vgui overrides
	void OnTick();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:

	// methods
	void RequestInfo();
	void ConnectToServer();
	void OnManageServer(int serverID);

	void SetControlText(const char *textEntryName, const char *text);

	void StartLogAddress();
	void StopLogAddress();

	// controls the enabled/disabled state of rcon dependant GUI elements
	void RconPasswordDependant(bool state);

	vgui::Button *m_pConnectButton;
	vgui::Button *m_pCloseButton;
	vgui::Button *m_pRefreshButton;
	vgui::Button *m_pPasswordButton;
	vgui::Button *m_pStopButton;
	vgui::Button *m_pHelpButton;


	class RulesListPanel: public vgui::ListPanel
	{
	public:
		RulesListPanel(vgui::Panel *parent, const char *panelName): vgui::ListPanel(parent,panelName) { m_pParent=parent; m_pHelpText=NULL; allowDoubleClick=true;};
		
		virtual void	OnMouseDoublePressed( vgui::MouseCode code );
		virtual void	OnMousePressed(vgui::MouseCode code);
		void SetHelp(CHelpText *help) {m_pHelpText=help;}
		void EnableDoubleClick(bool state) { allowDoubleClick=state;}
	private:
		vgui::Panel *m_pParent;
		CHelpText *m_pHelpText;
		bool allowDoubleClick;

	};	


	// GUI pabels
	CPlayerPanel *m_pPlayerListPanel;
	RulesListPanel *m_pRulesListPanel;
	CBanPanel *m_pBanListPanel;
	vgui::TextEntry *m_pServerText;
	vgui::ComboBox *m_pMapsCombo;
	vgui::PropertySheet *m_pDetailsSheet;
	CRawLogPanel *m_pServerLogPanel;
	CChatPanel *m_pServerChatPanel;
	CServerConfigPanel *m_pServerConfigPanel;
	CGraphPanel *m_pGraphsPanel;
	CMOTDPanel *m_pMOTDPanel;

	// true if we should try connect to the server when it refreshes
	bool m_bConnecting;

	// password, if entered
	char m_szPassword[PASSWORD_LEN];
	// the mod name
	char m_sMod[MOD_LEN]; 

	// the server we are talking to
	CServerInfo *m_Server; 
	// a msg handler to get the player list
	CPlayerList *m_pPlayerList;
	// to send and recv rcon's
	CRcon *m_pRcon;
	// a msg handler to get server rules
	CRulesInfo *m_pRulesInfo;
	// a msg handler to get the map list from the server
	CMapsList *m_pMapsList;
	// Game server query socket for logs
	CSocket	*m_pQuery;
	// the msg handler for the logs
	CLogMsgHandlerDetails *m_pLog;
	// a msg handler to get the ban list
	CBanList *m_pBanList;
	// a msg handler to determine server ping times, used in the graph
	CServerPing *m_pServerPing;
	// a class that contains all the cmds the server supports
	CCMDList *m_pCMDList;

	// the help text for the rules list panel
	CHelpText *m_pHelpText; 

	// state
	bool m_bServerNotResponding;
	bool m_bServerFull;
	bool m_bPlayerStatus;
	bool m_bAdminMod;
	bool m_bMapsRefresh; // we don't want to refresh the maps list often..
	
	int m_iTimeLeft; // the time left on the current map, -1 means don't display.
	long m_iTimeLeftStart; // the time we recorded the timeleft value, in seconds
	int m_iTimeLeftLast; // the last time we updated the text, so we only update once per second

	bool m_bNewLogAddress; // whether the server supports the new style logaddress cmd's

	long m_iRequestRetry;	// time at which to retry the request

	long m_iRefreshTime; // the time between auto-refreshes, 0 means disabled, in milliseconds

	bool m_bDoingCmdList; // whether we are currently running the command list class
	bool m_bNewStatsReport; // whether we are using the new stats command
	bool m_bDoingStats; // whethere we in the middle of a stats command (waiting for the rcon response) 
	long m_iLastStatsReportTime; // the last time we did a stat request
	int m_iGraphsRefreshTime; // the time in seconds between stat requests
	bool m_bDoingMOTD; // whether have access to the MOTD command
	bool m_bDoingLogging; // whether we want to ask for server logs

	int m_iServerPing; // for stats
	bool m_bDoRestart; // server is doing a "_restart" so we need to restart ourself

	// context menu
	CRulesContextMenu *m_pRulesContextMenu;
	CChatContextMenu *m_pChatContextMenu;

	DECLARE_PANELMAP();
	typedef vgui::Frame BaseClass;

};

#endif // GAMEPANELINFO_H
