#include "quakedef.h"
#include "client.h"
#include "server.h"
#include "protocol.h"
#include "conprint.h"
#include "sys.h"
#include "mem.h"
#include <stdarg.h>
#include "measure_section.h"
#include "host.h"
#include "demo.h"
#include "filesystem_engine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAKE_FRAGID(id,count)	( ( ( id & 0xffff ) << 16 ) | ( count & 0xffff ) )
#define FRAG_GETID(fragid)		( ( fragid >> 16 ) & 0xffff )
#define FRAG_GETCOUNT(fragid)	( fragid & 0xffff )

// UDP has 28 byte headers
#define UDP_HEADER_SIZE 28

// How fast to converge flow estimates
#define FLOW_AVG ( 2.0 / 3.0 )
 // Don't compute more often than this
#define FLOW_INTERVAL 0.1    

// Biggest packet that has frag and or reliable data
#define MAX_RELIABLE_PAYLOAD 1200

// Biggest packet on a resend ( if datagram size is > this - the 1200 ( 200 bytes ) for the reliable, it gets discarded )
#define MAX_RESEND_PAYLOAD 1400

extern ConVar scr_downloading;

// Forward declarations
void Netchan_FlushIncoming( netchan_t *chan, int stream );
void Netchan_AddBufferToList( fragbuf_t **pplist, fragbuf_t *pbuf );

int		net_drop;

ConVar	net_showpackets( "net_showpackets", "0", 0, "Dump packet summary to console" );
ConVar	net_showdrop( "net_showdrop", "0", 0, "Show dropped packets in console" );
ConVar	net_drawslider( "net_drawslider", "0", 0, "Draw completion slider during signon" );
ConVar  net_chokeloopback( "net_chokeloop", "0", 0, "Apply bandwidth choke to loopback packets" ); 

/*
==============================
Netchan_UnlinkFragment

==============================
*/
void Netchan_UnlinkFragment( fragbuf_t *buf, fragbuf_t **list )
{
	fragbuf_t *search;

	if ( !list )
	{
		Con_Printf( "Netchan_UnlinkFragment:  Asked to unlink fragment from empty list, ignored\n" );
		return;
	}

	// At head of list
	if ( buf == *list )
	{
		// Remove first element
		*list = buf->next;
		
		// Destroy remnant
		delete[] buf;
		return;
	}

	search = *list;
	while ( search->next )
	{
		if ( search->next == buf )
		{
			search->next = buf->next;
			// Detroy remnant
			delete[] buf;
			return;
		}

		search = search->next;
	}

	Con_Printf( "Netchan_UnlinkFragment:  Couldn't find fragment\n" );
}

/*
===============
Netchan_OutOfBand

Sends an out-of-band datagram
================
*/
void Netchan_OutOfBand (netsrc_t sock, netadr_t adr, int length, byte *data)
{
	sizebuf_t	send;
	byte		send_buf[ NET_MAX_PAYLOAD ];
// write the packet header
	send.data = send_buf;
	send.maxsize = sizeof(send_buf);
	send.cursize = 0;

	MSG_WriteLong (&send, -1);	// -1 sequence means out of band
	SZ_Write (&send, data, length);

// send the datagram
	//zoid, no input in demo playback mode
	if (!Demo_IsPlayingBack())
	{
		NET_SendPacket (sock, send.cursize, send.data, adr);
	}
}

/*
===============
Netchan_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void Netchan_OutOfBandPrint (netsrc_t sock, netadr_t adr, char *format, ...)
{
	va_list		argptr;
	static char		string[8192];		// ??? why static?
	
	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);

	Netchan_OutOfBand (sock, adr, Q_strlen(string) + 1, (byte *)string);
}


/*
==============================
Netchan_ClearFragbufs

==============================
*/
void Netchan_ClearFragbufs( fragbuf_t **ppbuf )
{
	fragbuf_t *buf, *n;

	if ( !ppbuf )
		return;

	// Throw away any that are sitting around
	buf = *ppbuf;
	while ( buf )
	{
		n = buf->next;
		delete[] buf;
		buf = n;
	}
	*ppbuf = NULL;
}

/*
==============================
Netchan_ClearFragments

==============================
*/
void Netchan_ClearFragments( netchan_t *chan )
{
	fragbufwaiting_t *wait;
	int i;

	for ( i = 0; i < MAX_STREAMS; i++ )
	{
		wait = chan->waitlist[ i ];
		while ( wait )
		{
			Netchan_ClearFragbufs( &wait->fragbufs );
			wait = wait->next;
		}
		chan->waitlist[ i ] = NULL;

		Netchan_ClearFragbufs( &chan->fragbufs[ i ] );
		Netchan_FlushIncoming( chan, i );
	}
}

/*
==============================
Netchan_Clear

==============================
*/
void Netchan_Clear( netchan_t *chan )
{
	int i;
	Netchan_ClearFragments( chan );

	chan->cleartime			= 0.0;
	chan->reliable_length	= 0;

	for ( i = 0 ; i < MAX_STREAMS; i++ )
	{
		chan->reliable_fragid[ i ]		= 0;
		chan->reliable_fragment[ i ]	= 0;
		chan->fragbufcount[ i ]			= 0;
		chan->frag_startpos[ i ]		= 0;
		chan->frag_length[ i ]			= 0;
		chan->incomingready[ i ]		= false;
	}

	Q_memset( chan->flow, 0, sizeof( chan->flow ) );
}

