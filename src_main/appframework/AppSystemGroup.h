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

#ifndef APPSYSTEMGROUP_H
#define APPSYSTEMGROUP_H

#ifdef _WIN32
#pragma once
#endif

#include "UtlVector.h"
#include "UtlDict.h"
#include "AppFramework/AppFramework.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IAppSystem;
class CSysModule;
class IBaseInterface;


//-----------------------------------------------------------------------------
// This class represents a group of app systems that all have the same lifetime
// that need to be connected/initialized, etc. in a well-defined order
//-----------------------------------------------------------------------------
class CAppSystemGroup : public IAppSystemGroup
{
public:
	// constructor
	CAppSystemGroup();

	// Methods to load + unload DLLs
	virtual AppModule_t LoadModule( const char *pDLLName );
	void UnloadAllModules( );

	// Method to add various global singleton systems 
	virtual IAppSystem *AddSystem( AppModule_t module, const char *pInterfaceName );
	void RemoveAllSystems();

	// Method to connect/disconnect all systems
	bool ConnectSystems();
	void DisconnectSystems();

	// Method to initialize/shutdown all systems
	bool InitSystems();
	void ShutdownSystems();

	// Method to look up a particular named system...
	virtual void *FindSystem( const char *pInterfaceName );

	// Gets at a factory that works just like FindSystem
	virtual CreateInterfaceFn GetFactory();

private:
	CUtlVector<CSysModule*> m_Modules;
	CUtlVector<IAppSystem*> m_Systems;
	CUtlDict<int, unsigned short> m_SystemDict;
};

#endif // APPSYSTEMGROUP_H


