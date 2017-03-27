//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: BSP Building tool
//
// $NoKeywords: $
//=============================================================================



#include "vbsp.h"
#include "detail.h"
#include "physdll.h"
#include "utilmatlib.h"
#include "disp_vbsp.h"
#include "writebsp.h"
#include "vstdlib/icommandline.h"

extern	float g_maxLightmapDimension;

char		source[1024];
char		mapbase[ 64 ];
char		name[1024];
char		materialPath[1024];

vec_t		microvolume = 1.0;
qboolean	noprune;
qboolean	glview;
qboolean	nodetail;
qboolean	fulldetail;
qboolean	onlyents;
bool		onlyprops;
qboolean	nomerge;
qboolean	nowater;
qboolean	nofill;
qboolean	nocsg;
qboolean	noweld;
qboolean	noshare;
qboolean	nosubdiv;
qboolean	notjunc;
qboolean	noopt;
qboolean	leaktest;
qboolean	verboseentities;
qboolean	dumpcollide = false;
qboolean	nodetailcuts = true;
qboolean	g_bLowPriority = false;
qboolean	g_DumpStaticProps = false;
bool		g_bLightIfMissing = false;
bool		g_snapAxialPlanes = false;

float		g_defaultLuxelSize = DEFAULT_LUXEL_SIZE;
float		g_luxelScale = 1.0f;
bool		g_BumpAll = true;

int			g_nDXLevel = 90; // default dxlevel if you don't specify it on the command-line.

char		outbase[32];

// HLTOOLS: Introduce these calcs to make the block algorithm proportional to the proper 
// world coordinate extents.  Assumes square spatial constraints.
#define BLOCKS_SPACE	(COORD_EXTENT/1024)
#define BLOCKX_OFFSET	(BLOCKS_SPACE/2)
#define BLOCKY_OFFSET	(BLOCKS_SPACE/2)
#define BLOCKS_MIN		(-(BLOCKS_SPACE/2))
#define BLOCKS_MAX		((BLOCKS_SPACE/2)-1)

int			block_xl = BLOCKS_MIN, block_xh = BLOCKS_MAX, block_yl = BLOCKS_MIN, block_yh = BLOCKS_MAX;

int			entity_num;


node_t		*block_nodes[BLOCKS_SPACE+2][BLOCKS_SPACE+2];

/*
============
BlockTree

============
*/
node_t	*BlockTree (int xl, int yl, int xh, int yh)
{
	node_t	*node;
	Vector	normal;
	float	dist;
	int		mid;

	if (xl == xh && yl == yh)
	{
		node = block_nodes[xl+BLOCKX_OFFSET][yl+BLOCKY_OFFSET];
		if (!node)
		{	// return an empty leaf
			node = AllocNode ();
			node->planenum = PLANENUM_LEAF;
			node->contents = 0; //CONTENTS_SOLID;
			return node;
		}
		return node;
	}

	// create a seperator along the largest axis
	node = AllocNode ();

	if (xh - xl > yh - yl)
	{	// split x axis
		mid = xl + (xh-xl)/2 + 1;
		normal[0] = 1;
		normal[1] = 0;
		normal[2] = 0;
		dist = mid*1024;
		node->planenum = FindFloatPlane (normal, dist);
		node->children[0] = BlockTree ( mid, yl, xh, yh);
		node->children[1] = BlockTree ( xl, yl, mid-1, yh);
	}
	else
	{
		mid = yl + (yh-yl)/2 + 1;
		normal[0] = 0;
		normal[1] = 1;
		normal[2] = 0;
		dist = mid*1024;
		node->planenum = FindFloatPlane (normal, dist);
		node->children[0] = BlockTree ( xl, mid, xh, yh);
		node->children[1] = BlockTree ( xl, yl, xh, mid-1);
	}

	return node;
}

