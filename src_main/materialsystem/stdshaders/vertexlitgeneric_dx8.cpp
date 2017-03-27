//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

DEFINE_FALLBACK_SHADER( VertexLitGeneric, VertexLitGeneric_DX8 )

BEGIN_VS_SHADER( VertexLitGeneric_DX8, 
				"Help for VertexLitGeneric" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "envmap frame number" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTimesLightmapPlusMaskedCubicEnvMap_glass", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// default to 'MODEL' mode...
		if (!IS_FLAG_DEFINED( MATERIAL_VAR_MODEL ))
			SET_FLAGS( MATERIAL_VAR_MODEL );

		if( !params[ENVMAPMASKSCALE]->IsDefined() )
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );

		if( !params[ENVMAPMASKFRAME]->IsDefined() )
			params[ENVMAPMASKFRAME]->SetIntValue( 0 );
		
		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[SELFILLUMTINT]->IsDefined() )
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 4.0f );

		if( !params[ENVMAPCONTRAST]->IsDefined() )
			params[ENVMAPCONTRAST]->SetFloatValue( 0.0f );
		
		if( !params[ENVMAPSATURATION]->IsDefined() )
			params[ENVMAPSATURATION]->SetFloatValue( 1.0f );
		
		if( !params[ENVMAPFRAME]->IsDefined() )
			params[ENVMAPFRAME]->SetIntValue( 0 );

		if( !params[BUMPFRAME]->IsDefined() )
			params[BUMPFRAME]->SetIntValue( 0 );

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

		// Force software skinning if the file says so
		if (!IS_FLAG_SET(MATERIAL_VAR_NEEDS_SOFTWARE_SKINNING))
		{
			// Software skin if diffuse bump mapping is used at all.
			if( g_pConfig->bBumpmap && 
				( ( params[BASETEXTURE]->IsDefined() && params[BUMPMAP]->IsDefined() ) ||
				  ( params[BUMPMAP]->IsDefined() && !params[ENVMAP]->IsDefined() ) ) )
			{
				SET_FLAGS( MATERIAL_VAR_NEEDS_SOFTWARE_SKINNING );
			}
		}

		if( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		}
		
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

		if( !params[DETAIL]->IsDefined() && !params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_TEXKILL );
		}
	}

	SHADER_FALLBACK
	{	
		if (g_pConfig->bSoftwareLighting)
			return "VertexLitGeneric_DX6";

		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
			return "VertexLitGeneric_DX7";
		return 0;
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
				CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		if (params[DETAIL]->IsDefined())
		{
			LoadTexture( DETAIL );
		}

		if (g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined())
		{
			LoadBumpMap( BUMPMAP );
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
			
		if (params[ENVMAP]->IsDefined())
		{
			if( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
				LoadCubeMap( ENVMAP );
			else
				LoadTexture( ENVMAP );

			if( !g_pHardwareConfig->SupportsCubeMaps() )
			{
				SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
			}

			if (params[ENVMAPMASK]->IsDefined())
				LoadTexture( ENVMAPMASK );
		}
	}

	inline const char *GetUnbumpedVertexShaderName( IMaterialVar **params )
	{
		static char const* s_pVertexShaders[] = 
		{
			"VertexLitTexture",
			"VertexLitEnvMappedTexture",
			"VertexLitTexture",
			"VertexLitEnvMappedTexture_Spheremap",

			"VertexLitTexture",
			"VertexLitEnvMappedTexture_CameraSpace",
			"VertexLitTexture",
			"VertexLitEnvMappedTexture_Spheremap",

			"VertexLitGeneric_Detail",
			"VertexLitGeneric_DetailEnvMap",
			"VertexLitGeneric_Detail",
			"VertexLitGeneric_DetailEnvMapSphere",

			"VertexLitGeneric_Detail",
			"VertexLitGeneric_DetailEnvMapCameraSpace",
			"VertexLitGeneric_Detail",
			"VertexLitGeneric_DetailEnvMapSphere",
		};

		// FIXME: Add vertex color support?
		int vshIndex = 0;
		if (params[ENVMAP]->IsDefined())
			vshIndex |= 0x1;
		if (IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE))
			vshIndex |= 0x2;
		if (IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE))
			vshIndex |= 0x4;
		if (params[DETAIL]->IsDefined())
			vshIndex |= 0x8;
		return s_pVertexShaders[vshIndex];
	}
	
	inline const char *GetUnbumpedPixelShaderName( IMaterialVar** params )
	{
		static char const* s_pPixelShaders[] = 
		{
			"VertexLitGeneric_EnvmapV2",
			"VertexLitGeneric_SelfIlluminatedEnvmapV2",

			"VertexLitGeneric_BaseAlphaMaskedEnvmapV2",
			"VertexLitGeneric_SelfIlluminatedEnvmapV2",

			// Env map mask
			"VertexLitGeneric_MaskedEnvmapV2",
			"VertexLitGeneric_SelfIlluminatedMaskedEnvmapV2",

			"VertexLitGeneric_MaskedEnvmapV2",
			"VertexLitGeneric_SelfIlluminatedMaskedEnvmapV2",

			// Detail
			"VertexLitGeneric_DetailEnvmapV2",
			"VertexLitGeneric_DetailSelfIlluminatedEnvmapV2",

			"VertexLitGeneric_DetailBaseAlphaMaskedEnvmapV2",
			"VertexLitGeneric_DetailSelfIlluminatedEnvmapV2",

			// Env map mask
			"VertexLitGeneric_DetailMaskedEnvmapV2",
			"VertexLitGeneric_DetailSelfIlluminatedMaskedEnvmapV2",

			"VertexLitGeneric_DetailMaskedEnvmapV2",
			"VertexLitGeneric_DetailSelfIlluminatedMaskedEnvmapV2",
		};

		if (!params[BASETEXTURE]->IsDefined())
		{
			if (params[ENVMAP]->IsDefined())
			{
				if (!params[ENVMAPMASK]->IsDefined())
				{
					return "VertexLitGeneric_EnvmapNoTexture";
				}
				else
				{
					return "VertexLitGeneric_MaskedEnvmapNoTexture";
				}
			}
			else
			{
				if (params[DETAIL]->IsDefined())
				{
					return "VertexLitGeneric_DetailNoTexture";
				}
				else
				{
					return "VertexLitGeneric_NoTexture";
				}
			}
		}
		else
		{
			if (params[ENVMAP]->IsDefined())
			{
				int pshIndex = 0;
				if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM))
					pshIndex |= 0x1;
				if (IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
					pshIndex |= 0x2;
				if (params[ENVMAPMASK]->IsDefined())
					pshIndex |= 0x4;
				if (params[DETAIL]->IsDefined())
					pshIndex |= 0x8;
				return s_pPixelShaders[pshIndex];
			}
			else
			{
				if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM))
				{
					if (params[DETAIL]->IsDefined())
						return "VertexLitGeneric_DetailSelfIlluminated";
					else
						return "VertexLitGeneric_SelfIlluminated";
				}
				else
					if (params[DETAIL]->IsDefined())
						return "VertexLitGeneric_Detail";
					else
						return "VertexLitGeneric";
			}
		}
	}

	void DrawUnbumpedUsingVertexShader( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );

			if (!IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT))
				pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );

			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX;

			// FIXME: We could enable this, but we'd never get it working on dx7 or lower
			// FIXME: This isn't going to work until we make more vertex shaders that
			// pass the vertex color and alpha values through.
