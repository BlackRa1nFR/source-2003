//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( Cloud,
			  "Help for Cloud" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( CLOUDALPHATEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "cloud alpha texture" )
		SHADER_PARAM( CLOUDSCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "cloudscale" )
		SHADER_PARAM( MASKSCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "maskscale" )
		SHADER_PARAM( ADDITIVE, SHADER_PARAM_TYPE_BOOL, "0", "additive blend (as opposed to over blend)" )
	END_SHADER_PARAMS
	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		LoadTexture( CLOUDALPHATEXTURE );
		if( !params[CLOUDSCALE]->IsDefined() )
		{
			params[CLOUDSCALE]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[MASKSCALE]->IsDefined() )
		{
			params[MASKSCALE]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[ADDITIVE]->IsDefined() )
		{
			params[ADDITIVE]->SetIntValue( 0 );
		}
	}

	SHADER_DRAW
	{
		if( g_pHardwareConfig->GetNumTextureUnits() >= 2 )
		{
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				if( params[ADDITIVE]->GetIntValue() )
				{
					pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
				}
				else
				{
					pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				}
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | 
					SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_TEXCOORD1 );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				BindTexture( SHADER_TEXTURE_STAGE1, CLOUDALPHATEXTURE );

				// handle scrolling of base texture
				SetFixedFunctionTextureScaledTransform( MATERIAL_TEXTURE0,
					BASETEXTURETRANSFORM, CLOUDSCALE );
				SetFixedFunctionTextureScale( MATERIAL_TEXTURE1, MASKSCALE );
				FogToFogColor();
			}
			Draw();
		}
		else
		{
			ShaderWarning("Cloud not supported for single-texturing boards!\n");
		}
	}
END_SHADER
