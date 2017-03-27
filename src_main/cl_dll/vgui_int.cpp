//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "itextmessage.h"
#include "vguicenterprint.h"
#include "iloadingdisc.h"
#include "ifpspanel.h"
#include "iconnectmsg.h"
#include "idownloadslider.h"
#include "iprofiling.h"
#include "imessagechars.h"
#include "inetgraphpanel.h"
#include "idebugoverlaypanel.h"
#include <vgui/isurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include "tier0/vprof.h"
#include <vgui_controls/PanelAnimationVar.h>
#include <vgui_controls/Panel.h>
#include <KeyValues.h>

#include "iviewrender.h"

using namespace vgui;

#include <vgui/IInputInternal.h>
vgui::IInputInternal *g_InputInternal = NULL;

#include <vgui_controls/Controls.h>

void GetVGUICursorPos( int& x, int& y )
{
	vgui::input()->GetCursorPos(x, y);
}

void SetVGUICursorPos( int x, int y )
{
	vgui::input()->SetCursorPos(x, y);
}

class CHudTextureHandleProperty : public vgui::IPanelAnimationPropertyConverter
{
public:
	virtual void GetData( Panel *panel, KeyValues *kv, PanelAnimationMapEntry *entry )
	{
		void *data = ( void * )( (*entry->m_pfnLookup)( panel ) );
		CHudTextureHandle *pHandle = ( CHudTextureHandle * )data;

		// lookup texture name for id
		if ( pHandle->Get() )
		{
			kv->SetString( entry->name(), pHandle->Get()->szShortName );
		}
		else
		{
			kv->SetString( entry->name(), "" );
		}
	}
	
	virtual void SetData( Panel *panel, KeyValues *kv, PanelAnimationMapEntry *entry )
	{
		void *data = ( void * )( (*entry->m_pfnLookup)( panel ) );
		
		CHudTextureHandle *pHandle = ( CHudTextureHandle * )data;

		const char *texturename = kv->GetString( entry->name() );
		if ( texturename && texturename[ 0 ] )
		{
			CHudTexture *currentTexture = gHUD.GetIcon( texturename );
			pHandle->Set( currentTexture );
		}
		else
		{
			pHandle->Set( NULL );
		}
	}

	virtual void InitFromDefault( vgui::Panel *panel, PanelAnimationMapEntry *entry )
	{
		void *data = ( void * )( (*entry->m_pfnLookup)( panel ) );

		CHudTextureHandle *pHandle = ( CHudTextureHandle * )data;

		const char *texturename = entry->defaultvalue();
		if ( texturename && texturename[ 0 ] )
		{
			CHudTexture *currentTexture = gHUD.GetIcon( texturename );
			pHandle->Set( currentTexture );
		}
		else
		{
			pHandle->Set( NULL );
		}
	}
};

static CHudTextureHandleProperty textureHandleConverter;

static void VGui_OneTimeInit()
{
	static bool initialized = false;
	if ( initialized )
		return;
	initialized = true;

	vgui::Panel::AddPropertyConverter( "CHudTextureHandle", &textureHandleConverter );

}

bool VGui_Startup( CreateInterfaceFn appSystemFactory )
{
	if ( !vgui::VGui_InitInterfacesList( "CLIENT", &appSystemFactory, 1 ) )
		return false;

	g_InputInternal = (IInputInternal *)appSystemFactory( VGUI_INPUTINTERNAL_INTERFACE_VERSION,  NULL );
	if ( !g_InputInternal )
	{
		return false; // c_vguiscreen.cpp needs this!
	}

	VGui_OneTimeInit();

	// Create any root panels for .dll
	VGUI_CreateClientDLLRootPanel();

	// Make sure we have a panel
	VPANEL root = VGui_GetClientDLLRootPanel();
	if ( !root )
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGui_CreateGlobalPanels( void )
{
	VPANEL gameParent = enginevgui->GetPanel( PANEL_CLIENTDLL );
	VPANEL toolParent = enginevgui->GetPanel( PANEL_TOOLS );

	//console->Create( parent );
	// Part of game
	textmessage->Create( gameParent );
	internalCenterPrint->Create( gameParent );
	loadingdisc->Create( gameParent );
	connectmsg->Create( gameParent );
	downloadslider->Create( gameParent );
	messagechars->Create( gameParent );

	// Debugging or related tool
	fps->Create( toolParent );
	profiling->Create( toolParent );
	netgraphpanel->Create( toolParent );
#ifdef VPROF_ENABLED
	vprofgraphpanel->Create( toolParent );
#endif
	debugoverlaypanel->Create( toolParent );
}

void VGui_Shutdown()
{
	VGUI_DestroyClientDLLRootPanel();

	debugoverlaypanel->Destroy();
#ifdef VPROF_ENABLED
	vprofgraphpanel->Destroy();
#endif
	netgraphpanel->Destroy();
	profiling->Destroy();
	fps->Destroy();

	messagechars->Destroy();
	downloadslider->Destroy();
	connectmsg->Destroy();
	loadingdisc->Destroy();
	internalCenterPrint->Destroy();
	textmessage->Destroy();
	//console->Destroy();

	// Make sure anything "marked for deletion"
	//  actually gets deleted before this dll goes away
	vgui::ivgui()->RunFrame();
}


//-----------------------------------------------------------------------------
// Things to do before rendering vgui stuff...
//-----------------------------------------------------------------------------
void VGui_PreRender()
{
	VPROF( "VGui_PreRender" );
	loadingdisc->SetVisible( engine->IsDrawingLoadingImage() && !render->IsPlayingDemo() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cl_panelanimation - 
//-----------------------------------------------------------------------------
CON_COMMAND( cl_panelanimation, "Shows panel animation variables: <panelname | blanak for all panels>." )
{
	if ( engine->Cmd_Argc() == 2 )
	{
		PanelAnimationDumpVars( engine->Cmd_Argv(1) );
	}
	else
	{
		PanelAnimationDumpVars( NULL );
	}
}