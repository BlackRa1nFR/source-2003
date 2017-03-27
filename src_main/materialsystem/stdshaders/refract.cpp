//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

#define MAXBLUR 1

BEGIN_VS_SHADER( Refract, 
			  "Help for Refract" )

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
// FIXME: doesn't support fresnel!
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
		if( g_pHardwareConfig->GetDXSupportLevel() < 82 )
			return "Refract_DX80";

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

	inline int ComputePixelShaderIndex( int blurAmount, bool bFadeOutOnSilhouette,
		bool bHasEnvmap, bool bRefractTintTexture )
	{
		// "BLUR" "0..1"
		// "FADEOUTONSILHOUETTE" "0..1"
		// "CUBEMAP" "0..1"
		// "REFRACTTINTTEXTURE" "0..1"

		int pshIndex = 0;
		if( blurAmount )			pshIndex |= ( 1 << 0 );
		if( bFadeOutOnSilhouette )	pshIndex |= ( 1 << 1 );
		if( bHasEnvmap )			pshIndex |= ( 1 << 2 );
		if( bRefractTintTexture )	pshIndex |= ( 1 << 3 );
		return pshIndex;
	}
	
	SHADER_DRAW
	{
		bool bIsModel = IS_FLAG_SET( MATERIAL_VAR_MODEL );
		bool bHasEnvmap = params[ENVMAP]->IsDefined();
		bool bRefractTintTexture = params[REFRACTTINTTEXTURE]->IsDefined();
		bool bFadeOutOnSilhouette = params[FADEOUTONSILHOUETTE]->GetIntValue() != 0;
		int blurAmount = params[BLURAMOUNT]->GetIntValue();
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
			SetInitialShadowState( );
			// renderable texture for refraction
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			// normal map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			if( bHasEnvmap )
			{
				// envmap
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
			}
			if( bRefractTintTexture )
			{
				// refract tint texture
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE5, true );
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
			pShaderShadow->SetVertexShader( "Refract_vs20" );
			int pshIndex = ComputePixelShaderIndex( blurAmount != 0, 
				bFadeOutOnSilhouette,
				bHasEnvmap, 
				bRefractTintTexture );
			pShaderShadow->SetPixelShader( "Refract_ps20", pshIndex );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();

			if ( params[DUDVFRAME]->GetIntValue() == 0 )
			{
				BindTexture( SHADER_TEXTURE_STAGE0, DUDVMAP, BUMPFRAME );
			}
			else
			{
				BindTexture( SHADER_TEXTURE_STAGE0, DUDVMAP, DUDVFRAME );
			}

			pShaderAPI->BindFBTexture( SHADER_TEXTURE_STAGE2 );
			BindTexture( SHADER_TEXTURE_STAGE3, NORMALMAP, BUMPFRAME );
			if( bHasEnvmap )
			{
				BindTexture( SHADER_TEXTURE_STAGE4, ENVMAP, ENVMAPFRAME );
			}
			if( bRefractTintTexture )
			{
				BindTexture( SHADER_TEXTURE_STAGE5, REFRACTTINTTEXTURE, REFRACTTINTTEXTUREFRAME );
			}

			int numBones	= pShaderAPI->GetCurrentNumBones();

			//	"NUM_BONES"				"0..3"
			//  "MODEL"					"0..1"
			int vshIndex = bIsModel * 4 + numBones;
			pShaderAPI->SetVertexShaderIndex( vshIndex );

			SetVertexShaderTextureTransform( 91, BUMPTRANSFORM );

			SetPixelShaderConstant( 0, ENVMAPTINT );
			SetPixelShaderConstant( 1, REFRACTTINT );
			SetPixelShaderConstant( 2, ENVMAPCONTRAST );
			SetPixelShaderConstant( 3, ENVMAPSATURATION );
			float c5[4] = { params[REFRACTAMOUNT]->GetFloatValue(), 
				params[REFRACTAMOUNT]->GetFloatValue(), 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 5, c5, 1 );

			FogToFogColor();
		}
		Draw();
	}
END_SHADER

