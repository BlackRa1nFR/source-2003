//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "VMatrix.h"
#include "common_hlsl_cpp_consts.h" // hack hack hack!

BEGIN_VS_SHADER( Water, 
			  "Help for Water" )

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
		SHADER_PARAM( FORCECHEAP, SHADER_PARAM_TYPE_BOOL, "", "" )
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
		if( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "Water_DX81";
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

	inline int ComputeReflectionRefractionPixelShaderIndex( bool bReflection, bool bRefraction )
	{
		int pshIndex = 
			( bReflection ? 1 : 0 ) |
			( bRefraction ? 2 : 0 );				
		return pshIndex;
	}
	
	inline void GetVecParam( int constantVar, float *val )
	{
		if( constantVar == -1 )
			return;

		IMaterialVar* pVar = s_ppParams[constantVar];
		Assert( pVar );

		if (pVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pVar->GetVecValue( val, 4 );
		else
			val[0] = val[1] = val[2] = val[3] = pVar->GetFloatValue();
	}

	inline void DrawReflectionRefraction( IMaterialVar **params, IShaderShadow* pShaderShadow,
		IShaderDynamicAPI* pShaderAPI, bool bReflection, bool bRefraction ) 
	{
		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
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
			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE6, true );

			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "Water_vs20" );

#ifdef HDR
			// Enable alpha writes for HDR
			pShaderShadow->EnableAlphaWrites( true );
#endif
			// "REFLECT" "0..1"
			// "REFRACT" "0..1"
			pShaderShadow->SetPixelShader ( "Water_ps20", 
				ComputeReflectionRefractionPixelShaderIndex( bReflection, bRefraction ) );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			if( bRefraction )
			{
				BindTexture( SHADER_TEXTURE_STAGE2, REFRACTTEXTURE, -1 );
			}
			BindTexture( SHADER_TEXTURE_STAGE3, NORMALMAP, BUMPFRAME );
			if( bReflection )
			{
				BindTexture( SHADER_TEXTURE_STAGE4, REFLECTTEXTURE, -1 );
			}
			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE6 );

			pShaderAPI->SetVertexShaderIndex( 0 );
			
			float tint[4];
			// Refraction tint
			if( bRefraction )
			{
				GetVecParam( REFRACTTINT, tint );
				tint[3] = tint[0] * 0.30f + tint[1] * 0.59f + tint[2] * 0.11f;
				pShaderAPI->SetPixelShaderConstant( 1, tint, 1 );
			}
			// Reflection tint
			if( bReflection )
			{
				GetVecParam( REFLECTTINT, tint );
				tint[3] = tint[0] * 0.30f + tint[1] * 0.59f + tint[2] * 0.11f;
				pShaderAPI->SetPixelShaderConstant( 4, tint, 1 );
			}

			SetVertexShaderTextureTransform( 91, BUMPTRANSFORM );
			
			float c0[4] = { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 0, c0, 1 );
			
			float c2[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
			pShaderAPI->SetPixelShaderConstant( 2, c2, 1 );
			
			// fresnel constants
			float c3[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 3, c3, 1 );

			float c5[4] = { params[REFLECTAMOUNT]->GetFloatValue(), params[REFLECTAMOUNT]->GetFloatValue(), 
				params[REFRACTAMOUNT]->GetFloatValue(), params[REFRACTAMOUNT]->GetFloatValue() };
			pShaderAPI->SetPixelShaderConstant( 5, c5, 1 );

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
			if( bBlend )
			{
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			// envmap
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			// normal map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			// Normalizing cube map
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE6, true );
			int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S | VERTEX_TANGENT_T;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );
			pShaderShadow->SetVertexShader( "WaterCheap_vs20" );
			pShaderShadow->SetPixelShader( "WaterCheap_ps20", bBlend ? 1 : 0 );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();
			BindTexture( SHADER_TEXTURE_STAGE0, ENVMAP, ENVMAPFRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, NORMALMAP, BUMPFRAME );
			pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE6 );
			SetPixelShaderConstant( 0, FOGCOLOR );
			float cheapWaterStartDistance = params[CHEAPWATERSTARTDISTANCE]->GetFloatValue();
			float cheapWaterEndDistance = params[CHEAPWATERENDDISTANCE]->GetFloatValue();
			float cheapWaterParams[4] = 
			{
				cheapWaterStartDistance * VSHADER_VECT_SCALE,
				cheapWaterEndDistance * VSHADER_VECT_SCALE,
				PSHADER_VECT_SCALE / ( cheapWaterEndDistance - cheapWaterStartDistance ),
				cheapWaterStartDistance / ( cheapWaterEndDistance - cheapWaterStartDistance ),
			};
			pShaderAPI->SetPixelShaderConstant( 1, cheapWaterParams );
			pShaderAPI->SetVertexShaderIndex( 0 );
			FogToFogColor();
		}
		Draw();
	}

	SHADER_DRAW
	{
		// TODO: fit the cheap water stuff into the water shader so that we don't have to do
		// 2 passes.
		bool bForceCheap = params[FORCECHEAP]->GetIntValue() != 0 || g_pConfig->bEditMode;
		bool bForceExpensive = params[FORCEEXPENSIVE]->GetIntValue() != 0;
		Assert( !( bForceCheap && bForceExpensive ) );
		bool bReflection = params[REFLECTTEXTURE]->IsDefined();
		bool bRefraction = params[REFRACTTEXTURE]->IsDefined();
		bool bDrewSomething = false;
		if( !bForceCheap )
		{
			bDrewSomething = true;
			DrawReflectionRefraction( params, pShaderShadow, pShaderAPI, bReflection, bRefraction );
		}

		// Use $decal to see if we are a decal or not. . if we are, then don't bother
		// drawing the cheap version for now since we don't have access to env_cubemap
		if( !bForceExpensive && params[ENVMAP]->IsDefined() && 
			!IS_FLAG_SET( MATERIAL_VAR_DECAL ) )
		{
			bDrewSomething = true;
			DrawCheapWater( params, pShaderShadow, pShaderAPI, !bForceCheap );
		}

		if( !bDrewSomething )
		{
			// We are likely here because of the tools. . . draw something so that 
			// we won't go into wireframe-land.
			Draw();
		}
	}
END_SHADER

