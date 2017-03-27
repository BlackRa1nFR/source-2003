//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Implementation of the sprite shader
//=============================================================================

#include "BaseVSShader.h"
#include <string.h>
#include "const.h"

DEFINE_FALLBACK_SHADER( Sprite, Sprite_DX8 )

// WARNING!  Change these in engine/SpriteGn.h if you change them here!
#define SPR_VP_PARALLEL_UPRIGHT		0
#define SPR_FACING_UPRIGHT			1
#define SPR_VP_PARALLEL				2
#define SPR_ORIENTED				3
#define SPR_VP_PARALLEL_ORIENTED	4


BEGIN_VS_SHADER( Sprite_DX8, 
			  "Help for Sprite_DX8" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SPRITEORIGIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "sprite origin" )
		SHADER_PARAM( SPRITEORIENTATION, SHADER_PARAM_TYPE_INTEGER, "0", "sprite orientation" )
		SHADER_PARAM( SPRITERENDERMODE, SHADER_PARAM_TYPE_INTEGER, "0", "sprite rendermode" )
		SHADER_PARAM( IGNOREVERTEXCOLORS, SHADER_PARAM_TYPE_BOOL, "1", "ignore vertex colors" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// FIXME: Not sure if this is the best solution, but it's a very]
		// easy one. When graphics aren't enabled, we oftentimes need to get
		// at the parameters of a shader. Therefore, we must set the default
		// values in a separate phase from when we load resources.

		if (!params[ALPHA]->IsDefined())
			params[ ALPHA ]->SetFloatValue( 1.0f );

		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		SET_FLAGS( MATERIAL_VAR_VERTEXCOLOR );
		SET_FLAGS( MATERIAL_VAR_VERTEXALPHA );

		// translate from a string orientation to an enumeration
		if (params[SPRITEORIENTATION]->IsDefined())
		{
			const char *orientationString = params[SPRITEORIENTATION]->GetStringValue();
			if( stricmp( orientationString, "parallel_upright" ) == 0 )
			{
				params[SPRITEORIENTATION]->SetIntValue( SPR_VP_PARALLEL_UPRIGHT );
			}
			else if( stricmp( orientationString, "facing_upright" ) == 0 )
			{
				params[SPRITEORIENTATION]->SetIntValue( SPR_FACING_UPRIGHT );
			}
			else if( stricmp( orientationString, "vp_parallel" ) == 0 )
			{
				params[SPRITEORIENTATION]->SetIntValue( SPR_VP_PARALLEL );
			}
			else if( stricmp( orientationString, "oriented" ) == 0 )
			{
				params[SPRITEORIENTATION]->SetIntValue( SPR_ORIENTED );
			}
			else if( stricmp( orientationString, "vp_parallel_oriented" ) == 0 )
			{
				params[SPRITEORIENTATION]->SetIntValue( SPR_VP_PARALLEL_ORIENTED );
			}
			else
			{
				Warning( "error with $spriteOrientation\n" );
				params[SPRITEORIENTATION]->SetIntValue( SPR_VP_PARALLEL_UPRIGHT );
			}
		}
		else
		{
			// default case
			params[SPRITEORIENTATION]->SetIntValue( SPR_VP_PARALLEL_UPRIGHT );
		}
	}

	SHADER_FALLBACK
	{
//		if (g_pHardwareConfig->GetDXSupportLevel() < 80)
			return "Sprite_DX6";
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableCulling( false );
		}

		switch( params[SPRITERENDERMODE]->GetIntValue() )
		{
		case kRenderNormal:
			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0, 0 );
				pShaderShadow->SetVertexShader( "unlitgeneric" );
				pShaderShadow->SetPixelShader( "SpriteRenderNormal", 0 );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToFogColor();
			}
			Draw();
			break;
		case kRenderTransColor:
		case kRenderTransTexture:
#if 1
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
				int numTexCoords = 1;
				pShaderShadow->VertexShaderVertexFormat( 
					flags, numTexCoords, 0, 0, 0 );
				pShaderShadow->SetVertexShader( "unlitgeneric_vertexcolor" );
				pShaderShadow->SetPixelShader( "SpriteRenderTransColor", 0 );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToFogColor();
			}
			Draw();
#else
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToFogColor();
			}
			Draw();
#endif
			break;
		case kRenderGlow:
		case kRenderWorldGlow:
#if 1
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableDepthTest( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
				int numTexCoords = 1;
				pShaderShadow->VertexShaderVertexFormat( 
					flags, numTexCoords, 0, 0, 0 );
				pShaderShadow->SetVertexShader( "unlitgeneric_vertexcolor" );
				pShaderShadow->SetPixelShader( "SpriteRenderTransColor", 0 );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToBlack();
			}
			Draw();
#else
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableDepthTest( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToBlack();
			}
			Draw();
#endif
			break;
		case kRenderTransAlpha:
#if 1
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
				int numTexCoords = 1;
				pShaderShadow->VertexShaderVertexFormat( 
					flags, numTexCoords, 0, 0, 0 );
				pShaderShadow->SetVertexShader( "unlitgeneric_vertexcolor" );
				pShaderShadow->SetPixelShader( "SpriteRenderTransColor", 0 );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToBlack();
			}
			Draw();
#else
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToFogColor();
			}
			Draw();
#endif
			break;
		case kRenderTransAlphaAdd:
#if 1
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
				int numTexCoords = 1;
				pShaderShadow->VertexShaderVertexFormat( 
					flags, numTexCoords, 0, 0, 0 );
				pShaderShadow->SetVertexShader( "unlitgeneric_vertexcolor" );
				pShaderShadow->SetPixelShader( "SpriteRenderTransColor", 0 );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToBlack();
			}
			Draw();
