#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "bsprender.h"
#include "qfiles.h"
#include "gl_model.h"
#include "cmodel_engine.h"
#include "engine.h"
#include "gl_image.h"
#include "gl_rsurf.h"
#include "cmdlib.h"

extern "C" void SetQdirFromPath( const char *path );


void R_RecursiveWorldNode( mnode_t *node );
void BspRenderSurfaceFlat( model_t *pModel, msurface_t *pSurface );
void BspRenderSurfaceTextured( model_t *pModel, msurface_t *pSurface );
void BspRenderSurfaceWire( model_t *pModel, msurface_t *pSurface );

void BspRenderPortals( void );
qboolean R_CullBox( Vector& mins, Vector& maxs );


int BspLoad( const char *pFileName )
{
	dheader_t	*header;
	FILE		*fp;
	char		tmpName[512];
	unsigned int checksum;


	if ( !pFileName )
	{
		Error( "need a BSP to load!\n");
		return false;
	}
	SetQdirFromPath( pFileName );

	SetExtension( tmpName, pFileName, ".bsp" );
	fp = fopen( tmpName, "rb" );
	if ( !fp )
	{
		Error( "no BSP" );
		return false;
	}

	fseek( fp, 0, SEEK_END );
	int size = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	char *buffer = new char[size];
	header = (dheader_t *)buffer;
	fread( buffer, size, 1, fp );

	if (header->ident != IDBSPHEADER)
	{
		Error ("%s is not a IBSP file", tmpName );
		return false;
	}
	if (header->version != BSPVERSION)
	{
		Error ("%s is version %i, not %i", tmpName, header->version, BSPVERSION);
		return false;
	}

	fclose( fp );

	Mod_LoadMap( buffer );
	delete[] buffer;
	CM_LoadMap( tmpName, false, &checksum );

	return true;
}


static byte g_Flatcolors[256*3];

static void (*RenderSurface)( model_t *pModel, msurface_t *pSurface ) = BspRenderSurfaceTextured;


void BspToggleTexture( void )
{
	if ( RenderSurface == BspRenderSurfaceTextured )
	{
		RenderSurface = BspRenderSurfaceFlat;
	}
	else
	{
		if ( RenderSurface == BspRenderSurfaceWire )
			RenderSurface = BspRenderSurfaceTextured;
		else
			RenderSurface = BspRenderSurfaceWire;
	}
}


void BspRenderSurfaceFlat( model_t *pModel, msurface_t *pSurface )
{
	byte *pColor = g_Flatcolors + ( (((int)pSurface>>4)&255) * 3 );

	glDisable( GL_TEXTURE_2D );
	glColor3ub( pColor[0], pColor[1], pColor[2] );

	int i, v;
	int *pEdge = pModel->surfedges + pSurface->firstedge;

	glBegin( GL_POLYGON );
	for ( i = 0; i < pSurface->numedges; i++ )
	{
		int edge = *pEdge++;
		if ( edge < 0 )
		{
			v = 1;
			edge = -edge;
		}
		else
		{
			v = 0;
		}
		Vector& pVert = pModel->vertexes[ pModel->edges[edge].v[v] ].position;
		glVertex3f( pVert[0], pVert[1], pVert[2] );
	}
	glEnd();
	glEnable( GL_TEXTURE_2D );
}



void BspRenderSurfaceWire( model_t *pModel, msurface_t *pSurface )
{
	BspRenderSurfaceFlat( pModel, pSurface );

	glDisable( GL_TEXTURE_2D );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glColor3ub( 255, 255, 255 );

	int i, v;
	int *pEdge = pModel->surfedges + pSurface->firstedge;
	Vector *pVert0 = 0, *pVert1 = 0, *pVert2 = NULL;

	glEnable( GL_POLYGON_OFFSET_LINE );
	glPolygonOffset( -1.0, 1.0 );
	glDepthMask( 0 );
	glBegin( GL_TRIANGLES );
	for ( i = 0; i < pSurface->numedges; i++ )
	{
		int edge = *pEdge++;
		if ( edge < 0 )
		{
			v = 1;
			edge = -edge;
		}
		else
		{
			v = 0;
		}
		pVert1 = pVert2;
		pVert2 = &pModel->vertexes[ pModel->edges[edge].v[v] ].position;
		if ( i > 1 )
		{
			glVertex3fv( pVert0->Base() );
			glVertex3fv( pVert1->Base() );
			glVertex3fv( pVert2->Base() );
		}
		else if ( i == 0 )
		{
			pVert0 = pVert2;
		}
	}
	glEnd();
	glDepthMask( 1 );
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_POLYGON_OFFSET_LINE );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

