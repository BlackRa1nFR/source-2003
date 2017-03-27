//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Teeth renderer
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "VMatrix.h"

BEGIN_VS_SHADER( Eyes, 
			  "Help for Eyes" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( IRIS, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "iris texture" )
		SHADER_PARAM( IRISFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame for the iris texture" )
		SHADER_PARAM( GLINT, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "glint texture" )
		SHADER_PARAM( EYEORIGIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "origin for the eyes" )
		SHADER_PARAM( EYEUP, SHADER_PARAM_TYPE_VEC3, "[0 0 1]", "up vector for the eyes" )
		SHADER_PARAM( IRISU, SHADER_PARAM_TYPE_VEC4, "[0 1 0 0 ]", "U projection vector for the iris" )
		SHADER_PARAM( IRISV, SHADER_PARAM_TYPE_VEC4, "[0 0 1 0]", "V projection vector for the iris" )
		SHADER_PARAM( GLINTU, SHADER_PARAM_TYPE_VEC4, "[0 1 0 0]", "U projection vector for the glint" )
		SHADER_PARAM( GLINTV, SHADER_PARAM_TYPE_VEC4, "[0 0 1 0]", "V projection vector for the glint" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// default to 'MODEL' mode...
		if (!IS_FLAG_DEFINED( MATERIAL_VAR_MODEL ))
			SET_FLAGS( MATERIAL_VAR_MODEL );
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		LoadTexture( IRIS );
	}

	void SetTextureTransform( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
					MaterialMatrixMode_t textureTransform, int uparam, int vparam )
	{
		Vector4D u, v;
		params[uparam]->GetVecValue( u.Base(), 4 );
		params[vparam]->GetVecValue( v.Base(), 4 );

		// Need to transform these puppies into camera space
		// they are defined in world space
		VMatrix mat, invTrans;
		pShaderAPI->GetMatrix( MATERIAL_VIEW, mat.m[0] );
		mat = mat.Transpose();

		// Compute the inverse transpose of the matrix
		// NOTE: I only have to invert it here because VMatrix is transposed
		// with respect to what gets returned from GetMatrix.
		mat.InverseGeneral( invTrans );
		invTrans = invTrans.Transpose();

		// Transform the u and v planes into view space
		Vector4D uview, vview;
		uview.AsVector3D() = invTrans.VMul3x3( u.AsVector3D() );
		vview.AsVector3D() = invTrans.VMul3x3( v.AsVector3D() );
		uview[3] = u[3] - DotProduct( mat.GetTranslation(), uview.AsVector3D() );
		vview[3] = v[3] - DotProduct( mat.GetTranslation(), vview.AsVector3D() );

		float m[16];
		m[0] = uview[0];	m[1] = vview[0];	m[2] = 0.0f;	m[3] = 0.0f;
		m[4] = uview[1];	m[5] = vview[1];	m[6] = 0.0f;	m[7] = 0.0f;
		m[8] = uview[2];	m[9] = vview[2];	m[10] = 1.0f;	m[11] = 0.0f;
		m[12] = uview[3];	m[13] = vview[3];	m[14] = 0.0f;	m[15] = 1.0f;

		pShaderAPI->MatrixMode( textureTransform );
		pShaderAPI->LoadMatrix( m );
	}

	void DrawUsingSoftwareLighting( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, g_pConfig->overbright );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR | SHADER_DRAW_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			FogToFogColor();
		}
		Draw();

		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, g_pConfig->overbright );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR );
			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_EYE_LINEAR );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, IRIS, IRISFRAME );
			SetTextureTransform( params, pShaderAPI, MATERIAL_TEXTURE0, IRISU, IRISV );
			FogToFogColor();
		}
		Draw();

		/* FIXME: One pass version, only works if we have alpha blend between stages
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableOverbright( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR | SHADER_DRAW_TEXCOORD1 );
			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_EYE_LINEAR );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, IRIS, IRISFRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE, FRAME );
			SetTextureTransform( params, pShaderAPI, MATERIAL_TEXTURE0, IRISU, IRISV );
			FogToFogColor();
		}
		Draw();
		*/

		// Glint
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, false );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, 1.0f );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, 1.0f );

			pShaderShadow->EnableConstantColor( true );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );

			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_EYE_LINEAR );

			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, GLINT );
			SetTextureTransform( params, pShaderAPI, MATERIAL_TEXTURE0, GLINTU, GLINTV );
			FogToBlack();
		}
		Draw( );
	}

	void DrawUsingVertexShader( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
			pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX, 1, 0, 3, 0 );
			pShaderShadow->SetVertexShader( "Eyes" );
			if (g_pConfig->overbright == 2)
			{
				pShaderShadow->SetPixelShader( "Eyes_Overbright2" );
			}
			else
			{
				pShaderShadow->SetPixelShader( "Eyes" );
			}
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, IRIS, IRISFRAME );
			BindTexture( SHADER_TEXTURE_STAGE2, GLINT );
			SetAmbientCubeDynamicStateVertexShader();
			
			SetVertexShaderConstant( 90, EYEORIGIN );
			SetVertexShaderConstant( 91, EYEUP );
			SetVertexShaderConstant( 92, IRISU );
			SetVertexShaderConstant( 93, IRISV );
			SetVertexShaderConstant( 94, GLINTU );
			SetVertexShaderConstant( 95, GLINTV );

			FogToFogColor();
		}
		Draw();
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
		}
		if( g_pConfig->bSoftwareLighting )
		{
			DrawUsingSoftwareLighting( params, pShaderAPI, pShaderShadow );
		}
		else if( g_pHardwareConfig->SupportsVertexAndPixelShaders() )
		{
			DrawUsingVertexShader( params, pShaderAPI, pShaderShadow );
		}
		else
		{
			DrawUsingSoftwareLighting( params, pShaderAPI, pShaderShadow );
		}
	}
END_SHADER

