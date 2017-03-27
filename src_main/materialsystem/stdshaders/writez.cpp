//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_SHADER( WriteZ, 
			  "Help for WriteZ" )
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
			pShaderShadow->SetVertexShader( "writez" );
			pShaderShadow->SetPixelShader( "white" );
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 0, 0, 0, 0 );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->FogMode( MATERIAL_FOG_NONE );
		}
		Draw();
	}
END_SHADER

