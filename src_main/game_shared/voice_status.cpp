//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "basetypes.h"
#include "hud.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "voice_status.h"
#include "r_efx.h"
#include <vgui_controls/TextImage.h>
#include <vgui/mousecode.h>
#include "cdll_client_int.h"
#include "hud_macros.h"
#include "cdll_util.h"
#include "c_playerresource.h"
#include "cliententitylist.h"
#include "c_baseplayer.h"
#include "materialsystem/imesh.h"
#include "view.h"
#include "convar.h"
#include <vgui_controls/Controls.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include "vgui_BitmapImage.h"
#include "materialsystem/imaterial.h"
#include "tier0/dbg.h"
#include "cdll_int.h"
#include <vgui/IPanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


extern int cam_thirdperson;


#define VOICE_MODEL_INTERVAL		0.3
#define SCOREBOARD_BLINK_FREQUENCY	0.3	// How often to blink the scoreboard icons.
#define SQUELCHOSCILLATE_PER_SECOND	2.0f

ConVar voice_modenable( "voice_modenable", "1", FCVAR_ARCHIVE, "Enable/disable voice in this mod." );
ConVar voice_clientdebug( "voice_clientdebug", "0" );

// ---------------------------------------------------------------------- //
// The voice manager for the client.
// ---------------------------------------------------------------------- //
static CVoiceStatus *g_VoiceStatus = NULL;

CVoiceStatus* GetClientVoiceMgr()
{
	if ( !g_VoiceStatus )
	{
		ClientVoiceMgr_Init();
	}

	return g_VoiceStatus;
}

void ClientVoiceMgr_Init()
{
	if ( g_VoiceStatus )
		return;

	g_VoiceStatus = new CVoiceStatus();
}

void ClientVoiceMgr_Shutdown()
{
	delete g_VoiceStatus;
	g_VoiceStatus = NULL;
}

// ---------------------------------------------------------------------- //
// CVoiceStatus.
// ---------------------------------------------------------------------- //

static CVoiceStatus *g_pInternalVoiceStatus = NULL;

void __MsgFunc_VoiceMask(const char *pszName, int iSize, void *pbuf)
{
	if(g_pInternalVoiceStatus)
		g_pInternalVoiceStatus->HandleVoiceMaskMsg(iSize, pbuf);
}

void __MsgFunc_RequestState(const char *pszName, int iSize, void *pbuf)
{
	if(g_pInternalVoiceStatus)
		g_pInternalVoiceStatus->HandleReqStateMsg(iSize, pbuf);
}


int g_BannedPlayerPrintCount;
void ForEachBannedPlayer(char id[16])
{
	char str[256];
	Q_snprintf(str,sizeof(str), "Ban %d: %2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x\n",
		g_BannedPlayerPrintCount++,
		id[0], id[1], id[2], id[3], 
		id[4], id[5], id[6], id[7], 
		id[8], id[9], id[10], id[11], 
		id[12], id[13], id[14], id[15]
		);
	strupr(str);
	Msg( "%s", str );
}


void ShowBannedCallback()
{
	if(g_pInternalVoiceStatus)
	{
		g_BannedPlayerPrintCount = 0;
		Msg("------- BANNED PLAYERS -------\n");
		g_pInternalVoiceStatus->m_BanMgr.ForEachBannedPlayer(ForEachBannedPlayer);
		Msg("------------------------------\n");
	}
}


// ---------------------------------------------------------------------- //
// CVoiceStatus.
// ---------------------------------------------------------------------- //

CVoiceStatus::CVoiceStatus()
{
	m_nControlSize = 0;
	m_bBanMgrInitialized = false;
	m_LastUpdateServerState = 0;

	m_pSpeakerLabelIcon = NULL;
	m_pScoreboardNeverSpoken = NULL;
	m_pScoreboardNotSpeaking = NULL;
	m_pScoreboardSpeaking = NULL;
	m_pScoreboardSpeaking2 = NULL;
	m_pScoreboardSquelch = NULL;
	m_pScoreboardBanned = NULL;
	
	m_pLocalBitmap = NULL;
	m_pAckBitmap = NULL;

	m_bTalking = m_bServerAcked = false;

	memset(m_pBanButtons, 0, sizeof(m_pBanButtons));

	m_bServerModEnable = -1;

	m_pHeadLabelMaterial = NULL;
}


