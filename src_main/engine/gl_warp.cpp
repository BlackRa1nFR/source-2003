// gl_warp.c -- sky and water polygons

#include "glquake.h"
#include "gl_water.h"
#include "zone.h"
#include "gl_model_private.h"
#include "gl_texture.h"
#include "gl_matsysiface.h"
#include "utlvector.h"
#include "materialsystem/IMesh.h"

#include "tier0/vprof.h"

#define	SUBDIVIDE_SIZE	64

static ConVar	r_drawskybox(  "r_drawskybox", "1" );
static ConVar	r_drawfullskybox( "r_drawfullskybox", "1" );

void BoundPoly (int numverts, Vector *verts, Vector& mins, Vector& maxs)
{
	int		i, j;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	for (i=0 ; i<numverts ; i++)
		for (j=0 ; j<3 ; j++)
		{
			if (verts[i][j] < mins[j])
				mins[j] = verts[i][j];
			if (verts[i][j] > maxs[j])
				maxs[j] = verts[i][j];
		}
}

//=========================================================

/*
==================
R_LoadSkys
==================
*/
extern ConVar	mat_loadtextures;
static IMaterial		*skyboxMaterials[6] = { NULL, NULL, NULL, NULL, NULL, NULL };

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void R_UnloadSkys( void )
{
	int i;

	for ( i = 0; i < 6; i++ )
	{
		if( skyboxMaterials[i] )
		{
			GL_UnloadMaterial( skyboxMaterials[i] );
			skyboxMaterials[i] = NULL;
		}
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool R_LoadNamedSkys( const char *skyname )
{
	char		name[ MAX_OSPATH ];
	bool		found;
	IMaterial	*skies[ 6 ];
	bool		success = true;
	char		*skyboxsuffix[ 6 ] = { "rt", "bk", "lf", "ft", "up", "dn" };

	for ( int i = 0; i < 6; i++ )
	{
		Q_snprintf (name, sizeof( name ), "skybox/%s%s", skyname, skyboxsuffix[i] );

		skies[i] = materialSystemInterface->FindMaterial( name, &found );		
		if( found )
		{
			continue;
		}

		success = false;
		break;
	}

	if ( !success )
	{
		return false;
	}

	// Increment references
	for ( i = 0; i < 6; i++ )
	{
		// Unload any old skybox
		if ( skyboxMaterials[ i ] )
		{
			skyboxMaterials[ i ]->DecrementReferenceCount();
			skyboxMaterials[ i ] = NULL;
		}
	
		// Use the new one
		assert( skies[ i ] );
		skyboxMaterials[i] = skies[ i ];
		skyboxMaterials[i]->IncrementReferenceCount();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void R_LoadSkys( void )
{
	bool success = true;

	char requestedsky[ 128 ];

	ConVar *skyname = ( ConVar * )cv->FindVar( "sv_skyname" );
	if ( skyname )
	{
		strcpy( requestedsky, skyname->GetString() );
	}
	else
	{
		Con_DPrintf( "Unable to find skyname ConVar!!!\n" );
		return;
	}

	// See if user's sky will work
	if ( !R_LoadNamedSkys( requestedsky ) )
	{
		// Assume failure
		success = false;

		// See if user requested other than the default
		if ( stricmp( requestedsky, "sky_urb01" ) )
		{
			// Try the default
			skyname->SetValue( "sky_urb01" );

			// See if we could load that one now
			if ( R_LoadNamedSkys( skyname->GetString() ) )
			{
				Con_DPrintf( "Unable to load sky %s, but successfully loaded %s\n", requestedsky, skyname->GetString() );
				success = true;
			}
		}
	}

	if ( !success )
	{
		Con_DPrintf( "Unable to load sky %s\n", requestedsky );
	}
}

Vector	skyclip[6] = 
{
	Vector(1,1,0),
	Vector(1,-1,0),
	Vector(0,-1,1),
	Vector(0,1,1),
	Vector(1,0,1),
	Vector(-1,0,1) 
};
int	c_sky;

// 1 = s, 2 = t, 3 = 2048
int	st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},		// 0 degrees yaw, look straight up
	{2,-1,-3}		// look straight down

//	{-1,2,3},
//	{1,2,-3}
};

// s = [0]/[2], t = [1]/[2]
int	vec_to_st[6][3] =
{
	{-2,3,1},
	{2,3,-1},

	{1,3,2},
	{-1,3,-2},

	{-2,-1,3},
	{-2,1,-3}

//	{-1,2,3},
//	{1,2,-3}
};

float	skymins[2][6], skymaxs[2][6];
// REVISIT: Do we want to spend this much CPU to save fill rate?
void DrawSkyPolygon (int nump, Vector* vecs)
{
	Vector	v, av;
	int		i,j;
	float	s, t, dv;
	int		axis;

	c_sky++;

	// decide which face it maps to
	VectorCopy (vec3_origin, v);
	for (i=0; i<nump ; i++)
	{
		v += vecs[i];
	}
	av[0] = fabsf(v[0]);
	av[1] = fabsf(v[1]);
	av[2] = fabsf(v[2]);
	if (av[0] > av[1] && av[0] > av[2])
	{
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	}
	else if (av[1] > av[2] && av[1] > av[0])
	{
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	}
	else
	{
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// project new texture coords
	for (i=0 ; i<nump ; i++)
	{
		j = vec_to_st[axis][2];
		if (j > 0)
			dv = (vecs[i])[j-1];
		else
			dv = -(vecs[i])[-j-1];

		j = vec_to_st[axis][0];
		if (j < 0)
			s = -(vecs[i])[-j-1] / dv;
		else
			s = (vecs[i])[j-1] / dv;
		j = vec_to_st[axis][1];
		if (j < 0)
			t = -(vecs[i])[-j-1] / dv;
		else
			t = (vecs[i])[j-1] / dv;

		if (s < skymins[0][axis])
			skymins[0][axis] = s;
		if (t < skymins[1][axis])
			skymins[1][axis] = t;
		if (s > skymaxs[0][axis])
			skymaxs[0][axis] = s;
		if (t > skymaxs[1][axis])
			skymaxs[1][axis] = t;
	}
}


// BUGBUG: The global optimizer nonsense is back!
#define	MAX_CLIP_VERTS	64

// NJS: Really should eliminate this pragma, or at least minimize it's impact as ClipSkyPolygon shows up 
// as a profiler spike without optimization.
#pragma optimize("g", off)
static void ClipSkyPolygon (int nump, Vector* vecs, int stage)
{
	// A static array of the vectors to be passed, 
	static  Vector newv[6][2][MAX_CLIP_VERTS];
	int		newc[2];
	bool	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	int		i, j;

	Assert( nump <= MAX_CLIP_VERTS-2 );
 
// NJS: Without global optimization on here, this ends up being a huge CPU burner on a lot of maps, 
// due to the rather braindead floating point, recursion, and branch generation code the compiler spews
// when global optimization is disabled.  (Looking at the generated code, it looks like the compiler can't
// see past each basic block without the globl optimizer)
// Once we fix this global optimizer bug we should re-evaluate this function.
#ifdef GLOBAL_OPTIMIZATION_BUG_FIXED
	if (stage == 6)
#endif
	{	
		// fully clipped, so draw it
		DrawSkyPolygon (nump, vecs);
		return;
	}

	front = back = false;
	Vector& norm = skyclip[stage];
	for (i=0; i<nump ; i++)
	{
		d = DotProduct (vecs[i], norm);
		dists[i] = d;

		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
	}

	if (!front || !back)
	{	// not clipped
		ClipSkyPolygon(nump, vecs, stage+1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	vecs[i] = vecs[0];
	newc[0] = newc[1] = 0;

	for (i = 0; i<nump ; i++)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			newv[stage][0][newc[0]] = vecs[i];
			newc[0]++;
			break;
		case SIDE_BACK:
			newv[stage][1][newc[1]] = vecs[i];
			newc[1]++;
			break;
		case SIDE_ON:
			newv[stage][0][newc[0]] = vecs[i];
			newc[0]++;
			newv[stage][1][newc[1]] = vecs[i];
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{
			e = (vecs[i])[j] + d*((vecs[i+1])[j] - (vecs[i])[j]);
			newv[stage][0][newc[0]][j] = e;
			newv[stage][1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon(newc[0], newv[stage][0], stage+1);
	ClipSkyPolygon(newc[1], newv[stage][1], stage+1);
}
#pragma optimize("", on)

void R_AddSkySurface( int surfID )
{
	Vector	verts[MAX_CLIP_VERTS];

	int count = MSurf_VertCount( surfID );
	if ( count > MAX_CLIP_VERTS )
		count = MAX_CLIP_VERTS;

	// calculate vertex values for sky box
	for ( int i = 0; i < count; i++ )
	{
		int vertIndex = host_state.worldmodel->brush.vertindices[MSurf_FirstVertIndex( surfID )+i];
		VectorSubtract( host_state.worldmodel->brush.vertexes[vertIndex].position, r_origin, verts[i] );
	}
	ClipSkyPolygon( count, verts, 0 );
}

/*
==============
R_ClearSkyBox
==============
*/
extern cplane_t	frustum[FRUSTUM_NUMPLANES];
void R_ClearSkyBox (void)
{
	int		i;

	for (i=0 ; i<6 ; i++)
	{
		skymins[0][i] = skymins[1][i] = 9999;
		skymaxs[0][i] = skymaxs[1][i] = -9999;
	}
}


#define SQRT3INV		(0.57735f)		// a little less than 1 / sqrt(3)

#pragma warning (disable : 4701)

void MakeSkyVec( float s, float t, int axis, float zFar, Vector& position, Vector2D &texCoord )
{
	Vector		v, b;
	int			j, k;
	float		width;

	width = zFar * SQRT3INV;

	if ( s < -1 )
		s = -1;
	else if ( s > 1 )
		s = 1;
	if ( t < -1 )
		t = -1;
	else if ( t > 1 )
		t = 1;

	b[0] = s*width;
	b[1] = t*width;
	b[2] = width;

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
		v[j] += r_origin[j];
	}

	// avoid bilerp seam
	s = (s+1)*0.5;
	t = (t+1)*0.5;

	if (s < 1.0/512)
		s = 1.0/512;
	else if (s > 511.0/512)
		s = 511.0/512;
	if (t < 1.0/512)
		t = 1.0/512;
	else if (t > 511.0/512)
		t = 511.0/512;

	t = 1.0 - t;
	VectorCopy( v, position );
	texCoord[0] = s;
	texCoord[1] = t;
}

#pragma warning (default : 4701)

/*
==============
R_DrawSkyBox
==============
*/
int	skytexorder[6] = {0,2,1,3,4,5};
#define SIGN(d)				((d)<0?-1:1)
static int	gFakePlaneType[6] = {1,-1,2,-2,3,-3};
void R_DrawSkyBox( float zFar, int nDrawFlags /*= 0x3F*/  )
{
	VPROF("R_DrawSkyBox");

	int		i;
	Vector	normal;

#ifdef USE_CONVARS
	if( !r_drawskybox.GetInt() || !mat_loadtextures.GetInt() )
	{
		return;
	}
#endif

	if( r_drawfullskybox.GetInt() )
	{
		for( i = 0; i < 6; i++ )
		{
			skymins[0][i] = skymins[1][i] = -1.0f;
			skymaxs[0][i] = skymaxs[1][i] = 1.0f;
		}
	}

	for (i=0 ; i<6 ; i++, nDrawFlags >>= 1 )
	{
		// Don't draw this panel of the skybox if the flag isn't set:
		if( ! (nDrawFlags & 1) )
			continue;

		if (skymins[0][i] >= skymaxs[0][i] || skymins[1][i] >= skymaxs[1][i])
			continue;

		VectorCopy( vec3_origin, normal );
		switch( gFakePlaneType[i] )
		{
		case 1:
			normal[0] = 1;
			break;

		case -1:
			normal[0] = -1;
			break;

		case 2:
			normal[1] = 1;
			break;

		case -2:
			normal[1] = -1;
			break;

		case 3:
			normal[2] = 1;
			break;

		case -3:
			normal[2] = -1;
			break;
		}
		// Hack, try to find backfacing surfaces on the inside of the cube to avoid binding their texture
		if ( DotProduct( vpn, normal ) < (-1 + 0.70710678) )
			continue;

		Vector positionArray[4];
		Vector2D texCoordArray[4];
		if (skyboxMaterials[skytexorder[i]])
		{
			materialSystemInterface->Bind( skyboxMaterials[skytexorder[i]] );

			MakeSkyVec( skymins[0][i], skymins[1][i], i, zFar, positionArray[0], texCoordArray[0] );
			MakeSkyVec( skymins[0][i], skymaxs[1][i], i, zFar, positionArray[1], texCoordArray[1] );
			MakeSkyVec( skymaxs[0][i], skymaxs[1][i], i, zFar, positionArray[2], texCoordArray[2] );
			MakeSkyVec( skymaxs[0][i], skymins[1][i], i, zFar, positionArray[3], texCoordArray[3] );

			IMesh* pMesh = materialSystemInterface->GetDynamicMesh();
			CMeshBuilder meshBuilder;
			meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

			for (int j = 0; j < 4; ++j)
			{
				meshBuilder.Position3fv( positionArray[j].Base() );
				meshBuilder.TexCoord2fv( 0, texCoordArray[j].Base() );
				meshBuilder.AdvanceVertex();
			}
		
			meshBuilder.End();
			pMesh->Draw();
		}
	}
}
