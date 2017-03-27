//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( DebugUnlit, 
			  "Help for DebugUnlit" )
			  
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
				pShaderShadow->EnableConstantColor( true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION );
				pShaderShadow->EnableVertexBlend( IS_FLAG_SET( MATERIAL_VAR_MODEL ) );
			}
			DYNAMIC_STATE
			{
			}
			Draw();
		}
		else
		{
			SHADOW_STATE
			{
				bool bDoSkin = IS_FLAG_SET( MATERIAL_VAR_MODEL );
				int fmt = VERTEX_POSITION;
				if ( bDoSkin )
				{
					fmt |= VERTEX_BONE_INDEX;
				}
				pShaderShadow->VertexShaderVertexFormat( fmt, 0, 0, bDoSkin ? 3 : 0, 0 );
				pShaderShadow->SetVertexShader( "UnlitGeneric_LightingOnly" );
				pShaderShadow->SetPixelShader( "UnlitGeneric_NoTexture" );
			}
			DYNAMIC_STATE
			{
			}
			Draw();
		}
	}
END_SHADER
