//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: loads and unloads main matsystem dll and interface
//
//=============================================================================

#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/materialsystem_config.h"
#include "materialsystem/imaterialsystemstats.h"
#include "materialsystem/imesh.h"
#include "tier0/dbg.h"
#include "sys_dll.h"
#include "conprint.h"
#include "host.h"
#include "glquake.h"
#include "cmodel_engine.h"
#include "gl_model_private.h"
#include "view.h"
#include "gl_matsysiface.h"
#include "gl_cvars.h"
#include "gl_lightmap.h"
#include "lightcache.h"
#include "vstdlib/random.h"
#include "gl_texture.h"
#include "vstdlib/ICommandLine.h"

// Start the frame count at one so stuff gets updated the first frame
int                     r_framecount = 1;               // used for dlight + lightstyle push checking
int     d_lightstylevalue[256];

IMaterialSystem *materialSystemInterface = NULL;
IMaterialSystemStats *materialSystemStatsInterface = 0;
IMaterialSystemHardwareConfig* g_pMaterialSystemHardwareConfig = 0;
MatSortArray_t *g_pWorldStatic = NULL;
int g_pFirstMatSort[MAX_MAT_SORT_GROUPS];

MaterialSystem_Config_t g_materialSystemConfig;

static CSysModule			*g_MaterialsDLL = NULL;
bool g_LostVideoMemory = false;

IMaterial*	g_materialEmpty;	// purple checkerboard for missing textures

void ReleaseMaterialSystemObjects();
void RestoreMaterialSystemObjects();

static ConVar mat_shadowstate( "mat_shadowstate", "1" );
static ConVar mat_bufferprimitives( "mat_bufferprimitives", "1" );
static ConVar mat_filtertextures( "mat_filtertextures", "1" );
static ConVar mat_mipmaptextures( "mat_mipmaptextures", "1" );
static ConVar mat_showmiplevels(  "mat_showmiplevels", "0" );
static ConVar mat_filterlightmaps( "mat_filterlightmaps", "1" );
static ConVar mat_bumpmap(  "mat_bumpmap", "1" );
ConVar mat_specular(  "mat_specular", "1" );
static ConVar mat_diffuse(  "mat_diffuse", "1" );
static ConVar mat_compressedtextures(  "mat_compressedtextures", "1", FCVAR_ARCHIVE );
ConVar mat_softwarelighting( "mat_softwarelighting", "0" );
static ConVar mat_measurefillrate( "mat_measurefillrate", "0" );
static ConVar mat_fillrate( "mat_fillrate", "0" );
static ConVar mat_normalmaps( "mat_normalmaps", "0" );
static ConVar mat_showlowresimage( "mat_showlowresimage", "0" );
static ConVar mat_maxframelatency( "mat_maxframelatency", "1" );
ConVar mat_showbadnormals( "mat_showbadnormals", "0" );
static ConVar mat_forceaniso( "mat_forceaniso", "0" );
ConVar mat_trilinear( "mat_trilinear", "0" );
ConVar mat_bilinear( "mat_bilinear", "0" );

// in megatexels
static ConVar s_PerfWorldTexHiEnd( "perf_worldtexhiend", "15" );
static ConVar s_PerfWorldTexLowEnd( "perf_worldtexlowend", "4" );
static ConVar s_PerfOtherTexHiEnd( "perf_scenetexhiend", "3" );
static ConVar s_PerfOtherTexLowEnd( "perf_scenetexlowend", "2" );
static ConVar s_PerfDepthComplexity( "perf_depthcomplexity", "5.0" );
static ConVar s_PerfMaxTextureMemory( "perf_maxtexturememory", "90" );
static ConVar s_PerfMaxLightmapMemory( "perf_maxlightmapmemory", "6" );
static ConVar s_PerfMaxModelMemory( "perf_maxmodelmemory", "16" );
static ConVar s_PerfNewTextureMemoryPerFrame( "perf_newtexturememory", "1" );
static ConVar mat_proxy("mat_proxy", "0");
void WorldStaticMeshCreate( void );
void WorldStaticMeshDestroy( void );
static ConVar mat_slopescaledepthbias_decal( "mat_slopescaledepthbias_decal", "-1.0f" );
static ConVar mat_slopescaledepthbias_normal( "mat_slopescaledepthbias_normal", "0.0f" );
static ConVar mat_depthbias_decal( "mat_depthbias_decal", "-0.001f" );
static ConVar mat_depthbias_normal( "mat_depthbias_normal", "0.0f" );
static ConVar mat_fastnobump( "mat_fastnobump", "0" );
ConVar mat_norendering( "mat_norendering", "0" );

