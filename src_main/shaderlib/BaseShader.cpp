//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/BaseShader.h"
#include "shaderlib/ShaderDLL.h"
#include "tier0/dbg.h"
#include "shaderDLL_Global.h"
#include "materialsystem/IShaderSystem.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"
#include "materialsystem/ishaderapi.h"
#include "materialsystem/materialsystem_config.h"
#include "shaderlib/cshader.h"
#include "vmatrix.h"
#include "vstdlib/strtools.h"

// NOTE: This must be the last include file in a .cpp file!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
IMaterialVar **CBaseShader::s_ppParams;
IShaderShadow *CBaseShader::s_pShaderShadow;
IShaderDynamicAPI *CBaseShader::s_pShaderAPI;
IShaderInit *CBaseShader::s_pShaderInit;
int CBaseShader::s_nModulationFlags;
CMeshBuilder *CBaseShader::s_pMeshBuilder;

	
//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
CBaseShader::CBaseShader() : m_SoftwareVertexShader( NULL )
{
	GetShaderDLL()->InsertShader( this );
}


//-----------------------------------------------------------------------------
// Shader parameter info
//-----------------------------------------------------------------------------
static ShaderParamInfo_t s_StandardParams[NUM_SHADER_MATERIAL_VARS] =
{
	{ "$flags",				"flags",			SHADER_PARAM_TYPE_INTEGER,	"0" },
	{ "$flags_defined",		"flags_defined",	SHADER_PARAM_TYPE_INTEGER,	"0" },
	{ "$flags2",  			"flags2",			SHADER_PARAM_TYPE_INTEGER,	"0" },
	{ "$flags_defined2",	"flags2_defined",	SHADER_PARAM_TYPE_INTEGER,	"0" },
	{ "$color",		 		"color",			SHADER_PARAM_TYPE_COLOR,	"[1 1 1]" },
	{ "$alpha",	   			"alpha",			SHADER_PARAM_TYPE_FLOAT,	"1.0" },
	{ "$basetexture",  		"Base Texture (albedo)", SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture" },
	{ "$frame",	  			"Animation Frame",	SHADER_PARAM_TYPE_INTEGER,	"0" },
	{ "$basetexturetransform", "Base Texture Texcoord Transform",SHADER_PARAM_TYPE_MATRIX,	"center .5 .5 scale 1 1 rotate 0 translate 0 0" },
};


//-----------------------------------------------------------------------------
// Gets the standard shader parameter names
// FIXME: Turn this into one function?
//-----------------------------------------------------------------------------
char const* CBaseShader::GetParamName( int nParamIndex ) const
{
	Assert( nParamIndex < NUM_SHADER_MATERIAL_VARS );
	return s_StandardParams[nParamIndex].m_pName;
}

const char *CBaseShader::GetParamHelp( int nParamIndex ) const
{
	Assert( nParamIndex < NUM_SHADER_MATERIAL_VARS );
	return s_StandardParams[nParamIndex].m_pHelp;
}

ShaderParamType_t CBaseShader::GetParamType( int nParamIndex ) const
{
	Assert( nParamIndex < NUM_SHADER_MATERIAL_VARS );
	return s_StandardParams[nParamIndex].m_Type;
}

const char *CBaseShader::GetParamDefault( int nParamIndex ) const
{
	Assert( nParamIndex < NUM_SHADER_MATERIAL_VARS );
	return s_StandardParams[nParamIndex].m_pDefaultValue;
}


//-----------------------------------------------------------------------------
// FIXME: There's gotta be a better way!
//-----------------------------------------------------------------------------
void CBaseShader::SetShaderDLLHandle( int hShaderDLL )
{
	m_hShaderDLL = hShaderDLL;
}

int CBaseShader::GetShaderDLLHandle( ) const
{
	return m_hShaderDLL;
}


//-----------------------------------------------------------------------------
// Necessary to snag ahold of some important data for the helper methods
//-----------------------------------------------------------------------------
void CBaseShader::InitShaderParams( IMaterialVar** ppParams, const char *pMaterialName )
{
	// Re-entrancy check
	Assert( !s_ppParams );

	s_ppParams = ppParams;

	OnInitShaderParams( ppParams, pMaterialName );

	s_ppParams = NULL;
}

void CBaseShader::InitShaderInstance( IMaterialVar** ppParams, IShaderInit *pShaderInit, const char *pMaterialName )
{
	// Re-entrancy check
	Assert( !s_ppParams );

	s_ppParams = ppParams;
	s_pShaderInit = pShaderInit;

	OnInitShaderInstance( ppParams, pShaderInit, pMaterialName );

	s_ppParams = NULL;
	s_pShaderInit = NULL;
}

void CBaseShader::DrawElements( IMaterialVar **ppParams, int nModulationFlags,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI )
{
	// Re-entrancy check
	Assert( !s_ppParams );

	s_ppParams = ppParams;
	s_pShaderAPI = pShaderAPI;
	s_pShaderShadow = pShaderShadow;
	s_nModulationFlags = nModulationFlags;
	s_pMeshBuilder = pShaderAPI ? pShaderAPI->GetVertexModifyBuilder() : NULL;

	if ( IsSnapshotting() )
	{
		// Set up the shadow state
		SetInitialShadowState( );
	}

	OnDrawElements( ppParams, pShaderShadow, pShaderAPI );

	s_nModulationFlags = 0;
	s_ppParams = NULL;
	s_pShaderAPI = NULL;
	s_pShaderShadow = NULL;
	s_pMeshBuilder = NULL;
}


//-----------------------------------------------------------------------------
// Sets the default shadow state
//-----------------------------------------------------------------------------
void CBaseShader::SetInitialShadowState( )
{
	// Set the default state
	s_pShaderShadow->SetDefaultState();

	// Init the standard states...
	int flags = s_ppParams[FLAGS]->GetIntValue();
	if (flags & MATERIAL_VAR_IGNOREZ)
	{
		s_pShaderShadow->EnableDepthTest( false );
		s_pShaderShadow->EnableDepthWrites( false );
	}

	if (flags & MATERIAL_VAR_DECAL)
	{
		s_pShaderShadow->EnablePolyOffset( true );
		s_pShaderShadow->EnableDepthWrites( false );
	}

	if ((flags & MATERIAL_VAR_MODEL) && (g_pHardwareConfig->MaxBlendMatrices() > 1))
	{
		s_pShaderShadow->EnableVertexBlend( true );
	}

	if (flags & MATERIAL_VAR_NOCULL)
	{
		s_pShaderShadow->EnableCulling( false );
	}

	if (flags & MATERIAL_VAR_ZNEARER)
	{
		s_pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_NEARER );
	}
}


