//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

BEGIN_VS_SHADER( DebugBumpedVertexLit, 
			  "Help for DebugBumpedVertexLit" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( HASSELFILLUM, SHADER_PARAM_TYPE_INTEGER, "0", "Do we have self-illumination?" )
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
			return "DebugVertexLit";
		}
		return 0;
	}

	SHADER_DRAW
	{
		if( g_pHardwareConfig->GetDXSupportLevel() >= 90 ) 
		{
			bool bHasSelfIllum = (params[HASSELFILLUM]->GetIntValue() != 0);

			SHADOW_STATE
			{
				pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
				int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX;
				pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );
				pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 3, 4 /* userDataSize */ );

				pShaderShadow->SetVertexShader( "VertexLit_and_unlit_Generic_vs20" );

				int nPshIndex = 0;
				if( true /*bHasBump*/ )		nPshIndex |= ( 1 << 0 );
				if( true /*bIsVertexLit*/ )	nPshIndex |= ( 1 << 1 );
				if( bHasSelfIllum )			nPshIndex |= ( 1 << 2 );
				pShaderShadow->SetPixelShader( "VertexLit_Lighting_Only_ps20", nPshIndex );
			}
			DYNAMIC_STATE
			{
				int nVshIndex = ComputeVertexLitShaderIndex( true, true, false, false, true );

//				BindTexture( SHADER_TEXTURE_STAGE0, info.m_nBaseTextureVar, info.m_nBaseTextureFrameVar );
				if( params[BUMPMAP]->IsDefined() )
				{
					BindTexture( SHADER_TEXTURE_STAGE1, BUMPMAP, BUMPFRAME );
				}
				else
				{
					pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE1 );
				}

				SetAmbientCubeDynamicStateVertexShader();
				SetVertexShaderTextureTransform( 92, BUMPTRANSFORM );
				pShaderAPI->SetVertexShaderIndex( nVshIndex );
				EnablePixelShaderOverbright( 4, true, false );
				SetPixelShaderConstant( 5, SELFILLUMTINT );
			}
			Draw();
		}
		else
		{
			SHADOW_STATE
			{
				pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
				int fmt = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX;
				pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 3, 4 /* userDataSize */ );

				pShaderShadow->SetVertexShader( "VertexLitGeneric_DiffBumpTimesBase" );
				if( g_pConfig->overbright == 1.0f )
				{
					pShaderShadow->SetPixelShader( "VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright1" );
				}
				else
				{
					pShaderShadow->SetPixelShader( "VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright2" );
				}
			}
			DYNAMIC_STATE
			{
				if( params[BUMPMAP]->IsDefined() )
				{
					BindTexture( SHADER_TEXTURE_STAGE1, BUMPMAP, BUMPFRAME );
				}
				else
				{
 					pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE1 );
				}

				SetAmbientCubeDynamicStateVertexShader();
				LoadBumpLightmapCoordinateAxes_VertexShader( 90 );
				LoadBumpLightmapCoordinateAxes_PixelShader( 0 );
			}
			Draw();
		}
	}
END_SHADER
