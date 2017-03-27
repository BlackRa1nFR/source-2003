//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include "winsock.h" // this BUGGER defines PropertySheet to PropertySheetA ....
#include "tokenline.h"

#include "GamePanelInfo.h"
#include "Info.h"
#include "IRunGameEngine.h"
#include "IGameList.h"
#include "TrackerProtocol.h"
#include "serverpage.h"
#include "rcon.h"
#include "DialogServerPassword.h"
#include "INetAPI.h"
#include "rulesinfo.h"
//#include "playerlistcompare.h"
#include "ban.h"

#include "DialogGameInfo.h"
#include "DialogCvarChange.h"
#include "DialogKickPlayer.h"
#include "DialogAddBan.h"


#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_ISurface.h>
#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>
#include <VGUI_TextEntry.h>
#include <VGUI_Button.h>
#include <VGUI_ToggleButton.h>
#include <VGUI_RadioButton.h>
#include <VGUI_ListPanel.h>
#include <VGUI_ComboBox.h>
#include <VGUI_PHandle.h>
#include <VGUI_PropertySheet.h>
#include <VGUI_MouseCode.h>
#include <VGUI_MessageBox.h>
#include <VGUI_Image.h>
#include <VGUI_ImagePanel.h>
#include <VGUI_Point.h> // for the graphs

#include <VGUI_ILocalize.h>

#include <proto_oob.h>
#include <netadr.h>
#include "util.h"


using namespace vgui;

static const long RETRY_TIME = 10000;		// refresh server every 10 seconds
static const long MAP_CHANGE_TIME = 10000;		// refresh 10 seconds after a map change 
static const long RESTART_TIME = 60000;		// refresh 60 seconds after a "_restart"

