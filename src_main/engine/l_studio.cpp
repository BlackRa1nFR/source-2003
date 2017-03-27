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
// l_studio.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.
//=============================================================================

#include "quakedef.h"

#include "glquake.h"
#include "modelgen.h"
#include "gl_model_private.h"
#include "gl_texture.h"
#include "studio.h"
#include "gl_cvars.h"
#include "gl_matsysiface.h"
#include "phyfile.h"
#include "cdll_int.h"
#include "istudiorender.h"
#include "client_class.h"
#include "float.h"
#include "r_local.h"
#include "cmodel_engine.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "enginestats.h"
#include "modelloader.h"
#include "lightcache.h"
#include "studio_internal.h"
#include "cdll_engine_int.h"
#include "vphysics_interface.h"
#include "utllinkedlist.h"
#include "view.h"
#include "studio.h"
#include "gl_rsurf.h"
#include "icliententitylist.h"
#include "engine/ivmodelrender.h"
#include "render.h"
#include "modelloader.h"
#include "optimize.h"
#include "materialsystem/imesh.h"
#include "icliententity.h"
#include "sys_dll.h"
#include "gl_rmain.h"
#include "debugoverlay.h"
#include "enginetrace.h"
#include "l_studio.h"
#include "IOcclusionSystem.h"
#include "filesystem_engine.h"
#include "ModelInfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
void R_StudioInitLightingCache( void );
float Engine_WorldLightDistanceFalloff( const dworldlight_t *wl, const Vector& delta, bool bNoRadiusCheck = false );


//-----------------------------------------------------------------------------
// Model rendering state
//-----------------------------------------------------------------------------
struct DrawModelState_t
{
	studiohdr_t*			m_pStudioHdr;
	IClientRenderable*		m_pRenderable;
	const matrix3x4_t		*m_pModelToWorld;
	const Vector			*m_BBoxMins;
	const Vector			*m_BBoxMaxs;
};


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

extern ConVar r_worldlights;
static ConVar r_lod( "r_lod", "-1" );
static ConVar r_drawmodelstatsoverlay( "r_drawmodelstatsoverlay", "0" );
static ConVar r_drawmodelstatsoverlaydistance( "r_drawmodelstatsoverlaydistance", "0" );
static ConVar r_lightinterp( "r_lightinterp", "5", 0, "Controls the speed of light interpolation, 0 turns off interpolation" );
IStudioRender *g_pStudioRender = 0;


//=============================================================================


/*
=================
Mod_LoadStudioModel
=================
*/
bool Mod_LoadStudioModel (model_t *mod, void *buffer, bool zerostructure )
{
	byte				*pin, *pout;
	studiohdr_t			*phdr;

	int					version;

	pin = (byte *)buffer;

	phdr = (studiohdr_t *)pin;

	Studio_ConvertStudioHdrToNewVersion( phdr );
	
	version = LittleLong (phdr->version);
	if (version != STUDIO_VERSION)
	{
		memset( phdr, 0, sizeof( studiohdr_t ));

		Msg("%s has wrong version number (%i should be %i)\n", mod->name, version, STUDIO_VERSION);
		return false;
	}


	// Make sure we don't have too many verts in each model (will kill decal vertex caching if so)
	int bodyPartID;
	for( bodyPartID = 0; bodyPartID < phdr->numbodyparts; bodyPartID++ )
	{
		mstudiobodyparts_t *pBodyPart = phdr->pBodypart( bodyPartID );
		int modelID;
		for( modelID = 0; modelID < pBodyPart->nummodels; modelID++ )
		{
			mstudiomodel_t *pModel = pBodyPart->pModel( modelID );
//			Assert( pModel->numvertices < MAXSTUDIOVERTS );
			if( pModel->numvertices >= MAXSTUDIOVERTS )
			{
				Warning( "%s exceeds MAXSTUDIOVERTS (%d >= %d)\n", phdr->name, pModel->numvertices,
					( int )MAXSTUDIOVERTS );
				return false;
			}
		}
	}

	mod->type = mod_studio;
	// clear out studio specific data if loading for the first time
	if ( zerostructure )
	{
		memset( &mod->studio, 0, sizeof(mod->studio) );
	}

	// Allocate cache space for a working header, texture data and old (3*byte*256) palettes
	Cache_Alloc( &mod->cache, phdr->length, mod->name );
	if (!mod->cache.data)
		return false;

	pout = (byte *)mod->cache.data;

//
// move the complete, relocatable alias model to the cache
//	
	memcpy( pout, pin, phdr->length );

	VectorCopy( phdr->hull_min, mod->mins );
	VectorCopy( phdr->hull_max, mod->maxs );
	mod->radius = 0;
	for ( int i = 0; i < 3; i++ )
	{
		if ( fabs(mod->mins[i]) > mod->radius )
			mod->radius = fabs(mod->mins[i]);
		if ( fabs(mod->maxs[i]) > mod->radius )
			mod->radius = fabs(mod->maxs[i]);
	}

	if (version != STUDIO_VERSION)
	{
		return false;
	}
	else
	{
		// force the collision to load
		Mod_VCollide( mod );
		return true;
	}
}

//-----------------------------------------------------------------------------
// Release static meshes when task switch happens
//-----------------------------------------------------------------------------

void Mod_ReleaseStudioModel(model_t *mod)
{
	if( mod->studio.studiomeshLoaded )
	{
		g_pStudioRender->UnloadModel( &mod->studio.hardwareData );
		memset( &mod->studio.hardwareData, 0, sizeof( mod->studio.hardwareData ) );
		mod->studio.studiomeshLoaded = false;
	}
}
 
static void Mod_FreeVCollide( model_t *pModel )
{
	if ( !pModel->studio.vcollisionLoaded )
		return;

	pModel->studio.vcollisionLoaded = false;
	vcollide_t *pCollide = &pModel->studio.vcollisionData;
	physcollision->VCollideUnload( pCollide );
}

// Call destructors on certain things in the map.
void Mod_UnloadStudioModel(model_t *mod)
{
	if( mod->studio.studiomeshLoaded )
	{
		g_pStudioRender->UnloadModel( &mod->studio.hardwareData );
		memset( &mod->studio.hardwareData, 0, sizeof( mod->studio.hardwareData ) );
		mod->studio.studiomeshLoaded = false;
	}
	Mod_FreeVCollide( mod );
	if( Cache_Check( &mod->cache ) )
	{
		Cache_Free( &mod->cache );
	}
}

//=============================================================================
vcollide_t *Mod_VCollide( model_t *pModel )
{
	if ( pModel->type != mod_studio )
	{
		return NULL;
	}

	if ( pModel->studio.vcollisionLoaded )
	{
		// We've loaded an empty collision file or no file was found, so return NULL
		if ( !pModel->studio.vcollisionData.solidCount )
			return NULL;

		return &pModel->studio.vcollisionData;
	}

	// load the collision data
	pModel->studio.vcollisionLoaded = true;
	// clear existing data
	memset( &pModel->studio.vcollisionData, 0, sizeof( pModel->studio.vcollisionData ) );

	char fileName[256];
	int fileSize = 0;

	COM_StripExtension( pModel->name, fileName, sizeof( fileName ) );
	COM_DefaultExtension( fileName, ".PHY", sizeof( fileName ) );
	
	// load into tempalloc
	byte *buf = COM_LoadFile( fileName, 2, &fileSize );
	if ( !buf )
		return NULL;

	phyheader_t header;
	memcpy( &header, buf, sizeof(header) );
	buf += sizeof(header);
	fileSize -= sizeof(header);

	if ( header.size != sizeof(header) || header.solidCount <= 0 )
		return NULL;

	vcollide_t *pCollide = &pModel->studio.vcollisionData;
	physcollision->VCollideLoad( pCollide, header.solidCount, (const char *)buf, fileSize );
	
	return pCollide;
}


static ConVar	r_showenvcubemap( "r_showenvcubemap", "0" );
static ConVar	r_staticlighting( "r_staticlighting", "1" );
static ConVar	r_eyegloss		( "r_eyegloss", "1", FCVAR_ARCHIVE ); // wet eyes
static ConVar	r_eyemove		( "r_eyemove", "1", FCVAR_ARCHIVE ); // look around
static ConVar	r_eyeshift_x	( "r_eyeshift_x", "0", FCVAR_ARCHIVE ); // eye X position
static ConVar	r_eyeshift_y	( "r_eyeshift_y", "0", FCVAR_ARCHIVE ); // eye Y position
static ConVar	r_eyeshift_z	( "r_eyeshift_z", "0", FCVAR_ARCHIVE ); // eye Z position
static ConVar	r_eyesize		( "r_eyesize", "0", FCVAR_ARCHIVE ); // adjustment to iris textures
static ConVar	mat_softwareskin( "mat_softwareskin", "0" );
static ConVar	r_nohw			( "r_nohw", "0" );
static ConVar	r_nosw			( "r_nosw", "0" );
static ConVar	r_teeth			( "r_teeth", "1" );
static ConVar	r_drawentities	( "r_drawentities", "1" );
static ConVar	r_flex			( "r_flex", "1" );
static ConVar	r_eyes			( "r_eyes", "1" );
static ConVar	r_skin			( "r_skin","0" );
static ConVar	r_useambientcube( "r_useambientcube", "1" );
static ConVar	r_maxmodeldecal ( "r_maxmodeldecal", "50" );
static ConVar	r_modelwireframedecal ( "r_modelwireframedecal", "0" );

static StudioRenderConfig_t s_StudioRenderConfig;

void UpdateStudioRenderConfig( void )
{
	memset( &s_StudioRenderConfig, 0, sizeof(s_StudioRenderConfig) );

	s_StudioRenderConfig.eyeGloss = r_eyegloss.GetInt();
	s_StudioRenderConfig.bEyeMove = !!r_eyemove.GetInt();
	s_StudioRenderConfig.fEyeShiftX = r_eyeshift_x.GetFloat();
	s_StudioRenderConfig.fEyeShiftY = r_eyeshift_y.GetFloat();
	s_StudioRenderConfig.fEyeShiftZ = r_eyeshift_z.GetFloat();
	s_StudioRenderConfig.fEyeSize = r_eyesize.GetFloat();	
	if( mat_softwareskin.GetInt() || mat_wireframe.GetInt() )
	{
		s_StudioRenderConfig.bSoftwareSkin = true;
	}
	else
	{
		s_StudioRenderConfig.bSoftwareSkin = false;
	}
	s_StudioRenderConfig.bNoHardware = !!r_nohw.GetInt();
	s_StudioRenderConfig.bNoSoftware = !!r_nosw.GetInt();
	s_StudioRenderConfig.bTeeth = !!r_teeth.GetInt();
	s_StudioRenderConfig.drawEntities = r_drawentities.GetInt();
	s_StudioRenderConfig.bFlex = !!r_flex.GetInt();
	s_StudioRenderConfig.bEyes = !!r_eyes.GetInt();
	s_StudioRenderConfig.bWireframe = !!mat_wireframe.GetInt();
	s_StudioRenderConfig.bNormals = mat_normals.GetBool();
	s_StudioRenderConfig.skin = r_skin.GetInt();
	s_StudioRenderConfig.bUseAmbientCube = !!r_useambientcube.GetInt();
	s_StudioRenderConfig.modelLightBias = 1.0f;
	s_StudioRenderConfig.maxDecalsPerModel = r_maxmodeldecal.GetInt();
	s_StudioRenderConfig.bWireframeDecals = r_modelwireframedecal.GetInt() != 0;
	
	s_StudioRenderConfig.fullbright = mat_fullbright.GetInt();
	s_StudioRenderConfig.bSoftwareLighting = mat_softwarelighting.GetInt() != 0;
	s_StudioRenderConfig.pConDPrintf = Con_DPrintf;
	s_StudioRenderConfig.pConPrintf = Con_Printf;

	s_StudioRenderConfig.gamma = v_gamma.GetFloat();
	s_StudioRenderConfig.texGamma = v_texgamma.GetFloat();
	s_StudioRenderConfig.overbrightFactor =  1.0;
	if (g_pMaterialSystemHardwareConfig->SupportsOverbright())
		s_StudioRenderConfig.overbrightFactor = mat_overbright.GetFloat();
	s_StudioRenderConfig.brightness = v_brightness.GetFloat();
	s_StudioRenderConfig.bShowEnvCubemapOnly = r_showenvcubemap.GetInt() ? true : false;
	s_StudioRenderConfig.bStaticLighting = r_staticlighting.GetBool();
	s_StudioRenderConfig.r_speeds = r_speeds.GetInt();
	g_pStudioRender->UpdateConfig( s_StudioRenderConfig );
}

