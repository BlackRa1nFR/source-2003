//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

#include "showdestalpha_ps20.inc"

BEGIN_SHADER( ShowDestAlpha, "Help for ShowDestAlpha" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( MULTIPLYBYCOLOR, SHADER_PARAM_TYPE_BOOL, "0", "" )
		SHADER_PARAM( MULTIPLYBYALPHA, SHADER_PARAM_TYPE_BOOL, "0", "" )
		SHADER_PARAM( MULTIPLYBY_MAX_HDR_OVERBRIGHT, SHADER_PARAM_TYPE_BOOL, "0", "" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( params[FBTEXTURE]->IsDefined() )
		{
			LoadTexture( FBTEXTURE );
		}
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
			return "Wireframe";
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );

			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "ScreenSpaceEffect_VS20" );
			
			showdestalpha_ps20_Index pshIndex;
			pshIndex.SetMULTIPLYBYCOLOR( params[MULTIPLYBYCOLOR]->GetIntValue() != 0 );
			pshIndex.SetMULTIPLYBYALPHA( params[MULTIPLYBYALPHA]->GetIntValue() != 0 );
			pshIndex.SetMULTIPLYBY_MAX_HDR_OVERBRIGHT( params[MULTIPLYBY_MAX_HDR_OVERBRIGHT]->GetIntValue() != 0 );
			pShaderShadow->SetPixelShader( "ShowDestAlpha_ps20", pshIndex.GetIndex() );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, FBTEXTURE, -1 );

			float highlight[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 0, highlight );
		}
		Draw();
	}
END_SHADER
