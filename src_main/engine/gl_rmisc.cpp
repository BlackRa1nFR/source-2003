//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "winquake.h"
#include "glquake.h"
#include "r_local.h"
#include "gl_rsurf.h"
#include "gl_shader.h"
#include "gl_texture.h"
#include "gl_lightmap.h"
#include "gl_matsysiface.h"
#include "cdll_int.h"
#include "dispchain.h"
#include "client_class.h"
#include "icliententitylist.h"
#include "traceinit.h"
#include "server.h"
#include "ispatialpartitioninternal.h"
#include "cdll_engine_int.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "ivtex.h"
#include "materialsystem/itexture.h"
#include "ibsppack.h"
#include "view.h"
#include "tier0/dbg.h"
#include "staticpropmgr.h"
#include "icliententity.h"
#include "gl_drawlights.h"
#include "Overlay.h"
#include "vmodes.h"
#include "gl_cvars.h"
#include "UtlBuffer.h"
#include "vtf/vtf.h"
#include "imageloader.h"

#ifdef _WIN32
#include "procinfo.h"
#endif

void Linefile_Read_f(void);

ConVar building_cubemaps( "building_cubemaps", "0" );

void VID_TakeSnapshotRect( const char *pFilename, int x, int y, int w, int h, int resampleWidth, int resampleHeight );

extern viddef_t	vid;				// global video state

static void TakeCubemapSnapshot( const Vector &origin, const char *pFileNameBase, int screenBufSize,
						 int tgaSize )
{
	// HACK HACK HACK!!!!  
	// If this is lower than the size of the render target (I think) we don't get water.
	screenBufSize = 512;
	
	char	name[1024];
	CViewSetup	view = g_EngineRenderer->ViewGetCurrent();
	view.origin = origin;
	view.m_bForceAspectRatio1To1 = true;

	// garymcthack
	view.zNear = 8.0f;
	view.zFar = 28400.0f;

	view.x = 0;
	
	int width, height;
	materialSystemInterface->GetRenderTargetDimensions( width, height );
	// OpenGL has 0,0 in lower left
	view.y = height - screenBufSize;

	view.width = ( float )screenBufSize;
	view.height = ( float )screenBufSize;

	view.angles[0] = 0;
	view.angles[1] = 0;
	view.angles[2] = 0;
	view.fov = 90;
	view.fovViewmodel = 90;
	view.clearColor = true;
	view.clearDepth = true;

	Shader_BeginRendering();
	g_ClientDLL->RenderView( view, false );
	Q_snprintf( name, sizeof( name ), "%srt.tga", pFileNameBase );
	assert( strlen( name ) < 1023 );
	VID_TakeSnapshotRect( name, 0, 0, screenBufSize, screenBufSize,
		tgaSize, tgaSize );

	view.angles[0] = 0;
	view.angles[1] = 90;
	view.angles[2] = 0;
	view.fov = 90;
	view.fovViewmodel = 90;
	view.origin = origin;
	Shader_BeginRendering ();
	g_ClientDLL->RenderView( view, false );
	Q_snprintf( name, sizeof( name ), "%sbk.tga", pFileNameBase );
	assert( strlen( name ) < 1023 );
	VID_TakeSnapshotRect( name, 0, 0, screenBufSize, screenBufSize,
		tgaSize, tgaSize );

	view.angles[0] = 0;
	view.angles[1] = 180;
	view.angles[2] = 0;
	view.fov = 90;
	view.fovViewmodel = 90;
	view.origin = origin;
	Shader_BeginRendering ();
	g_ClientDLL->RenderView( view, false );
	Q_snprintf( name, sizeof( name ), "%slf.tga", pFileNameBase );
	assert( strlen( name ) < 1023 );
	VID_TakeSnapshotRect( name, 0, 0, screenBufSize, screenBufSize,
		tgaSize, tgaSize );

	view.angles[0] = 0;
	view.angles[1] = 270;
	view.angles[2] = 0;
	view.fov = 90;
	view.fovViewmodel = 90;
	view.origin = origin;
	Shader_BeginRendering ();
	g_ClientDLL->RenderView( view, false );
	Q_snprintf( name, sizeof( name ), "%sft.tga", pFileNameBase );
	assert( strlen( name ) < 1023 );
	VID_TakeSnapshotRect( name, 0, 0, screenBufSize, screenBufSize,
		tgaSize, tgaSize );

	view.angles[0] = -90;
	view.angles[1] = 0;
	view.angles[2] = 0;
	view.fov = 90;
	view.fovViewmodel = 90;
	view.origin = origin;
	Shader_BeginRendering ();
	g_ClientDLL->RenderView( view, false );
	Q_snprintf( name, sizeof( name ), "%sup.tga", pFileNameBase );
	assert( strlen( name ) < 1023 );
	VID_TakeSnapshotRect( name, 0, 0, screenBufSize, screenBufSize,
		tgaSize, tgaSize );

	view.angles[0] = 90;
	view.angles[1] = 0;
	view.angles[2] = 0;
	view.fov = 90;
	view.fovViewmodel = 90;
	view.origin = origin;
	Shader_BeginRendering ();
	g_ClientDLL->RenderView( view, false );
	Q_snprintf( name, sizeof( name ), "%sdn.tga", pFileNameBase );
	assert( strlen( name ) < 1023 );
	VID_TakeSnapshotRect( name, 0, 0, screenBufSize, screenBufSize,
		tgaSize, tgaSize );
}