void BspRenderSurfaceTextured( model_t *pModel, msurface_t *pSurface )
{
	int i, v;
	int *pEdge = pModel->surfedges + pSurface->firstedge;

	GL_Bind( pSurface->texinfo->image->gl_texturenum );
	glColor3ub( 255, 255, 255 );
	glBegin( GL_POLYGON );
	for ( i = 0; i < pSurface->numedges; i++ )
	{
		int edge = *pEdge++;
		float s, t;

		if ( edge < 0 )
		{
			v = 1;
			edge = -edge;
		}
		else
		{
			v = 0;
		}
		Vector& pVert = pModel->vertexes[ pModel->edges[edge].v[v] ].position;

		s = DotProduct( pVert.Base(), pSurface->texinfo->textureVecsTexelsPerWorldUnits[0] ) + 
			pSurface->texinfo->textureVecsTexelsPerWorldUnits[0][3];
		s /= pSurface->texinfo->image->width;

		t = DotProduct( pVert.Base(), pSurface->texinfo->textureVecsTexelsPerWorldUnits[1]) + 
			pSurface->texinfo->textureVecsTexelsPerWorldUnits[1][3];
		t /= pSurface->texinfo->image->height;
		glTexCoord2f( s, t );
		glVertex3f( pVert[0], pVert[1], pVert[2] );
	}
	glEnd();
}


static int g_RenderPortals = 0, g_RenderLightmaps = 1, g_RenderBmodels = 1;
static int r_framecount = 0;
Vector modelorg;
int g_PolyCount;
void BspRender( Vector& cameraPosition )
{
	static int firsttime = 1;
	glPushMatrix();

	int i;

	r_framecount++;
	g_PolyCount = 0;
	R_SetupVis( cameraPosition, r_worldmodel );
	if ( firsttime )
	{
		firsttime = 0;

		for (i = 0; i < 256 * 3; i += 3) 
		{
			do 
			{
				g_Flatcolors[i] = rand()&255;
				g_Flatcolors[i+1] = rand()&255;
				g_Flatcolors[i+2] = rand()&255;
			} while (g_Flatcolors[i] + g_Flatcolors[i+1] + g_Flatcolors[i+2] < 256);
		}
		g_Flatcolors[0] = 0;
		g_Flatcolors[1] = 0xFF;
		g_Flatcolors[2] = 0xFF;
		g_Flatcolors[3] = 0;
		g_Flatcolors[4] = 0xFF;
		g_Flatcolors[5] = 0xFF;
		g_Flatcolors[6] = 0xFF;
		g_Flatcolors[7] = 0;
	}

	VectorCopy( cameraPosition, modelorg );

#if 0
	for ( i = 0; i < r_worldmodel->numsurfaces; i++ )
	{
		BspRenderSurfaceFlat( r_worldmodel, r_worldmodel->surfaces + i );
	}
	return;
#endif

	R_RecursiveWorldNode( r_worldmodel->nodes );

	if ( g_RenderLightmaps )
	{
		LM_DrawSurfaces();
	}

	glPopMatrix();
	if ( g_RenderPortals )
	{
		BspRenderPortals();
	}

	if ( g_RenderBmodels )
	{
		for ( i = 1; i < r_worldmodel->numsubmodels; i++ )
		{
			extern model_t	mod_inline[];

			model_t *pModel = mod_inline + i;

			if ( !R_CullBox( pModel->mins, pModel->maxs ) )
			{
				msurface_t *pSurface;
				pSurface = pModel->surfaces + pModel->firstmodelsurface;

				for ( int s = 0; s < pModel->nummodelsurfaces; s++, pSurface++ )
				{
					g_PolyCount++;
					RenderSurface( pModel, pSurface );
					if ( g_RenderLightmaps )
					{
						LM_AddSurface( pSurface );
					}
				}
			}
			if ( g_RenderLightmaps )
			{
				LM_DrawSurfaces();
			}
		}
	}
}


