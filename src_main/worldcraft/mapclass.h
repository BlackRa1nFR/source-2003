//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines a common class for all objects in the world object tree.
//
//			Pointers to other objects in the object tree may be stored, but
//			they should be set via UpdateDependency rather than directly. This
//			insures proper linkage to the other object so that if it moves, is
//			removed from the world, or changes in any other way, the dependent
//			object is properly notified.
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPCLASS_H
#define MAPCLASS_H
#pragma once


#include <fstream.h>
#include <afxtempl.h>
#include <afxcoll.h>
#include "BoundBox.h"
#include "MapPoint.h"


class Box3D;
class CBaseTool;
class CChunkFile;
class GDclass;
class KeyValues;
class CMapClass;
class CMapEntity;
class CMapSolid;
class CMapView2D;
class CMapWorld;
class CPoint;
class CRender3D;
class CSaveInfo;
class CSSolid;
class CVisGroup;


enum ChunkFileResult_t;


struct MapObjectPair_t
{
	CMapClass *pObject1;
	CMapClass *pObject2;
};


//
// Passed into PrepareSelection to control what gets selected.
//
enum SelectMode_t
{
	selectGroups = 0,	// select groups, ungrouped entities, and ungrouped solids
	selectObjects,		// select entities and solids not in entities
	selectSolids,		// select point entities, solids in entities, solids
};


typedef const char * MAPCLASSTYPE;
typedef BOOL (*ENUMMAPCHILDRENPROC)(CMapClass *, DWORD dwParam);
typedef CTypedPtrList<CPtrList, CMapClass *> CMapObjectList;
typedef CTypedPtrList<CPtrList, MapObjectPair_t *> CMapObjectPairList;


#define MAX_ENUM_CHILD_DEPTH	16


struct EnumChildrenStackEntry_t
{
	CMapClass *pParent;
	POSITION pos;
};


struct EnumChildrenPos_t
{
	EnumChildrenStackEntry_t Stack[MAX_ENUM_CHILD_DEPTH];
	int nDepth;
};


typedef struct
{
	MAPCLASSTYPE Type;
	CMapClass * (*pfnNew)();
} MCMSTRUCT;


class CMapClass : public CMapPoint
{
	public:
		//
		// Construction/destruction:
		//
		CMapClass(void);
		virtual ~CMapClass(void);

		inline int GetID(void);
		inline void SetID(int nID);
		virtual size_t GetSize(void);

		//
		// Can belong to visgroups:
		//
		CVisGroup *GetVisGroup(void);
		void SetVisGroup(CVisGroup *pVisGroup);
		virtual void SetColorFromVisGroups(void);

		//
		// Can be tracked in the Undo/Redo system:
		//
		inline void SetTemporary(bool bTemporary) { m_bTemporary = bTemporary; }
		inline bool IsTemporary(void) const { return m_bTemporary; }
		union
		{
			struct
			{
				unsigned ID : 28;
				unsigned Types : 4;	// 0 - copy, 1 - relationship, 2 - delete
			} Kept;

			DWORD dwKept;
		};

		//
		// Has children:
		//
		virtual void AddChild(CMapClass *pChild);
		virtual void CopyChildrenFrom(CMapClass *pobj, bool bUpdateDependencies);
		virtual void RemoveAllChildren(void);
		virtual void RemoveChild(CMapClass *pChild, bool bUpdateBounds = true);
		virtual void UpdateChild(CMapClass *pChild);

		int GetChildCount(void) { return(Children.GetCount()); }
		CMapClass *GetFirstChild(POSITION &pos);
		CMapClass *GetNextChild(POSITION &pos);

		CMapClass *GetFirstDescendent(EnumChildrenPos_t &pos);
		CMapClass *GetNextDescendent(EnumChildrenPos_t &pos);

		virtual CMapClass *GetParent(void) { return(Parent); }
		virtual void SetObjParent(CMapClass *pParent); // dvs: HACK to distinguish from CMapAtom::SetParent. Need to merge the two!

		virtual void ReplaceTargetname(const char *szOldName, const char *szNewName);

		//
		// Notifications.
		//
		virtual void OnAddToWorld(CMapWorld *pWorld);
		virtual void OnClone(CMapClass *pClone, CMapWorld *pWorld, CMapObjectList &OriginalList, CMapObjectList &NewList);
		virtual void OnPreClone(CMapClass *pClone, CMapWorld *pWorld, CMapObjectList &OriginalList, CMapObjectList &NewList);
		virtual void OnPrePaste(CMapClass *pCopy, CMapWorld *pSourceWorld, CMapWorld *pDestWorld, CMapObjectList &OriginalList, CMapObjectList &NewList);
		virtual void OnPaste(CMapClass *pCopy, CMapWorld *pSourceWorld, CMapWorld *pDestWorld, CMapObjectList &OriginalList, CMapObjectList &NewList);
		virtual void OnNotifyDependent(CMapClass *pObject, Notify_Dependent_t eNotifyType);
		virtual void OnParentKeyChanged(LPCSTR key, LPCSTR value) {}
		virtual void OnRemoveFromWorld(CMapWorld *pWorld, bool bNotifyChildren);
		virtual void OnUndoRedo(void) {}
				
