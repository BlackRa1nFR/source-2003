//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( DebugFBTexture,
			  "Help for DebugFBTexture" )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
	}
	
	SHADER_DRAW
	{
#if 0
		// vertex shader version - doesn't work on nvidia since they don't
		// do power of two textures properly.
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );
		}

		DYNAMIC_STATE
		{
			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE0 );
			SetVertexShader( "DebugFBTexture" );
			SetPixelShader( "DebugFBTexture" );
			float c95[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
			g_pShaderAPI->SetVertexShaderConstant( 95, c95, 1 );
		}
		Draw();
#else
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
		}

		DYNAMIC_STATE
		{
			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE0 );
		}
		Draw();
#endif
	}
END_SHADER