//-----------------------------------------------------------------------------
// Draws a snapshot
//-----------------------------------------------------------------------------
void CBaseShader::Draw( )
{
	if ( IsSnapshotting() )
	{
		// Turn off transparency if we're asked to....
		if (g_pConfig->bNoTransparency && 
			((s_ppParams[FLAGS]->GetIntValue() & MATERIAL_VAR_NO_DEBUG_OVERRIDE) == 0))
		{
			s_pShaderShadow->EnableDepthWrites( true );
 			s_pShaderShadow->EnableBlending( false );
		}

		GetShaderSystem()->TakeSnapshot();
	}
	else
	{
		GetShaderSystem()->DrawSnapshot();
	}
}


//-----------------------------------------------------------------------------
// Finds a particular parameter	(works because the lowest parameters match the shader)
//-----------------------------------------------------------------------------
int CBaseShader::FindParamIndex( const char *pName ) const
{
	int numParams = GetNumParams();
	for( int i = 0; i < numParams; i++ )
	{
		if( Q_strnicmp( GetParamName( i ), pName, 64 ) == 0 )
		{
			return i;
		}
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Are we using graphics?
//-----------------------------------------------------------------------------
bool CBaseShader::IsUsingGraphics()
{
	return GetShaderSystem()->IsUsingGraphics();
}


//-----------------------------------------------------------------------------
// Gets the builder...
//-----------------------------------------------------------------------------
CMeshBuilder* CBaseShader::MeshBuilder()
{
	return s_pMeshBuilder;
}


//-----------------------------------------------------------------------------
// Loads a texture
//-----------------------------------------------------------------------------
void CBaseShader::LoadTexture( int nTextureVar )
{
	if ((!s_ppParams) || (nTextureVar == -1))
		return;

	IMaterialVar* pNameVar = s_ppParams[nTextureVar];
	if( pNameVar && pNameVar->IsDefined() )
	{
		s_pShaderInit->LoadTexture( pNameVar );
	}
}


//-----------------------------------------------------------------------------
// Loads a bumpmap
//-----------------------------------------------------------------------------
void CBaseShader::LoadBumpMap( int nTextureVar )
{
	if ((!s_ppParams) || (nTextureVar == -1))
		return;

	IMaterialVar* pNameVar = s_ppParams[nTextureVar];
	if( pNameVar && pNameVar->IsDefined() )
	{
		s_pShaderInit->LoadBumpMap( pNameVar );
	}
}


//-----------------------------------------------------------------------------
// Loads a cubemap
//-----------------------------------------------------------------------------
void CBaseShader::LoadCubeMap( int nTextureVar )
{
	if ((!s_ppParams) || (nTextureVar == -1))
		return;

	IMaterialVar* pNameVar = s_ppParams[nTextureVar];
	if( pNameVar && pNameVar->IsDefined() )
	{
		s_pShaderInit->LoadCubeMap( s_ppParams, pNameVar );
	}
}


//-----------------------------------------------------------------------------
// Binds a texture
//-----------------------------------------------------------------------------
void CBaseShader::BindTexture( TextureStage_t stage, int nTextureVar, int nFrameVar )
{
	Assert( !IsSnapshotting() );
	Assert( nTextureVar != -1 );
	Assert ( s_ppParams );

	IMaterialVar* pTextureVar = s_ppParams[nTextureVar];
	IMaterialVar* pFrameVar = (nFrameVar != -1) ? s_ppParams[nFrameVar] : NULL;
	if (pTextureVar)
	{
		int nFrame = pFrameVar ? pFrameVar->GetIntValue() : 0;
		GetShaderSystem()->BindTexture( stage, pTextureVar->GetTextureValue(), nFrame );
	}
}

void CBaseShader::BindTexture( TextureStage_t stage, ITexture *pTexture, int nFrame )
{
	Assert( !IsSnapshotting() );
	GetShaderSystem()->BindTexture( stage, pTexture, nFrame );
}


//-----------------------------------------------------------------------------
// Does the texture store translucency in its alpha channel?
//-----------------------------------------------------------------------------
bool CBaseShader::TextureIsTranslucent( int textureVar, bool isBaseTexture )
{
	if (textureVar < 0)
		return false;

	IMaterialVar** params = s_ppParams;
	if (params[textureVar]->GetType() == MATERIAL_VAR_TYPE_TEXTURE)
	{
		if (!isBaseTexture)
		{
			return params[textureVar]->GetTextureValue()->IsTranslucent();
		}
		else
		{
			// Override translucency settings if this flag is set.
			if (IS_FLAG_SET(MATERIAL_VAR_OPAQUETEXTURE))
				return false;

			if ( (CurrentMaterialVarFlags() & (MATERIAL_VAR_SELFILLUM | MATERIAL_VAR_BASEALPHAENVMAPMASK)) == 0)
			{
				if ((CurrentMaterialVarFlags() & MATERIAL_VAR_TRANSLUCENT) ||
					(CurrentMaterialVarFlags() & MATERIAL_VAR_ALPHATEST))
				{
					return params[textureVar]->GetTextureValue()->IsTranslucent();
				}
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
//
// Helper methods for color modulation
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Are we alpha or color modulating?
//-----------------------------------------------------------------------------
bool CBaseShader::IsAlphaModulating()
{
	return (s_nModulationFlags & SHADER_USING_ALPHA_MODULATION) != 0;
}

bool CBaseShader::IsColorModulating()
{
	return (s_nModulationFlags & SHADER_USING_COLOR_MODULATION) != 0;
}


//-----------------------------------------------------------------------------
// FIXME: Figure out a better way to do this?
//-----------------------------------------------------------------------------
int CBaseShader::ComputeModulationFlags( IMaterialVar** params )
{
    int mod = 0;
	if ( GetAlpha(params) < 1.0f )
	{
		mod |= SHADER_USING_ALPHA_MODULATION;
	}

	float color[3];
	params[COLOR]->GetVecValue( color, 3 );
	if ((color[0] < 1.0) || (color[1] < 1.0) || (color[2] < 1.0))
	{
		mod |= SHADER_USING_COLOR_MODULATION;
	}

	return mod;
}


//-----------------------------------------------------------------------------
// Returns the translucency...
//-----------------------------------------------------------------------------
float CBaseShader::GetAlpha( IMaterialVar** ppParams )
{
	if ( !ppParams )
	{
		ppParams = s_ppParams;
	}

	if (!ppParams)
		return 1.0f;

	if ( ppParams[FLAGS]->GetIntValue() & MATERIAL_VAR_NOALPHAMOD )
		return 1.0f;

	float flAlpha = ppParams[ALPHA]->GetFloatValue();
	return clamp( flAlpha, 0.0f, 1.0f );
}


//-----------------------------------------------------------------------------
// Sets the color + transparency
//-----------------------------------------------------------------------------
void CBaseShader::SetColorState( int colorVar, bool setAlpha )
{
	Assert( !IsSnapshotting() );
	if ( !s_ppParams )
		return;

	// Use tint instead of color if it was specified...
	IMaterialVar* pColorVar = (colorVar != -1) ? s_ppParams[colorVar] : 0;

	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	if (pColorVar)
	{
		if (pColorVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
		{
			pColorVar->GetVecValue( color, 3 );
		}
		else
		{
			color[0] = color[1] = color[2] = pColorVar->GetFloatValue();
		}

		// Clamp...
		color[0] = clamp( color[0], 0.0f, 1.0f );
		color[1] = clamp( color[1], 0.0f, 1.0f );
		color[2] = clamp( color[2], 0.0f, 1.0f );
	}

	color[3] = setAlpha ? GetAlpha() : 1.0f;
	s_pShaderAPI->Color4fv( color );	
}


void CBaseShader::SetModulationShadowState( int tintVar )
{
	// Have have no control over the tint var...
	bool doModulation = (tintVar != -1);

	// We activate color modulating when we're alpha or color modulating
	doModulation = doModulation || IsAlphaModulating() || IsColorModulating();

	s_pShaderShadow->EnableConstantColor( doModulation );
}

void CBaseShader::SetModulationDynamicState( int tintVar )
{
	if (tintVar != -1)
	{
		SetColorState( tintVar, true );
	}
	else
	{
		SetColorState( COLOR, true );
	}
}

void CBaseShader::ComputeModulationColor( float* color )
{
	Assert( !IsSnapshotting() );
	if (!s_ppParams)
		return;

	IMaterialVar* pColorVar = s_ppParams[COLOR];
	if (pColorVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
	{
		pColorVar->GetVecValue( color, 3 );
	}
	else
	{
		color[0] = color[1] = color[2] = pColorVar->GetFloatValue();
	}

	if( !g_pConfig->bShowDiffuse )
	{
		color[0] = color[1] = color[2] = 0.0f;
	}
	color[3] = GetAlpha();
}


//-----------------------------------------------------------------------------
//
// Helper methods for alpha blending....
//
//-----------------------------------------------------------------------------
void CBaseShader::EnableAlphaBlending( ShaderBlendFactor_t src, ShaderBlendFactor_t dst )
{
	Assert( IsSnapshotting() );
	s_pShaderShadow->EnableBlending( true );
	s_pShaderShadow->BlendFunc( src, dst );
	s_pShaderShadow->EnableDepthWrites(false);
}

void CBaseShader::DisableAlphaBlending()
{
	Assert( IsSnapshotting() );
	s_pShaderShadow->EnableBlending( false );
	s_pShaderShadow->EnableDepthWrites( true );
}

void CBaseShader::SetNormalBlendingShadowState( int textureVar, bool isBaseTexture )
{
	Assert( IsSnapshotting() );

	// Either we've got a constant modulation
	bool isTranslucent = IsAlphaModulating();

	// Or we've got a vertex alpha
	isTranslucent = isTranslucent || (CurrentMaterialVarFlags() & MATERIAL_VAR_VERTEXALPHA);

	// Or we've got a texture alpha
	isTranslucent = isTranslucent || ( TextureIsTranslucent( textureVar, isBaseTexture ) &&
		                               !(CurrentMaterialVarFlags() & MATERIAL_VAR_ALPHATEST ) );

	if (isTranslucent)
	{
		EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
	}
	else
	{
		DisableAlphaBlending();
	}
}

void CBaseShader::SetAdditiveBlendingShadowState( int textureVar, bool isBaseTexture )
{
	Assert( IsSnapshotting() );

	// Either we've got a constant modulation
	bool isTranslucent = IsAlphaModulating();

	// Or we've got a vertex alpha
	isTranslucent = isTranslucent || (CurrentMaterialVarFlags() & MATERIAL_VAR_VERTEXALPHA);

	// Or we've got a texture alpha
	isTranslucent = isTranslucent || ( TextureIsTranslucent( textureVar, isBaseTexture ) &&
		                               !(CurrentMaterialVarFlags() & MATERIAL_VAR_ALPHATEST ) );

	if (isTranslucent)
	{
		EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
	}
	else
	{
		EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
	}
}

void CBaseShader::SetDefaultBlendingShadowState( int textureVar, bool isBaseTexture ) 
{
	if ( CurrentMaterialVarFlags() & MATERIAL_VAR_ADDITIVE )
	{
		SetAdditiveBlendingShadowState( textureVar, isBaseTexture );
	}
	else
	{
		SetNormalBlendingShadowState( textureVar, isBaseTexture );
	}
}

//-----------------------------------------------------------------------------
// Sets lightmap blending mode for single texturing
//-----------------------------------------------------------------------------
void CBaseShader::SingleTextureLightmapBlendMode( )
{
	Assert( IsSnapshotting() );

	s_pShaderShadow->EnableBlending( true );
	if( g_pConfig->overbright == 2.0f )
	{
		s_pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );
	}
	else
	{
		s_pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_ZERO );
	}
}


//-----------------------------------------------------------------------------
// Loads the identity transform into a matrix
//-----------------------------------------------------------------------------
void CBaseShader::LoadIdentity( MaterialMatrixMode_t matrixMode )
{
	Assert( !IsSnapshotting() );

	s_pShaderAPI->MatrixMode( matrixMode );
	s_pShaderAPI->LoadIdentity( );
}


//-----------------------------------------------------------------------------
// Loads the camera to world transform into a matrix
//-----------------------------------------------------------------------------
void CBaseShader::LoadCameraToWorldTransform( MaterialMatrixMode_t matrixMode )
{
	s_pShaderAPI->MatrixMode( matrixMode );
	s_pShaderAPI->LoadCameraToWorld();
}

void CBaseShader::LoadCameraSpaceSphereMapTransform( MaterialMatrixMode_t matrixMode )
{
	static float mat[4][4] = 
	{
		{ 0.5f,  0.0f, 0.0f, 0.0f },
		{ 0.0f, -0.5f, 0.0f, 0.0f },
		{ 0.0f,  0.0f, 0.0f, 0.0f },
		{ 0.5f, -0.5f, 0.0f, 1.0f },
	};

	s_pShaderAPI->MatrixMode( matrixMode );
	s_pShaderAPI->LoadMatrix( (float*)mat );
}


//-----------------------------------------------------------------------------
//
// Sets a texture translation transform
//
//-----------------------------------------------------------------------------
void CBaseShader::SetFixedFunctionTextureTranslation( MaterialMatrixMode_t textureTransform, int translationVar )
{
	Assert( !IsSnapshotting() );

	// handle scrolling of base texture
	Vector2D vDelta( 0, 0 );

	if (translationVar != -1)
	{
		s_ppParams[translationVar]->GetVecValue( vDelta.Base(), 2 );
	}

	if( vDelta[0] != 0.0f || vDelta[1] != 0.0f )
	{
		s_pShaderAPI->MatrixMode( textureTransform );

		// only do the upper 3x3 since this is a 2D matrix
		float mat[16];
		mat[0] = 1.0f;		mat[1] = 0.0f;		mat[2] = 0.0f;
		mat[4] = 0.0f;		mat[5] = 1.0f;		mat[6] = 0.0f;
		mat[8] = vDelta[0]; mat[9] = vDelta[1]; mat[10] = 1.0f;

		// Better set the stuff we don't set with some sort of value!
		mat[3] = mat[7] = mat[11] = 0;
		mat[12] = mat[13] = mat[14] = 0;
		mat[15] = 1;

		s_pShaderAPI->LoadMatrix( mat );
	}
	else
	{
		LoadIdentity( textureTransform );
	}
}

void CBaseShader::SetFixedFunctionTextureScale( MaterialMatrixMode_t textureTransform, int scaleVar )
{
	Assert( !IsSnapshotting() );

	// handle scrolling of base texture
	Vector2D vScale;
	s_ppParams[scaleVar]->GetVecValue( vScale.Base(), 2 );
	if( vScale[0] != 0.0f || vScale[1] != 0.0f )
	{
		s_pShaderAPI->MatrixMode( textureTransform );

		// only do the upper 3x3 since this is a 2D matrix
		float mat[16];
		mat[0] = vScale[0];	mat[1] = 0.0f;		mat[2] = 0.0f;
		mat[4] = 0.0f;		mat[5] = vScale[1];	mat[6] = 0.0f;
		mat[8] = 0.0f;		mat[9] = 0.0f;		mat[10] = 1.0f;

		// Better set the stuff we don't set with some sort of value!
		mat[3] = mat[7] = mat[11] = 0;
		mat[12] = mat[13] = mat[14] = 0;
		mat[15] = 1;

		s_pShaderAPI->LoadMatrix( mat );
	}
	else
	{
		LoadIdentity( textureTransform );
	}
}

void CBaseShader::SetFixedFunctionTextureTransform( MaterialMatrixMode_t textureTransform, int transformVar )
{
	Assert( !IsSnapshotting() );

	IMaterialVar* pTransformationVar = s_ppParams[transformVar];
	if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
	{
		s_pShaderAPI->MatrixMode( textureTransform );

		const VMatrix &transformation = pTransformationVar->GetMatrixValue();

		// only do the upper 3x3 since this is a 2D matrix
		float mat[16];
		mat[0] = transformation[0][0];	mat[1] = transformation[1][0];	mat[2] = transformation[3][0];
		mat[4] = transformation[0][1];	mat[5] = transformation[1][1];	mat[6] = transformation[3][1];
		mat[8] = transformation[0][3];	mat[9] = transformation[1][3];	mat[10] = transformation[3][3];

		// Better set the stuff we don't set with some sort of value!
		mat[3] = mat[7] = mat[11] = 0;
		mat[12] = mat[13] = mat[14] = 0;
		mat[15] = 1;

		s_pShaderAPI->LoadMatrix( mat );
	}
	else
	{
		LoadIdentity( textureTransform );
	}
}

void CBaseShader::SetFixedFunctionTextureScaledTransform( MaterialMatrixMode_t textureTransform, 
													   int transformVar, int scaleVar )
{
	Assert( !IsSnapshotting() );

	float mat[16];
	IMaterialVar* pTransformationVar = s_ppParams[transformVar];
	if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
	{
		Vector2D scale( 1, 1 );
		IMaterialVar* pScaleVar = s_ppParams[scaleVar];
		if (pScaleVar)
		{
			if (pScaleVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
				pScaleVar->GetVecValue( scale.Base(), 2 );
			else if (pScaleVar->IsDefined())
				scale[0] = scale[1] = pScaleVar->GetFloatValue();
		}

		s_pShaderAPI->MatrixMode( textureTransform );

		const VMatrix &transformation = pTransformationVar->GetMatrixValue();

		// only do the upper 3x3 since this is a 2D matrix
		mat[0] = transformation[0][0] * scale[0]; mat[1] = transformation[1][0] * scale[0]; mat[2] = transformation[3][0] * scale[0];
		mat[4] = transformation[0][1] * scale[1]; mat[5] = transformation[1][1] * scale[1]; mat[6] = transformation[3][1] * scale[1];
		mat[8] = transformation[0][3];	mat[9] = transformation[1][3];	mat[10] = transformation[3][3];

		// Better set the stuff we don't set with some sort of value!
		mat[3] = mat[7] = mat[11] = 0;
		mat[12] = mat[13] = mat[14] = 0;
		mat[15] = 1;

		s_pShaderAPI->LoadMatrix( mat );
	}
	else
	{
		SetFixedFunctionTextureScale( textureTransform, scaleVar );
	}
}


//-----------------------------------------------------------------------------
//
// Helper methods for fog
//
//-----------------------------------------------------------------------------
void CBaseShader::FogToWhite( void )
{
	Assert( !IsSnapshotting() );
	if (( CurrentMaterialVarFlags() & MATERIAL_VAR_NOFOG ) == 0)
	{
		s_pShaderAPI->FogMode( s_pShaderAPI->GetSceneFogMode() );
		if ( g_pConfig->overbright == 2.0f )
		{
			s_pShaderAPI->FogColor3ub( 127, 127, 127 );
		}
		else
		{
			s_pShaderAPI->FogColor3ub( 255, 255, 255 );
		}
	}
	else
	{
		s_pShaderAPI->FogMode( MATERIAL_FOG_NONE );
	}
}

void CBaseShader::FogToBlack( void )
{
	Assert( !IsSnapshotting() );
	if (( CurrentMaterialVarFlags() & MATERIAL_VAR_NOFOG ) == 0)
	{
		s_pShaderAPI->FogMode( s_pShaderAPI->GetSceneFogMode() );
		s_pShaderAPI->FogColor3ub( 0, 0, 0 );
	}
	else
	{
		s_pShaderAPI->FogMode( MATERIAL_FOG_NONE );
	}
}

void CBaseShader::FogToGrey( void )
{
	Assert( !IsSnapshotting() );
	if (( CurrentMaterialVarFlags() & MATERIAL_VAR_NOFOG ) == 0)
	{
		s_pShaderAPI->FogMode( s_pShaderAPI->GetSceneFogMode() );
		s_pShaderAPI->FogColor3ub( 127, 127, 127 );
	}
	else
	{
		s_pShaderAPI->FogMode( MATERIAL_FOG_NONE );
	}
}

void CBaseShader::FogToFogColor( void )
{
	Assert( !IsSnapshotting() );
	if (( CurrentMaterialVarFlags() & MATERIAL_VAR_NOFOG ) == 0)
	{
		s_pShaderAPI->FogMode( s_pShaderAPI->GetSceneFogMode() );
		unsigned char fogColor[3];
		s_pShaderAPI->GetSceneFogColor( fogColor );
		s_pShaderAPI->FogColor3ubv( fogColor );
	}
	else
	{
		s_pShaderAPI->FogMode( MATERIAL_FOG_NONE );
	}
}

void CBaseShader::DisableFog( void )
{
	Assert( !IsSnapshotting() );
	s_pShaderAPI->FogMode( MATERIAL_FOG_NONE );
}

void CBaseShader::DefaultFog( void )
{
	if ( CurrentMaterialVarFlags() & MATERIAL_VAR_ADDITIVE )
	{
		FogToBlack();
	}
	else
	{
		FogToFogColor();
	}
}


//-----------------------------------------------------------------------------
// Fixed function multiply by detail texture pass
//-----------------------------------------------------------------------------
void CBaseShader::FixedFunctionMultiplyByDetailPass( int baseTextureVar, int frameVar, 
		int textureTransformVar, int detailVar, int detailScaleVar )
{
	IMaterialVar** params = s_ppParams;

	if (!params[detailVar]->IsDefined())
		return;

	if (IsSnapshotting())
	{
		s_pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

		bool translucentTexture = TextureIsTranslucent( baseTextureVar, true ) ||
			IS_FLAG_SET(MATERIAL_VAR_ALPHATEST);
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, translucentTexture );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		s_pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, false );
		s_pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE1, false );

		// Mod 2x blend here
		EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );

		s_pShaderShadow->EnableCustomPixelPipe( true );
		s_pShaderShadow->CustomTextureStages( 2 );

		// We need to blend towards grey based on alpha...
		// We can never get the perfect alpha (vertex alpha * cc alpha * texture alpha)
		// so we'll just choose to use cc alpha * texture alpha

		int flags = SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD1;

		// Compute alpha, stage 0 is used, stage 1 isn't.
		if ( translucentTexture )
		{
			s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_MODULATE, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_CONSTANTCOLOR );
			flags |= SHADER_DRAW_TEXCOORD0;
		}
		else
		{
			bool hasVertexAlpha = (CurrentMaterialVarFlags() & MATERIAL_VAR_VERTEXALPHA) != 0;
			if (hasVertexAlpha)
			{
				flags |= SHADER_DRAW_COLOR;
			}

			s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_ALPHA, hasVertexAlpha ? SHADER_TEXOP_MODULATE : SHADER_TEXOP_SELECTARG2, 
				SHADER_TEXARG_VERTEXCOLOR, SHADER_TEXARG_CONSTANTCOLOR );
		}

		s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
			SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_NONE );

		// This here will perform color = vertex light * alpha + 0.5f * (1 - alpha)
		// Stage 0 really doesn't do anything
		s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1, 
			SHADER_TEXARG_TEXTURE, SHADER_TEXARG_CONSTANTCOLOR );

		s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_BLEND_PREVIOUSSTAGEALPHA, 
			SHADER_TEXARG_TEXTURE, SHADER_TEXARG_CONSTANTCOLOR );

		s_pShaderShadow->DrawFlags( flags );

		Draw( );

		s_pShaderShadow->EnableCustomPixelPipe( false );
		DisableAlphaBlending();
	}
	else
	{
		if (TextureIsTranslucent( baseTextureVar, true ) )
		{
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, textureTransformVar );
			BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, frameVar );
		}

		BindTexture( SHADER_TEXTURE_STAGE1, detailVar, frameVar );
		SetFixedFunctionTextureScaledTransform( MATERIAL_TEXTURE1, textureTransformVar, detailScaleVar );
		float alpha = GetAlpha();
		s_pShaderAPI->Color4ub( 128, 128, 128, 255 * alpha );
		FogToGrey();

		Draw( );
	}
}


