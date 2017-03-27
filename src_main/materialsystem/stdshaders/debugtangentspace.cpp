//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( DebugTangentSpace, 
			  "Help for DebugTangentSpace" )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		if (g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			SHADOW_STATE
			{
				int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX;
				pShaderShadow->VertexShaderVertexFormat( fmt, 0, 0, 3, 4 /* userDataSize */ );
				pShaderShadow->SetVertexShader( "DebugTangentSpace" );
				pShaderShadow->SetPixelShader( "LightingOnly" );
			}
			DYNAMIC_STATE
			{
			}
			Draw();
		}
	}
END_SHADER

