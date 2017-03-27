//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPLIGHT_H
#define MAPLIGHT_H
#ifdef _WIN32
#pragma once
#endif


#include "MapHelper.h"
#include "HelperInfo.h"
#include "MapEntity.h"


class CMapLight : public CMapHelper
{
public:

	DECLARE_MAPCLASS(CMapLight);


	static CMapClass *CreateMapLight(CHelperInfo *pHelperInfo, CMapEntity *pParent);

	virtual void OnParentKeyChanged(LPCSTR key, LPCSTR value);
	virtual CMapClass *Copy(bool bUpdateDependencies);

	virtual CMapClass *PrepareSelection(SelectMode_t eSelectMode);
};


#endif // MAPLIGHT_H
