//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Model loading / unloading interface
//
// $NoKeywords: $
//=============================================================================
#include "glquake.h"
#include "gl_model_private.h"
#include "modelgen.h"
#include "gl_matsysiface.h"
#include "gl_lightmap.h"
#include "gl_texture.h"
#include "utlvector.h"
#include "vstdlib/strtools.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "istudiorender.h"
#include "spritegn.h"
#include "zone.h"
#include "edict.h"
#include "cmodel_engine.h"
#include "cdll_engine_int.h"
#include "iscratchpad3d.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "dispchain.h"
#include "gl_rsurf.h"
#include "materialsystem/itexture.h"
#include "Overlay.h"
#include "utldict.h"
#include "mempool.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern	CGlobalVars g_ServerGlobalVariables;
extern	IMaterial	*g_materialEmpty;

//-----------------------------------------------------------------------------
// A dictionary used to store where to find game lump data in the .bsp file
//-----------------------------------------------------------------------------

static CUtlVector< dgamelump_t > g_GameLumpDict;
static char g_pGameLumpFile[128];

ConVar mat_loadtextures("mat_loadtextures", "1", 0 );

// FIXME/TODO:  Right now Host_FreeToLowMark unloads all models including studio
//  models that have Cache_Alloc data, too.  This needs to be fixed before shipping
//

//-----------------------------------------------------------------------------
// Purpose: Implements IModelLoader
//-----------------------------------------------------------------------------
class CModelLoader : public IModelLoader
{
// Implement IModelLoader interface
public:
	CModelLoader() : m_ModelPool( sizeof( model_t ), MAX_KNOWN_MODELS )
	{
	}

	void		Init( void );
	void		Shutdown( void );

	// Look up name for model
	const char	*GetName( model_t const *model );

	// Check cache for data, reload model if needed
	void		*GetExtraData( model_t *model );

	int			GetModelFileSize( char const *name );

	// Finds the model, and loads it if it isn't already present.  Updates reference flags
	model_t		*GetModelForName( const char *name, REFERENCETYPE referencetype );
	// Mark as referenced by name
	model_t		*ReferenceModel( const char *name, REFERENCETYPE referencetype );

	// Unmasks the referencetype field for the model
	void		ReleaseModel( model_t *model, REFERENCETYPE referencetype );
	// Unmasks the specified reference type across all models
	void		ReleaseAllModels( REFERENCETYPE referencetype );

	// For any models with referencetype blank, frees all memory associated with the model
	//  and frees up the models slot
	void		UnloadUnreferencedModels( void );

	void		Studio_ReleaseModels( void );
	void		Studio_RestoreModels( void );

	bool		Map_GetRenderInfoAllocated( void );
	void		Map_SetRenderInfoAllocated( bool allocated );

	virtual void	Map_LoadDisplacements( model_t *pModel, bool bRestoring );

	// Validate version/header of a .bsp file
	bool		Map_IsValid( char const *mapname );

	void		Print( void );
// Internal types
private:
	// TODO, flag these and allow for UnloadUnreferencedModels to check for allocation type
	//  so we don't have to flush all of the studio models when we free the hunk
	enum
	{
		FALLOC_USESHUNKALLOC = (1<<31),
		FALLOC_USESCACHEALLOC = (1<<30),
	};

// Internal methods
private:
	// Finds the model, and loads it if it isn't already present.  Updates reference flags
	model_t		*FindModel( const char *name );
	// Set reference flags and loag model if it's not present already
	model_t		*LoadModel( model_t	*model, REFERENCETYPE *referencetype );
	// Unload models ( won't unload referenced models if checkreferences is true )
	void		UnloadAllModels( bool checkreference );

	// World/map
	void		Map_LoadModel( model_t *mod );
	void		Map_UnloadModel( model_t *mod );
	void		Map_UnloadCubemapSamples( model_t *mod );

	// World loading helper
	void		SetWorldModel( model_t *mod );
	void		ClearWorldModel( void );
	bool		IsWorldModelSet( void );
	int			GetNumWorldSubmodels( void );

	// Sprites
	void		Sprite_LoadModel( model_t *mod );
	void		Sprite_UnloadModel( model_t *mod );

	// Studio models
	void		Studio_LoadModel( model_t *mod, void *buffer );
	void		Studio_UnloadModel( model_t *mod );
	void		Studio_LoadStaticMeshes( model_t* mod );

	// Internal data
private:
	enum 
	{
		MAX_KNOWN_MODELS = 1024,
	};

	struct ModelEntry
	{
		model_t *modelpointer;
	};

	CUtlDict< ModelEntry, int >	m_Models;

	CMemoryPool			m_ModelPool;

	model_t				m_InlineModels[ MAX_KNOWN_MODELS ];

	model_t				*m_pWorldModel;

	// For HUNK tags
	char				s_szLoadName[ 64 ];

	bool				m_bMapRenderInfoLoaded;
};

// Expose interface
static CModelLoader g_ModelLoader;
IModelLoader *modelloader = ( IModelLoader * )&g_ModelLoader;

//-----------------------------------------------------------------------------
// Globals used by the MapLoadHelper
//-----------------------------------------------------------------------------

