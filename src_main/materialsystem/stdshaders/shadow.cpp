//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( Shadow,
			  "Help for Shadow" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// FIXME: Need fallback for dx5, don't fade out shadows, just pop them out

		/*
		The alpha blending state either must be:

	Src Color * Dst Color + Dst Color * 0	
		(src color = C*A + 1-A)

or

  // Can't be this, doesn't work with fog
	Src Color * Dst Color + Dst Color * (1-Src Alpha)	
		(src color = C * A, Src Alpha = A)

	*/
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "Shadow_DX8";
		}
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_ZERO );
			unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
			int numTexCoords = 1;
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "Shadow_vs20" );
			pShaderShadow->SetPixelShader( "Shadow_ps20" );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );

			SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			SetPixelShaderConstant( 1, COLOR );

			// Get texture dimensions...
			int nWidth = 16;
			int nHeight = 16;
			ITexture *pTexture = params[BASETEXTURE]->GetTextureValue();
			if (pTexture)
			{
				nWidth = pTexture->GetActualWidth();
				nHeight = pTexture->GetActualHeight();
			}

			Vector4D vecJitter( 1.0 / nWidth, 1.0 / nHeight, 0.0, 0.0 );
			pShaderAPI->SetVertexShaderConstant( 92, vecJitter.Base() );

			vecJitter.y *= -1.0f;
			pShaderAPI->SetVertexShaderConstant( 93, vecJitter.Base() );

			MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
			int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;

			pShaderAPI->SetVertexShaderIndex( fogIndex );

			// We need to fog to *white* regardless of overbrighting...
			pShaderAPI->FogMode( pShaderAPI->GetSceneFogMode() );
			pShaderAPI->FogColor3f( 1.0f, 1.0f, 1.0f );
		}
		Draw( );
	}
END_SHADER