void R_InitStudio( void )
{
#ifndef SWDS
	R_StudioInitLightingCache();
#endif
}


//-----------------------------------------------------------------------------
// Converts world lights to materialsystem lights
//-----------------------------------------------------------------------------

#define MIN_LIGHT_VALUE 0.03f

bool WorldLightToMaterialLight( dworldlight_t* pWorldLight, LightDesc_t& light )
{
	// BAD
	light.m_Attenuation0 = 0.0f;
	light.m_Attenuation1 = 0.0f;
	light.m_Attenuation2 = 0.0f;

	switch(pWorldLight->type)
	{
	case emit_spotlight:
		light.m_Type = MATERIAL_LIGHT_SPOT;
		light.m_Attenuation0 = pWorldLight->constant_attn;
		light.m_Attenuation1 = pWorldLight->linear_attn;
		light.m_Attenuation2 = pWorldLight->quadratic_attn;
		light.m_Theta = 2.0 * acos( pWorldLight->stopdot );
		light.m_Phi = 2.0 * acos( pWorldLight->stopdot2 );
		light.m_ThetaDot = pWorldLight->stopdot;
		light.m_PhiDot = pWorldLight->stopdot2;
		light.m_Falloff = pWorldLight->exponent ? pWorldLight->exponent : 1.0f;
		break;

	case emit_surface:
		// A 180 degree spotlight
		light.m_Type = MATERIAL_LIGHT_SPOT;
		light.m_Attenuation2 = 1.0;
		light.m_Theta = 0;
		light.m_Phi = M_PI;
		light.m_ThetaDot = 1.0f;
		light.m_PhiDot = 0.0f;
		light.m_Falloff = 1.0f;
		break;

	case emit_point:
		light.m_Type = MATERIAL_LIGHT_POINT;
		light.m_Attenuation0 = pWorldLight->constant_attn;
		light.m_Attenuation1 = pWorldLight->linear_attn;
		light.m_Attenuation2 = pWorldLight->quadratic_attn;
		break;

	case emit_skylight:
		light.m_Type = MATERIAL_LIGHT_DIRECTIONAL;
		break;

	// NOTE: Can't do quake lights in hardware (x-r factor)
	case emit_quakelight:	// not supported
	case emit_skyambient:	// doesn't factor into local lighting
		// skip these
		return false;
	}

	// No attenuation case..
	if ((light.m_Attenuation0 == 0.0f) && (light.m_Attenuation1 == 0.0f) &&
		(light.m_Attenuation2 == 0.0f))
	{
		light.m_Attenuation0 = 1.0f;
	}

	// renormalize light intensity...
	memcpy( &light.m_Position, &pWorldLight->origin, 3 * sizeof(float) );
	memcpy( &light.m_Direction, &pWorldLight->normal, 3 * sizeof(float) );
	light.m_Color[0] = pWorldLight->intensity[0];
	light.m_Color[1] = pWorldLight->intensity[1];
	light.m_Color[2] = pWorldLight->intensity[2];

	// Make it stop when the lighting gets to min%...
	float intensity = sqrtf( DotProduct( light.m_Color, light.m_Color ) );

	// Compute the light range based on attenuation factors
	if (pWorldLight->radius != 0)
	{
		light.m_Range = pWorldLight->radius;
	}
	else
	{
		// FALLBACK: older lights use this
		if (light.m_Attenuation2 == 0.0f)
		{
			if (light.m_Attenuation1 == 0.0f)
			{
				light.m_Range = sqrtf(FLT_MAX);
			}
			else
			{
				light.m_Range = (intensity / MIN_LIGHT_VALUE - light.m_Attenuation0) / light.m_Attenuation1;
			}
		}
		else
		{
			float a = light.m_Attenuation2;
			float b = light.m_Attenuation1;
			float c = light.m_Attenuation0 - intensity / MIN_LIGHT_VALUE;
			float discrim = b * b - 4 * a * c;
			if (discrim < 0.0f)
				light.m_Range = sqrtf(FLT_MAX);
			else
			{
				light.m_Range = (-b + sqrtf(discrim)) / (2.0f * a);
				if (light.m_Range < 0)
					light.m_Range = 0;
			}
		}
	}

	// FIXME (maybe?)  Are we doing this redundantly in some cases.
	light.m_Flags = 0;
	if( light.m_Attenuation0 != 0.0f )
	{
		light.m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0;
	}
	if( light.m_Attenuation1 != 0.0f )
	{
		light.m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1;
	}
	if( light.m_Attenuation2 != 0.0f )
	{
		light.m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Sets the hardware lighting state
//-----------------------------------------------------------------------------

static void R_SetNonAmbientLightingState( float overbrightFactor, int numLights, dworldlight_t *locallight[MAXLOCALLIGHTS] )
{
	// convert dworldlight_t's to LightDesc_t's and send 'em down to g_pStudioRender->
	int numHardwareLights = 0;
	LightDesc_t lightDesc[MAXLOCALLIGHTS];
	{
		LightDesc_t *pLightDesc;
		for ( int i = 0; i < numLights; i++)
		{
			pLightDesc = &lightDesc[numHardwareLights];
			if (!WorldLightToMaterialLight( locallight[i], *pLightDesc ))
				continue;

			// Apply lightstyle
			float bias = LightStyleValue( locallight[i]->style );

			// Deal with overbrighting + bias
			pLightDesc->m_Color[0] *= bias * overbrightFactor;
			pLightDesc->m_Color[1] *= bias * overbrightFactor;
			pLightDesc->m_Color[2] *= bias * overbrightFactor;

			numHardwareLights++;
			Assert( numHardwareLights <= MAXLOCALLIGHTS );
		}
	}

	g_pStudioRender->SetLocalLights( numHardwareLights, lightDesc );
}


//-----------------------------------------------------------------------------
// Computes the center of the studio model for illumination purposes
//-----------------------------------------------------------------------------
void R_StudioCenter( studiohdr_t* pStudioHdr, const matrix3x4_t &matrix, Vector& center )
{
	VectorTransform( pStudioHdr->illumposition, matrix, center );
}



#if 0
// garymct - leave this in here for now. . we might need this for bumped models
void R_StudioCalculateVirtualLightAndLightCube( Vector& mid, Vector& virtualLightPosition,
											   Vector& virtualLightColor, Vector* lightBoxColor )
{
	int i, j;
	Vector delta;
	float dist2, ratio;
	byte  *pvis;
	float t;
	static ConVar bumpLightBlendRatioMin( "bump_light_blend_ratio_min", "0.00002" );
	static ConVar bumpLightBlendRatioMax( "bump_light_blend_ratio_max", "0.00004" );

	if ( mat_fullbright.GetInt() == 1 )
		return;
	
	VectorClear( virtualLightPosition );
	VectorClear( virtualLightColor );
	for( i = 0; i < 6; i++ )
		{
		VectorClear( lightBoxColor[i] );
	}
	
	pvis = CM_ClusterPVS( CM_LeafCluster( CM_PointLeafnum( mid ) ) );

	float sumBumpBlendParam = 0;
	for (i = 0; i < host_state.worldmodel->brush.numworldlights; i++)
	{
		dworldlight_t *wl = &host_state.worldmodel->brush.worldlights[i];

		if (wl->cluster < 0)
			continue;

		// only do it if the entity can see into the lights leaf
		if (!BIT_SET( pvis, (wl->cluster)))
			continue;
	
		// hack: for this test, only deal with point light sources.
		if( wl->type != emit_point )
			continue;

		// check distance
		VectorSubtract( wl->origin, mid, delta );
		dist2 = DotProduct( delta, delta );
		
		ratio = R_WorldLightDistanceFalloff( wl, delta );
		
		VectorNormalize( delta );
		
		ratio = ratio * R_WorldLightAngle( wl, wl->normal, delta, delta );

		float bumpBlendParam; // 0.0 = all cube, 1.0 = all bump 
		
		// lerp
		bumpBlendParam = 
			( ratio - bumpLightBlendRatioMin.GetFloat() ) / 
			( bumpLightBlendRatioMax.GetFloat() - bumpLightBlendRatioMin.GetFloat() );
	
		if( bumpBlendParam > 0.0 )
		{
			// Get the bit that goes into the bump light
			sumBumpBlendParam += bumpBlendParam;
			VectorMA( virtualLightPosition, bumpBlendParam, wl->origin, virtualLightPosition );
			VectorMA( virtualLightColor, bumpBlendParam, wl->intensity, virtualLightColor );
		}

		if( bumpBlendParam < 1.0f )
		{
			// Get the bit that goes into the cube
			float cubeBlendParam;
			cubeBlendParam = 1.0f - bumpBlendParam;
			if( cubeBlendParam < 0.0f )
			{
				cubeBlendParam = 0.0f;
			}
			for (j = 0; j < numBoxDir; j++)
			{
				t = DotProduct( r_boxdir[j], delta );
				if (t > 0)
				{
					VectorMA( lightBoxColor[j], ratio * t * cubeBlendParam, wl->intensity, lightBoxColor[j] );
				}
			}
		}
	}
	// Get the final virtual light position and color.
	VectorMultiply( virtualLightPosition, 1.0f / sumBumpBlendParam, virtualLightPosition );
	VectorMultiply( virtualLightColor, 1.0f / sumBumpBlendParam, virtualLightColor );
}
#endif


Vector R_TestLightForPoint(const Vector &vPoint, bool bClamp)
{
	Vector ret(0,0,0);
	bool checked_skylight = false;

	for (int i = 0; i < host_state.worldmodel->brush.numworldlights; i++)
	{
		dworldlight_t *wl = &host_state.worldmodel->brush.worldlights[i];

		if (wl->cluster < 0)
			continue;

		// only do it if the entity can see into the lights leaf
//		if (!BIT_SET( pvis, (wl->cluster)))
//			continue;

		if (wl->type == emit_skylight)
		{
			// only check for the sun once
			if (checked_skylight)
				continue;

			checked_skylight = true;

			Vector end;
			// check to see if you can hit the sky texture
			VectorMA( vPoint, -MAX_TRACE_LENGTH, wl->normal, end );

			// BUGBUG: Shouldn't have to test tr.surface, but if there are no physents yet
			// PM_TraceLine fails.
			trace_t tr;
			CTraceFilterWorldAndPropsOnly traceFilter;
			Ray_t ray;
			ray.Init( vPoint, end );
#ifdef SWDS
			g_pEngineTraceServer->TraceRay( ray, MASK_OPAQUE, &traceFilter, &tr );
#else
			g_pEngineTraceClient->TraceRay( ray, MASK_OPAQUE, &traceFilter, &tr );
#endif
			if ( !(tr.surface.flags & SURF_SKY) )
				continue;

			ret = ret + wl->intensity;
		}
		else if (wl->type == emit_skyambient)
		{
			// always ignore these
			continue;
		}
		else
		{
			// check distance
			Vector delta = Vector(wl->origin) - vPoint;

			float ratio = Engine_WorldLightDistanceFalloff( wl, delta );

			ret = ret + Vector(wl->intensity) * ratio;
		}
	}

	if(bClamp)
	{
		if(ret.x > 1.0f) ret.x = 1.0f;
		if(ret.y > 1.0f) ret.y = 1.0f;
		if(ret.z > 1.0f) ret.z = 1.0f;
	}

	return ret;
}

		 
// TODO: move cone calcs to position
// TODO: cone clipping calc's wont work for boxlight since the player asks for a single point.  Not sure what the volume is.
float Engine_WorldLightDistanceFalloff( const dworldlight_t *wl, const Vector& delta, bool bNoRadiusCheck )
{
	float falloff;

	switch (wl->type)
	{
		case emit_surface:
#if 1
			// Cull out stuff that's too far
			if (wl->radius != 0)
			{
				if ( DotProduct( delta, delta ) > (wl->radius * wl->radius))
					return 0.0f;
			}

			return InvRSquared(delta);
#else
			// 1/r*r
			falloff = DotProduct( delta, delta );
			if (falloff < 1)
				return 1.f;
			else
				return 1.f / falloff;
#endif

			break;

		case emit_skylight:
			return 1.f;
			break;

		case emit_quakelight:
			// X - r;
			falloff = wl->linear_attn - FastSqrt( DotProduct( delta, delta ) );
			if (falloff < 0)
				return 0.f;

			return falloff;
			break;

		case emit_skyambient:
			return 1.f;
			break;

		case emit_point:
		case emit_spotlight:	// directional & positional
			{
				float dist2, dist;

				dist2 = DotProduct( delta, delta );
				dist = FastSqrt( dist2 );

				// Cull out stuff that's too far
				if (!bNoRadiusCheck && (wl->radius != 0) && (dist > wl->radius))
					return 0.f;

				return 1.f / (wl->constant_attn + wl->linear_attn * dist + wl->quadratic_attn * dist2);
			}

			break;
		default:
			// Bug: need to return an error
			break;
	}
	return 1.f;
}

/*
  light_normal (lights normal translated to same space as other normals)
  surface_normal
  light_direction_normal | (light_pos - vertex_pos) |
*/
float Engine_WorldLightAngle( const dworldlight_t *wl, const Vector& lnormal, const Vector& snormal, const Vector& delta )
{
	float dot, dot2, ratio = 0;

	switch (wl->type)
	{
		case emit_surface:
			dot = DotProduct( snormal, delta );
			if (dot < 0)
				return 0;

			dot2 = -DotProduct (delta, lnormal);
			if (dot2 <= ON_EPSILON/10)
				return 0; // behind light surface

			return dot * dot2;

		case emit_point:
			dot = DotProduct( snormal, delta );
			if (dot < 0)
				return 0;
			return dot;

		case emit_spotlight:
			dot = DotProduct( snormal, delta );
			if (dot < 0)
				return 0;

			dot2 = -DotProduct (delta, lnormal);
			if (dot2 <= wl->stopdot2)
				return 0; // outside light cone

			ratio = dot;
			if (dot2 >= wl->stopdot)
				return ratio;	// inside inner cone

			if ((wl->exponent == 1) || (wl->exponent == 0))
			{
				ratio *= (dot2 - wl->stopdot2) / (wl->stopdot - wl->stopdot2);
			}
			else
			{
				ratio *= pow((dot2 - wl->stopdot2) / (wl->stopdot - wl->stopdot2), wl->exponent );
			}
			return ratio;

		case emit_skylight:
			dot2 = -DotProduct( snormal, lnormal );
			if (dot2 < 0)
				return 0;
			return dot2;

		case emit_quakelight:
			// linear falloff
			dot = DotProduct( snormal, delta );
			if (dot < 0)
				return 0;
			return dot;

		case emit_skyambient:
			// not supported
			return 1;

		default:
			// Bug: need to return an error
			break;
	} 
	return 0;
}

static CSysModule *g_pStudioRenderModule = NULL;

void InitStudioRender( void )
{
	if ( g_pStudioRenderModule )
		return;

	extern CreateInterfaceFn g_MaterialSystemClientFactory;

	g_pStudioRenderModule = FileSystem_LoadModule( "StudioRender.dll" );
	if( !g_pStudioRenderModule )
	{
		Sys_Error( "Can't load StudioRender.dll\n" );
	}
	CreateInterfaceFn studioRenderFactory = Sys_GetFactory( g_pStudioRenderModule );
	if (!studioRenderFactory )
	{
		Sys_Error( "Can't get factory for StudioRender.dll\n" );
	}
	g_pStudioRender = ( IStudioRender * )studioRenderFactory( STUDIO_RENDER_INTERFACE_VERSION, NULL );
	if (!g_pStudioRender)
	{
		Sys_Error( "Unable to init studio render system version %s\n", STUDIO_RENDER_INTERFACE_VERSION );
	}

	IClientStats* pStudioStats = ( IClientStats * )studioRenderFactory( INTERFACEVERSION_CLIENTSTATS, NULL );
	if (!pStudioStats)
	{
		Sys_Error( "Unable to init studio render stats version %s\n", INTERFACEVERSION_CLIENTSTATS );
	}
	g_EngineStats.InstallClientStats( pStudioStats );

	g_pStudioRender->Init( g_AppSystemFactory, g_MaterialSystemClientFactory );

	UpdateStudioRenderConfig();
}

void ShutdownStudioRender( void )
{
	if ( !g_pStudioRenderModule )
		return;

	if ( g_pStudioRender )
	{
		g_pStudioRender->Shutdown();
	}
	g_pStudioRender = NULL;

	FileSystem_UnloadModule( g_pStudioRenderModule );
	g_pStudioRenderModule = NULL;
}

ConVar r_fullcull( "r_fullcull", "0" );
bool R_CullBox( const Vector& mins, const Vector& maxs, const Frustum_t &frustum )
{
	if ( r_fullcull.GetInt() )
	{
		return (( BoxOnPlaneSide( mins, maxs, frustum.GetPlane(0) ) == 2 ) || 
				( BoxOnPlaneSide( mins, maxs, frustum.GetPlane(1) ) == 2 ) ||
				( BoxOnPlaneSide( mins, maxs, frustum.GetPlane(2) ) == 2 ) ||
				( BoxOnPlaneSide( mins, maxs, frustum.GetPlane(FRUSTUM_FARZ) ) == 2 ) ||
				( BoxOnPlaneSide( mins, maxs, frustum.GetPlane(FRUSTUM_NEARZ) ) == 2 ) ||
				( BoxOnPlaneSide( mins, maxs, frustum.GetPlane(3) ) == 2 ) );
	}
	else
	{
		return (( BoxOnPlaneSide( mins, maxs, frustum.GetPlane(0) ) == 2 ) || 
				( BoxOnPlaneSide( mins, maxs, frustum.GetPlane(1) ) == 2 ) ||
				( BoxOnPlaneSide( mins, maxs, frustum.GetPlane(2) ) == 2 ) ||
				( BoxOnPlaneSide( mins, maxs, frustum.GetPlane(3) ) == 2 ) );
	}
}

//-----------------------------------------------------------------------------
// Computes the bounding box for a studiohdr + a sequence
//-----------------------------------------------------------------------------
void R_ComputeBBox( DrawModelState_t& state, const Vector& origin, 
							int sequence, Vector& mins, Vector& maxs )
{
	// Fake bboxes for models.
	static Vector gFakeHullMins( -16, -16, -16 );
	static Vector gFakeHullMaxs( 16, 16, 16 );

	if (!VectorCompare( vec3_origin, state.m_pStudioHdr->view_bbmin ))
	{
		// clipping bounding box
		VectorAdd ( origin, state.m_pStudioHdr->view_bbmin, mins);
		VectorAdd ( origin, state.m_pStudioHdr->view_bbmin, maxs);
	}
	else if (!VectorCompare( vec3_origin, state.m_pStudioHdr->hull_min ))
	{
		// movement bounding box
		VectorAdd ( origin, state.m_pStudioHdr->hull_min, mins);
		VectorAdd ( origin, state.m_pStudioHdr->hull_max, maxs);
	}
	else
	{
		// fake bounding box
		VectorAdd ( origin, gFakeHullMins, mins);
		VectorAdd ( origin, gFakeHullMaxs, maxs);
	}

// construct the base bounding box for this frame
	if ( sequence >= state.m_pStudioHdr->numseq) 
	{
		sequence = 0;
	}

	mstudioseqdesc_t* pseqdesc;
	pseqdesc = state.m_pStudioHdr->pSeqdesc( sequence );

// UNDONE: Test this, it should be faster and still 100% accurate
#if 0
	Vector localCenter = (pseqdesc->bbmin + pseqdesc->bbmax) * 0.5;
	Vector localExtents = pseqdesc->bbmax - localCenter;
	Vector worldCenter;
	VectorTransform( localCenter, *state.m_pModelToWorld, worldCenter );
	Vector worldExtents;
	worldExtents.x = DotProductAbs( localExtents, (*state.m_pModelToWorld)[0] );
	worldExtents.y = DotProductAbs( localExtents, (*state.m_pModelToWorld)[1] );
	worldExtents.z = DotProductAbs( localExtents, (*state.m_pModelToWorld)[2] );

	VectorMin( mins, origin - worldExtents, mins );
	VectorMax( maxs, origin + worldExtents, maxs );
#else
  	for (int i=0; i<8 ; i++)
  	{
  		Vector p1, p2;
   
  		p1[0] = (i & 1) ? pseqdesc->bbmin[0] : pseqdesc->bbmax[0];
  		p1[1] = (i & 2) ? pseqdesc->bbmin[1] : pseqdesc->bbmax[1];
  		p1[2] = (i & 4) ? pseqdesc->bbmin[2] : pseqdesc->bbmax[2];
  
  		VectorTransform( p1, *state.m_pModelToWorld, p2 );
  
  		if (p2[0] < mins[0]) mins[0] = p2[0];
  		if (p2[0] > maxs[0]) maxs[0] = p2[0];
  		if (p2[1] < mins[1]) mins[1] = p2[1];
  		if (p2[1] > maxs[1]) maxs[1] = p2[1];
  		if (p2[2] < mins[2]) mins[2] = p2[2];
  		if (p2[2] > maxs[2]) maxs[2] = p2[2];
  	}
#endif
}

//-----------------------------------------------------------------------------
// Checks to see if the bounding box of the model is in the view frustum
//-----------------------------------------------------------------------------
static bool R_StudioCheckBBox( DrawModelState_t& state, const Vector& origin, 
									int sequence, int flags )
{
	Vector vecMins, vecMaxs;
	if( state.m_BBoxMins && state.m_BBoxMaxs )
	{
#ifdef _DEBUG
		Vector	mins, maxs;
		R_ComputeBBox( state, origin, sequence, mins, maxs );

		static int errorCount = 0;
		if ( errorCount < 4 )
		{
			Vector delta1 = *state.m_BBoxMaxs - maxs;
			Vector delta2 = *state.m_BBoxMins - mins;

			bool invalid = false;

			if ( fabs( delta1[0] ) >= .1f )
			{
				invalid = true;
			}
			if ( fabs( delta1[1] ) >= .1f )
			{
				invalid = true;
			}
			if ( fabs( delta1[1] ) >= .1f )
			{
				invalid = true;
			}
			if ( fabs( delta2[0] ) >= .1f )
			{
				invalid = true;
			}
			if ( fabs( delta2[1] ) >= .1f )
			{
				invalid = true;
			}
			if ( fabs( delta2[2] ) >= .1f )
			{
				invalid = true;
			}

			if ( invalid )
			{
				Assert( !"R_StudioCheckBBox:  Boxes differ!!!" );
				++errorCount;
			}
		}
#endif
		vecMins = *state.m_BBoxMins;
		vecMaxs = *state.m_BBoxMaxs;
	}
	else
	{
		R_ComputeBBox( state, origin, sequence, vecMins, vecMaxs );
	}

	if ( flags & STUDIO_FRUSTUMCULL )
	{
		if ( R_CullBox( vecMins, vecMaxs ) )
			return false;
	}

	if ( flags & STUDIO_OCCLUSIONCULL )
	{
		if ( OcclusionSystem()->IsOccluded( vecMins, vecMaxs ) )
			return false;
	}

	return true;
}



//-----------------------------------------------------------------------------
//
// Implementation of IVModelRender
//
//-----------------------------------------------------------------------------

// UNDONE: Move this to hud export code, subsume previous functions
class CModelRender : public IVModelRender
{
public:
	CModelRender() : m_LightingOriginOverride(false) {}

	// members of the IVModelRender interface
	virtual void ForcedMaterialOverride( IMaterial *newMaterial );
	virtual int DrawModel( 	
					int flags, IClientRenderable *cliententity,
					ModelInstanceHandle_t instance, int entity_index, const model_t *model, 
					const Vector& origin, QAngle const& angles, int sequence,
					int skin, int body, int hitboxset, const Vector *bboxMins, 
					const Vector *bboxMaxs,
					const matrix3x4_t* pModelToWorld );

	virtual void  SetViewTarget( const Vector& target );
	virtual void  SetFlexWeights( const float *weights, int count );

	virtual matrix3x4_t* pBoneToWorld( int i );
	virtual matrix3x4_t* pBoneToWorldArray();

	// Creates, destroys instance data to be associated with the model
	virtual ModelInstanceHandle_t CreateInstance( IClientRenderable *pRenderable, bool bStaticProp );
	virtual void DestroyInstance( ModelInstanceHandle_t handle );

	// Creates a decal on a model instance by doing a planar projection
	// along the ray. The material is the decal material, the radius is the
	// radius of the decal to create.
	virtual void AddDecal( ModelInstanceHandle_t handle, Ray_t const& ray, 
		const Vector& decalUp, int decalIndex, int body, bool noPokethru = false, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );

	// Removes all the decals on a model instance
	virtual void RemoveAllDecals( ModelInstanceHandle_t handle );

	// Shadow rendering (render-to-texture)
	virtual void DrawModelShadow( IClientRenderable *pRenderable, int body );

	// Lighting origin override... (NULL means use normal model origin)
	virtual void UseLightcache( LightCacheHandle_t* pCache = NULL );

	// Used to allow the shadow mgr to manage a list of shadows per model
	unsigned short& FirstShadowOnModelInstance( ModelInstanceHandle_t handle ) { return m_ModelInstances[handle].m_FirstShadow; }

	// This gets called when overbright, etc gets changed to recompute static prop lighting.
	virtual void RecomputeStaticLighting( IClientRenderable *pRenderable, ModelInstanceHandle_t handle );

private:
	enum
	{
		CURRENT_LIGHTING_UNINITIALIZED = -999999
	};

	struct ModelInstance_t
	{
		IClientRenderable* m_pRenderable;

		// Need to store off the model. When it changes, we lose all instance data..
		model_t* m_pModel;
		StudioDecalHandle_t	m_DecalHandle;

		// Stores off the current lighting state
		LightingState_t m_CurrentLightingState;
		Vector m_flLightIntensity[MAXLOCALLIGHTS];
		float m_flLightingTime;

		// First shadow projected onto the model
		unsigned short	m_FirstShadow;

		IMesh **m_ppColorMeshes;
	};

	// Sets up the render state for a model
	void SetupModelState( IClientRenderable *pRenderable );
	
	void RenderModel( DrawModelState_t& state, model_t const *pModel,
		StudioDecalHandle_t decals, const Vector& origin, int skin, int body, int hitboxset,
		int drawFlags, LightCacheHandle_t* pLightCache, ModelInstanceHandle_t instance, int lod );

	void CreateStaticPropColorData( ModelInstanceHandle_t handle );
	void DestroyStaticPropColorData( ModelInstanceHandle_t handle );
	void UpdateStaticPropColorData( IHandleEntity *pEnt, ModelInstanceHandle_t handle );

	// Returns true if the model instance is valid
	bool IsModelInstanceValid( ModelInstanceHandle_t handle );

	bool HasStaticPropColorData( int modelInstanceID );
	
	LightingState_t *TimeAverageLightingState( ModelInstanceHandle_t handle, LightingState_t *pLightingState );

	// Cause the current lighting state to match the given one
	void SnapCurrentLightingState( ModelInstance_t &inst, LightingState_t *pLightingState );

	// Sets up lighting state for rendering
	void StudioSetupLighting( const Vector& absEntCenter, LightCacheHandle_t* pLightcache,
		ModelInstanceHandle_t handle, bool bVertexLit, bool bNeedsEnvCubemap, bool bStaticLighting );

	// Time average the ambient term
	void TimeAverageAmbientLight( LightingState_t &actualLightingState, ModelInstance_t &inst, 
		float flAttenFactor, LightingState_t *pLightingState );

	// Old-style computation of vertex lighting
	void ComputeModelVertexLightingOld( mstudiomodel_t *pModel, 
		matrix3x4_t& matrix, const LightingState_t &lightingState, color24 *pLighting );

	// Old-style computation of vertex lighting
	void ComputeModelVertexLighting( IHandleEntity *pProp, 
		mstudiomodel_t *pModel, OptimizedModel::ModelLODHeader_t *pVtxLOD,
		matrix3x4_t& matrix, Vector4D *pTempMem, color24 *pLighting );

	// Model instance data
	CUtlLinkedList< ModelInstance_t, ModelInstanceHandle_t > m_ModelInstances; 

	bool	m_LightingOriginOverride;
	LightCacheHandle_t	m_LightCacheHandle;
};

static CModelRender s_ModelRender;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CModelRender, IVModelRender, VENGINE_HUDMODEL_INTERFACE_VERSION, s_ModelRender );
IVModelRender* modelrender = &s_ModelRender;


//-----------------------------------------------------------------------------
// Hook needed for shadows to work
//-----------------------------------------------------------------------------
unsigned short& FirstShadowOnModelInstance( ModelInstanceHandle_t handle )
{
	return s_ModelRender.FirstShadowOnModelInstance( handle );
}


//-----------------------------------------------------------------------------
// Cause the current lighting state to match the given one
//-----------------------------------------------------------------------------
void CModelRender::SnapCurrentLightingState( ModelInstance_t &inst, LightingState_t *pLightingState )
{
	inst.m_CurrentLightingState = *pLightingState;
	for ( int i = 0; i < MAXLOCALLIGHTS; ++i )
	{
		if ( i < pLightingState->numlights )
		{
			inst.m_flLightIntensity[i] = pLightingState->locallight[i]->intensity; 
		}
		else
		{
			inst.m_flLightIntensity[i].Init( 0.0f, 0.0f, 0.0f );
		}
	}

#ifndef SWDS
	inst.m_flLightingTime = cl.gettime();
#endif
}


//-----------------------------------------------------------------------------
// Time average the ambient term
//-----------------------------------------------------------------------------
void CModelRender::TimeAverageAmbientLight( LightingState_t &actualLightingState, ModelInstance_t &inst, float flAttenFactor, LightingState_t *pLightingState )
{
	Vector vecDelta;
	for ( int i = 0; i < 6; ++i )
	{
		VectorSubtract( pLightingState->r_boxcolor[i], inst.m_CurrentLightingState.r_boxcolor[i], vecDelta );
		vecDelta *= flAttenFactor;
		inst.m_CurrentLightingState.r_boxcolor[i] = pLightingState->r_boxcolor[i] - vecDelta;

		VectorSubtract( pLightingState->totalboxcolor[i], inst.m_CurrentLightingState.totalboxcolor[i], vecDelta );
		vecDelta *= flAttenFactor;
		inst.m_CurrentLightingState.totalboxcolor[i] = pLightingState->totalboxcolor[i] - vecDelta;
	}

	memcpy( &actualLightingState.r_boxcolor, &inst.m_CurrentLightingState.r_boxcolor, sizeof(inst.m_CurrentLightingState.r_boxcolor) );
	memcpy( &actualLightingState.totalboxcolor, &inst.m_CurrentLightingState.totalboxcolor, sizeof(inst.m_CurrentLightingState.totalboxcolor) );
}


//-----------------------------------------------------------------------------
// Do time averaging of the lighting state to avoid popping...
//-----------------------------------------------------------------------------
LightingState_t *CModelRender::TimeAverageLightingState( ModelInstanceHandle_t handle, LightingState_t *pLightingState )
{
	// Currently disabled
	return pLightingState;
#ifndef SWDS

	float flInterpFactor = r_lightinterp.GetFloat();
	if ( flInterpFactor == 0 )
		return pLightingState;

	if ( handle == MODEL_INSTANCE_INVALID)
		return pLightingState;

	ModelInstance_t &inst = m_ModelInstances[handle];
	if ( inst.m_flLightingTime == CURRENT_LIGHTING_UNINITIALIZED )
	{
		SnapCurrentLightingState( inst, pLightingState );
		return pLightingState;
	}

	float dt = (cl.gettime() - inst.m_flLightingTime);
	if ( dt == 0.0f )
		return pLightingState;

	inst.m_flLightingTime = cl.gettime();

	static LightingState_t actualLightingState;
	static dworldlight_t s_WorldLights[MAXLOCALLIGHTS];
	
	// I'm creating the equation v = vf - (vf-vi)e^-at 
	// where vf = this frame's lighting value, vi = current time averaged lighting value
	int i;
	Vector vecDelta;
	float flAttenFactor = exp( -flInterpFactor * dt );
	TimeAverageAmbientLight( actualLightingState, inst, flAttenFactor, pLightingState );

	// Max # of lights...
	int nWorldLights = r_worldlights.GetInt();
	
	// Create a mapping of identical lights
	int nMatchCount = 0;
	bool pMatch[MAXLOCALLIGHTS];
	Vector pLight[MAXLOCALLIGHTS];
	dworldlight_t *pSourceLight[MAXLOCALLIGHTS];

	memset( pMatch, 0, sizeof(pMatch) );
	for ( i = 0; i < pLightingState->numlights; ++i )
	{
		pLight[i].Init( 0.0f, 0.0f, 0.0f );
		int j;
		for ( j = 0; j < inst.m_CurrentLightingState.numlights; ++j )
		{
			if ( pLightingState->locallight[i] == inst.m_CurrentLightingState.locallight[j] )
			{
				++nMatchCount;
				pMatch[j] = true;
				pLight[i] = inst.m_flLightIntensity[j];
				break;
			}
		}
	}
	
	// Find out if we can simply ramp...
	int nTotalLights = pLightingState->numlights + inst.m_CurrentLightingState.numlights - nMatchCount;
	bool bCanRamp = nTotalLights <= nWorldLights;
	
	actualLightingState.numlights = bCanRamp ? nTotalLights : pLightingState->numlights;
	for ( i = 0; i < pLightingState->numlights; ++i )
	{
		actualLightingState.locallight[i] = &s_WorldLights[i];
		memcpy( &s_WorldLights[i], pLightingState->locallight[i], sizeof(dworldlight_t) );

		// Light already exists? Attenuate to it...
		VectorSubtract( pLightingState->locallight[i]->intensity, pLight[i], vecDelta );
		vecDelta *= flAttenFactor;

		s_WorldLights[i].intensity = pLightingState->locallight[i]->intensity - vecDelta;
		pSourceLight[i] = pLightingState->locallight[i];
	}

	int nCurrLight = pLightingState->numlights;
	if (bCanRamp)
	{
		for ( i = 0; i < inst.m_CurrentLightingState.numlights; ++i )
		{
			if (pMatch[i])
				continue;

			actualLightingState.locallight[nCurrLight] = &s_WorldLights[nCurrLight];
			memcpy( &s_WorldLights[nCurrLight], inst.m_CurrentLightingState.locallight[i], sizeof(dworldlight_t) );

			// Attenuate to black (fade out)
			VectorMultiply( inst.m_CurrentLightingState.locallight[i]->intensity, flAttenFactor, vecDelta );

			s_WorldLights[nCurrLight].intensity = vecDelta;
			pSourceLight[nCurrLight] = inst.m_CurrentLightingState.locallight[i];
			++nCurrLight;
		}
	}

	inst.m_CurrentLightingState.numlights = nCurrLight;
	for ( i = 0; i < nCurrLight; ++i )
	{
		inst.m_CurrentLightingState.locallight[i] = pSourceLight[i];
		inst.m_flLightIntensity[i] = s_WorldLights[i].intensity;
	}

//	else
//	{
//		SnapCurrentLightingState( inst, pLightingState );
//		return pLightingState;
//	}

	return &actualLightingState;
#endif

}


//-----------------------------------------------------------------------------
// Sets up lighting state for rendering
//-----------------------------------------------------------------------------
void CModelRender::StudioSetupLighting( const Vector& absEntCenter, LightCacheHandle_t* pLightcache,
	ModelInstanceHandle_t handle, bool bVertexLit, bool bNeedsEnvCubemap, bool bStaticLighting )
{
#ifndef SWDS
	ITexture *pEnvCubemapTexture;
	LightingState_t lightingState;
	if (pLightcache)
	{
		if( bStaticLighting )
		{
			// We already have the static part baked into a color mesh, so just get the dynamic stuff.
			pEnvCubemapTexture = LightcacheGet( *pLightcache, lightingState, LIGHTCACHEFLAGS_DYNAMIC | LIGHTCACHEFLAGS_LIGHTSTYLE );
		}
		else
		{
			pEnvCubemapTexture = LightcacheGet( *pLightcache, lightingState );
		}
	}
	else
	{
		pEnvCubemapTexture = LightcacheGet( absEntCenter, lightingState );
	}
	
	// Do time averaging of the lighting state to avoid popping...
	LightingState_t *pState = TimeAverageLightingState( handle, &lightingState );

	if( bNeedsEnvCubemap && pEnvCubemapTexture )
	{
		materialSystemInterface->BindLocalCubemap( pEnvCubemapTexture );
	}
	if (mat_fullbright.GetInt() == 1)
	{
		materialSystemInterface->SetAmbientLight( 1.0, 1.0, 1.0 );

		static Vector white[6] = 
		{
			Vector( 1.0, 1.0, 1.0 ),
			Vector( 1.0, 1.0, 1.0 ),
			Vector( 1.0, 1.0, 1.0 ),
			Vector( 1.0, 1.0, 1.0 ),
			Vector( 1.0, 1.0, 1.0 ),
			Vector( 1.0, 1.0, 1.0 ),
		};

		g_pStudioRender->SetAmbientLightColors( white, white );

		// Disable all the lights..
		for( int i = 0; i < g_pMaterialSystemHardwareConfig->MaxNumLights(); ++i)
		{
			LightDesc_t desc;
			desc.m_Type = MATERIAL_LIGHT_DISABLE;
			materialSystemInterface->SetLight( i, desc );
		}
	}
	else if( bVertexLit )
	{
		g_pStudioRender->SetAmbientLightColors( pState->r_boxcolor, pState->totalboxcolor );
		
		float overbrightFactor = 1.0;
		
		// Don't divide by the overbright factor when vertex shaders are active
		// FIXME: I'd like the engine to not have to worry about this logic
		if (g_pMaterialSystemHardwareConfig->SupportsOverbright() &&
			(mat_softwarelighting.GetInt() || 
			!g_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders()) )
		{
			overbrightFactor /= mat_overbright.GetFloat();
		}
		
		materialSystemInterface->SetAmbientLight( 0.0, 0.0, 0.0 );
		R_SetNonAmbientLightingState( overbrightFactor, pState->numlights, 
			pState->locallight );
	}
#endif
}


//-----------------------------------------------------------------------------
// Uses this material instead of the one the model was compiled with
//-----------------------------------------------------------------------------
void CModelRender::ForcedMaterialOverride( IMaterial *newMaterial )
{
	g_pStudioRender->ForcedMaterialOverride( newMaterial );
}


//-----------------------------------------------------------------------------
// Sets up the render state for a model
//-----------------------------------------------------------------------------
void CModelRender::SetupModelState( IClientRenderable *pRenderable )
{
	const model_t *pModel = pRenderable->GetModel();
	if (!pModel)
		return;

	studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( const_cast<model_t*>(pModel) );
	if (pStudioHdr->numbodyparts == 0)
		return;

#ifndef SWDS
	// Set up skinning state
	Assert ( pRenderable );
	{
		MEASURE_TIMED_STAT( ENGINE_STATS_SETUP_BONES_TIME );
		pRenderable->SetupBones( pBoneToWorldArray(), MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, cl.gettime() ); // hack hack
	}
#endif
	// Sets up flexes
	pRenderable->SetupWeights( );
}


//-----------------------------------------------------------------------------
// Actually renders the model
//-----------------------------------------------------------------------------
void CModelRender::RenderModel( DrawModelState_t& state, model_t const *pModel,
	StudioDecalHandle_t decals, const Vector& origin, int skin, int body, int hitboxset,
	int drawFlags, LightCacheHandle_t* pLightCache, ModelInstanceHandle_t instance, int lod )
{

#ifndef SWDS
	// Does setup for flexes
	state.m_pRenderable->SetupWeights( );

	bool staticLighting = ( drawFlags & STUDIORENDER_DRAW_STATIC_LIGHTING ) &&
						( state.m_pStudioHdr->flags & STUDIOHDR_FLAGS_STATIC_PROP ) && 
						( !( state.m_pStudioHdr->flags & STUDIOHDR_FLAGS_USES_BUMPMAPPING ) ) && 
						r_staticlighting.GetBool() &&
						( instance != MODEL_INSTANCE_INVALID ) &&
						g_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders();
	
	bool bVertexLit = ( pModel->flags & MODELFLAG_VERTEXLIT ) != 0;
	bool bNeedsEnvCubemap = r_showenvcubemap.GetInt() || 
		( ( state.m_pStudioHdr->flags & STUDIOHDR_FLAGS_USES_ENV_CUBEMAP ) != 0 );
	
	// get lighting from ambient light sources and radiosity bounces
	// also set up the env_cubemap from the light cache if necessary.
	if( bVertexLit || bNeedsEnvCubemap )
	{
		MEASURE_TIMED_STAT( ENGINE_STATS_UPDATE_MRM_LIGHTING_TIME );

		// Choose the lighting origin
		Vector entOrigin;
		if (!pLightCache)
		{
			R_StudioCenter( state.m_pStudioHdr, *state.m_pModelToWorld, entOrigin );
		}

		// Set up lighting based on the lighting origin
		StudioSetupLighting( entOrigin, pLightCache, instance, bVertexLit, bNeedsEnvCubemap, staticLighting );
	}

	// Set up the camera state
	g_pStudioRender->SetViewState( r_origin, vright, vup, vpn );

	// Color + alpha modulation
	g_pStudioRender->SetColorModulation( r_colormod );
	g_pStudioRender->SetAlphaModulation( r_blend );

	DrawModelInfo_t info;
	info.m_pStudioHdr = state.m_pStudioHdr;
	info.m_pHardwareData = (studiohwdata_t *)&pModel->studio.hardwareData;
	info.m_Decals = decals;
	info.m_Skin = skin;
	info.m_Body = body;
	info.m_HitboxSet = hitboxset;
	info.m_pClientEntity = (void*)state.m_pRenderable;
	info.m_Lod = lod;
	if( staticLighting ) 
	{
		info.m_ppColorMeshes = m_ModelInstances[instance].m_ppColorMeshes;
	}
	else
	{
		info.m_ppColorMeshes = NULL;
	}

	g_pStudioRender->DrawModel( info, origin, &info.m_Lod, NULL, drawFlags );
	g_EngineStats.IncrementCountedStat( 
		ENGINE_STATS_ACTUAL_MODEL_TRIANGLES, info.m_ActualTriCount );
//	g_pStudioRender->GetPerfStats( info, actualTrisRendered, 
//		effectiveTrisRendered, textureMemoryBytes );
//	g_EngineStats.IncrementCountedStat( ENGINE_STATS_NUM_MRM_TRIS_RENDERED, numTrianglesRendered );

	if( r_drawmodelstatsoverlay.GetBool() )
	{
		float alpha = 1;
		if( r_drawmodelstatsoverlaydistance.GetFloat() > 0 )
		{
			alpha = 1.f - clamp( r_origin.DistTo(origin) / r_drawmodelstatsoverlaydistance.GetFloat(), 0, 1.f );
		}

		Assert( info.m_pStudioHdr );
		Assert( info.m_pStudioHdr->name );
		Assert( info.m_pHardwareData );
		float duration = 0.0f;
		int lineOffset = 0;
		if( !info.m_pStudioHdr || !info.m_pStudioHdr->name || !info.m_pHardwareData )
		{
			CDebugOverlay::AddTextOverlay( origin, lineOffset++, duration, "This model has problems. . see a programmer." );
			return;
		}

		char buf[1024];
		CDebugOverlay::AddTextOverlay( origin, lineOffset++, duration, alpha, info.m_pStudioHdr->name );
		Q_snprintf( buf, 1023, "lod: %d/%d\n", info.m_Lod+1, ( int )info.m_pHardwareData->m_NumLODs );
		CDebugOverlay::AddTextOverlay( origin, lineOffset++, duration, alpha, buf );
		Q_snprintf( buf, 1023, "tris: %d\n",  info.m_ActualTriCount );
		CDebugOverlay::AddTextOverlay( origin, lineOffset++, duration, alpha, buf );
		Q_snprintf( buf, 1023, "bones: %d\n",  info.m_pStudioHdr->numbones );
		CDebugOverlay::AddTextOverlay( origin, lineOffset++, duration, alpha, buf );		
		Q_snprintf( buf, 1023, "textures: %d (%d bytes)\n", info.m_pStudioHdr->numtextures, info.m_TextureMemoryBytes );
		CDebugOverlay::AddTextOverlay( origin, lineOffset++, duration, alpha, buf );		
		Q_snprintf( buf, 1023, "Render Time: %0.1f ms\n", info.m_RenderTime.GetDuration().GetMillisecondsF());
		CDebugOverlay::AddTextOverlay( origin, lineOffset++, duration, alpha, buf );

		//Q_snprintf( buf, 1023, "Render Time: %0.1f ms\n", info.m_pClientEntity 
		

	}
#endif
}

//-----------------------------------------------------------------------------
// Main entry point for model rendering in the engine
//-----------------------------------------------------------------------------
int CModelRender::DrawModel( 	
	int flags,
	IClientRenderable *pRenderable,
	ModelInstanceHandle_t instance,
	int entity_index, 
	const model_t *pModel, 
	const Vector &origin,
	const QAngle &angles,
	int sequence,
	int skin,
	int body,
	int hitboxset,
	const Vector *bboxMins,
	const Vector *bboxMaxs,
	const matrix3x4_t* pModelToWorld )
{
#ifndef SWDS

	if ( (char*)pRenderable < (char*)1024 )
	{
		Error( "CModelRender::DrawModel: pRenderable == 0x%x", pRenderable );
	}	
	
	DrawModelState_t state;
	state.m_pStudioHdr = modelinfo->GetStudiomodel( pModel );
	state.m_pRenderable = pRenderable;
	state.m_BBoxMins = bboxMins;
	state.m_BBoxMaxs = bboxMaxs;

	// quick exit
	if (state.m_pStudioHdr->numbodyparts == 0)
		return 1;

	matrix3x4_t tmpmat;

	if( !pModelToWorld )
	{
		state.m_pModelToWorld = &tmpmat;

		// Turns the origin + angles into a matrix
		AngleMatrix( angles, origin, tmpmat );
	}
	else
	{
		state.m_pModelToWorld = pModelToWorld;
	}

	// see if the bounding box lets us trivially reject
	if (flags & (STUDIO_FRUSTUMCULL | STUDIO_OCCLUSIONCULL))
	{
		if (!R_StudioCheckBBox ( state, origin, sequence, flags ))
			return 0;
	}

	Assert ( pRenderable );
	if( r_speeds.GetBool() )
	{
		if( state.m_pStudioHdr->numbones == 1 )
		{
			materialSystemStatsInterface->SetStatGroup( MATERIAL_SYSTEM_STATS_STATIC_PROPS );
		}
		else
		{
			materialSystemStatsInterface->SetStatGroup( MATERIAL_SYSTEM_STATS_OTHER );
		}
	}

	int lod = r_lod.GetInt();
	if( lod == -1 )
	{
		float screenSize = g_EngineRenderer->GetScreenSize( pRenderable->GetRenderOrigin(), 0.5f );
		lod = g_pStudioRender->ComputeModelLod( (studiohwdata_t *)&pModel->studio.hardwareData, screenSize );
	}
	else
	{
		if( lod > pModel->studio.hardwareData.m_NumLODs - 1 )
		{
			lod = pModel->studio.hardwareData.m_NumLODs - 1;
		}
		else if( lod < 0 )
		{
			lod = 0;
		}
	}

	int boneMask = BONE_USED_BY_ANYTHING_AT_LOD( lod );
	pRenderable->SetupBones( pBoneToWorldArray(), MAXSTUDIOBONES, boneMask, cl.gettime() );
	// Why isn't this always set?!?
	if (flags & STUDIO_RENDER)
	{
		// Convert the instance to a decal handle.
		StudioDecalHandle_t decalHandle = STUDIORENDER_DECAL_INVALID;
		if (instance != MODEL_INSTANCE_INVALID)
			decalHandle = m_ModelInstances[instance].m_DecalHandle;

		int drawFlags = STUDIORENDER_DRAW_ENTIRE_MODEL;
		if (flags & STUDIO_TWOPASS)
		{
			if (flags & STUDIO_TRANSPARENCY)
			{
				drawFlags = STUDIORENDER_DRAW_TRANSLUCENT_ONLY; 
			}
			else
			{
				drawFlags = STUDIORENDER_DRAW_OPAQUE_ONLY; 
			}
		}
		if( flags & STUDIO_STATIC_LIGHTING )
		{
			drawFlags |= STUDIORENDER_DRAW_STATIC_LIGHTING;
		}

		
		if( r_drawmodelstatsoverlay.GetInt() == 2)
		{
			drawFlags |= STUDIORENDER_DRAW_ACCURATETIME;
		}

		// Shadow state...
		g_pShadowMgr->SetModelShadowState( instance );

		RenderModel( state, pModel, decalHandle, origin, skin, body, hitboxset, drawFlags,
			m_LightingOriginOverride ? &m_LightCacheHandle : NULL, instance, lod );
	}

	if( r_speeds.GetBool() )
	{
		materialSystemStatsInterface->SetStatGroup( MATERIAL_SYSTEM_STATS_OTHER );
	}
	return 1;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Shadow rendering
//-----------------------------------------------------------------------------
void CModelRender::DrawModelShadow( IClientRenderable *pRenderable, int body )
{
#ifndef SWDS
	static ConVar r_shadowlod("r_shadowlod", "-1");
	static ConVar r_shadowlodbias("r_shadowlodbias", "2");

	MEASURE_TIMED_STAT( ENGINE_STATS_SHADOW_TRIANGLE_TIME ); 

	model_t const* pModel = pRenderable->GetModel();
	if ( !pModel )
		return;

	// FIXME: Make brush shadows work
	if (pModel->type == mod_brush)
		return;

	DrawModelInfo_t info;
	info.m_pStudioHdr = modelinfo->GetStudiomodel( pModel );
	info.m_ppColorMeshes = NULL;

	// quick exit
	if (info.m_pStudioHdr->numbodyparts == 0)
		return;

	Assert ( pRenderable );
	info.m_pHardwareData = (studiohwdata_t *)&pModel->studio.hardwareData;
	info.m_Decals = STUDIORENDER_DECAL_INVALID;
	info.m_Skin = 0;
	info.m_Body = body;
	info.m_pClientEntity = (void*)pRenderable;
	info.m_HitboxSet = 0;

	info.m_Lod = r_shadowlod.GetInt();
	// If the .mdl has a shadowlod, force the use of that one instead
	if ( info.m_pStudioHdr->flags & STUDIOHDR_FLAGS_HASSHADOWLOD )
	{
		info.m_Lod = info.m_pHardwareData->m_NumLODs-1;
	}
	if ( info.m_Lod != USESHADOWLOD && info.m_Lod < 0 )
	{
		// Compute the shadow LOD...
		float factor = r_shadowlodbias.GetFloat() > 0.0f ? 1.0f / r_shadowlodbias.GetFloat() : 1.0f;
		float screenSize = factor * g_EngineRenderer->GetScreenSize( pRenderable->GetRenderOrigin(), 0.5f );
		info.m_Lod = g_pStudioRender->ComputeModelLod( info.m_pHardwareData, screenSize ); 
		info.m_Lod = info.m_pHardwareData->m_NumLODs-2;
		if( info.m_Lod < 0 )
		{
			info.m_Lod = 0;
		}
	}
	pRenderable->SetupBones( pBoneToWorldArray(), MAXSTUDIOBONES, BONE_USED_BY_ANYTHING_AT_LOD( info.m_Lod ), cl.gettime() );

	// Does setup for flexes
	pRenderable->SetupWeights( );

	// Color + alpha modulation
	Vector white( 1, 1, 1 );
	g_pStudioRender->SetColorModulation( white.Base() );
	g_pStudioRender->SetAlphaModulation( 1.0f );

	if ((info.m_pStudioHdr->flags & STUDIOHDR_FLAGS_USE_SHADOWLOD_MATERIALS) == 0)
	{
		g_pStudioRender->ForcedMaterialOverride( g_pMaterialShadowBuild, OVERRIDE_BUILD_SHADOWS );
	}

	g_pStudioRender->DrawModel( info, pRenderable->GetRenderOrigin(),
		NULL, NULL, STUDIORENDER_DRAW_ENTIRE_MODEL | STUDIORENDER_DRAW_NO_FLEXES );
	g_EngineStats.IncrementCountedStat( 
		ENGINE_STATS_ACTUAL_SHADOW_RENDER_TO_TEXTURE_TRIANGLES, info.m_ActualTriCount );

	g_pStudioRender->ForcedMaterialOverride( 0 );
#endif
}


//-----------------------------------------------------------------------------
// Lighting origin override... (NULL means use normal model origin)
//-----------------------------------------------------------------------------
void CModelRender::UseLightcache( LightCacheHandle_t *pHandle )
{
	if (pHandle)
	{
		m_LightingOriginOverride = true;
		m_LightCacheHandle = *pHandle;
	}
	else
	{
		m_LightingOriginOverride = false;
	}
}

void  CModelRender::SetViewTarget( const Vector& target )
{
	g_pStudioRender->SetEyeViewTarget( target );
}

void  CModelRender::SetFlexWeights( const float *weights, int count )
{
	g_pStudioRender->SetFlexWeights( count, weights );
}

matrix3x4_t* CModelRender::pBoneToWorld(int i)
{
	return g_pStudioRender->GetBoneToWorld( i );
}

matrix3x4_t* CModelRender::pBoneToWorldArray()
{
	return g_pStudioRender->GetBoneToWorldArray();
}


static int CountMeshGroups( studiohwdata_t *pStudioHWData )
{
	int totalNumMeshGroups = 0;
	int lod;
	for( lod = 0; lod < pStudioHWData->m_NumLODs; lod++ )
	{
		studioloddata_t *pLOD = &pStudioHWData->m_pLODs[lod];
		int meshID;
		for( meshID = 0; meshID < pStudioHWData->m_NumStudioMeshes; meshID++ )
		{
			studiomeshdata_t *pMesh = &pLOD->m_pMeshData[meshID];
			int groupID;
			for( groupID = 0; groupID < pMesh->m_NumGroup; groupID++ )
			{
				totalNumMeshGroups++;
			}
		}
	}
	return totalNumMeshGroups;
}

// FIXME? : Move this to StudioRender?
void CModelRender::CreateStaticPropColorData( ModelInstanceHandle_t handle )
{
	studiohwdata_t *pStudioHWData = &m_ModelInstances[handle].m_pModel->studio.hardwareData;
	Assert( pStudioHWData );
	int totalNumMeshGroups = CountMeshGroups( pStudioHWData );
	if( totalNumMeshGroups <= 0 )
	{
		return;
	}
	typedef IMesh *IMeshPtr;
	m_ModelInstances[handle].m_ppColorMeshes = new IMeshPtr[totalNumMeshGroups];
	Assert( m_ModelInstances[handle].m_ppColorMeshes );
	if( !m_ModelInstances[handle].m_ppColorMeshes )
	{
		return;
	}

	int i;
	for( i = 0; i < totalNumMeshGroups; i++ )
	{
		m_ModelInstances[handle].m_ppColorMeshes[i] = materialSystemInterface->CreateStaticMesh( MATERIAL_VERTEX_FORMAT_COLOR, false );
		Assert( m_ModelInstances[handle].m_ppColorMeshes[i] );
		if( !m_ModelInstances[handle].m_ppColorMeshes[i] )
		{
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Old-style computation of vertex lighting	(for static props)
//-----------------------------------------------------------------------------
static void LightForVert( const LightingState_t &lightState, const Vector &pos, const Vector &normal, 
						 Vector &destColor )
{
	// FIXME: This conversion could be done per model instead of per vertex.
	LightDesc_t lightDesc[MAXLOCALLIGHTS];
	for ( int i = 0; i < lightState.numlights; i++)
	{
		if( !WorldLightToMaterialLight( lightState.locallight[i], lightDesc[i] ) )
		{
			Assert( 0 );
			continue;
		}

		// Set these up just in case ( Hardware T&L normally doesn't, since they
		// are usually used for software ).
		lightDesc[i].m_Flags = 0.f;
		if( lightDesc[i].m_Attenuation0 != 0.0f )
		{
			lightDesc[i].m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION0;
		}
		if( lightDesc[i].m_Attenuation1 != 0.0f )
		{
			lightDesc[i].m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION1;
		}
		if( lightDesc[i].m_Attenuation2 != 0.0f )
		{
			lightDesc[i].m_Flags |= LIGHTTYPE_OPTIMIZATIONFLAGS_HAS_ATTENUATION2;
		}
	}

	g_pStudioRender->ComputeLighting( lightState.r_boxcolor, lightState.numlights,
		lightDesc, pos, normal, destColor );
}


//-----------------------------------------------------------------------------
// Old-style computation of vertex lighting
//-----------------------------------------------------------------------------
void CModelRender::ComputeModelVertexLightingOld( mstudiomodel_t *pModel, 
	matrix3x4_t& matrix, const LightingState_t &lightingState, color24 *pLighting )
{
	Vector worldPos, worldNormal, destColor;
	for ( int i = 0; i < pModel->numvertices; ++i )
	{
		const Vector &pos = *pModel->Position( i );
		const Vector &normal = *pModel->Normal( i );
		VectorTransform( pos, matrix, worldPos );
		VectorRotate( normal, matrix, worldNormal );

		LightForVert( lightingState, worldPos, worldNormal, destColor );

		destColor[0] = LinearToVertexLight( destColor[0] );
		destColor[1] = LinearToVertexLight( destColor[1] );
		destColor[2] = LinearToVertexLight( destColor[2] );

		ColorClampTruncate( destColor );

		pLighting[i].r = FastFToC(destColor[0]);
		pLighting[i].g = FastFToC(destColor[1]);
		pLighting[i].b = FastFToC(destColor[2]);
	}
}


//-----------------------------------------------------------------------------
// New-style computation of vertex lighting
//-----------------------------------------------------------------------------
void CModelRender::ComputeModelVertexLighting( IHandleEntity *pProp, 
	mstudiomodel_t *pModel, OptimizedModel::ModelLODHeader_t *pVtxLOD,
	matrix3x4_t& matrix, Vector4D *pTempMem, color24 *pLighting )
{
#ifndef SWDS
	int i;
	unsigned char *pInSolid = (unsigned char*)stackalloc( ((pModel->numvertices + 7) >> 3) * sizeof(unsigned char) );
	Vector worldPos, worldNormal;
	for ( i = 0; i < pModel->numvertices; ++i )
	{
		const Vector &pos = *pModel->Position( i );
		const Vector &normal = *pModel->Normal( i );
		VectorTransform( pos, matrix, worldPos );
		VectorRotate( normal, matrix, worldNormal );
		bool bNonSolid = ComputeVertexLightingFromSphericalSamples( worldPos, worldNormal, pProp, &(pTempMem[i].AsVector3D()) );
		
		int nByte = i >> 3;
		int nBit = i & 0x7;

		if ( bNonSolid )
		{
			pTempMem[i].w = 1.0f;
			pInSolid[ nByte ] &= ~(1 << nBit);
		}
		else
		{
			pTempMem[i].Init( );
			pInSolid[ nByte ] |= (1 << nBit);
		}
	}

	// Must iterate over each triangle to average out the colors for those
	// vertices in solid.
	// Iterate over all the meshes....
	for (int meshID = 0; meshID < pModel->nummeshes; ++meshID)
	{
		Assert( pModel->nummeshes == pVtxLOD->numMeshes );
		mstudiomesh_t* pMesh = pModel->pMesh(meshID);
		OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh(meshID);

		// Iterate over all strip groups.
		for( int stripGroupID = 0; stripGroupID < pVtxMesh->numStripGroups; ++stripGroupID )
		{
			OptimizedModel::StripGroupHeader_t* pStripGroup = pVtxMesh->pStripGroup(stripGroupID);
			
			// Iterate over all indices
			Assert( pStripGroup->numIndices % 3 == 0 );
			for (i = 0; i < pStripGroup->numIndices; i += 3)
			{
				unsigned short nIndex1 = *pStripGroup->pIndex( i );
				unsigned short nIndex2 = *pStripGroup->pIndex( i + 1 );
				unsigned short nIndex3 = *pStripGroup->pIndex( i + 2 );

				int v[3];
				v[0] = pStripGroup->pVertex( nIndex1 )->origMeshVertID + pMesh->vertexoffset;
				v[1] = pStripGroup->pVertex( nIndex2 )->origMeshVertID + pMesh->vertexoffset;
				v[2] = pStripGroup->pVertex( nIndex3 )->origMeshVertID + pMesh->vertexoffset;

				Assert( v[0] < pModel->numvertices );
				Assert( v[1] < pModel->numvertices );
				Assert( v[2] < pModel->numvertices );

				bool bSolid[3];
				bSolid[0] = ( pInSolid[ v[0] >> 3 ] & ( 1 << ( v[0] & 0x7 ) ) ) != 0;
				bSolid[1] = ( pInSolid[ v[1] >> 3 ] & ( 1 << ( v[1] & 0x7 ) ) ) != 0;
				bSolid[2] = ( pInSolid[ v[2] >> 3 ] & ( 1 << ( v[2] & 0x7 ) ) ) != 0;

				int nValidCount = 0;
				int nAverage[3];
				if ( !bSolid[0] ) { nAverage[nValidCount++] = v[0]; }
				if ( !bSolid[1] ) { nAverage[nValidCount++] = v[1]; }
				if ( !bSolid[2] ) { nAverage[nValidCount++] = v[2]; }

				if ( nValidCount == 3 )
					continue;

				Vector vecAverage( 0, 0, 0 );
				for ( int j = 0; j < nValidCount; ++j )
				{
					vecAverage += pTempMem[nAverage[j]].AsVector3D();
				}

				if (nValidCount != 0)
				{
					vecAverage /= nValidCount;
				}

				if ( bSolid[0] ) { pTempMem[ v[0] ].AsVector3D() += vecAverage; pTempMem[ v[0] ].w += 1.0f; }
				if ( bSolid[1] ) { pTempMem[ v[1] ].AsVector3D() += vecAverage; pTempMem[ v[1] ].w += 1.0f; }
				if ( bSolid[2] ) { pTempMem[ v[2] ].AsVector3D() += vecAverage; pTempMem[ v[2] ].w += 1.0f; }
			}
		}
	}

	Vector destColor;
	for ( i = 0; i < pModel->numvertices; ++i )
	{
		if ( pTempMem[i].w != 0.0f )
		{
			pTempMem[i] /= pTempMem[i].w;
		}

		destColor[0] = LinearToVertexLight( pTempMem[i][0] );
		destColor[1] = LinearToVertexLight( pTempMem[i][1] );
		destColor[2] = LinearToVertexLight( pTempMem[i][2] );

		ColorClampTruncate( destColor );

		pLighting[i].r = FastFToC(destColor[0]);
		pLighting[i].g = FastFToC(destColor[1]);
		pLighting[i].b = FastFToC(destColor[2]);
	}
#endif
}



//-----------------------------------------------------------------------------
// Computes the static prop color data
//-----------------------------------------------------------------------------
static ConVar r_newproplighting( "r_newproplighting", "0" );

void CModelRender::UpdateStaticPropColorData( IHandleEntity *pProp, ModelInstanceHandle_t handle )
{
#ifndef SWDS
	// FIXME? : Move this to StudioRender?
	ModelInstance_t &inst = m_ModelInstances[handle];
	Assert( inst.m_pModel );

	studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( inst.m_pModel );
	studiohwdata_t *pStudioHWData = &inst.m_pModel->studio.hardwareData;
	Assert( pStudioHdr && pStudioHWData );

#ifdef _DEBUG
	// Used below to make sure everything's ok
	int totalNumMeshGroups = CountMeshGroups( pStudioHWData );
#endif // _DEBUG

	// FIXME!!!!  We should try to load this at the same time that we load the rest of the model.
	CUtlMemory<unsigned char> tmpVtxMem; 
	CUtlMemory<color24> tmpLightingMem; 
	CUtlMemory<Vector4D> tmpColorMem; 

	// Load the vtx file into a temp buffer that'll go away after we leave this scope.
#ifdef _DEBUG
	bool retVal = 
#endif		
		Mod_LoadStudioModelVtxFileIntoTempBuffer( inst.m_pModel, tmpVtxMem );
	Assert( retVal );
	OptimizedModel::FileHeader_t *pVtxHdr = ( OptimizedModel::FileHeader_t * )tmpVtxMem.Base();

	// Sets the model transform state in g_pStudioRender
	matrix3x4_t matrix;
	AngleMatrix( inst.m_pRenderable->GetRenderAngles(), inst.m_pRenderable->GetRenderOrigin(), matrix );
	
	Assert( m_LightingOriginOverride );
	Assert( m_LightCacheHandle );
	
	// Get static lighting only!!  We'll add dynamic and lightstyles in in the vertex shader.
	LightingState_t lightingState;
	LightcacheGet( m_LightCacheHandle, lightingState, LIGHTCACHEFLAGS_STATIC );
		
	int colorMeshID = 0;
	int lodID;
	bool bUseNewLighting = r_newproplighting.GetInt() != 0;
	for( lodID = 0; lodID < pVtxHdr->numLODs; lodID++ )
	{
		studioloddata_t *pStudioLODData = &pStudioHWData->m_pLODs[lodID];
		studiomeshdata_t *pStudioMeshData = pStudioLODData->m_pMeshData;
		int	bodyPartID, modelID, meshID, stripGroupID;

		// Iterate over every body part...
		for ( bodyPartID = 0; bodyPartID < pStudioHdr->numbodyparts; bodyPartID++ )
		{
			mstudiobodyparts_t* pBodyPart = pStudioHdr->pBodypart(bodyPartID);
			OptimizedModel::BodyPartHeader_t* pVtxBodyPart = pVtxHdr->pBodyPart(bodyPartID);

			// Iterate over every submodel...
			for (modelID = 0; modelID < pBodyPart->nummodels; ++modelID)
			{
				mstudiomodel_t* pModel = pBodyPart->pModel(modelID);
				OptimizedModel::ModelHeader_t* pVtxModel = pVtxBodyPart->pModel(modelID);
				OptimizedModel::ModelLODHeader_t *pVtxLOD = pVtxModel->pLOD( lodID );

				// Make sure we've got enough space allocated
				tmpLightingMem.EnsureCapacity( pModel->numvertices );

				// Compute lighting for each unique vertex in the model exactly once
				if (!bUseNewLighting)
				{
					ComputeModelVertexLightingOld( pModel, matrix, lightingState, tmpLightingMem.Base() );
				}
				else
				{
					tmpColorMem.EnsureCapacity( pModel->numvertices );
					ComputeModelVertexLighting( pProp, pModel, pVtxLOD, matrix, tmpColorMem.Base(), tmpLightingMem.Base() );
				}

				// Iterate over all the meshes....
				for (meshID = 0; meshID < pModel->nummeshes; ++meshID)
				{
					Assert( pModel->nummeshes == pVtxLOD->numMeshes );
					mstudiomesh_t* pMesh = pModel->pMesh(meshID);
					OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh(meshID);

					// Iterate over all strip groups.
					for( stripGroupID = 0; stripGroupID < pVtxMesh->numStripGroups; ++stripGroupID )
					{
						OptimizedModel::StripGroupHeader_t* pStripGroup = pVtxMesh->pStripGroup(stripGroupID);
						studiomeshgroup_t* pMeshGroup = &pStudioMeshData[meshID].m_pMeshGroup[stripGroupID];

						pMeshGroup->m_ColorMeshID = colorMeshID;
						
						CMeshBuilder meshBuilder;
						meshBuilder.Begin( m_ModelInstances[handle].m_ppColorMeshes[colorMeshID], 
							MATERIAL_HETEROGENOUS, pStripGroup->numVerts, 0 );

						// Iterate over all vertices
						int i;
						for (i = 0; i < pStripGroup->numVerts; ++i)
						{
							OptimizedModel::Vertex_t *pVtxVertex = pStripGroup->pVertex(i);
							int nVertIndex = pMesh->vertexoffset + pVtxVertex->origMeshVertID;
							Assert( nVertIndex < pModel->numvertices );
							meshBuilder.Specular3ub( tmpLightingMem[nVertIndex].r, tmpLightingMem[nVertIndex].g, tmpLightingMem[nVertIndex].b );
							meshBuilder.AdvanceVertex();
						}
						meshBuilder.End();
						
						colorMeshID++;
					}
				}
			}
		}
	}
	Assert( colorMeshID == totalNumMeshGroups );
#endif
}


//-----------------------------------------------------------------------------
// FIXME? : Move this to StudioRender?
//-----------------------------------------------------------------------------
void CModelRender::DestroyStaticPropColorData( ModelInstanceHandle_t handle )
{
#ifndef SWDS
	studiohwdata_t *pStudioHWData = &m_ModelInstances[handle].m_pModel->studio.hardwareData;
	Assert( pStudioHWData );
	int totalNumMeshGroups = CountMeshGroups( pStudioHWData );
	int i;
	for( i = 0; i < totalNumMeshGroups; i++ )
	{
		if( m_ModelInstances[handle].m_ppColorMeshes[i] )
		{
			materialSystemInterface->DestroyStaticMesh( m_ModelInstances[handle].m_ppColorMeshes[i] );
			m_ModelInstances[handle].m_ppColorMeshes[i] = NULL;
		}
	}
	delete [] m_ModelInstances[handle].m_ppColorMeshes;
	m_ModelInstances[handle].m_ppColorMeshes = NULL;
#endif
}


//-----------------------------------------------------------------------------
// Creates, destroys instance data to be associated with the model
//-----------------------------------------------------------------------------
ModelInstanceHandle_t CModelRender::CreateInstance( IClientRenderable *pRenderable, bool bStaticProp )
{
	Assert( pRenderable );
	ModelInstanceHandle_t handle = m_ModelInstances.AddToTail();
	m_ModelInstances[handle].m_pRenderable = pRenderable;
	m_ModelInstances[handle].m_DecalHandle = STUDIORENDER_DECAL_INVALID;
#ifndef SWDS
	m_ModelInstances[handle].m_FirstShadow = g_pShadowMgr->InvalidShadowIndex();
#endif
	m_ModelInstances[handle].m_pModel = (model_t*)pRenderable->GetModel();
	m_ModelInstances[handle].m_ppColorMeshes = NULL;
	m_ModelInstances[handle].m_flLightingTime = CURRENT_LIGHTING_UNINITIALIZED;

	studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( m_ModelInstances[handle].m_pModel );
	studiohwdata_t *pStudioHWData = &m_ModelInstances[handle].m_pModel->studio.hardwareData;
	Assert( pStudioHdr && pStudioHWData );
	if( !pStudioHdr || !pStudioHWData )
	{
		return MODEL_INSTANCE_INVALID;
	}
	if( bStaticProp && !( pStudioHdr->flags & STUDIOHDR_FLAGS_STATIC_PROP ) )
	{
		static int bitchCount = 0;
		if( bitchCount < 100 )
		{
			Warning( "model %s used as a static prop, but not compiled as a static prop\n", pStudioHdr->name );
			bitchCount++;
		}
	}
	if( pStudioHdr && pStudioHWData && bStaticProp )
	{
		if( g_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() )
		{
			CreateStaticPropColorData( handle );
			UpdateStaticPropColorData( pRenderable->GetIClientUnknown(), handle );
		}
	}

	return handle;
}

// This gets called when overbright, etc gets changed to recompute static prop lighting.
void CModelRender::RecomputeStaticLighting( IClientRenderable *pRenderable, ModelInstanceHandle_t handle )
{
	studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( m_ModelInstances[handle].m_pModel );
	studiohwdata_t *pStudioHWData = &m_ModelInstances[handle].m_pModel->studio.hardwareData;
	Assert( pStudioHdr && pStudioHWData );
	if( pStudioHdr && pStudioHWData && pStudioHdr->flags & STUDIOHDR_FLAGS_STATIC_PROP )
	{
		if( g_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() )
		{
			// FIXME: Should destroy in reverse order and the recreate.
			DestroyStaticPropColorData( handle );
			CreateStaticPropColorData( handle );
			UpdateStaticPropColorData( pRenderable->GetIClientUnknown(), handle );
		}
	}
}

void CModelRender::DestroyInstance( ModelInstanceHandle_t handle )
{
	if (handle != MODEL_INSTANCE_INVALID)
	{
		g_pStudioRender->DestroyDecalList(m_ModelInstances[handle].m_DecalHandle);
#ifndef SWDS
		g_pShadowMgr->RemoveAllShadowsFromModel( handle );
#endif
		if( m_ModelInstances[handle].m_ppColorMeshes )
		{
			DestroyStaticPropColorData( handle );
		}
		m_ModelInstances.Remove( handle );
	}
}

bool CModelRender::HasStaticPropColorData( int modelInstanceID )
{
	return m_ModelInstances[modelInstanceID].m_ppColorMeshes != NULL;
}


//-----------------------------------------------------------------------------
// It's not valid if the model index changed + we have non-zero instance data
//-----------------------------------------------------------------------------
bool CModelRender::IsModelInstanceValid( ModelInstanceHandle_t handle )
{
	ModelInstance_t inst = m_ModelInstances[handle];
	if ( inst.m_DecalHandle == STUDIORENDER_DECAL_INVALID )
		return true;

	model_t const* pModel = inst.m_pRenderable->GetModel();
	return inst.m_pModel == pModel;
}


//-----------------------------------------------------------------------------
// Creates a decal on a model instance by doing a planar projection
//-----------------------------------------------------------------------------
void CModelRender::AddDecal( ModelInstanceHandle_t handle, Ray_t const& ray,
	const Vector& decalUp, int decalIndex, int body, bool noPokeThru, int maxLODToDecal )
{
	if (handle == MODEL_INSTANCE_INVALID)
		return;

	// Get the decal material + radius
	IMaterial* pDecalMaterial;
	float w, h;
	R_DecalGetMaterialAndSize( decalIndex, pDecalMaterial, w, h );
	w *= 0.5f;
	h *= 0.5f;

	// FIXME: For now, don't render fading decals on props...
	bool found;
	pDecalMaterial->FindVar( "$decalFadeDuration", &found, false );
	if (found)
		return;

	// FIXME: Pass w and h into AddDecal
	float radius = (w > h) ? w : h;

	ModelInstance_t& inst = m_ModelInstances[handle];
	if (!IsModelInstanceValid(handle))
	{
		g_pStudioRender->DestroyDecalList(inst.m_DecalHandle);
		inst.m_DecalHandle = STUDIORENDER_DECAL_INVALID;
	}

	if ( inst.m_DecalHandle == STUDIORENDER_DECAL_INVALID )
	{
		inst.m_DecalHandle = g_pStudioRender->CreateDecalList( &(inst.m_pModel->studio.hardwareData) );
	}

	SetupModelState( inst.m_pRenderable );

	g_pStudioRender->AddDecal(inst.m_DecalHandle, modelinfo->GetStudiomodel(inst.m_pModel),
		ray, decalUp, pDecalMaterial, radius, body, noPokeThru, maxLODToDecal );
}

//-----------------------------------------------------------------------------
// Purpose: Removes all the decals on a model instance
//-----------------------------------------------------------------------------
void CModelRender::RemoveAllDecals( ModelInstanceHandle_t handle )
{
	if (handle == MODEL_INSTANCE_INVALID)
		return;

	ModelInstance_t& inst = m_ModelInstances[handle];
	if (!IsModelInstanceValid(handle))
		return;

	g_pStudioRender->RemoveAllDecals( inst.m_DecalHandle, modelinfo->GetStudiomodel(inst.m_pModel) );
	inst.m_DecalHandle = STUDIORENDER_DECAL_INVALID;
}
