//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( ISCOREBOARDINTERFACE_H )
#define ISCOREBOARDINTERFACE_H
#ifdef _WIN32
#pragma once
#endif

#include <cl_dll/IVGuiClientDll.h>

//-----------------------------------------------------------------------------
// Purpose: interface for apps to update the scoreboard
//-----------------------------------------------------------------------------
class IScoreBoardInterface
{
public:
	// should be called on level change to reset the scoreboard
	virtual void Reset() = 0;

	virtual void Update( const char *servername, bool teamplay, bool spectator ) = 0;
	virtual void RebuildTeams( const char *servername, bool teamplay, bool spectator ) = 0;
	virtual void DeathMsg( int killer, int victim ) = 0;
	virtual void Activate( bool spectatorUIVisible ) = 0;
	virtual void MoveToFront() = 0;
	virtual bool IsVisible() = 0;
	virtual void SetVisible( bool state ) = 0;
	virtual void SetParent( vgui::VPANEL parent ) = 0;
	virtual void SetMouseInputEnabled( bool state ) = 0;
	virtual void SetTeamName(  int index,  const char *name) = 0;
	virtual void SetTeamDetails( int index, int frags, int deaths) = 0;
	virtual const char *GetTeamName( int playerIndex ) = 0;
	virtual VGuiLibraryTeamInfo_t GetPlayerTeamInfo( int playerIndex ) = 0;
};

#endif // ISCOREBOARDINTERFACE_H

