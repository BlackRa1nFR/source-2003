//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "ConVar.h"

#include "lightmappedgeneric_hdr_vs20.inc"
#include "lightmappedgeneric_hdr_ps20.inc"

ConVar building_cubemaps( "building_cubemaps", "0" );

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

		SHADER_PARAM( BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture2", "base texture2 help" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $baseTexture" )
		SHADER_PARAM( BASETEXTURETRANSFORM2, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$baseTexture texcoord transform" )
		SHADER_PARAM( BUMPBASETEXTURE2WITHBUMPMAP, SHADER_PARAM_TYPE_BOOL, "0", "" )
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
		// hack
/*
		params[BUMPMAP]->SetUndefined();
		params[ENVMAP]->SetUndefined();
		params[ENVMAPMASK]->SetUndefined();
		params[BUMPBASETEXTURE2WITHBUMPMAP]->SetUndefined();
		CLEAR_FLAGS( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
*/
	
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

		if( !params[BUMPBASETEXTURE2WITHBUMPMAP]->IsDefined() )
		{
			params[BUMPBASETEXTURE2WITHBUMPMAP]->SetIntValue( 0 );
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

		if (params[BASETEXTURE2]->IsDefined())
		{
			LoadTexture( BASETEXTURE2 );
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
		else
		{
			params[ENVMAPMASK]->SetUndefined();
		}
		if( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		}
	}

	inline void DrawPass( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI,
								BlendType_t blendType,
								bool alphaTest,
								int pass )
	{
		bool bNeedsAlpha = false;
		if( alphaTest )
		{
			// Shader needs alpha for alpha testing
			bNeedsAlpha = true;
		}
		else
		{
			// When blending shader needs alpha on the first pass
			if (((blendType == BT_BLEND) || (blendType == BT_BLENDADD)) && (pass == 0))
				bNeedsAlpha = true;
		}

		// Check if rendering alpha (second pass of blending)
		bool bRenderAlphaOnly;
		if (((blendType == BT_BLEND) || (blendType == BT_BLENDADD)) && (pass == 1))
			bRenderAlphaOnly = true;
		else
			bRenderAlphaOnly = false;

		bool hasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) != 0;

		SHADOW_STATE
		{
			SetInitialShadowState();

			pShaderShadow->EnableSRGBWrite( true );

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

			bool bBlendTwoBase = params[BASETEXTURE2]->IsDefined();
			bool bBlendSecondBaseAfter = params[BUMPBASETEXTURE2WITHBUMPMAP]->GetIntValue() == 0 ? false : true;
			if( bBlendSecondBaseAfter )
			{
				bBlendTwoBase = false;
			}

			// Alpha output
			if( !bNeedsAlpha || ( blendType == BT_BLEND ) )
				pShaderShadow->EnableAlphaWrites( true );

			// On second pass we just adjust alpha portion, so don't need color
			if( pass == 1 )
				pShaderShadow->EnableColorWrites( false );

			// For blending-type modes we need to setup separate alpha blending
			if( blendType == BT_BLEND )
			{
				pShaderShadow->EnableBlendingSeparateAlpha( true );

				if( pass == 0)
					pShaderShadow->BlendFuncSeparateAlpha( SHADER_BLEND_ZERO, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				else
					pShaderShadow->BlendFuncSeparateAlpha( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			}
			else if( blendType == BT_BLENDADD )
			{
				if( pass == 1)
				{
					pShaderShadow->EnableBlendingSeparateAlpha( true );
					pShaderShadow->BlendFuncSeparateAlpha( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
				}
			}

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
			if( params[BASETEXTURE2]->IsDefined() )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE6, true );
			}
			if( hasVertexColor )
			{
				flags |= VERTEX_COLOR;
			}

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
			
			pShaderShadow->SetVertexShader( "lightmappedgeneric_hdr_vs20" );
			
			lightmappedgeneric_hdr_ps20_Index pshIndex;
			pshIndex.SetBASETEXTURE( hasBaseTexture );
			pshIndex.SetDETAILTEXTURE( hasDetailTexture );
			pshIndex.SetBUMPMAP( hasBump );
			pshIndex.SetCUBEMAP( hasEnvmap );
			pshIndex.SetVERTEXCOLOR( hasVertexColor );
			pshIndex.SetENVMAPMASK( hasEnvmapMask );
			pshIndex.SetBASEALPHAENVMAPMASK( hasBaseAlphaEnvmapMask );
			pshIndex.SetSELFILLUM( hasSelfIllum );
			pshIndex.SetNORMALMAPALPHAENVMAPMASK( hasNormalMapAlphaEnvmapMask );
			pshIndex.SetNEEDSALPHA( bNeedsAlpha );
			pshIndex.SetRENDERALPHAONLY( bRenderAlphaOnly );
			pshIndex.SetBLENDTWOBASE( bBlendTwoBase );
			pshIndex.SetBLENDSECONDBASEAFTER( bBlendSecondBaseAfter );
			pShaderShadow->SetPixelShader( "lightmappedgeneric_hdr_ps20", pshIndex.GetIndex() );
		}
		DYNAMIC_STATE
		{
			bool hasBaseTexture = params[BASETEXTURE]->IsDefined();
			bool hasDetailTexture = params[DETAIL]->IsDefined();
			bool hasBump = params[BUMPMAP]->IsDefined();
			bool hasEnvmap = params[ENVMAP]->IsDefined();
//			bool hasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) != 0;
			bool hasEnvmapMask = params[ENVMAPMASK]->IsDefined();
//			bool hasBaseAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK );
//			bool hasSelfIllum = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );
//			bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );

			bool bBlendTwoBase = params[BASETEXTURE2]->IsDefined();
			bool bBlendSecondBaseAfter = params[BUMPBASETEXTURE2WITHBUMPMAP]->GetIntValue() == 0 ? false : true;
			if( bBlendSecondBaseAfter )
			{
				bBlendTwoBase = false;
			}

			pShaderAPI->SetDefaultState();
			if( hasBaseTexture )
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				pShaderAPI->EnableSRGBRead( SHADER_TEXTURE_STAGE0, true );
			}
//			if( hasLightmap )
			{
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
				pShaderAPI->EnableSRGBRead( SHADER_TEXTURE_STAGE1, true );
			}
			if( hasEnvmap )
			{
				BindTexture( SHADER_TEXTURE_STAGE2, ENVMAP, ENVMAPFRAME );
				pShaderAPI->EnableSRGBRead( SHADER_TEXTURE_STAGE2, true );
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
			if( params[BASETEXTURE2]->IsDefined() )
			{
				BindTexture( SHADER_TEXTURE_STAGE6, BASETEXTURE2, FRAME2 );
				pShaderAPI->EnableSRGBRead( SHADER_TEXTURE_STAGE6, true );
			}

			SetModulationVertexShaderDynamicState();

			SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			// GR - for second bas in WorldVertexTransition
			SetVertexShaderTextureTransform( 96, BASETEXTURETRANSFORM2 );
			
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

			MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
			bool bTangentSpace = params[ENVMAP]->IsDefined();

			lightmappedgeneric_hdr_vs20_Index vshIndex;
			vshIndex.SetFOG_TYPE( fogType );
			vshIndex.SetENVMAP_MASK( hasEnvmapMask );
			vshIndex.SetTANGENTSPACE( bTangentSpace );
			vshIndex.SetBUMPMAP( hasBump );
			vshIndex.SetVERTEXCOLOR( hasVertexColor );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

//			SetEnvMapTintPixelShaderDynamicState( 0, ENVMAPTINT, -1 );
			SetModulationPixelShaderDynamicState( 1 );
			SetPixelShaderConstant( 2, ENVMAPCONTRAST );
			SetPixelShaderConstant( 3, ENVMAPSATURATION );
			SetPixelShaderConstant( 7, SELFILLUMTINT );

			float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			IMaterialVar* pTintVar = s_ppParams[ENVMAPTINT];
			if( g_pConfig->bShowSpecular )
			{
				pTintVar->GetVecValue( color, 3 );
				color[3] = (color[0] + color[1] + color[2]) / 3.0f;
			}
			pShaderAPI->SetPixelShaderConstant( 0, color, 1 );


			// [ 1-R(0), R(0), 0 ,0 ]
			float fresnel[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			fresnel[1] = fresnel[3]= params[FRESNELREFLECTION]->GetFloatValue();
			fresnel[0] = 1.0f - fresnel[1];
			pShaderAPI->SetPixelShaderConstant( 4, fresnel );

			fresnel[3] = 1.0f - fresnel[3];
			pShaderAPI->SetPixelShaderConstant( 5, fresnel );

			float overbright[4];
			overbright[0] = overbright[1] = overbright[2] = overbright[3] = g_pConfig->overbright;
			pShaderAPI->SetPixelShaderConstant( 6, overbright );

#if 1
			bool bBlendableOutput = !building_cubemaps.GetBool();
			lightmappedgeneric_hdr_ps20_Index pshIndex;
			pshIndex.SetBLENDOUTPUT( bBlendableOutput );
			pShaderAPI->SetPixelShaderIndex( pshIndex.GetIndex() );
#endif

			FogToFogColor();
		}
		Draw();
	}

	SHADER_DRAW
	{
		bool alphaTest = IS_FLAG_SET(MATERIAL_VAR_ALPHATEST);
		BlendType_t blendType;

		if (params[BASETEXTURE]->IsDefined())
			blendType = EvaluateBlendRequirements( BASETEXTURE, true );
		else
			blendType = EvaluateBlendRequirements( ENVMAPMASK, false );

		DrawPass( params, pShaderShadow, pShaderAPI, blendType, alphaTest, 0 );
		if(!alphaTest && ((blendType == BT_BLEND) || (blendType == BT_BLENDADD)))
		{
			// Second pass to complete 
			DrawPass( params, pShaderShadow, pShaderAPI, blendType, alphaTest, 1 );
		}
	}
END_SHADER
