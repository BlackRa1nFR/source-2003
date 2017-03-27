//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

#include "bloom_ps20.inc"

BEGIN_VS_SHADER( Bloom, "Help for Bloom" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( BLURTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( params[FBTEXTURE]->IsDefined() )
		{
			LoadTexture( FBTEXTURE );
		}
		if( params[BLURTEXTURE]->IsDefined() )
		{
			LoadTexture( BLURTEXTURE );
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
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			// Pre-cache shaders
			bloom_ps20_Index pshIndex;
			pShaderShadow->SetPixelShader( "Bloom_ps20", pshIndex.GetIndex() );
			pShaderShadow->SetVertexShader( "ScreenSpaceEffect_VS20" );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, FBTEXTURE, -1 );
			BindTexture( SHADER_TEXTURE_STAGE1, BLURTEXTURE, -1 );

		}
		Draw();
	}
END_SHADER