/*
============
ProcessBlock_Thread

============
*/
int			brush_start, brush_end;
void ProcessBlock_Thread (int threadnum, int blocknum)
{
	int		xblock, yblock;
	Vector		mins, maxs;
	bspbrush_t	*brushes;
	tree_t		*tree;
	node_t		*node;

	yblock = block_yl + blocknum / (block_xh-block_xl+1);
	xblock = block_xl + blocknum % (block_xh-block_xl+1);

	qprintf ("############### block %2i,%2i ###############\n", xblock, yblock);

	// HLTOOLS: Should be fixed now
	mins[0] = xblock*1024;
	mins[1] = yblock*1024;
	mins[2] = MIN_COORD_INTEGER;
	maxs[0] = (xblock+1)*1024;
	maxs[1] = (yblock+1)*1024;
	maxs[2] = MAX_COORD_INTEGER;

	// the makelist and chopbrushes could be cached between the passes...
	brushes = MakeBspBrushList (brush_start, brush_end, mins, maxs, NO_DETAIL);
	if (!brushes)
	{
		node = AllocNode ();
		node->planenum = PLANENUM_LEAF;
		node->contents = CONTENTS_SOLID;
		block_nodes[xblock+BLOCKX_OFFSET][yblock+BLOCKY_OFFSET] = node;
		return;
	}    

	FixupAreaportalWaterBrushes( brushes );
	if (!nocsg)
		brushes = ChopBrushes (brushes);

	tree = BrushBSP (brushes, mins, maxs);

	block_nodes[xblock+BLOCKX_OFFSET][yblock+BLOCKY_OFFSET] = tree->headnode;
}


/*
============
ProcessWorldModel

============
*/
void SplitSubdividedFaces( node_t *headnode ); // garymcthack
void ProcessWorldModel (void)
{
	entity_t	*e;
	tree_t		*tree;
	qboolean	leaked;
	int	optimize;
	int			start;

	e = &entities[entity_num];

	brush_start = e->firstbrush;
	brush_end = brush_start + e->numbrushes;
	leaked = false;

	//
	// perform per-block operations
	//

	// JAY: Fixed this to take the larger coordinate system into account
	if (block_xh * 1024 > map_maxs[0])
		block_xh = floor(map_maxs[0]/1024.0);
	if ( (block_xl+1) * 1024 < map_mins[0])
		block_xl = floor(map_mins[0]/1024.0);
	if (block_yh * 1024 > map_maxs[1])
		block_yh = floor(map_maxs[1]/1024.0);
	if ( (block_yl+1) * 1024 < map_mins[1])
		block_yl = floor(map_mins[1]/1024.0);

	// HLTOOLS: updated to +/- MAX_COORD_INTEGER ( new world size limits / worldsize.h )
	if (block_xl < BLOCKS_MIN)
		block_xl = BLOCKS_MIN;
	if (block_yl < BLOCKS_MIN)
		block_yl = BLOCKS_MIN;
	if (block_xh > BLOCKS_MAX)
		block_xh = BLOCKS_MAX;
	if (block_yh > BLOCKS_MAX)
		block_yh = BLOCKS_MAX;

	for (optimize = 0 ; optimize <= 1 ; optimize++)
	{
		qprintf ("--------------------------------------------\n");

		RunThreadsOnIndividual ((block_xh-block_xl+1)*(block_yh-block_yl+1),
			!verbose, ProcessBlock_Thread);

		//
		// build the division tree
		// oversizing the blocks guarantees that all the boundaries
		// will also get nodes.
		//

		qprintf ("--------------------------------------------\n");

		tree = AllocTree ();
		tree->headnode = BlockTree (block_xl-1, block_yl-1, block_xh+1, block_yh+1);

		tree->mins[0] = (block_xl)*1024;
		tree->mins[1] = (block_yl)*1024;
		tree->mins[2] = map_mins[2] - 8;

		tree->maxs[0] = (block_xh+1)*1024;
		tree->maxs[1] = (block_yh+1)*1024;
		tree->maxs[2] = map_maxs[2] + 8;

		//
		// perform the global operations
		//

		// make the portals/faces by traversing down to each empty leaf
		MakeTreePortals (tree);

		if (FloodEntities (tree))
		{
			// turns everthing outside into solid
			FillOutside (tree->headnode);
		}
		else
		{
			Warning( ("**** leaked ****\n") );
			leaked = true;
			LeakFile (tree);
			if (leaktest)
			{
				Warning( ("--- MAP LEAKED ---\n") );
				exit (0);
			}
		}

		// mark the brush sides that actually turned into faces
		MarkVisibleSides (tree, brush_start, brush_end, NO_DETAIL);
		if (noopt || leaked)
			break;
		if (!optimize)
		{
			// If we are optimizing, free the tree.  Next time we will construct it again, but
			// we'll use the information in MarkVisibleSides() so we'll only split with planes that
			// actually contribute renderable geometry
			FreeTree (tree);
		}
	}

	FloodAreas (tree);

	RemoveAreaPortalBrushes_R( tree->headnode );

	start = I_FloatTime();
	Msg("Building Faces...");
	// this turns portals with one solid side into faces
	// it also subdivides each face if necessary to fit max lightmap dimensions
	MakeFaces (tree->headnode);
	Msg("done (%d)\n", (int)(I_FloatTime() - start) );

	if (glview)
	{
		WriteGLView (tree, source);
	}

	face_t *pLeafFaceList = NULL;
	if ( !nodetail )
	{
		pLeafFaceList = MergeDetailTree( tree, brush_start, brush_end );
	}

	start = I_FloatTime();

	Msg("FixTjuncs...\n");
	
	// This unifies the vertex list for all edges (splits collinear edges to remove t-junctions)
	// It also welds the list of vertices out of each winding/portal and rounds nearly integer verts to integer
	pLeafFaceList = FixTjuncs (tree->headnode, pLeafFaceList);

	// this merges all of the solid nodes that have separating planes
	if (!noprune)
	{
		Msg("PruneNodes...\n");
		PruneNodes (tree->headnode);
	}

	Msg( "SplitSubdividedFaces...\n" );
//	SplitSubdividedFaces( tree->headnode );
	
	Msg("WriteBSP...\n");
	WriteBSP (tree->headnode, pLeafFaceList);
	Msg("done (%d)\n", (int)(I_FloatTime() - start) );

	if (!leaked)
	{
		WritePortalFile (tree);
	}

	FreeTree (tree);
	FreeLeafFaces( pLeafFaceList );
}

