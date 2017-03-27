//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "common_hlsl_cpp_consts.h"


BEGIN_VS_SHADER( Downsample, "Help for Downsample" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
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
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
			return "Wireframe";
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableAlphaWrites( true );

			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			pShaderShadow->SetVertexShader( "Downsample_vs20" );
			pShaderShadow->SetPixelShader( "Downsample_ps20", 0 );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, FBTEXTURE, -1 );

			int width, height;
			pShaderAPI->GetBackBufferDimensions( width, height );

			float v[4];
			float dX = 1.0f / width;
			float dY = 1.0f / height;

			v[0] = -dX;
			v[1] = -dY;
			pShaderAPI->SetVertexShaderConstant( 90, v, 1 );
			v[0] = -dX;
			v[1] = dY;
			pShaderAPI->SetVertexShaderConstant( 91, v, 1 );
			v[0] = dX;
			v[1] = -dY;
			pShaderAPI->SetVertexShaderConstant( 92, v, 1 );
			v[0] = dX;
			v[1] = dY;
			pShaderAPI->SetVertexShaderConstant( 93, v, 1 );

			// Setup luminance threshold (all values are scaled down by max luminance)
//			v[0] = 1.0f / MAX_HDR_OVERBRIGHT;
			v[0] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 0, v, 1 );
		}
		Draw();
	}
END_SHADER
