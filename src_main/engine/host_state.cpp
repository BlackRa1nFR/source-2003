//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Runs the state machine for the host & server
//
// $NoKeywords: $
//=============================================================================

#include "host_state.h"
#include "eiface.h"
#include "quakedef.h"
#include "server.h"
#include "sv_main.h"
#include "host_cmd.h"
#include "host.h"
#include "screen.h"
#include "game_interface.h"

typedef enum 
{
	HS_NEW_GAME = 0,
	HS_LOAD_GAME,
	HS_CHANGE_LEVEL_SP,
	HS_CHANGE_LEVEL_MP,
	HS_RUN,
	HS_GAME_SHUTDOWN,
	HS_SHUTDOWN,
	HS_RESTART,
} HOSTSTATES;

// a little class that manages the state machine for the host
class CHostState
{
public:
			CHostState();
	void	Init();
	int		FrameUpdate( float time, int iState );
	void	SetNextState( HOSTSTATES nextState );

	void	RunGameInit();

	void	SetState( HOSTSTATES newState, bool clearNext );
	void	GameShutdown();

	void	State_NewGame();
	void	State_LoadGame();
	void	State_ChangeLevelMP();
	void	State_ChangeLevelSP();
	void	State_Run( float time, int iState );
	void	State_GameShutdown();
	void	State_Shutdown();
	void	State_Restart();

	bool	IsGameShuttingDown();


	HOSTSTATES		m_currentState;
	HOSTSTATES		m_nextState;
	char	m_levelName[256];
	char	m_landmarkName[256];
	bool	m_activeGame;
	char	m_saveName[256];
};

static bool Host_ValidGame( void );
static CHostState	g_HostState;


//-----------------------------------------------------------------------------
// external API for manipulating the host state machine
//-----------------------------------------------------------------------------
void HostState_Init()
{
	g_HostState.Init();
}

int HostState_Frame( float time, int iState )
{
	return g_HostState.FrameUpdate( time, iState );
}

void HostState_RunGameInit()
{
	g_HostState.RunGameInit();
}

// start a new game as soon as possible
void HostState_NewGame( char const *pMapName )
{
	strcpy( g_HostState.m_levelName, pMapName );
	g_HostState.m_landmarkName[0] = 0;
	g_HostState.SetNextState( HS_NEW_GAME );
}

// load a new game as soon as possible
void HostState_LoadGame( char const *pSaveFileName )
{
	strcpy( g_HostState.m_saveName, pSaveFileName );
	g_HostState.SetNextState( HS_LOAD_GAME );
}

// change level (single player style - smooth transition)
void HostState_ChangeLevelSP( char const *pNewLevel, char const *pLandmarkName )
{
	strcpy( g_HostState.m_levelName, pNewLevel );
	strcpy( g_HostState.m_landmarkName, pLandmarkName );
	g_HostState.SetNextState( HS_CHANGE_LEVEL_SP );
}

// change level (multiplayer style - respawn all connected clients)
void HostState_ChangeLevelMP( char const *pNewLevel, char const *pLandmarkName )
{
	strcpy( g_HostState.m_levelName, pNewLevel );
	strcpy( g_HostState.m_landmarkName, pLandmarkName );
	g_HostState.SetNextState( HS_CHANGE_LEVEL_MP );
}

// shutdown the game as soon as possible
void HostState_GameShutdown()
{
	// This will get called during shutdown, ignore it.
	if ( g_HostState.m_currentState != HS_SHUTDOWN &&
		 g_HostState.m_currentState != HS_RESTART )
	{
		g_HostState.SetNextState( HS_GAME_SHUTDOWN );
	}
}

// shutdown the engine/program as soon as possible
void HostState_Shutdown()
{
	g_HostState.SetNextState( HS_SHUTDOWN );
}

//-----------------------------------------------------------------------------
// Purpose: Restart the engine
//-----------------------------------------------------------------------------
void HostState_Restart()
{
	g_HostState.SetNextState( HS_RESTART );
}

