//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <FileSystem.h>

#include <stdio.h>
#include <string.h>

#include <vgui/IInput.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>

#include <vgui_controls/Controls.h>

namespace vgui
{

vgui::IInput *g_pInputInterface = NULL;
vgui::IInput *input()
{
	return g_pInputInterface;
}

vgui::ISchemeManager *g_pSchemeInterface = NULL;
vgui::ISchemeManager *scheme()
{
	return g_pSchemeInterface;
}

vgui::ISurface *g_pSurfaceInterface = NULL;
vgui::ISurface *surface()
{
	return g_pSurfaceInterface;
}

vgui::ISystem *g_pSystemInterface = NULL;
vgui::ISystem *system()
{
	return g_pSystemInterface;
}

vgui::IVGui *g_pVGuiInterface = NULL;
vgui::IVGui *ivgui()
{
	return g_pVGuiInterface;
}

vgui::IPanel *g_pPanelInterface = NULL;
vgui::IPanel *ipanel()
{
	return g_pPanelInterface;
}

IFileSystem *g_pFileSystemInterface = NULL;
IFileSystem *filesystem()
{
	return g_pFileSystemInterface;
}

vgui::ILocalize *g_pLocalizeInterface = NULL;
vgui::ILocalize *localize()
{
	return g_pLocalizeInterface;
}

//-----------------------------------------------------------------------------
// Purpose: finds a particular interface in the factory set
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
	// Assert( !"No provider for requested interface!!!" );

	return NULL;
}

static char g_szControlsModuleName[256];

//-----------------------------------------------------------------------------
// Purpose: Initializes the controls
//-----------------------------------------------------------------------------
bool VGui_InitInterfacesList( const char *moduleName, CreateInterfaceFn *factoryList, int numFactories )
{
	strncpy(g_szControlsModuleName, moduleName, sizeof(g_szControlsModuleName));
	g_szControlsModuleName[sizeof(g_szControlsModuleName) - 1] = 0;

	g_pVGuiInterface = (IVGui *)InitializeInterface( VGUI_IVGUI_INTERFACE_VERSION, factoryList, numFactories );
	g_pPanelInterface = (IPanel *)InitializeInterface( VGUI_PANEL_INTERFACE_VERSION, factoryList, numFactories );
	g_pSurfaceInterface = (ISurface *)InitializeInterface( VGUI_SURFACE_INTERFACE_VERSION, factoryList, numFactories );
	g_pSchemeInterface = (ISchemeManager *)InitializeInterface( VGUI_SCHEME_INTERFACE_VERSION, factoryList, numFactories );
	g_pSystemInterface = (ISystem *)InitializeInterface( VGUI_SYSTEM_INTERFACE_VERSION, factoryList, numFactories );
	g_pInputInterface = (IInput *)InitializeInterface( VGUI_INPUT_INTERFACE_VERSION, factoryList, numFactories );
	g_pLocalizeInterface = (ILocalize *)InitializeInterface( VGUI_LOCALIZE_INTERFACE_VERSION, factoryList, numFactories );
	g_pFileSystemInterface = (IFileSystem *)InitializeInterface( FILESYSTEM_INTERFACE_VERSION, factoryList, numFactories );

	if (!g_pVGuiInterface)
		return false;

	g_pVGuiInterface->Init( factoryList, numFactories );

	if ( g_pSchemeInterface && 
		 g_pSurfaceInterface && 
		 g_pSystemInterface && 
		 g_pInputInterface && 
		 g_pVGuiInterface && 
		 g_pFileSystemInterface && 
		 g_pLocalizeInterface && 
		 g_pPanelInterface)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns the name of the module this has been compiled into
//-----------------------------------------------------------------------------
const char *GetControlsModuleName()
{
	return g_szControlsModuleName;
}



} // namespace vgui



