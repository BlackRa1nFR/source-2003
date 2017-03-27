//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Deals with singleton  
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "igamesystem.h"
#include "utlvector.h"


// Pointer to a member method of IGameSystem
typedef void (IGameSystem::*GameSystemFunc_t)();

// Used to invoke a method of all added Game systems in order
static void InvokeMethod( GameSystemFunc_t f );

// Used to invoke a method of all added Game systems in reverse order
static void InvokeMethodReverseOrder( GameSystemFunc_t f );

// List of all installed Game systems
static CUtlVector<IGameSystem*> s_GameSystems( 0, 4 );

// The map name
static char* s_pMapName = 0;


//-----------------------------------------------------------------------------
// Auto-registration of game systems
//-----------------------------------------------------------------------------
static	CAutoGameSystem *s_pSystemList = NULL;

CAutoGameSystem::CAutoGameSystem()
{
	m_pNext = s_pSystemList;
	s_pSystemList = this;
}


//-----------------------------------------------------------------------------
// destructor, cleans up automagically....
//-----------------------------------------------------------------------------
IGameSystem::~IGameSystem()
{
	Remove( this );
}


//-----------------------------------------------------------------------------
// Adds a system to the list of systems to run
//-----------------------------------------------------------------------------
void IGameSystem::Add( IGameSystem* pSys )
{
	s_GameSystems.AddToTail( pSys );
}


//-----------------------------------------------------------------------------
// Removes a system from the list of systems to update
//-----------------------------------------------------------------------------
void IGameSystem::Remove( IGameSystem* pSys )
{
	s_GameSystems.FindAndRemove( pSys );
}


//-----------------------------------------------------------------------------
// Removes *all* systems from the list of systems to update
//-----------------------------------------------------------------------------
void IGameSystem::RemoveAll(  )
{
	s_GameSystems.RemoveAll();
}


//-----------------------------------------------------------------------------
// Client systems can use this to get at the map name
//-----------------------------------------------------------------------------
char const*	IGameSystem::MapName()
{
	return s_pMapName;
}


//-----------------------------------------------------------------------------
// Invokes methods on all installed game systems
//-----------------------------------------------------------------------------
bool IGameSystem::InitAllSystems()
{
	int i;

	// first add any auto systems to the end
	CAutoGameSystem *pSystem = s_pSystemList;
	while ( pSystem )
	{
		if ( s_GameSystems.Find( pSystem ) == s_GameSystems.InvalidIndex() )
		{
			Add( pSystem );
		}
		else
		{
			DevWarning( 1, "AutoGameSystem already added to game system list!!!\n" );
		}
		pSystem = pSystem->m_pNext;
	}
	s_pSystemList = NULL;

	for ( i = 0; i < s_GameSystems.Count(); ++i )
	{
		if ( !s_GameSystems[i]->Init() )
			return false;
	}

	return true;
}

void IGameSystem::ShutdownAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::Shutdown );
}

void IGameSystem::LevelInitPreEntityAllSystems( char const* pMapName )
{
	// Store off the map name
	if ( s_pMapName )
	{
		delete[] s_pMapName;
	}

	s_pMapName = new char [ strlen(pMapName) + 1 ];
	strcpy( s_pMapName, pMapName );

	InvokeMethod( &IGameSystem::LevelInitPreEntity );
}

void IGameSystem::LevelInitPostEntityAllSystems( void )
{
	InvokeMethod( &IGameSystem::LevelInitPostEntity );
}

void IGameSystem::LevelShutdownPreEntityAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::LevelShutdownPreEntity );
}

void IGameSystem::LevelShutdownPostEntityAllSystems()
{
	InvokeMethodReverseOrder( &IGameSystem::LevelShutdownPostEntity );

	if ( s_pMapName )
	{
		delete[] s_pMapName;
		s_pMapName = 0;
	}
}

void IGameSystem::OnSaveAllSystems()
{
	InvokeMethod( &IGameSystem::OnSave );
}

void IGameSystem::OnRestoreAllSystems()
{
	InvokeMethod( &IGameSystem::OnRestore );
}

#ifdef CLIENT_DLL

void IGameSystem::PreRenderAllSystems()
{
	InvokeMethod( &IGameSystem::PreRender );
}

void IGameSystem::UpdateAllSystems( float frametime )
{
	int i;
	for ( i = 0; i < s_GameSystems.Count(); ++i )
	{
		s_GameSystems[i]->Update( frametime );
	}
}

#else

void IGameSystem::FrameUpdatePreEntityThinkAllSystems()
{
	InvokeMethod( &IGameSystem::FrameUpdatePreEntityThink );
}

void IGameSystem::FrameUpdatePostEntityThinkAllSystems()
{
	InvokeMethod( &IGameSystem::FrameUpdatePostEntityThink );
}

#endif


//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokeMethod( GameSystemFunc_t f )
{
	int i;
	int c = s_GameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystem *sys = s_GameSystems[i];
		(sys->*f)();
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in reverse order
//-----------------------------------------------------------------------------
void InvokeMethodReverseOrder( GameSystemFunc_t f )
{
	int i;
	int c = s_GameSystems.Count();
	for ( i = c; --i >= 0; )
	{
		IGameSystem *sys = s_GameSystems[i];
		(sys->*f)();
	}
}