/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup (netsrc_t socketnumber, netchan_t *chan, netadr_t adr )
{
	Netchan_Clear( chan );

	Q_memset (chan, 0, sizeof(*chan));
	
	chan->sock				= socketnumber;
	chan->remote_address	= adr;
	chan->last_received		= realtime;
	chan->connect_time		= realtime;

	chan->message.StartWriting(chan->message_buf, sizeof(chan->message_buf));
	chan->message.SetDebugName( "netchan_t::message" );

	chan->rate				= DEFAULT_RATE;

	// Prevent the first message from getting dropped after connection is set up.
	chan->outgoing_sequence = 1;
}

/*
===============
Netchan_CanPacket

Returns true if the bandwidth choke isn't active
================
*/
qboolean Netchan_CanPacket ( netchan_t *chan )
{
	// Never choke loopback packets.
	if ( !net_chokeloopback.GetInt() && ( chan->remote_address.type == NA_LOOPBACK ) )
	{
		chan->cleartime = realtime;
		return true;
	}

	if ( chan->cleartime < realtime )
	{
		return true;
	}

	return false;
}

/*
==============================
Netchan_UpdateFlow

==============================
*/
void Netchan_UpdateFlow( netchan_t *chan )
{
	int flow;
	int	start;
	int i;
	int bytes = 0;
	float faccumulatedtime = 0.0;
	flow_t *pflow;
	flowstats_t *pstat, *pprev;

	if ( !chan )
		return;

	for ( flow = 0; flow < 2; flow++ )
	{
		pflow = &chan->flow[ flow ];

		if ( ( realtime - pflow->nextcompute ) < FLOW_INTERVAL )
			continue;

		pflow->nextcompute = realtime + FLOW_INTERVAL;

		start = pflow->current - 1;

		pprev = &pflow->stats[ start & ( MAX_LATENT - 1 ) ];

		// Compute data flow rate
		for ( i = 1; i < MAX_LATENT/2; i++ )
		{
			// Most recent message then backward from there
			pstat = &pflow->stats[ ( start - i ) & ( MAX_LATENT - 1 ) ];

			bytes += ( pstat->size );
			faccumulatedtime += ( pprev->time - pstat->time );

			pprev = pstat;
		}

		if ( !faccumulatedtime )
		{
			pflow->kbytespersec = 0.0;
		}
		else
		{
			pflow->kbytespersec = (float)bytes / faccumulatedtime;
			pflow->kbytespersec /= 1024.0;
		}

		pflow->avgkbytespersec = ( FLOW_AVG ) * pflow->avgkbytespersec + ( 1.0 - FLOW_AVG ) * pflow->kbytespersec;
	}
}

