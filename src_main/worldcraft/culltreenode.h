//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include "BoundBox.h"
#include "MapClass.h"


class CCullTreeNode;


typedef CTypedPtrArray<CPtrArray, CCullTreeNode *> CullTreeNodeArray;
typedef CTypedPtrArray<CPtrArray, CMapClass *> MapClassArray;


class CCullTreeNode : public BoundBox
{
	public:

		CCullTreeNode(void);
		~CCullTreeNode(void);

		//
		// Children.
		//
		inline int GetChildCount(void) { return(m_nChildCount); }
		inline CCullTreeNode *GetCullTreeChild(int nChild) { ASSERT(nChild < m_nChildCount); return(m_Children.GetAt(nChild)); }

		void AddCullTreeChild(CCullTreeNode *pChild);
		void RemoveCullTreeChild(CCullTreeNode *pChild);

		//
		// Objects.
		//
		inline int GetObjectCount(void) { return(m_nObjectCount); }
		inline CMapClass *GetCullTreeObject(int nObject) { ASSERT(nObject < m_nObjectCount); return(m_Objects.GetAt(nObject)); }

		void AddCullTreeObject(CMapClass *pObject);
		void AddCullTreeObjectRecurse(CMapClass *pObject);

		CCullTreeNode *FindCullTreeObjectRecurse(CMapClass *pObject);

		void RemoveAllCullTreeObjects(void);
		void RemoveAllCullTreeObjectsRecurse(void);

		void RemoveCullTreeObject(CMapClass *pObject);
		void RemoveCullTreeObjectRecurse(CMapClass *pObject);

		void UpdateAllCullTreeObjects(void);
		void UpdateAllCullTreeObjectsRecurse(void);

		void UpdateCullTreeObject(CMapClass *pObject);
		void UpdateCullTreeObjectRecurse(CMapClass *pObject);

	protected:

		CullTreeNodeArray m_Children;	// The child nodes. This is an octree.
		int m_nChildCount;

		MapClassArray m_Objects;		// The objects contained in this node.
		int m_nObjectCount;
};

