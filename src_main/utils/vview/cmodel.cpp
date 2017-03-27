// cmodel.cpp -- model loading
#include "qfiles.h"
#include "cmdlib.h"
#include "mathlib.h"
#include "cmodel_engine.h"
#include "engine.h"

typedef struct
{
	cplane_t	*plane;
	int			children[2];		// negative numbers are leafs
} cnode_t;

typedef struct
{
	cplane_t	*plane;
	csurface_t	*surface;
} cbrushside_t;

typedef struct cleaf_s
{
	int			contents;
	int			cluster;
	int			area;
	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
} cleaf_t;

typedef struct cbrush_s
{
	int			contents;
	int			numsides;
	int			firstbrushside;
	int			checkcount;		// to avoid repeated testings
} cbrush_t;

typedef struct carea_s
{
	int		numareaportals;
	int		firstareaportal;
	int		floodnum;			// if two areas have equal floodnums, they are connected
	int		floodvalid;
} carea_t;

static int			checkcount;

static char			map_name[MAX_QPATH];

static int			numbrushsides;
static cbrushside_t map_brushsides[MAX_MAP_BRUSHSIDES];

static int			numtexinfo;
static csurface_t	map_surfaces[MAX_MAP_TEXINFO];

static int			numplanes;
static cplane_t		map_planes[MAX_MAP_PLANES+6];		// extra for box hull

static int			numnodes;
static cnode_t		map_nodes[MAX_MAP_NODES+6];		// extra for box hull

static int			numleafs = 1;	// allow leaf funcs to be called without a map
static cleaf_t		map_leafs[MAX_MAP_LEAFS];
static int			emptyleaf, solidleaf;

static int			numleafbrushes;
static unsigned short	map_leafbrushes[MAX_MAP_LEAFBRUSHES];

static int			numcmodels;
static cmodel_t		map_cmodels[MAX_MAP_MODELS];

static int			numbrushes;
static cbrush_t		map_brushes[MAX_MAP_BRUSHES];

static int			numvisibility;
static byte			map_visibility[MAX_MAP_VISIBILITY];
static dvis_t		*map_vis = (dvis_t *)map_visibility;

static int			numentitychars;
static char			map_entitystring[MAX_MAP_ENTSTRING];

static int			numareas = 1;
static carea_t		map_areas[MAX_MAP_AREAS];

static int			numareaportals;
static dareaportal_t map_areaportals[MAX_MAP_AREAPORTALS];

static int			numclusters = 1;

static csurface_t	nullsurface;

static int			floodvalid;

static qboolean		portalopen[MAX_MAP_AREAPORTALS];


static cvar_t		map_noareas;

void	CM_InitBoxHull (void);
void	FloodAreaConnections (void);


static int		c_pointcontents;
static int		c_traces, c_brush_traces;

typedef struct cmodel_collision_s
{
	cmodel_t	*pmodels;
	int			modelCount;
	cplane_t	*pplanes;
	int			planeCount;
	cnode_t		*pnodes;
	int			nodeCount;
	cleaf_t		*pleafs;
	int			leafCount;
} cmodel_collision_t;

void CM_GetCollisionModel( cmodel_collision_t *pcollide )
{
	pcollide->pmodels = map_cmodels;
	pcollide->modelCount = numcmodels;
	pcollide->pplanes = map_planes;
	pcollide->planeCount = numplanes;
	pcollide->pnodes = map_nodes;
	pcollide->nodeCount = numnodes;
	pcollide->pleafs = map_leafs;
	pcollide->leafCount = numleafs;
}

/*
===============================================================================

					MAP LOADING

===============================================================================
*/

static byte	*cmod_base;
/*
=================
CMod_LoadSubmodels
=================
*/
void CMod_LoadSubmodels (lump_t *l)
{
	dmodel_t	*in;
	cmodel_t	*out;
	int			i, j, count;

	in = (dmodel_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		engine.Error( "Map with no models" );
	if (count > MAX_MAP_MODELS)
		engine.Error( "Map has too many models" );

	numcmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out = &map_cmodels[i];

		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		out->headnode = LittleLong (in->headnode);
	}
}


