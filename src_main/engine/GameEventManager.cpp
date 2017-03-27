// GameEventManager.cpp: implementation of the CGameEventManager class.
//
//////////////////////////////////////////////////////////////////////

#include "gameeventmanager.h"
#include "filesystem_engine.h"
#include "checksum_engine.h"
#include "server.h"
#include <keyvalues.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static CGameEventManager s_GameEventManager;
CGameEventManager * g_pGameEventManager = &s_GameEventManager;

static const int MAX_DATA_TYPES = 6;

static char * s_GameEnventTypeMap[MAX_DATA_TYPES] = { "string", "float", "long", "short", "byte", "bool" };

// Expose CVEngineServer to the engine.

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameEventManager, IGameEventManager, INTERFACEVERSION_GAMEEVENTSMANAGER, s_GameEventManager );

CGameEventManager::CGameEventManager()
{
	Reset();
}

CGameEventManager::~CGameEventManager()
{
	
}

bool CGameEventManager::Init()
{
	Reset();
	return true;
}

void CGameEventManager::Shutdown()
{
	Reset();
}

void CGameEventManager::Reset()
{
	int number = m_GameEvents.Count();

	for (int i = 0; i<number; i++)
	{
		GameEvent_t * e = m_GameEvents.Element( i );

		if ( e->keys )
			e->keys->deleteThis(); // free the value keys
					
		e->listeners.RemoveAll();	// remove listeners

		delete e;	// free event memory
	}

	m_GameEvents.RemoveAll();

	m_FileCRC = 0;

	Assert( m_GameEvents.Count() == 0 );
}

bool CGameEventManager::FireEvent( KeyValues * event, IRecipientFilter * filter)
{
	if ( event == NULL || m_FileCRC == 0 )
		return false;

	GameEvent_t * eventtype = GetEventType( event->GetName() );

	if ( eventtype == NULL )
	{
		DevMsg(1, "FireEvent: event '%s' not registerd.\n", event->GetName() );
		event->deleteThis();
		return false;
	}

	// add time & eventid to event

	event->SetInt("time", sv.gettime()*1000 );	// use server time in ms
	event->SetInt("eventid", eventtype->eventid );

	DevMsg(1, "Firing local event '%s' (%i).\n", eventtype->name, eventtype->eventid );

	int number = eventtype->listeners.Count();

	for (int i = 0; i < number; i++ )
	{
		IGameEventListener * listener = eventtype->listeners.Element( i );
		
		Assert ( listener );

		if ( listener->IsLocalListener() )
		{
			listener->FireGameEvent( event );
		}
	}

	// ok, all registerd listeners got the message, now send it out to clients

	SendGameEventToClients( event, eventtype, filter );

	event->deleteThis();

	return false;
}

bool CGameEventManager::FireRemoteEvent( KeyValues * event )
{
	if ( event == NULL )
		return false;

	GameEvent_t * et = GetEventType( event->GetName() );

	if ( et == NULL )
	{
		DevMsg(1, "FireEvent: event '%s' not registerd.\n", event->GetName() );
		event->deleteThis();
		return false;
	}

	// add current time to event

	event->SetInt("eventid", et->eventid );
	event->SetInt("time", cl.gettime()*1000 );	// use client time in ms

	DevMsg(1, "Firing remote event '%s'.\n", event->GetName() );

	int number = et->listeners.Count();

	for (int i = 0; i < number; i++ )
	{
		IGameEventListener * listener = et->listeners.Element( i );
		
		Assert ( listener );

		if ( !listener->IsLocalListener() ) // for only NOT locals
		{
			listener->FireGameEvent( event );
		}
	}

	return true;
}

