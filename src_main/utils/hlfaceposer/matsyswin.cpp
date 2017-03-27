/*

  glapp.c - Simple OpenGL shell
  
	There are several options allowed on the command line.  They are:
	-height : what window/screen height do you want to use?
	-width  : what window/screen width do you want to use?
	-bpp    : what color depth do you want to use?
	-window : create a rendering window rather than full-screen
	-fov    : use a field of view other than 90 degrees
*/


//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           MatSysWindow.cpp
// last modified:  May 04 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
// version:        1.2
//
// email:          mete@swissquake.ch
// web:            http://www.swissquake.ch/chumbalum-soft/
//
#include <mx/mx.h>
#include <mx/mxMessageBox.h>
#include <mx/mxTga.h>
#include <mx/mxPcx.h>
#include <mx/mxBmp.h>
#include <mx/mxMatSysWindow.h>
// #include "gl.h"
// #include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "MatSysWin.h"
#include "MDLViewer.h"
#include "StudioModel.h"
#include "ControlPanel.h"
#include "ViewerSettings.h"
#include "cmdlib.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialproxyfactory.h"
#include "FileSystem.h"
#include "FileSystem_Tools.h"
#include "materialsystem/IMesh.h"
#include "expressions.h"
#include "hlfaceposer.h"
#include "ifaceposersound.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "materialsystem/ITexture.h"
#include "materialsystem/MaterialSystem_Config.h"
#include "istudiorender.h"
#include "choreowidgetdrawhelper.h"
#include "faceposer_models.h"
#include "vstdlib/icommandline.h"

IFileSystem *filesystem = NULL;

extern char g_appTitle[];

// hack - this should go somewhere else.
CreateInterfaceFn g_MaterialSystemClientFactory = NULL;
CreateInterfaceFn g_MaterialSystemFactory = NULL;

// FIXME: move all this to mxMatSysWin

class DummyMaterialProxyFactory : public IMaterialProxyFactory
{
public:
	virtual IMaterialProxy *CreateProxy( const char *proxyName )	{return NULL;}
	virtual void DeleteProxy( IMaterialProxy *pProxy )				{}
};
DummyMaterialProxyFactory	g_DummyMaterialProxyFactory;


static void ReleaseMaterialSystemObjects()
{
	StudioModel::ReleaseStudioModel();
	models->ReleaseModels();
}

static void RestoreMaterialSystemObjects()
{
	StudioModel::RestoreStudioModel();
	models->RestoreModels();
}

void InitMaterialSystemConfig(MaterialSystem_Config_t *pConfig)
{
	memset( pConfig, 0, sizeof(MaterialSystem_Config_t) );
	pConfig->screenGamma = 2.2f;
	pConfig->texGamma = 2.2;
	pConfig->overbright = 2;
	pConfig->bAllowCheats = false;
	pConfig->bLinearFrameBuffer = false;
	pConfig->polyOffset = 4;
	pConfig->skipMipLevels = 0;
	pConfig->lightScale = 1.0f;
	pConfig->bFilterLightmaps = true;
	pConfig->bFilterTextures = true;
	pConfig->bMipMapTextures = true;
	pConfig->bShowMipLevels = false;
	pConfig->bReverseDepth = false;
	pConfig->bCompressedTextures = false;
	pConfig->bBumpmap = true;
	pConfig->bShowSpecular = true;
	pConfig->bShowDiffuse = true;
	pConfig->maxFrameLatency = 1;
	pConfig->bDrawFlat = false;
	pConfig->bLightingOnly = false;
	pConfig->bSoftwareLighting = false;
	pConfig->bEditMode = false;	// No, we're not in WorldCraft.
	pConfig->m_bForceTrilinear = false;
	pConfig->m_nForceAnisotropicLevel = 0;
	pConfig->m_bForceBilinear = false;
}

IMaterialSystem *g_pMaterialSystem;
IMaterialSystemHardwareConfig *g_pMaterialSystemHardwareConfig;
Vector g_vright( 50, 50, 0 );		// needs to be set to viewer's right in order for chrome to work

