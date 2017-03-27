//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( ShatteredGlass,
			  "Help for ShatteredGlass" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/Detail", "detail" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "detail scale" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTimesLightmapPlusMaskedCubicEnvMap_glass", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$envmapmask texcoord transform" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( FRESNELREFLECTION, SHADER_PARAM_TYPE_FLOAT, "0.0", "0.0 == no fresnel, 1.0 == full fresnel" )
		SHADER_PARAM( UNLITFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.7", "0.0 == multiply by lightmap, 1.0 == multiply by 1" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 1.0f );

		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[ENVMAPCONTRAST]->IsDefined() )
			params[ENVMAPCONTRAST]->SetFloatValue( 0.0f );
		
		if( !params[ENVMAPSATURATION]->IsDefined() )
			params[ENVMAPSATURATION]->SetFloatValue( 1.0f );
		
		if( !params[UNLITFACTOR]->IsDefined() )
			params[UNLITFACTOR]->SetFloatValue( 0.3f );

		// No texture means no self-illum or env mask in base alpha
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

	SHADER_FALLBACK
	{
        // MMW Shattered Glass runs as a DX9 effect for 8.2 hardware
        bool isDX9 = (g_pHardwareConfig->GetDXSupportLevel() >= 82);
		if( !isDX9 )
		{
			return "ShatteredGlass_DX7";
		}
		return 0;
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if ( !params[BASETEXTURE]->GetTextureValue()->IsTranslucent() )
			{
				if ( IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK ) )
					CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		if ( params[DETAIL]->IsDefined() )
		{					 
			LoadTexture( DETAIL );
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if ( IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
			
		if (params[ENVMAP]->IsDefined())
		{
			LoadCubeMap( ENVMAP );
			if ( params[ENVMAPMASK]->IsDefined() )
			{
				LoadTexture( ENVMAPMASK );
			}
		}
	}

	inline int ComputePixelShaderIndex( 
		bool bHasEnvmap, bool bHasVertexColor, bool bHasEnvmapMask,
		bool bHasBaseAlphaEnvmapMask )
	{
		int pshIndex = 0;
		if( bHasEnvmap )				pshIndex |= ( 1 << 0 );
		if( bHasVertexColor )			pshIndex |= ( 1 << 1 );
		if( bHasEnvmapMask )			pshIndex |= ( 1 << 2 );
		if( bHasBaseAlphaEnvmapMask )	pshIndex |= ( 1 << 3 );
		return pshIndex;
	}
	
	SHADER_DRAW
	{
		bool bHasEnvmapMask = false;
		bool bHasEnvmap = false;
		if ( params[ENVMAP]->IsDefined() )
		{
			bHasEnvmap = true;
			if ( params[ENVMAPMASK]->IsDefined() )
			{
				bHasEnvmapMask = true;
			}
		}
		bool bHasVertexColor = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
		bool bHasBaseAlphaEnvmapMask = IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK);
	
		// Base
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Alpha blending, enable alpha blending if the detail texture is translucent
			bool detailIsTranslucent = TextureIsTranslucent( DETAIL, false );
			if ( detailIsTranslucent )
			{
				if ( IS_FLAG_SET( MATERIAL_VAR_ADDITIVE ) )
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				else
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			else
			{
				SetDefaultBlendingShadowState( BASETEXTURE, true );
			}

			// Base texture
			unsigned int flags = VERTEX_POSITION;
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );

			// Lightmap
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			// Detail texture
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );

			// Envmap
			if ( bHasEnvmap )
			{
				flags |= VERTEX_NORMAL;
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			
				if( bHasEnvmapMask )
				{
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE5, true );
				}
			}

			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE6, true );

			if ( bHasVertexColor )
			{
				flags |= VERTEX_COLOR;
			}

			pShaderShadow->VertexShaderVertexFormat( flags, 3, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "shatteredglass_vs20" );

//			"CUBEMAP"				"0..1"
//			"VERTEXCOLOR"			"0..1"
//			"ENVMAPMASK"			"0..1"
//			"BASEALPHAENVMAPMASK"	"0..1"
			int pshIndex = ComputePixelShaderIndex( bHasEnvmap, bHasVertexColor, 
				bHasEnvmapMask, bHasBaseAlphaEnvmapMask );
			pShaderShadow->SetPixelShader( "shatteredglass_ps20", pshIndex );
		}
		DYNAMIC_STATE
		{
			SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			SetVertexShaderTextureScale( 92, DETAILSCALE );

			if ( params[ENVMAP]->IsDefined() && params[ENVMAPMASK]->IsDefined() )
			{
				SetVertexShaderTextureTransform( 94, ENVMAPMASKTRANSFORM );
			}

			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
			BindTexture( SHADER_TEXTURE_STAGE3, DETAIL );

			if( bHasEnvmap )
			{
				BindTexture( SHADER_TEXTURE_STAGE2, ENVMAP, ENVMAPFRAME );
			}
			if( bHasEnvmapMask )
			{
				BindTexture( SHADER_TEXTURE_STAGE5, ENVMAPMASK, ENVMAPMASKFRAME );
			}

			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE6 );

			/*
			"FOG_TYPE"				"0..1"
			"ENVMAP_MASK"			"0..1"
			*/
			MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();

			int vshIndex = 0;
			if( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) 
			{
				vshIndex |= ( 1 << 0 );
			}
			if( bHasEnvmapMask )
			{
				vshIndex |= ( 1 << 1 );
			}

			pShaderAPI->SetVertexShaderIndex( vshIndex );

			SetEnvMapTintPixelShaderDynamicState( 0, ENVMAPTINT, -1 );
			SetModulationPixelShaderDynamicState( 1 );
			SetPixelShaderConstant( 2, ENVMAPCONTRAST );
			SetPixelShaderConstant( 3, ENVMAPSATURATION );

			// [ 0, 0 ,0, R(0) ]
			float fresnel[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			fresnel[3] = params[FRESNELREFLECTION]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( 4, fresnel );

			// [ 0, 0 ,0, 1-R(0) ]
			fresnel[3] = 1.0f - fresnel[3];
			pShaderAPI->SetPixelShaderConstant( 5, fresnel );

			float overbright[4];
			overbright[0] = g_pConfig->overbright;
			overbright[1] = overbright[2] = overbright[3] = params[UNLITFACTOR]->GetFloatValue();
			pShaderAPI->SetPixelShaderConstant( 6, overbright );

			SetModulationDynamicState();
			DefaultFog();
		}
		Draw();
	}
END_SHADER
