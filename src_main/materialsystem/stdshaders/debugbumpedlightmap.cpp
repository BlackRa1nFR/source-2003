//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( DebugBumpedLightmap, 
			  "Help for DebugBumpedLightmap" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
			return "DebugLightmap";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 3, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "BumpmappedLightmap" );
			if( g_pConfig->overbright == 1.0f )
			{
				pShaderShadow->SetPixelShader( "BumpmappedLightmap_LightingOnly_OverBright1" );
			}
			else
			{
				pShaderShadow->SetPixelShader( "BumpmappedLightmap_LightingOnly_OverBright2" );
			}
		}
		DYNAMIC_STATE
		{
			if ( params[BUMPMAP]->IsDefined() )
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP, BUMPFRAME );
			}
			else
			{
				pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE0 );
			}
			SetVertexShaderTextureTransform( 90, BUMPTRANSFORM );
			LoadBumpLightmapCoordinateAxes_PixelShader( 0 );
			pShaderAPI->BindBumpLightmap( SHADER_TEXTURE_STAGE1 );
		}
		Draw();
	}
END_SHADER
