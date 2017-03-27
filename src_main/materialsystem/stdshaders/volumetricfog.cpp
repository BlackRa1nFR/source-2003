//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( VolumetricFog, "Help for VolumetricFog" )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_NEAREROREQUAL );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR );
		}
		DYNAMIC_STATE
		{
		}
		Draw();
	}
END_SHADER
