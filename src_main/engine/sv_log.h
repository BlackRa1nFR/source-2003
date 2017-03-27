// sv_log.h
// Server logging functions

#ifndef SV_LOG_H
#define SV_LOG_H
#pragma once

#include <igameevents.h>

class CLog : public IGameEventListener
{
public:
	CLog();
	virtual ~CLog();

public: // IGameEventListener Interface
	
	void FireGameEvent( KeyValues * event);
	void GameEventsUpdated();
	bool IsLocalListener() { return true; };

public: 

	void SetLogConsole(bool state);
	void SetLogUDP(bool state);
	void SetLogLevel(int level);
	void SetLogEvents(bool state);
	void SetLogFile(bool state);

	void AddLogAddress(netadr_t addr);

	bool IsActive();	// true if any component (file, console, udp) is logging
	int  GetLogLevel();
	netadr_t GetLogAddress();
	
	void Init( void );
	void Open( void );  // opens all logging streams
	void Close( void );	// closes all logging streams
	void Reset( void );	// reset all logging streams
	void Shutdown( void );
	
	void Printf( const char *fmt, ... );	// log a line to log file
	void Print( const char * text );
	void PrintEvent( KeyValues * event);
	void PrintServerVars( void ); // prints server vars to log file

private:

	bool m_bConsole;	// true if output to console
	bool m_bFile;		// true if output to log file
	bool m_bUDP;		// true if output to remote host
	bool m_bEvents;		// true if logging raw events
	int  m_nLogLevel;	// logging level

	CUtlVector< netadr_t >	m_LogAddresses;       // Server frag log stream is sent to this addres
	FileHandle_t			m_hLogFile;        // File where frag log is put.
};

extern CLog g_Log;

#endif // SV_LOG_H
