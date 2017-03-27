//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( DebugLuxels, 
			  "Help for DebugLuxels" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( NOSCALE, SHADER_PARAM_TYPE_BOOL, "0", "fixme" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
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

			if (IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT))
			{
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}

			if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
			else
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );

			if (!params[NOSCALE]->GetIntValue())
			{
				int texCoordScaleX, texCoordScaleY;
				pShaderAPI->GetLightmapDimensions( &texCoordScaleX, &texCoordScaleY );
				pShaderAPI->MatrixMode( MATERIAL_TEXTURE0 );
				pShaderAPI->LoadIdentity( );
				pShaderAPI->ScaleXY( texCoordScaleX, texCoordScaleY );
			}
		}
		Draw();
	}
END_SHADER
