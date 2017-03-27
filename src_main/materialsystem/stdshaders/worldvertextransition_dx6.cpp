//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

DEFINE_FALLBACK_SHADER( WorldVertexTransition, WorldVertexTransition_DX6 )

BEGIN_SHADER( WorldVertexTransition_DX6,
			  "Help for WorldVertexTransition_dx6" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture2", "base texture2 help" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		LoadTexture( BASETEXTURE2 );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, g_pConfig->overbright );

			// Use vertex color for WorldCraft because it puts the blending alpha in the vertices.
			unsigned int colorFlag = 0;
			if( g_pConfig->bEditMode )
			{
				colorFlag |= SHADER_DRAW_COLOR;
			}

			pShaderShadow->DrawFlags( colorFlag | SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD1 | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			//pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_NEAREST );
			//pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_NEAREST );
			BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE );
			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
			//FogToFogColor();
		}
		Draw();

		 
		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableBlending( true );

			// Looks backwards, but this is done so that lightmap alpha = 1 when only
			// using 1 texture (needed for translucent displacements).
			pShaderShadow->BlendFunc( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_SRC_ALPHA );

			// Use vertex color for WorldCraft because it puts the blending alpha in the vertices.
			unsigned int colorFlag = 0;
			if( g_pConfig->bEditMode )
			{
				colorFlag |= SHADER_DRAW_COLOR;
			}

			pShaderShadow->DrawFlags( colorFlag | SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD1 | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE2 );
			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
			//FogToFogColor();
		}
		Draw();
	}
END_SHADER