//static const long STATS_TIMEOUT = 1000; // refresh stats once per second

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGamePanelInfo::CGamePanelInfo(vgui::Panel *parent, const char *name,const char *mod,
							   int refreshtime,int graphsrefreshtime,bool dolog) :  Frame(parent, name)
{
//	SetBounds(0, 0, 500, 550);
//	SetMinimumSize(500, 550);

	MakePopup();


	m_bMapsRefresh			= true; // we need to initialise the list
	m_bConnecting			= false;
	m_bServerFull			= false;
	m_bServerNotResponding	= false;
	m_bPlayerStatus			= false;
	m_bAdminMod				= false;
	m_iTimeLeft				= -1;
	m_iTimeLeftStart		= -1;
	m_iTimeLeftLast			= -1; // the last time a timeleft text was posted	
	m_bNewLogAddress		= false; // by default assume it doesn't support the new style
	m_bDoingStats			= false; // are we asking about stats?
	m_bNewStatsReport		= false; // do we have the stats capability?
	m_bDoingCmdList			= true; // we still need to query this
	m_iServerPing			= 0; // no default ping time
	m_iLastStatsReportTime	= 0; 
	m_bDoingMOTD			= false; // are we doing a cmdlist to find out capabilities?
	m_bDoingLogging			= dolog;
	m_bDoRestart			= false;

	//if(refreshtime!=0)
	//{
		m_iRefreshTime=refreshtime*1000; // refresh time is in seconds :-)
	//}

	m_iGraphsRefreshTime = graphsrefreshtime*1000;

	m_szPassword[0]			= 0;
	v_strncpy(m_sMod,mod,64);

		
	m_pConnectButton = new Button(this, "Connect", "&Join Game");
	m_pRefreshButton = new Button(this, "Refresh", "&Refresh");
	m_pStopButton = new Button(this, "Stop", "&Restart Server");
	m_pPasswordButton = new Button(this, "Password", "&Rcon Password");
	m_pHelpButton = new Button(this, "Help", "&Help");
	m_pPlayerListPanel = new CPlayerPanel(this, "Player List");
	m_pRulesListPanel = new RulesListPanel(this, "Rules List");
	//m_pRulesListPanel->SetTooltipText("Dis is dem rules");
	
	m_pBanListPanel = new CBanPanel(this, "Ban List");
	m_pServerText = new TextEntry(this, "ServerText");
	m_pMapsCombo = new ComboBox(this, "MapsCombo",0,false);

	m_pServerLogPanel = new CRawLogPanel(this, "ServerLog");
	if(m_bDoingLogging)
	{
		m_pServerChatPanel = new CChatPanel(this, "ChatPanel");
	}
	m_pServerConfigPanel = new CServerConfigPanel(this, "ServerConfigPanel",mod);

	m_pGraphsPanel = new CGraphPanel(this,"Graphs");

	m_pMOTDPanel = new CMOTDPanel(this,"MOTD");

	m_pDetailsSheet = new PropertySheet(this, "Details");
//	m_pDetailsSheet->SetScrolling(true);
	m_pDetailsSheet->AddActionSignalTarget(this);


	m_pConnectButton->SetCommand(new KeyValues("Connect"));
	m_pRefreshButton->SetCommand(new KeyValues("Refresh"));
	m_pPasswordButton->SetCommand(new KeyValues("Password"));
	m_pStopButton->SetCommand(new KeyValues("Stop"));
	m_pHelpButton->SetCommand(new KeyValues("Help"));
		


	m_pRulesListPanel->AddColumnHeader(0, "cvar", util->GetString("Cvar"), 200, true, RESIZABLE, RESIZABLE );
	m_pRulesListPanel->AddColumnHeader(1, "value", util->GetString("Value"), 200, true,  RESIZABLE, NOT_RESIZABLE);
	m_pRulesListPanel->SetSortColumn(0);

	m_pBanListPanel->AddColumnHeader(0, "name", util->GetString("Name"), 200, true, RESIZABLE, RESIZABLE );
	m_pBanListPanel->AddColumnHeader(1, "time", util->GetString("Time (minutes)"), 200, true, RESIZABLE, NOT_RESIZABLE );
	m_pBanListPanel->SetSortColumn(0);

	// now populate the details page :-)
	// update RconPasswordDependant() if you change these titles
	m_pDetailsSheet->AddPage(m_pPlayerListPanel,"Players");
	m_pDetailsSheet->AddPage(m_pRulesListPanel,"Variables");
	m_pDetailsSheet->AddPage(m_pBanListPanel,"Bans");	
	m_pDetailsSheet->AddPage(m_pServerConfigPanel,"Configure");	
	m_pDetailsSheet->AddPage(m_pMOTDPanel,"MOTD");	
	if(m_bDoingLogging)
	{
		m_pDetailsSheet->AddPage(m_pServerChatPanel,"Chat Log");
	}
	m_pDetailsSheet->AddPage(m_pServerLogPanel,"Raw Log");	
	
	m_pDetailsSheet->AddPage(m_pGraphsPanel,"Stats");	
	if( graphsrefreshtime == 0 )
	{
		m_pDetailsSheet->DisablePage("Stats"); // disabled this until we know we have this command on the server
	}
	

	m_pRulesContextMenu = new CRulesContextMenu(this);
	m_pRulesContextMenu->SetVisible(false);

	m_iRequestRetry = 0;

	m_Server		= NULL;//new CServerInfo(this,server);
	m_pPlayerList	= NULL; //new CPlayerList(this,server,m_szPassword);
	m_pRcon			= NULL;//new CRcon(this,server,m_szPassword);
	m_pRulesInfo	= NULL;//new CRulesInfo(this,server);
	m_pMapsList		= NULL;
	m_pQuery		= NULL;
	m_pLog			= NULL;
	m_pBanList		= NULL;
	m_pServerPing	= NULL;
	m_pCMDList		= NULL;
	m_pHelpText		= NULL;
  
	// let us be ticked every frame
	ivgui()->AddTickSignal(this->GetVPanel());


	LoadControlSettings("Admin\\GamePanelInfo.res");	

//	LoadDialogState(this, name);


	StartRefresh();
	SetTitle(name, true);
	SetVisible(true);

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGamePanelInfo::~CGamePanelInfo()
{
	
	StopLogAddress();

	if( m_Server != NULL ) 
	{
		delete m_Server;
	}

	if( m_pPlayerList != NULL ) 
	{
		delete m_pPlayerList;
	}

	if( m_pRcon != NULL ) 
	{
		delete m_pRcon;
	}
	
	if( m_pRulesInfo != NULL ) 
	{
		delete m_pRulesInfo;
	}

	if( m_pMapsList != NULL )
	{
		delete m_pMapsList;
	}

	if( m_pQuery != NULL ) // this also cleans up m_pLog
	{
		delete m_pQuery;
	}

	if( m_pBanList != NULL ) 
	{
		delete m_pBanList;
	}

	if( m_pServerPing != NULL ) 
	{
		delete m_pServerPing;
	}

	if( m_pCMDList != NULL ) 
	{
		delete m_pCMDList;
	}

	if( m_pHelpText != NULL ) 
	{
		delete m_pHelpText;
	}
}

//-----------------------------------------------------------------------------
// Purpose: control the enabled/disabled state of rcon password dependant elements
//-----------------------------------------------------------------------------
void CGamePanelInfo::RconPasswordDependant(bool state)
{
	if(state)
	{
		m_pDetailsSheet->EnablePage("Chat Log"); // needs rcon to do logging
		m_pDetailsSheet->EnablePage("Raw Log"); // needs rcon for logging and raw rcons
		m_pDetailsSheet->EnablePage("Configure"); // needs rcon to submit changes
		m_pDetailsSheet->EnablePage("Bans"); // needs rcon to get and effect list
		m_pDetailsSheet->EnablePage("MOTD"); // needs rcon to get and change
	} 
	else
	{
		m_pDetailsSheet->DisablePage("Chat Log");
		m_pDetailsSheet->DisablePage("Raw Log");
		m_pDetailsSheet->DisablePage("Configure");
		m_pDetailsSheet->DisablePage("Bans");
		m_pDetailsSheet->DisablePage("MOTD");

	}

	m_pRulesListPanel->EnableDoubleClick(state);
	m_pStopButton->SetEnabled(state); // needs rcon to execute "quit" command
	m_pMapsCombo->SetEnabled(state); // needs rcon to get list of maps
	m_pPlayerListPanel->SetHasRcon(state); // needs rcon to kick/ban/slap/chat
	m_pGraphsPanel->PingOnly(!state); // needs rcon for "stat" command

}

//-----------------------------------------------------------------------------
// Purpose: Changes which server to watch
//-----------------------------------------------------------------------------
void CGamePanelInfo::ChangeGame(int serverIP, int serverPort,const char *pass)
{
	// check to see if it's the same game
	if(m_Server!=NULL)
	{
		serveritem_t &server = m_Server->GetServer();

		if (*(int *)server.ip == serverIP && server.port == serverPort)
		{
			return;
		}
	}

	//delete m_Server;

	// create a new server to watch
	serveritem_t newServer;
	memset(&newServer, 0, sizeof(newServer));
	*((int *)newServer.ip) = serverIP;
	newServer.port = serverPort;

	ChangeGame(newServer,pass);
}

//-----------------------------------------------------------------------------
// Purpose: Changes which server to watch
//-----------------------------------------------------------------------------
void CGamePanelInfo::ChangeGame(serveritem_t serverIn, const char *pass)
{
	// check to see if it's the same game
	if(m_Server!=NULL)
	{
		serveritem_t &server = m_Server->GetServer();

		if (server.ip == serverIn.ip && server.port == serverIn.port)
		{
			return;
		}
	}

	v_strncpy(m_szPassword,pass,PASSWORD_LEN);

	if( m_Server!=NULL) 
	{
		delete m_Server;
	}
	m_Server = new CServerInfo(this,serverIn);

	if(m_pRcon!=NULL)
	{
		delete m_pRcon;
	}
	m_pRcon = new CRcon(this,serverIn,m_szPassword);

	if( m_pPlayerList!=NULL) 
	{
		delete m_pPlayerList;
	}
	m_pPlayerList= new CPlayerList(this,serverIn,m_szPassword);


	if(m_pRulesInfo!=NULL)
	{
		delete m_pRulesInfo;
	}
	m_pRulesInfo = new CRulesInfo(this,serverIn);


	if(m_pMapsList != NULL) 
	{
		m_bMapsRefresh = true; // we just deleted it so we had better refresh
		delete m_pMapsList;
	}
	m_pMapsList = new CMapsList(this,serverIn,pass,m_sMod);

	if(m_pBanList != NULL) 
	{
		delete m_pBanList;
	}
	m_pBanList = new CBanList(this,serverIn,pass);

	if( m_pServerPing != NULL ) 
	{
		delete m_pServerPing;
	}
	m_pServerPing = new CServerPing(this,serverIn);



	if( m_pCMDList != NULL ) 
	{
		delete m_pCMDList;
	}
	m_pCMDList = new CCMDList(this,serverIn,pass);

	if( m_pHelpText != NULL ) 
	{
		delete m_pHelpText;
	}
	m_pHelpText = new CHelpText(m_sMod);
	m_pRulesListPanel->SetHelp(m_pHelpText);

//	if(m_pLog != NULL)
//	{
//		delete m_pLog;
//	}

	if (m_pQuery != NULL) // this also cleans up m_pLog
	{
			delete m_pQuery;
	}

	if(m_bDoingLogging	)
	{
		m_pQuery = new CSocket("log catcher", -1);

		int bytecode = S2A_INFO_DETAILED;
		m_pLog=new CLogMsgHandlerDetails(this, CMsgHandler::MSGHANDLER_ALL, &bytecode);
		// now setup the chat dialog
		m_pQuery->AddMessageHandler(m_pLog);
	}

	m_pRcon->SetPassword(pass);
	m_pPlayerList->SetPassword(pass);
	m_pBanList->SetPassword(pass);
	m_pMapsList->SetPassword(pass);
	m_pCMDList->SetPassword(pass);

	if(m_bDoingLogging	)
	{
		m_pServerChatPanel->SetRcon(m_pRcon);
	}
	m_pServerLogPanel->SetRcon(m_pRcon);
	m_pServerConfigPanel->SetRcon(m_pRcon);
	m_pMOTDPanel->SetRcon(m_pRcon);

	//refresh now we have a new pw
	RequestInfo();

}


//-----------------------------------------------------------------------------
// Purpose: Relayouts the data
//-----------------------------------------------------------------------------
void CGamePanelInfo::PerformLayout()
{
	BaseClass::PerformLayout();

	if( m_Server==NULL) 
	{
		return;
	}

	// get the server we're watching
	serveritem_t &server = m_Server->GetServer();

	SetControlText("ServerText", server.name);
	SetControlText("GameText", server.gameDescription);
	SetControlText("MapsCombo", server.map);

	char buf[128];
	if (server.maxPlayers > 0)
	{
		sprintf(buf, "%d / %d", server.players, server.maxPlayers);
	}
	else
	{
		buf[0] = 0;
	}
	SetControlText("PlayersText", buf);

	if (server.ip[0] && server.port)
	{
		char buf[64];
		sprintf(buf, "%d.%d.%d.%d:%d", server.ip[0], server.ip[1], server.ip[2], server.ip[3], server.port);
		SetControlText("ServerIPText", buf);
		m_pConnectButton->SetEnabled(true);
	}
	else
	{
		SetControlText("ServerIPText", "");
		m_pConnectButton->SetEnabled(false);
	}

	sprintf(buf, "%d", server.ping);
	SetControlText("PingText", buf);



	// layout the details tab
	int x,y,w,h;
	m_pPasswordButton->GetBounds(x,y,w,h);

	// make it 10 pixels from the left, 20 below the password button.
	// then fill the screen for the (width - 20) pixels and the rest of the 
	// height - 40 pixel border
	m_pDetailsSheet->SetBounds(10,y+h+10,GetWide()-20,GetTall()-y-h-20);

	// set the info text
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the current scheme colors
//-----------------------------------------------------------------------------
void CGamePanelInfo::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

}

//-----------------------------------------------------------------------------
// Purpose: Forces the game info dialog to try and connect
//-----------------------------------------------------------------------------
void CGamePanelInfo::Connect()
{
	OnConnect();
}

/*
//-----------------------------------------------------------------------------
// Purpose: Activates the page, starts refresh
//----------------------------------------------------------------------------
void CGamePanelInfo::OnPageShow()
{
	StartRefresh();
}
*/
//-----------------------------------------------------------------------------
// Purpose: starts the servers refreshing
//-----------------------------------------------------------------------------
void CGamePanelInfo::StartRefresh()
{
	RequestInfo();
}

/*
//-----------------------------------------------------------------------------
// Purpose: Called on page hide, stops any refresh
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnPageHide()
{
	StopRefresh();
}
*/

//-----------------------------------------------------------------------------
// Purpose: stops current refresh/GetNewServerList()
//-----------------------------------------------------------------------------
void CGamePanelInfo::StopRefresh()
{

}

//-----------------------------------------------------------------------------
// Purpose: returns true if the list is currently refreshing servers
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGamePanelInfo::IsRefreshing()
{
	return (( m_Server && m_Server->IsRefreshing()) || (m_pPlayerList && m_pPlayerList->IsRefreshing()));
}

//-----------------------------------------------------------------------------
// Purpose: sets the rcon password for this session
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnPassword()
{
	CDialogCvarChange *box = new CDialogCvarChange();
	box->AddActionSignalTarget(this);
	box->MakePassword();
	box->Activate("", "","rconpassword","Enter Rcon Password for this Server");

}

//-----------------------------------------------------------------------------
// Purpose: Connects the user to this game
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnConnect()
{
	// flag that we are attempting connection
	m_bConnecting = true;

	// reset state
	m_bServerFull = false;
	m_bServerNotResponding = false;

	InvalidateLayout();

	// need to refresh server before attempting to connect, to make sure there is enough room on the server
	RequestInfo();
}

//-----------------------------------------------------------------------------
// Purpose: Handles Refresh button press, starts a re-ping of the server
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnRefresh()
{
	m_iRequestRetry = system()->GetTimeMillis() + 1000; // do a refresh in 1 second 
	
	CServerPage::GetInstance()->UpdateStatusText("Refreshing....");
	// re-ask the server for the game info
//	RequestInfo();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CGamePanelInfo::SetControlText(const char *textEntryName, const char *text)
{
	TextEntry *entry = dynamic_cast<TextEntry *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Requests the right info from the server
//-----------------------------------------------------------------------------
void CGamePanelInfo::RequestInfo()
{
	// reset the time at which we auto-refresh
	// don't auto refresh :)
	if(m_iRefreshTime!=0) 
	{
		m_iRequestRetry = system()->GetTimeMillis() + m_iRefreshTime;
	}
	else
	{
		// reset it to zero in case somewhere else asked for a refresh
		m_iRequestRetry = 0;
	}

	if (m_Server && !m_Server->IsRefreshing())
	{	
		m_Server->Refresh();
	}

	if(m_pPlayerList &&  !m_pPlayerList->IsRefreshing()) 
	{
		m_pPlayerList->Refresh();
	}

	if(m_pRulesInfo && !m_pRulesInfo->IsRefreshing()) 
	{
		m_pRulesInfo->Refresh();
	}

	if(m_bMapsRefresh && m_pMapsList && !m_pMapsList->IsRefreshing()) 
	{
		m_bMapsRefresh=false; // only do this once, it takes a fair amount of bandwidth and
							  // the maps on the server only change rarely
		m_pMapsList->Refresh();
	}

	if(m_pBanList!=NULL)
	{
		m_pBanList->Refresh();
	}

	if(m_pRcon && !m_pRcon->Disabled())
	{
		m_pRcon->SendRcon("motd");
		m_bDoingMOTD=true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame, handles resending network messages
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnTick()
{
	// check to see if we should perform an auto-refresh
	if (m_iRequestRetry && m_iRequestRetry < system()->GetTimeMillis())
	{
		// reask
		RequestInfo();
	}

	if(m_iGraphsRefreshTime && m_iLastStatsReportTime+m_iGraphsRefreshTime< system()->GetTimeMillis())
	{
		if(m_bDoRestart)
		{
			m_bDoRestart=false;
			ChangeGame(m_Server->GetServer(), m_szPassword);
		}
		m_iLastStatsReportTime =  system()->GetTimeMillis();
		m_bDoingStats=true;
		if(m_bNewStatsReport)
		{
			m_pRcon->SendRcon("stats"); // get stats
		}

		m_pServerPing->Query(); // and ping the server
	} 



	if( m_Server!=NULL) 
	{
		m_Server->RunFrame();
	}

	if( m_pPlayerList!=NULL) 
	{
		m_pPlayerList->RunFrame();
	}

	if( m_pRcon!=NULL) 
	{
		m_pRcon->RunFrame();
	}
	
	if( m_pRulesInfo!=NULL) 
	{
		m_pRulesInfo->RunFrame();
	}

	if( m_pMapsList!=NULL) 
	{
		m_pMapsList->RunFrame();
	}

	if( m_pQuery != NULL)
	{
		m_pQuery->Frame();
	}

	if( m_pBanList != NULL)
	{
		m_pBanList->RunFrame();
	}

	if( m_pServerPing != NULL)
	{
		m_pServerPing->RunFrame();
	}

	if( m_pCMDList != NULL)
	{
		m_pCMDList->RunFrame();
	}

	if( m_iTimeLeft>0 ) 
	{

		char timeleftstr[10];
		int timeleft =  m_iTimeLeft -  (( system()->GetTimeMillis() /1000) - m_iTimeLeftStart) ; 

		
		if(timeleft>0 && (m_iTimeLeftLast -timeleft)>=1 )  
			// if there is timeleft AND 1 sec or more has passed since the last update (timeleft is decrementing)
		{
			// Should it be hh:mm:ss or just mm:ss ??
		//	if( (timeleft/3600) > 1 ) // this is more than 1 hour...
		//	{
				_snprintf(timeleftstr,10,"%0.2i:%0.2i:%0.2i",(timeleft/3600),(timeleft/60)%60,timeleft%60);
		//	}
		//	else
		//	{
		//		_snprintf(timeleftstr,10,"%0.2i:%0.2i",timeleft/60,timeleft%60);
		//	}	

			SetControlText("TimeLeftText", timeleftstr);
			m_iTimeLeftLast=timeleft;
		}
		else if(timeleft<=0) 
		{
			m_iTimeLeft=-1; // time on map has run out
			m_iTimeLeftLast=-1;
		
			SetControlText("TimeLeftText", ""); // reset the display

			m_iRequestRetry = system()->GetTimeMillis() + MAP_CHANGE_TIME; // queue a refresh after a suitable wait
		}

	}

}

//-----------------------------------------------------------------------------
// Purpose: called when the server has successfully responded
//-----------------------------------------------------------------------------
void CGamePanelInfo::ServerResponded()
{
	if (m_bConnecting)
	{
		ConnectToServer();
	}

	CServerPage::GetInstance()->UpdateStatusText("");

	if(m_pPlayerList && m_pPlayerList->NewPlayerList() ) 
	{		
		m_pPlayerListPanel->NewPlayerList(m_pPlayerList->GetPlayerList());
	}

	if(m_pRulesInfo && m_pRulesInfo->Refreshed() ) {
		// clear the existing panel
		m_pRulesListPanel->DeleteAllItems();
		
		CUtlVector<KeyValues *> *rules= m_pRulesInfo->Rules();

		// update the config panel with the new rules and set the hostname
		m_pServerConfigPanel->SetRules(rules);
		serveritem_t &server = m_Server->GetServer();
		m_pServerConfigPanel->SetHostName(server.name);
		m_pServerConfigPanel->OnPageShow(); // cause it to redraw

		int index;
			
		if(rules!=NULL) 
		{
			for (int i = 0; i < rules->Count(); i++)
			{
				// check if this server is running Admin Mod :-)
				if(!strncmp("admin_mod_version",(*rules)[i]->GetString("cvar"),13) )
				{
					m_bAdminMod=true;
					m_pPlayerListPanel->SetAdminMod(true);
				}
				if(!strncmp("mp_timelimit",(*rules)[i]->GetString("cvar"),12) )
				{
					SetControlText("TimelimitText",(*rules)[i]->GetString("value") );
				}
				if(!strncmp("mp_fraglimit",(*rules)[i]->GetString("cvar"),12)  // for normal hl games
					|| !strncmp("mp_winlimit",(*rules)[i]->GetString("cvar"),11)) // for cstrike 
				{
					SetControlText("FraglimitText",(*rules)[i]->GetString("value") );
				}
				
				if(!strncmp("mp_timeleft",(*rules)[i]->GetString("cvar"),11) )  // most mods will have this
				{
					m_iTimeLeft = atoi( (*rules)[i]->GetString("value") ); // timeleft on this map in seconds, 
																		   //   "0" means time has run out
					m_iTimeLeftStart = (system()->GetTimeMillis() /1000); // turn this into seconds 
					m_iTimeLeftLast = m_iTimeLeft;
					
					char timeleftstr[10];
					_snprintf(timeleftstr,10,"%0.2i:%0.2i:%0.2i",(m_iTimeLeft/3600),(m_iTimeLeft/60)%60,m_iTimeLeft%60);
					SetControlText("TimeLeftText", timeleftstr);

				}
					
				index =	m_pRulesListPanel->AddItem( (*rules)[i],i);
			}		
			m_pRulesListPanel->SortList();
			m_pRulesListPanel->Repaint();	
		 
		}		
	}

	if(m_pBanList && m_pBanList->NewBanList() ) {
		// clear the existing panel
		m_pBanListPanel->DeleteAllItems();
		
		CUtlVector<Bans_t> *bans= m_pBanList->GetBanList();
		KeyValues *kv;

		int index;
			
		if(bans!=NULL) 
		{
			for (int i = 0; i < bans->Count(); i++)
			{		
				kv = new KeyValues("BanX");
				kv->SetString("name", (*bans)[i].name);
				kv->SetFloat("time", (*bans)[i].time);
				index =	m_pBanListPanel->AddItem(kv);
		
			}		
			m_pBanListPanel->SortList();
			m_pBanListPanel->Repaint();	
		}		
	}


	if(m_pMapsList && m_pMapsList->NewMapsList() ) 
	{
							
		// clear the existing panel
		m_pMapsCombo->RemoveAllItems();

	
		CUtlVector<Maps_t> *maps= m_pMapsList->GetMapsList();


		m_pMapsCombo->SetNumberOfEditLines(maps->Count());	
		m_pMapsCombo->SetDropdownButtonVisible(true);

		for (int i = 0; i < maps->Count(); i++)
		{
		    m_pMapsCombo->AddItem((*maps)[i].name);
		}

		if(maps->Count()==0)
		{
			m_pMapsCombo->SetEnabled(false);
			MessageBox *dlg = new MessageBox("Server Error", "Unable to retrieve maps list.");
			dlg->DoModal();

		}
		m_pMapsCombo->Repaint();

	
	}



	if(m_bDoingLogging	&& m_pLog && m_pLog->NewMessage() )
	{
		const char *newline=m_pLog->GetBuf();
		// this is a say log format:
		//		log L 07/01/2002 - 16:22:32: "Player<1><4294967295><1>" say "hello"

		if(strstr(newline,"say \""))  // if this is a say message
		{
			TokenLine chatLine( const_cast<char *>(newline));

			if(chatLine.CountToken() >= 8)  // if its a valid line
			{
				// turn the chat into something that looks like IRC
				char chat_text[512];
				
				char *player=chatLine.GetToken(5);
				char *tmp= strchr(player,'<');
				if(tmp!=NULL)
				{
					*tmp='\0';
				}
				char *say = chatLine.GetRestOfLine(7);
				say[strlen(say)-2]='\n'; // remove the " at the end
				say[strlen(say)-1]='\0';

				_snprintf(chat_text,512,"<%s> %s",player,say);
				m_pServerChatPanel->DoInsertString(chat_text);
	
			} // if enough tokens
		} // if a say line
		else if(strstr(newline,"say \"!page")) 
		{
			

			//m_pServerChatPanel->doInsertString("Someone is paging the Admin!!");
		}

		if(m_bDoingStats && strstr(newline,"\"rcon") && strstr(newline,"stats\"") ) 
			// don't let stats responses in the raw log
		{

		}
		else
		{
			m_pServerLogPanel->DoInsertString(newline);
		}
	}
		

	if(m_bDoingCmdList && m_pCMDList->GotCommands())
	{
			m_bNewStatsReport=m_pCMDList->QueryCommand("stats");
			m_bNewLogAddress=m_pCMDList->QueryCommand("logaddress_add");
			StartLogAddress();
			m_bDoingCmdList=false;
			m_pGraphsPanel->PingOnly(!m_bNewStatsReport);
	}

	if(m_pRcon && m_pRcon->NewRcon() )
	{
		const char *resp = m_pRcon->RconResponse();
		const char *stats = strstr(resp," CPU   In    Out   Uptime  Users FPS");
		const char *motd = strstr(resp,"motd:");
		
		if ( m_bDoingStats && stats)
		{
			// format is:
			//CPU     In      Out     Uptime  Users	FPS
			//0.00    0.00    0.00    0       1		30.54
			stats=stats+strlen(" CPU   In    Out   Uptime  Users FPS")+1;
			TokenLine tok;
			tok.SetLine(stats);
			Points_t p;
			memset(&p,0x0,sizeof(Points_t));

			if(tok.CountToken() >= 5 ) 
			{
			
				sscanf(tok.GetToken(0),"%f",&p.cpu);
				p.cpu=p.cpu/100; // its given as a value between 0<x<100, we want 0<x<1
				sscanf(tok.GetToken(1),"%f",&p.in);
				sscanf(tok.GetToken(2),"%f",&p.out);
				sscanf(tok.GetToken(5),"%f",&p.fps);
				p.time = static_cast<float> (system()->GetTimeMillis()/1000 );  // set the time we got this, in seconds
				p.ping = static_cast<float>(m_iServerPing);
				m_pGraphsPanel->AddPoint(p);

				SetControlText("TotalUsers",tok.GetToken(4) );

				int timeleft= atoi(tok.GetToken(3))*60;
				char timeText[20];
				_snprintf(timeText,10,"%0.2i:%0.2i:%0.2i",(timeleft/3600),(timeleft/60)%60,timeleft%60);
				SetControlText("UpTime", timeText);

			}
			

		}
		else if (m_bDoingMOTD && motd)
		{
			m_pMOTDPanel->DoInsertString(resp+strlen("motd:"));
			m_bDoingMOTD=false;
		}
		else
		{
			// display it in the raw log panel
			m_pServerLogPanel->DoInsertString(resp);
		}
	

	}

	if(m_pServerPing && m_bDoingStats && m_pServerPing->Refreshed())
	{
		m_iServerPing = m_pServerPing->GetServer().ping;
		if(m_bNewStatsReport==false)
		{
			Points_t p;
			memset(&p,0x0,sizeof(Points_t));
			p.time = static_cast<float> (system()->GetTimeMillis()/1000 );  // set the time we got this, in seconds
			p.ping = static_cast<float>(m_iServerPing);
			m_pGraphsPanel->AddPoint(p);
		}
	}
	
	m_bServerNotResponding = false;

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: called when a server response has timed out
//-----------------------------------------------------------------------------
void CGamePanelInfo::ServerFailedToRespond()
{

	CServerPage::GetInstance()->UpdateStatusText("");

	if (m_pRcon &&  m_pRcon->PasswordFail() ) 
	{
		const char *resp = m_pRcon->RconResponse();
		
		CDialogKickPlayer *box = new CDialogKickPlayer();
		box->AddActionSignalTarget(this);
		if(strstr(resp,"no password set"))
		{
			box->Activate("","No Rcon password set for this server. Set it first.","badrcon");
			m_pPasswordButton->SetEnabled(false);
		}
		else
		{
			box->Activate("","Bad Rcon Password","badrcon");
		}
		RconPasswordDependant(false);
	} 
	else if(m_bServerNotResponding==false)
	{ // if its not a rcon password problem....
		m_bServerNotResponding = true;
		serveritem_t &server = m_Server->GetServer();
		netadr_t addr;
		memcpy(addr.ip,server.ip,4);
		addr.port=(server.port & 0xff) << 8 | (server.port & 0xff00) >> 8;
		addr.type=NA_IP;

		const char *netString = net->AdrToString(&addr);

		char msg[60];
		_snprintf(msg,60,"Server at \"%s\" failed to respond.",netString);

		MessageBox *dlg = new MessageBox("Server Error", msg);
		dlg->DoModal();
	}

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Connects to the server
//-----------------------------------------------------------------------------
void CGamePanelInfo::ConnectToServer()
{
	m_bConnecting = false;

	if( m_Server==NULL)
	{
		return;
	}

	serveritem_t &server = m_Server->GetServer();

	// check to see if we need a password
	if (server.password && !m_szPassword[0])
	{
		CDialogServerPassword *box = new CDialogServerPassword();
		box->AddActionSignalTarget(this);
		box->Activate(server.name, server.serverID);
		return;
	}

	// check the player count
	if (server.players >= server.maxPlayers)
	{
		// mark why we cannot connect
		m_bServerFull = true;
		// give them access to auto-retry options
		InvalidateLayout();
		return;
	}

	// tell the engine to connect
	char buf[64];
	sprintf(buf, "%d.%d.%d.%d:%d", server.ip[0], server.ip[1], server.ip[2], server.ip[3], server.port);

	const char *gameDir = server.gameDir;
	if (g_pRunGameEngine->IsRunning())
	{
		char command[256];

		// set the server password, if any
		if (m_szPassword[0])
		{
			sprintf(command, "password \"%s\"\n", m_szPassword);
			g_pRunGameEngine->AddTextCommand(command);
		}

		// send engine command to change servers
		sprintf(command, "connect %s\n", buf);
		g_pRunGameEngine->AddTextCommand(command);
	}
	else
	{
		char command[256];
//		sprintf(command, " -game %s +connect %s", gameDir, buf);
		sprintf(command, " +connect %s", buf);
		if (m_szPassword[0])
		{
			strcat(command, " +password \"");
			strcat(command, m_szPassword);
			strcat(command, "\"");\
		}
	
		g_pRunGameEngine->RunEngine(gameDir, command);
		
	//	sprintf(command,"%s/hltv +connect %s",gameDir,buf);
	//	system(command);
	}

}


//-----------------------------------------------------------------------------
// Purpose: opens context menu (user right clicked on a server)
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnOpenContextMenu(int row)
{

	if (m_pRulesListPanel->IsVisible() && m_pRulesListPanel->IsCursorOver() && 
		m_pRulesListPanel->GetNumSelectedRows() && !m_pRcon->Disabled())
		// show the rules changing menu IF its the visible panel and the cursor is
		// over it AND rcon isn't disabled
	{
		// get the cvar 
		unsigned int cvarID =m_pRulesListPanel->GetSelectedRow(0);
		
		// activate context menu
		m_pRulesContextMenu->ShowMenu(this, cvarID);
	}


 }



//-----------------------------------------------------------------------------
// Purpose: handles responses from MULTIPLE dialogs, logic to use is keyed off "type"
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnPlayerDialog(vgui::KeyValues *data)
{
	const char *type=data->GetString("type");
	const char *playerName=data->GetString("player");


	if(!stricmp(type,"kick")) 
	{
		char kickText[255];
		_snprintf(kickText,255,"kick \"%s\"",playerName);
		m_pRcon->SendRcon(kickText);
		OnRefresh();

	} 
	else if(!stricmp(type,"slap")) 
	{
		char slapText[255];
		_snprintf(slapText,255,"admin_command admin_slap %s",playerName);
		m_pRcon->SendRcon(slapText);
		OnRefresh();

	} 	
	else if(!stricmp(type,"chat")) 
	{
		char slapText[255];
		const char *message=data->GetString("value");

		_snprintf(slapText,255,"admin_command admin_psay %s %s",playerName,message);
		m_pRcon->SendRcon(slapText);
		OnRefresh();

	} 
	else if(!stricmp(type,"ban")) 
	{
		const char *valueBan=data->GetString("value");
		float timeBan = (float)atof(valueBan);
		if(timeBan<0.0) 
		{
			timeBan=0.0;
		}

		CUtlVector<Players_t> *players= m_pPlayerList->GetPlayerList();
		int i;
		
		for (i = 0; i < players->Count(); i++)
		{
			if(!_strnicmp(playerName,(*players)[i].name,strlen((*players)[i].name))) 
			{
				break;
			}
		}

		if(i< players->Count()) 
		{
			char banText[255];
			_snprintf(banText,255,"banid %f %s kick",timeBan,(*players)[i].authid);
			m_pRcon->SendRcon(banText);
			m_pRcon->SendRcon("writeid");
			OnRefresh();
		}

	}
	else if(!stricmp(type,"status")) 
	{
		//m_bPlayerStatus=true;
		//	m_pRcon->SendRcon("status",m_szPassword);

	}
	else if(!stricmp(type,"cvar")) 
	{

		char cvar_text[512];
		const char *value=data->GetString("value");
		
		_snprintf(cvar_text,512,"%s %s",playerName,value);
		
		m_pRcon->SendRcon(cvar_text);
		OnRefresh();	

	}
	else if(!stricmp(type,"rconpassword")) 
	{
		const char *value=data->GetString("value");
		m_pRcon->SetPassword(value);
		m_pPlayerList->SetPassword(value);
		m_pBanList->SetPassword(value);
		m_pMapsList->SetPassword(value);
		m_pCMDList->SetPassword(value);
		m_bDoingCmdList=true; // need to query
		//DetectLogAddress(); // redo this now we have a valid pw :-)
		
		serveritem_t &server = m_Server->GetServer();
		v_strncpy(server.rconPassword,value,sizeof(server.rconPassword));

		CServerPage::GetInstance()->UpdateServer(server);
		RconPasswordDependant(true);


		//refresh now we have a new pw
		RequestInfo();

	}
	else if(!stricmp(type,"changemap")) 
	{
		char map_text[512];
		
		_snprintf(map_text,512,"changelevel %s",playerName);
		m_pRcon->SendRcon(map_text);

		// queue a refresh, but give it long enough for the map to actually change
		m_iRequestRetry = system()->GetTimeMillis() + MAP_CHANGE_TIME;
	}
	else if(!stricmp(type,"changeban")) 
	{
		char kickText[255];
		const char *value=data->GetString("value");
		int i;

		CUtlVector<Bans_t> *bans= m_pBanList->GetBanList();
	
		for (i = 0; i < bans->Count(); i++)
		{
			if(!_strnicmp(playerName,(*bans)[i].name,strlen((*bans)[i].name))) 
			{
				break;
			}
		}


		if(i < bans->Count() )
		{
			if((*bans)[i].type== IP)
			{
				_snprintf(kickText,255,"addip %s %s",value,(*bans)[i].name);
			} 
			else // must be an id
			{
				_snprintf(kickText,255,"banid %s %s",value,(*bans)[i].name);
			}

			m_pRcon->SendRcon(kickText);
			OnRefresh();
		}		
	
	}
	else if(!stricmp(type,"removeban")) 
	{
		char kickText[255];
		int i;

		CUtlVector<Bans_t> *bans= m_pBanList->GetBanList();
	
		// search for this user in the ban list
		for (i = 0; i < bans->Count(); i++)
		{
			if(!_strnicmp(playerName,(*bans)[i].name,strlen((*bans)[i].name))) 
			{
				break;
			}
		}


		if(i < bans->Count() )
		{ // found them

			if((*bans)[i].type== IP)
			{
				_snprintf(kickText,255,"removeip %s",(*bans)[i].name);
			} 
			else // must be an id
			{
				_snprintf(kickText,255,"removeid %s",(*bans)[i].name);
			}
			m_pRcon->SendRcon(kickText);
			OnRefresh();
		}		
	


	} 
	else if(!stricmp(type,"addban")) 
	{
		char kickText[255];
		const char *id=data->GetString("id");
		const char *time=data->GetString("time");

		bool ipcheck=data->GetInt("ipcheck");
		if( ipcheck )
		{
			_snprintf(kickText,255,"addip %s %s",time,id);
		}
		else
		{
			_snprintf(kickText,255,"banid %s %s",time,id);
		}
	
		m_pRcon->SendRcon(kickText);
		OnRefresh();
	} 
	else if(!stricmp(type,"stop")) 
	{
		char banText[255];

		_snprintf(banText,255,"Do you REALLY want to restart the server?");
		CDialogKickPlayer*box = new CDialogKickPlayer();
		box->AddActionSignalTarget(this);
		box->SetTitle("Stop the server",true);
		box->Activate("", banText,"stop2");
	}
	else if(!stricmp(type,"stop2")) 
	{
		if(m_pCMDList->QueryCommand("_restart"))
		{ //  if the server has the new restart functionality
			StopLogAddress();
			m_pRcon->SendRcon("_restart");
			m_iRequestRetry = system()->GetTimeMillis() + RESTART_TIME; // do a refresh in 1 second 
			m_bDoRestart = true;
			m_bDoingCmdList=true; // need to query
			//ChangeGame(m_Server->GetServer(), m_szPassword);
			//StartLogAddress();
		}
		else
		{ // else just quit
			m_pRcon->SendRcon("quit");
		}


	//	OnRefresh();
	} 

}

//-----------------------------------------------------------------------------
// Purpose: generic function to produce a dialog box
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnEffectPlayer(int playerID,const char *question, const char *type)
{
	if(playerID!= -1 )  // only kick if a player is selected
	{
		//char kickText[255];
		
		KeyValues *kv = m_pPlayerListPanel->GetSelected();
		if(kv!=NULL) 
		{
			CDialogKickPlayer*box = new CDialogKickPlayer();
			box->AddActionSignalTarget(this);
			box->Activate(kv->GetString("name"), question,type);
		}
		// display a message dialog checking if this is okay	

	}
}

//-----------------------------------------------------------------------------
// Purpose: handles "kick" menu item in the player list
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnKickPlayer(int playerID)
{	
	if(playerID!= -1 )  // only kick if a player is selected
	{	
		KeyValues *kv = m_pPlayerListPanel->GetSelected();
		if(kv!=NULL) 
		{
			char kickText[255];
			_snprintf(kickText,255,"Kick %s?",kv->GetString("name"));
			CDialogKickPlayer*box = new CDialogKickPlayer();
			box->SetTitle("Kick Player",true);
			box->AddActionSignalTarget(this);
			box->Activate(kv->GetString("name"), kickText,"kick");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: handles "ban" menu item in the player list
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnBanPlayer(int playerID)
{
	//OnEffectPlayer(playerID,"Ban this player?","ban");
	if(playerID!= -1)  // only kick if a player is selected
	{
		//char kickText[255];
		
		KeyValues *kv = m_pPlayerListPanel->GetSelected();
		if(kv!=NULL) 
		{
			CDialogCvarChange*box = new CDialogCvarChange();
			box->AddActionSignalTarget(this);
			box->SetTitle("Ban Player",true);
			box->SetLabelText("CvarNameLabel","Name:");
			box->SetLabelText("PasswordLabel","Time:");
			box->Activate(kv->GetString("name"), "0.0","ban","Ban player for how many minutes?");
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: handles "chat" menu item in the player list
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnChatPlayer(int playerID)
{
	//OnEffectPlayer(playerID,"Ban this player?","ban");
	if(playerID!= -1)  // only kick if a player is selected
	{
		//char kickText[255];
		
		KeyValues *kv = m_pPlayerListPanel->GetSelected();
		if(kv!=NULL) 
		{
			CDialogCvarChange*box = new CDialogCvarChange();
			box->AddActionSignalTarget(this);
			box->SetTitle("Chat",true);
			box->Activate(kv->GetString("name"), "","chat","Type message to send");
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: handles "slap" menu item in the player list
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnSlapPlayer(int playerID)
{
	//OnEffectPlayer(playerID,"Ban this player?","ban");
	if(playerID!= -1)  // only kick if a player is selected
	{
		KeyValues *kv = m_pPlayerListPanel->GetSelected();
		if(kv!=NULL) 
		{
			char slapText[255];
			_snprintf(slapText,255,"Slap %s?",kv->GetString("name"));
			CDialogKickPlayer*box = new CDialogKickPlayer();
			box->AddActionSignalTarget(this);
			box->SetTitle("Slap Player",true);
			box->Activate(kv->GetString("name"), slapText,"slap");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: handles "status" menu item in the player list
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnStatusPlayer(int playerID)
{
	OnEffectPlayer(playerID,"Get the status for player?","status");
}



//-----------------------------------------------------------------------------
// Purpose: Gets called when the menu item for changing cvars is selected
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnCvarChange(int cvarID)
{
	KeyValues *kv = m_pRulesListPanel->GetItem(m_pRulesListPanel->GetSelectedRow(0));
	if(kv!=NULL) 
	{
			CDialogCvarChange*box = new CDialogCvarChange();
			box->AddActionSignalTarget(this);
			box->SetTitle("Change CVAR",true);
			box->Activate(kv->GetString("cvar"), kv->GetString("value"),"cvar","Enter the new CVAR value");
			box->RequestFocus();
	}
}




//-----------------------------------------------------------------------------
// Purpose: removes an existing ban from the system
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnBanRemove(int banID)
{
	KeyValues *kv = m_pBanListPanel->GetItem(m_pBanListPanel->GetSelectedRow(0));
	if(kv!=NULL) 
	{
		char banText[255];
	
		_snprintf(banText,255,"Remove ban on ID %s?",kv->GetString("name"));
		CDialogKickPlayer*box = new CDialogKickPlayer();
		box->AddActionSignalTarget(this);
		box->SetTitle("Remove Ban",true);
		box->Activate(kv->GetString("name"), banText,"removeban");
	}
	
}


//-----------------------------------------------------------------------------
// Purpose: produces a dialog asking the player for the new timelimit for a ban
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnBanChange(int banID)
{	
	KeyValues *kv = m_pBanListPanel->GetItem(m_pBanListPanel->GetSelectedRow(0));
	if(kv!=NULL) 
	{	
		char timeText[20];
		float time = kv->GetFloat("time");
		_snprintf(timeText,20,"%0.2f",time);
			
		CDialogCvarChange*box = new CDialogCvarChange();
		box->AddActionSignalTarget(this);
		box->SetTitle("Change Ban",true);

		box->Activate(kv->GetString("name"), timeText,"changeban","Change ban to how many minutes?");

	}

}

//-----------------------------------------------------------------------------
// Purpose: produces a dialog asking a player to enter a new ban
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnBanAdd(int banID)
{	
			
	CDialogAddBan *box = new CDialogAddBan();
	box->AddActionSignalTarget(this);
	box->SetTitle("Add Ban",true);
	box->Activate("addban");
}

//-----------------------------------------------------------------------------
// Purpose: produces a dialog asking a player to enter a new ban
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnStop()
{	
	char banText[255];

	_snprintf(banText,255,"Stop the server?\nNOTE - that the server will take a while to restart.");
	CDialogKickPlayer*box = new CDialogKickPlayer();
	box->AddActionSignalTarget(this);
	box->SetTitle("Stop the server",true);
	box->Activate("", banText,"stop");

}


//-----------------------------------------------------------------------------
// Purpose: displays the help page
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnHelp()
{
	system()->ShellExecute("open", "Admin\\Admin.html");
}

//-----------------------------------------------------------------------------
// Purpose: handles response from the get password dialog
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnJoinServerWithPassword(const char *password)
{
	// copy out the password
	v_strncpy(m_szPassword, password, sizeof(m_szPassword));

	// retry connecting to the server again
	OnConnect();
}

//-----------------------------------------------------------------------------
// Purpose: gets run when the hostname text bos is changed
//-----------------------------------------------------------------------------
void CGamePanelInfo::OnTextChanged(Panel *panel, const char *text)
{

	if(panel==m_pMapsCombo) // trying to change map
	{
		char changemap[30];
		_snprintf(changemap,30,"Change map to %s?",text);
		
		// set the text back to the current map
		SetControlText("MapsCombo", m_Server->GetServer().map);

		// now ask if they want to change map
		CDialogKickPlayer*box = new CDialogKickPlayer();
		box->SetTitle("Change Map",true);
		
		box->AddActionSignalTarget(this);
		box->Activate(text,changemap,"changemap");
	} 
	else if(panel==m_pServerText)  // trying to change host name
	{

	}
}


//-----------------------------------------------------------------------------
// Purpose: send an rcon to the server to stop it sending log address packets to us
//-----------------------------------------------------------------------------
void CGamePanelInfo::StopLogAddress()
{
	// reset the logaddress if we are using rcon on the server
	if(m_bDoingLogging	&& m_pQuery && m_pRcon && !m_pRcon->Disabled() )
		// only do this IF rcon is not disabled
		// the challenge id got found out by initally adding the rcon address
	{
		const netadr_t *a = m_pQuery->GetAddress();
	
		// setup the logaddress redirect for it
		char logaddr_cmd[512];
		if(m_bNewLogAddress)
		{
			_snprintf(logaddr_cmd,512,"logaddress_del %i.%i.%i.%i %i", a->ip[0], a->ip[1], a->ip[2], a->ip[3], ntohs( a->port ) );
		}
		else
		{	
				// hmm, this command "crashes" the servers rcon ability, so lets not do it for now :-)
			//_snprintf(logaddr_cmd,512,"logaddress 127.0.0.1 26999"); // loop the logging back to itself
		}

		m_pRcon->SendRcon(logaddr_cmd);
	}

}

//-----------------------------------------------------------------------------
// Purpose: send an rcon to the server to start it sending log address packets to us
//-----------------------------------------------------------------------------
void CGamePanelInfo::StartLogAddress()
{

	// reset the logaddress if we are using rcon on the server
	if(m_bDoingLogging	&& m_pQuery && m_pRcon && !m_pRcon->Disabled() )
		// only do this IF rcon is not disabled
		// the challenge id got found out by initally adding the rcon address
	{
		const netadr_t *a = m_pQuery->GetAddress();
	
		// setup the logaddress redirect for it
		char logaddr_cmd[512];

		if(m_bNewLogAddress)
		{
			_snprintf(logaddr_cmd,512,"logaddress_add %i.%i.%i.%i %i", a->ip[0], a->ip[1], a->ip[2], a->ip[3], ntohs( a->port ) );
		}
		else
		{
			_snprintf(logaddr_cmd,512,"logaddress %i.%i.%i.%i %i", a->ip[0], a->ip[1], a->ip[2], a->ip[3], ntohs( a->port ) );
		}

		m_pRcon->SendRcon(logaddr_cmd);
	}

}

//-----------------------------------------------------------------------------
// Purpose: single click handler for the rules list, causes tooltip help :)
//----------------------------------------------------------------------------
void CGamePanelInfo::RulesListPanel::OnMousePressed(vgui::MouseCode code)
{	
	// let the base class do its stuff first 
	ListPanel::OnMousePressed(code);

	if (m_pHelpText && GetNumSelectedRows())
	{
		KeyValues *kv = GetItem(GetSelectedRow(0));
		const char *helpText =m_pHelpText->GetHelp(kv->GetString("cvar"));
		if (strlen(helpText)<=0) 
		{
			SetTooltipText("No help on this variable");
		} 
		else
		{
			SetTooltipText( helpText);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: double click handler for the rules list, causes a cvar change dialog :)
//----------------------------------------------------------------------------
void CGamePanelInfo::RulesListPanel::OnMouseDoublePressed(vgui::MouseCode code)
{	
	if (allowDoubleClick && code==MOUSE_LEFT && GetNumSelectedRows())
	{
		unsigned int cvarID =GetSelectedRow(0);
		PostMessage(m_pParent->GetVPanel(),  new KeyValues("cvar", "cvarID", cvarID));
		//return;
	}
	ListPanel::OnMouseDoublePressed(code);
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CGamePanelInfo::m_MessageMap[] =
{
	MAP_MESSAGE( CGamePanelInfo, "Refresh", OnRefresh ),
	MAP_MESSAGE( CGamePanelInfo, "Connect", OnConnect ),
	MAP_MESSAGE( CGamePanelInfo, "Password", OnPassword ),
	MAP_MESSAGE( CGamePanelInfo, "Stop", OnStop ),
	MAP_MESSAGE( CGamePanelInfo, "Help", OnHelp ),

	MAP_MESSAGE_CONSTCHARPTR( CGamePanelInfo, "JoinServerWithPassword", OnJoinServerWithPassword, "password" ),
	// shows the rules list
	MAP_MESSAGE_INT( CGamePanelInfo, "OpenContextMenu", OnOpenContextMenu, "row" ),

	MAP_MESSAGE_INT( CGamePanelInfo, "Kick", OnKickPlayer, "playerID" ), // kick a player menu
	MAP_MESSAGE_INT( CGamePanelInfo, "Ban", OnBanPlayer, "playerID" ),
	MAP_MESSAGE_INT( CGamePanelInfo, "Slap", OnSlapPlayer, "playerID" ),
	MAP_MESSAGE_INT( CGamePanelInfo, "Chat", OnChatPlayer, "playerID" ), // chat to a player
	MAP_MESSAGE_INT( CGamePanelInfo, "cvar", OnCvarChange, "cvarID" ), // change a cvar value

	MAP_MESSAGE_INT( CGamePanelInfo, "removeban", OnBanRemove, "banID" ),
	MAP_MESSAGE_INT( CGamePanelInfo, "changeban", OnBanChange, "banID" ),
	MAP_MESSAGE_INT( CGamePanelInfo, "addban", OnBanAdd, "banID" ),

	// detects map change
	MAP_MESSAGE_PTR_CONSTCHARPTR( CGamePanelInfo, "TextChanged", OnTextChanged, "panel", "text" ),

	//these functions go to the same handler
	MAP_MESSAGE_PARAMS( CGamePanelInfo, "CvarChangeValue", OnPlayerDialog ),
	MAP_MESSAGE_PARAMS( CGamePanelInfo, "KickPlayer", OnPlayerDialog),
	MAP_MESSAGE_PARAMS( CGamePanelInfo, "AddBanValue", OnPlayerDialog),

};

IMPLEMENT_PANELMAP( CGamePanelInfo, Frame );
