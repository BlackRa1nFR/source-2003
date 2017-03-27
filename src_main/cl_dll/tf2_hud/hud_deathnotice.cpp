//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Draws TF2's death notices
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hud_macros.h"
#include "vgui_ScorePanel.h"
#include "c_playerresource.h"
#include "parsemsg.h"
#include "iclientmode.h"
#include <vgui_controls/controls.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include "c_baseplayer.h"
#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );

#define MAX_PLAYER_NAME_LENGTH		64

// Player entries in a death notice
struct DeathNoticePlayer
{
	char		szName[MAX_PLAYER_NAME_LENGTH];
	int			iEntIndex;
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem 
{
	DeathNoticePlayer	Killer;
	DeathNoticePlayer   Victim;
	DeathNoticePlayer   Assist;
	CHudTexture *iconDeath;
	int			iSuicide;
	int			iTeamKill;
	float		flDisplayTime;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDeathNotice : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDeathNotice, vgui::Panel );
public:
	CHudDeathNotice( const char *pElementName );
	void Init( void );
	void VidInit( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	void SetColorForNoticePlayer( C_BasePlayer *pPlayer, int iTeamNumber );
	void RetireExpiredDeathNotices( void );
	void MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf );

private:

	CPanelAnimationVarAliasType( float, m_flLineHeight, "LineHeight", "15", "proportional_float" );

	CPanelAnimationVar( float, m_flMaxDeathNotices, "MaxDeathNotices", "4" );
	CPanelAnimationVar( bool, m_bRightJustify, "RightJustify", "1" );

	CPanelAnimationVar( Color, m_clrFriendlyText, "FriendlyTextColor", "FriendlyTextColor" );
	CPanelAnimationVar( Color, m_clrEnemyText, "EnemyTextColor", "EnemyTextColor" );
	CPanelAnimationVar( Color, m_clrMyKillsText, "MyKillsTextColor", "MyKillsTextColor" );

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudNumbersTimer" );

	ScorePanel		*m_pScorePanel;
	// Texture for skull symbol
	CHudTexture		*m_iconD_skull;  

	CUtlVector<DeathNoticeItem> m_DeathNotices;
};

using namespace vgui;

