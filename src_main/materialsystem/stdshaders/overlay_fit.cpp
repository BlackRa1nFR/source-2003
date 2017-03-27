//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "BaseVSShader.h"
#include "convar.h"

// FIXME: Need to make a dx9 version so that "CENTROID" works.

BEGIN_VS_SHADER( Overlay_Fit,
			  "Help for TerrainTest2" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	// Set up anything that is necessary to make decisions in SHADER_FALLBACK.
	SHADER_INIT_PARAMS()
	{
		// No texture means no self-illum or env mask in base alpha
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		
		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->SupportsVertexAndPixelShaders() )
			return 0;
		else
			return "WorldTwoTextureBlend";
	}

	SHADER_INIT
	{
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );	// BASETEXTURE
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );	// LIGHTMAP
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );	// BASETEXTURE

			// texcoord0 : base texcoord
			// texcoord1 : alpha texcoord (mapped to fill the overlay)
			unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
			int numTexCoords = 3;
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0, 0 );

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

			pShaderShadow->EnablePolyOffset( true );

			pShaderShadow->SetVertexShader( "Overlay_Fit_vs11" );
			pShaderShadow->SetPixelShader ( "Overlay_Fit_ps11", 0 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
			BindTexture( SHADER_TEXTURE_STAGE2, BASETEXTURE, FRAME );

			pShaderAPI->SetVertexShaderIndex( 0 );

			FogToFogColor();
		}
		Draw();
	}
END_SHADER

