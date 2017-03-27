//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

DEFINE_FALLBACK_SHADER( LightmappedGeneric, LightmappedGeneric_DX8 )

BEGIN_VS_SHADER( LightmappedGeneric_DX8,
			  "Help for LightmappedGeneric_DX8" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( FRESNELREFLECTION, SHADER_PARAM_TYPE_FLOAT, "0.0", "0.0 == no fresnel, 1.0 == full fresnel" )
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
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
	}

	SHADER_FALLBACK
	{
		if (g_pHardwareConfig->GetDXSupportLevel() < 80)
			return "LightmappedGeneric_DX6";
		return 0;
	}

	SHADER_INIT
	{
		if( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() )
		{
			LoadBumpMap( BUMPMAP );
		}
		
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

		if( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		}
	}

	inline const char *GetVertexShaderName( IMaterialVar **params )
	{
		int materialVarFlags = params[FLAGS]->GetIntValue();

		static char const* s_pVertexShaders[] = 
		{
			"LightmappedGeneric",
			"LightmappedGeneric_VertexColor",
			"LightmappedGeneric",
			"LightmappedGeneric_VertexColor",
			"LightmappedGeneric",
			"LightmappedGeneric_VertexColor",
			"LightmappedGeneric",
			"LightmappedGeneric_VertexColor",

			"LightmappedGeneric_EnvMap",
			"LightmappedGeneric_EnvMapVertexColor",
			"LightmappedGeneric_EnvMapSphere",
			"LightmappedGeneric_EnvMapSphereVertexColor",
			"LightmappedGeneric_EnvMapCameraSpace",
			"LightmappedGeneric_EnvMapCameraSpaceVertexColor",
			"LightmappedGeneric_EnvMapSphere",
			"LightmappedGeneric_EnvMapSphereVertexColor",
		};

		int vshIndex = 0;
		if (materialVarFlags & MATERIAL_VAR_VERTEXCOLOR)
			vshIndex |= 0x1;
		if (materialVarFlags & MATERIAL_VAR_ENVMAPSPHERE)
			vshIndex |= 0x2;
		if (materialVarFlags & MATERIAL_VAR_ENVMAPCAMERASPACE)
			vshIndex |= 0x4;
		if (params[ENVMAP]->IsDefined())
			vshIndex |= 0x8;

		return s_pVertexShaders[vshIndex];
	}
	inline const char *GetPixelShaderName( IMaterialVar** params )
	{
		static char const* s_pPixelShaders[] = 
		{
			// Unmasked
			"LightmappedGeneric_EnvMapV2",
			"LightmappedGeneric_SelfIlluminatedEnvMapV2",

			"LightmappedGeneric_BaseAlphaMaskedEnvMapV2",
			"LightmappedGeneric_SelfIlluminatedEnvMapV2",

			// Env map mask
			"LightmappedGeneric_MaskedEnvMapV2",
			"LightmappedGeneric_SelfIlluminatedMaskedEnvMapV2",

			"LightmappedGeneric_MaskedEnvMapV2",
			"LightmappedGeneric_SelfIlluminatedMaskedEnvMapV2",
		};

		if (!params[BASETEXTURE]->IsDefined())
		{
			if (params[ENVMAP]->IsDefined())
			{
				if (!params[ENVMAPMASK]->IsDefined())
				{
					return "LightmappedGeneric_EnvmapNoTexture";
				}
				else
				{
					return "LightmappedGeneric_MaskedEnvmapNoTexture";
				}
			}
			else
			{
				return "LightmappedGeneric_NoTexture";
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
				return s_pPixelShaders[pshIndex];
			}
			else
			{
				if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM))
					return "LightmappedGeneric_SelfIlluminated";
				else
					return "LightmappedGeneric";
			}
		}
	}

	void DrawUnbumpedUsingVertexShader( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			// Alpha test
			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Base texture on stage 0
			if (params[BASETEXTURE]->IsDefined())
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );

			// Lightmap on stage 1
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			int fmt = VERTEX_POSITION;

			if (params[ENVMAP]->IsDefined())
			{
				fmt |= VERTEX_NORMAL;

				// envmap on stage 2
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );

				// envmapmask on stage 3
				if (params[ENVMAPMASK]->IsDefined() || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK ) )
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			}

			if (params[BASETEXTURE]->IsDefined())
				SetDefaultBlendingShadowState( BASETEXTURE, true );
			else
				SetDefaultBlendingShadowState( ENVMAPMASK, false );

			if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
				fmt |= VERTEX_COLOR;

			pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0, 0 );
			const char *vshName = GetVertexShaderName( params );
			pShaderShadow->SetVertexShader( vshName );
			const char *pshName = GetPixelShaderName( params );
			pShaderShadow->SetPixelShader( pshName );
		}
		DYNAMIC_STATE
		{
			if (params[BASETEXTURE]->IsDefined())
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			}

			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );

			if (params[ENVMAP]->IsDefined())
			{
				BindTexture( SHADER_TEXTURE_STAGE2, ENVMAP, ENVMAPFRAME );

				if (params[ENVMAPMASK]->IsDefined() || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
				{
					if (params[ENVMAPMASK]->IsDefined() )
						BindTexture( SHADER_TEXTURE_STAGE3, ENVMAPMASK, ENVMAPMASKFRAME );
					else
						BindTexture( SHADER_TEXTURE_STAGE3, BASETEXTURE, FRAME );
		
					SetVertexShaderTextureScaledTransform( 92, BASETEXTURETRANSFORM, ENVMAPMASKSCALE );
				}

				if (IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) ||
					IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE))
				{
					LoadViewMatrixIntoVertexShaderConstant( 17 );
				}
				SetEnvMapTintPixelShaderDynamicState( 2, ENVMAPTINT, -1 );
			}

			SetModulationVertexShaderDynamicState();
			EnablePixelShaderOverbright( 0, !IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT), true );
			SetPixelShaderConstant( 1, SELFILLUMTINT );
			DefaultFog();
		}
		Draw();
	}

	void DrawDetailNoEnvmap( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool doSelfIllum )
	{
		SHADOW_STATE
		{
			// Alpha test
			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Base texture on stage 0
			if (params[BASETEXTURE]->IsDefined())
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );

			// Lightmap on stage 1
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			// Detail on stage 2
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );

			int fmt = VERTEX_POSITION;

			SetDefaultBlendingShadowState( BASETEXTURE, true );

			if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
				fmt |= VERTEX_COLOR;

			pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0, 0 );

			if (!IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			{
				pShaderShadow->SetVertexShader("LightmappedGeneric_Detail");
			}
			else
			{
				pShaderShadow->SetVertexShader("LightmappedGeneric_DetailVertexColor");
			}

			if (!params[BASETEXTURE]->IsDefined())
			{
				pShaderShadow->SetPixelShader("LightmappedGeneric_DetailNoTexture");
			}
			else
			{
				if (!IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || (!doSelfIllum))
				{
					pShaderShadow->SetPixelShader("LightmappedGeneric_Detail");
				}
				else
				{
					pShaderShadow->SetPixelShader("LightmappedGeneric_DetailSelfIlluminated");
				}
			}

		}
		DYNAMIC_STATE
		{
			if (params[BASETEXTURE]->IsDefined())
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			}

			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );

			BindTexture( SHADER_TEXTURE_STAGE2, DETAIL, FRAME );
			SetVertexShaderTextureScaledTransform( 94, BASETEXTURETRANSFORM, DETAILSCALE );

			SetModulationVertexShaderDynamicState();
			EnablePixelShaderOverbright( 0, !IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT), true );

			if (doSelfIllum)
			{
				SetPixelShaderConstant( 1, SELFILLUMTINT );
			}

			DefaultFog();
		}
		Draw();
	}

	void DrawDetailWithEnvmap( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		VertexShaderUnlitGenericPass( 
			false, BASETEXTURE, FRAME, BASETEXTURETRANSFORM,
			DETAIL, DETAILSCALE, true, ENVMAP, ENVMAPFRAME, ENVMAPMASK,
			ENVMAPMASKFRAME, ENVMAPMASKSCALE, ENVMAPTINT );
	}

	void MultiplyByLighting( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			// Multiply the frame buffer by the lightmap
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_ZERO );

			// Alpha test
			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Base texture on stage 0
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, params[BASETEXTURE]->IsDefined() );

			// Lightmap on stage 1
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			// Disable other things
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, false );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, false );

			int fmt = VERTEX_POSITION;
			if (IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA))
				fmt |= VERTEX_COLOR;

			pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0, 0 );

			if (!IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA))
			{
				pShaderShadow->SetVertexShader( "LightmappedGeneric" );
			}
			else
			{
				pShaderShadow->SetVertexShader( "LightmappedGeneric_VertexColor" );
			}

			if (!IS_FLAG_SET(MATERIAL_VAR_SELFILLUM))
			{
				if (TextureIsTranslucent(BASETEXTURE, true))
				{
					pShaderShadow->SetPixelShader( "LightmappedGeneric_MultiplyByLighting" );
				}
				else
				{
					pShaderShadow->SetPixelShader( "LightmappedGeneric_MultiplyByLightingNoTexture" );
				}
			}
			else
			{
				pShaderShadow->SetPixelShader( "LightmappedGeneric_MultiplyByLightingSelfIllum" );
			}
		}
		DYNAMIC_STATE
		{
			if (params[BASETEXTURE]->IsDefined())
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			}

			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );

			SetModulationVertexShaderDynamicState();
			SetPixelShaderConstant( 1, SELFILLUMTINT );
			EnablePixelShaderOverbright( 0, !IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT), true );

			FogToWhite();
		}
		Draw();
	}

	void DrawDetailMode0( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Mode 0:
		// Pass 1 : B * D + E * M
		// Pass 2 : Multiply by lighting (L) (with Self Illum)
		DrawDetailWithEnvmap( params, pShaderAPI, pShaderShadow );
		MultiplyByLighting( params, pShaderAPI, pShaderShadow );
	}

	inline const char *GetAdditiveEnvmapPixelShaderName( bool usingMask, 
		bool usingBaseTexture, bool usingBaseAlphaEnvmapMask )
	{
		static char const* s_pPixelShaders[] = 
		{
			"UnlitGeneric_EnvMapNoTexture",
			"UnlitGeneric_EnvMapMaskNoTexture",
		};

		if (!usingMask && usingBaseTexture && usingBaseAlphaEnvmapMask )
		{
			return "UnlitGeneric_BaseAlphaMaskedEnvMap";
		}
		else
		{
			int pshIndex = 0;
			if (usingMask)
				pshIndex |= 0x1;
			return s_pPixelShaders[pshIndex];
		}
	}
	
	void DrawAdditiveEnvmap( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		bool usingBaseTexture = params[BASETEXTURE]->IsDefined();
		bool usingMask = params[ENVMAPMASK]->IsDefined();
		bool usingBaseAlphaEnvmapMask = IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK);
		SHADOW_STATE
		{
			// Alpha test
			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Base texture on stage 0
			if (params[BASETEXTURE]->IsDefined())
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );

			// envmap on stage 1
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			// envmapmask on stage 2
			if (params[ENVMAPMASK]->IsDefined() || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK ) )
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );

			if (params[BASETEXTURE]->IsDefined())
				SetAdditiveBlendingShadowState( BASETEXTURE, true );
			else
				SetAdditiveBlendingShadowState( ENVMAPMASK, false );

			int fmt = VERTEX_POSITION | VERTEX_NORMAL;

			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			// Re-use the unlit envmapped shaders; they're what we need...
			static char const* s_pVertexShaders[] = 
			{
				"UnlitGeneric_EnvMap",
				"UnlitGeneric_EnvMapVertexColor",
				"UnlitGeneric_EnvMapSphere",
				"UnlitGeneric_EnvMapSphereVertexColor",

				"UnlitGeneric_EnvMapCameraSpace",
				"UnlitGeneric_EnvMapCameraSpaceVertexColor",
				"UnlitGeneric_EnvMapSphere",
				"UnlitGeneric_EnvMapSphereVertexColor",
			};

			int vshIndex = 0;
			if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
				vshIndex |= 0x1;
			if (IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE))
				vshIndex |= 0x2;
			if (IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE))
				vshIndex |= 0x4;
			pShaderShadow->SetVertexShader( s_pVertexShaders[vshIndex] );

			const char *pshName = GetAdditiveEnvmapPixelShaderName( usingMask, 
				usingBaseTexture, usingBaseAlphaEnvmapMask );
			pShaderShadow->SetPixelShader( pshName );
		}
		DYNAMIC_STATE
		{
			if (usingBaseTexture)
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			}

			BindTexture( SHADER_TEXTURE_STAGE1, ENVMAP, ENVMAPFRAME );

			if (usingMask || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
			{
				if (usingMask)
					BindTexture( SHADER_TEXTURE_STAGE2, ENVMAPMASK, ENVMAPMASKFRAME );
				else
					BindTexture( SHADER_TEXTURE_STAGE2, BASETEXTURE, FRAME );

				SetVertexShaderTextureScaledTransform( 92, BASETEXTURETRANSFORM, ENVMAPMASKSCALE );
			}

			SetPixelShaderConstant( 2, ENVMAPTINT );

			if (IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) || IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE))
			{
				LoadViewMatrixIntoVertexShaderConstant( 17 );
			}

			SetModulationVertexShaderDynamicState();
			FogToBlack();
		}
		Draw();
	}

	void DrawDetailMode1( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Mode 1 :
		// Pass 1 : B * L * D + Self Illum
		// Pass 2 : Add E * M

		// Draw the detail w/ no envmap
		DrawDetailNoEnvmap( params, pShaderAPI, pShaderShadow, true );
		DrawAdditiveEnvmap( params, pShaderAPI, pShaderShadow );
	}

	void DrawDetailUsingVertexShader( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// We don't have enough textures; gotta do this in two passes if there's envmapping
		if (!params[ENVMAP]->IsDefined())
		{
			DrawDetailNoEnvmap( params, pShaderAPI, pShaderShadow, true );
		}
		else
		{
			if (!params[BASETEXTURE]->IsDefined())
			{
				// If there's an envmap but no base texture, ignore detail
				DrawUnbumpedUsingVertexShader( params, pShaderAPI, pShaderShadow );
			}
			else
			{
				DrawDetailMode1( params, pShaderAPI, pShaderShadow );
			}
		}
	}

	SHADER_DRAW
	{
		if( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() )
		{
			DrawWorldBumpedUsingVertexShader( BASETEXTURE, BASETEXTURETRANSFORM,
				BUMPMAP, BUMPFRAME, BUMPTRANSFORM, ENVMAPMASK, ENVMAPMASKFRAME, ENVMAP, 
				ENVMAPFRAME, ENVMAPTINT, ALPHA,	ENVMAPCONTRAST, ENVMAPSATURATION, FRAME, FRESNELREFLECTION );
		}
		else
		{
			if (params[DETAIL]->IsDefined())
				DrawDetailUsingVertexShader( params, pShaderAPI, pShaderShadow );
			else
				DrawUnbumpedUsingVertexShader( params, pShaderAPI, pShaderShadow );
		}
	}
END_SHADER
