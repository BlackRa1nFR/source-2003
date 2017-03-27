//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

// FIXME: Need to make a dx9 version so that "CENTROID" works.

BEGIN_SHADER( DebugLightingOnly, 
			  "Help for DebugLightingOnly" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldDiffuseBumpMap_bump", "bump map" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
//		LoadBumpMap( BUMPMAP );
	}

	SHADER_DRAW
	{
		if( params[BUMPMAP] && g_pConfig->bBumpmap )
		{
			ShaderWarning( "DebugLightingOnly not implemented for diffuse bumpmapping\n" );
#if 0
			// This is for bumpmapped rendering with lightmaps
			InitNVForBumpDotConstantVectorTimesLightmap( g_pConfig->overbright );
			// texture0 = bumpmap
			// texture1 = lightmap
			// primaryColor = light vector
			
			// texture 0 = bumpmap
			pShaderShadow->SetCurrentTextureUnit( 0 );
			pShaderShadow->BindTexture( *m_params[BUMPMAP] );
			pShaderShadow->EnableTexture( true );
			
			// texture1 = lightmap
			pShaderShadow->SetCurrentTextureUnit( 1 );
			pShaderShadow->BindLightmap();
			pShaderShadow->EnableTexture( true );
			pShaderShadow->SetCurrentTextureUnit( 0 );
			glActiveTextureARB( GL_TEXTURE0_ARB );
			
			float thing[3];
			
			// Draw the first pass.
			int pass;
			
			for( pass = 0; pass < NUM_BUMP_VECTS; pass++ )
			{
				thing[0] = ( g_localBumpBasis[pass][0] * .5f ) + .5f;
				thing[1] = ( g_localBumpBasis[pass][1] * .5f ) + .5f;
				thing[2] = ( g_localBumpBasis[pass][2] * .5f ) + .5f;
				glColor3fv( thing );
				
				glBegin( GL_TRIANGLES );
				int i;
				for( i = 0; i < numIndices; i++ )
				{
					int index = vertexArrayData->pIndicesArray[i];
					glTexCoord2f( 
						vertexArrayData->pTexCoordArray[2*index],
						vertexArrayData->pTexCoordArray[2*index+1] );
					glMultiTexCoord2fARB( GL_TEXTURE1_ARB, 
						vertexArrayData->pLightmapTexCoordArray[pass+1][2 * index],
						vertexArrayData->pLightmapTexCoordArray[pass+1][2 * index+1] );
					glVertex3fv( vertexArrayData->pPositionArray + 4 * index );
				}
				glEnd();
				
				glEnable( GL_BLEND );
				glBlendFunc( GL_ONE, GL_ONE );
			}
			
			glActiveTextureARB( GL_TEXTURE1_ARB );
			pShaderShadow->EnableTexture( false );
			glDisable( GL_REGISTER_COMBINERS_NV );
			glActiveTextureARB( GL_TEXTURE0_ARB );
			glColor3f( 1.0f, 1.0f, 1.0f );
			glDisable( GL_BLEND );
#endif
		}
		else
		{
			SHADOW_STATE
			{
				SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
			}
			DYNAMIC_STATE
			{
				pShaderAPI->Color3f( g_pConfig->lightScale, g_pConfig->lightScale, g_pConfig->lightScale );
				pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE0 );
				FogToFogColor();
			}
			Draw();
		}
	}
END_SHADER
