//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "maplight.h"
#include "worldcraft.h"


IMPLEMENT_MAPCLASS( CMapLight );


CMapClass* CMapLight::CreateMapLight(CHelperInfo *pHelperInfo, CMapEntity *pParent)
{
	return new CMapLight;
}


void CMapLight::OnParentKeyChanged(LPCSTR key, LPCSTR value)
{
	APP()->SetForceRenderNextFrame();
}


CMapClass *CMapLight::Copy(bool bUpdateDependencies)
{
	APP()->SetForceRenderNextFrame();

	CMapLight *pNew = new CMapLight;
	pNew->CopyFrom( this, bUpdateDependencies );
	return pNew;
}


//-----------------------------------------------------------------------------
// Purpose: Never select anything because of this helper.
//-----------------------------------------------------------------------------
CMapClass *CMapLight::PrepareSelection(SelectMode_t eSelectMode)
{
	return NULL;
}
