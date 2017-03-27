//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Implementation of the VGUI ISurface interface using the 
// material system to implement it
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#include <windows.h>
#include "vstdlib/strtools.h"
#include "tier0/dbg.h"
#include "FileSystem.h"
#include <vgui/vgui.h>
#include <Color.h>
#include "UtlVector.h"
#include "Clip2D.h"
#include <vgui_controls/Panel.h>
#include <vgui/IInput.h>
#include <vgui/Point.h>
#include "TextureDictionary.h"
#include "Cursor.h"
#include "input.h"
#include <vgui/IHTML.h>
#include <vgui/IVGui.h>
#include "vgui_surfacelib/FontManager.h"
#include "FontTextureCache.h"
#include "vgui/htmlwindow.h"
#include "MatSystemSurface.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISystem.h>
#include "icvar.h"
#include "vguimatsurfacestats.h"
#include "mathlib.h"
#include <vgui/ILocalize.h>
#include "VMatrix.h"
#include <tier0/vprof.h>

#include "../vgui2/src/VPanel.h"
#include <vgui/IInputInternal.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define VPANEL_NORMAL	((vgui::SurfacePlat *) NULL)
#define VPANEL_MINIMIZED ((vgui::SurfacePlat *) 0x00000001)

// #define ENABLE_HTMLWINDOW

using namespace vgui;

HWND thisWindow=0;

//-----------------------------------------------------------------------------
// Globals...
//-----------------------------------------------------------------------------
IMaterialSystem *g_pMaterialSystem;
IFileSystem *g_pFileSystem;
vgui::IPanel *g_pIPanel;
vgui::IInputInternal *g_pIInput;
vgui::ISurface *g_pISurface;
vgui::IVGui *g_pIVGUI;
static CFontTextureCache g_FontTextureCache;
ICvar	*g_pICvar = NULL;
ConVar const *g_pCVr_speeds = NULL;
int g_nCVr_speeds = 0;
static bool g_bInDrawing;


CMatSystemSurface g_MatSystemSurface;
//-----------------------------------------------------------------------------
// Make sure the panel is the same size as the viewport
//-----------------------------------------------------------------------------
CMatEmbeddedPanel::CMatEmbeddedPanel() : BaseClass( NULL, "MatSystemTopPanel" )
{
	SetPaintBackgroundEnabled( false );
}

void CMatEmbeddedPanel::OnThink()
{
	int x, y, width, height;
	g_pMaterialSystem->GetViewport( x, y, width, height );
	SetSize( width, height );
	SetPos( x, y );
	Repaint();
}

VPANEL CMatEmbeddedPanel::IsWithinTraverse(int x, int y, bool traversePopups)
{
	VPANEL retval = BaseClass::IsWithinTraverse( x, y, traversePopups );
	if ( retval == GetVPanel() )
		return 0;
	return retval;
}

//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------

 
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CMatSystemSurface, ISurface, 
						VGUI_SURFACE_INTERFACE_VERSION, g_MatSystemSurface );



//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CMatSystemSurface::CMatSystemSurface() : m_pEmbeddedPanel(NULL), m_pWhite(NULL)
{
	m_iBoundTexture = -1; 
	m_HWnd = NULL; 
	m_bIn3DPaintMode = false;
	m_bDrawingIn3DWorld = false;
	m_PlaySoundFunc = NULL;
	m_bInThink = false;

	m_hCurrentFont = NULL;
	m_pRestrictedPanel = NULL;

	_needKB = true;
	_needMouse = true;

	memset( m_WorkSpaceInsets, 0, sizeof( m_WorkSpaceInsets ) );
}

CMatSystemSurface::~CMatSystemSurface()
{
}


//-----------------------------------------------------------------------------
// Connect, disconnect...
//-----------------------------------------------------------------------------
bool CMatSystemSurface::Connect( CreateInterfaceFn factory )
{
	if ( !factory )
		return false;

	g_pFileSystem = (IFileSystem *)factory( FILESYSTEM_INTERFACE_VERSION, NULL );
	if( !g_pFileSystem )
	{
		Warning( "MatSystemSurface requires the file system to run!\n" );
		return false;
	}

	g_pMaterialSystem = (IMaterialSystem *)factory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
	if( !g_pMaterialSystem )
	{
		Warning( "MatSystemSurface requires the material system to run!\n" );
		return false;
	}

	g_pIPanel = (IPanel *)factory( VGUI_PANEL_INTERFACE_VERSION, NULL );
	if( !g_pIPanel )
	{
		Warning( "MatSystemSurface requires the vgui::IPanel system to run!\n" );
		return false;
	}

	g_pIInput = (IInputInternal *)factory( VGUI_INPUTINTERNAL_INTERFACE_VERSION, NULL );
	if( !g_pIInput )
	{
		Warning( "MatSystemSurface requires the vgui::IInput system to run!\n" );
		return false;
	}

	// It's okay if this comes back null
	g_pICvar = ( ICvar *)factory( VENGINE_CVAR_INTERFACE_VERSION, NULL );
	if ( g_pICvar )
	{
		g_pCVr_speeds = g_pICvar->FindVar( "r_speeds" );
	}

	g_pIVGUI = ( IVGui *)factory( VGUI_IVGUI_INTERFACE_VERSION, NULL );
	if ( !g_pIVGUI )
	{
		Warning( "MatSystemSurface requires the vgui::IVGUI system to run!\n" );
		return false;
	}

	g_pISurface = this;

	// initialize vgui_control interfaces
	if (!vgui::VGui_InitInterfacesList( "MATSURFACE", &factory, 1 ))
		return false;

	return true;	
}

void CMatSystemSurface::Disconnect()
{
	g_pMaterialSystem = NULL;
	g_pFileSystem = NULL;
	g_pIPanel = NULL;
}


//-----------------------------------------------------------------------------
// Access to other interfaces...
//-----------------------------------------------------------------------------
void *CMatSystemSurface::QueryInterface( const char *pInterfaceName )
{
	// We also implement the IMatSystemSurface interface
	if (!Q_strncmp(	pInterfaceName, MAT_SYSTEM_SURFACE_INTERFACE_VERSION, Q_strlen(MAT_SYSTEM_SURFACE_INTERFACE_VERSION) + 1))
		return (IMatSystemSurface*)this;

	return NULL;
}


//-----------------------------------------------------------------------------
// Initialization and shutdown...
//-----------------------------------------------------------------------------
InitReturnVal_t CMatSystemSurface::Init( void )
{
	// Allocate a white material
	bool bFound;
	m_pWhite = g_pMaterialSystem->FindMaterial( "vgui/white", &bFound, false );
	if (!m_pWhite)
	{
		Warning("Unable to load material vgui/white!\n");
		return INIT_FAILED;
	}
	m_pWhite->IncrementReferenceCount();

	m_DrawColor[0] = m_DrawColor[1] = m_DrawColor[2] = m_DrawColor[3] = 255;
	m_nTranslateX = m_nTranslateY = 0;
	EnableScissor( false );
	SetScissorRect( 0, 0, 100000, 100000 );

	// By default, use the default embedded panel
	m_pDefaultEmbeddedPanel = new CMatEmbeddedPanel;
	SetEmbeddedPanel( m_pDefaultEmbeddedPanel->GetVPanel() );

	m_iBoundTexture = -1;

	// Initialize font info..
	m_pDrawTextPos[0] = m_pDrawTextPos[1] = 0;
	m_DrawTextColor[0] = m_DrawTextColor[1] = m_DrawTextColor[2] = m_DrawTextColor[3] = 255;

	m_bIn3DPaintMode = false;
	m_bDrawingIn3DWorld = false;
	m_PlaySoundFunc = NULL;

	// Input system
	InitInput();

	// Initialize cursors
	InitCursors();

	return INIT_OK;
}

#include <malloc.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMatSystemSurface::Shutdown( void )
{
	// Release all textures
	TextureDictionary()->DestroyAllTextures();
	m_iBoundTexture = -1;

	// Release the white material
	if ( m_pWhite )
	{
		m_pWhite->DecrementReferenceCount();
		m_pWhite = NULL;
	}

	if (m_pDefaultEmbeddedPanel)
		delete m_pDefaultEmbeddedPanel;

#if defined( ENABLE_HTMLWINDOW )
	// we need to delete these BEFORE we close our window down, as the browser is using it
	// if this DOESN'T run then it will crash when we close the main window
	for(int i=0;i<GetHTMLWindowCount();i++)
	{
		delete GetHTMLWindow(i);
	}
#endif

	m_Titles.Purge();
	m_PaintStateStack.Purge();

 	// release any custom font files
 	{for (int i = 0; i < m_CustomFontFileNames.Count(); i++)
 	{
 		::RemoveFontResource(m_CustomFontFileNames[i].String());
 	}}
 	m_CustomFontFileNames.RemoveAll();
}

void CMatSystemSurface::SetEmbeddedPanel(VPANEL pEmbeddedPanel)
{
	m_pEmbeddedPanel = pEmbeddedPanel;
	((VPanel *)pEmbeddedPanel)->Client()->RequestFocus();
}

//-----------------------------------------------------------------------------
// hierarchy root
//-----------------------------------------------------------------------------
VPANEL CMatSystemSurface::GetEmbeddedPanel()
{
	return m_pEmbeddedPanel;
}

//-----------------------------------------------------------------------------
// Purpose: cap bits
//-----------------------------------------------------------------------------
bool CMatSystemSurface::SupportsFeature(SurfaceFeature_e feature)
{
	switch (feature)
	{
	case ISurface::ESCAPE_KEY:
	case ISurface::ANTIALIASED_FONTS:
	case ISurface::DROPSHADOW_FONTS:
	case ISurface::OUTLINE_FONTS:
		return true;

	case ISurface::OPENING_NEW_HTML_WINDOWS:
	case ISurface::FRAME_MINIMIZE_MAXIMIZE:
	default:
		return false;
	};
}

