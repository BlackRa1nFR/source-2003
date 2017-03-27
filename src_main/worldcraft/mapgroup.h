//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPGROUP_H
#define MAPGROUP_H
#pragma once

#include "MapClass.h"


class CMapGroup : public CMapClass
{
	public:

		DECLARE_MAPCLASS(CMapGroup)

		LPCTSTR GetDescription(void);

		virtual CMapClass *Copy(bool bUpdateDependencies);
		virtual CMapClass *CopyFrom(CMapClass *pFrom, bool bUpdateDependencies);

		virtual bool IsGroup(void) { return true; }

		void AddChild(CMapClass *pChild);

		//
		// Serialization.
		//
		ChunkFileResult_t LoadVMF(CChunkFile *pFile);
		ChunkFileResult_t SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo);
};

#endif // MAPGROUP_H
