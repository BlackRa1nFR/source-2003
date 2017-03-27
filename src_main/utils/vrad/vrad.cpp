/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/

// vrad.c

#include "vrad.h"
#include "physdll.h"
#include "lightmap.h"
#include "vstdlib/strtools.h"
#include "vmpi.h"
#include "macro_texture.h"
#include "vmpi_tools_shared.h"


#define ALLOWOPTIONS (0 || _DEBUG)


/*

NOTES
-----

every surface must be divided into at least two patches each axis

*/

//patch_t		patches[MAX_PATCHES];
//unsigned		num_patches;
//patch_t		*face_patches[MAX_MAP_FACES];
//patch_t		*face_parents[MAX_MAP_FACES];
//patch_t		*cluster_children[MAX_MAP_CLUSTERS];
//Vector		emitlight[MAX_PATCHES];
//Vector		addlight[MAX_PATCHES];

CUtlVector<patch_t>		patches;			
CUtlVector<int>			facePatches;		// constains all patches, children first
CUtlVector<int>			faceParents;		// contains only root patches, use next parent to iterate
CUtlVector<int>			clusterChildren;
CUtlVector<Vector>		emitlight;
CUtlVector<Vector>		addlight;

int g_nDXLevel = 90;

int num_sky_cameras;
sky_camera_t sky_cameras[MAX_MAP_AREAS];
int area_sky_cameras[MAX_MAP_AREAS];

entity_t	*face_entity[MAX_MAP_FACES];
Vector		face_offset[MAX_MAP_FACES];		// for rotating bmodels
int			fakeplanes;

unsigned	numbounce = 100; // 25; /* Originally this was 8 */

float		maxchop = 64;
float		minchop = 64;
qboolean	dumppatches;
bool	    bDumpNormals = false;
bool		bRed2Black = false;

int			TestLine (Vector& start, Vector& stop);

int			junk;

Vector		ambient( 0, 0, 0 );
float		maxlight = (256 * 4 - 1);	// 4x overbright

float		lightscale = 1.0;
float		dlight_threshold = 0.1;  // was DIRECT_LIGHT constant

char		source[MAX_PATH] = "";
char		platformPath[MAX_PATH] = "";

char		global_lights[MAX_PATH] = "";
char		designer_lights[MAX_PATH] = "";
char		level_lights[MAX_PATH] = "";

char		vismatfile[_MAX_PATH] = "";
char		incrementfile[_MAX_PATH] = "";

IIncremental *g_pIncremental = 0;
bool		g_bInterrupt = false;	// Wsed with background lighting in WC. Tells VRAD
									// to stop lighting.

float		gamma = 0.5;
float		indirect_sun = 1.0;
float		reflectivityScale = 1.0;
qboolean	do_extra = true;
bool		debug_extra = false;
qboolean	do_fast = false;
qboolean	do_centersamples = false;
qboolean	do_dispblend = true;
int			extrapasses = 4;
float		smoothing_threshold = 0.7071067; // cos(45.0*(M_PI/180)) 
// Cosine of smoothing angle(in radians)
float		coring = 1.0;	// Light threshold to force to blackness(minimizes lightmaps)
qboolean	texscale = true;
int			dlight_map = 0; // Setting to 1 forces direct lighting into different lightmap than radiosity

float		luxeldensity = 1.0;
unsigned	num_degenerate_faces;

qboolean	g_bLowPriority = false;
qboolean	g_bLogHashData = false;

double		g_flStartTime;

CUtlVector<byte> g_FacesVisibleToLights;


/*
===================================================================

MISC

===================================================================
*/


int		leafparents[MAX_MAP_LEAFS];
int		nodeparents[MAX_MAP_NODES];

void MakeParents (int nodenum, int parent)
{
	int		i, j;
	dnode_t	*node;

	nodeparents[nodenum] = parent;
	node = &dnodes[nodenum];

	for (i=0 ; i<2 ; i++)
	{
		j = node->children[i];
		if (j < 0)
			leafparents[-j - 1] = nodenum;
		else
			MakeParents (j, nodenum);
	}
}


/*
===================================================================

  TEXTURE LIGHT VALUES

===================================================================
*/

typedef struct
{
	char	name[256];
	Vector	value;
	char	*filename;
} texlight_t;

#define	MAX_TEXLIGHTS	128

texlight_t	texlights[MAX_TEXLIGHTS];
int			num_texlights;

/*
============
ReadLightFile
============
*/
void ReadLightFile (char *filename)
{
	char	scan[128];
	short	argCnt;
	int		j, file_texlights = 0;

	FileHandle_t f = g_pFileSystem->Open( filename, "r" );
	if (!f)
		Error ("ERROR: Couldn't open texlight file %s", filename);
	else
		Msg("[Reading texlights from '%s']\n", filename);

	while ( CmdLib_FGets(scan, sizeof(scan), f) )
	{
		char	szTexlight[256];
		Vector	value;
		if (num_texlights == MAX_TEXLIGHTS)
			Error ("MAX_TEXLIGHTS");

		argCnt = sscanf (scan, "%s ",szTexlight );
		
		if( argCnt != 1 )
		{
			if (strlen( scan ) > 4)
				Msg("ignoring bad texlight '%s' in %s", scan, filename );
			continue;
		}

		LightForString( scan + strlen( szTexlight ) + 1, value );

		for( j=0; j<num_texlights; j++ )
		{
			if ( strcmp( texlights[j].name, szTexlight ) == 0 )
			{
				if ( strcmp(texlights[j].filename, filename ) == 0 )
				{
					Msg( "ERROR\a: Duplication of '%s' in file '%s'!\n",
							texlights[j].name, texlights[j].filename );
				} 
				else if ( texlights[j].value[0] != value[0]
				  || texlights[j].value[1] != value[1]
				  || texlights[j].value[2] != value[2] )
				{
					Warning("Warning: Overriding '%s' from '%s' with '%s'!\n", 
							texlights[j].name, texlights[j].filename, filename );
				}
				else
				{
					Warning("Warning: Redundant '%s' def in '%s' AND '%s'!\n", 
							texlights[j].name, texlights[j].filename, filename );
				}
				break;
			}
		}
		strcpy( texlights[j].name, szTexlight );
		VectorCopy( value, texlights[j].value );
		texlights[j].filename = filename;
		file_texlights++;

		num_texlights = max( num_texlights, j+1 );
	}		
	
	qprintf ("[%i texlights parsed from '%s']\n\n", file_texlights, filename);
	g_pFileSystem->Close( f );
}


/*
============
LightForTexture
============
*/
void LightForTexture( const char *name, Vector& result )
{
	int		i;

	result[ 0 ] = result[ 1 ] = result[ 2 ] = 0;

	for (i=0 ; i<num_texlights ; i++)
	{
		if (!Q_strcasecmp (name, texlights[i].name))
		{
			VectorCopy( texlights[i].value, result );
			return;
		}
	}
}

/*
=======================================================================

MAKE FACES

=======================================================================
*/

/*
=============
WindingFromFace
=============
*/
winding_t	*WindingFromFace (dface_t *f, Vector& origin )
{
	int			i;
	int			se;
	dvertex_t	*dv;
	int			v;
	winding_t	*w;

	w = AllocWinding (f->numedges);
	w->numpoints = f->numedges;

	for (i=0 ; i<f->numedges ; i++)
	{
		se = dsurfedges[f->firstedge + i];
		if (se < 0)
			v = dedges[-se].v[1];
		else
			v = dedges[se].v[0];

		dv = &dvertexes[v];
		VectorAdd (dv->point, origin, w->p[i]);
	}

	RemoveColinearPoints (w);

	return w;
}

