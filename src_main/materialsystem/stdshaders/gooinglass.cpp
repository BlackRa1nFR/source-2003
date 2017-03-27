//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

// FIXME: Need to make a dx9 version so that "CENTROID" works.

BEGIN_VS_SHADER( GooInGlass,
			  "Help for GooInGlass" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bump texcoord transform" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/cubemap", "envmap" )
		SHADER_PARAM( GLASSENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/cubemap", "envmap" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( GLASSENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( TRANSLUCENTGOO, SHADER_PARAM_TYPE_BOOL, "0", "whether or not goo is translucent" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadBumpMap( BUMPMAP );
		LoadTexture( BASETEXTURE );
		LoadCubeMap( ENVMAP );
		LoadCubeMap( GLASSENVMAP );
		if( !params[ENVMAPTINT]->IsDefined() )
		{
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[GLASSENVMAPTINT]->IsDefined() )
		{
			params[GLASSENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[TRANSLUCENTGOO]->IsDefined() )
		{
			params[TRANSLUCENTGOO]->SetIntValue( 0 );
		}
	}

	SHADER_DRAW
	{
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			// hack - draw the unlit alpha blended texture. . this should
			// really fall back to something else, but leave it as this so 
			// that the tools don't barf.
			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToFogColor();
			}
			
			Draw( );
		}
		else
		{
			// + MASKED BUMPED CUBEMAP * ENVMAPTINT
			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
				if( params[TRANSLUCENTGOO]->GetIntValue() )
				{
					pShaderShadow->EnableBlending( true );
					pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				}
				// FIXME: Remove the normal (needed for tangent space gen)
				pShaderShadow->VertexShaderVertexFormat( 
					VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
					VERTEX_TANGENT_T, 1, 0, 0, 0 );

				pShaderShadow->SetVertexShader( "BumpmappedEnvmap" );
				pShaderShadow->SetPixelShader( "BumpmappedEnvmap" );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP );
				BindTexture( SHADER_TEXTURE_STAGE3, ENVMAP );
				
				float constantColor[4];
				params[ENVMAPTINT]->GetVecValue( constantColor, 3 );
				constantColor[3] = 0.0f;
				pShaderAPI->SetPixelShaderConstant( 0, constantColor, 1 );
				FogToBlack();

				// handle scrolling of bump texture
				SetVertexShaderTextureTransform( 94, BUMPTRANSFORM );
			}
			Draw();
			// glass envmap
			SHADOW_STATE
			{
				SetInitialShadowState( );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );

				// FIXME: Remove the normal (needed for tangent space gen)
				pShaderShadow->VertexShaderVertexFormat( 
					VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
					VERTEX_TANGENT_T, 1, 0, 0, 0 );
				pShaderShadow->SetVertexShader( "BumpmappedEnvMap" );
				pShaderShadow->SetPixelShader( "BumpmappedEnvMap" );
			}
			DYNAMIC_STATE
			{
				// fixme: doesn't support camera space envmapping!!!!!!
				pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE0 );
				BindTexture( SHADER_TEXTURE_STAGE3, GLASSENVMAP );

				float constantColor[4];
				params[GLASSENVMAPTINT]->GetVecValue( constantColor, 3 );
				constantColor[3] = 0.0f;
				pShaderAPI->SetPixelShaderConstant( 0, constantColor, 1 );
				FogToBlack();
				SetVertexShaderTextureTransform( 94, BUMPTRANSFORM );
			}
			Draw();

			// BASE TEXTURE * LIGHTMAP
			SHADOW_STATE
			{				
				SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
				SetInitialShadowState( );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
				pShaderShadow->VertexShaderVertexFormat( 
					VERTEX_POSITION, 2, 0, 0, 0 );
				pShaderShadow->SetVertexShader( "LightmappedGeneric" );
				pShaderShadow->SetPixelShader( "LightmappedGeneric" );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
				SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );
				EnablePixelShaderOverbright( true, 0, true );
				FogToFogColor();
			}
			Draw();
		}
	}
END_SHADER