/*
============
ProcessSubModel

============
*/
void ProcessSubModel( )
{
	entity_t	*e;
	int			start, end;
	tree_t		*tree;
	bspbrush_t	*list;
	Vector		mins, maxs;

	e = &entities[entity_num];

	start = e->firstbrush;
	end = start + e->numbrushes;

	mins[0] = mins[1] = mins[2] = MIN_COORD_INTEGER;
	maxs[0] = maxs[1] = maxs[2] = MAX_COORD_INTEGER;
	list = MakeBspBrushList (start, end, mins, maxs, FULL_DETAIL);

	if (!nocsg)
		list = ChopBrushes (list);
	tree = BrushBSP (list, mins, maxs);
	MakeTreePortals (tree);
	MarkVisibleSides (tree, start, end, FULL_DETAIL);
	MakeFaces (tree->headnode);

	FixTjuncs( tree->headnode, NULL );
	WriteBSP( tree->headnode, NULL );
	FreeTree (tree);
}


//-----------------------------------------------------------------------------
// Returns true if the entity is a func_occluder
//-----------------------------------------------------------------------------
bool IsFuncOccluder( int entity_num )
{
	entity_t *mapent = &entities[entity_num];
	const char *pClassName = ValueForKey( mapent, "classname" );
	return (strcmp("func_occluder", pClassName) == 0);
}


//-----------------------------------------------------------------------------
// Computes the area of a brush's occluders
//-----------------------------------------------------------------------------
float ComputeOccluderBrushArea( mapbrush_t *pBrush )
{
	float flArea = 0.0f;
	for ( int j = 0; j < pBrush->numsides; ++j )
	{
		side_t *pSide = &(pBrush->original_sides[j]);

		// Skip nodraw surfaces
		if ( texinfo[pSide->texinfo].flags & SURF_NODRAW )
			continue;

		if ( !pSide->winding )
			continue;

		flArea += WindingArea( pSide->winding );
	}

	return flArea;
}