void CGameEventManager::SendGameEventToClients(KeyValues * event, GameEvent_t * eventtype, IRecipientFilter * filter)
{
	if ( filter == NULL || filter->GetRecipientCount() == 0 )
	{
		return;	// no filter or empty client filter
	}

	if ( eventtype->keys == NULL )
	{
		DevMsg(1, "SendGameEvent: data description for event key '%s'.\n", event->GetName() );
		return;	// ups, we have no data definition for this event
	}

	char	 buffer_data[256];
	bf_write buffer;	// binary buffer we write the keyvalues to

	buffer.StartWriting(buffer_data, sizeof(buffer_data));
	buffer.SetDebugName( "gameeventkey" );
		
	SerializeKeyValues( event, &buffer, eventtype );	// create bytestream from KeyValues

	if ( buffer.IsOverflowed() )
	{
		Msg( "CGameEventManager: event key '%s' serialization overflowed\n", eventtype->name );
		return;
	}

	// following code is much copy & past of CVEngineServer::MessageEnd

	if ( filter->IsInitMessage() || filter->IsBroadcastMessage() )	// put in servers sigon on buffer, so everybody gets it during connect
	{
		bf_write * pBuffer = &sv.datagram;  // assume unreliable message

		if ( filter->IsInitMessage() )
		{
			pBuffer = &sv.signon;
		}
		else if ( filter->IsReliable() )
		{
			pBuffer = &sv.reliable_datagram;
		}
				
		if ( buffer.GetNumBitsWritten() > pBuffer->GetNumBitsLeft() )
		{
			if ( filter->IsInitMessage() )
			{
				Sys_Error( "CGameEventManager: Init event would overflow signon buffer!\n" );
			}
			else if ( filter->IsReliable() )
			{
				Msg( "CGameEventManager: Reliable broadcast event would overflow server reliable datagram! Ignoring event\n" );
			}
			return;
		}

		pBuffer->WriteBytes( buffer.m_pData, buffer.GetNumBytesWritten() );
	}
	else
	{
		// Now iterate the recipients and write the data into the appropriate buffer if possible
		int c = filter->GetRecipientCount();

		for ( int i = 0; i < c; i++ )
		{
			int clientindex = filter->GetRecipientIndex( i );
			if ( clientindex < 1 || clientindex > svs.maxclients )
			{
				Msg( "CGameEventManager: bogus client index (%i) in list of %i clients\n", clientindex,	c );
				continue;
			}

			client_t *cl = &svs.clients[ clientindex - 1 ];
			// Never output to bots
			if ( cl->fakeclient )
				continue;

			if ( !cl->active && !cl->spawned )
				continue;

			bf_write *pBuffer = NULL;

			if ( filter->IsReliable() )
			{
				pBuffer = &cl->netchan.message;
			}
			else
			{
				pBuffer = &cl->datagram;
			}

			if ( !pBuffer->m_pData )
			{
				Con_DPrintf("Game event sent to uninitialized buffer\n");
				continue;
			}

			if ( buffer.GetNumBitsWritten() > pBuffer->GetNumBitsLeft() )
			{
				if ( filter->IsReliable() )
				{
					SV_DropClient( cl, false, "CGameEventManager: %s for reliable event with no room in reliable buffer\n",
						cl->name );
				}
				continue;
			}
			
			pBuffer->WriteBytes( buffer.m_pData, buffer.GetNumBytesWritten() );
		}
	}
}

bool CGameEventManager::ParseGameEvent(bf_read * msg)
{
	// read KeyValues name number
	// unseri

	KeyValues * event = UnserializeKeyValue( msg );

	if ( !event )
	{
		DevMsg(1, "CGameEventManager:: UnserializeKeyValue failed.\n" );
		return false;
	}

	FireRemoteEvent( event );
	
	return true;
}

bool CGameEventManager::SerializeKeyValues( KeyValues* event, bf_write* buf, GameEvent_t* eventtype )
{
	if ( eventtype == NULL )
	{
		eventtype = GetEventType( event->GetName() );

		if ( eventtype == NULL )
		{
			DevMsg(1, "CGameEventManager:: SerializeKeyValue failed, unkown event '%s'.\n", event->GetName() );
			return false;
		}
	}

	buf->WriteByte( svc_gameevent );	// hey, here comes an game event
	buf->WriteByte( eventtype->eventid ); // and it's me

	// now iterate trough all fields descripted in gameevents.res and put them in the buffer

	KeyValues * key = eventtype->keys->GetFirstSubKey(); 
	
	while ( key )
	{
		const char * keyName = key->GetName();

		if ( Q_strcmp(keyName,"eventid") == 0 ||
			 Q_strcmp(keyName,"priority") == 0 ||
			 Q_strcmp(keyName,"time") == 0 )
		{
			key = key->GetNextKey();	// ignore standard keys
			continue;
		}

		int type = key->GetInt();

		switch ( type )
		{
			case 0 : buf->WriteString( event->GetString( keyName, "") ); break;
			case 1 : buf->WriteFloat( event->GetFloat( keyName, 0.0f) ); break;
			case 2 : buf->WriteLong( event->GetInt( keyName, 0) ); break;
			case 3 : buf->WriteShort( event->GetInt( keyName, 0) ); break;
			case 4 : buf->WriteByte( event->GetInt( keyName, 0) ); break;
			case 5 : buf->WriteByte( event->GetInt( keyName, 0) ); break;
			default: DevMsg(1, "CGameEventManager:: unkown type %i in key '%s'.\n", type, key->GetName() ); break;
		}

		key = key->GetNextKey();
	}

	return true;
}

