//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

DEFINE_FALLBACK_SHADER( UnlitGeneric, UnlitGeneric_DX8 )

BEGIN_VS_SHADER( UnlitGeneric_DX8,
			  "Help for UnlitGeneric_DX8" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTimesLightmapPlusMaskedCubicEnvMap_glass", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			return "UnlitGeneric_DX6";
		}
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		if( !params[ENVMAPMASKSCALE]->IsDefined() )
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );
		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 4.0f );

		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				if (IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
					CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if ( IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );

		// the second texture (if there is one)
		if (params[DETAIL]->IsDefined())
		{
			LoadTexture( DETAIL );
		}
	
		if (params[ENVMAP]->IsDefined())
		{
			if( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
				LoadCubeMap( ENVMAP );
			else
				LoadTexture( ENVMAP );

			if( !g_pHardwareConfig->SupportsCubeMaps() )
				SET_FLAGS(MATERIAL_VAR_ENVMAPSPHERE);

			if (params[ENVMAPMASK]->IsDefined())
				LoadTexture( ENVMAPMASK );
		}
	}

	SHADER_DRAW
	{
		bool doSkin = IS_FLAG_SET( MATERIAL_VAR_MODEL );
		VertexShaderUnlitGenericPass( 
			doSkin, BASETEXTURE, FRAME, BASETEXTURETRANSFORM,
			DETAIL, DETAILSCALE, true, ENVMAP, ENVMAPFRAME, ENVMAPMASK,
			ENVMAPMASKFRAME, ENVMAPMASKSCALE, ENVMAPTINT );
	}
END_SHADER

