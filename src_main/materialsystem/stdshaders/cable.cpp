//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: A wet version of base * lightmap
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"


BEGIN_VS_SHADER( Cable, 
			  "Help for Cable shader" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "base texture" )
		SHADER_PARAM( MINLIGHT, SHADER_PARAM_TYPE_FLOAT, "0.25", "Minimum amount of light (0-1 value)" )
		SHADER_PARAM( MAXLIGHT, SHADER_PARAM_TYPE_FLOAT, "0.25", "Maximum amount of light" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			return "UnlitGeneric_DX6";
		}
		return 0;
	}

	SHADER_INIT
	{
		LoadBumpMap( BUMPMAP );
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			// Enable blending?
			if ( IS_FLAG_SET( MATERIAL_VAR_TRANSLUCENT ) )
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}

			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			
			int tCoordDimensions[] = {2,2};
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_COLOR | VERTEX_TANGENT_S | VERTEX_TANGENT_T, 
				2, tCoordDimensions, 
				0, 0 );

			pShaderShadow->SetVertexShader( "Cable" );
			pShaderShadow->SetPixelShader( "Cable" );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP );
			BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE );
			
			FogToFogColor();


			// The dot product with the light is remapped from the range 
			// [-1,1] to [MinLight, MaxLight].

			// Given:
			//	-A + B = MinLight
			//	 A + B = MaxLight
			// then A = (MaxLight - MinLight) / 2
			// and  B = (MaxLight + MinLight) / 2

			// So here, we multiply the light direction by A to scale the dot product.
			// Then in the pixel shader we add by B.
			float flMinLight = params[MINLIGHT]->GetFloatValue();
			float flMaxLight = params[MAXLIGHT]->GetFloatValue();
			
			float A = (flMaxLight - flMinLight) * 0.5f;
			float B = (flMaxLight + flMinLight) * 0.5f;

			float b4[4] = {B,B,B,B};
			pShaderAPI->SetPixelShaderConstant( 0, b4 );
			
			// This is the light direction [0,1,0,0] * A * 0.5
			float lightDir[4] = {0, A*0.5, 0, 0};
			pShaderAPI->SetVertexShaderConstant( 90, lightDir );
		}
		Draw();
	}
END_SHADER
