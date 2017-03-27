//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Core implementation of vgui
//
// $NoKeywords: $
//=============================================================================

#include "vgui_internal.h"

#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include "FileSystem.h"
#include <vstdlib/IKeyValuesSystem.h>

#include <stdio.h>

namespace vgui
{

ISurface *g_pSurface = NULL;
ISurface *surface()
{
	return g_pSurface;
}

IFileSystem *g_pFileSystem = NULL;
IFileSystem *filesystem()
{
	return g_pFileSystem;
}

/*IKeyValues *g_pKeyValues = NULL;
IKeyValues *keyvalues()
{
	return g_pKeyValues;
}*/

ILocalize *g_pLocalize = NULL;
ILocalize *localize()
{
	return g_pLocalize;
}

IPanel *g_pIPanel = NULL;
IPanel *ipanel()
{
	return g_pIPanel;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void *InitializeInterface( char const *interfaceName, CreateInterfaceFn *factoryList, int numFactories )
{
	void *retval;

	for ( int i = 0; i < numFactories; i++ )
	{
		CreateInterfaceFn factory = factoryList[ i ];
		if ( !factory )
			continue;

		retval = factory( interfaceName, NULL );
		if ( retval )
			return retval;
	}

	// No provider for requested interface!!!
	// assert( !"No provider for requested interface!!!" );

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool VGui_InternalLoadInterfaces( CreateInterfaceFn *factoryList, int numFactories )
{
	// loads all the interfaces
	g_pSurface = (ISurface *)InitializeInterface(VGUI_SURFACE_INTERFACE_VERSION, factoryList, numFactories );
	g_pFileSystem = (IFileSystem *)InitializeInterface(FILESYSTEM_INTERFACE_VERSION, factoryList, numFactories );
//	g_pKeyValues = (IKeyValues *)InitializeInterface(KEYVALUES_INTERFACE_VERSION, factoryList, numFactories );
	g_pLocalize = (ILocalize *)InitializeInterface(VGUI_LOCALIZE_INTERFACE_VERSION, factoryList, numFactories );
	g_pIPanel = (IPanel *)InitializeInterface(VGUI_PANEL_INTERFACE_VERSION, factoryList, numFactories );

	if (g_pSurface && g_pFileSystem && /*g_pKeyValues &&*/ g_pLocalize && g_pIPanel)
		return true;

	return false;
}

} // namespace vgui
