//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPLIGHTCONE_H
#define MAPLIGHTCONE_H
#pragma once


#include "MapHelper.h"
#include "MapFace.h"


class CHelperInfo;
class CRender3D;


class CMapLightCone : public CMapHelper
{
	public:

		DECLARE_MAPCLASS(CMapLightCone)

		//
		// Factory for building from a list of string parameters.
		//
		static CMapClass *Create(CHelperInfo *pInfo, CMapEntity *pParent);

		//
		// Construction/destruction:
		//
		CMapLightCone(void);
		~CMapLightCone(void);

		void CalcBounds(BOOL bFullUpdate = FALSE);

		virtual CMapClass *Copy(bool bUpdateDependencies);
		virtual CMapClass *CopyFrom(CMapClass *pFrom, bool bUpdateDependencies);

		void Render3D(CRender3D *pRender);

		int SerializeRMF(fstream &File, BOOL bRMF);
		int SerializeMAP(fstream &File, BOOL bRMF);

		virtual void PostloadWorld(CMapWorld *pWorld);

		virtual bool IsVisualElement(void) { return(false); } // Only visible when parent entity is selected.
		virtual bool IsClutter(void) { return true; }
		virtual bool IsCulledByCordon(void) { return false; } // We don't hide unless our parent hides.

		virtual CMapClass *PrepareSelection(SelectMode_t eSelectMode);
		
		LPCTSTR GetDescription() { return("Light cone helper"); }

		void OnParentKeyChanged( LPCSTR key, LPCSTR value );
		bool ShouldRenderLast(void) { return(true); }

	protected:

		//
		// Implements CMapAtom transformation functions.
		//
		void DoTransform(Box3D *t);
		void DoTransRotate(const Vector *pRefPoint, const QAngle &Angles);
		void DoTransFlip(const Vector &RefPoint);

		void BuildCone(void);
		void GetAngles(QAngle& fAngles);
		float GetBrightnessAtDist(float fDistance);
		float GetLightDist(float fBrightness);
		void SetAngles(const QAngle &Angles);
		bool SolveQuadratic(float &x, float y, float A, float B, float C);

		float m_fBrightness;

		float m_fQuadraticAttn;
		float m_fLinearAttn;
		float m_fConstantAttn;

		float m_fInnerConeAngle;
		float m_fOuterConeAngle;

		QAngle m_Angles;

		bool m_bPitchSet;
		float m_fPitch;

		float m_fFocus;

		CMapFaceList m_Faces;
};

#endif // MAPLIGHTCONE_H
