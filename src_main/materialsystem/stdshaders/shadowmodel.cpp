//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( ShadowModel,
			  "Help for ShadowModel" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BASETEXTUREOFFSET, SHADER_PARAM_TYPE_VEC2, "[0 0]", "$baseTexture texcoord offset" )
		SHADER_PARAM( BASETEXTURESCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "$baseTexture texcoord scale" )
		SHADER_PARAM( FALLOFFOFFSET, SHADER_PARAM_TYPE_FLOAT, "0", "Distance at which shadow starts to fade" )
		SHADER_PARAM( FALLOFFDISTANCE, SHADER_PARAM_TYPE_FLOAT, "100", "Max shadow distance" )
		SHADER_PARAM( FALLOFFAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.9", "Amount to brighten the shadow at max dist" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if (!params[BASETEXTURESCALE]->IsDefined())
		{
			Vector2D scale(1, 1);
			params[BASETEXTURESCALE]->SetVecValue( scale.Base(), 2 );
		}

		if (!params[FALLOFFDISTANCE]->IsDefined())
			params[FALLOFFDISTANCE]->SetFloatValue( 100.0f );

		if (!params[FALLOFFAMOUNT]->IsDefined())
			params[FALLOFFAMOUNT]->SetFloatValue( 0.9f );
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
			LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			// Base texture on stage 0
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );

			// Multiplicative blending state...
			EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_ZERO );

			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 3, 0 );
			pShaderShadow->SetVertexShader( "ShadowModel" );
			pShaderShadow->SetPixelShader( "ShadowModel" );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			SetVertexShaderMatrix3x4( 90, BASETEXTURETRANSFORM );

			SetVertexShaderConstant( 93, BASETEXTUREOFFSET );
			SetVertexShaderConstant( 94, BASETEXTURESCALE );

			Vector4D shadow;
			shadow[0] = params[FALLOFFOFFSET]->GetFloatValue();
			shadow[1] = params[FALLOFFDISTANCE]->GetFloatValue() + shadow[0];
			if (shadow[1] != 0.0f)
				shadow[1] = 1.0f / shadow[1];
			shadow[2] = params[FALLOFFAMOUNT]->GetFloatValue();
			pShaderAPI->SetVertexShaderConstant( 95, shadow.Base(), 1 );

			// The constant color is the shadow color...
			SetModulationVertexShaderDynamicState();

			// We need to fog to *white* regardless of overbrighting...
			pShaderAPI->FogMode( pShaderAPI->GetSceneFogMode() );
			pShaderAPI->FogColor3f( 1.0f, 1.0f, 1.0f );
		}
		Draw( );
	}
END_SHADER
