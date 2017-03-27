//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

// FIXME: Need to make a dx9 version so that "CENTROID" works.

BEGIN_SHADER( WorldTwoTextureBlend, 
			  "Help for WorldTwoTextureBlend" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, ".1", "scale of the detail texture" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		LoadTexture( DETAIL );
	}

	SHADER_DRAW
	{
		// FIXME: add multitexture support
		float scale = params[DETAILSCALE]->GetFloatValue();
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			FogToFogColor();
		}
		Draw();

		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			if (scale != 1.0f)
			{
				pShaderAPI->MatrixMode( MATERIAL_TEXTURE0 );
				pShaderAPI->LoadIdentity();
				pShaderAPI->ScaleXY( scale, scale );
			}
			BindTexture( SHADER_TEXTURE_STAGE0, DETAIL );
			FogToFogColor();
		}
		Draw();

		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
			SingleTextureLightmapBlendMode();
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			if (scale != 1.0f)
				pShaderAPI->LoadIdentity( );
			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
			FogToWhite();
		}
		Draw();

	}
END_SHADER
