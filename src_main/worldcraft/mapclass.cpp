//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include <afxtempl.h>
#include "ChunkFile.h"
#include "SaveInfo.h"
#include "MapClass.h"
#include "MapEntity.h"			// dvs: evil - base knows about the derived class
#include "MapGroup.h"			// dvs: evil - base knows about the derived class
#include "MapWorld.h"			// dvs: evil - base knows about the derived class
#include "GlobalFunctions.h"
#include "GameData.h"
#include "MapDoc.h"
#include "VisGroup.h"


typedef CArray<MCMSTRUCT, MCMSTRUCT&> ClassList;
static ClassList *pClasses;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//			pfnNew - 
//-----------------------------------------------------------------------------
CMapClassManager::CMapClassManager(MAPCLASSTYPE Type, CMapClass *(*pfnNew)())
{
	if (!pClasses)
	{
		pClasses = new ClassList;
	}

	MCMSTRUCT mcms;
	mcms.Type = Type;
	mcms.pfnNew = pfnNew;
	
	pClasses->Add(mcms);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapClassManager::~CMapClassManager(void)
{
	if (pClasses)
	{
		pClasses->RemoveAll();
		delete pClasses;
		pClasses = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapClassManager::CreateObject(MAPCLASSTYPE Type)
{
	int i = pClasses->GetSize() - 1;
	unsigned uLen = strlen(Type)+1;
	for (; i >= 0; i--)
	{
		MCMSTRUCT &mcms = pClasses->GetData()[i];
		if (!memcmp(mcms.Type, Type, uLen))
		{
			return (*mcms.pfnNew)();
		}
	}

	ASSERT(FALSE);
	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor. Initializes data members.
//-----------------------------------------------------------------------------
CMapClass::CMapClass(void)
{
	//
	// The document manages the unique object IDs. Eventually all object construction
	// should be done through the document, eliminating the need for CMapClass to know
	// about CMapDoc.
	//
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc != NULL)
	{
		m_nID = pDoc->GetNextMapObjectID();
	}
	else
	{
		m_nID = 0;
	}

	dwKept = 0;
	m_bVisible = true;
	m_bVisible2D = true;

	m_bTemporary = FALSE;
	m_pVisGroup = NULL;
	r = g = b = 220;
	Parent = NULL;
	m_nRenderFrame = 0;
	m_bShouldSerialize = TRUE;
	m_pEditorKeys = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Deletes all children.
//-----------------------------------------------------------------------------
CMapClass::~CMapClass(void)
{
	//
	// Delete all of our children.
	//
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = Children.GetNext(pos);
		delete pObject;
	}

	delete m_pEditorKeys;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pDependent - 
//-----------------------------------------------------------------------------
void CMapClass::AddDependent(CMapClass *pDependent)
{
	//
	// Never add ourselves to our dependents. It creates a circular dependency
	// which is bad.
	//
	if (pDependent != this)
	{
		//
		// Also, never add one of our ancestors as a dependent. This too creates a
		// nasty circular dependency.
		//
		bool bIsOurAncestor = false;
		CMapClass *pTestParent = GetParent();
		while (pTestParent != NULL)
		{
			if (pTestParent == pDependent)
			{
				bIsOurAncestor = true;
				break;
			}

			pTestParent = pTestParent->GetParent();
		}

		if (!bIsOurAncestor)
		{
			m_Dependents.AddTail(pDependent);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns a copy of this object. We should never call this implementation
//			since CMapClass cannot be instantiated.
// Input  : bUpdateDependencies - Whether to update object dependencies when copying object pointers.
//-----------------------------------------------------------------------------
CMapClass *CMapClass::Copy(bool bUpdateDependencies)
{
	ASSERT(FALSE);
	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Turns this object into a duplicate of the given object.
// Input  : pFrom - The object to replicate.
// Output : Returns a pointer to this object.
//-----------------------------------------------------------------------------
CMapClass *CMapClass::CopyFrom(CMapClass *pFrom, bool bUpdateDependencies)
{
	//
	// Copy CMapPoint stuff. dvs: should be in CMapPoint implementation!
	//
	m_Origin = pFrom->m_Origin;

	//
	// Copy CMapAtom stuff. dvs: should be in CMapAtom implementation!
	//
	//m_eSelectionState = pFrom->m_eSelectionState; dvs: don't copy selection state - causes bad behavior when hollowing, for example
	m_pParent = pFrom->m_pParent;

	//
	// Copy CMapClass stuff.
	//
	m_pVisGroup = pFrom->m_pVisGroup;
	m_bShouldSerialize = pFrom->m_bShouldSerialize;
	//m_bVisible = pFrom->m_bVisible;
	m_bTemporary = pFrom->m_bTemporary;
	m_bVisible2D = pFrom->m_bVisible2D;
	m_nRenderFrame = pFrom->m_nRenderFrame;
	m_CullBox = pFrom->m_CullBox;
	m_Render2DBox = pFrom->m_Render2DBox;

	r = pFrom->r;
	g = pFrom->g;
	b = pFrom->b;

	m_Dependents.RemoveAll();
	m_Dependents.AddTail(&pFrom->m_Dependents);

	// dvs: should I copy m_pEditorKeys?

	//
	// Don't link to the parent if we're not updating dependencies, just copy the pointer.
	//
	if (bUpdateDependencies)
	{
		UpdateParent(pFrom->Parent);
	}
	else
	{
		Parent = pFrom->Parent;
	}

	return(this);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the culling bbox of this object.
// Input  : mins - receives the minima for culling
//			maxs - receives the maxima for culling.
//-----------------------------------------------------------------------------
void CMapClass::GetCullBox(Vector &mins, Vector &maxs)
{
	m_CullBox.GetBounds(mins, maxs);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the bbox for 2D rendering of this object.
//			FIXME: this can be removed if we do all our 2D rendering in this->Render2D.
// Input  : mins - receives the minima for culling
//			maxs - receives the maxima for culling.
//-----------------------------------------------------------------------------
void CMapClass::GetRender2DBox(Vector &mins, Vector &maxs)
{
	m_Render2DBox.GetBounds(mins, maxs);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CMapClass::GetEditorKeyValue(const char *szKey)
{
	if (m_pEditorKeys != NULL)
	{
		return(m_pEditorKeys->GetValue(szKey));
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Begins iterating this object's children. This function does not
//			recurse into grandchildren. Use GetFirstDescendent for that.
// Input  : pos - An iterator.
// Output : Returns a pointer to the first child, NULL if none.
//-----------------------------------------------------------------------------
CMapClass *CMapClass::GetFirstChild(POSITION &pos)
{
	pos = Children.GetHeadPosition();
	if (pos != NULL)
	{
		return(Children.GetNext(pos));
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Continues iterating this object's children. This function does not
//			recurse into grandchildren. Use GetNextDescendent for that.
// Input  : pos - An iterator.
// Output : Returns a pointer to the next child, NULL if none.
//-----------------------------------------------------------------------------
CMapClass *CMapClass::GetNextChild(POSITION &pos)
{
	if (pos != NULL)
	{
		return(Children.GetNext(pos));
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Begins a depth-first search of the map heirarchy.
// Input  : pos - An iterator 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapClass::GetFirstDescendent(EnumChildrenPos_t &pos)
{
	pos.nDepth = 0;

	pos.Stack[0].pParent = this;
	pos.Stack[0].pos = Children.GetHeadPosition();

	if (pos.Stack[0].pos != NULL)
	{
		return(GetNextDescendent(pos));
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Continues a depth-first search of the map heirarchy.
// Input  : &pos - 
// Output : CMapClass
//-----------------------------------------------------------------------------
CMapClass *CMapClass::GetNextDescendent(EnumChildrenPos_t &pos)
{
	while (pos.nDepth >= 0)
	{
		while (pos.Stack[pos.nDepth].pos != NULL)
		{
			//
			// Get the next child of the parent on top of the stack.
			//
			CMapClass *pChild = pos.Stack[pos.nDepth].pParent->Children.GetNext(pos.Stack[pos.nDepth].pos);

			//
			// If this object has children, push it onto the stack.
			//
			POSITION HeadPos = pChild->Children.GetHeadPosition();
			if (HeadPos != NULL)
			{
				pos.nDepth++;

				if (pos.nDepth < MAX_ENUM_CHILD_DEPTH)
				{
					pos.Stack[pos.nDepth].pParent = pChild;
					pos.Stack[pos.nDepth].pos = HeadPos;
				}
				else
				{
					// dvs: stack overflow!
					pos.nDepth--;
				}
			}
			//
			// If this object has no children, return it.
			//
			else
			{
				return(pChild);
			}
		}
		
		//
		// Finished with this object's children, pop the stack and return the object.
		//
		pos.nDepth--;
		if (pos.nDepth >= 0)
		{
			return(pos.Stack[pos.nDepth + 1].pParent);
		}
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the world object that the given object belongs to.
// Input  : pStart - Object to traverse up from to find the world object.
//-----------------------------------------------------------------------------
CMapWorld *CMapClass::GetWorldObject(CMapClass *pStart)
{
	CMapClass *pobj = pStart;

	while (pobj != NULL)
	{
		if (pobj->IsWorldObject())
		{
			return((CMapWorld *)pobj);
		}
		pobj = pobj->Parent;
	}

	// has no world:
	return NULL;
}


BOOL CMapClass::IsChildOf(CMapClass *pObject)
{
	CMapClass *pParent = Parent;

	while(1)
	{
		if(pParent == pObject)
			return TRUE;
		if(pParent->IsWorldObject())
			break;	// world object, not parent .. return false.
		pParent = pParent->Parent;
	}

	return FALSE;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the visgroup to which this object belongs, NULL if none.
//			If the object has a parent other than the world, it belongs to its
//			parent's visgroup.
//-----------------------------------------------------------------------------
CVisGroup *CMapClass::GetVisGroup(void)
{
	if (!Parent || Parent->IsWorldObject())
	{
		return(m_pVisGroup);
	}

	return(Parent->GetVisGroup());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapClass::SetColorFromVisGroups(void)
{
	if (m_pVisGroup)
	{
		// set group color
		color32 rgbColor = m_pVisGroup->GetColor();
		SetRenderColor(rgbColor);
	}
	else if (Parent && !Parent->IsWorldObject())
	{
		color32 rgbColor = Parent->GetRenderColor();
		SetRenderColor(rgbColor);
	}
	else
	{
		// random blue/green
		SetRenderColor(0, 100 + (random() % 156), 100 + (random() % 156));
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVisGroup - 
//-----------------------------------------------------------------------------
void CMapClass::SetVisGroup(CVisGroup *pVisGroup)
{
	m_pVisGroup = pVisGroup;
	SetColorFromVisGroups();
}


//-----------------------------------------------------------------------------
// Purpose: Adds the specified child to this object.
// Input  : pChild - Object to add as a child of this object.
//-----------------------------------------------------------------------------
void CMapClass::AddChild(CMapClass *pChild)
{
	if (Children.Find(pChild))
	{
		pChild->Parent = this;
		return;
	}

	Children.AddTail(pChild);
	pChild->Parent = this;

	//
	// Update our bounds with the child's bounds.
	//
	Vector vecMins;
	Vector vecMaxs;

	pChild->GetCullBox(vecMins, vecMaxs);
	m_CullBox.UpdateBounds(vecMins, vecMaxs);

	pChild->GetRender2DBox(vecMins, vecMaxs);
	m_Render2DBox.UpdateBounds(vecMins, vecMaxs);

	if (Parent != NULL)
	{
		Parent->UpdateChild(this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Removes all of this object's children.
//-----------------------------------------------------------------------------
void CMapClass::RemoveAllChildren(void)
{
	//
	// Detach the children from us. They are no longer in our world heirarchy.
	//
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{	
		CMapClass *pChild = Children.GetNext(pos);
		pChild->Parent = NULL;
	}	

	//
	// Remove them from our list.
	//
	Children.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: Removes the specified child from this object.
// Input  : pChild - The child to remove.
//			bUpdateBounds - TRUE to calculate new bounds, FALSE not to.
//-----------------------------------------------------------------------------
void CMapClass::RemoveChild(CMapClass *pChild, bool bUpdateBounds)
{
	POSITION p = Children.Find(pChild);
	if (!p)
	{
		pChild->Parent = NULL;
		return;
	}

	Children.RemoveAt(p);
	pChild->Parent = NULL;

	if (bUpdateBounds)
	{
		PostUpdate(Notify_Removed);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Copies all children of a given object as children of this object.
//			NOTE: The child objects are replicated, not merely added as children.
// Input  : pobj - The object whose children are to be copied.
//-----------------------------------------------------------------------------
void CMapClass::CopyChildrenFrom(CMapClass *pobj, bool bUpdateDependencies)
{
	POSITION p = pobj->Children.GetHeadPosition();
	while (p != NULL)
	{
		CMapClass *pChild = pobj->Children.GetNext(p);

		CMapClass *pChildCopy = pChild->Copy(bUpdateDependencies);
		pChildCopy->CopyChildrenFrom(pChild, bUpdateDependencies);
		AddChild(pChildCopy);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Recalculate's this object's bounding boxes. CMapClass-derived classes
//			should call this first, then update using their local data.
// Input  : bFullUpdate - When set to TRUE, call CalcBounds on all children
//				before updating our bounds.
//-----------------------------------------------------------------------------
void CMapClass::CalcBounds(BOOL bFullUpdate)
{
	m_CullBox.ResetBounds();
	m_Render2DBox.ResetBounds();

	POSITION p = Children.GetHeadPosition();
	while (p)
	{
		CMapClass *pChild = Children.GetNext(p);
		if (bFullUpdate)
		{
			pChild->CalcBounds(TRUE);
		}

		m_CullBox.UpdateBounds(&pChild->m_CullBox);
		m_Render2DBox.UpdateBounds(&pChild->m_Render2DBox);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the render color of this object and all its children.
// Input  : uchRed, uchGreen, uchBlue - Color components.
//-----------------------------------------------------------------------------
void CMapClass::SetRenderColor(color32 rgbColor)
{
	CMapAtom::SetRenderColor(rgbColor);

	//
	// Set the render color of all our children.
	//
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pChild = Children.GetNext(pos);
		if (pChild != NULL)
		{
			pChild->SetRenderColor(rgbColor);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the render color of this object and all its children.
// Input  : uchRed, uchGreen, uchBlue - Color components.
//-----------------------------------------------------------------------------
void CMapClass::SetRenderColor(unsigned char uchRed, unsigned char uchGreen, unsigned char uchBlue)
{
	CMapAtom::SetRenderColor(uchRed, uchGreen, uchBlue);

	//
	// Set the render color of all our children.
	//
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pChild = Children.GetNext(pos);
		if (pChild != NULL)
		{
			pChild->SetRenderColor(uchRed, uchGreen, uchBlue);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the object that should be added to the selection
//			list because this object was clicked on with a given selection mode.
// Input  : eSelectMode - 
//-----------------------------------------------------------------------------
CMapClass *CMapClass::PrepareSelection(SelectMode_t eSelectMode)
{
	if ((eSelectMode == selectGroups) && (Parent != NULL) && !Parent->IsWorldObject())
	{
		return Parent->PrepareSelection(eSelectMode);
	}

	return this;
}


//-----------------------------------------------------------------------------
// Purpose: Calls an enumerating function for each of our children that are of
//			of a given type, recursively enumerating their children also.
// Input  : pfn - Enumeration callback function. Called once per child.
//			dwParam - User data to pass into the enumerating callback.
//			Type - Unless NULL, only objects of the given type will be enumerated.
// Output : Returns FALSE if the enumeration was terminated early, TRUE if it completed.
//-----------------------------------------------------------------------------
BOOL CMapClass::EnumChildren(ENUMMAPCHILDRENPROC pfn, DWORD dwParam, MAPCLASSTYPE Type)
{
	POSITION p = Children.GetHeadPosition();
	while (p)
	{
		CMapClass *pChild = Children.GetNext(p);
		if (!Type || pChild->IsMapClass(Type))
		{
			if(!(*pfn)(pChild, dwParam))
			{
				return FALSE;
			}
		}

		// enum this child's children
		if (!pChild->EnumChildren(pfn, dwParam, Type))
		{
			return FALSE;
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Enumerates a this object's children, only recursing into groups.
//			Children of entities will not be enumerated.
// Input  : pfn - Enumeration callback function. Called once per child.
//			dwParam - User data to pass into the enumerating callback.
//			Type - Unless NULL, only objects of the given type will be enumerated.
// Output : Returns FALSE if the enumeration was terminated early, TRUE if it completed.
//-----------------------------------------------------------------------------
BOOL CMapClass::EnumChildrenRecurseGroupsOnly(ENUMMAPCHILDRENPROC pfn, DWORD dwParam, MAPCLASSTYPE Type)
{
	POSITION pos = Children.GetHeadPosition();
	while (pos)
	{
		CMapClass *pChild = Children.GetNext(pos);

		if (!Type || pChild->IsMapClass(Type))
		{
			if (!(*pfn)(pChild, dwParam))
			{
				return FALSE;
			}
		}

		if (pChild->IsGroup())
		{
			if (!pChild->EnumChildrenRecurseGroupsOnly(pfn, dwParam, Type))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Iterates through an object, and all it's children, looking for an
//			entity with a matching key and value
// Input  : key - 
//			value - 
// Output : CMapEntity - the entity found
//-----------------------------------------------------------------------------
CMapEntity *CMapClass::FindChildByKeyValue( LPCSTR key, LPCSTR value )
{
	if ( !key || !value )
		return NULL;

	POSITION p = Children.GetHeadPosition();
	while( p )
	{
		CMapClass *pChild = Children.GetNext( p );
		CMapEntity *e = pChild->FindChildByKeyValue( key, value );
		if ( e )
			return e;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Called after this object is added to the world.
//
//			NOTE: This function is NOT called during serialization. Use PostloadWorld
//				  to do similar bookkeeping after map load.
//
// Input  : pWorld - The world that we have been added to.
//-----------------------------------------------------------------------------
void CMapClass::OnAddToWorld(CMapWorld *pWorld)
{
	//
	// Notify all our children.
	//
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pChild = Children.GetNext(pos);
		pChild->OnAddToWorld(pWorld);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called to notify the object that it has just been cloned
//			iterates through and notifies all the children of their cloned state
//			NOTE: assumes that the children are in the same order in both the
//			original and the clone
// Input  : pNewObj - the clone of this object
//			OriginalList - The list of objects that were cloned
//			NewList - The parallel list of clones of objects in OriginalList
//-----------------------------------------------------------------------------
void CMapClass::OnClone( CMapClass *pNewObj, CMapWorld *pWorld, CMapObjectList &OriginalList, CMapObjectList &NewList )
{
	POSITION p = Children.GetHeadPosition();
	POSITION q = pNewObj->Children.GetHeadPosition();
	while ( p && q )
	{
		CMapClass *pChild = Children.GetNext( p );
		CMapClass *pNewChild = pNewObj->Children.GetNext( q );

		pChild->OnClone( pNewChild, pWorld, OriginalList, NewList );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called to notify the object that it has just been cloned
//			iterates through and notifies all the children of their cloned state
//			NOTE: assumes that the children are in the same order in both the
//			original and the clone
// Input  : pNewObj - the clone of this object
//			OriginalList - The list of objects that were cloned
//			NewList - The parallel list of clones of objects in OriginalList
//-----------------------------------------------------------------------------
void CMapClass::OnPreClone( CMapClass *pNewObj, CMapWorld *pWorld, CMapObjectList &OriginalList, CMapObjectList &NewList )
{
	POSITION p = Children.GetHeadPosition();
	POSITION q = pNewObj->Children.GetHeadPosition();
	while ( p && q )
	{
		CMapClass *pChild = Children.GetNext( p );
		CMapClass *pNewChild = pNewObj->Children.GetNext( q );

		pChild->OnPreClone( pNewChild, pWorld, OriginalList, NewList );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Notifies this object that a copy of itself is about to be pasted.
//			Allows the object to generate new unique IDs in the copy of itself.
// Input  : pCopy - 
//			pSourceWorld - 
//			pDestWorld - 
//			OriginalList - 
//			NewList - 
//-----------------------------------------------------------------------------
void CMapClass::OnPrePaste(CMapClass *pCopy, CMapWorld *pSourceWorld, CMapWorld *pDestWorld, CMapObjectList &OriginalList, CMapObjectList &NewList)
{
	POSITION p = Children.GetHeadPosition();
	POSITION q = pCopy->Children.GetHeadPosition();
	while (p && q)
	{
		CMapClass *pChild = Children.GetNext(p);
		CMapClass *pCopyChild = pCopy->Children.GetNext(q);

		pChild->OnPrePaste(pCopyChild, pSourceWorld, pDestWorld, OriginalList, NewList);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Notifies this object that a copy of itself is being pasted.
//			Allows the object to fixup any references to other objects in the
//			clipboard with references to their copies.
// Input  : pCopy - 
//			pSourceWorld - 
//			pDestWorld - 
//			OriginalList - 
//			NewList - 
//-----------------------------------------------------------------------------
void CMapClass::OnPaste(CMapClass *pCopy, CMapWorld *pSourceWorld, CMapWorld *pDestWorld, CMapObjectList &OriginalList, CMapObjectList &NewList)
{
	POSITION p = Children.GetHeadPosition();
	POSITION q = pCopy->Children.GetHeadPosition();
	while (p && q)
	{
		CMapClass *pChild = Children.GetNext(p);
		CMapClass *pCopyChild = pCopy->Children.GetNext(q);

		pChild->OnPaste(pCopyChild, pSourceWorld, pDestWorld, OriginalList, NewList);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called just after this object has been removed from the world so
//			that it can unlink itself from other objects in the world.
// Input  : pWorld - The world that we were just removed from.
//			bNotifyChildren - Whether we should forward notification to our children.
//-----------------------------------------------------------------------------
void CMapClass::OnRemoveFromWorld(CMapWorld *pWorld, bool bNotifyChildren)
{
	//
	// Since we are being removed from the world, we cannot have any dependents.
	// Notify any dependent objects, so they can release pointers to us.
	// Our dependencies will be regenerated if we are added back into the world.
	//
	NotifyDependents(Notify_Removed);
	m_Dependents.RemoveAll();

	if (bNotifyChildren)
	{
		POSITION pos = Children.GetHeadPosition();
		while (pos != NULL)
		{
			CMapClass *pChild = Children.GetNext(pos);
			pChild->OnRemoveFromWorld(pWorld, true);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called after a map file has been completely loaded.
// Input  : pWorld - The world that we are in.
//-----------------------------------------------------------------------------
void CMapClass::PostloadWorld(CMapWorld *pWorld)
{
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pChild = Children.GetNext(pos);
		pChild->PostloadWorld(pWorld);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Calls RenderPreload for each of our children. This allows them to
//			cache any resources that they need for rendering.
// Input  : pRender - Pointer to the 3D renderer.
//-----------------------------------------------------------------------------
bool CMapClass::RenderPreload(CRender3D *pRender, bool bNewContext)
{
	POSITION pos = Children.GetHeadPosition();

	while (pos != NULL)
	{
		CMapClass *pChild = Children.GetNext(pos);
		pChild->RenderPreload(pRender, bNewContext);
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pRender - 
//-----------------------------------------------------------------------------
void CMapClass::Render2D(CRender2D *pRender)
{
//	POSITION pos = Children.GetHeadPosition();
//	while (pos != NULL)
//	{
//		CMapClass *pChild = Children.GetNext(pos);
//		pChild->Render2D(pRender);
//	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRender - 
//-----------------------------------------------------------------------------
void CMapClass::Render3D(CRender3D *pRender)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pParent - 
//-----------------------------------------------------------------------------
// dvs: HACK to distinguish from CMapAtom::SetParent. Need to merge the two!
//		So far the only reason this is ever called is to prune an object from
//		the world tree, such as when Copying to the clipboard. Maybe there needs
//		to be a Prune/Graft interface instead?
void CMapClass::SetObjParent(CMapClass *pParent)
{
	UpdateParent(pParent);
}


//-----------------------------------------------------------------------------
// Purpose: Transforms all children. Derived implementations should call this,
//			then do their own thing.
// Input  : t - Pointer to class containing transformation information.
//-----------------------------------------------------------------------------
void CMapClass::DoTransform(Box3D *t)
{
	CMapPoint::DoTransform(t);

	POSITION p = Children.GetHeadPosition();
	while (p != NULL)
	{
		CMapClass *pChild = Children.GetNext(p);
		pChild->Transform(t);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Moves all children. Derived implementations should call this,
//			then do their own thing.
// Input  : delta - 
//-----------------------------------------------------------------------------
void CMapClass::DoTransMove(const Vector &delta)
{
	CMapPoint::DoTransMove(delta);

	POSITION p = Children.GetHeadPosition();
	while (p)
	{
		CMapClass *pChild = Children.GetNext(p);
		pChild->TransMove(delta);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pRefPoint - 
//			Angle - 
//-----------------------------------------------------------------------------
void CMapClass::DoTransRotate(const Vector *pRefPoint, const QAngle &Angles)
{
	CMapPoint::DoTransRotate(pRefPoint, Angles);

	//
	// If no reference point was supplied, rotate around
	// our bounding box's center.
	//
	Vector OurRef;
	if (pRefPoint == NULL)
	{
		pRefPoint = &OurRef;
		m_Render2DBox.GetBoundsCenter(OurRef);
	}

	POSITION p = Children.GetHeadPosition();
	while (p)
	{
		CMapClass *pChild = Children.GetNext(p);
		pChild->TransRotate(pRefPoint, Angles);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : RefPoint - 
//			Scale - 
//-----------------------------------------------------------------------------
void CMapClass::DoTransScale(const Vector &RefPoint, const Vector &Scale)
{
	CMapPoint::DoTransScale(RefPoint, Scale);

	POSITION p = Children.GetHeadPosition();
	while(p)
	{
		CMapClass *pChild = Children.GetNext(p);
		pChild->TransScale(RefPoint, Scale);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Flips this object about a given point.
// Input  : RefPoint - Reference point about which to flip. Set any of the indices
//				to COORD_NOTINIT to NOT flip about that axis.
//-----------------------------------------------------------------------------
void CMapClass::DoTransFlip(const Vector &RefPoint)
{
	CMapPoint::DoTransFlip(RefPoint);

	POSITION p = Children.GetHeadPosition();
	while(p)
	{
		CMapClass *pChild = Children.GetNext(p);
		pChild->TransFlip(RefPoint);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
size_t CMapClass::GetSize(void)
{
	return(sizeof(*this));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pView - 
//			point - 
//			nData - 
// Output : 
//-----------------------------------------------------------------------------
CMapClass *CMapClass::HitTest2D(CMapView2D *pView, const Vector2D &point, int &nHitData)
{
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pChild = Children.GetNext(pos);
		CMapClass *pHit = pChild->HitTest2D(pView, point, nHitData);
		if (pHit != NULL)
		{
			return pHit;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the selection state of this object's children.
// Input  : eSelectionState - 
//-----------------------------------------------------------------------------
SelectionState_t CMapClass::SetSelectionState(SelectionState_t eSelectionState)
{
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapAtom *pObject = Children.GetNext(pos);
		pObject->SetSelectionState(eSelectionState);
	}

	return CMapAtom::SetSelectionState(eSelectionState);
}


//-----------------------------------------------------------------------------
// Purpose: Our child's bounding box has changed - recalculate our bounding box.
// Input  : pChild - The child whose bounding box changed.
//-----------------------------------------------------------------------------
void CMapClass::UpdateChild(CMapClass *pChild)
{
	CalcBounds(FALSE);
}


//-----------------------------------------------------------------------------
// Purpose: Returns a coordinate frame to render in
// Input  : matrix - 
// Output : returns true if a new matrix is returned, false if it is invalid
//-----------------------------------------------------------------------------
bool CMapClass::GetTransformMatrix( matrix4_t& matrix )
{
	// try and get our parents transform matrix
	CMapClass *p = CMapClass::GetParent();
	if ( p )
	{
		return p->GetTransformMatrix( matrix );
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pLoadInfo - 
//			pWorld - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapClass::LoadEditorCallback(CChunkFile *pFile, CMapClass *pObject)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadEditorKeyCallback, pObject));
}


//-----------------------------------------------------------------------------
// Purpose: Handles keyvalues when loading the editor chunk of an object from the
//			MAP file. Keys are transferred to a special keyvalue list for use after
//			the entire map has been loaded.
// Input  : szKey - Key to handle.
//			szValue - Value of key.
//			pObject - Object being loaded.
// Output : Returns ChunkFile_Ok.
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapClass::LoadEditorKeyCallback(const char *szKey, const char *szValue, CMapClass *pObject)
{
	if (!stricmp(szKey, "color"))
	{
		CChunkFile::ReadKeyValueColor(szValue, pObject->r, pObject->g, pObject->b);
	}
	else if (!stricmp(szKey, "id"))
	{
		CChunkFile::ReadKeyValueInt(szValue, pObject->m_nID);
	}
	else  if (!stricmp(szKey, "comments"))
	{
		//
		// Load the object comments.
		// HACK: upcast to CEditGameClass *
		//
		CEditGameClass *pEdit = dynamic_cast <CEditGameClass *> (pObject);
		if (pEdit != NULL)
		{
			pEdit->SetComments(szValue);
		}
	}
	else
	{
		pObject->SetEditorKeyValue(szKey, szValue);
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: Call this function after changing this object via transformation,
//			etc. Notifies dependents and updates the parent with this object's
//			new size.
//-----------------------------------------------------------------------------
void CMapClass::PostUpdate(Notify_Dependent_t eNotifyType)
{
	CalcBounds();

	NotifyDependents(eNotifyType);

	if (Parent != NULL)
	{
		Parent->UpdateChild(this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Notifies all our dependents that something about us has changed,
//			giving them the chance to update themselves.
//-----------------------------------------------------------------------------
void CMapClass::NotifyDependents(Notify_Dependent_t eNotifyType)
{
	if (m_Dependents.GetCount() != 0)
	{
		// Get a copy of the dependecies list because it may changes during iteration.
		CMapObjectList TempDependents;
		TempDependents.AddTail(&m_Dependents);

		POSITION pos = TempDependents.GetHeadPosition();
		while (pos != NULL)
		{
			CMapClass *pDependent = TempDependents.GetNext(pos);
			
			//
			// Maybe we should give our dependents the opportunity to unlink themselves here?
			// Returning false from OnNotify could indicate a desire to sever the dependency.
			// Currently this is accomplished by calling UpdateDependency from OnNotifyDependent.
			//
			pDependent->OnNotifyDependent(this, eNotifyType);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Informs us that an object that we are dependent upon has changed,
//			giving us the opportunity to update ourselves accordingly.
// Input  : pObject - Object that we are dependent upon that has changed.
//-----------------------------------------------------------------------------
void CMapClass::OnNotifyDependent(CMapClass *pObject, Notify_Dependent_t eNotifyType)
{
}


//-----------------------------------------------------------------------------
// Purpose: Default implementation for saving editor-specific data. Does nothing.
// Input  : pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapClass::SaveEditorData(CChunkFile *pFile)
{
	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapClass::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	//
	// Write the editor chunk.
	//
	ChunkFileResult_t eResult = pFile->BeginChunk("editor");

	//
	// Save the object's color.
	//
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueColor("color", r, g, b);
	}

	//
	// Save the group ID, if any.
	//
	if (eResult == ChunkFile_Ok)
	{
		CMapGroup *pGroup = dynamic_cast<CMapGroup *>(Parent);
		if (pGroup != NULL)
		{
			eResult = pFile->WriteKeyValueInt("groupid", pGroup->GetID());
		}
	}

	//
	// Save the visgroup ID, if any.
	//
	if ((eResult == ChunkFile_Ok) && (m_pVisGroup != NULL))
	{
		eResult = pFile->WriteKeyValueInt("visgroupid", m_pVisGroup->GetID());
	}

	//
	// Save the object comments, if any.
	// HACK: upcast to CEditGameClass *
	//
	CEditGameClass *pEdit = dynamic_cast <CEditGameClass *> (this);
	if (pEdit != NULL)
	{
		if ((eResult == ChunkFile_Ok) && (strlen(pEdit->GetComments()) > 0))
		{
			eResult = pFile->WriteKeyValue("comments", pEdit->GetComments());
		}
	}

	//
	// Save any other editor-specific data.
	//
	if (eResult == ChunkFile_Ok)
	{
		eResult = SaveEditorData(pFile);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pDependent - 
//-----------------------------------------------------------------------------
void CMapClass::RemoveDependent(CMapClass *pDependent)
{
	POSITION pos = m_Dependents.Find(pDependent);
	if (pos != NULL)
	{
		m_Dependents.RemoveAt(pos);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Frees all the keys that were loaded from the editor chunk of the MAP file.
//-----------------------------------------------------------------------------
void CMapClass::RemoveEditorKeys(void)
{
	delete m_pEditorKeys;
	m_pEditorKeys = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szOldName - 
//			*szNewName - 
//-----------------------------------------------------------------------------
void CMapClass::ReplaceTargetname(const char *szOldName, const char *szNewName)
{
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pObject = Children.GetNext(pos);
		pObject->ReplaceTargetname(szOldName, szNewName);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Updates an object attachment, making this object no longer dependent
//			on changes to the old object, and dependent on changes to the new object.
// Input  : pOldAttached - Object that this object was attached to (possibly NULL).
//			pNewAttached - New object being attached to (possibly NULL).
// Output : Returns pNewAttached.
//-----------------------------------------------------------------------------
CMapClass *CMapClass::UpdateDependency(CMapClass *pOldAttached, CMapClass *pNewAttached)
{
	if (pOldAttached != pNewAttached)
	{
		//
		// If we were attached to another object via this pointer, detach us now.
		//
		if (pOldAttached != NULL)
		{
			pOldAttached->RemoveDependent(this);
		}

		//
		// Attach ourselves as a dependent of the other object. We will now be notified
		// of any changes to that object.
		//
		if (pNewAttached != NULL)
		{
			pNewAttached->AddDependent(this);
		}
	}

	return(pNewAttached);
}


//-----------------------------------------------------------------------------
// Purpose: Updates this object's parent, removing it from it's old parent (if any)
//			attaching it to the new parent (if any).
// Input  : pNewParent - A pointer to the new parent for this object.
// Output : Returns a pointer to the new parent.
//-----------------------------------------------------------------------------
void CMapClass::UpdateParent(CMapClass *pNewParent)
{
	if (Parent != pNewParent)
	{
		if (Parent != NULL)
		{
			Parent->RemoveChild(this);
		}

		if (pNewParent != NULL)
		{
			pNewParent->AddChild(this);
		}
	}

	Parent = pNewParent;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
// Output : const char
//-----------------------------------------------------------------------------
void CMapClass::SetEditorKeyValue(const char *szKey, const char *szValue)
{
	if (m_pEditorKeys == NULL)
	{
		m_pEditorKeys = new KeyValues;
	}

	if (m_pEditorKeys != NULL)
	{
		m_pEditorKeys->SetValue(szKey, szValue);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the origin of this object and its children.
//			FIXME: Should our children necessarily have the same origin as us?
//				   Seems like we should translate our children by our origin delta
//-----------------------------------------------------------------------------
void CMapClass::SetOrigin( Vector& origin )
{
	CMapPoint::SetOrigin( origin );
 
	POSITION pos = Children.GetHeadPosition();

	while ( pos != NULL )
	{
		CMapClass *pChild = Children.GetNext( pos );

		pChild->SetOrigin( origin );
	}

	PostUpdate(Notify_Changed);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bVisible - 
//-----------------------------------------------------------------------------
void CMapClass::SetVisible(bool bVisible)
{
	POSITION pos = Children.GetHeadPosition();
	while (pos != NULL)
	{
		CMapClass *pChild = Children.GetNext(pos);
		pChild->SetVisible(bVisible);
	}

	m_bVisible = bVisible;
}


//-----------------------------------------------------------------------------
// Purpose: Causes all objects in the world to update any object dependencies (pointers)
//			that they might be holding. This is a static function.
//-----------------------------------------------------------------------------
void CMapClass::UpdateAllDependencies(CMapClass *pObject)
{
	//
	// Try to locate the world object.
	//
	CMapWorld *pWorld;
	if (pObject == NULL)
	{
		CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
		if ((pDoc == NULL) || (pDoc->IsLoading()))
		{
			return;
		}
		
		pWorld = pDoc->GetMapWorld();
	}
	else
	{
		pWorld = pObject->GetWorldObject(pObject);
	}

	if (pWorld == NULL)
	{
		return;
	}

	//
	// We found the world. Tell all its children to update their dependencies
	// because of the given object.
	//
	EnumChildrenPos_t pos;
	CMapClass *pChild = pWorld->GetFirstDescendent(pos);
	while (pChild != NULL)
	{
		pChild->UpdateDependencies(pWorld, pObject);
		pChild = pWorld->GetNextDescendent(pos);
	}
}


