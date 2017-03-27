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

#include "stdafx.h"
#include "EntityConnection.h"
#include "MapEntity.h"
#include "MapDoc.h"
#include "MapWorld.h"

//------------------------------------------------------------------------------
// Purpose : Returns true if output string is valid for all the entities in 
//			 the entity list
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CEntityConnection::ValidateOutput(CMapEntity *pEntity, const char* pszOutput)
{
	if (!pEntity)
	{
		return false;
	}

	GDclass*	pClass	= pEntity->GetClass();
	if (pClass != NULL)
	{
		if (pClass->FindOutput(pszOutput) == NULL)
		{
			return false;
		}
	}
	return true;
}

//------------------------------------------------------------------------------
// Purpose : Returns true if output string is valid for all the entities in 
//			 the entity list
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CEntityConnection::ValidateOutput(CMapEntityList *pEntityList, const char* pszOutput)
{
	if (!pEntityList)
	{
		return false;
	}

	POSITION pos = pEntityList->GetHeadPosition();
	while (pos != NULL)
	{
		CMapEntity*	pEntity = pEntityList->GetNext(pos);
		if (!ValidateOutput(pEntity,pszOutput))
		{
			return false;
		}
	}
	return true;
}

//------------------------------------------------------------------------------
// Purpose : Returns true if the given entity list contains an entity of the
//			 given target name
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CEntityConnection::ValidateTarget(CMapEntityList *pEntityList, const char* pszTarget)
{
	if (!pEntityList || !pszTarget)
	{
		return false;
	}

	POSITION pos = pEntityList->GetHeadPosition();
	while (pos != NULL)
	{
		CMapEntity*	pEntity = pEntityList->GetNext(pos);
		const char* pszTargetName = pEntity->GetKeyValue( "targetname" );
		if (pszTargetName && !stricmp(pszTarget,pszTargetName))
		{
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
// Purpose : Returns true if all entities withe the given target name
//			 have an input of the given input name
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CEntityConnection::ValidateInput(const char* pszTarget, const char* pszInput)
{
	CMapDoc *pDoc = CMapDoc::GetActiveMapDoc();
	CMapEntityList pEntityList;
	pDoc->FindEntitiesByKeyValue(&pEntityList, "targetname", pszTarget);

	if (pEntityList.GetCount() == 0)
	{
		return false;
	}

	if (!pEntityList.HasInput(pszInput))
	{
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
// Purpose : Check if all the output connections in the given entity list
//			 are valid
// Input   :
// Output  :		OUTPUTS_NONE,	// if entity list has no outputs
//					OUTPUTS_GOOD,	// if all entity outpus are good
//					OUTPUTS_BAD,	// if any entity output is bad
//------------------------------------------------------------------------------
int CEntityConnection::ValidateOutputConnections(CMapEntity *pEntity)
{
	if (!pEntity)
	{
		return CONNECTION_NONE;
	}

	// Get a list of all the entities in the world
	CMapEntityList	*pAllWorldEntities = NULL;
	CMapDoc			*pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc)
	{
		CMapWorld *pWorld = pDoc->GetMapWorld();
		if (pWorld)
		{
			pAllWorldEntities = pWorld->EntityList_GetList();
		}
	}

	if (pEntity->Connections_GetCount() == 0)
	{
		return CONNECTION_NONE;
	}

	// For each connection
	POSITION pos = pEntity->Connections_GetHeadPosition();
	while (pos != NULL)
	{
		CEntityConnection *pConnection = pEntity->Connections_GetNext(pos);
		if (pConnection != NULL)
		{
			// Check validity of output for the list of entities
			if (!CEntityConnection::ValidateOutput(pEntity,pConnection->GetOutputName()))
			{
				return CONNECTION_BAD;
			}

			// Check validity of target entity (is it in the map?)
			if (!CEntityConnection::ValidateTarget(pAllWorldEntities, pConnection->GetTargetName()))
			{
				return CONNECTION_BAD;
			}

			// Check validity of input
			if (!CEntityConnection::ValidateInput(pConnection->GetTargetName(),pConnection->GetInputName()))
			{
				return CONNECTION_BAD;
			}
		}
	}
	return CONNECTION_GOOD;	
}

//------------------------------------------------------------------------------
// Purpose : Check if all the output connections in the given entity list
//			 are valid
// Input   :
// Output  :		INPUTS_NONE,	// if entity list has no inputs
//					INPUTS_GOOD,	// if all entity inputs are good
//					INPUTS_BAD,		// if any entity input is bad
//------------------------------------------------------------------------------
int CEntityConnection::ValidateInputConnections(CMapEntity *pEntity)
{
	if (!pEntity)
	{
		return CONNECTION_NONE;
	}

	// No inputs if entity doesn't have a target name
	const char *pszTargetName = pEntity->GetKeyValue("targetname");
	if (!pszTargetName)
	{
		return CONNECTION_NONE;
	}

	GDclass *pClass = pEntity->GetClass();
	if (!pClass)
	{
		return CONNECTION_NONE;
	}

	// Get a list of all the entities in the world
	CMapEntityList	*pAllWorldEntities = NULL;
	CMapDoc			*pDoc = CMapDoc::GetActiveMapDoc();
	if (pDoc)
	{
		CMapWorld *pWorld = pDoc->GetMapWorld();
		if (pWorld)
		{
			pAllWorldEntities = pWorld->EntityList_GetList();
		}
	}

	// Look at outputs from each entity in the world
	POSITION pos = pAllWorldEntities->GetHeadPosition();
	bool bHaveConnection = false;
	while (pos != NULL)
	{
		CMapEntity *pTestEntity = pAllWorldEntities->GetNext(pos);
		if (pTestEntity != NULL)
		{
			int nNumConnections = pTestEntity->Connections_GetCount();
			if (nNumConnections != 0)
			{
				POSITION pos = pTestEntity->Connections_GetHeadPosition();
				while (pos != NULL)
				{
					// If the connection targets me
					CEntityConnection *pConnection = pTestEntity->Connections_GetNext(pos);
					if (pConnection != NULL && 
						!stricmp(pConnection->GetTargetName(),pszTargetName) )
					{
						// Validate output
						if (!ValidateOutput(pTestEntity,pConnection->GetOutputName()))
						{
							return CONNECTION_BAD;
						}

						// Validate input
						if (pClass->FindInput(pConnection->GetInputName()) == NULL)
						{
							return CONNECTION_BAD;
						}

						bHaveConnection = true;
					}
				}
			}
		}
	}
	if (bHaveConnection)
	{
		return CONNECTION_GOOD;
	}
	return CONNECTION_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Compares by delays. Used as a secondary sort by all other columns.
//-----------------------------------------------------------------------------
int CALLBACK CEntityConnection::CompareDelaysSecondary(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection)
{
	float fDelay1;
	float fDelay2;

	if (eDirection == Sort_Ascending)
	{
		fDelay1 = pConn1->GetDelay();
		fDelay2 = pConn2->GetDelay();
	}
	else
	{
		fDelay1 = pConn2->GetDelay();
		fDelay2 = pConn1->GetDelay();
	}

	if (fDelay1 < fDelay2)
	{
		return(-1);
	}
	else if (fDelay1 > fDelay2)
	{
		return(1);
	}

	return(0);
}

//-----------------------------------------------------------------------------
// Purpose: Compares by delays, does a secondary compare by output name.
//-----------------------------------------------------------------------------
int CALLBACK CEntityConnection::CompareDelays(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection)
{
	int nReturn = CompareDelaysSecondary(pConn1, pConn2, eDirection);
	if (nReturn != 0)
	{
		return(nReturn);
	}

	//
	// Always do a secondary sort by output name.
	//
	return(CompareOutputNames(pConn1, pConn2, Sort_Ascending));
}


//-----------------------------------------------------------------------------
// Purpose: Compares by output name, does a secondary compare by delay.
//-----------------------------------------------------------------------------
int CALLBACK CEntityConnection::CompareOutputNames(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection)
{
	int nReturn = 0;

	if (eDirection == Sort_Ascending)
	{
		nReturn = stricmp(pConn1->GetOutputName(), pConn2->GetOutputName());
	}
	else
	{
		nReturn = stricmp(pConn2->GetOutputName(), pConn1->GetOutputName());
	}

	//
	// Always do a secondary sort by delay.
	//
	if (nReturn == 0)
	{
		nReturn = CompareDelaysSecondary(pConn1, pConn2, Sort_Ascending);
	}

	return(nReturn);
}


//-----------------------------------------------------------------------------
// Purpose: Compares by input name, does a secondary compare by delay.
//-----------------------------------------------------------------------------
int CALLBACK CEntityConnection::CompareInputNames(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection)
{
	int nReturn = 0;

	if (eDirection == Sort_Ascending)
	{
		nReturn = stricmp(pConn1->GetInputName(), pConn2->GetInputName());
	}
	else
	{
		nReturn = stricmp(pConn2->GetInputName(), pConn1->GetInputName());
	}

	//
	// Always do a secondary sort by delay.
	//
	if (nReturn == 0)
	{
		nReturn = CompareDelaysSecondary(pConn1, pConn2, Sort_Ascending);
	}

	return(nReturn);
}

//-----------------------------------------------------------------------------
// Purpose: Compares by source name, does a secondary compare by delay.
//-----------------------------------------------------------------------------
int CALLBACK CEntityConnection::CompareSourceNames(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection)
{
	int nReturn = 0;

	if (eDirection == Sort_Ascending)
	{
		nReturn = stricmp(pConn1->GetSourceName(), pConn2->GetSourceName());
	}
	else
	{
		nReturn = stricmp(pConn2->GetSourceName(), pConn1->GetSourceName());
	}

	//
	// Always do a secondary sort by delay.
	//
	if (nReturn == 0)
	{
		nReturn = CompareDelaysSecondary(pConn1, pConn2, Sort_Ascending);
	}

	return(nReturn);
}

//-----------------------------------------------------------------------------
// Purpose: Compares by target name, does a secondary compare by delay.
//-----------------------------------------------------------------------------
int CALLBACK CEntityConnection::CompareTargetNames(CEntityConnection *pConn1, CEntityConnection *pConn2, SortDirection_t eDirection)
{
	int nReturn = 0;

	if (eDirection == Sort_Ascending)
	{
		nReturn = stricmp(pConn1->GetTargetName(), pConn2->GetTargetName());
	}
	else
	{
		nReturn = stricmp(pConn2->GetTargetName(), pConn1->GetTargetName());
	}

	//
	// Always do a secondary sort by delay.
	//
	if (nReturn == 0)
	{
		nReturn = CompareDelaysSecondary(pConn1, pConn2, Sort_Ascending);
	}

	return(nReturn);
}







