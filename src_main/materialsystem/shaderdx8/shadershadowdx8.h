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
// The dx8 implementation of the shader API
//=============================================================================

#ifndef SHADERSHADOWDX8_H
#define SHADERSHADOWDX8_H

#include "locald3dtypes.h"
#include "ShaderAPI.h"

class IShaderAPIDX8;

//-----------------------------------------------------------------------------
// Important enumerations
//-----------------------------------------------------------------------------
enum
{
	NUM_SIMULTANEOUS_TEXTURES = 8,
	NUM_TEXTURE_STAGES = 8,
};


//-----------------------------------------------------------------------------
// A structure maintaining the shadowed board state
//-----------------------------------------------------------------------------
struct TextureStageShadowState_t
{
	// State shadowing affects these
	D3DTEXTUREOP	m_ColorOp;
	int				m_ColorArg1;
	int				m_ColorArg2;
	D3DTEXTUREOP	m_AlphaOp;
	int				m_AlphaArg1;
	int				m_AlphaArg2;
	int				m_TexCoordIndex;
	bool			m_TextureEnable;
	bool			m_GenerateSphericalCoords;
};

struct ShadowState_t
{
	// Depth buffering state
	D3DCMPFUNC		m_ZFunc;
	bool			m_ZWriteEnable;
	D3DZBUFFERTYPE	m_ZEnable;
	bool			m_ZBias;

	// Cull state
	bool			m_CullEnable;

	// Write enable
	DWORD			m_ColorWriteEnable;

	// Fill mode
	D3DFILLMODE		m_FillMode;

	// Modulate constant color into the vertex color
	bool			m_ModulateConstantColor;

	// Lighting in hardware?
	bool			m_Lighting;
	bool			m_SpecularEnable;

	// Auto-convert from linear to gamma upon writing to the frame buffer?
	bool			m_SRGBWriteEnable;

	// Vertex shader overbright value
	float			m_VertexShaderOverbright;

	// Alpha state
	bool			m_AlphaBlendEnable;
	D3DBLEND		m_SrcBlend;
	D3DBLEND		m_DestBlend;

	// Separate alpha bland state
	bool			m_SeparateAlphaBlendEnable;
	D3DBLEND		m_SrcBlendAlpha;
	D3DBLEND		m_DestBlendAlpha;

	bool			m_AlphaTestEnable;
	D3DCMPFUNC		m_AlphaFunc;
	int				m_AlphaRef;

	// Fixed function?
	bool			m_UsingFixedFunction;

	// Vertex blending.
	bool			m_VertexBlendEnable;

	// FIXME: This is only used to optimize model rendering to help us know
	// when we don't have to set the texture transform to identity during reset
	// Ambient cube?
	bool			m_AmbientCubeOnStage0;

	// Vertex data used by this snapshot
	// Note that the vertex format actually used will be the
	// aggregate of the vertex formats used by all snapshots in a material
	VertexFormat_t	m_VertexUsage;

	// Texture stage state
	TextureStageShadowState_t m_TextureStage[NUM_TEXTURE_STAGES];
};


//-----------------------------------------------------------------------------
// These are part of the "shadow" since they describe the shading algorithm
// but aren't actually captured in the state transition table 
// because it would produce too many transitions
//-----------------------------------------------------------------------------
struct ShadowShaderState_t
{
	// The vertex + pixel shader group to use...
	VertexShader_t	m_VertexShader;
	PixelShader_t	m_PixelShader;

	// The static vertex + pixel shader indices
	int				m_nStaticVshIndex;
	int				m_nStaticPshIndex;
};


//-----------------------------------------------------------------------------
// The shader setup API
//-----------------------------------------------------------------------------
class IShaderShadowDX8 : public IShaderShadow
{
public:
	// Initializes it
	virtual void Init() = 0;

	// Gets at the shadow state
	virtual ShadowState_t const& GetShadowState() = 0;
	virtual ShadowShaderState_t const& GetShadowShaderState() = 0;

	// This must be called right before taking a snapshot
	virtual void ComputeAggregateShadowState( ) = 0;

	// Class factory methods
	static IShaderShadowDX8* Create( IShaderAPIDX8* pShaderAPIDX8 );
	static void Destroy( IShaderShadowDX8* pShaderShadow );
};


#endif // SHADERSHADOWDX8_H
