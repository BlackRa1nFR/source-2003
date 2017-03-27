//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "MapStudioModel.h"
#include "ModelFactory.h"


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eModelType - 
//			pszModelData - 
// Output : 
//-----------------------------------------------------------------------------
CMapClass *CModelFactory::CreateModel(ModelType_t eModelType, const char *pszModelData)
{
	switch (eModelType)
	{
		case ModelTypeStudio:
		{
			int nLen = strlen(pszModelData);
			if ((nLen > 4) && (!stricmp(&pszModelData[nLen - 4], ".mdl")))
			{
				CMapStudioModel *pModel = new CMapStudioModel(pszModelData);
				return(pModel);
			}
			break;
		}

		default:
		{
			break;
		}
	}

	return(NULL);
}