bool HostState_IsGameShuttingDown()
{
	return g_HostState.IsGameShuttingDown();
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Class implementation
//-----------------------------------------------------------------------------

CHostState::CHostState()
{
}

void CHostState::Init()
{
	SetState( HS_RUN, true );
	m_currentState = HS_RUN;
	m_nextState = HS_RUN;
	m_activeGame = false;
	m_levelName[0] = 0;
	m_saveName[0] = 0;
	m_landmarkName[0] = 0;
}

void CHostState::SetState( HOSTSTATES newState, bool clearNext )
{
	m_currentState = newState;
	if ( clearNext )
	{
		m_nextState = newState;
	}
}

void CHostState::SetNextState( HOSTSTATES next )
{
	Assert( m_currentState == HS_RUN );
	m_nextState = next;
}

void CHostState::RunGameInit()
{
	Assert( !m_activeGame );

	if ( serverGameDLL )
	{
		serverGameDLL->GameInit();
	}
	m_activeGame = true;
}

void CHostState::GameShutdown()
{
	if ( m_activeGame )
	{
		serverGameDLL->GameShutdown();
		m_activeGame = false;
	}
}


// These State_ functions execute that state's code right away
// The external API queues up state changes to happen when the state machine is processed.
void CHostState::State_NewGame()
{
	if ( Host_ValidGame() )
	{
		if ( !serverGameClients )
		{
			Warning( "Can't start game, no valid server.dll loaded\n" );
		}
		else
		{
			if ( modelloader->Map_IsValid( m_levelName ) )
			{
				if ( Host_NewGame( m_levelName, false ) )
				{
					// succesfully started the new game
					SetState( HS_RUN, true );
					return;
				}
			}
		}
	}

	// new game failed
	GameShutdown();
	// run the server at the console
	SetState( HS_RUN, true );
}

void CHostState::State_LoadGame()
{
#ifndef SWDS
	HostState_RunGameInit();
	if ( saverestore->LoadGame( m_saveName ) )
	{
		// succesfully started the new game
		SetState( HS_RUN, true );
		return;
	}
#endif
	// load game failed
	GameShutdown();
	// run the server at the console
	SetState( HS_RUN, true );
}


void CHostState::State_ChangeLevelMP()
{
	if ( Host_ValidGame() )
	{
		serverGameDLL->LevelShutdown();
		if ( modelloader->Map_IsValid( m_levelName ) )
		{
			Host_Changelevel( false, m_levelName, m_landmarkName );
			SetState( HS_RUN, true );
			return;
		}
	}
	// fail
	Con_Printf( "Unable to change level!\n" );
	SetState( HS_RUN, true );
}


void CHostState::State_ChangeLevelSP()
{
	if ( Host_ValidGame() )
	{
		if ( modelloader->Map_IsValid( m_levelName ) )
		{
			Host_Changelevel( true, m_levelName, m_landmarkName );
			SetState( HS_RUN, true );
			return;
		}
	}
	// fail
	Con_Printf( "Unable to change level!\n" );
	SetState( HS_RUN, true );
}

void CHostState::State_Run( float time, int iState )
{
	Host_RunFrame( time, iState );

	switch( m_nextState )
	{
	case HS_RUN:
		break;

	case HS_LOAD_GAME:
	case HS_NEW_GAME:
#ifndef SWDS
		SCR_BeginLoadingPlaque ();
		// FALL THROUGH INTENTIONALLY
#endif
	case HS_SHUTDOWN:
	case HS_RESTART:
		// NOTE: The game must be shutdown before a new game can start, 
		// before a game can load, and before the system can be shutdown.
		// This is done here instead of pathfinding through a state transition graph.
		// That would be equivalent as the only way to get from HS_RUN to HS_LOAD_GAME is through HS_GAME_SHUTDOWN.
	case HS_GAME_SHUTDOWN:
		SetState( HS_GAME_SHUTDOWN, false );
		break;

	case HS_CHANGE_LEVEL_MP:
	case HS_CHANGE_LEVEL_SP:
		SetState( m_nextState, true );
		break;

	default:
		SetState( HS_RUN, true );
		break;
	}
}

void CHostState::State_GameShutdown()
{
	if ( serverGameDLL )
	{
		if ( sv.active )
		{
			int i;
			client_t *cl;
			for ( i=0, cl = svs.clients ; i < svs.maxclients ; i++, cl++ )
			{
				// now tell dll about disconnect
				if ( ( !cl->active && !cl->spawned && !cl->connected ) || !cl->edict || cl->edict->free )
					continue;

				// call the prog function for removing a client
				SV_DropClient( cl, false, "Server shutting down\n" );
			}
		}

		serverGameDLL->LevelShutdown();
	}

	GameShutdown();
#ifndef SWDS
	saverestore->ClearSaveDir();
#endif
	Host_ShutdownServer();

	SV_InactivateClients();

	switch( m_nextState )
	{
	case HS_LOAD_GAME:
	case HS_NEW_GAME:
	case HS_SHUTDOWN:
	case HS_RESTART:
		SetState( m_nextState, true );
		break;
	default:
		SetState( HS_RUN, true );
		break;
	}
}


// Tell the launcher we're done.
void CHostState::State_Shutdown()
{
#ifndef SWDS
	extern void CL_EndMovie_f( void );
	extern char cl_moviename[];

	if ( cl_moviename[0] )
	{
		CL_EndMovie_f();
	}
#endif
	giActive = DLL_CLOSE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHostState::State_Restart( void )
{
	// Just like a regular shutdown
	State_Shutdown();
	// But signal launcher/front end to restart engine
	giActive = DLL_RESTART;
}


// this is the state machine's main processing loop
int CHostState::FrameUpdate( float time, int iState )
{
#if _DEBUG
	int loopCount = 0;
#endif

	while ( true )
	{
		int oldState = m_currentState;

		// execute the current state (and transition to the next state if not in HS_RUN)
		switch( m_currentState )
		{
		case HS_NEW_GAME:
			State_NewGame();
			break;
		case HS_LOAD_GAME:
			State_LoadGame();
			break;
		case HS_CHANGE_LEVEL_MP:
			State_ChangeLevelMP();
			break;
		case HS_CHANGE_LEVEL_SP:
			State_ChangeLevelSP();
			break;
		case HS_RUN:
			State_Run( time, iState );
			break;
		case HS_GAME_SHUTDOWN:
			State_GameShutdown();
			break;
		case HS_SHUTDOWN:
			State_Shutdown();
			break;
		case HS_RESTART:
			State_Restart();
			break;
		}

		// only do a single pass at HS_RUN per frame.  All other states loop until they reach HS_RUN 
		if ( oldState == HS_RUN )
			break;

		// shutting down
		if ( oldState == HS_SHUTDOWN ||
			 oldState == HS_RESTART )
			break;

		// Only HS_RUN is allowed to persist across loops!!!
		// Also, detect circular state graphs (more than 8 cycling changes is probably a loop)
		// NOTE: In the current graph, there are at most 2.
#if _DEBUG
		if ( (m_currentState == oldState) || (++loopCount > 8) )
		{
			Host_Error( "state crash!\n" );
		}
#endif
	}

	return giActive;
}


bool CHostState::IsGameShuttingDown( void )
{
	return ( ( m_currentState == HS_GAME_SHUTDOWN ) ||
			 ( m_nextState == HS_GAME_SHUTDOWN ) );
}

// Determine if this is a valid game
static bool Host_ValidGame( void )
{
	// No multi-client single player games
	if ( svs.maxclients > 1 )
	{
		if ( deathmatch.GetInt() )
			return true;
	}
	else
	{
		/*
		// Single player must have CD validation
		if ( IsValidCD() )
		{
			return true;
		}
		*/
		return true;
	}

	Con_DPrintf("Unable to launch game\n");
	return false;
}

