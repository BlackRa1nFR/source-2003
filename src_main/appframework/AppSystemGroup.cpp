//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Defines a group of app systems that all have the same lifetime
// that need to be connected/initialized, etc. in a well-defined order
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#include "AppSystemGroup.h"
#include "appframework/IAppSystem.h"
#include "interface.h"


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CAppSystemGroup::CAppSystemGroup() : m_SystemDict(false, 0, 16)
{
}


//-----------------------------------------------------------------------------
// Methods to load + unload DLLs
//-----------------------------------------------------------------------------
AppModule_t CAppSystemGroup::LoadModule( const char *pDLLName )
{
	CSysModule *pSysModule = Sys_LoadModule( pDLLName );
	if (!pSysModule)
	{
		Warning("AppFramework : Unable to load module %s!\n", pDLLName );
		return -1;
	}

	return m_Modules.AddToTail( pSysModule );
}

void CAppSystemGroup::UnloadAllModules( )
{
	// NOTE: Iterate in reverse order so they are unloaded in opposite order
	// from loading
	for (int i = m_Modules.Count(); --i >= 0; )
	{
		Sys_UnloadModule( m_Modules[i] );
	}
	m_Modules.RemoveAll();
}


//-----------------------------------------------------------------------------
// Methods to add/remove various global singleton systems 
//-----------------------------------------------------------------------------
IAppSystem *CAppSystemGroup::AddSystem( AppModule_t module, const char *pInterfaceName )
{
	if (module == APP_MODULE_INVALID)
		return NULL;

	Assert( (module >= 0) && (module < m_Modules.Count()) );
	CreateInterfaceFn pFactory = Sys_GetFactory( m_Modules[module] );

	int retval;
	void *pSystem = pFactory( pInterfaceName, &retval	);
	if ((retval != IFACE_OK) || (!pSystem))
	{
		Warning("AppFramework : Unable to create system %s!\n", pInterfaceName );
		return NULL;
	}

	IAppSystem *pAppSystem = static_cast<IAppSystem*>(pSystem);
	int sysIndex = m_Systems.AddToTail( pAppSystem );

	// Inserting into the dict will help us do named lookup later
	m_SystemDict.Insert( pInterfaceName, sysIndex );
	return pAppSystem;
}

void CAppSystemGroup::RemoveAllSystems()
{
	// NOTE: There's no deallcation here since we don't really know
	// how the allocation has happened. We could add a deallocation method
	// to the code in interface.h; although when the modules are unloaded
	// the deallocation will happen anyways
	m_Systems.RemoveAll();
	m_SystemDict.RemoveAll();
}


//-----------------------------------------------------------------------------
// Methods to find various global singleton systems 
//-----------------------------------------------------------------------------
void *CAppSystemGroup::FindSystem( const char *pSystemName )
{
	unsigned short i = m_SystemDict.Find( pSystemName );
	if (i != m_SystemDict.InvalidIndex())
		return m_Systems[m_SystemDict[i]];

	// If it's not an interface we know about, it could be an older
	// version of an interface, or maybe something implemented by
	// one of the instantiated interfaces...

	// QUESTION: What order should we iterate this in?
	// It controls who wins if multiple ones implement the same interface
 	for ( i = 0; i < m_Systems.Count(); ++i )
	{
		void *pInterface = m_Systems[i]->QueryInterface( pSystemName );
		if (pInterface)
			return pInterface;
	}

	// No dice..
	return NULL;
}


//-----------------------------------------------------------------------------
// Gets at a factory that works just like FindSystem
//-----------------------------------------------------------------------------
// This function is used to make this system appear to the outside world to
// function exactly like the currently existing factory system
CAppSystemGroup *s_pConnectingAppSystem;
void *AppSystemCreateInterfaceFn(const char *pName, int *pReturnCode)
{
	void *pInterface = s_pConnectingAppSystem->FindSystem( pName );
	if ( pReturnCode )
	{
		*pReturnCode = pInterface ? IFACE_OK : IFACE_FAILED;
	}
	return pInterface;
}

CreateInterfaceFn CAppSystemGroup::GetFactory()
{
	s_pConnectingAppSystem = this;
	return AppSystemCreateInterfaceFn;
}


//-----------------------------------------------------------------------------
// Method to connect/disconnect all systems
//-----------------------------------------------------------------------------

bool CAppSystemGroup::ConnectSystems()
{
	for (int i = 0; i < m_Systems.Count(); ++i )
	{
		IAppSystem *sys = m_Systems[i];

		if (!sys->Connect( GetFactory() ))
			return false;
	}
	return true;
}

void CAppSystemGroup::DisconnectSystems()
{
	// Disconnect in reverse order of connection
	for (int i = m_Systems.Count(); --i >= 0; )
	{
		m_Systems[i]->Disconnect( );
	}
}


//-----------------------------------------------------------------------------
// Method to initialize/shutdown all systems
//-----------------------------------------------------------------------------
bool CAppSystemGroup::InitSystems()
{
	for (int i = 0; i < m_Systems.Count(); ++i )
	{
		if (!m_Systems[i]->Init( ))
			return false;
	}
	return true;
}

void CAppSystemGroup::ShutdownSystems()
{
	// Shutdown in reverse order of initialization
	for (int i = m_Systems.Count(); --i >= 0; )
	{
		m_Systems[i]->Shutdown( );
	}
}