IMaterial *g_materialBackground = NULL;
IMaterial *g_materialWireframe = NULL;
IMaterial *g_materialFlatshaded = NULL;
IMaterial *g_materialSmoothshaded = NULL;
IMaterial *g_materialBones = NULL;
IMaterial *g_materialLines = NULL;
IMaterial *g_materialFloor = NULL;

MatSysWindow		*g_pMatSysWindow = 0;

#define MATSYSWIN_NAME "3D View"

MatSysWindow::MatSysWindow (mxWindow *parent, int x, int y, int w, int h, const char *label, int style)
: IFacePoserToolWindow( "3D View", "3D View" ), mxMatSysWindow ( parent, x, y, w, h, label, style )
{
	SetAutoProcess( true );

	setLabel( MATSYSWIN_NAME );

	m_bSuppressSwap = false;

	m_hWnd = (HWND)getHandle();


	const char *pPath = basegamedir;

	Con_Printf( "Directory:  %s\n", basegamedir );
	
	Con_Printf( "Loading materialsystem.dll\n" );

	// Load the material system DLL and get its interface.
	m_hMaterialSystemInst = LoadLibrary( "MaterialSystem.dll" );
	if( !m_hMaterialSystemInst )
	{
		Error( "Can't load MaterialSystem.dll\n" );
	}

	Con_Printf( "Getting materialsystem factory\n" );

	g_MaterialSystemFactory = Sys_GetFactory( "MaterialSystem.dll" );
	if ( g_MaterialSystemFactory )
	{
		g_pMaterialSystem = (IMaterialSystem *)g_MaterialSystemFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
		if ( !g_pMaterialSystem )
		{
			Error( "Could not get the material system interface from materialsystem.dll" );
		}
	}
	else
	{
		Error( "Could not find factory interface in library MaterialSystem.dll" );
	}

	const char *pShaderDLL = CommandLine()->ParmValue("-shaderdll");
	if(!pShaderDLL)
	{
		pShaderDLL = "shaderapidx9.dll";
	}

	if ( CommandLine()->FindParm( "-noshaderapi" ) )
	{
		pShaderDLL = "shaderapiempty.dll";
	}

	Con_Printf( "Initializing materialsystem\n" );

	if(!( g_MaterialSystemClientFactory = g_pMaterialSystem->Init(pShaderDLL, &g_DummyMaterialProxyFactory, FileSystem_GetFactory() )) )
		Error("IMaterialSystem::Init failed");
	
	g_pMaterialSystemHardwareConfig = (IMaterialSystemHardwareConfig*)
		g_MaterialSystemClientFactory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, 0 );
	if ( !g_pMaterialSystemHardwareConfig )
		return;

	Con_Printf( "Setting material system video mode\n" );

	MaterialVideoMode_t mode;
	int modeFlags = MATERIAL_VIDEO_MODE_WINDOWED | MATERIAL_VIDEO_MODE_RESIZING;
	mode.m_Width = mode.m_Height = 0;
	if (!g_pMaterialSystem->SetMode( (void*)m_hWnd, mode, modeFlags ))
		return;

	g_pMaterialSystem->AddReleaseFunc( ReleaseMaterialSystemObjects );
	g_pMaterialSystem->AddRestoreFunc( RestoreMaterialSystemObjects );

	Con_Printf( "Calling UpdateConfig\n" );

	MaterialSystem_Config_t config;
	InitMaterialSystemConfig(&config);
	g_pMaterialSystem->UpdateConfig(&config, false);
	
	Con_Printf( "Loading debug materials\n" );

	ITexture *pCubemapTexture = g_pMaterialSystem->FindTexture( "hlmv/cubemap", NULL, true );
	pCubemapTexture->IncrementReferenceCount();
	g_pMaterialSystem->BindLocalCubemap( pCubemapTexture );

	g_materialBackground	= g_pMaterialSystem->FindMaterial("particle/particleapp_background", NULL, true);
	g_materialWireframe		= g_pMaterialSystem->FindMaterial("debug/debugmrmwireframe", NULL, true);
	g_materialFlatshaded	= g_pMaterialSystem->FindMaterial("debug/debugdrawflatpolygons", NULL, true);
	g_materialSmoothshaded	= g_pMaterialSystem->FindMaterial("debug/debugmrmfullbright2", NULL, true);
	g_materialBones			= g_pMaterialSystem->FindMaterial("debug/debugmrmwireframe", NULL, true);
	g_materialLines			= g_pMaterialSystem->FindMaterial("debug/debugwireframevertexcolor", NULL, true);
	g_materialFloor			= g_pMaterialSystem->FindMaterial("hlmv/floor", NULL, true);

	if (!parent)
		setVisible (true);
	else
		mx::setIdleWindow (this);

	m_bSuppressResize = false;
}