ConVar	r_norefresh( "r_norefresh","0");
ConVar	r_speeds( "r_speeds","0");
ConVar	r_speedsquiet( "r_speedsquiet","0", FCVAR_ARCHIVE );
ConVar	r_lightmapcolorscale( "r_lightmapcolorscale","1");
ConVar	r_decals( "r_decals","4096");
ConVar	mp_decals( "mp_decals","300", FCVAR_ARCHIVE);
ConVar	r_lightmap( "r_lightmap","-1");
ConVar	r_lightstyle( "r_lightstyle","-1");
ConVar	r_dynamic( "r_dynamic","1");

ConVar	mat_drawflat( "mat_drawflat","0", FCVAR_CHEAT);
ConVar	mat_fullbright( "mat_fullbright","0", FCVAR_CHEAT );
ConVar	mat_reversedepth( "mat_reversedepth", "0" );
ConVar	mat_polyoffset(  "mat_polyoffset", "4", FCVAR_ARCHIVE );
ConVar	mat_overbright(  "mat_overbright", "2" );
ConVar	mat_wireframe(  "mat_wireframe", "0", FCVAR_CHEAT );
ConVar	mat_luxels(  "mat_luxels", "0" );
ConVar	mat_showlightmappage(  "mat_showlightmappage", "-1" );
ConVar	mat_lightmapsonly( "mat_lightmapsonly", "0" );
ConVar	mat_normals(  "mat_normals", "0" );
ConVar	mat_bumpbasis(  "mat_bumpbasis", "0" );
ConVar	mat_leafvis(  "mat_leafvis", "0" );
ConVar	mat_envmapsize(  "mat_envmapsize", "128" );
ConVar  mat_envmaptgasize( "mat_envmaptgasize", "32.0" );
ConVar  mat_levelflush( "mat_levelflush", "1" );

ConVar		v_gamma( "gamma", "2.2" );
ConVar		v_brightness( "brightness", "0" );
ConVar		v_texgamma( "texgamma", "2.2" );
ConVar		v_linearFrameBuffer( "linearFrameBuffer", "0" );


ConVar mat_numtextureunits( "mat_numtextureunits", "0", 0, 
						   "Set to 0 to let the hardware/driver decide how many texture units are available.  Set to a positive integer to limit the number of texture units." );
ConVar mat_forcehardwaresync( "mat_forcehardwaresync", "1" );

MaterialSystem_SortInfo_t *materialSortInfoArray = 0;
int g_NumMaterialSortBins;
static bool s_bConfigLightingChanged = false;


//-----------------------------------------------------------------------------
// Purpose: Unloads the material system .dll
//-----------------------------------------------------------------------------
void UnloadMaterialSystemInterface( void )
{
	materialSystemInterface = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Loads the material system dll
//-----------------------------------------------------------------------------
void LoadMaterialSystemInterface( void )
{
	materialSystemInterface = (IMaterialSystem *)g_AppSystemFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );
	if ( !materialSystemInterface )
	{
		Sys_Error( "Could not get the material system interface from materialsystem.dll" );
	}
}

//-----------------------------------------------------------------------------
// return true if lightmaps need to be redownloaded
//-----------------------------------------------------------------------------
bool MaterialConfigLightingChanged()
{
	return s_bConfigLightingChanged;
}

void ClearMaterialConfigLightingChanged()
{
	s_bConfigLightingChanged = false;
}