void BspRenderPortals( void )
{
	int i;

	mportal_t *p = r_worldmodel->portals;
	glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	glDisable( GL_TEXTURE_2D );
	glDepthMask( 0 );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	
	// the p++ has to be in the for construct because of the continue below
	for ( i = 0; i < r_worldmodel->numportals; i++, p++ )
	{
		if ( p->visframe == r_visframecount )
		{
			int j;

			if ( p->cluster[0] == r_viewcluster || p->cluster[1] == r_viewcluster )
			{
				glColor3ub( 255, 0, 0 );
			}
			else
			{
				glColor3ub( 255, 255, 255 );
				if ( g_RenderPortals == 2 )	// this mode skips all portals outside the current cluster
					continue;
			}

			glBegin( GL_POLYGON );
			for ( j = 0; j < p->numportalverts; j++ )
			{
				glVertex3fv( r_worldmodel->vertexes[ p->vertList[j] ].position.Base() );
			}
			glEnd();
		}
	}
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glDepthMask( 1 );
}
void BspTogglePortals( void )
{
	g_RenderPortals = (g_RenderPortals+1)%3;
}


void BspToggleVis( void )
{
	gl_lockpvs->value = !gl_lockpvs->value;
}

void BspToggleLight( void )
{
	g_RenderLightmaps = !g_RenderLightmaps;
}

void BspToggleBmodels( void )
{
	g_RenderBmodels = !g_RenderBmodels;
}

void BspFree( void )
{
}



// JAY: Setup frustum
/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (Vector& mins, Vector& maxs)
{
#if 0
	int		i;

	if (r_nocull->value)
		return false;

	for (i=0 ; i<4 ; i++)
		if ( BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
#else
	return false;
#endif
}


void R_DrawLeaf( mleaf_t *pleaf )
{
	msurface_t	*surf;

	for ( int i = 0; i < pleaf->nummarksurfaces; i++ )
	{
		surf = pleaf->firstmarksurface[i];
		if ( surf->visframe != r_framecount )
		{
			if ( surf->flags & SURFDRAW_NODRAW )
				continue;

			surf->visframe = r_framecount;
			g_PolyCount++;

			if (surf->flags & SURFDRAW_SKY)
			{	// just adds to visible sky bounds
				RenderSurface( r_worldmodel, surf );
	//			R_AddSkySurface (surf);
			}
			else if (surf->texinfo->flags & SURF_TRANS )
			{	// add to the translucent chain
	//			surf->texturechain = r_alpha_surfaces;
	//			r_alpha_surfaces = surf;
			}
			else
			{
				RenderSurface( r_worldmodel, surf );
				if ( g_RenderLightmaps )
				{
					LM_AddSurface( surf );
				}
			}
		}
	}
}


/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode( mnode_t *node )
{
	int			side;
	cplane_t	*plane;
	float		dot;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;
	if (R_CullBox (node->mins, node->maxs))
		return;

// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		R_DrawLeaf( (mleaf_t *)node );
		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	if ( plane->type <= PLANE_Z )
		dot = modelorg[plane->type] - plane->dist;
	else
		dot = DotProduct (modelorg, plane->normal) - plane->dist;

	if (dot >= 0)
		side = 0;
	else
		side = 1;

// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side]);

	// recurse down the back side
	R_RecursiveWorldNode (node->children[!side]);
}
