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
#if !defined( IGAMEEVENTS_H )
#define IGAMEEVENTS_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

#define INTERFACEVERSION_GAMEEVENTSMANAGER	"GAMEEVENTSMANAGER001"

//-----------------------------------------------------------------------------
// Purpose: Engine interface into global game event management
//-----------------------------------------------------------------------------

/* 

The GameEventManager keeps track of all global game events. Game events are
fired by game.dll for events like player death or team wins. Each event has a
unique name and comes with a KeyValue structure providing informations about
this event. Some events are generated also by the engine. KeyValues contains
data about the event, some data keys are optional, but should not be used in
other meanings:

	"name" "string"  : name of this event, eg "player_death"
	"time" "integer" : game time of this event in miliseconds since last
                           change level
	"prioity" "byte" : priority of this event 0..15 ( 15 = most important,
                           eg a team win)

Mod developers may add their own events by register them first at the
EventManager and the firing them, when they occur. This way, all registerd
listener have a chance to keep track of the whole game. To ensure, that 3rd
Party Tools  may use your new events, publish all you used events in a public
file. For each new event you must provide the name and all used data keys and
their data types. For example:

    event name 	: "player_death"
    desciptions :  A player died for any reason (does not include
                   disconnecting). For a suicide, KillerIndex == Victimindex and
                   Reason contains 'world'or 'worldspawn'. For killings, Reason
                   contains the weapon used.
    data keys:	:  "VictimIndex" <integer> 
                   "Reason" <string>
                   "KillerIndex" <integer> 
                   "Headshot" <bool> 
   
Common data types are strings("Hello World"), integer("1234"), float("12.34")
and bool("0" or "1"). */

// these data types are allowed:
#define GAME_EVENT_TYPE_STRING	"string"	// zero terminated ASCII string
#define GAME_EVENT_TYPE_FLOAT	"float"		// 32 bit float
#define	GAME_EVENT_TYPE_LONG	"long"		// 32 bit unsigned integer
#define GAME_EVENT_TYPE_SHORT	"short"		// 16 bit unsigned integer
#define	GAME_EVENT_TYPE_BYTE	"byte"		// 8 bit unsigned integer
#define	GAME_EVENT_TYPE_BOOL	"bool"		// 0 or 1

#define MAX_EVENT_NAME_LENGTH	32			// max game event name length
#define MAX_EVENT_NUMBER		256			// max number of events allowed

class IRecipientFilter;
class KeyValues;

class IGameEventListener
{
public:
	virtual	~IGameEventListener( void ) {};
	
	// FireEvent is called by EventManager if event just occured
	// KeyValue memory will be freed by manager if not needed anymore
	virtual void FireGameEvent( KeyValues * event) = 0;

	// the EventManager was updated, please re-register for all wanted events
	virtual void GameEventsUpdated() = 0;

	// an object is a local listener if it's a server side listener
	// eg, true for server side plugins, false for client side listeners
	virtual bool IsLocalListener() = 0;
};

class IGameEventManager : public IBaseInterface
{
public:
	virtual	~IGameEventManager( void ) {};
	
	// load game event descriptions from a file eg "resource\gameevents.res"
	virtual int LoadEventsFromFile( const char * filename ) = 0;

	// returns a CRC of current event table to verifiy that all using the same
	virtual unsigned long GetEventTableCRC() = 0;

	// removes all and anything
	virtual void  Reset() = 0;	
				
	virtual int GetEventNumber() = 0;	// returns total number of registerd events
	virtual KeyValues * GetEvent(const char * name) = 0; // returns keys for event
	
	// adds a listener for a particular event
	virtual bool AddListener( IGameEventListener * listener, const char * event ) = 0;
	// registers for all known events
	virtual bool AddListener( IGameEventListener * listener) = 0; 
	
	// removes a listener 
	virtual void RemoveListener( IGameEventListener * listener) = 0;
		
	// fires an global event, specific event data is stored in KeyValues
	// local listeners will receive the event instantly 
	// a network message will be send to remote listeners specified in 
	// *filter.
	virtual bool FireEvent( KeyValues * event, IRecipientFilter *filter) = 0;

	// fires a remote event only on this client
	// can be used to fake events coming over the network 
	virtual bool FireRemoteEvent( KeyValues * event ) = 0;
};

#endif // IGAMEEVENTS_H