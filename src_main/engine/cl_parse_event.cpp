//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "con_nprint.h"
#include "dt_recv_eng.h"
#include "cl_ents_parse.h"
#include "client_class.h"
#include "demo.h"
#include "cdll_engine_int.h"

static ConVar	cl_showevents		( "cl_showevents", "0", 0, "Print event firing info in the console" );

ClientClass* FindClientClass(ClientClass *pHead, const char *pClassName);

//-----------------------------------------------------------------------------
// Purpose: Zero out a "used" event
// Input  : *ei - 
//-----------------------------------------------------------------------------
void CL_ResetEvent( CEventInfo *ei )
{
	ei->index = 0;
	Q_memset( &ei->data, 0, sizeof( ei->data ) );
	ei->fire_time = 0.0;
	ei->flags = 0;
	ei->bits = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Show descriptive info about an event in the numbered console area
// Input  : slot - 
//			*eventname - 
//-----------------------------------------------------------------------------
void CL_DescribeEvent( int slot, CEventInfo *event, const char *eventname )
{
	int idx = (slot & 31);

	if ( !cl_showevents.GetInt() )
		return;

	if ( !eventname )
		return;

	con_nprint_t n;
	n.index = idx;
	n.fixed_width_font = true;
	n.time_to_live = 4.0f;
	n.color[0] = 0.8;
	n.color[1] = 0.8;
	n.color[2] = 1.0;

	Con_NXPrintf( &n, "%02i %6.3ff %20s %03i bytes", slot, cl.gettime(), eventname, (int)( event->bits >> 3 ) );
}

//-----------------------------------------------------------------------------
// Purpose: Decode raw event data into underlying class structure using the specified data table
// Input  : *RawData - 
//			*pToData - 
//			*pRecvTable - 
//-----------------------------------------------------------------------------
void CL_ParseEventDelta( void *RawData, void *pToData, RecvTable *pRecvTable )
{
	// Make sure we have a decoder
	assert(pRecvTable->m_pDecoder);

	// Only so much data allowed
	bf_read fromBuf( "CL_ParseEventDelta->fromBuf", RawData, MAX_EVENT_DATA );

	// First, decode all properties as zeros since temp ents are delta'd from zeros.
	RecvTable_DecodeZeros( pRecvTable, pToData, -1 );

	// Now decode the data from the network on top of that.
	RecvTable_Decode( pRecvTable, pToData, &fromBuf, -1 );

	// Make sure the server, etc. didn't try to send too much
	assert(!fromBuf.IsOverflowed());
}

//-----------------------------------------------------------------------------
// Purpose: Once per frame, walk the client's event slots and look for any events
//  that are ready for playing.
//-----------------------------------------------------------------------------
void CL_FireEvents( void )
{
	int				i;
	CEventState	*es;
	CEventInfo	*ei;
	bool		success;

	es = &cl.events;

	for ( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		ei = &es->ei[ i ];

		if ( ei->index == 0 )
			continue;

		if ( cls.state == ca_disconnected )
		{
			CL_ResetEvent( ei );
			continue;
		}

		// Delayed event!
		if ( ei->fire_time && ( ei->fire_time > cl.gettime() ) )
			continue;

		success = false;

		// Look up the client class, etc.
		//
		ClientClass *pClientClass;
		C_ServerClassInfo *pServerClass;
		RecvTable *pRT;

		// Match the server classes to the client classes.
		pServerClass = cl.m_pServerClasses ? &cl.m_pServerClasses[ ei->index - 1 ] : NULL;
		if ( pServerClass )
		{
			// See if the client .dll has a handler for this class
			pClientClass = FindClientClass( g_ClientDLL->GetAllClasses(), pServerClass->m_ClassName );
			if ( pClientClass )
			{
				// Get the receive table if it exists
				pRT = pClientClass->m_pRecvTable;
				if ( pRT )
				{
					// Get pointer to the event.
					if(pClientClass->m_pCreateEventFn)
					{
						IClientNetworkable *pCE = pClientClass->m_pCreateEventFn();
						if(pCE)
						{
							// Prepare to copy in the data
							pCE->PreDataUpdate( DATA_UPDATE_CREATED );

							// Decode data into client event object
							CL_ParseEventDelta( ei->data, pCE->GetDataTableBasePtr(), pRT );

							// Fire the event!!!
							pCE->PostDataUpdate( DATA_UPDATE_CREATED );

							// Spew to debug area if needed
							CL_DescribeEvent( i, ei, (const char *)pServerClass->m_ClassName );

							success = true;
						}
					}
				}
			}
		}

		if ( !success )
		{
			Con_DPrintf( "Failed to execute event for classId %i\n", ei->index - 1 );
		}

		// Zero out the remaining fields
		CL_ResetEvent( ei );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find first empty event slot
//-----------------------------------------------------------------------------
CEventInfo *CL_FindEmptyEvent( void )
{
	int				i;
	CEventState		*es;
	CEventInfo		*ei;

	es = &cl.events;

	// Look for first slot where index is != 0
	for ( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		ei = &es->ei[ i ];
		if ( ei->index != 0 )
			continue;

		return ei;
	}

	// No slots available
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:  All slots are full, kill off the first one that's not reliable
// Output : CEventInfo
//-----------------------------------------------------------------------------
CEventInfo *CL_FindUnreliableEvent( void )
{
	int i;
	CEventState *es;
	CEventInfo *ei;

	es = &cl.events;
	for ( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		ei = &es->ei[ i ];
		if ( ei->index != 0 )
		{
			// It's reliable, so skip it
			if ( ei->flags & FEV_RELIABLE )
			{
				continue;
			}
		}

		return ei;
	}

	// This should never happen
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Places raw event data onto the event queue
// Input  : flags - 
//			index - 
//			delay - 
//			*pargs - 
//-----------------------------------------------------------------------------
void CL_QueueEvent( int flags, int index, float delay, unsigned char *pargs, int arglen )
{
	bool unreliable = !( flags & FEV_RELIABLE ) ? true : false;

	CEventInfo *ei;

	if ( unreliable )
	{
		// Don't actually queue unreliable events if playing a demo and skipping ahead
		if ( demo->IsPlayingBack() && 
			( demo->IsSkippingAhead() || demo->IsFastForwarding() ) )
		{
			return;
		}
	}

	// Find a normal slot
	ei = CL_FindEmptyEvent();
	if ( !ei && unreliable )
	{
		return;
	}

	// Okay, so find any old unreliable slot
	if ( !ei )
	{
		ei = CL_FindUnreliableEvent();
		if ( !ei )
		{
			return;
		}
	}

	ei->index			= index;
	ei->fire_time		= delay ? ( cl.gettime() + delay ) : 0.0;
	ei->flags			= flags;
	
	// Copy in raw event data
	memcpy( ei->data, pargs, sizeof( ei->data ) );
	ei->bits = arglen;
}

//-----------------------------------------------------------------------------
// Purpose: Parse a raw event from the network stream.
//-----------------------------------------------------------------------------
void CL_Parse_Event( void )
{
	int					i;
	int					num_events;
	int					event_index;
	float				delay;
	unsigned char		data[ 1024 ];
	int					datalen;

	num_events = MSG_ReadBitByte( Q_log2( MAX_EVENT_QUEUE ) );

	for ( i = 0 ; i < num_events; i++ )
	{
		delay			= 0.0;

		event_index = MSG_ReadBitLong( cl.m_nServerClassBits );

		// Get event length
		datalen = MSG_ReadBitShort( EVENT_DATA_LEN_BITS );
		MSG_GetReadBuf()->ReadBits( data, datalen );		

		if ( MSG_ReadOneBit() )
		{
			delay = (float)MSG_ReadBitShort( 16 ) / 100.0;
		}
	
		// Place event on queue
		CL_QueueEvent( 0, event_index, delay, data, datalen );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parses a reliable event from the network
//-----------------------------------------------------------------------------
void CL_Parse_ReliableEvent( void )
{
	int					event_index;
	float				delay;
	unsigned char		data[ 1024 ];
	int					datalen;

	delay			= 0.0;

	event_index = MSG_ReadBitLong( cl.m_nServerClassBits );
	// Get event length
	datalen = MSG_ReadBitLong( EVENT_DATA_LEN_BITS );
	MSG_GetReadBuf()->ReadBits( data, datalen );		
	if ( MSG_ReadOneBit() )
	{
		delay = (float)MSG_ReadBitLong( 16 ) / 100.0;
	}

	CL_QueueEvent( FEV_RELIABLE, event_index, delay, data, datalen );
}