/*
===============
Netchan_TransmitBits

tries to send an unreliable message to a connection, and handles the
transmition / retransmition of the reliable messages.

A 0 length will still generate a packet and deal with the reliable messages.
================
*/
void Netchan_TransmitBits (netchan_t *chan, int length, byte *data)
{
	byte		send_buf[ NET_MAX_MESSAGE ];
	qboolean	send_reliable;
	qboolean	send_reliable_fragment;
	qboolean	send_resending = false;
	unsigned	w1, w2;
	int i, j;

	float       fRate;

// check for message overflow
	if (chan->message.IsOverflowed())
	{
		Con_Printf ("%s:Outgoing message overflow\n"
			, NET_AdrToString (chan->remote_address));
		return;
	}

// if the remote side dropped the last reliable message, resend it
	send_reliable = false;

	if ( chan->incoming_acknowledged > chan->last_reliable_sequence &&
		 chan->incoming_reliable_acknowledged != chan->reliable_sequence )
	{
		send_reliable = true;
		send_resending = true;
	}

	//
	// A packet can have "reliable payload + frag payload + unreliable payload
	// frag payload can be a file chunk, if so, it needs to be parsed on the receiving end and reliable payload + unreliable payload need
	// to be passed on to the message queue.  The processing routine needs to be able to handle the case where a message comes in and a file
	// transfer completes
	//
	//
	// if the reliable transmit buffer is empty, copy the current message out
	if ( !chan->reliable_length )
	{
		qboolean send_frag = false;
		fragbuf_t *pbuf;

		// Will be true if we are active and should let chan->message get some bandwidth
		int		 send_from_frag[ MAX_STREAMS ] = { 0, 0 };
		int		 send_from_regular	= 0;

		// If we have data in the waiting list(s) and we have cleared the current queue(s), then 
		//  push the waitlist(s) into the current queue(s)
		Netchan_FragSend( chan );

		// Sending regular payload
		send_from_regular = ( chan->message.GetNumBytesWritten() ) ? 1 : 0;

		// Check to see if we are sending a frag payload
		//
		for ( i = 0; i < MAX_STREAMS; i++ )
		{
			if ( chan->fragbufs[ i ] )
			{
				send_from_frag[ i ] = 1;
			}
		}

		// Stall reliable payloads if sending from frag buffer
		if ( send_from_regular && ( send_from_frag[ FRAG_NORMAL_STREAM ] ) )
		{	
			send_from_regular = false;

			// If the reliable buffer has gotten too big, queue it at the end of everything and clear out buffer
			//
			if ( chan->message.GetNumBytesWritten() > MAX_RELIABLE_PAYLOAD )
			{
				Netchan_CreateFragments( chan == &cls.netchan ? false : true, chan, &chan->message );
				chan->message.Reset();
			}
		}

		// Startpos will be zero if there is no regular payload
		for ( i = 0 ; i < MAX_STREAMS; i++ )
		{
			chan->frag_startpos[ i ]		= 0;

			// Assume no fragment is being sent

			chan->reliable_fragment[ i ]	= 0;
			chan->reliable_fragid[ i ]		= 0;
			chan->frag_length[ i ]			= 0;

			if ( send_from_frag[ i ] )
			{
				send_frag = true;
			}
		}

		if ( send_from_regular || send_frag )
		{
			chan->reliable_sequence ^= 1;
			send_reliable = true;
		}

		if ( send_from_regular )
		{
			Q_memcpy ( chan->reliable_buf, chan->message_buf, chan->message.GetNumBytesWritten() );
			chan->reliable_length = chan->message.GetNumBitsWritten();
			chan->message.Reset();

			// If we send fragments, this is where they'll start
			for ( i = 0 ; i < MAX_STREAMS; i++ )
			{
				chan->frag_startpos[ i ] = chan->reliable_length;
			}
		}

		for ( i = 0 ; i < MAX_STREAMS; i++ )
		{
			int fragment_size;

			extern int host_framecount;

			// Is there someting in the fragbuf?
			pbuf = chan->fragbufs[ i ];

			fragment_size = 0; // Compiler warning.
			if ( pbuf )
			{
				fragment_size = pbuf->frag_message.GetNumBytesWritten();
				
				// Files set size a bit differently.
				if ( pbuf->isfile && !pbuf->isbuffer )
				{
					fragment_size = pbuf->size;
				}
			}

			int newpayloadsize = ( ( chan->reliable_length + ( fragment_size << 3 ) ) + 7 ) >> 3;

			// Make sure we have enought space left
			if ( send_from_frag[ i ] && 
				pbuf && 
				( newpayloadsize < MAX_RELIABLE_PAYLOAD ) )
			{
				chan->reliable_fragid[ i ] = MAKE_FRAGID(pbuf->bufferid,chan->fragbufcount[ i ]); // Which buffer are we sending?
			
				// If it's not in-memory, then we'll need to copy it in frame the file handle.
				if ( pbuf->isfile && !pbuf->isbuffer )
				{
					unsigned char filebuffer[ 2048 ];

					FileHandle_t hfile;

					COM_OpenFile( pbuf->filename, &hfile );
					g_pFileSystem->Seek( hfile, pbuf->foffset, FILESYSTEM_SEEK_HEAD );
					g_pFileSystem->Read( filebuffer, pbuf->size, hfile );

					pbuf->frag_message.WriteBits( filebuffer, pbuf->size << 3 );

					COM_CloseFile( hfile );
				}

				// Copy frag stuff on top of current buffer
				bf_write temp;
				temp.StartWriting( chan->reliable_buf, sizeof( chan->reliable_buf ), chan->reliable_length );

				temp.WriteBits( pbuf->frag_message.GetData(), pbuf->frag_message.GetNumBitsWritten() );

				chan->reliable_length += pbuf->frag_message.GetNumBitsWritten();
				chan->frag_length[ i ] = pbuf->frag_message.GetNumBitsWritten();

				// Unlink  pbuf
				Netchan_UnlinkFragment( pbuf, &chan->fragbufs[ i ] );	

				chan->reliable_fragment[ i ] = 1;

				// Offset the rest of the starting positions
				for ( j = i + 1; j < MAX_STREAMS; j++ )
				{
					chan->frag_startpos[ j ] += chan->frag_length[ i ];
				}
			}
		}
	}

	bf_write send( "NetChan_TransmitBits->send", send_buf, sizeof(send_buf) );

	// Prepare the packet header
	w1 = chan->outgoing_sequence | (send_reliable<<31);
	w2 = chan->incoming_sequence | (chan->incoming_reliable_sequence<<31);

	send_reliable_fragment = false;

	for ( i = 0 ; i < MAX_STREAMS; i++ )
	{
		if ( chan->reliable_fragment[ i ] )
		{
			send_reliable_fragment = true;
			break;
		}
	}

	if ( send_reliable && send_reliable_fragment )
	{
		w1 |= ( 1 << 30 );
	}

	chan->outgoing_sequence++;

	send.WriteLong ( w1 );
	send.WriteLong ( w2 );
	
	if ( send_reliable && send_reliable_fragment )
	{
		for ( i = 0 ; i < MAX_STREAMS; i++ )
		{
			if ( chan->reliable_fragment[ i ] )
			{
				send.WriteByte( 1 );
				send.WriteLong( chan->reliable_fragid[ i ] );
				send.WriteLong( chan->frag_startpos[ i ] );
				send.WriteLong( chan->frag_length[ i ] );
			}
			else 
			{
				send.WriteByte( 0 );
			}
		}
	}

	// Copy the reliable message to the packet first
	if ( send_reliable )
	{
		send.WriteBits( chan->reliable_buf, chan->reliable_length );
		chan->last_reliable_sequence = chan->outgoing_sequence - 1;
	}

	// Is there room for the unreliable payload?
	if ( send.GetNumBitsLeft() >= length )
	{
		send.WriteBits(data, length);
	}
	else
	{
		Con_DPrintf("Netchan_Transmit:  Unreliable would overfow, ignoring\n");
	}

	// Deal with packets that are too small for some networks
	if ( send.GetNumBytesWritten() < 16 )	// Packet too small for some networks
	{
		int i;
		// Go ahead and pad a full 16 extra bytes -- this only happens during authentication / signon
		for ( i = send.GetNumBytesWritten(); i < 16; i++ )		
		{
			// Note that the server can parse svc_nop, too.
			send.WriteByte(svc_nop);
		}
	}

	chan->flow[FLOW_OUTGOING].stats[chan->flow[FLOW_OUTGOING].current & ( MAX_LATENT - 1 )].size = send.GetNumBytesWritten() + UDP_HEADER_SIZE;
	chan->flow[FLOW_OUTGOING].stats[chan->flow[FLOW_OUTGOING].current & ( MAX_LATENT - 1 )].time = realtime;
	chan->flow[FLOW_OUTGOING].totalbytes += ( send.GetNumBytesWritten() + UDP_HEADER_SIZE );
	chan->flow[FLOW_OUTGOING].current++;

	Netchan_UpdateFlow( chan );

	// Send the datagram
	if ( !Demo_IsPlayingBack() )
	{
		NET_SendPacket (chan->sock, send.GetNumBytesWritten(), send.GetData(), chan->remote_address);
	}

#if !defined( _DEBUG )
	if ( sv.active && sv_lan.GetInt() )
	{
		fRate = 1.0f / DEFAULT_RATE;
	}
	else
#endif
	{
		fRate = 1.0f / chan->rate;
	}

	if ( chan->cleartime < realtime )
	{
		chan->cleartime = realtime;
	}

	chan->cleartime += ( send.GetNumBytesWritten() + UDP_HEADER_SIZE ) * fRate;

	if ( net_showpackets.GetInt() && net_showpackets.GetInt() != 2 )
	{
		char c;
		int mask = 63;
	
		c = ( chan == &cls.netchan ) ? 'c' : 's';

		Con_Printf (" %c --> sz=%i seq=%i ack=%i rel=%i tm=%f\n"
			, c
			, send.GetNumBytesWritten()
			, ( chan->outgoing_sequence - 1 ) & mask
			, chan->incoming_sequence & mask
			, send_reliable ? 1 : 0
			, (float)realtime );
	}
}

