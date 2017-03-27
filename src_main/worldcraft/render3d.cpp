//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <windows.h>
#include "Render3D.h"
#include "Render3DMS.h"


CRender3D *CreateRender3D(Render3DType_t eRender3DType)
{
	switch (eRender3DType)
	{
		case Render3DTypeMaterialSystem:
		{
			return(new CRender3DMS());
		}
	}

	return(NULL);
}
