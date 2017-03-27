//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "materialsystem/IMaterialProxy.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"

// $sineVar : name of variable that controls the alpha level (float)
class CzTestMaterialProxy : public IMaterialProxy
{
public:
	CzTestMaterialProxy();
	virtual ~CzTestMaterialProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release( void ) { delete this; }
private:
	IMaterialVar *m_AlphaVar;
};

CzTestMaterialProxy::CzTestMaterialProxy()
{
	m_AlphaVar = NULL;
}

CzTestMaterialProxy::~CzTestMaterialProxy()
{
}


bool CzTestMaterialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;
	m_AlphaVar = pMaterial->FindVar( "$alpha", &foundVar, false );
	return foundVar;
}

void CzTestMaterialProxy::OnBind( void *pC_BaseEntity )
{
	C_BaseEntity *pEnt = ( C_BaseEntity * )pC_BaseEntity;
	if( !pEnt )
	{
		return;
	}

	if (m_AlphaVar)
	{
		m_AlphaVar->SetFloatValue( pEnt->m_clrRender.a );
	}

}

EXPOSE_INTERFACE( CzTestMaterialProxy, IMaterialProxy, "zTest" IMATERIAL_PROXY_INTERFACE_VERSION );
