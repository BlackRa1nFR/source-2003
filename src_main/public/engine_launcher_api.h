// engine/launcher interface
#if !defined( ENGINE_LAUNCHER_APIH )
#define ENGINE_LAUNCHER_APIH
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class IEngineAPI
{
// Functions
public:
	virtual bool Run( void *instance, char *basedir, CreateInterfaceFn launcherFactory ) = 0;
};


#define VENGINE_LAUNCHER_API_VERSION "VENGINE_LAUNCHER_API_VERSION001"

#endif // ENGINE_LAUNCHER_APIH