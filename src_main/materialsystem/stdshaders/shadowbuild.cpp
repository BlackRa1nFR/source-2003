//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: A shader that builds the shadow using render-to-texture
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "VMatrix.h"

BEGIN_VS_SHADER( ShadowBuild,
			  "Help for ShadowBuild" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( TRANSLUCENT_MATERIAL, SHADER_PARAM_TYPE_MATERIAL, "", "Points to a material to grab translucency from" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
			return "ShadowBuild_DX6";
		return 0;
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture(BASETEXTURE);
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			// Add the alphas into the frame buffer
			EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION | VERTEX_BONE_INDEX, 0, 0, 3, 0 );
			pShaderShadow->SetVertexShader( "UnlitGeneric" );
			pShaderShadow->SetPixelShader( "ShadowBuildTexture" );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->FogMode( MATERIAL_FOG_NONE );
			SetModulationVertexShaderDynamicState();

			// Snack important parameters from the original material
			// FIXME: What about alpha modulation? Need a solution for that
			ITexture *pTexture = NULL;
			IMaterialVar **ppTranslucentParams = NULL;
			if (params[TRANSLUCENT_MATERIAL]->IsDefined())
			{
				IMaterial *pMaterial = params[TRANSLUCENT_MATERIAL]->GetMaterialValue();
				if (pMaterial)
				{
					ppTranslucentParams = pMaterial->GetShaderParams();
					if ( ppTranslucentParams[BASETEXTURE]->IsDefined() )
					{
						pTexture = ppTranslucentParams[BASETEXTURE]->GetTextureValue();
					}
				}
			}

			if (pTexture)
			{
				BindTexture( SHADER_TEXTURE_STAGE0, pTexture, ppTranslucentParams[FRAME]->GetIntValue() );

				Vector4D transformation[2];
				const VMatrix &mat = ppTranslucentParams[BASETEXTURETRANSFORM]->GetMatrixValue();
				transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
				transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
				pShaderAPI->SetVertexShaderConstant( 90, transformation[0].Base(), 2 ); 
			}
			else
			{
				pShaderAPI->BindWhite( SHADER_TEXTURE_STAGE0 );
			}
		}
		Draw( );
	}
END_SHADER
