//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include <WorldSize.h>
#include "GameData.h"
#include "GlobalFunctions.h"
#include "HelperInfo.h"
#include "Worldcraft.h"
#include "MapDoc.h"
#include "MapDoc.h"
#include "MapEntity.h"
#include "MapWorld.h"
#include "vstdlib/strtools.h"

#pragma warning(disable:4244)


const int MAX_ERRORS = 5;


GameData *pGD;
CGameConfig g_DefaultGameConfig;
CGameConfig *g_pGameConfig = &g_DefaultGameConfig;


float g_MAX_MAP_COORD = 4096;
float g_MIN_MAP_COORD = -4096;


//-----------------------------------------------------------------------------
// Purpose: Fetches the next token from the file.
// Input  : tr - 
//			ppszStore - Destination buffer, one of the following:
//				pointer to NULL - token will be placed in an allocated buffer
//				pointer to non-NULL buffer - token will be placed in buffer
//			ttexpecting - 
//			pszExpecting - 
// Output : 
//-----------------------------------------------------------------------------
static bool DoGetToken(TokenReader &tr, char **ppszStore, int nSize, trtoken_t ttexpecting, LPCTSTR pszExpecting)
{
	trtoken_t ttype;

	if (*ppszStore != NULL)
	{
		// Reads the token into the given buffer.
		ttype = tr.NextToken(*ppszStore, nSize);
	}
	else
	{
		// Allocates a buffer to hold the token.
		ttype = tr.NextTokenDynamic(ppszStore);
	}

	if (ttype == TOKENSTRINGTOOLONG)
	{
		GDError(tr, "unterminated string or string too long");
		return false;
	}

	//
	// Check for a bad token type.
	//
	char *pszStore = *ppszStore;
	bool bBadTokenType = false;
	if ((ttype != ttexpecting) && (ttexpecting != TOKENNONE))
	{
		//
		// If we were expecting a string and got an integer, don't worry about it.
		// We can translate from integer to string.
		//
		if (!((ttexpecting == STRING) && (ttype == INTEGER)))
		{
			bBadTokenType = true;
		}
	}

	if (bBadTokenType && (pszExpecting == NULL))
	{
		//
		// We didn't get the expected token type but no expected
		// string was specified.
		//
		char *pszTokenName;
		switch (ttexpecting)
		{
			case IDENT:
			{
				pszTokenName = "identifier";
				break;
			}

			case INTEGER:
			{
				pszTokenName = "integer";
				break;
			}

			case STRING:
			{
				pszTokenName = "string";
				break;
			}

			case OPERATOR:
			{
				pszTokenName = "symbol";
				break;
			}
		}
		
		GDError(tr, "expecting %s", pszTokenName);
		return false;
	}
	else if (bBadTokenType || ((pszExpecting != NULL) && !IsToken(pszStore, pszExpecting)))
	{
		//
		// An expected string was specified, and we got either the wrong type or
		// the right type but the wrong string,
		//
		GDError(tr, "expecting '%s', but found '%s'", pszExpecting, pszStore);
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : tr - 
//			error - 
// Output : 
//-----------------------------------------------------------------------------
bool GDError(TokenReader& tr, char *error, ...)
{
	char szBuf[128];
	va_list vl;
	va_start(vl, error);
	vsprintf(szBuf, error, vl);
	va_end(vl);
	Msg(mwError, tr.Error(szBuf));

	if (tr.GetErrorCount() >= MAX_ERRORS)
	{
		Msg(mwError, "   - too many errors; aborting.");
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Fetches the next token from the file.
// Input  : tr - The token reader object with which to fetch the token.
//			pszStore - Buffer in which to place the token, NULL to discard the token.
//			ttexpecting - The token type that we are expecting. If this is not TOKENNONE
//				and token type read is different, the operation will fail.
//			pszExpecting - The token string that we are expecting. If this string
//				is not NULL and the token string read is different, the operation will fail.
// Output : Returns TRUE if the operation succeeded, FALSE if there was an error.
//			If there was an error, the error will be reported in the message window.
//-----------------------------------------------------------------------------
bool GDGetToken(TokenReader &tr, char *pszStore, int nSize, trtoken_t ttexpecting, LPCTSTR pszExpecting)
{
	ASSERT(pszStore != NULL);
	if (pszStore != NULL)
	{
		return DoGetToken(tr, &pszStore, nSize, ttexpecting, pszExpecting);
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Fetches the next token from the file.
// Input  : tr - The token reader object with which to fetch the token.
//			pszStore - Buffer in which to place the token, NULL to discard the token.
//			ttexpecting - The token type that we are expecting. If this is not TOKENNONE
//				and token type read is different, the operation will fail.
//			pszExpecting - The token string that we are expecting. If this string
//				is not NULL and the token string read is different, the operation will fail.
// Output : Returns TRUE if the operation succeeded, FALSE if there was an error.
//			If there was an error, the error will be reported in the message window.
//-----------------------------------------------------------------------------
bool GDSkipToken(TokenReader &tr, trtoken_t ttexpecting, LPCTSTR pszExpecting)
{
	//
	// Read the next token into a buffer and discard it.
	//
	char szDiscardBuf[MAX_TOKEN];
	char *pszDiscardBuf = szDiscardBuf;
	return DoGetToken(tr, &pszDiscardBuf, sizeof(szDiscardBuf), ttexpecting, pszExpecting);
}


//-----------------------------------------------------------------------------
// Purpose: Fetches the next token from the file, allocating a buffer exactly
//			large enough to hold the token.
// Input  : tr - 
//			ppszStore - 
//			ttexpecting - 
//			pszExpecting - 
// Output : 
//-----------------------------------------------------------------------------
bool GDGetTokenDynamic(TokenReader &tr, char **ppszStore, trtoken_t ttexpecting, LPCTSTR pszExpecting)
{
	if (ppszStore == NULL)
	{
		return false;
	}

	*ppszStore = NULL;
	return DoGetToken(tr, ppszStore, -1, ttexpecting, pszExpecting);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGameConfig *GetActiveGame(void)
{
	return(g_pGameConfig);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pGame - 
//-----------------------------------------------------------------------------
void SetActiveGame(CGameConfig *pGame)
{
	if (pGame != NULL)
	{
		g_pGameConfig = pGame;
		pGD = &pGame->GD;

		if (pGame->mapformat == mfHalfLife)
		{
			g_MAX_MAP_COORD = 4096;
			g_MIN_MAP_COORD = -4096;
		}
		else
		{
			g_MAX_MAP_COORD = pGD->GetMaxMapCoord();
			g_MIN_MAP_COORD = pGD->GetMinMapCoord();
		}
	}
	else
	{
		g_pGameConfig = &g_DefaultGameConfig;
		pGD = NULL;

		g_MAX_MAP_COORD = 4096;
		g_MIN_MAP_COORD = -4096;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
GameData::GameData(void)
{
	m_nMaxMapCoord = 8192;
	m_nMinMapCoord = -8192;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
GameData::~GameData(void)
{
	ClearData();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GameData::ClearData(void)
{
	// delete classes.
	POSITION p = Classes.GetHeadPosition();
	while(p)
	{
		GDclass *pm = Classes.GetNext(p);
		delete pm;
	}
	Classes.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: Loads a gamedata (FGD) file into this object.
// Input  : pszFilename - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL GameData::Load(LPCTSTR pszFilename)
{
	TokenReader tr;
	BOOL bMainRetrieved = FALSE;

	if(GetFileAttributes(pszFilename) == 0xffffffff)
		return FALSE;

	if(!tr.Open(pszFilename))
		return FALSE;

	trtoken_t ttype;
	char szToken[128];

	while (1)
	{
		if (tr.GetErrorCount() >= MAX_ERRORS)
		{
			break;
		}

		ttype = tr.NextToken(szToken, sizeof(szToken));

		if(ttype == TOKENEOF)
			break;

		if(ttype != OPERATOR || !IsToken(szToken, "@"))
		{
			if(!GDError(tr, "expected @"))
				return FALSE;
		}

		// check what kind it is, and parse a new object
		if (tr.NextToken(szToken, sizeof(szToken)) != IDENT)
		{
			if(!GDError(tr, "expected identifier after @"))
				return FALSE;
		}

		if (IsToken(szToken, "baseclass") || IsToken(szToken, "pointclass") || IsToken(szToken, "solidclass") || IsToken(szToken, "keyframeclass") || IsToken(szToken, "moveclass") || IsToken(szToken, "npcclass") || IsToken(szToken, "filterclass"))
		{
			//
			// New class.
			//
			GDclass *pNewClass = new GDclass;
			if (!pNewClass->InitFromTokens(tr, this))
			{
				tr.IgnoreTill(OPERATOR, "@");	// go to next section
				delete pNewClass;
			}
			else
			{
				if (ClassForName(pNewClass->GetName()))
				{
					// delete the new class - redefinition of an existing class
					delete pNewClass;
				}
				else
				{
					if (IsToken(szToken, "baseclass"))			// Not directly available to user.
					{
						pNewClass->SetBaseClass(true);
					}
					else if (IsToken(szToken, "pointclass"))	// Generic point class.
					{
						pNewClass->SetPointClass(true);
					}
					else if (IsToken(szToken, "solidclass"))	// Tied to solids.
					{
						pNewClass->SetSolidClass(true);
					}
					else if (IsToken(szToken, "npcclass"))		// NPC class - can be spawned by npc_maker.
					{
						pNewClass->SetPointClass(true);
						pNewClass->SetNPCClass(true);
					}
					else if (IsToken(szToken, "filterclass"))	// Filter class - can be used as a filter
					{
						pNewClass->SetPointClass(true);
						pNewClass->SetFilterClass(true);
					}
					else if (IsToken(szToken, "moveclass"))		// Animating
					{
						pNewClass->SetMoveClass(true);
						pNewClass->SetPointClass(true);
					}
					else if (IsToken(szToken, "keyframeclass"))	// Animation keyframes
					{
						pNewClass->SetKeyFrameClass(true);
						pNewClass->SetPointClass(true);
					}

					Classes.AddTail(pNewClass);
				}
			}
		}
		else if (IsToken(szToken, "include"))
		{
			if (GDGetToken(tr, szToken, sizeof(szToken), STRING))
			{
				if (!Load(szToken))
				{
					GDError(tr, "error including file: %s", szToken);
				}
			}
		}
		else if (IsToken(szToken, "mapsize"))
		{
			if (!ParseMapSize(tr))
			{
				// Error in map size specifier, skip to next @ sign. 
				tr.IgnoreTill(OPERATOR, "@");
			}
		}
		else
		{
			GDError(tr, "unrecognized section name %s", szToken);
			tr.IgnoreTill(OPERATOR, "@");
		}
	}

	if (tr.GetErrorCount() > 0)
	{
		return FALSE;
	}

	tr.Close();

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Parses the "mapsize" specifier, which should be of the form:
//
//			mapsize(min, max)
//
//			ex: mapsize(-8192, 8192)
//
// Input  : tr - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool GameData::ParseMapSize(TokenReader &tr)
{
	if (!GDSkipToken(tr, OPERATOR, "("))
	{
		return false;
	}

	char szToken[128];
	if (!GDGetToken(tr, szToken, sizeof(szToken), INTEGER))
	{
		return false;
	}
	int nMin = atoi(szToken);	

	if (!GDSkipToken(tr, OPERATOR, ","))
	{
		return false;
	}

	if (!GDGetToken(tr, szToken, sizeof(szToken), INTEGER))
	{
		return false;
	}
	int nMax = atoi(szToken);	

	if (nMin != nMax)
	{
		m_nMinMapCoord = min(nMin, nMax);
		m_nMaxMapCoord = max(nMin, nMax);
	}

	if (!GDSkipToken(tr, OPERATOR, ")"))
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszName - 
//			piIndex - 
// Output : 
//-----------------------------------------------------------------------------
GDclass *GameData::ClassForName(LPCTSTR pszName, int *piIndex)
{
	POSITION p = Classes.GetHeadPosition();
	int iIndex = 0;

	while(p)
	{
		GDclass *mp = Classes.GetNext(p);
		if(!strcmp(mp->GetName(), pszName))
		{
			if(piIndex)
				piIndex[0] = iIndex;
			return mp;
		}
		++iIndex;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : tr - 
//			pGD - 
// Output : Returns TRUE if worth continuing, FALSE otherwise	
//-----------------------------------------------------------------------------
BOOL GDmain::InitFromTokens(TokenReader& tr, GameData *pGD)
{
	if (!GDSkipToken(tr, OPERATOR, "=") || !GDSkipToken(tr, OPERATOR, "["))
	{
		return FALSE;
	}

	// we now skip main section altogether
	char szToken[128];
	while (1)
	{
		trtoken_t tt = tr.NextToken(szToken, sizeof(szToken));
		if ((int)tt < 0)
		{
			return FALSE;
		}

		if ((tt == OPERATOR) && !strcmp(szToken, "]"))
		{
			return TRUE;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Constructor. Maintains a static	counter uniquely identifying each
//			game configuration.
//-----------------------------------------------------------------------------
CGameConfig::CGameConfig(void)
{
	nGDFiles = 0;
	textureformat = tfNone;
	m_fDefaultTextureScale = 1;
	m_nDefaultLightmapScale = DEFAULT_LIGHTMAP_SCALE;
	m_MaterialExcludeCount = 0;

	memset(szName, 0, sizeof(szName));
	memset(szExecutable, 0, sizeof(szExecutable));
	memset(szDefaultPoint, 0, sizeof(szDefaultPoint));
	memset(szDefaultSolid, 0, sizeof(szDefaultSolid));
	memset(m_szCSG, 0, sizeof(m_szCSG));
	memset(szBSP, 0, sizeof(szBSP));
	memset(szLIGHT, 0, sizeof(szLIGHT));
	memset(szVIS, 0, sizeof(szVIS));
	memset(szMapDir, 0, sizeof(szMapDir));
	memset(m_szGameExeDir, 0, sizeof(m_szGameExeDir));
	memset(szBSPDir, 0, sizeof(szBSPDir));
	memset(m_szModDir, 0, sizeof(m_szModDir));
	memset(m_szGameDir, 0, sizeof(m_szGameDir));
	memset(m_szMaterialExcludeDirs, 0, sizeof( m_szMaterialExcludeDirs ));
	strcpy(m_szCordonTexture, "BLACK");

	static DWORD __dwID = 0;
	dwID = __dwID++;
}


//-----------------------------------------------------------------------------
// Purpose: Imports an old binary GameCfg.wc file.
// Input  : file - 
//			fVersion - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL CGameConfig::Import(fstream& file, float fVersion)
{
	file.read(szName, sizeof szName);
	file.read((char*)&nGDFiles, sizeof nGDFiles);
	file.read((char*)&textureformat, sizeof textureformat);

	if (fVersion >= 1.1f)
	{
		file.read((char*)&mapformat, sizeof mapformat);
	}
	else
	{
		mapformat = mfQuake;
	}

	//
	// If reading an old (pre 1.4) format file, skip past the obselete palette
	// file path.
	//
	if (fVersion < 1.4f)
	{
		char szPalette[128];
		file.read(szPalette, sizeof szPalette);
	}

	file.read(szExecutable, sizeof szExecutable);
	file.read(szDefaultSolid, sizeof szDefaultSolid);
	file.read(szDefaultPoint, sizeof szDefaultPoint);

	if (fVersion >= 1.2f)
	{
		file.read(szBSP, sizeof szBSP);
		file.read(szLIGHT, sizeof szLIGHT);
		file.read(szVIS, sizeof szVIS);
		file.read(m_szGameExeDir, sizeof m_szGameExeDir);
		file.read(szMapDir, sizeof szMapDir);
	}

	if (fVersion >= 1.3f)
	{
		file.read(szBSPDir, sizeof(szBSPDir));
	}

	if (fVersion >= 1.4f)
	{
		file.read(m_szCSG, sizeof(m_szCSG));
		file.read(m_szModDir, sizeof(m_szModDir));
		file.read(m_szGameDir, sizeof(m_szGameDir));
	}

	// read game data files
	char szBuf[128];
	for(int i = 0; i < nGDFiles; i++)
	{
		file.read(szBuf, sizeof szBuf);
		GDFiles.Add(CString(szBuf));
	}

	LoadGDFiles();
	
	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: Loads this game configuration from the INI file.
// Input  : pszFileName - INI file from which to load.
//			pszSection - Section name of this game configuration.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameConfig::Load(const char *pszFileName, const char *pszSection)
{
	char szKey[MAX_PATH];

	GetPrivateProfileString(pszSection, "Name", pszSection, szName, sizeof(szName), pszFileName);
	
	//
	// Load the game data filenames from the "GameData0..GameDataN" keys.
	//
	nGDFiles = 0;
	int nStrlen = 0;
	do
	{
		char szGameData[MAX_PATH];

		sprintf(szKey, "GameData%d", nGDFiles);
		nStrlen = GetPrivateProfileString(pszSection, szKey, "", szGameData, sizeof(szGameData), pszFileName);
		if (nStrlen > 0)
		{
			GDFiles.Add(szGameData);
			nGDFiles++;
		}
	} while (nStrlen > 0);

	textureformat = (TEXTUREFORMAT)GetPrivateProfileInt(pszSection, "TextureFormat", tfVMT, pszFileName);
	mapformat = (MAPFORMAT)GetPrivateProfileInt(pszSection, "MapFormat", mfHalfLife2, pszFileName);

	GetPrivateProfileString(pszSection, "DefaultTextureScale", "1", szKey, sizeof(szKey), pszFileName);
	m_fDefaultTextureScale = (float)atof(szKey);
	if (m_fDefaultTextureScale == 0)
	{
		m_fDefaultTextureScale = 1;
	}

	m_nDefaultLightmapScale = GetPrivateProfileInt(pszSection, "DefaultLightmapScale", DEFAULT_LIGHTMAP_SCALE, pszFileName);

	GetPrivateProfileString(pszSection, "GameExe", "", szExecutable, sizeof(szExecutable), pszFileName);
	GetPrivateProfileString(pszSection, "DefaultSolidEntity", "", szDefaultSolid, sizeof(szDefaultSolid), pszFileName);
	GetPrivateProfileString(pszSection, "DefaultPointEntity", "", szDefaultPoint, sizeof(szDefaultPoint), pszFileName);

	GetPrivateProfileString(pszSection, "BSP", "", szBSP, sizeof(szBSP), pszFileName);
	GetPrivateProfileString(pszSection, "Vis", "", szVIS, sizeof(szVIS), pszFileName);
	GetPrivateProfileString(pszSection, "Light", "", szLIGHT, sizeof(szLIGHT), pszFileName);
	GetPrivateProfileString(pszSection, "GameExeDir", "", m_szGameExeDir, sizeof(m_szGameExeDir), pszFileName);
	GetPrivateProfileString(pszSection, "MapDir", "", szMapDir, sizeof(szMapDir), pszFileName);

	GetPrivateProfileString(pszSection, "BSPDir", "", szBSPDir, sizeof(szBSPDir), pszFileName);
	GetPrivateProfileString(pszSection, "CSG", "", m_szCSG, sizeof(m_szCSG), pszFileName);
	GetPrivateProfileString(pszSection, "ModDir", "", m_szModDir, sizeof(m_szModDir), pszFileName);
	GetPrivateProfileString(pszSection, "GameDir", "", m_szGameDir, sizeof(m_szGameDir), pszFileName);

	GetPrivateProfileString(pszSection, "CordonTexture", "BLACK", m_szCordonTexture, sizeof(m_szCordonTexture), pszFileName);

	char szExcludeDir[MAX_PATH];
	GetPrivateProfileString(pszSection, "MaterialExcludeCount", "0", szKey, sizeof(szKey), pszFileName );
	m_MaterialExcludeCount = atoi( szKey );
	for( int i = 0; i < m_MaterialExcludeCount; i++ )
	{
		sprintf( &szExcludeDir[0], "-MaterialExcludeDir%d", i );
		GetPrivateProfileString(pszSection, szExcludeDir, "", m_szMaterialExcludeDirs[i], sizeof( m_szMaterialExcludeDirs[0] ), pszFileName ); 

		// Strip trailing /
		int len = Q_strlen( m_szMaterialExcludeDirs[i] );
		char &lastChar = m_szMaterialExcludeDirs[i][len-1];
		if ((lastChar == '\\') || (lastChar == '/'))
			lastChar = 0;
	}

	LoadGDFiles();
	
	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: Saves this game configuration to the INI file.
// Input  : pszFileName - INI file to which to save.
//			pszSection - Section name of this game configuration.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameConfig::Save(const char *pszFileName, const char *pszSection)
{
	//
	// First delete everything from this section of the INI file.
	//
	WritePrivateProfileString(pszSection, NULL, NULL, pszFileName);

	//
	// Write the name of this game configuration.
	//
	WritePrivateProfileString(pszSection, "Name", szName, pszFileName);
	
	//
	// Write the game data filenames to "GameData0..GameDataN" keys.
	//
	for (int i = 0; i < nGDFiles; i++)
	{
		char szKey[MAX_PATH];

		sprintf(szKey, "GameData%d", i);
		WritePrivateProfileString(pszSection, szKey, GDFiles.GetAt(i), pszFileName);
	}

	char szKey[MAX_PATH];
	WritePrivateProfileString(pszSection, "TextureFormat", itoa(textureformat, szKey, 10), pszFileName);
	WritePrivateProfileString(pszSection, "MapFormat", itoa(mapformat, szKey, 10), pszFileName);
	
	sprintf(szKey, "%g", m_fDefaultTextureScale);
	WritePrivateProfileString(pszSection, "DefaultTextureScale", szKey, pszFileName);

	WritePrivateProfileString(pszSection, "DefaultLightmapScale", itoa(m_nDefaultLightmapScale, szKey, 10), pszFileName);

	WritePrivateProfileString(pszSection, "GameExe", szExecutable, pszFileName);
	WritePrivateProfileString(pszSection, "DefaultSolidEntity", szDefaultSolid, pszFileName);
	WritePrivateProfileString(pszSection, "DefaultPointEntity", szDefaultPoint, pszFileName);

	WritePrivateProfileString(pszSection, "BSP", szBSP, pszFileName);
	WritePrivateProfileString(pszSection, "Vis", szVIS, pszFileName);
	WritePrivateProfileString(pszSection, "Light", szLIGHT, pszFileName);
	WritePrivateProfileString(pszSection, "GameExeDir", m_szGameExeDir, pszFileName);
	WritePrivateProfileString(pszSection, "MapDir", szMapDir, pszFileName);

	WritePrivateProfileString(pszSection, "BSPDir", szBSPDir, pszFileName);
	WritePrivateProfileString(pszSection, "CSG", m_szCSG, pszFileName);
	WritePrivateProfileString(pszSection, "ModDir", m_szModDir, pszFileName);
	WritePrivateProfileString(pszSection, "GameDir", m_szGameDir, pszFileName);

	WritePrivateProfileString(pszSection, "CordonTexture", m_szCordonTexture, pszFileName);

	char szExcludeDir[MAX_PATH];
	WritePrivateProfileString(pszSection, "MaterialExcludeCount", itoa( m_MaterialExcludeCount, szKey, 10 ), pszFileName );
	for( i = 0; i < m_MaterialExcludeCount; i++ )
	{
		sprintf( &szExcludeDir[0], "-MaterialExcludeDir%d", i );
		WritePrivateProfileString(pszSection, szExcludeDir, m_szMaterialExcludeDirs[i], pszFileName ); 
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : file - 
//-----------------------------------------------------------------------------
void CGameConfig::Save(fstream &file)
{
	file.write(szName, sizeof szName);
	file.write((char*)&nGDFiles, sizeof nGDFiles);
	file.write((char*)&textureformat, sizeof textureformat);
	file.write((char*)&mapformat, sizeof mapformat);
	file.write(szExecutable, sizeof szExecutable);
	file.write(szDefaultSolid, sizeof szDefaultSolid);
	file.write(szDefaultPoint, sizeof szDefaultPoint);

	// 1.2
	file.write(szBSP, sizeof szBSP);
	file.write(szLIGHT, sizeof szLIGHT);
	file.write(szVIS, sizeof szVIS);
	file.write(m_szGameExeDir, sizeof(m_szGameExeDir));
	file.write(szMapDir, sizeof szMapDir);
	
	// 1.3
	file.write(szBSPDir, sizeof szBSPDir);

	// 1.4
	file.write(m_szCSG, sizeof(m_szCSG));
	file.write(m_szModDir, sizeof(m_szModDir));
	file.write(m_szGameDir, sizeof(m_szGameDir));

	// write game data files
	char szBuf[128];
	for(int i = 0; i < nGDFiles; i++)
	{
		strcpy(szBuf, GDFiles[i]);
		file.write(szBuf, sizeof szBuf);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pConfig - 
//-----------------------------------------------------------------------------
void CGameConfig::CopyFrom(CGameConfig *pConfig)
{
	nGDFiles = pConfig->nGDFiles;

	GDFiles.RemoveAll();
	GDFiles.Append(pConfig->GDFiles);

	strcpy(szName, pConfig->szName);
	strcpy(szExecutable, pConfig->szExecutable);
	strcpy(szDefaultPoint, pConfig->szDefaultPoint);
	strcpy(szDefaultSolid, pConfig->szDefaultSolid);
	strcpy(m_szCSG, pConfig->m_szCSG);
	strcpy(szBSP, pConfig->szBSP);
	strcpy(szLIGHT, pConfig->szLIGHT);
	strcpy(szVIS, pConfig->szVIS);
	strcpy(szMapDir, pConfig->szMapDir);
	strcpy(m_szGameExeDir, pConfig->m_szGameExeDir);
	strcpy(szBSPDir, pConfig->szBSPDir);
	strcpy(m_szModDir, pConfig->m_szModDir);
	strcpy(m_szGameDir, pConfig->m_szGameDir);

	pConfig->m_MaterialExcludeCount = m_MaterialExcludeCount;
	for( int i = 0; i < m_MaterialExcludeCount; i++ )
	{
		strcpy( m_szMaterialExcludeDirs[i], pConfig->m_szMaterialExcludeDirs[i] );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEntity - 
//			pGD - 
// Output : Returns TRUE to keep enumerating.
//-----------------------------------------------------------------------------
static BOOL UpdateClassPointer(CMapEntity *pEntity, GameData *pGD)
{
	GDclass *pClass = pGD->ClassForName(pEntity->GetClassName());
	pEntity->SetClass(pClass);
	return(TRUE);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameConfig::LoadGDFiles(void)
{
	GD.ClearData();
	
	for (int i = 0; i < nGDFiles; i++)
	{
		GD.Load(GDFiles[i]);
	}

	//
	// All the class pointers have changed - now we have to
	// reset all the class pointers in each map doc that 
	// uses this game.
	//
	POSITION p = CMapDoc::GetFirstDocPosition();
	while (p != NULL)
	{
		CMapDoc *pDoc = CMapDoc::GetNextDoc(p);
		if (pDoc->GetGame() == this)
		{
			CMapWorld *pWorld = pDoc->GetMapWorld();
			pWorld->SetClass(GD.ClassForName(pWorld->GetClassName()));
			pWorld->EnumChildren((ENUMMAPCHILDRENPROC)UpdateClassPointer, (DWORD)&GD, MAPCLASS_TYPE(CMapEntity));
		}
	}
}

