//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPSTUDIOMODEL_H
#define MAPSTUDIOMODEL_H
#pragma once

#include "MapHelper.h"
#include "StudioModel.h"


class CRender2D;
class CRender3D;


class CMapStudioModel : public CMapHelper
{
	public:

		//
		// Factories.
		//
		static CMapClass *CreateMapStudioModel(CHelperInfo *pHelperInfo, CMapEntity *pParent);
		static CMapStudioModel *CreateMapStudioModel(const char *pszModelPath, bool bOrientedBBox, bool bReversePitch);

		static void AdvanceAnimation(float flInterval);

		//
		// Construction/destruction:
		//
		CMapStudioModel(void);
		~CMapStudioModel(void);

		DECLARE_MAPCLASS(CMapStudioModel)

		void CalcBounds(BOOL bFullUpdate = FALSE);

		virtual CMapClass *Copy(bool bUpdateDependencies);
		virtual CMapClass *CopyFrom(CMapClass *pFrom, bool bUpdateDependencies);

		void Initialize(void);

		void Render2D(CRender2D *pRender);
		void Render3D(CRender3D *pRender);

		void GetAngles(QAngle& pfAngles);
		void SetAngles(QAngle& fAngles);

		void OnParentKeyChanged(LPCSTR szKey, LPCSTR szValue);

		bool RenderPreload(CRender3D *pRender, bool bNewContext);

		int SerializeRMF(fstream &File, BOOL bRMF);
		int SerializeMAP(fstream &File, BOOL bRMF);

		static void SetRenderDistance(float fRenderDistance);
		static void EnableAnimation(BOOL bEnable);

		bool IsVisualElement(void) { return(true); }
		
		bool ShouldRenderLast();

		LPCTSTR GetDescription() { return("Studio model"); }

		int GetFrame(void);
		void SetFrame(int nFrame);

		int GetSequence(void);
		int GetSequenceCount(void);
		void GetSequenceName(int nIndex, char *szName);
		void SetSequence(int nIndex);

	protected:
		
		//
		// Implements CMapAtom transformation functions.
		//
		void DoTransform(Box3D *t);
		void DoTransRotate(const Vector *pRefPoint, const QAngle &Angles);
		void DoTransFlip(const Vector &RefPoint);

		inline void ReversePitch(bool bReversePitch);
		inline void SetOrientedBounds(bool bOrientedBounds);

		StudioModel *m_pStudioModel;		// Pointer to a studio model in the model cache.
		QAngle m_Angles;					// Euler angles of this studio model.
		int	m_Skin;							// the model skin

		bool m_bOrientedBounds;				// Whether the bounding box should consider the orientation of the model.
											// Note that this is not a true oriented bounding box, but an axial box
											// indicating the extents of the oriented model.

		bool m_bReversePitch;				// Lights negate pitch, so models representing light sources in Hammer
											// must do so as well.

		//
		// Data that is common to all studio models.
		//
		static float m_fRenderDistance;		// Distance beyond which studio models render as bounding boxes.
		static BOOL m_bAnimateModels;		// Whether to animate studio models.
};


//-----------------------------------------------------------------------------
// Purpose: Sets whether this object has an oriented or axial bounding box.
//			Note that this is not a true oriented bounding box, but an axial box
//			indicating the extents of the oriented model.
//-----------------------------------------------------------------------------
void CMapStudioModel::SetOrientedBounds(bool bOrientedBounds)
{
	m_bOrientedBounds = bOrientedBounds;
}


//-----------------------------------------------------------------------------
// Purpose: Sets whether this object negates pitch.
//-----------------------------------------------------------------------------
void CMapStudioModel::ReversePitch(bool bReversePitch)
{
	m_bReversePitch = bReversePitch;
}


#endif // MAPSTUDIOMODEL_H