//-----------------------------------------------------------------------------
// Hook needed to Get input to work
//-----------------------------------------------------------------------------
void CMatSystemSurface::AttachToWindow( void *hWnd, bool bLetAppDriveInput )
{
	InputDetachFromWindow( m_HWnd );
	m_HWnd = hWnd;
	
	if ( !bLetAppDriveInput )
		InputAttachToWindow( hWnd );
}


void CMatSystemSurface::HandleWindowMessage( void *hwnd, unsigned int uMsg, unsigned int wParam, long lParam )
{
	InputHandleWindowMessage( hwnd, uMsg, wParam, lParam );
}


//-----------------------------------------------------------------------------
// Draws a panel in 3D space. Assumes view + projection are already set up
// Also assumes the (x,y) coordinates of the panels are defined in 640xN coords
// (N isn't necessary 480 because the panel may not be 4x3)
// The width + height specified are the size of the panel in world coordinates
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawPanelIn3DSpace( vgui::VPANEL pRootPanel, const VMatrix &panelCenterToWorld, int pw, int ph, float sw, float sh )
{
	Assert( pRootPanel );

	// FIXME: When should such panels be solved?!?
	SolveTraverse( pRootPanel, false );

	// Force Z buffering to be on for all panels drawn...
	g_pMaterialSystem->OverrideDepthEnable( true, false );

	Assert(!m_bDrawingIn3DWorld);
	m_bDrawingIn3DWorld = true;

	StartDrawingIn3DSpace( panelCenterToWorld, pw, ph, sw, sh );

	((VPanel *)pRootPanel)->Client()->Repaint();
	((VPanel *)pRootPanel)->Client()->PaintTraverse(true, false);

	FinishDrawing();

	// Reset z buffering to normal state
	g_pMaterialSystem->OverrideDepthEnable( false, true ); 

	m_bDrawingIn3DWorld = false;
}


