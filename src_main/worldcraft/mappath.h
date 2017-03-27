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

#include <afxtempl.h>
#include <fstream.h>
#include "Axes2.h"
#include "KeyValues.h"
#include "vector.h"

class BoundBox;
class CMapEntity;
class Path3D;


class CMapPathNode
{
	public:

		CMapPathNode();

		char szName[128];	// if blank, use default

		Vector pos;
		DWORD dwID;
		BOOL bSelected;

		char szTargets[2][128];	// resolved when saving to map - not used otherwise
		int nTargets;

		// other values
		KeyValues kv;

		CMapPathNode& operator=(const CMapPathNode& src);
};


class CMapPath
{
	friend Path3D;

	public:

		CMapPath();
		~CMapPath();

		enum
		{
			ADD_START	= 0xfffffff0L,
			ADD_END		= 0xfffffff1L
		};

		DWORD AddNode(DWORD dwAfterID, const Vector &vecPos);
		void DeleteNode(DWORD dwID);
		void SetNodePosition(DWORD dwID, Vector& pt);
		CMapPathNode * NodeForID(DWORD dwID, int* piIndex = NULL);
		void GetNodeName(int iIndex, int iName, CString& str);

		// set name/class
		void SetName(LPCTSTR pszName) { strcpy(m_szName, pszName); }
		LPCTSTR GetName() { return m_szName; }
		void SetClass(LPCTSTR pszClass) { strcpy(m_szClass, pszClass); }
		LPCTSTR GetClass() { return m_szClass; }

		void EditInfo();

		// save/load to/from RMF:
		void SerializeRMF(fstream&, BOOL fIsStoring);
		// save to map: (no load!!)
		void SerializeMAP(fstream&, BOOL fIsStoring, BoundBox *pIntersecting = NULL);

		//void SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo);
		//void LoadVMF(CChunkFile *pFile);

		CMapEntity *CreateEntityForNode(DWORD dwNodeID);
		void CopyNodeFromEntity(DWORD dwNodeID, CMapEntity *pEntity);

		// directions
		enum
		{
			dirOneway,
			dirCircular,
			dirPingpong
		};

		int GetNodeCount() { return m_nNodes; }

	private:

		// nodes + number of:
		CArray<CMapPathNode, CMapPathNode&> m_Nodes;
		int m_nNodes;

		DWORD GetNewNodeID();

		// name:
		char m_szName[128];
		char m_szClass[128];
		int m_iDirection;
};


typedef CTypedPtrList<CPtrList, CMapPath*> CMapPathList;

