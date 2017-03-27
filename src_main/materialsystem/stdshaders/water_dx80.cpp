//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "VMatrix.h"

BEGIN_VS_SHADER( Water_DX80, 
			  "Help for Water_DX80" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( REFRACTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( REFLECTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_WaterReflection", "" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( REFLECTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFLECTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "reflection tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "", "dudv bump map" )
		SHADER_PARAM( NORMALMAP, SHADER_PARAM_TYPE_TEXTURE, "", "normal map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( SCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "" )
		SHADER_PARAM( TIME, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( WATERDEPTH, SHADER_PARAM_TYPE_FLOAT, "", "" )
		SHADER_PARAM( CHEAPWATERSTARTDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should start transitioning to a cheaper water shader." )
		SHADER_PARAM( CHEAPWATERENDDISTANCE, SHADER_PARAM_TYPE_FLOAT, "", "This is the distance from the eye in inches that the shader should finish transitioning to a cheaper water shader." )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "env_cubemap", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( FOGCOLOR, SHADER_PARAM_TYPE_COLOR, "", "" )
		SHADER_PARAM( FORCECHEAP, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( FORCEEXPENSIVE, SHADER_PARAM_TYPE_BOOL, "", "" )
		SHADER_PARAM( REFLECTENTITIES, SHADER_PARAM_TYPE_BOOL, "", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		if( !params[CHEAPWATERSTARTDISTANCE]->IsDefined() )
		{
			params[CHEAPWATERSTARTDISTANCE]->SetFloatValue( 500.0f );
		}
		if( !params[CHEAPWATERENDDISTANCE]->IsDefined() )
		{
			params[CHEAPWATERENDDISTANCE]->SetFloatValue( 1000.0f );
		}
		if( !params[SCALE]->IsDefined() )
		{
			params[SCALE]->SetVecValue( 1.0f, 1.0f );
		}
		if( !params[FOGCOLOR]->IsDefined() )
		{
			params[FOGCOLOR]->SetVecValue( 1.0f, 0.0f, 0.0f );
			Warning( "material %s needs to have a $fogcolor.\n", pMaterialName );
		}
		if( !params[REFLECTENTITIES]->IsDefined() )
		{
			params[REFLECTENTITIES]->SetIntValue( 0 );
		}
		if( !params[FORCEEXPENSIVE]->IsDefined() )
		{
			params[FORCEEXPENSIVE]->SetIntValue( 0 );
		}
		if( g_pConfig->bEditMode )
		{
			params[ENVMAP]->SetStringValue( "engine/defaultcubemap" );
			params[FORCEEXPENSIVE]->SetIntValue( 0 );
		}
		if( params[FORCEEXPENSIVE]->GetIntValue() && params[FORCECHEAP]->GetIntValue() )
		{
			params[FORCEEXPENSIVE]->SetIntValue( 0 );
		}
		if( !params[FORCEEXPENSIVE]->GetIntValue() && !params[ENVMAP]->IsDefined() )
		{
			params[ENVMAP]->SetStringValue( "engine/defaultcubemap" );
		}
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 80 ||
			!g_pHardwareConfig->HasProjectedBumpEnv() )
		{
			return "Water_DX60";
		}
		return 0;
	}

	SHADER_INIT
	{
		Assert( params[WATERDEPTH]->IsDefined() );
		if( !g_pConfig->bEditMode )
		{
			if( params[REFRACTTEXTURE]->IsDefined() )
			{
				LoadTexture( REFRACTTEXTURE );
			}
			if( params[REFLECTTEXTURE]->IsDefined() )
			{
				LoadTexture( REFLECTTEXTURE );
			}
		}
		if (params[BUMPMAP]->IsDefined() )
		{
			LoadTexture( BUMPMAP );
		}
		if (params[ENVMAP]->IsDefined() )
		{
			LoadTexture( ENVMAP );
		}
		if (params[NORMALMAP]->IsDefined() )
		{
			LoadBumpMap( NORMALMAP );
		}
	}

	inline void DrawReflection( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI, bool bBlendReflection )
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );
			if( bBlendReflection )
			{
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			pShaderShadow->SetVertexShader( "Water_vs11" );
			pShaderShadow->SetPixelShader( "WaterReflect_ps11", 0 );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			pShaderAPI->SetTextureTransformDimension( 1, 2, true );
			float fReflectionAmount = params[REFLECTAMOUNT]->GetFloatValue();
			pShaderAPI->SetBumpEnvMatrix( 1, fReflectionAmount, 0.0f, 0.0f, fReflectionAmount );
			BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP, BUMPFRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, REFLECTTEXTURE, -1 );
			BindTexture( SHADER_TEXTURE_STAGE2, NORMALMAP, BUMPFRAME );
			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE3 );
			pShaderAPI->SetVertexShaderIndex( 0 );

			SetVertexShaderTextureTransform( 91, BUMPTRANSFORM );

			// used to invert y
			float c94[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			pShaderAPI->SetVertexShaderConstant( 94, c94, 1 );

			FogToFogColor();
		}
		Draw();
	}
	
	inline void DrawRefraction( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI )
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "Water_vs11" );
			pShaderShadow->SetPixelShader( "WaterRefract_ps11", 0 );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			pShaderAPI->SetTextureTransformDimension( 1, 2, true );
			float fRefractionAmount = params[REFRACTAMOUNT]->GetFloatValue();
			pShaderAPI->SetBumpEnvMatrix( 1, fRefractionAmount, 0.0f, 0.0f, fRefractionAmount );
			BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP, BUMPFRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, REFRACTTEXTURE, -1 );
			BindTexture( SHADER_TEXTURE_STAGE2, NORMALMAP, BUMPFRAME );
			pShaderAPI->SetVertexShaderIndex( 0 );

			SetVertexShaderTextureTransform( 91, BUMPTRANSFORM );

			// used to invert y
			float c94[4] = { 0.0f, 0.0f, 0.0f, -1.0f };
			pShaderAPI->SetVertexShaderConstant( 94, c94, 1 );

			FogToFogColor();
		}
		Draw();
	}

	inline void DrawCheapWater( IMaterialVar **params, IShaderShadow* pShaderShadow, 
		                        IShaderDynamicAPI* pShaderAPI, bool bBlend )
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			if( bBlend )
			{
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
				VERTEX_TANGENT_T, 1, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "WaterCheap_vs11" );
			pShaderShadow->SetPixelShader( "WaterCheap_ps11" );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			BindTexture( SHADER_TEXTURE_STAGE0, NORMALMAP, BUMPFRAME );
			BindTexture( SHADER_TEXTURE_STAGE3, ENVMAP, ENVMAPFRAME );
			
			SetEnvMapTintPixelShaderDynamicState( 0, REFLECTTINT, ALPHA );
			float cheapWaterStartDistance = params[CHEAPWATERSTARTDISTANCE]->GetFloatValue();
			float cheapWaterEndDistance = params[CHEAPWATERENDDISTANCE]->GetFloatValue();
			float cheapWaterConstants[4] = 
			{ 
				cheapWaterStartDistance, 
				1.0f / ( cheapWaterEndDistance - cheapWaterStartDistance ), 0.0f, 0.0f 
			};
			pShaderAPI->SetVertexShaderConstant( 92, cheapWaterConstants );

			SetPixelShaderConstant( 0, FOGCOLOR );
			SetPixelShaderConstant( 1, REFLECTTINT );
			SetPixelShaderConstant( 2, REFRACTTINT );
	
			FogToFogColor();
			SetVertexShaderTextureTransform( 90, BUMPTRANSFORM );
		}
		Draw();
	}

	SHADER_DRAW
	{
		// FIXME: for dx9, put this whole shader into a single pass
		// FIXME: make a dx8 version of DrawCheapWater.
		bool blendReflection = false;
		bool bForceCheap = params[FORCECHEAP]->GetIntValue() != 0 || g_pConfig->bEditMode;
		bool bForceExpensive = params[FORCEEXPENSIVE]->GetIntValue() != 0 || g_pConfig->bEditMode;
		Assert( !( bForceCheap && bForceExpensive ) );
		bool bReflection = params[REFLECTTEXTURE]->IsDefined();
		bool bRefraction = params[REFRACTTEXTURE]->IsDefined();
		if( !bForceCheap )
		{
			if( bRefraction )
			{
				DrawRefraction( params, pShaderShadow, pShaderAPI );
				blendReflection = true;
			}
			if( bReflection )
			{
				DrawReflection( params, pShaderShadow, pShaderAPI, blendReflection );
			}
		}

		// Use $decal to see if we are a decal or not. . if we are, then don't bother
		// drawing the cheap version for now since we don't have access to env_cubemap
		if( !bForceExpensive && params[ENVMAP]->IsDefined() && 
			!IS_FLAG_SET( MATERIAL_VAR_DECAL ) )
		{
			DrawCheapWater( params, pShaderShadow, pShaderAPI, !bForceCheap );
		}
	}
END_SHADER

