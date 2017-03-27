//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef MATERIALPATCH_H
#define MATERIALPATCH_H
#ifdef _WIN32
#pragma once
#endif

#include "utilmatlib.h"

void CreateMaterialPatch( const char *pOriginalMaterialName, const char *pNewMaterialName,
						 const char *pNewKey, const char *pNewValue );

bool GetValueFromMaterial( const char *pMaterialName, const char *pKey, char *pValue, int len );

const char *GetOriginalMaterialNameForPatchedMaterial( const char *pPatchMaterialName );

MaterialSystemMaterial_t FindOriginalMaterial( const char *materialName, bool *pFound, bool bComplain = true );

#endif // MATERIALPATCH_H