CVoiceStatus::~CVoiceStatus()
{
	if ( m_pHeadLabelMaterial )
	{
		m_pHeadLabelMaterial->DecrementReferenceCount();
	}

	g_pInternalVoiceStatus = NULL;
	
	for(int i=0; i < MAX_VOICE_SPEAKERS; i++)
	{
		delete m_Labels[i].m_pLabel;
		m_Labels[i].m_pLabel = NULL;

		delete m_Labels[i].m_pIcon;
		m_Labels[i].m_pIcon = NULL;
		
		delete m_Labels[i].m_pBackground;
		m_Labels[i].m_pBackground = NULL;
	}				

	delete m_pLocalLabel;
	m_pLocalLabel = NULL;

	FreeBitmaps();

	const char *pGameDir = engine->GetGameDirectory();
	if( pGameDir )
	{
		if(m_bBanMgrInitialized)
		{
			m_BanMgr.SaveState( pGameDir );
		}
	}
}

static ConCommand voice_showbanned( "voice_showbanned", ShowBannedCallback );

int CVoiceStatus::Init(
	IVoiceStatusHelper *pHelper,
	VPANEL pParentPanel)
{
	const char *pGameDir = engine->GetGameDirectory();
	if( pGameDir )
	{
		m_BanMgr.Init( pGameDir );
		m_bBanMgrInitialized = true;
	}

	Assert(!g_pInternalVoiceStatus);
	g_pInternalVoiceStatus = this;

	m_BlinkTimer = 0;
	memset(m_Labels, 0, sizeof(m_Labels));
	
	for(int i=0; i < MAX_VOICE_SPEAKERS; i++)
	{
		CVoiceLabel *pLabel = &m_Labels[i];

		pLabel->m_pBackground = new Label( NULL, "voicebg", "" );
		pLabel->m_pBackground->SetParent( pParentPanel );
		pLabel->m_pBackground->SetPaintBackgroundEnabled( false );

		if(pLabel->m_pLabel = new Label( pLabel->m_pBackground, "voicelabel", ""))
		{
			pLabel->m_pLabel->SetVisible( true );

			// FIXME: This is busted
			IScheme *pScheme = vgui::scheme()->GetIScheme( vgui::scheme()->GetDefaultScheme() ); 
			pLabel->m_pLabel->SetFont( pScheme->GetFont( "DefaultSmall" ) );
			pLabel->m_pLabel->SetContentAlignment( Label::a_east );
			pLabel->m_pLabel->SetPaintBackgroundEnabled( false );
		}

		if( pLabel->m_pIcon = new CVoicePanel() )
		{
			pLabel->m_pIcon->SetVisible( true );
			pLabel->m_pIcon->SetParent( pLabel->m_pBackground );
		}

		pLabel->m_clientindex = -1;
	}

	m_pHeadLabelMaterial = materials->FindMaterial( "voice/icntlk_pl", NULL );
	m_pHeadLabelMaterial->IncrementReferenceCount();

	m_pLocalLabel = new CVoicePanel();

	m_bInSquelchMode = false;

	m_pHelper = pHelper;
	m_pParentPanel = pParentPanel;

	HOOK_MESSAGE(VoiceMask);
	HOOK_MESSAGE(RequestState);

	return 1;
}


BitmapImage* vgui_LoadMaterial( vgui::VPANEL pParent, const char *pFilename )
{
	return new BitmapImage( pParent, pFilename );
}