//-----------------------------------------------------------------------------
// Multiply by lightmap pass
//-----------------------------------------------------------------------------
void CBaseShader::FixedFunctionMultiplyByLightmapPass( int baseTextureVar, 
	int frameVar, int baseTextureTransformVar, float alphaOverride )
{
	if (IsSnapshotting())
	{
		s_pShaderShadow->EnableAlphaTest( false );

		s_pShaderShadow->EnableBlending( true );
		SingleTextureLightmapBlendMode();
		
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
 		s_pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, false );
		s_pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE1, false );

		s_pShaderShadow->EnableCustomPixelPipe( true );
		s_pShaderShadow->CustomTextureStages( 2 );

		// Stage zero color is not used, this op doesn't matter
		s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1, 
			SHADER_TEXARG_CONSTANTCOLOR, SHADER_TEXARG_CONSTANTCOLOR );

		// This here will perform color = lightmap * (cc alpha) + 1 * (1- cc alpha)
		s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_BLEND_PREVIOUSSTAGEALPHA, 
			SHADER_TEXARG_TEXTURE, SHADER_TEXARG_CONSTANTCOLOR );

		int flags = SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD1;

		// Multiply the constant alpha by the texture alpha for total alpha
		if (TextureIsTranslucent(baseTextureVar, true))
		{
			s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_MODULATE, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_CONSTANTCOLOR );

			flags |= SHADER_DRAW_TEXCOORD0;
		}
		else
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, false );
			s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG2, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_CONSTANTCOLOR );
		}

		// Alpha isn't used, it doesn't matter what we set it to.
		s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
			SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_NONE );
		 
		s_pShaderShadow->DrawFlags( flags );

		Draw();

		s_pShaderShadow->EnableCustomPixelPipe( false );
	}
	else
	{
		// Put the alpha in the color channel to modulate the color down....
		float alpha = (alphaOverride < 0) ? GetAlpha() : alphaOverride;
		if (g_pConfig->overbright == 2.0f)
			s_pShaderAPI->Color4f( 0.5f, 0.5f, 0.5f, alpha );
		else
			s_pShaderAPI->Color4f( 1.0f, 1.0f, 1.0f, alpha );

		if (TextureIsTranslucent(baseTextureVar, true))
		{
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, baseTextureTransformVar );
			BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, frameVar );
		}

		LoadIdentity( MATERIAL_TEXTURE1 );
		s_pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE1 );
		FogToWhite();

		Draw();
	}
}