/*
==============================
Netchan_Transmit

==============================
*/
void Netchan_Transmit (netchan_t *chan, int lengthInBytes, byte *data)
{
	Netchan_TransmitBits(chan, lengthInBytes << 3, data);
}

/*
==============================
Netchan_AllocFragbuf

==============================
*/
fragbuf_t *Netchan_AllocFragbuf( void )
{
	fragbuf_t *buf;

	buf = ( fragbuf_t * )new fragbuf_t;
	memset( buf, 0, sizeof( *buf ) );

	buf->frag_message.StartWriting( buf->frag_message_buf, sizeof( buf->frag_message_buf ) );
	buf->frag_message.SetDebugName( "fragbuf_t::frag_message" );

	return buf;
}

/*
==============================
Netchan_FindBufferById

==============================
*/
fragbuf_t *Netchan_FindBufferById( fragbuf_t **pplist, int id, qboolean allocate )
{
	fragbuf_t *list = *pplist;
	fragbuf_t *pnewbuf;

	while ( list )
	{
		if ( list->bufferid == id )
			return list;

		list = list->next;
	}

	if ( !allocate )
		return NULL;

// Create new entry
	pnewbuf = Netchan_AllocFragbuf();
	pnewbuf->bufferid = id;
	Netchan_AddBufferToList( pplist, pnewbuf );

	return pnewbuf;
}

/*
==============================
Netchan_CheckForCompletion

==============================
*/
void Netchan_CheckForCompletion( netchan_t *chan, int stream, int intotalbuffers )
{
	int c;
	int size;
	int id;
	fragbuf_t *p;

	size = 0;
	c = 0;

	
	p = chan->incomingbufs[ stream ];
	if ( !p )
		return;

	while ( p )
	{
		size += p->frag_message.GetNumBytesWritten();
		c++;

		id = FRAG_GETID( p->bufferid );
		if ( id != c )
		{
			if ( chan == &cls.netchan )
			{
				Con_Printf( "Netchan_CheckForCompletion:  Lost/dropped fragment would cause stall, retrying connection\n" );
				Cbuf_AddText( "retry\n" );
			}
		}

		p = p->next;
	}

	// Received final message
	if ( c == intotalbuffers )
	{
		chan->incomingready[ stream ] = true;
		// Con_Printf( "\nincoming is complete %i bytes waiting\n", size );
	}
}