/*
=================
CMod_LoadSurfaces
=================
*/
void CMod_LoadSurfaces( lump_t *l, dtexdata_t *pTexData, int texDataCount,
					   char* pStringData, const int* pStringTable )
{
	texinfo_t	*in;
	csurface_t	*out;
	int			i, count;

	in = (texinfo_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error( "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);
	if (count < 1)
		engine.Error( "Map with no surfaces");
	if (count > MAX_MAP_TEXINFO)
		engine.Error( "Map has too many surfaces");

	numtexinfo = count;
	out = map_surfaces;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		if ( in->texdata < texDataCount )
		{
			int texNameId = pTexData[in->texdata].nameStringTableID;
			strncpy( out->name, &pStringData[pStringTable[texNameId]], sizeof(out->name)-1);
		}
		out->flags = LittleLong (in->flags);
	}
}


/*
=================
CMod_LoadNodes

=================
*/
void CMod_LoadNodes (lump_t *l)
{
	dnode_t		*in;
	int			child;
	cnode_t		*out;
	int			i, j, count;
	
	in = (dnode_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error( "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		engine.Error( "Map has no nodes");
	if (count > MAX_MAP_NODES)
		engine.Error( "Map has too many nodes");

	out = map_nodes;

	numnodes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->plane = map_planes + LittleLong(in->planenum);
		for (j=0 ; j<2 ; j++)
		{
			child = LittleLong (in->children[j]);
			out->children[j] = child;
		}
	}

}

/*
=================
CMod_LoadBrushes

=================
*/
void CMod_LoadBrushes (lump_t *l)
{
	dbrush_t	*in;
	cbrush_t	*out;
	int			i, count;
	
	in = (dbrush_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error( "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_BRUSHES)
		engine.Error( "Map has too many brushes");

	out = map_brushes;

	numbrushes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->firstbrushside = LittleLong(in->firstside);
		out->numsides = LittleLong(in->numsides);
		out->contents = LittleLong(in->contents);
	}

}

/*
=================
CMod_LoadLeafs
=================
*/
void CMod_LoadLeafs (lump_t *l)
{
	int			i;
	cleaf_t		*out;
	dleaf_t 	*in;
	int			count;
	
	in = (dleaf_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error( "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		engine.Error( "Map with no leafs");
	// need to save space for box planes
	if (count > MAX_MAP_PLANES)
		engine.Error( "Map has too many planes");

	out = map_leafs;	
	numleafs = count;
	numclusters = 0;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->contents = LittleLong (in->contents);
		out->cluster = LittleShort (in->cluster);
		out->area = LittleShort (in->area);
		out->firstleafbrush = (unsigned short)LittleShort (in->firstleafbrush);
		out->numleafbrushes = (unsigned short)LittleShort (in->numleafbrushes);

		if (out->cluster >= numclusters)
			numclusters = out->cluster + 1;
	}

	if (map_leafs[0].contents != CONTENTS_SOLID)
		engine.Error( "Map leaf 0 is not CONTENTS_SOLID");
	solidleaf = 0;
	emptyleaf = -1;
	for (i=1 ; i<numleafs ; i++)
	{
		if (!map_leafs[i].contents)
		{
			emptyleaf = i;
			break;
		}
	}
	if (emptyleaf == -1)
		engine.Error( "Map does not have an empty leaf");
}

/*
=================
CMod_LoadPlanes
=================
*/
void CMod_LoadPlanes (lump_t *l)
{
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (dplane_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error( "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		engine.Error( "Map with no planes");
	// need to save space for box planes
	if (count > MAX_MAP_PLANES)
		engine.Error( "Map has too many planes");

	out = map_planes;	
	numplanes = count;

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

/*
=================
CMod_LoadLeafBrushes
=================
*/
void CMod_LoadLeafBrushes (lump_t *l)
{
	int			i;
	unsigned short	*out;
	unsigned short 	*in;
	int			count;
	
	in = (unsigned short *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error( "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		engine.Error( "Map with no planes");
	// need to save space for box planes
	if (count > MAX_MAP_LEAFBRUSHES)
		engine.Error( "Map has too many leafbrushes");

	out = map_leafbrushes;
	numleafbrushes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
		*out = (unsigned short)LittleShort (*in);
}

/*
=================
CMod_LoadBrushSides
=================
*/
void CMod_LoadBrushSides (lump_t *l)
{
	int			i, j;
	cbrushside_t	*out;
	dbrushside_t 	*in;
	int			count;
	int			num;

	in = (dbrushside_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error( "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	// need to save space for box planes
	if (count > MAX_MAP_BRUSHSIDES)
		engine.Error( "Map has too many planes");

	out = map_brushsides;	
	numbrushsides = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		num = (unsigned short)LittleShort (in->planenum);
		out->plane = &map_planes[num];
		j = LittleShort (in->texinfo);
		if (j >= numtexinfo)
			engine.Error( "Bad brushside texinfo");
		out->surface = &map_surfaces[j];
	}
}

/*
=================
CMod_LoadAreas
=================
*/
void CMod_LoadAreas (lump_t *l)
{
	int			i;
	carea_t		*out;
	darea_t 	*in;
	int			count;

	in = (darea_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error( "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_AREAS)
		engine.Error( "Map has too many areas");

	out = map_areas;
	numareas = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->numareaportals = LittleLong (in->numareaportals);
		out->firstareaportal = LittleLong (in->firstareaportal);
		out->floodvalid = 0;
		out->floodnum = 0;
	}
}

/*
=================
CMod_LoadAreaPortals
=================
*/
void CMod_LoadAreaPortals (lump_t *l)
{
	int			i;
	dareaportal_t		*out;
	dareaportal_t 	*in;
	int			count;

	in = (dareaportal_t *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		engine.Error( "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_AREAS)
		engine.Error( "Map has too many areas");

	out = map_areaportals;
	numareaportals = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->portalnum = LittleLong (in->portalnum);
		out->otherarea = LittleLong (in->otherarea);
	}
}

/*
=================
CMod_LoadVisibility
=================
*/
void CMod_LoadVisibility (lump_t *l)
{
	int		i;

	numvisibility = l->filelen;
	if (l->filelen > MAX_MAP_VISIBILITY)
		engine.Error( "Map has too large visibility lump");

	memcpy (map_visibility, cmod_base + l->fileofs, l->filelen);

	map_vis->numclusters = LittleLong (map_vis->numclusters);
	for (i=0 ; i<map_vis->numclusters ; i++)
	{
		map_vis->bitofs[i][0] = LittleLong (map_vis->bitofs[i][0]);
		map_vis->bitofs[i][1] = LittleLong (map_vis->bitofs[i][1]);
	}
}


/*
=================
CMod_LoadEntityString
=================
*/
void CMod_LoadEntityString (lump_t *l)
{
	numentitychars = l->filelen;
	if (l->filelen > MAX_MAP_ENTSTRING)
		engine.Error( "Map has too large entity lump");

	memcpy( map_entitystring, cmod_base + l->fileofs, l->filelen );
}



/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
cmodel_t *CM_LoadMap( char *name, qboolean clientload, unsigned *checksum )
{
	unsigned		*buf;
	int				i;
	dheader_t		header;
	int				length;
	static unsigned	last_checksum;

	// UNDONE: Don't load the map twice - call this from a function that loads once,
	// parses this part first, then parses/copies the renderer data

	// JAY: Later
//	map_noareas = Cvar_Get ("map_noareas", "0", 0);

	if (  !strcmp (map_name, name) && (clientload /*|| !Cvar_VariableValue ("flushmap")*/) )
	{
		*checksum = last_checksum;
		if (!clientload)
		{
			memset (portalopen, 0, sizeof(portalopen));
			FloodAreaConnections ();
		}
		return &map_cmodels[0];		// still have the right version
	}

	// free old stuff
	numplanes = 0;
	numnodes = 0;
	numleafs = 0;
	numcmodels = 0;
	numvisibility = 0;
	numentitychars = 0;
	map_entitystring[0] = 0;
	map_name[0] = 0;

	if (!name || !name[0])
	{
		numleafs = 1;
		numclusters = 1;
		numareas = 1;
		*checksum = 0;
		return &map_cmodels[0];			// cinematic servers won't have anything at all
	}

	//
	// load the file
	//
	length = engine.LoadFile( name, (void **)&buf );
	if (!buf)
		engine.Error( "Couldn't load %s", name);

	// JAY: Later
	last_checksum = 0;//LittleLong (Com_BlockChecksum (buf, length));
	*checksum = last_checksum;

	header = *(dheader_t *)buf;
	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)&header)[i] = LittleLong ( ((int *)&header)[i]);

	if (header.version != BSPVERSION)
		engine.Error( "CMod_LoadBrushModel: %s has wrong version number (%i should be %i)"
		, name, header.version, BSPVERSION);

	cmod_base = (byte *)buf;

	// load into heap
	dtexdata_t *pTexData = (dtexdata_t *)(cmod_base + header.lumps[LUMP_TEXDATA].fileofs);
	int texDataCount = header.lumps[LUMP_TEXDATA].filelen / sizeof(*pTexData);

	CMod_LoadSurfaces( &header.lumps[LUMP_TEXINFO], pTexData, texDataCount );
	CMod_LoadLeafs( &header.lumps[LUMP_LEAFS] );
	CMod_LoadLeafBrushes( &header.lumps[LUMP_LEAFBRUSHES] );
	CMod_LoadPlanes( &header.lumps[LUMP_PLANES] );
	CMod_LoadBrushes( &header.lumps[LUMP_BRUSHES] );
	CMod_LoadBrushSides( &header.lumps[LUMP_BRUSHSIDES] );
	CMod_LoadSubmodels( &header.lumps[LUMP_MODELS] );
	CMod_LoadNodes( &header.lumps[LUMP_NODES] );
	CMod_LoadAreas( &header.lumps[LUMP_AREAS] );
	CMod_LoadAreaPortals( &header.lumps[LUMP_AREAPORTALS] );
	CMod_LoadVisibility( &header.lumps[LUMP_VISIBILITY] );
	CMod_LoadEntityString( &header.lumps[LUMP_ENTITIES] );

	engine.FreeFile( buf );

	CM_InitBoxHull();

	memset( portalopen, 0, sizeof(portalopen) );
	FloodAreaConnections();

	strcpy( map_name, name );

	return &map_cmodels[0];
}

/*
==================
CM_InlineModel
==================
*/
cmodel_t	*CM_InlineModel (char *name)
{
	int		num;

	if (!name || name[0] != '*')
		engine.Error( "CM_InlineModel: bad name");
	num = atoi (name+1);
	if (num < 1 || num >= numcmodels)
		engine.Error( "CM_InlineModel: bad number");
	return CM_InlineModelNumber( num );
}

cmodel_t	*CM_InlineModelNumber( int index )
{
	if ( index < 0 || index > numcmodels )
		return NULL;

	return map_cmodels + index;
}

int		CM_NumClusters (void)
{
	return numclusters;
}

int		CM_NumInlineModels (void)
{
	return numcmodels;
}

char	*CM_EntityString (void)
{
	return map_entitystring;
}

int		CM_LeafContents (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		engine.Error( "CM_LeafContents: bad number");
	return map_leafs[leafnum].contents;
}

int		CM_LeafCluster (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		engine.Error( "CM_LeafCluster: bad number");
	return map_leafs[leafnum].cluster;
}

int		CM_LeafArea (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		engine.Error( "CM_LeafArea: bad number");
	return map_leafs[leafnum].area;
}

//=======================================================================


cplane_t	*box_planes;
int			box_headnode;
cbrush_t	*box_brush;
cleaf_t		*box_leaf;

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (void)
{
	int			i;
	int			side;
	cnode_t		*c;
	cplane_t	*p;
	cbrushside_t	*s;

	box_headnode = numnodes;
	box_planes = &map_planes[numplanes];
	if (numnodes+6 > MAX_MAP_NODES
		|| numbrushes+1 > MAX_MAP_BRUSHES
		|| numleafbrushes+1 > MAX_MAP_LEAFBRUSHES
		|| numbrushsides+6 > MAX_MAP_BRUSHSIDES
		|| numplanes+12 > MAX_MAP_PLANES)
		engine.Error( "Not enough room for box tree");

	box_brush = &map_brushes[numbrushes];
	box_brush->numsides = 6;
	box_brush->firstbrushside = numbrushsides;
	box_brush->contents = CONTENTS_MONSTER;

	box_leaf = &map_leafs[numleafs];
	box_leaf->contents = CONTENTS_MONSTER;
	box_leaf->firstleafbrush = numleafbrushes;
	box_leaf->numleafbrushes = 1;

	map_leafbrushes[numleafbrushes] = numbrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &map_brushsides[numbrushsides+i];
		s->plane = 	map_planes + (numplanes+i*2+side);
		s->surface = &nullsurface;

		// nodes
		c = &map_nodes[box_headnode+i];
		c->plane = map_planes + (numplanes+i*2);
		c->children[side] = -1 - emptyleaf;
		if (i != 5)
			c->children[side^1] = box_headnode+i + 1;
		else
			c->children[side^1] = -1 - numleafs;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;
	}	
}


/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
int	CM_HeadnodeForBox (Vector& mins, Vector& maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_headnode;
}


/*
==================
CM_PointLeafnum_r

==================
*/
int CM_PointLeafnum_r (Vector& p, int num)
{
	float		d;
	cnode_t		*node;
	cplane_t	*plane;

	while (num >= 0)
	{
		node = map_nodes + num;
		plane = node->plane;
		
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	c_pointcontents++;		// optimize counter

	return -1 - num;
}

int CM_PointLeafnum (Vector& p)
{
	if (!numplanes)
		return 0;		// sound may call this without map loaded
	return CM_PointLeafnum_r (p, 0);
}



/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
int		leaf_count, leaf_maxcount;
int		*leaf_list;
Vector	*leaf_mins, *leaf_maxs;
int		leaf_topnode;

void CM_BoxLeafnums_r (int nodenum)
{
	cplane_t	*plane;
	cnode_t		*node;
	int		s;

	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
			{
//				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
				return;
			}
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}
	
		node = &map_nodes[nodenum];
		plane = node->plane;
//		s = BoxOnPlaneSide (*leaf_mins, *leaf_maxs, plane);
		s = BOX_ON_PLANE_SIDE(*leaf_mins, *leaf_maxs, plane);
		if (s == 1)
			nodenum = node->children[0];
		else if (s == 2)
			nodenum = node->children[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			CM_BoxLeafnums_r (node->children[0]);
			nodenum = node->children[1];
		}

	}
}

int	CM_BoxLeafnums_headnode (Vector& mins, Vector& maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = &mins;
	leaf_maxs = &maxs;

	leaf_topnode = -1;

	CM_BoxLeafnums_r (headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

int	CM_BoxLeafnums (Vector& mins, Vector& maxs, int *list, int listsize, int *topnode)
{
	return CM_BoxLeafnums_headnode (mins, maxs, list,
		listsize, map_cmodels[0].headnode, topnode);
}



/*
==================
CM_PointContents

==================
*/
int CM_PointContents (Vector& p, int headnode)
{
	int		l;

	if (!numnodes)	// map not loaded
		return 0;

	l = CM_PointLeafnum_r (p, headnode);

	return map_leafs[l].contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int	CM_TransformedPointContents (Vector& p, int headnode, Vector& origin, Vector& angles)
{
	Vector		p_l;
	Vector		temp;
	Vector		forward, right, up;
	int			l;

	// subtract origin offset
	VectorSubtract (p, origin, p_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode && 
	(angles[0] || angles[1] || angles[2]) )
	{
		AngleVectors (angles, &forward, &right, &up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	l = CM_PointLeafnum_r (p_l, headnode);

	return map_leafs[l].contents;
}


/*
===============================================================================

BOX TRACING

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

Vector	trace_start, trace_end;
Vector	trace_mins, trace_maxs;
Vector	trace_extents;

trace_t	trace_trace;
int		trace_contents;
qboolean	trace_ispoint;		// optimized case

// JAYHL2: This used to be -1, but that caused lots of epsilon issues
// around slow sloping planes.  Perhaps Quake2 limited maps to a certain
// slope / angle on walkable ground.  It has to be a negative number
// so that the tests work out.
#define		NEVER_UPDATED		-9999

/*
================
CM_ClipBoxToBrush
================
*/
void CM_ClipBoxToBrush (Vector& mins, Vector& maxs, Vector& p1, Vector& p2,
					  trace_t *trace, cbrush_t *brush, bool bDispSurf)
{
	int			i, j;
	cplane_t	*plane, *clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	Vector		ofs;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
	cbrushside_t	*side, *leadside;

	enterfrac = NEVER_UPDATED;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numsides)
		return;

	c_brush_traces++;

	getout = false;
	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
		}
		else
		{	// special point case
			dist = plane->dist;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if (d2 > 0)
			getout = true;	// endpoint is not in solid
		if (d1 > 0)
			startout = true;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-DIST_EPSILON) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1+DIST_EPSILON) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return;
	}
	if (enterfrac < leavefrac)
	{
		if (enterfrac > NEVER_UPDATED && enterfrac < trace->fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = leadside->surface;
			trace->contents = brush->contents;
			trace->brush = brush;
			trace->sideNumber = (leadside - map_brushsides) - brush->firstbrushside;
		}
	}
}

/*
================
CM_TestBoxInBrush
================
*/
void CM_TestBoxInBrush (Vector& mins, Vector& maxs, Vector& p1,
					  trace_t *trace, cbrush_t *brush)
{
	int			i, j;
	cplane_t	*plane;
	float		dist;
	Vector		ofs;
	float		d1;
	cbrushside_t	*side;

	if (!brush->numsides)
		return;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}
		dist = DotProduct (ofs, plane->normal);
		dist = plane->dist - dist;

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;

	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}


/*
================
CM_TraceToLeaf
================
*/
void CM_TraceToLeaf (int leafnum)
{
	int			k;
	int			brushnum;
	cleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &map_leafs[leafnum];

	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = map_leafbrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b, FALSE );
		if (!trace_trace.fraction)
			return;
	}
#if 0
	//
	// trace box against all the displacement surfaces in the leaves
	//
	for( int i = 0; i < leaf->dispInfoCount; i++ )
	{
		cdispinfo_t *pCDispInfo = leaf->ppDispInfoList[i];

		// ensure that we test each surface only once per trace
		if( pCDispInfo->checkcount == checkcount )
			continue;
		
		CM_SweepBoxAgainstDispSurf( pCDispInfo, trace_start, trace_end, trace_mins, trace_maxs, &trace_trace );
		
		// mark tested for this trace
		pCDispInfo->checkcount = checkcount;
	}
#endif
}


/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf (int leafnum)
{
	int			k;
	int			brushnum;
	cleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = map_leafbrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_TestBoxInBrush (trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

}


/*
==================
CM_RecursiveHullCheck

==================
*/
void CM_RecursiveHullCheck (int num, float p1f, float p2f, const Vector& p1, const Vector& p2)
{
	cnode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int			i;
	Vector		mid;
	int			side;
	float		midf;

	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToLeaf (-1-num);
		return;
	}

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	node = map_nodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (trace_ispoint)
			offset = 0;
		else
			offset = fabs(trace_extents[0]*plane->normal[0]) +
				fabs(trace_extents[1]*plane->normal[1]) +
				fabs(trace_extents[2]*plane->normal[2]);
	}


#if 0
CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
return;
#endif

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;
		
	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);


	// go past the node
	if (frac2 < 0)
		frac2 = 0;
	if (frac2 > 1)
		frac2 = 1;
		
	midf = p1f + (p2f - p1f)*frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
}



//======================================================================

/*
==================
CM_BoxTrace
==================
*/
trace_t		CM_BoxTrace (const Vector& start, const Vector& end,
						  const Vector& mins, const Vector& maxs,
						  int headnode, int brushmask)
{
	int		i;

	checkcount++;		// for multi-check avoidance

	c_traces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &nullsurface;

	if (!numnodes)	// map not loaded
		return trace_trace;

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);

	//
	// check for position test special case
	//
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
	{
		int		leafs[1024];
		int		i, numleafs;
		Vector	c1, c2;
		int		topnode;

		VectorAdd (start, mins, c1);
		VectorAdd (start, maxs, c2);
		for (i=0 ; i<3 ; i++)
		{
			c1[i] -= 1;
			c2[i] += 1;
		}

		numleafs = CM_BoxLeafnums_headnode (c1, c2, leafs, 1024, headnode, &topnode);
		for (i=0 ; i<numleafs ; i++)
		{
			CM_TestInLeaf (leafs[i]);
			if (trace_trace.allsolid)
				break;
		}
		VectorCopy (start, trace_trace.endpos);
		return trace_trace;
	}
	//
	// check for point special case
	//
	if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0
		&& maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0)
	{
		trace_ispoint = true;
		VectorClear (trace_extents);
	}
	else
	{
		trace_ispoint = false;
		trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	//
	// general sweeping through world
	//
	CM_RecursiveHullCheck (headnode, 0, 1, start, end);

	if (trace_trace.fraction == 1)
	{
		VectorCopy (end, trace_trace.endpos);
	}
	else
	{
		for (i=0 ; i<3 ; i++)
			trace_trace.endpos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
	}
	return trace_trace;
}


#ifdef _WIN32
#pragma optimize( "", off )
#endif
/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/

trace_t		CM_TransformedBoxTrace (Vector& start, Vector& end,
						  Vector& mins, Vector& maxs,
						  int headnode, int brushmask,
						  Vector& origin, Vector& angles)
{
	trace_t		trace;
	Vector		start_l, end_l;
	Vector		forward, right, up;
	Vector		temp;
	qboolean	rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode && 
	(angles[0] || angles[1] || angles[2]) )
		rotated = true;
	else
		rotated = false;

	if (rotated)
	{
		AngleVectors (angles, &forward, &right, &up);

		// transform the start / end points into the local space of this entity
		VectorCopy (start_l, temp);
		start_l[0] = DotProduct (temp, forward);
		start_l[1] = -DotProduct (temp, right);
		start_l[2] = DotProduct (temp, up);

		VectorCopy (end_l, temp);
		end_l[0] = DotProduct (temp, forward);
		end_l[1] = -DotProduct (temp, right);
		end_l[2] = DotProduct (temp, up);
	}

	// sweep the box through the model
	trace = CM_BoxTrace (start_l, end_l, mins, maxs, headnode, brushmask);

	if (rotated && trace.fraction != 1.0)
	{
		// transform the normal from the local space of this entity to world space
		AngleVectorsTranspose( angles, &forward, &right, &up );
		VectorCopy (trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct (temp, forward);
		trace.plane.normal[1] = DotProduct (temp, right);
		trace.plane.normal[2] = DotProduct (temp, up);
	}

	// No need to transform the impact/end point out.  The parametric coordinate (fraction) is
	// invariant under transformation
	trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
	trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
	trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

	return trace;
}

#ifdef _WIN32
#pragma optimize( "", on )
#endif

/*
===============================================================================

PVS / PAS

===============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/
void CM_DecompressVis (byte *in, byte *out)
{
	int		c;
	byte	*out_p;
	int		row;

	row = (numclusters+7)>>3;	
	out_p = out;

	if (!in || !numvisibility)
	{	// no vis info, so make all visible
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}
		return;		
	}

	do
	{
		if (*in)
		{
			*out_p++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((out_p - out) + c > row)
		{
			c = row - (out_p - out);
			engine.Con_Printf( PRINT_ALL, "warning: Vis decompression overrun\n" );
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}

byte	pvsrow[MAX_MAP_LEAFS/8];
byte	pasrow[MAX_MAP_LEAFS/8];

byte	*CM_ClusterPVS (int cluster)
{
	if (cluster == -1)
		memset (pvsrow, 0, (numclusters+7)>>3);
	else
		CM_DecompressVis (map_visibility + map_vis->bitofs[cluster][DVIS_PVS], pvsrow);
	return pvsrow;
}

byte	*CM_ClusterPAS (int cluster)
{
	if (cluster == -1)
		memset (pasrow, 0, (numclusters+7)>>3);
	else
		CM_DecompressVis (map_visibility + map_vis->bitofs[cluster][DVIS_PAS], pasrow);
	return pasrow;
}


/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void FloodArea_r (carea_t *area, int floodnum)
{
	int		i;
	dareaportal_t	*p;

	if (area->floodvalid == floodvalid)
	{
		if (area->floodnum == floodnum)
			return;
		engine.Error( "FloodArea_r: reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = floodvalid;
	p = &map_areaportals[area->firstareaportal];
	for (i=0 ; i<area->numareaportals ; i++, p++)
	{
		if (portalopen[p->portalnum])
			FloodArea_r (&map_areas[p->otherarea], floodnum);
	}
}

/*
====================
FloodAreaConnections


====================
*/
void	FloodAreaConnections (void)
{
	int		i;
	carea_t	*area;
	int		floodnum;

	// all current floods are now invalid
	floodvalid++;
	floodnum = 0;

	// area 0 is not used
	for (i=1 ; i<numareas ; i++)
	{
		area = &map_areas[i];
		if (area->floodvalid == floodvalid)
			continue;		// already flooded into
		floodnum++;
		FloodArea_r (area, floodnum);
	}

}

void	CM_SetAreaPortalState (int portalnum, qboolean open)
{
	if (portalnum > numareaportals)
		engine.Error( "areaportal > numareaportals");

	portalopen[portalnum] = open;
	FloodAreaConnections ();
}

qboolean	CM_AreasConnected (int area1, int area2)
{
	// JAY: Later
	if (map_noareas.value)
		return true;

	if (area1 > numareas || area2 > numareas)
		engine.Error( "area > numareas");

	if (map_areas[area1].floodnum == map_areas[area2].floodnum)
		return true;
	return false;
}


/*
=================
CM_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_WriteAreaBits (byte *buffer, int area)
{
	int		i;
	int		floodnum;
	int		bytes;

	bytes = (numareas+7)>>3;

	// JAY: Later
	if ( map_noareas.value )
	{	// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else
	{
		memset (buffer, 0, bytes);

		floodnum = map_areas[area].floodnum;
		for (i=0 ; i<numareas ; i++)
		{
			if (map_areas[i].floodnum == floodnum || !area)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}


/*
===================
CM_WritePortalState

Writes the portal state to a savegame file
===================
*/
void	CM_WritePortalState (FILE *f)
{
	fwrite (portalopen, sizeof(portalopen), 1, f);
}

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
void	CM_ReadPortalState (FILE *f)
{
	fread(portalopen, sizeof(portalopen), 1, f);
	FloodAreaConnections ();
}

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
qboolean CM_HeadnodeVisible (int nodenum, byte *visbits)
{
	int		leafnum;
	int		cluster;
	cnode_t	*node;

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = map_leafs[leafnum].cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &map_nodes[nodenum];
	if (CM_HeadnodeVisible(node->children[0], visbits))
		return true;
	return CM_HeadnodeVisible(node->children[1], visbits);
}

