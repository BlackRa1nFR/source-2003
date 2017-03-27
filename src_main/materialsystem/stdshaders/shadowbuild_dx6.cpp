//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: A shader that builds the shadow using render-to-texture
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

DEFINE_FALLBACK_SHADER( ShadowBuild, ShadowBuild_DX6 )

BEGIN_SHADER( ShadowBuild_DX6,
			  "Help for ShadowBuild" )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableConstantColor( true );
			pShaderShadow->EnableConstantAlpha( true );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->FogMode( MATERIAL_FOG_NONE );
			pShaderAPI->Color4ub( 128, 128, 128, 128 );
		}
		Draw( );
	}
END_SHADER
