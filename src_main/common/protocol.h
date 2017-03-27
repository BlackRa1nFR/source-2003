// protocol.h -- communications protocols
#ifndef PROTOCOL_H
#define PROTOCOL_H

#ifdef _WIN32
#pragma once
#endif

#ifndef LAUNCHERONLY
#ifndef NET_H
#include "net.h"
#endif
#endif

#include "packed_entity.h"

// Used to classify entity update types in DeltaPacketEntities.
enum UpdateType
{
	EnterPVS = 0,	// Entity came back into pvs, create new entity if one doesn't exist

	LeavePVS,		// Entity left pvs

	DeltaEnt,		// There is a delta for this entity.
	PreserveEnt		// Entity stays alive but no delta ( could be LOD, or just unchanged )
};

// Flags for delta encoding header
enum
{
	FHDR_ZERO			= 0x0000,
	FHDR_LEAVEPVS		= 0x0001,
	FHDR_DELETE			= 0x0002,
	FHDR_ENTERPVS		= 0x0004,
	FHDR_FORCERECREATE	= 0x0008
};



#define INSTANCE_BASELINE_TABLENAME	"InstanceBaseline"


#define CURRENT_PROTOCOL    1


#define DELTA_OFFSET_BITS 5
#define DELTA_OFFSET_MAX ( ( 1 << DELTA_OFFSET_BITS ) - 1 )


#define DELTASIZE_BITS		21


#define DELTAFRAME_NUMBITS	12
#define DELTAFRAME_MASK		((1 << DELTAFRAME_NUMBITS)-1)

// Max number of history commands to send ( 2 by default ) in case of dropped packets
#define NUM_BACKUP_COMMAND_BITS		3
#define MAX_BACKUP_COMMANDS			(1 << NUM_BACKUP_COMMAND_BITS)

//
#define	PORT_MASTER	27010       // Default master port
#define PORT_MODMASTER 27011    // Default mod-master port
#define PORT_CLIENT "27005"     // Must use atoi to convert to integer
#define PORT_SERVER "27015"     //  "

// This is used, unless overridden in the registry
#define VALVE_MASTER_ADDRESS "half-life.east.won.net:27010"

#define PROTOCOL_AUTHCERTIFICATE 0x01   // Connection from client is using a WON authenticated certificate
#define PROTOCOL_HASHEDCDKEY     0x02    // Connection from client is using hashed CD key because WON comm. channel was unreachable
#define PROTOCOL_LASTVALID       0x02    // Last valid protocol

#define SIGNED_GUID_LEN 32 // Hashed CD Key (32 hex alphabetic chars + 0 terminator )

//
// server to client
//
#define	svc_bad				0
#define	svc_nop				1
#define	svc_disconnect		2
#define	svc_event			3	// 
// #define	svc_version			4	// [long] server version
#define	svc_setview			5	// [short] entity number
#define	svc_sound			6	// <see code>
#define	svc_time			7	// [float] server time
#define	svc_print			8	// [string] null terminated string
#define	svc_stufftext		9	// [string] stuffed into client's console buffer
								// the string should be \n terminated
#define	svc_setangle		10	// [angle3] set the view angle to this absolute value
#define	svc_serverinfo		11	// [long] version
						// [string] signon string
						// [string]..[0]model cache
						// [string]...[0]sounds cache
#define	svc_lightstyle		12	// [byte] [string]
#define	svc_updateuserinfo	13		// [byte] slot [long] uid [string] userinfo
#define	svc_event_reliable	14
#define	svc_clientdata		15	// <shortbits + data>
#define	svc_stopsound		16	// <see code>
#define	svc_createstringtables	17	// custom
#define	svc_updatestringtable	18	// custom
// Message from server side to client side entity
#define svc_entitymessage	19
#define	svc_spawnbaseline	20
#define	svc_bspdecal		21
#define	svc_setpause		22	// [byte] on / off
#define	svc_signonnum		23	// [byte]  used for the signon sequence
#define	svc_centerprint		24	// [string] to put in center of the screen
#define	svc_spawnstaticsound	25	// [coord3] [byte] samp [byte] vol [byte] aten
#define	svc_gameevent		26		// [keyValues] used by global game event system
#define	svc_finale			27		// [string] music [string] text
#define	svc_cdtrack			28		// [byte] track [byte] looptrack
#define svc_restore			29
#define svc_cutscene		30
// #define svc_decalname		31		// [byte] index [string] name
#define	svc_roomtype		32		// [byte] roomtype (dsp effect)
#define	svc_addangle		33		// [angle3] set the view angle to this absolute value
#define svc_usermessage		34		
#define	svc_packetentities	35		    // [...]  Non-delta compressed entities
#define	svc_deltapacketentities	36		// [...]  Delta compressed entities
#define	svc_choke   		37		// # of packets held back on channel because too much data was flowing.
#define svc_setconvar		38      // #count + var/value string pairs.  For replicating FCVAR_REPLICATED convar values
#define svc_sendlogo		39      // Server wants client to upload a logo file
#define	svc_crosshairangle	40		// [char] pitch * 5 [char] yaw * 5
#define svc_soundfade       41      // char percent, char holdtime, char fadeouttime, char fadeintime
#define svc_skippedupdate   42      // Client sent usercmds faster than server frame rate, don't show a false dropped packet.
#define svc_voicedata		43		// Voicestream data from the server
#define svc_sendtable		44		// A SendTable.
#define svc_classinfo		45		// Info about classes (first byte is a CLASSINFO_ define).
#define svc_debugentityoverlay	46		// A debug text overlay to be displayed
#define svc_debugboxoverlay		47		// A debug box overlay to be displayed
#define svc_debuglineoverlay	48		// A debug box overlay to be displayed
#define svc_debugtextoverlay	49		// A debug box overlay to be displayed
#define svc_debuggridoverlay	50		// A grid overlay to be displayed
#define svc_voiceinit			51		// Initialize voice stuff.
#define svc_debugscreentext		52		// Debug screen test to be displayed
#define svc_debugtriangleoverlay	53	// A debug triangle to be displayed
#define SVC_LASTMSG				53


// For svc_classinfo
#define CLASSINFO_NUMCLASSES	0	// # of classes
#define CLASSINFO_CLASSDATA		1	// data about a class
#define CLASSINFO_ENDCLASSES	2	// signals end of classes (ie: the client can precompute its decoders now)

#define CLASSINFO_CREATEFROMSENDTABLES 3 // Signals that we should read send table and class infos directly from game .dll on client

//
// client to server
//
#define	clc_bad			0
#define	clc_nop 		1
#define	clc_sendlogo	2		// Client is requesting logo download
#define	clc_move		3		// [CUserCmd]
#define	clc_stringcmd	4		// [string] message
#define clc_delta       5       // [byte] request for delta compressed entity packet, 
								//  delta is from last incoming sequence [byte].
#define clc_voicedata   6       // Voicestream data from a client

extern int SV_UPDATE_BACKUP;	// Copies of entity_state_t to keep buffered
extern int SV_UPDATE_MASK;	// must be power of two.  Increasing this number costs a lot of RAM.
extern int CL_UPDATE_BACKUP;
extern int CL_UPDATE_MASK;

#define RES_FATALIFMISSING	(1<<0)   // Disconnect if we can't get this file.
#define RES_PRELOAD			(1<<1)  // Load on client rather than just reserving name

#endif // PROTOCOL_H