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
#include "hud_macros.h"
#include "clientmode_tfnormal.h"
#include "clientmode.h"
#include "clientmode_commander.h"
#include "ivmodemanager.h"
#include "parsemsg.h"
#include "hud_timer.h"
#include "hud_technologytreedoc.h"
#include "CommanderOverlay.h"
#include "c_tf2rootpanel.h"
#include "c_info_act.h"

// default FOV for TF2
ConVar default_fov( "default_fov", "90", 0 );

//-----------------------------------------------------------------------------
// Purpose: Handles switching to/from commander mode
//-----------------------------------------------------------------------------
class CTFModeManager : public IVModeManager
{
public:
	virtual void	Init( void );
	virtual void	SwitchMode( bool commander, bool force );
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );

					CTFModeManager( void );
	virtual			~CTFModeManager( void );

	void			UserCmd_Commander( void );
	void			UserCmd_Normal( void );

};

static CTFModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;

// The current client mode. Always ClientModeNormal in HL.
IClientMode *g_pClientMode = NULL;

DECLARE_COMMAND( g_ModeManager, Commander );
DECLARE_COMMAND( g_ModeManager, Normal );

HOOK_COMMAND( commander, Commander );
HOOK_COMMAND( normal, Normal );

void __MsgFunc_ActBegin(const char *pszName, int iSize, void *pbuf);
void __MsgFunc_ActEnd(const char *pszName, int iSize, void *pbuf);

#define MINIMAP_FILE	"scripts/minimap_overlays.txt"
#define SCREEN_FILE		"scripts/vgui_screens.txt"

//-----------------------------------------------------------------------------
// Purpose: Intialize the mode manager
//-----------------------------------------------------------------------------
void CTFModeManager::Init( void )
{
	g_pClientMode = GetClientModeNormal();

	// These define the panels that can be used by the engine
	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( MINIMAP_FILE );
	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );

	// FIXME: Turn these into client systems
	HudCommanderOverlayMgr()->GameInit();
	MapData().Init();
	GetTechnologyTreeDoc().Init();

	HOOK_MESSAGE( ActBegin );
	HOOK_MESSAGE( ActEnd );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CTFModeManager::CTFModeManager( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CTFModeManager::~CTFModeManager( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFModeManager::UserCmd_Commander( void )
{
	engine->ServerCmd( "tactical 1\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFModeManager::UserCmd_Normal( void )
{
	engine->ServerCmd( "tactical 0\n" );
}

//-----------------------------------------------------------------------------
// Purpose: Switch to / from commander mode ( won't change if current mode is already
//  correct
// Input  : commander - 
//-----------------------------------------------------------------------------
void CTFModeManager::SwitchMode( bool commander, bool force )
{
	if ( commander && ( ( g_pClientMode != ClientModeCommander() ) || force ) )
	{
		g_pClientMode->Disable();
		g_pClientMode = ClientModeCommander();
		g_pClientMode->Enable();
	}
	else if ( !commander && ( ( g_pClientMode != GetClientModeNormal() ) || force ) )
	{
		g_pClientMode->Disable();
		g_pClientMode = GetClientModeNormal();
		g_pClientMode->Enable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *newmap - 
//-----------------------------------------------------------------------------
void CTFModeManager::LevelInit( const char *newmap )
{
	GetTechnologyTreeDoc().LevelInit();
	g_pTF2RootPanel->LevelInit();

	CHudTimer *timer = GET_HUDELEMENT( CHudTimer );
	if ( timer )
	{
		timer->Init();
	}

	// Tell all modes about the map change
	ClientModeCommander()->LevelInit( newmap );
	GetClientModeNormal()->LevelInit( newmap );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFModeManager::LevelShutdown( void )
{
	GetClientModeNormal()->LevelShutdown();
	ClientModeCommander()->LevelShutdown();

	g_pTF2RootPanel->LevelShutdown();
	GetTechnologyTreeDoc().LevelShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: A new act has just begun
//-----------------------------------------------------------------------------
void __MsgFunc_ActBegin(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int iActNumber = (char)READ_BYTE();
	float flStartTime = READ_FLOAT();

	StartAct( iActNumber, flStartTime );
}

//-----------------------------------------------------------------------------
// Purpose: An act has just ended
//-----------------------------------------------------------------------------
void __MsgFunc_ActEnd(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	CHudTimer *timer = GET_HUDELEMENT( CHudTimer );
	if ( timer )
	{
		timer->SetNoFixedTimer( 0.0f );
	}
}
