//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include <time.h>
#include "server.h"
#include "sv_log.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "tier0/vcrmode.h"
#include "sv_main.h"
#include "vstdlib/ICommandLine.h"
#include <proto_oob.h>
#include "gameeventmanager.h"


CLog g_Log;	// global Log object


CON_COMMAND( log_file, "Enables standard log file <0|1>." )
{
	if ( Cmd_Argc() != 2 )
	{
		Msg( "log_udp:  usage\nlog_file <0|1>\n" );
		return;
	}

	g_Log.SetLogFile( atoi(Cmd_Argv(1))!=0 );
}

CON_COMMAND( log_console, "Echos event logging in console <0|1>." )
{
		if ( Cmd_Argc() != 2 )
	{
		Msg( "log_udp:  usage\nlog_console <0|1>\n" );
		return;
	}

	g_Log.SetLogConsole( atoi(Cmd_Argv(1))!=0 );	
}

CON_COMMAND( log_events, "Set UDP logging to remote hoste <0|1>." )
{
		Msg( "log_events:  TODO\n" );
}

CON_COMMAND( log_addaddress, "Set address and port for remote host <ip:port>." )
{
	netadr_t adr;

	if ( Cmd_Argc() != 2 )
	{
		Msg( "log_addaddress:  usage\nlog_addaddress ip:port\n" );
		return;
	}

	char * szAdr = Cmd_Argv(1);

	if ( !szAdr || !szAdr[0] )
	{
		Msg( "log_address:  unparseable address\n" );
		return;
	}

	if ( NET_StringToAdr( szAdr, &adr ) )
	{
		g_Log.AddLogAddress( adr );
		Msg( "log address:  %s\n", NET_AdrToString( adr ) );
	}
	else
	{
		Msg( "log address:  unable to resolve %s\n", szAdr );
		return;
	}
}

CON_COMMAND( log_level, "Specifies a logging level 0..15 <n>." )
{
	if ( Cmd_Argc() != 2 )
	{
		Msg( "log_level:  usage\nlog_level <0-15>\n" );
		return;
	}

	g_Log.SetLogLevel( atoi(Cmd_Argv(1)) );
}

CON_COMMAND( log_udp, "Send log packets to hosts in address list <0|1>." )
{
	if ( Cmd_Argc() != 2 )
	{
		Msg( "log_udp:  usage\nlog_udp <0|1>\n" );
		return;
	}

	g_Log.SetLogUDP( atoi(Cmd_Argv(1))!=0 );
}


CLog::CLog()
{
	Reset();
}

CLog::~CLog()
{

}

void CLog::SetLogConsole(bool state)
{
	m_bConsole = state;
}

void CLog::SetLogLevel(int level)
{
	m_nLogLevel = level;
}

void CLog::SetLogUDP(bool state)
{
	m_bUDP = state;
}

void CLog::SetLogFile(bool state)
{
	m_bFile = state;
}

void CLog::Reset( void )	// reset all logging streams
{
	m_bFile = false;
	m_bConsole = false;
	m_bUDP = false;
	m_bEvents = false;
	m_nLogLevel = 15;
	m_LogAddresses.RemoveAll();
	m_hLogFile = FILESYSTEM_INVALID_HANDLE;
}

void CLog::Init( void )
{
	Reset();
	g_pGameEventManager->AddListener( this );
}

void CLog::Shutdown()
{
	Close();
	Reset();
	g_pGameEventManager->RemoveListener( this );
}

void CLog::AddLogAddress(netadr_t addr)
{
	m_LogAddresses.AddToTail( addr );
}

void CLog::PrintEvent( KeyValues * event)
{
	
}

/*
==================
Log_PrintServerVars

==================
*/
void CLog::PrintServerVars( void )
{
	const ConCommandBase	*var;			// Temporary Pointer to cvars

	if ( !m_bFile )
		return;

	Printf( "server cvars start\n" );
	// Loop through cvars...
	for (var= ConCommandBase::GetCommands() ; var ; var=var->GetNext() )
	{
		if ( var->IsCommand() )
			continue;

		if ( !( var->IsBitSet( FCVAR_SERVER ) ) )
			continue;

		Printf( "\"%s\" = \"%s\"\n", var->GetName(), ((ConVar*)var)->GetString() );
	}

	Printf( "server cvars end\n" );
}

bool CLog::IsActive()
{
	return ( m_bFile || m_bConsole || m_bUDP || m_bEvents );
}