KeyValues * CGameEventManager::UnserializeKeyValue(bf_read * buf)
{
	static char stringbuf[1024];

	// read event id

	int eventid = buf->ReadByte();

	GameEvent_t * eventtype = GetEventType( eventid );

	if ( eventtype == NULL )
	{
		DevMsg(1, "CGameEventManager:: NULL event type for index %i.\n", eventid );
		return NULL;
	}

	KeyValues * event = new KeyValues( eventtype->name );

	KeyValues * key = eventtype->keys->GetFirstSubKey(); 

	while ( key )
	{
		const char * keyName = key->GetName();

		if ( Q_strcmp(keyName,"eventid") == 0 ||
			 Q_strcmp(keyName,"priority") == 0 ||
			 Q_strcmp(keyName,"time") == 0 )
		{
			key = key->GetNextKey();	// ignore standard keys
			continue;
		}

		int type = key->GetInt();

		switch ( type )
		{
			case 0 : if ( buf->ReadString( stringbuf, 1024) )
						event->SetString( keyName, stringbuf );
					 break;
			case 1 : event->SetFloat( keyName, buf->ReadFloat() ); break;
			case 2 : event->SetInt( keyName, buf->ReadLong() ); break;
			case 3 : event->SetInt( keyName, buf->ReadShort() ); break;
			case 4 : event->SetInt( keyName, buf->ReadByte() ); break;
			case 5 : event->SetInt( keyName, buf->ReadByte() ); break;
			default: DevMsg(1, "CGameEventManager:: unkown type %i in key '%s'.\n", type, key->GetName() ); break;

		}

		key = key->GetNextKey();
	}

	return event;
}

void CGameEventManager::RemoveListener(IGameEventListener * listener)
{
	for (int i=0; i < m_GameEvents.Count(); i++ )
	{
		GameEvent_t * et = m_GameEvents.Element( i );

		if ( et )
		{
			et->listeners.FindAndRemove( listener );
		}
	}

	m_Listerners.FindAndRemove( listener );
}



int CGameEventManager::LoadEventsFromFile( const char * filename )
{
	CRC32_t newCRC;

	int count = 0;

	CRC_File( &newCRC, const_cast<char*>(filename) );	// UNDO const cast

	// if we are loading the same file again (same CRC), skip loading
	// assuming we can keep current events
	if ( newCRC == m_FileCRC )
	{
		DevMsg(1, "Event System skipped loading file %s, same CRC (%u).\n", filename, m_FileCRC );
		return m_GameEvents.Count();
	}
	
	KeyValues * key = new KeyValues(filename);

	if  ( !key->LoadFromFile( g_pFileSystem, filename, "GAME" ) )
		return false;

	Reset();

	m_FileCRC = newCRC;

	KeyValues * subkey = key->GetFirstSubKey();

	while ( subkey )
	{
		if ( subkey->GetDataType() == KeyValues::TYPE_NONE )
		{
			RegisterEvent( subkey );
			count++;
		}

		subkey = subkey->GetNextKey();
	}

	DevMsg(1, "Event System loaded %i events from file %s (CRC %u).\n", m_GameEvents.Count(), filename, m_FileCRC );

	key->deleteThis();

	UpdateListeners();

	return m_GameEvents.Count();
}

void CGameEventManager::UpdateListeners()
{
	for ( int i=0; i < m_Listerners.Count(); i++ )
	{
		m_Listerners.Element(i)->GameEventsUpdated();
	}
}


