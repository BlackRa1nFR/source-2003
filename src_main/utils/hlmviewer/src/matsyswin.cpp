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
#include "materialsystem/IMesh.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "materialsystem/ITexture.h"
#include "materialsystem/MaterialSystem_Config.h"
#include "tier0/dbg.h"
#include "istudiorender.h"
#include "vstdlib/icommandline.h"


extern char g_appTitle[];


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
}

static void RestoreMaterialSystemObjects()
{
	StudioModel::RestoreStudioModel();
}

void InitMaterialSystemConfig(MaterialSystem_Config_t *pConfig)
{
	memset( pConfig, 0, sizeof(MaterialSystem_Config_t) );
	pConfig->screenGamma = 2.2f;
	pConfig->texGamma = 2.2;
	if( g_viewerSettings.overbright )
	{
		pConfig->overbright = 2;
	}
	else
	{
		pConfig->overbright = 1;
	}
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
	pConfig->bCompressedTextures = true;
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

MatSysWindow *g_MatSysWindow = 0;
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


// hack - this should go somewhere else.
CreateInterfaceFn g_MaterialSystemClientFactory = NULL;
CreateInterfaceFn g_MaterialSystemFactory = NULL;

MatSysWindow::MatSysWindow (mxWindow *parent, int x, int y, int w, int h, const char *label, int style)
: mxMatSysWindow (parent, x, y, w, h, label, style)
{
	m_pCubemapTexture = NULL;
	m_hWnd = (HWND)getHandle();

	const char *pPath = CommandLine()->ParmValue("-game");
	if(!pPath)
	{
		// If they didn't specify -game on the command line, use VPROJECT.
		CmdLib_InitFileSystem( ".", true );
		pPath = basegamedir;
	}
	else
	{
		CmdLib_InitFileSystem( pPath, true );
		pPath = basegamedir;
	}

	
	// Load the material system DLL and get its interface.
	m_hMaterialSystemInst = LoadLibrary( "MaterialSystem.dll" );
	if( !m_hMaterialSystemInst )
	{
		return; // Sys_Error( "Can't load MaterialSystem.dll\n" );
	}

	g_MaterialSystemFactory = Sys_GetFactory( "MaterialSystem.dll" );
	if ( g_MaterialSystemFactory )
	{
		g_pMaterialSystem = (IMaterialSystem *)g_MaterialSystemFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
		if ( !g_pMaterialSystem )
		{
			return; // Sys_Error( "Could not get the material system interface from materialsystem.dll" );
		}
	}
	else
	{
		return; // Sys_Error( "Could not find factory interface in library MaterialSystem.dll" );
	}

	// Figure out the material path.
	char szMaterialPath[1024];
	sprintf(szMaterialPath, "%s/materials", pPath);
							   
	const char *pShaderDLL = CommandLine()->ParmValue("-shaderdll");
	if(!pShaderDLL)
	{
		pShaderDLL = "shaderapidx9.dll";
	}

	if(!( g_MaterialSystemClientFactory = g_pMaterialSystem->Init(pShaderDLL, &g_DummyMaterialProxyFactory, CmdLib_GetFileSystemFactory() )) )
		return; // Sys_Error("IMaterialSystem::Init failed");


	g_pMaterialSystemHardwareConfig = (IMaterialSystemHardwareConfig*)
		g_MaterialSystemClientFactory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, 0 );
	if ( !g_pMaterialSystemHardwareConfig )
		return;

	MaterialVideoMode_t mode;
	int modeFlags = MATERIAL_VIDEO_MODE_WINDOWED | MATERIAL_VIDEO_MODE_RESIZING;
//	modeFlags |= MATERIAL_VIDEO_MODE_SOFTWARE_TL; // hack
	mode.m_Width = mode.m_Height = 0;
	if (!g_pMaterialSystem->SetMode( (void*)m_hWnd, mode, modeFlags ))
		return;
	
	g_pMaterialSystem->AddReleaseFunc( ReleaseMaterialSystemObjects );
	g_pMaterialSystem->AddRestoreFunc( RestoreMaterialSystemObjects );

	MaterialSystem_Config_t config;
	InitMaterialSystemConfig(&config);
	g_pMaterialSystem->UpdateConfig(&config, false);
	
	m_pCubemapTexture = g_pMaterialSystem->FindTexture( "hlmv/cubemap", NULL, true );
	m_pCubemapTexture->IncrementReferenceCount();
	g_pMaterialSystem->BindLocalCubemap( m_pCubemapTexture );

	g_materialBackground	= g_pMaterialSystem->FindMaterial("hlmv/background", NULL, true);
	g_materialWireframe		= g_pMaterialSystem->FindMaterial("debug/debugmrmwireframe", NULL, true);
	g_materialFlatshaded	= g_pMaterialSystem->FindMaterial("debug/debugdrawflatpolygons", NULL, true);
	g_materialSmoothshaded	= g_pMaterialSystem->FindMaterial("debug/debugmrmfullbright2", NULL, true);
	g_materialBones			= g_pMaterialSystem->FindMaterial("debug/debugskeleton", NULL, true);
	g_materialLines			= g_pMaterialSystem->FindMaterial("debug/debugwireframevertexcolor", NULL, true);
	g_materialFloor			= g_pMaterialSystem->FindMaterial("hlmv/floor", NULL, true);

	if (!parent)
		setVisible (true);
	else
		mx::setIdleWindow (this);
}



