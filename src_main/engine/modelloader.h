//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( MOD_LOADER_H )
#define MOD_LOADER_H
#ifdef _WIN32
#pragma once
#endif

struct model_t;
class IMaterial;


#include "utlmemory.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class IModelLoader
{
public:
	typedef enum
	{
		// The name is allocated, but nothing else is in memory or being referenced
		FMODELLOADER_NOTLOADEDORREFERENCED = 0,
		// The model has been loaded into memory
		FMODELLOADER_LOADED		= (1<<0),
		// The model is being referenced by the server code
		FMODELLOADER_SERVER		= (1<<1),
		// The model is being referenced by the client code
		FMODELLOADER_CLIENT		= (1<<2),
		// The model is being referenced in the client .dll
		FMODELLOADER_CLIENTDLL	= (1<<3),
		// The model is being referenced by static props
		FMODELLOADER_STATICPROP	= (1<<4),
		// The model is a detail prop
		FMODELLOADER_DETAILPROP = (1<<5),

		FMODELLOADER_REFERENCEMASK = (FMODELLOADER_SERVER | FMODELLOADER_CLIENT | FMODELLOADER_CLIENTDLL | FMODELLOADER_STATICPROP | FMODELLOADER_DETAILPROP ),
	} REFERENCETYPE;

	// Start up modelloader subsystem
	virtual void		Init( void ) = 0;
	virtual void		Shutdown( void ) = 0;

	// Look up name for model
	virtual const char *GetName( model_t const *model ) = 0;

	// Check for extra data, reload studio model if needed
	virtual void		*GetExtraData( model_t *model ) = 0;

	// Get disk size for model
	virtual int			GetModelFileSize( char const *name ) = 0;

	// Finds the model, and loads it if it isn't already present.  Updates reference flags
	virtual model_t		*GetModelForName( const char *name, REFERENCETYPE referencetype ) = 0;
	virtual model_t		*ReferenceModel( const char *name, REFERENCETYPE referencetype ) = 0;
	// Unmasks the referencetype field for the model
	virtual void		ReleaseModel( model_t *model, REFERENCETYPE referencetype ) = 0;
	// Unmasks the specified reference type across all models
	virtual void		ReleaseAllModels( REFERENCETYPE referencetype ) = 0;

	// For any models with referencetype blank, frees all memory associated with the model
	//  and frees up the models slot
	virtual void		UnloadUnreferencedModels( void ) = 0;

	// Material system wants us to flush all studio models from system
	virtual void		Studio_ReleaseModels( void ) = 0;
	// Material system wants us to restore all studio models from system
	virtual void		Studio_RestoreModels( void ) = 0;

	// On the client only, there is some information that is computed at the time we are just
	//  about to render the map the first time.  If we don't change/unload the map, then we
	//  shouldn't have to recompute it each time we reconnect to the same map
	virtual bool		Map_GetRenderInfoAllocated( void ) = 0;
	virtual void		Map_SetRenderInfoAllocated( bool allocated ) = 0;

	// Load all the displacements for rendering. Set bRestoring to true if we're recovering from an alt+tab.
	virtual void		Map_LoadDisplacements( model_t *model, bool bRestoring ) = 0;

	// Print which models are in the cache/known
	virtual void		Print( void ) = 0;

	// Validate version/header of a .bsp file
	virtual bool		Map_IsValid( char const *mapname ) = 0;
};

extern IModelLoader *modelloader;

//-----------------------------------------------------------------------------
// Purpose: Loads the lump to temporary memory and automatically cleans up the
//  memory when it goes out of scope.
//-----------------------------------------------------------------------------

class CMapLoadHelper
{
public:
						CMapLoadHelper( int lumpToLoad );
						~CMapLoadHelper( void );

	// Get raw memory pointer
	byte				*LumpBase( void );
	int					LumpSize( void );
	int					LumpVersion() const;
	const char			*GetMapName( void );
	char				*GetLoadName( void );
	model_t				*GetMap( void );

	// Global setup/shutdown
	static void			Init( model_t *map, const char *loadname );
	static void			Shutdown( void );

	// Returns the size of a particular lump without loading it...
	static int			LumpSize( int lumpId );

	// Loads one element in a lump.
	static void			LoadLumpElement( int nLumpId, int nElemIndex, int nElemSize, void *pData );
	static void			LoadLumpData( int nLumpId, int offset, int size, void *pData );

private:

	byte				*m_pData;
	int					m_nLumpSize;
	int					m_nLumpVersion;

};

//-----------------------------------------------------------------------------
// Recomputes translucency for the model...
//-----------------------------------------------------------------------------

void Mod_RecomputeTranslucency( model_t* mod );

//-----------------------------------------------------------------------------
// game lumps
//-----------------------------------------------------------------------------

int Mod_GameLumpSize( int lumpId );
int Mod_GameLumpVersion( int lumpId );
bool Mod_LoadGameLump( int lumpId, void* pBuffer, int size );

// returns the material count...
int Mod_GetMaterialCount( model_t* mod );

// returns the first n materials.
void Mod_GetModelMaterials( model_t* mod, int count, IMaterial** ppMaterial );

bool Mod_LoadStudioModelVtxFileIntoTempBuffer( model_t *mod, CUtlMemory<unsigned char>& tmpVtxMem );

#endif // MOD_LOADER_H