//-----------------------------------------------------------------------------
// Fixed function Self illumination pass
//-----------------------------------------------------------------------------
void CBaseShader::FixedFunctionSelfIlluminationPass( TextureStage_t stage, 
	int baseTextureVar, int frameVar, int baseTextureTransformVar, int selfIllumTintVar )
{
//	IMaterialVar** params = s_ppParams;

	if (IsSnapshotting())
	{
		// A little setup for self illum here...
		SetModulationShadowState( selfIllumTintVar );

		s_pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, false );
		s_pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE1, false );

		s_pShaderShadow->EnableAlphaTest( false );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, false );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, false );
		s_pShaderShadow->EnableTexture( stage, true );

		// No overbrighting
		s_pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, 1.0f );
		s_pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, 1.0f );

		// Don't bother with z writes here...
 		s_pShaderShadow->EnableDepthWrites( false );

		// We're always blending
		EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

		int flags = SHADER_DRAW_POSITION;
		if (stage == SHADER_TEXTURE_STAGE0)
			flags |= SHADER_DRAW_TEXCOORD0;
		else
			flags |= SHADER_DRAW_TEXCOORD1;

		s_pShaderShadow->DrawFlags( flags );
	}
	else
	{
		SetFixedFunctionTextureTransform( 
			(stage == SHADER_TEXTURE_STAGE0) ? MATERIAL_TEXTURE0 : MATERIAL_TEXTURE1, 
			baseTextureTransformVar );
		BindTexture( stage, baseTextureVar, frameVar );

		// NOTE: Texture + texture offset are set from BaseTimesLightmap
		SetModulationDynamicState( selfIllumTintVar );
		FogToFogColor();
	}
	Draw();
}


