//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "proxyentity.h"
#include "IClientRenderable.h"

//-----------------------------------------------------------------------------
// Cleanup
//-----------------------------------------------------------------------------
void CEntityMaterialProxy::Release( void )
{ 
	delete this; 
}

//-----------------------------------------------------------------------------
// Helper class to deal with floating point inputs
//-----------------------------------------------------------------------------
void CEntityMaterialProxy::OnBind( void *pRenderable )
{
	if( !pRenderable )
		return;

	IClientRenderable *pRend = (IClientRenderable *)pRenderable;
	C_BaseEntity *pEnt = pRend->GetIClientUnknown()->GetBaseEntity();
	if (pEnt)
	{
		OnBind(pEnt);
	}
}