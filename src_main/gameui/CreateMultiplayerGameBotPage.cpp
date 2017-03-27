//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include <time.h>

#include "CreateMultiplayerGameBotPage.h"

using namespace vgui;

#include <KeyValues.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>

#include "FileSystem.h"
#include "PanelListPanel.h"
#include "ScriptObject.h"
#include <tier0/vcrmode.h>

#include "EngineInterface.h"
#include "CvarToggleCheckButton.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// for join team combo box
#define BOT_GUI_TEAM_RANDOM	0
#define BOT_GUI_TEAM_CT			1
#define BOT_GUI_TEAM_T			2

static const char *joinTeamArg[] = { "any", "ct", "t", NULL };

extern void UTIL_StripInvalidCharacters( char *pszInput );


void CCreateMultiplayerGameBotPage::SetJoinTeamCombo( const char *team )
{
	if (team)
	{
		if (!stricmp( team, "T" ))
			m_joinTeamCombo->ActivateItemByRow( BOT_GUI_TEAM_T );
		else if (!stricmp( team, "CT" ))
			m_joinTeamCombo->ActivateItemByRow( BOT_GUI_TEAM_CT );
		else
			m_joinTeamCombo->ActivateItemByRow( BOT_GUI_TEAM_RANDOM );
	}
	else
	{
		m_joinTeamCombo->ActivateItemByRow( BOT_GUI_TEAM_RANDOM );
	}
}

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameBotPage::CCreateMultiplayerGameBotPage( vgui::Panel *parent, const char *name, KeyValues *botKeys ) : PropertyPage( parent, name )
{
	m_allowRogues = new CCvarToggleCheckButton( this, "BotAllowRogueCheck", "", "bot_allow_rogues" );
	m_allowPistols = new CCvarToggleCheckButton( this, "BotAllowPistolsCheck", "", "bot_allow_pistols" );
	m_allowShotguns = new CCvarToggleCheckButton( this, "BotAllowShotgunsCheck", "", "bot_allow_shotguns" );
	m_allowSubmachineGuns = new CCvarToggleCheckButton( this, "BotAllowSubmachineGunsCheck", "", "bot_allow_sub_machine_guns" );
	m_allowRifles = new CCvarToggleCheckButton( this, "BotAllowRiflesCheck", "", "bot_allow_rifles" );
	m_allowMachineGuns = new CCvarToggleCheckButton( this, "BotAllowMachineGunsCheck", "", "bot_allow_machine_guns" );
	m_allowGrenades = new CCvarToggleCheckButton( this, "BotAllowGrenadesCheck", "", "bot_allow_grenades" );
	m_allowSnipers = new CCvarToggleCheckButton( this, "BotAllowSnipersCheck", "", "bot_allow_snipers" );
	m_allowShields = new CCvarToggleCheckButton( this, "BotAllowShieldCheck", "", "bot_allow_shield" );

	m_joinAfterPlayer = new CCvarToggleCheckButton( this, "BotJoinAfterPlayerCheck", "", "bot_join_after_player" );

	// set up team join combo box
	// NOTE: If order of AddItem is changed, update the associated #defines
	m_joinTeamCombo = new ComboBox( this, "BotJoinTeamCombo", 3, false );
	m_joinTeamCombo->AddItem( "Random", NULL );
	m_joinTeamCombo->AddItem( "Counter-Terrorist", NULL );
	m_joinTeamCombo->AddItem( "Terrorist", NULL );

	// create text entry fields for quota and prefix
	m_quotaEntry = new TextEntry( this, "BotQuotaEntry" );
	m_prefixEntry = new TextEntry( this, "BotPrefixEntry" );

	// set positions and sizes from resources file
	LoadControlSettings( "Resource/CreateMultiplayerGameBotPage.res" );

	// set initial values
//	if (engine->pfnGetCvarPointer( "bot_difficulty" ))
	ConVar const *var = cvar->FindVar( "bot_difficulty" );
	if ( var )
	{
		// cvars exist - get initial values directly from them
		SetJoinTeamCombo( var->GetString() );

		char buffer[128];
		var = cvar->FindVar( "bot_quota" );
		if ( var )
		{
			sprintf( buffer, "%d", var->GetInt() );
			m_quotaEntry->SetText( buffer );
		}

		var = cvar->FindVar( "bot_prefix" );
		if ( var )
		{
			SetControlString( "BotPrefixEntry", var->GetString() );
		}
	}
	else
	{
		// cvars do not exist, get initial values from bot keys
		m_joinAfterPlayer->SetSelected( botKeys->GetInt( "bot_join_after_player" ) );
		m_allowRogues->SetSelected( botKeys->GetInt( "bot_allow_rogues" ) );
		m_allowPistols->SetSelected( botKeys->GetInt( "bot_allow_pistols" ) );
		m_allowShotguns->SetSelected( botKeys->GetInt( "bot_allow_shotguns" ) );
		m_allowSubmachineGuns->SetSelected( botKeys->GetInt( "bot_allow_sub_machine_guns" ) );
		m_allowMachineGuns->SetSelected( botKeys->GetInt( "bot_allow_machine_guns" ) );
		m_allowRifles->SetSelected( botKeys->GetInt( "bot_allow_rifles" ) );
		m_allowSnipers->SetSelected( botKeys->GetInt( "bot_allow_snipers" ) );
		m_allowGrenades->SetSelected( botKeys->GetInt( "bot_allow_grenades" ) );
		m_allowShields->SetSelected( botKeys->GetInt( "bot_allow_shield" ) );

		SetJoinTeamCombo( botKeys->GetString( "bot_join_team" ) );

		// set bot_prefix
		const char *prefix = botKeys->GetString( "bot_prefix" );
		if (prefix)
			SetControlString( "BotPrefixEntry", prefix );

		// set bot_quota
		char buffer[128];
		sprintf( buffer, "%d", botKeys->GetInt( "bot_quota" ) );
		m_quotaEntry->SetText( buffer );
	}
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CCreateMultiplayerGameBotPage::~CCreateMultiplayerGameBotPage()
{
	delete m_allowRogues;
	delete m_allowPistols;
	delete m_allowShotguns;
	delete m_allowSubmachineGuns;
	delete m_allowMachineGuns;
	delete m_allowRifles;
	delete m_allowSnipers;
	delete m_allowGrenades;
	delete m_allowShields;
	delete m_joinAfterPlayer;
	delete m_joinTeamCombo;
	delete m_quotaEntry;
	delete m_prefixEntry;
}

//-----------------------------------------------------------------------------
// Reset values
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameBotPage::OnResetChanges()
{
}

//-----------------------------------------------------------------------------
// Put all cvars and their current values into 'botKeys'
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameBotPage::UpdateKeys( KeyValues *botKeys )
{
	botKeys->SetInt( "bot_join_after_player", m_joinAfterPlayer->IsSelected() );
	botKeys->SetInt( "bot_allow_rogues", m_allowRogues->IsSelected() );
	botKeys->SetInt( "bot_allow_pistols", m_allowPistols->IsSelected() );
	botKeys->SetInt( "bot_allow_shotguns", m_allowShotguns->IsSelected() );
	botKeys->SetInt( "bot_allow_sub_machine_guns", m_allowSubmachineGuns->IsSelected() );
	botKeys->SetInt( "bot_allow_machine_guns", m_allowMachineGuns->IsSelected() );
	botKeys->SetInt( "bot_allow_rifles", m_allowRifles->IsSelected() );
	botKeys->SetInt( "bot_allow_snipers", m_allowSnipers->IsSelected() );
	botKeys->SetInt( "bot_allow_grenades", m_allowGrenades->IsSelected() );
	botKeys->SetInt( "bot_allow_shield", m_allowShields->IsSelected() );

	// set bot_join_team
	botKeys->SetString( "bot_join_team", joinTeamArg[ m_joinTeamCombo->GetActiveItem() ] );

	// set bot_prefix
	#define BUF_LENGTH 256
	char entryBuffer[ BUF_LENGTH ];
	m_prefixEntry->GetText( entryBuffer, BUF_LENGTH );
	botKeys->SetString( "bot_prefix", entryBuffer );

	// set bot_quota
	m_quotaEntry->GetText( entryBuffer, BUF_LENGTH );
	botKeys->SetInt( "bot_quota", atoi( entryBuffer ) );
}

//-----------------------------------------------------------------------------
// Called to get data from the page
//-----------------------------------------------------------------------------
void CCreateMultiplayerGameBotPage::OnApplyChanges()
{
	// all values are sent via console cvar commands in CreateMultiplayerGameDialog.cpp
}

