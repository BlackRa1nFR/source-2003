//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( Jojirium,
			  "Help for Jojirium" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$baseTexture texcoord transform" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bump texcoord transform" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/cubemap", "envmap" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$baseTexture texcoord transform" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadBumpMap( BUMPMAP );
		LoadTexture( BASETEXTURE );

		if( params[DETAIL]->IsDefined() )
		{
			LoadTexture( DETAIL );
		}

		LoadCubeMap( ENVMAP );
		if( !params[ENVMAPTINT]->IsDefined() )
		{
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
	}

	SHADER_DRAW
	{
		VertexShaderUnlitGenericPass( 
			false, BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILTRANSFORM, false,
			-1, -1, -1, -1, -1, -1 );

		if (g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			// + MASKED BUMPED CUBEMAP * ENVMAPTINT
			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );


				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

				// FIXME: Remove the normal (needed for tangent space gen)
				pShaderShadow->VertexShaderVertexFormat( 
					VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
					VERTEX_TANGENT_T, 1, 0, 0, 0 );
				pShaderShadow->SetVertexShader( "Jojirium" );
				pShaderShadow->SetPixelShader( "BumpmappedEnvmap" );
			}
			DYNAMIC_STATE
			{
				pShaderAPI->SetDefaultState();
				BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP );
				BindTexture( SHADER_TEXTURE_STAGE3, ENVMAP );
				
				float constantColor[4];
				params[ENVMAPTINT]->GetVecValue( constantColor, 3 );
				constantColor[3] = 0.0f;
				pShaderAPI->SetPixelShaderConstant( 0, constantColor, 1 );
				FogToBlack();

				// Handle envmap thingy..
				SetVertexShaderMatrix3x4( 90, ENVMAPTRANSFORM );

				// handle scrolling of bump texture
				SetVertexShaderTextureTransform( 94, BUMPTRANSFORM );
			}
			Draw();
		}
	}
END_SHADER
