//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

SOFTWARE_VERTEX_SHADER( myHappyLittleSoftwareVertexShader )
{
	float t = pShaderAPI->CurrentTime();
	for( int i = 0; i < meshBuilder.NumVertices(); i++ )
	{
		const float *pPos = meshBuilder.Position();
		meshBuilder.Position3f( pPos[0], pPos[1], pPos[2] + 10.0f * sin( t + pPos[0] ) );
		meshBuilder.AdvanceVertex();
	}
}

FORWARD_DECLARE_SOFTWARE_VERTEX_SHADER( myHappyLittleSoftwareVertexShader );

BEGIN_SHADER( DebugSoftwareVertexShader,
			  "blah" )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( g_pHardwareConfig->SupportsVertexAndPixelShaders() )
		{
			USE_SOFTWARE_VERTEX_SHADER( myHappyLittleSoftwareVertexShader );
		}
		else
		{
			USE_SOFTWARE_VERTEX_SHADER( myHappyLittleSoftwareVertexShader );
		}
	}
	SHADER_DRAW
	{
		SHADOW_STATE
		{
		}
		DYNAMIC_STATE
		{
		}
		Draw();
	}
END_SHADER
