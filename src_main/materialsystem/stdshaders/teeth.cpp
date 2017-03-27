//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Teeth renderer
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( Teeth, 
			  "Help for Teeth" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ILLUMFACTOR, SHADER_PARAM_TYPE_FLOAT, "1", "Amount to darken or brighten the teeth" )
		SHADER_PARAM( FORWARD, SHADER_PARAM_TYPE_VEC3, "[1 0 0]", "Forward direction vector for teeth lighting" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "envmap frame number" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTimesLightmapPlusMaskedCubicEnvMap_glass", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$envmapmask texcoord transform" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "0.0", "contrast 0 == normal 1 == color*color" )
		SHADER_PARAM( ENVMAPSATURATION, SHADER_PARAM_TYPE_FLOAT, "1.0", "saturation 0 == greyscale 1 == normal" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// default to 'MODEL' mode...
		if (!IS_FLAG_DEFINED( MATERIAL_VAR_MODEL ))
			SET_FLAGS( MATERIAL_VAR_MODEL );

		if( !params[ENVMAPMASKFRAME]->IsDefined() )
			params[ENVMAPMASKFRAME]->SetIntValue( 0 );
		
		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[ENVMAPFRAME]->IsDefined() )
			params[ENVMAPFRAME]->SetIntValue( 0 );

		if( !params[BUMPFRAME]->IsDefined() )
			params[BUMPFRAME]->SetIntValue( 0 );

		if( !params[ENVMAPCONTRAST]->IsDefined() )
			params[ENVMAPCONTRAST]->SetFloatValue( 0.0f );
		
		if( !params[ENVMAPSATURATION]->IsDefined() )
			params[ENVMAPSATURATION]->SetFloatValue( 1.0f );

		if( ( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() ) || params[ENVMAP]->IsDefined() )
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
			bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
			if( hasNormalMapAlphaEnvmapMask )
			{
				params[ENVMAPMASK]->SetUndefined();
				CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}
		else
		{
			CLEAR_FLAGS( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
		}
		
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
	}

	SHADER_FALLBACK
	{	
		// FIXME FIXME!!!  E3 hack!  Disable the dx9 version of Teeth since it's broken
		// and we aren't using any of it's features.
		return "Teeth_DX8";
		bool isDX9 = g_pHardwareConfig->SupportsVertexShaders_2_0() && 
			         g_pHardwareConfig->SupportsPixelShaders_2_0();
		if( !isDX9 )
		{
			return "Teeth_DX8";
		}
		return 0;
	}


	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		if ( g_pConfig->bBumpmap && params[BUMPMAP]->IsDefined() )
		{
			LoadBumpMap( BUMPMAP );
		}
		if ( params[ENVMAP]->IsDefined() )
		{
			LoadCubeMap( ENVMAP );
		}
		if ( params[ENVMAPMASK]->IsDefined() )
		{
			LoadTexture( ENVMAPMASK );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );
		
			int userDataSize = 0;
			if( params[ENVMAP]->IsDefined() )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
				userDataSize = 4; // tangent S
			}

			if( params[BUMPMAP]->IsDefined() )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
				userDataSize = 4; // tangent S
			}

			if( params[ENVMAPMASK]->IsDefined() )
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
			}
		
			pShaderShadow->SetVertexShader( "Teeth_vs20" );
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX, 1, 0, 3, userDataSize );

			int pshIndex = ( 1 << 0 ) | ( 1 << 4 );
			if( params[BUMPMAP]->IsDefined() )	pshIndex |= ( 1 << 2 );
			if( params[ENVMAP]->IsDefined() )	pshIndex |= ( 1 << 3 );
			if( params[ENVMAPMASK]->IsDefined() ) pshIndex |= ( 1 << 5 );
			if( IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK ) )		pshIndex |= ( 1 << 6 );
			if( IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK ) )	pshIndex |= ( 1 << 8 );
			
			pShaderShadow->SetPixelShader( "vertexlit_and_unlit_generic_ps20", pshIndex );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			SetVertexShaderTextureTransform( 90, BASETEXTURETRANSFORM );

			if( params[ENVMAP]->IsDefined() )
			{
				BindTexture( SHADER_TEXTURE_STAGE1, ENVMAP, ENVMAPFRAME );
			}

			bool hasBump = false;
			if( params[BUMPMAP]->IsDefined() )
			{
				hasBump = true;
				if( !g_pConfig->m_bFastNoBump )
				{
					BindTexture( SHADER_TEXTURE_STAGE3, BUMPMAP, BUMPFRAME );
					SetVertexShaderTextureTransform( 92, BUMPTRANSFORM );
				}
				else
				{
					pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE3 );
				}
			}

			if( params[ENVMAPMASK]->IsDefined() )
			{
				BindTexture( SHADER_TEXTURE_STAGE4, ENVMAPMASK, ENVMAPMASKFRAME );
				SetVertexShaderTextureTransform( 94, ENVMAPMASKTRANSFORM );
			}

			SetAmbientCubeDynamicStateVertexShader();
			
			Vector4D lighting;
			params[FORWARD]->GetVecValue( lighting.Base(), 3 );
			lighting[3] = params[ILLUMFACTOR]->GetFloatValue();
			pShaderAPI->SetVertexShaderConstant( 20, lighting.Base() );

			int nLightCombo = pShaderAPI->GetCurrentLightCombo();
			int nFogIndex = ( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			int nNumBones = pShaderAPI->GetCurrentNumBones();
			int nVshIndex = nLightCombo + ( 22 * nFogIndex ) + 
										( 22 * 2 * nNumBones ) + 
					( hasBump ?           22 * 2 * 4 : 0 );

			pShaderAPI->SetVertexShaderIndex( nVshIndex );

			SetEnvMapTintPixelShaderDynamicState( 0, ENVMAPTINT, -1 );
			SetModulationPixelShaderDynamicState( 1 );
			SetPixelShaderConstant( 2, ENVMAPCONTRAST );
			SetPixelShaderConstant( 3, ENVMAPSATURATION );

			FogToFogColor();
		}
		Draw();
	}
END_SHADER
