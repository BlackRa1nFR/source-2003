// engine/launcher interface
#if !defined( ENGINE_HLDS_API_H )
#define ENGINE_HLDS_API_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class IDedicatedServerAPI
{
// Functions
public:
	virtual bool		Init( const char *basedir, CreateInterfaceFn launcherFactory ) = 0;
	virtual void		Shutdown( void ) = 0;
	virtual bool		RunFrame( void ) = 0;
	virtual void		AddConsoleText( char *text ) = 0;
	virtual void		UpdateStatus(float *fps, int *nActive, int *nMaxPlayers, char *pszMap) = 0;
};

#define VENGINE_HLDS_API_VERSION "VENGINE_HLDS_API_VERSION001"

#endif // ENGINE_HLDS_API_H