MatSysWindow::~MatSysWindow ()
{
	mx::setIdleWindow (0);
}

void MatSysWindow::redraw()
{
	BaseClass::redraw();
return;
	if ( IsLocked() )
	{
		RECT bounds;
		GetClientRect( (HWND)getHandle(), &bounds );
		bounds.bottom = bounds.top + GetCaptionHeight();
		CChoreoWidgetDrawHelper helper( this, bounds );
		HandleToolRedraw( helper );
	}
}

void MatSysWindow::CheckForAccleratorKeys( void )
{
	if ( !GetAsyncKeyState( VK_CONTROL ) )
		return;

	if ( GetAsyncKeyState( 'v' ) || GetAsyncKeyState( 'V' ) )
	{
		g_MDLViewer->Paste();
	}
	else if ( GetAsyncKeyState( 'c' ) || GetAsyncKeyState( 'C' ) )
	{
		g_MDLViewer->Copy();
	}
	else if ( GetAsyncKeyState( 'z' ) || GetAsyncKeyState( 'Z' ) )
	{
		g_MDLViewer->Undo();
	}
	else if ( GetAsyncKeyState( 'y' ) || GetAsyncKeyState( 'Y' ) )
	{
		g_MDLViewer->Redo();
	}
}

#define MAX_FPS 250.0f
#define MIN_TIMESTEP ( 1.0f / MAX_FPS )

double realtime = 0.0f;

void MatSysWindow::Frame( void )
{
	static bool recursion_guard = false;

	static double prev = 0.0;
	double curr = (double) mx::getTickCount () / 1000.0;
	double dt = ( curr - prev );
	
	if ( recursion_guard )
		return;

	recursion_guard = true;

	// clamp to MAX_FPS
	if ( dt >= 0.0 && dt < MIN_TIMESTEP )
	{
		Sleep( max( 0, (int)( ( MIN_TIMESTEP - dt ) * 1000.0f ) ) );

		recursion_guard = false;
		return;
	}

	if ( prev != 0.0 )
	{
		dt = min( 0.1, dt );
		
		g_MDLViewer->Think( dt );

		realtime += dt;
	}
	
	prev = curr;

	DrawFrame();

	recursion_guard = false;
}

void MatSysWindow::DrawFrame( void )
{
	if (!g_viewerSettings.pause)
	{
		redraw ();
	}
}

