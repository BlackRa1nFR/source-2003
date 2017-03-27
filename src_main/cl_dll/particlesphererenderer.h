//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PARTICLESPHERERENDERER_H
#define PARTICLESPHERERENDERER_H
#ifdef _WIN32
#pragma once
#endif


#include "particlemgr.h"
#include "particle_util.h"


class CParticleSphereRenderer
{
public:
	class CLightInfo
	{
	public:
		Vector	m_vPos;
		Vector	m_vColor;	// 0-1
		float	m_flIntensity;
	};

	enum
	{
		NUM_AMBIENT_LIGHTS=2,
		NUM_TOTAL_LIGHTS = NUM_AMBIENT_LIGHTS + 1
	};


public:

				CParticleSphereRenderer();

	// Initialize and tell it the material you'll be using.
	void		Init( IMaterial *pMaterial );
	
	// Call this from your particle system's update function after you've
	// updated any lights you want to update.
	void		Update( CParticleMgr *pMgr );

	// Call this to	render a spherical particle.
	void		RenderParticle( 
		ParticleDraw *pDraw, 
		const Vector &vOriginalPos,
		const Vector &vTransformedPos,
		float flAlpha,			// value 0 - 255.4
		float flParticleSize ); 
	
	// Call this to	render a spherical particle.
	inline void		RenderParticle_AddColor( 
		ParticleDraw *pDraw, 
		const Vector &vOriginalPos,
		const Vector &vTransformedPos,
		float flAlpha,			// value 0 - 255.4
		float flParticleSize,
		const Vector &vColor	// value 0 - 255.4
		); 

	// Use these to access the lights.
	inline CLightInfo&	AmbientLight( int index );		// This gets lights between 0 and NUM_AMBIENT_LIGHTS.
	inline CLightInfo&	Light( int index );				// This gets either ambients or directional.
	inline CLightInfo&	DirectionalLight();				// Returns the directional light.

	// Set the directional light to use one of the common directions (like 
	// lit from the bottom or lit from the top). Pass in one of the 
	// CParticleMgr::POINTSOURCE_INDEX codes. If you pass in -1, then the actual
	// position and intensity of your dirlight will be used.
	// (default value is -1).
	void		SetCommonDirLight( CParticleMgr *pMgr, int iCommon );


private:

	void		AddLightColor( 
		Vector const *pPos,
		Vector const *pLightPos, 
		Vector const *pLightColor, 
		float flLightIntensity,
		Vector *pOutColor );

	inline void	ClampColor( Vector &vColor );


private:
	
	// Lights. The first NUM_AMBIENT_LIGHTS ones are ambient and the last one 
	// is directional.
	CLightInfo		m_Lights[NUM_TOTAL_LIGHTS];

	// This is either one of the CParticleMgr::POINTSOURCE_INDEX codes or -1.
	// If -1, then AllocatePointSourceLight is called each frame to get a light index.
	// Otherwise, the straight index is used.
	int				m_CommonDirLight;

	// Precalculated to fill in the vertices faster.
	unsigned char	m_cDirLightColor[4];

	bool			m_bUsingPixelShaders;
	
	float			m_flLightIndex;			// given to the vertex shader.
};


// ------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------ //

inline void CParticleSphereRenderer::AddLightColor( 
	Vector const *pPos,
	Vector const *pLightPos, 
	Vector const *pLightColor, 
	float flLightIntensity,
	Vector *pOutColor )
{
	if( flLightIntensity )
	{
		float fDist = pPos->DistToSqr( *pLightPos );
		float fAmt;
		if( fDist > 0.0001f )
			fAmt = flLightIntensity * 255.0f / fDist;
		else
			fAmt = 1000.f;

		if( fAmt > 255.4f )
			fAmt = 255.4f;

		*pOutColor += *pLightColor * fAmt;
	}
}


inline void CParticleSphereRenderer::ClampColor( Vector &vColor )
{
	float flMax = max( vColor.x, max( vColor.y, vColor.z ) );
	if( flMax > 255.4f )
	{
		vColor *= 255.4f / flMax;
	}
}


inline CParticleSphereRenderer::CLightInfo& CParticleSphereRenderer::AmbientLight( int index )
{
	assert( index >= 0 && index < CParticleSphereRenderer::NUM_AMBIENT_LIGHTS );
	return m_Lights[index];
}


inline CParticleSphereRenderer::CLightInfo& CParticleSphereRenderer::Light( int index )
{
	assert( index >= 0 && index < CParticleSphereRenderer::NUM_TOTAL_LIGHTS );
	return m_Lights[index];
}


inline CParticleSphereRenderer::CLightInfo& CParticleSphereRenderer::DirectionalLight()
{
	return m_Lights[NUM_AMBIENT_LIGHTS];
}


inline void CParticleSphereRenderer::RenderParticle( 
	ParticleDraw *pDraw,
	const Vector &vOriginalPos,
	const Vector &vTransformedPos,
	float flAlpha,
	float flParticleSize )
{
	Vector vColor( 0, 0, 0 );
	for( int i=0; i < NUM_AMBIENT_LIGHTS; i++ )
	{
		AddLightColor( &vOriginalPos, &m_Lights[i].m_vPos, &m_Lights[i].m_vColor, m_Lights[i].m_flIntensity, &vColor );
	}
	
	if( !m_bUsingPixelShaders )
	{
		// Add the dirlight color.
		CLightInfo *pDirLight = &m_Lights[NUM_AMBIENT_LIGHTS];
		AddLightColor( &vOriginalPos, &pDirLight->m_vPos, &pDirLight->m_vColor, pDirLight->m_flIntensity, &vColor );
	}
	
	ClampColor( vColor );

	RenderParticle_Color255SizeSpecularTCoord3(
		pDraw,
		vTransformedPos,
		vColor,			// ambient color
		flAlpha,		// alpha
		flParticleSize,	// size
		m_cDirLightColor,
		m_flLightIndex // light index
		);
}


inline void CParticleSphereRenderer::RenderParticle_AddColor( 
	ParticleDraw *pDraw,
	const Vector &vOriginalPos,
	const Vector &vTransformedPos,
	float flAlpha,
	float flParticleSize,
	const Vector &vInColor )
{
	Vector vColor = vInColor;
	for( int i=0; i < NUM_AMBIENT_LIGHTS; i++ )
	{
		AddLightColor( &vOriginalPos, &m_Lights[i].m_vPos, &m_Lights[i].m_vColor, m_Lights[i].m_flIntensity, &vColor );
	}
	
	if( !m_bUsingPixelShaders )
	{
		// Add the dirlight color.
		CLightInfo *pDirLight = &m_Lights[NUM_AMBIENT_LIGHTS];
		AddLightColor( &vOriginalPos, &pDirLight->m_vPos, &pDirLight->m_vColor, pDirLight->m_flIntensity, &vColor );
	}
	
	ClampColor( vColor );

	RenderParticle_Color255SizeSpecularTCoord3(
		pDraw,
		vTransformedPos,
		vColor,			// ambient color
		flAlpha,		// alpha
		flParticleSize,	// size
		m_cDirLightColor,
		m_flLightIndex // light index
		);
}


#endif // PARTICLESPHERERENDERER_H
