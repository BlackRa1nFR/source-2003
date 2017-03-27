//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
// This is what all vs/ps (dx8+) shaders inherit from.
//=============================================================================

#include "basevsshader.h"
#include "vmatrix.h"
#include "bumpvects.h"

#include "ConVar.h"

#ifdef HDR
#include "vertexlit_and_unlit_generic_hdr_ps20.inc"
#endif

//-----------------------------------------------------------------------------
// Standard vertex shader constants used by shaders in the std shader DLLs
//-----------------------------------------------------------------------------
enum
{
	VERTEX_SHADER_MODULATION_COLOR = 38,
};


//-----------------------------------------------------------------------------
// Computes the shader index for vertex lit materials
//-----------------------------------------------------------------------------
int CBaseVSShader::ComputeVertexLitShaderIndex( bool bVertexLitGeneric, bool hasBump, bool hasEnvmap, bool hasVertexColor, bool bHasNormal ) const
{
/*
		"LIGHT_COMBO"			"0..21"
		"FOG_TYPE"				"0..1"
		"NUM_BONES"				"0..3"
		"BUMPMAP"				"0..1"
		"VERTEXCOLOR"			"0..1"
		"TANGENTSPACE"			"0..1"
		"HASNORMAL"				"0..1"
*/

	int lightCombo = 0;
	if( bVertexLitGeneric )
	{
		lightCombo = s_pShaderAPI->GetCurrentLightCombo();
	}
	MaterialFogMode_t fogType = s_pShaderAPI->GetSceneFogMode();
	int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
	int numBones	= s_pShaderAPI->GetCurrentNumBones();
	int vshIndex = 0;
	bool bTangentSpace = hasBump || hasEnvmap;
	vshIndex = lightCombo + ( 22 * fogIndex ) + 
							( 22 * 2 * numBones ) + 
		( hasBump ?           22 * 2 * 4 : 0 ) + 
		( hasVertexColor ?    22 * 2 * 4 * 2 : 0 ) +
		( ( bHasNormal || bTangentSpace ) ?     22 * 2 * 4 * 2 * 2 : 0 );
		
	return vshIndex;
}


// These functions are to be called from the shaders.

//-----------------------------------------------------------------------------
// Pixel and vertex shader constants....
//-----------------------------------------------------------------------------
void CBaseVSShader::SetPixelShaderConstant( int pixelReg, int constantVar )
{
	Assert( !IsSnapshotting() );
	if ((!s_ppParams) || (constantVar == -1))
		return;

	IMaterialVar* pPixelVar = s_ppParams[constantVar];
	Assert( pPixelVar );

	float val[4];
	if (pPixelVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
		pPixelVar->GetVecValue( val, 4 );
	else
		val[0] = val[1] = val[2] = val[3] = pPixelVar->GetFloatValue();
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, val );	
}

// GR - special version with fix for const/lerp issue
void CBaseVSShader::SetPixelShaderConstantFudge( int pixelReg, int constantVar )
{
	Assert( !IsSnapshotting() );
	if ((!s_ppParams) || (constantVar == -1))
		return;

	IMaterialVar* pPixelVar = s_ppParams[constantVar];
	Assert( pPixelVar );

	float val[4];
	if (pPixelVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
	{
		pPixelVar->GetVecValue( val, 4 );
		val[0] = val[0] * 0.992f + 0.0078f;
		val[1] = val[1] * 0.992f + 0.0078f;
		val[2] = val[2] * 0.992f + 0.0078f;
		val[3] = val[3] * 0.992f + 0.0078f;
	}
	else
		val[0] = val[1] = val[2] = val[3] = pPixelVar->GetFloatValue() * 0.992f + 0.0078f;
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, val );	
}

void CBaseVSShader::SetVertexShaderConstant( int vertexReg, int constantVar )
{
	Assert( !IsSnapshotting() );
	if ((!s_ppParams) || (constantVar == -1))
		return;

	IMaterialVar* pVertexVar = s_ppParams[constantVar];
	Assert( pVertexVar );

	float val[4];
	if (pVertexVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
		pVertexVar->GetVecValue( val, 4 );
	else
		val[0] = val[1] = val[2] = val[3] = pVertexVar->GetFloatValue();
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, val );	
}

void CBaseVSShader::SetVertexShaderConstantVect( int vertexReg, int constantVar )
{
	Assert( !IsSnapshotting() );
	if ((!s_ppParams) || (constantVar == -1))
		return;

	IMaterialVar* pVertexVar = s_ppParams[constantVar];
	Assert( pVertexVar );

	float val[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	if (pVertexVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
		pVertexVar->GetVecValue( val, 4 );
	else
		val[0] = pVertexVar->GetFloatValue();
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, val );	
}


//-----------------------------------------------------------------------------
// Sets normalized light color for pixel shaders.
//-----------------------------------------------------------------------------
void CBaseVSShader::SetPixelShaderLightColors( int pixelReg )
{
	int i;
	int maxLights = s_pShaderAPI->GetMaxLights();
	for( i = 0; i < maxLights; i++ )
	{
		const LightDesc_t & lightDesc = s_pShaderAPI->GetLight( i );
		if( lightDesc.m_Type != MATERIAL_LIGHT_DISABLE )
		{
			Vector color( lightDesc.m_Color[0], lightDesc.m_Color[1], lightDesc.m_Color[2] );
			VectorNormalize( color );
			float val[4] = { color[0], color[1], color[2], 1.0f };
			s_pShaderAPI->SetPixelShaderConstant( pixelReg + i, val, 1 );
		}
		else
		{
			float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			s_pShaderAPI->SetPixelShaderConstant( pixelReg + i, zero, 1 );
		}
	}
}


//-----------------------------------------------------------------------------
// Sets vertex shader texture transforms
//-----------------------------------------------------------------------------
void CBaseVSShader::SetVertexShaderTextureTranslation( int vertexReg, int translationVar )
{
	float offset[2] = {0, 0};

	IMaterialVar* pTranslationVar = s_ppParams[translationVar];
	if (pTranslationVar)
	{
		if (pTranslationVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pTranslationVar->GetVecValue( offset, 2 );
		else
			offset[0] = offset[1] = pTranslationVar->GetFloatValue();
	}

	Vector4D translation[2];
	translation[0].Init( 1.0f, 0.0f, 0.0f, offset[0] );
	translation[1].Init( 0.0f, 1.0f, 0.0f, offset[1] );
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, translation[0].Base(), 2 ); 
}

void CBaseVSShader::SetVertexShaderTextureScale( int vertexReg, int scaleVar )
{
	float scale[2] = {1, 1};

	IMaterialVar* pScaleVar = s_ppParams[scaleVar];
	if (pScaleVar)
	{
		if (pScaleVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pScaleVar->GetVecValue( scale, 2 );
		else if (pScaleVar->IsDefined())
			scale[0] = scale[1] = pScaleVar->GetFloatValue();
	}

	Vector4D scaleMatrix[2];
	scaleMatrix[0].Init( scale[0], 0.0f, 0.0f, 0.0f );
	scaleMatrix[1].Init( 0.0f, scale[1], 0.0f, 0.0f );
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, scaleMatrix[0].Base(), 2 ); 
}

void CBaseVSShader::SetVertexShaderTextureTransform( int vertexReg, int transformVar )
{
	Vector4D transformation[2];
	IMaterialVar* pTransformationVar = s_ppParams[transformVar];
	if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
	{
		const VMatrix &mat = pTransformationVar->GetMatrixValue();
		transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
		transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
	}
	else
	{
		transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
		transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
	}
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, transformation[0].Base(), 2 ); 
}

void CBaseVSShader::SetVertexShaderTextureScaledTransform( int vertexReg, int transformVar, int scaleVar )
{
	Vector4D transformation[2];
	IMaterialVar* pTransformationVar = s_ppParams[transformVar];
	if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
	{
		const VMatrix &mat = pTransformationVar->GetMatrixValue();
		transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
		transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
	}
	else
	{
		transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
		transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
	}

	Vector2D scale( 1, 1 );
	IMaterialVar* pScaleVar = s_ppParams[scaleVar];
	if (pScaleVar)
	{
		if (pScaleVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pScaleVar->GetVecValue( scale.Base(), 2 );
		else if (pScaleVar->IsDefined())
			scale[0] = scale[1] = pScaleVar->GetFloatValue();
	}

	// Apply the scaling
	transformation[0][0] *= scale[0];
	transformation[0][1] *= scale[1];
	transformation[1][0] *= scale[0];
	transformation[1][1] *= scale[1];
	transformation[0][3] *= scale[0];
	transformation[1][3] *= scale[1];
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, transformation[0].Base(), 2 ); 
}