//-----------------------------------------------------------------------------
// Fixed function Base * detail pass
//-----------------------------------------------------------------------------
void CBaseShader::FixedFunctionBaseTimesDetailPass( int baseTextureVar, 
	int frameVar, int baseTextureTransformVar, int detailVar, int detailScaleVar )
{
	IMaterialVar** params = s_ppParams;

	// We can't do this one one pass if CC and VC are both active...
	bool hasDetail = (detailVar != -1) && params[detailVar]->IsDefined();
	bool detailInSecondPass = hasDetail &&	IsColorModulating() && 
		(IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR) || IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA));

	if (IsSnapshotting())
	{
		IMaterialVar** params = s_ppParams;

		s_pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, false );
		s_pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE1, false );

		// alpha test
 		s_pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

		// Alpha blending
		SetDefaultBlendingShadowState( baseTextureVar, true );

		// independently configure alpha and color
		s_pShaderShadow->EnableAlphaPipe( true );

		// Here's the color	states (NOTE: SHADER_DRAW_COLOR == use Vertex Color)
		s_pShaderShadow->EnableConstantColor( IsColorModulating() );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );

		int flags = SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0;

		// Detail texture..
		if (hasDetail && (!detailInSecondPass))
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			// Force mod2x
			s_pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, 2.0f );

			flags |= SHADER_DRAW_TEXCOORD1;
		}

		// Here's the alpha states
		s_pShaderShadow->EnableConstantAlpha( IsAlphaModulating() );
		s_pShaderShadow->EnableVertexAlpha( IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA) );
		s_pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE0, TextureIsTranslucent(baseTextureVar, true) );

		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			flags |= SHADER_DRAW_COLOR;
		s_pShaderShadow->DrawFlags( flags );

		Draw();

		s_pShaderShadow->EnableAlphaPipe( false );
	}
	else
	{
		SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, baseTextureTransformVar );
		BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, frameVar );

		// Detail texture..
		if (hasDetail && (!detailInSecondPass))
		{
			BindTexture( SHADER_TEXTURE_STAGE1, detailVar, frameVar );
			SetFixedFunctionTextureScaledTransform( MATERIAL_TEXTURE1, baseTextureTransformVar, detailScaleVar );
		}

		SetModulationDynamicState();
		DefaultFog();

		Draw();
	}

	if (detailInSecondPass)
	{
		FixedFunctionMultiplyByDetailPass( baseTextureVar, frameVar, baseTextureTransformVar, detailVar, detailScaleVar );
	}
}