/*
=================
Netchan_Process

called when the current net_message is from remote_address
modifies net_message so that it points to the packet payload
=================
*/
qboolean Netchan_Process (netchan_t *chan)
{
	int				i;
	unsigned int	sequence, sequence_ack;
	unsigned int	reliable_ack, reliable_message;
	unsigned int	fragid[ MAX_STREAMS ] = { 0, 0 };
	qboolean		frag_message[ MAX_STREAMS ] = { false, false };
	int				frag_offset[ MAX_STREAMS ] = { 0, 0 };
	int				frag_length[ MAX_STREAMS ] = { 0, 0 };
	qboolean		message_contains_fragments;

	if (
		!Demo_IsPlayingBack() &&
		!NET_CompareAdr ( net_from, chan->remote_address ) )
	{
		return false;
	}

	
// get sequence numbers		
	MSG_BeginReading ();
	sequence		= MSG_ReadLong ();
	sequence_ack	= MSG_ReadLong ();

	reliable_message = sequence >> 31;
	reliable_ack = sequence_ack >> 31;

	message_contains_fragments = sequence & ( 1 << 30 ) ? true : false;

	if ( message_contains_fragments )
	{
		for ( i = 0; i < MAX_STREAMS; i++ )
		{
			if ( MSG_ReadByte() )
			{
				frag_message[ i ] = true;
				fragid[ i ] = MSG_ReadLong();
				frag_offset[ i ] = (int)MSG_ReadLong();
				frag_length[ i ] = (int)MSG_ReadLong();
			}
		}
	}

	sequence &= ~(1<<31);	
	sequence &= ~(1<<30);
	sequence_ack &= ~(1<<31);	

	if ( net_showpackets.GetInt() && net_showpackets.GetInt() != 3 )
	{
		char c;
		
		c = ( chan == &cls.netchan ) ? 'c' : 's';

		Con_Printf (" %c <-- sz=%i seq=%i ack=%i rel=%i tm=%f\n"
			, c
			, net_message.cursize
			, sequence & 63
			, sequence_ack & 63 
			, reliable_message
			, realtime );
	}

//
// discard stale or duplicated packets
//
	if (sequence <= (unsigned)chan->incoming_sequence)
	{
		if ( net_showdrop.GetInt() )
		{
			if ( sequence == (unsigned)chan->incoming_sequence )
			{
				Con_Printf ("%s:duplicate packet %i at %i\n"
					, NET_AdrToString (chan->remote_address)
					,  sequence
					, chan->incoming_sequence);
			}
			else
			{
				Con_Printf ("%s:out of order packet %i at %i\n"
					, NET_AdrToString (chan->remote_address)
					,  sequence
					, chan->incoming_sequence);
			}
		}
		
		return false;
	}

//
// dropped packets don't keep the message from being used
//
	net_drop = sequence - (chan->incoming_sequence+1);
	if (net_drop > 0)
	{
		if ( net_showdrop.GetInt() )
		{
			Con_Printf ("%s:Dropped %i packets at %i\n"
			, NET_AdrToString (chan->remote_address)
			, sequence-(chan->incoming_sequence+1)
			, sequence);

		}
	}

//
// if the current outgoing reliable message has been acknowledged
// clear the buffer to make way for the next
//
	if (reliable_ack == (unsigned)chan->reliable_sequence)
	{
		// Make sure we actually could have ack'd this message
		if ( chan->incoming_acknowledged + 1 >= chan->last_reliable_sequence )
		{
			chan->reliable_length = 0;	// it has been received
		}
	}
	
//
// if this message contains a reliable message, bump incoming_reliable_sequence 
//
	chan->incoming_sequence = sequence;
	chan->incoming_acknowledged = sequence_ack;
	chan->incoming_reliable_acknowledged = reliable_ack;
	if (reliable_message)
	{
		chan->incoming_reliable_sequence ^= 1;
	}

	chan->last_received = realtime;

	// Update data flow stats
	chan->flow[FLOW_INCOMING].stats[chan->flow[FLOW_INCOMING].current & ( MAX_LATENT - 1 )].size = net_message.cursize + UDP_HEADER_SIZE;
	chan->flow[FLOW_INCOMING].stats[chan->flow[FLOW_INCOMING].current & ( MAX_LATENT - 1 )].time = realtime;
	chan->flow[FLOW_INCOMING].totalbytes += ( net_message.cursize + UDP_HEADER_SIZE );
	chan->flow[FLOW_INCOMING].current++;

	Netchan_UpdateFlow( chan );

	if ( message_contains_fragments )
	{
		for ( i = 0 ; i < MAX_STREAMS; i++ )
		{
			int j;
			fragbuf_t *pbuf;
			int inbufferid;
			int intotalbuffers;

			if ( !frag_message[ i ] )
				continue;
		
			inbufferid = FRAG_GETID( fragid[ i ] );
			intotalbuffers = FRAG_GETCOUNT( fragid[ i ] );

			if ( fragid[ i ]  != 0 )
			{
				pbuf = Netchan_FindBufferById( &chan->incomingbufs[ i ], fragid[ i ], true );
				if ( pbuf )
				{
					int bits;

					bits = frag_length[ i ];
				
						// Copy in data
					pbuf->frag_message.Reset();

					bf_read temp;
					temp.StartReading( net_message.data, net_message.cursize, 
						MSG_GetReadBuf()->GetNumBitsRead() + frag_offset[ i ] );

					byte buffer[ 2048 ];
					temp.ReadBits( buffer, bits );
					pbuf->frag_message.WriteBits( buffer, bits );
				}
				else
				{
					Con_Printf( "Netchan_Process:  Couldn't allocate or find buffer %i\n", inbufferid );
				}

				// Count # of incoming bufs we've queued? are we done?
				Netchan_CheckForCompletion( chan, i, intotalbuffers );
			}

			// Rearrange incoming data to not have the frag stuff in the middle of it
			int oldpos = MSG_GetReadBuf()->GetNumBitsRead();
			int curbit = MSG_GetReadBuf()->GetNumBitsRead() + frag_offset[ i ];
			int numbitstoremove = frag_length[ i ];
			MSG_GetReadBuf()->ExciseBits( curbit , numbitstoremove );
			MSG_GetReadBuf()->Seek( oldpos );

			for ( j = i + 1; j < MAX_STREAMS; j++ )
			{
				frag_offset[ j ] -= frag_length[ i ];
			}
		}

		// Is there anything left to process?
		if ( MSG_GetReadBuf()->GetNumBitsLeft() <= 0 )
		{
			return false;
		}
	}
	return true;
}

/*
==============================
Netchan_FragSend

Fragmentation buffer is full and user is prepared to send
==============================
*/
void Netchan_FragSend( netchan_t *chan )
{
	fragbufwaiting_t *wait;
	int i;

	if ( !chan )
		return;

	for ( i = 0 ; i < MAX_STREAMS; i++ )
	{
		// Already something queued up, just leave in waitlist
		if ( chan->fragbufs[ i ] )
		{
			continue;
		}

		// Nothing to queue?
		if ( !chan->waitlist[ i ] )
		{
			continue;
		}

		wait = chan->waitlist[ i ] ;
		chan->waitlist[ i ]  = chan->waitlist[ i ] ->next;

		wait->next = NULL;

		// Copy in to fragbuf
		chan->fragbufs[ i ] = wait->fragbufs;
		chan->fragbufcount[ i ] = wait->fragbufcount;

		// Throw away wait list
		delete wait;
	}
}

/*
==============================
Netchan_AddBufferToList

==============================
*/
void Netchan_AddBufferToList( fragbuf_t **pplist, fragbuf_t *pbuf )
{
	// Find best slot
	fragbuf_t *pprev, *n;
	int		id1, id2;

	pbuf->next = NULL;

	if ( !pplist )
		return;

	if ( !*pplist )
	{
		pbuf->next = *pplist;
		*pplist = pbuf;
		return;
	}

	pprev = *pplist;
	while ( pprev->next )
	{
		n = pprev->next; // Next item in list
		id1 = FRAG_GETID( n->bufferid );
		id2 = FRAG_GETID( pbuf->bufferid );

		if ( id1 > id2 )
		{
			// Insert here
			pbuf->next = n->next;
			pprev->next = pbuf;
			return;
		}

		pprev = pprev->next;
	}

	// Insert at end
	pprev->next = pbuf;
}