//-----------------------------------------------------------------------------
// Sets pixel shader texture transforms
//-----------------------------------------------------------------------------
void CBaseVSShader::SetPixelShaderTextureTranslation( int pixelReg, int translationVar )
{
	float offset[2] = {0, 0};

	IMaterialVar* pTranslationVar = s_ppParams[translationVar];
	if (pTranslationVar)
	{
		if (pTranslationVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pTranslationVar->GetVecValue( offset, 2 );
		else
			offset[0] = offset[1] = pTranslationVar->GetFloatValue();
	}

	Vector4D translation[2];
	translation[0].Init( 1.0f, 0.0f, 0.0f, offset[0] );
	translation[1].Init( 0.0f, 1.0f, 0.0f, offset[1] );
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, translation[0].Base(), 2 ); 
}

void CBaseVSShader::SetPixelShaderTextureScale( int pixelReg, int scaleVar )
{
	float scale[2] = {1, 1};

	IMaterialVar* pScaleVar = s_ppParams[scaleVar];
	if (pScaleVar)
	{
		if (pScaleVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pScaleVar->GetVecValue( scale, 2 );
		else if (pScaleVar->IsDefined())
			scale[0] = scale[1] = pScaleVar->GetFloatValue();
	}

	Vector4D scaleMatrix[2];
	scaleMatrix[0].Init( scale[0], 0.0f, 0.0f, 0.0f );
	scaleMatrix[1].Init( 0.0f, scale[1], 0.0f, 0.0f );
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, scaleMatrix[0].Base(), 2 ); 
}

void CBaseVSShader::SetPixelShaderTextureTransform( int pixelReg, int transformVar )
{
	Vector4D transformation[2];
	IMaterialVar* pTransformationVar = s_ppParams[transformVar];
	if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
	{
		const VMatrix &mat = pTransformationVar->GetMatrixValue();
		transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
		transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
	}
	else
	{
		transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
		transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
	}
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, transformation[0].Base(), 2 ); 
}

void CBaseVSShader::SetPixelShaderTextureScaledTransform( int pixelReg, int transformVar, int scaleVar )
{
	Vector4D transformation[2];
	IMaterialVar* pTransformationVar = s_ppParams[transformVar];
	if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
	{
		const VMatrix &mat = pTransformationVar->GetMatrixValue();
		transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
		transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
	}
	else
	{
		transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
		transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
	}

	Vector2D scale( 1, 1 );
	IMaterialVar* pScaleVar = s_ppParams[scaleVar];
	if (pScaleVar)
	{
		if (pScaleVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
			pScaleVar->GetVecValue( scale.Base(), 2 );
		else if (pScaleVar->IsDefined())
			scale[0] = scale[1] = pScaleVar->GetFloatValue();
	}

	// Apply the scaling
	transformation[0][0] *= scale[0];
	transformation[0][1] *= scale[1];
	transformation[1][0] *= scale[0];
	transformation[1][1] *= scale[1];
	transformation[0][3] *= scale[0];
	transformation[1][3] *= scale[1];
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, transformation[0].Base(), 2 ); 
}


//-----------------------------------------------------------------------------
// Moves a matrix into vertex shader constants 
//-----------------------------------------------------------------------------
void CBaseVSShader::SetVertexShaderMatrix3x4( int vertexReg, int matrixVar )
{
	IMaterialVar* pTranslationVar = s_ppParams[matrixVar];
	if (pTranslationVar)
	{
		s_pShaderAPI->SetVertexShaderConstant( vertexReg, &pTranslationVar->GetMatrixValue( )[0][0], 3 ); 
	}
	else
	{
		VMatrix matrix;
		MatrixSetIdentity( matrix );
		s_pShaderAPI->SetVertexShaderConstant( vertexReg, &matrix[0][0], 3 ); 
	}
}

void CBaseVSShader::SetVertexShaderMatrix4x4( int vertexReg, int matrixVar )
{
	IMaterialVar* pTranslationVar = s_ppParams[matrixVar];
	if (pTranslationVar)
	{
		s_pShaderAPI->SetVertexShaderConstant( vertexReg, &pTranslationVar->GetMatrixValue( )[0][0], 4 ); 
	}
	else
	{
		VMatrix matrix;
		MatrixSetIdentity( matrix );
		s_pShaderAPI->SetVertexShaderConstant( vertexReg, &matrix[0][0], 4 ); 
	}
}


//-----------------------------------------------------------------------------
// Loads the view matrix into pixel shader constants
//-----------------------------------------------------------------------------
void CBaseVSShader::LoadViewMatrixIntoVertexShaderConstant( int vertexReg )
{
	VMatrix mat, transpose;
	s_pShaderAPI->GetMatrix( MATERIAL_VIEW, mat.m[0] );

	MatrixTranspose( mat, transpose );
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, transpose.m[0], 3 );
}


//-----------------------------------------------------------------------------
// Loads bump lightmap coordinates into the pixel shader
//-----------------------------------------------------------------------------
void CBaseVSShader::LoadBumpLightmapCoordinateAxes_PixelShader( int pixelReg )
{
	Vector4D basis[3];
	for (int i = 0; i < 3; ++i)
	{
		memcpy( &basis[i], &g_localBumpBasis[i], 3 * sizeof(float) );
		basis[i][3] = 0.0f;
	}
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, (float*)basis, 3 );
}


//-----------------------------------------------------------------------------
// Loads bump lightmap coordinates into the pixel shader
//-----------------------------------------------------------------------------
void CBaseVSShader::LoadBumpLightmapCoordinateAxes_VertexShader( int vertexReg )
{
	Vector4D basis[3];

	// transpose
	int i;
	for (i = 0; i < 3; ++i)
	{
		basis[i][0] = g_localBumpBasis[0][i];
		basis[i][1] = g_localBumpBasis[1][i];
		basis[i][2] = g_localBumpBasis[2][i];
		basis[i][3] = 0.0f;
	}
	s_pShaderAPI->SetVertexShaderConstant( vertexReg, (float*)basis, 3 );
	for (i = 0; i < 3; ++i)
	{
		memcpy( &basis[i], &g_localBumpBasis[i], 3 * sizeof(float) );
		basis[i][3] = 0.0f;
	}
	s_pShaderAPI->SetVertexShaderConstant( vertexReg + 3, (float*)basis, 3 );
}


//-----------------------------------------------------------------------------
// Helper methods for pixel shader overbrighting
//-----------------------------------------------------------------------------
void CBaseVSShader::EnablePixelShaderOverbright( int reg, bool bEnable, bool bDivideByTwo )
{
	// can't have other overbright values with pixel shaders as it stands.
	Assert( g_pConfig->overbright == 1.0f || g_pConfig->overbright == 2.0f );
	float v[4];
	if( bEnable )
	{
		v[0] = v[1] = v[2] = v[3] = bDivideByTwo ? g_pConfig->overbright / 2.0f : g_pConfig->overbright;
	}
	else
	{
		v[0] = v[1] = v[2] = v[3] = bDivideByTwo ? 1.0f / 2.0f : 1.0f;
	}
	s_pShaderAPI->SetPixelShaderConstant( reg, v, 1 );
}


//-----------------------------------------------------------------------------
// Helper for dealing with modulation
//-----------------------------------------------------------------------------
void CBaseVSShader::SetModulationVertexShaderDynamicState()
{
 	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	ComputeModulationColor( color );
	s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_MODULATION_COLOR, color );
}

void CBaseVSShader::SetModulationPixelShaderDynamicState( int modulationVar )
{
	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
	ComputeModulationColor( color );
	s_pShaderAPI->SetPixelShaderConstant( modulationVar, color );
}


//-----------------------------------------------------------------------------
// Helpers for dealing with envmap tint
//-----------------------------------------------------------------------------
// set alphaVar to -1 to ignore it.
void CBaseVSShader::SetEnvMapTintPixelShaderDynamicState( int pixelReg, int tintVar, int alphaVar )
{
	float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	IMaterialVar* pTintVar = s_ppParams[tintVar];
	IMaterialVar* pAlphaVar = NULL;
	if( alphaVar >= 0 )
	{
		pAlphaVar = s_ppParams[tintVar];
	}
	if( pAlphaVar )
	{
		color[3] = pAlphaVar->GetFloatValue();
	}
	if( g_pConfig->bShowSpecular )
	{
		pTintVar->GetVecValue( color, 3 );
	}
	s_pShaderAPI->SetPixelShaderConstant( pixelReg, color, 1 );
}

void CBaseVSShader::SetAmbientCubeDynamicStateVertexShader( )
{
	s_pShaderAPI->SetVertexShaderStateAmbientLightCube();
}


