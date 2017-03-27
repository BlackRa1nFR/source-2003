//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "VMatrix.h"

DEFINE_FALLBACK_SHADER( Water, Water_DX81 )

BEGIN_VS_SHADER( Water_DX81, 
			  "Help for Water_DX81" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( REFRACTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( REFLECTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_WaterReflection", "" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
		SHADER_PARAM( REFLECTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFLECTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "reflection tint" )
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
		if( g_pHardwareConfig->GetDXSupportLevel() < 81 )
		{
			return "Water_DX80";
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
		if (params[ENVMAP]->IsDefined() )
		{
			LoadTexture( ENVMAP );
		}
		if (params[NORMALMAP]->IsDefined() )
		{
			LoadBumpMap( NORMALMAP );
		}
	}

	inline int GetReflectionRefractionPixelShaderIndex( bool bReflection, bool bRefraction )
	{
		// "REFLECT" "0..1"
		// "REFRACT" "0..1"
		int pshIndex = 
			( bReflection ? 1 : 0 ) |
			( bReflection ? 2 : 0 );				
		return pshIndex;
	}

	inline void DrawReflectionRefraction( IMaterialVar **params, IShaderShadow* pShaderShadow,
		IShaderDynamicAPI* pShaderAPI, bool bReflection, bool bRefraction ) 
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			if( bRefraction )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			}
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			if( bReflection )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
			}
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "Water_ps14" );
			int pshIndex = GetReflectionRefractionPixelShaderIndex( bReflection, bRefraction );
			pShaderShadow->SetPixelShader ( "Water_ps14", pshIndex );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE1 );
			if( bRefraction )
			{
				BindTexture( SHADER_TEXTURE_STAGE2, REFRACTTEXTURE, -1 );
			}
			BindTexture( SHADER_TEXTURE_STAGE3, NORMALMAP, BUMPFRAME );
			if( bReflection )
			{
				BindTexture( SHADER_TEXTURE_STAGE4, REFLECTTEXTURE, -1 );
			}

			pShaderAPI->SetVertexShaderIndex( 0 );
			
			SetVertexShaderConstant( 90, REFLECTAMOUNT );
			SetVertexShaderTextureTransform( 91, BUMPTRANSFORM );
			SetVertexShaderConstant( 93, REFRACTAMOUNT );
			
			float c0[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 0, c0, 1 );
			
			SetPixelShaderConstant( 1, REFRACTTINT );
			SetPixelShaderConstant( 4, REFLECTTINT );
			
			float c2[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
			pShaderAPI->SetPixelShaderConstant( 2, c2, 1 );
			
			// ERASE ME!
			float c3[4] = { 5.0f, 0.0f, 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 3, c3, 1 );

			// reflection/refraction scale
			float reflectionRefractionScale[4] = { params[REFLECTAMOUNT]->GetFloatValue(), 
				params[REFRACTAMOUNT]->GetFloatValue(), 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 5, reflectionRefractionScale, 1 );
			pShaderAPI->SetVertexShaderConstant( 94, reflectionRefractionScale, 1 );


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
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
			if( bBlend )
			{
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
				VERTEX_TANGENT_T, 1, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "WaterCheap_vs14" );
			pShaderShadow->SetPixelShader( "WaterCheap_ps14" );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			BindTexture( SHADER_TEXTURE_STAGE0, NORMALMAP, BUMPFRAME );
			BindTexture( SHADER_TEXTURE_STAGE3, ENVMAP, ENVMAPFRAME );
			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE4 );
			
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
		// FIXME: make a dx8 version of DrawCheapWater.
		bool bForceCheap = params[FORCECHEAP]->GetIntValue() != 0 || g_pConfig->bEditMode;
		bool bForceExpensive = params[FORCEEXPENSIVE]->GetIntValue() != 0 || g_pConfig->bEditMode;
		Assert( !( bForceCheap && bForceExpensive ) );
		bool bReflection = params[REFLECTTEXTURE]->IsDefined();
		bool bRefraction = params[REFRACTTEXTURE]->IsDefined();
		if( !bForceCheap )
		{
			DrawReflectionRefraction( params, pShaderShadow, pShaderAPI, bReflection, bRefraction );
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

