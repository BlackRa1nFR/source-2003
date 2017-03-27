//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPSPHERE_H
#define MAPSPHERE_H
#pragma once


#include "KeyValues.h"
#include "MapHelper.h"
#include "ToolInterface.h"


class CToolSphere;
class CHelperInfo;
class CRender2D;
class CRender3D;
class IMesh;


class CMapSphere : public CMapHelper
{
	friend class CToolSphere;

	public:

		DECLARE_MAPCLASS(CMapSphere)

		//
		// Factory for building from a list of string parameters.
		//
		static CMapClass *Create(CHelperInfo *pInfo, CMapEntity *pParent);

		//
		// Construction/destruction:
		//
		CMapSphere(void);
		~CMapSphere(void);

		virtual void CalcBounds(BOOL bFullUpdate = FALSE);

		virtual CMapClass *Copy(bool bUpdateDependencies);
		virtual CMapClass *CopyFrom(CMapClass *pFrom, bool bUpdateDependencies);

		virtual void OnParentKeyChanged(const char *szKey, const char *szValue);

		virtual void Render2D(CRender2D *pRender);
		virtual void Render3D(CRender3D *pRender);

		virtual int SerializeRMF(fstream &File, BOOL bRMF) { return(0); }
		virtual int SerializeMAP(fstream &File, BOOL bRMF) { return(0); }

		virtual bool IsVisualElement(void) { return false; } // Only visible when the parent entity is selected.
		virtual bool IsScaleable(void) { return false; } // TODO: allow for scaling the sphere by itself
		virtual bool IsClutter(void) { return true; }
		virtual bool IsCulledByCordon(void) { return false; } // We don't hide unless our parent hides.

		virtual CBaseTool *GetToolObject(int nHitData);
		
		virtual CMapClass *HitTest2D(CMapView2D *pView, const Vector2D &vecPoint, int &nHitData);

		virtual LPCTSTR GetDescription() { return "Sphere helper"; }

	protected:

		void SetRadius(float flRadius);
		void RenderHandle(CRender3D *pRender, IMesh *pMesh, const Vector2D &ScreenPos, unsigned char *color);

		char m_szKeyName[KEYVALUE_MAX_KEY_LENGTH];
		float m_flRadius;
};

#endif // MAPSPHERE_H