const char *CBaseVSShader::UnlitGeneric_ComputeVertexShaderName( bool bMask,
														  bool bEnvmap,
														  bool bBaseTexture,
														  bool bBaseAlphaEnvmapMask,
														  bool bDetail,
														  bool bVertexColor,
														  bool bEnvmapCameraSpace,
														  bool bEnvmapSphere )
{
	static char const* s_pVertexShaders[] = 
	{
		"UnlitGeneric",
		"UnlitGeneric_VertexColor",
		"UnlitGeneric_EnvMap",
		"UnlitGeneric_EnvMapVertexColor",
		"UnlitGeneric",
		"UnlitGeneric_VertexColor",
		"UnlitGeneric_EnvMapSphere",
		"UnlitGeneric_EnvMapSphereVertexColor",

		"UnlitGeneric",
		"UnlitGeneric_VertexColor",
		"UnlitGeneric_EnvMapCameraSpace",
		"UnlitGeneric_EnvMapCameraSpaceVertexColor",
		"UnlitGeneric",
		"UnlitGeneric_VertexColor",
		"UnlitGeneric_EnvMapSphere",
		"UnlitGeneric_EnvMapSphereVertexColor",

		// Detail texture versions
		"UnlitGeneric_Detail",
		"UnlitGeneric_DetailVertexColor",
		"UnlitGeneric_DetailEnvMap",
		"UnlitGeneric_DetailEnvMapVertexColor",
		"UnlitGeneric_Detail",
		"UnlitGeneric_DetailVertexColor",
		"UnlitGeneric_DetailEnvMapSphere",
		"UnlitGeneric_DetailEnvMapSphereVertexColor",

		"UnlitGeneric_Detail",
		"UnlitGeneric_DetailVertexColor",
		"UnlitGeneric_DetailEnvMapCameraSpace",
		"UnlitGeneric_DetailEnvMapCameraSpaceVertexColor",
		"UnlitGeneric_Detail",
		"UnlitGeneric_DetailVertexColor",
		"UnlitGeneric_DetailEnvMapSphere",
		"UnlitGeneric_DetailEnvMapSphereVertexColor",
	};

	int vshIndex = 0;
	if (bVertexColor)
		vshIndex |= 0x1;
	if (bEnvmap)
		vshIndex |= 0x2;
	if (bEnvmapSphere)
		vshIndex |= 0x4;
	if (bEnvmapCameraSpace)
		vshIndex |= 0x8;
	if (bDetail)
		vshIndex |= 0x10;											
	return s_pVertexShaders[vshIndex];
}

const char *CBaseVSShader::UnlitGeneric_ComputePixelShaderName( bool bMask,
														  bool bEnvmap,
														  bool bBaseTexture,
														  bool bBaseAlphaEnvmapMask,
														  bool bDetail )
{
	static char const* s_pPixelShaders[] = 
	{
		"UnlitGeneric_NoTexture",
		"UnlitGeneric",
		"UnlitGeneric_EnvMapNoTexture",
		"UnlitGeneric_EnvMap",
		"UnlitGeneric_NoTexture",
		"UnlitGeneric",
		"UnlitGeneric_EnvMapMaskNoTexture",
		"UnlitGeneric_EnvMapMask",

		// Detail texture
		// The other commented-out versions are used if we want to
		// apply the detail *after* the environment map is added
		"UnlitGeneric_DetailNoTexture",
		"UnlitGeneric_Detail",
		"UnlitGeneric_EnvMapNoTexture", //"UnlitGeneric_DetailEnvMapNoTexture",
		"UnlitGeneric_DetailEnvMap",
		"UnlitGeneric_DetailNoTexture",
		"UnlitGeneric_Detail",
		"UnlitGeneric_EnvMapMaskNoTexture", //"UnlitGeneric_DetailEnvMapMaskNoTexture",
		"UnlitGeneric_DetailEnvMapMask",
	};

	if (!bMask && bEnvmap && bBaseTexture && bBaseAlphaEnvmapMask)
	{
		if (!bDetail)
			return "UnlitGeneric_BaseAlphaMaskedEnvMap";
		else
			return "UnlitGeneric_DetailBaseAlphaMaskedEnvMap";
	}
	else
	{
		int pshIndex = 0;
		if (bBaseTexture)
			pshIndex |= 0x1;
		if (bEnvmap)
			pshIndex |= 0x2;
		if (bMask)
			pshIndex |= 0x4;
		if (bDetail)
			pshIndex |= 0x8;
		return s_pPixelShaders[pshIndex];
	}
}

//-----------------------------------------------------------------------------
// Vertex shader unlit generic pass
//-----------------------------------------------------------------------------
void CBaseVSShader::VertexShaderUnlitGenericPass( bool doSkin, int baseTextureVar, int frameVar, 
	int baseTextureTransformVar, int detailVar, int detailTransform, bool bDetailTransformIsScale,
	int envmapVar, int envMapFrameVar, int envmapMaskVar, int envmapMaskFrameVar, 
	int envmapMaskScaleVar, int envmapTintVar )
{
	IMaterialVar** params = s_ppParams;

	bool bBaseAlphaEnvmapMask = IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK);
	bool bEnvmap = (envmapVar >= 0) && params[envmapVar]->IsDefined();
	bool bMask = false;
	if (bEnvmap)
	{
		bMask = params[envmapMaskVar]->IsDefined();
	}
	bool bDetail = (detailVar >= 0) && params[detailVar]->IsDefined(); 
	bool bBaseTexture = params[baseTextureVar]->IsDefined();
	bool bVertexColor = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
	bool bEnvmapCameraSpace = IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE);
	bool bEnvmapSphere = IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE);

	if (IsSnapshotting())
	{
		// Alpha test
		s_pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

		// Base texture on stage 0
		if (params[baseTextureVar]->IsDefined())
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		}

		if ((detailVar >= 0) && params[detailVar]->IsDefined())
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		}

		bool bEnvMapDefined = (envmapVar >= 0) && params[envmapVar]->IsDefined();
		if (bEnvMapDefined)
		{
			// envmap on stage 1
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			// envmapmask on stage 2
			if (params[envmapMaskVar]->IsDefined() || bBaseAlphaEnvmapMask )
				s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		}

		if (params[baseTextureVar]->IsDefined())
			SetDefaultBlendingShadowState( baseTextureVar, true );
		else
			SetDefaultBlendingShadowState( envmapMaskVar, false );

		int fmt = VERTEX_POSITION;
		if( bEnvMapDefined )
			fmt |= VERTEX_NORMAL;
		if (doSkin)
			fmt |= VERTEX_BONE_INDEX;
		if (bVertexColor)
			fmt |= VERTEX_COLOR;

		s_pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, doSkin ? 3 : 0, 0 );
		const char *pshName = UnlitGeneric_ComputePixelShaderName(
			bMask,
			bEnvmap,
			bBaseTexture,
			bBaseAlphaEnvmapMask,
			bDetail );
		s_pShaderShadow->SetPixelShader( pshName );

		const char *vshName = UnlitGeneric_ComputeVertexShaderName( bMask,
			bEnvmap,
			bBaseTexture,
			bBaseAlphaEnvmapMask,
			bDetail,
			bVertexColor,
			bEnvmapCameraSpace,
			bEnvmapSphere );
		s_pShaderShadow->SetVertexShader( vshName );
	}
	else
	{
		if (bBaseTexture)
		{
			BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, frameVar );
			SetVertexShaderTextureTransform( 90, baseTextureTransformVar );
		}

		if (bDetail)
		{
			BindTexture( SHADER_TEXTURE_STAGE3, detailVar, frameVar );

			if (bDetailTransformIsScale)
			{
				SetVertexShaderTextureScaledTransform( 94, baseTextureTransformVar, detailTransform );
			}
			else
			{
				SetVertexShaderTextureTransform( 94, detailTransform );
			}
		}

		if (bEnvmap)
		{
			BindTexture( SHADER_TEXTURE_STAGE1, envmapVar, envMapFrameVar );

			if (bMask || bBaseAlphaEnvmapMask)
			{
				if (bMask)
					BindTexture( SHADER_TEXTURE_STAGE2, envmapMaskVar, envmapMaskFrameVar );
				else
					BindTexture( SHADER_TEXTURE_STAGE2, baseTextureVar, frameVar );

				SetVertexShaderTextureScaledTransform( 92, baseTextureTransformVar, envmapMaskScaleVar );
			}

			SetEnvMapTintPixelShaderDynamicState( 2, envmapTintVar, -1 );

			if (bEnvmapSphere || IS_FLAG_SET(MATERIAL_VAR_ENVMAPCAMERASPACE))
			{
				LoadViewMatrixIntoVertexShaderConstant( 17 );
			}
		}

		SetModulationVertexShaderDynamicState();
		DefaultFog();
	}

	Draw();
}


void CBaseVSShader::DrawWorldBaseTexture( int baseTextureVar, int baseTextureTransformVar,
									int frameVar )
{
	if( IsSnapshotting() )
	{
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION, 1, 0, 0, 0 );
		s_pShaderShadow->SetPixelShader( "LightmappedGeneric_BaseTexture" );
		s_pShaderShadow->SetVertexShader( "LightmappedGeneric_BaseTexture" );
	}
	else
	{
		BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, frameVar );
		SetVertexShaderTextureTransform( 90, baseTextureTransformVar );
		FogToWhite();
	}
	Draw();
}