/*
===============
R_Envmap_f

Grab six views for environment mapping tests
===============
*/
#ifdef _WIN32
void R_Envmap_f (void)
{
	char	base[ 256 ];
	IClientEntity *world = entitylist->GetClientEntity( 0 );

	if( world && world->GetModel() )
	{
		COM_FileBase( modelloader->GetName( ( model_t *)world->GetModel() ), base );
	}
	else
	{
		strcpy( base, "Env" );
	}

	int strLen = strlen( base ) + strlen( "cubemap_screenshots/" ) + 1;
	char *str = ( char * )_alloca( strLen );
	Q_snprintf( str, strLen, "cubemap_screenshots/%s", base );
	g_pFileSystem->CreateDirHierarchy( "cubemap_screenshots", "GAME" );
	CViewSetup	view = g_EngineRenderer->ViewGetCurrent();
	TakeCubemapSnapshot( view.origin, str, mat_envmapsize.GetInt(), mat_envmaptgasize.GetInt() );
}
#endif

/*
===============
R_BuildCubemapSamples

Take a cubemap at each "cubemap" entity in the current map.
===============
*/
#ifdef _WIN32
static bool LoadSrcVTFFiles( IVTFTexture *pSrcVTFTextures[6], const char *pSkyboxBaseName )
{
	const char *facingName[6] = { "rt", "lf", "bk", "ft", "up", "dn" };
	int i;
	for( i = 0; i < 6; i++ )
	{
		char srcVTFFileName[1024];
		sprintf( srcVTFFileName, "materials/skybox/%s%s.vtf", pSkyboxBaseName, facingName[i] );

		FileHandle_t fp;
		fp = g_pFileSystem->Open( srcVTFFileName, "rb" );
		if( !fp )
		{
			return false;
		}
		g_pFileSystem->Seek( fp, 0, FILESYSTEM_SEEK_TAIL );
		int srcVTFLength = g_pFileSystem->Tell( fp );
		g_pFileSystem->Seek( fp, 0, FILESYSTEM_SEEK_HEAD );

		CUtlBuffer buf;
		buf.EnsureCapacity( srcVTFLength );
		g_pFileSystem->Read( buf.Base(), srcVTFLength, fp );
		g_pFileSystem->Close( fp );

		pSrcVTFTextures[i] = CreateVTFTexture();
		if (!pSrcVTFTextures[i]->Unserialize(buf))
		{
			Warning("*** Error unserializing skybox texture: %s\n", pSkyboxBaseName );
			return false;
		}

		if ( (pSrcVTFTextures[i]->Width() != pSrcVTFTextures[0]->Width()) ||
			 (pSrcVTFTextures[i]->Flags() != pSrcVTFTextures[0]->Flags()) )
		{
			Warning("*** Error: Skybox vtf files for %s weren't compiled with the same size texture and/or same flags!\n", pSkyboxBaseName );
			return false;
		}
	}

	return true;
}

