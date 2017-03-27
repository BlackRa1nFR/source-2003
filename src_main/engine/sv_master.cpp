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
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "server.h"
#include "master.h"
#include "proto_oob.h"
#include "vstdlib/ICommandLine.h"



//-----------------------------------------------------------------------------
// Purpose: List of master servers and some state info about them
//-----------------------------------------------------------------------------
typedef struct adrlist_s
{
	// Next master in chain
	struct adrlist_s	*next;
	// Challenge request sent to master
	qboolean			heartbeatwaiting;     
	// Challenge request send time
	float				heartbeatwaitingtime; 
	// Last one is Main master
	int					heartbeatchallenge;
	// Time we sent last heartbeat
	double				last_heartbeat;      
	// Master server address
	netadr_t			adr;
} adrlist_t;

//-----------------------------------------------------------------------------
// Purpose: Implements the master server interface
//-----------------------------------------------------------------------------
class CMaster : public IMaster
{
public:
	CMaster( void );
	virtual ~CMaster( void );

	// Heartbeat functions.
	void Init( void );
	void Shutdown( void );
	// Sets up master address
	void InitConnection(void);		  
	void ShutdownConnection(void);
	void SendHeartbeat( struct adrlist_s *p );
	void AddServer( struct netadr_s *adr );
	void UseDefault ( void );
	void CheckHeartbeat (void);
	void RespondToHeartbeatChallenge(void);

	void SetMaster_f( void );
	void Heartbeat_f( void );
private:
	// List of known master servers
	adrlist_t *m_pMasterAddresses;
	// If nomaster is true, the server will not send heartbeats to the master server
	bool	m_bNoMasters;
};

static CMaster g_MasterServer;
IMaster *master = (IMaster *)&g_MasterServer;

#define	HEARTBEAT_SECONDS	300
#define MASTER_PARSE_FILE "woncomm.lst"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMaster::CMaster( void )
{
	m_pMasterAddresses	= NULL;
	m_bNoMasters		= false;
}

