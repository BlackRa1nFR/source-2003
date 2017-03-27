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
#include "Vector.h"
#include "MatSysWin.h"
#include "ViewerSettings.h"
#include "cmdlib.h"
#include "imaterialsystem.h"
#include "imaterialproxyfactory.h"
#include "FileSystem.h"
#include "FileSystem_Tools.h"
#include "IMesh.h"
#include "MaterialSystem_Config.h"

extern char g_appTitle[];


// FIXME: move all this to mxMatSysWin

class DummyMaterialProxyFactory : public IMaterialProxyFactory
{
public:
	virtual IMaterialProxy *CreateProxy( const char *proxyName )	{return NULL;}
	virtual void DeleteProxy( IMaterialProxy *pProxy )				{}
};
DummyMaterialProxyFactory	g_DummyMaterialProxyFactory;



bool Sys_Error(const char *pMsg, ...)
{
	va_list marker;
	char msg[4096];
	
	va_start(marker, pMsg);
	vsprintf(msg, pMsg, marker);
	va_end(marker);

	MessageBox(NULL, msg, "FATAL ERROR", MB_OK);

	// g_MaterialSystemApp.Term();
	exit(1);
	return false;
}


static void MaterialSystem_Error( char *fmt, ... )
{
	char str[4096];
	va_list marker;
	
	va_start(marker, fmt);
	vsprintf(str, fmt, marker);
	va_end(marker);
	
	Sys_Error(str);
}


static void MaterialSystem_Warning( char *fmt, ... )
{
}

void InitMaterialSystemConfig(MaterialSystem_Config_t *pConfig, const char *matPath)
{
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
	pConfig->errorFunc = MaterialSystem_Error;
	pConfig->warningFunc = MaterialSystem_Warning;
	pConfig->bUseGraphics = true;
	pConfig->bEditMode = false;	// No, we're not in WorldCraft.
}

MatSysWindow *g_MatSysWindow = 0;
IMaterialSystem *g_pMaterialSystem;
Vector g_vright( 50, 50, 0 );		// needs to be set to viewer's right in order for chrome to work

IMaterial *g_materialBackground = NULL;
IMaterial *g_materialWireframe = NULL;
IMaterial *g_materialFlatshaded = NULL;
IMaterial *g_materialSmoothshaded = NULL;
IMaterial *g_materialBones = NULL;
IMaterial *g_materialFloor = NULL;