		//
		// Bounds calculation and intersection functions.
		//
		virtual void CalcBounds(BOOL bFullUpdate = FALSE);
		void GetCullBox(Vector &mins, Vector &maxs);
		void GetRender2DBox(Vector &mins, Vector &maxs);

		// HACK: temp stuff to ease the transition to not inheriting from BoundBox
		void GetBoundsCenter(Vector &vecCenter) { m_Render2DBox.GetBoundsCenter(vecCenter); }
		void GetBoundsSize(Vector &vecSize) { m_Render2DBox.GetBoundsSize(vecSize); }
		inline bool IsInsideBox(Vector const &Mins, Vector const &Maxs) const { return(m_Render2DBox.IsInsideBox(Mins, Maxs)); }
		inline bool IsIntersectingBox(const Vector &vecMins, const Vector& vecMaxs) const { return(m_Render2DBox.IsIntersectingBox(vecMins, vecMaxs)); }

		virtual CMapClass *PrepareSelection(SelectMode_t eSelectMode);

		void PostUpdate(Notify_Dependent_t eNotifyType);
		static void UpdateAllDependencies(CMapClass *pObject);

		void SetOrigin(Vector& origin);

		// hierarchy
		virtual void UpdateAnimation( float animTime ) {}
		virtual bool GetTransformMatrix( matrix4_t& matrix );
		
		virtual MAPCLASSTYPE GetType(void) = 0;
		virtual BOOL IsMapClass(MAPCLASSTYPE Type) = 0;
		virtual CMapClass *Copy(bool bUpdateDependencies);
		virtual CMapClass *CopyFrom(CMapClass *pFrom, bool bUpdateDependencies);

		virtual CMapClass *HitTest2D(CMapView2D *pView, const Vector2D &point, int &nHitData);

		// Objects that can be clicked on and activated as tools implement this and return a CBaseTool-derived object.
		virtual CBaseTool *GetToolObject(int nHitData) { return NULL; }

