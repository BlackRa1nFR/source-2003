//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( JellyFish, 
			  "Help for JellyFish" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( GRADIENTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "1D texture for silhouette glowy bits" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/cubemap", "envmap" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmaptint" )
		SHADER_PARAM( PULSERATE, SHADER_PARAM_TYPE_FLOAT, "1", "blah" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( GRADIENTTEXTURE );
		LoadTexture( BASETEXTURE );
		if( params[ENVMAP]->IsDefined() )
		{
			LoadCubeMap( ENVMAP );
		}
		if( !params[ENVMAPTINT]->IsDefined() )
		{
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[PULSERATE]->IsDefined() )
		{
			params[PULSERATE]->SetFloatValue( 0.0f );
		}
	}

	SHADER_DRAW
	{
		// fixme
		if( !g_pHardwareConfig->SupportsVertexAndPixelShaders() )
		{
			return;
		}

		SHADOW_STATE
		{				
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL, 1, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "JellyFish" );
			pShaderShadow->SetPixelShader( "JellyFish" );
		}
		DYNAMIC_STATE
		{
			float time[4];
			time[0] = pShaderAPI->CurrentTime() * params[PULSERATE]->GetFloatValue();
			BindTexture( SHADER_TEXTURE_STAGE0, GRADIENTTEXTURE );
			BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE );
			pShaderAPI->SetVertexShaderConstant( 95, time, 1 );
			LoadViewMatrixIntoVertexShaderConstant( 90 );
			FogToBlack();
		}
		Draw();

		if( params[ENVMAP]->IsDefined() )
		{
			SHADOW_STATE
			{
				SetInitialShadowState( );
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_CAMERASPACEREFLECTIONVECTOR );
				pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_NORMAL );
				pShaderShadow->EnableConstantColor( true );
			}
			DYNAMIC_STATE
			{
				SetColorState( ENVMAPTINT );
				LoadCameraToWorldTransform( MATERIAL_TEXTURE0 );
				BindTexture( SHADER_TEXTURE_STAGE0, ENVMAP );
				FogToFogColor();
			}
			Draw();
		}
	}
END_SHADER