/*
=============
BaseLightForFace
=============
*/
void BaseLightForFace( dface_t *f, Vector& light, float *parea, Vector& reflectivity )
{
	texinfo_t	*tx;
	dtexdata_t	*texdata;

	//
	// check for light emited by texture
	//
	tx = &texinfo[f->texinfo];
	texdata = &dtexdata[tx->texdata];

	LightForTexture (TexDataStringTable_GetString( texdata->nameStringTableID ), light);


	*parea = texdata->height * texdata->width;

	VectorScale( texdata->reflectivity, reflectivityScale, reflectivity );
	
	// always keep this less than 1 or the solution will not converge
	for ( int i = 0; i < 3; i++ )
	{
		if ( reflectivity[i] > 0.99 )
			reflectivity[i] = 0.99;
	}
}

qboolean IsSky (dface_t *f)
{
	texinfo_t	*tx;

	tx = &texinfo[f->texinfo];
	if (tx->flags & SURF_SKY)
		return true;
	return false;
}

#ifdef STATIC_FOG
/*=============
IsFog
=============*/
qboolean IsFog( dface_t *f )
{
	texinfo_t	*tx;

	tx = &texinfo[f->texinfo];

    // % denotes a fog texture
    if( tx->texture[0] == '%' )
		return true;

	return false;
}
#endif


void ProcessSkyCameras()
{
	int i;
	num_sky_cameras = 0;
	for (i = 0; i < numareas; ++i)
	{
		area_sky_cameras[i] = -1;
	}

	for (i = 0; i < num_entities; ++i)
	{
		entity_t *e = &entities[i];
		const char *name = ValueForKey (e, "classname");
		if (stricmp (name, "sky_camera"))
			continue;

		Vector origin;
		GetVectorForKey( e, "origin", origin );
		int node = PointLeafnum( origin );
		int area = -1;
		if (node >= 0 && node < numleafs) area = dleafs[node].area;
		float scale = FloatForKey( e, "scale" );

		if (scale > 0.0f)
		{
			sky_cameras[num_sky_cameras].origin = origin;
			sky_cameras[num_sky_cameras].sky_to_world = scale;
			sky_cameras[num_sky_cameras].world_to_sky = 1.0f / scale;
			sky_cameras[num_sky_cameras].area = area;

			if (area >= 0 && area < numareas)
			{
				area_sky_cameras[area] = num_sky_cameras;
			}

			++num_sky_cameras;
		}
	}

}


/*
=============
MakePatchForFace
=============
*/
float	totalarea;
void MakePatchForFace (int fn, winding_t *w)
{
	dface_t     *f = dfaces + fn;
	float	    area;
	patch_t		*patch;
	Vector		centroid(0,0,0);
	int			i, j;
	texinfo_t	*tx;

    // get texture info
    tx = &texinfo[f->texinfo];

	// No patches at all for fog!
#ifdef STATIC_FOG
	if ( IsFog( f ) )
		return;
#endif

	// the sky needs patches or the form factors don't work out correctly
	// if (IsSky( f ) )
	// 	return;

	area = WindingArea (w);
	if (area <= 0)
	{
		num_degenerate_faces++;
		// Msg("degenerate face\n");
		return;
	}

	totalarea += area;

	// get a patch
	int ndxPatch = patches.AddToTail();
	patch = &patches[ndxPatch];
	memset( patch, 0, sizeof( patch_t ) );
	patch->ndxNext = patches.InvalidIndex();
	patch->ndxNextParent = patches.InvalidIndex();
	patch->ndxNextClusterChild = patches.InvalidIndex();
	patch->child1 = patches.InvalidIndex();
	patch->child2 = patches.InvalidIndex();
	patch->parent = patches.InvalidIndex();

	// link and save patch data
	patch->ndxNext = facePatches.Element( fn );
	facePatches[fn] = ndxPatch;
//	patch->next = face_patches[fn];
//	face_patches[fn] = patch;

	// compute a separate scale for chop - since the patch "scale" is the texture scale
	// we want textures with higher resolution lighting to be chopped up more
	float chopscale[2];
	chopscale[0] = chopscale[1] = 1.0f;
    if ( texscale )
    {
        // Compute the texture "scale" in s,t
        for( i=0; i<2; i++ )
        {
            patch->scale[i] = 0.0f;
			chopscale[i] = 0.0f;
            for( j=0; j<3; j++ )
			{
                patch->scale[i] += 
					tx->textureVecsTexelsPerWorldUnits[i][j] * 
					tx->textureVecsTexelsPerWorldUnits[i][j];
                chopscale[i] += 
					tx->lightmapVecsLuxelsPerWorldUnits[i][j] * 
					tx->lightmapVecsLuxelsPerWorldUnits[i][j];
			}
            patch->scale[i] = sqrt( patch->scale[i] );
			chopscale[i] = sqrt( chopscale[i] ) * 16.0f;
        }
    }
    else
		patch->scale[0] = patch->scale[1] = 1.0f;

	patch->area = area;
 
	patch->sky = IsSky( f );

	// chop scaled up lightmaps coarser
	patch->chop = maxchop / ((chopscale[0]+chopscale[1])/2);
	if (patch->chop < maxchop)
		patch->chop = maxchop;

#ifdef STATIC_FOG
    patch->fog = FALSE;
#endif

	patch->winding = w;

	patch->plane = &dplanes[f->planenum];

	// make a new plane to adjust for origined bmodels
	if (face_offset[fn][0] || face_offset[fn][1] || face_offset[fn][2] )
	{	
		dplane_t	*pl;

		// origin offset faces must create new planes
		if (numplanes + fakeplanes >= MAX_MAP_PLANES)
			Error ("numplanes + fakeplanes >= MAX_MAP_PLANES");
		pl = &dplanes[numplanes + fakeplanes];
		fakeplanes++;

		*pl = *(patch->plane);
		pl->dist += DotProduct (face_offset[fn], pl->normal);
		patch->plane = pl;
	}

	patch->faceNumber = fn;
	WindingCenter (w, patch->origin);

	// Save "center" for generating the face normals later.
	VectorSubtract( patch->origin, face_offset[fn], face_centroids[fn] ); 

	VectorCopy( patch->plane->normal, patch->normal );
	VectorAdd (patch->origin, patch->normal, patch->origin);

	WindingBounds (w, patch->face_mins, patch->face_maxs);
	VectorCopy( patch->face_mins, patch->mins );
	VectorCopy( patch->face_maxs, patch->maxs );

	BaseLightForFace( f, patch->baselight, &patch->basearea, patch->reflectivity );

	// Chop all texlights very fine.
	if ( !VectorCompare( patch->baselight, vec3_origin ) )
		patch->chop = do_extra ? minchop / 2 : minchop;

	// get rid of do extra functionality on displacement surfaces
	if( ValidDispFace( f ) )
	{
		patch->chop = minchop;
	}
}


entity_t *EntityForModel (int modnum)
{
	int		i;
	char	*s;
	char	name[16];

	sprintf (name, "*%i", modnum);
	// search the entities for one using modnum
	for (i=0 ; i<num_entities ; i++)
	{
		s = ValueForKey (&entities[i], "model");
		if (!strcmp (s, name))
			return &entities[i];
	}

	return &entities[0];
}

/*
=============
MakePatches
=============
*/
void MakePatches (void)
{
	int		    i, j;
	dface_t	    *f;
	int		    fn;
	winding_t	*w;
	dmodel_t	*mod;
	Vector		origin;
	entity_t	*ent;

	ParseEntities ();
	qprintf ("%i faces\n", numfaces);

	for (i=0 ; i<nummodels ; i++)
	{
		mod = dmodels+i;
		ent = EntityForModel (i);
		VectorCopy (vec3_origin, origin);

		// bmodels with origin brushes need to be offset into their
		// in-use position
		GetVectorForKey (ent, "origin", origin);

		for (j=0 ; j<mod->numfaces ; j++)
		{
			fn = mod->firstface + j;
			face_entity[fn] = ent;
			VectorCopy (origin, face_offset[fn]);
			f = &dfaces[fn];
			if( f->dispinfo == -1 )
			{
	            w = WindingFromFace (f, origin );
		        MakePatchForFace( fn, w );
			}
		}
	}

	if (num_degenerate_faces > 0)
	{
		qprintf("%d degenerate faces\n", num_degenerate_faces );
	}

	qprintf ("%i square feet [%.2f square inches]\n", (int)(totalarea/144), totalarea );

	// make the displacement surface patches
	StaticDispMgr()->MakePatches();
}

