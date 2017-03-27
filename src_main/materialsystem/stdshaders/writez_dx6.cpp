//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

DEFINE_FALLBACK_SHADER( WriteZ, WriteZ_DX6 )

BEGIN_SHADER( WriteZ_DX6, 
			  "Help for WriteZ_DX6" )
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
			pShaderShadow->EnableColorWrites( false );
			pShaderShadow->EnableAlphaWrites( false );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->FogMode( MATERIAL_FOG_NONE );
		}
		Draw();
	}
END_SHADER