//-----------------------------------------------------------------------------
// Clips all occluder brushes against each other
//-----------------------------------------------------------------------------
static tree_t *ClipOccluderBrushes( )
{
	// Create a list of all occluder brushes in the level
	CUtlVector< mapbrush_t * > mapBrushes( 1024, 1024 );
	for ( entity_num=0; entity_num < num_entities; ++entity_num )
	{
		if (!IsFuncOccluder(entity_num))
			continue;

		entity_t *e = &entities[entity_num];
		int end = e->firstbrush + e->numbrushes;
		int i;
		for ( i = e->firstbrush; i < end; ++i )
		{
			mapBrushes.AddToTail( &mapbrushes[i] );
		}
	}

	int nBrushCount = mapBrushes.Count();
	if ( nBrushCount == 0 )
		return NULL;

	Vector mins, maxs;
	mins[0] = mins[1] = mins[2] = MIN_COORD_INTEGER;
	maxs[0] = maxs[1] = maxs[2] = MAX_COORD_INTEGER;

	bspbrush_t *list = MakeBspBrushList( mapBrushes.Base(), nBrushCount, mins, maxs );

	if (!nocsg)
		list = ChopBrushes (list);
	tree_t *tree = BrushBSP (list, mins, maxs);
	MakeTreePortals (tree);
	MarkVisibleSides (tree, mapBrushes.Base(), nBrushCount);
	MakeFaces( tree->headnode );

	// NOTE: This will output the occluder face vertices + planes
	FixTjuncs( tree->headnode, NULL );

	return tree;
}


//-----------------------------------------------------------------------------
// Generate a list of unique sides in the occluder tree
//-----------------------------------------------------------------------------
static void GenerateOccluderSideList( int nEntity, CUtlVector<side_t*> &occluderSides )
{
	entity_t *e = &entities[nEntity];
	int end = e->firstbrush + e->numbrushes;
	int i, j;
	for ( i = e->firstbrush; i < end; ++i )
	{
		mapbrush_t *mb = &mapbrushes[i];
		for ( j = 0; j < mb->numsides; ++j )
		{
			occluderSides.AddToTail( &(mb->original_sides[j]) );
		}
	}
}


//-----------------------------------------------------------------------------
// Generate a list of unique faces in the occluder tree
//-----------------------------------------------------------------------------
static void GenerateOccluderFaceList( node_t *pOccluderNode, CUtlVector<face_t*> &occluderFaces )
{
	if (pOccluderNode->planenum == PLANENUM_LEAF)
		return;

	for ( face_t *f=pOccluderNode->faces ; f ; f = f->next )
	{
		occluderFaces.AddToTail( f );
	}

	GenerateOccluderFaceList( pOccluderNode->children[0], occluderFaces );
	GenerateOccluderFaceList( pOccluderNode->children[1], occluderFaces );
}


//-----------------------------------------------------------------------------
// Emits occluder brushes
//-----------------------------------------------------------------------------
static void EmitOccluderBrushes()
{
	char str[64];

	g_OccluderData.RemoveAll();
	g_OccluderPolyData.RemoveAll();
	g_OccluderVertexIndices.RemoveAll();

	tree_t *pOccluderTree = ClipOccluderBrushes();
	if (!pOccluderTree)
		return;

	CUtlVector<face_t*> faceList( 1024, 1024 );
	CUtlVector<side_t*> sideList( 1024, 1024 );
	GenerateOccluderFaceList( pOccluderTree->headnode, faceList );

#ifdef _DEBUG
	int *pEmitted = (int*)stackalloc( faceList.Count() * sizeof(int) );
	memset( pEmitted, 0, faceList.Count() * sizeof(int) );
#endif

	for ( entity_num=1; entity_num < num_entities; ++entity_num )
	{
		if (!IsFuncOccluder(entity_num))
			continue;

		// Output only those parts of the occluder tree which are a part of the brush
		int nOccluder = g_OccluderData.AddToTail();
		doccluderdata_t &occluderData = g_OccluderData[ nOccluder ];
		occluderData.firstpoly = g_OccluderPolyData.Count();
		occluderData.mins.Init( FLT_MAX, FLT_MAX, FLT_MAX );
		occluderData.maxs.Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
		occluderData.flags = 0;

		sprintf (str, "%i", nOccluder);
		SetKeyValue (&entities[entity_num], "occludernumber", str);

		sideList.RemoveAll();
		GenerateOccluderSideList( entity_num, sideList );
		for ( int i = faceList.Count(); --i >= 0; )
		{
			// Skip nodraw surfaces
			face_t *f = faceList[i];
			if ( texinfo[f->texinfo].flags & SURF_NODRAW )
				continue;

			// Only emit faces that appear in the side list of the occluder
			for ( int j = sideList.Count(); --j >= 0; )
			{
				if ( sideList[j] != f->originalface )
					continue;

				if ( f->numpoints < 3 )
					continue;

				// not a final face
				Assert ( !f->merged && !f->split[0] && !f->split[1] );

#ifdef _DEBUG
				Assert( !pEmitted[i] );
				pEmitted[i] = entity_num;
#endif

				int k = g_OccluderPolyData.AddToTail();
				doccluderpolydata_t *pOccluderPoly = &g_OccluderPolyData[k];

				pOccluderPoly->planenum = f->planenum;
				pOccluderPoly->vertexcount = f->numpoints;
				pOccluderPoly->firstvertexindex = g_OccluderVertexIndices.Count();
				for( k = 0; k < f->numpoints; ++k )
				{
					g_OccluderVertexIndices.AddToTail( f->vertexnums[k] );

					const Vector &p = dvertexes[f->vertexnums[k]].point; 
					VectorMin( occluderData.mins, p, occluderData.mins );
					VectorMax( occluderData.maxs, p, occluderData.maxs );
				}

				break;
			}
		}

		occluderData.polycount = g_OccluderPolyData.Count() - occluderData.firstpoly;

		// Mark this brush as not having brush geometry so it won't be re-emitted with a brush model
		entities[entity_num].numbrushes = 0;
	}

	FreeTree( pOccluderTree );
}