void CBaseVSShader::DrawWorldBumpedDiffuseLighting( int bumpmapVar, int bumpFrameVar,
											  int bumpTransformVar, bool bMultiply )
{
	if( IsSnapshotting() )
	{
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		if( bMultiply )
		{
			s_pShaderShadow->EnableBlending( true );
			SingleTextureLightmapBlendMode();
		}
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION, 3, 0, 0, 0 );
		s_pShaderShadow->SetVertexShader( "LightmappedGeneric_BumpmappedLightmap" );
		s_pShaderShadow->SetPixelShader( "LightmappedGeneric_BumpmappedLightmap" );
	}
	else
	{
		if( !g_pConfig->m_bFastNoBump )
		{
			BindTexture( SHADER_TEXTURE_STAGE0, bumpmapVar, bumpFrameVar );
		}
		else
		{
			s_pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE0 );
		}
		LoadBumpLightmapCoordinateAxes_PixelShader( 0 );
		s_pShaderAPI->BindBumpLightmap( SHADER_TEXTURE_STAGE1 );
		SetVertexShaderTextureTransform( 90, bumpTransformVar );
		SetModulationPixelShaderDynamicState( 3 );
		FogToFogColor();
	}
	Draw();
}

//#define USE_DEST_ALPHA
#define USE_NORMALMAP_ALPHA

void CBaseVSShader::DrawWorldBumpedSpecularLighting( int bumpmapVar, int envmapVar,
											   int bumpFrameVar, int envmapFrameVar,
											   int envmapTintVar, int alphaVar,
											   int envmapContrastVar, int envmapSaturationVar,
											   int bumpTransformVar, int fresnelReflectionVar,
											   bool bBlend )
{
#ifdef USE_DEST_ALPHA
	if( bBlend )
	{
		// WRITE SPECULAR MASK TO DESTINATION ALPHA
		// Can't do this until we get destination alpha to work.
		if( params[envmapMaskVar]->GetType() == MATERIAL_VAR_TYPE_TEXTURE )
		{
			if( IsSnapshotting() )
			{
				SetInitialShadowState( params );
				s_pShaderShadow->EnableColorWrites( false );
				s_pShaderShadow->EnableAlphaWrites( true );
				s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
				s_pShaderShadow->VertexShaderVertexFormat( 
					VERTEX_POSITION, 1, 0, 0, 0 );
			}
			else
			{
				s_pShaderAPI->SetDefaultState();
				BindTexture( SHADER_TEXTURE_STAGE0, envmapMaskVar, envmapMaskFrame );
				SetVertexShader( "WriteEnvMapMaskToAlphaBuffer" );
				SetPixelShader( "WriteEnvMapMaskToAlphaBuffer" );
			}
			Draw();
		}
	}
#endif
	
	// + BUMPED CUBEMAP
	if( IsSnapshotting() )
	{
		SetInitialShadowState( );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		}
		if( bBlend )
		{
			s_pShaderShadow->EnableBlending( true );
			s_pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
		}
		// FIXME: Remove the normal (needed for tangent space gen)
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
			VERTEX_TANGENT_T, 1, 0, 0, 0 );
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			s_pShaderShadow->SetVertexShader( "LightmappedGeneric_BumpmappedEnvmap_ps14" );
			s_pShaderShadow->SetPixelShader( "LightmappedGeneric_BumpmappedEnvmap_ps14" );
		}
		else
		{
			s_pShaderShadow->SetVertexShader( "LightmappedGeneric_BumpmappedEnvmap" );
			s_pShaderShadow->SetPixelShader( "LightmappedGeneric_BumpmappedEnvmap" );
		}
	}
	else
	{
		IMaterialVar** params = s_ppParams;
		s_pShaderAPI->SetDefaultState();
		BindTexture( SHADER_TEXTURE_STAGE0, bumpmapVar, bumpFrameVar );
		BindTexture( SHADER_TEXTURE_STAGE3, envmapVar, envmapFrameVar );
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			s_pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE4 );
		}
				
		SetEnvMapTintPixelShaderDynamicState( 0, envmapTintVar, alphaVar );
		// GR - fudge consts a bit to fix const/lerp issues
		SetPixelShaderConstantFudge( 1, envmapContrastVar );
		SetPixelShaderConstantFudge( 2, envmapSaturationVar );
		float greyWeights[4] = { 0.299f, 0.587f, 0.114f, 0.0f };
		s_pShaderAPI->SetPixelShaderConstant( 3, greyWeights );

		// [ 0, 0 ,0, R(0) ]
		float fresnel[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		fresnel[3] = params[fresnelReflectionVar]->GetFloatValue();
		s_pShaderAPI->SetPixelShaderConstant( 4, fresnel );
		// [ 0, 0 ,0, 1-R(0) ]
		fresnel[3] = 1.0f - fresnel[3];
		s_pShaderAPI->SetPixelShaderConstant( 6, fresnel );

		float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		s_pShaderAPI->SetPixelShaderConstant( 5, one );
		FogToBlack();
		SetVertexShaderTextureTransform( 90, bumpTransformVar );
	}
	Draw();
}

void CBaseVSShader::DrawModelBumpedSpecularLighting( int bumpMapVar, int bumpMapFrameVar,
											   int envMapVar, int envMapVarFrame,
											   int envMapTintVar, int alphaVar,
											   int envMapContrastVar, int envMapSaturationVar,
											   int bumpTransformVar,
											   bool bBlendSpecular )
{
	IMaterialVar** params = s_ppParams;
	
	if( IsSnapshotting() )
	{
		SetInitialShadowState( );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		}
		s_pShaderShadow->EnableAlphaTest( false );
		if( bBlendSpecular )
		{
			s_pShaderShadow->EnableBlending( true );
			s_pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			SetAdditiveBlendingShadowState( -1, false );
		}
		else
		{
			s_pShaderShadow->EnableBlending( false );
			SetNormalBlendingShadowState( -1, false );
		}
		
		int numLights = 0;
		
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION | VERTEX_NORMAL | VERTEX_BONE_INDEX, 
			1, 0, numLights, 4 /* userDataSize */ );
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			s_pShaderShadow->SetVertexShader( "VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14" );
			if( IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK ) )
			{
				s_pShaderShadow->SetPixelShader( "VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha_ps14" );
			}
			else
			{
				s_pShaderShadow->SetPixelShader( "VertexLitGeneric_EnvmappedBumpmapV2_ps14" );
			}
		}
		else
		{
			s_pShaderShadow->SetVertexShader( "VertexLitGeneric_EnvmappedBumpmap_NoLighting" );
			// This version does not multiply by lighting.
			// NOTE: We don't support multiplying by lighting for bumped specular stuff.
			if( IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK ) )
			{
				s_pShaderShadow->SetPixelShader( "VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha" );
			}
			else
			{
				s_pShaderShadow->SetPixelShader( "VertexLitGeneric_EnvmappedBumpmapV2" );
			}
		}
	}
	else
	{
		s_pShaderAPI->SetDefaultState();
		BindTexture( SHADER_TEXTURE_STAGE0, bumpMapVar, bumpMapFrameVar );
		BindTexture( SHADER_TEXTURE_STAGE3, envMapVar, envMapVarFrame );
		if( g_pHardwareConfig->SupportsPixelShaders_1_4() )
		{
			s_pShaderAPI->BindNormalizationCubeMap( SHADER_TEXTURE_STAGE4 );
		}
				
		if( bBlendSpecular )
		{
			SetEnvMapTintPixelShaderDynamicState( 0, envMapTintVar, -1 );
		}
		else
		{
			SetEnvMapTintPixelShaderDynamicState( 0, envMapTintVar, alphaVar );
		}
		// GR - fudge consts a bit to fix const/lerp issues
		SetPixelShaderConstantFudge( 1, envMapContrastVar );
		SetPixelShaderConstantFudge( 2, envMapSaturationVar );
		float greyWeights[4] = { 0.299f, 0.587f, 0.114f, 0.0f };
		s_pShaderAPI->SetPixelShaderConstant( 3, greyWeights );
		FogToBlack();
		
		// handle scrolling of bump texture
		SetVertexShaderTextureTransform( 94, bumpTransformVar );
	}
	Draw();
}

void CBaseVSShader::DrawBaseTextureBlend( int baseTextureVar, int baseTextureTransformVar, 
									 int baseTextureFrameVar,
									 int baseTexture2Var, int baseTextureTransform2Var, 
									 int baseTextureFrame2Var )
{
	if( IsSnapshotting() )
	{
		s_pShaderShadow->SetDefaultState();
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
		s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		s_pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 |
			SHADER_DRAW_LIGHTMAP_TEXCOORD1 );
		// FIXME: Remove the normal (needed for tangent space gen)
		s_pShaderShadow->VertexShaderVertexFormat( 
			VERTEX_POSITION, 2, 0, 0, 0 );
		s_pShaderShadow->SetVertexShader( "lightmappedgeneric_basetextureblend" );
		s_pShaderShadow->SetPixelShader( "lightmappedgeneric_basetextureblend", 0 );
	}
	else
	{
		s_pShaderAPI->SetDefaultState();
		BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, baseTextureFrameVar );
		BindTexture( SHADER_TEXTURE_STAGE1, baseTexture2Var, baseTextureFrame2Var );
		s_pShaderAPI->BindLightmap( SHADER_TEXTURE_STAGE2 );
		FogToWhite();
		SetVertexShaderTextureTransform( 90, baseTextureTransformVar );
		SetVertexShaderTextureTransform( 92, baseTextureTransform2Var );
	}
	Draw();
}

