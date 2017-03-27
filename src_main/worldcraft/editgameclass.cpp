//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements a class that encapsulates much of the functionality
//			of entities. CMapWorld and CMapEntity are both derived from this
//			class.
//
//			CEditGameClass-derived objects have the following properties:
//
//			Key/value pairs - A list of string pairs that hold data properties
//				of the object. Each property has a unique name.
//
//			Connections - A list of outputs in this object that are connected to
//				inputs in another entity.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "ChunkFile.h"
#include "GameData.h"
#include "EditGameClass.h"
#include "Mathlib.h"


//
// An empty string returned by GetComments when we have no comments set.
//
char *CEditGameClass::g_pszEmpty = "";


//-----------------------------------------------------------------------------
// Purpose: Constructor. Initializes data members.
//-----------------------------------------------------------------------------
CEditGameClass::CEditGameClass(void)
{
	m_pClass = NULL;
	m_szClass[0] = '\0';
	m_pszComments = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Frees memory.
//-----------------------------------------------------------------------------
CEditGameClass::~CEditGameClass(void)
{
	delete m_pszComments;

	Connections_RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pConnection - 
//-----------------------------------------------------------------------------
void CEditGameClass::Connections_Add(CEntityConnection *pConnection)
{
	m_Connections.AddHead(pConnection);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pConnection - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CEditGameClass::Connections_Remove(CEntityConnection *pConnection)
{
	POSITION pos = m_Connections.Find(pConnection);
	if (pos != NULL)
	{
		m_Connections.RemoveAt(pos);
		delete pConnection;
		return(true);
	}

	return(false);
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditGameClass::Connections_RemoveAll(void)
{
	POSITION pos = m_Connections.GetHeadPosition();
	while (pos != NULL)
	{
		CEntityConnection *pConn = m_Connections.GetNext(pos);
		delete pConn;
	}
	m_Connections.RemoveAll();
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszClass - 
//			bLoading - 
//-----------------------------------------------------------------------------
void CEditGameClass::SetClass(LPCTSTR pszClass, bool bLoading)
{
	extern GameData *pGD;
	strcpy(m_szClass, pszClass);
	int iLen = strlen(m_szClass) - 1;

	while (m_szClass[iLen] == ' ')
	{
		m_szClass[iLen--] = '\0';
	}

	if (pGD)
	{
		m_pClass = pGD->ClassForName(m_szClass);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Copies the data from a given CEditGameClass object into this one.
// Input  : pFrom - Object to copy.
// Output : Returns a pointer to this.
//-----------------------------------------------------------------------------
CEditGameClass *CEditGameClass::CopyFrom(CEditGameClass *pFrom)
{
	m_pClass = pFrom->m_pClass;
	strcpy( m_szClass, pFrom->m_szClass );

	//
	// Copy all the keys.
	//
	m_KeyValues.RemoveAll();
	int nKeyValues = pFrom->GetKeyValueCount();
	for (int i = 0; i < nKeyValues; i++)
	{
		m_KeyValues.SetValue(pFrom->GetKey(i), pFrom->GetKeyValue(i));
	}
	ASSERT(m_KeyValues.GetCount() == nKeyValues);

	//
	// Copy all the connections.
	//
	Connections_RemoveAll();
	POSITION pos = pFrom->Connections_GetHeadPosition();
	while (pos != NULL)
	{
		CEntityConnection *pConn = pFrom->Connections_GetNext(pos);
		CEntityConnection *pNewConn = new CEntityConnection;
		*pNewConn = *pConn;
		m_Connections.AddTail(pNewConn);
	}

	//
	// Copy the comments.
	//
	SetComments(pFrom->GetComments());

	return(this);
}


//-----------------------------------------------------------------------------
// Purpose: Applies the default keys for this object's game class. Called when
//			the entity's class is changed.
//-----------------------------------------------------------------------------
void CEditGameClass::GetDefaultKeys( void )
{
	if ( m_pClass != NULL )
	{
		//
		// For each variable from the base class...
		//
		int nVariableCount = m_pClass->GetVariableCount();
		for ( int i = 0; i < nVariableCount; i++ )
		{
			GDinputvariable *pVar = m_pClass->GetVariableAt(i);
			ASSERT(pVar != NULL);

			if (pVar != NULL)
			{
				int iIndex;
				LPCTSTR p = m_KeyValues.GetValue(pVar->GetName(), &iIndex);

				//
				// If the variable is not present in this object, set the default value.
				//
				if (p == NULL) 
				{
					MDkeyvalue tmpkv;
					pVar->ResetDefaults();
					pVar->ToKeyValue(&tmpkv);

					//
					// Only set the key value if it is non-zero.
					//
					if ((tmpkv.szKey[0] != 0) && (tmpkv.szValue[0] != 0) && (stricmp(tmpkv.szValue, "0")))
					{
						SetKeyValue(tmpkv.szKey, tmpkv.szValue);
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns this object's angles as a vector of the form:
//			[0] PITCH [1] YAW [2] ROLL
//-----------------------------------------------------------------------------
void CEditGameClass::GetAngles(QAngle &vecAngles)
{
	vecAngles = vec3_angle;
	const char *pszAngles = GetKeyValue("angles");
	if (pszAngles != NULL)
	{
		sscanf(pszAngles, "%f %f %f", &vecAngles[PITCH], &vecAngles[YAW], &vecAngles[ROLL]);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets a new yaw, overriding any existing angles. Uses special values
//			for yaw to indicate pointing straight up or straight down.
//
//			This method of representing orientation is obsolete; this code is
//			only for importing old RMF or MAP files.
//
// Input  : a - Value for angle.
//-----------------------------------------------------------------------------
void CEditGameClass::ImportAngle(int nAngle)
{
	QAngle vecAngles;
	vecAngles.Init();

	if (nAngle == -1) // UP
	{
		vecAngles[PITCH] = -90;
	}
	else if (nAngle == -2) // DOWN
	{
		vecAngles[PITCH] = 90;
	}
	else
	{
		vecAngles[YAW] = nAngle;
	}

	SetAngles(vecAngles);
}


//-----------------------------------------------------------------------------
// Purpose: Sets this object's angles as a vector of the form:
//			[0] PITCH [1] YAW [2] ROLL
//-----------------------------------------------------------------------------
void CEditGameClass::SetAngles(const QAngle &vecAngles)
{
	char szAngles[80];	
	sprintf(szAngles, "%g %g %g", (double)vecAngles[PITCH], (double)vecAngles[YAW], (double)vecAngles[ROLL]);
	SetKeyValue("angles", szAngles);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CEditGameClass::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	ChunkFileResult_t eResult = pFile->WriteKeyValue("classname", m_szClass);

	if (eResult != ChunkFile_Ok)
	{
		return(eResult);
	}

	//
	// Determine whether we have a game data class. This will help us decide which keys
	// to write.
	//
	GDclass *pGameDataClass = NULL;
	if (pGD != NULL)
	{
		pGameDataClass = pGD->ClassForName(m_szClass);
	}

	//
	// Consider all the keyvalues in this object for serialization.
	//
	int nNumKeys = m_KeyValues.GetCount();
	for (int i = 0; i < nNumKeys; i++)
	{
		MDkeyvalue &KeyValue = m_KeyValues.GetKeyValue(i);

		//
		// Don't write keys that were already written above.
		//
		bool bAlreadyWritten = false;
		if (!stricmp(KeyValue.szKey, "classname"))
		{
			bAlreadyWritten = true;
		}

		//
		// If the variable wasn't already written above.
		//
		if (!bAlreadyWritten)
		{
			//
			// Write it to the MAP file.
			//
			eResult = pFile->WriteKeyValue(KeyValue.szKey, KeyValue.szValue);
			if (eResult != ChunkFile_Ok)
			{
				return(eResult);
			}
		}
	}

	//
	// If we have a game data class, for each keyvalue in the class definition, write out all keys
	// that are not present in the object and whose defaults are nonzero in the class definition.
	//
	if (pGameDataClass != NULL)
	{
		//
		// For each variable from the base class...
		//
		int nVariableCount = pGameDataClass->GetVariableCount();
		for (i = 0; i < nVariableCount; i++)
		{
			GDinputvariable *pVar = pGameDataClass->GetVariableAt(i);
			ASSERT(pVar != NULL);

			if (pVar != NULL)
			{
				int iIndex;
				LPCTSTR p = m_KeyValues.GetValue(pVar->GetName(), &iIndex);

				//
				// If the variable is not present in this object, write out the default value.
				//
				if (p == NULL) 
				{
					MDkeyvalue TempKey;
					pVar->ResetDefaults();
					pVar->ToKeyValue(&TempKey);

					//
					// Only write the key value if it is non-zero.
					//
					if ((TempKey.szKey[0] != 0) && (TempKey.szValue[0] != 0) && (stricmp(TempKey.szValue, "0")))
					{
						eResult = pFile->WriteKeyValue(TempKey.szKey, TempKey.szValue);
						if (eResult != ChunkFile_Ok)
						{
							return(eResult);
						}
					}
				}
			}
		}
	}

	//
	// Save all the connections.
	//
	if ((eResult == ChunkFile_Ok) && (Connections_GetCount() > 0))
	{
		eResult = pFile->BeginChunk("connections");
		if (eResult == ChunkFile_Ok)
		{
			POSITION pos = Connections_GetHeadPosition();
			while (pos != NULL)
			{
				CEntityConnection *pConnection = Connections_GetNext(pos);
				if (pConnection != NULL)
				{
					char szTemp[512];

					sprintf(szTemp, "%s,%s,%s,%g,%d", pConnection->GetTargetName(), pConnection->GetInputName(), pConnection->GetParam(), pConnection->GetDelay(), pConnection->GetTimesToFire());
					eResult = pFile->WriteKeyValue(pConnection->GetOutputName(), szTemp);

					if (eResult != ChunkFile_Ok)
					{
						return(eResult);
					}
				}
			}
		
			eResult = pFile->EndChunk();
		}
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: Slightly modified strtok. Does not modify the input string. Does
//			not skip over more than one seperator at a time. This allows parsing
//			strings where tokens between seperators may or may not be present:
//
//			Door01,,,0 would be parsed as "Door01"  ""  ""  "0"
//			Door01,Open,,0 would be parsed as "Door01"  "Open"  ""  "0"
//
// Input  : token - Returns with a token, or zero length if the token was missing.
//			str - String to parse.
//			sep - Character to use as seperator. UNDONE: allow multiple seperator chars
// Output : Returns a pointer to the next token to be parsed.
//-----------------------------------------------------------------------------
static const char *nexttoken(char *token, const char *str, char sep)
{
	if (*str == '\0')
	{
		return(NULL);
	}

	//
	// Find the first seperator.
	//
	const char *ret = str;
	while ((*str != sep) && (*str != '\0'))
	{
		str++;
	}

	//
	// Copy everything up to the first seperator into the return buffer.
	// Do not include seperators in the return buffer.
	//
	while (ret < str)
	{
		*token++ = *ret++;
	}
	*token = '\0';

	//
	// Advance the pointer unless we hit the end of the input string.
	//
	if (*str == '\0')
	{
		return(str);
	}

	return(++str);
}


//-----------------------------------------------------------------------------
// Purpose: Builds a connection from a keyvalue pair.
// Input  : szKey - Contains the name of the output.
//			szValue - Contains the target, input, delay, and parameter, comma delimited.
//			pEditGameClass - Entity to receive the connection.
// Output : Returns ChunkFile_Ok if the data was well-formed, ChunkFile_Fail if not.
//-----------------------------------------------------------------------------
ChunkFileResult_t CEditGameClass::LoadKeyCallback(const char *szKey, const char *szValue, CEditGameClass *pEditGameClass)
{
	CEntityConnection *pConnection = new CEntityConnection;

	pConnection->SetOutputName(szKey);

	char szToken[MAX_PATH];

	//
	// Parse the target name.
	//
	const char *psz = nexttoken(szToken, szValue, ',');
	if (szToken[0] != '\0')
	{
		pConnection->SetTargetName(szToken);
	}

	//
	// Parse the input name.
	//
	psz = nexttoken(szToken, psz, ',');
	if (szToken[0] != '\0')
	{
		pConnection->SetInputName(szToken);
	}

	//
	// Parse the parameter override.
	//
	psz = nexttoken(szToken, psz, ',');
	if (szToken[0] != '\0')
	{
		pConnection->SetParam(szToken);
	}

	//
	// Parse the delay.
	//
	psz = nexttoken(szToken, psz, ',');
	if (szToken[0] != '\0')
	{
		pConnection->SetDelay((float)atof(szToken));
	}

	//
	// Parse the number of times to fire the output.
	//
	nexttoken(szToken, psz, ',');
	if (szToken[0] != '\0')
	{
		pConnection->SetTimesToFire(atoi(szToken));
	}

	pEditGameClass->Connections_Add(pConnection);

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pEntity - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CEditGameClass::LoadConnectionsCallback(CChunkFile *pFile, CEditGameClass *pEditGameClass)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadKeyCallback, pEditGameClass));
}


//-----------------------------------------------------------------------------
// Purpose: Returns all the spawnflags.
//-----------------------------------------------------------------------------
int CEditGameClass::GetSpawnFlags(void)
{
	LPCTSTR pszVal = GetKeyValue("spawnflags");
	if (pszVal == NULL)
	{
		return(0);
	}

	return(atoi(pszVal));
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if a given spawnflag (or flags) is set, false if not.
//-----------------------------------------------------------------------------
bool CEditGameClass::GetSpawnFlag(int nFlags)
{
	int nSpawnFlags = GetSpawnFlags();
	return((nSpawnFlags & nFlags) != 0);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the given spawnflag (or flags) to the given state.
// Input  : nFlag - Flag values  to set or clear (ie 1, 2, 4, 8, 16, etc.)
//			bSet - True to set the flags, false to clear them.
//-----------------------------------------------------------------------------
void CEditGameClass::SetSpawnFlag(int nFlags, bool bSet)
{
	int nSpawnFlags = GetSpawnFlags();

	if (bSet)
	{
		nSpawnFlags |= nFlags;
	}
	else
	{
		nSpawnFlags &= ~nFlags;
	}

	SetSpawnFlags(nSpawnFlags);
}


//-----------------------------------------------------------------------------
// Purpose: Sets all the spawnflags at once.
// Input  : nSpawnFlags - New value for spawnflags.
//-----------------------------------------------------------------------------
void CEditGameClass::SetSpawnFlags(int nSpawnFlags)
{
	char szValue[80];
	itoa(nSpawnFlags, szValue, 10);
	SetKeyValue("spawnflags", nSpawnFlags);
}
