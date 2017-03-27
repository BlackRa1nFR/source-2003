//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

#include "lightmappedgeneric_ps20.inc"
#include "lightmappedgeneric_vs20.inc"

BEGIN_VS_SHADER( LightmappedGeneric,
			  "Help for LightmappedGeneric" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $detail" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$envmapmask texcoord transform" )
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

		if( !params[DETAILFRAME]->IsDefined() )
			params[DETAILFRAME]->SetIntValue( 0 );

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

		if( params[BUMPMAP]->IsDefined() )
		{
			params[ENVMAPMASK]->SetUndefined();
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
		if( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "LightmappedGeneric_DX8";
		}
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
		{
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
		}
			
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
		else
		{
			params[ENVMAPMASK]->SetUndefined();
		}

		if( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			// Alpha test: FIXME: shouldn't this be handled in Shader_t::SetInitialShadowState
			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );
			if (params[BASETEXTURE]->IsDefined())
				SetDefaultBlendingShadowState( BASETEXTURE, true );
			else
				SetDefaultBlendingShadowState( ENVMAPMASK, false );
			
			bool hasBaseTexture = params[BASETEXTURE]->IsDefined();
			bool hasDetailTexture = params[DETAIL]->IsDefined();
			bool hasBump = params[BUMPMAP]->IsDefined();
			bool hasEnvmap = params[ENVMAP]->IsDefined();
			bool hasEnvmapMask = params[ENVMAPMASK]->IsDefined();
			bool hasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) != 0;

			unsigned int flags = VERTEX_POSITION;
			if( hasBaseTexture )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			}
//			if( hasLightmap )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			}
			if( hasEnvmap )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
				flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T | VERTEX_NORMAL;
			}
			if( hasDetailTexture )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			}
			if( hasBump )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
			}
			if( hasEnvmapMask )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE5, true );
			}
			if( hasVertexColor )
			{
				flags |= VERTEX_COLOR;
			}

			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE6, true );

			// texcoord0 : base texcoord
			// texcoord1 : lightmap texcoord
			// texcoord2 : lightmap texcoord offset
			int numTexCoords = 0;
			if( hasBump )
			{
				numTexCoords = 3;
			}
			else 
			{
				numTexCoords = 2;
			}
			
			pShaderShadow->VertexShaderVertexFormat( 
				flags, numTexCoords, 0, 0, 0 );

			// Pre-cache pixel shaders
			bool hasBaseAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			bool hasSelfIllum = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );
			bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );

			lightmappedgeneric_ps20_Index pshIndex;
			pshIndex.SetBASETEXTURE( hasBaseTexture );
			pshIndex.SetDETAILTEXTURE( hasDetailTexture );
			pshIndex.SetBUMPMAP( hasBump );
			pshIndex.SetCUBEMAP( hasEnvmap );
			pshIndex.SetVERTEXCOLOR( hasVertexColor );
			pshIndex.SetENVMAPMASK( hasEnvmapMask );
			pshIndex.SetBASEALPHAENVMAPMASK( hasBaseAlphaEnvmapMask );
			pshIndex.SetSELFILLUM( hasSelfIllum );
			pshIndex.SetNORMALMAPALPHAENVMAPMASK( hasNormalMapAlphaEnvmapMask );
			pshIndex.SetAACLAMP( false );

			pShaderShadow->SetPixelShader( "lightmappedgeneric_ps20", pshIndex.GetIndex() );
			pShaderShadow->SetVertexShader( "lightmappedgeneric_vs20" );
		}
		DYNAMIC_STATE
		{
			bool hasBaseTexture = params[BASETEXTURE]->IsDefined();
			bool hasDetailTexture = params[DETAIL]->IsDefined();
			bool hasBump = params[BUMPMAP]->IsDefined();
			bool hasEnvmap = params[ENVMAP]->IsDefined();
			bool hasEnvmapMask = params[ENVMAPMASK]->IsDefined();

			pShaderAPI->SetDefaultState();
			if( hasBaseTexture )
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			}
//			if( hasLightmap )
			{
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
			}
			if( hasEnvmap )
			{
				BindTexture( SHADER_TEXTURE_STAGE2, ENVMAP, ENVMAPFRAME );
			}
			if( hasDetailTexture )
			{
				BindTexture( SHADER_TEXTURE_STAGE3, DETAIL, DETAILFRAME );
			}
			if( hasBump )
			{
				if( !g_pConfig->m_bFastNoBump )
				{
					BindTexture( SHADER_TEXTURE_STAGE4, BUMPMAP, BUMPFRAME );
				}
				else
				{
					pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE4 );
				}
			}
			if( hasEnvmapMask )
			{
				BindTexture( SHADER_TEXTURE_STAGE5, ENVMAPMASK, ENVMAPMASKFRAME );
			}

			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE6 );

			SetModulationVertexShaderDynamicState();
			MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();

			lightmappedgeneric_vs20_Index vshIndex;
			vshIndex.SetFOG_TYPE( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			vshIndex.SetENVMAP_MASK( hasEnvmapMask );
			vshIndex.SetTANGENTSPACE( params[ENVMAP]->IsDefined() );
			vshIndex.SetBUMPMAP( hasBump );
			vshIndex.SetVERTEXCOLOR( IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) != 0 );
			vshIndex.SetAACLAMP( false );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

			SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			if( hasDetailTexture )
			{
				SetVertexShaderTextureScaledTransform( 92, BASETEXTURETRANSFORM, DETAILSCALE );
				Assert( !hasBump );
			}
			if( hasBump )
			{
				SetVertexShaderTextureTransform( 92, BUMPTRANSFORM );
				Assert( !hasDetailTexture );
			}
			if( hasEnvmapMask )
			{
				SetVertexShaderTextureTransform( 94, ENVMAPMASKTRANSFORM );
			}
				
			SetEnvMapTintPixelShaderDynamicState( 0, ENVMAPTINT, -1 );
			SetModulationPixelShaderDynamicState( 1 );
			SetPixelShaderConstant( 2, ENVMAPCONTRAST );
			SetPixelShaderConstant( 3, ENVMAPSATURATION );
			SetPixelShaderConstant( 7, SELFILLUMTINT );

			// [ 0, 0 ,0, R(0) ]
			float fresnel[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			fresnel[3] = params[FRESNELREFLECTION]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( 4, fresnel );

			// [ 0, 0 ,0, 1-R(0) ]
			fresnel[3] = 1.0f - fresnel[3];
			pShaderAPI->SetPixelShaderConstant( 5, fresnel );

			float overbright[4];
			overbright[0] = overbright[1] = overbright[2] = overbright[3] = g_pConfig->overbright;
			pShaderAPI->SetPixelShaderConstant( 6, overbright );
			
			FogToFogColor();
		}
		Draw();
	}
END_SHADER
