#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif	// _WIN32
#include "mathlib.h"
#include "utlvector.h"
#include "vphysics_internal.h"

#ifdef _WIN32
HMODULE	gPhysicsDLLHandle;

#pragma warning (disable:4100)

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
 	if ( fdwReason == DLL_PROCESS_ATTACH )
    {
		MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false );
		// store out module handle
		gPhysicsDLLHandle = (HMODULE)hinstDLL;
	}
	else if ( fdwReason == DLL_PROCESS_DETACH )
    {
    }
	return TRUE;
}

#endif		// _WIN32


#include "interface.h"
#include "vphysics_interface.h"

extern IPhysicsEnvironment *CreatePhysicsEnvironment( void );

class CPhysicsInterface : public IPhysics
{
public:
	virtual	IPhysicsEnvironment *CreateEnvironment( void )
	{
		IPhysicsEnvironment *pEnvironment = CreatePhysicsEnvironment();
		m_envList.AddToTail( pEnvironment );
		return pEnvironment;
	}

	void DestroyEnvironment( IPhysicsEnvironment *pEnvironment )
	{
		m_envList.FindAndRemove( pEnvironment );
		delete pEnvironment;
	}

	virtual IPhysicsEnvironment		*GetActiveEnvironmentByIndex( int index )
	{
		if ( index < 0 || index >= m_envList.Count() )
			return NULL;

		return m_envList[index];
	}

	CUtlVector< IPhysicsEnvironment *>	m_envList;
};


static CPhysicsInterface g_MainDLLInterface;
IPhysics *g_PhysicsInternal = &g_MainDLLInterface;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CPhysicsInterface, IPhysics, VPHYSICS_INTERFACE_VERSION, g_MainDLLInterface );
