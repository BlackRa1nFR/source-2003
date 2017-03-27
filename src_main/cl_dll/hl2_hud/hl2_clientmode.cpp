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
#include "ivmodemanager.h"
#include "clientmode_hlnormal.h"

// default FOV for HL2
ConVar default_fov( "default_fov", "75", 0 );

// The current client mode. Always ClientModeNormal in HL.
IClientMode *g_pClientMode = NULL;

class CHLModeManager : public IVModeManager
{
public:
				CHLModeManager( void );
	virtual		~CHLModeManager( void );

	virtual void	Init( void );
	virtual void	SwitchMode( bool commander, bool force );
	virtual void	OverrideView( CViewSetup *pSetup );
	virtual void	CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *cmd );
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
};

CHLModeManager::CHLModeManager( void )
{
}

CHLModeManager::~CHLModeManager( void )
{
}

void CHLModeManager::Init( void )
{
	g_pClientMode = GetClientModeNormal();
}

void CHLModeManager::SwitchMode( bool commander, bool force )
{
}

void CHLModeManager::OverrideView( CViewSetup *pSetup )
{
}

void CHLModeManager::CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *cmd )
{
}

void CHLModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );
}

void CHLModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();
}


static CHLModeManager g_HLModeManager;
IVModeManager *modemanager = &g_HLModeManager;

