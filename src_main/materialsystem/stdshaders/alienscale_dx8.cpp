//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

DEFINE_FALLBACK_SHADER( AlienScale, AlienScale_DX8 )

BEGIN_VS_SHADER( AlienScale_DX8, 
				"Help for VertexLitGeneric" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( TEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/basetexture", "texture 2" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $texture2" )
		SHADER_PARAM( TEXTURE2TRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "transform for texture2 scrolling" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "transform for bumpmap scrolling" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// default to 'MODEL' mode...
		if (!IS_FLAG_DEFINED( MATERIAL_VAR_MODEL ))
			SET_FLAGS( MATERIAL_VAR_MODEL );
		
		if( !params[FRAME2]->IsDefined() )
			params[FRAME2]->SetIntValue( 0 );

		if( !params[BUMPFRAME]->IsDefined() )
			params[BUMPFRAME]->SetIntValue( 0 );

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}

		if( params[BUMPMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		}
		
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
	}

	SHADER_FALLBACK
	{
		// On dx7, just fall back to normal vertex lit generic
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

		if (params[TEXTURE2]->IsDefined())
		{
			LoadTexture( TEXTURE2 );
		}

		if (params[BUMPMAP]->IsDefined())
		{
			LoadBumpMap( BUMPMAP );
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
		{
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );

			if ( g_pHardwareConfig->GetDXSupportLevel() >= 81 )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			}

			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX, 
				1, 0, 3, 4 /* userDataSize */ );

			if ( g_pHardwareConfig->GetDXSupportLevel() >= 81 )
			{
				pShaderShadow->SetVertexShader( "AlienScale_vs14" );

				if( IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT) || g_pConfig->overbright == 1.0f )
				{
					pShaderShadow->SetPixelShader( "AlienScale_ps14" );
				}
				else
				{
					pShaderShadow->SetPixelShader( "AlienScale_overbright2_ps14" );
				}
			}
			else
			{
				pShaderShadow->SetVertexShader( "AlienScale_vs11" );

				if( IS_FLAG_SET(MATERIAL_VAR_NOOVERBRIGHT) || g_pConfig->overbright == 1.0f )
				{
					pShaderShadow->SetPixelShader( "AlienScale_ps11" );
				}
				else
				{
					pShaderShadow->SetPixelShader( "AlienScale_overbright2_ps11" );
				}
			}
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, TEXTURE2, FRAME2 );
			BindTexture( SHADER_TEXTURE_STAGE2, BUMPMAP, BUMPFRAME );
			if ( g_pHardwareConfig->GetDXSupportLevel() >= 81 )
			{
				pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE3 );
			}
			
			LoadViewMatrixIntoVertexShaderConstant( 17 );
			SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			SetVertexShaderTextureTransform( 92, TEXTURE2TRANSFORM );
			SetVertexShaderTextureTransform( 94, BUMPTRANSFORM );
			FogToFogColor();
		}
		Draw();
	}
END_SHADER