int MatSysWindow::handleEvent (mxEvent *event)
{
	int iret = 0;
	
	if ( HandleToolEvent( event ) )
	{
		return iret;
	}

	static float oldrx = 0, oldry = 0, oldtz = 50, oldtx = 0, oldty = 0;
	static float oldlrx = 0, oldlry = 0;
	static int oldx, oldy;

	switch (event->event)
	{
	case mxEvent::Idle:
		{
			iret = 1;

			Frame();

			CheckForAccleratorKeys();
		}
		break;

	case mxEvent::MouseDown:
		{
			oldrx = g_viewerSettings.rot[0];
			oldry = g_viewerSettings.rot[1];
			oldtx = g_viewerSettings.trans[0];
			oldty = g_viewerSettings.trans[1];
			oldtz = g_viewerSettings.trans[2];
			oldx = (short)event->x;
			oldy = (short)event->y;
			oldlrx = g_viewerSettings.lightrot[0];
			oldlry = g_viewerSettings.lightrot[1];
			g_viewerSettings.pause = false;

			float r = 1.0/3.0 * min( w(), h() );

			float d = sqrt( (event->x - w()/2) * (event->x - w()/2) + (event->y - h()/2) * (event->y - h()/2) );

			if (d < r)
				g_viewerSettings.rotating = false;
			else
				g_viewerSettings.rotating = true;

			iret = 1;
		}
		break;

	case mxEvent::MouseDrag:
		{
			if (event->buttons & mxEvent::MouseLeftButton)
			{
				if (event->modifiers & mxEvent::KeyShift)
				{
					g_viewerSettings.trans[1] = oldty - (float) ((short)event->x - oldx);
					g_viewerSettings.trans[2] = oldtz + (float) ((short)event->y - oldy);
				}
				else if (event->modifiers & mxEvent::KeyCtrl)
				{
					float ry = (float) (event->y - oldy);
					float rx = (float) (event->x - oldx);
					oldx = event->x;
					oldy = event->y;
					
					QAngle movement = QAngle( ry, rx, 0 );
					
					matrix3x4_t tmp1, tmp2, tmp3;
					AngleMatrix( g_viewerSettings.lightrot, tmp1 );
					AngleMatrix( movement, tmp2 );
					ConcatTransforms( tmp2, tmp1, tmp3 );
					MatrixAngles( tmp3, g_viewerSettings.lightrot );
				}
				else
				{
					if (!g_viewerSettings.rotating)
					{
						float ry = (float) (event->y - oldy);
						float rx = (float) (event->x - oldx);
						oldx = event->x;
						oldy = event->y;
						
						QAngle movement;
						matrix3x4_t tmp1, tmp2, tmp3;
						
						movement = QAngle( 0, rx, 0 );
						AngleMatrix( g_viewerSettings.rot, tmp1 );
						AngleMatrix( movement, tmp2 );
						ConcatTransforms( tmp1, tmp2, tmp3 );
						MatrixAngles( tmp3, g_viewerSettings.rot );
						
						movement = QAngle( ry, 0, 0 );
						AngleMatrix( g_viewerSettings.rot, tmp1 );
						AngleMatrix( movement, tmp2 );
						ConcatTransforms( tmp2, tmp1, tmp3 );
						MatrixAngles( tmp3, g_viewerSettings.rot );
					}
					else
					{
						float ang1 = (180 / 3.1415) * atan2( oldx - w()/2.0, oldy - h()/2.0 );
						float ang2 = (180 / 3.1415) * atan2( event->x - w()/2.0, event->y - h()/2.0 );
						oldx = event->x;
						oldy = event->y;
						
						QAngle movement = QAngle( 0, 0, ang2 - ang1 );
						
						matrix3x4_t tmp1, tmp2, tmp3;
						AngleMatrix( g_viewerSettings.rot, tmp1 );
						AngleMatrix( movement, tmp2 );
						ConcatTransforms( tmp2, tmp1, tmp3 );
						MatrixAngles( tmp3, g_viewerSettings.rot );
					}			
				}
			}
			else if (event->buttons & mxEvent::MouseRightButton)
			{
				g_viewerSettings.trans[0] = oldtx + (float) ((short)event->y - oldy);
				g_viewerSettings.trans[0] = clamp( g_viewerSettings.trans[0], 8.0f, 1024.0f );
			}
			redraw ();

			iret = 1;
		}
		break;

	case mxEvent::KeyDown:
		{
			iret = 1;
			switch (event->key)
			{
			default:
				iret = 0;
				break;
			case 116: // F5
				{
					g_MDLViewer->Refresh();
				}
				break;
			case 32:
				{
					int iSeq = models->GetActiveStudioModel()->GetSequence ();
					if (iSeq == models->GetActiveStudioModel()->SetSequence (iSeq + 1))
					{
						models->GetActiveStudioModel()->SetSequence (0);
					}
				}
				break;
				
			case 27:
				if (!getParent ()) // fullscreen mode ?
					mx::quit ();
				break;
				
			case 'g':
				g_viewerSettings.showGround = !g_viewerSettings.showGround;
				break;
				
			case 'h':
				g_viewerSettings.showHitBoxes = !g_viewerSettings.showHitBoxes;
				break;
				
			case 'o':
				g_viewerSettings.showBones = !g_viewerSettings.showBones;
				break;
				
			case 'b':
				g_viewerSettings.showBackground = !g_viewerSettings.showBackground;
				break;
				
			case 'm':
				g_viewerSettings.showMovement = !g_viewerSettings.showMovement;
				break;
				
			case '1':
			case '2':
			case '3':
			case '4':
				g_viewerSettings.renderMode = event->key - '1';
				break;
				
			case '-':
				g_viewerSettings.speedScale -= 0.1f;
				if (g_viewerSettings.speedScale < 0.0f)
					g_viewerSettings.speedScale = 0.0f;
				break;
				
			case '+':
				g_viewerSettings.speedScale += 0.1f;
				if (g_viewerSettings.speedScale > 5.0f)
					g_viewerSettings.speedScale = 5.0f;
				break;
			}
		}
		break;
	} // switch (event->event)

	return iret;
}



