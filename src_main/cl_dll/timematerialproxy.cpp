//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "FunctionProxy.h"

class CTimeMaterialProxy : public CResultProxy
{
public:
	virtual void OnBind( void *pC_BaseEntity );
private:
	IMaterialVar *m_pTimeVar;
};

void CTimeMaterialProxy::OnBind( void *pC_BaseEntity )
{
	SetFloatResult( gpGlobals->curtime );
}

EXPOSE_INTERFACE( CTimeMaterialProxy, IMaterialProxy, "Time" IMATERIAL_PROXY_INTERFACE_VERSION );