//-----------------------------------------------------------------------------
// Helpers for environment mapping...
//-----------------------------------------------------------------------------
int CBaseShader::SetShadowEnvMappingState( int envMapMaskVar, int tintVar )
{
	Assert( IsSnapshotting() );
	IMaterialVar** params = s_ppParams;

	int varFlags = params[FLAGS]->GetIntValue();

	s_pShaderShadow->EnableAlphaTest( false );

	// envmap on stage 0
	s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
	s_pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );
	if ( (varFlags & MATERIAL_VAR_ENVMAPSPHERE) == 0 )
		s_pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_CAMERASPACEREFLECTIONVECTOR );
	else
		s_pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_SPHERE_MAP );

	int flags = SHADER_DRAW_POSITION | SHADER_DRAW_NORMAL;

	// mask on stage 1
	if (params[envMapMaskVar]->IsDefined() || (varFlags & MATERIAL_VAR_BASEALPHAENVMAPMASK))
	{
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		flags |= SHADER_DRAW_TEXCOORD1;
	}
	else
	{
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, false );
	}

	if (varFlags & MATERIAL_VAR_BASEALPHAENVMAPMASK)
	{
		s_pShaderShadow->EnableCustomPixelPipe( true );
		s_pShaderShadow->CustomTextureStages( 2 );

		// Color = base texture * envmaptint * (1 - mask alpha)
		s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE, SHADER_TEXARG_TEXTURE, SHADER_TEXARG_CONSTANTCOLOR );  
		s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE, SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_INVTEXTUREALPHA );

		// Use alpha modulation * vertex alpha * env map alpha
		s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_MODULATE, SHADER_TEXARG_VERTEXCOLOR, SHADER_TEXARG_TEXTURE );  
		s_pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_CONSTANTCOLOR );
	}
	else
	{
		s_pShaderShadow->EnableAlphaPipe( true );

		// Color = base texture * envmaptint * mask
		s_pShaderShadow->EnableConstantColor( tintVar >= 0 );

		// Alpha = vertex alpha * constant alpha * env map alpha * mask alpha (only if it's not a base alpha mask)
		s_pShaderShadow->EnableConstantAlpha( IsAlphaModulating() );
		s_pShaderShadow->EnableVertexAlpha( (varFlags & MATERIAL_VAR_VERTEXALPHA) != 0 );
		s_pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE1, params[envMapMaskVar]->IsDefined() );
	}

	return flags;
}

