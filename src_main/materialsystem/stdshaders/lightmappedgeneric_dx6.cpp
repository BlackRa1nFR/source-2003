//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

DEFINE_FALLBACK_SHADER( LightmappedGeneric, LightmappedGeneric_DX6 )
 
BEGIN_SHADER( LightmappedGeneric_DX6,
			  "Help for LightmappedGeneric_DX6" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTimesLightmapPlusMaskedCubicEnvMap_glass", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if( params[BASETEXTURE]->IsDefined() )
		{
			if( !IS_FLAG_SET(MATERIAL_VAR_MULTIPASS) )
			{
				params[ENVMAP]->SetUndefined();
				params[ENVMAPMASK]->SetUndefined();
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
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM))
					CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
				if (IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
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
			{
				LoadCubeMap( ENVMAP );
			}
			else
			{
				LoadTexture( ENVMAP );
			}

			if( !g_pHardwareConfig->SupportsCubeMaps() )
			{
				SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
			}

			if (params[ENVMAPMASK]->IsDefined())
			{
				LoadTexture( ENVMAPMASK );
			}
		}
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	int GetDrawFlagsPass1(IMaterialVar** params)
	{
		int flags = SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			flags |= SHADER_DRAW_COLOR;
		if (params[BASETEXTURE]->IsDefined())
			flags |= SHADER_DRAW_TEXCOORD1;
		return flags;
	}

	void DrawLightmapOnly( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, g_pConfig->overbright );

			SetModulationShadowState();
			SetDefaultBlendingShadowState( );
			pShaderShadow->DrawFlags( GetDrawFlagsPass1( params ) );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
			SetModulationDynamicState();
			DefaultFog();
		}
		Draw();
	}

	void DrawBaseTimesLightmap( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Base times lightmap
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Alpha blending
			SetDefaultBlendingShadowState( BASETEXTURE, true );

			// Independenly configure alpha and color
			pShaderShadow->EnableAlphaPipe( true );

			// color channel
			
			// lightmap
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );

			// base
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			if (!IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT))
				pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, g_pConfig->overbright );

			pShaderShadow->EnableConstantColor( IsColorModulating() );

			// alpha channel
			pShaderShadow->EnableConstantAlpha( IsAlphaModulating() );
			pShaderShadow->EnableVertexAlpha( IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA) );
			pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE0, false );
			pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE1, TextureIsTranslucent(BASETEXTURE, true) );

			pShaderShadow->DrawFlags( GetDrawFlagsPass1( params ) );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE1, BASETEXTURETRANSFORM );
			BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE, FRAME );
			SetModulationDynamicState();
			DefaultFog();
		}
		Draw();

		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaPipe( false );
		}
	}

	void DrawMode1( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Pass 1 : Base * lightmap or just lightmap
		if ( params[BASETEXTURE]->IsDefined() )
		{
			DrawBaseTimesLightmap( params, pShaderAPI, pShaderShadow );
			FixedFunctionMultiplyByDetailPass(
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );

			// Draw the selfillum pass
			if ( IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) )
			{
				FixedFunctionSelfIlluminationPass( 
					SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME, BASETEXTURETRANSFORM, SELFILLUMTINT );
			}
		}
		else
		{
			DrawLightmapOnly( params, pShaderAPI, pShaderShadow );
			FixedFunctionMultiplyByDetailPass(
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );
		}

		// Pass 2 : Masked environment map
		if (params[ENVMAP]->IsDefined())
		{
			FixedFunctionAdditiveMaskedEnvmapPass( 
				ENVMAP, ENVMAPMASK, BASETEXTURE,
				ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
				BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
		}
	}

	SHADER_DRAW
	{
		// Base * Lightmap + env
		DrawMode1( params, pShaderAPI, pShaderShadow );
	}
END_SHADER
