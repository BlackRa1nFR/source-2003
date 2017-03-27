//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Normal HUD mode
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hud_chat.h"
#include "clientmode_tfnormal.h"
#include "clientmode.h"
#include "hud.h"
#include "iinput.h"
#include "c_basetfplayer.h"
#include "vgui_int.h"
#include "hud_timer.h"
#include "usercmd.h"
#include "in_buttons.h"
#include "vgui_scorepanel.h"
#include "vgui_vprofpanel.h"
#include "c_tf_playerclass.h"
#include "engine/IEngineSound.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar hud_autoreloadscript;
extern ConVar cl_drawhud;

//-----------------------------------------------------------------------------
// Purpose: Instance the singleton and expose the interface to it.
// Output : IClientMode
//-----------------------------------------------------------------------------
IClientMode *GetClientModeNormal( void )
{
	static ClientModeTFNormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::Init( void )
{
	BaseClass::Init();
	m_Viewport.LoadControlSettings("scripts/HudLayout.res");
//	Layout();
}

ClientModeTFNormal::Viewport::Viewport() : 
	vgui::EditablePanel( NULL, "ClientModeTFNormalViewport"), m_CursorNone(vgui::dc_none)
{
	SetCursor( m_CursorNone );

	// use a custom scheme for the hud
	m_bHumanScheme = true;
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile("resource/ClientSchemeHuman.res", "HudScheme");
	SetScheme(scheme);
	SetProportional(true);

	// create our animation controller
	m_pAnimController = new vgui::AnimationController(this);
	m_pAnimController->SetScheme(scheme);
	m_pAnimController->SetProportional(true);
	if (!m_pAnimController->SetScriptFile("scripts/HudAnimations.txt"))
	{
		Assert(0);
	}
}

void ClientModeTFNormal::Viewport::Enable()
{
}

void ClientModeTFNormal::Viewport::Disable()
{
}

void ClientModeTFNormal::Viewport::OnThink()
{
	m_pAnimController->UpdateAnimations( gpGlobals->curtime );

	// check the auto-reload cvar
	m_pAnimController->SetAutoReloadScript(hud_autoreloadscript.GetBool());

	// See if scheme should change
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return;
	int team = pPlayer->GetTeamNumber();
	if ( !team )
		return;

	bool human = ( team == TEAM_HUMANS ) ? true : false;
	if ( human != m_bHumanScheme )
	{
		ReloadScheme();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::Viewport::ReloadScheme()
{
	// See if scheme should change
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return;
	int team = pPlayer->GetTeamNumber();
	if ( team )
	{
		bool human = ( team == TEAM_HUMANS ) ? true : false;
		m_bHumanScheme = human;
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile( human ?
			"resource/ClientSchemeHuman.res" :
			"resource/ClientSchemeAlien.res", "HudScheme");
		SetScheme(scheme);
		SetProportional( true );
		m_pAnimController->SetScheme(scheme);
	}

	// Force a reload
	if ( !m_pAnimController->SetScriptFile("scripts/HudAnimations.txt", true) )
	{
		Assert( 0 );
	}

	SetProportional( true );
	// reload the .res file from disk
	// scheme()->ReloadSchemes();
	LoadControlSettings("scripts/HudLayout.res");

	gHUD.RefreshHudTextures();

	InvalidateLayout( true, true );
}

CON_COMMAND( hud_reloadscheme, "Reloads hud layout and animation scripts." )
{
	ClientModeTFNormal *mode = ( ClientModeTFNormal * )GetClientModeNormal();
	if ( !mode )
		return;

	mode->ReloadScheme();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeTFNormal::ClientModeTFNormal() :
	m_CursorNone(vgui::dc_none)
{
}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModeTFNormal::~ClientModeTFNormal()
{
}

void ClientModeTFNormal::ReloadScheme( void )
{
	m_Viewport.ReloadScheme();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : vgui::Panel
//-----------------------------------------------------------------------------
vgui::Panel *ClientModeTFNormal::GetMinimapParent( void )
{
	return GetViewport();
}

//-----------------------------------------------------------------------------
// Purpose: Enable this viewport
//-----------------------------------------------------------------------------
void ClientModeTFNormal::Enable()
{
	vgui::VPANEL pRoot;

	// Add our viewport to the root panel.
	if(pRoot = VGui_GetClientDLLRootPanel() )
	{
		m_Viewport.SetParent( pRoot );
	}

	m_Viewport.SetCursor( m_CursorNone );
	vgui::surface()->SetCursor( m_CursorNone );

	m_Viewport.SetVisible( true );

	BaseClass::Enable();

	Layout();
}

//-----------------------------------------------------------------------------
// Purpose: Disable this viewport
//-----------------------------------------------------------------------------
void ClientModeTFNormal::Disable()
{
	BaseClass::Disable();

	vgui::VPANEL pRoot;

	// Remove our viewport from the root panel.
	if(pRoot = VGui_GetClientDLLRootPanel() )
	{
		m_Viewport.SetParent( (vgui::VPANEL)NULL );
	}

	m_Viewport.SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::Update()
{
	m_Viewport.SetVisible( cl_drawhud.GetBool() );
	BaseClass::Update();
	HudCommanderOverlayMgr()->Tick( );
}

//-----------------------------------------------------------------------------
// Purpose: Set viewport bounds, etc.
//-----------------------------------------------------------------------------
void ClientModeTFNormal::Layout()
{
	BaseClass::Layout();

	vgui::VPANEL pRoot;
	int wide, tall;

	// Make the viewport fill the root panel.
	if(pRoot = VGui_GetClientDLLRootPanel() )
	{
		vgui::ipanel()->GetSize(pRoot, wide, tall);
		m_Viewport.SetBounds(0, 0, wide, tall);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *angles - 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::OverrideView( CViewSetup *pSetup )
{
	// Let the player override the view.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return;

	pPlayer->OverrideView( pSetup );
	BaseClass::OverrideView( pSetup );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frametime - 
//			*cmd - 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *cmd )
{
	// Let the player override the view.
	C_BaseTFPlayer *pPlayer = C_BaseTFPlayer::GetLocalPlayer();
	if(!pPlayer)
		return;

	// Let the player at it
	pPlayer->CreateMove( flFrameTime, flInputSampleTime, cmd );

	// Handle knockdowns
	if ( pPlayer->CheckKnockdownAngleOverride() )
	{
		QAngle ang;
		pPlayer->GetKnockdownAngles( ang );

		cmd->viewangles = ang;
		engine->SetViewAngles( ang );

		cmd->forwardmove = cmd->sidemove = cmd->upmove = 0;
		// Only keep score if it's down
		cmd->buttons &= ( IN_SCORE );
	}

	if ( pPlayer->GetPlayerClass() )
	{
		C_PlayerClass *pPlayerClass = pPlayer->GetPlayerClass();
		pPlayerClass->CreateMove( flFrameTime, flInputSampleTime, cmd );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *newmap - 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::LevelInit( const char *newmap )
{
	BaseClass::LevelInit( newmap );

	m_Viewport.GetAnimationController()->StartAnimationSequence("LevelInit");
	m_Viewport.Enable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::LevelShutdown( void )
{
	m_Viewport.Disable();

	BaseClass::LevelShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ClientModeTFNormal::ShouldDrawViewModel( void )
{
	C_BaseTFPlayer *pPlayer = C_BaseTFPlayer::GetLocalPlayer();
	if(!pPlayer)
		return false;

	return pPlayer->ShouldDrawViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ClientModeTFNormal::ShouldDrawCrosshair( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : x - 
//			y - 
//			width - 
//			height - 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::AdjustEngineViewport( int& x, int& y, int& width, int& height )
{
	// Nothing
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::PreRender( CViewSetup *pSetup )
{
	BaseClass::PreRender(pSetup);
}


void ClientModeTFNormal::PostRenderWorld()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::PostRender( void )
{
	// Nothing
	BaseClass::PostRender();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
vgui::AnimationController *ClientModeTFNormal::GetViewportAnimationController()
{
	return m_Viewport.GetAnimationController();
}