void CBaseVSShader::DrawWorldBumpedUsingVertexShader( int baseTextureVar, int baseTextureTransformVar,
												int bumpmapVar, int bumpFrameVar, 
												int bumpTransformVar,
												int envmapMaskVar, int envmapMaskFrame,
												int envmapVar, 
												int envmapFrameVar,
												int envmapTintVar, int alphaVar,
												int envmapContrastVar, int envmapSaturationVar,
												int frameVar, int fresnelReflectionVar,
												bool doBaseTexture2,
												int baseTexture2Var, int baseTextureTransform2Var,
												int baseTextureFrame2Var
												)
{
	IMaterialVar** params = s_ppParams;
	// Draw base texture
	bool bMultiplyDiffuseLighting = false;
	bool bBlendSpecular = false;
	if( doBaseTexture2 && params[baseTexture2Var]->IsDefined() && params[baseTextureVar]->IsDefined() )
	{
		DrawBaseTextureBlend( baseTextureVar, baseTextureTransformVar, frameVar,
			baseTexture2Var, baseTextureTransform2Var, baseTextureFrame2Var );
		bMultiplyDiffuseLighting = true;
		bBlendSpecular = true;
	}
	else if( params[baseTextureVar]->IsDefined() )
	{
		DrawWorldBaseTexture( baseTextureVar, baseTextureTransformVar, frameVar );
		bMultiplyDiffuseLighting = true;
		bBlendSpecular = true;
	}
	
	// Draw diffuse lighting
	if( params[baseTextureVar]->IsDefined() || !params[envmapVar]->IsDefined() )
	{
		DrawWorldBumpedDiffuseLighting( bumpmapVar, bumpFrameVar, bumpTransformVar, 
			bMultiplyDiffuseLighting );
		bBlendSpecular = true;
	}
	
	// Add specular lighting
	if( params[envmapVar]->IsDefined() )
	{
		DrawWorldBumpedSpecularLighting(
			bumpmapVar, envmapVar,
			bumpFrameVar, envmapFrameVar,
			envmapTintVar, alphaVar,
			envmapContrastVar, envmapSaturationVar,
			bumpTransformVar, fresnelReflectionVar,
			bBlendSpecular );
	}
}

void CBaseVSShader::InitParamsVertexLitAndUnlitGeneric_DX9( bool bVertexLitGeneric,
	int baseTextureVar, int frameVar, int baseTextureTransformVar, 
	int selfIllumTintVar,
	int detailVar, int detailFrameVar, int detailScaleVar, 
	int envmapVar, int envmapFrameVar, 
	int envmapMaskVar, int envmapMaskFrameVar, int envmapMaskTransformVar,
	int envmapTintVar, 
	int bumpmapVar, int bumpFrameVar, int bumpTransformVar,
	int envmapContrastVar, int envmapSaturationVar )
{
	IMaterialVar** params = s_ppParams;
	
	if( bVertexLitGeneric )
	{
		SET_FLAGS( MATERIAL_VAR_MODEL );
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
	}
	else
	{
		CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
	}

	if( !params[envmapMaskFrameVar]->IsDefined() )
	{
		params[envmapMaskFrameVar]->SetIntValue( 0 );
	}
	
	if( !params[envmapTintVar]->IsDefined() )
	{
		params[envmapTintVar]->SetVecValue( 1.0f, 1.0f, 1.0f );
	}

	if( (selfIllumTintVar != -1) && (!params[selfIllumTintVar]->IsDefined()) )
	{
		params[selfIllumTintVar]->SetVecValue( 1.0f, 1.0f, 1.0f );
	}

	if( !params[detailScaleVar]->IsDefined() )
	{
		params[detailScaleVar]->SetFloatValue( 4.0f );
	}

	if( !params[envmapContrastVar]->IsDefined() )
	{
		params[envmapContrastVar]->SetFloatValue( 0.0f );
	}
	
	if( !params[envmapSaturationVar]->IsDefined() )
	{
		params[envmapSaturationVar]->SetFloatValue( 1.0f );
	}
	
	if( !params[envmapFrameVar]->IsDefined() )
	{
		params[envmapFrameVar]->SetIntValue( 0 );
	}

	if( (bumpFrameVar != -1) && !params[bumpFrameVar]->IsDefined() )
	{
		params[bumpFrameVar]->SetIntValue( 0 );
	}

	// No texture means no self-illum or env mask in base alpha
	if ( !params[baseTextureVar]->IsDefined() )
	{
		CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
		CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	}

	// If in decal mode, no debug override...
	if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	if( ( (bumpmapVar != -1) && g_pConfig->bBumpmap && params[bumpmapVar]->IsDefined() ) || params[envmapVar]->IsDefined() )
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_TANGENT_SPACES );
		bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
		if( hasNormalMapAlphaEnvmapMask )
		{
			params[envmapMaskVar]->SetUndefined();
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}
	}
	else
	{
		CLEAR_FLAGS( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	}
}

void CBaseVSShader::InitParamsUnlitGeneric_DX9(
	int baseTextureVar, int frameVar, int baseTextureTransformVar, 
	int detailVar, int detailFrameVar, int detailScaleVar, 
	int envmapVar, int envmapFrameVar, 
	int envmapMaskVar, int envmapMaskFrameVar, int envmapMaskTransformVar,
	int envmapTintVar, int envmapContrastVar, int envmapSaturationVar )
{
	InitParamsVertexLitAndUnlitGeneric_DX9( false, baseTextureVar, frameVar, baseTextureTransformVar,
		-1, detailVar, detailFrameVar, detailScaleVar, envmapVar, envmapFrameVar, 
		envmapMaskVar,envmapMaskFrameVar, envmapMaskTransformVar, envmapTintVar,
		-1, -1, -1, envmapContrastVar, envmapSaturationVar );
}


void CBaseVSShader::InitVertexLitAndUnlitGeneric_DX9( 
		bool bVertexLitGeneric,
		int baseTextureVar, 
		int frameVar, 
		int baseTextureTransformVar, 
		int selfIllumTintVar,
		int detailVar, 
		int detailFrameVar, 
		int detailScaleVar, 
		int envmapVar, 
		int envmapFrameVar, 
		int envmapMaskVar, 
		int envmapMaskFrameVar, 
		int envmapMaskTransformVar,
		int envmapTintVar, 
		int bumpmapVar, 
		int bumpFrameVar,
		int bumpTransformVar,
		int envmapContrastVar,
		int envmapSaturationVar )
{
	IMaterialVar** params = s_ppParams;
	if (params[baseTextureVar]->IsDefined())
	{
		LoadTexture( baseTextureVar );
		
		if (!params[baseTextureVar]->GetTextureValue()->IsTranslucent())
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}
	}
	
	if (params[detailVar]->IsDefined())
	{
		LoadTexture( detailVar );
	}
	
	if ((bumpmapVar != -1) && g_pConfig->bBumpmap && params[bumpmapVar]->IsDefined())
	{
		LoadBumpMap( bumpmapVar );
		SET_FLAGS2( MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL );
	}
	
	// Don't alpha test if the alpha channel is used for other purposes
	if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
	{
		CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
	}
	
	if (params[envmapVar]->IsDefined())
	{
		if( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
		{
			LoadCubeMap( envmapVar );
		}
		else
		{
			LoadTexture( envmapVar );
		}
		
		if( !g_pHardwareConfig->SupportsCubeMaps() )
		{
			SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
		}
		
		if (params[envmapMaskVar]->IsDefined())
		{
			LoadTexture( envmapMaskVar );
		}
	}
}

void CBaseVSShader::InitUnlitGeneric_DX9( 
	int baseTextureVar, int frameVar,			int baseTextureTransformVar, 
	int detailVar,		int detailFrameVar,		int detailScaleVar, 
	int envmapVar,		int envmapFrameVar, 
	int envmapMaskVar,	int envmapMaskFrameVar, int envmapMaskTransformVar,
	int envmapTintVar,	int envmapContrastVar,	int envmapSaturationVar )
{
	InitVertexLitAndUnlitGeneric_DX9( false, baseTextureVar, frameVar, baseTextureTransformVar,
		-1, detailVar, detailFrameVar, detailScaleVar, envmapVar, envmapFrameVar, 
		envmapMaskVar,envmapMaskFrameVar, envmapMaskTransformVar, envmapTintVar,
		-1, -1, -1, envmapContrastVar, envmapSaturationVar );
}

