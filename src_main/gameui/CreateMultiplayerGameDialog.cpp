//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "CreateMultiplayerGameDialog.h"
#include "CreateMultiplayerGameServerPage.h"
#include "CreateMultiplayerGameGameplayPage.h"
#include "CreateMultiplayerGameBotPage.h"

#include "EngineInterface.h"
#include "ModInfo.h"

#include <stdio.h>

using namespace vgui;

#include "Taskbar.h"
extern CTaskbar *g_pTaskbar;

#include <vgui/ILocalize.h>

#include "FileSystem.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// CS
#include <vgui_controls/RadioButton.h>

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameDialog::CCreateMultiplayerGameDialog(vgui::Panel *parent) : PropertyDialog(parent, "CreateMultiplayerGameDialog")
{
	SetSize(424, 420);
	
	g_pTaskbar->AddTask(GetVPanel());

	SetTitle("#GameUI_CreateServer", true);
	SetOKButtonText("#GameUI_Start");

	m_pServerPage = new CCreateMultiplayerGameServerPage(this, "ServerPage");
	AddPage(m_pServerPage, "#GameUI_Server");
	AddPage(new CCreateMultiplayerGameGameplayPage(this, "GameplayPage"), "#GameUI_Game");

	// create KeyValues object to load/save bot options
	m_pBotSavedData = new KeyValues( "CSBotConfig" );

	// load the bot config data
	if (m_pBotSavedData)
	{
		m_pBotSavedData->LoadFromFile( filesystem(), "CSBotConfig.vdf", "CONFIG" );
	}
	// add a page of advanced bot controls
	// NOTE: These controls will use the bot keys to initialize their values
	m_pBotPage = new CCreateMultiplayerGameBotPage( this, "BotPage", m_pBotSavedData );
	AddPage( m_pBotPage, "CPU Player Options" );

//	int botDiff = (int)(engine->pfnGetCvarFloat( "bot_difficulty" ));
	ConVar const *var = cvar->FindVar( "bot_difficulty" );
	int botDiff = 0;
	if ( var )
	{
		botDiff = var->GetInt();
	}
	else
	{
		// no, use the current key value loaded from the config file
		botDiff = m_pBotSavedData->GetInt( "bot_difficulty" );
	}

/*	// do bot cvars exist yet?
	if (engine->pfnGetCvarPointer( "bot_difficulty" ))
	{
		// yes, use the current value of the cvar
		botDiff = (int)(engine->pfnGetCvarFloat( "bot_difficulty" ));
	}
	else
	{
		// no, use the current key value loaded from the config file
		botDiff = m_pBotSavedData->GetInt( "bot_difficulty" );
	}
*/	
	// shift to array index range
	++botDiff;

	// set bot difficulty radio buttons
	static const char *buttonName[] = 
	{
		"NoBots",
		"SkillLevelEasy",
		"SkillLevelNormal",
		"SkillLevelHard",
		"SkillLevelExpert"
	};
	if (botDiff < 0)
		botDiff = 0;
	else if (botDiff > 4)
		botDiff = 4;
	vgui::RadioButton *button = static_cast<vgui::RadioButton *>( m_pServerPage->FindChildByName( buttonName[ botDiff ] ) );
	if (button)
		button->SetSelected( true );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameDialog::~CCreateMultiplayerGameDialog()
{
	if (m_pBotSavedData)
	{
		m_pBotSavedData->deleteThis();
		m_pBotSavedData = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Overrides the base class so it can setup the taskbar title properly
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameDialog::SetTitle(const char *title, bool surfaceTitle)
{
	BaseClass::SetTitle(title,surfaceTitle);
	if (g_pTaskbar)
	{
		wchar_t *wTitle;
		wchar_t w_szTitle[1024];

		wTitle = vgui::localize()->Find(title);

		if(!wTitle)
		{
			vgui::localize()->ConvertANSIToUnicode(title,w_szTitle,sizeof(w_szTitle));
			wTitle = w_szTitle;
		}

		g_pTaskbar->SetTitle(GetVPanel(), wTitle);
	}


}

//-----------------------------------------------------------------------------
// Purpose: runs the server when the OK button is pressed
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameDialog::OnOK()
{
	BaseClass::OnOK();

	// get these values from m_pServerPage and store them temporarily
	char szMapName[64], szHostName[64], szPassword[64];
	strncpy(szMapName, m_pServerPage->GetMapName(), sizeof( szMapName ));
	strncpy(szHostName, m_pServerPage->GetHostName(), sizeof( szHostName ));
	strncpy(szPassword, m_pServerPage->GetPassword(), sizeof( szPassword ));

	char szMapCommand[1024];

	//
	// Changes for Counter-strike game dialog - MSB
	//
	if (!stricmp( ModInfo().GetGameDescription(), "Counter-Strike" ))
	{
		// get bot difficulty level
		float botSkill = -1.0f;

		vgui::RadioButton *noBots = static_cast<vgui::RadioButton *>( m_pServerPage->FindChildByName( "NoBots" ) );
		if (noBots && !noBots->IsSelected())
		{
			// "not no bots" -> use bots

			vgui::RadioButton *easy = static_cast<vgui::RadioButton *>( m_pServerPage->FindChildByName( "SkillLevelEasy" ) );
			if (easy && easy->IsSelected())
				botSkill = 0.0f;

			vgui::RadioButton *normal = static_cast<vgui::RadioButton *>( m_pServerPage->FindChildByName( "SkillLevelNormal" ) );
			if (normal && normal->IsSelected())
				botSkill = 1.0f;

			vgui::RadioButton *hard = static_cast<vgui::RadioButton *>( m_pServerPage->FindChildByName( "SkillLevelHard" ) );
			if (hard && hard->IsSelected())
				botSkill = 2.0f;

			vgui::RadioButton *expert = static_cast<vgui::RadioButton *>( m_pServerPage->FindChildByName( "SkillLevelExpert" ) );
			if (expert && expert->IsSelected())
				botSkill = 3.0f;
		}

		// update keys
		m_pBotSavedData->SetInt( "bot_difficulty", (int)botSkill );
		m_pBotPage->UpdateKeys( m_pBotSavedData );

		// if don't want bots (difficulty == -1), override and set the quota to zero
		if (m_pBotSavedData->GetInt( "bot_difficulty" ) < 0)
		{
			m_pBotSavedData->SetInt( "bot_quota", 0 );
		}

		// save bot config to a file
		m_pBotSavedData->SaveToFile( filesystem(), "CSBotConfig.vdf", "CONFIG" );

		// create command to load map, set password, etc
		sprintf( szMapCommand, "disconnect\nsv_lan 1\nsetmaster enable\nmaxplayers %i\nsv_password \"%s\"\nhostname \"%s\"\nmap %s\n",
			m_pServerPage->GetMaxPlayers(),
			szPassword,
			szHostName,
			szMapName
		);

		// start loading the map
		engine->ClientCmd(szMapCommand);

		// wait a few frames after the map loads to make sure everything is "settled"
		engine->ClientCmd( "wait\nwait\n" );

		// set all of the bot cvars
		for( KeyValues *key = m_pBotSavedData->GetFirstSubKey(); key; key = key->GetNextKey() )
		{
			static char buffer[128];

			if (key->GetDataType() == KeyValues::TYPE_STRING)
				sprintf( buffer, "%s \"%s\"\n", key->GetName(), key->GetString() );
			else
				sprintf( buffer, "%s %d\n", key->GetName(), key->GetInt() );

			engine->ClientCmd( buffer );
		}

		// if bots are forced onto a specific team, we must disable team balancing and stacking checks
		const char *team = m_pBotSavedData->GetString( "bot_join_team" );
		if (team && stricmp( team, "any" ))
		{
			engine->ClientCmd( "mp_autoteambalance 0\n" );
			engine->ClientCmd( "mp_limitteams 0\n" );
		}

	}
	else
	{
		// create the command to execute
		sprintf(szMapCommand, "disconnect\nsv_lan 1\nsetmaster enable\nmaxplayers %i\nsv_password \"%s\"\nhostname \"%s\"\nmap %s\n",
			m_pServerPage->GetMaxPlayers(),
			szPassword,
			szHostName,
			szMapName
		);

		// exec
		engine->ClientCmd(szMapCommand);
	}
}

//-----------------------------------------------------------------------------
// Purpose: deletes the dialog when it gets closed
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameDialog::OnClose()
{
	MarkForDeletion();
	BaseClass::OnClose();
}
