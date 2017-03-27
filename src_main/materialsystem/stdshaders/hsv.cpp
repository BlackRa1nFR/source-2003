//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( HSV, "Help for HSV" )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if (g_pHardwareConfig->GetDXSupportLevel() != 90)
			return "Wireframe";
		return 0;
	}

	bool NeedsFrameBufferTexture( IMaterialVar **params ) const
	{
		return true;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "ScreenSpaceEffect_VS20" );
			pShaderShadow->SetPixelShader( "HSV_PS20" );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE0 );
		}
		Draw();
	}
END_SHADER
