//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: gameeventmanager.h: interface for the CGameEventManager class.
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================

#if !defined ( GAMEEVENTMANAGER_H )
#define GAMEEVENTMANAGER_H

#ifdef _WIN32
#pragma once
#endif 

#include "igameevents.h"
#include "server.h"
#include <utlvector.h>
#include <keyvalues.h>
#include <irecipientfilter.h>

class CGameEventManager  : public IGameEventManager
{
private :

	typedef struct {
		char		name[MAX_EVENT_NAME_LENGTH];	// name of this event
		int			eventid;	// internal index number
		KeyValues * keys;		// KeyValue describing data types, if NULL only name 
		CUtlVector<IGameEventListener*>	listeners;	// registered listeners
	} GameEvent_t;

public:	// IGameEventManager functions

	CGameEventManager();
	virtual ~CGameEventManager();
	
	int  LoadEventsFromFile( const char * filename );
	int  GetEventNumber();
	void Reset();
	unsigned long GetEventTableCRC();
	KeyValues *   GetEvent(const char * name);
	
		
	bool AddListener( IGameEventListener * listener, const char * event);
	bool AddListener( IGameEventListener * listener);
	
	void RemoveListener(IGameEventListener * listener);
		
	bool FireEvent( KeyValues * event, IRecipientFilter * filter );
	bool FireRemoteEvent( KeyValues * event );
	void UnregisterEvent(int index);
	
public:

	bool RegisterEvent( KeyValues * keys );

	void SendGameEventToClients(KeyValues * event, GameEvent_t * eventtype, IRecipientFilter * filter);
	bool ParseGameEvent(bf_read * msg);	// read event (KeyValues) from network stream after svc_gameevent

	bool SerializeKeyValues( KeyValues * event, bf_write *buf, GameEvent_t * eventtype = NULL );
	KeyValues * UnserializeKeyValue(bf_read * msg);

	bool Init();
	void Shutdown();
	void UpdateListeners();

private:

	GameEvent_t * GetEventType(const char * name);
	GameEvent_t * GetEventType(int eventid);

	CUtlVector<GameEvent_t*>		m_GameEvents;	// list of events
	CUtlVector<IGameEventListener*>	m_Listerners;	// list of all known isteners
	CRC32_t							m_FileCRC;	// CRC of current used game events
};

extern CGameEventManager * g_pGameEventManager;

#endif 
