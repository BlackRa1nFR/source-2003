//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( Modulate,
			  "Help for Modulate" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( WRITEZ, SHADER_PARAM_TYPE_BOOL, "0", "Forces z to be written if set" )
		SHADER_PARAM( MOD2X, SHADER_PARAM_TYPE_BOOL, "0", "forces a 2x modulate so that you can brighten and darken things" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{	
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
			return "Modulate_DX6";
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
			LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		bool bMod2X = params[MOD2X]->IsDefined() && params[MOD2X]->GetIntValue();
		bool bVertexColorOrAlpha = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) || IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );

		SHADOW_STATE
		{
			if( bMod2X )
			{
				EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );
			}
			else
			{
				EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_ZERO );
			}

			if (params[WRITEZ]->GetIntValue() != 0)
			{
				// This overrides the disabling of depth writes performed in
				// EnableAlphaBlending
				pShaderShadow->EnableDepthWrites( true );
			}

			unsigned int flags = VERTEX_POSITION;
			int numTexCoords = 0;

			if( params[BASETEXTURE]->IsDefined() )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				numTexCoords = 1;
			}

			if( bVertexColorOrAlpha )
			{
				flags |= VERTEX_COLOR;
			}

			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, NULL, 0, 0 );

			if( bVertexColorOrAlpha )
			{
				pShaderShadow->SetVertexShader( "UnlitGeneric_vertexcolor" );
			}
			else
			{
				pShaderShadow->SetVertexShader( "UnlitGeneric" );
			}
			pShaderShadow->SetPixelShader( "Modulate_ps11" );
		}
		DYNAMIC_STATE
		{
			if( params[BASETEXTURE]->IsDefined() )
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
			}

			// set constant color for modulation
			SetModulationVertexShaderDynamicState();

			// We need to fog to *white* regardless of overbrighting...
			if( bMod2X )
			{
				float grey[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
				pShaderAPI->SetPixelShaderConstant( 0, grey );
				FogToGrey();
			}
			else
			{
				float white[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
				pShaderAPI->SetPixelShaderConstant( 0, white );
				FogToWhite();
			}
		}
		Draw( );
	}
END_SHADER