CMaster::~CMaster( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Sends a heartbeat to the master server
// Input  : *p - 
//-----------------------------------------------------------------------------
void CMaster::SendHeartbeat ( adrlist_t *p )
{
// TODO REENABLE BEFORE SHIPPING
#if 0
	static char	string[2048];    // Buffer for sending heartbeat
	int			active;          // Number of active client connections
	char        szGD[ MAX_OSPATH ];


	if ( gfUseLANAuthentication )  // No HB on lan games
		return;

	if ( !p )
		return;

	// Still waiting on challenge response?
	if ( p->heartbeatwaiting )
		return;

	// Waited too long
	if ( (realtime - p->heartbeatwaitingtime ) >= HB_TIMEOUT )
		return;

	InitConnection();

	//
	// count active users
	//
	SV_CountPlayers( &active );

	memset(string, 0, 2048);
	
	COM_FileBase( com_gamedir, szGD );

	// Send to master
	Q_snprintf (string, sizeof( string ), "%c\n%i\n%i\n%i\n%i\n%s\n", S2M_HEARTBEAT, p->heartbeatchallenge,
		1, active, PROTOCOL_VERSION, szGD );

	NET_SendPacket (NS_SERVER, strlen(string), string,  p->adr );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Requests a challenge so we can then send a heartbeat
//-----------------------------------------------------------------------------
void CMaster::CheckHeartbeat (void)
{
// TODO REENABLE BEFORE SHIPPING
#if 0
	unsigned char c;    // Buffer for sending heartbeat
	adrlist_t *p;

	if ( m_bNoMasters ||      // We are ignoring heartbeats
	    gfUseLANAuthentication || // No HB on lan games
		sv_lan.GetInt() ||           // Lan servers don't heartbeat
		(svs.maxclients <= 1) ||  // not a multiplayer server.
		!sv.active )			  // only heartbeat if a server is running.
		return;

	InitConnection();

	p = m_pMasterAddresses;
	while ( p )
	{
		// Time for another try?
		if ( ( realtime - p->last_heartbeat) < HEARTBEAT_SECONDS)  // not time to send yet
		{
			p = p->next;
			continue;
		}

		// Should we resend challenge request?
		if ( p->heartbeatwaiting && 
			( ( realtime - p->heartbeatwaitingtime ) < HB_TIMEOUT ) )
		{
			p = p->next;
			continue;
		}

		p->heartbeatwaiting     = true;
		p->heartbeatwaitingtime = realtime;
	
		p->last_heartbeat       = realtime;  // Flag at start so we don't just keep trying for hb's when

		c = A2A_GETCHALLENGE;

		// Send to master asking for a challenge #
		NET_SendPacket (NS_SERVER, 1, &c, p->adr);
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Server is shutting down, unload master servers list, tell masters that we are closing the server
//-----------------------------------------------------------------------------
void CMaster::ShutdownConnection( void )
{
	char		string[2048];
	adrlist_t *p;

	if ( !host_initialized )
		return;

	if ( m_bNoMasters ||      // We are ignoring heartbeats
	    gfUseLANAuthentication || // No HB on lan games
		sv_lan.GetInt() ||           // Lan servers don't heartbeat
		(svs.maxclients <= 1) )   // not a multiplayer server.
		return;

	InitConnection();

	Q_snprintf (string, sizeof( string ), "%c\n", S2M_SHUTDOWN);
	
	p = m_pMasterAddresses;
	while ( p )
	{
		NET_SendPacket (NS_SERVER, strlen(string), string, p->adr);
		p = p->next;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add server to the master list
// Input  : *adr - 
//-----------------------------------------------------------------------------
void CMaster::AddServer( netadr_t *adr )
{
	adrlist_t *n;

	InitConnection();

	// See if it's there
	n = m_pMasterAddresses;
	while ( n )
	{
		if ( NET_CompareAdr( n->adr, *adr ) )
			break;
		n = n->next;
	}

	// Found it in the list.
	if ( n )
		return;

	n = ( adrlist_t * ) malloc ( sizeof( adrlist_t ) );
	if ( !n )
		Sys_Error( "Error allocating %i bytes for master address.", sizeof( adrlist_t ) );

	memset( n, 0, sizeof( adrlist_t ) );

	n->adr = *adr;

	// Queue up a full heartbeat to all master servers.
	n->last_heartbeat = -99999;

	// Link it in.
	n->next = m_pMasterAddresses;
	m_pMasterAddresses = n;
}

//-----------------------------------------------------------------------------
// Purpose: Add built-in default master if woncomm.lst doesn't parse
//-----------------------------------------------------------------------------
void CMaster::UseDefault ( void )
{
	char szValveMasterAddress[256];
	netadr_t adr;

	// Get default master
	Q_snprintf( szValveMasterAddress, sizeof( szValveMasterAddress ), VALVE_MASTER_ADDRESS );    // IP:PORT string

	// Convert to netadr_t
	if ( NET_StringToAdr ( szValveMasterAddress, &adr ) )
	{
		// Add to master list
		AddServer( &adr );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the default master server address
//-----------------------------------------------------------------------------
void CMaster::InitConnection( void )
{
	char szValveMasterAddress[256];
	char szWONCommFile[MAX_OSPATH];
	char *pbuffer;
	char *pstart;
	netadr_t adr;
	char szAdr[64];
	int nPort;
	int nCount = 0;
	qboolean bIgnore;
	int nDefaultPort;
	static qboolean bInitialized = false;

	if ( m_bNoMasters ||      // We are ignoring heartbeats
	     gfUseLANAuthentication || // No HB on lan games
		 sv_lan.GetInt() ||           // Lan servers don't heartbeat
		 (svs.maxclients <= 1) )
	{
		return;
	}

	// Already able to initialize at least once?
	if ( bInitialized )
		return;

	// So we don't do this a second time.
	bInitialized = true;

	// Assume default master
	Q_snprintf( szValveMasterAddress, sizeof( szValveMasterAddress ), VALVE_MASTER_ADDRESS);    // IP:PORT string
	Q_snprintf( szWONCommFile, sizeof( szWONCommFile ), "%s", MASTER_PARSE_FILE );

	const char *pComFile = CommandLine()->ParmValue( "-comm" );
	if ( pComFile )
	{
		strcpy ( szWONCommFile, pComFile );
	}

	// Read them in from WONCOMM.LST
	pbuffer = (char*)COM_LoadFile( szWONCommFile, 5, NULL ); // Use malloc
	if ( !pbuffer )
	{
		Con_Printf( "Couldn't load woncomm.lst\nUsing default master\n" );
		UseDefault();
		return;
	}

	pstart = pbuffer;

	while ( 1 )
	{
		pstart = COM_Parse( pstart );

		if ( strlen(com_token) <= 0)
			break;

		bIgnore = true;

		nDefaultPort = PORT_MASTER;
		if ( !stricmp( com_token, "Master" ) )
		{
			bIgnore = FALSE;
		}

		// Now parse all addresses between { }
		pstart = COM_Parse( pstart );
		if ( strlen(com_token) <= 0 )
			break;

		if ( stricmp ( com_token, "{" ) )
			break;

		// Parse addresses until we get to "}"
		while ( 1 )
		{
			char base[256];

			// Now parse all addresses between { }
			pstart = COM_Parse( pstart );
			
			if (strlen(com_token) <= 0)
				break;

			if ( !stricmp ( com_token, "}" ) )
				break;
			
			Q_snprintf( base, sizeof( base ), "%s", com_token );
				
			pstart = COM_Parse( pstart );
			
			if (strlen(com_token) <= 0)
				break;

			if ( stricmp( com_token, ":" ) )
				break;

			pstart = COM_Parse( pstart );
			
			if (strlen(com_token) <= 0)
				break;

			nPort = atoi ( com_token );
			if ( !nPort )
				nPort = nDefaultPort;

			Q_snprintf( szAdr, sizeof( szAdr ), "%s:%i", base, nPort );

			// Can we resolve it any better
			if ( !NET_StringToAdr( szAdr, &adr ) )
				bIgnore = true;

			if ( !bIgnore )
			{
				Con_Printf( "Adding master server %s\n", NET_AdrToString( adr ) );
				AddServer( &adr );
				nCount++;
			}
		}
	}

	if ( !nCount )
	{
		Con_Printf( "No masters parsed from woncomm.lst\nUsing default master\n" );
		UseDefault();
	}

	free( pbuffer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMaster::RespondToHeartbeatChallenge(void)
{
	adrlist_t *p;

	// No masters, just ignore.
	if ( !m_pMasterAddresses )
		return;

	p = m_pMasterAddresses;
	while ( p )
	{
		if ( NET_CompareAdr( p->adr, net_from ) )
			break;

		p = p->next;
	}

	// Not a known master server.
	if ( !p )
		return;

	// Kill timer
	p->heartbeatwaiting   = false;
	p->heartbeatchallenge = MSG_ReadLong();

	// Send the actual heartbeat request to this master server.
	SendHeartbeat( p );
}

//-----------------------------------------------------------------------------
// Purpose: Add/remove master servers
//-----------------------------------------------------------------------------
void CMaster::SetMaster_f (void)
{
	int		i;
	char   *pMasterAddress; // IP:Port string of master
	char   *pMasterPort;    // Port of master
	char    szMaster[128];  // Full string
	int     ipPort;         // Numeric port
	char    *pszCmd;
	char    szMasterPort[20];
	netadr_t adr;
	qboolean bfirst = false;

	ipPort = PORT_MASTER; // default
	Q_snprintf( szMasterPort, sizeof( szMasterPort ), "%i", ipPort );

	master->InitConnection();

	if ( !m_pMasterAddresses )
		bfirst = true;

	i = Cmd_Argc();

	// Usage help
	if ( ( i < 2 ) ||
		 ( i > 4 ) )
	{
		adrlist_t *p;
		Con_Printf("Usage:\nSetmaster <add | remove | enable | disable> <IP:port>\n");

		p = m_pMasterAddresses;
		if ( !p )
			Con_Printf("Current:  None\n");
		else
		{
			int i = 1;
			Con_Printf("Current:\n");
			while ( p )
			{
				Con_Printf("  %i:  %s\n", i++, NET_AdrToString( p->adr ) );
				p = p->next;
			}
		}
		return;
	}

	pszCmd = Cmd_Argv(1);
	if ( !pszCmd || !pszCmd[0] )
		return;

	// Check for disabling...
	if ( !stricmp( pszCmd, "disable") )
	{
		m_bNoMasters = true;
		return;
	}
	else if (!stricmp( pszCmd, "enable") )
	{
		m_bNoMasters = false;
		return;
	}

	if ( stricmp( pszCmd, "add" ) && 
		 stricmp( pszCmd, "remove" ) )
	{
		Con_Printf( "Setmaster:  Unknown command %s\n", pszCmd );
		return;
	}

	// Get address and, if a port is specified get it, too.
	pMasterAddress = Cmd_Argv(2);
	if (i == 4)
	{
		pMasterPort      = Cmd_Argv(3);
		if ( pMasterPort && pMasterPort[0] )
		{
			ipPort = atoi(pMasterPort);
			if (ipPort == 0)   // If any problem, assuem default
				ipPort = PORT_MASTER;
		}
		else
			pMasterPort = szMasterPort;
	}
				 
	// Construct a full string
	Q_snprintf( szMaster, sizeof( szMaster ), "%s:%i", pMasterAddress, ipPort );

	// Convert to a valid address
	if ( !NET_StringToAdr ( szMaster, &adr ) )
	{
		Con_Printf(" Invalid address \"%s\", setmaster command ignored\n", szMaster );
		return;
	}

	if ( !stricmp( pszCmd, "add" ) )
	{
		master->AddServer( &adr );

		// If we get here, masters are definitely being used.
		m_bNoMasters = false;

		Con_Printf ( "Adding master at %s\n", szMaster );
	}
	else
	{
		// Find master server
		adrlist_t *p;

		if ( !m_pMasterAddresses )
		{
			Con_Printf( "Can't remove master, list is empty\n" );
			return;
		}

		p = m_pMasterAddresses;
		while ( p )
		{
			if ( NET_CompareAdr( p->adr, adr ) )
				break;
			p = p->next;
		}

		if ( !p )
		{
			Con_Printf( "Can't remove master %s, not in list\n", szMaster );
			return;
		}

		// p is pointing at the master to remove.
		if ( p == m_pMasterAddresses )
		{
			m_pMasterAddresses = m_pMasterAddresses->next;
			free( p );
		}
		else
		{		
			adrlist_t *pbefore;

			pbefore = m_pMasterAddresses;
			while ( pbefore )
			{
				if ( pbefore->next == p )
					break;
				pbefore = pbefore->next;
			}

			if ( !pbefore ) // !!!
				return;

			// Unlink p
			pbefore->next = p->next;
			free( p );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Send a new heartbeat to the master
//-----------------------------------------------------------------------------
void CMaster::Heartbeat_f (void)
{
	adrlist_t *p;

	p = m_pMasterAddresses;
	while ( p )
	{
		// Queue up a full hearbeat
		p->last_heartbeat = -9999;
		p = p->next;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SetMaster_f( void )
{
	master->SetMaster_f();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Heartbeat_f( void )
{
	master->Heartbeat_f();
}

static ConCommand setmaster("setmaster", SetMaster_f );  
static ConCommand heartbeat("heartbeat", Heartbeat_f ); 

//-----------------------------------------------------------------------------
// Purpose: Adds master server console commands
//-----------------------------------------------------------------------------
void CMaster::Init( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMaster::Shutdown(void)
{
	adrlist_t *p, *n;

	// Free the master list now.
	p = m_pMasterAddresses;
	while ( p )
	{
		n = p->next;
		free( p );
		p = n;
	}

	m_pMasterAddresses = NULL;
}