inline int ComplutePixelShaderIndex_VertexLitAndUnlitGeneric_DX9(
	bool hasBaseTexture,
	bool hasDetailTexture,
	bool hasBump,
	bool hasEnvmap,
	bool hasDiffuseLighting,
	bool hasEnvmapMask,
	bool hasBaseAlphaEnvmapMask,
	bool hasSelfIllum,
	bool hasNormalMapAlphaEnvmapMask,
	bool hasVertexColor,
	bool hasVertexAlpha )
{
/*
	"BASETEXTURE"			"0..1"
	"DETAILTEXTURE"			"0..1"
	"BUMPMAP"				"0..1"
	"CUBEMAP"				"0..1"
	"DIFFUSELIGHTING"		"0..1"
	"ENVMAPMASK"			"0..1"
	"BASEALPHAENVMAPMASK"	"0..1"
	"SELFILLUM"				"0..1"
	"NORMALMAPALPHAENVMAPMASK" "0..1"
	"VERTEXCOLOR"			"0..1"
	"VERTEXALPHA"			"0..1"
*/
	int pshIndex = 0;
	if( hasBaseTexture )				pshIndex |= ( 1 << 0 );
	if( hasDetailTexture )				pshIndex |= ( 1 << 1 );
	if( hasBump )						pshIndex |= ( 1 << 2 );
	if( hasEnvmap )						pshIndex |= ( 1 << 3 );
	if( hasDiffuseLighting )			pshIndex |= ( 1 << 4 );
	if( hasEnvmapMask )					pshIndex |= ( 1 << 5 );
	if( hasBaseAlphaEnvmapMask )		pshIndex |= ( 1 << 6 );
	if( hasSelfIllum )					pshIndex |= ( 1 << 7 );
	if( hasNormalMapAlphaEnvmapMask )	pshIndex |= ( 1 << 8 );
	if( hasVertexColor )				pshIndex |= ( 1 << 9 );
	if( hasVertexAlpha )				pshIndex |= ( 1 << 10 );

	return pshIndex;
}

void CBaseVSShader::DrawVertexLitAndUnlitGeneric_DX9( bool bVertexLitGeneric,
		int baseTextureVar, 
		int frameVar, 
		int baseTextureTransformVar, 
		int selfIllumTintVar,
		int detailVar, 
		int detailFrameVar, 
		int detailScaleVar, 
		int envmapVar, 
		int envmapFrameVar, 
		int envmapMaskVar, 
		int envmapMaskFrameVar, 
		int envmapMaskTransformVar,
		int envmapTintVar, 
		int bumpmapVar, 
		int bumpFrameVar,
		int bumpTransformVar,
		int envmapContrastVar,
		int envmapSaturationVar )
{
	IMaterialVar** params = s_ppParams;
	bool hasBaseTexture = params[baseTextureVar]->IsDefined();
	bool hasDetailTexture = params[detailVar]->IsDefined();
	bool hasBump = (bumpmapVar != -1) && params[bumpmapVar]->IsDefined();
	bool hasEnvmap = params[envmapVar]->IsDefined();
	bool hasDiffuseLighting = bVertexLitGeneric;
	bool hasEnvmapMask = params[envmapMaskVar]->IsDefined();
	bool hasBaseAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	bool hasSelfIllum = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );
	bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	bool hasVertexColor = bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
	bool hasVertexAlpha = bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );
	bool bHasNormal = bVertexLitGeneric || hasEnvmap;
	if( ( hasDetailTexture && hasBump ) ||
		( hasEnvmapMask && hasBump ) ||
		( hasNormalMapAlphaEnvmapMask && hasBaseAlphaEnvmapMask ) ||
		( hasNormalMapAlphaEnvmapMask && hasEnvmapMask ) ||
		( hasBaseAlphaEnvmapMask && hasEnvmapMask ) ||
		( hasBaseAlphaEnvmapMask && hasSelfIllum ) )
	{
		// illegal combos
		Assert( 0 );
	}
	int vshIndex = 0;
	int pshIndex = 0;
	if( IsSnapshotting() )
	{
		// Alpha test: FIXME: shouldn't this be handled in CBaseVSShader::SetInitialShadowState
		s_pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

		if (params[baseTextureVar]->IsDefined())
			SetDefaultBlendingShadowState( baseTextureVar, true );
		else
			SetDefaultBlendingShadowState( envmapMaskVar, false );
		
		unsigned int flags = VERTEX_POSITION;
		if( bHasNormal )
		{
			flags |= VERTEX_NORMAL;
		}

		int userDataSize = 0;
		if( hasBaseTexture )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		}
		if( hasEnvmap )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			userDataSize = 4; // tangent S
		}
		if( hasDetailTexture )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		}
		if( hasBump )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			userDataSize = 4; // tangent S
		}
		if( hasEnvmapMask )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		}

		if( hasVertexColor || hasVertexAlpha )
		{
			flags |= VERTEX_COLOR;
		}
		
		// texcoord0 : base texcoord
		const int numTexCoords = 1;
		int numBoneWeights = 0;
		if( IS_FLAG_SET( MATERIAL_VAR_MODEL ) )
		{
			numBoneWeights = 3;
			flags |= VERTEX_BONE_INDEX;
		}
		
		s_pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );
		s_pShaderShadow->VertexShaderVertexFormat( 
			flags, numTexCoords, NULL, numBoneWeights, userDataSize );

		// Pre-cache shaders
		pshIndex = ComplutePixelShaderIndex_VertexLitAndUnlitGeneric_DX9(
			hasBaseTexture,
			hasDetailTexture,
			hasBump,
			hasEnvmap,
			hasDiffuseLighting,
			hasEnvmapMask,
			hasBaseAlphaEnvmapMask,
			hasSelfIllum,
			hasNormalMapAlphaEnvmapMask,
			hasVertexColor,
			hasVertexAlpha );
		
		s_pShaderShadow->SetVertexShader( "vertexlit_and_unlit_generic_vs20" );
		s_pShaderShadow->SetPixelShader( "vertexlit_and_unlit_generic_ps20", pshIndex );
	}
	else
	{
		s_pShaderAPI->SetDefaultState();
		if( hasBaseTexture )
		{
			BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, frameVar );
		}
		if( hasEnvmap )
		{
			BindTexture( SHADER_TEXTURE_STAGE1, envmapVar, envmapFrameVar );
		}
		if( hasDetailTexture )
		{
			BindTexture( SHADER_TEXTURE_STAGE2, detailVar, detailFrameVar );
		}
		if( hasBump )
		{
			if( !g_pConfig->m_bFastNoBump )
			{
				BindTexture( SHADER_TEXTURE_STAGE3, bumpmapVar, bumpFrameVar );
			}
			else
			{
				s_pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE3 );
			}
		}
		if( hasEnvmapMask )
		{
			BindTexture( SHADER_TEXTURE_STAGE4, envmapMaskVar, envmapMaskFrameVar );
		}

		vshIndex = ComputeVertexLitShaderIndex( bVertexLitGeneric, hasBump, hasEnvmap, hasVertexColor, bHasNormal );

		s_pShaderAPI->SetVertexShaderIndex( vshIndex );
		SetVertexShaderTextureTransform( 90, baseTextureTransformVar );
		if( hasDetailTexture )
		{
			SetVertexShaderTextureScaledTransform( 92, baseTextureTransformVar, detailScaleVar );
			Assert( !hasBump );
		}
		if( hasBump )
		{
			SetVertexShaderTextureTransform( 92, bumpTransformVar );
			Assert( !hasDetailTexture );
		}
		if( hasEnvmapMask )
		{
			SetVertexShaderTextureTransform( 94, envmapMaskTransformVar );
		}
		
		SetEnvMapTintPixelShaderDynamicState( 0, envmapTintVar, -1 );
		SetModulationPixelShaderDynamicState( 1 );
		SetPixelShaderConstant( 2, envmapContrastVar );
		SetPixelShaderConstant( 3, envmapSaturationVar );
		EnablePixelShaderOverbright( 4, true, false );
		SetPixelShaderConstant( 5, selfIllumTintVar );
		DefaultFog();
		SetAmbientCubeDynamicStateVertexShader();
	}
	Draw();
}

void CBaseVSShader::DrawUnlitGeneric_DX9( 
	int baseTextureVar, int frameVar,			int baseTextureTransformVar, 
	int detailVar,		int detailFrameVar,		int detailScaleVar, 
	int envmapVar,		int envmapFrameVar, 
	int envmapMaskVar,	int envmapMaskFrameVar, int envmapMaskTransformVar,
	int envmapTintVar,	int envmapContrastVar,	int envmapSaturationVar )
{
	DrawVertexLitAndUnlitGeneric_DX9( false,
		baseTextureVar,		frameVar,			baseTextureTransformVar,
		-1,
		detailVar,			detailFrameVar,		detailScaleVar,
		envmapVar,			envmapFrameVar,
		envmapMaskVar,		envmapMaskFrameVar,	envmapMaskTransformVar,
		envmapTintVar,
		-1, -1, -1,
		envmapContrastVar,	envmapSaturationVar );
}


//-----------------------------------------------------------------------------
// HDR-related methods lie below
//-----------------------------------------------------------------------------
void CBaseVSShader::InitParamsVertexLitAndUnlitGeneric_HDR(
	bool bVertexLitGeneric,													 
	int baseTextureVar, int frameVar,			int baseTextureTransformVar,
	int selfIllumTintVar,
	int brightnessTextureVar,
	int detailVar,		int detailFrameVar,		int detailScaleVar, 
	int envmapVar,		int envmapFrameVar, 
	int envmapMaskVar,	int envmapMaskFrameVar, int envmapMaskTransformVar,
	int envmapTintVar,	
	int bumpmapVar,     int bumpFrameVar,		int bumpTransformVar,
	int envmapContrastVar,	int envmapSaturationVar )
{
	InitParamsVertexLitAndUnlitGeneric_DX9( bVertexLitGeneric, baseTextureVar, frameVar, baseTextureTransformVar,
		selfIllumTintVar, detailVar, detailFrameVar, detailScaleVar, envmapVar, envmapFrameVar, 
		envmapMaskVar,envmapMaskFrameVar, envmapMaskTransformVar, envmapTintVar,
		bumpmapVar, bumpFrameVar, bumpTransformVar, envmapContrastVar, envmapSaturationVar );
}

