//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"


SOFTWARE_VERTEX_SHADER( fogVert )
{
	Vector eyePos;
	pShaderAPI->GetWorldSpaceCameraPosition( &eyePos.x );
	float fogStart = pShaderAPI->GetFogStart();
	float fogEnd = pShaderAPI->GetFogEnd();
	// garymcthack
	fogStart = 1.0f;
	fogEnd = 1500.0f;
	float fogDelta = fogEnd - fogStart;
	for( int i = 0; i < meshBuilder.NumVertices(); i++ )
	{
		const float *pPos = meshBuilder.Position();
		float fogAlpha;
		Vector pos( pPos[0], pPos[1], pPos[2] ); // garymcthack
		
		Vector delta;
		VectorSubtract( pos, eyePos, delta );
		float mag = sqrt( DotProduct( delta, delta ) );
		mag -= fogStart;
		if( mag < 0.0f )
		{
			fogAlpha = 0.0f;
		}
		else if( mag > fogDelta )
		{
			fogAlpha = 1.0f;
		}
		else
		{
			fogAlpha = mag / ( fogDelta );
		}
		meshBuilder.Color4f( 1.0f, 1.0f, 1.0f, fogAlpha );
		meshBuilder.AdvanceVertex();
	}
}

BEGIN_SHADER( FogSurface, 
			  "Help for FogSurface" )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
		USE_SOFTWARE_VERTEX_SHADER( fogVert );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableCulling( false ); // hack hack hack
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableConstantColor( true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR );
		}
		DYNAMIC_STATE
		{
			unsigned char fogColor[3];
			pShaderAPI->GetSceneFogColor( fogColor );
			pShaderAPI->Color3ubv( fogColor );
		}
		Draw();
	}
END_SHADER
