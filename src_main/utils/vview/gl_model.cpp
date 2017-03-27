//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

// gl_model.cpp -- model loading and caching

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "qfiles.h"
#include "mathlib.h"
#include "gl_model.h"
#include "cmdlib.h"
#include "map_entity.h"

#include "gl_image.h"
#include "engine.h"

#pragma warning(disable : 4244)     // float->double

char			*wadpath = NULL; // Path of wad files associated with current BSP


void Mod_LoadMap( void *buffer );
model_t *Mod_LoadModel( model_t *mod, qboolean crash );


// JAY: Later
#if 0
void Mod_LoadSpriteModel( model_t *mod, void *buffer );
#endif


#define	MAX_MOD_KNOWN	512
model_t	mod_known[MAX_MOD_KNOWN];
int		mod_numknown;
// the inline * models from the current map are kept seperate
model_t	mod_inline[MAX_MOD_KNOWN];

byte	mod_novis[MAX_MAP_LEAFS/8];

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf (Vector& p, model_t *model)
{
	mnode_t		*node;
	float		d;
	cplane_t	*plane;
	
	if (!model || !model->nodes)
		engine.Error( "Mod_PointInLeaf: bad model" );

	node = model->nodes;
	while (1)
	{
		if (node->contents != -1)
			return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct( p, plane->normal ) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->vis->numclusters+7)>>3;	
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;		
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
	
	return decompressed;
}

/*
==============
Mod_ClusterPVS
==============
*/
byte *Mod_ClusterPVS (int cluster, model_t *model)
{
	if (cluster == -1 || !model->vis)
		return mod_novis;
	return Mod_DecompressVis ( (byte *)model->vis + model->vis->bitofs[cluster][DVIS_PVS],
		model);
}


//===============================================================================

#if 0
/*
================
Mod_Modellist_f
================
*/
void Mod_Modellist_f (void)
{
	int		i;
	model_t	*mod;
	int		total;

	total = 0;
	engine.Con_Printf( PRINT_ALL,"Loaded models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		engine.Con_Printf( PRINT_ALL, "%8i : %s\n",mod->extradatasize, mod->name );
		total += mod->extradatasize;
	}
	engine.Con_Printf( PRINT_ALL, "Total resident: %i\n", total );
}
#endif

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	memset (mod_novis, 0xff, sizeof(mod_novis));
}



/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod;
//	unsigned *buf;
	int		i;
	
	if (!name[0])
		engine.Error("Mod_ForName: NULL name");
		
	//
	// inline models are grabbed only from worldmodel
	//
	if (name[0] == '*')
	{
		i = atoi(name+1);
		if (i < 1 || !r_worldmodel || i >= r_worldmodel->numsubmodels)
			engine.Error ("bad inline model number");
		return &mod_inline[i];
	}

	//
	// search the currently loaded models
	//
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (!strcmp (mod->name, name) )
			return mod;
	}
	
	//
	// find a free model slot spot
	//
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			break;	// free spot
	}
	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
			engine.Error ("mod_numknown == MAX_MOD_KNOWN");
		mod_numknown++;
	}
	strcpy (mod->name, name);

#if 0
	//
	// load the file
	//
	modfilelen = ri.FS_LoadFile (mod->name, &buf);
	if (!buf)
	{
		if (crash)
			engine.Error ("Mod_NumForName: %s not found", mod->name);
		memset (mod->name, 0, sizeof(mod->name));
		return NULL;
	}
	
	loadmodel = mod;

	//
	// fill it in
	//


	// call the apropriate loader
	
	switch (LittleLong(*(unsigned *)buf))
	{
	case IDALIASHEADER:
		loadmodel->extradata = Hunk_Begin (0x200000);
		Mod_LoadAliasModel (mod, buf);
		break;
		
	case IDSPRITEHEADER:
		loadmodel->extradata = Hunk_Begin (0x10000);
		Mod_LoadSpriteModel (mod, buf);
		break;
	
	case IDBSPHEADER:
		engine.Error( "Can't load a BSP model (%s)!\n", mod->name );
		break;

	default:
		engine.Error ("Mod_NumForName: unknown fileid for %s", mod->name);
		break;
	}

	loadmodel->extradatasize = Hunk_End ();

	ri.FS_FreeFile (buf);