/*
==============================
Netchan_AddFragbufToTail

==============================
*/
void Netchan_AddFragbufToTail( fragbufwaiting_t *wait, fragbuf_t *buf )
{
	fragbuf_t *p;

	buf->next = NULL;
	wait->fragbufcount++;
	if ( !wait->fragbufs )
	{
		wait->fragbufs = buf;
		return;
	}

	p = wait->fragbufs;
	while ( p->next )
	{
		p = p->next;
	}

	p->next = buf;
}

static ConVar net_blocksize( "net_blocksize", "1024", 0, "Network file fragmentation block size.",
	true, 16, true, 1400 );

/*
==============================
Netchan_CreateFragments_

==============================
*/
void Netchan_CreateFragments_( qboolean server, netchan_t *chan, bf_write *msg )
{
	fragbuf_t *buf;
	int chunksize;
	int send;
	int remaining;
	int pos;
	int bufferid = 1;
	
	fragbufwaiting_t *wait, *p;
	
	if ( msg->GetNumBytesWritten() == 0 )
		return;

	chunksize = clamp( net_blocksize.GetInt(), 16, 1400 );

	wait = ( fragbufwaiting_t * )new fragbufwaiting_t;
	memset( wait, 0, sizeof( *wait ) );

	remaining = msg->GetNumBytesWritten();
	pos = 0;
	while ( remaining > 0 )
	{
		send = min( remaining, chunksize );
		remaining -= send;
	
		// 
		buf = Netchan_AllocFragbuf();
		if ( !buf )
		{
			Con_Printf( "Couldn't allocate fragbuf_t\n" );
			delete wait;

			if ( server )
			{
				SV_DropClient( host_client, false, "Malloc problem" );
			}
			else
			{
				Host_Disconnect();
			}
			return;
		}

		buf->bufferid = bufferid++;

		// Copy in data
		buf->frag_message.Reset();
		buf->frag_message.WriteBits( &msg->GetData()[ pos ], send << 3 );
		pos += send;

		Netchan_AddFragbufToTail( wait, buf );
	}

	// Now add waiting list item to end of buffer queue
	if ( !chan->waitlist[ FRAG_NORMAL_STREAM ] )
	{
		chan->waitlist[ FRAG_NORMAL_STREAM ] = wait;
	}
	else
	{
		p = chan->waitlist[ FRAG_NORMAL_STREAM ];
		while ( p->next )
		{
			p = p->next;
		}

		p->next = wait;
	}
}

/*
==============================
Netchan_CreateFragments

==============================
*/
void Netchan_CreateFragments( qboolean server, netchan_t *chan, bf_write *msg )
{
	// Always queue any pending reliable data ahead of the fragmentation buffer
	if ( chan->message.GetNumBytesWritten() > 0 )
	{
		Netchan_CreateFragments_( server, chan, &chan->message );
		chan->message.Reset();
	}

	Netchan_CreateFragments_( server, chan, msg );
}

/*
==============================
Netchan_CreateFileFragmentsFromBuffer

==============================
*/
void Netchan_CreateFileFragmentsFromBuffer ( qboolean server, netchan_t *chan, char *filename, unsigned char *pbuf, int size )
{
	fragbuf_t *buf;
	int chunksize;
	int send;
	int remaining;
	int pos;
	int bufferid = 1;
	qboolean firstfragment = true;
	
	fragbufwaiting_t *wait, *p;

	if ( !size )
		return;

	chunksize = clamp( net_blocksize.GetInt(), 16, 512 );

	wait = ( fragbufwaiting_t * )new fragbufwaiting_t;
	memset( wait, 0, sizeof( *wait ) );

	remaining = size;
	pos = 0;
	while ( remaining > 0 )
	{
		send = min( remaining, chunksize );

		buf = Netchan_AllocFragbuf();
		if ( !buf )
		{
			Con_Printf( "Couldn't allocate fragbuf_t\n"  );
			delete wait;

			if ( server )
			{
				SV_DropClient( host_client, false, "Malloc problem" );
			}
			else
			{
				Host_Disconnect();
			}
			return;
		}

		buf->bufferid = bufferid++;

		// Copy in data
		buf->frag_message.Reset();

		// 
		if ( firstfragment )
		{
			firstfragment = false;
			// Write filename
			buf->frag_message.WriteString( filename );

			// Send a bit less on first package
			send -= buf->frag_message.GetNumBytesWritten();
		}

		buf->isbuffer = true;
		buf->isfile = true;
		buf->size = send;
		buf->foffset = pos;
	
		buf->frag_message.WriteBits( &pbuf[ pos ], send << 3 );

		pos += send;
		remaining -= send;

		Netchan_AddFragbufToTail( wait, buf );
	}

	// Now add waiting list item to end of buffer queue
	if ( !chan->waitlist[ FRAG_FILE_STREAM ] )
	{
		chan->waitlist[ FRAG_FILE_STREAM ] = wait;
	}
	else
	{
		p = chan->waitlist[ FRAG_FILE_STREAM ];
		while ( p->next )
		{
			p = p->next;
		}

		p->next = wait;
	}
}

