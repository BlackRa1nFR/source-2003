//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Eyeball shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( Eyeball, 
			  "Help for EyeBall" )
			   
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// default to 'MODEL' mode...
		SET_FLAGS( MATERIAL_VAR_MODEL );
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		// FIXME ?
		// I took out hardware lighting here because the ambient cube
		// was adding way too much to the light color. So, for now, eyeballs
		// are always software-lit.

		// FIXME: Remove when lighting is in material system
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, g_pConfig->overbright );
			EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			FogToFogColor();
		}
		Draw();
	}
END_SHADER
