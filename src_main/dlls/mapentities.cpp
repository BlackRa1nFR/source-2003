//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Controls the loading, parsing and creation of the entities from the BSP.
//
//=============================================================================

#ifdef _WIN32
#include "winlite.h"
#endif

#include "cbase.h"
#include "entitylist.h"
#include "entitymapdata.h"
#include "soundent.h"
#include "TemplateEntities.h"
#include "point_template.h"
#include "ai_initutils.h"
#include "lights.h"
#include "mapentities.h"

const char *MapEntity_ParseToken( const char *data, char *newToken, const char *braceChars );
const char *MapEntity_SkipToNextEntity( const char *pMapData );

struct HierarchicalSpawn_t
{
	CBaseEntity *m_pEntity;
	int			m_nDepth;
	const char	*m_pMapData;
	int			m_iMapDataLength;
};


static const char *g_BraceChars = "{}()\'";


// creates an entity by string name, but does not spawn it
CBaseEntity *CreateEntityByName( const char *className )
{
	return static_cast<CBaseEntity*>(EntityFactoryDictionary()->Create( className ));
}


void FreeContainingEntity( edict_t *ed )
{
	if ( ed )
	{
		CBaseEntity *ent = GetContainingEntity( ed );
		if ( ent )
		{
			ed->m_pEnt = NULL;
			CBaseEntity::PhysicsRemoveTouchedList( ent );
			UTIL_RemoveImmediate( ent );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Callback function for qsort, used to sort entities by their depth
//			in the movement hierarchy.
// Input  : pEnt1 - 
//			pEnt2 - 
// Output : Returns -1, 0, or 1 per qsort spec.
//-----------------------------------------------------------------------------
static int CompareByHierarchyDepth(HierarchicalSpawn_t *pEnt1, HierarchicalSpawn_t *pEnt2)
{
	if (pEnt1->m_nDepth == pEnt2->m_nDepth)
		return 0;

	if (pEnt1->m_nDepth > pEnt2->m_nDepth)
		return 1;

	return -1;
}


//-----------------------------------------------------------------------------
// Computes the hierarchical depth of the entities to spawn..
//-----------------------------------------------------------------------------
static int ComputeSpawnHierarchyDepth_r( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return 1;

	if (pEntity->m_iParent == NULL_STRING)
		return 1;

	CBaseEntity *pParent = gEntList.FindEntityByName(NULL, pEntity->m_iParent, NULL);
	if (!pParent)
		return 1;

	return 1 + ComputeSpawnHierarchyDepth_r( pParent );
}

static void ComputeSpawnHierarchyDepth( int nEntities, HierarchicalSpawn_t *pSpawnList )
{
	// NOTE: This isn't particularly efficient, but so what? It's at the beginning of time
	// I did it this way because it simplified the parent setting in hierarchy (basically
	// eliminated questions about whether you should transform origin from global to local or not)
	int nEntity;
	for (nEntity = 0; nEntity < nEntities; nEntity++)
	{
		CBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;
		if (pEntity && !pEntity->IsDormant())
		{
			pSpawnList[nEntity].m_nDepth = ComputeSpawnHierarchyDepth_r( pEntity );
		}
		else
		{
			pSpawnList[nEntity].m_nDepth = 1;
		}
	}
}


bool ShouldProcessEntity( CBaseEntity *pEnt, IMapEntityFilter *pFilter )
{
	if ( !pEnt )
		return false;

	if ( !pFilter )
		return true;

	return pFilter->ShouldCreateEntity( pEnt->GetClassname() );
}


//-----------------------------------------------------------------------------
// Purpose: Only called on BSP load. Parses and spawns all the entities in the BSP.
// Input  : pMapData - Pointer to the entity data block to parse.
//-----------------------------------------------------------------------------
void MapEntity_ParseAllEntities(const char *pMapData, IMapEntityFilter *pFilter, bool bActivateEntities)
{
	HierarchicalSpawn_t pSpawnList[NUM_ENT_ENTRIES];
	CUtlVector< CPointTemplate* > pPointTemplates;
	int nEntities = 0;

	// The world entity is assumed to be the first entity
	bool isWorldEntity = true;

	//
	//  Loop through all entities in the map data, creating each.
	//
	while (1)
	{
		//
		// Parse the opening brace.
		//
		char token[MAPKEY_MAXLENGTH];
		pMapData = MapEntity_ParseToken(pMapData, token, g_BraceChars);

		//
		// Check to see if we've finished or not.
		//
		if (!pMapData)
		{
			break;
		}

		if (token[0] != '{')
		{
			Error( "MapEntity_ParseAllEntities: found %s when expecting {", token);
		}

		//
		// Parse the entity and add it to the spawn list.
		//
		CBaseEntity *pEntity;
		const char *pCurMapData = pMapData;
		pMapData = MapEntity_ParseEntity(pEntity, pMapData, pFilter);
		if (pEntity != NULL)
		{
			if (!pEntity->IsTemplate())
			{
				// To 
				if ( isWorldEntity )
				{
					isWorldEntity = false;
					pEntity->m_iParent = NULL_STRING;	// don't allow a parent on the first entity (worldspawn)

					if (DispatchSpawn(pEntity) < 0)
					{
						gEntList.CleanupDeleteList();
					}
				}
				else if ( dynamic_cast<CNodeEnt*>(pEntity) || dynamic_cast<CLight*>(pEntity) )
				{
					// We overflow the max edicts on large maps that have lots of entities.
					// Nodes & Lights remove themselves immediately on Spawn(), so dispatch their
					// spawn now, to free up the slot inside this loop.
					// NOTE: This solution prevents nodes & lights from being used inside point_templates.
					if (DispatchSpawn(pEntity) < 0)
					{
						gEntList.CleanupDeleteList();
					}
				}
				else
				{
					// Build a list of all point_template's so we can spawn them before everything else
					CPointTemplate *pTemplate = dynamic_cast< CPointTemplate* >(pEntity);
					if ( pTemplate )
					{
						pPointTemplates.AddToTail( pTemplate );
					}
					else
					{
						// Queue up this entity for spawning
						pSpawnList[nEntities].m_pEntity = pEntity;
						pSpawnList[nEntities].m_nDepth = 0;
						pSpawnList[nEntities].m_pMapData = pCurMapData;
						pSpawnList[nEntities].m_iMapDataLength = (pMapData - pCurMapData) + 2;
						nEntities++;
					}
				}
			}
			else
			{
				//
				// It's a template entity. Squirrel away its keyvalue text so that we can
				// recreate the entity later via a spawner. pMapData points at the '}'
				// so we must add one to include it in the string.
				//
				Templates_Add(pEntity, pCurMapData, (pMapData - pCurMapData) + 2);

				//
				// Remove the template entity so that it does not show up in FindEntityXXX searches.
				//
				UTIL_Remove(pEntity);
				gEntList.CleanupDeleteList();
			}
		}

		//
		// Make sure we've skipped forward to the next entity.
		//
		pMapData = MapEntity_SkipToNextEntity(pMapData);
	}

	// Now loop through all our point_template entities and tell them to make templates of everything they're pointing to
	int iTemplates = pPointTemplates.Count();
	for ( int i = 0; i < iTemplates; i++ )
	{
		CPointTemplate *pPointTemplate = pPointTemplates[i];

		// First, tell the Point template to Spawn
		if ( DispatchSpawn(pPointTemplate) < 0 )
		{
			UTIL_Remove(pPointTemplate);
			gEntList.CleanupDeleteList();
			continue;
		}

		pPointTemplate->StartBuildingTemplates();

		// Now go through all it's templates and turn the entities into templates
		int iNumTemplates = pPointTemplate->GetNumTemplateEntities();
		for ( int iTemplateNum = 0; iTemplateNum < iNumTemplates; iTemplateNum++ )
		{
			// Find it in the spawn list
			CBaseEntity *pEntity = pPointTemplate->GetTemplateEntity( iTemplateNum );
			for ( int iEntNum = 0; iEntNum < nEntities; iEntNum++ )
			{
				if ( pSpawnList[iEntNum].m_pEntity == pEntity )
				{
					// Give the point_template the mapdata
					pPointTemplate->AddTemplate( pEntity, pSpawnList[iEntNum].m_pMapData, pSpawnList[iEntNum].m_iMapDataLength );

					if ( pPointTemplate->ShouldRemoveTemplateEntities() )
					{
						// Remove the template entity so that it does not show up in FindEntityXXX searches.
						UTIL_Remove(pEntity);
						gEntList.CleanupDeleteList();

						// Remove the entity from the spawn list
						pSpawnList[iEntNum].m_pEntity = NULL;
					}
					break;
				}
			}
		}

		pPointTemplate->FinishBuildingTemplates();
	}

	// Compute the hierarchical depth of all entities hierarchically attached
	ComputeSpawnHierarchyDepth( nEntities, pSpawnList );

	// Sort the entities (other than the world) by hierarchy depth, in order to spawn them in
	// that order. This insures that each entity's parent spawns before it does so that
	// it can properly set up anything that relies on hierarchy.
#ifdef _WIN32
	qsort(&pSpawnList[0], nEntities, sizeof(pSpawnList[0]), (int (__cdecl *)(const void *, const void *))CompareByHierarchyDepth);
#elif _LINUX
	qsort(&pSpawnList[0], nEntities, sizeof(pSpawnList[0]), (int (*)(const void *, const void *))CompareByHierarchyDepth);
#endif

	// Set up entity movement hierarchy in reverse hierarchy depth order. This allows each entity
	// to use its parent's world spawn origin to calculate its local origin.
	int nEntity;
	for (nEntity = nEntities - 1; nEntity >= 0; nEntity--)
	{
		CBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;
		if ( pEntity )
		{
			CBaseEntity *pParent = gEntList.FindEntityByName(NULL, pEntity->m_iParent, NULL);
			if ((pParent != NULL) && (pParent->edict() != NULL))
			{
				pEntity->SetParent( pParent ); 
			}
		}
	}

	// Spawn all the entities in hierarchy depth order so that parents spawn before their children.
	for (nEntity = 0; nEntity < nEntities; nEntity++)
	{
		CBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;

		if ( pEntity )
		{
			if (DispatchSpawn(pEntity) < 0)
			{
				// Spawn failed.
				gEntList.CleanupDeleteList();
			}
		}
	}

	if ( bActivateEntities )
	{
		for (nEntity = 0; nEntity < nEntities; nEntity++)
		{
			CBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;

			if ( pEntity )
				pEntity->Activate();
		}
	}
}


bool MapEntity_ExtractValue( const char *pEntData, const char *keyName, char Value[MAPKEY_MAXLENGTH] )
{
	char token[MAPKEY_MAXLENGTH];
	const char *inputData = pEntData;

	while ( inputData )
	{
		inputData = MapEntity_ParseToken( inputData, token, g_BraceChars );	// get keyname
		if ( token[0] == '}' )									// end of entity?
			break;												// must not have seen the classname

		// is this the right key?
		if ( !strcmp(token, keyName) )
		{
			inputData = MapEntity_ParseToken( inputData, token, g_BraceChars );	// get value and return it
			Q_strncpy( Value, token, MAPKEY_MAXLENGTH );
			return true;
		}

		inputData = MapEntity_ParseToken( inputData, token, g_BraceChars );	// skip over value
	}

	return false;
}

int MapEntity_GetNumKeysInEntity( const char *pEntData )
{
	char token[MAPKEY_MAXLENGTH];
	const char *inputData = pEntData;
	int iNumKeys = 0;

	while ( inputData )
	{
		inputData = MapEntity_ParseToken( inputData, token, g_BraceChars );	// get keyname
		if ( token[0] == '}' )									// end of entity?
			break;												// must not have seen the classname

		iNumKeys++;

		inputData = MapEntity_ParseToken( inputData, token, g_BraceChars );	// skip over value
	}

	return iNumKeys;
}

//-----------------------------------------------------------------------------
// Purpose: Takes a block of character data as the input
// Input  : pEntity - Receives the newly constructed entity, NULL on failure.
//			pEntData - Data block to parse to extract entity keys.
// Output : Returns the current position in the entity data block.
//-----------------------------------------------------------------------------
const char *MapEntity_ParseEntity(CBaseEntity *&pEntity, const char *pEntData, IMapEntityFilter *pFilter)
{
	CEntityMapData entData( (char*)pEntData );
	char className[MAPKEY_MAXLENGTH];
	
	if (!entData.ExtractValue("classname", className))
	{
		Error( "classname missing from entity!\n" );
	}

	pEntity = NULL;
	if ( !pFilter || pFilter->ShouldCreateEntity( className ) )
	{
		//
		// Construct via the LINK_ENTITY_TO_CLASS factory.
		//
		pEntity = CreateEntityByName(className);

		//
		// Set up keyvalues.
		//
		if (pEntity != NULL)
		{
			pEntity->ParseMapData(&entData);
		}
		else
		{
			Warning("Can't init %s\n", className);
		}
	}
	else
	{
		// Just skip past all the keys.
		char keyName[MAPKEY_MAXLENGTH];
		char value[MAPKEY_MAXLENGTH];
		if ( entData.GetFirstKey(keyName, value) )
		{
			do 
			{
			} 
			while ( entData.GetNextKey(keyName, value) );
		}
	}

	//
	// Return the current parser position in the data block
	//
	return entData.CurrentBufferPosition();
}


// skips to the beginning of the next entity in the data block
// returns NULL if no more entities
const char *MapEntity_SkipToNextEntity( const char *pMapData )
{
	static char szToken[MAPKEY_MAXLENGTH];

	if ( !pMapData )
		return NULL;

	// search through the map string for the next matching '{'
	int openBraceCount = 1;
	while ( pMapData != NULL )
	{
		pMapData = MapEntity_ParseToken( pMapData, szToken, g_BraceChars );

		if ( FStrEq(szToken, "{") )
		{
			openBraceCount++;
		}
		else if ( FStrEq(szToken, "}") )
		{
			if ( --openBraceCount == 0 )
			{
				// we've found the closing brace, so return the next character
				return pMapData;
			}
		}
	}

	// eof hit
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: parses a token out of a char data block
//			the token gets fully read no matter what the length, but only MAPKEY_MAXLENGTH 
//			characters are written into newToken
// Input  : char *data - the data to parse
//			char *newToken - the buffer into which the new token is written
//			char *braceChars - a string of characters that constitute braces.  this pointer needs to be
//			distince for each set of braceChars, since the usage is cached.
// Output : const char * - returns a pointer to the position in the data following the newToken
//-----------------------------------------------------------------------------
const char *MapEntity_ParseToken( const char *data, char *newToken, const char *braceChars )
{
	int             c;
	int             len;
	static bool s_BraceCharacters[256];
	static const char *s_OldBraceCharPtr = NULL;
	
	len = 0;
	newToken[0] = 0;
	
	if (!data)
		return NULL;

	// build the new table if we have to
	if ( s_OldBraceCharPtr != braceChars )
	{
		s_OldBraceCharPtr = braceChars;
		memset( s_BraceCharacters, 0, sizeof(s_BraceCharacters) );

		for ( ; *braceChars; braceChars++ )
		{
			s_BraceCharacters[*braceChars] = true;
		}
	}
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while ( len < MAPKEY_MAXLENGTH )
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				newToken[len] = 0;
				return data;
			}
			newToken[len] = c;
			len++;
		}

		if ( len >= MAPKEY_MAXLENGTH )
		{
			len--;
			newToken[len] = 0;
		}
	}

// parse single characters
	if ( s_BraceCharacters[c]/*c=='{' || c=='}'|| c==')'|| c=='(' || c=='\''*/ )
	{
		newToken[len] = c;
		len++;
		newToken[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		newToken[len] = c;
		data++;
		len++;
		c = *data;
		if ( s_BraceCharacters[c] /*c=='{' || c=='}'|| c==')'|| c=='(' || c=='\''*/ )
			break;

		if ( len >= MAPKEY_MAXLENGTH )
		{
			len--;
			newToken[len] = 0;
		}

	} while (c>32);
	
	newToken[len] = 0;
	return data;
}


/* ================= CEntityMapData definition ================ */

bool CEntityMapData::ExtractValue( const char *keyName, char *value )
{
	return MapEntity_ExtractValue( m_pEntData, keyName, value );
}

bool CEntityMapData::GetFirstKey( char *keyName, char *value )
{
	m_pCurrentKey = m_pEntData; // reset the status pointer
	return GetNextKey( keyName, value );
}

const char *CEntityMapData::CurrentBufferPosition( void )
{
	return m_pCurrentKey;
}

bool CEntityMapData::GetNextKey( char *keyName, char *value )
{
	char token[MAPKEY_MAXLENGTH];

	// parse key
	char *pPrevKey = m_pCurrentKey;
	m_pCurrentKey = (char*)MapEntity_ParseToken( m_pCurrentKey, token, g_BraceChars );
	if ( token[0] == '}' )
	{
		// step back
		m_pCurrentKey = pPrevKey;
		return false;
	}

	if ( !m_pCurrentKey )
	{
		Warning( "CEntityMapData::GetNextKey: EOF without closing brace\n" );
		Assert(0);
		return false;
	}
	
	Q_strncpy( keyName, token, MAPKEY_MAXLENGTH );

	// fix up keynames with trailing spaces
	int n = strlen(keyName);
	while (n && keyName[n-1] == ' ')
	{
		keyName[n-1] = 0;
		n--;
	}

	// parse value	
	m_pCurrentKey = (char*)MapEntity_ParseToken( m_pCurrentKey, token, g_BraceChars );
	if ( !m_pCurrentKey )
	{
		Warning( "CEntityMapData::GetNextKey: EOF without closing brace\n" );
		Assert(0);
		return false;
	}
	if ( token[0] == '}' )
	{
		Warning( "CEntityMapData::GetNextKey: closing brace without data\n" );
		Assert(0);
		return false;
	}

	// value successfully found
	Q_strncpy( value, token, MAPKEY_MAXLENGTH );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: find the keyName in the endata and change its value to specified one
//-----------------------------------------------------------------------------
bool CEntityMapData::SetValue( const char *keyName, char *NewValue )
{
	char token[MAPKEY_MAXLENGTH];
	char *inputData = m_pEntData;
	char *prevData;

	while ( inputData )
	{
		inputData = (char*)MapEntity_ParseToken( inputData, token, g_BraceChars );	// get keyname
		if ( token[0] == '}' )									// end of entity?
			break;												// must not have seen the classname

		// is this the right key?
		if ( !strcmp(token, keyName) )
		{
			// Find the start & end of the token we're going to replace
			int entLen = strlen(m_pEntData);
			char *postData = new char[entLen];
			prevData = inputData;
			inputData = (char*)MapEntity_ParseToken( inputData, token, g_BraceChars );	// get keyname
			Q_strncpy( postData, inputData, entLen );

			int iPadding = strlen(NewValue) - strlen( token ) - 1; // -1 for the whitespace
			// Prepend quotes if they didn't
			if ( NewValue[0] != '\"' )
			{
				Q_strcpy( prevData+1, "\"" );		// +1 for the whitespace
				Q_strcat( prevData, NewValue );
				Q_strcat( prevData, "\"" );
				Q_strcat( prevData, postData );
				iPadding += 2;
			}
			else
			{
				Q_strcpy( prevData+1, NewValue );	// +1 for the whitespace
				Q_strcat( prevData, postData );
			}

			m_pCurrentKey += iPadding;
			return true;
		}

		inputData = (char*)MapEntity_ParseToken( inputData, token, g_BraceChars );	// skip over value
	}

	return false;
}
