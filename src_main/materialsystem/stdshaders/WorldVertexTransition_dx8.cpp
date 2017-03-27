//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "convar.h"

DEFINE_FALLBACK_SHADER( WorldVertexTransition, WorldVertexTransition_DX8 )

#ifdef _DEBUG
//#define DEBUG_CONVARS
#endif

#ifdef DEBUG_CONVARS
ConVar mat_wvt_Texture1( "mat_wvt_Texture1", "1", 0, "Enable/disable texture 1 in the WorldVertexTransition shader." );
ConVar mat_wvt_Texture2( "mat_wvt_Texture2", "1", 0, "Enable/disable texture 2 in the WorldVertexTransition shader." );
ConVar mat_wvt_Micros( "mat_wvt_Micros", "1", 0, "Enable/disable micro textures in the WorldVertexTransition shader." );
#endif

BEGIN_VS_SHADER( WorldVertexTransition_DX8,
			  "Help for WorldVertexTransition_DX8" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture2", "base texture2 help" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $baseTexture" )
		SHADER_PARAM( BASETEXTURETRANSFORM2, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$baseTexture texcoord transform" )
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map for BASETEXTURE" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( BUMPBASETEXTURE2WITHBUMPMAP, SHADER_PARAM_TYPE_BOOL, "0", "" )
		SHADER_PARAM( FRESNELREFLECTION, SHADER_PARAM_TYPE_FLOAT, "0.0", "1.0 == mirror, 0.0 == water" )
	
		SHADER_PARAM( MICROS, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "micro textures (in blue and alpha channels)" )
		SHADER_PARAM( MICROS_TRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( MICROS_FRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if( !g_pHardwareConfig->SupportsVertexAndPixelShaders() ||
			(g_pConfig->bEditMode && !params[MICROS]->IsDefined()) )
		{
			return "WorldVertexTransition_DX6";
		}
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );

		if( IsUsingGraphics() && params[ENVMAP]->IsDefined() )
		{
			if( stricmp( params[ENVMAP]->GetStringValue(), "env_cubemap" ) == 0 )
			{
				Warning( "env_cubemap used on world geometry without rebuilding map. . ignoring: %s\n", pMaterialName );
				params[ENVMAP]->SetUndefined();
			}
		}
		
		if( !params[ENVMAPMASKSCALE]->IsDefined() )
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );

		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[SELFILLUMTINT]->IsDefined() )
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 4.0f );

		if( !params[FRESNELREFLECTION]->IsDefined() )
			params[FRESNELREFLECTION]->SetFloatValue( 1.0f );

		if( !params[ENVMAPMASKFRAME]->IsDefined() )
			params[ENVMAPMASKFRAME]->SetIntValue( 0 );
		
		if( !params[ENVMAPFRAME]->IsDefined() )
			params[ENVMAPFRAME]->SetIntValue( 0 );

		if( !params[BUMPFRAME]->IsDefined() )
			params[BUMPFRAME]->SetIntValue( 0 );

		if( !params[ENVMAPCONTRAST]->IsDefined() )
			params[ENVMAPCONTRAST]->SetFloatValue( 0.0f );
		
		if( !params[ENVMAPSATURATION]->IsDefined() )
			params[ENVMAPSATURATION]->SetFloatValue( 1.0f );
		
		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		if( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP );
		}

		if( !params[BUMPBASETEXTURE2WITHBUMPMAP]->IsDefined() )
		{
			params[BUMPBASETEXTURE2WITHBUMPMAP]->SetIntValue( 0 );
		}
	}
	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		LoadTexture( BASETEXTURE2 );
		if ( params[MICROS]->IsDefined() )
			LoadTexture( MICROS );

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		if( params[ENVMAP]->IsDefined() && !params[BUMPMAP]->IsDefined() )
		{
			Warning( "must have $bumpmap if you have $envmap for worldvertextransition\n" );
			params[ENVMAP]->SetUndefined();
			params[BUMPMAP]->SetUndefined();
		}
		if( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP );
			LoadBumpMap( BUMPMAP );
		}
		if( params[ENVMAP]->IsDefined() )
		{
			if( !IS_FLAG_SET( MATERIAL_VAR_ENVMAPSPHERE ) )
			{
				LoadCubeMap( ENVMAP );
			}
			else
			{
				Warning( "$envmapsphere not supported by worldvertextransition\n" );
				params[ENVMAP]->SetUndefined();
			}
		}
	}

	SHADER_DRAW
	{
		if( !params[BUMPMAP]->IsDefined() || !g_pConfig->bBumpmap )
		{
			SHADOW_STATE
			{
				// This is the dx8, non-worldcraft version, non-bumped version
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
				if ( params[MICROS]->IsDefined() )
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
				
				int fmt = VERTEX_POSITION;
				pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0, 0 );

				if ( params[MICROS]->IsDefined() )
				{
					pShaderShadow->SetVertexShader( "WorldVertexTransition_Micros" );
					if( g_pConfig->bEditMode )
					{
						pShaderShadow->SetPixelShader( "WorldVertexTransition_Micros_EditMode", 0 );
					}
					else
					{
						pShaderShadow->SetPixelShader( "WorldVertexTransition_Micros" );
					}
				}
				else
				{				
					pShaderShadow->SetVertexShader( "WorldVertexTransition" );
					pShaderShadow->SetPixelShader( "WorldVertexTransition" );
				}
			}

			DYNAMIC_STATE
			{
				// Texture 1
#ifdef DEBUG_CONVARS
				if ( mat_wvt_Texture1.GetInt() )
					BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				else
					pShaderAPI->BindWhite( SHADER_TEXTURE_STAGE0 );

				// Texture 2
				if ( mat_wvt_Texture2.GetInt() )
					BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE2, FRAME2 );
				else
					pShaderAPI->BindWhite( SHADER_TEXTURE_STAGE1 );
#else
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE2, FRAME2 );
#endif
				

				// Texture 3 = lightmap
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE2 );
				
				// Texture 4 = micro texture
				if ( params[MICROS]->IsDefined() )
				{
#ifdef DEBUG_CONVARS
					if ( mat_wvt_Micros.GetInt() )
					{
						BindTexture( SHADER_TEXTURE_STAGE3, MICROS, MICROS_FRAME );
					}
					else
					{
						pShaderAPI->BindGrey( SHADER_TEXTURE_STAGE3 );
					}
#else
					BindTexture( SHADER_TEXTURE_STAGE3, MICROS, MICROS_FRAME );
#endif

					if ( params[MICROS_TRANSFORM]->IsDefined() )
					{
						SetVertexShaderTextureTransform( 94, MICROS_TRANSFORM );
					}
				}
				
				EnablePixelShaderOverbright( 0, !IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT), true );
				
				SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
				SetVertexShaderTextureTransform( 92, BASETEXTURETRANSFORM2 );
			
				FogToFogColor();
			}
			Draw();
		}
		else
		{
			if( params[BUMPBASETEXTURE2WITHBUMPMAP]->GetIntValue() )
			{
				DrawWorldBumpedUsingVertexShader( BASETEXTURE, BASETEXTURETRANSFORM,
					BUMPMAP, BUMPFRAME, BUMPTRANSFORM, ENVMAPMASK, ENVMAPMASKFRAME, ENVMAP, 
					ENVMAPFRAME, ENVMAPTINT, ALPHA,	ENVMAPCONTRAST, ENVMAPSATURATION, FRAME,
					FRESNELREFLECTION, true, BASETEXTURE2, BASETEXTURETRANSFORM2, FRAME2 );
			}
			else
			{
				// draw the base texture with everything else you normally would for
				// bumped world materials
				DrawWorldBumpedUsingVertexShader( BASETEXTURE, BASETEXTURETRANSFORM,
					BUMPMAP, BUMPFRAME, BUMPTRANSFORM, ENVMAPMASK, ENVMAPMASKFRAME, ENVMAP, 
					ENVMAPFRAME, ENVMAPTINT, ALPHA,	ENVMAPCONTRAST, ENVMAPSATURATION, FRAME,
					FRESNELREFLECTION );

				// blend basetexture 2 on top of everything using lightmap alpha.
				SHADOW_STATE
				{
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
					pShaderShadow->VertexShaderVertexFormat( 
						VERTEX_POSITION, 2, 0, 0, 0 );
					pShaderShadow->EnableBlending( true );
					pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
					pShaderShadow->SetVertexShader( "LightmappedGeneric" );
					pShaderShadow->SetPixelShader( "WorldVertexTransition_BlendBase2" );
				}
				DYNAMIC_STATE
				{
					BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE2, FRAME2 );
					pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
					
					float half[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
					pShaderAPI->SetPixelShaderConstant( 4, half );
					EnablePixelShaderOverbright( 0, !IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT), true );
					SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM2 );
					FogToFogColor();
				}
				Draw();
			}
		}
	}
END_SHADER

