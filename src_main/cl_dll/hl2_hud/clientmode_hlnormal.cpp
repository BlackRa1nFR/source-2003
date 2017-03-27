//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Draws the normal TF2 or HL2 HUD.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "clientmode_hlnormal.h"
#include "vgui_int.h"
#include "hud.h"
#include "vgui_scorepanel.h"
#include "vgui_vprofpanel.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui_controls/AnimationController.h>
#include "iinput.h"

extern ConVar hud_autoreloadscript;
extern ConVar cl_drawhud;

// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
	static ClientModeHLNormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public vgui::EditablePanel
{
	typedef vgui::EditablePanel BaseClass;
public:
	CHudViewport(vgui::Panel *parent, const char *name);

	virtual void Enable();
	vgui::AnimationController *GetAnimationController() { return m_pAnimController; }

protected:
	virtual void OnThink();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		gHUD.InitColors( pScheme );

		SetPaintBackgroundEnabled( false );
	}

private:
	typedef vgui::EditablePanel BaseClass;
	vgui::HCursor m_hCursorNone;
	vgui::AnimationController *m_pAnimController;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudViewport::CHudViewport(vgui::Panel *parent, const char *name) : BaseClass( parent, "ClientModeHLNormal" )
{
	SetCursor( m_hCursorNone );

	// use a custom scheme for the hud
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFile("resource/ClientScheme.res", "ViewPort");
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

//-----------------------------------------------------------------------------
// Purpose: Called every frame the viewport is visible
//-----------------------------------------------------------------------------
void CHudViewport::OnThink()
{
	m_pAnimController->UpdateAnimations( gpGlobals->curtime);

	// check the auto-reload cvar
	m_pAnimController->SetAutoReloadScript(hud_autoreloadscript.GetBool());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudViewport::Enable()
{
}

//-----------------------------------------------------------------------------
// ClientModeHLNormal implementation
//-----------------------------------------------------------------------------
ClientModeHLNormal::ClientModeHLNormal() : m_hCursorNone(vgui::dc_none)
{
	m_pViewport = new CHudViewport(NULL, "ClientModeHLNormal");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeHLNormal::~ClientModeHLNormal()
{
	// NOTE: map must be deleted first
	delete m_pViewport;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeHLNormal::Init( void )
{
	BaseClass::Init();
	m_pViewport->LoadControlSettings("scripts/HudLayout.res");
}

//-----------------------------------------------------------------------------
// Purpose: Data accessor
//-----------------------------------------------------------------------------
vgui::Panel *ClientModeHLNormal::GetViewport()
{ 
	return m_pViewport; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
vgui::AnimationController *ClientModeHLNormal::GetViewportAnimationController()
{
	return m_pViewport->GetAnimationController();
}

void ClientModeHLNormal::Enable()
{
	vgui::VPANEL pRoot;

	// Add our viewport to the root panel.
	if(pRoot = VGui_GetClientDLLRootPanel())
	{
		m_pViewport->SetParent( pRoot );
	}

	m_pViewport->SetCursor( m_hCursorNone );
	vgui::surface()->SetCursor( m_hCursorNone );
	m_pViewport->SetVisible( true );

	Layout();
}


void ClientModeHLNormal::Disable()
{
	vgui::VPANEL pRoot;

	// Remove our viewport from the root panel.
	if ( pRoot = VGui_GetClientDLLRootPanel() )
	{
		m_pViewport->SetParent( (vgui::VPANEL)NULL );
	}

	m_pViewport->SetVisible( false );
}

void ClientModeHLNormal::Layout()
{
	vgui::VPANEL pRoot;
	int wide, tall;


	// Make the viewport fill the root panel.
	if (pRoot = VGui_GetClientDLLRootPanel())
	{
		vgui::ipanel()->GetSize(pRoot, wide, tall);
		m_pViewport->SetBounds(0, 0, wide, tall);
	}
}

void ClientModeHLNormal::CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *cmd )
{
	// Let the player override the view.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return;

	// Let the player at it
	pPlayer->CreateMove( flFrameTime, flInputSampleTime, cmd );

}


void ClientModeHLNormal::LevelInit( const char *newmap )
{
	BaseClass::LevelInit( newmap );

	m_pViewport->GetAnimationController()->StartAnimationSequence("LevelInit");
	m_pViewport->Enable();
}

void ClientModeHLNormal::LevelShutdown( void )
{
	BaseClass::LevelShutdown();
}

bool ClientModeHLNormal::ShouldDrawCrosshair( void )
{
	return true;
}

void ClientModeHLNormal::OverrideView( CViewSetup *pSetup )
{
	// Let the player override the view.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return;

	pPlayer->OverrideView( pSetup );
	BaseClass::OverrideView( pSetup );
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame, updates hud visibility
//-----------------------------------------------------------------------------
void ClientModeHLNormal::Update()
{
	// only draw the hud if the cvar is not set
	m_pViewport->SetVisible( cl_drawhud.GetBool() );
	BaseClass::Update();
}