#define DEFAULT_CUBEMAP_SIZE 32

void Cubemap_CreateDefaultCubemap( const char *pMapName, IBSPPack *iBSPPack )
{
	// NOTE: This implementation depends on the fact that all VTF files contain
	// all mipmap levels
	ConVar *pSkyboxBaseNameConVar = ( ConVar * )cv->FindVar( "sv_skyname" );

	IVTFTexture *pSrcVTFTextures[6];

	if( !pSkyboxBaseNameConVar || !pSkyboxBaseNameConVar->GetString() )
	{
		Warning( "Couldn't create default cubemap\n" );
		return;
	}

	const char *pSkyboxBaseName = pSkyboxBaseNameConVar->GetString();

	if( !LoadSrcVTFFiles( pSrcVTFTextures, pSkyboxBaseName ) )
	{
		Warning( "Can't load skybox file %s to build the default cubemap!\n", pSkyboxBaseName );
		return;
	}
	Msg( "Creating default cubemaps for env_cubemap using skybox %s...\n", pSkyboxBaseName );
			
	// Figure out the mip differences between the two textures
	int iMipLevelOffset = 0;
	int tmp = pSrcVTFTextures[0]->Width();
	while( tmp > DEFAULT_CUBEMAP_SIZE )
	{
		iMipLevelOffset++;
		tmp >>= 1;
	}

	// Create the destination cubemap
	IVTFTexture *pDstCubemap = CreateVTFTexture();
	pDstCubemap->Init( DEFAULT_CUBEMAP_SIZE, DEFAULT_CUBEMAP_SIZE, 
		pSrcVTFTextures[0]->Format(), pSrcVTFTextures[0]->Flags() | TEXTUREFLAGS_ENVMAP, 
		pSrcVTFTextures[0]->FrameCount() );

	// First iterate over all frames
	for (int iFrame = 0; iFrame < pDstCubemap->FrameCount(); ++iFrame)
	{
		// Next iterate over all normal cube faces (we know there's 6 cause it's an envmap)
		for (int iFace = 0; iFace < 6; ++iFace )
		{
			// Finally, iterate over all mip levels in the *destination*
			for (int iMip = 0; iMip < pDstCubemap->MipCount(); ++iMip )
			{
				// Copy the bits from the source images into the cube faces
				unsigned char *pSrcBits = pSrcVTFTextures[iFace]->ImageData( iFrame, 0, iMip + iMipLevelOffset );
				unsigned char *pDstBits = pDstCubemap->ImageData( iFrame, iFace, iMip );
				int iSize = pDstCubemap->ComputeMipSize( iMip );

				memcpy( pDstBits, pSrcBits, iSize ); 
			}
		}
	}

	int flagUnion = 0;
	int i;
	for( i = 0; i < 6; i++ )
	{
		flagUnion |= pSrcVTFTextures[i]->Flags();
	}
	bool bHasAlpha = 
		( flagUnion & ( TEXTUREFLAGS_ONEBITALPHA | TEXTUREFLAGS_EIGHTBITALPHA ) != 0 );
	
	// Convert the cube to format that we can apply tools to it...
//	ImageFormat originalFormat = pDstCubemap->Format();
	pDstCubemap->ConvertImageFormat( IMAGE_FORMAT_DEFAULT, false );

	if( !bHasAlpha )
	{
		// set alpha to zero since the source doesn't have any alpha in it
		unsigned char *pImageData = pDstCubemap->ImageData();
		int size = pDstCubemap->ComputeTotalSize(); // in bytes!
		unsigned char *pEnd = pImageData + size;
		for( ; pImageData < pEnd; pImageData += 4 )
		{
			pImageData[3] = ( unsigned char )0;
		}
	}

	// Fixup the cubemap facing
	pDstCubemap->FixCubemapFaceOrientation();

	// Now that the bits are in place, compute the spheremaps...
	pDstCubemap->GenerateSpheremap();

	// Convert the cubemap to the final format
	pDstCubemap->ConvertImageFormat( IMAGE_FORMAT_DXT5, false );

	// Write the puppy out!
	char dstVTFFileName[1024];
	sprintf( dstVTFFileName, "materials/maps/%s/cubemapdefault.vtf", pMapName );

	CUtlBuffer outputBuf;
	if (!pDstCubemap->Serialize( outputBuf ))
	{
		Warning( "Error serializing default cubemap %s\n", dstVTFFileName );
		return;
	}

	// spit out the default one.
	iBSPPack->AddBufferToPack( dstVTFFileName, outputBuf.Base(), outputBuf.TellPut(), false );

	// Clean up the textures
	for( i = 0; i < 6; i++ )
	{
		DestroyVTFTexture( pSrcVTFTextures[i] );
	}
	DestroyVTFTexture( pDstCubemap );
}	