/*
============
ProcessModels
============
*/
void ProcessModels (void)
{
	BeginBSPFile ();

	// emit the displacement surfaces
	EmitInitialDispInfos();

	// Clip occluder brushes against each other
	EmitOccluderBrushes( );

	for ( entity_num=0; entity_num < num_entities; ++entity_num )
	{
		if (!entities[entity_num].numbrushes)
			continue;

		qprintf ("############### model %i ###############\n", nummodels);

		BeginModel ();

		if (entity_num == 0)
		{
			ProcessWorldModel();
		}
		else
		{
			ProcessSubModel( );
		}

		EndModel ();

		if (!verboseentities)
			verbose = false;	// don't bother printing submodels
	}

	// Turn the skybox into a cubemap in case we don't build env_cubemap textures.
	Cubemap_CreateDefaultCubemaps();
	Cubemap_MakeDefaultVersionsOfEnvCubemapMaterials();
	EndBSPFile ();
}


void LoadPhysicsDLL( void )
{
	char dirname[512];
	strcpy( dirname, basegamedir );
	StripLastDir( dirname );

	strcat( dirname, "VPHYSICS.DLL" );
	PhysicsDLLPath( dirname );
}

/*
============
main
============
*/
int main (int argc, char **argv)
{
	int		i;
	double		start, end;
	char		path[1024];

	CommandLine()->CreateCmdLine( argc, argv );
	MathLib_Init( 2.2f, 2.2f, 0.0f, 1, false, false, false, false );
	InstallSpewFunction();

	Msg( "Valve Software - vbsp.exe (%s)\n", __DATE__ );

	for (i=1 ; i<argc ; i++)
	{
		if (!strcmp(argv[i],"-threads"))
		{
			numthreads = atoi (argv[i+1]);
			i++;
		}
		else if (!strcmp(argv[i],"-glview"))
		{
			glview = true;
		}
		else if (!strcmp(argv[i], "-v"))
		{
			Msg("verbose = true\n");
			verbose = true;
		}
		else if (!strcmp(argv[i], "-draw"))
		{
			Msg ("drawflag = true\n");
			drawflag = true;
		}
		else if (!strcmp(argv[i], "-noweld"))
		{
			Msg ("noweld = true\n");
			noweld = true;
		}
		else if (!strcmp(argv[i], "-nocsg"))
		{
			Msg ("nocsg = true\n");
			nocsg = true;
		}
		else if (!strcmp(argv[i], "-noshare"))
		{
			Msg ("noshare = true\n");
			noshare = true;
		}
		else if (!strcmp(argv[i], "-notjunc"))
		{
			Msg ("notjunc = true\n");
			notjunc = true;
		}
		else if (!strcmp(argv[i], "-nowater"))
		{
			Msg ("nowater = true\n");
			nowater = true;
		}
		else if (!strcmp(argv[i], "-noopt"))
		{
			Msg ("noopt = true\n");
			noopt = true;
		}
		else if (!strcmp(argv[i], "-noprune"))
		{
			Msg ("noprune = true\n");
			noprune = true;
		}
		else if (!strcmp(argv[i], "-nofill"))
		{
			Msg ("nofill = true\n");
			nofill = true;
		}
		else if (!strcmp(argv[i], "-nomerge"))
		{
			Msg ("nomerge = true\n");
			nomerge = true;
		}
		else if (!strcmp(argv[i], "-nosubdiv"))
		{
			Msg ("nosubdiv = true\n");
			nosubdiv = true;
		}
		else if (!strcmp(argv[i], "-nodetail"))
		{
			Msg ("nodetail = true\n");
			nodetail = true;
		}
		else if (!strcmp(argv[i], "-fulldetail"))
		{
			Msg ("fulldetail = true\n");
			fulldetail = true;
		}
		else if (!strcmp(argv[i], "-onlyents"))
		{
			Msg ("onlyents = true\n");
			onlyents = true;
		}
		else if (!strcmp(argv[i], "-onlyprops"))
		{
			Msg ("onlyprops = true\n");
			onlyprops = true;
		}
		else if (!strcmp(argv[i], "-micro"))
		{
			microvolume = atof(argv[i+1]);
			Msg ("microvolume = %f\n", microvolume);
			i++;
		}
		else if (!strcmp(argv[i], "-leaktest"))
		{
			Msg ("leaktest = true\n");
			leaktest = true;
		}
		else if (!strcmp(argv[i], "-verboseentities"))
		{
			Msg ("verboseentities = true\n");
			verboseentities = true;
		}
		else if (!strcmp(argv[i], "-snapaxial"))
		{
			Msg ("snap axial = true\n");
			g_snapAxialPlanes = true;
		}
#if 0
		else if (!strcmp(argv[i], "-maxlightmapdim"))
		{
			g_maxLightmapDimension = atof(argv[i+1]);
			Msg ("g_maxLightmapDimension = %f\n", g_maxLightmapDimension);
			i++;
		}
#endif
		else if (!strcmp(argv[i], "-block"))
		{
			block_xl = block_xh = atoi(argv[i+1]);
			block_yl = block_yh = atoi(argv[i+2]);
			Msg ("block: %i,%i\n", block_xl, block_yl);
			i+=2;
		}
		else if (!strcmp(argv[i], "-blocks"))
		{
			block_xl = atoi(argv[i+1]);
			block_yl = atoi(argv[i+2]);
			block_xh = atoi(argv[i+3]);
			block_yh = atoi(argv[i+4]);
			Msg ("blocks: %i,%i to %i,%i\n", 
				block_xl, block_yl, block_xh, block_yh);
			i+=4;
		}
		else if ( !strcmp( argv[i], "-dumpcollide" ) )
		{
			Msg("Dumping collision models to collideXXX.txt\n" );
			dumpcollide = true;
		}
		else if ( !strcmp( argv[i], "-dumpstaticprop" ) )
		{
			Msg("Dumping static props to staticpropXXX.txt\n" );
			g_DumpStaticProps = true;
		}
		else if ( !strcmp( argv[i], "-forcedetailcuts" ) )
		{
			Msg("Force detail brushes to cut each other\n" );
			nodetailcuts = false;
		}
		else if (!strcmp (argv[i],"-tmpout"))
		{
			strcpy (outbase, "/tmp");
		}
#if 0
		else if( !strcmp( argv[i], "-defaultluxelsize" ) )
		{
			g_defaultLuxelSize = atof( argv[i+1] );
			i++;
		}
#endif
		else if( !strcmp( argv[i], "-luxelscale" ) )
		{
			g_luxelScale = atof( argv[i+1] );
			i++;
		}
		else if( !strcmp( argv[i], "-dxlevel" ) )
		{
			g_nDXLevel = atoi( argv[i+1] );
			Msg( "DXLevel = %d\n", g_nDXLevel );
			i++;
		}
		else if( !strcmp( argv[i], "-bumpall" ) )
		{
			g_BumpAll = true;
		}
		else if( !strcmp( argv[i], "-matpath" ) )
		{
			strcpy(qproject, argv[i+1]);
			++i;
		}
		else if( !stricmp( argv[i], "-low" ) )
		{
			g_bLowPriority = true;
		}
		else if( !stricmp( argv[i], "-lightifmissing" ) )
		{
			g_bLightIfMissing = true;
		}
		else if (argv[i][0] == '-')
			Error ("Unknown option \"%s\"", argv[i]);
		else
			break;
	}

	if (i != argc - 1)
		Error ("usage: vbsp [-threads -matpath -glview -v -draw -noweld -nocsg -noshare\n"
				"             -notjunc -nowater -noopt -noprune -nofill -nomerge -nosubdiv\n"
				"             -nodetail -fulldetail -onlyents -onlyprops -micro -leaktest\n"
				"             -verboseentities -snapaxial -block -blocks -dumpstaticprop -dumpcollide\n"
				"             -forcedetailcuts -luxelscale -tmpout -bumpall -lowpriority\n"
				"             -lightifmissing]\n"
				"             mapfile\n");

	start = I_FloatTime ();

	strcpy (source, ExpandArg (argv[i]));
	StripExtension (source);

	// Run in the background?
	if( g_bLowPriority )
	{
		SetLowPriority();
	}

	if( g_nDXLevel < 80 )
	{
		g_BumpAll = false;
	}

	if( g_luxelScale == 1.0f )
	{
		if( g_nDXLevel == 60 )
		{
			g_luxelScale = 4.0f;
		}
	}

	ThreadSetDefault ();
	numthreads = 1;		// multiple threads aren't helping...
	
	CmdLib_InitFileSystem( argv[i], true );

	// Setup the logfile.
	char logFile[512];
	_snprintf( logFile, sizeof(logFile), "%s.log", source );
	SetSpewFunctionLogFile( logFile );

	LoadPhysicsDLL();
	LoadSurfaceProperties();

#if 0
	Msg( "qdir: %s  This is the the path of the initial source file \n", qdir );
	Msg( "gamedir: %s This is the base engine + mod-specific game dir (e.g. d:/tf2/mytfmod/) \n", gamedir );
	Msg( "basegamedir: %s This is the base engine + base game directory (e.g. e:/hl2/hl2/, or d:/tf2/tf2/ )\n", basegamedir );
#endif

	sprintf( materialPath, "%smaterials", gamedir );
	InitMaterialSystem( materialPath, CmdLib_GetFileSystemFactory() );
	Msg( "materialPath: %s\n", materialPath );
	
	LoadEmitDetailObjectDictionary( gamedir );

	ExtractFileBase( source, mapbase, sizeof( mapbase ) );
	strlwr( mapbase );

	// delete portal and line files
	sprintf (path, "%s.prt", source);
	remove (path);
	sprintf (path, "%s.lin", source);
	remove (path);

	strcpy (name, ExpandArg (argv[i]));	
	DefaultExtension (name, ".vmf");

	char platformBSPFileName[1024];
	GetPlatformMapPath( source, platformBSPFileName, g_nDXLevel, 1024 );
	
	//
	// if onlyents, just grab the entites and resave
	//
	if (onlyents)
	{
		LoadBSPFile (platformBSPFileName);
		num_entities = 0;

		LoadMapFile (name);
		SetModelNumbers ();
		SetLightStyles ();

		// NOTE: If we ever precompute lighting for static props in
		// vrad, EmitStaticProps should be removed here

		// Emit static props found in the .vmf file
		EmitStaticProps();

		// NOTE: Don't deal with detail props here, it blows away lighting

		// Recompute the skybox
		ComputeBoundsNoSkybox();

		// Doing this here because stuff abov may filter out entities
		UnparseEntities ();

		WriteBSPFile (platformBSPFileName);
	}
	else if (onlyprops)
	{
		// In the only props case, deal with static + detail props only
		LoadBSPFile (platformBSPFileName);

		LoadMapFile (name);
		SetModelNumbers ();
		SetLightStyles ();

		// Emit static props found in the .vmf file
		EmitStaticProps();

		// Place detail props found in .vmf and based on material properties
		EmitDetailObjects();

		WriteBSPFile (platformBSPFileName);
	}
	else
	{
		//
		// start from scratch
		//
		LoadMapFile (name);
		if( g_nDXLevel >= 70 )
		{
			Cubemap_FixupBrushSidesMaterials();
		}
		SetModelNumbers ();
		SetLightStyles ();

		ProcessModels ();
	}

	end = I_FloatTime ();
	Msg( "%5.0f seconds elapsed\n", end-start );

	CmdLib_Cleanup();
	return 0;
}

