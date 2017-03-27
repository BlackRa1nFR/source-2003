//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Material editor
//=============================================================================
#include <windows.h>
#include "appframework/AppFramework.h"
#include "FileSystem.h"
#include "materialsystem/IMaterialSystem.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Panel.h"
#include "vgui/ISurface.h"
#include "vgui_controls/controls.h"
#include "vgui/IScheme.h"
#include "vgui/IPanel.h"
#include "tier0/dbg.h"
#include "vgui_controls/Frame.h"

#include "vstdlib/ICommandLine.h"
#include "materialsystem/MaterialSystem_Config.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "MatEditPanel.h"

IFileSystem *g_pFileSystem;
IMaterialSystem *g_pMaterialSystem;
vgui::ISurface *g_pVGuiSurface;
vgui::IVGui	*g_pVGui;
IMatSystemSurface *g_pMatSystemSurface;


//-----------------------------------------------------------------------------
// The application object
//-----------------------------------------------------------------------------
class CVMTEditApp : public IApplication
{
public:
	// Methods of IApplication
	virtual bool Create( IAppSystemGroup *pAppSystemGroup );
	virtual bool PreInit( IAppSystemGroup *pAppSystemGroup );
	virtual void Main();
	virtual void PostShutdown();
	virtual void Destroy();

private:
	bool CreateAppWindow( char const *pTitle, bool bWindowed, int w, int h );
	void AppPumpMessages();

	// Sets the video mode
	bool SetVideoMode( );

	// Windproc
	static LONG WINAPI WinAppWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LONG WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND m_HWnd;
};


//-----------------------------------------------------------------------------
// Define the application object
//-----------------------------------------------------------------------------
CVMTEditApp	g_ApplicationObject;
DEFINE_APPLICATION_OBJECT_GLOBALVAR( g_ApplicationObject );


//-----------------------------------------------------------------------------
// Create all singleton systems
//-----------------------------------------------------------------------------
bool CVMTEditApp::Create( IAppSystemGroup *pAppSystemGroup )
{
	AppModule_t fileSystemModule = pAppSystemGroup->LoadModule( "filesystem_stdio.dll" );
	g_pFileSystem = (IFileSystem*)pAppSystemGroup->AddSystem( fileSystemModule, FILESYSTEM_INTERFACE_VERSION );

	AppModule_t materialSystemModule = pAppSystemGroup->LoadModule( "materialsystem.dll" );
	g_pMaterialSystem = (IMaterialSystem*)pAppSystemGroup->AddSystem( materialSystemModule, MATERIAL_SYSTEM_INTERFACE_VERSION );

	AppModule_t vguiModule = pAppSystemGroup->LoadModule( "vgui2.dll" );
	g_pVGui = (vgui::IVGui*)pAppSystemGroup->AddSystem( vguiModule, VGUI_IVGUI_INTERFACE_VERSION );

	// This is the surface we will be rendering on 
	AppModule_t vguiMatSurfaceModule = pAppSystemGroup->LoadModule( "vguimatsurface.dll" );
	g_pVGuiSurface = (vgui::ISurface*)pAppSystemGroup->AddSystem( vguiMatSurfaceModule, VGUI_SURFACE_INTERFACE_VERSION );

	// NOTE: To render on a regular windows surface, we'd do this:
	// vgui has the windows version compiled into itself
//	vgui::ISurface *pSurface = (vgui::ISurface*)pAppSystemGroup->AddSystem( vguiModule, VGUI_SURFACE_INTERFACE_VERSION );

	return g_pFileSystem && g_pMaterialSystem && g_pVGui && g_pVGuiSurface;
}

void CVMTEditApp::Destroy()
{
}



//-----------------------------------------------------------------------------
// Window management
//-----------------------------------------------------------------------------
bool CVMTEditApp::CreateAppWindow( char const *pTitle, bool bWindowed, int w, int h )
{
	WNDCLASSEX		wc;
	memset( &wc, 0, sizeof( wc ) );
	wc.cbSize		 = sizeof( wc );
    wc.style         = CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc   = DefWindowProc;
    wc.hInstance     = (HINSTANCE)GetAppInstance();
    wc.lpszClassName = "Valve001";
	wc.hIcon		 = NULL; //LoadIcon( s_HInstance, MAKEINTRESOURCE( IDI_LAUNCHER ) );
	wc.hIconSm		 = wc.hIcon;

    RegisterClassEx( &wc );

	// Note, it's hidden
	DWORD style = WS_POPUP | WS_CLIPSIBLINGS;
	
	if ( bWindowed )
	{
		// Give it a frame
		style |= WS_OVERLAPPEDWINDOW;
		style &= ~WS_THICKFRAME;
	}

	// Never a max box
	style &= ~WS_MAXIMIZEBOX;

	RECT windowRect;
	windowRect.top		= 0;
	windowRect.left		= 0;
	windowRect.right	= w;
	windowRect.bottom	= h;

	// Compute rect needed for that size client area based on window style
	AdjustWindowRectEx(&windowRect, style, FALSE, 0);

	// Create the window
	m_HWnd = CreateWindow( wc.lpszClassName, pTitle, style, 0, 0, 
		windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, 
		NULL, NULL, (HINSTANCE)GetAppInstance(), NULL );

	if (!m_HWnd)
		return false;

    int     CenterX, CenterY;

	CenterX = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
	CenterY = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
	CenterX = (CenterX < 0) ? 0: CenterX;
	CenterY = (CenterY < 0) ? 0: CenterY;

	// In VCR modes, keep it in the upper left so mouse coordinates are always relative to the window.
	SetWindowPos (m_HWnd, NULL, CenterX, CenterY, 0, 0,
				  SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_DRAWFRAME);

	return true;
}