void CVoiceStatus::VidInit()
{
	FreeBitmaps();


	if( m_pLocalBitmap = vgui_LoadMaterial(m_pParentPanel, "voice/icntlk_pl") )
	{
		m_pLocalBitmap->SetColor(Color(255,255,255,192));
	}

	if( m_pAckBitmap = vgui_LoadMaterial(m_pParentPanel, "voice/icntlk_sv") )
	{
		m_pAckBitmap->SetColor(Color(255,255,255,192));
	}

	m_pLocalLabel->setImage( m_pLocalBitmap );
	m_pLocalLabel->SetVisible( false );


	if( m_pSpeakerLabelIcon = vgui_LoadMaterial(m_pParentPanel, "voice/speaker4" ) )
		m_pSpeakerLabelIcon->SetColor( Color(255,255,255,254) );		// Give just a tiny bit of translucency so software draws correctly.

	if (m_pScoreboardNeverSpoken = vgui_LoadMaterial(m_pParentPanel, "voice/640_speaker1"))
		m_pScoreboardNeverSpoken->SetColor(Color(255,255,255,254));	// Give just a tiny bit of translucency so software draws correctly.

	if(m_pScoreboardNotSpeaking = vgui_LoadMaterial(m_pParentPanel, "voice/640_speaker2"))
		m_pScoreboardNotSpeaking->SetColor(Color(255,255,255,254));	// Give just a tiny bit of translucency so software draws correctly.
	
	if(m_pScoreboardSpeaking = vgui_LoadMaterial(m_pParentPanel, "voice/640_speaker3"))
		m_pScoreboardSpeaking->SetColor(Color(255,255,255,254));	// Give just a tiny bit of translucency so software draws correctly.
	
	if(m_pScoreboardSpeaking2 = vgui_LoadMaterial(m_pParentPanel, "voice/640_speaker4"))
		m_pScoreboardSpeaking2->SetColor(Color(255,255,255,254));	// Give just a tiny bit of translucency so software draws correctly.
	
	if(m_pScoreboardSquelch  = vgui_LoadMaterial(m_pParentPanel, "voice/icntlk_squelch"))
		m_pScoreboardSquelch->SetColor(Color(255,255,255,254));	// Give just a tiny bit of translucency so software draws correctly.

	if(m_pScoreboardBanned = vgui_LoadMaterial(m_pParentPanel, "voice/640_voiceblocked"))
		m_pScoreboardBanned->SetColor(Color(255,255,255,254));	// Give just a tiny bit of translucency so software draws correctly.
}


void CVoiceStatus::Frame(double frametime)
{
	// check server banned players once per second
	if(gpGlobals->curtime - m_LastUpdateServerState > 1)
	{
		UpdateServerState(false);
	}

	m_BlinkTimer += frametime;

	// Update speaker labels.
	if( m_pHelper->CanShowSpeakerLabels() )
	{
		for( int i=0; i < MAX_VOICE_SPEAKERS; i++ )
			m_Labels[i].m_pBackground->SetVisible( m_Labels[i].m_clientindex != -1 );
	}
	else
	{
		for( int i=0; i < MAX_VOICE_SPEAKERS; i++ )
			m_Labels[i].m_pBackground->SetVisible( false );
	}

	for(int i=0; i < VOICE_MAX_PLAYERS; i++)
		UpdateBanButton(i);
}