void CBaseVSShader::InitVertexLitAndUnlitGeneric_HDR(
	bool bVertexLitGeneric,													 
	int baseTextureVar, int frameVar,			int baseTextureTransformVar,
	int selfIllumTintVar,
	int brightnessTextureVar,
	int detailVar,		int detailFrameVar,		int detailScaleVar, 
	int envmapVar,		int envmapFrameVar, 
	int envmapMaskVar,	int envmapMaskFrameVar, int envmapMaskTransformVar,
	int envmapTintVar,	
	int bumpmapVar,     int bumpFrameVar,		int bumpTransformVar,
	int envmapContrastVar,	int envmapSaturationVar )
{
	InitVertexLitAndUnlitGeneric_DX9( bVertexLitGeneric, baseTextureVar, frameVar, baseTextureTransformVar,
		selfIllumTintVar, detailVar, detailFrameVar, detailScaleVar, envmapVar, envmapFrameVar, 
		envmapMaskVar,envmapMaskFrameVar, envmapMaskTransformVar, envmapTintVar,
		bumpmapVar, bumpFrameVar, bumpTransformVar, envmapContrastVar, envmapSaturationVar );

	IMaterialVar** params = s_ppParams;
	if ( ( brightnessTextureVar != -1 ) && params[brightnessTextureVar]->IsDefined() )
	{
		LoadTexture( brightnessTextureVar );
	}
}

void CBaseVSShader::InitUnlitGeneric_HDR( 
	int baseTextureVar, int frameVar,			int baseTextureTransformVar, 
	int brightnessTextureVar,
	int detailVar,		int detailFrameVar,		int detailScaleVar, 
	int envmapVar,		int envmapFrameVar, 
	int envmapMaskVar,	int envmapMaskFrameVar, int envmapMaskTransformVar,
	int envmapTintVar,	int envmapContrastVar,	int envmapSaturationVar )
{
	InitVertexLitAndUnlitGeneric_HDR( false, baseTextureVar, frameVar, baseTextureTransformVar,
		-1, brightnessTextureVar, 
		detailVar, detailFrameVar, detailScaleVar, 
		envmapVar, envmapFrameVar, 
		envmapMaskVar,envmapMaskFrameVar, envmapMaskTransformVar, 
		envmapTintVar,
		-1, -1, -1, envmapContrastVar, envmapSaturationVar );
}

void CBaseVSShader::DrawVertexLitAndUnlitGenericPass_HDR( bool bVertexLitGeneric,
		int baseTextureVar, 
		int frameVar, 
		int baseTextureTransformVar, 
		int selfIllumTintVar,
		int brightnessTextureVar,
		int detailVar, 
		int detailFrameVar, 
		int detailScaleVar, 
		int envmapVar, 
		int envmapFrameVar, 
		int envmapMaskVar, 
		int envmapMaskFrameVar, 
		int envmapMaskTransformVar,
		int envmapTintVar, 
		int bumpmapVar, 
		int bumpFrameVar,
		int bumpTransformVar,
		int envmapContrastVar,
		int envmapSaturationVar,
		BlendType_t blendType,
		bool alphaTest,
		int pass )
{
#ifdef HDR
	IMaterialVar** params = s_ppParams;
	bool hasBaseTexture = params[baseTextureVar]->IsDefined();
	bool hasBrightnessTexture = (brightnessTextureVar != -1) && params[brightnessTextureVar]->IsDefined();
	bool hasDetailTexture = params[detailVar]->IsDefined();
	bool hasBump = (bumpmapVar != -1) && params[bumpmapVar]->IsDefined();
	bool hasEnvmap = params[envmapVar]->IsDefined();
	bool hasDiffuseLighting = bVertexLitGeneric;
	bool hasEnvmapMask = params[envmapMaskVar]->IsDefined();
	bool hasBaseAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK );
	bool hasSelfIllum = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );
	bool hasNormalMapAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	bool hasVertexColor = bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
	bool hasVertexAlpha = bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );
	bool bHasNormal = bVertexLitGeneric || hasEnvmap;
	if( ( hasDetailTexture && hasBump ) ||
		( hasEnvmapMask && hasBump ) ||
		( hasNormalMapAlphaEnvmapMask && hasBaseAlphaEnvmapMask ) ||
		( hasNormalMapAlphaEnvmapMask && hasEnvmapMask ) ||
		( hasBaseAlphaEnvmapMask && hasEnvmapMask ) ||
		( hasBaseAlphaEnvmapMask && hasSelfIllum ) )
	{
		// illegal combos
		Assert( 0 );
	}

	bool bNeedsAlpha = false;

	if( alphaTest )
	{
		// Shader needs alpha for alpha testing
		bNeedsAlpha = true;
	}
	else
	{
		// When blending shader needs alpha on the first pass
		if (((blendType == BT_BLEND) || (blendType == BT_BLENDADD)) && (pass == 0))
			bNeedsAlpha = true;
	}

	// Check if rendering alpha (second pass of blending)
	bool bRenderAlpha;
	if (((blendType == BT_BLEND) || (blendType == BT_BLENDADD)) && (pass == 1))
		bRenderAlpha = true;
	else
		bRenderAlpha = false;

	int vshIndex = 0;
	if( IsSnapshotting() )
	{
		SetInitialShadowState();

		// Alpha test: FIXME: shouldn't this be handled in Shader_t::SetInitialShadowState
		s_pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

		if (params[baseTextureVar]->IsDefined())
			SetDefaultBlendingShadowState( baseTextureVar, true );
		else
			SetDefaultBlendingShadowState( envmapMaskVar, false );
		
		// Alpha output
		if( !bNeedsAlpha || ( blendType == BT_BLEND ) )
			s_pShaderShadow->EnableAlphaWrites( true );

		// On second pass we just adjust alpha portion, so don't need color
		if( pass == 1 )
			s_pShaderShadow->EnableColorWrites( false );

		// For blending-type modes we need to setup separate alpha blending
		if( blendType == BT_BLEND )
		{
			s_pShaderShadow->EnableBlendingSeparateAlpha( true );

			if( pass == 0)
				s_pShaderShadow->BlendFuncSeparateAlpha( SHADER_BLEND_ZERO, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			else
				s_pShaderShadow->BlendFuncSeparateAlpha( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
		}
		else if( blendType == BT_BLENDADD )
		{
			if( pass == 1)
			{
				s_pShaderShadow->EnableBlendingSeparateAlpha( true );
				s_pShaderShadow->BlendFuncSeparateAlpha( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			}
		}

		unsigned int flags = VERTEX_POSITION;
		if( bHasNormal )
		{
			flags |= VERTEX_NORMAL;
		}

		int userDataSize = 0;
		if( hasBaseTexture )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
		}
		if( hasEnvmap )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			userDataSize = 4; // tangent S
		}
		if( hasDetailTexture )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );
		}
		if( hasBump )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE3, true );
			userDataSize = 4; // tangent S
		}
		if( hasEnvmapMask )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE4, true );
		}
		if( hasBrightnessTexture )
		{
			s_pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE5, true );
		}

		if( hasVertexColor || hasVertexAlpha )
		{
			flags |= VERTEX_COLOR;
		}
		
		// texcoord0 : base texcoord
		const int numTexCoords = 1;
		int numBoneWeights = 0;
		if( IS_FLAG_SET( MATERIAL_VAR_MODEL ) )
		{
			numBoneWeights = 3;
			flags |= VERTEX_BONE_INDEX;
		}
		
		s_pShaderShadow->VertexShaderOverbrightValue( g_pConfig->overbright );
		s_pShaderShadow->VertexShaderVertexFormat( 
			flags, numTexCoords, NULL, numBoneWeights, userDataSize );

		s_pShaderShadow->SetVertexShader( "vertexlit_and_unlit_generic_hdr_vs20" );

		vertexlit_and_unlit_generic_hdr_ps20_Index pshIndex;
		pshIndex.SetBASETEXTURE( hasBaseTexture );
		pshIndex.SetBRIGHTNESS( hasBrightnessTexture );
		pshIndex.SetDETAILTEXTURE( hasDetailTexture );
		pshIndex.SetBUMPMAP( hasBump );
		pshIndex.SetCUBEMAP( hasEnvmap );
		pshIndex.SetDIFFUSELIGHTING( hasDiffuseLighting );
		pshIndex.SetENVMAPMASK( hasEnvmapMask );
		pshIndex.SetBASEALPHAENVMAPMASK( hasBaseAlphaEnvmapMask );
		pshIndex.SetSELFILLUM( hasSelfIllum );
		pshIndex.SetNORMALMAPALPHAENVMAPMASK( hasNormalMapAlphaEnvmapMask );
		pshIndex.SetVERTEXCOLOR( hasVertexColor);
		pshIndex.SetVERTEXALPHA( hasVertexAlpha );
		pshIndex.SetNEEDSALPHA( bNeedsAlpha );
		pshIndex.SetRENDERALPHA( bRenderAlpha );
		s_pShaderShadow->SetPixelShader( "vertexlit_and_unlit_generic_hdr_ps20", pshIndex.GetIndex() );

		s_pShaderShadow->EnableSRGBWrite( true );
	}
	else
	{
		s_pShaderAPI->SetDefaultState();
		if( hasBaseTexture )
		{
			BindTexture( SHADER_TEXTURE_STAGE0, baseTextureVar, frameVar );
			s_pShaderAPI->EnableSRGBRead( SHADER_TEXTURE_STAGE0, true );
		}
		if( hasEnvmap )
		{
			BindTexture( SHADER_TEXTURE_STAGE1, envmapVar, envmapFrameVar );
			s_pShaderAPI->EnableSRGBRead( SHADER_TEXTURE_STAGE1, true );
		}
		if( hasDetailTexture )
		{
			BindTexture( SHADER_TEXTURE_STAGE2, detailVar, detailFrameVar );
		}
		if( hasBump )
		{
			if( !g_pConfig->m_bFastNoBump )
			{
				BindTexture( SHADER_TEXTURE_STAGE3, bumpmapVar, bumpFrameVar );
			}
			else
			{
				s_pShaderAPI->BindFlatNormalMap( SHADER_TEXTURE_STAGE3 );
			}
		}
		if( hasEnvmapMask )
		{
			BindTexture( SHADER_TEXTURE_STAGE4, envmapMaskVar, envmapMaskFrameVar );
		}
		if( hasBrightnessTexture )
		{
			BindTexture( SHADER_TEXTURE_STAGE5, brightnessTextureVar, frameVar );
		}

		vshIndex = ComputeVertexLitShaderIndex( bVertexLitGeneric, hasBump, hasEnvmap, hasVertexColor, bHasNormal );

		s_pShaderAPI->SetVertexShaderIndex( vshIndex );
		SetPixelShaderTextureTransform( 6, baseTextureTransformVar );
		if( hasDetailTexture )
		{
			SetPixelShaderTextureScaledTransform( 8, baseTextureTransformVar, detailScaleVar );
			Assert( !hasBump );
		}
		if( hasBump )
		{
			SetPixelShaderTextureTransform( 8, bumpTransformVar );
			Assert( !hasDetailTexture );
		}
		if( hasEnvmapMask )
		{
			SetPixelShaderTextureTransform( 10, envmapMaskTransformVar );
		}

		float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		IMaterialVar* pTintVar = s_ppParams[envmapTintVar];
		if( g_pConfig->bShowSpecular )
		{
			pTintVar->GetVecValue( color, 3 );
			color[3] = (color[0] + color[1] + color[2]) / 3.0f;
		}
		s_pShaderAPI->SetPixelShaderConstant( 0, color, 1 );

		SetModulationPixelShaderDynamicState( 1 );
		SetPixelShaderConstant( 2, envmapContrastVar );
		SetPixelShaderConstant( 3, envmapSaturationVar );
		EnablePixelShaderOverbright( 4, true, false );
		SetPixelShaderConstant( 5, selfIllumTintVar );
		FogToFogColor();
		SetAmbientCubeDynamicStateVertexShader();

		float preMultHack[4];
		if (((blendType == BT_BLEND) || (blendType == BT_BLENDADD)) && (pass == 1))
		{
			// Need to pre-multiply by alpha
			preMultHack[0] = 1.0f;
			preMultHack[1] = 0.0f;
		}
		else
		{
			preMultHack[0] = 0.0f;
			preMultHack[1] = 1.0f;
		}
		s_pShaderAPI->SetPixelShaderConstant( 8, preMultHack );

#if 1
		extern ConVar building_cubemaps;
		bool bBlendableOutput = !building_cubemaps.GetBool();
		vertexlit_and_unlit_generic_hdr_ps20_Index pshIndex;
		pshIndex.SetBLENDOUTPUT( bBlendableOutput );
		s_pShaderAPI->SetPixelShaderIndex( pshIndex.GetIndex() );
#endif
	}
	Draw();
