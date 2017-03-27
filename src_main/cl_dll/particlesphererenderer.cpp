//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "ParticleSphereRenderer.h"
#include "materialsystem/IMaterialVar.h"



CParticleSphereRenderer::CParticleSphereRenderer()
{
	// Initialize the lights.
	for( int i=0; i < NUM_TOTAL_LIGHTS; i++ )
	{
		m_Lights[i].m_flIntensity = 0;
		m_Lights[i].m_vColor.Init( 0, 0, 0 );
		m_Lights[i].m_vPos.Init( 0, 0, 0 );
	}

	memset( m_cDirLightColor, 0, sizeof(m_cDirLightColor) );
	m_bUsingPixelShaders = false;
	m_CommonDirLight = -1;
}


void CParticleSphereRenderer::Init( IMaterial *pMaterial )
{
	// Figure out how we need to draw.
	bool bFound = false;
	IMaterialVar *pVar = pMaterial->FindVar( "$USINGPIXELSHADER", &bFound );
	if( bFound && pVar && pVar->GetIntValue() )
		m_bUsingPixelShaders = true;
	else
		m_bUsingPixelShaders = false;
}


void CParticleSphereRenderer::Update( CParticleMgr *pMgr )
{
	if( m_CommonDirLight == -1 )
	{
		m_flLightIndex = (float)pMgr->AllocatePointSourceLight( DirectionalLight().m_vPos, DirectionalLight().m_flIntensity );
	}
	else
	{
		m_flLightIndex = (float)m_CommonDirLight;
		
		// Copy lighting parameters from the appropriate light..
		CParticleMgr::CPointSourceLight const *pLight = &pMgr->GetPointSourceLights()[m_CommonDirLight / 2];
		DirectionalLight().m_vPos = pLight->m_vPos;
		DirectionalLight().m_flIntensity = pLight->m_flIntensity;
	}

	m_cDirLightColor[0] = (unsigned char)( DirectionalLight().m_vColor.x * 255.4f );
	m_cDirLightColor[1] = (unsigned char)( DirectionalLight().m_vColor.y * 255.4f );
	m_cDirLightColor[2] = (unsigned char)( DirectionalLight().m_vColor.z * 255.4f );
}


void CParticleSphereRenderer::SetCommonDirLight( CParticleMgr *pMgr, int iCommon )
{
	m_CommonDirLight = iCommon;
}



