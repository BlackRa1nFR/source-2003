//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPWORLD_H
#define MAPWORLD_H
#pragma once


#include "MapEntity.h"
#include "EditGameClass.h"
#include "MapClass.h"
#include "MapPath.h"

// Flags for SaveVMF.
#define SAVEFLAGS_LIGHTSONLY	(1<<0)


#define MAX_VISIBLE_OBJECTS		10000


class BoundBox;
class CChunkFile;
class CVisGroup;
class CCullTreeNode;
class IEditorTexture;

struct SaveLists_t;


class CMapWorld : public CMapClass, public CEditGameClass
{
	public:

		DECLARE_MAPCLASS(CMapWorld)

		CMapWorld(void);
		~CMapWorld(void);

		//
		// Public interface to the culling tree.
		//
		void CullTree_Build(void);
		inline CCullTreeNode *CullTree_GetCullTree(void) { return(m_pCullTree); }

		//
		// CMapClass virtual overrides.
		//
		virtual void AddChild(CMapClass *pChild);
		virtual CMapClass *Copy(bool bUpdateDependencies);
		virtual CMapClass *CopyFrom(CMapClass *pFrom, bool bUpdateDependencies);
		virtual bool IsWorldObject(void) { return(true); }
		virtual void PresaveWorld(void);
		virtual void RemoveChild(CMapClass *pChild, bool bUpdateBounds = true);

		void AddObjectToWorld(CMapClass *pObject, CMapClass *pParent = NULL);
		void RemoveObjectFromWorld(CMapClass *pObject, bool bRemoveChildren);

		//
		// Serialization.
		//
		ChunkFileResult_t LoadVMF(CChunkFile *pFile);
		void PostloadWorld(void);
		
		// saveFlags is a combination of SAVEFLAGS_ defines.
		ChunkFileResult_t SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo, int saveFlags);
		
		virtual int SerializeRMF(fstream &file, BOOL fIsStoring);
		virtual int SerializeMAP(fstream &file, BOOL fIsStoring, BoundBox *pIntersecting = NULL);

		virtual void UpdateChild(CMapClass *pChild);

		//
		// Face ID management.
		//
		inline int FaceID_GetNext(void);
		inline void FaceID_SetNext(int nNextFaceID);
		CMapFace *FaceID_FaceForID(int nFaceID);
		void FaceID_StringToFaceIDLists(CMapFaceIDList *pFullFaceList, CMapFaceIDList *pPartialFaceList, const char *pszValue);
		void FaceID_StringToFaceLists(CMapFaceList *pFullFaceList, CMapFaceList *pPartialFaceList, const char *pszValue);
		static bool FaceID_FaceIDListsToString(char *pszList, int nSize, CMapFaceIDList *pFullFaceIDList, CMapFaceIDList *pPartialFaceIDList);
		static bool FaceID_FaceListsToString(char *pszValue, int nSize, CMapFaceList *pFullFaceList, CMapFaceList *pPartialFaceList);

		void GetUsedTextures(ITextureList *pList);
		void Subtract(CMapObjectList &Results, CMapClass *pSubtractWith, CMapClass *pSubtractFrom);

		CMapPathList m_Paths;

		//
		// Interface to list of all the entities in the world:
		//
		inline CMapEntityList *EntityList_GetList(void) { return(&m_EntityList); } // dvs: fix somehow
		CMapEntity *EntityList_GetFirst(POSITION &pos);
		inline CMapEntity *EntityList_GetNext(POSITION &pos);
		inline int EntityList_GetCount(void);

		void GetGroupList(CMapObjectList &GroupList);

		// displacement management
		inline IWorldEditDispMgr *GetWorldEditDispManager( void ) { return m_pWorldDispMgr; }

	protected:

		//
		// Protected entity list functions.
		//
		void EntityList_Add(CMapClass *pObject);
		void EntityList_Remove(CMapClass *pObject, bool bRemoveChildren);

		//
		// Serialization.
		//
		static ChunkFileResult_t LoadKeyCallback(const char *szKey, const char *szValue, CMapWorld *pWorld);
		static ChunkFileResult_t LoadGroupCallback(CChunkFile *pFile, CMapWorld *pWorld);
		static ChunkFileResult_t LoadHiddenCallback(CChunkFile *pFile, CMapWorld *pWorld);
		static ChunkFileResult_t LoadHiddenSolidCallback(CChunkFile *pFile, CMapWorld *pWorld);
		static ChunkFileResult_t LoadSolidCallback(CChunkFile *pFile, CMapWorld *pWorld);

		ChunkFileResult_t LoadSolid(CChunkFile *pFile, bool bVisible);

		static BOOL BuildSaveListsCallback(CMapClass *pObject, SaveLists_t *pSaveLists);
		ChunkFileResult_t SaveObjectListVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo, CMapObjectList *pList, int saveFlags);

		//
		// Culling tree operations.
		//
		void CullTree_SplitNode(CCullTreeNode *pNode);
		void CullTree_DumpNode(CCullTreeNode *pNode, int nDepth);
		void CullTree_FreeNode(CCullTreeNode *pNode);
		void CullTree_Free(void);

		CCullTreeNode *m_pCullTree;		// This world's objects stored in a spatial hierarchy for culling.
		CMapEntityList m_EntityList;	// A list of all the entities in this world.
		int m_nNextFaceID;				// Used for assigning unique IDs to every solid face in this world.

		IWorldEditDispMgr	*m_pWorldDispMgr;	// world editable displacement manager
};


//-----------------------------------------------------------------------------
// Purpose: Returns the number of entities in this world.
//-----------------------------------------------------------------------------
int CMapWorld::EntityList_GetCount(void)
{
	return(m_EntityList.GetCount());
}


//-----------------------------------------------------------------------------
// Purpose: Iterates this world's entity list.
//-----------------------------------------------------------------------------
CMapEntity *CMapWorld::EntityList_GetNext(POSITION &pos)
{
	if (pos == NULL)
	{
		return(NULL);
	}
	
	return(m_EntityList.GetNext(pos));
}


//-----------------------------------------------------------------------------
// Purpose: Returns the next unique face ID for this world.
//-----------------------------------------------------------------------------
inline int CMapWorld::FaceID_GetNext(void)
{
	return(m_nNextFaceID++);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the unique face ID to assign to the next face that is added
//			to this world.
//-----------------------------------------------------------------------------
inline void CMapWorld::FaceID_SetNext(int nNextFaceID)
{
	m_nNextFaceID = nNextFaceID;
}


#endif // MAPWORLD_H
