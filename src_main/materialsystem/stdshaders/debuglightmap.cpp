//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

// FIXME: Need to make a dx9 version so that "CENTROID" works.

BEGIN_SHADER( DebugLightmap, 
			  "Help for DebugLightmap" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FIXEDFUNCTION, SHADER_PARAM_TYPE_INTEGER, "0", "Using the fixed function or vertex shader path" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		if ( params[FIXEDFUNCTION]->GetIntValue() || g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
			SHADOW_STATE
			{
				pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, g_pConfig->overbright );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
			}
			DYNAMIC_STATE
			{
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
			}
			Draw();
		}
		else
		{
			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
				pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 2, 0, 0, 0 );
				pShaderShadow->SetVertexShader( "LightmappedGeneric_LightingOnly" );
				if( g_pConfig->overbright == 2.0f )
				{
					pShaderShadow->SetPixelShader( "LightmappedGeneric_LightingOnly_Overbright2" );
				}
				else
				{
					pShaderShadow->SetPixelShader( "LightmappedGeneric_LightingOnly_Overbright1" );
				}
			}
			DYNAMIC_STATE
			{
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
			}
			Draw();
		}
	}
END_SHADER
