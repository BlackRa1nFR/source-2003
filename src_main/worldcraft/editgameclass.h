//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef EDITGAMECLASS_H
#define EDITGAMECLASS_H
#pragma once

#include <fstream.h>
#include "BlockArray.h"
#include "EntityDefs.h"
#include "GameData.h"
#include "GDClass.h"
#include "KeyValues.h"
#include "EntityConnection.h"


#define MAX_CLASS_NAME_LEN		64


class CChunkFile;
class CMapClass;
class CSaveInfo;


enum ChunkFileResult_t;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEditGameClass
{
	public:

		CEditGameClass(void);
		~CEditGameClass(void);

		inline bool IsClass(const char *pszClass = NULL);
		inline GDclass *GetClass(void) { return(m_pClass); }
		inline void SetClass(GDclass *pClass) { m_pClass = pClass; }
		inline LPCSTR GetClassName(void) { return(m_szClass); }
		inline bool IsKeyFrameClass(void) { return((m_pClass != NULL) && (m_pClass->IsKeyFrameClass())); }
		inline bool IsMoveClass(void) { return((m_pClass != NULL) && (m_pClass->IsMoveClass())); }
		inline bool IsPointClass(void) { return((m_pClass != NULL) && (m_pClass->IsPointClass())); }
		inline bool IsNPCClass(void) { return((m_pClass != NULL) && (m_pClass->IsNPCClass())); }
		inline bool IsFilterClass(void) { return((m_pClass != NULL) && (m_pClass->IsFilterClass())); }
		inline bool IsSolidClass(void) { return((m_pClass != NULL) && (m_pClass->IsSolidClass())); }
		inline bool IsNodeClass(void) { return((m_pClass != NULL) && (m_pClass->IsNodeClass())); }
		static inline bool IsNodeClass(const char *pszClassName) { return GDclass::IsNodeClass(pszClassName); }

		//
		// Interface to key/value information:
		//
		virtual void RemoveKey(int nIndex) { m_KeyValues.RemoveKey(nIndex); }
		virtual void SetKeyValue(LPCTSTR pszKey, LPCTSTR pszValue) { m_KeyValues.SetValue(pszKey, pszValue); }
		virtual void SetKeyValue(LPCTSTR pszKey, int iValue) { m_KeyValues.SetValue(pszKey, iValue); }
		virtual void DeleteKeyValue(LPCTSTR pszKey) { m_KeyValues.RemoveKey(pszKey); }
		virtual LPCTSTR GetKey(int nIndex) { return(m_KeyValues.GetKey(nIndex)); }
		virtual LPCTSTR GetKeyValue(int nIndex) { return(m_KeyValues.GetValue(nIndex)); }
		virtual LPCTSTR GetKeyValue(LPCTSTR pszKey, int *piIndex = NULL) { return(m_KeyValues.GetValue(pszKey, piIndex)); }
		virtual int GetKeyValueCount(void) { return(m_KeyValues.GetCount()); }

		//
		// Interface to spawnflags.
		//
		bool GetSpawnFlag(int nFlag);
		int GetSpawnFlags(void);
		void SetSpawnFlag(int nFlag, bool bSet);
		void SetSpawnFlags(int nFlags);

		//
		// Interface to entity connections.
		//
		void Connections_Add(CEntityConnection *pConnection);
		inline int Connections_GetCount(void);
		inline POSITION Connections_GetHeadPosition(void);
		inline CEntityConnection *Connections_GetNext(POSITION &pos);
		bool Connections_Remove(CEntityConnection *pConnection);
		void Connections_RemoveAll(void);

		//
		// Interface to comments.
		//
		inline const char *GetComments(void);
		inline void SetComments(const char *pszComments);

		//
		// Serialization functions.
		//
		static ChunkFileResult_t LoadConnectionsCallback(CChunkFile *pFile, CEditGameClass *pEditGameClass);
		static ChunkFileResult_t LoadKeyCallback(const char *szKey, const char *szValue, CEditGameClass *pEditGameClass);

		ChunkFileResult_t SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo);

		int SerializeRMF(fstream&, BOOL);
		int SerializeMAP(fstream&, BOOL);

		virtual void SetClass(LPCTSTR pszClassname, bool bLoading = false);
		CEditGameClass *CopyFrom(CEditGameClass *pFrom);
		void GetDefaultKeys( void );

		virtual void SetAngles(const QAngle &vecAngles);
		virtual void GetAngles(QAngle &vecAngles);

		// Import the old-style yaw only representation of orientation.
		void ImportAngle(int nAngle);

	protected:

		KeyValues m_KeyValues;
		GDclass *m_pClass;
		char m_szClass[MAX_CLASS_NAME_LEN];
		char *m_pszComments;		// Comments text, dynamically allocated.

		static char *g_pszEmpty;

		CEntityConnectionList m_Connections;
};


//-----------------------------------------------------------------------------
// Purpose: Returns the number of input/output connections that this object has.
//-----------------------------------------------------------------------------
int CEditGameClass::Connections_GetCount(void)
{
	return(m_Connections.GetCount());
}


//-----------------------------------------------------------------------------
// Purpose: Returns an iterator for this object's list of input/output connections.
//-----------------------------------------------------------------------------
POSITION CEditGameClass::Connections_GetHeadPosition(void)
{
	return(m_Connections.GetHeadPosition());
}


//-----------------------------------------------------------------------------
// Purpose: Returns the next connection in this object's list of input/output connections.
// Input  : pos - Iterator.
//-----------------------------------------------------------------------------
CEntityConnection *CEditGameClass::Connections_GetNext(POSITION &pos)
{
	return(m_Connections.GetNext(pos));
}


//-----------------------------------------------------------------------------
// Purpose: Returns the comments text, NULL if none have been set.
//-----------------------------------------------------------------------------
const char *CEditGameClass::GetComments(void)
{
	if (m_pszComments == NULL)
	{
		return(g_pszEmpty);
	}

	return(m_pszComments);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : NULL - 
// Output : inline bool
//-----------------------------------------------------------------------------
inline bool CEditGameClass::IsClass(const char *pszClass)
{
	if (pszClass == NULL)
	{
		return(m_pClass != NULL);
	}
	return((m_pClass != NULL) && (!stricmp(pszClass, m_szClass)));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszComments - 
//-----------------------------------------------------------------------------
void CEditGameClass::SetComments(const char *pszComments)
{
	delete m_pszComments;

	if (pszComments != NULL)
	{
		int nLen = strlen(pszComments);
		if (nLen == 0)
		{
			m_pszComments = NULL;
		}
		else
		{
			m_pszComments = new char [nLen + 1];
			strcpy(m_pszComments, pszComments);
		}
	}
	else
	{
		m_pszComments = NULL;
	}
}


#endif // EDITGAMECLASS_H
