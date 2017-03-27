//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef MAPENTITY_H
#define MAPENTITY_H
#pragma once


#include "MapClass.h"
#include "MapFace.h"			// FIXME: For PLANE definition.
#include "EditGameClass.h"


class CMapAnimator;
class CRender2D;


class CMapEntity : public CMapClass, public CEditGameClass
{
	public:

		DECLARE_MAPCLASS(CMapEntity)

		CMapEntity(); 
		~CMapEntity();

		void Debug(void);

		size_t GetSize();

		//
		// For flags field.
		//
		enum
		{
			flagPlaceholder = 0x01,	// No solids - just a point entity
		};

		enum alignType_e
		{
			ALIGN_TOP,
			ALIGN_BOTTOM,
		};

		static inline void ShowEntityNames(bool bShow) { s_bShowEntityNames = bShow; }
		static inline bool GetShowEntityNames(void) { return s_bShowEntityNames; }

		static inline void ShowEntityConnections(bool bShow) { s_bShowEntityConnections = bShow; }
		static inline bool GetShowEntityConnections(void) { return s_bShowEntityConnections; }

		static void GenerateNewTargetname( const char *startName, char *newName, int newNameBufferSize );
		void ReplaceTargetname(const char *szOldName, const char *szNewName);

		inline void SetPlaceholder(BOOL bSet)
		{
			if (bSet)
			{
				flags |= flagPlaceholder;
			}
			else
			{
				flags &= ~flagPlaceholder;
			}
		}

		inline BOOL IsPlaceholder(void)
		{
			return((flags & flagPlaceholder) ? TRUE : FALSE);
		}

		//
		// CMapClass overrides.
		//

		//
		// Serialization.
		//
		ChunkFileResult_t LoadVMF(CChunkFile *pFile);
		ChunkFileResult_t SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo);
		int SerializeRMF(fstream&, BOOL);
		int SerializeMAP(fstream&, BOOL);
		virtual void PostloadWorld(CMapWorld *pWorld);

		//
		// Rendering.
		//
		virtual void Render2D(CRender2D *pRender);

		virtual bool ShouldSnapToHalfGrid();

		virtual void SetOrigin(Vector& o);
		virtual void CalcBounds(BOOL bFullUpdate = FALSE);
		inline void SetClass(GDclass *pClass) { CEditGameClass::SetClass(pClass); } // Works around a namespace issue.
		virtual void SetClass(LPCTSTR pszClassname, bool bLoading = false);
		virtual void SetColorFromVisGroups(void);
		virtual void AlignOnPlane( Vector& pos, PLANE *plane, alignType_e align );

		virtual CMapClass *PrepareSelection(SelectMode_t eSelectMode);

		//
		// Notifications.
		//
		virtual void OnClone(CMapClass *pClone, CMapWorld *pWorld, CMapObjectList &OriginalList, CMapObjectList &NewList);
		virtual void OnPreClone(CMapClass *pClone, CMapWorld *pWorld, CMapObjectList &OriginalList, CMapObjectList &NewList);
		virtual void OnAddToWorld(CMapWorld *pWorld);
		virtual void OnNotifyDependent(CMapClass *pObject, Notify_Dependent_t eNotifyType);

		virtual void UpdateDependencies(CMapWorld *pWorld, CMapClass *pObject);

		//
		// Keyvalue access. We need to know any time one of our keyvalues changes.
		//
		virtual void SetKeyValue(LPCSTR pszKey, LPCSTR pszValue);
		virtual void DeleteKeyValue(LPCTSTR pszKey);

		int GetNodeID(void);
		int SetNodeID(int nNodeID);

		void NotifyChildKeyChanged(CMapClass *pChild, const char *szKey, const char *szValue);

		virtual CMapEntity *FindChildByKeyValue( LPCSTR key, LPCSTR value );

		virtual CMapClass *Copy(bool bUpdateDependencies);
		virtual CMapClass *CopyFrom(CMapClass *pFrom, bool bUpdateDependencies);

		virtual void AddChild(CMapClass *pChild);

		bool HasSolidChildren(void);

		void AssignNodeID(void);

		LPCTSTR GetDescription();
		bool IsScaleable() { return !IsPlaceholder(); }

		// animation
		bool GetTransformMatrix( matrix4_t& matrix );
		BOOL IsAnimationController( void ) { return IsMoveClass(); }

		//-----------------------------------------------------------------------------
		// Purpose: If the first child of this entity is of type MapClass, this function
		//			returns a pointer to that child.
		// Output : Returns a pointer to the MapClass that is a child of this
		//			entity, NULL if the first child of this entity is not MapClass.
		//-----------------------------------------------------------------------------
		template< class MapClass >
		MapClass *GetChildOfType( MapClass *null )
		{
			POSITION pos = Children.GetHeadPosition();
			while ( pos != NULL )
			{
				MapClass *pChild = dynamic_cast<MapClass*>( Children.GetNext(pos) );
				if ( pChild != NULL )
				{
					return pChild;
				}
			}

			return NULL;
		}

		//
		// CMapAtom implementation.
		//
		virtual void GetRenderColor(unsigned char &red, unsigned char &green, unsigned char &blue);
		virtual color32 GetRenderColor(void);

	protected:

		void EnsureUniqueNodeID(CMapWorld *pWorld);
		void OnKeyValueChanged(const char *pszKey, const char *pszOldValue, const char *pszValue);

		//
		// Each CMapEntity may have one or more helpers as its children, depending
		// on its class definition in the FGD file.
		//
		void AddBoundBoxForClass(GDclass *pClass, bool bLoading);
		void AddHelper(CMapClass *pHelper, bool bLoading);
		void AddHelpersForClass(GDclass *pClass, bool bLoading);
		void RemoveHelpers(bool bRemoveSolids);
		void UpdateHelpers(bool bLoading);

		//
		// Chunk and key value handlers for loading.
		//
		static ChunkFileResult_t LoadSolidCallback(CChunkFile *pFile, CMapEntity *pEntity);
		static ChunkFileResult_t LoadHiddenCallback(CChunkFile *pFile, CMapEntity *pEntity);
		static ChunkFileResult_t LoadKeyCallback(const char *szKey, const char *szValue, CMapEntity *pEntity);

		static bool s_bShowEntityNames;			// Whether to render entity names in the 2D views.
		static bool s_bShowEntityConnections;	// Whether to render lines indicating entity connections in the 2D views.

		WORD flags;							// flagPlaceholder
		CMapEntity *m_pMoveParent;			// for entity movement hierarchy
		CMapAnimator *m_pAnimatorChild;
};