//-----------------------------------------------------------------------------
// Purpose: Setup rendering for vgui on a panel existing in 3D space
//-----------------------------------------------------------------------------
void CMatSystemSurface::StartDrawingIn3DSpace( const VMatrix &screenToWorld, int pw, int ph, float sw, float sh )
{
	g_bInDrawing = true;
	m_iBoundTexture = -1; 

	int px = 0;
	int py = 0;

	m_pSurfaceExtents[0] = px;
	m_pSurfaceExtents[1] = py;
	m_pSurfaceExtents[2] = px + pw;
	m_pSurfaceExtents[3] = py + ph;

	// In order for this to work, the model matrix must have its origin
	// at the upper left corner of the screen. We must also scale down the
	// rendering from pixel space to screen space. Let's construct a matrix
	// transforming from pixel coordinates (640xN) to screen coordinates
	// (wxh, with the origin at the upper left of the screen). Then we'll
	// concatenate it with the panelCenterToWorld to produce pixelToWorld transform
	VMatrix pixelToScreen;

	// First, scale it so that 0->pw transforms to 0->sw
	MatrixBuildScale( pixelToScreen, sw / pw, -sh / ph, 1.0f );

	// Construct pixelToWorld
	VMatrix pixelToWorld;
	MatrixMultiply( screenToWorld, pixelToScreen, pixelToWorld );

	// make sure there is no translation and rotation laying around
	g_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	g_pMaterialSystem->PushMatrix();
	g_pMaterialSystem->LoadMatrix( pixelToWorld );

	// These are only here so that FinishDrawing works...
	g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	g_pMaterialSystem->PushMatrix();

	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	g_pMaterialSystem->PushMatrix();

	// Always enable scissoring (translate to origin because of the glTranslatef call above..)
	EnableScissor( true );

	m_nTranslateX = 0;
	m_nTranslateY = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Setup ortho for vgui
//-----------------------------------------------------------------------------
void CMatSystemSurface::StartDrawing( void )
{
	g_bInDrawing = true;
	m_iBoundTexture = -1; 

	int x, y, width, height;

	g_pMaterialSystem->GetViewport( x, y, width, height);

	m_pSurfaceExtents[0] = x;
	m_pSurfaceExtents[1] = y;
	m_pSurfaceExtents[2] = x + width;
	m_pSurfaceExtents[3] = y + height;

	g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	g_pMaterialSystem->PushMatrix();
	g_pMaterialSystem->LoadIdentity();
	g_pMaterialSystem->Scale( 1, -1, 1 );
	g_pMaterialSystem->Ortho( -0.5f, -0.5f, width - 0.5f, height - 0.5f, -1, 1 ); // offset by 0.5 texels to account for the different in pixel vs. texel centers in dx7-9

	// make sure there is no translation and rotation laying around
	g_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	g_pMaterialSystem->PushMatrix();
	g_pMaterialSystem->LoadIdentity();

	// Always enable scissoring (translate to origin because of the glTranslatef call above..)
	EnableScissor( true );

	m_nTranslateX = 0;
	m_nTranslateY = 0;

	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	g_pMaterialSystem->PushMatrix();
	g_pMaterialSystem->LoadIdentity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMatSystemSurface::FinishDrawing( void )
{
	// We're done with scissoring
	EnableScissor( false );

	// Restore the matrices
	g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	g_pMaterialSystem->PopMatrix();

	g_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	g_pMaterialSystem->PopMatrix();

	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	g_pMaterialSystem->PopMatrix();

  	Assert( g_bInDrawing );
	g_bInDrawing = false;
}


// frame
void CMatSystemSurface::RunFrame()
{
}


//-----------------------------------------------------------------------------
// Sets up a particular painting state...
//-----------------------------------------------------------------------------
void CMatSystemSurface::SetupPaintState( const PaintState_t &paintState )
{
	m_nTranslateX = paintState.m_iTranslateX;
	m_nTranslateY = paintState.m_iTranslateY;
	SetScissorRect( paintState.m_iScissorLeft, paintState.m_iScissorTop, 
		paintState.m_iScissorRight, paintState.m_iScissorBottom );
}


//-----------------------------------------------------------------------------
// Indicates a particular panel is about to be rendered 
//-----------------------------------------------------------------------------
void CMatSystemSurface::PushMakeCurrent(VPANEL pPanel, bool useInSets)
{
//	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_PUSHMAKECURRENT_TIME );

	int inSets[4] = {0, 0, 0, 0};
	int absExtents[4];
	int clipRect[4];

	if (useInSets)
	{
		g_pIPanel->GetInset(pPanel, inSets[0], inSets[1], inSets[2], inSets[3]);
	}

	g_pIPanel->GetAbsPos(pPanel, absExtents[0], absExtents[1]);
	int wide, tall;
	g_pIPanel->GetSize(pPanel, wide, tall);
	absExtents[2] = absExtents[0] + wide;
	absExtents[3] = absExtents[1] + tall;

	g_pIPanel->GetClipRect(pPanel, clipRect[0], clipRect[1], clipRect[2], clipRect[3]);

	int i = m_PaintStateStack.AddToTail();
	PaintState_t &paintState = m_PaintStateStack[i];
	paintState.m_pPanel = pPanel;

	// Determine corrected top left origin
	paintState.m_iTranslateX = inSets[0] + absExtents[0] - m_pSurfaceExtents[0];	
	paintState.m_iTranslateY = inSets[1] + absExtents[1] - m_pSurfaceExtents[1];

	// Setup clipping rectangle for scissoring
	paintState.m_iScissorLeft	= clipRect[0] - m_pSurfaceExtents[0];
	paintState.m_iScissorTop	= clipRect[1] - m_pSurfaceExtents[1];
	paintState.m_iScissorRight	= clipRect[2] - m_pSurfaceExtents[0];
	paintState.m_iScissorBottom	= clipRect[3] - m_pSurfaceExtents[1];
	SetupPaintState( paintState );
}

void CMatSystemSurface::PopMakeCurrent(VPANEL pPanel)
{
//	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_POPMAKECURRENT_TIME );

	int top = m_PaintStateStack.Count() - 1;

	// More pops that pushes?
	Assert( top >= 0 );

	// Didn't pop in reverse order of push?
	Assert( m_PaintStateStack[top].m_pPanel == pPanel );

	m_PaintStateStack.Remove(top);

	if (top > 0)
		SetupPaintState( m_PaintStateStack[top-1] );

//	m_iBoundTexture = -1; 
}


//-----------------------------------------------------------------------------
// Color Setting methods
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawSetColor(int r, int g, int b, int a)
{
  	Assert( g_bInDrawing );
	m_DrawColor[0]=(unsigned char)r;
	m_DrawColor[1]=(unsigned char)g;
	m_DrawColor[2]=(unsigned char)b;
	m_DrawColor[3]=(unsigned char)a;
}

void CMatSystemSurface::DrawSetColor(Color col)
{
  	Assert( g_bInDrawing );
	DrawSetColor(col[0], col[1], col[2], col[3]);
}


//-----------------------------------------------------------------------------
// material Setting methods 
//-----------------------------------------------------------------------------
void CMatSystemSurface::InternalSetMaterial( IMaterial *pMaterial )
{
	if (!pMaterial)
	{
		pMaterial = m_pWhite;
	}

	m_pMesh = g_pMaterialSystem->GetDynamicMesh( true, NULL, NULL, pMaterial );
}


//-----------------------------------------------------------------------------
// Helper method to initialize vertices (transforms them into screen space too)
//-----------------------------------------------------------------------------
void CMatSystemSurface::InitVertex( vgui::Vertex_t &vertex, int x, int y, float u, float v )
{
	vertex.m_Position.Init( x + m_nTranslateX, y + m_nTranslateY );
	vertex.m_TexCoord.Init( u, v );
}


//-----------------------------------------------------------------------------
// Draws a line!
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawTexturedLineInternal( const Vertex_t &a, const Vertex_t &b )
{
	Assert( !m_bIn3DPaintMode );

	// Don't bother drawing fully transparent lines
	if( m_DrawColor[3] == 0 )
		return;

	vgui::Vertex_t verts[2] = { a, b };
	
	verts[0].m_Position.x += m_nTranslateX;
	verts[0].m_Position.y += m_nTranslateY;
	
	verts[1].m_Position.x += m_nTranslateX;
	verts[1].m_Position.y += m_nTranslateY;

	vgui::Vertex_t clippedVerts[2];

	if (!ClipLine( verts, clippedVerts ))
		return;

	meshBuilder.Begin( m_pMesh, MATERIAL_LINES, 1 );

	meshBuilder.Color4ubv( m_DrawColor );
	meshBuilder.TexCoord2fv( 0, clippedVerts[0].m_TexCoord.Base() );
	meshBuilder.Position3f( clippedVerts[0].m_Position.x, clippedVerts[0].m_Position.y, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( m_DrawColor );
	meshBuilder.TexCoord2fv( 0, clippedVerts[1].m_TexCoord.Base() );
	meshBuilder.Position3f( clippedVerts[1].m_Position.x, clippedVerts[1].m_Position.y, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	m_pMesh->Draw();
}

void CMatSystemSurface::DrawLine( int x0, int y0, int x1, int y1 )
{
	Assert( g_bInDrawing );

	// Don't bother drawing fully transparent lines
	if( m_DrawColor[3] == 0 )
		return;

	vgui::Vertex_t verts[2];
	verts[0].Init( Vector2D( x0, y0 ), Vector2D( 0, 0 ) );
	verts[1].Init( Vector2D( x1, y1 ), Vector2D( 1, 1 ) );
	
	InternalSetMaterial( );
	DrawTexturedLineInternal( verts[0], verts[1] );
}


void CMatSystemSurface::DrawTexturedLine( const Vertex_t &a, const Vertex_t &b )
{
	IMaterial *pMaterial = TextureDictionary()->GetTextureMaterial(m_iBoundTexture);
	InternalSetMaterial( pMaterial );
	DrawTexturedLineInternal( a, b );
}


//-----------------------------------------------------------------------------
// Draws a line!
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawPolyLine( int *px, int *py ,int n )
{
	Assert( g_bInDrawing );

	Assert( !m_bIn3DPaintMode );

	// Don't bother drawing fully transparent lines
	if( m_DrawColor[3] == 0 )
		return;

	InternalSetMaterial( );
	meshBuilder.Begin( m_pMesh, MATERIAL_LINES, n );

	for ( int i = 0; i < n ; i++ )
	{
		int inext = ( i + 1 ) % n;

		vgui::Vertex_t verts[2];
		vgui::Vertex_t clippedVerts[2];
		
		int x0, y0, x1, y1;

		x0 = px[ i ];
		x1 = px[ inext ];
		y0 = py[ i ];
		y1 = py[ inext ];

		InitVertex( verts[0], x0, y0, 0, 0 );
		InitVertex( verts[1], x1, y1, 1, 1 );

		if (!ClipLine( verts, clippedVerts ))
			continue;

		meshBuilder.Color4ubv( m_DrawColor );
		meshBuilder.TexCoord2fv( 0, clippedVerts[0].m_TexCoord.Base() );
		meshBuilder.Position3f( clippedVerts[0].m_Position.x, clippedVerts[0].m_Position.y, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ubv( m_DrawColor );
		meshBuilder.TexCoord2fv( 0, clippedVerts[1].m_TexCoord.Base() );
		meshBuilder.Position3f( clippedVerts[1].m_Position.x, clippedVerts[1].m_Position.y, 0 );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	m_pMesh->Draw();
}


void CMatSystemSurface::DrawTexturedPolyLine( const vgui::Vertex_t *p,int n )
{
	int iPrev = n - 1;
	for ( int i=0; i < n; i++ )
	{
		DrawTexturedLine( p[iPrev], p[i] );
		iPrev = i;
	}
}


//-----------------------------------------------------------------------------
// Draws a quad: 
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawQuad( const vgui::Vertex_t &ul, const vgui::Vertex_t &lr, unsigned char *pColor )
{
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_DRAWQUAD_TIME );

	Assert( !m_bIn3DPaintMode );

	if ( !m_pMesh )
		return;

	meshBuilder.Begin( m_pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv( pColor );
	meshBuilder.Position3f( ul.m_Position.x, ul.m_Position.y, 0 );
	meshBuilder.TexCoord2f( 0, ul.m_TexCoord.x, ul.m_TexCoord.y );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( pColor );
	meshBuilder.Position3f( lr.m_Position.x, ul.m_Position.y, 0 );
	meshBuilder.TexCoord2f( 0, lr.m_TexCoord.x, ul.m_TexCoord.y );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( pColor );
	meshBuilder.Position3f( lr.m_Position.x, lr.m_Position.y, 0 );
	meshBuilder.TexCoord2f( 0, lr.m_TexCoord.x, lr.m_TexCoord.y );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( pColor );
	meshBuilder.Position3f( ul.m_Position.x, lr.m_Position.y, 0 );
	meshBuilder.TexCoord2f( 0, ul.m_TexCoord.x, lr.m_TexCoord.y );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	m_pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Purpose: Draws an array of quads
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawQuadArray( int quadCount, vgui::Vertex_t *pVerts, unsigned char *pColor )
{
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_DRAWQUADARRAY_TIME );

	Assert( !m_bIn3DPaintMode );

	if ( !m_pMesh )
		return;

	meshBuilder.Begin( m_pMesh, MATERIAL_QUADS, quadCount );

	for (int i = 0; i < quadCount; ++i )
	{
		vgui::Vertex_t &ul = pVerts[2*i];
		vgui::Vertex_t &lr = pVerts[2*i + 1];

		meshBuilder.Color4ubv( pColor );
		meshBuilder.Position3f( ul.m_Position.x, ul.m_Position.y, 0 );
		meshBuilder.TexCoord2f( 0, ul.m_TexCoord.x, ul.m_TexCoord.y );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ubv( pColor );
		meshBuilder.Position3f( lr.m_Position.x, ul.m_Position.y, 0 );
		meshBuilder.TexCoord2f( 0, lr.m_TexCoord.x, ul.m_TexCoord.y );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ubv( pColor );
		meshBuilder.Position3f( lr.m_Position.x, lr.m_Position.y, 0 );
		meshBuilder.TexCoord2f( 0, lr.m_TexCoord.x, lr.m_TexCoord.y );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ubv( pColor );
		meshBuilder.Position3f( ul.m_Position.x, lr.m_Position.y, 0 );
		meshBuilder.TexCoord2f( 0, ul.m_TexCoord.x, lr.m_TexCoord.y );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	m_pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Purpose: Draws a rectangle colored with the current drawcolor
//		using the white material
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawFilledRect(int x0,int y0,int x1,int y1)
{
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_DRAWFILLEDRECT_TIME );

	Assert( g_bInDrawing );

	// Don't even bother drawing fully transparent junk
	if( m_DrawColor[3]==0 )
		return;

	vgui::Vertex_t rect[2];
	vgui::Vertex_t clippedRect[2];
	InitVertex( rect[0], x0, y0, 0, 0 );
	InitVertex( rect[1], x1, y1, 0, 0 );

	// Fully clipped?
	if ( !ClipRect(rect[0], rect[1], &clippedRect[0], &clippedRect[1]) )
		return;	
	
	InternalSetMaterial( );
	DrawQuad( clippedRect[0], clippedRect[1], m_DrawColor );
}


//-----------------------------------------------------------------------------
// Purpose: Draws an unfilled rectangle in the current drawcolor
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawOutlinedRect(int x0,int y0,int x1,int y1)
{		
	// Don't even bother drawing fully transparent junk
	if ( m_DrawColor[3] == 0 )
		return;

	DrawFilledRect(x0,y0,x1,y0+1);     //top
	DrawFilledRect(x0,y1-1,x1,y1);	   //bottom
	DrawFilledRect(x0,y0+1,x0+1,y1-1); //left
	DrawFilledRect(x1-1,y0+1,x1,y1-1); //right
}


//-----------------------------------------------------------------------------
// Purpose: Draws an outlined circle in the current drawcolor
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawOutlinedCircle(int x, int y, int radius, int segments)
{
	Assert( g_bInDrawing );

	Assert( !m_bIn3DPaintMode );

	// Don't even bother drawing fully transparent junk
	if( m_DrawColor[3]==0 )
		return;

	// NOTE: Gotta use lines instead of linelist or lineloop due to clipping
	InternalSetMaterial( );
	meshBuilder.Begin( m_pMesh, MATERIAL_LINES, segments );

	vgui::Vertex_t renderVertex[2];
	vgui::Vertex_t vertex[2];
	vertex[0].m_Position.Init( m_nTranslateX + x + radius, m_nTranslateY + y );
	vertex[0].m_TexCoord.Init( 1.0f, 0.5f );

	float invDelta = 2.0f * M_PI / segments;
	for ( int i = 1; i <= segments; ++i )
	{
		float flRadians = i * invDelta;
		float ca = cos( flRadians );
		float sa = sin( flRadians );
					 
		// Rotate it around the circle
		vertex[1].m_Position.x = m_nTranslateX + x + (radius * ca);
		vertex[1].m_Position.y = m_nTranslateY + y + (radius * sa);
		vertex[1].m_TexCoord.x = 0.5f * (ca + 1.0f);
		vertex[1].m_TexCoord.y = 0.5f * (sa + 1.0f);

		if (ClipLine( vertex, renderVertex ))
		{
			meshBuilder.Color4ubv( m_DrawColor );
			meshBuilder.TexCoord2fv( 0, renderVertex[0].m_TexCoord.Base() );
			meshBuilder.Position3f( renderVertex[0].m_Position.x, renderVertex[0].m_Position.y, 0 );
			meshBuilder.AdvanceVertex();

			meshBuilder.Color4ubv( m_DrawColor );
			meshBuilder.TexCoord2fv( 0, renderVertex[1].m_TexCoord.Base() );
			meshBuilder.Position3f( renderVertex[1].m_Position.x, renderVertex[1].m_Position.y, 0 );
			meshBuilder.AdvanceVertex();
		}

		vertex[0].m_Position = vertex[1].m_Position;
		vertex[0].m_TexCoord = vertex[1].m_TexCoord;
	}

	meshBuilder.End();
	m_pMesh->Draw();
}


//-----------------------------------------------------------------------------
// Loads a particular texture (material)
//-----------------------------------------------------------------------------
int CMatSystemSurface::CreateNewTextureID( bool procedural /*=false*/ )
{
	return TextureDictionary()->CreateTexture( procedural );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : id - 
//			*filename - 
//			maxlen - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMatSystemSurface::DrawGetTextureFile(int id, char *filename, int maxlen )
{
	if ( !TextureDictionary()->IsValidId( id ) )
		return false;

	IMaterial *texture = TextureDictionary()->GetTextureMaterial(id);
	if ( !texture )
		return false;

	Q_strncpy( filename, texture->GetName(), maxlen );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
// Output : int
//-----------------------------------------------------------------------------
int CMatSystemSurface::DrawGetTextureId( char const *filename )
{
	return TextureDictionary()->FindTextureIdForTextureFile( filename );
}

//-----------------------------------------------------------------------------
// Associates a texture with a material file (also binds it)
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawSetTextureFile(int id, const char *pFileName, int hardwareFilter, bool forceReload /*= false*/)
{
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_DRAWSETTEXTURE_TIME );

	TextureDictionary()->BindTextureToFile( id, pFileName );
	DrawSetTexture( id );
}


//-----------------------------------------------------------------------------
// Associates a texture with a material file (also binds it)
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawSetTextureMaterial(int id, IMaterial *pMaterial)
{
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_DRAWSETTEXTURE_TIME );

	TextureDictionary()->BindTextureToMaterial( id, pMaterial );
	DrawSetTexture( id );
}


void CMatSystemSurface::ReferenceProceduralMaterial( int id, int referenceId, IMaterial *pMaterial )
{
	TextureDictionary()->BindTextureToMaterialReference( id, referenceId, pMaterial );
}


//-----------------------------------------------------------------------------
// Binds a texture
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawSetTexture(int id)
{
	m_iBoundTexture = id;
}


//-----------------------------------------------------------------------------
// Returns texture size
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawGetTextureSize(int id, int &iWide, int &iTall)
{
	TextureDictionary()->GetTextureSize( id, iWide, iTall );
}


//-----------------------------------------------------------------------------
// Draws a textured rectangle
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawTexturedRect( int x0, int y0, int x1, int y1 )
{
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_DRAWTEXTUREDRECT_TIME );

	Assert( g_bInDrawing );

	// Don't even bother drawing fully transparent junk
	if( m_DrawColor[3] == 0 )
		return;

	float s0, t0, s1, t1;
	TextureDictionary()->GetTextureTexCoords( m_iBoundTexture, s0, t0, s1, t1 );

	vgui::Vertex_t rect[2];
	vgui::Vertex_t clippedRect[2];
	InitVertex( rect[0], x0, y0, s0, t0 );
	InitVertex( rect[1], x1, y1, s1, t1 );

	// Fully clipped?
	if ( !ClipRect(rect[0], rect[1], &clippedRect[0], &clippedRect[1]) )
		return;	

	IMaterial *pMaterial = TextureDictionary()->GetTextureMaterial(m_iBoundTexture);
	InternalSetMaterial( pMaterial );
	DrawQuad( clippedRect[0], clippedRect[1], m_DrawColor );
}

//-----------------------------------------------------------------------------
// Draws a textured rectangle
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 )
{
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_DRAWTEXTUREDRECT_TIME );

	Assert( g_bInDrawing );

	// Don't even bother drawing fully transparent junk
	if( m_DrawColor[3] == 0 )
		return;

	float s0, t0, s1, t1;
	TextureDictionary()->GetTextureTexCoords( m_iBoundTexture, s0, t0, s1, t1 );

	float ssize = s1 - s0;
	float tsize = t1 - t0;

	// Rescale tex values into range of s0 to s1 ,etc.
	texs0 = s0 + texs0 * ( ssize );
	texs1 = s0 + texs1 * ( ssize );
	text0 = t0 + text0 * ( tsize );
	text1 = t0 + text1 * ( tsize );

	vgui::Vertex_t rect[2];
	vgui::Vertex_t clippedRect[2];
	InitVertex( rect[0], x0, y0, texs0, text0 );
	InitVertex( rect[1], x1, y1, texs1, text1 );

	// Fully clipped?
	if ( !ClipRect(rect[0], rect[1], &clippedRect[0], &clippedRect[1]) )
		return;	

	IMaterial *pMaterial = TextureDictionary()->GetTextureMaterial(m_iBoundTexture);
	InternalSetMaterial( pMaterial );
	DrawQuad( clippedRect[0], clippedRect[1], m_DrawColor );
}

//-----------------------------------------------------------------------------
// Draws a textured polygon
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawTexturedPolygon(int n, Vertex_t *pVertices)
{
	Assert( !m_bIn3DPaintMode );

	Assert( g_bInDrawing );

	// Don't even bother drawing fully transparent junk
	if( (n == 0) || (m_DrawColor[3]==0) )
		return;

	int iCount;
	Vertex_t **ppClippedVerts;

	// Clip vertices...
	iCount = ClipPolygon( n, pVertices, m_nTranslateX, m_nTranslateY, &ppClippedVerts );
	if (iCount <= 0)
		return;

	IMaterial *pMaterial = TextureDictionary()->GetTextureMaterial(m_iBoundTexture);
	InternalSetMaterial( pMaterial );

	meshBuilder.Begin( m_pMesh, MATERIAL_POLYGON, iCount );

	for (int i = 0; i < iCount; ++i)
	{
		meshBuilder.Color4ubv( m_DrawColor );
		meshBuilder.TexCoord2fv( 0, ppClippedVerts[i]->m_TexCoord.Base() );
		meshBuilder.Position3f( ppClippedVerts[i]->m_Position.x, 
			ppClippedVerts[i]->m_Position.y, 0 );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	m_pMesh->Draw();
}



//-----------------------------------------------------------------------------
//
// Font-related methods begin here
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: creates a new empty font
//-----------------------------------------------------------------------------
HFont CMatSystemSurface::CreateFont()
{
	return FontManager().CreateFont();
}

//-----------------------------------------------------------------------------
// Purpose: adds glyphs to a font created by CreateFont()
//-----------------------------------------------------------------------------
bool CMatSystemSurface::AddGlyphSetToFont(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int lowRange, int highRange)
{
	return FontManager().AddGlyphSetToFont(font, windowsFontName, tall, weight, blur, scanlines, flags, lowRange, highRange);
}

//-----------------------------------------------------------------------------
// Purpose: returns the max height of a font
//-----------------------------------------------------------------------------
int CMatSystemSurface::GetFontTall(HFont font)
{
	return FontManager().GetFontTall(font);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : font - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMatSystemSurface::IsFontAdditive(HFont font)
{
	return FontManager().IsFontAdditive(font);
}

//-----------------------------------------------------------------------------
// Purpose: returns the abc widths of a single character
//-----------------------------------------------------------------------------
void CMatSystemSurface::GetCharABCwide(HFont font, int ch, int &a, int &b, int &c)
{
	FontManager().GetCharABCwide(font, ch, a, b, c);
}

//-----------------------------------------------------------------------------
// Purpose: returns the pixel width of a single character
//-----------------------------------------------------------------------------
int CMatSystemSurface::GetCharacterWidth(HFont font, int ch)
{
	return FontManager().GetCharacterWidth(font, ch);
}

//-----------------------------------------------------------------------------
// Purpose: returns the area of a text string, including newlines
//-----------------------------------------------------------------------------
void CMatSystemSurface::GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall)
{
	FontManager().GetTextSize(font, text, wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: adds a custom font file (only supports true type font files (.ttf) for now)
//-----------------------------------------------------------------------------
bool CMatSystemSurface::AddCustomFontFile(const char *fontFileName)
{
	// get local path
	char *fullPath = (char *)_alloca(filesystem()->GetLocalPathLen(fontFileName) + 1);
	if (!filesystem()->GetLocalPath(fontFileName, fullPath))
	{
		Msg("Couldn't find custom font file '%s'\n", fontFileName);
		return false;
	}
	strlwr(fullPath);

	// only add if it's not already in the list
	CUtlSymbol sym(fullPath);
	int i;
	for (i = 0; i < m_CustomFontFileNames.Count(); i++)
	{
		if (m_CustomFontFileNames[i] == sym)
			break;
	}
	if (!m_CustomFontFileNames.IsValidIndex(i))
	{
	 	m_CustomFontFileNames.AddToTail(fullPath);
	}

	// add to windows
	bool success = (::AddFontResource(fullPath) > 0);
	if (!success)
	{
		Msg("Failed to load custom font file '%s'\n", fullPath);
	}
	Assert(success);
	return success;
}


/*
//-----------------------------------------------------------------------------
// Creates a font
//-----------------------------------------------------------------------------
HFont CMatSystemSurface::CreateFont(const char *pName, int iTall, int iWide, 
	float flRotation, int iWeight, bool bItalic, bool bUnderline, bool bStrikeout, 
	bool bSymbol, bool bBorder, bool bAdditive, bool bOverbright )
{
	int iFlags = 0;
	if (bItalic)
		iFlags |= FONT_ITALIC;
	if (bUnderline)
		iFlags |= FONT_UNDERLINE;
	if (bStrikeout)
		iFlags |= FONT_STRIKEOUT;
	if (bSymbol)
		iFlags |= FONT_SYMBOL;
	if (bBorder)
		iFlags |= FONT_BORDER;
	if (bAdditive)
		iFlags |= FONT_ADDITIVE;
	if (bOverbright)
		iFlags |= FONT_OVERBRIGHT;

	return (HFont)CreateFontTemplate( pName, iTall, iWide, flRotation, iWeight, iFlags );
}

//-----------------------------------------------------------------------------
// Font information
//-----------------------------------------------------------------------------
int CMatSystemSurface::GetFontTall(HFont font)
{
	IFontTemplate *pFontTemplate = GetFontTemplate(	(FontTemplateHandle_t)font );
	return pFontTemplate->GetTall();
}

void CMatSystemSurface::GetCharABCwide(HFont font, int ch, int &a, int &b, int &c)
{
	IFontTemplate *pFontTemplate = GetFontTemplate(	(FontTemplateHandle_t)font );
	pFontTemplate->GetCharABCwide(ch, a, b, c);
}

void CMatSystemSurface::GetTextSize(HFont font, const char *pText, int &iWide, int &iTall)
{
	IFontTemplate *pFontTemplate = GetFontTemplate(	(FontTemplateHandle_t)font );
	pFontTemplate->GetTextSize(pText, iWide, iTall);
}


//-----------------------------------------------------------------------------
// Draws a textured polygon
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawSetTextFont(HFont font)
{
//	CScreenDisable sd;

	SetRenderFont( font );
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawSetTextFont(HFont font)
{
	Assert( g_bInDrawing );

	m_hCurrentFont = font;
}

//-----------------------------------------------------------------------------
// Purpose: Renders any batched up text
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawFlushText()
{
}

//-----------------------------------------------------------------------------
// Sets the text color
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawSetTextColor(int r, int g, int b, int a)
{
	m_DrawTextColor[0] = (unsigned char)r;
	m_DrawTextColor[1] = (unsigned char)g;
	m_DrawTextColor[2] = (unsigned char)b;
	m_DrawTextColor[3] = (unsigned char)a;
}

void CMatSystemSurface::DrawSetTextColor(Color col)
{
	DrawSetTextColor(col[0], col[1], col[2], col[3]);
}


//-----------------------------------------------------------------------------
// Text rendering location
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawSetTextPos(int x, int y)
{
	Assert( g_bInDrawing );

	m_pDrawTextPos[0] = x;
	m_pDrawTextPos[1] = y;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : x - 
//			y - 
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawGetTextPos(int& x,int& y)
{
	Assert( g_bInDrawing );

	x = m_pDrawTextPos[0];
	y = m_pDrawTextPos[1];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ch - 
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawUnicodeChar(wchar_t ch, FontDrawType_t drawType /*= FONT_DRAW_DEFAULT */ )
{
	CharRenderInfo info;
	info.drawType = drawType;
	if ( DrawGetUnicodeCharRenderInfo( ch, info ) )
	{
		DrawRenderCharFromInfo( info );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ch - 
//			info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMatSystemSurface::DrawGetUnicodeCharRenderInfo( wchar_t ch, CharRenderInfo& info )
{
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_DRAWUNICODECHAR_TIME );

	Assert( g_bInDrawing );
	info.valid = false;

	if ( !m_hCurrentFont )
	{
		return info.valid;
	}

	info.valid = true;
	info.ch = ch;
	DrawGetTextPos(info.x, info.y);

	info.currentFont = m_hCurrentFont;
	info.fontTall = GetFontTall(m_hCurrentFont);

	GetCharABCwide(m_hCurrentFont, ch, info.abcA, info.abcB, info.abcC);
	
	// Do prestep before generating texture coordinates, etc.
	info.x += info.abcA;

	// get the character texture from the cache
	info.textureId = 0;
	float *texCoords = NULL;
	if (!g_FontTextureCache.GetTextureForChar(m_hCurrentFont, info.drawType, ch, &info.textureId, &texCoords))
	{
		info.valid = false;
		return info.valid;
	}

	InitVertex( info.verts[0], info.x, info.y, texCoords[0], texCoords[1] );
	InitVertex( info.verts[1], info.x + info.abcB, info.y + info.fontTall, texCoords[2], texCoords[3] );

	info.shouldclip = true;

	return info.valid;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : info - 
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawRenderCharInternal( const CharRenderInfo& info )
{
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_DRAWPRINTCHAR_TIME );

	Assert( g_bInDrawing );
	// Don't even bother drawing fully transparent junk
	if( m_DrawTextColor[3]==0 )
		return;

	vgui::Vertex_t clippedRect[2];

	if ( info.shouldclip )
	{
		// Fully clipped?
		if ( !ClipRect(info.verts[0], info.verts[1], &clippedRect[0], &clippedRect[1]) )
			return;	
	}
	else
	{
		clippedRect[0] = info.verts[0];
		clippedRect[1] = info.verts[1];
	}

	IMaterial *pMaterial = TextureDictionary()->GetTextureMaterial(m_iBoundTexture);
	InternalSetMaterial( pMaterial );
	DrawQuad( clippedRect[0], clippedRect[1], m_DrawTextColor );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : info - 
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawRenderCharFromInfo( const CharRenderInfo& info )
{
	if ( !info.valid )
		return;

	int x = info.x;

	// get the character texture from the cache
	DrawSetTexture( info.textureId );
	
	DrawRenderCharInternal( info );

	// Only do post step
	x += ( info.abcB + info.abcC );

	// Update cursor pos
	DrawSetTextPos(x, info.y);
}

//-----------------------------------------------------------------------------
// Renders a text buffer
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawPrintText(const wchar_t *text, int iTextLen, FontDrawType_t drawType /*= FONT_DRAW_DEFAULT */ )
{
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_DRAWPRINTTEXT_TIME );

	Assert( g_bInDrawing );
	if (!text)
		return;

	if (!m_hCurrentFont)
		return;

	int x = m_pDrawTextPos[0] + m_nTranslateX;
	int y = m_pDrawTextPos[1] + m_nTranslateY;

	int iTall = GetFontTall(m_hCurrentFont);
	int iLastTexId = -1;

	int iCount = 0;
	vgui::Vertex_t *pQuads = (vgui::Vertex_t*)stackalloc((2 * iTextLen) * sizeof(vgui::Vertex_t) );

	int iTotalWidth = 0;
	for (int i=0; i<iTextLen; ++i)
	{
		wchar_t ch = text[i];

		int abcA,abcB,abcC;
		GetCharABCwide(m_hCurrentFont, ch, abcA, abcB, abcC);

		iTotalWidth += abcA;
		int iWide = abcB;

		if ( !iswspace( ch ) )
		{
			// get the character texture from the cache
			int iTexId = 0;
			float *texCoords = NULL;
			if (!g_FontTextureCache.GetTextureForChar(m_hCurrentFont, drawType, ch, &iTexId, &texCoords))
				continue;

			Assert( texCoords );

			if (iTexId != iLastTexId)
			{
				// FIXME: At the moment, we just draw all the batched up
				// text when the font changes. We Should batch up per material
				// and *then* draw
				if (iCount)
				{
					IMaterial *pMaterial = TextureDictionary()->GetTextureMaterial(iLastTexId);
					InternalSetMaterial( pMaterial );
					DrawQuadArray( iCount, pQuads, m_DrawTextColor );
					iCount = 0;
				}

				iLastTexId = iTexId;
			}

 			vgui::Vertex_t &ul = pQuads[2*iCount];
 			vgui::Vertex_t &lr = pQuads[2*iCount + 1];
			++iCount;

			ul.m_Position.x = x + iTotalWidth;
			ul.m_Position.y = y;
			lr.m_Position.x = ul.m_Position.x + iWide;
			lr.m_Position.y = ul.m_Position.y + iTall;

			// Gets at the texture coords for this character in its texture page
			ul.m_TexCoord[0] = texCoords[0];
			ul.m_TexCoord[1] = texCoords[1];
			lr.m_TexCoord[0] = texCoords[2];
			lr.m_TexCoord[1] = texCoords[3];
		}

		iTotalWidth += iWide + abcC;
	}

	// Draw any left-over characters
	if (iCount)
	{
		IMaterial *pMaterial = TextureDictionary()->GetTextureMaterial(iLastTexId);
		InternalSetMaterial( pMaterial );
		DrawQuadArray( iCount, pQuads, m_DrawTextColor );
	}

	m_pDrawTextPos[0] += iTotalWidth;

	stackfree(pQuads);
}


//-----------------------------------------------------------------------------
// Returns the screen size
//-----------------------------------------------------------------------------
void CMatSystemSurface::GetScreenSize(int &iWide, int &iTall)
{
	int x, y;
	g_pMaterialSystem->GetViewport( x, y, iWide, iTall );
}


//-----------------------------------------------------------------------------
// Returns the size of the embedded panel
//-----------------------------------------------------------------------------
void CMatSystemSurface::GetWorkspaceBounds(int &x, int &y, int &iWide, int &iTall)
{
	// NOTE: This is equal to the viewport size by default,
	// but other embedded panels can be used
	x = m_WorkSpaceInsets[0];
	y = m_WorkSpaceInsets[1];
	g_pIPanel->GetSize(m_pEmbeddedPanel, iWide, iTall);

	iWide -= m_WorkSpaceInsets[2];
	iTall -= m_WorkSpaceInsets[3];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : left - 
//			top - 
//			right - 
//			bottom - 
//-----------------------------------------------------------------------------
void CMatSystemSurface::SetWorkspaceInsets( int left, int top, int right, int bottom )
{
	m_WorkSpaceInsets[0] = left;
	m_WorkSpaceInsets[1] = top;
	m_WorkSpaceInsets[2] = right;
	m_WorkSpaceInsets[3] = bottom;
}

//-----------------------------------------------------------------------------
// A bunch of methods needed for the windows version only
//-----------------------------------------------------------------------------
void CMatSystemSurface::SetAsTopMost(VPANEL panel, bool state)
{
}

void CMatSystemSurface::SetAsToolBar(VPANEL panel, bool state)		// removes the window's task bar entry (for context menu's, etc.)
{
}

void CMatSystemSurface::SetForegroundWindow (VPANEL panel)
{
	BringToFront(panel);
}

void CMatSystemSurface::SetPanelVisible(VPANEL panel, bool state)
{
}

void CMatSystemSurface::SetMinimized(VPANEL panel, bool state)
{
	if (state)
	{
		g_pIPanel->SetPlat(panel, VPANEL_MINIMIZED);
		g_pIPanel->SetVisible(panel, false);
	}
	else
	{
		g_pIPanel->SetPlat(panel, VPANEL_NORMAL);
	}
}

bool CMatSystemSurface::IsMinimized(vgui::VPANEL panel)
{
	return (g_pIPanel->Plat(panel) == VPANEL_MINIMIZED);

}

void CMatSystemSurface::FlashWindow(VPANEL panel, bool state)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *panel - 
//			*title - 
//-----------------------------------------------------------------------------
void CMatSystemSurface::SetTitle(VPANEL panel, const wchar_t *title)
{
	int entry = GetTitleEntry( panel );
	if ( entry == -1 )
	{
		entry = m_Titles.AddToTail();
	}

	TitleEntry *e = &m_Titles[ entry ];
	Assert( e );
	wcsncpy( e->title, title, sizeof( e->title )/ sizeof( wchar_t ) );
	e->panel = panel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *panel - 
// Output : char const
//-----------------------------------------------------------------------------
wchar_t const *CMatSystemSurface::GetTitle( VPANEL panel )
{
	int entry = GetTitleEntry( panel );
	if ( entry != -1 )
	{
		TitleEntry *e = &m_Titles[ entry ];
		return e->title;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Private lookup method
// Input  : *panel - 
// Output : TitleEntry
//-----------------------------------------------------------------------------
int CMatSystemSurface::GetTitleEntry( vgui::VPANEL panel )
{
	for ( int i = 0; i < m_Titles.Count(); i++ )
	{
		TitleEntry* entry = &m_Titles[ i ];
		if ( entry->panel == panel )
			return i;
	}
	return -1;
}

void CMatSystemSurface::SwapBuffers(VPANEL panel)
{
}

void CMatSystemSurface::Invalidate(VPANEL panel)
{
}

void CMatSystemSurface::ApplyChanges()
{
}

// notify icons?!?
VPANEL CMatSystemSurface::GetNotifyPanel()
{
	return NULL;
}

void CMatSystemSurface::SetNotifyIcon(VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text)
{
}

bool CMatSystemSurface::IsWithin(int x, int y)
{
	return true;
}

bool CMatSystemSurface::ShouldPaintChildPanel(VPANEL childPanel)
{
	if (m_pRestrictedPanel && !g_pIPanel->HasParent(childPanel, m_pRestrictedPanel))
		return false;

	bool isPopup = ipanel()->IsPopup(childPanel);

	if ( isPopup )
	{
		
		HPanel p = ivgui()->PanelToHandle( childPanel );

		int index = m_PopupList.Find( p );
		if ( index != -1 )
		{
			ipanel()->Render_SetPopupVisible(childPanel, true);
		}
	}

	return !isPopup;
}

bool CMatSystemSurface::RecreateContext(VPANEL panel)
{
	return false;
}


//-----------------------------------------------------------------------------
// Focus-related methods
//-----------------------------------------------------------------------------
bool CMatSystemSurface::HasFocus()
{
	return true;
}

void CMatSystemSurface::BringToFront(VPANEL panel)
{
	// move panel to top of list
	g_pIPanel->MoveToFront(panel);

	// move panel to top of popup list
	if ( g_pIPanel->IsPopup( panel ) )
	{
		MovePopupToFront( panel );
	}
}


// engine-only focus handling (replacing WM_FOCUS windows handling)
void CMatSystemSurface::SetTopLevelFocus(VPANEL pSubFocus)
{
	// walk up the hierarchy until we find what popup panel belongs to
	while (pSubFocus)
	{
		if (ipanel()->IsPopup(pSubFocus) && ipanel()->IsMouseInputEnabled(pSubFocus))
		{
			BringToFront(pSubFocus);
			break;
		}
		
		pSubFocus = ipanel()->GetParent(pSubFocus);
	}
}


//-----------------------------------------------------------------------------
// Installs a function to play sounds
//-----------------------------------------------------------------------------
void CMatSystemSurface::InstallPlaySoundFunc( PlaySoundFunc_t soundFunc )
{
	m_PlaySoundFunc = soundFunc;
}


//-----------------------------------------------------------------------------
// plays a sound
//-----------------------------------------------------------------------------
void CMatSystemSurface::PlaySound(const char *pFileName)
{
	if (m_PlaySoundFunc)
		m_PlaySoundFunc( pFileName );
}


//-----------------------------------------------------------------------------
// handles mouse movement
//-----------------------------------------------------------------------------
void CMatSystemSurface::SetCursorPos(int x, int y)
{
	CursorSetPos( m_HWnd, x, y );
}

void CMatSystemSurface::GetCursorPos(int &x, int &y)
{
	CursorGetPos( m_HWnd, x, y );
}

void CMatSystemSurface::SetCursor(HCursor hCursor)
{
	_currentCursor = hCursor;
	CursorSelect( hCursor );
}

void CMatSystemSurface::EnableMouseCapture(VPANEL panel, bool state)
{
}

//-----------------------------------------------------------------------------
// Purpose: Turns the panel into a standalone window
//-----------------------------------------------------------------------------
void CMatSystemSurface::CreatePopup(VPANEL panel, bool minimized,  bool showTaskbarIcon, bool disabled , bool mouseInput , bool kbInput)
{
	if (!g_pIPanel->GetParent(panel))
	{
		g_pIPanel->SetParent(panel, GetEmbeddedPanel());
	}
	((VPanel *)panel)->SetPopup(true);
	((VPanel *)panel)->SetKeyBoardInputEnabled(kbInput);
	((VPanel *)panel)->SetMouseInputEnabled(mouseInput);


	HPanel p = ivgui()->PanelToHandle( panel );

	if ( m_PopupList.Find( p ) == m_PopupList.InvalidIndex() )
	{
		m_PopupList.AddToTail( p );
	}
	else
	{
		MovePopupToFront( panel );
	}
}


//-----------------------------------------------------------------------------
// Create/destroy panels..
//-----------------------------------------------------------------------------
void CMatSystemSurface::AddPanel(VPANEL panel)
{
	if (g_pIPanel->IsPopup(panel))
	{
		// turn it into a popup menu
		CreatePopup(panel, false);
	}
}

void CMatSystemSurface::ReleasePanel(VPANEL panel)
{
	// Remove from popup list if needed and remove any dead popups while we're at it
	RemovePopup( panel );

	int entry = GetTitleEntry( panel );
	if ( entry != -1 )
	{
		m_Titles.Remove( entry );
	}
}


//-----------------------------------------------------------------------------
// Popup accessors used by VGUI
//-----------------------------------------------------------------------------
int CMatSystemSurface::GetPopupCount(  )
{
	return m_PopupList.Count();
}

VPANEL CMatSystemSurface::GetPopup(  int index )
{

	HPanel p = m_PopupList[ index ];

	VPANEL panel = ivgui()->HandleToPanel( p );

	return panel;
}

void CMatSystemSurface::ResetPopupList(  )
{
	m_PopupList.RemoveAll();
}

void CMatSystemSurface::AddPopup( VPANEL panel )
{
	HPanel p = ivgui()->PanelToHandle( panel );

	if ( m_PopupList.Find( p ) == m_PopupList.InvalidIndex() )
	{
		m_PopupList.AddToTail( p );
	}
}


void CMatSystemSurface::RemovePopup( vgui::VPANEL panel )
{
	// Remove from popup list if needed and remove any dead popups while we're at it
	int c = GetPopupCount();

	for ( int i = c -  1; i >= 0 ; i-- )
	{
		VPANEL popup = GetPopup(i );
		if ( popup && ( popup != panel ) )
			continue;

		m_PopupList.Remove( i );
	}
}

//-----------------------------------------------------------------------------
// Methods associated with iterating + drawing the panel tree
//-----------------------------------------------------------------------------
void CMatSystemSurface::AddPopupsToList( VPANEL panel )
{
	if (!g_pIPanel->IsVisible(panel))
		return;

	// Add to popup list as we visit popups
	// Note:  popup list is cleared in RunFrame which occurs before this call!!!
	if ( g_pIPanel->IsPopup( panel ) )
	{
		AddPopup( panel );
	}

	int count = g_pIPanel->GetChildCount(panel);
	for (int i = 0; i < count; ++i)
	{
		VPANEL child = g_pIPanel->GetChild(panel, i);
		AddPopupsToList( child );
	}
}


//-----------------------------------------------------------------------------
// Purpose: recurses the panels calculating absolute positions
//			parents must be solved before children
//-----------------------------------------------------------------------------
void CMatSystemSurface::InternalSolveTraverse(VPANEL panel)
{
	// solve the parent
	((VPanel *)panel)->Solve();
	
	// now we can solve the children
	for (int i = 0; i < ((VPanel *)panel)->GetChildCount(); i++)
	{
		VPanel *child = ((VPanel *)panel)->GetChild(i);
		if (child->IsVisible())
		{
			InternalSolveTraverse((VPANEL)child);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: recurses the panels giving them a chance to do a user-defined think,
//			PerformLayout and ApplySchemeSettings
//			must be done child before parent
//-----------------------------------------------------------------------------
void CMatSystemSurface::InternalThinkTraverse(VPANEL panel)
{
	// think the parent
	((VPanel *)panel)->Client()->Think();

	// and then the children...
	for (int i = 0; i < ((VPanel *)panel)->GetChildCount(); i++)
	{
		VPanel *child = ((VPanel *)panel)->GetChild(i);
		if (child->IsVisible())
		{
			InternalThinkTraverse((VPANEL)child);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: recurses the panels giving them a chance to do apply settings,
//-----------------------------------------------------------------------------
void CMatSystemSurface::InternalSchemeSettingsTraverse(VPANEL panel, bool forceApplySchemeSettings)
{
	// apply to the children...
	for (int i = 0; i < ((VPanel *)panel)->GetChildCount(); i++)
	{

		VPanel *child = ((VPanel *)panel)->GetChild(i);
		if ( forceApplySchemeSettings || child->IsVisible() )
		{	
			InternalSchemeSettingsTraverse((VPANEL)child, forceApplySchemeSettings);
		}
	}
	// and then the parent
	((VPanel *)panel)->Client()->PerformApplySchemeSettings();
}

//-----------------------------------------------------------------------------
// Purpose: Walks through the panel tree calling Solve() on them all, in order
//-----------------------------------------------------------------------------
void CMatSystemSurface::SolveTraverse(VPANEL panel, bool forceApplySchemeSettings)
{
	{
		VPROF( "InternalSchemeSettingsTraverse" );
		InternalSchemeSettingsTraverse(panel, forceApplySchemeSettings);
	}

	{
		VPROF( "InternalThinkTraverse" );
		InternalThinkTraverse(panel);
	}

	{
		VPROF( "InternalSolveTraverse" );
		InternalSolveTraverse(panel);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Restricts rendering to a single panel
//-----------------------------------------------------------------------------
void CMatSystemSurface::RestrictPaintToSinglePanel(VPANEL panel)
{
	m_pRestrictedPanel = panel;
}


void CMatSystemSurface::PaintTraverse(VPANEL panel)
{
	VPROF( "CMatSystemSurface::PaintTraverse" );
	MEASURE_TIMED_STAT( VGUIMATSURFACE_STATS_PAINTTRAVERSE_TIME );
	bool topLevelDraw = false;

	if ( g_bInDrawing == false )
	{
		topLevelDraw = true;
		StartDrawing();
	}

	if (!ipanel()->IsVisible(panel))
		return;

	if (panel == GetEmbeddedPanel())
	{
		if ( m_pRestrictedPanel ) // only paint the restricted panel
		{
			ipanel()->PaintTraverse(m_pRestrictedPanel, true);
		}
		else
		{
			int i;
			{
				VPROF( "CMatSystemSurface::PaintTraverse loop 1" );
				for ( i = 0; i < GetPopupCount(); i++)
				{
					ipanel()->Render_SetPopupVisible( GetPopup( i ), false );
				}
			}
			{
				VPROF( "ipanel()->PaintTraverse" );
				ipanel()->PaintTraverse(panel, true);
			}


			{
				VPROF( "CMatSystemSurface::PaintTraverse loop 2" );
				for ( i = 0; i < GetPopupCount(); i++)
				{
					if ( ipanel()->Render_GetPopupVisible( GetPopup( i ) ) )
					{ 
						ipanel()->PaintTraverse(GetPopup( i ), true);
					}
				}
			}
		}
	}
	else
	{
		ipanel()->PaintTraverse(panel, true);
	}

	if ( topLevelDraw )
	{
		VPROF( "FinishDrawing" );	
		FinishDrawing();
	}
}


//-----------------------------------------------------------------------------
// Begins, ends 3D painting from within a panel paint() method
//-----------------------------------------------------------------------------
void CMatSystemSurface::Begin3DPaint( int iLeft, int iTop, int iRight, int iBottom )
{
	Assert( iRight > iLeft );
	Assert( iBottom > iTop );

	// Can't use this feature when drawing into the 3D world
	Assert( !m_bDrawingIn3DWorld );
	Assert( !m_bIn3DPaintMode );
	m_bIn3DPaintMode = true;

	// Save off the matrices in case the painting method changes them.
	g_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	g_pMaterialSystem->PushMatrix();

	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	g_pMaterialSystem->PushMatrix();

	g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	g_pMaterialSystem->PushMatrix();

	// FIXME: Set the viewport to match the clip rectangle?
	// Set the viewport to match the scissor rectangle
	g_pMaterialSystem->Viewport( m_pSurfaceExtents[0] + m_nTranslateX + iLeft, 
		m_pSurfaceExtents[1] + m_nTranslateY + iTop, iRight - iLeft, iBottom - iTop );
}

void CMatSystemSurface::End3DPaint()
{
	// Can't use this feature when drawing into the 3D world
	Assert( !m_bDrawingIn3DWorld );
	Assert( m_bIn3DPaintMode );
	m_bIn3DPaintMode = false;

	// Restore the matrices
	g_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	g_pMaterialSystem->PopMatrix();

	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	g_pMaterialSystem->PopMatrix();

	g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	g_pMaterialSystem->PopMatrix();

	// Restore the viewport (it was stored off in StartDrawing)
	g_pMaterialSystem->Viewport( m_pSurfaceExtents[0], m_pSurfaceExtents[1],
		m_pSurfaceExtents[2] - m_pSurfaceExtents[0], m_pSurfaceExtents[3] - m_pSurfaceExtents[1] );

	// ReSet the material state
	InternalSetMaterial(NULL);
}


//-----------------------------------------------------------------------------
// Some drawing methods that cannot be accomplished under Win32
//-----------------------------------------------------------------------------
#define CIRCLE_POINTS		360

void CMatSystemSurface::DrawColoredCircle( int centerx, int centery, float radius, int r, int g, int b, int a )
{
	Assert( g_bInDrawing );
	// Draw a circle
	int iDegrees = 0;
	Vector vecPoint, vecLastPoint(0,0,0);
	vecPoint.z = 0.0f;
	Color clr;
	clr.SetColor( r, g, b, a );
	DrawSetColor( clr );

	for ( int i = 0; i < CIRCLE_POINTS; i++ )
	{
		float flRadians = DEG2RAD( iDegrees );
		iDegrees += (360 / CIRCLE_POINTS);

		float ca = cos( flRadians );
		float sa = sin( flRadians );
					 
		// Rotate it around the circle
		vecPoint.x = centerx + (radius * sa);
		vecPoint.y = centery - (radius * ca);

		// Draw the point, if it's not on the previous point, to avoid smaller circles being brighter
		if ( vecLastPoint != vecPoint )
		{
			DrawFilledRect( vecPoint.x, vecPoint.y,  vecPoint.x + 1, vecPoint.y + 1 );
		}

		vecLastPoint = vecPoint;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws colored text to a vgui panel
// Input  : *font - font to use
//			x - position of text
//			y - 
//			r - color of text
//			g - 
//			b - 
//			a - alpha ( 255 = opaque, 0 = transparent )
//			*fmt - va_* text string
//			... - 
// Output : int - horizontal # of pixels drawn
//-----------------------------------------------------------------------------
int CMatSystemSurface::DrawColoredText( vgui::HFont font, int x, int y, int r, int g, int b, int a, char *fmt, va_list argptr )
{
	Assert( g_bInDrawing );
	int len;
	char data[ 1024 ];

	DrawSetTextPos( x, y );
	DrawSetTextColor( r, g, b, a );

	len = Q_vsnprintf(data, 1024, fmt, argptr);

	DrawSetTextFont( font );

	wchar_t szconverted[ 1024 ];
	vgui::localize()->ConvertANSIToUnicode( data, szconverted, 1024 );
	
	DrawPrintText( szconverted, wcslen(szconverted ) );

	int totalLength = DrawTextLen( font, data );

	return x + totalLength;
}

int CMatSystemSurface::DrawColoredText( vgui::HFont font, int x, int y, int r, int g, int b, int a, char *fmt, ... )
{
	va_list argptr;
	va_start( argptr, fmt );
	int ret = DrawColoredText( font, x, y, r, g, b, a, fmt, argptr );
	va_end(argptr);
	return ret;
}


//-----------------------------------------------------------------------------
// Draws text with current font at position and wordwrapped to the rect using color values specified
//-----------------------------------------------------------------------------
void CMatSystemSurface::SearchForWordBreak( vgui::HFont font, char *text, int& chars, int& pixels )
{
	chars = pixels = 0;
	while ( 1 )
	{
		char ch = text[ chars ];
		int a, b, c;
		GetCharABCwide( font, ch, a, b, c );

		if ( ch == 0 || ch <= 32 )
		{
			if ( ch == 32 && chars == 0 )
			{
				pixels += ( b + c );
				chars++;
			}
			break;
		}

		pixels += ( b + c );
		chars++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: If text width is specified, reterns height of text at that width
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawTextHeight( vgui::HFont font, int w, int& h, char *fmt, ... )
{
	if ( !font )
		return;

	int len;
	char data[ 8192 ];

	va_list argptr;
	va_start( argptr, fmt );
	len = Q_vsnprintf( data, 8192, fmt, argptr );
	va_end( argptr );

	int x = 0;
	int y = 0;

	int ystep = GetFontTall( font );
	int startx = x;
	int endx = x + w;
	//int endy = y + h;
	int endy = 0;

	int chars = 0;
	int pixels = 0;
	for ( int i = 0 ; i < len; i += chars )
	{
		SearchForWordBreak( font, &data[ i ], chars, pixels );

		if ( data[ i ] == '\n' )
		{
			x = startx;
			y += ystep;
			chars = 1;
			continue;
		}

		if ( x + ( pixels ) >= endx )
		{
			x = startx;
			// No room even on new line!!!
			if ( x + pixels >= endx )
				break;

			y += ystep;
		}

		for ( int j = 0 ; j < chars; j++ )
		{
			int a, b, c;
			char ch = data[ i + j ];

			GetCharABCwide( font, ch, a, b, c );
	
			x += a + b + c;
		}
	}

	endy = y+ystep;

	h = endy;
}


//-----------------------------------------------------------------------------
// Draws text with current font at position and wordwrapped to the rect using color values specified
//-----------------------------------------------------------------------------
void CMatSystemSurface::DrawColoredTextRect( vgui::HFont font, int x, int y, int w, int h, int r, int g, int b, int a, char *fmt, ... )
{
	Assert( g_bInDrawing );
	if ( !font )
		return;

	int len;
	char data[ 8192 ];

	va_list argptr;
	va_start( argptr, fmt );
	len = Q_vsnprintf( data, 8192, fmt, argptr );
	va_end( argptr );

	DrawSetTextPos( x, y );
	DrawSetTextColor( r, g, b, a );
	DrawSetTextFont( font );

	int ystep = GetFontTall( font );
	int startx = x;
	int endx = x + w;
	int endy = y + h;

	int chars = 0;
	int pixels = 0;

	char word[ 512 ];
	char space[ 2 ];
	space[1] = 0;
	space[0] = ' ';

	for ( int i = 0 ; i < len; i += chars )
	{
		SearchForWordBreak( font, &data[ i ], chars, pixels );

		if ( data[ i ] == '\n' )
		{
			x = startx;
			y += ystep;
			chars = 1;
			continue;
		}

		if ( x + ( pixels ) >= endx )
		{
			x = startx;
			// No room even on new line!!!
			if ( x + pixels >= endx )
				break;

			y += ystep;
		}

		if ( y + ystep >= endy )
			break;

		if ( x == startx )
		{
			while ( data[ i ] && data[ i ] <= 32 && chars > 0 )
			{
				i++;
				chars--;
			}
		}

		if ( chars <= 0 )
			continue;

		Q_strncpy( word, &data[ i ], chars + 1 );

		DrawSetTextPos( x, y );

		wchar_t szconverted[ 1024 ];
		vgui::localize()->ConvertANSIToUnicode( word, szconverted, 1024 );
		DrawPrintText( szconverted, wcslen(szconverted ) );

		// Leave room for space, too
		x += ( DrawTextLen( font, word ) + DrawTextLen( font, space ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Determine length of text string
//-----------------------------------------------------------------------------
int	CMatSystemSurface::DrawTextLen( vgui::HFont font, char *fmt, ... )
{
	va_list argptr;
	char data[ 1024 ];
	int len;

	va_start(argptr, fmt);
	len = Q_vsnprintf(data, 1024, fmt, argptr);
	va_end(argptr);

	int i;
	int x = 0;

	for ( i = 0 ; i < len; i++ )
	{
		int a, b, c;
		GetCharABCwide( font, data[i], a, b, c );

		// Ignore a
		if ( i != 0 )
			x += a;
		x += b;
		if ( i != len - 1 )
			x += c;
	}

	return x;
}

//-----------------------------------------------------------------------------
// Disable clipping during rendering
//-----------------------------------------------------------------------------
void CMatSystemSurface::DisableClipping( bool bDisable )
{
	EnableScissor( !bDisable );
}


//-----------------------------------------------------------------------------
// Purpose: unlocks the cursor state
//-----------------------------------------------------------------------------
bool CMatSystemSurface::IsCursorLocked() const
{
	return ::IsCursorLocked();
}


//-----------------------------------------------------------------------------
// Sets the mouse Get + Set callbacks
//-----------------------------------------------------------------------------
void CMatSystemSurface::SetMouseCallbacks( GetMouseCallback_t GetFunc, SetMouseCallback_t SetFunc )
{
	CursorSetMouseCallbacks( GetFunc, SetFunc );
}


//-----------------------------------------------------------------------------
// Tells the surface to ignore windows messages
//-----------------------------------------------------------------------------
void CMatSystemSurface::EnableWindowsMessages( bool bEnable )
{
	EnableInput( bEnable );
}


#if 0



//-----------------------------------------------------------------------------
// Purpose: This is sort of a hack, but it works.  When this object is in scope, it
//  Sets scr_disabled_for_loading to true, thereby preventing vgui fonts with multiple font pages
//  from drawing with bogus font pages for the not-yet-cached pages
//-----------------------------------------------------------------------------
extern qboolean scr_disabled_for_loading;
class CScreenDisable
{
public:
	CScreenDisable( void )
	{
		// Remember value
		oldValue = scr_disabled_for_loading;
		scr_disabled_for_loading = true;
	}

	~CScreenDisable( void )
	{
		// Restore
		scr_disabled_for_loading = oldValue;
	}
private:
	int			oldValue;
};


//-----------------------------------------------------------------------------
void CMatSystemSurface::EnableMouseCapture(bool state)
{
//	_CVGuiMatSurface->enableMouseCapture(state);
}

void CMatSystemSurface::EnableMouseCapture(vgui::VPANEL panel, bool state)
{
	//!! undone
}


#endif

void CMatSystemSurface::MovePopupToFront(VPANEL panel)
{
	HPanel p = ivgui()->PanelToHandle( panel );

	int index = m_PopupList.Find( p );
	if ( index == m_PopupList.InvalidIndex() )
	{
		return;
	}

	m_PopupList.Remove( index );
	m_PopupList.AddToTail( p );
}

void CMatSystemSurface::MovePopupToBack(VPANEL panel)
{
	HPanel p = ivgui()->PanelToHandle( panel );

	int index = m_PopupList.Find( p );
	if ( index == m_PopupList.InvalidIndex() )
	{
		return;
	}

	m_PopupList.Remove( index );
	m_PopupList.AddToHead( p );
}


bool CMatSystemSurface::IsInThink( VPANEL panel)
{
	if ( m_bInThink )
	{
		if ( panel == m_CurrentThinkPanel ) // HasParent() returns true if you pass yourself in
		{
			return false;
		}

		return ipanel()->HasParent( panel, m_CurrentThinkPanel);
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMatSystemSurface::IsCursorVisible()
{
	return (_currentCursor != dc_none);
}


bool CMatSystemSurface::IsTextureIDValid(int id)
{
	// FIXME:
	return true;
}

IHTML *CMatSystemSurface::CreateHTMLWindow(vgui::IHTMLEvents *events,VPANEL context)
{
#if defined( ENABLE_HTMLWINDOW )
	HtmlWindow *IE = new HtmlWindow(events,context,reinterpret_cast<HWND>(m_HWnd));
	IE->Show(false);
	_htmlWindows.AddToTail(IE);
	return dynamic_cast<IHTML *>(IE);
#else
	Assert( 0 );
	return NULL;
#endif
}


void CMatSystemSurface::DeleteHTMLWindow(IHTML *htmlwin)
{
#if defined( ENABLE_HTMLWINDOW )
	HtmlWindow *IE =static_cast<HtmlWindow *>(htmlwin);

	if(IE)
	{
		_htmlWindows.FindAndRemove( IE );
		delete IE;
	}
#else
#error "GameUI now NEEDS the HTML component!!"
#endif
}



void CMatSystemSurface::PaintHTMLWindow(IHTML *htmlwin)
{
#if defined( ENABLE_HTMLWINDOW )
	HtmlWindow *IE = static_cast<HtmlWindow *>(htmlwin);
	if(IE)
	{
		//HBITMAP bits;
		HDC hdc = ::GetDC(reinterpret_cast<HWND>(m_HWnd));
		IE->OnPaint(hdc);
	}
#endif
}

void CMatSystemSurface::DrawSetTextureRGBA(int id,const unsigned char* rgba,int wide,int tall,int hardwareFilter, bool forceUpload)
{
	TextureDictionary()->SetTextureRGBA( id, (const char *)rgba, wide, tall );
}

void CMatSystemSurface::DrawSetSubTextureRGBA(int textureID, int drawX, int drawY, unsigned const char *rgba, int subTextureWide, int subTextureTall)
{
	TextureDictionary()->SetSubTextureRGBA( textureID, drawX, drawY, rgba, subTextureWide, subTextureTall );
}

void CMatSystemSurface::SetModalPanel(VPANEL )
{
}

VPANEL CMatSystemSurface::GetModalPanel()
{
return 0;
}

void CMatSystemSurface::UnlockCursor()
{
	::LockCursor( false );
}

void CMatSystemSurface::LockCursor()
{
	::LockCursor( true );
}

void CMatSystemSurface::SetTranslateExtendedKeys(bool state)
{
}

VPANEL CMatSystemSurface::GetTopmostPopup()
{
	return 0;
}




//-----------------------------------------------------------------------------
// Purpose: gets the absolute coordinates of the screen (in screen space)
//-----------------------------------------------------------------------------
void CMatSystemSurface::GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall)
{
	// always work in full window screen space
	x = 0;
	y = 0;
	GetScreenSize(wide, tall);
}


void CMatSystemSurface::CalculateMouseVisible()
{
	int i;
	_needMouse = false;
	_needKB = false;

	for(i = 0 ; i < surface()->GetPopupCount() ; i++ )
	{
		VPanel *pop = (VPanel *)surface()->GetPopup(i) ;
		
		bool isVisible=pop->IsVisible();
		VPanel *p= pop->GetParent();

		while(p && isVisible)
		{
			if( p->IsVisible()==false)
			{
				isVisible=false;
				break;
			}
			p=p->GetParent();
		}
	
		if(isVisible)
		{
			_needMouse |= pop->IsMouseInputEnabled();
			_needKB |= pop->IsKeyBoardInputEnabled();
		}
	}

	if(_needMouse)
	{
		if ( _currentCursor == vgui::dc_none )
			SetCursor(vgui::dc_arrow);

		UnlockCursor();
	}
	else
	{
		SetCursor(vgui::dc_none);
		LockCursor();
	}
}


bool CMatSystemSurface::NeedKBInput()
{
	return _needKB;
}
void CMatSystemSurface::SurfaceGetCursorPos(int &x, int &y)
{
	GetCursorPos( x, y );
}
void CMatSystemSurface::SurfaceSetCursorPos(int x, int y)
{
	SetCursorPos( x, y );
}


