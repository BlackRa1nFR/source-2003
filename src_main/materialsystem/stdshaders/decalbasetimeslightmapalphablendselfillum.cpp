//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( DecalBaseTimesLightmapAlphaBlendSelfIllum, 
			  "Help for DecalBaseTimesLightmapAlphaBlendSelfIllum" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SELFILLUMTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "self-illum texture" )
		SHADER_PARAM( SELFILLUMTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "self-illum texture frame" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		LoadTexture( SELFILLUMTEXTURE );
	}

	SHADER_DRAW
	{
		if( g_pHardwareConfig->GetNumTextureUnits() < 2 )
		{
			ShaderWarning( "DecalBaseTimesLightmapAlphaBlendSelfIllum: not implemented for single-texturing hardware\n" );
			return;
		}


		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( true );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, g_pConfig->overbright );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | 
						SHADER_DRAW_TEXCOORD1 | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
			BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE, FRAME );
			FogToFogColor();
		}
		Draw();

		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( true );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, SELFILLUMTEXTURE, SELFILLUMTEXTUREFRAME );
			FogToBlack();
		}
		Draw();
	}
END_SHADER
