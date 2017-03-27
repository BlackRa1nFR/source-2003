//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

DEFINE_FALLBACK_SHADER( VertexLitGeneric, VertexLitGeneric_DX7 )

BEGIN_SHADER( VertexLitGeneric_DX7, 
			  "Help for VertexLitGeneric_DX7" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTimesLightmapPlusMaskedCubicEnvMap_glass", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// default to 'MODEL' mode...
		if (!IS_FLAG_DEFINED( MATERIAL_VAR_MODEL ))
			SET_FLAGS( MATERIAL_VAR_MODEL );

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

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
	}

	SHADER_FALLBACK
	{
		if (g_pConfig->bSoftwareLighting)
			return "VertexLitGeneric_DX6";

		if (!g_pHardwareConfig->SupportsHardwareLighting())
			return "VertexLitGeneric_DX6";
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

	int GetDrawFlagsPass1(IMaterialVar** params)
	{
		int flags = SHADER_DRAW_POSITION;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			flags |= SHADER_DRAW_COLOR;
		if (params[BASETEXTURE]->IsDefined())
			flags |= SHADER_DRAW_TEXCOORD1;
		return flags;
	}

	void DrawVertexLightingOnly( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableAmbientLightCubeOnStage0( true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, false );
			pShaderShadow->EnableLighting( true );

			SetModulationShadowState();
			SetDefaultBlendingShadowState( );

			pShaderShadow->DrawFlags( GetDrawFlagsPass1( params ) );
		}
		DYNAMIC_STATE
		{
			SetAmbientCubeDynamicStateFixedFunction( );
			SetModulationDynamicState();
			DefaultFog();
		}
		Draw();
	}
	 
	void MultiplyByVertexLighting( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{				
			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, false );
			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE1, false );

			pShaderShadow->EnableAmbientLightCubeOnStage0( true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, false );
			pShaderShadow->EnableLighting( true );

			// NOTE: We're not doing lightmapping here, but we want to use the
			// same blend mode as we used for lightmapping
			pShaderShadow->EnableBlending( true );
			SingleTextureLightmapBlendMode();
			
			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 2 );

			// This here will perform color = vertex light * (cc alpha) + 1 * (1 - cc alpha)
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_ADDSIGNED2X, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_VERTEXCOLOR );

			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_BLEND_CONSTANTALPHA, 
				SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_CONSTANTCOLOR );

			// Alpha isn't used, it doesn't matter what we set it to.
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
				SHADER_TEXARG_NONE, SHADER_TEXARG_NONE );
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
				SHADER_TEXARG_NONE, SHADER_TEXARG_NONE );

			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION );
		}
		DYNAMIC_STATE
		{
			// Put the alpha in the color channel to modulate the color down....
			float alpha = GetAlpha();
			float overbright = IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT) ? 1.0 : g_pConfig->overbright;
			if (overbright == 2.0f)
				pShaderAPI->Color4f( 0.5f, 0.5f, 0.5f, alpha );
			else
				pShaderAPI->Color4f( 1.0f, 1.0f, 1.0f, alpha );

			SetAmbientCubeDynamicStateFixedFunction( );
			FogToWhite();
		}
		Draw();

		SHADOW_STATE
		{
			pShaderShadow->EnableCustomPixelPipe( false );
			pShaderShadow->EnableAmbientLightCubeOnStage0( false );
			pShaderShadow->EnableLighting( false );
		}
	}


	//-----------------------------------------------------------------------------
	// Used by mode 1
	//-----------------------------------------------------------------------------

	void DrawBaseTimesVertexLighting( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Base times vertex lighting, no vertex color
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Vertex lighting + ambient cube
			pShaderShadow->EnableAmbientLightCubeOnStage0( true );
			pShaderShadow->EnableLighting( true );

			if (!IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT))
				pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, g_pConfig->overbright );

			// base
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			// Can't have color modulation here
			pShaderShadow->EnableConstantColor( false );

			// Independenly configure alpha and color
			pShaderShadow->EnableAlphaPipe( true );
			pShaderShadow->EnableConstantAlpha( IsAlphaModulating() );
			pShaderShadow->EnableVertexAlpha( IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA) );
			pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE0, false );
			pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE1, true );

			SetDefaultBlendingShadowState( BASETEXTURE, true );

			pShaderShadow->DrawFlags( GetDrawFlagsPass1( params ) );
		}
		DYNAMIC_STATE
		{
			SetAmbientCubeDynamicStateFixedFunction( );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE1, BASETEXTURETRANSFORM );
			BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE, FRAME );
			SetModulationDynamicState();
			DefaultFog();
		}
		Draw();

		SHADOW_STATE
		{
			// Vertex lighting + ambient cube
			pShaderShadow->EnableAmbientLightCubeOnStage0( false );
			pShaderShadow->EnableLighting( false );
			pShaderShadow->EnableAlphaPipe( false );
		}
	}

	inline int DrawBaseTimesVertexColor_GetDrawFlags(IMaterialVar** params)
	{
		int flags = SHADER_DRAW_POSITION;// | SHADER_DRAW_SPECULAR;
//		if (params[BASETEXTURE]->IsDefined())
			flags |= SHADER_DRAW_TEXCOORD0;
		return flags;
	}

	
	void DrawBaseTimesVertexColor( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
#if 0
			pShaderShadow->EnableCustomPixelPipe( true );
			
			pShaderShadow->CustomTextureStages( 1 );

			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE2X, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_VERTEXCOLOR );
		
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->DrawFlags( DrawBaseTimesVertexColor_GetDrawFlags( params ) );
#else
			// alpha test