#endif

	return mod;
}

/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

byte	*mod_base;


/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting( model_t *loadmodel, lump_t *l )
{
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}
	loadmodel->lightdata = (colorRGBExp32 *)engine.MemAlloc( l->filelen );
	memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadWorldlights
=================
*/
void Mod_LoadWorldlights( model_t *loadmodel, lump_t *l )
{
#if 0
	int i;
	int vissize;
	byte *plightvis;
#endif

	if (!l->filelen)
	{
		loadmodel->numworldlights = 0;
		loadmodel->worldlights = NULL;
		return;
	}
	loadmodel->numworldlights = l->filelen / sizeof( dworldlight_t );
	loadmodel->worldlights = (dworldlight_t *)engine.MemAlloc( l->filelen );
	memcpy (loadmodel->worldlights, mod_base + l->fileofs, l->filelen);

#if 0
	// preexpand vis data
	vissize = (loadmodel->numleafs + 7) / 8;
	loadmodel->worldlightvis = (byte *)Hunk_AllocName( loadmodel->numworldlights * vissize, loadname );

	plightvis = loadmodel->worldlightvis;
	for (i = 0; i < loadmodel->numworldlights; i++)
	{
		byte *vis = Mod_LeafPVS( &loadmodel->leafs[loadmodel->worldlights[i].leaf], loadmodel );

		memcpy( plightvis, vis, (loadmodel->numleafs + 7) / 8 );
		plightvis += vissize;
	}
#endif
}



struct MapEntities_t
{
	int				count;
	mapentity_t		list[ MAX_MAP_ENTITIES ];
} g_MapEntities;


void StripTrailing (char *e)
{
	char	*s;

	s = e + strlen(e)-1;
	while (s >= e && *s <= 32)
	{
		*s = 0;
		s--;
	}
}

/*
=================
ParseEpair
=================
*/
char *ParseKeypair( char *pString, mapentity_t *pEntity )
{
	keypair_t	*e;

	e = (keypair_t *)malloc (sizeof(epair_t));
	memset (e, 0, sizeof(epair_t));
	
	StripTrailing (com_token);
	if (strlen(com_token) >= MAX_KEY-1)
		return NULL;
	e->key = strdup(com_token);
	pString = COM_Parse(pString);
	// strip trailing spaces
	StripTrailing (com_token);
	if (strlen(com_token) >= MAX_VALUE-1)
		return NULL;
	e->value = strdup(com_token);


	// link it in
	e->next = pEntity->pKeyValues;
	pEntity->pKeyValues = e;
	return pString;
}


/*
================
ParseEntity
================
*/
char *ParseEntity( char *pString )
{
	mapentity_t	*mapent;

	if (! (pString = COM_Parse(pString)) )
		return NULL;

	if (strcmp (com_token, "{") )
		return NULL;
	
	if (g_MapEntities.count == MAX_MAP_ENTITIES)
		Error ("g_MapEntities.count == MAX_MAP_ENTITIES");

	mapent = g_MapEntities.list + g_MapEntities.count;
	g_MapEntities.count++;

	do
	{
		pString = COM_Parse( pString );
		if ( !pString )
			return NULL;

		if (!strcmp (com_token, "}") )
			break;
		pString = ParseKeypair( pString, mapent );
		if ( mapent->pKeyValues )
		{
			if ( !strcmp(mapent->pKeyValues->key, "classname") )
			{
				mapent->pClassName = mapent->pKeyValues->value;
			}
			else if ( !strcmp( mapent->pKeyValues->key, "origin") )
			{
				sscanf( mapent->pKeyValues->value, "%f %f %f", &mapent->origin[0], &mapent->origin[1], &mapent->origin[2] );
			}
			else if ( !strcmp( mapent->pKeyValues->key, "angles") )
			{
				sscanf( mapent->pKeyValues->value, "%f %f %f", &mapent->angles[0], &mapent->angles[1], &mapent->angles[2] );
			}
			else if ( !strcmp(mapent->pKeyValues->key, "model") )
			{
				mapent->pModelName = mapent->pKeyValues->value;
			}
		}
	} while (1);
	
	return pString;
}

