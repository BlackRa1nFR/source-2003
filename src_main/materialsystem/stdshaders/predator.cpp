//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( Predator, 
			  "Help for Predator" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( REFRACTIONAMOUNT, SHADER_PARAM_TYPE_VEC2, "[0 0]", "refaction amount" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "envmap frame number" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTimesLightmapPlusMaskedCubicEnvMap_glass", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPOFFSET, SHADER_PARAM_TYPE_VEC2, "[0 0]", "2D bump offset for texture scrolling" )
	END_SHADER_PARAMS
			
	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		SET_FLAGS( MATERIAL_VAR_MODEL );
		SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
	}

	SHADER_FALLBACK
	{
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
			return "VertexLitGeneric";
		return 0;
	}

	bool NeedsFrameBufferTexture( IMaterialVar **params ) const
	{
		return true;
	}

	SHADER_INIT
	{
		if( params[ENVMAP]->IsDefined() )
		{
			LoadCubeMap( ENVMAP );
		}
		if( params[ENVMAPMASK]->IsDefined() )
		{
			LoadTexture( ENVMAPMASK );
		}
		if( !params[ENVMAPTINT]->IsDefined() )
		{
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[REFRACTTINT]->IsDefined() )
		{
			params[REFRACTTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
	}

	SHADER_DRAW
	{
		// Refractive pass
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 3, 0 );
			pShaderShadow->SetVertexShader( "predator" );
			pShaderShadow->SetPixelShader( "predator" );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE0 );
			SetVertexShaderConstant( 94, REFRACTIONAMOUNT );
			float c95[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
			pShaderAPI->SetVertexShaderConstant( 95, c95, 1 );
			SetPixelShaderConstant( 2, REFRACTTINT );
		}
		Draw();

		if( params[ENVMAP]->IsDefined() )
		{
			if( params[ENVMAPMASK]->IsDefined() )
			{
				SHADOW_STATE
				{
					SetInitialShadowState( );
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
					pShaderShadow->EnableBlending( true );
					pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
					pShaderShadow->SetVertexShader( "Predator_envmaptimesenvmapmask" );
					pShaderShadow->SetPixelShader( "Predator_envmaptimesenvmapmask" );
				}
				DYNAMIC_STATE
				{
					BindTexture( SHADER_TEXTURE_STAGE0, ENVMAP, ENVMAPFRAME );
					BindTexture( SHADER_TEXTURE_STAGE1, ENVMAPMASK, ENVMAPMASKFRAME );
					SetPixelShaderConstant( 2, ENVMAPTINT );
				}
				Draw();
			}
			else
			{
				SHADOW_STATE
				{
					SetInitialShadowState( );
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
					pShaderShadow->EnableBlending( true );
					pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
					pShaderShadow->SetVertexShader( "Predator_envmap" );
					pShaderShadow->SetPixelShader( "Predator_envmap" );
				}
				DYNAMIC_STATE
				{
					BindTexture( SHADER_TEXTURE_STAGE0, ENVMAP, ENVMAPFRAME );
					SetPixelShaderConstant( 2, ENVMAPTINT );
				}
				Draw();
			}
		}
	}
END_SHADER