#endif // HDR
}


//-----------------------------------------------------------------------------
// GR - translucency query
//-----------------------------------------------------------------------------
BlendType_t CBaseVSShader::EvaluateBlendRequirements( int textureVar, bool isBaseTexture )
{
	// Either we've got a constant modulation
	bool isTranslucent = IsAlphaModulating();

	// Or we've got a vertex alpha
	isTranslucent = isTranslucent || (CurrentMaterialVarFlags() & MATERIAL_VAR_VERTEXALPHA);

	// Or we've got a texture alpha (for blending or alpha test)
	isTranslucent = isTranslucent || ( TextureIsTranslucent( textureVar, isBaseTexture ) &&
		                               !(CurrentMaterialVarFlags() & MATERIAL_VAR_ALPHATEST ) );

	if ( CurrentMaterialVarFlags() & MATERIAL_VAR_ADDITIVE )
	{
		// Additive
		return isTranslucent ? BT_BLENDADD : BT_ADD;
	}
	else
	{
		// Normal blending
		return isTranslucent ? BT_BLEND : BT_NONE;
	}
}


void CBaseVSShader::DrawVertexLitAndUnlitGeneric_HDR( 
	bool bVertexLitGeneric,
	int baseTextureVar, int frameVar, int baseTextureTransformVar, 
	int selfIllumTintVar, int brightnessTextureVar,
	int detailVar, int detailFrameVar, int detailScaleVar, 
	int envmapVar, int envmapFrameVar, 
	int envmapMaskVar, int envmapMaskFrameVar, int envmapMaskTransformVar,
	int envmapTintVar, 
	int bumpmapVar, int bumpFrameVar, int bumpTransformVar,
	int envmapContrastVar, int envmapSaturationVar )
{
	IMaterialVar** params = s_ppParams;
	bool alphaTest = IS_FLAG_SET(MATERIAL_VAR_ALPHATEST);
	BlendType_t blendType;

	if (params[baseTextureVar]->IsDefined())
	{
		blendType = EvaluateBlendRequirements( baseTextureVar, true );
	}
	else
	{
		blendType = EvaluateBlendRequirements( envmapMaskVar, false );
	}

	DrawVertexLitAndUnlitGenericPass_HDR(
		bVertexLitGeneric,
		baseTextureVar, 
		frameVar, 
		baseTextureTransformVar, 
		selfIllumTintVar,
		brightnessTextureVar,
		detailVar, 
		detailFrameVar, 
		detailScaleVar, 
		envmapVar, 
		envmapFrameVar, 
		envmapMaskVar, 
		envmapMaskFrameVar, 
		envmapMaskTransformVar,
		envmapTintVar, 
		bumpmapVar, 
		bumpFrameVar,
		bumpTransformVar,
		envmapContrastVar,
		envmapSaturationVar,
		blendType, alphaTest, 0 );

	if(!alphaTest && ((blendType == BT_BLEND) || (blendType == BT_BLENDADD)))
	{
		// Second pass to complete 
		DrawVertexLitAndUnlitGenericPass_HDR(
			bVertexLitGeneric,
			baseTextureVar, 
			frameVar, 
			baseTextureTransformVar, 
			selfIllumTintVar,
			brightnessTextureVar,
			detailVar, 
			detailFrameVar, 
			detailScaleVar, 
			envmapVar, 
			envmapFrameVar, 
			envmapMaskVar, 
			envmapMaskFrameVar, 
			envmapMaskTransformVar,
			envmapTintVar, 
			bumpmapVar, 
			bumpFrameVar,
			bumpTransformVar,
			envmapContrastVar,
			envmapSaturationVar,
			blendType, alphaTest, 1 );
	}
}


void CBaseVSShader::DrawUnlitGeneric_HDR( 
	int baseTextureVar,	int frameVar,			int baseTextureTransformVar, 
	int brightnessTextureVar,
	int detailVar,		int detailFrameVar, 	int detailScaleVar, 
	int envmapVar, 		int envmapFrameVar, 
	int envmapMaskVar, 	int envmapMaskFrameVar, int envmapMaskTransformVar,
	int envmapTintVar, 
	int envmapContrastVar, int envmapSaturationVar )
{
	DrawVertexLitAndUnlitGeneric_HDR( false,
		baseTextureVar,		frameVar,			baseTextureTransformVar,
		-1,
		brightnessTextureVar,
		detailVar,			detailFrameVar,		detailScaleVar,
		envmapVar,			envmapFrameVar,
		envmapMaskVar,		envmapMaskFrameVar,	envmapMaskTransformVar,
		envmapTintVar,
		-1, -1, -1,
		envmapContrastVar, envmapSaturationVar );

}