void CBaseShader::SetDynamicEnvMappingState( int envMapVar, int envMapMaskVar, 
	int baseTextureVar, int envMapFrameVar, int envMapMaskFrameVar, int frameVar,  
	int maskOffsetVar, int maskScaleVar, int tintVar )
{
	Assert( !IsSnapshotting() );

	IMaterialVar** params = s_ppParams;
	int varFlags = params[FLAGS]->GetIntValue();

	if( (varFlags & MATERIAL_VAR_ENVMAPSPHERE) == 0 )
	{
		if ( (varFlags & MATERIAL_VAR_ENVMAPCAMERASPACE) == 0 )
		{
			LoadCameraToWorldTransform( MATERIAL_TEXTURE0 );
		}
		else
		{
			LoadIdentity( MATERIAL_TEXTURE0 );
		}
	}
	else
	{
		LoadCameraSpaceSphereMapTransform( MATERIAL_TEXTURE0 );
	}

	BindTexture( SHADER_TEXTURE_STAGE0, envMapVar, envMapFrameVar );

	if (params[envMapMaskVar]->IsDefined())
	{
		SetFixedFunctionTextureScaledTransform( MATERIAL_TEXTURE1, 
			maskOffsetVar, maskScaleVar );
		BindTexture( SHADER_TEXTURE_STAGE1, envMapMaskVar, envMapMaskFrameVar );
	}
	else if (varFlags & MATERIAL_VAR_BASEALPHAENVMAPMASK)
	{
		SetFixedFunctionTextureScaledTransform( MATERIAL_TEXTURE1, 
			maskOffsetVar, maskScaleVar );
		BindTexture( SHADER_TEXTURE_STAGE1, baseTextureVar, frameVar );
	}

	SetModulationDynamicState( tintVar );
}


