//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "ProxyEntity.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"

// $sineVar : name of variable that controls the alpha level (float)
class CAlphaMaterialProxy : public CEntityMaterialProxy
{
public:
	CAlphaMaterialProxy();
	virtual ~CAlphaMaterialProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( C_BaseEntity *pEntity );

private:
	IMaterialVar *m_AlphaVar;
};

CAlphaMaterialProxy::CAlphaMaterialProxy()
{
	m_AlphaVar = NULL;
}

CAlphaMaterialProxy::~CAlphaMaterialProxy()
{
}


bool CAlphaMaterialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;
	m_AlphaVar = pMaterial->FindVar( "$alpha", &foundVar, false );
	return foundVar;
}

void CAlphaMaterialProxy::OnBind( C_BaseEntity *pEnt )
{
	if (m_AlphaVar)
	{
		m_AlphaVar->SetFloatValue( pEnt->m_clrRender->a );
	}
}

EXPOSE_INTERFACE( CAlphaMaterialProxy, IMaterialProxy, "Alpha" IMATERIAL_PROXY_INTERFACE_VERSION );