// 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// base
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableLighting( true );
			if (!IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT))
				pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, g_pConfig->overbright );

			// Independenly configure alpha and color

			// Color = Color mod * Vertex Light * Tex (x2)
			// Alpha = Constant Alpha * Tex Alpha (no tex alpha if self illum == 1)
			// Can't have color modulation here
//			pShaderShadow->EnableConstantColor( IsColorModulating() );

			// Independenly configure alpha and color
//			pShaderShadow->EnableAlphaPipe( true );
//			pShaderShadow->EnableConstantAlpha( IsAlphaModulating() );
//			pShaderShadow->EnableVertexAlpha( IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA) );

						
			
//			if (!IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) && !IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
//				pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE0, true );

//			SetDefaultBlendingShadowState( BASETEXTURE, true );
			pShaderShadow->DrawFlags( DrawBaseTimesVertexColor_GetDrawFlags( params ) );
#endif

		}
		DYNAMIC_STATE
		{
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
//			SetModulationDynamicState();
			DefaultFog();
		}
		Draw();
	}

	void DrawMode0( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		bool texDefined = params[BASETEXTURE]->IsDefined();
		bool envDefined = params[ENVMAP]->IsDefined();
//		bool maskDefined = params[ENVMAPMASK]->IsDefined();

		// Pass 1 : Base + env

		// FIXME: Could make it 1 pass for base + env, if it wasn't
		// for the envmap tint. So this is 3 passes for now....

		// If it's base + mask * env, gotta do that in 2 passes
		if ( texDefined )
		{
			FixedFunctionBaseTimesDetailPass( 
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE ); 
		}
		else
		{
			FixedFunctionMaskedEnvmapPass( 
				ENVMAP, ENVMAPMASK, BASETEXTURE,
				ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
				BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
		}

		// We can get here if multipass isn't set if we specify a vertex color
		if ( IS_FLAG_SET(MATERIAL_VAR_MULTIPASS) )
		{
			if ( texDefined && envDefined )
			{
				FixedFunctionAdditiveMaskedEnvmapPass( 
					ENVMAP, ENVMAPMASK, BASETEXTURE,
					ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
					BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
			}
		}

		// Pass 2 : * vertex lighting
 		MultiplyByVertexLighting( params, pShaderAPI, pShaderShadow );

		// FIXME: We could add it to the lightmap 
		// Draw the selfillum pass (blows away envmap at self-illum points)
		if ( IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) )
		{
			FixedFunctionSelfIlluminationPass( 
				SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME, BASETEXTURETRANSFORM, SELFILLUMTINT );
		}
	}

	void DrawMode1( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Pass 1 : Base * lightmap or just lightmap
		if ( params[BASETEXTURE]->IsDefined() )
		{
			DrawBaseTimesVertexLighting( params, pShaderAPI, pShaderShadow );

			// Detail map
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
			DrawVertexLightingOnly( params, pShaderAPI, pShaderShadow );

			FixedFunctionMultiplyByDetailPass(
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );
		}
		 

		// Pass 2 : Masked environment map
		if ( params[ENVMAP]->IsDefined() && (IS_FLAG_SET(MATERIAL_VAR_MULTIPASS)) )
		{
			FixedFunctionAdditiveMaskedEnvmapPass( 
				ENVMAP, ENVMAPMASK, BASETEXTURE,
				ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
				BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
		}
	}

	SHADER_DRAW
	{
		// Base * Vertex lighting + env
		DrawMode1( params, pShaderAPI, pShaderShadow );
	}
END_SHADER

