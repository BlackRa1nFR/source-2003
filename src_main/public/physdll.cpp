#include <stdio.h>
#include "physdll.h"

static CSysModule *pPhysicsModule = NULL;
CreateInterfaceFn GetPhysicsFactory( void )
{
	if ( !pPhysicsModule )
	{
		pPhysicsModule = Sys_LoadModule( "VPHYSICS.DLL" );
		if ( !pPhysicsModule )
			return NULL;
	}

	return Sys_GetFactory( pPhysicsModule );
}

void PhysicsDLLPath( const char *pPathname )
{
	if ( !pPhysicsModule )
	{
		pPhysicsModule = Sys_LoadModule( pPathname );
	}
}