typedef CTypedPtrList<CPtrList, CMapEntity *> CSimpleMapEntityList;


class CMapEntityList
{
	public:

		inline POSITION AddTail(CMapEntity *pEntity);

		inline POSITION Find(CMapEntity *pEntity);
		inline POSITION FindIndex(int nIndex);

		inline int Count(void);
		inline CMapEntity *Element(int nIndex);
		inline void AddToTail(CMapEntity *pEntity);

		// FIXME: port to CUtlVector and remove
		inline int GetCount(void);
		inline CMapEntity* GetAt(POSITION &pos);
		inline POSITION GetHeadPosition(void);
		inline CMapEntity *GetNext(POSITION &pos);

		bool HasInputOfType(InputOutputType_t eType);
		bool HasInput(const char *szInput);

		inline bool Remove(CMapEntity *pEntity);
		inline void RemoveAll(void);

	protected:

		CSimpleMapEntityList m_List;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMapEntityList::Count(void)
{
	return m_List.GetCount();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapEntity *CMapEntityList::Element(int nIndex)
{
	// FIXME: this is just for porting to CUtlVector! Remove!!
	int nCurrent = 0;

	POSITION pos = m_List.GetHeadPosition();
	while (pos)
	{
		CMapEntity *pEntity = m_List.GetNext(pos);
		if (nCurrent == nIndex)
		{
			return pEntity;
		}

		nCurrent++;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Adds an entity to the list.
// Input  : pEntity - Entity to add.
// Output : Returns the position of the entity in the list.
//-----------------------------------------------------------------------------
void CMapEntityList::AddToTail(CMapEntity *pEntity)
{
	m_List.AddTail(pEntity);
}


//-----------------------------------------------------------------------------
// Purpose: Adds an entity to the list.
// Input  : pEntity - Entity to add.
// Output : Returns the position of the entity in the list.
//-----------------------------------------------------------------------------
POSITION CMapEntityList::AddTail(CMapEntity *pEntity)
{
	return(m_List.AddTail(pEntity));
}


//-----------------------------------------------------------------------------
// Purpose: Returns the position of a given entity in the list, NULL if not found.
// Input  : pEntity - Entity to find.
//-----------------------------------------------------------------------------
POSITION CMapEntityList::Find(CMapEntity *pEntity)
{
	return(m_List.Find(pEntity));
}


//-----------------------------------------------------------------------------
// Purpose: Returns the number of entities in the list.
//-----------------------------------------------------------------------------
int CMapEntityList::GetCount(void)
{
	return(m_List.GetCount());
}

//-----------------------------------------------------------------------------
// Purpose: Returns the number of entities in the list.
//-----------------------------------------------------------------------------
CMapEntity *CMapEntityList::GetAt(POSITION &pos)
{
	return(m_List.GetAt(pos));
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position for the given index
//-----------------------------------------------------------------------------
POSITION CMapEntityList::FindIndex(int nIndex)
{
	return(m_List.FindIndex(nIndex));
}

//-----------------------------------------------------------------------------
// Purpose: Returns an iterator set to the head of this list.
//-----------------------------------------------------------------------------
POSITION CMapEntityList::GetHeadPosition(void)
{
	return(m_List.GetHeadPosition());
}


//-----------------------------------------------------------------------------
// Purpose: Returns the next entity in the list, based on the iterator 'pos'.
// Input  : pos - Iterator.
//-----------------------------------------------------------------------------
CMapEntity *CMapEntityList::GetNext(POSITION &pos)
{
	return(m_List.GetNext(pos));
}


//-----------------------------------------------------------------------------
// Purpose: Removes a specific entity from the list.
// Input  : pEntity - Entity to remove.
// Output : Returns true on success, false if the entity was not found in the list.
//-----------------------------------------------------------------------------
bool CMapEntityList::Remove(CMapEntity *pEntity)
{
	POSITION pos = m_List.Find(pEntity);
	if (pos != NULL)
	{
		m_List.RemoveAt(pos);
		return(true);
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Empties the list. Does not delete the entities themselves.
//-----------------------------------------------------------------------------
void CMapEntityList::RemoveAll(void)
{
	m_List.RemoveAll();
}


#endif // MAPENTITY_H