/*
==================
Log_Printf

Prints a frag log message to the server's frag log file, console, and possible a UDP port.
==================
*/
void CLog::Printf( const char *fmt, ... )
{
	va_list			argptr;
	static char		string[1024];
	
	if ( !IsActive() )
		return;

	va_start ( argptr, fmt );
	Q_vsnprintf ( string, sizeof( string ), fmt, argptr );
	va_end   ( argptr );

	Print( string );
}

void CLog::Print( const char * text )
{
	static char	string[1100];

	if ( !IsActive() || !text || !text[0] )
		return;

	if ( Q_strlen( text ) > 1024 )
	{
		DevMsg( 1, "CLog::Print: string too long (>1024 bytes)." );
		return;
	}

	tm today; g_pVCR->Hook_LocalTime( &today );

	Q_snprintf( string, sizeof( string ), "L %02i/%02i/%04i - %02i:%02i:%02i: %s",
		today.tm_mon+1, today.tm_mday, 1900 + today.tm_year,
		today.tm_hour, today.tm_min, today.tm_sec, text );

	// Echo to server console
	if ( m_bConsole ) 
	{
		Con_Printf( string );
	}

	// Echo to log file
	if ( m_bFile && (m_hLogFile != FILESYSTEM_INVALID_HANDLE) )
	{
		g_pFileSystem->FPrintf( m_hLogFile, "%s", string );
		// fflush( svs.mp_logfile );
	}

	// Echo to UPD port
	if ( m_bUDP )
	{
		// out of band sending
		for ( int i = 0; i < m_LogAddresses.Count();i++ )
		{
			Netchan_OutOfBandPrint (NS_SERVER, m_LogAddresses.Element(i), "%c%s", S2A_LOGSTRING, string );
		}
	}
}

void CLog::FireGameEvent( KeyValues * event )
{
	if ( !IsActive() )
		return;

	// log server events

	const char * name = event->GetName();

	if ( !name || !name[0]) 
		return;

	if ( Q_strcmp(name, "server_spawn") == 0 )
	{
		Printf( "server_spawn: \"%s\" \"%s\"\n", event->GetString("game"), event->GetString("mapname")  );
	}
	else if ( Q_strcmp(name, "server_shutdown") == 0 )
	{
		Printf( "server_message: \"%s\"\n", event->GetString("reason") );
	}
	else if ( Q_strcmp(name, "server_cvar") == 0 )
	{
		Printf( "server_cvar: \"%s\" \"%s\"\n", event->GetString("cvarname"), event->GetString("cvarvalue")  );
	}
	else if ( Q_strcmp(name, "server_message") == 0 )
	{
		Printf( "server_message: \"%s\"\n", event->GetString("text") );
	}
	
}

void CLog::GameEventsUpdated()
{
	g_pGameEventManager->AddListener( this );
}

/*
====================
Log_Close

Close logging file.
====================
*/
void CLog::Close( void )
{
	if ( m_bFile &&  (m_hLogFile != FILESYSTEM_INVALID_HANDLE) )
	{
		Printf( "Log closed.\n" );
		g_pFileSystem->Close( m_hLogFile );
	}

	m_hLogFile = FILESYSTEM_INVALID_HANDLE;
}

/*
====================
Log_Open

Open frag logging file.
====================
*/
void CLog::Open( void )
{
	char szFileBase[ MAX_OSPATH ];
	char szTestFile[ MAX_OSPATH ];
	int i;
	FileHandle_t fp = 0;

	if ( !m_bFile )
		return;

	Close();

	// Find a new log file slot
	tm today;
	g_pVCR->Hook_LocalTime( &today );

	Q_snprintf( szFileBase, sizeof( szFileBase ), "logs/L%02i%02i", today.tm_mon + 1, today.tm_mday );
	for ( i = 0; i < 1000; i++ )
	{
		Q_snprintf( szTestFile, sizeof( szTestFile ), "%s%03i.log", szFileBase, i );

		COM_FixSlashes( szTestFile );
		COM_CreatePath( szTestFile );

		fp = g_pFileSystem->Open( szTestFile, "r", "LOGDIR" );
		if ( !fp )
		{
			COM_CreatePath( szTestFile );

			fp = g_pFileSystem->Open( szTestFile, "wt", "LOGDIR" );
			if ( !fp )
			{
				i = 1000;
			}
			else
			{
				Con_Printf( "Server logging data to file %s\n", szTestFile );
			}
			break;
		}
		g_pFileSystem->Close( fp );
	}

	if ( i == 1000 )
	{
		Con_Printf( "Unable to open logfiles under %s\nLogging disabled\n", szFileBase );
		m_bFile = false;
		return;
	}

	if ( fp )
	{
		m_hLogFile = fp;
	}

	Printf( "Log file started.\n" );
}
