//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "map_shared.h"
#include "bsplib.h"
#include "cmdlib.h"


CMapError g_MapError;
int g_nMapFileVersion;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pLoadEntity - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadEntityKeyCallback(const char *szKey, const char *szValue, LoadEntity_t *pLoadEntity)
{
	if (!stricmp(szKey, "classname"))
	{
		if (!stricmp(szValue, "func_detail"))
		{
			pLoadEntity->nBaseContents = CONTENTS_DETAIL;
		}
		else if (!stricmp(szValue, "func_ladder"))
		{
			pLoadEntity->nBaseContents = CONTENTS_LADDER;
		}
		else if (!stricmp(szValue, "func_water"))
		{
			pLoadEntity->nBaseContents = CONTENTS_WATER;
		}
	}
	else if (!stricmp(szKey, "id"))
	{
		// UNDONE: flag errors by ID instead of index
		//g_MapError.BrushState( atoi( szValue ), 0 );
		return(ChunkFile_Ok);
	}
	else if( !stricmp( szKey, "mapversion" ) )
	{
		// .vmf map revision number
		g_MapRevision = atoi( szValue );
		return ( ChunkFile_Ok );
	}

	//
	// Create new epair and fill it out.
	//
	epair_t *e = new epair_t;

	e->key = new char [strlen(szKey) + 1];
	e->value = new char [strlen(szValue) + 1];

	strcpy(e->key, szKey);
	strcpy(e->value, szValue);

	//
	// Append it to the end of epair list.
	//
	e->next = NULL;

	if (!pLoadEntity->pEntity->epairs)
	{
		pLoadEntity->pEntity->epairs = e;
	}
	else
	{
		epair_t *ep;
		for ( ep = pLoadEntity->pEntity->epairs; ep->next != NULL; ep = ep->next )
		{
		}
		ep->next = e;
	}

	return(ChunkFile_Ok);
}


static ChunkFileResult_t LoadEntityCallback( CChunkFile *pFile, int nParam )
{
	if (num_entities == MAX_MAP_ENTITIES)
	{
		// Exits.
		g_MapError.ReportError ("num_entities == MAX_MAP_ENTITIES");
	}

	entity_t *mapent = &entities[num_entities];
	num_entities++;
	memset(mapent, 0, sizeof(*mapent));
	mapent->numbrushes = 0;

	LoadEntity_t LoadEntity;
	LoadEntity.pEntity = mapent;

	// No default flags/contents
	LoadEntity.nBaseFlags = 0;
	LoadEntity.nBaseContents = 0;

	//
	// Read the entity chunk.
	//
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadEntityKeyCallback, &LoadEntity);

	return eResult;
}


bool LoadEntsFromMapFile( char const *pFilename )
{
	//
	// Dummy this up for the texture handling. This can be removed when old .MAP file
	// support is removed.
	//
	g_nMapFileVersion = 400;

	//
	// Open the file.
	//
	CChunkFile File;
	ChunkFileResult_t eResult = File.Open( pFilename, ChunkFile_Read );

	if(eResult == ChunkFile_Ok)
	{
		num_entities = 0;

		//
		// Set up handlers for the subchunks that we are interested in.
		//
		CChunkHandlerMap Handlers;
		Handlers.AddHandler("entity", (ChunkHandler_t)LoadEntityCallback, 0);

		File.PushHandlers(&Handlers);

		//
		// Read the sub-chunks. We ignore keys in the root of the file.
		//
		while (eResult == ChunkFile_Ok)
		{
			eResult = File.ReadChunk();
		}

		File.PopHandlers();
		return true;
	}
	else
	{
		Error("Error in LoadEntsFromMapFile (in-memory file): %s.\n", File.GetErrorText(eResult));
		return false;
	}
}