//-----------------------------------------------------------------------------
// Masked environment map
//-----------------------------------------------------------------------------
void CBaseShader::FixedFunctionMaskedEnvmapPass( int envMapVar, int envMapMaskVar, 
	int baseTextureVar, int envMapFrameVar, int envMapMaskFrameVar, 
	int frameVar, int maskOffsetVar, int maskScaleVar, int envMapTintVar )
{
//	IMaterialVar** params = ShaderState().m_ppParams;

	if (IsSnapshotting())
	{
		// Alpha blending
		SetDefaultBlendingShadowState( envMapMaskVar, false );

		// Disable overbright
		s_pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, 1.0f );
		s_pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, 1.0f );

		int flags = SetShadowEnvMappingState( envMapMaskVar, envMapTintVar );
		s_pShaderShadow->DrawFlags( flags );

		Draw();

		s_pShaderShadow->EnableCustomPixelPipe( false );
		s_pShaderShadow->EnableAlphaPipe( false );
	}
	else
	{
		SetDynamicEnvMappingState( envMapVar, envMapMaskVar, baseTextureVar,
			envMapFrameVar, envMapMaskFrameVar, frameVar, 
			maskOffsetVar, maskScaleVar, envMapTintVar );
		DefaultFog();

		Draw();
	}
}


//-----------------------------------------------------------------------------
// Add masked environment map
//-----------------------------------------------------------------------------
void CBaseShader::FixedFunctionAdditiveMaskedEnvmapPass( int envMapVar, int envMapMaskVar, 
	int baseTextureVar, int envMapFrameVar, int envMapMaskFrameVar, 
	int frameVar, int maskOffsetVar, int maskScaleVar, int envMapTintVar )
{
//	IMaterialVar** params = ShaderState().m_ppParams;

	if (IsSnapshotting())
	{
		// Alpha blending
		SetAdditiveBlendingShadowState( envMapMaskVar, false );

		// Disable overbright
		s_pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, 1.0f );
		s_pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, 1.0f );

		// Don't bother with z writes here...
 		s_pShaderShadow->EnableDepthWrites( false );

		int flags = SetShadowEnvMappingState( envMapMaskVar, envMapTintVar );
		s_pShaderShadow->DrawFlags( flags );

		Draw();

		s_pShaderShadow->EnableCustomPixelPipe( false );
		s_pShaderShadow->EnableAlphaPipe( false );
	}
	else
	{
		SetDynamicEnvMappingState( envMapVar, envMapMaskVar, baseTextureVar,
			envMapFrameVar, envMapMaskFrameVar, frameVar, 
			maskOffsetVar, maskScaleVar, envMapTintVar );
		FogToBlack();

		Draw();
	}
}


//-----------------------------------------------------------------------------
// Sets up ambient light cube...
//-----------------------------------------------------------------------------
void CBaseShader::SetAmbientCubeDynamicStateFixedFunction( )
{
	Assert( !IsSnapshotting() );

	s_pShaderAPI->BindAmbientLightCubeToStage0( );
	LoadCameraToWorldTransform( MATERIAL_TEXTURE0 );
}

void CBaseShader::CleanupDynamicStateFixedFunction( )
{
	Assert( !IsSnapshotting() );
	LoadIdentity( MATERIAL_TEXTURE0 );
}

