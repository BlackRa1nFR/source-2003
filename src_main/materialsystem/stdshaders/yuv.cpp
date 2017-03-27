//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( YUV, "Help for YUV" )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
	}
	
	SHADER_FALLBACK
	{
		// Requires DX8 + above
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
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
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "ScreenSpaceEffect" );
			pShaderShadow->SetPixelShader( "YUV" );
		}

		DYNAMIC_STATE
		{
			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE0 );
		}
		Draw();
	}
END_SHADER
