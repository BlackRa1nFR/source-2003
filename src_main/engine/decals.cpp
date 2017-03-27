// decals.c
// functionality common to wad and decal code in gl_draw.c and draw.c
#include "quakedef.h"
#include "decal.h"
#include "decal_private.h"
#include "zone.h"
#include "sys.h"
#include "gl_texture.h"
#include "gl_matsysiface.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include <malloc.h>
#include "utldict.h"

struct DecalEntry
{
	IMaterial *material;
	int			index;
};

// This stores the list of all decals
CUtlDict< DecalEntry, int >	g_DecalDictionary;

// This is a list of indices into the dictionary.
// This list is indexed by network id, so it maps network ids to decal dictionary entries
CUtlVector< int >	g_DecalLookup;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int Draw_DecalMax( void )
{
	return MAX_DECALS;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Sets the name of the bitmap from decals.wad to be used in a specific slot #
// called from cl_parse.cpp twice
// This sets the name of a decal prototype texture
// Input  : decal - 
//			*name - 
//-----------------------------------------------------------------------------
// called from gl_rsurf.cpp
IMaterial *Draw_DecalMaterial( int index )
{
	if ( index < 0 || index >= g_DecalLookup.Count() )
		return NULL;

	int slot = g_DecalLookup[ index ];
	if ( slot < 0 || slot >= (int)g_DecalDictionary.Count() )
		return NULL;

	return g_DecalDictionary[ slot ].material;
}

#ifndef SWDS
void Draw_DecalSetName( int decal, char *name )
{
	while ( decal >= g_DecalLookup.Count() )
	{
		int idx = g_DecalLookup.AddToTail();
		g_DecalLookup[ idx ] = 0;
	}

	int lookup = g_DecalDictionary.Find( name );
	if ( lookup == g_DecalDictionary.InvalidIndex() )
	{
		DecalEntry entry;
		entry.material = GL_LoadMaterial( name );
		entry.index = decal;

		lookup = g_DecalDictionary.Insert( name, entry );
	}

	g_DecalLookup[ decal ] = lookup;
}



// called from cl_parse.cpp
// find the server side decal id given it's name.
// used for save/restore
int Draw_DecalIndexFromName( char *name )
{
	int lookup = g_DecalDictionary.Find( name );
	if ( lookup == g_DecalDictionary.InvalidIndex() )
		return 0;

	return g_DecalDictionary[ lookup ].index;
}
#endif

// This is called to reset all loaded decals
// called from cl_parse.cpp and host.cpp
void Decal_Init( void )
{
	g_DecalDictionary.RemoveAll();
	g_DecalLookup.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Decal_Shutdown( void )
{
	g_DecalLookup.Purge();

	int c = g_DecalDictionary.Count();

	int decal;
	for ( decal = 0; decal < c; decal++ )
	{
		IMaterial *mat = g_DecalDictionary[ decal ].material;
		if ( mat )
		{
			GL_UnloadMaterial( mat );
		}
	}

	g_DecalDictionary.Purge();
}

