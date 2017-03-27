//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "CullTreeNode.h"
#include "GlobalFunctions.h"
#include "MainFrm.h"
#include "MapDefs.h"
#include "MapDoc.h"		// dvs: think of a way around the world knowing about the doc
#include "MapEntity.h"
#include "MapGroup.h"
#include "MapSolid.h"
#include "MapWorld.h"
#include "SaveInfo.h"
#include "StatusBarIDs.h"
#include "VisGroup.h"
#include "Worldcraft.h"
#include "Worldsize.h"


#pragma warning(disable:4244)


class CCullTreeNode;


IMPLEMENT_MAPCLASS(CMapWorld)


struct SaveLists_t
{
	CMapObjectList Solids;
	CMapObjectList Entities;
	CMapObjectList Groups;
};


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSolid - 
//			*pList - 
// Output : static BOOL
//-----------------------------------------------------------------------------
static BOOL AddUsedTextures(CMapSolid *pSolid, ITextureList *pList)
{
	int nFaces = pSolid->GetFaceCount();
	IEditorTexture *pLastTex = NULL;
	for (int i = 0; i < nFaces; i++)
	{
		CMapFace *pFace = pSolid->GetFace(i);
		IEditorTexture *pTex = pFace->GetTexture();

		if ((pTex != NULL) && (pTex != pLastTex))
		{
			if (!pList->Find(pTex))
			{
				pList->AddTail(pTex);
			}
			pLastTex = pTex;
		}
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether the two boxes intersect.
// Input  : mins1 - 
//			maxs1 - 
//			mins2 - 
//			maxs2 - 
//-----------------------------------------------------------------------------
bool BoxesIntersect(Vector const &mins1, Vector const &maxs1, Vector const &mins2, Vector const &maxs2)
{
	if ((maxs1[0] < mins2[0]) || (mins1[0] > maxs2[0]) ||
		(maxs1[1] < mins2[1]) || (mins1[1] > maxs2[1]) ||
		(maxs1[2] < mins2[2]) || (mins1[2] > maxs2[2]))
	{
		return(false);
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor. Initializes data members.
//-----------------------------------------------------------------------------
CMapWorld::CMapWorld(void)
{
	//
	// Make sure subsequent UpdateBounds() will be effective.
	//
	m_Render2DBox.ResetBounds();
	Vector pt( 0, 0, 0 );
	m_Render2DBox.UpdateBounds(pt);

	SetClass("worldspawn");
	m_pCullTree = NULL;

	m_nNextFaceID = 1;			// Face IDs start at 1. An ID of 0 means no ID.

	// create the world displacement manager
	m_pWorldDispMgr = CreateWorldEditDispMgr();
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Deletes all paths in the world and the culling tree.
//-----------------------------------------------------------------------------
CMapWorld::~CMapWorld(void)	
{
	//
	// Delete paths.
	//
	POSITION pos = m_Paths.GetHeadPosition();
	while (pos != NULL)
	{
		CMapPath *pPath = m_Paths.GetNext(pos);
		delete pPath;
	}

	//
	// Delete the culling tree.
	//
	CullTree_Free();

	// destroy the world displacement manager
	DestroyWorldEditDispMgr( &m_pWorldDispMgr );
}


//-----------------------------------------------------------------------------
// Purpose: Overridden to maintain the culling tree. Root level children of the
//			world are kept in the culling tree.
// Input  : pChild - object to add as a child.
//-----------------------------------------------------------------------------
void CMapWorld::AddChild(CMapClass *pChild)
{
	CMapClass::AddChild(pChild);

	//
	// Add the object to the culling tree.
	//
	if (m_pCullTree != NULL)
	{
		m_pCullTree->AddCullTreeObjectRecurse(pChild);
	}
}


//-----------------------------------------------------------------------------
// Purpose: The sole way to add an object to the world. 
//
//			NOTE: Do not call this during file load!! Similar (but different)
//				  bookkeeping is done in PostloadWorld during serialization.
//
// Input  : pObject - object to add to the world.
//			pParent - object to use as the new object's parent.
//-----------------------------------------------------------------------------
void CMapWorld::AddObjectToWorld(CMapClass *pObject, CMapClass *pParent)
{
	ASSERT(pObject != NULL);
	if (pObject == NULL)
	{
		return;
	}

	//
	// Link the object into the tree.
	//
	if (pParent == NULL)
	{
		pParent = this;
	}

	pParent->AddChild(pObject);

	//
	// If this object or any of its children are entities, add the entities
	// to our optimized list of entities.
	//
	EntityList_Add(pObject);

	//
	// Notify the object that it has been added to the world.
	//
	pObject->OnAddToWorld(this);
}


//-----------------------------------------------------------------------------
// Purpose: Sorts all the objects in the world into three lists: entities, solids,
//			and groups. These lists are then serialized in SaveVMF.
// Input  : pSaveLists - Receives lists of objects.
//-----------------------------------------------------------------------------
BOOL CMapWorld::BuildSaveListsCallback(CMapClass *pObject, SaveLists_t *pSaveLists)
{
	CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pObject);
	if (pEntity != NULL)
	{
		pSaveLists->Entities.AddTail(pEntity);
		return(TRUE);
	}

	CMapSolid *pSolid = dynamic_cast<CMapSolid *>(pObject);
	if (pSolid != NULL)
	{
		pSaveLists->Solids.AddTail(pSolid);
		return(TRUE);
	}

	CMapGroup *pGroup = dynamic_cast<CMapGroup *>(pObject);
	if (pGroup != NULL)
	{
		pSaveLists->Groups.AddTail(pGroup);
		return(TRUE);
	}

	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapWorld::Copy(bool bUpdateDependencies)
{
	CMapWorld *pWorld = new CMapWorld;
	pWorld->CopyFrom(this, bUpdateDependencies);
	return pWorld;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pobj - 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapWorld::CopyFrom(CMapClass *pobj, bool bUpdateDependencies)
{
	return this;
}


//-----------------------------------------------------------------------------
// Purpose: Adds any entities found in the given object tree to the list of
//			entities that are in this world. Called whenever an object is added
//			to this world.
// Input  : pObject - object (and children) to add to the entity list.
//-----------------------------------------------------------------------------
void CMapWorld::EntityList_Add(CMapClass *pObject)
{
	CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pObject);
	if ((pEntity != NULL) && (m_EntityList.Find(pEntity) == NULL))
	{
		m_EntityList.AddTail(pEntity);
	}

	EnumChildrenPos_t pos;	
	CMapClass *pChild = pObject->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pChild);
		if ((pEntity != NULL) && (m_EntityList.Find(pEntity) == NULL))
		{
			m_EntityList.AddTail(pEntity);
		}

		pChild = pObject->GetNextDescendent(pos);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the first entity in this world's entity list.
//-----------------------------------------------------------------------------
CMapEntity *CMapWorld::EntityList_GetFirst(POSITION &pos)
{
	pos = m_EntityList.GetHeadPosition(); 
	if (pos != NULL)
	{
		return(m_EntityList.GetNext(pos));
	}
	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Removes this object (if it is an entity) or any of its entity
//			descendents from this world's entity list. Called when an object is
//			removed from this world.
// Input  : pObject - Object to remove from the entity list.
//-----------------------------------------------------------------------------
void CMapWorld::EntityList_Remove(CMapClass *pObject, bool bRemoveChildren)
{
	//
	// Remove the object itself.
	//
	CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pObject);
	if (pEntity != NULL)
	{
		m_EntityList.Remove(pEntity);
	}
	
	//
	// Remove entity children.
	//
	if (bRemoveChildren)
	{
		EnumChildrenPos_t pos;
		CMapClass *pChild = pObject->GetFirstDescendent(pos);
		while (pChild != NULL)
		{
			CMapEntity *pEntity = dynamic_cast<CMapEntity *>(pChild);
			if (pEntity != NULL)
			{
				m_EntityList.Remove(pEntity);
			}
			pChild = pObject->GetNextDescendent(pos);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Overridden to maintain the culling tree. Root level children of the
//			world are kept in the culling tree.
// Input  : pChild - child to remove.
//-----------------------------------------------------------------------------
void CMapWorld::RemoveChild(CMapClass *pChild, bool bUpdateBounds)
{
	CMapClass::RemoveChild(pChild, bUpdateBounds);

	//
	// Remove the object from the culling tree because it is no longer a root-level child.
	//
	if (m_pCullTree != NULL)
	{
		m_pCullTree->RemoveCullTreeObjectRecurse(pChild);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Removes an object from the world.
// Input  : pObject - object to remove from the world.
//			bChildren - whether we're removing the object's children as well.
//-----------------------------------------------------------------------------
void CMapWorld::RemoveObjectFromWorld(CMapClass *pObject, bool bRemoveChildren)
{
	ASSERT(pObject != NULL);
	if (pObject == NULL)
	{
		return;
	}

	//
	// Unlink the object from the tree.
	//
	CMapClass *pParent = pObject->GetParent();
	ASSERT(pParent != NULL);
	if (pParent != NULL)
	{
		pParent->RemoveChild(pObject);
	}

	//
	// Notify the object so it can release any pointers it may have to other
	// objects in the world. We don't do this in RemoveChild because the object
	// may only be changing parents rather than leaving the world.
	//
	pObject->OnRemoveFromWorld(this, bRemoveChildren);

	//
	// If it (or any of its children) is an entity, remove it from this
	// world's list of entities.
	//
	EntityList_Remove(pObject, bRemoveChildren);
}


//-----------------------------------------------------------------------------
// Purpose: Special implementation of UpdateChild for the world object. This
//			notifies the document that an object's bounding box has changed.
// Input  : pChild - 
//-----------------------------------------------------------------------------
void CMapWorld::UpdateChild(CMapClass *pChild)
{
	//
	// Update the child in the culling tree.
	//
	if (m_pCullTree != NULL)
	{
		m_pCullTree->UpdateCullTreeObjectRecurse(pChild);
	}

	//
	// Notify the document that an object in the world has changed.
	//
	if (!IsTemporary()) // HACK: check to avoid prefab objects ending up in the doc's update list
	{
		CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
		if (pDoc != NULL)
		{
			pDoc->UpdateObject(pChild);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pList - 
//-----------------------------------------------------------------------------
void CMapWorld::GetUsedTextures(ITextureList *pList)
{
	pList->RemoveAll();
	EnumChildren(ENUMMAPCHILDRENPROC(AddUsedTextures), DWORD(pList), MAPCLASS_TYPE(CMapSolid));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pNode - 
//-----------------------------------------------------------------------------
void CMapWorld::CullTree_FreeNode(CCullTreeNode *pNode)
{
	ASSERT(pNode != NULL);

	int nChildCount = pNode->GetChildCount();
	if (nChildCount != 0)
	{
		for (int nChild = 0; nChild < nChildCount; nChild++)
		{
			CCullTreeNode *pChild = pNode->GetCullTreeChild(nChild);
			CullTree_FreeNode(pChild);
		}
	}

	delete pNode;
}


//-----------------------------------------------------------------------------
// Purpose: Recursively deletes the entire culling tree if is it not NULL.
//			This does not delete the map objects that the culling tree contains,
//			only the leaves and nodes themselves.
//-----------------------------------------------------------------------------
void CMapWorld::CullTree_Free(void)
{
	if (m_pCullTree != NULL)
	{
		CullTree_FreeNode(m_pCullTree);
		m_pCullTree = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Determines if this node is a node or a leaf. If it is a node, it will
//			be split into eight children and each child will be populated with
//			objects whose bounding boxes intersect them, then split recursively.
//			If this node is a leaf, no action is taken and recursion terminates.
// Input  : pNode - 
//-----------------------------------------------------------------------------
#define MIN_NODE_DIM			1024		// Minimum node size of 170 x 170 x 170 feet
#define MIN_NODE_OBJECT_SPLIT	2			// Don't split nodes with fewer than two objects.

void CMapWorld::CullTree_SplitNode(CCullTreeNode *pNode)
{
	Vector Mins;
	Vector Maxs;
	Vector Size;

	pNode->GetBounds(Mins, Maxs);
	VectorSubtract(Maxs, Mins, Size);

	if ((Size[0] > MIN_NODE_DIM) && (Size[1] > MIN_NODE_DIM) && (Size[2] > MIN_NODE_DIM))
	{
		Vector Mids;
		int nChild;

		Mids[0] = (Mins[0] + Maxs[0]) / 2.0;
		Mids[1] = (Mins[1] + Maxs[1]) / 2.0;
		Mids[2] = (Mins[2] + Maxs[2]) / 2.0;

		for (nChild = 0; nChild < 8; nChild++)
		{
			Vector ChildMins;
			Vector ChildMaxs;

			//
			// Create a child and set its bounding box.
			//
			CCullTreeNode *pChild = new CCullTreeNode;

			if (nChild & 1)
			{
				ChildMins[0] = Mins[0];
				ChildMaxs[0] = Mids[0];
			}
			else
			{
				ChildMins[0] = Mids[0];
				ChildMaxs[0] = Maxs[0];
			}

			if (nChild & 2)
			{
				ChildMins[1] = Mins[1];
				ChildMaxs[1] = Mids[1];
			}
			else
			{
				ChildMins[1] = Mids[1];
				ChildMaxs[1] = Maxs[1];
			}

			if (nChild & 4)
			{
				ChildMins[2] = Mins[2];
				ChildMaxs[2] = Mids[2];
			}
			else
			{
				ChildMins[2] = Mids[2];
				ChildMaxs[2] = Maxs[2];
			}

			pChild->UpdateBounds(ChildMins, ChildMaxs);
			
			pNode->AddCullTreeChild(pChild);

			Vector mins1;
			Vector maxs1;
			pChild->GetBounds(mins1, maxs1);

			//
			// Check all objects in this node against the child's bounding box, adding the
			// objects that intersect to the child's object list.
			//
			int nObjectCount = pNode->GetObjectCount();
			for (int nObject = 0; nObject < nObjectCount; nObject++)
			{
				CMapClass *pObject = pNode->GetCullTreeObject(nObject);
				ASSERT(pObject != NULL);

				Vector mins2;
				Vector maxs2;
				pObject->GetCullBox(mins2, maxs2);
				if (BoxesIntersect(mins1, maxs1, mins2, maxs2))
				{
					pChild->AddCullTreeObject(pObject);
				}
			}
		}
				
		//
		// Remove all objects from this node's object list (since we are not a leaf).
		//
		pNode->RemoveAllCullTreeObjects();

		//
		// Recurse into all children with at least two objects, splitting them.
		//
		int nChildCount = pNode->GetChildCount();
		for (nChild = 0; nChild < nChildCount; nChild++)
		{
			CCullTreeNode *pChild = pNode->GetCullTreeChild(nChild);
			if (pChild->GetObjectCount() >= MIN_NODE_OBJECT_SPLIT)
			{
				CullTree_SplitNode(pChild);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pNode - 
//-----------------------------------------------------------------------------
void CMapWorld::CullTree_DumpNode(CCullTreeNode *pNode, int nDepth)
{
	int nChildCount = pNode->GetChildCount();
	char szText[100];

	if (nChildCount == 0)
	{
		// Leaf
		OutputDebugString("LEAF:\n");
		int nObjectCount = pNode->GetObjectCount();
		for (int nObject = 0; nObject < nObjectCount; nObject++)
		{
			CMapClass *pMapClass = pNode->GetCullTreeObject(nObject);
			sprintf(szText, "%*c %X %s\n", nDepth, ' ', pMapClass, pMapClass->GetType());
			OutputDebugString(szText);
		}
	}
	else
	{
		// Node
		sprintf(szText, "%*s\n", nDepth, "+");
		OutputDebugString(szText);

		for (int nChild = 0; nChild < nChildCount; nChild++)
		{
			CCullTreeNode *pChild = pNode->GetCullTreeChild(nChild);
			CullTree_DumpNode(pChild, nDepth + 1);
		}

		OutputDebugString("\n");
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapWorld::CullTree_Build(void)
{
	CullTree_Free();
	m_pCullTree = new CCullTreeNode;

	//
	// The top level node in the tree uses the largest possible bounding box.
	//
	Vector BoxMins( g_MIN_MAP_COORD, g_MIN_MAP_COORD, g_MIN_MAP_COORD );
	Vector BoxMaxs( g_MAX_MAP_COORD, g_MAX_MAP_COORD, g_MAX_MAP_COORD );
	m_pCullTree->UpdateBounds(BoxMins, BoxMaxs);

	//
	// Populate the top level node with the contents of the world.
	//
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = Children.GetNext(pos);
		m_pCullTree->AddCullTreeObject(pObject);
	}

	//
	// Recursively split this node into children and populate them.
	//
	CullTree_SplitNode(m_pCullTree);

	//DumpCullTreeNode(m_pCullTree, 1);
	//OutputDebugString("\n");
}


//-----------------------------------------------------------------------------
// Purpose: Returns a list of all the groups in the world.
//-----------------------------------------------------------------------------
void CMapWorld::GetGroupList(CMapObjectList &GroupList)
{
	GroupList.RemoveAll();
	EnumChildrenPos_t pos;
	CMapClass *pChild = GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		CMapGroup *pGroup = dynamic_cast <CMapGroup *>(pChild);
		if (pGroup != NULL)
		{
			GroupList.AddTail(pChild);
		}

		pChild = GetNextDescendent(pos);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called after all objects in the World have been loaded. Calls the
//			PostLoadWorld function for every object in the world, then
//			builds the culling tree.
//-----------------------------------------------------------------------------
void CMapWorld::PostloadWorld(void)
{
	//
	// Set the class name from our "classname" key and discard the key.
	//
	int nIndex;
	const char *pszValue = pszValue = m_KeyValues.GetValue("classname", &nIndex);
	if (pszValue != NULL)
	{
		SetClass(pszValue);
		RemoveKey(nIndex);
	}

	//
	// Get a list of all the groups.
	//
	CMapObjectList GroupList;
	GetGroupList(GroupList);

	//
	// Call PostLoadWorld on all children and assign all loaded objects to their
	// proper groups.
	//
	CMapObjectPairList GroupedObjects;

	POSITION pos;
	CMapClass *pChild = GetFirstChild(pos);
	while (pChild != NULL)
	{
		pChild->PostloadWorld(this);

		//
		// Assign the object to its group, if any.
		//
		const char *pszGroupID = pChild->GetEditorKeyValue("groupid");
		if (pszGroupID != NULL)
		{
			int nID;
			CChunkFile::ReadKeyValueInt(pszGroupID, nID);

			POSITION pos2 = GroupList.GetHeadPosition();
			while (pos2 != NULL)
			{
				CMapGroup *pGroup = (CMapGroup *)GroupList.GetNext(pos2);
				if (pGroup->GetID() == nID)
				{
					// Add the object to a list for removal from the world.
					MapObjectPair_t *pPair = new MapObjectPair_t;

					pPair->pObject1 = pChild;
					pPair->pObject2 = pGroup;

					GroupedObjects.AddTail(pPair);
					break;
				}
			}
		}

		//
		// Add any entities to the global entity list.
		//
		EntityList_Add(pChild);

		pChild = GetNextChild(pos);
	}

	//
	// Remove all the objects that were added to groups from the world, since they are
	// now children of CMapGroup objects that are already in the world.
	//
	POSITION DeletePos = GroupedObjects.GetHeadPosition();
	while (DeletePos != NULL)
	{
		MapObjectPair_t *pPair = GroupedObjects.GetNext(DeletePos);

		RemoveChild(pPair->pObject1);
		pPair->pObject2->AddChild(pPair->pObject1);

		delete pPair;
	}

	//
	// Remove any empty groups or groups with only one child.
	//
	/*bool bFound;
	do
	{
		bFound = false;
		POSITION GroupPos = GroupList.GetHeadPosition();
		while (GroupPos != NULL)
		{
			POSITION DelPos = GroupPos;
			CMapGroup *pGroup = (CMapGroup *)GroupList.GetNext(GroupPos);

			if (!pGroup->GetChildCount() || (pGroup->GetChildCount() == 1))
			{
				//ASSERT(false);

				//
				// We found an empty group or a group with only one child. Remove it.
				//
				bFound = true;
				CMapClass *pParent = pGroup->GetParent();
				pParent->RemoveChild(pGroup);

				//
				// If the group had one child, move the child to the group's parent.
				//
				if (pGroup->GetChildCount() == 1)
				{
					POSITION TempPos;
					CMapClass *pChild = pGroup->GetFirstChild(TempPos);
					pGroup->RemoveAllChildren();
					pParent->AddChild(pChild);
				}

				GroupList.RemoveAt(DelPos);
				delete pGroup;
			}
		}
	} while (bFound);*/

	CullTree_Build();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pData - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapWorld::LoadGroupCallback(CChunkFile *pFile, CMapWorld *pWorld)
{
	CMapGroup *pGroup = new CMapGroup;

	ChunkFileResult_t eResult = pGroup->LoadVMF(pFile);
	if (eResult == ChunkFile_Ok)
	{
		pWorld->AddChild(pGroup);
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pLoadInfo - 
//			*pWorld - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapWorld::LoadHiddenCallback(CChunkFile *pFile, CMapWorld *pWorld)
{
	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("solid", (ChunkHandler_t)LoadSolidCallback, pWorld);
	
	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk();
	pFile->PopHandlers();

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Handles keyvalues when loading the world chunk of MAP files.
// Input  : szKey - Key to handle.
//			szValue - Value of key.
//			pWorld - World being loaded.
// Output : Returns ChunkFile_Ok if all is well.
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapWorld::LoadKeyCallback(const char *szKey, const char *szValue, CMapWorld *pWorld)
{
	if (!stricmp(szKey, "id"))
	{
		pWorld->SetID(atoi(szValue));
	}
	else if (stricmp(szKey, "mapversion") != 0)
	{
		pWorld->SetKeyValue(szKey, szValue);
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pLoadInfo - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapWorld::LoadVMF(CChunkFile *pFile)
{
	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("solid", (ChunkHandler_t)LoadSolidCallback, this);
	Handlers.AddHandler("hidden", (ChunkHandler_t)LoadHiddenCallback, this);
	Handlers.AddHandler("group", (ChunkHandler_t)LoadGroupCallback, this);
	Handlers.AddHandler("connections", (ChunkHandler_t)LoadConnectionsCallback, (CEditGameClass *)this);

	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadKeyCallback, this);
	pFile->PopHandlers();

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pLoadInfo - 
//			*pWorld - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapWorld::LoadSolidCallback(CChunkFile *pFile, CMapWorld *pWorld)
{
	CMapSolid *pSolid = new CMapSolid;

	bool bValid;
	ChunkFileResult_t eResult = pSolid->LoadVMF(pFile, bValid);

	if ((eResult == ChunkFile_Ok) && (bValid))
	{
		const char *pszValue = pSolid->GetEditorKeyValue("cordonsolid");
		if (pszValue == NULL)
		{
			pWorld->AddChild(pSolid);
		}
	}
	else
	{
		delete pSolid;
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Calls PresaveWorld in all of the world's descendents.
//-----------------------------------------------------------------------------
void CMapWorld::PresaveWorld(void)
{
	EnumChildrenPos_t pos;
	CMapClass *pChild = GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		pChild->PresaveWorld();
		pChild = GetNextDescendent(pos);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Saves all solids, entities, and groups in the world to a VMF file.
// Input  : pFile - File object to use for saving.
//			pSaveInfo - Holds rules for which objects to save.
// Output : Returns ChunkFile_Ok if the save was successful, or an error code.
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapWorld::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo, int saveFlags)
{
	PresaveWorld();

	//
	// Sort the world objects into lists for saving into different chunks.
	//
	SaveLists_t SaveLists;
	EnumChildrenRecurseGroupsOnly((ENUMMAPCHILDRENPROC)BuildSaveListsCallback, (DWORD)&SaveLists);

	//
	// Begin the world chunk.
	//
	ChunkFileResult_t  eResult = ChunkFile_Ok;

	if( !(saveFlags & SAVEFLAGS_LIGHTSONLY) )
	{
		eResult = pFile->BeginChunk("world");

		//
		// Save world ID - it's always zero.
		//
		if (eResult == ChunkFile_Ok)
		{
			eResult = pFile->WriteKeyValueInt("id", GetID());
		}

		//
		// HACK: Save map version. This is already being saved in the version info block by the doc.
		//
		if (eResult == ChunkFile_Ok)
		{
			eResult = pFile->WriteKeyValueInt("mapversion", CMapDoc::GetActiveMapDoc()->GetDocVersion());
		}

		//
		// Save world keys.
		//
		if (eResult == ChunkFile_Ok)
		{
			CEditGameClass::SaveVMF(pFile, pSaveInfo);
		}

		//
		// Save world solids.
		//
		if (eResult == ChunkFile_Ok)
		{
			eResult = SaveObjectListVMF(pFile, pSaveInfo, &SaveLists.Solids, saveFlags);
		}

		//
		// Save groups.
		//
		if (eResult == ChunkFile_Ok)
		{
			eResult = SaveObjectListVMF(pFile, pSaveInfo, &SaveLists.Groups, saveFlags);
		}

		//
		// End the world chunk.
		//
		if (eResult == ChunkFile_Ok)
		{
			pFile->EndChunk();
		}
	}

	//
	// Save entities and their solid children.
	//
	if (eResult == ChunkFile_Ok)
	{
		eResult = SaveObjectListVMF(pFile, pSaveInfo, &SaveLists.Entities, saveFlags);
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pList - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapWorld::SaveObjectListVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo, CMapObjectList *pList, int saveFlags)
{
	POSITION pos = pList->GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = pList->GetNext(pos);

		// Only save lights if that's what they want.
		if( saveFlags & SAVEFLAGS_LIGHTSONLY )
		{
			CMapEntity *pMapEnt = dynamic_cast<CMapEntity*>( pObject );
			bool bIsLight = pMapEnt && strncmp( pMapEnt->GetClassName(), "light", 5 ) == 0;
			if( !bIsLight )
				continue;
		}

		
		if (pObject != NULL)
		{
			ChunkFileResult_t eResult = pObject->SaveVMF(pFile, pSaveInfo);
			if (eResult != ChunkFile_Ok)
			{
				return(eResult);
			}
		}
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: Adds a given character to the end of a string if there isn't one already.
// Input  : psz - String to add the backslash to.
//			ch - Character to check for (and add if not found).
//			nSize - Size of buffer pointer to by psz.
// Output : Returns true if there was enough space in the dest buffer, false if not.
//-----------------------------------------------------------------------------
static bool EnsureTrailingChar(char *psz, char ch, int nSize)
{
	int nLen = strlen(psz);
	if ((psz[0] != '\0') && (psz[nLen - 1] != ch))
	{
		if (nLen < (nSize - 1))
		{
			psz[nLen++] = ch;
			psz[nLen] = '\0';
		}
		else
		{
			// No room to add the character.
			return(false);
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Finds the face with the corresponding face ID.
//			FIXME: AAARGH, slow!! Need to build a table or something.
// Input  : nFaceID - 
//-----------------------------------------------------------------------------
CMapFace *CMapWorld::FaceID_FaceForID(int nFaceID)
{
	EnumChildrenPos_t pos;
	CMapClass *pChild = GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		CMapSolid *pSolid = dynamic_cast <CMapSolid *>(pChild);
		if (pSolid != NULL)
		{
			int nFaceCount = pSolid->GetFaceCount();
			for (int nFace = 0; nFace < nFaceCount; nFace++)
			{
				CMapFace *pFace = pSolid->GetFace(nFace);
				if (pFace->GetFaceID() == nFaceID)
				{
					return(pFace);
				}
			}
		}

		pChild = GetNextDescendent(pos);
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Concatenates strings without overrunning the dest buffer.
// Input  : szDest - 
//			szSrc - 
//			nDestSize - 
// Output : Returns true if all chars were copied, false if we ran out of room.
//-----------------------------------------------------------------------------
static bool AppendString(char *szDest, char const *szSrc, int nDestSize)
{
	int nDestLen = strlen(szDest);
	int nDestAvail = nDestSize - nDestLen - 1;

	char *pszStart = szDest + nDestLen;
	char *psz = pszStart;

	while ((nDestAvail > 0) && (*szSrc != '\0'))
	{
		*psz++ = *szSrc++;
		nDestAvail--;
	}

	*psz = '\0';

	if (*szSrc != '\0')
	{
		// If we ran out of room, don't append anything. We don't want partial strings.
		*pszStart = '\0';
	}

	return(*szSrc == '\0');
}


//-----------------------------------------------------------------------------
// Purpose: Encode the list of fully selected and partially selected faces in
//			a string of the form: "4 5 12 (1 8)"
//
//			This is used for multiselect editing of sidelist keyvalues.
//
// Input  : pszValue - The buffer to receive the face lists encoded as a string.
//			pFullFaceList - the list of faces that are considered fully in the list
//			pPartialFaceList - the list of faces that are partially in the list
//-----------------------------------------------------------------------------
bool CMapWorld::FaceID_FaceIDListsToString(char *pszList, int nSize, CMapFaceIDList *pFullFaceIDList, CMapFaceIDList *pPartialFaceIDList)
{
	if (pszList == NULL)
	{
		return(false);
	}

	pszList[0] = '\0';

	//
	// Add the fully selected faces, space delimited.
	//
	if (pFullFaceIDList != NULL)
	{
		for (int i = 0; i < pFullFaceIDList->Count(); i++)
		{
			int nFace = pFullFaceIDList->Element(i);

			char szID[64];
			itoa(nFace, szID, 10);
			if (!EnsureTrailingChar(pszList, ' ', nSize) || !AppendString(pszList, szID, nSize))
			{
				return(false);
			}
		}
	}

	//
	// Add the partially selected faces inside of parentheses.
	//
	if (pPartialFaceIDList != NULL)
	{
		if (pPartialFaceIDList->Count() > 0)
		{
			if (!EnsureTrailingChar(pszList, ' ', nSize) || !AppendString(pszList, "(", nSize))
			{
				return(false);
			}

			bool bFirst = true;
			
			for (int i = 0; i < pPartialFaceIDList->Count(); i++)
			{
				int nFace = pPartialFaceIDList->Element(i);

				char szID[64];
				itoa(nFace, szID, 10);
				if (!bFirst)
				{
					if (!EnsureTrailingChar(pszList, ' ', nSize))
					{
						return(false);
					}
				}
				bFirst = false;
				if (!AppendString(pszList, szID, nSize))
				{
					return(false);
				}
			}

			AppendString(pszList, ")", nSize);
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Encode the list of fully selected and partially selected faces in
//			a string of the form: "4 5 12 (1 8)"
//
//			This is used for multiselect editing of sidelist keyvalues.
//
// Input  : pszValue - The buffer to receive the face lists encoded as a string.
//			pFullFaceList - the list of faces that are considered fully in the list
//			pPartialFaceList - the list of faces that are partially in the list
//-----------------------------------------------------------------------------
bool CMapWorld::FaceID_FaceListsToString(char *pszList, int nSize, CMapFaceList *pFullFaceList, CMapFaceList *pPartialFaceList)
{
	if (pszList == NULL)
	{
		return(false);
	}

	pszList[0] = '\0';

	//
	// Add the fully selected faces, space delimited.
	//
	if (pFullFaceList != NULL)
	{
		for (int i = 0; i < pFullFaceList->Count(); i++)
		{
			CMapFace *pFace = pFullFaceList->Element(i);

			char szID[64];
			itoa(pFace->GetFaceID(), szID, 10);
			if (!EnsureTrailingChar(pszList, ' ', nSize) || !AppendString(pszList, szID, nSize))
			{
				return(false);
			}
		}
	}

	//
	// Add the partially selected faces inside of parentheses.
	//
	if (pPartialFaceList != NULL)
	{
		if (pPartialFaceList->Count() > 0)
		{
			if (!EnsureTrailingChar(pszList, ' ', nSize) || !AppendString(pszList, "(", nSize))
			{
				return(false);
			}

			bool bFirst = true;
			
			for (int i = 0; i < pPartialFaceList->Count(); i++)
			{
				CMapFace *pFace = pPartialFaceList->Element(i);

				char szID[64];
				itoa(pFace->GetFaceID(), szID, 10);
				if (!bFirst)
				{
					if (!EnsureTrailingChar(pszList, ' ', nSize))
					{
						return(false);
					}
				}
				bFirst = false;
				if (!AppendString(pszList, szID, nSize))
				{
					return(false);
				}
			}

			AppendString(pszList, ")", nSize);
		}
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Decode a string of the form: "4 5 12 (1 8)" into a list of fully
//			selected and a list of partially selected face IDs.
//
//			This is used for multiselect editing of sidelist keyvalues.
//
// Input  : pszValue - The buffer to receive the face lists encoded as a string.
//			pFullFaceList - the list of faces that are considered fully in the list
//			pPartialFaceList - the list of faces that are partially in the list
//-----------------------------------------------------------------------------
void CMapWorld::FaceID_StringToFaceIDLists(CMapFaceIDList *pFullFaceList, CMapFaceIDList *pPartialFaceList, const char *pszValue)
{
	if (pFullFaceList != NULL)
	{
		pFullFaceList->RemoveAll();
	}

	if (pPartialFaceList != NULL)
	{
		pPartialFaceList->RemoveAll();
	}

	if (pszValue != NULL)
	{
		char szVal[KEYVALUE_MAX_VALUE_LENGTH];
		strcpy(szVal, pszValue);

		int nParens = 0;
		bool bInParens = false;

		char *psz = strtok(szVal, " ");
		while (psz != NULL)
		{
			//
			// Strip leading or trailing parentheses from the substring.
			//
			bool bFirstValid = true;
			char *pszRemoveParens = psz;
			while (*pszRemoveParens != '\0')
			{
				if (*pszRemoveParens == '(')
				{
					nParens++;
					*pszRemoveParens = '\0';
				}
				else if (*pszRemoveParens == ')')
				{
					nParens--;
					*pszRemoveParens = '\0';
				}
				else if (bFirstValid)
				{
					//
					// Note the parentheses depth at the start of this number.
					//
					if (nParens > 0)
					{
						bInParens = true;
					}
					else
					{
						bInParens = false;
					}

					psz = pszRemoveParens;
					bFirstValid = false;
				}
				pszRemoveParens++;
			}

			//
			// The substring should now be a single face ID. Get the corresponding
			// face and add it to the list.
			//
			int nFaceID = atoi(psz);
			if (bInParens)
			{
				if (pPartialFaceList != NULL)
				{
					pPartialFaceList->AddToTail(nFaceID);
				}
			}
			else
			{
				if (pFullFaceList != NULL)
				{
					pFullFaceList->AddToTail(nFaceID);
				}
			}

			//
			// Get the next substring.
			//
			psz = strtok(NULL, " ");
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Decode a string of the form: "4 5 12 (1 8)" into a list of fully
//			selected and a list of partially selected faces.
//
//			This is used for multiselect editing of sidelist keyvalues.
//
// Input  : pszValue - The buffer to receive the face lists encoded as a string.
//			pFullFaceList - the list of faces that are considered fully in the list
//			pPartialFaceList - the list of faces that are partially in the list
//-----------------------------------------------------------------------------
void CMapWorld::FaceID_StringToFaceLists(CMapFaceList *pFullFaceList, CMapFaceList *pPartialFaceList, const char *pszValue)
{
	CMapFaceIDList FullFaceIDList;
	CMapFaceIDList PartialFaceIDList;

	FaceID_StringToFaceIDLists(&FullFaceIDList, &PartialFaceIDList, pszValue);

	if (pFullFaceList != NULL)
	{
		pFullFaceList->RemoveAll();

		for (int i = 0; i < FullFaceIDList.Count(); i++)
		{
			//
			// Get the corresponding face and add it to the list.
			//
			// FACEID TODO: fix so we only interate the world objects once
			CMapFace *pFace = FaceID_FaceForID(FullFaceIDList.Element(i));
			if (pFace != NULL)
			{
				pFullFaceList->AddToTail(pFace);
			}
		}
	}

	if (pPartialFaceList != NULL)
	{
		pPartialFaceList->RemoveAll();

		for (int i = 0; i < PartialFaceIDList.Count(); i++)
		{
			//
			// Get the corresponding face and add it to the list.
			//
			// FACEID TODO: fix so we only interate the world objects once
			CMapFace *pFace = FaceID_FaceForID(PartialFaceIDList.Element(i));
			if (pFace != NULL)
			{
				pPartialFaceList->AddToTail(pFace);
			}
		}
	}
}

