//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

#define MAXBLUR 1

DEFINE_FALLBACK_SHADER( Refract, Refract_DX80 )

BEGIN_VS_SHADER( Refract_DX80, 
			  "Help for Refract_DX80" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( DUDVMAP, SHADER_PARAM_TYPE_TEXTURE, "", "dudv bump map" )
		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "", "normal map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( DUDVFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $dudvmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( BLURAMOUNT, SHADER_PARAM_TYPE_INTEGER, "", "0, 1, or 2 for how much blur you want" )
		SHADER_PARAM( FADEOUTONSILHOUETTE, SHADER_PARAM_TYPE_BOOL, "", "0 for no fade out on silhouette, 1 for fade out on sillhouette" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "envmap frame number" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
		SHADER_PARAM( REFRACTTINTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( REFRACTTINTTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( FRESNELREFLECTION, SHADER_PARAM_TYPE_FLOAT, "0.0", "0.0 == no fresnel, 1.0 == full fresnel" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
		if( !params[ENVMAPTINT]->IsDefined() )
		{
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[ENVMAPCONTRAST]->IsDefined() )
		{
			params[ENVMAPCONTRAST]->SetFloatValue( 0.0f );
		}
		if( !params[ENVMAPSATURATION]->IsDefined() )
		{
			params[ENVMAPSATURATION]->SetFloatValue( 1.0f );
		}
		if( !params[ENVMAPFRAME]->IsDefined() )
		{
			params[ENVMAPFRAME]->SetIntValue( 0 );
		}
		if( !params[FRESNELREFLECTION]->IsDefined() )
		{
			params[FRESNELREFLECTION]->SetFloatValue( 1.0f );
		}
	}

	bool NeedsFrameBufferTexture( IMaterialVar **params ) const
	{
		return true;
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 80 )
			return "Wireframe";

		return 0;
	}

	SHADER_INIT
	{
		if (params[DUDVMAP]->IsDefined() )
		{
			LoadTexture( DUDVMAP );
		}
		if (params[NORMALMAP]->IsDefined() )
		{
			LoadBumpMap( NORMALMAP );
		}
		if( params[ENVMAP]->IsDefined() )
		{
			LoadCubeMap( ENVMAP );
		}
		if( params[REFRACTTINTTEXTURE]->IsDefined() )
		{
			LoadTexture( REFRACTTINTTEXTURE );
		}
		if( !params[BLURAMOUNT]->IsDefined() )
		{
			params[BLURAMOUNT]->SetIntValue( 0 );
		}
		if( !params[FADEOUTONSILHOUETTE]->IsDefined() )
		{
			params[FADEOUTONSILHOUETTE]->SetIntValue( 0 );
		}
	}

	inline int ComputePixelShaderIndex( bool bRefractTintTexture )
	{
		// "REFRACTTINTTEXTURE" "0..1"
		int pshIndex = 0;
		if( bRefractTintTexture ) pshIndex |= 1;
		return pshIndex;
	}
	
	SHADER_DRAW
	{
		bool bIsModel = IS_FLAG_SET( MATERIAL_VAR_MODEL );
		bool bHasEnvmap = params[ENVMAP]->IsDefined();
		int blurAmount = params[BLURAMOUNT]->GetIntValue();
		bool bRefractTintTexture = params[REFRACTTINTTEXTURE]->IsDefined();
		if( blurAmount < 0 )
		{
			blurAmount = 0;
		}
		else if( blurAmount > MAXBLUR )
		{
			blurAmount = MAXBLUR;
		}

		SHADOW_STATE
		{
			// dudv map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			// renderable texture for refraction
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			if( bRefractTintTexture )
			{
				// refract tint texture
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			}
			int fmt = VERTEX_POSITION | VERTEX_NORMAL;
			int numBoneWeights = 0;
			int userDataSize = 0;
			if( bIsModel )
			{
				numBoneWeights = 3;
				userDataSize = 4;
				fmt |= VERTEX_BONE_INDEX;
			}
			else
			{
				fmt |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			}
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, numBoneWeights, userDataSize );

			if( bIsModel )
			{
				pShaderShadow->SetVertexShader( "Refract_model_vs11" );
			}
			else
			{
				pShaderShadow->SetVertexShader( "Refract_world_vs11" );
			}

			int pshIndex = 0;
			pshIndex = ComputePixelShaderIndex( bRefractTintTexture );
			pShaderShadow->SetPixelShader( "Refract_ps11", pshIndex );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			pShaderAPI->SetTextureTransformDimension( 1, 2, true );

			if ( params[DUDVFRAME]->GetIntValue() == 0 )
			{
				BindTexture( SHADER_TEXTURE_STAGE0, DUDVMAP, BUMPFRAME );
			}
			else
			{
				BindTexture( SHADER_TEXTURE_STAGE0, DUDVMAP, DUDVFRAME );
			}

			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE1 );

			if( bRefractTintTexture )
			{
				BindTexture( SHADER_TEXTURE_STAGE2, REFRACTTINTTEXTURE, REFRACTTINTTEXTUREFRAME );
			}

			float fRefractionAmount = params[REFRACTAMOUNT]->GetFloatValue();
			pShaderAPI->SetBumpEnvMatrix( 1, fRefractionAmount, 0.0f, 0.0f, fRefractionAmount );

			SetVertexShaderTextureTransform( 91, BUMPTRANSFORM );

			// used to invert y
			float c94[4] = { 0.0f, 0.0f, 0.0f, -1.0f };
			pShaderAPI->SetVertexShaderConstant( 94, c94, 1 );

			SetPixelShaderConstant( 0, REFRACTTINT );

			FogToFogColor();
		}
		Draw();

		if( bHasEnvmap )
		{
			const bBlendSpecular = true;
			if( bIsModel )
			{
				DrawModelBumpedSpecularLighting( NORMALMAP, BUMPFRAME,
					ENVMAP, ENVMAPFRAME,
					ENVMAPTINT, ALPHA,
					ENVMAPCONTRAST, ENVMAPSATURATION,
					BUMPTRANSFORM,
					bBlendSpecular );
			}
			else
			{
				DrawWorldBumpedSpecularLighting( NORMALMAP, ENVMAP,
					BUMPFRAME, ENVMAPFRAME,
					ENVMAPTINT, ALPHA,
					ENVMAPCONTRAST, ENVMAPSATURATION,
					BUMPTRANSFORM, FRESNELREFLECTION,
					bBlendSpecular );
			}
		}
	}
END_SHADER

