#if !defined( PROTO_OOB_H )
#define PROTO_OOB_H
#ifdef _WIN32
#pragma once
#endif

#include "proto_version.h"

// This is used, unless overridden in the registry
#define VALVE_MASTER_ADDRESS "half-life.east.won.net:27010"

#define	PORT_MASTER	27010       // Default master port
#define PORT_MODMASTER 27011    // Default mod-master port

#define HB_TIMEOUT 15

// out of band message id bytes

// M = master, S = server, C = client, A = any
// the second character will allways be \n if the message isn't a single
// byte long (?? not true anymore?)

// Server is sending heartbeat to Server Master
#define	S2M_HEARTBEAT			'a'	// + challeange + sequence + active + #channels + channels
// Server is ending current game
#define	S2M_SHUTDOWN			'b' // no params

// Requesting for full server list from Server Master
#define	A2M_GET_SERVERS			'c'	// no params

// Master response with full server list
#define	M2A_SERVERS				'd'	// + 6 byte IP/Port list.

// Request for full server list from Server Master done in batches
#define A2M_GET_SERVERS_BATCH	'e' // + in532 uniqueID ( -1 for first batch )

// Master response with server list for channel
#define M2A_SERVER_BATCH		'f' // + int32 next uniqueID( -1 for last batch ) + 6 byte IP/Port list.

// Request for MOTD from Server Master  (Message of the Day)
#define	A2M_GET_MOTD			'g'	// no params

// MOTD response Server Master
#define	M2A_MOTD				'h'	// + string 

// Generic Ping Request
#define	A2A_PING				'i'	// respond with an A2A_ACK

// Generic Ack
#define	A2A_ACK					'j'	// general acknowledgement without info

// Generic Nack with comment
#define	A2A_NACK				'k'	// [+ comment] general failure

// Print to client console.
#define	A2C_PRINT				'l'	// print a message on client

// Client is sending it's CD Key to Client Master
#define S2A_INFO_DETAILED		'm'	// New Query protocol, returns dedicated or not, + other performance info.

// Batches for MODS
#define A2M_GETMODLIST			'n'
#define M2A_MODLIST				'o'
#define A2M_SELECTMOD			'p'	// Unused

// Another user is requesting a challenge value from this machine
#define A2A_GETCHALLENGE		'q'	// Request challenge # from another machine
// Challenge response from master.
#define M2A_CHALLENGE			's'	// + challenge value 

// A user is requesting the list of master servers, auth servers, and titan dir servers from the Client Master server
#define A2M_GETMASTERSERVERS	'v' // + byte (type of request, TYPE_CLIENT_MASTER or TYPE_SERVER_MASTER)

// Master server list response
#define M2A_MASTERSERVERS		'w'	// + byte type + 6 byte IP/Port List

#define A2M_GETACTIVEMODS		'x' // + string Request to master to provide mod statistics ( current usage ).  "1" for first mod.

#define M2A_ACTIVEMODS			'y' // response:  modname\r\nusers\r\nservers

#define M2M_MSG					'z' // Master peering message

// SERVER TO CLIENT/ANY

// Client connection is initiated by requesting a challenge value
//  the server sends this value back
#define S2C_CHALLENGE			'A' // + challenge value

// Server notification to client to commence signon process using challenge value.
#define	S2C_CONNECTION			'B' // no params

// Response to server info requests
#define S2A_INFO				'C' // + Address, hostname, map, gamedir, gamedescription, active players, maxplayers, protocol
#define S2A_PLAYER				'D' // + Playernum, name, frags, /*deaths*/, time on server

// Request for detailed server/rule information.
#define S2A_RULES				'E' // + number of rules + string key and string value pairs

#define M2A_ACTIVEMODS2			'F' // response:  modname\r\key\value\r\end\\

// Proxy sends multicast IP or 0.0.0.0 if he's not broadcasting the game
#define S2C_LISTEN				'G'	// + IP x.x.x.x:port

#define S2A_INFOSTRING			'H' // + key value set
// MASTER TO MASTER

// #define P2P_STATUS				'I'	// Inter-Proxy status message

#define S2M_GETFILE				'J'	// request module from master
#define M2S_SENDFILE			'K'	// send module to server

#define S2C_REDIRECT			'L'	// + IP x.x.x.x:port, redirect client to other server/proxy 

#define	C2M_CHECKMD5			'M'	// player client asks secure master if Module MD5 is valid
#define M2C_ISVALIDMD5			'N'	// secure servers answer to C2M_CHECKMD5

// MASTER TO SERVER
#define M2S_REQUESTRESTART		'O' // HLMaster rejected a server's connection because the server needs to be updated

#define M2A_ACTIVEMODS3			'P' // response:  keyvalues struct of mods
#define A2M_GETACTIVEMODS3		'Q' // get a list of mods and the stats about them

#define S2A_LOGSTRING			'R'	// send a log string
#define S2A_LOGKEY				'S'	// send a log event as key value

#define S2M_HEARTBEAT2			'0' // New style heartbeat

#define A2M_GET_SERVERS_BATCH2	'1' // New style server query

#define A2M_GETACTIVEMODS2		'2' // New style mod info query

#define C2S_AUTHREQUEST1        '3' // 
#define S2C_AUTHCHALLENGE1      '4' //
#define C2S_AUTHCHALLENGE2      '5' //
#define S2C_AUTHCOMPLETE        '6'
#define C2S_AUTHCONNECT         '7'  // Unused, signals that the client has
									 // authenticated the server
#define S2C_BADPASSWORD         '8' // Special protocol for bad passwords.

#define S2C_CONNREJECT			'9'  // Special protocol for rejected connections.

#endif