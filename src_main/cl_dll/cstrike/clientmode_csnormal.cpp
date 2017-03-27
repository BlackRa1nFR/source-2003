//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "clientmode_csnormal.h"
#include "vgui_int.h"
#include "cdll_client_int.h"
#include "iinput.h"
#include "vgui/isurface.h"
#include "vgui/ipanel.h"
#include <vgui_controls/AnimationController.h>
#include "vgui_scorepanel.h"
#include "vgui_vprofpanel.h"
#include "ivmodemanager.h"
#include "hud_macros.h"
#include "BuyMenu.h"
#include "filesystem.h"
#include "vgui/ivgui.h"
#include "keydefs.h"
#include <imapoverview.h>


extern ConVar cl_drawhud;

ConVar default_fov( "default_fov", "90", 0 );

IClientMode *g_pClientMode = NULL;


// --------------------------------------------------------------------------------- //
// CCSModeManager.
// --------------------------------------------------------------------------------- //

class CCSModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CCSModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;

static void __MsgFunc_VGUIMenu( const char *pszName, int iSize, void *pbuf )
{
	const char *pMenuName = (const char*)pbuf;
	if ( Q_stricmp( pMenuName, "buy" ) == 0 )
	{
		GetClientModeCSNormal()->m_pBuyMenu = new CBuyMenu( GetClientModeCSNormal()->GetViewport() );
		GetClientModeCSNormal()->m_pBuyMenu->ActivateBuyMenu();
	}
}


// --------------------------------------------------------------------------------- //
// CCSModeManager implementation.
// --------------------------------------------------------------------------------- //

void CCSModeManager::Init()
{
	g_pClientMode = GetClientModeNormal();
}

void CCSModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );
}

void CCSModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();
}



// --------------------------------------------------------------------------------- //
// ClientModeCSNormal implementation.
// --------------------------------------------------------------------------------- //

void ClientModeCSNormal::Init( void )
{
	BaseClass::Init();

	m_Viewport.LoadControlSettings("scripts/HudLayout.res");
	HOOK_MESSAGE( VGUIMenu );

	//m_pBuyMenu = new CBuyMenu( &m_Viewport );
}

CON_COMMAND( hud_reloadscheme, "Reloads hud layout and animation scripts." )
{
	ClientModeCSNormal *mode = ( ClientModeCSNormal * )GetClientModeNormal();
	if ( !mode )
		return;

	mode->ReloadScheme();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeCSNormal::ClientModeCSNormal() :
	m_CursorNone(vgui::dc_none)
{
	m_Viewport.Start( gameuifuncs, gameeventmanager );
}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModeCSNormal::~ClientModeCSNormal()
{
}

void ClientModeCSNormal::ReloadScheme( void )
{
	m_Viewport.ReloadScheme();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : vgui::Panel
//-----------------------------------------------------------------------------
vgui::Panel *ClientModeCSNormal::GetMinimapParent( void )
{
	return GetViewport();
}

//-----------------------------------------------------------------------------
// Purpose: Enable this viewport
//-----------------------------------------------------------------------------
void ClientModeCSNormal::Enable()
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
void ClientModeCSNormal::Disable()
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
void ClientModeCSNormal::Update()
{
	m_Viewport.SetVisible( cl_drawhud.GetBool() );

	

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if( pPlayer && pPlayer->IsObserver() )
	{
		IMapOverview * overviewmap = m_Viewport.GetMapOverviewInterface();

		Vector worldpos = pPlayer->GetNetworkOrigin();
		Vector2D mappos = overviewmap->WorldToMap( worldpos );
		QAngle angles = pPlayer->GetNetworkAngles();

		overviewmap->SetCenter( mappos );
		overviewmap->SetAngle( angles.y );
	}
		

	BaseClass::Update();
}

//-----------------------------------------------------------------------------
// Purpose: Set viewport bounds, etc.
//-----------------------------------------------------------------------------
void ClientModeCSNormal::Layout()
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
void ClientModeCSNormal::OverrideView( CViewSetup *pSetup )
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
void ClientModeCSNormal::CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *cmd )
{
	// Let the player override the view.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return;

	// Let the player at it
	pPlayer->CreateMove( flFrameTime, flInputSampleTime, cmd );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *newmap - 
//-----------------------------------------------------------------------------
void ClientModeCSNormal::LevelInit( const char *newmap )
{
	BaseClass::LevelInit( newmap );

	m_Viewport.GetAnimationController()->StartAnimationSequence("LevelInit");
	m_Viewport.Enable();
	m_Viewport.OnLevelChange( newmap );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeCSNormal::LevelShutdown( void )
{
	m_Viewport.Disable();

	BaseClass::LevelShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ClientModeCSNormal::ShouldDrawViewModel( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ClientModeCSNormal::ShouldDrawCrosshair( void )
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
void ClientModeCSNormal::AdjustEngineViewport( int& x, int& y, int& width, int& height )
{
	// Nothing
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeCSNormal::PreRender( CViewSetup *pSetup )
{
	BaseClass::PreRender(pSetup);
}


void ClientModeCSNormal::PostRenderWorld()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeCSNormal::PostRender( void )
{
	// Nothing
	BaseClass::PostRender();
}

//-----------------------------------------------------------------------------
// Purpose: We've received a keypress from the engine. Return 1 if the engine is allowed to handle it.
//-----------------------------------------------------------------------------
int	ClientModeCSNormal::KeyInput( int down, int keynum, const char *pszCurrentBinding )
{
	ISpectatorInterface * spectatorUI = m_Viewport.GetSpectatorInterface(); // a shortcut

	if ( spectatorUI->IsVisible() ) 
	{
		// we are in spectator mode, open spectator menu
		if ( down && keynum == K_CTRL  && !spectatorUI->IsGUIVisible() )
		{
			spectatorUI->ShowGUI();
			return 0; // we handled it
		}
		
		if ( down && keynum == K_MOUSE1 )
		{
			engine->ClientCmd( "specnext" );
			return 0;
		}

		if ( down && keynum == K_MOUSE2 )
		{
			engine->ClientCmd( "specprev" );
			return 0;
		}

		if ( down && keynum == K_SPACE )
		{
			engine->ClientCmd( "specmode" );
			return 0;
		}

	}
	

			
	return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
}


vgui::AnimationController* ClientModeCSNormal::GetViewportAnimationController()
{
	return m_Viewport.GetAnimationController();
}


IClientMode *GetClientModeNormal()
{
	static ClientModeCSNormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}


ClientModeCSNormal* GetClientModeCSNormal()
{
	Assert( dynamic_cast< ClientModeCSNormal* >( GetClientModeNormal() ) );

	return static_cast< ClientModeCSNormal* >( GetClientModeNormal() );
}
