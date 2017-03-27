//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( DebugNormalMap, 
			  "Help for DebugNormalMap" )
			  
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
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			bool bDoSkin = IS_FLAG_SET( MATERIAL_VAR_MODEL );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			int fmt = VERTEX_POSITION;
			if (bDoSkin)
			{
				fmt |= VERTEX_BONE_INDEX;
			}
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, bDoSkin ? 3 : 0, 0 );
			pShaderShadow->SetVertexShader( "unlitgeneric" );
			pShaderShadow->SetPixelShader( "unlitgeneric" );
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
		}
		Draw();
	}
END_SHADER