mapentity_t *Map_EntityIndex( int index )
{
	if ( index >= 0 && index < g_MapEntities.count )
		return g_MapEntities.list + index;

	return NULL;
}


mapentity_t *Map_FindEntity( const char *pClassName )
{
	for ( int i = 0; i < g_MapEntities.count; i++ )
	{
		if ( !strcmp( g_MapEntities.list[i].pClassName, pClassName ) )
			return g_MapEntities.list + i;
	}
	return NULL;
}

int	Map_EntityCount( void )
{
	return g_MapEntities.count;
}

const char *Map_EntityValue( const mapentity_t *pEntity, const char *pKeyName )
{
	keypair_t *pKeys = pEntity->pKeyValues;
	while ( pKeys )
	{
		if ( !strcmp( pKeys->key, pKeyName ) )
			return pKeys->value;
		pKeys = pKeys->next;
	}

	return NULL;
}

void Map_ParseEntities( char *pEntData )
{
	g_MapEntities.count = 0;
	
	while ( pEntData = ParseEntity( pEntData ) )
	{
	}
}


/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities( model_t *loadmodel, lump_t *l )
{
	char	*pszInputStream, *pEntities;

	loadmodel->entities = NULL;
	pEntities = (char *) (mod_base + l->fileofs);

	if ( pEntities )
	{
		// Parse the WAD file name from the entity lump
		for ( pszInputStream = COM_Parse(pEntities); // '{'  this is here to fix DevStudio's brace matching
			  *pszInputStream && *com_token != '}';
			  pszInputStream = COM_Parse(pszInputStream) )
		{
			if ( strcmp(com_token,"wad") == 0 )
			{
				pszInputStream = COM_Parse(pszInputStream);
				if ( wadpath ) 
					free(wadpath);
				wadpath = strdup( com_token );
				break;
			}
		}
		Map_ParseEntities( pEntities );

	}
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility( model_t *loadmodel, lump_t *l )
{
	int		i;

	if (!l->filelen)
	{
		loadmodel->vis = NULL;
		return;
	}
	loadmodel->vis = (dvis_t *)engine.MemAlloc( l->filelen );
	memcpy (loadmodel->vis, mod_base + l->fileofs, l->filelen);

	loadmodel->vis->numclusters = LittleLong (loadmodel->vis->numclusters);
	for (i=0 ; i<loadmodel->vis->numclusters ; i++)
	{
		loadmodel->vis->bitofs[i][0] = LittleLong (loadmodel->vis->bitofs[i][0]);
		loadmodel->vis->bitofs[i][1] = LittleLong (loadmodel->vis->bitofs[i][1]);
	}
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes( model_t *loadmodel, lump_t *l )
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	in = (dvertex_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mvertex_t *)engine.MemAlloc( count*sizeof(*out) );

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds (Vector& mins, Vector& maxs)
{
	int		i;
	Vector	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength (corner);
}


/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels( model_t *loadmodel, lump_t *l )
{
	dmodel_t	*in;
	mmodel_t		*out;
	int			i, j, count;

	in = (dmodel_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mmodel_t *)engine.MemAlloc( count*sizeof(*out) );

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

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

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges (model_t *loadmodel, lump_t *l)
{
	dedge_t *in;
	medge_t *out;
	int 	i, count;

	in = (dedge_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (medge_t *)engine.MemAlloc( (count + 1) * sizeof(*out));	

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}


/*
=================
Mod_LoadTexdata
=================
*/
void Mod_LoadTexdata (model_t *loadmodel, lump_t *l)
{
	dtexdata_t *in;
	mtexdata_t *out;
	int 	i, count;

	in = (dtexdata_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mtexdata_t *)engine.MemAlloc( count*sizeof(*out));	

	loadmodel->texdata = out;
	loadmodel->numtexdata = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		strcpy( out->name, in->name );
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo (model_t *loadmodel, lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count;

	in = (texinfo_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mtexinfo_t *)engine.MemAlloc( count*sizeof(*out));	

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<8 ; j++)
		{
			out->textureVecsTexelsPerWorldUnits[0][j] = LittleFloat (in->textureVecsTexelsPerWorldUnits[0][j]);
			out->lightmapVecsLuxelsPerWorldUnits[0][j] = LittleFloat (in->lightmapVecsLuxelsPerWorldUnits[0][j]);
		}

		out->flags = LittleLong (in->flags);

		out->image = GL_LoadImage( loadmodel->texdata[ in->texdata ].name );
		if ( !out->image )
		{
			out->image = GL_EmptyTexture();
		}
	}

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Mod_LoadDispInfo( model_t *pModel, lump_t *pLump )
{
    // don't deal with displacement maps currently
    return;
}


/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents (model_t *loadmodel, msurface_t *s)
{
	float	lightmapMins[2], lightmapMaxs[2];
	float	textureMins[2], textureMaxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	lightmapMins[0] = lightmapMins[1] = 999999;
	lightmapMaxs[0] = lightmapMaxs[1] = -99999;
	textureMins[0] = textureMins[1] = 999999;
	textureMaxs[0] = textureMaxs[1] = -99999;

	tex = s->texinfo;
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		
		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->lightmapVecsLuxelsPerWorldUnits[j][0] + 
				  v->position[1] * tex->lightmapVecsLuxelsPerWorldUnits[j][1] +
				  v->position[2] * tex->lightmapVecsLuxelsPerWorldUnits[j][2] +
				  tex->lightmapVecsLuxelsPerWorldUnits[j][3];
			if (val < lightmapMins[j])
				lightmapMins[j] = val;
			if (val > lightmapMaxs[j])
				lightmapMaxs[j] = val;

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
		bmins[i] = floor( lightmapMins[i] );
		bmaxs[i] = ceil( lightmapMaxs[i] );
		s->lightmapMins[i] = bmins[i];
		s->lightmapExtents[i] = ( bmaxs[i] - bmins[i] );

		bmins[i] = floor( textureMins[i] );
		bmaxs[i] = ceil( textureMaxs[i] );
		s->textureMins[i] = bmins[i];
		s->textureExtents[i] = ( bmaxs[i] - bmins[i] );

#if 0
		if ( !(tex->flags & TEX_SPECIAL) && s->lightmapExtents[i] > MAX_LIGHTMAP_DIM_INCLUDING_BORDER )
		{
			Sys_Error ("Bad surface extents on texture %s", tex->material->GetName() );
		}
#endif
	}
}


void GL_BuildPolygonFromSurface(msurface_t *fa);
void GL_CreateSurfaceLightmap (msurface_t *surf);
void GL_EndBuildingLightmaps (void);
void GL_BeginBuildingLightmaps (model_t *m);

/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces( model_t *loadmodel, lump_t *l )
{
	dface_t		*in;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum;
	int			ti;

	in = (dface_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t *)engine.MemAlloc( count*sizeof(*out) );

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);		
		out->flags = 0;
//		out->polys = NULL;

		planenum = (unsigned short)LittleShort(in->planenum);
		if ( in->onNode )
			out->flags |= SURFDRAW_NODE;

		out->plane = loadmodel->planes + planenum;

		ti = LittleShort (in->texinfo);
		if (ti < 0 || ti >= loadmodel->numtexinfo)
			engine.Error ("MOD_LoadBmodel: bad texinfo number");
		out->texinfo = loadmodel->texinfo + ti;

		CalcSurfaceExtents (loadmodel, out);
				
	// lighting info

		for (i=0 ; i<MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = (colorRGBExp32 *)( ((byte *)loadmodel->lightdata) + i );
	// set the drawing flags flag
		if ( out->texinfo->flags & SURF_NOLIGHT )
		{
			out->flags |= SURFDRAW_NOLIGHT;
		}
		// skip these faces
		if ( out->texinfo->flags & SURF_NODRAW )
		{
			out->flags |= SURFDRAW_NODRAW;
		}
		
#ifdef STATIC_FOG
		// UNDONE: Mark these faces somehow and fix this hack
        if ( out->texinfo->image->name[0] == '%' )
        {
            out->flags |= ( SURFDRAW_FOG );
        }
#endif

		if ( out->texinfo->flags & SURF_WARP )
		{
			out->flags |= SURFDRAW_TURB;
		}


		if ( out->texinfo->flags & SURF_SKY )
		{
			out->flags |= SURFDRAW_SKY;
//			R_InitSky();
		}
	}
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}


/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes( model_t *loadmodel, lump_t *l )
{
	int			i, j, count, p;
	dnode_t		*in;
	mnode_t 	*out;

	in = (dnode_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mnode_t *)engine.MemAlloc( count*sizeof(*out) );

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleShort (in->mins[j]);
			out->maxs[j] = LittleShort (in->maxs[j]);
		}
	
		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = (unsigned short)LittleShort (in->firstface);
		out->numsurfaces = (unsigned short)LittleShort (in->numfaces);
		out->contents = -1;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
		}
	}
	
	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs( model_t *loadmodel, lump_t *l )
{
	dleaf_t 	*in;
	mleaf_t 	*out;
	int			i, j, count, p;
//	glpoly_t	*poly;

	in = (dleaf_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mleaf_t *)engine.MemAlloc( count*sizeof(*out) );

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->mins[j] = LittleShort (in->mins[j]);
			out->maxs[j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		out->firstmarksurface = loadmodel->marksurfaces +
			(unsigned short)LittleShort(in->firstleafface);
		out->nummarksurfaces = (unsigned short)LittleShort(in->numleaffaces);
		out->parent = NULL;
		
		// gl underwater warp
#if 0
		if (out->contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_THINWATER) )
		{
			for (j=0 ; j<out->nummarksurfaces ; j++)
			{
				out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				for (poly = out->firstmarksurface[j]->polys ; poly ; poly=poly->next)
					poly->flags |= SURF_UNDERWATER;
			}
		}
#endif
	}	
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces( model_t *loadmodel, lump_t *l )
{	
	int		i, j, count;
	short		*in;
	msurface_t **out;
	
	in = (short *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (msurface_t **)engine.MemAlloc( count*sizeof(*out) );

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = (unsigned short)LittleShort(in[i]);
		if (j < 0 ||  j >= loadmodel->numsurfaces)
			engine.Error ("Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges( model_t *loadmodel, lump_t *l )
{	
	int		i, count;
	int		*in, *out;
	
	in = (int *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	if (count < 1 || count >= MAX_MAP_SURFEDGES)
		engine.Error ("MOD_LoadBmodel: bad surfedges count in %s: %i",
		loadmodel->name, count);

	out = (int *)engine.MemAlloc( count*sizeof(*out) );

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes( model_t *loadmodel, lump_t *l )
{
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (dplane_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (cplane_t *)engine.MemAlloc( count*2*sizeof(*out) );
	
	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Loads the vertex index list for all portals
// Input  : *loadmodel - 
//			*l - 
//-----------------------------------------------------------------------------
void Mod_LoadPortalVerts( model_t *loadmodel, lump_t *l )
{
	int			i;
	unsigned short	*out;
	unsigned short	*in;
	int			count;
	
	in = (unsigned short  *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (unsigned short *)engine.MemAlloc( count*sizeof(*out) );
	
	loadmodel->portalverts = out;
	loadmodel->numportalverts = count;

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
void Mod_LoadClusterPortals( model_t *loadmodel, lump_t *l )
{
	int			i;
	unsigned short	*out;
	unsigned short	*in;
	int			count;
	
	in = (unsigned short  *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (unsigned short *)engine.MemAlloc( count*sizeof(*out) );
	
	loadmodel->clusterportals = out;
	loadmodel->numclusterportals = count;

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
void Mod_LoadClusters( model_t *loadmodel, lump_t *l )
{
	int			i;
	mcluster_t	*out;
	dcluster_t	*in;
	int			count;
	
	in = (dcluster_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mcluster_t *)engine.MemAlloc( count*sizeof(*out) );
	
	loadmodel->clusters = out;
	loadmodel->numclusters = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		out->numportals = LittleLong( in->numportals );
		out->portalList = loadmodel->clusterportals + LittleLong( in->firstportal );
	}
}
	
void Mod_LoadPortals( model_t *loadmodel, lump_t *l )
{
	int			i;
	mportal_t	*out;
	dportal_t	*in;
	int			count;
	
	in = (dportal_t *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error ("MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = (mportal_t *)engine.MemAlloc( count*sizeof(*out) );
	
	loadmodel->portals = out;
	loadmodel->numportals = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		out->cluster[0] = (unsigned short)LittleShort( in->cluster[0] );
		out->cluster[1] = (unsigned short)LittleShort( in->cluster[1] );
		out->numportalverts = LittleLong( in->numportalverts );
		out->plane = loadmodel->planes + LittleLong( in->planenum );
		out->vertList = loadmodel->portalverts + LittleLong( in->firstportalvert );
		out->visframe = 0;
	}
}


model_t *r_worldmodel = NULL;
/*
=================
Mod_LoadMap
=================
*/
void Mod_LoadMap( void *buffer )
{
	int			i;
	dheader_t	*header;
	model_t		*mod = mod_known;
		
	mod->type = mod_brush;
	if (mod != mod_known)
		engine.Error ("Loaded a brush model after the world");

	header = (dheader_t *)buffer;

	i = LittleLong (header->version);
	if (i != BSPVERSION)
		engine.Error ("Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSPVERSION);

// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap
	
	// we don't really load the entities, just parse out the .wads' that are used
	// This should go away when shaders are done
	Mod_LoadEntities( mod, &header->lumps[LUMP_ENTITIES] );

	Mod_LoadVertexes( mod, &header->lumps[LUMP_VERTEXES] );
	Mod_LoadEdges( mod, &header->lumps[LUMP_EDGES] );
	Mod_LoadSurfedges( mod, &header->lumps[LUMP_SURFEDGES] );
	Mod_LoadLighting( mod, &header->lumps[LUMP_LIGHTING] );
	Mod_LoadPlanes( mod, &header->lumps[LUMP_PLANES] );
	// texdata needs to load before texinfo
	Mod_LoadTexdata( mod, &header->lumps[LUMP_TEXDATA] );

	Mod_LoadTexinfo( mod, &header->lumps[LUMP_TEXINFO] );
    Mod_LoadDispInfo( mod, &header->lumps[LUMP_DISPINFO] );
	Mod_LoadFaces( mod, &header->lumps[LUMP_FACES] );
	Mod_LoadMarksurfaces( mod, &header->lumps[LUMP_LEAFFACES] );

	// UNDONE: Client shouldn't need visibility, use common model
	Mod_LoadVisibility( mod, &header->lumps[LUMP_VISIBILITY] );

	Mod_LoadLeafs( mod, &header->lumps[LUMP_LEAFS] );
	Mod_LoadNodes( mod, &header->lumps[LUMP_NODES] );
	
	// UNDONE: Does the cmodel need worldlights?
	Mod_LoadWorldlights( mod, &header->lumps[LUMP_WORLDLIGHTS] );
	
	// load the portal information
	Mod_LoadPortalVerts( mod, &header->lumps[LUMP_PORTALVERTS] );
	Mod_LoadClusterPortals( mod, &header->lumps[LUMP_CLUSTERPORTALS] );
	Mod_LoadClusters( mod, &header->lumps[LUMP_CLUSTERS] );
	Mod_LoadPortals( mod, &header->lumps[LUMP_PORTALS] );
	
//	mod->numframes = 2;		// regular and alternate animation

	Mod_LoadSubmodels( mod, &header->lumps[LUMP_MODELS] );
//
// set up the submodels
//
	for (i=0 ; i<mod->numsubmodels ; i++)
	{
		model_t	*starmod;
		mmodel_t 	*bm;

		bm = &mod->submodels[i];
		starmod = &mod_inline[i];

		*starmod = *mod;
		
		starmod->firstmodelsurface = bm->firstface;
		starmod->nummodelsurfaces = bm->numfaces;
		starmod->firstnode = bm->headnode;
		if (starmod->firstnode >= mod->numnodes)
			engine.Error ("Inline model %i has bad firstnode", i);

		VectorCopy (bm->maxs, starmod->maxs);
		VectorCopy (bm->mins, starmod->mins);
		starmod->radius = bm->radius;
	
		if (i == 0)
			*mod = *starmod;

		starmod->numleafs = bm->visleafs;
	}

	r_worldmodel = mod;
	GL_BeginBuildingLightmaps( mod );
	for ( i = 0; i < mod->numsurfaces; i++ )
	{
		GL_CreateSurfaceLightmap( mod->surfaces + i );
	}
	GL_EndBuildingLightmaps();
}

#if 0
/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel (model_t *mod, void *buffer)
{
	dsprite_t	*sprin, *sprout;
	int			i;

	sprin = (dsprite_t *)buffer;
	sprout = Hunk_Alloc (modfilelen);

	sprout->ident = LittleLong (sprin->ident);
	sprout->version = LittleLong (sprin->version);
	sprout->numframes = LittleLong (sprin->numframes);

	if (sprout->version != SPRITE_VERSION)
		engine.Error ("%s has wrong version number (%i should be %i)",
				 mod->name, sprout->version, SPRITE_VERSION);

	if (sprout->numframes > MAX_MD2SKINS)
		engine.Error ("%s has too many frames (%i > %i)",
				 mod->name, sprout->numframes, MAX_MD2SKINS);

	// byte swap everything
	for (i=0 ; i<sprout->numframes ; i++)
	{
		sprout->frames[i].width = LittleLong (sprin->frames[i].width);
		sprout->frames[i].height = LittleLong (sprin->frames[i].height);
		sprout->frames[i].origin_x = LittleLong (sprin->frames[i].origin_x);
		sprout->frames[i].origin_y = LittleLong (sprin->frames[i].origin_y);
		memcpy (sprout->frames[i].name, sprin->frames[i].name, MAX_SKINNAME);
		mod->skins[i] = GL_FindImage (sprout->frames[i].name,
			it_sprite);
	}

	mod->type = mod_sprite;
}
#endif


#if 0
//=============================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginRegistration (char *model)
{
	char	fullname[MAX_QPATH];
	cvar_t	*flushmap;

	registration_sequence++;
	r_oldviewcluster = -1;		// force markleafs

	Com_sprintf (fullname, sizeof(fullname), "maps/%s.bsp", model);

	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	flushmap = ri.Cvar_Get ("flushmap", "0", 0);
	if ( strcmp(mod_known[0].name, fullname) || flushmap->value)
		Mod_Free (&mod_known[0]);
	r_worldmodel = Mod_ForName(fullname, true);

	r_viewcluster = -1;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_RegisterModel

@@@@@@@@@@@@@@@@@@@@@
*/
struct model_s *R_RegisterModel (char *name)
{
	model_t	*mod;
	int		i;
	dsprite_t	*sprout;
	dmdl_t		*pheader;

	mod = Mod_ForName (name, false);
	if (mod)
	{
		mod->registration_sequence = registration_sequence;

		// register any images used by the models
		if (mod->type == mod_sprite)
		{
			sprout = (dsprite_t *)mod->extradata;
			for (i=0 ; i<sprout->numframes ; i++)
				mod->skins[i] = GL_FindImage (sprout->frames[i].name, it_sprite);
		}
		else if (mod->type == mod_alias)
		{
			pheader = (dmdl_t *)mod->extradata;
			for (i=0 ; i<pheader->num_skins ; i++)
				mod->skins[i] = GL_FindImage ((char *)pheader + pheader->ofs_skins + i*MAX_SKINNAME, it_skin);
		}
		else if (mod->type == mod_brush)
		{
			for (i=0 ; i<mod->numtexinfo ; i++)
				mod->texinfo[i].image->registration_sequence = registration_sequence;
		}
	}
	return mod;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndRegistration (void)
{
	int		i;
	model_t	*mod;

	for (i=0, mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (mod->registration_sequence != registration_sequence)
		{	// don't need this model
			Mod_Free (mod);
		}
	}

	GL_FreeUnusedImages ();
}


//=============================================================================


/*
================
Mod_Free
================
*/
void Mod_Free (model_t *mod)
{
	Hunk_Free (mod->extradata);
	memset (mod, 0, sizeof(*mod));
}

/*
================
Mod_FreeAll
================
*/
void Mod_FreeAll (void)
{
	int		i;

	for (i=0 ; i<mod_numknown ; i++)
	{
		if (mod_known[i].extradatasize)
			Mod_Free (&mod_known[i]);
	}
}

#endif

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
int r_oldviewcluster = -2, r_viewcluster = -2, r_oldviewcluster2 = -2, r_viewcluster2 = -2, r_visframecount = 0;

void R_MarkLeaves( void );

void R_SetupVis( Vector& origin, model_t *worldmodel )
{
	mleaf_t *leaf;
	
	r_worldmodel = worldmodel;
	leaf = Mod_PointInLeaf( origin, worldmodel );
	r_viewcluster = r_viewcluster2 = leaf->cluster;
	R_MarkLeaves();
}


void R_MarkLeaves( void )
{
	byte	*vis;
	byte	fatvis[MAX_MAP_LEAFS/8];
	mnode_t	*node;
	int		i, c;
	mleaf_t	*leaf;
	int		cluster;

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if (gl_lockpvs->value)
		return;

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !r_worldmodel->vis)
	{
		// mark everything
		for (i=0 ; i<r_worldmodel->numleafs ; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i=0 ; i<r_worldmodel->numnodes ; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		for ( i = 0; i < r_worldmodel->numportals; i++ )
			r_worldmodel->portals[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_ClusterPVS (r_viewcluster, r_worldmodel);
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		memcpy (fatvis, vis, (r_worldmodel->numleafs+7)/8);
		vis = Mod_ClusterPVS (r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatvis)[i] |= ((int *)vis)[i];
		vis = fatvis;
	}
	
	for (i=0,leaf=r_worldmodel->leafs ; i<r_worldmodel->numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;

		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			int j;
			unsigned short *start;

			if ( r_worldmodel->portals )
			{
				start = r_worldmodel->clusters[ cluster ].portalList;
				for ( j = 0; j < r_worldmodel->clusters[ cluster ].numportals; j++ )
				{
					r_worldmodel->portals[ *start ].visframe = r_visframecount;
					start++;
				}
			}

			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}