MatSysWindow::~MatSysWindow ()
{
	if (m_pCubemapTexture)
		m_pCubemapTexture->DecrementReferenceCount();
	mx::setIdleWindow (0);
}



int
MatSysWindow::handleEvent (mxEvent *event)
{
	static float oldrx = 0, oldry = 0, oldtz = 50, oldtx = 0, oldty = 0;
	static float oldlrx = 0, oldlry = 0;
	static int oldx, oldy;

	switch (event->event)
	{

	case mxEvent::Idle:
	{
		static double prev;
		double curr = (double) mx::getTickCount () / 1000.0;
		double dt = (curr - prev);

		// clamp to 100fps
		if (dt >= 0.0 && dt < 0.01)
		{
			Sleep( 10 - dt * 1000.0 );
			return 1;
		}

		if ( prev != 0.0 )
		{
	//		dt = 0.001;

			g_pStudioModel->AdvanceFrame ( dt * g_viewerSettings.speedScale );
			g_ControlPanel->updateFrameSlider( );
		}
		prev = curr;

		if (!g_viewerSettings.pause)
			redraw ();

		g_ControlPanel->updateTransitionAmount();

		return 1;
	}
	break;

	case mxEvent::MouseUp:
	{
		g_viewerSettings.mousedown = false;
	}
	break;

	case mxEvent::MouseDown:
	{
		g_viewerSettings.mousedown = true;

		oldrx = g_viewerSettings.rot[0];
		oldry = g_viewerSettings.rot[1];
		oldtx = g_viewerSettings.trans[0];
		oldty = g_viewerSettings.trans[1];
		oldtz = g_viewerSettings.trans[2];
		oldx = event->x;
		oldy = event->y;
		oldlrx = g_viewerSettings.lightrot[1];
		oldlry = g_viewerSettings.lightrot[0];
		g_viewerSettings.pause = false;

		float r = 1.0/3.0 * min( w(), h() );

		float d = sqrt( (event->x - w()/2) * (event->x - w()/2) + (event->y - h()/2) * (event->y - h()/2) );

		if (d < r)
			g_viewerSettings.rotating = false;
		else
			g_viewerSettings.rotating = true;

		return 1;
	}
	break;

	case mxEvent::MouseDrag:
	{
		if (event->buttons & mxEvent::MouseLeftButton)
		{
			if (event->modifiers & mxEvent::KeyShift)
			{
				g_viewerSettings.trans[1] = oldty - (float) (event->x - oldx);
				g_viewerSettings.trans[2] = oldtz + (float) (event->y - oldy);
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

				// g_viewerSettings.lightrot[0] = oldlrx + (float) (event->y - oldy);
				// g_viewerSettings.lightrot[1] = oldlry + (float) (event->x - oldx);
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
			g_viewerSettings.trans[0] = oldtx + (float) (event->y - oldy);
		}
		redraw ();

		return 1;
	}
	break;

	case mxEvent::KeyDown:
	{
		switch (event->key)
		{
		case 116: // F5
		{
			g_MDLViewer->Refresh();
			break;
		}
		case 32:
		{
			int iSeq = g_pStudioModel->GetSequence ();
			if (iSeq == g_pStudioModel->SetSequence (iSeq + 1))
			{
				g_pStudioModel->SetSequence (0);
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

	return 1;
}



void
drawFloor ()
{
	/*
	glBegin (GL_TRIANGLE_STRIP);
	glTexCoord2f (0.0f, 0.0f);
	glVertex3f (-100.0f, 100.0f, 0.0f);

	glTexCoord2f (0.0f, 1.0f);
	glVertex3f (-100.0f, -100.0f, 0.0f);

	glTexCoord2f (1.0f, 0.0f);
	glVertex3f (100.0f, 100.0f, 0.0f);

	glTexCoord2f (1.0f, 1.0f);
	glVertex3f (100.0f, -100.0f, 0.0f);

	glEnd ();
	*/
}


void DrawBackground()
{
	if (!g_viewerSettings.showBackground)
		return;

	float mTempModel[4][4];
	float mTempView[4][4];
	g_pMaterialSystem->Bind(g_materialBackground);
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
}

void DrawHelpers()
{
	if (g_viewerSettings.mousedown)
	{
		g_pMaterialSystem->Bind( g_materialBones );

		g_pMaterialSystem->MatrixMode(MATERIAL_MODEL);
		g_pMaterialSystem->LoadIdentity();
		g_pMaterialSystem->MatrixMode(MATERIAL_VIEW);
		g_pMaterialSystem->LoadIdentity();

		IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh();

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_LINES, 1 );

		if (g_viewerSettings.rotating)
			meshBuilder.Color3ub( 255, 255, 0 );
		else
			meshBuilder.Color3ub(   0, 255, 0 );

		for (int i = 0; i < 360; i += 5)
		{
			float a = i * (3.151492653/180.0f);

			if (g_viewerSettings.rotating)
				meshBuilder.Color3ub( 255, 255, 0 );
			else
				meshBuilder.Color3ub(   0, 255, 0 );

			meshBuilder.Position3f( sin( a ), cos( a ), -3.0f );
			meshBuilder.AdvanceVertex();
		}
		meshBuilder.End();
		pMesh->Draw();
	}
}


void DrawGroundPlane()
{
	if (!g_viewerSettings.showGround)
		return;

	float mTempModel[4][4];
	float mTempView[4][4];
	g_pMaterialSystem->Bind(g_materialFloor);
	g_pMaterialSystem->MatrixMode(MATERIAL_MODEL);
	g_pMaterialSystem->GetMatrix(MATERIAL_MODEL, (float*)mTempModel);
	g_pMaterialSystem->LoadIdentity();
	g_pMaterialSystem->MatrixMode(MATERIAL_VIEW);
	g_pMaterialSystem->GetMatrix(MATERIAL_VIEW, (float*)mTempView);
	g_pMaterialSystem->LoadIdentity();

	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	g_pMaterialSystem->LoadIdentity( );

	g_pMaterialSystem->Rotate( -90,  1, 0, 0 );	    // put Z going up
	g_pMaterialSystem->Rotate( -90,  0, 0, 1 );

    g_pMaterialSystem->Translate( -g_viewerSettings.trans[0],  -g_viewerSettings.trans[1],  -g_viewerSettings.trans[2] );

	g_pMaterialSystem->Rotate( g_viewerSettings.rot[1],  0, 0, 1 );
    g_pMaterialSystem->Rotate( g_viewerSettings.rot[0],  0, 1, 0 );
    g_pMaterialSystem->Rotate( g_viewerSettings.rot[2],  1, 0, 0 );

	static Vector tMap( 0, 0, 0 );
	static Vector dxMap( 1, 0, 0 );
	static Vector dyMap( 0, 1, 0 );
	static float prevframe = 0.0f;

	float currframe = g_pStudioModel->GetCycle( );

	Vector deltaPos;
	QAngle deltaAngles;

	// assume that changes < -0.5 are loops....
	if (currframe - prevframe < -0.5)
	{
		prevframe = prevframe - 1.0;
		// assumes a max of one loop, in any case prevframe gets reset below
	}

	g_pStudioModel->GetMovement( prevframe, currframe, deltaPos, deltaAngles );
	prevframe = currframe;

	IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	float scale = 10.0;
	float dist=-100.0f;

	float dpdd = scale / dist;

	tMap.x = tMap.x + dxMap.x * deltaPos.x * dpdd + dxMap.y * deltaPos.y * dpdd;	
	tMap.y = tMap.y + dyMap.x * deltaPos.x * dpdd + dyMap.y * deltaPos.y * dpdd;

	while (tMap.x < 0.0) tMap.x +=  1.0;
	while (tMap.x > 1.0) tMap.x += -1.0;
	while (tMap.y < 0.0) tMap.y +=  1.0;
	while (tMap.y > 1.0) tMap.y += -1.0;

	VectorYawRotate( dxMap, -deltaAngles.y, dxMap );
	VectorYawRotate( dyMap, -deltaAngles.y, dyMap );

	// ARRGHHH, this is right but I don't know what I've done
	meshBuilder.Position3f( -dist, dist, 0 );
	meshBuilder.TexCoord2f( 0, tMap.x + (-dxMap.x - dyMap.x) * scale, tMap.y + (dxMap.y + dyMap.y) * scale );
	meshBuilder.Color4ub( 128, 128, 128, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( dist, dist, 0 );
	meshBuilder.TexCoord2f( 0, tMap.x + (dxMap.x - dyMap.x) * scale, tMap.y + (-dxMap.y + dyMap.y) * scale );
	meshBuilder.Color4ub( 128, 128, 128, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( dist, -dist, 0 );
	meshBuilder.TexCoord2f( 0, tMap.x + (dxMap.x + dyMap.x) * scale, tMap.y + (-dxMap.y - dyMap.y) * scale );
	meshBuilder.Color4ub( 128, 128, 128, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( -dist, -dist, 0 );
	meshBuilder.TexCoord2f( 0, tMap.x - (dxMap.x - dyMap.x) * scale, tMap.y - (-dxMap.y + dyMap.y) * scale );
	meshBuilder.Color4ub( 128, 128, 128, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	g_pMaterialSystem->MatrixMode(MATERIAL_MODEL);
	g_pMaterialSystem->LoadMatrix((float*)mTempModel);
	g_pMaterialSystem->MatrixMode(MATERIAL_VIEW);
	g_pMaterialSystem->LoadMatrix((float*)mTempView);
}




void DrawMovementBoxes()
{
	if (!g_viewerSettings.showMovement)
		return;

	float mTempModel[4][4];
	float mTempView[4][4];
	g_pMaterialSystem->Bind(g_materialFloor);
	g_pMaterialSystem->MatrixMode(MATERIAL_MODEL);
	g_pMaterialSystem->GetMatrix(MATERIAL_MODEL, (float*)mTempModel);
	g_pMaterialSystem->LoadIdentity();
	g_pMaterialSystem->MatrixMode(MATERIAL_VIEW);
	g_pMaterialSystem->GetMatrix(MATERIAL_VIEW, (float*)mTempView);
	g_pMaterialSystem->LoadIdentity();

	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	g_pMaterialSystem->LoadIdentity( );

	g_pMaterialSystem->Rotate( -90,  1, 0, 0 );	    // put Z going up
	g_pMaterialSystem->Rotate( -90,  0, 0, 1 );

    g_pMaterialSystem->Translate( -g_viewerSettings.trans[0],  -g_viewerSettings.trans[1],  -g_viewerSettings.trans[2] );

	g_pMaterialSystem->Rotate( g_viewerSettings.rot[1],  0, 0, 1 );
    g_pMaterialSystem->Rotate( g_viewerSettings.rot[0],  0, 1, 0 );
    g_pMaterialSystem->Rotate( g_viewerSettings.rot[2],  1, 0, 0 );

	static matrix3x4_t mStart( 1, 0, 0, 0 ,  0, 1, 0, 0 ,  0, 0, 1, 0 );
	matrix3x4_t mTemp;
	static float prevframe = 0.0f;

	float currframe = g_pStudioModel->GetCycle( );

	Vector deltaPos;
	QAngle deltaAngles;

	// assume that changes < -0.5 are loops....
	if (currframe - prevframe < -0.5)
	{
		prevframe = 0;
		// assumes a max of one loop, in any case prevframe gets reset below
		SetIdentityMatrix( mStart );
	}

	g_pStudioModel->GetMovement( prevframe, currframe, deltaPos, deltaAngles );
	prevframe = currframe;

	AngleMatrix( deltaAngles, deltaPos, mTemp );
	MatrixInvert( mTemp, mTemp );
	ConcatTransforms( mTemp, mStart, mStart );

	Vector bboxMin, bboxMax;
	g_pStudioModel->ExtractBbox( bboxMin, bboxMax  );


	// starting position
	{
		float color[] = { 0.7, 1, 0, 0.5 };
		float wirecolor[] = { 1, 1, 0, 1.0 };
		g_pStudioModel->drawTransparentBox( bboxMin, bboxMax, mStart, color, wirecolor );
	}

	// current position
	{
		float color[] = { 1, 0.7, 0, 0.5 };
		float wirecolor[] = { 1, 0, 0, 1.0 };
		SetIdentityMatrix( mTemp );
		g_pStudioModel->drawTransparentBox( bboxMin, bboxMax, mTemp, color, wirecolor );
	}

	g_pMaterialSystem->MatrixMode(MATERIAL_MODEL);
	g_pMaterialSystem->LoadMatrix((float*)mTempModel);
	g_pMaterialSystem->MatrixMode(MATERIAL_VIEW);
	g_pMaterialSystem->LoadMatrix((float*)mTempView);
}


void
MatSysWindow::draw ()
{
	g_pMaterialSystem->BeginFrame();
	g_pStudioModel->GetStudioRender()->BeginFrame();

	g_pMaterialSystem->ClearColor3ub(g_viewerSettings.bgColor[0] * 255, g_viewerSettings.bgColor[1] * 255, g_viewerSettings.bgColor[2] * 255);
	g_pMaterialSystem->ClearBuffers(true, true);

	g_pMaterialSystem->Viewport( 0, 0, w(), h() );

	g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	g_pMaterialSystem->LoadIdentity( );
	g_pMaterialSystem->PerspectiveX(g_viewerSettings.fov, (float)w() / (float)h(), 1.0f, 20000.0f);
	
	DrawBackground();
	DrawGroundPlane();
	DrawMovementBoxes();
	DrawHelpers();

	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	g_pMaterialSystem->LoadIdentity( );
	// FIXME: why is this needed?  Doesn't SetView() override this?
	g_pMaterialSystem->Rotate( -90,  1, 0, 0 );	    // put Z going up
	g_pMaterialSystem->Rotate( -90,  0, 0, 1 );

	int polycount = g_pStudioModel->DrawModel ();

	int lod;
	float metric;
	metric = g_pStudioModel->GetLodMetric();
	lod = g_pStudioModel->GetLodUsed();
	g_ControlPanel->setLOD( lod, true, false );
	g_ControlPanel->setLODMetric( metric );

	g_ControlPanel->setPolycount( polycount );

	g_ControlPanel->updatePoseParameters( );

	// g_vright[0] = g_vright[1] = g_viewerSettings.trans[2];

	/*
	if (g_viewerSettings.mirror)
	{
		glPushMatrix ();
		glScalef (1, 1, -1);
		glCullFace (GL_BACK);
		setupRenderMode ();
		g_pStudioModel->DrawModel ();
		glPopMatrix ();
	}
	*/

	/*
	//
	// draw ground
	//
	if (g_viewerSettings.showGround)
	{
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glEnable (GL_DEPTH_TEST);
		glEnable (GL_CULL_FACE);

		if (g_viewerSettings.useStencil)
			glFrontFace (GL_CW);
		else
			glDisable (GL_CULL_FACE);

		glEnable (GL_BLEND);
		if (!d_textureNames[1])
		{
			glDisable (GL_TEXTURE_2D);
			glColor4f (g_viewerSettings.gColor[0], g_viewerSettings.gColor[1], g_viewerSettings.gColor[2], 0.7f);
			glBindTexture (GL_TEXTURE_2D, 0);
		}
		else
		{
			glEnable (GL_TEXTURE_2D);
			glColor4f (1.0f, 1.0f, 1.0f, 0.6f);
			glBindTexture (GL_TEXTURE_2D, d_textureNames[1]);
		}

		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		drawFloor ();

		glDisable (GL_BLEND);

		if (g_viewerSettings.useStencil)
		{
			glCullFace (GL_BACK);
			glColor4f (0.1f, 0.1f, 0.1f, 1.0f);
			glBindTexture (GL_TEXTURE_2D, 0);
			drawFloor ();

			glFrontFace (GL_CCW);
		}
		else
			glEnable (GL_CULL_FACE);
	}

	glPopMatrix ();
	*/

    g_pMaterialSystem->SwapBuffers();

	g_pStudioModel->GetStudioRender()->EndFrame();
	g_pMaterialSystem->EndFrame();
}



/*
int
MatSysWindow::loadTexture (const char *filename, int name)
{
	if (!filename || !strlen (filename))
	{
		if (d_textureNames[name])
		{
			glDeleteTextures (1, (const GLuint *) &d_textureNames[name]);
			d_textureNames[name] = 0;

			if (name == 0)
				strcpy (g_viewerSettings.backgroundTexFile, "");
			else
				strcpy (g_viewerSettings.groundTexFile, "");
		}

		return 0;
	}

	mxImage *image = 0;

	char ext[16];
	strcpy (ext, mx_getextension (filename));

	if (!mx_strcasecmp (ext, ".tga"))
		image = mxTgaRead (filename);
	else if (!mx_strcasecmp (ext, ".pcx"))
		image = mxPcxRead (filename);
	else if (!mx_strcasecmp (ext, ".bmp"))
		image = mxBmpRead (filename);

	if (image)
	{
		if (name == 0)
			strcpy (g_viewerSettings.backgroundTexFile, filename);
		else
			strcpy (g_viewerSettings.groundTexFile, filename);

		d_textureNames[name] = name + 1;

		if (image->bpp == 8)
		{
			mstudiotexture_t texture;
			texture.width = image->width;
			texture.height = image->height;

			g_pStudioModel->UploadTexture (&texture, (byte *) image->data, (byte *) image->palette, name + 1);
		}
		else
		{
			glBindTexture (GL_TEXTURE_2D, d_textureNames[name]);
			glTexImage2D (GL_TEXTURE_2D, 0, 3, image->width, image->height, 0, GL_RGB, GL_UNSIGNED_BYTE, image->data);
			glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		delete image;

		return name + 1;
	}

	return 0;
}
*/


void
MatSysWindow::dumpViewport (const char *filename)
{
	redraw ();
	int w = w2 ();
	int h = h2 ();

	mxImage *image = new mxImage ();
	if (image->create (w, h, 24))
	{
#if 0
		glReadBuffer (GL_FRONT);
		glReadPixels (0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, image->data);
#else
		HDC hdc = GetDC ((HWND) getHandle ());
		byte *data = (byte *) image->data;
		int i = 0;
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				COLORREF cref = GetPixel (hdc, x, y);
				data[i++] = (byte) ((cref >> 0)& 0xff);
				data[i++] = (byte) ((cref >> 8) & 0xff);
				data[i++] = (byte) ((cref >> 16) & 0xff);
			}
		}
		ReleaseDC ((HWND) getHandle (), hdc);
#endif
		if (!mxTgaWrite (filename, image))
			mxMessageBox (this, "Error writing screenshot.", g_appTitle, MX_MB_OK | MX_MB_ERROR);

		delete image;
	}
}