void UpdateMaterialSystemConfig( void )
{
	if( host_state.worldmodel && !host_state.worldmodel->brush.lightdata )
	{
		mat_fullbright.SetValue( 1 );
	}
	
	memset( &g_materialSystemConfig, 0, sizeof(g_materialSystemConfig) );
	g_materialSystemConfig.screenGamma = v_gamma.GetFloat();
	g_materialSystemConfig.texGamma = v_texgamma.GetFloat();
	g_materialSystemConfig.bEditMode = false; // Not in WorldCraft..

	if ( g_pMaterialSystemHardwareConfig && g_pMaterialSystemHardwareConfig->SupportsOverbright())
		g_materialSystemConfig.overbright = mat_overbright.GetFloat();
	else
		g_materialSystemConfig.overbright = 0.0f;
	if ((g_materialSystemConfig.overbright != 2) && (g_materialSystemConfig.overbright != 1))
	{
		mat_overbright.SetValue( 2.0f );
		g_materialSystemConfig.overbright = 2;
	}

	g_materialSystemConfig.bAllowCheats = false; // hack
	g_materialSystemConfig.bLinearFrameBuffer = v_linearFrameBuffer.GetInt() ? true : false;
	g_materialSystemConfig.polyOffset = mat_polyoffset.GetFloat();
	g_materialSystemConfig.skipMipLevels = mat_picmip.GetInt();
	g_materialSystemConfig.bShowNormalMap = mat_normalmaps.GetInt() ? true : false;
	g_materialSystemConfig.bShowLowResImage = mat_showlowresimage.GetInt() ? true : false;
	if (materialSystemInterface)
	{
		int skipMipLevels = 0;
		if (g_pMaterialSystemHardwareConfig && g_pMaterialSystemHardwareConfig->TextureMemorySize() <= 4 * 1024 * 1024) 
			skipMipLevels = 2;
		else if ( g_pMaterialSystemHardwareConfig && g_pMaterialSystemHardwareConfig->TextureMemorySize() <= 8 * 1024 * 1024)
			skipMipLevels = 1;
		if (g_materialSystemConfig.skipMipLevels < skipMipLevels)
			g_materialSystemConfig.skipMipLevels = skipMipLevels;
	}						    
	g_materialSystemConfig.bMeasureFillRate = mat_measurefillrate.GetInt() ? true : false;
	g_materialSystemConfig.bVisualizeFillRate = mat_fillrate.GetInt() ? true : false;
	g_materialSystemConfig.lightScale = 1.0f;
	g_materialSystemConfig.bFilterLightmaps = mat_filterlightmaps.GetInt() ? true : false;
	g_materialSystemConfig.bFilterTextures = mat_filtertextures.GetInt() ? true : false;
	g_materialSystemConfig.bMipMapTextures = mat_mipmaptextures.GetInt() ? true : false;
	g_materialSystemConfig.bShowMipLevels = mat_showmiplevels.GetInt() ? true : false;
	g_materialSystemConfig.bReverseDepth = mat_reversedepth.GetInt() ? true : false;
	g_materialSystemConfig.bBufferPrimitives = mat_bufferprimitives.GetInt() ? true : false;
	g_materialSystemConfig.bDrawFlat = mat_drawflat.GetInt() ? true : false;
	g_materialSystemConfig.bSoftwareLighting = mat_softwarelighting.GetInt() ? true : false;
	g_materialSystemConfig.proxiesTestMode = mat_proxy.GetInt();
	g_materialSystemConfig.m_bSuppressRendering = mat_norendering.GetInt() != 0;

	int dxlevel = CommandLine()->ParmValue( "-dxlevel", 0 );
	if ( dxlevel < 60 )
	{
		dxlevel = 0;
	}

	g_materialSystemConfig.dxSupportLevel = dxlevel;

	if( mat_compressedtextures.GetInt() )
	{
		g_materialSystemConfig.bCompressedTextures = true;
	}
	else
	{
		g_materialSystemConfig.bCompressedTextures = false;
	}
	g_materialSystemConfig.bShowSpecular = mat_specular.GetInt() ? true : false;
	g_materialSystemConfig.bShowDiffuse = mat_diffuse.GetInt() ? true : false;
	g_materialSystemConfig.maxFrameLatency = mat_maxframelatency.GetInt();

#ifdef BENCHMARK
	if ( g_materialSystemConfig.maxFrameLatency == 666 )
	{
		g_materialSystemConfig.maxFrameLatency = -1;
	}

	if( g_materialSystemConfig.dxSupportLevel != 0 && g_materialSystemConfig.dxSupportLevel < 80 )
	{
		Error( "dx6 and dx7 hardware not supported by this benchmark!" );
	}
#endif

	if( mat_bumpmap.GetInt() )
	{
		g_materialSystemConfig.bBumpmap = true;
	}
	else
	{
		g_materialSystemConfig.bBumpmap = false;
	}
	if( mat_fullbright.GetInt() == 2 || mat_showbadnormals.GetInt() != 0 )
	{
		g_materialSystemConfig.bLightingOnly = true;
	}
	else
	{
		g_materialSystemConfig.bLightingOnly = false;
	}
	g_materialSystemConfig.numTextureUnits = mat_numtextureunits.GetInt();
	
	g_materialSystemConfig.m_MaxWorldMegabytes	= s_PerfWorldTexHiEnd.GetInt();
	g_materialSystemConfig.m_MaxOtherMegabytes	= s_PerfOtherTexHiEnd.GetInt();
	g_materialSystemConfig.m_MaxDepthComplexity = s_PerfDepthComplexity.GetFloat();
	g_materialSystemConfig.m_MaxTextureMemory	= s_PerfMaxTextureMemory.GetInt();
	g_materialSystemConfig.m_MaxLightmapMemory	= s_PerfMaxLightmapMemory.GetInt();
	g_materialSystemConfig.m_MaxModelMemory		= s_PerfMaxModelMemory.GetInt();
	g_materialSystemConfig.m_NewTextureMemoryPerFrame = s_PerfNewTextureMemoryPerFrame.GetInt();

	g_materialSystemConfig.m_nForceAnisotropicLevel = max( mat_forceaniso.GetInt(), 1 );
	g_materialSystemConfig.m_bForceTrilinear = mat_trilinear.GetBool();
	g_materialSystemConfig.m_bForceBilinear = mat_bilinear.GetBool();

	// Force trilinear on for high-end systems
	if ( (!g_materialSystemConfig.m_bForceBilinear) && (g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 80) )
	{
		g_materialSystemConfig.m_bForceTrilinear = true;
	}

#ifdef BENCHMARK
	if ( g_materialSystemConfig.m_nForceAnisotropicLevel )
	{
		if ( g_materialSystemConfig.m_nForceAnisotropicLevel > g_pMaterialSystemHardwareConfig->MaximumAnisotropicLevel() )
		{
			Error("This hardware does not support that level of anisotropic filtering!\n");
		}
	}
#endif

	g_materialSystemConfig.m_SlopeScaleDepthBias_Decal = mat_slopescaledepthbias_decal.GetFloat();
	g_materialSystemConfig.m_SlopeScaleDepthBias_Normal = mat_slopescaledepthbias_normal.GetFloat();
	g_materialSystemConfig.m_DepthBias_Decal = mat_depthbias_decal.GetFloat();
	g_materialSystemConfig.m_DepthBias_Normal = mat_depthbias_normal.GetFloat();
	g_materialSystemConfig.m_bFastNoBump = mat_fastnobump.GetInt() != 0;
	g_materialSystemConfig.m_bForceHardwareSync = mat_forcehardwaresync.GetBool();

	bool bLightmapsNeedReloading = materialSystemInterface->UpdateConfig( &g_materialSystemConfig, false );
	if ( bLightmapsNeedReloading )
	{
		s_bConfigLightingChanged = true;
	}
}

