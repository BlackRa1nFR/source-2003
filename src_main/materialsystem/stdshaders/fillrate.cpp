//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( Fillrate, 
				"Help for Fillrate" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( PASSCOUNT, SHADER_PARAM_TYPE_INTEGER, "1", "Number of passes for this material" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_INIT
	{
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
			return "Fillrate_DX6";
		}
		return 0;
	}

	
	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 0, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "fillrate" );
			pShaderShadow->SetPixelShader( "fillrate" );
		}
		DYNAMIC_STATE
		{
			int numPasses =  params[PASSCOUNT]->GetIntValue();
			float color[4];
			if (g_pConfig->bMeasureFillRate)
			{
				// have to multiply by 2/255 since pixel shader constant are 1.7.
				// Will divide the 2 out in the pixel shader.
				color[0] = numPasses * ( 2.0f / 255.0f );
			}
			else
			{
				color[0] = ( 16 * numPasses ) * ( 2.0f / 255.0f );
			}
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 0, color, 1 );
			DisableFog();
		}
		Draw();

		SHADOW_STATE
		{
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 0, 0, 0, 0 );
			pShaderShadow->PolyMode( SHADER_POLYMODEFACE_FRONT_AND_BACK, SHADER_POLYMODE_LINE );
			pShaderShadow->SetVertexShader( "fillrate" );
			pShaderShadow->SetPixelShader( "fillrate" );
		}
		DYNAMIC_STATE
		{
			float color[4] = { 0.0f, 0.05f, 0.05f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 0, color, 1 );
			DisableFog();
		}
		Draw();
	}
END_SHADER