MatSysWindow::MatSysWindow (mxWindow *parent, int x, int y, int w, int h, const char *label, int style)
: mxMatSysWindow (parent, x, y, w, h, label, style)
{
	m_hWnd = (HWND)getHandle();


	const char *pPath = NULL; // FindParameterArg("-game");
	if(!pPath)
	{
		// If they didn't specify -game on the command line, use VPROJECT.
		SetQdirFromPath(".");
		pPath = basegamedir;
	}

	
	// Load the material system DLL and get its interface.
	m_hMaterialSystemInst = LoadLibrary( "MaterialSystem.dll" );
	if( !m_hMaterialSystemInst )
	{
		return; // Sys_Error( "Can't load MaterialSystem.dll\n" );
	}

	CreateInterfaceFn clientFactory = Sys_GetFactory( "MaterialSystem.dll" );
	if ( clientFactory )
	{
		g_pMaterialSystem = (IMaterialSystem *)clientFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
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
		
	char *pShaderDLL = NULL; // FindParameterArg("-shaderdll");
	if(!pShaderDLL)
		pShaderDLL = "shaderapidx9.dll";

	FileSystem_Init();
	
	if(!g_pMaterialSystem->Init(pShaderDLL, &g_DummyMaterialProxyFactory,FileSystem_GetFactory()))
		return; // Sys_Error("IMaterialSystem::Init failed");


	MaterialVideoMode_t mode;
	mode.m_Width = mode.m_Height = 0;
	if (!g_pMaterialSystem->SetMode((void*)m_hWnd, 0, mode, 
		MATERIAL_VIDEO_MODE_WINDOWED | MATERIAL_VIDEO_MODE_RESIZING))
		return; // Sys_Error("IMaterialSystem::Init failed");

	MaterialSystem_Config_t config;
	memset( &config, 0, sizeof( config ) );
	InitMaterialSystemConfig(&config, szMaterialPath);
	if(!g_pMaterialSystem->ConfigInit(&config))
		return; //  Sys_Error("IMaterialSystem::ConfigInit failed. Make sure VPROJECT is set or use -game on the command line.");
	

	g_materialBackground	= g_pMaterialSystem->FindMaterial("particle/particleapp_background", NULL, false);
	g_materialWireframe		= g_pMaterialSystem->FindMaterial("debug/debugmrmwireframe", NULL, false);
	g_materialFlatshaded	= g_pMaterialSystem->FindMaterial("debug/drawflattriangles", NULL, false);
	g_materialSmoothshaded	= g_pMaterialSystem->FindMaterial("debug/debugmrmfullbright2", NULL, false);
	g_materialBones			= g_pMaterialSystem->FindMaterial("debug/debugmrmwireframe", NULL, false);
	g_materialFloor			= g_pMaterialSystem->FindMaterial("debug/debugmrmwireframe", NULL, false);

	if (!parent)
		setVisible (true);
	else
		mx::setIdleWindow (this);
}



MatSysWindow::~MatSysWindow ()
{
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

//		g_studioModel.AdvanceFrame ( (curr - prev) * g_viewerSettings.speedScale );

		prev = curr;

		redraw ();

		return 1;
	}
	break;

	case mxEvent::MouseDown:
	{
		oldrx = g_viewerSettings.rot[0];
		oldry = g_viewerSettings.rot[1];
		oldtx = g_viewerSettings.trans[0];
		oldty = g_viewerSettings.trans[1];
		oldtz = g_viewerSettings.trans[2];
		oldx = event->x;
		oldy = event->y;

		return 1;
	}
	break;

	case mxEvent::MouseDrag:
	{
		if (event->buttons & mxEvent::MouseLeftButton)
		{
			if (event->modifiers & mxEvent::KeyShift)
			{
				g_viewerSettings.trans[0] = oldtx - (float) (event->x - oldx);
				g_viewerSettings.trans[1] = oldty + (float) (event->y - oldy);
			}
			else
			{
				g_viewerSettings.rot[0] = oldrx + (float) (event->y - oldy);
				g_viewerSettings.rot[1] = oldry + (float) (event->x - oldx);
			}
		}
		else if (event->buttons & mxEvent::MouseRightButton)
		{
			g_viewerSettings.trans[2] = oldtz + (float) (event->y - oldy);
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
//			g_MDLViewer->Refresh();
			break;
		}

		case 27:
			if (!getParent ()) // fullscreen mode ?
				mx::quit ();
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
	if (g_viewerSettings.use3dfx)
	{
		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (100.0f, 100.0f, 0.0f);

		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (100.0f, -100.0f, 0.0f);

		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-100.0f, 100.0f, 0.0f);

		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (-100.0f, -100.0f, 0.0f);

		glEnd ();
	}
	else
	{
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
	}
	*/
}



void
setupRenderMode ()
{
	/*
	if (g_viewerSettings.renderMode == RM_WIREFRAME)
	{
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_CULL_FACE);
		glEnable (GL_DEPTH_TEST);
	}
	else if (g_viewerSettings.renderMode == RM_FLATSHADED ||
			g_viewerSettings.renderMode == RM_SMOOTHSHADED)
	{
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glDisable (GL_TEXTURE_2D);
		glEnable (GL_CULL_FACE);
		glEnable (GL_DEPTH_TEST);

		if (g_viewerSettings.renderMode == RM_FLATSHADED)
			glShadeModel (GL_FLAT);
		else
			glShadeModel (GL_SMOOTH);
	}
	else if (g_viewerSettings.renderMode == RM_TEXTURED)
	{
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glEnable (GL_TEXTURE_2D);
		glEnable (GL_CULL_FACE);
		glEnable (GL_DEPTH_TEST);
		glShadeModel (GL_SMOOTH);
	}
	*/
}