		//
		// Can be serialized:
		//
		virtual ChunkFileResult_t SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo);
		virtual ChunkFileResult_t SaveEditorData(CChunkFile *pFile);

		virtual bool ShouldSerialize(void) { return(m_bShouldSerialize); }
		virtual void SetShouldSerialize(bool bShouldSerialize) { m_bShouldSerialize = bShouldSerialize; }
		virtual int SerializeRMF(fstream &File, BOOL bRMF);
		virtual int SerializeMAP(fstream &File, BOOL bRMF);
		virtual void PostloadWorld(CMapWorld *pWorld);
		virtual void PresaveWorld(void) {}

		virtual bool IsWorldObject(void) { return false; }
		virtual bool IsGroup(void) { return false; }
		virtual bool IsScaleable(void) { return false; }
		virtual bool IsClutter(void) { return false; }			// Whether this object should be hidden when the user hides helpers.
		virtual bool IsCulledByCordon(void) { return true; }	// Whether this object is hidden based on its own intersection with the cordon, independent of its parent's intersection.

		virtual bool ShouldSnapToHalfGrid() { return false; }

		// searching
		virtual CMapEntity *FindChildByKeyValue( LPCSTR key, LPCSTR value );

		// HACK: get the world that this object is contained within.
		static CMapWorld *GetWorldObject(CMapClass *pStart);

		virtual LPCTSTR GetDescription() { return ""; }

		BOOL EnumChildren(ENUMMAPCHILDRENPROC pfn, DWORD dwParam = 0, MAPCLASSTYPE Type = NULL);
		BOOL EnumChildrenRecurseGroupsOnly(ENUMMAPCHILDRENPROC pfn, DWORD dwParam, MAPCLASSTYPE Type = NULL);
		BOOL IsChildOf(CMapClass *pObject);

		inline bool IsVisible(void) { return(m_bVisible); }
		virtual void SetVisible(bool bVisible);

		//
		// Visible2D functions are used only for hiding solids being morphed. Remove?
		//
		bool IsVisible2D() { return m_bVisible2D; }
		void SetVisible2D(bool bVisible2D) { m_bVisible2D = bVisible2D; }

		//
		// Overridden to set the render color of each of our children.
		//
		virtual void SetRenderColor(unsigned char red, unsigned char green, unsigned char blue);
		virtual void SetRenderColor(color32 rgbColor);

		//
		// Can be rendered:
		//
		virtual void Render2D(CRender2D *pRender);
		virtual void Render3D(CRender3D *pRender);
		virtual bool RenderPreload(CRender3D *pRender, bool bNewContext);
		inline int GetRenderFrame(void) { return(m_nRenderFrame); }
		inline void SetRenderFrame(int nRenderFrame) { m_nRenderFrame = nRenderFrame; }

		SelectionState_t SetSelectionState(SelectionState_t eSelectionState);

		//
		// Has a set of editor-specific properties that are loaded from the MAP file.
		//
		const char *GetEditorKeyValue(const char *szKey);
		void RemoveEditorKeys(void);
		void SetEditorKeyValue(const char *szKey, const char *szValue);

	protected:

		//
		// Implements CMapAtom transformation interface:
		//
		virtual void DoTransform(Box3D *t);
		virtual void DoTransMove(const Vector &Delta);
		virtual void DoTransRotate(const Vector *pRefPoint, const QAngle &Angles);
		virtual void DoTransScale(const Vector &RefPoint, const Vector &Scale);
		virtual void DoTransFlip(const Vector &RefPoint);

		//
		// Serialization callbacks.
		//
		static ChunkFileResult_t LoadEditorCallback(CChunkFile *pFile, CMapClass *pObject);
		static ChunkFileResult_t LoadEditorKeyCallback(const char *szKey, const char *szValue, CMapClass *pObject);

		//
		// Has a list of objects that must be notified if it changes size or position.
		//
		void AddDependent(CMapClass *pDependent);
		void NotifyDependents(Notify_Dependent_t eNotifyType);
		void RemoveDependent(CMapClass *pDependent);
		virtual void UpdateDependencies(CMapWorld *pWorld, CMapClass *pObject) {};
		CMapClass *UpdateDependency(CMapClass *pOldAttached, CMapClass *pNewAttached);

		void UpdateParent(CMapClass *pNewParent);

		BoundBox m_CullBox;				// Our bounds for culling in the 3D views and intersecting with the cordon.
		BoundBox m_Render2DBox;			// Our bounds for rendering in the 2D views.

		CMapClass *Parent;				// Each object in the map has one parent.
		CMapObjectList Children;		// Each object can have many children. Children usually transform with their parents, etc.

		CMapObjectList m_Dependents;	// Objects that this object should notify if it changes.

		int m_nID;						// This object's unique ID.
		bool m_bShouldSerialize;		// Whether this object should be written to disk.
		bool m_bTemporary;				// Whether to track this object for Undo/Redo.
		bool m_bVisible2D;				// Whether this object is visible in the 2D view. Currently only used for morphing.
		int m_nRenderFrame;				// Frame counter used to avoid rendering the same object twice in a 3D frame.
		bool m_bVisible;				// Whether this object is currently visible in the 2D and 3D views.
		CVisGroup *m_pVisGroup;			// VisGroup to which this object belongs, NULL if none.
		KeyValues *m_pEditorKeys;		// Temporary storage for keys loaded from the "editor" chunk of the VMF file, freed after loading.

	friend class CTrackEntry;			// Friends with Undo/Redo system so that parentage can be changed.
};


//-----------------------------------------------------------------------------
// Purpose: Returns this object's unique ID.
//-----------------------------------------------------------------------------
int CMapClass::GetID(void)
{
	return(m_nID);
}


//-----------------------------------------------------------------------------
// Purpose: Sets this object's unique ID.
//-----------------------------------------------------------------------------
void CMapClass::SetID(int nID)
{
	m_nID = nID;
}


class CMapClassManager
{
	public:

		virtual ~CMapClassManager();
		CMapClassManager(MAPCLASSTYPE Type, CMapClass * (*pfnNew)());

		static CMapClass * CreateObject(MAPCLASSTYPE Type);
};


#define MAPCLASS_TYPE(class_name) \
	(class_name::__Type)


#define IMPLEMENT_MAPCLASS(class_name) \
	char * class_name::__Type = #class_name; \
	MAPCLASSTYPE class_name::GetType() { return __Type; }	\
	BOOL class_name::IsMapClass(MAPCLASSTYPE Type) \
		{ return (Type == __Type) ? TRUE : FALSE; } \
	CMapClass * class_name##_CreateObject() \
		{ return new class_name; } \
	CMapClassManager mcm_##class_name(class_name::__Type, \
		class_name##_CreateObject);


#define DECLARE_MAPCLASS(class_name) \
	static char * __Type; \
	virtual MAPCLASSTYPE GetType(); \
	virtual BOOL IsMapClass(MAPCLASSTYPE Type);


class CCheckFaceInfo
{
	public:

		CCheckFaceInfo() { iPoint = -1; }
		char szDescription[128];
		int iPoint;
};


#endif // MAPCLASS_H
