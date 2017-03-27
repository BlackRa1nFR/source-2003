//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Teeth renderer
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

DEFINE_FALLBACK_SHADER( Teeth, Teeth_DX8 )

BEGIN_VS_SHADER( Teeth_DX8, 
			  "Help for Teeth_DX8" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ILLUMFACTOR, SHADER_PARAM_TYPE_FLOAT, "1", "Amount to darken or brighten the teeth" )
		SHADER_PARAM( FORWARD, SHADER_PARAM_TYPE_VEC3, "[1 0 0]", "Forward direction vector for teeth lighting" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// default to 'MODEL' mode...
		if (!IS_FLAG_DEFINED( MATERIAL_VAR_MODEL ))
			SET_FLAGS( MATERIAL_VAR_MODEL );
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	void DrawUsingSoftwareLighting( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, g_pConfig->overbright );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR | SHADER_DRAW_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			FogToFogColor();
		}
		Draw();
	}

	void DrawUsingVertexShader( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX, 1, 0, 3, 0 );
			pShaderShadow->SetVertexShader( "Teeth" );
			if (g_pConfig->overbright == 2)
			{
				pShaderShadow->SetPixelShader( "VertexLitTexture_Overbright2" );
			}
			else
			{
				pShaderShadow->SetPixelShader( "VertexLitTexture" );
			}
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			SetAmbientCubeDynamicStateVertexShader();
			
			Vector4D lighting;
			params[FORWARD]->GetVecValue( lighting.Base(), 3 );
			lighting[3] = params[ILLUMFACTOR]->GetFloatValue();
			pShaderAPI->SetVertexShaderConstant( 90, lighting.Base() );


			FogToFogColor();
		}
		Draw();
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
		}
		if( g_pConfig->bSoftwareLighting )
		{
			DrawUsingSoftwareLighting( params, pShaderAPI, pShaderShadow );
		}
		else if( g_pHardwareConfig->SupportsVertexAndPixelShaders() )
		{
			DrawUsingVertexShader( params, pShaderAPI, pShaderShadow );
		}
		else
		{
			DrawUsingSoftwareLighting( params, pShaderAPI, pShaderShadow );
		}
	}
END_SHADER