/*
==============================
Netchan_CreateFileFragments

==============================
*/
int Netchan_CreateFileFragments( qboolean server, netchan_t *chan, char *filename )
{
	fragbuf_t *buf;
	int chunksize;
	int send;
	int remaining;
	int pos;
	int bufferid = 1;
	FileHandle_t hfile;
	int filesize = 0;
	qboolean firstfragment = true;
	
	fragbufwaiting_t *wait, *p;
	
	chunksize = clamp( net_blocksize.GetInt(), 16, 512 );

	if ( ( filesize = COM_OpenFile( filename, &hfile ) ) == -1 )
	{
		Con_Printf( "Warning:  Unable to open %s for transfer\n", filename );
		return 0;
	}

	// close the file
	COM_CloseFile( hfile );

	wait = ( fragbufwaiting_t * )new fragbufwaiting_t;
	memset( wait, 0, sizeof( *wait ) );

	remaining = filesize;
	pos = 0;
	while ( remaining > 0 )
	{
		send = min( remaining, chunksize );

		buf = Netchan_AllocFragbuf();
		if ( !buf )
		{
			Con_Printf( "Couldn't allocate fragbuf_t\n"  );
			delete wait;

			if ( server )
			{
				SV_DropClient( host_client, false, "Malloc problem" );
			}
			else
			{
				Host_Disconnect();
			}
			return 0;
		}

		buf->bufferid = bufferid++;

		// Copy in data
		buf->frag_message.Reset();

		// 
		if ( firstfragment )
		{
			firstfragment = false;
			// Write filename
			buf->frag_message.WriteString( filename );

			// Send a bit less on first package
			send -= buf->frag_message.GetNumBytesWritten();
		}

		buf->isfile = true;
		buf->size = send;
		buf->foffset = pos;
		Q_strcpy( buf->filename, filename );

		pos += send;
		remaining -= send;

		Netchan_AddFragbufToTail( wait, buf );
	}

	// Now add waiting list item to end of buffer queue
	if ( !chan->waitlist[ FRAG_FILE_STREAM ] )
	{
		chan->waitlist[ FRAG_FILE_STREAM ] = wait;
	}
	else
	{
		p = chan->waitlist[ FRAG_FILE_STREAM ];
		while ( p->next )
		{
			p = p->next;
		}

		p->next = wait;
	}

	return 1;
}

/*
==============================
Netchan_FlushIncoming

==============================
*/
void Netchan_FlushIncoming( netchan_t *chan, int stream )
{
	fragbuf_t *p, *n;

	SZ_Clear( &net_message );
	MSG_GetReadBuf()->Reset();

	p = chan->incomingbufs[ stream ];
	while ( p )
	{
		n =p->next;
		delete p;
		p = n;
	}
	chan->incomingbufs[ stream ] = NULL;
	chan->incomingready[ stream ] = false;
}

/*
==============================
Netchan_CopyNormalFragments

==============================
*/
qboolean Netchan_CopyNormalFragments( netchan_t *chan )
{
	fragbuf_t *p, *n;

	if ( !chan->incomingready[ FRAG_NORMAL_STREAM ] )
		return false;

	if ( !chan->incomingbufs[ FRAG_NORMAL_STREAM ] )
	{
		Con_Printf( "Netchan_CopyNormalFragments:  Called with no fragments readied\n" );
		chan->incomingready[ FRAG_NORMAL_STREAM ] = false;
		return false;
	}

	p = chan->incomingbufs[ FRAG_NORMAL_STREAM ];

	SZ_Clear( &net_message );
	MSG_BeginReading();

	while ( p )
	{
		n = p->next;
		
		// Copy it in
		SZ_Write( &net_message, p->frag_message.GetData(), p->frag_message.GetNumBytesWritten() );

		delete p;
		p = n;
	}
	
	chan->incomingbufs[ FRAG_NORMAL_STREAM ] = NULL;

	// Reset flag
	chan->incomingready[ FRAG_NORMAL_STREAM ] = false;

	return true;
}

