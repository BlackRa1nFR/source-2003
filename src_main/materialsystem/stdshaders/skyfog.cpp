//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( SkyFog, 
			  "Help for SkyFog" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableConstantColor( true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION );
		}
		DYNAMIC_STATE
		{
			unsigned char fogColor[3];
			pShaderAPI->GetSceneFogColor( fogColor );
			pShaderAPI->Color3ubv( fogColor );
			DisableFog();
		}
		Draw( );
	}
END_SHADER