#if 0
			if ( IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) || IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA ) )
				fmt |= VERTEX_COLOR;
#endif

			if (params[ENVMAP]->IsDefined())
			{
				// envmap on stage 1
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

				// envmapmask on stage 2
				if (params[ENVMAPMASK]->IsDefined() || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK ) )
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			}

			if (params[BASETEXTURE]->IsDefined())
				SetDefaultBlendingShadowState( BASETEXTURE, true );
			else
				SetDefaultBlendingShadowState( ENVMAPMASK, false );

			if (params[DETAIL]->IsDefined())
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );

			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 3, 0 );
			const char *vshName = GetUnbumpedVertexShaderName( params );
			pShaderShadow->SetVertexShader( vshName );
			const char *pshName = GetUnbumpedPixelShaderName( params );
			pShaderShadow->SetPixelShader( pshName );
		}
		DYNAMIC_STATE
		{
			if (params[BASETEXTURE]->IsDefined())
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			}

			if (params[ENVMAP]->IsDefined())
			{
				BindTexture( SHADER_TEXTURE_STAGE1, ENVMAP, ENVMAPFRAME );

				if (params[ENVMAPMASK]->IsDefined() || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
				{
					if (params[ENVMAPMASK]->IsDefined() )
						BindTexture( SHADER_TEXTURE_STAGE2, ENVMAPMASK, ENVMAPMASKFRAME );
					else
						BindTexture( SHADER_TEXTURE_STAGE2, BASETEXTURE, FRAME );
		
					SetVertexShaderTextureScaledTransform( 92, BASETEXTURETRANSFORM, ENVMAPMASKSCALE );
				}

				if (IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) || 
					IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE))
				{
					LoadViewMatrixIntoVertexShaderConstant( 17 );
				}
				SetEnvMapTintPixelShaderDynamicState( 2, ENVMAPTINT, -1 );
			}

			if (params[DETAIL]->IsDefined())
			{
				BindTexture( SHADER_TEXTURE_STAGE3, DETAIL, FRAME );
				SetVertexShaderTextureScaledTransform( 94, BASETEXTURETRANSFORM, DETAILSCALE );
			}

			SetAmbientCubeDynamicStateVertexShader();
			SetModulationPixelShaderDynamicState( 3 );
			if (!IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT))
			{
				EnablePixelShaderOverbright( 0, true, true );
			}
			SetPixelShaderConstant( 1, SELFILLUMTINT );
			DefaultFog();
		}
		Draw();
	}

	inline const char *GetBumpedDiffusePixelShaderName( IMaterialVar **params )
	{
		static char const* s_pPixelShaders[] = 
		{
			"VertexLitGeneric_DiffBumpLightingOnly_Overbright2",
			"VertexLitGeneric_DiffBumpLightingOnly_Overbright2_Translucent",
			"VertexLitGeneric_DiffBumpTimesBase_Overbright2",
			"VertexLitGeneric_DiffBumpTimesBase_Overbright2_Translucent",

			"VertexLitGeneric_DiffBumpLightingOnly_Overbright1",
			"VertexLitGeneric_DiffBumpLightingOnly_Overbright1_Translucent",
			"VertexLitGeneric_DiffBumpTimesBase_Overbright1",
			"VertexLitGeneric_DiffBumpTimesBase_Overbright1_Translucent",
		};

		
		int pixelShaderID = 0x0;
		if( params[BASETEXTURE]->IsDefined() && TextureIsTranslucent( BASETEXTURE, true ) )
		{
			pixelShaderID |= 0x1;
		}
		if( params[BASETEXTURE]->IsDefined() )
		{
			pixelShaderID |= 0x2;
		}
		if( IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT) || g_pConfig->overbright == 1.0f )
		{
			pixelShaderID |= 0x4;
		}
		return s_pPixelShaders[pixelShaderID];
	}
	
	void DrawBumpedDiffusePass( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			
			if (!IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT))
				pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );
			
			SetDefaultBlendingShadowState( BASETEXTURE, true );

			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 3, 4 /* userDataSize */ );
			pShaderShadow->SetVertexShader( "VertexLitGeneric_DiffBumpTimesBase" );
			const char *pshName = GetBumpedDiffusePixelShaderName( params );
			pShaderShadow->SetPixelShader( pshName );
		}
		DYNAMIC_STATE
		{

			if( params[BASETEXTURE]->IsDefined() )
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			}
			BindTexture( SHADER_TEXTURE_STAGE1, BUMPMAP, BUMPFRAME );
			SetAmbientCubeDynamicStateVertexShader();

			LoadBumpLightmapCoordinateAxes_VertexShader( 90 );
			LoadBumpLightmapCoordinateAxes_PixelShader( 0 );
			
			//SetVertexShaderTextureTransform( 93, BASETEXTURETRANSFORM );
			//SetVertexShaderConstantVect( 95, BUMPOFFSET );
			SetModulationPixelShaderDynamicState( 3 );
		}
		Draw();
	}
	
	void DrawBumpedSelfIllumPass( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			// Don't bother with z writes here...
 			pShaderShadow->EnableDepthWrites( false );
			// Override the Modulation+blending shadow State this time; we're always modulating+blending
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			int fmt = VERTEX_POSITION | VERTEX_BONE_INDEX;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, NULL, 3, 0 /* userDataSize */ );
			pShaderShadow->SetVertexShader( "VertexLitGeneric_SelfIllumOnly" );
			pShaderShadow->SetPixelShader( "VertexLitGeneric_SelfIllumOnly" );
		}
					
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			// keep the base texture bound from the previous pass.

			float constantColor[4];
			params[SELFILLUMTINT]->GetVecValue( constantColor, 3 );
			constantColor[3] = params[ALPHA]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( 0, constantColor, 1 );
		}
		Draw();
	}
	

	void DrawBumpedUsingVertexShader( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		bool bBlendSpecular = false;
		// diffuse pass
		if( ( params[BASETEXTURE]->IsDefined() && params[BUMPMAP]->IsDefined() ) ||
			( params[BUMPMAP]->IsDefined() && !params[ENVMAP]->IsDefined() ) )
		{
			bBlendSpecular = true;
			DrawBumpedDiffusePass( params, pShaderAPI, pShaderShadow );
		}
		
		// OPTIMIZE!!  Can we fit self-illum and self-illum tint into the previous pass?
		// In some case, we can, but probably not the common cases.
		if( IS_FLAG_SET( MATERIAL_VAR_SELFILLUM ) )
		{
			DrawBumpedSelfIllumPass( params, pShaderAPI, pShaderShadow );
		}
		
		// specular pass
		if( params[ENVMAP]->IsDefined() )
		{
			DrawModelBumpedSpecularLighting( BUMPMAP, BUMPFRAME,
				ENVMAP, ENVMAPFRAME,
				ENVMAPTINT, ALPHA,
				ENVMAPCONTRAST, ENVMAPSATURATION,
				BUMPTRANSFORM,
				bBlendSpecular );
		}
	}

	SHADER_DRAW
	{
		if( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() )
		{
			DrawBumpedUsingVertexShader( params, pShaderAPI, pShaderShadow );
		}
		else
		{
			DrawUnbumpedUsingVertexShader( params, pShaderAPI, pShaderShadow );
		}
	}
END_SHADER


