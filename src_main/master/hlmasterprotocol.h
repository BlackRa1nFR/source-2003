#ifndef INC_HLMASTERPROTOCOLH
#define INC_HLMASTERPROTOCOLH

#include <stdio.h>
#include <string.h>
#include <winsock.h>
#include <time.h>
#include <errno.h>

/*
Master server protocols
The first byte is the command code.
Followed by a \n, then any other parameters, each followed by a \n.
An empty parameter is acceptable.
*/

#define LAUNCHERONLY
#include "protocol.h"
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#define	MAX_CHALLENGES	(32)  // 4K of challenges should be plenty
#define	SERVER_TIMEOUT	(60*15)   // Server's time out after 15 minutes w/o a heartbeat
#define MIN_SV_TIMEOUT_INTERVAL (60) // Once every minute is fast enough

typedef struct sockaddr_in	netadr_t;
typedef unsigned char byte;

// MAX_CHALLENGES is made large to prevent a denial
//  of service attack that could cycle all of them
//  out before legitimate users connected
//-----------------------------------------------------------------------------
// Purpose: Define master server challenge response
//-----------------------------------------------------------------------------
typedef struct
{
	// Address where challenge value was sent to. 
	netadr_t    adr;       
	// To connect, adr IP address must respond with this #
	int			challenge; 
	// # is valid for only a short duration.
	int			time;      
} challenge_t;

//-----------------------------------------------------------------------------
// Purpose: Represents statistics for a mod
//-----------------------------------------------------------------------------
typedef struct modsv_s
{
	// Next mod in chain
	struct modsv_s	*next;

	// Name of the mod
	char			gamedir[ 64 ];
	
	// Current number of players and servers
	int				ip_players;
	int				ip_servers;
	int				ip_bots;
	int				ip_bots_servers;
	int				lan_players;
	int				lan_servers;
	int				lan_bots;
	int				lan_bots_servers;
	int				proxy_servers;
	int				proxy_players;
} modsv_t;

#define MAX_SINFO 2048

#define VALUE_LENGTH 64
typedef struct string_criteria_s
{
	int		checksum;
	int		length;
	char	value[ VALUE_LENGTH ];
} string_criteria_t;

//-----------------------------------------------------------------------------
// Purpose: Represents a game server
//-----------------------------------------------------------------------------
typedef struct sv_s
{
	// Next server in chain
	struct	sv_s	*next;
	// IP address and port of the server
	netadr_t		address;
	// For master server list, is this an "old" server
	int				isoldserver;     
	// Is a local area network server (not in returned lists to clients)
	int				islan;
	// Time of last heartbeat from server
	int				time;
	// Unique id of the server, for batch requesting servers
	int				uniqueid;

	// Queryable data
	
	// Spectator proxy support
	int				isproxy;
	// Is a proxy target ( count of proxies connect to this regular server )
	int				isProxyTarget;
	// If watching server, this is the address of the server proxy is watching
	string_criteria_t proxyTarget;

	// Current # of players
	int				players;
	// Max # of players
	int				max;
	// # of fake players
	int				bots;
	// Mod this server is playing
	string_criteria_t	gamedir;
	// Map this server is playing
	string_criteria_t	map;
	// OS of the server
	char			os[ 2 ];
	// Is the server running a password protected game
	int				password;
	// Is the server a dedicated server
	int				dedicated;
	// Is the server a secure server using mobile anticheat code
	int				secure;
	// Raw server info from heartbeat packet
	char			info[ MAX_SINFO ];
	int				info_length;
} sv_t;

#endif // INC_HLMASTERPROTOCOLH