void DrawBackground()
{
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
		float dist=-15000.0f;
		float tMin=0, tMax=1;
		
		IMesh* pMesh = g_pMaterialSystem->GetDynamicMesh();
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

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
MatSysWindow::draw ()
{
	g_pMaterialSystem->BeginFrame();
	g_pStudioRender->BeginFrame();

	g_pMaterialSystem->ClearBuffers(true, true);

	g_pMaterialSystem->Viewport( 0, 0, w(), h() );

	g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	g_pMaterialSystem->LoadIdentity( );
	g_pMaterialSystem->PerspectiveX(65.0f, (float)w() / (float)h(), 1.0f, 20000.0f);
	
	DrawBackground();

	/*
	glClearColor (g_viewerSettings.bgColor[0], g_viewerSettings.bgColor[1], g_viewerSettings.bgColor[2], 0.0f);

	if (g_viewerSettings.useStencil)
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	else
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport (0, 0, w2 (), h2 ());

	//
	// show textures
	//

	if (g_viewerSettings.showTexture)
	{
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();

		glOrtho (0.0f, (float) w2 (), (float) h2 (), 0.0f, 1.0f, -1.0f);

		studiohdr_t *hdr = g_studioModel.getTextureHeader ();
		if (hdr)
		{
			mstudiotexture_t *ptextures = (mstudiotexture_t *) ((byte *) hdr + hdr->textureindex);
			float w = (float) ptextures[g_viewerSettings.texture].width * g_viewerSettings.textureScale;
			float h = (float) ptextures[g_viewerSettings.texture].height * g_viewerSettings.textureScale;

			glMatrixMode (GL_MODELVIEW);
			glPushMatrix ();
			glLoadIdentity ();

			glDisable (GL_CULL_FACE);

			glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
			float x = ((float) w2 () - w) / 2;
			float y = ((float) h2 () - h) / 2;

			glDisable (GL_TEXTURE_2D);
			glColor4f (1.0f, 0.0f, 0.0f, 1.0f);
			glRectf (x - 2, y - 2, x  + w + 2, y + h + 2);

			glEnable (GL_TEXTURE_2D);
			glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
			glBindTexture (GL_TEXTURE_2D, g_viewerSettings.texture + 3); //d_textureNames[0]);

			glBegin (GL_TRIANGLE_STRIP);

			glTexCoord2f (0, 0);
			glVertex2f (x, y);

			glTexCoord2f (1, 0);
			glVertex2f (x + w, y);

			glTexCoord2f (0, 1);
			glVertex2f (x, y + h);

			glTexCoord2f (1, 1);
			glVertex2f (x + w, y + h);

			glEnd ();

			glPopMatrix ();

			glClear (GL_DEPTH_BUFFER_BIT);
			glBindTexture (GL_TEXTURE_2D, 0);
		}
		return;
	}

	//
	// draw background
	//

	if (g_viewerSettings.showBackground && d_textureNames[0] && !g_viewerSettings.showTexture)
	{
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();

		glOrtho (0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f);

		glMatrixMode (GL_MODELVIEW);
		glPushMatrix ();
		glLoadIdentity ();

		glDisable (GL_CULL_FACE);
		glEnable (GL_TEXTURE_2D);

		glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

		glBindTexture (GL_TEXTURE_2D, d_textureNames[0]);

		glBegin (GL_TRIANGLE_STRIP);

		glTexCoord2f (0, 0);
		glVertex2f (0, 0);

		glTexCoord2f (1, 0);
		glVertex2f (1, 0);

		glTexCoord2f (0, 1);
		glVertex2f (0, 1);

		glTexCoord2f (1, 1);
		glVertex2f (1, 1);

		glEnd ();

		glPopMatrix ();

		glClear (GL_DEPTH_BUFFER_BIT);
		glBindTexture (GL_TEXTURE_2D, 0);
	}
	*/

	//g_pMaterialSystem->MatrixMode( MATERIAL_PROJECTION );
	//g_pMaterialSystem->LoadIdentity( );
	//g_pMaterialSystem->PerspectiveX(65.0f, w() / h(), 1.0f, 4096.0f);

	g_pMaterialSystem->MatrixMode( MATERIAL_VIEW );
	// g_pMaterialSystem->PushMatrix( );
	g_pMaterialSystem->LoadIdentity( );
	g_pMaterialSystem->MatrixMode( MATERIAL_MODEL );
	g_pMaterialSystem->LoadIdentity( );
	// g_pMaterialSystem->Translate( -g_viewerSettings.trans[0], -g_viewerSettings.trans[1], -g_viewerSettings.trans[2]);

	// g_pMaterialSystem->Rotate(g_viewerSettings.rot[0], 1.0f, 0.0f, 0.0f);
	// g_pMaterialSystem->Rotate(g_viewerSettings.rot[1], 0.0f, 0.0f, 1.0f);

//	g_studioModel.DrawModel ();

	// g_vright[0] = g_vright[1] = g_viewerSettings.trans[2];

	/*
	if (g_viewerSettings.mirror)
	{
		glPushMatrix ();
		glScalef (1, 1, -1);
		glCullFace (GL_BACK);
		setupRenderMode ();
		g_studioModel.DrawModel ();
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

	g_pStudioRender->EndFrame();
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

			g_studioModel.UploadTexture (&texture, (byte *) image->data, (byte *) image->palette, name + 1);
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


/*
void
MatSysWindow::dumpViewport (const char *filename)
{
#ifdef WIN32
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
#endif	
}
*/

