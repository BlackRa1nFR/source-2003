//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "basevsshader.h"

BEGIN_VS_SHADER( DebugVertexLit, 
			  "Help for DebugVertexLit" )
			  
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
			if ( g_pHardwareConfig->SupportsHardwareLighting() )
			{
				SHADOW_STATE
				{
					pShaderShadow->EnableAmbientLightCubeOnStage0( true );
					pShaderShadow->EnableVertexBlend( true );
					pShaderShadow->EnableLighting( true );
					pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, g_pConfig->overbright );
					pShaderShadow->DrawFlags( SHADER_DRAW_POSITION );
				}
				DYNAMIC_STATE
				{
					SetAmbientCubeDynamicStateFixedFunction();
				}
				Draw();
			}
			else
			{
				SHADOW_STATE
				{
					pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, g_pConfig->overbright );
					pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR );
				}
				DYNAMIC_STATE
				{
				}
				Draw();
			}
		}
		else
		{
			SHADOW_STATE
			{
				pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );
				pShaderShadow->VertexShaderVertexFormat( 
					VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX, 0, 0, 3, 0 );
				pShaderShadow->SetVertexShader( "LightingOnly" );
				if( g_pConfig->overbright == 2.0f )
				{
					pShaderShadow->SetPixelShader( "VertexLitGeneric_LightingOnly_Overbright2" );
				}
				else
				{
					pShaderShadow->SetPixelShader( "VertexLitGeneric_LightingOnly_Overbright1" );
				}
			}
			DYNAMIC_STATE
			{
				SetAmbientCubeDynamicStateVertexShader();
			}
			Draw();
		}
	}
END_SHADER
