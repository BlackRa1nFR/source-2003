//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "CstrikeSpectatorGUI.h"
#include "hud.h"
#include "cs_shareddefs.h"

#include <vgui/ILocalize.h>
#include <cl_dll/IVGuiClientDll.h>
#include <game_controls/IViewPort.h>
#include "cs_gamerules.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCSSpectatorGUI::CCSSpectatorGUI(vgui::Panel *parent) : CSpectatorGUI(parent)
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCSSpectatorGUI::~CCSSpectatorGUI()
{
}



//-----------------------------------------------------------------------------
// Purpose: Resets the list of players
//-----------------------------------------------------------------------------
void CCSSpectatorGUI::UpdateSpectatorPlayerList()
{
	BaseClass::UpdateSpectatorPlayerList();

	bool bFoundT = false, bFoundCT = false;

	for ( int j = 0; j < MAX_TEAMS; j++ )
	{
		VGuiLibraryTeamInfo_t team_info = gViewPortInterface->GetPlayerTeamInfo(j);
		if ( ! team_info.name ) 
			continue;

		if ( !stricmp( team_info.name, "TERRORIST" ) )
		{
			wchar_t frags[ 10 ];
			_snwprintf( frags, sizeof( frags ), L"%i", team_info.frags );
			
			SetLabelText( "TERScoreValue", frags );
			bFoundT = true;
		}
		else if ( !stricmp( team_info.name, "CT" ) )
		{
			wchar_t frags[ 10 ];
			_snwprintf( frags, sizeof( frags ), L"%i",  team_info.frags  );

			SetLabelText( "CTScoreValue", frags );
			bFoundCT = true;
		}

		if ( bFoundT == true && bFoundCT == true )
			 break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the timer label if one exists
//-----------------------------------------------------------------------------
void CCSSpectatorGUI::UpdateTimer()
{
	wchar_t szText[ 63 ];

	int timer;
	timer = (int)( CSGameRules()->TimeRemaining() );
	if ( timer < 0 )
		 timer  = 0;

	_snwprintf ( szText, sizeof( szText ), L"%d:%02d", (timer / 60), (timer % 60) );
	szText[63] = 0;

	SetLabelText("timerlabel", szText );
}


/*bool CCSSpectatorGUI::CanSpectateTeam( int iTeam )
{
	bool bRetVal = true;
	int iTeamOnly = 0;// TODO = gCSViewPortInterface->GetForceCamera();

	// if we're not a spectator or HLTV and iTeamOnly is set
	if ( C_BasePlayer::GetLocalPlayer()->GetTeamNumber() // && !gEngfuncs.IsSpectateOnly() 
	&& iTeamOnly )
	{
		// then we want to force the same team
		if ( C_BasePlayer::GetLocalPlayer()->GetTeamNumber() != iTeam )
		{
			bRetVal = false;
		}
	}

	return bRetVal;
}*/

void CCSSpectatorGUI::Update()
{
	wchar_t szText[ 63 ];

	BaseClass::Update();

	if ( (C_BasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_TERRORIST) || 
		(C_BasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_CT) )
	{
		_snwprintf ( szText, sizeof( szText ), L"$%i", C_CSPlayer::GetLocalCSPlayer()->GetAccount() );
		szText[63] = 0;

		SetLabelText( "extrainfo", szText );
	}
}