void R_BuildCubemapSamples( int numIterations )
{
	// disable the mouse so that it won't be recentered all the bloody time.
	ConVar *cl_mouseenable = ( ConVar * )cv->FindVar( "cl_mouseenable" );
	if( cl_mouseenable )
	{
		cl_mouseenable->SetValue( 0 );
	}
	// disable mat_bloom since it screws up buildcubemaps.
	// FIXME FIXME FIXME
	ConVar *mat_bloom = ( ConVar * )cv->FindVar( "mat_bloom" );
	bool saveBloom = true;
	if( mat_bloom )
	{
		saveBloom = mat_bloom->GetBool();
		mat_bloom->SetValue( 0 );
	}
	ConVar *r_shadows = ( ConVar * )cv->FindVar( "r_shadows" );
	bool saveShadows = true;
	if( r_shadows )
	{
		saveShadows = r_shadows->GetBool();
		r_shadows->SetValue( 0 );
	}
	
	bool bSaveMatSpecular = mat_specular.GetBool();

	ConVar *r_drawtranslucentworld = ( ConVar * )cv->FindVar( "r_drawtranslucentworld" );
	ConVar *r_drawtranslucentrenderables = ( ConVar * )cv->FindVar( "r_drawtranslucentrenderables" );

	bool bSaveDrawTranslucentWorld = true;
	bool bSaveDrawTranslucentRenderables = true;
	if( r_drawtranslucentworld )
	{
		bSaveDrawTranslucentWorld = r_drawtranslucentworld->GetBool();
		r_drawtranslucentworld->SetValue( 0 );
	}
	if( r_drawtranslucentrenderables )
	{
		bSaveDrawTranslucentRenderables = r_drawtranslucentrenderables->GetBool();
		r_drawtranslucentrenderables->SetValue( 0 );
	}
	
	building_cubemaps.SetValue( 1 );
	
	int bounce;
	for( bounce = 0; bounce < numIterations; bounce++ )
	{
		if( bounce == 0 )
		{
			mat_specular.SetValue( "0" );
		}
		else
		{
			mat_specular.SetValue( "1" );
		}
		UpdateMaterialSystemConfig();
		
		char	mapName[ 256 ];
		IClientEntity *world = entitylist->GetClientEntity( 0 );

		if( world && world->GetModel() )
		{
			const model_t *pModel = world->GetModel();
			const char *pModelName = modelloader->GetName( pModel );
			
			// This handles the case where you have a map in a directory under maps. 
			// We need to keep everything after "maps/" so it looks for the BSP file in the right place.
			if ( Q_stristr( pModelName, "maps/" ) == pModelName ||
				Q_stristr( pModelName, "maps\\" ) == pModelName )
			{
				Q_strncpy( mapName, &pModelName[5], sizeof( mapName ) );
				COM_StripExtension( mapName, mapName, sizeof( mapName ) );
			}
			else
			{
				COM_FileBase( pModelName, mapName );
			}
		}
		else
		{
			Con_DPrintf( "R_BuildCubemapSamples: No map loaded!\n" );
			return;
		}
		
		int oldDrawMRMModelsVal = 1;
		ConVar *pDrawMRMModelsCVar = ( ConVar * )cv->FindVar( "r_drawmrmmodels" );
		if( pDrawMRMModelsCVar )
		{
			oldDrawMRMModelsVal = pDrawMRMModelsCVar->GetInt();
			pDrawMRMModelsCVar->SetValue( 0 );
		}

		bool bOldLightSpritesActive = ActivateLightSprites( true );

		// load the vtex dll
		IVTex *ivt = NULL;
		CSysModule *pmodule = FileSystem_LoadModule( "vtex.dll" );
		if ( pmodule )
		{
			CreateInterfaceFn factory = Sys_GetFactory( pmodule );
			if ( factory )
			{
				ivt = ( IVTex * )factory( IVTEX_VERSION_STRING, NULL );
			}
		}
		if( !ivt )
		{
			Con_Printf( "Can't load vtex.dll\n" );
			return;
		}
		
		char matDir[1024];
		Q_snprintf( matDir, sizeof(matDir), "materials/maps/%s", mapName );
		g_pFileSystem->CreateDirHierarchy( matDir, "GAME" );
		Q_snprintf( matDir, sizeof(matDir), "materialsrc/maps/%s", mapName );
		g_pFileSystem->CreateDirHierarchy( matDir, "GAME" );
		
		char gameDir[MAX_OSPATH];
		COM_GetGameDir( gameDir );
		COM_FileBase( gameDir, gameDir );
		
		model_t *pWorldModel = ( model_t *)world->GetModel();
		int i;
		for( i = 0; i < pWorldModel->brush.m_nCubemapSamples; i++ )
		{
			mcubemapsample_t  *pCubemapSample = &pWorldModel->brush.m_pCubemapSamples[i];

			int tgaSize = ( pCubemapSample->size == 0 ) ? mat_envmaptgasize.GetInt() : 1 << ( pCubemapSample->size-1 );
			int screenBufSize = 4 * tgaSize;
			if ( (unsigned)screenBufSize > vid.width || (unsigned)screenBufSize > vid.height )
			{
				Warning( "Cube map buffer size %d x %d is bigger than screen!\nRun at a higher resolution! or reduce your cubemap resolution (needs 4X)\n", screenBufSize, screenBufSize );
				// BUGBUG: We'll leak DLLs/handles if we break out here, but this should be infrequent.
				return;
			}
		}

		for( i = 0; i < pWorldModel->brush.m_nCubemapSamples; i++ )
		{
			Warning( "bounce: %d/%d sample: %d/%d\n", bounce+1, numIterations, i+1, pWorldModel->brush.m_nCubemapSamples );
			char	textureName[ 256 ];
			mcubemapsample_t  *pCubemapSample = &pWorldModel->brush.m_pCubemapSamples[i];
			Q_snprintf( textureName, sizeof( textureName ), "%s/c%d_%d_%d", matDir, 
				( int )pCubemapSample->origin[0],
				( int )pCubemapSample->origin[1],
				( int )pCubemapSample->origin[2] );
			assert( strlen( textureName ) < 255 );
			int tgaSize = ( pCubemapSample->size == 0 ) ? mat_envmaptgasize.GetInt() : 1 << ( pCubemapSample->size-1 );
			int screenBufSize = 4 * tgaSize;

			TakeCubemapSnapshot( pCubemapSample->origin, textureName, screenBufSize, tgaSize );
			Q_strncat( textureName, ".txt", sizeof( textureName ) );
			assert( strlen( textureName ) < 255 );
			FileHandle_t fp = g_pFileSystem->Open( textureName, "w" );
			g_pFileSystem->Close( fp );
			// garymcthack
			char commandString[256];
			Q_snprintf( commandString, sizeof( commandString ), "%s\\materialsrc\\maps\\%s\\c%d_%d_%d.txt", gameDir, mapName, 
  					 ( int )pCubemapSample->origin[0],
					 ( int )pCubemapSample->origin[1],
					 ( int )pCubemapSample->origin[2] );
			int argc = 3;
			char *argv[3];
			argv[0] = "";
			argv[1] = "-quiet";
			argv[2] = commandString;
			ivt->VTex( argc, argv );
			Q_snprintf( textureName, sizeof( textureName ), "%s/c%d_%d_%d.txt", matDir, 
				( int )pCubemapSample->origin[0],
				( int )pCubemapSample->origin[1],
				( int )pCubemapSample->origin[2] );
			g_pFileSystem->RemoveFile( textureName, "GAME" );
			Q_snprintf( textureName, sizeof( textureName ), "%s/c%d_%d_%dlf.tga", matDir, 
				( int )pCubemapSample->origin[0],
				( int )pCubemapSample->origin[1],
				( int )pCubemapSample->origin[2] );
			g_pFileSystem->RemoveFile( textureName, "GAME" );
			Q_snprintf( textureName, sizeof( textureName ), "%s/c%d_%d_%dbk.tga", matDir, 
				( int )pCubemapSample->origin[0],
				( int )pCubemapSample->origin[1],
				( int )pCubemapSample->origin[2] );
			g_pFileSystem->RemoveFile( textureName, "GAME" );
			Q_snprintf( textureName, sizeof( textureName ), "%s/c%d_%d_%ddn.tga", matDir, 
				( int )pCubemapSample->origin[0],
				( int )pCubemapSample->origin[1],
				( int )pCubemapSample->origin[2] );
			g_pFileSystem->RemoveFile( textureName, "GAME" );
			Q_snprintf( textureName, sizeof( textureName ), "%s/c%d_%d_%dft.tga", matDir, 
				( int )pCubemapSample->origin[0],
				( int )pCubemapSample->origin[1],
				( int )pCubemapSample->origin[2] );
			g_pFileSystem->RemoveFile( textureName, "GAME" );
			Q_snprintf( textureName, sizeof( textureName ), "%s/c%d_%d_%drt.tga", matDir, 
				( int )pCubemapSample->origin[0],
				( int )pCubemapSample->origin[1],
				( int )pCubemapSample->origin[2] );
			g_pFileSystem->RemoveFile( textureName, "GAME" );
			Q_snprintf( textureName, sizeof( textureName ), "%s/c%d_%d_%dup.tga", matDir, 
				( int )pCubemapSample->origin[0],
				( int )pCubemapSample->origin[1],
				( int )pCubemapSample->origin[2] );
			g_pFileSystem->RemoveFile( textureName, "GAME" );
		}

		if( pDrawMRMModelsCVar )
		{ 
			pDrawMRMModelsCVar->SetValue( oldDrawMRMModelsVal );
		}

		ActivateLightSprites( bOldLightSpritesActive );

		FileSystem_UnloadModule( pmodule );

		// load the bsppack dll
		IBSPPack *iBSPPack = NULL;
		pmodule = FileSystem_LoadModule( "bsppack.dll" );
		if ( pmodule )
		{
			CreateInterfaceFn factory = Sys_GetFactory( pmodule );
			if ( factory )
			{
				iBSPPack = ( IBSPPack * )factory( IBSPPACK_VERSION_STRING, NULL );
			}
		}
		if( !iBSPPack )
		{
			Con_Printf( "Can't load bsppack.dll\n" );
			return;
		}

		char mapPath[1024];
		Q_snprintf( mapPath, sizeof( mapPath ), "%s/maps/%s.bsp", gameDir, mapName );
		iBSPPack->LoadBSPFile( mapPath );
		
		// Cram the textures into the bsp.
		Q_snprintf( matDir, sizeof(matDir), "materials/maps/%s", mapName );
		for ( i=0 ; i < pWorldModel->brush.m_nCubemapSamples ; i++ )
		{
			mcubemapsample_t *pSample = &pWorldModel->brush.m_pCubemapSamples[i];
			char textureName[512];
			Q_snprintf( textureName, sizeof( textureName ), "%s/c%d_%d_%d.vtf", matDir, ( int )pSample->origin[0], 
				( int )pSample->origin[1], ( int )pSample->origin[2] );
			int pathLen = g_pFileSystem->GetLocalPathLen( textureName );
			if( !pathLen )
			{
				Con_Printf( "Error getting pathLength for %s\n", textureName );
				assert( 0 );
				continue;
			}
			char localPath[1024];
			g_pFileSystem->GetLocalPath( textureName, localPath );
			COM_FixSlashes( localPath );
			iBSPPack->AddFileToPack( textureName, localPath );
			g_pFileSystem->RemoveFile( textureName, "GAME" );
		}
		Cubemap_CreateDefaultCubemap( mapName, iBSPPack );
		
		iBSPPack->WriteBSPFile( mapPath );
		iBSPPack->ClearPackFile();
		FileSystem_UnloadModule( pmodule );

		// Reload the bsp zip file and reloadmaterials
		g_pFileSystem->AddSearchPath( cl.levelname, "GAME", PATH_ADD_TO_HEAD );
		materialSystemInterface->ReloadMaterials();
	}
	// re-enable the mouse.
	if( cl_mouseenable )
	{
		cl_mouseenable->SetValue( 1 );
	}
	if( r_shadows )
	{
		r_shadows->SetValue( saveShadows );
	}
	if( bSaveMatSpecular )
	{
		mat_specular.SetValue( "1" );
	}
	else
	{
		mat_specular.SetValue( "0" );
	}
	if( mat_bloom )
	{
		mat_bloom->SetValue( saveBloom );
	}

	if( r_drawtranslucentworld )
	{
		r_drawtranslucentworld->SetValue( bSaveDrawTranslucentWorld );
	}
	if( r_drawtranslucentrenderables )
	{
		r_drawtranslucentrenderables->SetValue( bSaveDrawTranslucentRenderables );
	}

	building_cubemaps.SetValue( 0 );
	UpdateMaterialSystemConfig();
}

