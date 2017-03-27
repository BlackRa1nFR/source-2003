// net.h -- Half-Life's interface to the networking layer
// For banning IP addresses (or allowing private games)
#ifndef NET_H
#define NET_H
#ifdef _WIN32
#pragma once
#endif

#include "common.h"
#include "bitbuf.h"

// 0 == regular, 1 == file stream
#define MAX_STREAMS			2    

// Flow control bytes per second limits
#define MAX_RATE		20000				
#define MIN_RATE		1000

// Default data rate
#define DEFAULT_RATE	(9999.0f)

// NETWORKING INFO

// This is the packet payload without any header bytes (which are attached for actual sending)
#define	NET_MAX_PAYLOAD	80000

// This is the payload plus any header info (excluding UDP header)

// Packet header is:
//  4 bytes of outgoing seq
//  4 bytes of incoming seq
//  and for each stream
// {
//  byte (on/off)
//  int (fragment id)
//  short (startpos)
//  short (length)
// }
#define HEADER_BYTES ( 8 + MAX_STREAMS * 9 )

// Pad this to next higher 16 byte boundary
// This is the largest packet that can come in/out over the wire, before processing the header
//  bytes will be stripped by the networking channel layer
#define	NET_MAX_MESSAGE	PAD_NUMBER( ( NET_MAX_PAYLOAD + HEADER_BYTES ), 16 )

typedef enum
{
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

#ifndef NETADR_H
#include "netadr.h"
#endif

extern	netadr_t	net_local_adr;
extern	netadr_t	net_from;		// address of who sent the packet
extern	sizebuf_t	net_message;

extern	qboolean	noip;

// Start up networking
void		NET_Init( void );
// Shut down networking
void		NET_Shutdown (void);
// Retrieve packets from network layer
qboolean	NET_GetPacket (netsrc_t nSock);
// Send packet over network layer
void		NET_SendPacket (netsrc_t nSock, int length, void *data, netadr_t to);
// Start up/shut down sockets layer
void		NET_Config (qboolean multiplayer);
// Check state
int			NET_IsConfigured( void );
// Compare addresses
qboolean	NET_CompareAdr (netadr_t a, netadr_t b);
qboolean	NET_CompareClassBAdr (netadr_t a, netadr_t b);
qboolean	NET_IsReservedAdr (netadr_t a);
qboolean	NET_IsLocalAddress (netadr_t adr);
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b);
// Address conversion
const char	*NET_AdrToString (netadr_t a);
const char	*NET_BaseAdrToString (netadr_t a);
qboolean	NET_StringToAdr ( const char *s, netadr_t *a);
// Convert from host to network byte ordering
unsigned short NET_HostToNetShort( unsigned short us_in );
// and vice versa
unsigned short NET_NetToHostShort( unsigned short us_in );
// Clear fakelag data
void NET_ClearLagData( qboolean bClient, qboolean bServer );
//============================================================================

#define MAX_FLOWS 2

#define FLOW_OUTGOING 0
#define FLOW_INCOMING 1

// Message data
typedef struct
{
	// Size of message sent/received
	int		size;
	// Time that message was sent/received
	double	time;
} flowstats_t;

#define	MAX_LATENT	32

typedef struct
{
	// Data for last MAX_LATENT messages
	flowstats_t stats[ MAX_LATENT ];
	// Current message position
	int			current;
	// Time when we should recompute k/sec data
	double		nextcompute; 
	// Average data
	float		kbytespersec;
	float		avgkbytespersec;
	int			totalbytes;
} flow_t;

// Size of fragmentation buffer internal buffers
#define FRAGMENT_SIZE 1400

#define	FRAG_NORMAL_STREAM	0
#define FRAG_FILE_STREAM	1

// Generic fragment structure
typedef struct fragbuf_s
{
	// Next buffer in chain
	struct fragbuf_s	*next;
	// Id of this buffer
	int					bufferid;
	// Message buffer where raw data is stored
	bf_write			frag_message;
	// The actual data sits here
	byte				frag_message_buf[ FRAGMENT_SIZE ];
	// Is this a file buffer?
	qboolean			isfile;
	// Is this file buffer from memory ( custom decal, etc. ).
	qboolean			isbuffer;
	// Name of the file to save out on remote host
	char				filename[ MAX_OSPATH ];
	// Offset in file from which to read data
	int					foffset;  
	// Size of data to read at that offset
	int					size;
} fragbuf_t;

// Waiting list of fragbuf chains
typedef struct fragbufwaiting_s
{
	// Next chain in waiting list
	struct fragbufwaiting_s		*next;
	// Number of buffers in this chain
	int							fragbufcount;
	// The actual buffers
	fragbuf_t					*fragbufs;
} fragbufwaiting_t;