void CVoiceStatus::DrawHeadLabels()
{
	if( !m_pHeadLabelMaterial )
		return;

	for(int i=0; i < VOICE_MAX_PLAYERS; i++)
	{
		if ( !m_VoicePlayers[i] )
			continue;
		
		IClientNetworkable *pClient = cl_entitylist->GetClientEntity( i+1 );
		
		// Don't show an icon if the player is not in our PVS.
		if ( !pClient || pClient->IsDormant() )
			continue;

		C_BasePlayer *pPlayer = dynamic_cast<C_BasePlayer*>(pClient);
		if( !pPlayer )
			continue;

		// Don't show an icon for dead or spectating players (ie: invisible entities).
		if( pPlayer->IsPlayerDead() )
			continue;

		Vector vOrigin = pPlayer->GetRenderOrigin() + (CurrentViewUp() * (pPlayer->GetViewOffset()[2] + 20));
		float flSize = 16;

		materials->Bind( m_pHeadLabelMaterial );
		IMesh *pMesh = materials->GetDynamicMesh();
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,0,0 );
		meshBuilder.Position3fv( (vOrigin + (CurrentViewRight() * -flSize) + (CurrentViewUp() * flSize)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,1,0 );
		meshBuilder.Position3fv( (vOrigin + (CurrentViewRight() * flSize) + (CurrentViewUp() * flSize)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,1,1 );
		meshBuilder.Position3fv( (vOrigin + (CurrentViewRight() * flSize) + (CurrentViewUp() * -flSize)).Base() );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f( 1.0, 1.0, 1.0 );
		meshBuilder.TexCoord2f( 0,0,1 );
		meshBuilder.Position3fv( (vOrigin + (CurrentViewRight() * -flSize) + (CurrentViewUp() * -flSize)).Base() );
		meshBuilder.AdvanceVertex();
		meshBuilder.End();
		pMesh->Draw();
	}
}


void CVoiceStatus::UpdateSpeakerStatus(int entindex, bool bTalking)
{
	if(!m_pParentPanel)
		return;

	if( voice_clientdebug.GetInt() )
	{
		Msg( "CVoiceStatus::UpdateSpeakerStatus: ent %d talking = %d\n", entindex, bTalking );
	}

	// Is it the local player talking?
	if( entindex == -1 )
	{
		m_bTalking = !!bTalking;
		if( bTalking )
		{
			// Enable voice for them automatically if they try to talk.
			engine->ClientCmd( "voice_modenable 1" );
		}
	}
	else if( entindex == -2 )
	{
		m_bServerAcked = !!bTalking;
	}
	else if(entindex > 0 && entindex <= VOICE_MAX_PLAYERS)
	{
		int iClient = entindex - 1;
		if(iClient < 0)
			return;

		CVoiceLabel *pLabel = FindVoiceLabel(iClient);
		if(bTalking)
		{
			m_VoicePlayers[iClient] = true;
			m_VoiceEnabledPlayers[iClient] = true;

			// If we don't have a label for this guy yet, then create one.
			if(!pLabel)
			{
				if(pLabel = GetFreeVoiceLabel())
				{
					// Get the name from the engine.
					const char *pName = g_PR ? g_PR->Get_Name(entindex) : "unknown";
					if( !pName )
						pName = "unknown";

					char paddedName[512];
					Q_snprintf( paddedName, sizeof(paddedName), "%s   ", pName );

					int color[3];
					m_pHelper->GetPlayerTextColor( entindex, color );

					if( pLabel->m_pBackground )
					{
						pLabel->m_pBackground->SetParent( m_pParentPanel );
						pLabel->m_pBackground->SetVisible( m_pHelper->CanShowSpeakerLabels() );
					}

					if( pLabel->m_pLabel )
					{
						pLabel->m_pLabel->SetFgColor( Color( 255, 255, 255, 255 ) );
						pLabel->m_pLabel->SetBgColor( Color( 0, 0, 0, 0 ) );
						pLabel->m_pLabel->SetText( paddedName );
					}
					
					pLabel->m_clientindex = iClient;
				}
			}
		}
		else
		{
			m_VoicePlayers[iClient] = false;

			// If we have a label for this guy, kill it.
			if(pLabel)
			{
				pLabel->m_pBackground->SetVisible(false);
				pLabel->m_clientindex = -1;
			}
		}
	}

	RepositionLabels();
}


void CVoiceStatus::UpdateServerState(bool bForce)
{
	// Can't do anything when we're not in a level.
	if( !g_bLevelInitialized )
	{
		if( voice_clientdebug.GetInt() )
		{
			Msg( "CVoiceStatus::UpdateServerState: g_bLevelInitialized\n" );
		}

		return;
	}
	
	int bCVarModEnable = !!voice_modenable.GetInt();
	if(bForce || m_bServerModEnable != bCVarModEnable)
	{
		m_bServerModEnable = bCVarModEnable;

		char str[256];
		Q_snprintf(str, sizeof(str), "VModEnable %d", m_bServerModEnable);
		engine->ServerCmd(str);

		if( voice_clientdebug.GetInt() )
		{
			Msg( "CVoiceStatus::UpdateServerState: Sending '%s'\n", str );
		}
	}

	char str[2048];
	Q_snprintf(str,sizeof(str), "vban");
	bool bChange = false;

	for(unsigned long dw=0; dw < VOICE_MAX_PLAYERS_DW; dw++)
	{	
		unsigned long serverBanMask = 0;
		unsigned long banMask = 0;
		for(unsigned long i=0; i < 32; i++)
		{
			char playerID[16];
			if( !engine->GetPlayerUniqueID(i+1, playerID) )
				continue;

			if(m_BanMgr.GetPlayerBan(playerID))
				banMask |= 1 << i;

			if(m_ServerBannedPlayers[dw*32 + i])
				serverBanMask |= 1 << i;
		}

		if(serverBanMask != banMask)
			bChange = true;

		// Ok, the server needs to be updated.
		char numStr[512];
		Q_snprintf(numStr,sizeof(numStr), " %x", banMask);
		Q_strncat(str, numStr, sizeof(str));
	}

	if(bChange || bForce)
	{
		if( voice_clientdebug.GetInt() )
		{
			Msg( "CVoiceStatus::UpdateServerState: Sending '%s'\n", str );
		}

		engine->ServerCmd( str, false );	// Tell the server..
	}
	else
	{
		if( voice_clientdebug.GetInt() )
		{
			Msg( "CVoiceStatus::UpdateServerState: no change\n" );
		}
	}
	
	m_LastUpdateServerState = gpGlobals->curtime;
}

void CVoiceStatus::UpdateSpeakerImage(Label *pLabel, int iPlayer)
{
	m_pBanButtons[iPlayer-1] = pLabel;
	UpdateBanButton(iPlayer-1);
}

void CVoiceStatus::UpdateBanButton(int iClient)
{
	Label *pPanel = m_pBanButtons[iClient];

	if (!pPanel)
		return;

	char playerID[16];
	if( !engine->GetPlayerUniqueID(iClient+1, playerID) )
		return;

	// Figure out if it's blinking or not.
	bool bBlink   = fmod(m_BlinkTimer, SCOREBOARD_BLINK_FREQUENCY*2) < SCOREBOARD_BLINK_FREQUENCY;
	bool bTalking = !!m_VoicePlayers[iClient];
	bool bSquelch = !IsPlayerAudible(iClient+1);
	bool bBanned  = m_BanMgr.GetPlayerBan(playerID);
	bool bNeverSpoken = !m_VoiceEnabledPlayers[iClient];

	// Get the appropriate image to display on the panel.
	if (bSquelch)
	{
		pPanel->SetImageAtIndex(0, NULL, 0);
	}
	else if (bBanned)
	{
		pPanel->SetImageAtIndex(0,m_pScoreboardBanned,0);
	}
	else if (bTalking)
	{
		if (bBlink)
		{
			pPanel->SetImageAtIndex(0,m_pScoreboardSpeaking2,0);
		}
		else
		{
			pPanel->SetImageAtIndex(0,m_pScoreboardSpeaking,0);
		}
		pPanel->SetFgColor(Color( 255, 170, 0, 1 ));
	}
	else if (bNeverSpoken)
	{
		pPanel->SetImageAtIndex(0,m_pScoreboardNeverSpoken,0);
		pPanel->SetFgColor(Color( 100, 100, 100, 1));
	}
	else
	{
		pPanel->SetImageAtIndex(0,m_pScoreboardNotSpeaking,0);
	}
}


void CVoiceStatus::HandleVoiceMaskMsg(int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	unsigned long dw;
	for(dw=0; dw < VOICE_MAX_PLAYERS_DW; dw++)
	{
		m_AudiblePlayers.SetDWord(dw, (unsigned long)READ_LONG());
		m_ServerBannedPlayers.SetDWord(dw, (unsigned long)READ_LONG());

		if( voice_clientdebug.GetInt())
		{
			Msg("CVoiceStatus::HandleVoiceMaskMsg\n");
			Msg("    - m_AudiblePlayers[%d] = %lu\n", dw, m_AudiblePlayers.GetDWord(dw));
			Msg("    - m_ServerBannedPlayers[%d] = %lu\n", dw, m_ServerBannedPlayers.GetDWord(dw));
		}
	}

	m_bServerModEnable = READ_BYTE();
}

void CVoiceStatus::HandleReqStateMsg(int iSize, void *pbuf)
{
	if(voice_clientdebug.GetInt())
	{
		Msg("CVoiceStatus::HandleReqStateMsg\n");
	}

	UpdateServerState(true);	
}

void CVoiceStatus::StartSquelchMode()
{
	if(m_bInSquelchMode)
		return;

	m_bInSquelchMode = true;
	m_pHelper->UpdateCursorState();
}

void CVoiceStatus::StopSquelchMode()
{
	m_bInSquelchMode = false;
	m_pHelper->UpdateCursorState();
}

bool CVoiceStatus::IsInSquelchMode()
{
	return m_bInSquelchMode;
}

CVoiceLabel* CVoiceStatus::FindVoiceLabel(int clientindex)
{
	for(int i=0; i < MAX_VOICE_SPEAKERS; i++)
	{
		if(m_Labels[i].m_clientindex == clientindex)
			return &m_Labels[i];
	}

	return NULL;
}


CVoiceLabel* CVoiceStatus::GetFreeVoiceLabel()
{
	return FindVoiceLabel(-1);
}


void SetOrUpdateBounds( 
	vgui::Panel *pPanel, 
	int left, int top, int wide, int tall, 
	bool bOnlyUpdateBounds, int &topCoord, int &bottomCoord )
{
	if ( bOnlyUpdateBounds )
	{
		if ( top < topCoord )
			topCoord = top;

		if ( (top+tall) >= bottomCoord )
			bottomCoord = top+tall;
	}
	else
	{
		pPanel->SetBounds( left, top, wide, tall );
	}
}


int CVoiceStatus::RepositionLabels( bool bOnlyCalculateHeight )
{
	// find starting position to draw from, along right-hand side of screen
	int iconWide = 8, iconTall = 8;
	if( m_pSpeakerLabelIcon )
	{
		m_pSpeakerLabelIcon->GetSize( iconWide, iconTall );
	}

	int wide, tall;
	ipanel()->GetSize(m_pParentPanel, wide, tall );


	// Move up if showing local label icon
	int y = tall;

	int sizeX = 0, sizeY = 0;
	if ( m_pLocalBitmap )
	{
		m_pLocalBitmap->GetSize(sizeX, sizeY);
	}
	
	sizeX /= 2;
	sizeY /= 2;

	int x = sizeX;
	
	int topCoord = 100000, bottomCoord = -100000;

	if( m_pLocalBitmap && m_pAckBitmap && m_pLocalLabel && (m_bTalking || m_bServerAcked) )
	{
		m_pLocalLabel->SetParent(m_pParentPanel);
		m_pLocalLabel->SetVisible( true );

		if( m_bServerAcked && !!voice_clientdebug.GetInt() )
		{
			m_pLocalLabel->setImage( m_pAckBitmap );
		}
		else
		{
			m_pLocalLabel->setImage( m_pLocalBitmap );
		}

		SetOrUpdateBounds( m_pLocalLabel, 0, tall-sizeY-2, sizeX, sizeY, bOnlyCalculateHeight, topCoord, bottomCoord );
	}
	else
	{
		m_pLocalLabel->SetVisible( false );
		topCoord = bottomCoord = 0;
	}

	wide -= x;

	// Reposition active labels.( walk backward )
	for(int i = MAX_VOICE_SPEAKERS - 1; i >= 0 ; i-- )
	{
		CVoiceLabel *pLabel = &m_Labels[i];

		if( pLabel->m_clientindex == -1 || !pLabel->m_pLabel )
		{
			if( pLabel->m_pBackground )
				pLabel->m_pBackground->SetVisible( false );

			continue;
		}

		int textWide, textTall;
		pLabel->m_pLabel->GetContentSize( textWide, textTall );

		// Don't let it stretch too far across their screen.
		textWide = wide - iconWide-2;

		// Setup the background label to fit everything in.
		int border = 2;
		int bgTall = max( textTall, iconTall );
		
		// Move up to make room for label
		y -= ( bgTall + 2 );

		SetOrUpdateBounds( pLabel->m_pBackground, x, y, wide, bgTall, bOnlyCalculateHeight, topCoord, bottomCoord );

		if ( bOnlyCalculateHeight )
		{
			// Put the text at the left.
			pLabel->m_pLabel->SetBounds( border, (bgTall - textTall) / 2+1, textWide - border, textTall );

			// Put the icon at the right.
			int iconLeft = textWide;
			int iconTop = (bgTall - iconTall) / 2;
			if( pLabel->m_pIcon )
			{
				pLabel->m_pIcon->setImage( m_pSpeakerLabelIcon );
				pLabel->m_pIcon->SetBounds( iconLeft, iconTop, iconWide, iconTall );
			}
		}
	}

	return bottomCoord-topCoord;
}


void CVoiceStatus::FreeBitmaps()
{
	// Delete all the images we have loaded.
	delete m_pLocalBitmap;
	m_pLocalBitmap = NULL;

	delete m_pAckBitmap;
	m_pAckBitmap = NULL;

	delete m_pSpeakerLabelIcon;
	m_pSpeakerLabelIcon = NULL;

	delete m_pScoreboardNeverSpoken;
	m_pScoreboardNeverSpoken = NULL;

	delete m_pScoreboardNotSpeaking;
	m_pScoreboardNotSpeaking = NULL;

	delete m_pScoreboardSpeaking;
	m_pScoreboardSpeaking = NULL;

	delete m_pScoreboardSpeaking2;
	m_pScoreboardSpeaking2 = NULL;

	delete m_pScoreboardSquelch;
	m_pScoreboardSquelch = NULL;

	delete m_pScoreboardBanned;
	m_pScoreboardBanned = NULL;

	// Clear references to the images in panels.
	for(int i=0; i < VOICE_MAX_PLAYERS; i++)
	{
		if (m_pBanButtons[i])
		{
			m_pBanButtons[i]->SetImageAtIndex(0,NULL,0);
		}
	}

	if(m_pLocalLabel)
		m_pLocalLabel->setImage(NULL);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the target client has been banned
// Input  : playerID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVoiceStatus::IsPlayerBlocked(int iPlayer)
{
	char playerID[16];
	if ( !engine->GetPlayerUniqueID(iPlayer, playerID) )
		return false;

	return m_BanMgr.GetPlayerBan(playerID);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the player can't hear the other client due to game rules (eg. the other team)
// Input  : playerID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVoiceStatus::IsPlayerAudible(int iPlayer)
{
	return !!m_AudiblePlayers[iPlayer-1];
}


//-----------------------------------------------------------------------------
// returns true if the player is currently speaking
//-----------------------------------------------------------------------------
bool CVoiceStatus::IsPlayerSpeaking(int iPlayerIndex)
{
	return m_VoicePlayers[iPlayerIndex-1] != 0;
}


//-----------------------------------------------------------------------------
// Purpose: blocks/unblocks the target client from being heard
// Input  : playerID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CVoiceStatus::SetPlayerBlockedState(int iPlayer, bool blocked)
{
	if (voice_clientdebug.GetInt())
	{
		Msg( "CVoiceStatus::SetPlayerBlockedState part 1\n" );
	}

	char playerID[16];
	if( !engine->GetPlayerUniqueID(iPlayer, playerID) )
		return;

	if (voice_clientdebug.GetInt())
	{
		Msg( "CVoiceStatus::SetPlayerBlockedState part 2\n" );
	}

	// Squelch or (try to) unsquelch this player.
	if (m_AudiblePlayers[iPlayer-1])
	{
		if (voice_clientdebug.GetInt())
		{
			Msg("CVoiceStatus::SetPlayerBlockedState: setting player %d ban to %d\n", iPlayer, !m_BanMgr.GetPlayerBan(playerID));
		}

		m_BanMgr.SetPlayerBan(playerID, !m_BanMgr.GetPlayerBan(playerID));
		UpdateServerState(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns number of pixels required for client area
// Output : int
//-----------------------------------------------------------------------------
int CVoiceStatus::ComputeRequiredHeight( void )
{
	return RepositionLabels( true );
}

//-----------------------------------------------------------------------------
// VOICE PANEL
//-----------------------------------------------------------------------------
CVoicePanel::CVoicePanel( ) :	m_pImage(0)
{
	SetPaintBackgroundEnabled( false );
}
CVoicePanel::~CVoicePanel()
{
}

//-----------------------------------------------------------------------------
// Draws the puppy
//-----------------------------------------------------------------------------
void CVoicePanel::Paint( void )
{
	if ( !m_pImage )
		return;

	int r,g,b,a;
	m_pImage->GetColor( r, g, b, a );
	Color color;
	color.SetColor( r, g, b, a );
	m_pImage->SetColor( color );
	m_pImage->DoPaint( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the panel
//-----------------------------------------------------------------------------
void CVoicePanel::setImage( BitmapImage *pImage )
{
	m_pImage = pImage;
}