static dheader_t	s_MapHeader;
static FileHandle_t	s_MapFile = (FileHandle_t)0;
static char			s_szLoadName[ 64 ];
static char			s_MapName[ 64 ];
static model_t		*s_pMap = NULL;
static int			s_nMapLoadRecursion = 0;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *map - 
//			*loadname - 
//-----------------------------------------------------------------------------
void CMapLoadHelper::Init( model_t *map, const char *loadname )
{
	if ( ++s_nMapLoadRecursion > 1 )
		return;

	s_pMap = NULL;
	s_szLoadName[ 0 ] = 0;
	s_MapFile = (FileHandle_t)0;
	memset( &s_MapHeader, 0, sizeof( s_MapHeader ) );

	if ( !map )
	{
		strcpy( s_MapName, loadname );
	}
	else
	{
		strcpy( s_MapName, map->name );
	}

	s_MapFile = g_pFileSystem->Open( s_MapName, "rb" );
	if ( s_MapFile == FILESYSTEM_INVALID_HANDLE )
	{
		Host_Error( "CMapLoadHelper::Init, unable to open %s\n", s_MapName );
		return;
	}

	g_pFileSystem->Read( &s_MapHeader, sizeof( dheader_t ), s_MapFile );
	if ( s_MapHeader.ident != IDBSPHEADER )
	{
		g_pFileSystem->Close( s_MapFile );
		s_MapFile = (FileHandle_t)0;
		Host_Error( "CMapLoadHelper::Init, map %s has wrong identifier\n", s_MapName );
		return;
	}

	if ( s_MapHeader.version != BSPVERSION )
	{
		g_pFileSystem->Close( s_MapFile );
		s_MapFile = (FileHandle_t)0;
		Host_Error( "CMapLoadHelper::Init, map %s has wrong version (%i when expecting %i)\n", s_MapName,
			s_MapHeader.version, BSPVERSION );
		return;
	}

	Assert( strlen(loadname) < 64 );
	strcpy( s_szLoadName, loadname );

	// Store map version
	g_ServerGlobalVariables.mapversion = s_MapHeader.mapRevision;

	s_pMap = map;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMapLoadHelper::Shutdown( void )
{
	if ( --s_nMapLoadRecursion > 0 )
		return;

	if ( s_MapFile )
	{
		g_pFileSystem->Close( s_MapFile );
	}

	s_MapFile = (FileHandle_t)0;
	s_szLoadName[ 0 ] = 0;
	memset( &s_MapHeader, 0, sizeof( s_MapHeader ) );
	s_pMap = NULL;
}


//-----------------------------------------------------------------------------
// Returns the size of a particular lump without loading it...
//-----------------------------------------------------------------------------
int CMapLoadHelper::LumpSize( int lumpId )
{
	// Load raw lump from disk
	lump_t *pLump = &s_MapHeader.lumps[ lumpId ];
	Assert( pLump );
	return pLump->filelen;
}

//-----------------------------------------------------------------------------
// Loads one element in a lump.
//-----------------------------------------------------------------------------
void CMapLoadHelper::LoadLumpElement( int nLumpId, int nElemIndex, int nElemSize, void *pData )
{
	// Load raw lump from disk
	lump_t *pLump = &s_MapHeader.lumps[ nLumpId ];
	Assert( pLump );

	g_pFileSystem->Seek( s_MapFile, pLump->fileofs + nElemSize * nElemIndex, FILESYSTEM_SEEK_HEAD );
	g_pFileSystem->Read( pData, nElemSize, s_MapFile );
}


//-----------------------------------------------------------------------------
// Loads one element in a lump.
//-----------------------------------------------------------------------------
void CMapLoadHelper::LoadLumpData( int nLumpId, int offset, int size, void *pData )
{
	// Load raw lump from disk
	lump_t *pLump = &s_MapHeader.lumps[ nLumpId ];
	Assert( pLump );

	g_pFileSystem->Seek( s_MapFile, pLump->fileofs + offset, FILESYSTEM_SEEK_HEAD );
	g_pFileSystem->Read( pData, size, s_MapFile );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mapfile - 
//			lumpToLoad - 
//-----------------------------------------------------------------------------
CMapLoadHelper::CMapLoadHelper( int lumpToLoad )
{
	if ( s_MapHeader.version != BSPVERSION )
	{
		Sys_Error( "Map version is %i, expecting %i!!!",
			s_MapHeader.version, BSPVERSION );
	}

	m_nLumpSize = 0;
	m_pData = NULL;

	if ( s_MapFile == FILESYSTEM_INVALID_HANDLE )
	{
		Sys_Error( "Can't load map from invalid handle!!!" );
	}

	if ( lumpToLoad < 0 || lumpToLoad >= HEADER_LUMPS )
	{
		Sys_Error( "Can't load lump %i, range is 0 to %i!!!", lumpToLoad, HEADER_LUMPS - 1 );
	}

	// Load raw lump from disk
	lump_t *lump = &s_MapHeader.lumps[ lumpToLoad ];
	Assert( lump );

	m_nLumpSize = lump->filelen;
	m_nLumpVersion = lump->version;

	// At least one byte

	m_pData = (byte *)new byte[ m_nLumpSize + 1 ];
	if ( !m_pData )
	{
		Sys_Error( "Can't load lump %i, allocation of %i bytes failed!!!", lumpToLoad, m_nLumpSize + 1 );
	}

	g_pFileSystem->Seek( s_MapFile, lump->fileofs, FILESYSTEM_SEEK_HEAD );
	g_pFileSystem->Read( m_pData, lump->filelen, s_MapFile );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapLoadHelper::~CMapLoadHelper( void )
{
	// Wipe out previous
	delete[] m_pData;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : model_t
//-----------------------------------------------------------------------------
model_t *CMapLoadHelper::GetMap( void )
{
	Assert( s_pMap );
	return s_pMap;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CMapLoadHelper::GetMapName( void )
{
	return s_MapName;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
char *CMapLoadHelper::GetLoadName( void )
{
	return s_szLoadName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : byte
//-----------------------------------------------------------------------------
byte *CMapLoadHelper::LumpBase( void )
{
	return m_pData;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CMapLoadHelper::LumpSize()
{
	return m_nLumpSize;
}

int	CMapLoadHelper::LumpVersion() const
{
	return m_nLumpVersion;
}


//-----------------------------------------------------------------------------
// Allocates, frees lighting data
//-----------------------------------------------------------------------------
static void AllocateLightingData( model_t *mod, int nSize )
{
	// Specifically *not* adding it to the hunk.
	mod->brush.lightdata = (colorRGBExp32 *)malloc( nSize );
}

static void DeallocateLightingData( model_t *mod )
{
	if ( mod && mod->brush.lightdata )
	{
		free( mod->brush.lightdata );
		mod->brush.lightdata = NULL;
	}
}



static int ComputeLightmapSize( dface_t *pFace, mtexinfo_t *pTexInfo )
{
	bool bNeedsBumpmap = false;
	if( pTexInfo[pFace->texinfo].flags & SURF_BUMPLIGHT )
	{
		bNeedsBumpmap = true;
	}

    int lightstyles;
    for (lightstyles=0; lightstyles < MAXLIGHTMAPS; lightstyles++ )
    {
        if ( pFace->styles[lightstyles] == 255 )
            break;
    }

	int nLuxels = (pFace->m_LightmapTextureSizeInLuxels[0]+1) * (pFace->m_LightmapTextureSizeInLuxels[1]+1);
	if( bNeedsBumpmap )
	{
		return nLuxels * 4 * lightstyles * ( NUM_BUMP_VECTS + 1 );
	}

	return nLuxels * 4 * lightstyles;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadLighting()
{
	CMapLoadHelper lh( LUMP_LIGHTING );

	if ( !lh.LumpSize() )
	{
		lh.GetMap()->brush.lightdata = NULL;
		return;
	}

	Assert( lh.LumpSize() % sizeof( colorRGBExp32 ) == 0 );

	// BSP_VERSION_CHANGE: Start removing here! when the BSP version is upgraded to 19
	if ( lh.LumpVersion() == 0 )
	{
		// Need to interleave average light colors + light data...
		int nFaceCount;

		CMapLoadHelper lhf( LUMP_FACES );
		nFaceCount = lhf.LumpSize() / sizeof(dface_t);

		int nLightingDataSize = lh.LumpSize() + nFaceCount * MAXLIGHTMAPS * sizeof(colorRGBExp32);

		AllocateLightingData( lh.GetMap(), nLightingDataSize );

		// Leave room for the avg colors...
		int nLastLightOfs = -1;
		int nLightFaceCount = 0;
		dface_t *in = (dface_t*)lhf.LumpBase();
		unsigned char *out = (unsigned char*)lh.GetMap()->brush.lightdata;
		for ( int i = 0; i < nFaceCount; ++i, ++in )
		{
			int ofs = LittleLong( in->lightofs );
			if ( ofs == -1 )
				continue;

			++nLightFaceCount;

			Assert( in->lightofs >= ofs );
			nLastLightOfs = ofs;
			
			int nLightmapSize = ComputeLightmapSize( in, lh.GetMap()->brush.texinfo );
			memcpy( out + in->lightofs + MAXLIGHTMAPS * nLightFaceCount * sizeof(colorRGBExp32), lh.LumpBase() + in->lightofs, nLightmapSize );
		}
	}
	else
	// BSP_VERSION_CHANGE: End removing here! when the BSP version is upgraded to 19
	{
		AllocateLightingData( lh.GetMap(), lh.LumpSize() );
		memcpy( lh.GetMap()->brush.lightdata, lh.LumpBase(), lh.LumpSize());
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadWorldlights( void )
{
	CMapLoadHelper lh( LUMP_WORLDLIGHTS );

	if (!lh.LumpSize())
	{
		lh.GetMap()->brush.numworldlights = 0;
		lh.GetMap()->brush.worldlights = NULL;
		return;
	}
	lh.GetMap()->brush.numworldlights = lh.LumpSize() / sizeof( dworldlight_t );
	lh.GetMap()->brush.worldlights = (dworldlight_t *)Hunk_AllocName( lh.LumpSize(), lh.GetLoadName() );
	memcpy (lh.GetMap()->brush.worldlights, lh.LumpBase(), lh.LumpSize());

	// Fixup for backward compatability
	for ( int i = 0; i < lh.GetMap()->brush.numworldlights; i++ )
	{
		if( lh.GetMap()->brush.worldlights[i].type == emit_spotlight)
		{
			if ((lh.GetMap()->brush.worldlights[i].constant_attn == 0.0) && 
				(lh.GetMap()->brush.worldlights[i].linear_attn == 0.0) && 
				(lh.GetMap()->brush.worldlights[i].quadratic_attn == 0.0))
			{
				lh.GetMap()->brush.worldlights[i].quadratic_attn = 1.0;
			}

			if (lh.GetMap()->brush.worldlights[i].exponent == 0.0)
				lh.GetMap()->brush.worldlights[i].exponent = 1.0;
		}
		else if( lh.GetMap()->brush.worldlights[i].type == emit_point)
		{
			// To match earlier lighting, use quadratic...
			if ((lh.GetMap()->brush.worldlights[i].constant_attn == 0.0) && 
				(lh.GetMap()->brush.worldlights[i].linear_attn == 0.0) && 
				(lh.GetMap()->brush.worldlights[i].quadratic_attn == 0.0))
			{
				lh.GetMap()->brush.worldlights[i].quadratic_attn = 1.0;
			}
		}

   		// I replaced the cuttoff_dot field (which took a value from 0 to 1)
		// with a max light radius. Radius of less than 1 will never happen,
		// so I can get away with this. When I set radius to 0, it'll 
		// run the old code which computed a radius
		if (lh.GetMap()->brush.worldlights[i].radius < 1)
			lh.GetMap()->brush.worldlights[i].radius = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadVertices( void )
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	CMapLoadHelper lh( LUMP_VERTEXES );

	in = (dvertex_t *)lh.LumpBase();
	if ( lh.LumpSize() % sizeof(*in) )
	{
		Host_Error( "Mod_LoadVertices: funny lump size in %s", lh.GetMapName() );
	}
	count = lh.LumpSize() / sizeof(*in);
	out = (mvertex_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );

	lh.GetMap()->brush.vertexes = out;
	lh.GetMap()->brush.numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mins - 
//			maxs - 
// Output : float
//-----------------------------------------------------------------------------
static float RadiusFromBounds (Vector& mins, Vector& maxs)
{
	int		i;
	Vector	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength( corner );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadSubmodels( void )
{
	dmodel_t	*in;
	mmodel_t		*out;
	int			i, j, count;

	CMapLoadHelper lh( LUMP_MODELS );

	in = (dmodel_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error("Mod_LoadSubmodels: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (mmodel_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );

	lh.GetMap()->brush.submodels = out;
	lh.GetMap()->brush.numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		out->radius = RadiusFromBounds (out->mins, out->maxs);
		out->headnode = LittleLong (in->headnode);
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : medge_t *Mod_LoadEdges
//-----------------------------------------------------------------------------
medge_t *Mod_LoadEdges ( void )
{
	dedge_t *in;
	medge_t *out;
	int 	i, count;

	CMapLoadHelper lh( LUMP_EDGES );

	in = (dedge_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadEdges: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	medge_t *pedges = new medge_t[count];

	out = pedges;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}

	// delete this in the loader
	return pedges;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadOcclusion( void )
{
	CMapLoadHelper lh( LUMP_OCCLUSION );

	brushdata_t *b = &lh.GetMap()->brush;
	b->numoccluders = 0;
	b->occluders = NULL;
	b->numoccluderpolys = 0;
	b->occluderpolys = NULL;
	b->numoccludervertindices = 0;
	b->occludervertindices = NULL;

	if ( !lh.LumpSize() )
	{
		return;
	}

	CUtlBuffer buf( lh.LumpBase(), lh.LumpSize() );

	switch( lh.LumpVersion() )
	{
	case LUMP_OCCLUSION_VERSION:
		{
			b->numoccluders = buf.GetInt();
			if (b->numoccluders)
			{
				int nSize = b->numoccluders * sizeof(doccluderdata_t);
				b->occluders = (doccluderdata_t*)Hunk_AllocName( nSize, "occluder data" );
				buf.Get( b->occluders, nSize );
			}

			b->numoccluderpolys = buf.GetInt();
			if (b->numoccluderpolys)
			{
				int nSize = b->numoccluderpolys * sizeof(doccluderpolydata_t);
				b->occluderpolys = (doccluderpolydata_t*)Hunk_AllocName( nSize, "occluder poly data" );
				buf.Get( b->occluderpolys, nSize );
			}

			b->numoccludervertindices = buf.GetInt();
			if (b->numoccludervertindices)
			{
				int nSize = b->numoccludervertindices * sizeof(int);
				b->occludervertindices = (int*)Hunk_AllocName( nSize, "occluder vertices" );
				buf.Get( b->occludervertindices, nSize );
			}
		}
		break;

	case 0:
		break;

	default:
		Host_Error("Invalid occlusion lump version!\n");
		break;
	}
}



// UNDONE: Really, it's stored 2 times because the texture system keeps a 
// copy of the name too.  I guess we'll get rid of this when we have a material
// system that works without a graphics context.  At that point, everyone can
// reference the name in the material, or just the material itself.
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadTexdata( void )
{
	// Don't bother loading these again; they're already stored in the collision model
	// which is guaranteed to be loaded at this point
	s_pMap->brush.numtexdata = GetCollisionBSPData()->numtextures;
	s_pMap->brush.texdata = GetCollisionBSPData()->map_surfaces.Base();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadTexinfo( void )
{
	texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count;
	// UNDONE: Fix this

	CMapLoadHelper lh( LUMP_TEXINFO );

	in = (texinfo_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadTexinfo: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (mtexinfo_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );

	lh.GetMap()->brush.texinfo = out;
	lh.GetMap()->brush.numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0; j<2; ++j)
		{
			for (int k=0 ; k<4 ; ++k)
			{
				out->textureVecsTexelsPerWorldUnits[j][k] = 
					LittleFloat( in->textureVecsTexelsPerWorldUnits[j][k] );
				out->lightmapVecsLuxelsPerWorldUnits[j][k] =
					LittleFloat( in->lightmapVecsLuxelsPerWorldUnits[j][k] );
			}
		}

		// assume that the scale is the same on both s and t.
		out->luxelsPerWorldUnit = VectorLength( out->lightmapVecsLuxelsPerWorldUnits[0].AsVector3D() );
		out->worldUnitsPerLuxel = 1.0f / out->luxelsPerWorldUnit;

		out->flags = LittleLong (in->flags);

		if(mat_loadtextures.GetInt())
		{
			if( in->texdata >= 0 )
			{
				out->material = GL_LoadMaterial( lh.GetMap()->brush.texdata[ in->texdata ].name );
			}
			else
			{
				Con_Printf( "Mod_LoadTexinfo: texdata < 0\n" );
				out->material = NULL;
			}
//			Con_Printf( "lh.GetMap()->brush.texdata[%d].name = %s\n", ( int )in->texdata, lh.GetMap()->brush.texdata[ in->texdata ].name );
			if(!out->material)
			{
				out->material = g_materialEmpty;
				g_materialEmpty->IncrementReferenceCount();
			}
 		}
		else
		{
			out->material = g_materialEmpty;
			g_materialEmpty->IncrementReferenceCount();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loadmodel - 
//			*s - 
// Output : void CalcSurfaceExtents
//-----------------------------------------------------------------------------
static void CalcSurfaceExtents ( CMapLoadHelper& lh, int surfID )
{
	float	textureMins[2], textureMaxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	textureMins[0] = textureMins[1] = 999999;
	textureMaxs[0] = textureMaxs[1] = -99999;

	model_t *pModel = lh.GetMap();
	tex = MSurf_TexInfo( surfID, pModel );
	
	for (i=0 ; i<MSurf_VertCount( surfID, pModel ); i++)
	{
		e = pModel->brush.vertindices[MSurf_FirstVertIndex( surfID, pModel )+i];
		v = &pModel->brush.vertexes[e];
		
		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->textureVecsTexelsPerWorldUnits[j][0] + 
				  v->position[1] * tex->textureVecsTexelsPerWorldUnits[j][1] +
				  v->position[2] * tex->textureVecsTexelsPerWorldUnits[j][2] +
				  tex->textureVecsTexelsPerWorldUnits[j][3];
			if (val < textureMins[j])
				textureMins[j] = val;
			if (val > textureMaxs[j])
				textureMaxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{	
		if( MSurf_LightmapExtents( surfID, pModel )[i] == 0 )
		{
			MSurf_Flags( surfID, pModel ) |= SURFDRAW_NOLIGHT;
		}

		bmins[i] = Float2Int( textureMins[i] );
		bmaxs[i] = Ceil2Int( textureMaxs[i] );
		MSurf_TextureMins( surfID, pModel )[i] = bmins[i];
		MSurf_TextureExtents( surfID, pModel )[i] = ( bmaxs[i] - bmins[i] );

		if ( !(tex->flags & SURF_NOLIGHT) && MSurf_LightmapExtents( i, pModel )[i] > MSurf_MaxLightmapSizeWithBorder( surfID, pModel ) )
		{
			Sys_Error ("Bad surface extents on texture %s", tex->material->GetName() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pModel - 
//			*pLump - 
//-----------------------------------------------------------------------------
void Mod_LoadOrigFaces( void )
{
/*
	CMapLoadHelper lh( LUMP_ORIGINALFACES );

    // get a pointer to the original face data
    dface_t *origfaces = ( dface_t* )lh.LumpBase();
    
    //
    // verify dispinfo data size
    //
    if( lh.LumpSize() % sizeof( dface_t ) )
        Host_Error( "Mod_LoadOrigFaces: funny lump size in %s!\n", lh.GetMapName() );

	model_t *pModel = lh.GetMap();

    //
    // get number of original faces and allocate memory
    //
    pModel->brush.numOrigSurfaces = lh.LumpSize() / sizeof( dface_t );
    if( !( pModel->brush.pOrigSurfaces = new msurface_t[pModel->brush.numOrigSurfaces] ) )
	{
        Sys_Error( "Mod_LoadOrigFaces: couldn't allocate memory for the original surfaces!\n" );
	}

    //
    // load all of the origianl surface face data
    //
    for( int i = 0; i < pModel->brush.numOrigSurfaces; i++, origfaces++ )
    {
        //
        // get surface info
        //
		memset( &pModel->brush.pOrigSurfaces[i], 0, sizeof( msurface_t ) );
		// HACKHACK: As long as we do this in order, then vertindex == index into surfedges
        pModel->brush.pOrigSurfaces[i].firstvertindex = origfaces->firstedge;
        pModel->brush.pOrigSurfaces[i].vertCount = origfaces->numedges;

        pModel->brush.pOrigSurfaces[i].flags = 0;
        pModel->brush.pOrigSurfaces[i].origSurfaceID = -1;
		pModel->brush.pOrigSurfaces[i].pdecals = NULL;

		// No shadows on the surface to start with
		pModel->brush.pOrigSurfaces[i].m_ShadowDecals = SHADOW_DECAL_HANDLE_INVALID;

        //
        // get plane info
        //
        pModel->brush.pOrigSurfaces[i].plane = pModel->brush.planes + origfaces->planenum;

        //
        // get material info
        //
        pModel->brush.pOrigSurfaces[i].texinfo = pModel->brush.texinfo + origfaces->texinfo;
        int index = origfaces->dispinfo;
        if( index == -1 )
        {
            pModel->brush.pOrigSurfaces[i].pDispInfo = NULL;
        }
        else
        {
            pModel->brush.pOrigSurfaces[i].pDispInfo = DispInfo_IndexArray( pModel->brush.hDispInfos, index );
        }

        // calculate the surface extents
        if( pModel->brush.pOrigSurfaces[i].pDispInfo )
        {
            CalcSurfaceExtents( lh, &pModel->brush.pOrigSurfaces[i] );
        }

        //
        // lighting info
        //
        for( int j = 0; j < MAXLIGHTMAPS; j++ )
        {
            if( j != 0 )
            {
                pModel->brush.pOrigSurfaces[i].styles[j] = 255;
                continue;
            }
            pModel->brush.pOrigSurfaces[i].styles[j] = origfaces->styles[j];

        }
        index = origfaces->lightofs;
        if( index == -1 )
        {
            pModel->brush.pOrigSurfaces[i].samples = NULL;
        }
        else
        {
            pModel->brush.pOrigSurfaces[i].samples = ( colorRGBExp32* )( ( ( byte* )pModel->brush.lightdata ) + index );
        }

        //
        // surface flags
        //
        if( pModel->brush.pOrigSurfaces[i].texinfo->flags & SURF_NOLIGHT )
        {
            pModel->brush.pOrigSurfaces[i].flags |= SURFDRAW_NOLIGHT;
        }

        if( pModel->brush.pOrigSurfaces[i].texinfo->flags & SURF_NODRAW )
        {
            pModel->brush.pOrigSurfaces[i].flags |= SURFDRAW_NODRAW;
        }

        if( pModel->brush.pOrigSurfaces[i].texinfo->flags & SURF_WARP )
        {
            pModel->brush.pOrigSurfaces[i].flags |= SURFDRAW_WATERSURFACE;
        }

        if( pModel->brush.pOrigSurfaces[i].texinfo->flags & SURF_SKY )
        {
            pModel->brush.pOrigSurfaces[i].flags |= SURFDRAW_SKY;
        }
	}
	*/
}

//-----------------------------------------------------------------------------
// Input  : *pModel - 
//			*pLump - 
//			*loadname - 
//-----------------------------------------------------------------------------
void Mod_LoadVertNormals( void )
{
	CMapLoadHelper lh( LUMP_VERTNORMALS );

    // get a pointer to the vertex normal data.
	Vector *pVertNormals = ( Vector * )lh.LumpBase();

    //
    // verify vertnormals data size
    //
    if( lh.LumpSize() % sizeof( *pVertNormals ) )
        Host_Error( "Mod_LoadVertNormals: funny lump size in %s!\n", lh.GetMapName() );

	int count = lh.LumpSize() / sizeof(*pVertNormals);
	
	Vector *out = (Vector *)Hunk_AllocName( lh.LumpSize(), lh.GetLoadName() );
	memcpy( out, pVertNormals, lh.LumpSize() );
	
	lh.GetMap()->brush.vertnormals = out;
	lh.GetMap()->brush.numvertnormals = count;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadVertNormalIndices( void )
{
	CMapLoadHelper lh( LUMP_VERTNORMALINDICES );

    // get a pointer to the vertex normal data.
	unsigned short *pIndices = ( unsigned short * )lh.LumpBase();

	int count = lh.LumpSize() / sizeof(*pIndices);
	
	unsigned short *out = (unsigned short *)Hunk_AllocName( lh.LumpSize(), lh.GetLoadName() );
	memcpy( out, pIndices, lh.LumpSize() );
	
	lh.GetMap()->brush.vertnormalindices = out;
	lh.GetMap()->brush.numvertnormalindices = count;


	// OPTIMIZE: Water surfaces don't need vertex normals?
	int normalIndex = 0;
	for( int i = 0; i < lh.GetMap()->brush.numsurfaces; i++ )
	{
		MSurf_FirstVertNormal( i, lh.GetMap() ) = normalIndex;
		normalIndex += MSurf_VertCount( i, lh.GetMap() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loadmodel - 
//			*l - 
//			*loadname - 
//-----------------------------------------------------------------------------
void Mod_LoadPrimitives( void )
{
	dprimitive_t	*in;
	mprimitive_t	*out;
	int				i, count;

	CMapLoadHelper lh( LUMP_PRIMITIVES );

	in = (dprimitive_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadPrimitives: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (mprimitive_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
	memset( out, 0, count * sizeof( mprimitive_t ) );

	lh.GetMap()->brush.primitives = out;
	lh.GetMap()->brush.numprimitives = count;
	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->firstIndex		= in->firstIndex;
		out->firstVert		= in->firstVert;
		out->indexCount		= in->indexCount;
		out->type			= in->type;
		out->vertCount		= in->vertCount;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loadmodel - 
//			*l - 
//			*loadname - 
//-----------------------------------------------------------------------------
void Mod_LoadPrimVerts( void )
{
	dprimvert_t		*in;
	mprimvert_t		*out;
	int				i, count;

	CMapLoadHelper lh( LUMP_PRIMVERTS );

	in = (dprimvert_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadPrimVerts: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (mprimvert_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
	memset( out, 0, count * sizeof( mprimvert_t ) );

	lh.GetMap()->brush.primverts = out;
	lh.GetMap()->brush.numprimverts = count;
	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->pos				= in->pos;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loadmodel - 
//			*l - 
//			*loadname - 
//-----------------------------------------------------------------------------
void Mod_LoadPrimIndices( void )
{
	unsigned short	*in;
	unsigned short	*out;
	int				i, count;

	CMapLoadHelper lh( LUMP_PRIMINDICES );

	in = (unsigned short *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadPrimIndices: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (unsigned short *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
	memset( out, 0, count * sizeof( unsigned short ) );

	lh.GetMap()->brush.primindices = out;
	lh.GetMap()->brush.numprimindices = count;
	for ( i=0 ; i<count ; i++, in++, out++)
	{
		*out = *in;
	}
}


// This allocates memory for a lump and copies the lump data in.
void Mod_LoadLump( 
	model_t *loadmodel, 
	int iLump,
	char *loadname, 
	int elementSize,
	void **ppData, 
	int *nElements )
{
	CMapLoadHelper lh( iLump );

	if( lh.LumpSize() % elementSize )
		Host_Error ("Mod_LoadLump: funny lump size in %s", loadmodel->name);

	// How many elements?
	*nElements = lh.LumpSize() / elementSize;

	// Make room for the data and copy the data in.
	*ppData = Hunk_AllocName( lh.LumpSize(), loadname );
	memcpy( *ppData, lh.LumpBase(), lh.LumpSize() );
}


// BSP_VERSION_CHANGE: Start removing here! when the BSP version is upgraded to 19

//-----------------------------------------------------------------------------
// Sets up the msurfacelighting_t structure
//-----------------------------------------------------------------------------
bool Mod_LoadSurfaceLightingV0( msurfacelighting_t *pLighting, dface_t *in, colorRGBExp32 *pLightData, int &nLightFaceCount )
{
	// Get lightmap extents from the file.
	pLighting->m_pLightmapExtents[0] = in->m_LightmapTextureSizeInLuxels[0];
	pLighting->m_pLightmapExtents[1] = in->m_LightmapTextureSizeInLuxels[1];
	pLighting->m_pLightmapMins[0] = in->m_LightmapTextureMinsInLuxels[0];
	pLighting->m_pLightmapMins[1] = in->m_LightmapTextureMinsInLuxels[1];

	int ofs = LittleLong(in->lightofs);
	if ( (ofs == -1) || (!pLightData) )
	{
		pLighting->m_pSamples = NULL;
		
		// Can't have *any* lightstyles if we have no samples....
		for ( int i=0; i<MAXLIGHTMAPS; ++i)
		{
			pLighting->m_nStyles[i] = 255;
		}
		return false;
	}

	// In v0, we always reserve 4 spaces for avg light color...
	++nLightFaceCount;
	pLighting->m_pSamples = (colorRGBExp32 *)( (byte *)pLightData + ofs + (4*nLightFaceCount*sizeof(colorRGBExp32)) );

	// Copy over average intensity data
	for (int i=0 ; i<MAXLIGHTMAPS; ++i)
	{
		pLighting->m_nStyles[i] = in->styles[i];
		*(pLighting->m_pSamples - i - 1) = in->m_AvgLightColor[i];
	}

	return ((pLighting->m_nStyles[0] != 0) && (pLighting->m_nStyles[0] != 255)) || (pLighting->m_nStyles[1] != 255);
}

// BSP_VERSION_CHANGE: End removing here! when the BSP version is upgraded to 19


//-----------------------------------------------------------------------------
// Sets up the msurfacelighting_t structure
//-----------------------------------------------------------------------------
bool Mod_LoadSurfaceLightingV1( msurfacelighting_t *pLighting, dface_t *in, colorRGBExp32 *pBaseLightData )
{
	// Get lightmap extents from the file.
	pLighting->m_pLightmapExtents[0] = in->m_LightmapTextureSizeInLuxels[0];
	pLighting->m_pLightmapExtents[1] = in->m_LightmapTextureSizeInLuxels[1];
	pLighting->m_pLightmapMins[0] = in->m_LightmapTextureMinsInLuxels[0];
	pLighting->m_pLightmapMins[1] = in->m_LightmapTextureMinsInLuxels[1];

	int i = LittleLong(in->lightofs);
	if ( (i == -1) || (!pBaseLightData) )
	{
		pLighting->m_pSamples = NULL;

		// Can't have *any* lightstyles if we have no samples....
		for ( int i=0; i<MAXLIGHTMAPS; ++i)
		{
			pLighting->m_nStyles[i] = 255;
		}
	}
	else
	{
		pLighting->m_pSamples = (colorRGBExp32 *)( ((byte *)pBaseLightData) + i );

		for (i=0 ; i<MAXLIGHTMAPS; ++i)
		{
			pLighting->m_nStyles[i] = in->styles[i];
		}
	}

	return ((pLighting->m_nStyles[0] != 0) && (pLighting->m_nStyles[0] != 255)) || (pLighting->m_nStyles[1] != 255);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loadmodel - 
//			*l - 
//			*loadname - 
//-----------------------------------------------------------------------------
void Mod_LoadFaces( void )
{
	dface_t		*in;
	msurface1_t 	*out1;
	msurface2_t 	*out2;
	int			count, surfnum;
	int			planenum;
	int			ti, di;
	msurfacelighting_t *pLighting;

	CMapLoadHelper lh( LUMP_FACES );

	// hack - this should cause material loads, and I shouldn't have to do UpdateConfig here.
	UpdateMaterialSystemConfig();
	Con_DPrintf( "Begin loading faces (loads materials)\n" );
	
	in = (dface_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadFaces: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);

	// align to the nearest 32
	out1 = (msurface1_t *)Hunk_AllocName( 32 + count*sizeof(*out1), lh.GetLoadName() );
	out2 = (msurface2_t *)Hunk_AllocName( 32 + count*sizeof(*out2), lh.GetLoadName() );
	pLighting = (msurfacelighting_t *)Hunk_AllocName( 32 + count*sizeof(msurfacelighting_t), lh.GetLoadName() );

	out1 = ( msurface1_t * )( ( ( ( unsigned long )out1 ) + 31 ) & ~31 );
	out2 = ( msurface2_t * )( ( ( ( unsigned long )out2 ) + 31 ) & ~31 );
	pLighting = ( msurfacelighting_t * )( ( ( ( unsigned long )pLighting ) + 31 ) & ~31 );
	memset( out1, 0, count * sizeof( *out1 ) );
	memset( out2, 0, count * sizeof( *out2 ) );
	memset( pLighting, 0, count * sizeof( msurfacelighting_t ) );

	lh.GetMap()->brush.surfaces1 = out1;
	lh.GetMap()->brush.surfaces2 = out2;
	lh.GetMap()->brush.numsurfaces = count;
	lh.GetMap()->brush.surfacelighting = pLighting;

	model_t *pModel = lh.GetMap();
	
	// BSP_VERSION_CHANGE: Start removing here! when the BSP version is upgraded to 19
	int nLightFaceCount = 0;
	// BSP_VERSION_CHANGE: End removing here! when the BSP version is upgraded to 19

	for ( surfnum=0 ; surfnum<count ; ++surfnum, ++in, ++out1, ++out2, ++pLighting )
	{
		MSurf_FirstVertIndex( surfnum, pModel )  = LittleLong(in->firstedge);
		MSurf_VertCount( surfnum, pModel ) = LittleShort(in->numedges);		
		MSurf_Flags( surfnum, pModel ) = 0;

		planenum = (unsigned short)LittleShort(in->planenum);
		
		if ( in->onNode )
		{
			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_NODE;
		}
		if ( in->side )
		{
			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_PLANEBACK;
		}

		out2->plane = lh.GetMap()->brush.planes + planenum;

		ti = LittleShort (in->texinfo);
		if (ti < 0 || ti >= lh.GetMap()->brush.numtexinfo)
			Host_Error ("Mod_LoadFaces: bad texinfo number");
		out1->texinfo = lh.GetMap()->brush.texinfo + ti;

		// lighting info
		
		// BSP_VERSION_CHANGE: Start removing here! when the BSP version is upgraded to 19
		bool bHasLightstyles;
		if ( lh.LumpVersion() == 1 )
		{
			bHasLightstyles = Mod_LoadSurfaceLightingV1( pLighting, in, lh.GetMap()->brush.lightdata );
		}
		else
		{
			bHasLightstyles = Mod_LoadSurfaceLightingV0( pLighting, in, lh.GetMap()->brush.lightdata, nLightFaceCount );
		}

		if ( bHasLightstyles )
		{
			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_HASLIGHTSYTLES;
		}
		// BSP_VERSION_CHANGE: End removing here! when the BSP version is upgraded to 19

		// BSP_VERSION_CHANGE: Start uncommenting here! when the BSP version is upgraded to 19
//		if ( Mod_LoadSurfaceLightingV1( pLighting, in, lh.GetMap()->brush.lightdata ) )
//		{
//			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_HASLIGHTSYTLES;
//		}
		// BSP_VERSION_CHANGE: End uncommenting here! when the BSP version is upgraded to 19

		// set the drawing flags flag
		if ( out1->texinfo->flags & SURF_NOLIGHT )
		{
			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_NOLIGHT;
		}

		// skip these faces
		if ( out1->texinfo->flags & SURF_NODRAW )
		{
			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_NODRAW;
		}
		
		// big hack!
		if( !out1->texinfo->material )
		{
			out1->texinfo->material = g_materialEmpty;
			g_materialEmpty->IncrementReferenceCount();
		}

		// Deactivate culling if the material is two sided
		if (out1->texinfo->material->IsTwoSided())
		{
			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_NOCULL;
		}

		if ( out1->texinfo->flags & SURF_WARP )
		{
			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_WATERSURFACE;
			MSurf_FogVolumeID( surfnum, pModel ) = in->surfaceFogVolumeID;
		}
		else
		{
			out1->fogVolumeID = -1;
		}

		if ( out1->texinfo->flags & SURF_SKY )
		{
			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_SKY;
		}

		if( out1->texinfo->flags & SURF_TRANS )
		{
			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_TRANS;
		}

        di = LittleShort( in->dispinfo );
		out1->pDispInfo = NULL;
        if( di != -1 )
        {
//            out->origSurfaceID = LittleLong( in->origFace );
			MSurf_Flags( surfnum, pModel ) |= SURFDRAW_HAS_DISP;
        }
		out1->numPrims = in->numPrims;
		out1->firstPrimID = in->firstPrimID;
		
		// No shadows on the surface to start with
		out2->m_ShadowDecals = SHADOW_DECAL_HANDLE_INVALID;

		// No overlays on the surface to start with
		out2->m_nFirstOverlayFragment = OVERLAY_FRAGMENT_INVALID;

		CalcSurfaceExtents ( lh, surfnum );
	}
	Con_DPrintf( "End loading faces (loads materials)\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *node - 
//			*parent - 
// Output : void Mod_SetParent
//-----------------------------------------------------------------------------
void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents >= 0)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}


//-----------------------------------------------------------------------------
// Mark an entire subtree as being too small to bother with
//-----------------------------------------------------------------------------
static void MarkSmallNode( mnode_t *node )
{
	if (node->contents >= 0)
		return;
	node->contents = -2;
	MarkSmallNode (node->children[0]);
	MarkSmallNode (node->children[1]);
}

static void CheckSmallVolumeDifferences( mnode_t *pNode, const Vector &parentSize )
{
	if (pNode->contents >= 0)
		return;

	Vector delta;
	VectorSubtract( parentSize, pNode->m_vecHalfDiagonal, delta );

	if ((delta.x < 5) && (delta.y < 5) && (delta.z < 5))
	{
		pNode->contents = -3;
		CheckSmallVolumeDifferences( pNode->children[0], parentSize );
		CheckSmallVolumeDifferences( pNode->children[1], parentSize );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loadmodel - 
//			*l - 
//			*loadname - 
//-----------------------------------------------------------------------------
void Mod_LoadNodes( void )
{
	Vector mins( 0, 0, 0 ), maxs( 0, 0, 0 );
	int			i, j, count, p;
	dnode_t		*in;
	mnode_t 	*out;

	CMapLoadHelper lh( LUMP_NODES );

	in = (dnode_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadNodes: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (mnode_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );

	lh.GetMap()->brush.nodes = out;
	lh.GetMap()->brush.numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			mins[j] = LittleShort (in->mins[j]);
			maxs[j] = LittleShort (in->maxs[j]);
		}
	
		VectorAdd( mins, maxs, out->m_vecCenter );
		out->m_vecCenter *= 0.5f;
		VectorSubtract( maxs, out->m_vecCenter, out->m_vecHalfDiagonal );

		p = LittleLong(in->planenum);
		out->plane = lh.GetMap()->brush.planes + p;

		out->firstsurface = (unsigned short)LittleShort (in->firstface);
		out->numsurfaces = (unsigned short)LittleShort (in->numfaces);
		out->area = in->area;
		out->contents = -1;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = lh.GetMap()->brush.nodes + p;
			else
				out->children[j] = (mnode_t *)(lh.GetMap()->brush.leafs + (-1 - p));
		}

		// mark all of the surfaces as on-node to support old BSP versions
		for ( j = 0; j < out->numsurfaces; j++ )
		{
			MSurf_Flags( out->firstsurface + j, lh.GetMap() ) |= SURFDRAW_NODE;
		}
	}
	
	Mod_SetParent (lh.GetMap()->brush.nodes, NULL);	// sets nodes and leafs

	// Check for small-area parents... no culling below them...
	mnode_t *pNode = lh.GetMap()->brush.nodes;
	for ( i=0 ; i<count ; ++i, ++pNode)
	{
		if (pNode->contents == -1)
		{
			if ((pNode->m_vecHalfDiagonal.x <= 50) && (pNode->m_vecHalfDiagonal.y <= 50) && 
				(pNode->m_vecHalfDiagonal.z <= 50))
			{
				// Mark all children as being too small to bother with...
				MarkSmallNode( pNode->children[0] );
				MarkSmallNode( pNode->children[1] );
			}
			else
			{
				CheckSmallVolumeDifferences( pNode->children[0], pNode->m_vecHalfDiagonal );
				CheckSmallVolumeDifferences( pNode->children[1], pNode->m_vecHalfDiagonal );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loadmodel - 
//			*l - 
//			*loadname - 
//-----------------------------------------------------------------------------
void Mod_LoadLeafs( void )
{
	Vector mins( 0, 0, 0 ), maxs( 0, 0, 0 );
	dleaf_t 	*in;
	mleaf_t 	*out;
	int			i, j, count, p;

	CMapLoadHelper lh( LUMP_LEAFS );

	in = (dleaf_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadLeafs: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (mleaf_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );

	lh.GetMap()->brush.leafs = out;
	lh.GetMap()->brush.numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			mins[j] = LittleShort (in->mins[j]);
			maxs[j] = LittleShort (in->maxs[j]);
		}

		VectorAdd( mins, maxs, out->m_vecCenter );
		out->m_vecCenter *= 0.5f;
		VectorSubtract( maxs, out->m_vecCenter, out->m_vecHalfDiagonal );

		p = LittleLong(in->contents);
		out->contents = p;

		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

/*
		out->firstmarksurface = lh.GetMap()->brush.marksurfaces +
			(unsigned short)LittleShort(in->firstleafface);
*/
		out->firstmarksurface = (int)LittleShort(in->firstleafface);
		out->nummarksurfaces = (unsigned short)LittleShort(in->numleaffaces);
		out->parent = NULL;
		
		out->m_pDisplacements = NULL;

		out->leafWaterDataID = in->leafWaterDataID;
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadLeafWaterData( void )
{
	dleafwaterdata_t *in;
	mleafwaterdata_t *out;
	int count, i;

	CMapLoadHelper lh( LUMP_LEAFWATERDATA );

	in = (dleafwaterdata_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadLeafs: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (mleafwaterdata_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
	lh.GetMap()->brush.leafwaterdata = out;
	lh.GetMap()->brush.numleafwaterdata = count;
	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->minZ = in->minZ;
		out->surfaceTexInfoID = in->surfaceTexInfoID;
		out->surfaceZ = in->surfaceZ;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadCubemapSamples( void )
{
	char textureName[512];
	dcubemapsample_t *in;
	mcubemapsample_t *out;
	int count, i;

	CMapLoadHelper lh( LUMP_CUBEMAPS );

	in = (dcubemapsample_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadCubemapSamples: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (mcubemapsample_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
	lh.GetMap()->brush.m_pCubemapSamples = out;
	lh.GetMap()->brush.m_nCubemapSamples = count;
	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->origin.Init( ( float )in->origin[0], ( float )in->origin[1], ( float )in->origin[2] );
		out->size = in->size;
		Q_snprintf( textureName, sizeof( textureName ), "maps/%s/c%d_%d_%d", lh.GetLoadName(), ( int )in->origin[0], 
			( int )in->origin[1], ( int )in->origin[2] );
		bool found;
		out->pTexture = materialSystemInterface->FindTexture( textureName, &found, true );
		if( !found )
		{
			Q_snprintf( textureName, sizeof( textureName ), "maps/%s/cubemapdefault", lh.GetLoadName() );
			out->pTexture = materialSystemInterface->FindTexture( textureName, &found, true );
			if( !found )
			{
				out->pTexture = materialSystemInterface->FindTexture( "engine/defaultcubemap", &found, true );
			}
		}
		out->pTexture->IncrementReferenceCount();
	}
	if( count )
	{
		materialSystemInterface->BindLocalCubemap( lh.GetMap()->brush.m_pCubemapSamples[0].pTexture );
	}
	else
	{
		bool found;
		ITexture *pTexture;
		Q_snprintf( textureName, sizeof( textureName ), "maps/%s/cubemapdefault", lh.GetLoadName() );
		pTexture = materialSystemInterface->FindTexture( textureName, &found, true );
		if( !found )
		{
			pTexture = materialSystemInterface->FindTexture( "engine/defaultcubemap", &found, true );
		}
		pTexture->IncrementReferenceCount();
		materialSystemInterface->BindLocalCubemap( pTexture );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadLeafMinDistToWater( void )
{
	CMapLoadHelper lh( LUMP_LEAFMINDISTTOWATER );

	unsigned short *pTmp = ( unsigned short * )lh.LumpBase();

	int i;
	bool foundOne = false;
	for( i = 0; i < ( int )( lh.LumpSize() / sizeof( *pTmp ) ); i++ )
	{
		if( pTmp[i] != 65535 ) // FIXME: make a marcro for this.
		{
			foundOne = true;
			break;
		}
	}
	
	if( !foundOne || lh.LumpSize() == 0 || !g_pMaterialSystemHardwareConfig || !g_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders())
	{
		// We don't bother keeping this if:
		// 1) there is no water in the map
		// 2) we don't have this lump in the bsp file (old bsp file)
		// 3) we aren't going to use it because we are on old hardware.
		lh.GetMap()->brush.m_LeafMinDistToWater = NULL;
	}
	else
	{
		int		count;
		unsigned short	*in;
		unsigned short	*out;

		in = (unsigned short *)lh.LumpBase();
		if (lh.LumpSize() % sizeof(*in))
			Host_Error ("Mod_LoadMarksurfaces: funny lump size in %s",lh.GetMapName());
		count = lh.LumpSize() / sizeof(*in);
		out = (unsigned short *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
		memcpy( out, in, sizeof( out[0] ) * count );
		lh.GetMap()->brush.m_LeafMinDistToWater = out;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Mod_LoadMarksurfaces( void )
{	
	int		i, j, count;
	short		*in;
	int			*out;

	CMapLoadHelper lh( LUMP_LEAFFACES );
	
	in = (short *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadMarksurfaces: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (int *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );

	lh.GetMap()->brush.marksurfaces = out;
	lh.GetMap()->brush.nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = (unsigned short)LittleShort(in[i]);
		if (j >= lh.GetMap()->brush.numsurfaces)
			Host_Error ("Mod_ParseMarksurfaces: bad surface number");
		out[i] = j;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pedges - 
//			*loadmodel - 
//			*l - 
//			*loadname - 
//-----------------------------------------------------------------------------
void Mod_LoadSurfedges( medge_t *pedges )
{	
	int		i, count;
	int		*in;
	unsigned short *out;
	
	CMapLoadHelper lh( LUMP_SURFEDGES );

	in = (int *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadSurfedges: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	if (count < 1 || count >= MAX_MAP_SURFEDGES)
		Host_Error ("Mod_LoadSurfedges: bad surfedges count in %s: %i",
		lh.GetMapName(), count);

	out = (unsigned short *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
	lh.GetMap()->brush.vertindices = out;
	lh.GetMap()->brush.numvertindices = count;

	for ( i=0 ; i<count ; i++)
	{
		int edge = LittleLong (in[i]);
		int index = 0;
		if ( edge < 0 )
		{
			edge = -edge;
			index = 1;
		}
		out[i] = pedges[edge].v[index];
	}

	delete[] pedges;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loadmodel - 
//			*l - 
//			*loadname - 
//-----------------------------------------------------------------------------
void Mod_LoadPlanes( void )
{
	// Don't bother loading them, they're already stored
	s_pMap->brush.planes = GetCollisionBSPData()->map_planes.Base();
	s_pMap->brush.numplanes = GetCollisionBSPData()->numplanes;
}


#if 0
//-----------------------------------------------------------------------------
// Purpose: Loads the vertex index list for all portals
// Input  : *loadmodel - 
//			*l - 
//-----------------------------------------------------------------------------
void Mod_LoadPortalVerts( void )
{
	int			i;
	unsigned short	*out;
	unsigned short	*in;
	int			count;

	CMapLoadHelper lh( LUMP_PORTALVERTS );
	
	in = (unsigned short  *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadPortalVerts: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (unsigned short *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
	
	lh.GetMap()->brush.portalverts = out;
	lh.GetMap()->brush.numportalverts = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		*out = (unsigned short)LittleShort( *in );
	}
}

//-----------------------------------------------------------------------------
// Purpose: loads the portal index list for all clusters
// Input  : *loadmodel - 
//			*l - 
//-----------------------------------------------------------------------------
void Mod_LoadClusterPortals( void )
{
	int			i;
	unsigned short	*out;
	unsigned short	*in;
	int			count;
	
	CMapLoadHelper lh( LUMP_CLUSTERPORTALS );

	in = (unsigned short  *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadClusterPortals: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (unsigned short *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
	
	lh.GetMap()->brush.clusterportals = out;
	lh.GetMap()->brush.numclusterportals = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		*out = (unsigned short)LittleShort( *in );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Loads the data for each clusters (points to portal list)
// Input  : *loadmodel - 
//			*l - 
//-----------------------------------------------------------------------------
void Mod_LoadClusters( void )
{
	int			i;
	mcluster_t	*out;
	dcluster_t	*in;
	int			count;
	
	CMapLoadHelper lh( LUMP_CLUSTERS );

	in = (dcluster_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadClusters: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (mcluster_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
	
	lh.GetMap()->brush.clusters = out;
	lh.GetMap()->brush.numclusters = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		out->numportals = LittleLong( in->numportals );
		out->portalList = lh.GetMap()->brush.clusterportals + LittleLong( in->firstportal );
	}
}
	
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loadmodel - 
//			*l - 
//			*loadname - 
//-----------------------------------------------------------------------------
void Mod_LoadPortals( void )
{
	int			i;
	mportal_t	*out;
	dportal_t	*in;
	int			count;
	
	CMapLoadHelper lh( LUMP_PORTALS );

	in = (dportal_t *)lh.LumpBase();
	if (lh.LumpSize() % sizeof(*in))
		Host_Error ("Mod_LoadPortals: funny lump size in %s",lh.GetMapName());
	count = lh.LumpSize() / sizeof(*in);
	out = (mportal_t *)Hunk_AllocName( count*sizeof(*out), lh.GetLoadName() );
	
	lh.GetMap()->brush.portals = out;
	lh.GetMap()->brush.numportals = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		out->cluster[0] = (unsigned short)LittleShort( in->cluster[0] );
		out->cluster[1] = (unsigned short)LittleShort( in->cluster[1] );
		out->numportalverts = LittleLong( in->numportalverts );
		out->plane = lh.GetMap()->brush.planes + LittleLong( in->planenum );
		out->vertList = lh.GetMap()->brush.portalverts + LittleLong( in->firstportalvert );
		out->visframe = 0;
	}
}
#endif


//-----------------------------------------------------------------------------
// Returns game lump version
//-----------------------------------------------------------------------------

int Mod_GameLumpVersion( int lumpId )
{
	for (int i = g_GameLumpDict.Size(); --i >= 0; )
	{
		if (g_GameLumpDict[i].id == lumpId)
			return g_GameLumpDict[i].version;
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Returns game lump size
//-----------------------------------------------------------------------------

int Mod_GameLumpSize( int lumpId )
{
	for (int i = g_GameLumpDict.Size(); --i >= 0; )
	{
		if (g_GameLumpDict[i].id == lumpId)
			return g_GameLumpDict[i].filelen;
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Loads game lumps
//-----------------------------------------------------------------------------

bool Mod_LoadGameLump( int lumpId, void* pBuffer, int size )
{
	int i;
	for ( i = g_GameLumpDict.Size(); --i >= 0; )
	{
		if (g_GameLumpDict[i].id == lumpId)
			break;
	}
	if (i < 0)
		return false;

	if (size < g_GameLumpDict[i].filelen)
		return false;

	// Load file into buffer
	FileHandle_t fp = g_pFileSystem->Open( g_pGameLumpFile, "rb" );
	if (!fp)
		return false;

	g_pFileSystem->Seek( fp, g_GameLumpDict[i].fileofs, FILESYSTEM_SEEK_HEAD ); 
	bool ok = (g_pFileSystem->Read( pBuffer, g_GameLumpDict[i].filelen, fp ) > 0);
	g_pFileSystem->Close( fp );
	return ok;
}


//-----------------------------------------------------------------------------
// Loads game lump dictionary
//-----------------------------------------------------------------------------

void Mod_LoadGameLumpDict( void )
{
	CMapLoadHelper lh( LUMP_GAME_LUMP );

	// FIXME: This is brittle. If we ever try to load two game lumps
	// (say, in multiple BSP files), the dictionary info I store here will get whacked

	g_GameLumpDict.RemoveAll();
	strcpy( g_pGameLumpFile, lh.GetMapName() );
	if (lh.LumpSize() == 0)
		return;

	// Read dictionary...
	dgamelumpheader_t* pGameLumpHeader = (dgamelumpheader_t*)lh.LumpBase();
	dgamelump_t* pGameLump = (dgamelump_t*)(pGameLumpHeader + 1);
	for (int i = 0; i < pGameLumpHeader->lumpCount; ++i )
	{
		g_GameLumpDict.AddToTail( pGameLump[i] );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModelLoader::Init( void )
{
	m_Models.Purge();
	memset( m_InlineModels, 0, sizeof( m_InlineModels ) );

	m_pWorldModel = NULL;
	m_bMapRenderInfoLoaded = false;

	// Make sure we have physcollision and physprop interfaces
	CollisionBSPData_LinkPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModelLoader::Shutdown( void )
{
	m_pWorldModel = NULL;

	UnloadAllModels( false );

	m_ModelPool.Clear();
}

//-----------------------------------------------------------------------------
// Purpose: Look up name for model
// Input  : *model - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CModelLoader::GetName( const model_t *model )
{
	if ( model )
	{
		return model->name;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Finds the model, and loads it if it isn't already present.  Updates reference flags
// Input  : *name - 
//			referencetype - 
// Output : model_t
//-----------------------------------------------------------------------------
model_t *CModelLoader::FindModel( const char *name )
{
	int			i;
	if ( !name || !name[0] )
	{
		Sys_Error ("CModelLoader::FindModel: NULL name");
	}

	// inline models are grabbed only from worldmodel
	if ( name[0] == '*' )
	{
		i = atoi( name + 1 );
		if ( !IsWorldModelSet() )
		{
			Sys_Error( "bad inline model number %i, worldmodel not yet setup", i );
		}

		if ( i < 1 || i >= GetNumWorldSubmodels() )
		{
			Sys_Error( "bad inline model number %i", i );
		}
		return &m_InlineModels[ i ];
	}

	model_t *mod = NULL;

	i = m_Models.Find( name );
	if ( i == m_Models.InvalidIndex() )
	{
		mod = (model_t *)m_ModelPool.Alloc();
		Assert( mod );

		ModelEntry entry;
		entry.modelpointer = mod;

		m_Models.Insert( name, entry );

		memset( mod, 0, sizeof( *mod ) );

		// Copy in name
		strcpy( mod->name, name );
		// Mark that we should load from disk
		// FIXME:
		mod->needload = FMODELLOADER_NOTLOADEDORREFERENCED;
	}
	else
	{
		mod = m_Models[ i ].modelpointer;
	}

	Assert( mod );
	return mod;
}

//-----------------------------------------------------------------------------
// Purpose: Finds the model, and loads it if it isn't already present.  Updates reference flags
// Input  : *name - 
//			referencetype - 
// Output : model_t
//-----------------------------------------------------------------------------
model_t *CModelLoader::GetModelForName( const char *name, REFERENCETYPE referencetype )
{
	model_t *model = FindModel( name );
	Assert( model );
	if ( !model )
	{
		return NULL;
	}

	return LoadModel( model, &referencetype );
}


//-----------------------------------------------------------------------------
// Purpose: Add a reference to the model in question
// Input  : *name - 
//			referencetype - 
//-----------------------------------------------------------------------------
model_t *CModelLoader::ReferenceModel( const char *name, REFERENCETYPE referencetype )
{
	model_t *model = FindModel( name );
	if ( model )
	{
		model->needload |= referencetype;
		return model;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *entry - 
//			referencetype - 
//-----------------------------------------------------------------------------
model_t	*CModelLoader::LoadModel( model_t *mod, REFERENCETYPE *referencetype )
{
	unsigned *buf = NULL;
	// Try and avoid dirtying the cache heap
	byte	stackbuf[64 * 1024];		

	if ( referencetype )
	{
		mod->needload |= *referencetype;
	}

	// Only studio models use the cache system
	if ( mod->type == mod_studio )
	{
		if ( Cache_Check( &mod->cache ) )
		{
			Assert( FMODELLOADER_LOADED & mod->needload );
			return mod;
		}
	}
	else
	{
		if ( FMODELLOADER_LOADED & mod->needload ) 
		{
			return mod;
		}
	}

	// Set the name of the current model we are loading
	COM_FileBase( mod->name, s_szLoadName );

	// load the file
	if ( developer.GetInt() > 1 )
	{
		Con_DPrintf("loading %s\n", mod->name );
	}

	// HACK HACK, force sprites to correctly
	if ( Q_stristr( mod->name, ".spr" ) || Q_stristr( mod->name, ".vmt" ) )
	{
		mod->type = mod_sprite;
	}
	else if ( Q_stristr( mod->name, ".bsp" ) )
	{
		mod->type = mod_brush;
	}
	else
	{
		// Load file to temporary space
		buf = (unsigned *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));

		if ( !buf || com_filesize == 0 )
		{
			Con_DPrintf("Model %s not found!\n", mod->name );
			buf = (unsigned *)COM_LoadStackFile ("models/error.mdl", stackbuf, sizeof(stackbuf));
			if ( !buf )
			{
				Sys_Error ("Mod_NumForName: %s not found and %s couldn't be loaded", mod->name, "models/error.mdl" );
			}
		}

		if ( LittleLong(*(unsigned *)buf) == IDSTUDIOHEADER )
		{
			mod->type = mod_studio;
		}
		else if (LittleLong(*(unsigned *)buf) == IDSPRITEHEADER )
		{
			Assert( 0 );
		}
		else
		{
			mod->type = mod_brush;
		}
	}

	switch ( mod->type )
	{
	case mod_sprite:
		Sprite_LoadModel( mod );
		break;

	case mod_studio:
		Assert( buf );
		Studio_LoadModel( mod, buf );
		break;

	case mod_brush:
		// FIXME: There's gotta be a better place for this!
		// It's useful only on dedicated clients. On listen + dedicated servers, it's called twice.
		// Add to file system before loading so referenced objects in map can use the filename
		g_pFileSystem->AddSearchPath( mod->name, "GAME", PATH_ADD_TO_HEAD );
		Map_LoadModel( mod );
		break;

	default:
		Assert( 0 );
		break;
	};

	return mod;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
//			*pOut - 
//			outLen - 
// Output : static void
//-----------------------------------------------------------------------------
static void BuildSpriteLoadName( const char *pName, char *pOut, int outLen )
{
	// If it's a .vmt and they put a path in there, then use the path.
	// Otherwise, use the old method of prepending the sprites directory.
	if ( Q_stristr( pName, ".vmt" ) && strchr( pName, '/' ) )
	{
		// WorldCraft likes to prepend "materials" but the material system doesn't like this.
		if ( Q_stristr( pName, "materials/" ) == pName ||
			Q_stristr( pName, "materials\\" ) == pName )
		{
			char strTemp[MAX_PATH];
			Q_strncpy( strTemp, &pName[10], sizeof( strTemp ) );

			COM_StripExtension( strTemp, pOut, outLen );
		}
		else
		{
			COM_StripExtension( pName, pOut, outLen );
		}
	}
	else
	{
		char base[MAX_PATH];
		COM_FileBase( pName, base );
		Q_snprintf( pOut, outLen, "sprites/%s", base );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CModelLoader::GetModelFileSize( char const *name )
{
	if ( !name || !name[ 0 ] )
		return -1;

	model_t *model = FindModel( name );

	int size = -1;
	if ( Q_stristr( model->name, ".spr" ) || Q_stristr( model->name, ".vmt" ) )
	{
		char spritename[ MAX_PATH ];
		COM_StripExtension( va( "materials/%s", model->name ), spritename, MAX_PATH );
		COM_DefaultExtension( spritename, ".vmt", sizeof( spritename ) );

		size = COM_FileSize( spritename );
	}
	else
	{
		size = COM_FileSize( name );
	}

	return size;
}

//-----------------------------------------------------------------------------
// Purpose: Unmasks the referencetype field for the model
// Input  : *model - 
//			referencetype - 
//-----------------------------------------------------------------------------
void CModelLoader::ReleaseModel( model_t *model, REFERENCETYPE referencetype )
{
	model->needload &= ~referencetype;
}
//-----------------------------------------------------------------------------
// Purpose: Unmasks the specified reference type across all models
// Input  : referencetype - 
//-----------------------------------------------------------------------------
void CModelLoader::ReleaseAllModels( REFERENCETYPE referencetype )
{
	int				i;
	model_t			*mod;

	// UNDONE: If we ever free a studio model, write code to free the collision data
	// UNDONE: Reference count collision data?

	int c = m_Models.Count();
	for ( i=0 ; i < c ; i++ )
	{
		mod = m_Models[ i ].modelpointer;
		ReleaseModel( mod, referencetype );
	}
}

//-----------------------------------------------------------------------------
// Purpose: For any models with referencetype blank (if checking), frees all memory associated with the model
//  and frees up the models slot
//-----------------------------------------------------------------------------
void CModelLoader::UnloadAllModels( bool checkreference )
{
	int				i;
	model_t			*model;

	int c = m_Models.Count();
	for ( i=0 ; i < c ; i++ )
	{
		model = m_Models[ i ].modelpointer;
		if ( checkreference )
		{
			if ( model->needload & FMODELLOADER_REFERENCEMASK )
			{
				continue;
			}
		}
		else
		{
			// Wipe current flags
			model->needload &= ~FMODELLOADER_REFERENCEMASK;
		}

		// Was it even loaded
		if ( !( model->needload & FMODELLOADER_LOADED ) )
			continue;

		switch ( model->type )
		{
		case mod_brush:
			// Let it free data or call destructors..
			Map_UnloadModel( model );
			// Remove from file system
			g_pFileSystem->RemoveSearchPath( model->name, "GAME" );
			break;
		case mod_studio:
			Studio_UnloadModel( model );
			break;
		case mod_sprite:
			Sprite_UnloadModel( model );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: For any models with referencetype blank (if checking), frees all memory associated with the model
//  and frees up the models slot
//-----------------------------------------------------------------------------
void CModelLoader::UnloadUnreferencedModels( void )
{
	UnloadAllModels( true );
}


//-----------------------------------------------------------------------------
// Compute whether this submodel uses material proxies or not
//-----------------------------------------------------------------------------

static void Mod_ComputeBrushModelFlags( model_t *mod )
{
	Assert( mod );
	mod->flags = 0;

	// Iterate over all this models surfaces
	msurface1_t *psurf1 = &mod->brush.surfaces1[mod->brush.firstmodelsurface];
	for (int i = 0; i < mod->brush.nummodelsurfaces; ++i)
	{
		IMaterial* material = psurf1[i].texinfo->material;
		if (material->HasProxy())
		{
			mod->flags |= MODELFLAG_MATERIALPROXY;
		}

		if ( material->IsTranslucent() )
		{
			mod->flags |= MODELFLAG_TRANSLUCENT;

			// Make sure flags are consistent
//			Assert( psurf[i].flags & SURFDRAW_TRANS );

			if( !(MSurf_Flags( mod->brush.firstmodelsurface + i, mod ) & SURFDRAW_TRANS) )
				MSurf_Flags( mod->brush.firstmodelsurface + i, mod ) |= SURFDRAW_TRANS;
		}
		else
		{
			// Make sure flags are consistent
//			Assert( (psurf[i].flags & SURFDRAW_TRANS) == 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Recomputes translucency for the model...
//-----------------------------------------------------------------------------

void Mod_RecomputeTranslucency( model_t* mod )
{
	mod->flags &= ~MODELFLAG_TRANSLUCENT;

	switch( mod->type )
	{
	case mod_brush:
		{
			msurface1_t *psurf1 = &mod->brush.surfaces1[mod->brush.firstmodelsurface];
			for (int i = 0; i < mod->brush.nummodelsurfaces; ++i)
			{
				IMaterial* material = psurf1[i].texinfo->material;
				if ( material->IsTranslucent() )
				{
					mod->flags |= MODELFLAG_TRANSLUCENT;
					break;
				}
			}
		}
		break;

	case mod_studio:
		{
			studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( mod );
			if (pStudioHdr->flags & STUDIOHDR_FLAGS_FORCE_OPAQUE)
				return;

			for( int lodID = 0; lodID < mod->studio.hardwareData.m_NumLODs; lodID++ )
			{
				for (int i = 0; i < mod->studio.hardwareData.m_pLODs[lodID].numMaterials; ++i )
				{
					if (mod->studio.hardwareData.m_pLODs[lodID].ppMaterials[i]->IsTranslucent())
					{
						mod->flags |= MODELFLAG_TRANSLUCENT;
						break;
					}
				}
			}
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// returns the material count...
//-----------------------------------------------------------------------------

int Mod_GetMaterialCount( model_t* mod )
{
	switch( mod->type )
	{
	case mod_brush:
		{
			CUtlVector<IMaterial*> uniqueMaterials( 0, 32 );

			msurface1_t *psurf1 = &mod->brush.surfaces1[mod->brush.firstmodelsurface];
			for (int i = 0; i < mod->brush.nummodelsurfaces; ++i)
			{
				if ( MSurf_Flags( mod->brush.firstmodelsurface + i, mod ) & SURFDRAW_NODRAW )
					continue;

				IMaterial* pMaterial = psurf1[i].texinfo->material;

				// Try to find the material in the unique list of materials
				// if it's not there, then add it
				if (uniqueMaterials.Find(pMaterial) < 0)
					uniqueMaterials.AddToTail(pMaterial);
			}

			return uniqueMaterials.Size();
		}
		break;

	case mod_studio:
		{
			// FIXME: This should return the list of all materials
			// across all LODs if we every decide to implement this
			Assert(0);
		}
		break;

	default:
		// unimplemented
		Assert(0);
		break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// returns the first n materials.
//-----------------------------------------------------------------------------
  
void Mod_GetModelMaterials( model_t* mod, int count, IMaterial** ppMaterial )
{
	switch( mod->type )
	{
	case mod_brush:
		{
			int found = 0; 
			msurface1_t *psurf1 = &mod->brush.surfaces1[mod->brush.firstmodelsurface];
			for ( int i = 0; i < mod->brush.nummodelsurfaces; ++i)
			{
				if ( MSurf_Flags( mod->brush.firstmodelsurface + i, mod ) & SURFDRAW_NODRAW )
					continue;

				IMaterial* pMaterial = psurf1[i].texinfo->material;

				// Try to find the material in the unique list of materials
				// if it's not there, then add it
				int j = found;
				while ( --j >= 0 )
				{
					if (ppMaterial[j] == pMaterial)
						break;
				}

				if (j < 0)
					ppMaterial[found++] = pMaterial;

				// Stop when we've gotten count materials
				if (found == count)
					return;
			}
		}
		break;

	case mod_studio:
		{
			// FIXME: This should return the list of all materials
			// across all LODs if we every decide to implement this
			Assert(0);
		}
		break;

	default:
		// unimplemented
		Assert(0);
		break;
	}
}

//-----------------------------------------------------------------------------
// Used to compute which surfaces are in water or not
//-----------------------------------------------------------------------------

static void MarkWaterSurfaces_ProcessLeafNode( mleaf_t *pLeaf )
{
	bool bWater = ( pLeaf->leafWaterDataID == -1 ) ? false : true;
	int i;

	for( i = 0; i < pLeaf->nummarksurfaces; i++ )
	{
		int surfID = host_state.worldmodel->brush.marksurfaces[pLeaf->firstmarksurface + i];
		ASSERT_SURF_VALID( surfID );
		if( MSurf_Flags( surfID ) & SURFDRAW_WATERSURFACE )
		{
			continue;
		}
		MSurf_FogVolumeID( surfID ) = pLeaf->leafWaterDataID;

		if (SurfaceHasDispInfo( surfID ))
			continue;

		if( bWater )
		{
			MSurf_Flags( surfID ) |= SURFDRAW_UNDERWATER;
		}
		else
		{
			MSurf_Flags( surfID ) |= SURFDRAW_ABOVEWATER;
		}
	}

	// FIXME: This is somewhat bogus, but I can do it quickly, and it's
	// not clear I need to solve the harder problem.

	// If any portion of a displacement surface hits a water surface,
	// I'm going to mark it as being in water, and vice versa.
	for( CDispIterator it=pLeaf->GetDispIterator(); it.IsValid(); )
	{
		CDispLeafLink *pCur = it.Inc();

		int parentSurfID = static_cast<IDispInfo*>( pCur->m_pDispInfo )->GetParent();
		MSurf_Flags( parentSurfID ) |= ( bWater ? SURFDRAW_UNDERWATER : SURFDRAW_ABOVEWATER );
	}
}


void MarkWaterSurfaces_r( mnode_t *node )
{
	// no polygons in solid nodes
	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	// if a leaf node, . .mark all the polys as to whether or not they are in water.
	if (node->contents >= 0)
	{
		MarkWaterSurfaces_ProcessLeafNode( (mleaf_t *)node );
		return;
	}

	MarkWaterSurfaces_r( node->children[0] );
	MarkWaterSurfaces_r( node->children[1] );
}


//-----------------------------------------------------------------------------
// Marks identity brushes as being in fog volumes or not
//-----------------------------------------------------------------------------

class CBrushBSPIterator : public ISpatialLeafEnumerator
{
public:
	bool EnumerateLeaf( int leaf, int context )
	{
		// garymcthack - need to test identity brush models
		model_t* pModel = (model_t*)context;
		bool bUnderWater = ( m_pWorld->brush.leafs[leaf].leafWaterDataID != -1 );
		// Iterate over all this models surfaces
		int surfaceCount = pModel->brush.nummodelsurfaces;
		for (int i = 0; i < surfaceCount; ++i)
		{
			MSurf_Flags( pModel->brush.firstmodelsurface + i, pModel ) |= ( bUnderWater ? SURFDRAW_UNDERWATER : SURFDRAW_ABOVEWATER ); 
		}
		return true;
	}

	model_t* m_pWorld;
};

static void MarkBrushModelWaterSurfaces( model_t* world,
	Vector const& mins, Vector const& maxs, model_t* brush )
{
	static CBrushBSPIterator s_pBrushIterator;

	// HACK: This is a totally brutal hack dealing with initialization order issues.
	// I want to use the same box enumeration code so I don't have multiple
	// copies, but I want to use it from modelloader. host_state.worldmodel isn't
	// set up at that time however, so I have to fly through these crazy hoops.
	// Massive suckage.

	model_t* pTemp = host_state.worldmodel;
	s_pBrushIterator.m_pWorld = world;
	host_state.worldmodel = world;
	g_pToolBSPTree->EnumerateLeavesInBox( mins, maxs, &s_pBrushIterator, (int)brush );
	host_state.worldmodel = pTemp;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mod - 
//			*buffer - 
//-----------------------------------------------------------------------------
void CModelLoader::Map_LoadModel( model_t *mod )
{
	int			i;
	int			checksum;

	Assert( !( mod->needload & FMODELLOADER_LOADED ) );

	SetWorldModel( mod );

	// Need this first because the render model may reference data it sets up
	CM_LoadMap( mod->name, true, (unsigned int*)&checksum );

	mod->type = mod_brush;
	mod->needload |= FMODELLOADER_LOADED;

	CMapLoadHelper::Init( mod, s_szLoadName );

	// Load into hunk
	Mod_LoadVertices();
	medge_t *pedges = Mod_LoadEdges();
	Mod_LoadSurfedges( pedges );
	Mod_LoadPlanes();
	Mod_LoadOcclusion();
	// texdata needs to load before texinfo
	Mod_LoadTexdata();
	Mod_LoadTexinfo();

	// Until BSP version 19, this must occur after loading texinfo
	Mod_LoadLighting();

//	Mod_LoadOrigFaces();
	Mod_LoadPrimitives();
	Mod_LoadPrimVerts();
	Mod_LoadPrimIndices();

	// faces need to be loaded before vertnormals
	Mod_LoadFaces();
	Mod_LoadVertNormals();
	Mod_LoadVertNormalIndices();
    Mod_LoadMarksurfaces();
	Mod_LoadLeafs();
	Mod_LoadNodes();
	Mod_LoadLeafWaterData();
	Mod_LoadCubemapSamples();
	// UNDONE: Does the cmodel need worldlights?
#ifndef SWDS
	OverlayMgr()->LoadOverlays( );	
#endif
	Mod_LoadLeafMinDistToWater();

	Mod_LoadLump( mod, 
		LUMP_CLIPPORTALVERTS, 
		s_szLoadName, 
		sizeof(mod->brush.m_pClipPortalVerts[0]), 
		(void**)&mod->brush.m_pClipPortalVerts,
		&mod->brush.m_nClipPortalVerts );

	Mod_LoadLump( mod, 
		LUMP_AREAPORTALS, 
		s_szLoadName, 
		sizeof(mod->brush.m_pAreaPortals[0]), 
		(void**)&mod->brush.m_pAreaPortals,
		&mod->brush.m_nAreaPortals );
	
	Mod_LoadLump( mod, 
		LUMP_AREAS, 
		s_szLoadName, 
		sizeof(mod->brush.m_pAreas[0]), 
		(void**)&mod->brush.m_pAreas,
		&mod->brush.m_nAreas );

	Mod_LoadWorldlights();
	Mod_LoadGameLumpDict();
	// load the portal information
	// JAY: Disabled until we need this information.
#if 0
	Mod_LoadPortalVerts();
	Mod_LoadClusterPortals();
	Mod_LoadClusters();
	Mod_LoadPortals();
#endif

	Mod_LoadSubmodels();

	//
	// check for a valid number of sub models to load
	//
	if ( mod->brush.numsubmodels >= MAX_KNOWN_MODELS )
	{
		Sys_Error( "Max sub-model count exceeded, %d >= %d\n", mod->brush.numsubmodels, MAX_KNOWN_MODELS );
	}

	//
	// set up the submodels
	//
	mod->mins = mod->maxs = Vector(0,0,0);

	for (i=0 ; i<mod->brush.numsubmodels ; i++)
	{
		model_t	*starmod;
		mmodel_t 	*bm;

		bm = &mod->brush.submodels[i];
		starmod = &m_InlineModels[i];

		*starmod = *mod;
		
		starmod->brush.firstmodelsurface = bm->firstface;
		starmod->brush.nummodelsurfaces = bm->numfaces;
		starmod->brush.firstnode = bm->headnode;
		if (starmod->brush.firstnode >= mod->brush.numnodes)
			Sys_Error ("Inline model %i has bad firstnode", i);

		VectorCopy (bm->maxs, starmod->maxs);
		VectorCopy (bm->mins, starmod->mins);
		starmod->radius = bm->radius;
	
		if (i == 0)
		{
			*mod = *starmod;
		}
		else
		{
			Q_snprintf( starmod->name, sizeof( starmod->name ), "*%d", i );
		}

		starmod->brush.numleafs = bm->visleafs;

		// Compute whether this submodel uses material proxies or not
		Mod_ComputeBrushModelFlags(starmod);

		// Mark if brush models are in water or not; we'll use this
		// for identity brushes. If the brush is not an identity brush,
		// then we'll not have to worry.
		if( i != 0 )
		{
			MarkBrushModelWaterSurfaces( mod, bm->mins, bm->maxs, starmod );
		}
	}

	Map_VisClear();

	Map_SetRenderInfoAllocated( false );
	// garymcthack!!!!!!!!
	// host_state.worldmodel isn't set at this point, so. . . . 
	host_state.worldmodel = mod;
	MarkWaterSurfaces_r( mod->brush.nodes );
	host_state.worldmodel = NULL;

	// Close map file, etc.
	CMapLoadHelper::Shutdown();
}

void CModelLoader::Map_UnloadCubemapSamples( model_t *mod )
{
	int i;
	for ( i=0 ; i < mod->brush.m_nCubemapSamples ; i++ )
	{
		mcubemapsample_t *pSample = &mod->brush.m_pCubemapSamples[i];
		pSample->pTexture->DecrementReferenceCount();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mod - 
//-----------------------------------------------------------------------------
void CModelLoader::Map_UnloadModel( model_t *mod )
{
	Assert( !( mod->needload & FMODELLOADER_REFERENCEMASK ) );
	mod->needload &= ~FMODELLOADER_LOADED;
	
#ifndef SWDS
	OverlayMgr()->UnloadOverlays();
#endif

	DeallocateLightingData( mod );
#ifndef SWDS
	DispInfo_ReleaseMaterialSystemObjects( mod );
#endif

	Map_UnloadCubemapSamples( mod );
	
//	delete[] mod->brush.pOrigSurfaces;
//	mod->brush.pOrigSurfaces = NULL;

#ifndef SWDS
	// Free decals in displacements.
	R_DecalTerm( mod );
#endif

	if ( mod->brush.hDispInfos )
	{
		DispInfo_DeleteArray( mod->brush.hDispInfos );
		mod->brush.hDispInfos = NULL;
	}

	// Model loader loads world model materials, unload them here
	for( int texinfoID = 0; texinfoID < mod->brush.numtexinfo; texinfoID++ )
	{
		mtexinfo_t *pTexinfo = &mod->brush.texinfo[texinfoID];
		if ( pTexinfo )
		{
			GL_UnloadMaterial( pTexinfo->material );
		}
	}

	MaterialSystem_DestroySortinfo();

	// Don't store any reference to it here
	ClearWorldModel();
	// Wipe any submodels, too
	for ( int i = 0; i < MAX_KNOWN_MODELS; i++ )
	{
		model_t *m = &m_InlineModels[ i ];
		if ( m->cache.data )
		{
			Cache_Free( &m->cache );
		}
	}
	memset( m_InlineModels, 0, sizeof( m_InlineModels ) );

	Map_SetRenderInfoAllocated( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mod - 
//-----------------------------------------------------------------------------
void CModelLoader::Sprite_LoadModel( model_t *mod )
{
	Assert( !( mod->needload & FMODELLOADER_LOADED ) );

	mod->needload |= FMODELLOADER_LOADED;

	// The hunk data is not used on the server
	int size = 0;
#ifndef SWDS
	if ( g_ClientDLL )
	{
		size = g_ClientDLL->GetSpriteSize();
	}
#endif

	byte* pSprite = NULL;
	
	if ( size )
	{
		pSprite = ( byte * )new byte[ size ];
	}

	mod->type = mod_sprite;
	
	mod->sprite.numframes = 1;
	mod->sprite.sprite = (CEngineSprite *)pSprite;

	// Fake the bounding box. We need it for PVS culling, and we don't
	// know the scale at which the sprite is going to be rendered at
	// when we load it
	mod->mins = mod->maxs = Vector(0,0,0);

	// Figure out the real load name..
	char loadName[MAX_PATH];
	BuildSpriteLoadName( mod->name, loadName, MAX_PATH );

	IMaterial *material = GL_LoadMaterial( loadName );
	if ( material )
	{
		// Just copy # of animation frames into buffer
		mod->sprite.numframes = material->GetNumAnimationFrames();

		if ( material == g_materialEmpty )
		{
			Con_DPrintf( "Missing sprite material %s\n", s_szLoadName );
		}
	}
	
#ifndef SWDS
	if ( g_ClientDLL && mod->sprite.sprite )
	{
		g_ClientDLL->InitSprite( mod->sprite.sprite, loadName );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mod - 
//-----------------------------------------------------------------------------
void CModelLoader::Sprite_UnloadModel( model_t *mod )
{
	Assert( !( mod->needload & FMODELLOADER_REFERENCEMASK ) );
	mod->needload &= ~FMODELLOADER_LOADED;

	char loadName[MAX_PATH];
	BuildSpriteLoadName( mod->name, loadName, sizeof( loadName ) );

	bool found = false;
	IMaterial *mat = materialSystemInterface->FindMaterial( loadName, &found );
	if ( mat && found )
	{
		GL_UnloadMaterial( mat );
	}

#ifndef SWDS
	if ( g_ClientDLL && mod->sprite.sprite )
	{
		g_ClientDLL->ShutdownSprite( mod->sprite.sprite );
	}
#endif

	delete[] (byte *)mod->sprite.sprite;
	mod->sprite.sprite = 0;
	mod->sprite.numframes = 0;
}

//-----------------------------------------------------------------------------
// Loads vertex data into a temp buffer.
//-----------------------------------------------------------------------------

bool Mod_LoadStudioModelVtxFileIntoTempBuffer( model_t *mod, CUtlMemory<unsigned char>& tmpVtxMem )
{
	char tempFileName[128];
	FileHandle_t fileHandle;
	int fileLength;
	bool readOK;

	int nPrefix = 7;
	strcpy( tempFileName, "models/" );

	studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( mod );
	COM_StripExtension( pStudioHdr->name, &tempFileName[nPrefix], sizeof( tempFileName ) - nPrefix );
	int namelen = strlen( tempFileName );

	// FIXME:
	// Check for the particular board's version...
	if ( !g_pMaterialSystemHardwareConfig )
		return false;

	if (g_pMaterialSystemHardwareConfig->SupportsVertexAndPixelShaders() ||
		g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 70)
	{
		// Check for the default version...
		strcpy( tempFileName + namelen, ".dx80.vtx" );
	}
	else
	{
		if( g_pMaterialSystemHardwareConfig->MaxBlendMatrices() > 2 )
		{
			// Check for the default version...
			strcpy( tempFileName + namelen, ".dx7_3bone.vtx" );
		}
		else
		{
			// Check for the default version...
			strcpy( tempFileName + namelen, ".dx7_2bone.vtx" );
		}
	}

	fileHandle = g_pFileSystem->Open( tempFileName, "rb" );
	if( !fileHandle )
	{
		// Fall back....
		strcpy( tempFileName + namelen, ".dx7_2bone.vtx" );
		fileHandle = g_pFileSystem->Open( tempFileName, "rb" );
		if( !fileHandle )
			return false;
	}

	fileLength = g_pFileSystem->Size( fileHandle );
	tmpVtxMem.EnsureCapacity( fileLength );

	readOK = ( fileLength == g_pFileSystem->Read( tmpVtxMem.Base(), fileLength, fileHandle ) );
	g_pFileSystem->Close( fileHandle );
	return readOK;
}


//-----------------------------------------------------------------------------
// Computes model flags
//-----------------------------------------------------------------------------

static void ComputeFlags( model_t* mod )
{
	studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( mod );
	bool forceOpaque = (pStudioHdr->flags & STUDIOHDR_FLAGS_FORCE_OPAQUE) != 0;

	if (pStudioHdr->flags & STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS)
		mod->flags |= MODELFLAG_TRANSLUCENT_TWOPASS;

	int lodID;
	for( lodID = 0; lodID < mod->studio.hardwareData.m_NumLODs; lodID++ )
	{
		studioloddata_t *pLODData = &mod->studio.hardwareData.m_pLODs[lodID];
		for (int i = 0; i < pLODData->numMaterials; ++i )
		{
			if (pLODData->ppMaterials[i]->IsVertexLit())
			{
				mod->flags |= MODELFLAG_VERTEXLIT;
			}
			if ((!forceOpaque) && pLODData->ppMaterials[i]->IsTranslucent())
			{
				mod->flags |= MODELFLAG_TRANSLUCENT;
			}
			if (pLODData->ppMaterials[i]->HasProxy())
			{
				mod->flags |= MODELFLAG_MATERIALPROXY;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Loads the static meshes
//-----------------------------------------------------------------------------
void CModelLoader::Studio_LoadStaticMeshes( model_t* mod )
{
	// Don't try to load VTX files if we don't have focus....
	// Load the vtx file and build static meshes if they haven't already been built.
	if ((!g_LostVideoMemory) && (!mod->studio.studiomeshLoaded))
	{
		mod->studio.studiomeshLoaded = true;
		CUtlMemory<unsigned char> tmpVtxMem; // This goes away when we leave this scope.
		bool retVal;

		// Load the vtx file into a temp buffer that'll go away after we leave this scope.
		Assert( Cache_Check( &mod->cache ) );
		studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( mod );
		retVal = Mod_LoadStudioModelVtxFileIntoTempBuffer( mod, tmpVtxMem );
		if( !retVal )
		{
			if ( cls.state != ca_dedicated )
			{
				Con_DPrintf( "--ERROR-- : Can't load vtx file for %s\n", pStudioHdr->name );
			}
			mod->studio.studiomeshLoaded = false;
			return;
		}
		Assert( pStudioHdr );
		if( !pStudioHdr )
		{
			mod->studio.studiomeshLoaded = false;
			return;
		}
		if( !g_pStudioRender->LoadModel( pStudioHdr, tmpVtxMem.Base(), 
			&mod->studio.hardwareData ) )
		{
			mod->studio.studiomeshLoaded = false;
			return;
		}

		// Set flag...
		ComputeFlags(mod);
	}
	else
	{
		if (mod->studio.studiomeshLoaded)
		{
			// This can happen if the studiohdr_t was uncached.
			// In that case, make sure we reset all the fields of the studiohdr_t
			studiohdr_t *pStudioHdr = ( studiohdr_t * )modelloader->GetExtraData( mod );
			g_pStudioRender->RefreshStudioHdr( pStudioHdr, &mod->studio.hardwareData );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mod - 
//-----------------------------------------------------------------------------
void CModelLoader::Studio_LoadModel( model_t *mod, void *buffer )
{
	/*
	if ( mod->needload & FMODELLOADER_LOADED )
	{
		Con_DPrintf( "Studio model %s was flushed from cache without notice to CModelLoader\n", mod->name );
	}
	*/

	mod->needload |= FMODELLOADER_LOADED;

	// Load the cached part of the studiomodel
	if( !Mod_LoadStudioModel( mod, buffer, false ) )
	{
		// swap this model with "error.mdl" since it can't be loaded or is the wrong version
		char stackbuf[1024];
		Con_DPrintf("Model %s can't be loaded!\n", mod->name );
		Q_strcpy( mod->name, "models/error.mdl" );
		buffer = (unsigned *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));
		if ( buffer )
		{
			if ( !Mod_LoadStudioModel( mod, buffer, false ) )
				return;
		}
	}

	Studio_LoadStaticMeshes( mod );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mod - 
//-----------------------------------------------------------------------------
void CModelLoader::Studio_UnloadModel( model_t *mod )
{
	// leave these flags alone since we are going to return from alt-tab at some point.
	//	Assert( !( mod->needload & FMODELLOADER_REFERENCEMASK ) );
	mod->needload &= ~FMODELLOADER_LOADED;

	Mod_UnloadStudioModel( mod );
	if ( Cache_Check( &mod->cache ) )
	{
		Cache_Free( &mod->cache );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mod - 
//-----------------------------------------------------------------------------
void CModelLoader::SetWorldModel( model_t *mod )
{
	Assert( mod );
	m_pWorldModel = mod;
//	host_state.worldmodel = mod; // garymcthack
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModelLoader::ClearWorldModel( void )
{
	m_pWorldModel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CModelLoader::IsWorldModelSet( void )
{
	return m_pWorldModel ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CModelLoader::GetNumWorldSubmodels( void )
{
	if ( !IsWorldModelSet() )
		return 0;

	return m_pWorldModel->brush.numsubmodels;
}

//-----------------------------------------------------------------------------
// Purpose: Check cache or union data for info, reload studio model if needed 
// Input  : *model - 
//-----------------------------------------------------------------------------
void *CModelLoader::GetExtraData( model_t *model )
{
	if ( !model )
	{
		return NULL;
	}

	switch ( model->type )
	{
	case mod_sprite:
		{
			// sprites don't use the real cache yet
			if ( model->type == mod_sprite )
			{
				// The sprite got unloaded.
				if ( !( FMODELLOADER_LOADED & model->needload ) )
				{
					return NULL;
				}

				return model->sprite.sprite;
			}
		}
		break;
	case mod_studio:
		{
			void *r = Cache_Check( &model->cache );
			if ( r )
			{
				return r;
			}

			// Reload, but don't change referenceflags
			LoadModel( model, NULL );
			if ( !model->cache.data )
			{
				Sys_Error ("CModelLoader::GetExtraData: re-caching %s failed", model->name );
			}
			return model->cache.data;
		}
		break;
	default:
	case mod_brush:
		// Should never happen
		Assert( 0 );
		break;
	};

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Material system can force all studio model meshes 
// to unload on demand
//-----------------------------------------------------------------------------
void CModelLoader::Studio_ReleaseModels( void )
{
	int		i;
	model_t	*mod;
	
	int c = m_Models.Count();

	for ( i=0; i< c ; i++ )
	{
		mod = m_Models[ i ].modelpointer;
		if ( !( mod->needload & FMODELLOADER_LOADED ) )
			continue;

		if ( mod->type != mod_studio )
			continue;

		Mod_ReleaseStudioModel( mod );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Material system can force all studio models to reload on demand
//-----------------------------------------------------------------------------
void CModelLoader::Studio_RestoreModels( void )
{
	int		i;
	model_t	*mod;
	
	int c = m_Models.Count();

	for ( i=0; i< c ; i++ )
	{
		mod = m_Models[ i ].modelpointer;

		// Only need to reconstitute things that are referenced
		if ( !( mod->needload & FMODELLOADER_LOADED ) )
			continue;

		if ( mod->type != mod_studio )
			continue;
		
		Studio_LoadStaticMeshes( mod );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CModelLoader::Map_GetRenderInfoAllocated( void )
{
	return m_bMapRenderInfoLoaded;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModelLoader::Map_SetRenderInfoAllocated( bool allocated )
{
	m_bMapRenderInfoLoaded = allocated;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mod - 
//-----------------------------------------------------------------------------
void CModelLoader::Map_LoadDisplacements( model_t *mod, bool bRestoring )
{
	if ( !mod )
	{
		Assert( false );
		return;
	}

	COM_FileBase( mod->name, s_szLoadName );
	CMapLoadHelper::Init( mod, s_szLoadName );

    DispInfo_LoadDisplacements( mod, bRestoring );

	CMapLoadHelper::Shutdown();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModelLoader::Print( void )
{
	int		i;
	model_t	*mod;

	Con_Printf ("Cached models:\n");
	int c = m_Models.Count();
	for ( i=0 ; i < c ; i++ )
	{
		mod = m_Models[ i ].modelpointer;
		Con_Printf ("%8p : %s\n", mod->cache.data, mod->name );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determine if specified .bsp is valid
// Input  : *mapname - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CModelLoader::Map_IsValid( char const *mapname )
{
	if ( !mapname || !mapname[ 0 ] )
	{
		Con_Printf( "CModelLoader::Map_IsValid:  Empty mapname!!!\n" );
		return false;
	}

	FileHandle_t mapfile;

	mapfile = g_pFileSystem->Open( va( "maps/%s.bsp", mapname ), "rb" );
	if ( mapfile != FILESYSTEM_INVALID_HANDLE )
	{
		dheader_t header;
		memset( &header, 0, sizeof( header ) );
		g_pFileSystem->Read( &header, sizeof( dheader_t ), mapfile );
		g_pFileSystem->Close( mapfile );

		if ( header.ident == IDBSPHEADER )
		{
			if ( header.version == BSPVERSION )
			{
				// Map appears valid!!
				return true;
			}
			else
			{
				Warning( "CModelLoader::Map_IsValid:  Map '%s' bsp version %i, expecting %i\n", mapname, header.version, BSPVERSION );
			}
		}
		else
		{
			Warning( "CModelLoader::Map_IsValid: '%s' is not a valid BSP file\n", mapname );
		}
	}
	else
	{
		Warning( "CModelLoader::Map_IsValid:  No such map '%s'\n", mapname );
	}

	return false;
}