// Network Connection Channel
typedef struct
{
	// NS_SERVER or NS_CLIENT, depending on channel.
	netsrc_t    sock;               

	// Address this channel is talking to.
	netadr_t	remote_address;  
	
	// For timeouts.  Time last message was received.
	float		last_received;		
	// Time when channel was connected.
	float       connect_time;       

	// Bandwidth choke
	// Bytes per second
	double		rate;				
	// If realtime > cleartime, free to send next packet
	double		cleartime;			

	// Sequencing variables
	//
	// Increasing count of sequence numbers 
	int			incoming_sequence;              
	// # of last outgoing message that has been ack'd.
	int			incoming_acknowledged;          
	// Toggles T/F as reliable messages are received.
	int			incoming_reliable_acknowledged;	
	// single bit, maintained local
	int			incoming_reliable_sequence;	    
	// Message we are sending to remote
	int			outgoing_sequence;              
	// Whether the message contains reliable payload, single bit
	int			reliable_sequence;			    
	// Outgoing sequence number of last send that had reliable data
	int			last_reliable_sequence;		    

	// Staging and holding areas
	bf_write	message;
	byte		message_buf[NET_MAX_PAYLOAD];

	// Reliable message buffer.  We keep adding to it until reliable is acknowledged.  Then we clear it.
	int			reliable_length;
	byte		reliable_buf[NET_MAX_PAYLOAD];	// unacked reliable message

	// Waiting list of buffered fragments to go onto queue.
	// Multiple outgoing buffers can be queued in succession
	fragbufwaiting_t *waitlist[ MAX_STREAMS ]; 

	// Is reliable waiting buf a fragment?
	int				reliable_fragment[ MAX_STREAMS ];          
	// Buffer id for each waiting fragment
	unsigned int	reliable_fragid[ MAX_STREAMS ];

	// The current fragment being set
	fragbuf_t	*fragbufs[ MAX_STREAMS ];
	// The total number of fragments in this stream
	int			fragbufcount[ MAX_STREAMS ];

	// Position in outgoing buffer where frag data starts
	short		frag_startpos[ MAX_STREAMS ];
	// Length of frag data in the buffer
	short		frag_length[ MAX_STREAMS ];

	// Incoming fragments are stored here
	fragbuf_t	*incomingbufs[ MAX_STREAMS ];
	// Set to true when incoming data is ready
	qboolean	incomingready[ MAX_STREAMS ];

	// Only referenced by the FRAG_FILE_STREAM component
	// Name of file being downloaded
	char		incomingfilename[ MAX_OSPATH ];

	// Incoming and outgoing flow metrics
	flow_t flow[ MAX_FLOWS ];  
} netchan_t;

// packets dropped before this one
extern	int	net_drop;		

// Initialize subsystem
void	Netchan_Init( void );
void	Netchan_Shutdown( void );

// Clear out all channel data
void	Netchan_Clear( netchan_t *chan );
// Set up the network channel
void	Netchan_Setup (netsrc_t nSock, netchan_t *chan, netadr_t adr );
// Transmit reliable payload and unreliable data, if there's room
void	Netchan_Transmit (netchan_t *chan, int lengthInBytes, byte *data);
void	Netchan_TransmitBits (netchan_t *chan, int lengthInBits, byte *data);
// Send unsequenced messages
void	Netchan_OutOfBand (netsrc_t sock, netadr_t adr, int length, byte *data);
void	Netchan_OutOfBandPrint (netsrc_t sock, netadr_t adr, char *format, ...);
// Take buffer/file/filebuffer and create fragments for it
void	Netchan_CreateFragments( qboolean server, netchan_t *chan, bf_write *msg );
int		Netchan_CreateFileFragments( qboolean server, netchan_t *chan, char *filename );
void	Netchan_CreateFileFragmentsFromBuffer ( qboolean server, netchan_t *chan, char *filename, unsigned char *pbuf, int size );
// Force fragments into ready queue
void	Netchan_FragSend( netchan_t *chan );
// Update download/upload slider
void	Netchan_UpdateProgress( netchan_t *chan );
void	Netchan_ReportFlow( netchan_t *chan );

// Is bandwidth choke active?
qboolean Netchan_CanPacket (netchan_t *chan);
// Is incoming packet okay ( out of order or duplicates are discarded ).
qboolean Netchan_Process (netchan_t *chan);
// Recreate buffer from incoming data fragments
qboolean Netchan_CopyNormalFragments( netchan_t *chan );
// Recreate file from incoming data fragments
qboolean Netchan_CopyFileFragments( netchan_t *chan );
// Is data being sent on this channel
qboolean Netchan_IsSending( netchan_t *chan );
// Is data being received on this channel
qboolean Netchan_IsReceiving( netchan_t *chan );
// Is data ready
qboolean Netchan_IncomingReady( netchan_t *chan );
#endif // !NET_H