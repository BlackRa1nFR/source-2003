//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: This module implements all the proxies used by the particle systems.
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "particlemgr.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterialvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// This was specified in the .dsp with the /Od flag, but that causes a warning since
//  it's inconsistent with the release .pch, so just disable optimizations here instead
// FIXME:  Is this even needed any more?
#pragma optimize( "", off )


// ------------------------------------------------------------------------ //
// ParticleSphereProxy
// ------------------------------------------------------------------------ //

class ParticleSphereProxy : public IMaterialProxy
{
// IMaterialProxy overrides.
public:
	class CParticleSphereLightInfo
	{
	public:
		Vector		*m_pPositions;
		float		*m_pIntensities;
		int			m_nLights;
	};


	ParticleSphereProxy()
	{
		m_pLights = NULL;
		m_LightInfo.m_pPositions = m_Positions;
		m_LightInfo.m_pIntensities = m_Intensities;
	}

	virtual		~ParticleSphereProxy() 
	{
	}
	
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		m_pLights = pMaterial->FindVar( "$lights", NULL, false );
		return true;
	}

	virtual void OnBind( void *pvParticleMgr )
	{
		if( !pvParticleMgr || !m_pLights )
			return;

		CParticleMgr *pMgr = (CParticleMgr*)pvParticleMgr;

		// Specify all the lights.
		for( int i=0; i < pMgr->GetNumPointSourceLights(); i++ )
		{
			CParticleMgr::CPointSourceLight const *pLight = &pMgr->GetPointSourceLights()[i];

			TransformParticle( 
				pMgr->GetModelView(), 
				pLight->m_vPos, 
				m_Positions[i] );
			
			m_Intensities[i] = pLight->m_flIntensity;
		}

		m_pLights->SetFourCCValue( 
			MAKE_MATERIALVAR_FOURCC('P','S','L','I'),
			&m_LightInfo );
		
		m_LightInfo.m_nLights = pMgr->GetNumPointSourceLights();
	}

	virtual void	Release( void ) { delete this; }

private:
	CParticleSphereLightInfo	m_LightInfo;

	Vector			m_Positions[CParticleMgr::MAX_POINTSOURCE_LIGHTS];
	float			m_Intensities[CParticleMgr::MAX_POINTSOURCE_LIGHTS];
	
	IMaterialVar	*m_pLights;
};

EXPOSE_INTERFACE( ParticleSphereProxy, IMaterialProxy, "ParticleSphereProxy" IMATERIAL_PROXY_INTERFACE_VERSION );

#pragma optimize( "", on )

