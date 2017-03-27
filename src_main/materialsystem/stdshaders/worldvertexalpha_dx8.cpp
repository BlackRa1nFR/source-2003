//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

DEFINE_FALLBACK_SHADER( WorldVertexAlpha, WorldVertexAlpha_DX8 )

BEGIN_VS_SHADER( WorldVertexAlpha_DX8, 
			  "Help for WorldVertexAlpha_DX8" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		if( g_pHardwareConfig->SupportsVertexAndPixelShaders() && !g_pConfig->bEditMode )
		{
			// Support TexKill is available for water rendering operations.
			SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_TEXKILL );
		}
	}
	SHADER_INIT
	{
		// Load the base texture here!
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		if( g_pHardwareConfig->SupportsVertexAndPixelShaders() && !g_pConfig->bEditMode )
		{
			// NOTE: This is the DX8, Non-Hammer version.

			SHADOW_STATE
			{
				// Base time lightmap (Need two texture stages)
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

				int fmt = VERTEX_POSITION;
				pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0, 0 );

				pShaderShadow->EnableBlending( true );

				// Looks backwards, but this is done so that lightmap alpha = 1 when only
				// using 1 texture (needed for translucent displacements).
				pShaderShadow->BlendFunc( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_SRC_ALPHA );
				pShaderShadow->SetVertexShader( "WorldVertexAlpha" );
				pShaderShadow->SetPixelShader( "WorldVertexAlpha" );
			}

			DYNAMIC_STATE
			{
				// Bind the base texture (Stage0) and lightmap (Stage1)
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE );
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
				FogToFogColor();
				EnablePixelShaderOverbright( 0, !IS_FLAG_SET( MATERIAL_VAR_NOOVERBRIGHT ), true );
			}

			Draw();
		}
		else
		{
			// NOTE: This is the DX7, Hammer version.

			SHADOW_STATE
			{
				SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
				pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, g_pConfig->overbright );

				pShaderShadow->EnableBlending( true );

				// Looks backwards, but this is done so that lightmap alpha = 1 when only
				// using 1 texture (needed for translucent displacements).
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
//				pShaderShadow->BlendFunc( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_SRC_ALPHA );

				// Use vertex color for WorldCraft because it puts the blending alpha in the vertices.
				unsigned int colorFlag = 0;
				if( g_pConfig->bEditMode )
					colorFlag |= SHADER_DRAW_COLOR;

				pShaderShadow->DrawFlags( colorFlag | SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD1 | 
					                      SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE );
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
			}

			Draw();
		}
	}
END_SHADER