//-----------------------------------------------------------------------------
// Pump messages
//-----------------------------------------------------------------------------
void CVMTEditApp::AppPumpMessages()
{
	MSG msg;
	while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
}


//-----------------------------------------------------------------------------
// Init, shutdown
//-----------------------------------------------------------------------------
bool CVMTEditApp::PreInit( IAppSystemGroup *pAppSystemGroup )
{	
	// initialize interfaces
	CreateInterfaceFn appFactory = pAppSystemGroup->GetFactory(); 
	if (!vgui::VGui_InitInterfacesList( "VMTEDIT", &appFactory, 1 ))
		return false;

	g_pMatSystemSurface = (IMatSystemSurface *)pAppSystemGroup->FindSystem(MAT_SYSTEM_SURFACE_INTERFACE_VERSION);

	char *pArg;
	int iWidth = 1024;
	int iHeight = 768;
	bool bWindowed = (CommandLine()->CheckParm( "-fullscreen" ) == NULL);
	if (CommandLine()->CheckParm( "-width", &pArg ))
	{
		iWidth = atoi( pArg );
	}
	if (CommandLine()->CheckParm( "-height", &pArg ))
	{
		iHeight = atoi( pArg );
	}

	if (!CreateAppWindow( "VMTEdit", bWindowed, iWidth, iHeight ))
		return false;

	g_pMatSystemSurface->AttachToWindow( m_HWnd );

	// NOTE: If we specifically wanted to use a particular shader DLL, we set it here...
	//g_pMaterialSystem->SetShader( "shaderapidx8" );
	
	return true; 
}

void CVMTEditApp::PostShutdown()
{
}


