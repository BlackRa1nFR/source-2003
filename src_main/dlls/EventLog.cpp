//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
// EventLog.cpp: implementation of the CEventLog class.
//
//////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "EventLog.h"
#include <keyvalues.h>


CEventLog::CEventLog()
{

}

CEventLog::~CEventLog()
{

}


void CEventLog::FireGameEvent( KeyValues * event )
{
	PrintEvent ( event );
}

void CEventLog::GameEventsUpdated()
{
	gameeventmanager->AddListener( this );
}

bool CEventLog::PrintEvent( KeyValues * event )
{
	const char * name = event->GetName();

	if ( Q_strcmp(name, "server_") == 0 )
	{
		return true; // we don't care about server events (engine does)
	}
	else if ( Q_strcmp(name, "player_") == 0 )
	{
		return PrintPlayerEvent( event );
	}
	else if ( Q_strcmp(name, "team_") == 0 )
	{
		return PrintTeamEvent( event );
	}
	else if ( Q_strcmp(name, "game_") == 0 )
	{
		return PrintGameEvent( event );
	}

	return false; // didn't know how to log these event
}

bool CEventLog::PrintGameEvent( KeyValues * event )
{
//	const char * name = event->GetName() + Q_strlen("game_"); // remove prefix

	return false;
}

bool CEventLog::PrintPlayerEvent( KeyValues * event )
{
//	const char * name = event->GetName() + Q_strlen("player_"); // remove prefix

	return false;
}

bool CEventLog::PrintTeamEvent( KeyValues * event )
{
//	const char * name = event->GetName() + Q_strlen("team_"); // remove prefix

	return false;
}


bool CEventLog::Init()
{
	return gameeventmanager->AddListener( this );
}

void CEventLog::Shutdown()
{
	gameeventmanager->RemoveListener( this );
}

