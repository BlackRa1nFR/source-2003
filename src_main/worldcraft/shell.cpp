//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Handles parsing and routing of shell commands to their handlers.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "MainFrm.h"
#include "MapDoc.h"
#include "MapEntity.h"
#include "Shell.h"
#include "worldcraft.h"

//-----------------------------------------------------------------------------
// Shell command handler function pointer.
//-----------------------------------------------------------------------------
typedef bool (CShell::*ShellHandlerFunc_t)(const char *pszCommand, const char *pszArguments);


//-----------------------------------------------------------------------------
// Dispatch table entry.
//-----------------------------------------------------------------------------
struct ShellDispatchTable_t
{
	const char *pszCommand;			// Name of command associated with this entry.
	ShellHandlerFunc_t pfnHandler;	// Function handler for the command.
};


//-----------------------------------------------------------------------------
// Dispatch table for shell commands.
//-----------------------------------------------------------------------------
ShellDispatchTable_t CShell::m_DispatchTable[] =
{
	{ "session_begin", CShell::BeginSession },
	{ "session_end", CShell::EndSession },
	{ "entity_create", CShell::EntityCreate },
	{ "entity_delete", CShell::EntityDelete },
	{ "map_check_version", CShell::CheckMapVersion },
	{ "node_create", CShell::NodeCreate },
	{ "node_delete", CShell::NodeDelete },
	{ "nodelink_create", CShell::NodeLinkCreate },
	{ "nodelink_delete", CShell::NodeLinkDelete },
	{ "release_video_memory", CShell::ReleaseVideoMemory },
	{ "grab_video_memory", CShell::GrabVideoMemory },
};


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CShell::CShell(void)
{
	m_pDoc = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CShell::~CShell(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: Initiates a shell editing session.
// Input  : pszCommand - Should be "session_begin".
//			pszArguments - Filename and file version in the engine.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::BeginSession(const char *pszCommand, const char *pszArguments)
{
	if ((m_pDoc != NULL) && !m_pDoc->IsShellSessionActive())
	{
		if (DoVersionCheck(pszArguments))
		{
			m_pDoc->BeginShellSession();
			return(true);
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Verifies that the map begine edited in the engine is the same name
//			and version as the active document. This prevents problems with
//			editing out of sync versions of the map via the engine.
// Input  : pszCommand - Should be "map_check_version".
//			pszArguments - Filename and file version in the engine.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::CheckMapVersion(const char *pszCommand, const char *pszArguments)
{
	if ((m_pDoc != NULL) && m_pDoc->IsShellSessionActive())
	{
		return(DoVersionCheck(pszArguments));
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Verifies that the map being edited in the engine is the same name
//			and version as the active document. This prevents problems with
//			editing out of sync versions of the map via the engine.
// Input  : pszCommand - 
//			pszArguments - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::DoVersionCheck(const char *pszArguments)
{
	if (m_pDoc != NULL)
	{
		char szEngineMapPath[MAX_PATH];
		int nEngineMapVersion;

		if (sscanf(pszArguments, "%s %d", szEngineMapPath, &nEngineMapVersion) == 2)
		{
			char szEngineMapName[MAX_PATH];
			_splitpath(szEngineMapPath, NULL, NULL, szEngineMapName, NULL);

			char szDocName[MAX_PATH];
			_splitpath(m_pDoc->GetPathName(), NULL, NULL, szDocName, NULL);

			int nDocVersion = m_pDoc->GetDocVersion();

			if (!stricmp(szDocName, szEngineMapName) && (nDocVersion == nEngineMapVersion))
			{
				return(true);
			}
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Verifies that the map begine edited in the engine is the same name
//			and version as the active document. This prevents problems with
//			editing out of sync versions of the map via the engine.
// Input  : pszCommand - Should be "session_end".
//			pszArguments - Filename and file version in the engine.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::EndSession(const char *pszCommand, const char *pszArguments)
{
	if ((m_pDoc != NULL) && m_pDoc->IsShellSessionActive())
	{
		m_pDoc->EndShellSession();
		return(true);
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Creates an entity of a given class at a specified location.
// Input  : pszCommand - Should be "entity_create".
//			pszArguments - Class name of entity and x, y, z coordinate at which
//				to create it.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::EntityCreate(const char *pszCommand, const char *pszArguments)
{
	if ((m_pDoc != NULL) && m_pDoc->IsShellSessionActive())
	{
		float x;
		float y;
		float z;
		char szClassName[MAX_PATH];

		if (sscanf(pszArguments, "%s %f %f %f", szClassName, &x, &y, &z) == 4)
		{
			bool bCreated = (m_pDoc->CreateEntity(szClassName, x, y, z) != NULL);
			return(bCreated);
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Deletes an entity by class name and origin.
// Input  : pszCommand - Should be "entity_delete".
//			pszArguments - Class name of entity and x, y, z coordinates.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::EntityDelete(const char *pszCommand, const char *pszArguments)
{
	if ((m_pDoc != NULL) && m_pDoc->IsShellSessionActive())
	{
		float x;
		float y;
		float z;
		char szClassName[MAX_PATH];

		if (sscanf(pszArguments, "%s %f %f %f", szClassName, &x, &y, &z) == 4)
		{
			bool bDeleted = m_pDoc->DeleteEntity(szClassName, x, y, z);
			return(bDeleted);
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Creates a navigation node of a given class at a specified location.
// Input  : pszCommand - Should be "node_create".
//			pszArguments - Class name of node to create, ID to assign it, and
//			x, y, z coordinate at which to create the node.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::NodeCreate(const char *pszCommand, const char *pszArguments)
{
	if ((m_pDoc != NULL) && m_pDoc->IsShellSessionActive())
	{
		float x;
		float y;
		float z;
		int nID;
		char szClassName[MAX_PATH];

		if (sscanf(pszArguments, "%s %d %f %f %f", szClassName, &nID, &x, &y, &z) == 5)
		{
			m_pDoc->SetNextNodeID(nID);
			CMapEntity *pEntity = m_pDoc->CreateEntity(szClassName, x, y, z);

			return(true);
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Deletes a navigation node by ID.
// Input  : pszCommand - Should be "node_delete".
//			pszArguments - Unique node ID of node to delete.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::NodeDelete(const char *pszCommand, const char *pszArguments)
{
	bool bFound = false;

	if ((m_pDoc != NULL) && m_pDoc->IsShellSessionActive())
	{
		char szID[80];
		if (sscanf(pszArguments, "%s", szID) == 1)
		{
			CMapEntityList Found;
			if (m_pDoc->FindEntitiesByKeyValue(&Found, "nodeid", szID))
			{
				POSITION pos = Found.GetHeadPosition();
				while (pos != NULL)
				{
					CMapEntity *pEntity = Found.GetNext(pos);
					m_pDoc->DeleteObject(pEntity);
					bFound = true;
				}
			}
		}
	}

	return(bFound);
}


//-----------------------------------------------------------------------------
// Purpose: Creates a navigation node of a given class at a specified location.
// Input  : pszCommand - Should be "nodelink_create".
//			pszArguments - Node ids of start and end nodes, space delimited.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::NodeLinkCreate(const char *pszCommand, const char *pszArguments)
{
	if ((m_pDoc != NULL) && m_pDoc->IsShellSessionActive())
	{
		char szIDStart[80];
		char szIDEnd[80];

		if (sscanf(pszArguments, "%s %s", szIDStart, szIDEnd) == 2)
		{
			//
			// It doesn't matter where we place it because it will move to the midpoint of the
			// start and end entities.
			//
			CMapEntity *pEntity = m_pDoc->CreateEntity("info_node_link", 0, 0, 0);
			if (pEntity != NULL)
			{
				pEntity->SetKeyValue("startnode", szIDStart);
				pEntity->SetKeyValue("endnode", szIDEnd);

				return(true);
			}
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Deletes a navigation node by class name and ID.
// Input  : pszCommand - Should be "node_delete".
//			pszArguments - Class name of node and unique node ID.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::NodeLinkDelete(const char *pszCommand, const char *pszArguments)
{
	bool bFound = false;

	if ((m_pDoc != NULL) && m_pDoc->IsShellSessionActive())
	{
		char szIDStart[80];
		char szIDEnd[80];

		if (sscanf(pszArguments, "%s %s", szIDStart, szIDEnd) == 2)
		{
			//
			// Look for info_node_link entities with the appropriate start/end keys.
			//
			CMapEntityList Found;
			if (m_pDoc->FindEntitiesByClassName(&Found, "info_node_link"))
			{
				POSITION pos = Found.GetHeadPosition();
				while (pos != NULL)
				{
					CMapEntity *pEntity = Found.GetNext(pos);

					const char *pszNode1 = pEntity->GetKeyValue("startnode");
					const char *pszNode2 = pEntity->GetKeyValue("endnode");
					if ((pszNode1 != NULL) && (pszNode2 != NULL))
					{
						if (((!stricmp(pszNode1, szIDStart)) && (!stricmp(pszNode2, szIDEnd))) ||
							((!stricmp(pszNode1, szIDEnd)) && (!stricmp(pszNode2, szIDStart))))
						{
							m_pDoc->DeleteObject(pEntity);
							bFound = true;
						}
					}
				}
			}
		}
	}

	return(bFound);
}


//-----------------------------------------------------------------------------
// Purpose: Releases all video memory 
// Input  : pszCommand - Should be "release_video_memory".
//			pszArguments - None.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::ReleaseVideoMemory(const char *pszCommand, const char *pszArguments)
{
	APP()->ReleaseVideoMemory();
	APP()->SuppressVideoAllocation(true);
	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Indicates it's safe to grab video memory
// Input  : pszCommand - Should be "grab_video_memory".
//			pszArguments - None.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::GrabVideoMemory(const char *pszCommand, const char *pszArguments)
{
	APP()->SuppressVideoAllocation(false);
	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to fund a command in the dispatch table, then routes the
//			command and its arguments to the handler, if found.
// Input  : pszCommand - Command and arguments.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShell::RunCommand(const char *pszCommand)
{
	for (int nCommand = 0; nCommand < sizeof(m_DispatchTable) / sizeof(m_DispatchTable[0]); nCommand++)
	{
		int nCommandLen = strlen(m_DispatchTable[nCommand].pszCommand);

		if (!_strnicmp(pszCommand, m_DispatchTable[nCommand].pszCommand, nCommandLen))
		{
			return((this->*m_DispatchTable[nCommand].pfnHandler)(m_DispatchTable[nCommand].pszCommand, &pszCommand[nCommandLen]));
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the map document that this shell should operate on.
// Input  : pDoc - Pointer to document.
//-----------------------------------------------------------------------------
void CShell::SetDocument(CMapDoc *pDoc)
{
	m_pDoc = pDoc;
}