/*
=======================================================================

SUBDIVIDE

=======================================================================
*/


//-----------------------------------------------------------------------------
// Purpose: does this surface take/emit light
//-----------------------------------------------------------------------------
bool Nolight( patch_t *patch )
{
	dface_t *f = dfaces + patch->faceNumber;
	texinfo_t *tx = &texinfo[f->texinfo];
	if (tx->flags & SURF_NOLIGHT)
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: subdivide the "parent" patch
//-----------------------------------------------------------------------------
void SubdividePatch( int ndxPatch )
{
	winding_t *w, *o1, *o2;
	Vector	total;
	Vector	split;
	vec_t	dist;
	vec_t	widest = -1;
	int		i, widest_axis = -1;
	int		subdivide_it = 0;
	patch_t	*child1;
	patch_t	*child2;

	// get the current patch
	patch_t *patch = &patches.Element( ndxPatch );
	if( !patch )
		return;

	// never subdivide sky patches
	if( patch->sky )
		return;

	// get the patch winding
	w = patch->winding;

	//
	// subdivide along the widest axis
	//
	VectorSubtract (patch->maxs, patch->mins, total);
	for (i=0 ; i<3 ; i++)
	{
		if ( total[i] > widest )
		{
			widest_axis = i;
			widest = total[i];
		}
		if ( total[i] > patch->chop
			 /* || (patch->face_maxs[i] == patch->maxs[i] || patch->face_mins[i] == patch->mins[i] ) */
  			 && total[i] > minchop )
		{
			subdivide_it = 1;
		}
	}

	if ((! subdivide_it) && widest_axis != -1)
	{
		// make more square
		if (total[widest_axis] > total[(widest_axis + 1) % 3] * 3 && total[widest_axis] > total[(widest_axis + 2) % 3] * 3)
		{
			if (patch->chop > minchop)
			{
				subdivide_it = 1;
				patch->chop = patch->chop / 2;
			}
		}
	}

	if ( subdivide_it )
	{
		//
		// split the winding
		//
		VectorCopy (vec3_origin, split);
		split[widest_axis] = 1;
		dist = (patch->mins[widest_axis] + patch->maxs[widest_axis])*0.5f;
		ClipWindingEpsilon (w, split, dist, ON_EPSILON, &o1, &o2);

		// calculate the area of the patches to see if they are "significant"
		Vector center1;
		Vector center2;
		float area1 = WindingAreaAndBalancePoint( o1, center1 );
		float area2 = WindingAreaAndBalancePoint( o2, center2 );

		if( area1 == 0 || area2 == 0 )
		{
			Msg( "zero area child patch\n" );
//			Msg( "degenerate child patch\n" );
			return;
		}

		//
		// create a new patches
		//
		int ndxChild1Patch = patches.AddToTail();
		int ndxChild2Patch = patches.AddToTail();

		child1 = &patches[ndxChild1Patch];
		memset( child1, 0, sizeof( patch_t ) );
		child1->ndxNext = patches.InvalidIndex();
		child1->ndxNextParent = patches.InvalidIndex();
		child1->ndxNextClusterChild = patches.InvalidIndex();
		child1->child1 = patches.InvalidIndex();
		child1->child2 = patches.InvalidIndex();
		child1->parent = patches.InvalidIndex();

		patch = &patches.Element( ndxPatch );

		child2 = &patches[ndxChild2Patch];
		memset( child2, 0, sizeof( patch_t ) );
		child2->ndxNext = patches.InvalidIndex();
		child2->ndxNextParent = patches.InvalidIndex();
		child2->ndxNextClusterChild = patches.InvalidIndex();
		child2->child1 = patches.InvalidIndex();
		child2->child2 = patches.InvalidIndex();
		child2->parent = patches.InvalidIndex();

		patch = &patches.Element( ndxPatch );

		// copy all elements of parent patch to children
		*child1 = *patch;
		*child2 = *patch;

		patch->child1 = ndxChild1Patch;
		patch->child2 = ndxChild2Patch;		

//		child1->child1 = child1->child2 = 0;
//		child2->child1 = child2->child2 = 0;

		child1->parent = patch - patches.Base();
		child2->parent = patch - patches.Base();

		child1->winding = o1;
		child2->winding = o2;

		child1->area = area1;
		child2->area = area2;

		VectorCopy( patch->face_mins, child1->face_mins );
		VectorCopy( patch->face_maxs, child1->face_maxs );
		VectorCopy( patch->face_mins, child2->face_mins );
		VectorCopy( patch->face_maxs, child2->face_maxs );

		VectorCopy( center1, child1->origin );
		VectorCopy( center2, child2->origin );

		// VectorCopy( child1->plane->normal, child1->normal );
		// VectorCopy( child2->plane->normal, child2->normal );

		if ( ValidDispFace( dfaces + child1->faceNumber ) )
		{
			// shouldn't get here anymore!!
			Msg( "SubdividePatch: Error - Should not be here!\n" );
			StaticDispMgr()->GetDispSurfNormal( child1->faceNumber, child1->origin, child1->normal, true );
			VectorAdd( child1->origin, child1->normal, child1->origin );
		}
		else
		{
			GetPhongNormal( child1->faceNumber, child1->origin, child1->normal );
			VectorAdd( child1->origin, child1->plane->normal, child1->origin );
		}

		if ( ValidDispFace( dfaces + child2->faceNumber ) )
		{
			// shouldn't get here anymore!!
			Msg( "SubdividePatch: Error - Should not be here!\n" );
			StaticDispMgr()->GetDispSurfNormal( child2->faceNumber, child2->origin, child2->normal, true );
			VectorAdd( child2->origin, child2->normal, child2->origin );
		}
		else
		{
			GetPhongNormal( child2->faceNumber, child2->origin, child2->normal );
			VectorAdd( child2->origin, child2->plane->normal, child2->origin );
		}

		child1->planeDist = child1->plane->dist;
		child2->planeDist = child2->plane->dist;

		WindingBounds(child1->winding, child1->mins, child1->maxs);
		WindingBounds(child2->winding, child2->mins, child2->maxs);

		// Subdivide patch even more if on the edge of the face; this is a hack!
		child1 = &patches.Element( ndxChild1Patch );
		VectorSubtract (child1->maxs, child1->mins, total);
		if ( total[0] < child1->chop && total[1] < child1->chop && total[2] < child1->chop )
			for ( i=0; i<3; i++ )
				if ( (child1->face_maxs[i] == child1->maxs[i] || child1->face_mins[i] == child1->mins[i] )
				  && total[i] > minchop )
				{
					child1->chop = max( minchop, child1->chop / 2 );
					break;
				}

		SubdividePatch (ndxChild1Patch/*child1*/);

		// Subdivide patch even more if on the edge of the face; this is a hack!
		child2 = &patches.Element( ndxChild2Patch );
		VectorSubtract (child2->maxs, child2->mins, total);
		if ( total[0] < child2->chop && total[1] < child2->chop && total[2] < child2->chop )
			for ( i=0; i<3; i++ )
				if ( (child2->face_maxs[i] == child2->maxs[i] || child2->face_mins[i] == child2->mins[i] )
				  && total[i] > minchop )
				{
					child2->chop = max( minchop, child2->chop / 2 );
					break;
				}

		SubdividePatch (ndxChild2Patch/*child2*/);
	}
}


/*
=============
SubdividePatches
=============
*/
void SubdividePatches (void)
{
	unsigned		i, num;

	if (numbounce == 0)
		return;

	unsigned int uiPatchCount = patches.Size();
	qprintf ("%i patches before subdivision\n", uiPatchCount);

	for (i = 0; i < uiPatchCount; i++)
	{
		patch_t *pCur = &patches.Element( i );
		pCur->planeDist = pCur->plane->dist;

		pCur->ndxNextParent = faceParents.Element( pCur->faceNumber );
		faceParents[pCur->faceNumber] = pCur - patches.Base();
	}

	for (i=0 ; i< uiPatchCount; i++)
	{
		patch_t *patch = &patches.Element( i );
		patch->parent = -1;
		if ( Nolight(patch) )
			continue;

		if (!do_fast)
		{
			if( dfaces[patch->faceNumber].dispinfo == -1 )
			{
				SubdividePatch( i );
			}
			else
			{
				StaticDispMgr()->SubdividePatch( i );
			}
		}
	}

	// fixup next pointers
	for (i = 0; i < (unsigned)numfaces; i++)
	{
		facePatches[i] = facePatches.InvalidIndex();
	}

	uiPatchCount = patches.Size();
	for (i = 0; i < uiPatchCount; i++)
	{
		patch_t *pCur = &patches.Element( i );
		pCur->ndxNext = facePatches.Element( pCur->faceNumber );
		facePatches[pCur->faceNumber] = pCur - patches.Base();

#if 0
		patch_t *prev;
		prev = face_patches[patches[i].faceNumber];
		patches[i].next = prev;
		face_patches[patches[i].faceNumber] = &patches[i];
#endif
	}

	// Cache off the leaf number:
	// We have to do this after subdivision because some patches span leaves.
	// (only the faces for model #0 are split by it's BSP which is what governs the PVS, and the leaves we're interested in)
	// Sub models (1-255) are only split for the BSP that their model forms.
	// When those patches are subdivided their origins can end up in a different leaf.
	// The engine will split (clip) those faces at run time to the world BSP because the models
	// are dynamic and can be moved.  In the software renderer, they must be split exactly in order
	// to sort per polygon.
	for ( i = 0; i < uiPatchCount; i++ )
	{
		patches[i].clusterNumber = ClusterFromPoint( patches[i].origin );

		//
		// test for point in solid space (can happen with detail and displacement surfaces)
		//
		if( patches[i].clusterNumber == -1 )
		{
			for( int j = 0; j < patches[i].winding->numpoints; j++ )
			{
				int clusterNumber = ClusterFromPoint( patches[i].winding->p[j] );
				if( clusterNumber != -1 )
				{
					patches[i].clusterNumber = clusterNumber;
					break;
				}
			}
		}
	}

	// build the list of patches that need to be lit
	for ( num = 0; num < uiPatchCount; num++ )
	{
		// do them in reverse order
		i = uiPatchCount - num - 1;

		// skip patches with children
		patch_t *pCur = &patches.Element( i );
		if( pCur->child1 == patches.InvalidIndex() )
		{
			if( pCur->clusterNumber != - 1 )
			{
				pCur->ndxNextClusterChild = clusterChildren.Element( pCur->clusterNumber );
				clusterChildren[pCur->clusterNumber] = pCur - patches.Base();
			}
		}

#if 0
		if (patches[i].child1 == patches.InvalidIndex() )
		{
			if( patches[i].clusterNumber != -1 )
			{
				patches[i].nextclusterchild = cluster_children[patches[i].clusterNumber];
				cluster_children[patches[i].clusterNumber] = &patches[i];
			}
		}
#endif
	}

	qprintf ("%i patches after subdivision\n", uiPatchCount);
}


//=====================================================================

/*
=============
MakeScales

  This is the primary time sink.
  It can be run multi threaded.
=============
*/
int	total_transfer;
int max_transfer;


void MakeTransfer( int ndxPatch1, int ndxPatch2, transfer_t *all_transfers )
//void MakeTransfer (patch_t *patch, patch_t *patch2, transfer_t *all_transfers )
{
	Vector	delta;
	vec_t	dist, scale;
	float	trans;
	transfer_t *transfer;

	//
	// get patches
	//
	if( ndxPatch1 == patches.InvalidIndex() || ndxPatch2 == patches.InvalidIndex() )
		return;

	patch_t *patch = &patches.Element( ndxPatch1 );
	patch_t *patch2 = &patches.Element( ndxPatch2 );

	// overflow check!
	transfer = &all_transfers[patch->numtransfers];

	// calculate transferemnce
	VectorSubtract (patch2->origin, patch->origin, delta);
	dist = VectorNormalize (delta);
		
	scale = DotProduct (delta, patch->normal);
	scale *= -DotProduct (delta, patch2->normal);

	// patch normals may be > 90 due to smoothing groups
	if (scale <= 0)
	{
		//Msg("scale <= 0\n");
		return;
	}

	trans = (patch2->area*scale);

	// hack for patch areas that area <= 0 (degenerate)
	if (trans <= 0)
	{
		//Msg("trans <= 0 : %f %f %f\n", trans, patch2->area, scale );
		return;
	}

	trans = trans / (dist*dist);

	if (trans <= TRANSFER_EPSILON)
	{
		//Msg("trans2 <= 0\n");
		return;
	}

	transfer->patch = patch2 - patches.Base();

	// FIXME: why is this not trans?
	transfer->transfer = (patch2->area*scale) / (dist*dist);

	patch->numtransfers++;
}


void MakeScales ( int ndxPatch, transfer_t *all_transfers )
{
	int		j;
	float	total;
	transfer_t	*t, *t2;
	total = 0;

	if( ndxPatch == patches.InvalidIndex() )
		return;
	patch_t *patch = &patches.Element( ndxPatch );

	// copy the transfers out
	if (patch->numtransfers)
	{
		if (patch->numtransfers > max_transfer)
		{
			max_transfer = patch->numtransfers;
		}


		patch->transfers = ( transfer_t* )calloc (1, patch->numtransfers * sizeof(transfer_t));
		if (!patch->transfers)
			Error ("Memory allocation failure");

		// get total transfer energy
		t2 = all_transfers;

		// overflow check!
		for (j=0 ; j<patch->numtransfers ; j++, t2++)
		{
			total += t2->transfer;
		}

		// the total transfer should be PI, but we need to correct errors due to overlaping surfaces
		if (total > M_PI)
			total = 1.0f/total;
		else	
			total = 1.0f/M_PI;

		t = patch->transfers;
		t2 = all_transfers;
		for (j=0 ; j<patch->numtransfers ; j++, t++, t2++)
		{
			t->transfer = t2->transfer*total;
			t->patch = t2->patch;
		}
		if (patch->numtransfers > max_transfer)
		{
			max_transfer = patch->numtransfers;
		}
	}
	else
	{
		// Error - patch has no transfers
		// patch->totallight[2] = 255;
	}

	ThreadLock ();
	total_transfer += patch->numtransfers;
	ThreadUnlock ();
}


/*
=============
WriteWorld
=============
*/
void WriteWorld (char *name)
{
	unsigned	j;
	FILE		*out;
	patch_t		*patch;

	out = fopen (name, "w");
	if (!out)
		Error ("Couldn't open %s", name);

	unsigned int uiPatchCount = patches.Size();
	for (j=0; j<uiPatchCount; j++)
	{
		patch = &patches.Element( j );

		// skip parent patches
		if (patch->child1 != patches.InvalidIndex() )
			continue;

		if( patch->clusterNumber == -1 )
		{
			Vector vGreen;
			VectorClear( vGreen );
			vGreen[1] = 256.0f;
			WriteWinding( out, patch->winding, vGreen );
		}
		else
		{
			WriteWinding( out, patch->winding, patch->totallight );
		}
	}

	g_pFileSystem->Close (out);
}


void WriteWinding (FileHandle_t out, winding_t *w, Vector& color )
{
	int			i;

	CmdLib_FPrintf (out, "%i\n", w->numpoints);
	for (i=0 ; i<w->numpoints ; i++)
	{
		CmdLib_FPrintf (out, "%5.2f %5.2f %5.2f %5.3f %5.3f %5.3f\n",
			w->p[i][0],
			w->p[i][1],
			w->p[i][2],
			color[ 0 ] / 256,
			color[ 1 ] / 256,
			color[ 2 ] / 256 );
	}
	CmdLib_FPrintf (out, "\n");
}


void WriteNormal( FileHandle_t out, Vector const &nPos, Vector const &nDir, 
				  float length, Vector const &color )
{
	CmdLib_FPrintf( out, "2\n" );
	CmdLib_FPrintf( out, "%5.2f %5.2f %5.2f %5.3f %5.3f %5.3f\n", 
		nPos.x, nPos.y, nPos.z,
		color.x / 256, color.y / 256, color.z / 256 );
	CmdLib_FPrintf( out, "%5.2f %5.2f %5.2f %5.3f %5.3f %5.3f\n", 
		nPos.x + ( nDir.x * length ), 
		nPos.y + ( nDir.y * length ), 
		nPos.z + ( nDir.z * length ),
		color.x / 256, color.y / 256, color.z / 256 );
	CmdLib_FPrintf( out, "\n" );
}



/*
=============
CollectLight
=============
*/
// patch's totallight += new light received to each patch
// patch's emitlight = addlight (newly received light from GatherLight)
// patch's addlight = 0
// pull received light from children.
void CollectLight( Vector& total )
{
	int i;
	patch_t	*patch;

	VectorFill( total, 0 );

	// process patches in reverse order so that children are processed before their parents
	unsigned int uiPatchCount = patches.Size();
	for( i = uiPatchCount - 1; i >= 0; i-- )
	{
		patch = &patches.Element( i );

		// sky's never collect light, it is just dropped
		if (patch->sky)
		{
			VectorFill( emitlight[ i ], 0 );
			VectorFill( addlight[ i ], 0 );
			continue;
		}

		if ( patch->child1 == patches.InvalidIndex() )
		{
			// This is a leaf node.
			VectorAdd( patch->totallight, addlight[i], patch->totallight );
			VectorCopy( addlight[i], emitlight[i] );
			VectorAdd( total, emitlight[i], total );
			VectorFill( addlight[ i ], 0 );
		}
		else
		{
			// This is an interior node.
			// Pull received light from children.
			float s1, s2;
			patch_t *child1;
			patch_t *child2;

			child1 = &patches[patch->child1];
			child2 = &patches[patch->child2];

			// BUG: This doesn't do anything?
			if ((int)patch->area != (int)(child1->area + child2->area))
				s1 = 0;

			s1 = child1->area / (child1->area + child2->area);
			s2 = child2->area / (child1->area + child2->area);

			// patch->totallight = s1 * child1->totallight + s2 * child2->totallight
			VectorScale( child1->totallight, s1, patch->totallight );
			VectorMA( patch->totallight, s2, child2->totallight, patch->totallight );

			// patch->emitlight = s1 * child1->emitlight + s2 * child2->emitlight
			VectorScale( emitlight[patch->child1], s1, emitlight[i] );
			VectorMA( emitlight[i], s2, emitlight[patch->child2], emitlight[i] );

			VectorFill( addlight[ i ], 0 );
		}
	}
}

/*
=============
GatherLight

Get light from other patches
  Run multi-threaded
=============
*/

#ifdef _WIN32
#pragma warning (disable:4701)
#endif

void GatherLight (int threadnum, void *pUserData)
{
	int			i, j, k;
	transfer_t	*trans;
	int			num;
	patch_t		*patch;
	Vector		sum, v;

	while (1)
	{
		j = GetThreadWork ();
		if (j == -1)
			break;

		patch = &patches[j];

		trans = patch->transfers;
		num = patch->numtransfers;

		VectorFill( sum, 0 );

		for (k=0 ; k<num ; k++, trans++)
		{
			for(i=0; i<3; i++)
				v[i] = emitlight[trans->patch][i] * patches[trans->patch].reflectivity[i];
			VectorScale( v, trans->transfer, v );
			VectorAdd( sum, v, sum );
		}

		VectorCopy( sum, addlight[j] );
	}
}

#ifdef _WIN32
#pragma warning (default:4701)
#endif


/*
=============
BounceLight
=============
*/
void BounceLight (void)
{
	unsigned i;
	Vector	added;
	char		name[64];
	qboolean	bouncing = numbounce > 0;

	unsigned int uiPatchCount = patches.Size();
	for (i=0 ; i<uiPatchCount; i++)
	{
		VectorCopy( patches[i].totallight, emitlight[i] );
	}

#if 0
	FileHandle_t dFp = g_pFileSystem->Open( "lightemit.txt", "w" );

	unsigned int uiPatchCount = patches.Size();
	for (i=0 ; i<uiPatchCount; i++)
	{
		CmdLib_FPrintf( dFp, "Emit %d: %f %f %f\n", i, emitlight[i].x, emitlight[i].y, emitlight[i].z );
	}

	g_pFileSystem->Close( dFp );

	for (i=0; i<num_patches ; i++)
	{
		Vector total;

		VectorSubtract (patches[i].maxs, patches[i].mins, total);
		Msg("%4d %4d %4d %4d (%d) %.0f", i, patches[i].parent, patches[i].child1, patches[i].child2, patches[i].samples, patches[i].area );
		Msg(" [%.0f %.0f %.0f]", total[0], total[1], total[2] );
		if (patches[i].child1 != patches.InvalidIndex() )
		{
			Vector tmp;
			VectorScale( patches[i].totallight, patches[i].area, tmp );

			VectorMA( tmp, -patches[patches[i].child1].area, patches[patches[i].child1].totallight, tmp );
			VectorMA( tmp, -patches[patches[i].child2].area, patches[patches[i].child2].totallight, tmp );
			// Msg("%.0f ", VectorLength( tmp ) );
			// Msg("%d ", patches[i].samples - patches[patches[i].child1].samples - patches[patches[i].child2].samples );
			// Msg("%d ", patches[i].samples );
		}
		Msg("\n");
	}
#endif

	i = 0;
	while ( bouncing )
	{
		// transfer light from to the leaf patches from other patches via transfers
		// this moves shooter->emitlight to receiver->addlight
		unsigned int uiPatchCount = patches.Size();
		RunThreadsOn (uiPatchCount, true, GatherLight);
		// move newly received light (addlight) to light to be sent out (emitlight)
		// start at children and pull light up to parents
		// light is always received to leaf patches
		CollectLight( added );

		qprintf ("\tBounce #%i added RGB(%.0f, %.0f, %.0f)\n", i+1, added[0], added[1], added[2] );

		if ( i+1 == numbounce || (added[0] < 1.0 && added[1] < 1.0 && added[2] < 1.0) )
			bouncing = false;

		i++;
		if ( dumppatches && !bouncing && i != 1)
		{
			sprintf (name, "bounce%i.txt", i);
			WriteWorld (name);
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: Counts the number of clusters in a map with no visibility
// Output : int
//-----------------------------------------------------------------------------
int CountClusters( void )
{
	int clusterCount = 0;

	for ( int i = 0; i < numleafs; i++ )
	{
		if ( dleafs[i].cluster > clusterCount )
			clusterCount = dleafs[i].cluster;
	}

	return clusterCount + 1;
}


/*
=============
RadWorld
=============
*/
void RadWorld_Start()
{
	unsigned	i;

	if (luxeldensity < 1.0)
	{
		// Remember the old lightmap vectors.
		float oldLightmapVecs[MAX_MAP_TEXINFO][2][4];
		for (i = 0; i < numtexinfo; i++)
		{
			for( int j=0; j < 2; j++ )
				for( int k=0; k < 3; k++ )
					oldLightmapVecs[i][j][k] = texinfo[i].lightmapVecsLuxelsPerWorldUnits[j][k];
		}

		// rescale luxels to be no denser than "luxeldensity"
		for (i = 0; i < numtexinfo; i++)
		{
			texinfo_t	*tx = &texinfo[i];

			for (int j = 0; j < 2; j++ )
			{
				Vector tmp( tx->lightmapVecsLuxelsPerWorldUnits[j][0], tx->lightmapVecsLuxelsPerWorldUnits[j][1], tx->lightmapVecsLuxelsPerWorldUnits[j][2] );
				float scale = VectorNormalize( tmp );
				// only rescale them if the current scale is "tighter" than the desired scale
				// FIXME: since this writes out to the BSP file every run, once it's set high it can't be reset
				// to a lower value.
				if (fabs( scale ) > luxeldensity)
				{
					if (scale < 0)
						scale = -luxeldensity;
					else
						scale = luxeldensity;
					VectorScale( tmp, scale, tmp );
					tx->lightmapVecsLuxelsPerWorldUnits[j][0] = tmp.x;
					tx->lightmapVecsLuxelsPerWorldUnits[j][1] = tmp.y;
					tx->lightmapVecsLuxelsPerWorldUnits[j][2] = tmp.z;
				}
			}
		}
		
		UpdateAllFaceLightmapExtents();
	}

	MakeParents (0, -1);
	MakeTnodes (&dmodels[0]);

	BuildClusterTable();

	// turn each face into a single patch
	MakePatches ();
	PairEdges ();

	// store the vertex normals calculated in PairEdges
	// so that the can be written to the bsp file for 
	// use in the engine
	SaveVertexNormals();

	// subdivide patches to a maximum dimension
	SubdividePatches ();

	// add displacement faces to cluster table
	AddDispsToClusterTable();

	// create directlights out of patches and lights
	CreateDirectLights ();

	// set up sky cameras
	ProcessSkyCameras();
}


// This function should fill in the indices into dfaces[] for the faces
// with displacements that touch the specified leaf.
void STUB_GetDisplacementsTouchingLeaf( int iLeaf, CUtlVector<int> &dispFaces )
{
}


void BuildFacesVisibleToLights( bool bAllVisible )
{
	g_FacesVisibleToLights.SetSize( numfaces/8 + 1 );

	if( bAllVisible )
	{
		memset( g_FacesVisibleToLights.Base(), 0xFF, g_FacesVisibleToLights.Count() );
		return;
	}

	// First merge all the light PVSes.
	CUtlVector<byte> aggregate;
	aggregate.SetSize( (dvis->numclusters/8) + 1 );
	memset( aggregate.Base(), 0, aggregate.Count() );

	int nDWords = aggregate.Count() / 4;
	int nBytes = aggregate.Count() - nDWords*4;

	for( directlight_t *dl = activelights; dl != NULL; dl = dl->next )
	{
		byte *pIn  = dl->pvs;
		byte *pOut = aggregate.Base();
		for( int iDWord=0; iDWord < nDWords; iDWord++ )
		{
			*((unsigned long*)pOut) |= *((unsigned long*)pIn);
			pIn  += 4;
			pOut += 4;
		}

		for( int iByte=0; iByte < nBytes; iByte++ )
		{
			*pOut |= *pIn;
			++pOut;
			++pIn;
		}
	}


	// Setup a map from clusters to leafs.
	int i;
	CUtlVector<int> clusterToLeaf;
	clusterToLeaf.SetSize( numclusters );
	for( i=0; i < numclusters; i++ )
	{
		clusterToLeaf[i] = -1;
	}

	for( i=1; i < numleafs; i++ )
	{
		if( dleafs[i].cluster >= 0 && dleafs[i].cluster < numclusters )
		{
			clusterToLeaf[ dleafs[i].cluster ] = i;
		}
	}


	// Now tag any faces that are visible to this monster PVS.
	for( int iCluster=0; iCluster < numclusters; iCluster++ )
	{
		if( clusterToLeaf[iCluster] > 0 && clusterToLeaf[iCluster] < numleafs )
		{
			if( aggregate[iCluster>>3] & (1 << (iCluster & 7)) )
			{
				int iLeaf = clusterToLeaf[iCluster];

				// Tag all the faces.
				int iFace;
				for( iFace=0; iFace < dleafs[iLeaf].numleaffaces; iFace++ )
				{
					int index = dleafs[iLeaf].firstleafface + iFace;
					index = dleaffaces[index];
					
					assert( index < numfaces );
					g_FacesVisibleToLights[index >> 3] |= (1 << (index & 7));
				}

				// Fill in STUB_GetDisplacementsTouchingLeaf when it's available
				// so displacements get relit.
				CUtlVector<int> dispFaces;
				STUB_GetDisplacementsTouchingLeaf( iLeaf, dispFaces );
				for( iFace=0; iFace < dispFaces.Count(); iFace++ )
				{
					int index = dispFaces[iFace];
					g_FacesVisibleToLights[index >> 3] |= (1 << (index & 7));
				}
			}
		}
	}

	// For stats.. figure out how many faces it's going to touch.
	int nFacesToProcess = 0;
	for( i=0; i < numfaces; i++ )
	{
		if( g_FacesVisibleToLights[i>>3] & (1 << (i & 7)) )
			++nFacesToProcess;
	}
}



void MakeAllScales (void)
{
	// determine visibility between patches
	BuildVisMatrix ();
	
	// release visibility matrix
	FreeVisMatrix ();

	Msg("transfers %d, max %d\n", total_transfer, max_transfer );

	qprintf ("transfer lists: %5.1f megs\n"
		, (float)total_transfer * sizeof(transfer_t) / (1024*1024));
}


bool RadWorld_Go()
{
	g_iCurFace = 0;

	InitMacroTexture( source );

	if( g_pIncremental )
	{
		g_pIncremental->PrepareForLighting();

		// Cull out faces that aren't visible to any of the lights that we're updating with.
		BuildFacesVisibleToLights( false );
	}
	else
	{
		// Mark all faces visible.. when not doing incremental lighting, it's highly
		// likely that all faces are going to be touched by at least one light so don't
		// waste time here.
		BuildFacesVisibleToLights( true );
	}

	// build initial facelights
	if (g_bUseMPI) 
	{
		// RunThreadsOnIndividual (numfaces, true, BuildFacelights);
		RunMPIBuildFacelights();
	}
	else 
	{
		RunThreadsOnIndividual (numfaces, true, BuildFacelights);
	}

	// Was the process interrupted?
	if( g_pIncremental && (g_iCurFace != numfaces) )
		return false;

	// Figure out the offset into lightmap data for each face.
	PrecompLightmapOffsets();
	
	// If we're doing incremental lighting, stop here.
	if( g_pIncremental )
	{
		g_pIncremental->Finalize();
	}
	else
	{
		// free up the direct lights now that we have facelights
		DeleteDirectLights ();

		if ( dumppatches )
			WriteWorld( "bounce0.txt" );

		if (numbounce > 0)
		{
			// allocate memory for emitlight/addlight
			emitlight.SetSize( patches.Size() );
			memset( emitlight.Base(), 0, patches.Size() * sizeof( Vector ) );
			addlight.SetSize( patches.Size() );
			memset( addlight.Base(), 0, patches.Size() * sizeof( Vector ) );

			MakeAllScales ();

			// spread light around
			BounceLight ();

			// subtract out light gathered in the directlight pass
			unsigned int uiPatchCount = patches.Size();
			for( int i=0; i < uiPatchCount; i++ )
				if ( !VectorCompare( patches[i].directlight, vec3_origin ) )
					VectorSubtract( patches[i].totallight, patches[i].directlight, patches[i].totallight );
		}

		if ( g_bUseMPI && !g_bMPIMaster )
		{
			//
			// This is the last use of the MPI slaves
			//
			CmdLib_Exit( 0 );
		}

		//
		// displacement surface luxel accumulation (make threaded!!!)
		//
		StaticDispMgr()->StartTimer( "Build Patch/Sample Hash Table(s)....." );
		StaticDispMgr()->InsertSamplesDataIntoHashTable();
		StaticDispMgr()->InsertPatchSampleDataIntoHashTable();
		StaticDispMgr()->EndTimer();

		// blend bounced light into direct light and save
		RunThreadsOnIndividual (numfaces, true, FinalLightFace);
		Msg("FinalLightFace Done\n"); fflush(stdout);
	}

	return true;
}

// declare the sample file pointer -- the whole debug print system should
// be reworked at some point!! (cab)
FileHandle_t pFileSamples[4];

void LoadPhysicsDLL( void )
{
	PhysicsDLLPath( "VPHYSICS.DLL" );
}


void VRAD_LoadBSP( char const *pFilename )
{
	ThreadSetDefault ();

	g_flStartTime = I_FloatTime ();

	if( g_bLowPriority )
	{
		SetLowPriority();
	}

	strcpy (source, pFilename);
	StripExtension (source);
	CmdLib_InitFileSystem( source );

	// This part is just for VMPI. VMPI's file system needs the basedir in front of all filenames,
	// so we strip off the filename and prepend qdir here.
	ExtractFileBase( source, source, sizeof( source ) );
	strcpy( source, ExpandPath( source ) );

	if ( !g_bUseMPI )
	{
		// Setup the logfile.
		char logFile[512];
		_snprintf( logFile, sizeof(logFile), "%s.log", source );
		SetSpewFunctionLogFile( logFile );
	}

	LoadPhysicsDLL();

	// Set the required global lights filename and try looking in qproject
	strcat( strcpy( global_lights, gamedir ), "lights.rad" );
	if ( !g_pFileSystem->FileExists( global_lights ) )
	{
		// Otherwise, try looking in the BIN directory from which we were run from
		Msg( "Could not find lights.rad in %s.\nTrying VRAD BIN directory instead...\n", 
			    global_lights );
		GetModuleFileName( NULL, global_lights, sizeof( global_lights ) );
		ExtractFilePath( global_lights, global_lights );
		strcat( global_lights, "lights.rad" );
	}

	// Set the optional level specific lights filename
	DefaultExtension( strcpy( level_lights, source ), ".rad" );
	if ( !g_pFileSystem->FileExists( level_lights ) ) 
		*level_lights = 0;	

	ReadLightFile(global_lights);							// Required
	if ( *designer_lights ) ReadLightFile(designer_lights);	// Command-line
	if ( *level_lights )	ReadLightFile(level_lights);	// Optional & implied

	strcpy(incrementfile, source);
	DefaultExtension(incrementfile, ".r0");
	DefaultExtension(source, ".bsp");

	GetPlatformMapPath( source, platformPath, g_nDXLevel, MAX_PATH );

	Msg( "Loading %s\n", platformPath );
	LoadBSPFile (platformPath);

	ParseEntities ();

	StaticPropMgr()->Init();
	StaticDispMgr()->Init();

	if (!visdatasize)
	{
		Msg("No vis information, direct lighting only.\n");
		numbounce = 0;
		ambient[0] = ambient[1] = ambient[2] = 0.1f;
		dvis->numclusters = CountClusters();
	}

	//
	// patches and referencing data (ensure capacity)
	//
	// TODO: change the maxes to the amount from the bsp!!
	//
//	patches.EnsureCapacity( MAX_PATCHES );

	facePatches.SetSize( MAX_MAP_FACES );
	faceParents.SetSize( MAX_MAP_FACES );
	clusterChildren.SetSize( MAX_MAP_CLUSTERS );

	int ndx;
	for( ndx = 0; ndx < MAX_MAP_FACES; ndx++ )
	{
		facePatches[ndx] = facePatches.InvalidIndex();
		faceParents[ndx] = faceParents.InvalidIndex();
	}

	for( ndx = 0; ndx < MAX_MAP_CLUSTERS; ndx++ )
	{
		clusterChildren[ndx] = clusterChildren.InvalidIndex();
	}

	RadWorld_Start();

	// Setup incremental lighting.
	if( g_pIncremental )
	{
		if( !g_pIncremental->Init( source, incrementfile ) )
		{
			Error( "Unable to load incremental lighting file in %s.\n", incrementfile );
			return;
		}
	}
}


void VRAD_Finish()
{
	// Compute lighting for the bsp file
	ComputeDetailPropLighting( THREADINDEX_MAIN );

	if (verbose)
		PrintBSPFileSizes ();

	Msg( "Writing %s\n", platformPath );
	WriteBSPFile (platformPath);

	if( dumppatches )
	{
		for( int ndx = 0; ndx < 4; ndx++ )
		{
			g_pFileSystem->Close( pFileSamples[ndx] );
		}
	}

	double end = I_FloatTime ();
	Msg("%5.0f seconds elapsed\n", end-g_flStartTime);
}


// Run startup code like initialize mathlib (called from main() and from the 
// WorldCraft interface into vrad).
void VRAD_Init()
{
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false );
	InstallSpewFunction();
}


int RunVRAD( int argc, char **argv )
{
	int i;
	bool onlydetail = false;
	
#if defined(_MSC_VER) && ( _MSC_VER >= 1310 )
	Msg("Valve Software - vrad.exe SSE (" __DATE__ ")\n" );
#else
	Msg("Valve Software - vrad.exe (" __DATE__ ")\n" );
#endif

	Msg("----- Radiosity Simulator ----\n");

	verbose = true;  // Originally FALSE

	for( i=1 ; i<argc ; i++ )
	{
		if (!strcmp(argv[i],"-dump"))
		{
			dumppatches = true;

			for( int ndx = 0; ndx < 4; ndx++ )
			{
				char filename[MAX_PATH];
				sprintf( filename, "samples%i.txt", ndx );
				pFileSamples[ndx] = g_pFileSystem->Open( filename, "w" );
				if( !pFileSamples[ndx] )
					return -1;
			}
		}
		else if( !strcmp( argv[i], "-dxlevel" ) )
		{
			g_nDXLevel = atoi( argv[i+1] );
			Msg( "DXLevel = %d\n", g_nDXLevel );
			++i;
		}
		else if( !strcmp( argv[i], "-red2black" ) )
		{
			bRed2Black = true;
		}
		else if( !strcmp( argv[i], "-matpath" ) )
		{
			strcpy(qproject, argv[i+1]);
			++i;
		}
		else if( !strcmp( argv[i], "-dumpnormals" ) )
		{
			bDumpNormals = true;
		}
		else if (!strcmp(argv[i],"-bounce"))
		{
			if ( ++i < argc )
			{
#if ALLOWOPTIONS
				numbounce = atoi (argv[i]);
				if ( numbounce < 0 )
				{
					Warning("Error: expected non-negative value after '-bounce'\n" );
					return 1;
				}
#else
				if (atoi (argv[i]) == 0)
				{
					numbounce = 0;
				}
#endif
			}
			else
			{
				Warning("Error: expected a value after '-bounce'\n" );
				return 1;
			}
		}
		else if (!strcmp(argv[i],"-verbose"))
		{
			verbose = true;
		}
		else if (!strcmp(argv[i],"-terse"))
		{
			verbose = false;
		}
		else if (!strcmp(argv[i],"-threads"))
		{
			if ( ++i < argc )
			{
				numthreads = atoi (argv[i]);
				if ( numthreads <= 0 )
				{
					Warning("Error: expected positive value after '-threads'\n" );
					return 1;
				}
			}
			else
			{
				Warning("Error: expected a value after '-threads'\n" );
				return 1;
			}
		}
#if ALLOWOPTIONS
		else if (!strcmp(argv[i],"-maxchop"))
		{
			if ( ++i < argc )
			{
				maxchop = (float)atof (argv[i]);
				if ( maxchop < 2 )
				{
					Warning("Error: expected positive value after '-maxchop'\n" );
					return 1;
				}
			}
			else
			{
				Warning("Error: expected a value after '-maxchop'\n" );
				return 1;
			}
		}
#endif
#if ALLOWOPTIONS
		else if (!strcmp(argv[i],"-chop"))
		{
			if ( ++i < argc )
			{
				minchop = (float)atof (argv[i]);
				if ( minchop < 1 )
				{
					Warning("Error: expected positive value after '-chop'\n" );
					return 1;
				}
				if ( minchop < 32 )
				{
					Warning("WARNING: Chop values below 32 are not recommended.  Use -extra instead.\n");
				}
			}
			else
			{
				Warning("Error: expected a value after '-chop'\n" );
				return 1;
			}
		}
#endif
#if ALLOWOPTIONS
		else if (!strcmp(argv[i],"-scale"))
		{
			if ( ++i < argc )
			{
				lightscale = (float)atof (argv[i]);
			}
			else
			{
				Warning("Error: expected a value after '-scale'\n" );
				return 1;
			}
		}
#endif
#if ALLOWOPTIONS
		else if (!strcmp(argv[i],"-ambient"))
		{
			if ( i+3 < argc )
			{
 				ambient[0] = (float)atof (argv[++i]) * 128;
 				ambient[1] = (float)atof (argv[++i]) * 128;
 				ambient[2] = (float)atof (argv[++i]) * 128;
			}
			else
			{
				Warning("Error: expected three color values after '-ambient'\n" );
				return 1;
			}
		}
#endif
// UNDONE: JAY: Is this necessary anymore?
		else if( !strcmp(argv[i], "-proj") )
		{
			if ( ++i < argc && *argv[i] )
				strcpy( qproject, argv[i] );
			else
			{
				Warning("Error: expected path name after '-proj'\n" );
				return 1;
			}
		}
#if ALLOWOPTIONS
		else if ( !strcmp(argv[i], "-maxlight") )
		{
			if ( ++i < argc && *argv[i] )
			{
				maxlight = (float)atof (argv[i]) * 256;
				if ( maxlight <= 0 )
				{
					Warning("Error: expected positive value after '-maxlight'\n" );
					return 1;
				}
			}
			else
			{
				Warning("Error: expected a value after '-maxlight'\n" );
				return 1;
			}
		}
#endif
		else if ( !strcmp(argv[i], "-lights" ) )
		{
			if ( ++i < argc && *argv[i] )
			{
				strcpy( designer_lights, argv[i] );
			}
			else
			{
				Warning("Error: expected a filepath after '-lights'\n" );
				return 1;
			}
		}
#if ALLOWOPTIONS
		else if (!strcmp(argv[i],"-dlight"))
		{
			if ( ++i < argc )
			{
				dlight_threshold = (float)atof (argv[i]);
			}
			else
			{
				Warning("Error: expected a value after '-dlight'\n" );
				return 1;
			}
		}
#endif
		else if (!strcmp(argv[i],"-extra"))
		{
			do_extra = true;
		}
		else if (!strcmp(argv[i],"-noextra"))
		{
			do_extra = false;
		}
		else if (!strcmp(argv[i],"-debugextra"))
		{
			debug_extra = true;
		}
		else if (!strcmp(argv[i],"-fast"))
		{
			do_fast = true;
		}
		else if (!strcmp(argv[i],"-centersamples"))
		{
			do_centersamples = true;
		}
		else if( !strcmp( argv[i], "-dispblend" ) )
		{
			do_dispblend = false;
		}
#if ALLOWOPTIONS
		else if (!strcmp(argv[i],"-sky"))
		{
			if ( ++i < argc )
			{
				indirect_sun = (float)atof (argv[i]);
			}
			else
			{
				Warning("Error: expected a value after '-sky'\n" );
				return 1;
			}
		}
#endif
		else if (!strcmp(argv[i],"-smooth"))
		{
			if ( ++i < argc )
			{
				smoothing_threshold = (float)cos(atof(argv[i])*(M_PI/180.0));
			}
			else
			{
				Warning("Error: expected an angle after '-smooth'\n" );
				return 1;
			}
		}
#if ALLOWOPTIONS
		else if (!strcmp(argv[i],"-coring"))
		{
			if ( ++i < argc )
			{
				coring = (float)atof( argv[i] );
			}
			else
			{
				Warning("Error: expected a light threshold after '-coring'\n" );
				return 1;
			}
		}
#endif
#if ALLOWOPTIONS
		else if (!strcmp(argv[i],"-notexscale"))
		{
			texscale = false;
		}
#endif
		else if (!strcmp(argv[i],"-dlightmap"))
		{
			dlight_map = 1;
		}
		/*
		else if ( !strcmp(argv[i], "-reflectivity") )
		{
			if ( ++i < argc )
			{
				reflectivityScale = (float)atof (argv[i]);
			}
			else
			{
				fprintf( stderr, "Error: expected a value after '-reflectivity'\n" );
				return 1;
			}
		}
		*/
		else if (!strcmp(argv[i],"-luxeldensity"))
		{
			if ( ++i < argc )
			{
				luxeldensity = (float)atof (argv[i]);
				if (luxeldensity > 1.0)
					luxeldensity = 1.0 / luxeldensity;
			}
			else
			{
				Warning("Error: expected a value after '-luxeldensity'\n" );
				return 1;
			}
		}
		else if( !stricmp( argv[i], "-low" ) )
		{
			g_bLowPriority = true;
		}
		else if( !stricmp( argv[i], "-loghash" ) )
		{
			g_bLogHashData = true;
		}
		else if( !stricmp( argv[i], "-onlydetail" ) )
		{
			onlydetail = true;
		}
		else if ( stricmp( argv[i], "-StopOnExit" ) == 0 )
		{
			g_bStopOnExit = true;
		}
		else if ( !Q_strncasecmp( argv[i], "-mpi", 4 ) || !Q_strncasecmp( argv[i-1], "-mpi", 4 ) )
		{
			if ( stricmp( argv[i], "-mpi" ) == 0 )
				g_bUseMPI = true;
		
			// Any other args that start with -mpi are ok too.
			if ( i == argc - 1 )
				break;
		}
		else
		{
			break;
		}
	}

	if (i != argc - 1)
		Error ( "usage: vrad [-dump] [-inc] [-bounce n] [-threads n] [-verbose] [-terse] [-proj file] [-maxlight n] [-threads n] [-lights file] [-extra] [-smooth n] [-dlightmap] [-fast] [-blendsamples] [-lowpriority] [-StopOnExit] [-mpi] bspfile" );

	VRAD_LoadBSP( argv[i] );

	if (!onlydetail)
		RadWorld_Go();

	Msg("Ready to Finish\n"); fflush(stdout);
	VRAD_Finish();

	CmdLib_Cleanup();
	return 0;
}


int VRAD_Main(int argc, char **argv)
{
	VRAD_Init();

	// This must come first.
	VRAD_SetupMPI( argc, argv );


	if ( g_bUseMPI && !g_bMPIMaster )
	{
		// VMPI workers should catch crashes and asserts and suchlike and fail gracefully instead 
		// of pestering the person with dialogs.
		try
		{
			return RunVRAD( argc, argv );
		}
		catch( ... )
		{
			VMPI_HandleCrash( "crashed", true );
			return 0;
		}
	}
	else
	{
		return RunVRAD( argc, argv );
	}
}





