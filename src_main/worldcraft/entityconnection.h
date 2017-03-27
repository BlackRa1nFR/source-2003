//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines a connection (output-to-input) between two entities.
//
//			The behavior in-game is as follows:
//
//			When the given output in the source entity is triggered, the given
//			input in the target entity is called after a specified delay, and
//			the parameter override (if any) is passed to the input handler. If
//			there is no parameter override, the default parameter is passed.
//
//			This behavior will occur a specified number of times before the
//			connection between the two entities is removed.
//
// $NoKeywords: $
//=============================================================================

#ifndef ENTITYCONNECTION_H
#define ENTITYCONNECTION_H
#pragma once

#include "InputOutput.h"

#define EVENT_FIRE_ALWAYS	-1

enum SortDirection_t
{
	Sort_Ascending = 0,
	Sort_Descending,
};

enum
{
	CONNECTION_NONE,	// if entity list has no outputs
	CONNECTION_GOOD,	// if all entity outpus are good
	CONNECTION_BAD,	// if any entity output is bad
};

class CMapEntity;
class CMapEntityList;

class CEntityConnection
{
	public:
		
		inline CEntityConnection(void);

		inline bool CompareConnection(CEntityConnection *pConnection);

		inline float GetDelay(void) { return(m_fDelay); }
		inline void SetDelay(float fDelay) { m_fDelay = fDelay; }

		inline const char *GetInputName(void) { return(m_szInput); }
		inline void SetInputName(const char *pszName) { lstrcpyn(m_szInput, pszName, sizeof(m_szInput)); }

		inline const char *GetOutputName(void) { return(m_szOutput); }
		inline void SetOutputName(const char *pszName) { lstrcpyn(m_szOutput, pszName, sizeof(m_szOutput)); }

		inline const char *GetTargetName(void) { return(m_szTargetEntity); }
		inline void SetTargetName(const char *pszName) { lstrcpyn(m_szTargetEntity, pszName, sizeof(m_szSourceEntity)); }

		inline const char *GetSourceName(void) { return(m_szSourceEntity); }
		inline void SetSourceName(const char *pszName) { lstrcpyn(m_szSourceEntity, pszName, sizeof(m_szSourceEntity)); }

		inline int GetTimesToFire(void) { return(m_nTimesToFire); }
		inline void SetTimesToFire(int nTimesToFire) { m_nTimesToFire = nTimesToFire; }

		inline const char *GetParam(void) { return(m_szParam); }
		inline void SetParam(const char *pszParam) { lstrcpyn(m_szParam, pszParam, sizeof(m_szParam)); }

		inline CEntityConnection &operator =(CEntityConnection &Other);

		// Sorting functions
		static int CALLBACK CompareDelaysSecondary(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection);
		static int CALLBACK CompareDelays(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection);
		static int CALLBACK CompareOutputNames(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection);
		static int CALLBACK CompareInputNames(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection);
		static int CALLBACK CompareSourceNames(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection);
		static int CALLBACK CompareTargetNames(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection);

		// Validation functions
		static bool ValidateOutput(CMapEntity *pEntity, const char* pszOutput);
		static bool ValidateOutput(CMapEntityList *pEntityList, const char* pszOutput);
		static bool ValidateTarget(CMapEntityList *pEntityList, const char* pszTarget);
		static bool ValidateInput(const char* pszTarget, const char* pszInput);

		static int  ValidateOutputConnections(CMapEntity *pEntity);
		static int  ValidateInputConnections(CMapEntity *pEntity);


	protected:

		char m_szSourceEntity[MAX_ENTITY_NAME_LEN];		// Targetname of the source entity 
		char m_szOutput[MAX_IO_NAME_LEN];				// Name of the output in the source entity.
		char m_szTargetEntity[MAX_ENTITY_NAME_LEN];		// Targetname of the target entity.
		char m_szInput[MAX_IO_NAME_LEN];				// Name of the input to fire in the target entity.
		char m_szParam[MAX_IO_NAME_LEN];				// Parameter override, if any.

		float m_fDelay;									// Delay before firing this outout.
		int m_nTimesToFire;								// Maximum times to fire this output or EVENT_FIRE_ALWAYS.
};

typedef CTypedPtrList<CPtrList, CEntityConnection *> CEntityConnectionList;

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CEntityConnection::CEntityConnection(void)
{
	memset(m_szSourceEntity, 0, sizeof(m_szSourceEntity));
	memset(m_szTargetEntity, 0, sizeof(m_szTargetEntity));
	memset(m_szOutput, 0, sizeof(m_szOutput));
	memset(m_szInput, 0, sizeof(m_szInput));
	memset(m_szParam, 0, sizeof(m_szParam));

	m_fDelay = 0;
	m_nTimesToFire = EVENT_FIRE_ALWAYS;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the given connection is identical to this connection.
// Input  : pConnection - Connection to compare.
//-----------------------------------------------------------------------------
bool CEntityConnection::CompareConnection(CEntityConnection *pConnection)
{
	return((!stricmp(GetOutputName(), pConnection->GetOutputName())) &&
		   (!stricmp(GetTargetName(), pConnection->GetTargetName())) &&
		   (!stricmp(GetInputName(), pConnection->GetInputName())) &&
		   (!stricmp(GetParam(), pConnection->GetParam())) &&
		   (GetDelay() == pConnection->GetDelay()) &&
		   (GetTimesToFire() == pConnection->GetTimesToFire()));
}


//-----------------------------------------------------------------------------
// Purpose: Copy operator. Makes 'this' identical to 'Other'.
//-----------------------------------------------------------------------------
CEntityConnection &CEntityConnection::operator =(CEntityConnection &Other)
{
	strcpy(m_szSourceEntity, Other.m_szSourceEntity);
	strcpy(m_szTargetEntity, Other.m_szTargetEntity);
	strcpy(m_szOutput, Other.m_szOutput);
	strcpy(m_szInput, Other.m_szInput);
	strcpy(m_szParam, Other.m_szParam);
	m_fDelay = Other.m_fDelay;
	m_nTimesToFire = Other.m_nTimesToFire;
	
	return(*this);
}

#endif // ENTITYCONNECTION_H