void
drawFloor ()
{
	float mTempModel[4][4];
	float mTempView[4][4];
	g_pMaterialSystem->Bind(g_materialFloor);
	g_pMaterialSystem->MatrixMode(MATERIAL_MODEL);
	g_pMaterialSystem->GetMatrix(MATERIAL_MODEL, (float*)mTempModel);
	g_pMaterialSystem->LoadIdentity();
	g_pMaterialSystem->MatrixMode(MATERIAL_VIEW);
	g_pMaterialSystem->GetMatrix(MATERIAL_VIEW, (float*)mTempView);
	g_pMaterialSystem->LoadIdentity();
	{
		IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh();
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		float dist=-15000.0f;
		float tMin=0, tMax=1;
		
		meshBuilder.Position3f(-dist, dist, dist);
		meshBuilder.TexCoord2f( 0, tMin,tMax );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( dist, dist, dist);
		meshBuilder.TexCoord2f( 0, tMax,tMax );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( dist,-dist, dist);
		meshBuilder.TexCoord2f( 0, tMax,tMin );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f(-dist,-dist, dist);
		meshBuilder.TexCoord2f( 0, tMin,tMin );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
	g_pMaterialSystem->MatrixMode(MATERIAL_MODEL);
	g_pMaterialSystem->LoadMatrix((float*)mTempModel);
	g_pMaterialSystem->MatrixMode(MATERIAL_VIEW);
	g_pMaterialSystem->LoadMatrix((float*)mTempView);
}



void
setupRenderMode ()
{
}

void MatSysWindow::SuppressBufferSwap( bool bSuppress )
{
	m_bSuppressSwap = bSuppress;
}

void MatSysWindow::draw ()
{
	int i;

	g_pMaterialSystem->BeginFrame();
	CUtlVector< StudioModel * > modellist;

	modellist.AddToTail( models->GetActiveStudioModel() );

	if ( models->CountVisibleModels() > 0 )
	{
		modellist.RemoveAll();
		for ( i = 0; i < models->Count(); i++ )
		{
			if ( models->IsModelShownIn3DView( i ) )
			{
				modellist.AddToTail( models->GetStudioModel( i ) );
			}
		}
	}

	g_pMaterialSystem->ClearBuffers(true, true);

	int captiony = GetCaptionHeight();
	int viewh = h2() - captiony;

	g_pMaterialSystem->Viewport( 0, captiony, w2(), viewh );

	g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	g_pMaterialSystem->LoadIdentity( );
	g_pMaterialSystem->PerspectiveX(20.0f, (float)w2() / (float)viewh, 1.0f, 20000.0f);
	
	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	g_pMaterialSystem->LoadIdentity( );
	// FIXME: why is this needed?  Doesn't SetView() override this?
	g_pMaterialSystem->Rotate( -90,  1, 0, 0 );	    // put Z going up
	g_pMaterialSystem->Rotate( -90,  0, 0, 1 );

	ViewerSettings oldsettings = g_viewerSettings;

	int modelcount = modellist.Count();
	int countover2 = modelcount / 2;
	int ydelta = g_pControlPanel->GetModelGap();
	int yoffset = -countover2 * ydelta;
	for ( i = 0 ; i < modelcount; i++ )
	{
		g_viewerSettings.trans[ 1 ] = oldsettings.trans[ 1 ] + yoffset;
		yoffset += ydelta;

		modellist[ i ]->GetStudioRender()->BeginFrame();
		modellist[ i ]->DrawModel();
		modellist[ i ]->GetStudioRender()->EndFrame();
	}

	g_viewerSettings = oldsettings;

	//
	// draw ground
	//
	if (g_viewerSettings.showGround)
	{
		drawFloor ();
	}

	if (!m_bSuppressSwap)
	{
		g_pMaterialSystem->SwapBuffers();
	}

	g_pMaterialSystem->EndFrame();
}

void MatSysWindow::TakeSnapshotRect( const char *pFilename, int x, int y, int w, int h )
{
	int i;
	HANDLE hf;
	BITMAPFILEHEADER hdr;
	BITMAPINFOHEADER bi;
	DWORD dwTmp, imageSize;
	byte *hp, b, *pBlue, *pRed;

	w = ( w + 3 ) & ~3;

	imageSize = w * h * 3;
	// Create the file
	hf = CreateFile( pFilename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hf == INVALID_HANDLE_VALUE )
	{
		return;
	}

	// file header
	hdr.bfType = 0x4d42;	// 'BM'
	hdr.bfSize = (DWORD) ( sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize );
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;
	hdr.bfOffBits = (DWORD) ( sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) );

	if( !WriteFile( hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER), (LPDWORD) &dwTmp, NULL ) )
		Error( "Couldn't write file header to snapshot.\n" );

	// bitmap header
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = w;
	bi.biHeight = h;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;	//vid.rowbytes * vid.height;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	if( !WriteFile( hf, (LPVOID) &bi, sizeof(BITMAPINFOHEADER), (LPDWORD) &dwTmp, NULL ) )
		Error( "Couldn't write bitmap header to snapshot.\n" );

	// bitmap bits
	hp = (byte *) malloc(imageSize);
	
	if (hp == NULL)
		Error( "Couldn't allocate bitmap header to snapshot.\n" );

	// Get Bits from GL
	g_pMaterialSystem->ReadPixels( x, y, w, h, hp, IMAGE_FORMAT_RGB888 );

	// Invert vertically for BMP format
	for (i = 0; i < h / 2; i++)
	{
		byte *top = hp + i * w * 3;
		byte *bottom = hp + (h - i - 1) * w * 3;
		for (int j = 0; j < w * 3; j++)
		{
			b = *top;
			*top = *bottom;
			*bottom = b;
			top++;
			bottom++;
		}
	}

	// Reverse Red and Blue
	pRed = hp;
	pBlue = pRed + 2;
	for(i = 0; i < w * h;i++)
	{
		b = *pRed;
		*pRed = *pBlue;
		*pBlue = b;
		pBlue += 3;
		pRed += 3;
	}

	if( !WriteFile( hf, (LPVOID)hp, imageSize, (LPDWORD) &dwTmp, NULL ) )
		Error( "Couldn't write bitmap data snapshot.\n" );

	free(hp);

	// clean up
	if( !CloseHandle( hf ) )
		Error( "Couldn't close file for snapshot.\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool MatSysWindow::IsSuppressingResize( void )
{
	return m_bSuppressResize;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : suppress - 
//-----------------------------------------------------------------------------
void MatSysWindow::SuppressResize( bool suppress )
{
	m_bSuppressResize = suppress;
}