DECLARE_HUDELEMENT( CHudDeathNotice );
DECLARE_HUD_MESSAGE( CHudDeathNotice, DeathMsg );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDeathNotice::CHudDeathNotice( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudDeathNotice" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Init( void )
{
	HOOK_MESSAGE( DeathMsg );

	m_pScorePanel = GET_HUDELEMENT( ScorePanel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::VidInit( void )
{
	m_iconD_skull = gHUD.GetIcon( "d_skull" );
	m_DeathNotices.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CHudDeathNotice::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( m_DeathNotices.Count() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::SetColorForNoticePlayer( C_BasePlayer *pPlayer, int iTeamNumber )
{
	// Set the right color
	if ( pPlayer == C_BasePlayer::GetLocalPlayer() )
	{
		surface()->DrawSetTextColor( m_clrMyKillsText );
	}
	else if ( GetLocalTeam() && GetLocalTeam()->GetTeamNumber() == iTeamNumber )
	{
		surface()->DrawSetTextColor( m_clrFriendlyText );
	}
	else
	{
		surface()->DrawSetTextColor( m_clrEnemyText );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Paint()
{
	surface()->DrawSetTextFont( m_hTextFont );
	surface()->DrawSetTextColor( m_clrFriendlyText );

	int iCount = m_DeathNotices.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CHudTexture *icon = m_DeathNotices[i].iconDeath;
		if ( !icon )
			continue;

		// Get the team numbers for the players involved
		C_BasePlayer *pKiller = static_cast<C_BasePlayer*>(cl_entitylist->GetEnt( m_DeathNotices[i].Killer.iEntIndex ));
		int iKillerTeam = pKiller ? pKiller->GetTeamNumber() : 0;
		C_BasePlayer *pAssist = NULL;
		if ( m_DeathNotices[i].Assist.iEntIndex )
		{
			pAssist = static_cast<C_BasePlayer*>(cl_entitylist->GetEnt( m_DeathNotices[i].Assist.iEntIndex ));
		}
		C_BasePlayer *pVictim = static_cast<C_BasePlayer*>(cl_entitylist->GetEnt( m_DeathNotices[i].Victim.iEntIndex ));
		int iVictimTeam = pVictim ? pVictim->GetTeamNumber() : 0;

		// Get the local position for this notice
		int len = UTIL_ComputeStringWidth( m_hTextFont, m_DeathNotices[i].Victim.szName );
		int y = (m_flLineHeight * i);
		int x;
		if ( m_bRightJustify )
		{
			x = GetWide() - len - icon->Width() - 5;
		}
		else
		{
			x = 0;
		}

		// Only draw killers name if it wasn't a suicide
		if ( !m_DeathNotices[i].iSuicide )
		{
			if ( m_bRightJustify )
			{
				x -= UTIL_ComputeStringWidth( m_hTextFont, m_DeathNotices[i].Killer.szName );
				if ( pAssist )
				{
					x -= UTIL_ComputeStringWidth( m_hTextFont, " and " );
					x -= UTIL_ComputeStringWidth( m_hTextFont, m_DeathNotices[i].Assist.szName );
				}
			}

			SetColorForNoticePlayer( pKiller, iKillerTeam );

			// Draw killer's name
			surface()->DrawSetTextPos( x, y );
			char *p = m_DeathNotices[i].Killer.szName;
			while ( *p )
			{
				surface()->DrawUnicodeChar( *p++ );
			}
			surface()->DrawGetTextPos( x, y );

			// Did we have an assistant?
			if ( pAssist )
			{
				// Draw the conjuction
				char *sAnd = " and ";
				surface()->DrawSetTextPos( x, y );
				char *p = sAnd;
				while ( *p )
				{
					surface()->DrawUnicodeChar( *p++ );
				}
				surface()->DrawGetTextPos( x, y );

				// Draw assistants's name
				surface()->DrawSetTextPos( x, y );

				// Draw assistants's name
				surface()->DrawSetTextPos( x, y );
				p = m_DeathNotices[i].Assist.szName;
				while ( *p )
				{
					surface()->DrawUnicodeChar( *p++ );
				}
				surface()->DrawGetTextPos( x, y );
			}
		}

		Color iconColor( 255, 80, 0, 255 );
		if ( m_DeathNotices[i].iTeamKill )
		{
			// display it in sickly green
			iconColor = Color( 10, 240, 10, 255 );
		}

		// Draw death weapon
		icon->DrawSelf( x, y, iconColor );
		x += icon->Width();

		SetColorForNoticePlayer( pVictim, iVictimTeam );

		// Draw victims name
		surface()->DrawSetTextPos( x, y );
		char *p = m_DeathNotices[i].Victim.szName;
		while ( *p )
		{
			surface()->DrawUnicodeChar( *p++ );
		}
	}

	// Now retire any death notices that have expired
	RetireExpiredDeathNotices();
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CHudDeathNotice::RetireExpiredDeathNotices( void )
{
	// Loop backwards because we might remove one
	int iSize = m_DeathNotices.Size();
	for ( int i = iSize-1; i >= 0; i-- )
	{
		if ( m_DeathNotices[i].flDisplayTime < gpGlobals->curtime )
		{
			m_DeathNotices.Remove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CHudDeathNotice::MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf )
{
	if (!m_pScorePanel || !g_PR)
		return;

	BEGIN_READ( pbuf, iSize );

	int killer = READ_BYTE();
	int assist = READ_BYTE();
	int victim = READ_BYTE();

	char killedwith[32];
	char fullkilledwith[128];
	Q_strncpy( killedwith, READ_STRING(), sizeof(killedwith) );
	if ( killedwith && *killedwith )
	{
		Q_snprintf( fullkilledwith, sizeof(fullkilledwith), "d_%s", killedwith );
	}
	else
	{
		fullkilledwith[0] = 0;
	}

	m_pScorePanel->DeathMsg( killer, victim );

	// Do we have too many death messages in the queue?
	if ( m_DeathNotices.Count() > 0 &&
		m_DeathNotices.Count() >= (int)m_flMaxDeathNotices )
	{
		// Remove the oldest one in the queue, which will always be the first
		m_DeathNotices.Remove(0);
	}

	// Get the names of the players
	char *killer_name = g_PR->Get_Name( killer );
	char *assist_name = g_PR->Get_Name( assist );
	char *victim_name = g_PR->Get_Name( victim );
	if ( !killer_name )
		killer_name = "";
	if ( !assist_name )
		assist_name = "";
	if ( !victim_name )
		victim_name = "";

	// Make a new death notice
	DeathNoticeItem deathMsg;
	deathMsg.Killer.iEntIndex = killer;
	deathMsg.Assist.iEntIndex = assist;
	deathMsg.Victim.iEntIndex = victim;
	Q_strncpy( deathMsg.Killer.szName, killer_name, MAX_PLAYER_NAME_LENGTH );
	Q_strncpy( deathMsg.Assist.szName, assist_name, MAX_PLAYER_NAME_LENGTH );
	Q_strncpy( deathMsg.Victim.szName, victim_name, MAX_PLAYER_NAME_LENGTH );
	deathMsg.flDisplayTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.iSuicide = ( !killer || killer == victim );
	deathMsg.iTeamKill = ( !strcmp( fullkilledwith, "d_teammate" ) );

	// Try and find the death identifier in the icon list
	deathMsg.iconDeath = gHUD.GetIcon( fullkilledwith );
	if ( !deathMsg.iconDeath )
	{
		// Can't find it, so use the default skull & crossbones icon
		deathMsg.iconDeath = m_iconD_skull;
	}

	// Add it to our list of death notices
	m_DeathNotices.AddToTail( deathMsg );

	char sDeathMsg[512];

	// Record the death notice in the console
	if ( deathMsg.iSuicide )
	{
		if ( !strcmp( fullkilledwith, "d_world" ) )
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s died.\n", deathMsg.Victim.szName );
		}
		else
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s suicided.\n", deathMsg.Victim.szName );
		}
	}
	else if ( deathMsg.iTeamKill )
	{
		Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s teamkilled %s", deathMsg.Killer.szName, deathMsg.Victim.szName );
	}
	else
	{
		if ( assist )
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s and %s killed %s", deathMsg.Killer.szName, deathMsg.Assist.szName, deathMsg.Victim.szName );
		}
		else
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s killed %s", deathMsg.Killer.szName, deathMsg.Victim.szName );
		}
	}

	if ( fullkilledwith && *fullkilledwith && (*fullkilledwith > 13 ) && strcmp( fullkilledwith, "d_world" ) && !deathMsg.iTeamKill )
	{
		Q_strncat( sDeathMsg, VarArgs( " with %s.\n", fullkilledwith+2 ), sizeof( sDeathMsg ) );
	}

	Msg( sDeathMsg );
}



