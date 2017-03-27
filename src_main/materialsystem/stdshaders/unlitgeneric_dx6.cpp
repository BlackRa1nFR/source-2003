//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

DEFINE_FALLBACK_SHADER( UnlitGeneric, UnlitGeneric_DX6 )

BEGIN_SHADER( UnlitGeneric_DX6,
			  "Help for UnlitGeneric_DX6" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTimesLightmapPlusMaskedCubicEnvMap_glass", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		// The only thing we can't do here is masked env-mapped
		if ((g_pHardwareConfig->GetNumTextureUnits() < 2) && params[ENVMAPMASK]->IsDefined() )
		{
			return "UnlitGeneric_DX5";
		}
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		if( !params[ENVMAPMASKSCALE]->IsDefined() )
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );
		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 4.0f );

		// No texture means no env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				if (IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
					CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		// the second texture (if there is one)
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
				SET_FLAGS(MATERIAL_VAR_ENVMAPSPHERE);

			if (params[ENVMAPMASK]->IsDefined())
			{
				LoadTexture( ENVMAPMASK );
			}
		}
	}

	int GetDrawFlagsPass1(IMaterialVar** params, bool doDetail)
	{
		int flags = SHADER_DRAW_POSITION;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			flags |= SHADER_DRAW_COLOR;
		if (params[BASETEXTURE]->IsDefined())
			flags |= SHADER_DRAW_TEXCOORD0;
		if (doDetail)
			flags |= SHADER_DRAW_TEXCOORD1;
		return flags;
	}

	void SetDetailShadowState(IShaderShadow* pShaderShadow)
	{
		// Specifically choose overbright2, will cause mod2x here
		pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, 2.0f );
	}

	void SetDetailDymamicState(IShaderShadow* pShaderShadow)
	{
		BindTexture( SHADER_TEXTURE_STAGE1, DETAIL, FRAME );
		SetFixedFunctionTextureScaledTransform( MATERIAL_TEXTURE1, BASETEXTURETRANSFORM, DETAILSCALE );
	}

	void DrawAdditiveNonTextured( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool doDetail )
	{
		SHADOW_STATE
		{
			SetModulationShadowState();
			SetAdditiveBlendingShadowState( );
			if (doDetail)
				SetDetailShadowState(pShaderShadow);
			pShaderShadow->DrawFlags( GetDrawFlagsPass1(params, doDetail) );
		}
		DYNAMIC_STATE
		{
			SetModulationDynamicState();
			if (doDetail)
				SetDetailDymamicState(pShaderShadow);
			FogToBlack();
		}
		Draw( );
	}

	void DrawAdditiveTextured( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool doDetail )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			SetModulationShadowState();
			SetAdditiveBlendingShadowState( BASETEXTURE, true );
			if (doDetail)
				SetDetailShadowState(pShaderShadow);
			pShaderShadow->DrawFlags( GetDrawFlagsPass1(params, doDetail) );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			if (doDetail)
				SetDetailDymamicState(pShaderShadow);
			SetModulationDynamicState();
			FogToBlack();
		}
		Draw( );
	}

	void DrawNonTextured( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool doDetail )
	{
		SHADOW_STATE
		{
			SetModulationShadowState();
			SetNormalBlendingShadowState( );
			if (doDetail)
				SetDetailShadowState(pShaderShadow);
			pShaderShadow->DrawFlags( GetDrawFlagsPass1(params, doDetail) );
		}
		DYNAMIC_STATE
		{
			SetModulationDynamicState();
			if (doDetail)
				SetDetailDymamicState(pShaderShadow);
			FogToFogColor();
		}
		Draw( );
	}

	void DrawTextured( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool doDetail )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			SetModulationShadowState();
			SetNormalBlendingShadowState( BASETEXTURE, true );
			if (doDetail)
				SetDetailShadowState(pShaderShadow);
			pShaderShadow->DrawFlags( GetDrawFlagsPass1(params, doDetail) );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			if (doDetail)
				SetDetailDymamicState(pShaderShadow);
			SetModulationDynamicState();
			FogToFogColor();
		}
		Draw( );
	}

	SHADER_DRAW
	{
		bool isTextureDefined = params[BASETEXTURE]->IsDefined();
		bool hasVertexColor = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
		bool doFirstPass = isTextureDefined || hasVertexColor || (!params[ENVMAP]->IsDefined());

		if (doFirstPass)
		{
			SHADOW_STATE
			{
 				pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );
			}

			if (IS_FLAG_SET(MATERIAL_VAR_ADDITIVE))
			{
				if (!isTextureDefined)
				{
					bool hasDetailTexture = params[DETAIL]->IsDefined();
					DrawAdditiveNonTextured( params, pShaderAPI, pShaderShadow, hasDetailTexture );
				}
				else
				{
					// We can't do detail in a single pass if we're also
					// colormodulating and have vertex color
					bool hasDetailTexture = params[DETAIL]->IsDefined();
					bool isModulating = IsColorModulating() || IsAlphaModulating(); 
					bool onePassDetail = hasDetailTexture && (!hasVertexColor || !isModulating);
					DrawAdditiveTextured( params, pShaderAPI, pShaderShadow, onePassDetail );
					if (hasDetailTexture && !onePassDetail)
					{
						FixedFunctionMultiplyByDetailPass(
							BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );
					}
				}
			}
			else
			{
				if (!isTextureDefined)
				{
					bool hasDetailTexture = params[DETAIL]->IsDefined();
					DrawNonTextured( params, pShaderAPI, pShaderShadow, hasDetailTexture );
				}
				else
				{
					// We can't do detail in a single pass if we're also
					// colormodulating and have vertex color
					bool hasDetailTexture = params[DETAIL]->IsDefined();
					bool isModulating = IsColorModulating() || IsAlphaModulating(); 
					bool onePassDetail = hasDetailTexture && (!hasVertexColor || !isModulating);
					DrawTextured( params, pShaderAPI, pShaderShadow, onePassDetail );
					if (hasDetailTexture && !onePassDetail)
					{
						FixedFunctionMultiplyByDetailPass(
							BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );
					}
				}
			}
		}

		SHADOW_STATE
		{
			// Disable mod2x used by detail
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, 1.0f );
		}

		// Second pass...
		if (params[ENVMAP]->IsDefined() && 
			(!doFirstPass || IS_FLAG_SET(MATERIAL_VAR_MULTIPASS)) )
		{
			if (doFirstPass || IS_FLAG_SET(MATERIAL_VAR_ADDITIVE))
			{
				FixedFunctionAdditiveMaskedEnvmapPass( ENVMAP, ENVMAPMASK, BASETEXTURE,
					ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
					BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
			}
			else
			{
				FixedFunctionMaskedEnvmapPass( ENVMAP, ENVMAPMASK, BASETEXTURE,
					ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
					BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
			}
		}
	}
END_SHADER