//-----------------------------------------------------------------------------
// A console command to debug materials
//-----------------------------------------------------------------------------

void Cmd_MatDebug_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("usage: mat_debug [ <material name> ]\n");
		return;
	}

	materialSystemInterface->ToggleDebugMaterial( Cmd_Argv(1) );
}

//-----------------------------------------------------------------------------
// A console command to suppress materials
//-----------------------------------------------------------------------------

void Cmd_MatSuppress_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("usage: mat_suppress [ <material name> ]\n");
		return;
	}

	materialSystemInterface->ToggleSuppressMaterial( Cmd_Argv(1) );
}


//-----------------------------------------------------------------------------
// Purpose: Make the debug system materials
//-----------------------------------------------------------------------------
static void InitDebugMaterials( void )
{
	g_materialEmpty = GL_LoadMaterial( "debug/debugempty" );
#ifndef SWDS
	g_materialWireframe = GL_LoadMaterial( "debug/debugwireframe" );
	g_materialTranslucentSingleColor = GL_LoadMaterial( "debug/debugtranslucentsinglecolor" );
	g_materialTranslucentVertexColor = GL_LoadMaterial( "debug/debugtranslucentvertexcolor" );
	g_materialWorldWireframe = GL_LoadMaterial( "debug/debugworldwireframe" );
	g_materialWorldWireframeZBuffer = GL_LoadMaterial( "debug/debugworldwireframezbuffer" );

	g_materialBrushWireframe = GL_LoadMaterial( "debug/debugbrushwireframe" );
	g_materialDecalWireframe = GL_LoadMaterial( "debug/debugdecalwireframe" );
	g_materialDebugLightmap = GL_LoadMaterial( "debug/debuglightmap" );
	g_materialDebugLightmapZBuffer = GL_LoadMaterial( "debug/debuglightmapzbuffer" );
	g_materialDebugLuxels = GL_LoadMaterial( "debug/debugluxels" );

	g_materialLeafVisWireframe = GL_LoadMaterial( "debug/debugleafviswireframe" );
	g_materialVolumetricFog = GL_LoadMaterial( "engine/volumetricfog" );
	g_pMaterialWireframeVertexColor = GL_LoadMaterial( "debug/debugwireframevertexcolor" );
	g_pMaterialWireframeVertexColorIgnoreZ = GL_LoadMaterial( "debug/debugwireframevertexcolorignorez" );
	g_pMaterialLightSprite = GL_LoadMaterial( "engine/lightsprite" );
	g_pMaterialShadowBuild = GL_LoadMaterial( "engine/shadowbuild" );
	g_pMaterialMRMWireframe = GL_LoadMaterial( "debug/debugmrmwireframe" );

	g_pMaterialWriteZ = GL_LoadMaterial( "engine/writez" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void ShutdownDebugMaterials( void )
{
	GL_UnloadMaterial( g_materialEmpty );
#ifndef SWDS
	GL_UnloadMaterial( g_pMaterialLightSprite );
	GL_UnloadMaterial( g_pMaterialWireframeVertexColor );
	GL_UnloadMaterial( g_pMaterialWireframeVertexColorIgnoreZ );
	GL_UnloadMaterial( g_materialVolumetricFog );
	GL_UnloadMaterial( g_materialLeafVisWireframe );

	GL_UnloadMaterial( g_materialDebugLuxels );
	GL_UnloadMaterial( g_materialDebugLightmapZBuffer );
	GL_UnloadMaterial( g_materialDebugLightmap );
	GL_UnloadMaterial( g_materialDecalWireframe );
	GL_UnloadMaterial( g_materialBrushWireframe );

	GL_UnloadMaterial( g_materialWorldWireframeZBuffer );
	GL_UnloadMaterial( g_materialWorldWireframe );
	GL_UnloadMaterial( g_materialTranslucentSingleColor );
	GL_UnloadMaterial( g_materialTranslucentVertexColor );
	GL_UnloadMaterial( g_materialWireframe );
	GL_UnloadMaterial( g_pMaterialShadowBuild );
	GL_UnloadMaterial( g_pMaterialMRMWireframe );
	GL_UnloadMaterial( g_pMaterialWriteZ );
#endif
}

//-----------------------------------------------------------------------------
// A console command to spew out driver information
//-----------------------------------------------------------------------------

void Cmd_MatInfo_f (void)
{
	materialSystemInterface->SpewDriverInfo();
}

void InitMaterialSystem( void )
{
	materialSystemInterface->AddReleaseFunc( ReleaseMaterialSystemObjects );
	materialSystemInterface->AddRestoreFunc( RestoreMaterialSystemObjects );

	UpdateMaterialSystemConfig();

	InitDebugMaterials();
#ifndef SWDS
	DispInfo_InitMaterialSystem();
#endif

#ifdef BENCHMARK
	if ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 80 )
	{
		Error( "dx6 and dx7 hardware not supported by this benchmark!" );
	}
#endif
}

void ShutdownMaterialSystem( void )
{
	ShutdownDebugMaterials();
#ifndef SWDS
	DispInfo_ShutdownMaterialSystem();
#endif
}

//-----------------------------------------------------------------------------
// Methods to restore, release material system objects
//-----------------------------------------------------------------------------

void ReleaseMaterialSystemObjects()
{
#ifndef SWDS
	DispInfo_ReleaseMaterialSystemObjects( host_state.worldmodel );
#endif
	modelloader->Studio_ReleaseModels();
#ifndef SWDS
	WorldStaticMeshDestroy();
#endif
	g_LostVideoMemory = true;
}

void RestoreMaterialSystemObjects()
{
	g_LostVideoMemory = false;

	modelloader->Studio_RestoreModels();

	if (host_state.worldmodel)
	{
		modelloader->Map_LoadDisplacements( host_state.worldmodel, true );
#ifndef SWDS
		WorldStaticMeshCreate();
		// Gotta recreate the lightmaps
		R_RedownloadAllLightmaps( vec3_origin );
		materialSystemInterface->FlushLightmaps();

		// Need to re-figure out the env_cubemaps, so blow away the lightcache.
		R_StudioInitLightingCache();
#endif
	}
}

bool TangentSpaceSurfaceSetup( int surfID, Vector &tVect )
{
	Vector sVect;
	VectorCopy( MSurf_TexInfo( surfID )->textureVecsTexelsPerWorldUnits[0].AsVector3D(), sVect );
	VectorCopy( MSurf_TexInfo( surfID )->textureVecsTexelsPerWorldUnits[1].AsVector3D(), tVect );
	VectorNormalize( sVect );
	VectorNormalize( tVect );
	Vector tmpVect;
	CrossProduct( sVect, tVect, tmpVect );
	// Make sure that the tangent space works if textures are mapped "backwards".
	if( DotProduct( MSurf_Plane( surfID ).normal, tmpVect ) > 0.0f )
	{
		return true;
	}
	return false;
}

void TangentSpaceComputeBasis( Vector& tangent, Vector& binormal, const Vector& normal, const Vector& tVect, bool negateTangent )
{
	// tangent x binormal = normal
	// tangent = sVect
	// binormal = tVect
	CrossProduct( normal, tVect, tangent );
	VectorNormalize( tangent );
	CrossProduct( tangent, normal, binormal );
	VectorNormalize( binormal );

	if( negateTangent )
	{
		VectorScale( tangent, -1.0f, tangent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void MaterialSystem_DestroySortinfo( void )
{
	if ( materialSortInfoArray )
	{
#ifndef SWDS
		WorldStaticMeshDestroy();
#endif

		delete[] materialSortInfoArray;
		materialSortInfoArray = NULL;
	}
}


#ifndef SWDS

// scale a byte, saturating at [0,255]
static byte SaturatedScale( byte color, float scale )
{
	scale *= color;
	if ( scale > 255 )
		scale = 255;
	else if ( scale < 0 )
		scale = 0;

	return (byte)(int)(scale + 0.5f);
}

static void RandomColor( int key, byte flatColor[4], float overbright )
{
	static byte flatcolors[256*3];
	static bool firsttime = true;
	
	if ( firsttime )
	{
		firsttime = false;

		for (int i = 0; i < 256 * 3; i += 3) 
		{
			Vector color;
			color[0] = RandomFloat( 0.0f, 1.0f );
			color[1] = RandomFloat( 0.0f, 1.0f );
			color[2] = RandomFloat( 0.0f, 1.0f );
			if( color[0] == 0.0f && color[1] == 0.0f && color[2] == 0.0f )
			{
				color[0] = 1.0f;
			}
			VectorNormalize(color);
			flatcolors[i+0] = SaturatedScale( 255, color[0] );
			flatcolors[i+1] = SaturatedScale( 255, color[1] );
			flatcolors[i+2] = SaturatedScale( 255, color[2] );
		}
	}

	key = (key & 255) * 3;
	if ( overbright != 0 )
	{
		overbright = 1.0f/overbright;
	}
	else
	{
		overbright = 1.0f;
	}

	flatColor[0] = SaturatedScale(flatcolors[key+0], overbright);
	flatColor[1] = SaturatedScale(flatcolors[key+1], overbright);
	flatColor[2] = SaturatedScale(flatcolors[key+2], overbright);
}


//-----------------------------------------------------------------------------
// Purpose: Build a vertex buffer for this face
// Input  : *pWorld - world model base
//			*surf - surf to add to the mesh
//			overbright - overbright factor (for colors)
//			&builder - mesh that holds the vertex buffer
//-----------------------------------------------------------------------------
void BuildMSurfaceVertexArrays( model_t *pWorld, int surfID, float overbright, 
							   CMeshBuilder &builder )
{
	SurfaceCtx_t ctx;
	SurfSetupSurfaceContext( ctx, surfID );

	byte flatColor[4];

	Vector tVect;
	bool negate = false;
	if ( MSurf_Flags( surfID ) & SURFDRAW_TANGENTSPACE )
	{
		negate = TangentSpaceSurfaceSetup( surfID, tVect );
	}

	for ( int i = 0; i < MSurf_VertCount( surfID ); i++ )
	{
		int vertIndex = pWorld->brush.vertindices[MSurf_FirstVertIndex( surfID ) + i];

		// world-space vertex
		Vector& vec = pWorld->brush.vertexes[vertIndex].position;

		// output to mesh
		builder.Position3fv( vec.Base() );

		Vector2D uv;
		SurfComputeTextureCoordinate( ctx, surfID, vec, uv );
		builder.TexCoord2fv( 0, uv.Base() );

		// garymct: normalized (within space of surface) lightmap texture coordinates
		SurfComputeLightmapCoordinate( ctx, surfID, vec, uv );
		builder.TexCoord2fv( 1, uv.Base() );

		if ( MSurf_Flags( surfID ) & SURFDRAW_BUMPLIGHT )
		{
			// bump maps appear left to right in lightmap page memory, calculate 
			// the offset for the width of a single map. The pixel shader will use 
			// this to compute the actual texture coordinates
			builder.TexCoord2f( 2, ctx.m_BumpSTexCoordOffset, 0.0f );
		}

		Vector& normal = pWorld->brush.vertnormals[ pWorld->brush.vertnormalindices[MSurf_FirstVertNormal( surfID ) + i] ];
		builder.Normal3fv( normal.Base() );

		if ( MSurf_Flags( surfID ) & SURFDRAW_TANGENTSPACE )
		{
			Vector binormal, tangent;
			TangentSpaceComputeBasis( tangent, binormal, normal, tVect, negate );
			builder.TangentS3fv( tangent.Base() );
			builder.TangentT3fv( binormal.Base() );
		}

		RandomColor( surfID, flatColor, overbright );
		builder.Color3ubv( flatColor );

		builder.AdvanceVertex();
	}
}


//-----------------------------------------------------------------------------
// Builds a static mesh from a list of all surfaces with the same material
//-----------------------------------------------------------------------------

static IMesh* WorldStaticMeshCreate_Helper( int surfID )
{
	if ( !IS_SURF_VALID( surfID ) )
		return 0;

	// Count the total # of verts + triangles
	int vertCount = 0;
	int triCount = 0;
	int iter;
	for ( iter = surfID; iter >= 0; iter = MSurf_TextureChain( iter ) )
	{
		vertCount += MSurf_VertCount( iter );
		triCount += MSurf_VertCount( iter ) - 2;
	}

	IMesh* pMesh = materialSystemInterface->CreateStaticMesh( MSurf_TexInfo( surfID )->material );

	// NOTE: Index count is zero because this will be a static vertex buffer!!!
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_HETEROGENOUS, vertCount, 0 );

	int vertBufferIndex = 0;
	while ( IS_SURF_VALID( surfID ) )
	{
		MSurf_VertBufferIndex( surfID ) = vertBufferIndex;
		BuildMSurfaceVertexArrays( host_state.worldmodel, surfID, mat_overbright.GetFloat(), meshBuilder );
		
		vertBufferIndex += MSurf_VertCount( surfID );
		surfID = MSurf_TextureChain( surfID );
	}
	meshBuilder.End();

	return pMesh;
}


//-----------------------------------------------------------------------------
// Builds static meshes for all the world geometry
//-----------------------------------------------------------------------------
void WorldStaticMeshCreate( void )
{
	WorldStaticMeshDestroy();

	g_pWorldStatic = new MatSortArray_t[g_NumMaterialSortBins];

	// clear the arrays
	memset( g_pWorldStatic, 0, sizeof(*g_pWorldStatic) * g_NumMaterialSortBins );

	int i;
	for ( i = 0; i < g_NumMaterialSortBins; i++ )
	{
		g_pWorldStatic[i].m_Surf[0].m_nHeadSurfID = -1;
	}

	// sort the surfaces into the sort arrays
	int surfID;
	for( surfID = 0; surfID < host_state.worldmodel->brush.numsurfaces; surfID++ )
	{
		// set these flags here as they are determined by material data
		MSurf_Flags( surfID ) &= ~(SURFDRAW_DYNAMIC|SURFDRAW_TANGENTSPACE);

		// Keep this out of the static buffer
		if ( MSurf_TexInfo( surfID )->material->WillPreprocessData() )
		{
			MSurf_Flags( surfID ) |= SURFDRAW_DYNAMIC;
		}

		// do we need to compute tangent space here?
		if ( MSurf_TexInfo( surfID )->material->GetVertexFormat() & VERTEX_TANGENT_SPACE )
		{
			MSurf_Flags( surfID ) |= SURFDRAW_TANGENTSPACE;
		}
		
		// don't create vertex buffers for nodraw faces, water faces, or faces with dynamic data
		if ( (MSurf_Flags( surfID ) & (SURFDRAW_NODRAW|SURFDRAW_WATERSURFACE|SURFDRAW_DYNAMIC)) 
			|| SurfaceHasDispInfo( surfID ) )
		{
			MSurf_VertBufferIndex( surfID ) = 0xFFFF;
			continue;
		}

		// attach to head of list
		MSurf_TextureChain( surfID ) = g_pWorldStatic[MSurf_MaterialSortID( surfID )].m_Surf[0].m_nHeadSurfID;
		g_pWorldStatic[MSurf_MaterialSortID( surfID )].m_Surf[0].m_nHeadSurfID = surfID;
	}

	// iterate the arrays and create buffers
	for ( i = 0; i < g_NumMaterialSortBins; i++ )
	{
		g_pWorldStatic[i].m_pMesh = WorldStaticMeshCreate_Helper( g_pWorldStatic[i].m_Surf[0].m_nHeadSurfID );
	}
}

void WorldStaticMeshDestroy( void )
{
	if ( !g_pWorldStatic )
		return;

	// Blat out the static meshes associated with each material
	for ( int i = 0; i < g_NumMaterialSortBins; i++ )
	{
		if ( g_pWorldStatic[i].m_pMesh )
		{
			materialSystemInterface->DestroyStaticMesh( g_pWorldStatic[i].m_pMesh );
		}
	}
	delete[] g_pWorldStatic;
	g_pWorldStatic = NULL;
}


//-----------------------------------------------------------------------------
// Compute texture and lightmap coordinates
//-----------------------------------------------------------------------------

void SurfComputeTextureCoordinate( SurfaceCtx_t const& ctx, int surfID, 
									    Vector const& vec, Vector2D& uv )
{
	mtexinfo_t* pTexInfo = MSurf_TexInfo( surfID );

	// base texture coordinate
	uv.x = DotProduct (vec, pTexInfo->textureVecsTexelsPerWorldUnits[0].AsVector3D()) + 
		pTexInfo->textureVecsTexelsPerWorldUnits[0][3];
	uv.x /= pTexInfo->material->GetMappingWidth();

	uv.y = DotProduct (vec, pTexInfo->textureVecsTexelsPerWorldUnits[1].AsVector3D()) + 
		pTexInfo->textureVecsTexelsPerWorldUnits[1][3];
	uv.y /= pTexInfo->material->GetMappingHeight();
}

void SurfComputeLightmapCoordinate( SurfaceCtx_t const& ctx, int surfID, 
										 Vector const& vec, Vector2D& uv )
{
	if ( MSurf_Flags( surfID ) & SURFDRAW_NOLIGHT )
	{
		uv.x = uv.y = 0.5f;
	}
	else
	{
		mtexinfo_t* pTexInfo = MSurf_TexInfo( surfID );

		uv.x = DotProduct (vec, pTexInfo->lightmapVecsLuxelsPerWorldUnits[0].AsVector3D()) + 
			pTexInfo->lightmapVecsLuxelsPerWorldUnits[0][3];
		uv.x -= MSurf_LightmapMins( surfID )[0];
		uv.x += 0.5f;

		uv.y = DotProduct (vec, pTexInfo->lightmapVecsLuxelsPerWorldUnits[1].AsVector3D()) + 
			pTexInfo->lightmapVecsLuxelsPerWorldUnits[1][3];
		uv.y -= MSurf_LightmapMins( surfID )[1];
		uv.y += 0.5f;

		uv *= ctx.m_Scale;
		uv += ctx.m_Offset;

		assert( uv.IsValid() );
	}
}


//-----------------------------------------------------------------------------
// Compute a context necessary for creating vertex data
//-----------------------------------------------------------------------------

void SurfSetupSurfaceContext( SurfaceCtx_t& ctx, int surfID )
{
	materialSystemInterface->GetLightmapPageSize( 
		SortInfoToLightmapPage( MSurf_MaterialSortID( surfID ) ), 
		&ctx.m_LightmapPageSize[0], &ctx.m_LightmapPageSize[1] );
	ctx.m_LightmapSize[0] = ( MSurf_LightmapExtents( surfID )[0] ) + 1;
	ctx.m_LightmapSize[1] = ( MSurf_LightmapExtents( surfID )[1] ) + 1;

	ctx.m_Scale.x = 1.0f / ( float )ctx.m_LightmapPageSize[0];
	ctx.m_Scale.y = 1.0f / ( float )ctx.m_LightmapPageSize[1];

	ctx.m_Offset.x = ( float )MSurf_OffsetIntoLightmapPage( surfID )[0] * ctx.m_Scale.x;
	ctx.m_Offset.y = ( float )MSurf_OffsetIntoLightmapPage( surfID )[1] * ctx.m_Scale.y;

	ctx.m_BumpSTexCoordOffset = ( float )ctx.m_LightmapSize[0] / ( float )ctx.m_LightmapPageSize[0];
}


#endif