//-----------------------------------------------------------------------------
// Sets the video mode
//-----------------------------------------------------------------------------
bool CVMTEditApp::SetVideoMode( )
{
	int modeFlags = 0;
	if ( CommandLine()->CheckParm( "-ref" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_REFERENCE_RASTERIZER;
	if ( !CommandLine()->CheckParm( "-fullscreen" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_WINDOWED;
	if ( CommandLine()->CheckParm( "-resizing" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_RESIZING;
	if ( CommandLine()->CheckParm( "-mat_softwaretl" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_SOFTWARE_TL;
	if ( CommandLine()->CheckParm( "-mat_novsync" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_NO_WAIT_FOR_VSYNC;
	if ( CommandLine()->CheckParm( "-mat_antialias" ) )
		modeFlags |= MATERIAL_VIDEO_MODE_ANTIALIAS;

	// Get the adapter from the command line....
	char *pAdapterString;
	int adapter = 0;
	if (CommandLine()->CheckParm( "-adapter", &pAdapterString ))
	{
		adapter = atoi( pAdapterString );
	}

	if ( adapter >= g_pMaterialSystem->GetDisplayAdapterCount() )
	{
		adapter = 0;
	}

	MaterialVideoMode_t mode;
	mode.m_Width = mode.m_Height = 0;
	bool modeSet = g_pMaterialSystem->SetMode( m_HWnd, adapter, mode, modeFlags );
	if (!modeSet)
	{
		Error( "Unable to set mode\n" );
		return false;
	}

	return true;
}


/*
class CTestWindow : public vgui::Frame
{
public:
	CTestWindow() : vgui::Frame( NULL, "TestWindow" )
	{
		setVisible(true);
		setBounds (0, 0, 640, 480);
		m_iTexture = -1;
	}

	void paint()
	{
		int x, y;
		int wide, tall;
		getClientArea( x, y, wide, tall );

		if ( m_iTexture == -1 )
		{
			m_iTexture = vgui::surface()->createNewTextureID();
			vgui::surface()->drawSetTextureFile( m_iTexture, "brick/brickfloor001a" );
		}

		vgui::surface()->drawSetTexture( m_iTexture );
		vgui::surface()->drawTexturedRect( x, y, x + wide, y + tall );

		vgui::surface()->drawSetColor(255, 0, 0, 255);
		vgui::surface()->drawFilledRect(50,50,wide * 0.5f,tall * 0.5f);
		vgui::surface()->drawSetColor(0, 0, 255, 255);
		vgui::surface()->drawOutlinedRect(0,0,wide * 0.5f,tall * 0.5f);
		vgui::surface()->drawOutlinedRect(-5,-5,50,50);
		vgui::surface()->drawSetColor(0, 255, 0, 255);
		vgui::surface()->drawLine( 20, 60, wide * 2, tall * 1.5 );
		vgui::surface()->drawOutlinedCircle( 600, 400, 100, 30 );

		vgui::Vertex_t verts[4];
		verts[0].m_Position.Init( 40, 50 );
		verts[0].m_TexCoord.Init( 0, 0 );

		verts[1].m_Position.Init( 90, 40 );
		verts[1].m_TexCoord.Init( 1, 0 );

		verts[2].m_Position.Init( 100, 300 );
		verts[2].m_TexCoord.Init( 1, 1 );

		verts[3].m_Position.Init( 30, 290 );
		verts[3].m_TexCoord.Init( 0, 1 );

		vgui::surface()->drawTexturedPolygon( 4, verts );
	}

private:
	int m_iTexture;
};
*/

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : depth - 
//			start - 
//			end - 
//			*current - 
//			totaldrawn - 
//-----------------------------------------------------------------------------
void VGui_RecursivePrintTree( int depth, int start, int end, vgui::Panel *current, int& totaldrawn )
{
	// No more room
	if ( totaldrawn >= 128 )
		return;

	if ( !current )
		return;

	int count = current->GetChildCount();
	for ( int i = 0; i < count ; i++ )
	{
		vgui::Panel *panel = current->GetChild( i );
//		Msg( "%i:  %s : %p, %s %s\n", 
//			i + 1, 
//			panel->GetName(), 
//			panel, 
//			panel->IsVisible() ? "visible" : "hidden",
//			panel->IsPopup() ? "popup" : "" );

		int width = panel->GetWide();
		int height = panel->GetTall();
		int x, y;
		
		panel->GetPos( x, y );

		if ( depth >= start && depth <= end )
		{
			totaldrawn++;
			Msg( 
			// Con_NPrintf( totaldrawn++, 
				"%s (%i.%i): %p, %s %s x(%i) y(%i) w(%i) h(%i)\n", 
				panel->GetName(), 
				depth + 1,
				i + 1, 
				panel, 
				panel->IsVisible() ? "visible" : "hidden",
				panel->IsPopup() ? "popup" : "",
				x, y,
				width, height );
		}

		VGui_RecursivePrintTree( depth + 1, start, end, panel, totaldrawn );	
	}
}

#define VGUI_DRAWPOPUPS 0
#define VGUI_DRAWTREE 0
#define VGUI_DRAWPANEL ""
#define VGUI_TREESTART 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGui_DrawPopups( void )
{
	if ( !VGUI_DRAWPOPUPS )
		return;
	
	int c = vgui::surface()->GetPopupCount();
	for ( int i = 0; i < c; i++ )
	{
		vgui::VPANEL popup = vgui::surface()->GetPopup( i );
		if ( !popup )
			continue;

		const char *p = vgui::ipanel()->GetName( popup );
		bool visible = vgui::ipanel()->IsVisible( popup );

		int width, height;
		
		vgui::ipanel()->GetSize( popup, width, height );

		//Con_NPrintf( i, 
		Msg( 
			"%i:  %s : %p, %s w(%i) h(%i)\n",
			i,
			p,
			popup,
			visible ? "visible" : "hidden",
			width, height );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGui_DrawHierarchy( void )
{
	if ( VGUI_DRAWTREE <= 0 && Q_strlen( VGUI_DRAWPANEL ) <= 0 )
		return;

	Msg( "\n" );

	int startlevel = 0;
	int endlevel = 1000;

	bool wholetree = VGUI_DRAWTREE > 0 ? true : false;
	
	if ( wholetree )
	{
		startlevel = VGUI_TREESTART;
		endlevel = VGUI_DRAWTREE;
	}

	// Can't start after end
	startlevel = min( endlevel, startlevel );

	int drawn = 0;


	vgui::VPANEL root = vgui::surface()->GetEmbeddedPanel();
	if ( !root )
		return;

	vgui::Panel *p = vgui::ipanel()->GetPanel( root, "VMTEDIT" );

	if ( !wholetree )
	{
		// Find named panel
		char const *name = VGUI_DRAWPANEL;
		p = p->FindChildByName( name, true );
	}

	if ( !p )
		return;

	VGui_RecursivePrintTree( 0, startlevel, endlevel, p, drawn );
}

//-----------------------------------------------------------------------------
// main application
//-----------------------------------------------------------------------------
void CVMTEditApp::Main()
{
	if (!SetVideoMode())
		return;

	// load scheme
	if (!vgui::scheme()->LoadSchemeFromFile("resource/TrackerScheme.res", "VMTEDIT" ))
	{
		Assert( 0 );
	}

	// start vgui
	g_pVGui->Start();

	g_pMaterialSystem->SetForceBindWhiteLightmaps( true );

	// add our main window
	vgui::Panel *mainPanel = CreateVMTEditPanel();

	// run app frame loop
	while (g_pVGui->IsRunning())
	{
		AppPumpMessages();
	
		g_pMaterialSystem->ClearBuffers( true, true );

		g_pVGui->RunFrame();

		g_pVGuiSurface->PaintTraverse( NULL );

		g_pMaterialSystem->SwapBuffers();

		// Some debugging helpers
		VGui_DrawHierarchy();
		VGui_DrawPopups();
	}
}



