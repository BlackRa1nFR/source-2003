//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: A wet version of base * lightmap
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( BaseTimesLightmapWet,
   		      "Help for BaseTimesLightmapWet" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/cubemap", "envmap" )
		SHADER_PARAM( WETBRIGHTNESSFACTOR, SHADER_PARAM_TYPE_FLOAT, "1.0", "wet brightness factor" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		if (!params[WETBRIGHTNESSFACTOR]->IsDefined())
			params[WETBRIGHTNESSFACTOR]->SetFloatValue(1.0);

		if (g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			LoadBumpMap( BUMPMAP );
			LoadCubeMap( ENVMAP );
		}

		if (!params[WETBRIGHTNESSFACTOR]->IsDefined())
			params[WETBRIGHTNESSFACTOR]->SetFloatValue(1.0);
	}

	SHADER_DRAW
	{
		if (g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );

				// FIXME: Remove the normal (needed for tangent space gen)
				pShaderShadow->VertexShaderVertexFormat( 
					VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
					VERTEX_TANGENT_T, 1, 0, 0, 0 );
				pShaderShadow->SetVertexShader( "BaseTimesLightmapWet" );
				pShaderShadow->SetPixelShader( "BaseTimesLightmapWet" );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP );
				BindTexture( SHADER_TEXTURE_STAGE3, ENVMAP );
				SetPixelShaderConstant( 0, WETBRIGHTNESSFACTOR );
			}
			Draw();
		}
		else if( g_pHardwareConfig->GetNumTextureUnits() >= 2 )
		{
			SHADOW_STATE
			{
				SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
				pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, g_pConfig->overbright );
				pShaderShadow->EnableConstantColor( true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0 |
								  SHADER_DRAW_TEXCOORD1 );
			}
			DYNAMIC_STATE
			{
				SetColorState( WETBRIGHTNESSFACTOR );
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
				BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE, FRAME );
			}
			Draw();
		}
		else
		{
			// single-texturing version
			SHADOW_STATE
			{
				SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableConstantColor( true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
			}
			DYNAMIC_STATE
			{
				SetColorState( WETBRIGHTNESSFACTOR );
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
			}
			Draw();

			SHADOW_STATE
			{
				pShaderShadow->EnableBlending( true );
				SingleTextureLightmapBlendMode();
				pShaderShadow->EnableConstantColor( false );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
			}
			DYNAMIC_STATE
			{
				pShaderAPI->Color3f( 1.0f, 1.0f, 1.0f );
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			}
			Draw();			
		}
	}
END_SHADER
