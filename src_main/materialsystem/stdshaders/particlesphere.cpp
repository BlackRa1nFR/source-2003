//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: A wet version of base * lightmap
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"


// This is what 

#define PARTICLESPHERE_LIGHTINFO_FOURCC MAKE_MATERIALVAR_FOURCC('P','S','L','I')
class CParticleSphereLightInfo
{
public:
	Vector		*m_pPositions;
	float		*m_pIntensities;
	int			m_nLights;
};


BEGIN_VS_SHADER( ParticleSphere, 
			  "Help for BumpmappedEnvMap" )
			   
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( USINGPIXELSHADER, SHADER_PARAM_TYPE_BOOL, "0", "Tells to client code whether the shader is using DX8 vertex/pixel shaders or not" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "base texture" )
		SHADER_PARAM( LIGHTS, SHADER_PARAM_TYPE_FOURCC, "", "array of lights" )
		SHADER_PARAM( USEBUMPMAP, SHADER_PARAM_TYPE_BOOL, "1", "set this to false to force the dx7 renderer to be used" )
	END_SHADER_PARAMS

	bool UsePixelShaders( IMaterialVar **params )
	{
		return params[USEBUMPMAP]->GetIntValue() && 
			g_pHardwareConfig->SupportsVertexAndPixelShaders();
	}

	SHADER_INIT
	{
		if( !params[USEBUMPMAP]->IsDefined() )
			params[USEBUMPMAP]->SetIntValue( 1 );

		params[USINGPIXELSHADER]->SetIntValue( (int)UsePixelShaders(params) );
		
		if( UsePixelShaders(params) )
		{
			LoadBumpMap( BUMPMAP );
		}
		else
		{
			LoadTexture( BASETEXTURE );
		}
	}

	SHADER_DRAW
	{
		if( UsePixelShaders(params) )
		{
			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				
				int tCoordDimensions[] = {3};
				pShaderShadow->VertexShaderVertexFormat( 
					VERTEX_POSITION | VERTEX_COLOR | VERTEX_SPECULAR, 1, tCoordDimensions, 0, 0 );

				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->SetVertexShader( "ParticleSphere" );
				pShaderShadow->SetPixelShader( "ParticleSphere" );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP );
				FogToFogColor();

				// VERTEX SHADER CONSTANTS
				float c90[4] = {0, 1, -1, 1};
				pShaderAPI->SetVertexShaderConstant( 90, c90 );

				float c91[4] = {1.0f / 3.0f, 0.0f, 0.0f, 0.0f };
				pShaderAPI->SetVertexShaderConstant( 91, c91 );
				
				// Put the list of lights into registers 45-85.
				IMaterialVar::FourCC type;
				void *pData;
				params[LIGHTS]->GetFourCCValue( &type, &pData );
				if( type == PARTICLESPHERE_LIGHTINFO_FOURCC )
				{
					CParticleSphereLightInfo *pLights = (CParticleSphereLightInfo*)pData;
				
					int nActualLights = min( pLights->m_nLights, 20 );
					for( int i=0; i < nActualLights; i++ )
					{
						float pos[4] = 
						{
							pLights->m_pPositions[i][0],
							pLights->m_pPositions[i][1],
							pLights->m_pPositions[i][2],
							1
						};
						pShaderAPI->SetVertexShaderConstant( 45 + i*2 + 0, pos );

						float halfIntensity[4] = 
						{
							pLights->m_pIntensities[i],
							pLights->m_pIntensities[i],
							pLights->m_pIntensities[i],
							pLights->m_pIntensities[i]
						};
						pShaderAPI->SetVertexShaderConstant( 45 + i*2 + 1, halfIntensity );
					}
				}

				
				// PIXEL SHADER CONSTANTS
				// pixel shader c0 = [0.5, 0.5, 0.5, 0.5]
				float zeroPointFive[4] = {0.5f, 0.5f, 0.5f, 0.5f};
				pShaderAPI->SetPixelShaderConstant( 0, zeroPointFive );
			}
			Draw();
		}
		else
		{
			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, g_pConfig->overbright );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableDepthWrites(false);
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToFogColor();
			}
			Draw();
		}
	}
END_SHADER