void R_BuildCubemapSamples_f( void )
{
	if( Cmd_Argc() == 1 )
	{
		R_BuildCubemapSamples( 1 );
	}
	else if( Cmd_Argc() == 2 )
	{
		R_BuildCubemapSamples( atoi( Cmd_Argv( 1 ) ) );
	}
	else
	{
		Con_Printf( "Usage: buildcubemaps [numBounces]\n" );
	}
}
#endif

/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f (void)
{
	int			i;
	float		start, stop, time;
	CViewSetup		view;

	materialSystemInterface->Flush( true );

	view = g_EngineRenderer->ViewGetCurrent();

	
	int savedeveloper = developer.GetInt();
	developer.SetValue( 0 );

	start = Sys_FloatTime ();
	for (i=0 ; i<128 ; i++)
	{
		view.angles[1] = i/128.0*360.0;
		g_ClientDLL->RenderView( view, true );
		Shader_SwapBuffers();
	}

	materialSystemInterface->Flush( true );
	Shader_SwapBuffers();
	stop = Sys_FloatTime ();
	time = stop-start;

	developer.SetValue( savedeveloper );

	Con_Printf ("%f seconds (%f fps)\n", time, 128/time);
}

static ConCommand timerefresh("timerefresh", R_TimeRefresh_f);	
#if !defined( SWDS )
static ConCommand envmap("envmap", R_Envmap_f);	
static ConCommand buildcubemaps("buildcubemaps", R_BuildCubemapSamples_f);	
#endif
static ConCommand linefile("linefile", Linefile_Read_f );	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void R_Init (void)
{	
	extern byte *hunk_base;

	extern void InitMathlib( void );
	InitMathlib();

	UpdateMaterialSystemConfig();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void R_Shutdown( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void R_ResetLightStyles( void )
{
	for ( int i=0 ; i<256 ; i++ )
	{
		// normal light value
		d_lightstylevalue[i] = 264;		
		d_lightstyleframe[i] = 0;		
	}
}

/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	Con_DPrintf("R_NewMap\n");

	Assert( g_ClientDLL );

	R_ResetLightStyles();
	R_DecalInit ();
	R_LoadSkys();
	R_InitStudio();

	// FIXME: Is this the best place to initialize the kd tree when we're client-only?
	if ( !sv.active )
	{
		StaticPropMgr()->LevelShutdown();
		SpatialPartition()->Init( host_state.worldmodel->mins, host_state.worldmodel->maxs );
		StaticPropMgr()->LevelInit();
	}

	// We've fully loaded the new level, unload any models that we don't care about any more
	modelloader->UnloadUnreferencedModels();

	if ( host_state.worldmodel->brush.numworldlights == 0 )
	{
		Con_DPrintf( "Level unlit, setting 'mat_fullbright 1'\n" );
		mat_fullbright.SetValue( 1 );
	}

	UpdateMaterialSystemConfig();
	
	// get rid of the textures that are not used in this map, but 
	// are left over from the previous map.
	materialSystemInterface->UncacheUnusedMaterials();

	// FIXME: E3 2003 HACK
	if( mat_levelflush.GetBool() )
	{
		materialSystemInterface->ReleaseTempTextureMemory();
	}

	// precache any textures that are used in this map.
	// this is a no-op for textures that are already cached from the previous map.
	materialSystemInterface->CacheUsedMaterials();

	// Recreate the sortinfo arrays ( ack, uses new/delete right now ) because doing it with Hunk_AllocName will
	//  leak through every connect that doesn't wipe the hunk ( "reconnect" )
	MaterialSystem_DestroySortinfo();

	// Clear things out so that R_BuildLightmap doesn't try to download lightmaps before they are built.
	Shader_FreePerLevelData();

	MaterialSystem_RegisterLightmapSurfaces();

	MaterialSystem_CreateSortinfo();

	// If this is the first time we've tried to render this map, create a few one-time data structures
	// These all get cleared out if Map_UnloadModel is ever called by the modelloader interface ( and that happens
	//  any time we free the Hunk down to the low mark since these things all use the Hunk for their data ).
	if ( !modelloader->Map_GetRenderInfoAllocated() )
	{
		// create the displacement surfaces for the map
		modelloader->Map_LoadDisplacements( host_state.worldmodel, false );
		//if( !DispInfo_CreateStaticBuffers( host_state.worldmodel, materialSortInfoArray, false ) )
		//	Sys_Error( "Can't create static meshes for displacements" );

		modelloader->Map_SetRenderInfoAllocated( true );
	}

	// add the displacement surfaces to leaves for rendering
	AddDispSurfsToLeafs( host_state.worldmodel );

	extern void MarkWaterSurfaces_r( mnode_t *node ); // garymcthack

	// garymcthack - calling this again since it was called before AddDispSurfsToLeafs
	MarkWaterSurfaces_r( &host_state.worldmodel->brush.nodes[0] );

	// make sure and rebuild lightmaps when the level gets started.
	GL_RebuildLightmaps();
	
	// Build the overlay fragments.
	OverlayMgr()->CreateFragments();
}