#else
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToFogColor();
			}
			Draw();
#endif
			break;

		case kRenderTransAdd:
#if 1
			SHADOW_STATE
			{
				unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
				if( params[ IGNOREVERTEXCOLORS ]->GetIntValue() )
				{
					flags = VERTEX_POSITION;
				}
				else
				{
					flags = VERTEX_POSITION | VERTEX_COLOR;
				}
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				int numTexCoords = 1;
				pShaderShadow->VertexShaderVertexFormat( 
					flags, numTexCoords, 0, 0, 0 );
				if( params[ IGNOREVERTEXCOLORS ]->GetIntValue() )
				{
					pShaderShadow->SetVertexShader( "unlitgeneric" );
				}
				else
				{
					pShaderShadow->SetVertexShader( "unlitgeneric_vertexcolor" );
				}
				pShaderShadow->SetPixelShader( "SpriteRenderTransAdd", 0 );
			}
			DYNAMIC_STATE
			{
				float color[4];
				params[COLOR]->GetVecValue( color, 3 );
				color[3] = params[ALPHA]->GetFloatValue();
				pShaderAPI->SetPixelShaderConstant( 0, color );
//				SetColorState( COLOR, ALPHA );
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToBlack();
			}
			Draw();
#else
			SHADOW_STATE
			{
				if( params[ IGNOREVERTEXCOLORS ]->GetIntValue() )
				{
					pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
				}
				else
				{
					pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
				}
				pShaderShadow->EnableConstantColor( true );
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			}
			DYNAMIC_STATE
			{
				SetColorState( COLOR, ALPHA );
				BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
				FogToBlack();
			}
			Draw();
#endif
			break;
		case kRenderTransAddFrameBlend:
#if 1
			{
				float flFrame = params[FRAME]->GetFloatValue();
				float flFade = params[ALPHA]->GetFloatValue();
				SHADOW_STATE
				{
					unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
					if( params[ IGNOREVERTEXCOLORS ]->GetIntValue() )
					{
						flags = VERTEX_POSITION;
					}
					else
					{
						flags = VERTEX_POSITION | VERTEX_COLOR;
					}
					pShaderShadow->EnableDepthWrites( false );
					pShaderShadow->EnableBlending( true );
					pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
					int numTexCoords = 1;
					pShaderShadow->VertexShaderVertexFormat( 
						flags, numTexCoords, 0, 0, 0 );
					pShaderShadow->SetVertexShader( "unlitgeneric_vertexcolor" );
					pShaderShadow->SetPixelShader( "SpriteRenderTransAdd", 0 );
				}
				DYNAMIC_STATE
				{
					float frameBlendAlpha = 1.0f - ( flFrame - ( int )flFrame );
					float color[4];
					color[0] = color[1] = color[2] = color[3] = flFade * frameBlendAlpha;
					pShaderAPI->SetPixelShaderConstant( 0, color );
					BindTexture( SHADER_TEXTURE_STAGE0, params[BASETEXTURE]->GetTextureValue(), ( int )flFrame );
					FogToBlack();
				}
				Draw();

				SHADOW_STATE
				{
					pShaderShadow->SetVertexShader( "unlitgeneric_vertexcolor" );
					pShaderShadow->SetPixelShader( "SpriteRenderTransAdd", 0 );
				}
				DYNAMIC_STATE
				{
					float frameBlendAlpha = ( flFrame - ( int )flFrame );
					float color[4];
					color[0] = color[1] = color[2] = color[3] = flFade * frameBlendAlpha;
					pShaderAPI->SetPixelShaderConstant( 0, color );
					ITexture *pTexture = params[BASETEXTURE]->GetTextureValue();
					int numAnimationFrames = pTexture->GetNumAnimationFrames();
					BindTexture( SHADER_TEXTURE_STAGE0, pTexture, ( ( int )flFrame + 1 ) % numAnimationFrames );
					FogToBlack();
				}
				Draw();
			}
#else
			{
				float flFrame = params[FRAME]->GetFloatValue();
				float flFade = params[ALPHA]->GetFloatValue();
				SHADOW_STATE
				{
					if( params[ IGNOREVERTEXCOLORS ]->GetIntValue() )
					{
						pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
					}
					else
					{
						pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
					}
					pShaderShadow->EnableConstantColor( true );
					pShaderShadow->EnableDepthWrites( false );
					pShaderShadow->EnableBlending( true );
					pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
					pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				}
				DYNAMIC_STATE
				{
					float frameBlendAlpha = 1.0f - ( flFrame - ( int )flFrame );
					pShaderAPI->Color3f( flFade * frameBlendAlpha, flFade * frameBlendAlpha, flFade * frameBlendAlpha );
					ITextureInternal *pTexture = static_cast<ITextureInternal*>(params[BASETEXTURE]->GetTextureValue());
					if( pTexture )
						pTexture->Bind( SHADER_TEXTURE_STAGE0, ( int )flFrame );

					FogToBlack();
				}
				Draw();
				DYNAMIC_STATE
				{
					float frameBlendAlpha = ( flFrame - ( int )flFrame );
					pShaderAPI->Color3f( flFade * frameBlendAlpha, flFade * frameBlendAlpha, flFade * frameBlendAlpha );
					ITextureInternal *pTexture = static_cast<ITextureInternal*>(params[BASETEXTURE]->GetTextureValue());
					int numAnimationFrames = pTexture->GetNumAnimationFrames();
					if( pTexture )
						pTexture->Bind( SHADER_TEXTURE_STAGE0, ( ( int )flFrame + 1 ) % numAnimationFrames );

					FogToBlack();
				}
				Draw();
			}
#endif

			break;
		default:
			ShaderWarning( "shader Sprite: Unknown sprite render mode\n" );
			break;
		}
	}
END_SHADER
