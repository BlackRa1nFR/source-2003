//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Implements a helper that manages a single keyvalue of type "side"
//			or "sidelist" for the entity that is its parent.
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPSIDELIST_H
#define MAPSIDELIST_H
#ifdef _WIN32
#pragma once
#endif


#include "MapFace.h"
#include "MapHelper.h"


class CHelperInfo;
class CMapEntity;
class CMapSolid;


class CMapSideList : public CMapHelper
{
	public:

		//
		// Factory function.
		//
		static CMapClass *CreateMapSideList(CHelperInfo *pHelperInfo, CMapEntity *pParent);

		//
		// Construction/destruction:
		//
		CMapSideList(void);
		CMapSideList(char const *pszKeyName);
		virtual ~CMapSideList(void);
		DECLARE_MAPCLASS(CMapSprite)

	public:

		virtual size_t GetSize(void);

		//
		// Replication.
		//
		virtual CMapClass *Copy(bool bUpdateDependencies);
		virtual CMapClass *CopyFrom(CMapClass *pOther, bool bUpdateDependencies);

		//
		// Should NOT have children.
		//
		virtual void AddChild(CMapClass *pChild) { ASSERT(false); } 
		virtual void UpdateChild(CMapClass *pChild) { ASSERT(false); }

		//
		// Notifications.
		//
		virtual void OnClone(CMapClass *pClone, CMapWorld *pWorld, CMapObjectList &OriginalList, CMapObjectList &NewList);
		virtual void OnNotifyDependent(CMapClass *pObject, Notify_Dependent_t eNotifyType);
		virtual void OnParentKeyChanged(LPCSTR key, LPCSTR value);
		virtual void OnPaste(CMapClass *pCopy, CMapWorld *pSourceWorld, CMapWorld *pDestWorld, CMapObjectList &OriginalList, CMapObjectList &NewList);
		virtual void OnRemoveFromWorld(CMapWorld *pWorld, bool bNotifyChildren);

		//
		// Spatial functions.
		//
		virtual void CalcBounds(BOOL bFullUpdate = FALSE);

		virtual LPCTSTR GetDescription(void) { return "Side list helper"; }

		//
		// Serialization.
		//
		virtual void PostloadWorld(CMapWorld *pWorld);
		virtual bool ShouldSerialize(void) { return(false); }

		//
		// Can be rendered:
		//
		virtual void Render2D(CRender2D *pRender);
		virtual void Render3D(CRender3D *pRender);

	protected:

		virtual void UpdateDependencies(CMapWorld *pWorld, CMapClass *pObject);

		void BuildFaceListForValue(char const *pszValue, CMapWorld *pWorld);
		CMapFace *FindFaceIDInList(int nFaceID, CMapObjectList &List);
		void ReplaceFacesInCopy(CMapSideList *pCopy, CMapObjectList &OriginalList, CMapObjectList &NewList);
		bool ReplaceSolidFaces(CMapSolid *pOrigSolid, CMapSolid *pNewSolid);
		void RemoveFacesNotInList(CMapObjectList &List);
		void UpdateParentKey(void);

		char m_szKeyName[80];			// The name of the key in our parent entity that we represent.
		CMapFaceList m_Faces;			// The list of solid faces that this object manages.
		CUtlVector<int> m_LostFaceIDs;	// The list of face IDs that were in m_Faces, but were lost during this session through the deletion of their solid.
};


#endif // MAPSIDELIST_H