unsigned long CGameEventManager::GetEventTableCRC()
{
	return m_FileCRC;
}

bool CGameEventManager::AddListener( IGameEventListener * listener, const char * event)
{
	if ( !listener || !event )
		return false;	// bahh

	GameEvent_t * et = GetEventType( event );

	if ( et == NULL )
	{
		DevMsg(1, "CGameEventManager::AddListener: no event type for event '%s' %i.\n", event );
		return false;	// that should not happen
	}

	if ( et->listeners.Find( listener ) < 0 )
	{
		et->listeners.AddToTail( listener );
	}

	if ( m_Listerners.Find( listener ) < 0 )
	{
		m_Listerners.AddToTail( listener );
	}

	return true;
}

bool CGameEventManager::AddListener( IGameEventListener * listener)
{
	for (int i=0; i < m_GameEvents.Count(); i++ )
	{
		GameEvent_t * et = m_GameEvents.Element( i );

		if ( et )
		{
			if ( et->listeners.Find( listener ) < 0 )
			{
				et->listeners.AddToTail( listener );
			}
		}
	}

	if ( m_Listerners.Find( listener )  < 0 )
	{
		m_Listerners.AddToTail( listener );
	}

	return true;
}

bool CGameEventManager::RegisterEvent( KeyValues * event)
{
	if ( event == NULL )
		return false;

	GameEvent_t * et = GetEventType( event->GetName() );

	if ( et != NULL )
	{
		return true;	// event already known
	}

	if ( m_GameEvents.Count() == MAX_EVENT_NUMBER )
	{
		DevMsg(1, "CGameEventManager: couldn't register event '%s', limit reached (%i).\n",
			event->GetName(), MAX_EVENT_NUMBER );
		return false;
	}

	et = new GameEvent_t;

	Q_strncpy( et->name, event->GetName(), MAX_EVENT_NAME_LENGTH-1 );	
	et->name[MAX_EVENT_NAME_LENGTH-1]=0;

	et->eventid = event->GetInt("eventid");
	
	et->keys =event->MakeCopy(); // create local copy

	KeyValues * subkey = et->keys->GetFirstSubKey();

	// translate types strings to integers

	while ( subkey )
	{
		const char * keyName = subkey->GetName();

		if ( Q_strcmp(keyName,"eventid") == 0 ||
			 Q_strcmp(keyName,"priority") == 0 ||
			 Q_strcmp(keyName,"time") == 0 )
		{
			subkey = subkey->GetNextKey();	// ignore standard keys
			continue;
		}

		const char * type = subkey->GetString();

		int i;
		for (i = 0; i < MAX_DATA_TYPES; i++  )
		{
			if ( Q_strcmp( type, s_GameEnventTypeMap[i]) == 0 )
			{
				et->keys->SetInt( keyName, i );
				break;
			}
		}

		if ( i == MAX_DATA_TYPES )
		{
			et->keys->SetInt( keyName, 0 );	// string type
			DevMsg(1, "CGameEventManager:: unkown type '%s' in key '%s'.\n", type, subkey->GetName() );
		}
		
		subkey = subkey->GetNextKey();
	}
	
	et->listeners.RemoveAll();
	
	m_GameEvents.AddToTail( et );

	return true;
}

int  CGameEventManager::GetEventNumber()
{
	return m_GameEvents.Count();
}

CGameEventManager::GameEvent_t * CGameEventManager::GetEventType(int eventid) // returns event name or NULL
{
	for (int i=0; i < m_GameEvents.Count(); i++ )
	{
		GameEvent_t * e = m_GameEvents.Element( i );

		if ( e->eventid == eventid )
			return e;
	}

	return NULL;
}

CGameEventManager::GameEvent_t * CGameEventManager::GetEventType(const char * name)
{
	for (int i=0; i < m_GameEvents.Count(); i++ )
	{
		GameEvent_t * e = m_GameEvents.Element( i );

		if ( Q_strcmp(e->name, name ) == 0 )
			return e;
	}

	return NULL;
}

KeyValues * CGameEventManager::GetEvent(const char * name)
{
	GameEvent_t * et = GetEventType( name );

	if ( et )
		return et->keys;

	return NULL;
}