/*
==============================
Netchan_CopyFileFragments

==============================
*/
qboolean Netchan_CopyFileFragments( netchan_t *chan )
{
	char filename[ MAX_OSPATH ];
	fragbuf_t *p, *n;
	int nsize;
	unsigned char *buffer;
	FileHandle_t hfile;
	int pos;

	if ( !chan->incomingready[ FRAG_FILE_STREAM ] )
		return false;

	if ( !chan->incomingbufs[ FRAG_FILE_STREAM ] )
	{
		Con_Printf( "Netchan_CopyFileFragments:  Called with no fragments readied\n" );
		chan->incomingready[ FRAG_FILE_STREAM ] = false;
		return false;
	}

	p = chan->incomingbufs[ FRAG_FILE_STREAM ];

	SZ_Clear( &net_message );
	// Copy in first chunk so we can get filename out
	SZ_Write( &net_message, p->frag_message.GetData(), p->frag_message.GetNumBytesWritten() );

	MSG_BeginReading();

	Q_strcpy( filename, MSG_ReadString() );
	if ( Q_strlen( filename ) <= 0 )
	{
		Con_Printf( "File fragment received with no filename\nFlushing input queue\n" );
		
		// Clear out bufs
		Netchan_FlushIncoming( chan, FRAG_FILE_STREAM );
		return false;
	}
	else if ( Q_strstr( filename, ".." ) )
	{
		Con_Printf( "File fragment received with relative path, ignoring\n" );
		
		// Clear out bufs
		Netchan_FlushIncoming( chan, FRAG_FILE_STREAM );
		return false;
	}

	Q_strcpy( chan->incomingfilename, filename );

	nsize = COM_OpenFile( filename, &hfile );
	if ( nsize != -1 )
	{
		Con_Printf( "Can't download %s, already exists\n", filename );
		
		// Clear out bufs
		Netchan_FlushIncoming( chan, FRAG_FILE_STREAM );

		COM_CloseFile( hfile );
		return true;
	}

	COM_CreatePath( filename );

	// Create file from buffers
	nsize = 0;
	while ( p )
	{
		nsize += p->frag_message.GetNumBytesWritten(); // Size will include a bit of slop, oh well
		if ( p == chan->incomingbufs[ FRAG_FILE_STREAM ] )
		{
			nsize -= MSG_GetReadBuf()->GetNumBytesRead();
		}
		p = p->next;
	}

	buffer = new unsigned char[ nsize + 1 ];
	if ( !buffer )
	{
		Con_Printf( "Buffer allocation failed on %i bytes\n", nsize + 1 );
		
		// Clear out bufs
		Netchan_FlushIncoming( chan, FRAG_FILE_STREAM );
		return false;
	}

	Q_memset( buffer, 0, nsize + 1 );

	p = chan->incomingbufs[ FRAG_FILE_STREAM ];
	pos = 0;
	while ( p )
	{
		int cursize;

		n = p->next;
		
		cursize = p->frag_message.GetNumBytesWritten();

		// First message has the file name, don't write that into the data stream, just write the rest of the actual data
		if ( p == chan->incomingbufs[ FRAG_FILE_STREAM ] ) 
		{
			// Copy it in
			cursize -= MSG_GetReadBuf()->GetNumBytesRead();
			Q_memcpy( &buffer[ pos ], &p->frag_message.GetData()[ MSG_GetReadBuf()->GetNumBytesRead() ], cursize );
		}
		else
		{
			Q_memcpy( &buffer[ pos ], p->frag_message.GetData(), cursize);
		}

		pos += cursize;

		delete p;
		p = n;
	}

	COM_WriteFile ( filename, buffer, pos );
	delete[] buffer;

	// clear remnants
	SZ_Clear( &net_message );
	MSG_GetReadBuf()->Reset();
	
	chan->incomingbufs[ FRAG_FILE_STREAM ] = NULL;

	// Reset flag
	chan->incomingready[ FRAG_FILE_STREAM ] = false;

	return true;
}

/*
==============================
Netchan_IsSending

==============================
*/
qboolean Netchan_IsSending( netchan_t *chan )
{
	int i;
	for ( i = 0; i < MAX_STREAMS; i++ )
	{
		if ( chan->fragbufs[ i ] )
		{
			return true;
		}
	}
	return false;
}

/*
==============================
Netchan_IsReceiving

==============================
*/
qboolean Netchan_IsReceiving( netchan_t *chan )
{
	int i;
	for ( i = 0; i < MAX_STREAMS; i++ )
	{
		if ( chan->incomingbufs[ i ] )
		{
			return true;
		}
	}
	return false;
}

/*
==============================
Netchan_IncomingReady

==============================
*/
qboolean Netchan_IncomingReady( netchan_t *chan )
{
	int i;
	for ( i = 0; i < MAX_STREAMS; i++ )
	{
		if ( chan->incomingready[ i ] )
		{
			return true;
		}
	}
	return false;
}
char gDownloadFile[ 256 ];

/*
==============================
Netchan_UpdateProgress

==============================
*/
void Netchan_UpdateProgress( netchan_t *chan )
{
	fragbuf_t *p;
	int c = 0;
	int total = 0;
	int i;
	float bestpercent = 0.0;

#ifndef SWDS
	// Assume the worst
	if ( scr_downloading.GetInt() != -1 )
	{
		scr_downloading.SetValue( -1 );
		gDownloadFile[ 0 ] = '\0';
	}
#endif

	if ( net_drawslider.GetInt() != 1 )
	{
		// Do show slider for file downloads.
		if ( !chan->incomingbufs[ FRAG_FILE_STREAM ] )
		{
			return;
		}
	}

	for ( i = MAX_STREAMS - 1; i >= 0; i-- )
	{
		// Receiving data
		if ( chan->incomingbufs[ i ] )
		{
			// 
			p = chan->incomingbufs[ i ];

			total = FRAG_GETCOUNT( p->bufferid );
			while ( p )
			{
				c++;
				p = p->next;
			}

			if ( total )
			{
				float percent;

				percent = 100.0 * ( float ) c / (float) total;

				if ( percent > bestpercent )
				{
					bestpercent = percent;
				}
			}

			p = chan->incomingbufs[ i ];
			if ( i == FRAG_FILE_STREAM ) 
			{
				char sz[ 1024 ];
				char *in, *out;
				int len = 0;

				in = (char *)p->frag_message.GetData();
				out = sz;
				while ( *in )
				{
					*out++ = *in++;
					len++;
					if ( len > 128 )
						break;
				}
				*out = '\0';
			}
		}
		// Sending data
		else if ( chan->fragbufs[ i ] )
		{
			if ( chan->fragbufcount[ i ] )
			{
				float percent;
				
				percent = 100.0 * (float)chan->fragbufs[ i ]->bufferid / (float)chan->fragbufcount[ i ];

				if ( percent > bestpercent )
				{
					bestpercent = percent;
				}
			}
		}

	}
#ifndef SWDS
	scr_downloading.SetValue( bestpercent );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Netchan_Init (void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Netchan_Shutdown( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *chan - 
//-----------------------------------------------------------------------------
void Netchan_ReportFlow( netchan_t *chan )
{
	Assert( chan );

	char incoming[ 64 ];
	char outgoing[ 64 ];

	Q_strcpy( incoming, Q_pretifymem( (float)chan->flow[ FLOW_INCOMING ].totalbytes, 3 ) );
	Q_strcpy( outgoing, Q_pretifymem( (float)chan->flow[ FLOW_OUTGOING ].totalbytes, 3 ) );

	Con_DPrintf( "Signon network traffic:  %s from server, %s to server\n",
		incoming, outgoing );
}
