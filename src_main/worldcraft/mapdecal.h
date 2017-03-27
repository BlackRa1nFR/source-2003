//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines a decal helper. The decal attaches itself to nearby
//			solids, dynamically creating decal faces as necessary.
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPDECAL_H
#define MAPDECAL_H
#pragma once


#include "MapHelper.h"


class CHelperInfo;
class CMapFace;
class CRender3D;
class IEditorTexture;


//
// Structure containing a decal face and the solid to which it is attached.
// We keep one of these for every decal face that is created.
//
struct DecalFace_t
{
	CMapFace *pFace;		// Textured face representing the decal.
	CMapSolid *pSolid;		// The solid to which the face is attached.
};


typedef CTypedPtrList<CPtrList, DecalFace_t *> CDecalFaceList;


class CMapDecal : public CMapHelper
{
	public:

		DECLARE_MAPCLASS(CMapDecal)

		//
		// Factory for building from a list of string parameters.
		//
		static CMapClass *CreateMapDecal(CHelperInfo *pInfo, CMapEntity *pParent);

		//
		// Construction/destruction:
		//
		CMapDecal(void);
		CMapDecal(float *pfMins, float *pfMaxs);
		~CMapDecal(void);

		void CalcBounds(BOOL bFullUpdate = FALSE);

		virtual CMapClass *Copy(bool bUpdateDependencies);
		virtual CMapClass *CopyFrom(CMapClass *pFrom, bool bUpdateDependencies);

		virtual void OnNotifyDependent(CMapClass *pObject, Notify_Dependent_t eNotifyType);
		virtual void OnParentKeyChanged(LPCSTR szKey, LPCSTR szValue);
		virtual void OnPaste(CMapClass *pCopy, CMapWorld *pSourceWorld, CMapWorld *pDestWorld, CMapObjectList &OriginalList, CMapObjectList &NewList);
		virtual void OnRemoveFromWorld(CMapWorld *pWorld, bool bNotifyChildren);

		virtual void Render3D(CRender3D *pRender);

		int SerializeRMF(fstream &File, BOOL bRMF);
		int SerializeMAP(fstream &File, BOOL bRMF);

		bool IsVisualElement(void) { return(true); }
		
		LPCTSTR GetDescription() { return("Decal helper"); }

		virtual void PostloadWorld(CMapWorld *pWorld);

		void DecalAllSolids(CMapWorld *pWorld);

		bool ShouldRenderLast(void) { return true; }

	protected:

		//
		// Implements CMapAtom transformation functions.
		//
		void DoTransform(Box3D *t);
		void DoTransMove(const Vector &Delta);
		void DoTransRotate(const Vector *pRefPoint, const QAngle &Angles);
		void DoTransScale(const Vector &RefPoint, const Vector &Scale);
		void DoTransFlip(const Vector &RefPoint);

		void AddSolid(CMapSolid *pSolid);

		int CanDecalSolid(CMapSolid *pSolid, CMapFace **ppFaces);
		int DecalSolid(CMapSolid *pSolid);

		void RebuildDecalFaces(void);

		IEditorTexture *m_pTexture;		// Pointer to the texture this decal uses.
		CMapObjectList m_Solids;	// List of solids to which we are attached.
		CDecalFaceList m_Faces;		// List of decal faces and the solids that they are attached to.
};


#endif // MAPDECAL_H
