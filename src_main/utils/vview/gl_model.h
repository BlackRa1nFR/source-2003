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

#ifndef GL_MODEL_H
#define GL_MODEL_H
#pragma once

typedef struct msurface_s msurface_t;
typedef struct mnode_s mnode_t;
typedef struct mleaf_s mleaf_t;
typedef struct mtexinfo_s mtexinfo_t;
typedef struct mtexdata_s mtexdata_t;

#ifndef BSPFILE_H
#include "bspfile.h"
#endif

#ifndef QFILES_H
#include "qfiles.h"
#endif

#ifndef CMODEL_H
#include "cmodel.h"
#endif

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	Vector		position;
} mvertex_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	unsigned short	v[2];
//	unsigned int	cachededgeoffset;
} medge_t;

typedef struct image_s
{
	char		name[TEXTURE_NAME_LENGTH];		// texture name
//	imagetype_t	type;
	int			width, height;					// source image
	int			upload_width, upload_height;	// after power of two and picmip
//	int			view_width, view_height;		//
	int			registration_sequence;			// 0 = free
	msurface_t	*texturechain;					// for sort-by-texture world drawing
	int			gl_texturenum;					// gl texture binding
	
//	qboolean	has_alpha;
//	qboolean paletted;

	Vector		reflectivity;
} image_t;

typedef struct mtexinfo_s
{
	float		textureVecsTexelsPerWorldUnits[2][4];	// [s/t] unit vectors in world space. 
							                                // [i][3] is the s/t offset relative to the origin.
	float		lightmapVecsLuxelsPerWorldUnits[2][4];
	int			flags;
	int			numframes;
	image_t		*image;
} mtexinfo_t;

typedef struct mtexdata_s
{
	char		name[TEXTURE_NAME_LENGTH];		// texture name
#if 0
	int			width, height;					// source image
	int			view_width, view_height;		//
	float		reflectivity[3];
#endif
} mtexdata_t;

typedef struct mnode_s
{
// common with leaf
	int			contents;		// -1, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	
	Vector		mins;
	Vector		maxs;		// for bounding box culling

	mnode_t		*parent;

// node specific
	cplane_t	*plane;
	mnode_t		*children[2];	

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
} mnode_t;



typedef struct mleaf_s
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	Vector		mins;
	Vector		maxs;		// for bounding box culling

	mnode_t		*parent;

// leaf specific
	int			cluster;
	int			area;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
} mleaf_t;


// drawing surface flags
#define SURFDRAW_NOLIGHT		0x01		// no lightmap
#define	SURFDRAW_NODE			0x02		// This surface is on a node
#define	SURFDRAW_SKY			0x04		// portal to sky
#define SURFDRAW_TURB			0x10		// warped water effect
#define SURFDRAW_FOG			0x40		// draw as fog
#define SURFDRAW_UNDERWATER		0x80		// underwater, draw warped if view not in water
#define SURFDRAW_SCROLL			0x100		// scroll s axis by rendercolor (HACK)
#define SURFDRAW_NODRAW			0x200		// don't draw this surface, not really visible
#define SURFDRAW_TRANS			0x400		// sort this surface from back to front
#define SURFDRAW_PLANEBACK		0x800		// faces away from plane of the node that stores this face


typedef struct msurface_s
{
	int			visframe;		// should be drawn when node is crossed

	cplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges
	
	short		textureMins[2];		// smallest unnormalized s/t position on the surface.
	short		textureExtents[2];	// ?? s/t texture size, 1..512 for all non-sky surfaces

	short		lightmapMins[2];
	short		lightmapExtents[2];

	int			light_s, light_t;	// gl lightmap coordinates
	int			dlight_s, dlight_t; // gl lightmap coordinates for dynamic lightmaps
#if 0
	glpoly_t	*polys;				// multiple if warped
#endif
	msurface_t	*texturechain;
	msurface_t	*lightmapchain;
	mtexinfo_t	*texinfo;

// lighting info
	int			dlightframe;
	int			dlightbits;

	int			lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];
	float		cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	colorRGBExp32		*samples;		// [numstyles*surfsize]
} msurface_t;

typedef struct mportal_s
{
	unsigned short	*vertList;
	int				numportalverts;
	cplane_t		*plane;
	unsigned short	cluster[2];
	int				visframe;
} mportal_t;

typedef struct mcluster_s
{
	unsigned short	*portalList;
	int				numportals;
} mcluster_t;

typedef struct
{
	Vector		mins, maxs;
	Vector		origin;		// for sounds or lights
	float		radius;
	int			headnode;
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
} mmodel_t;

typedef enum {mod_bad, mod_brush, mod_sprite, mod_studio } modtype_t;

typedef struct model_s model_t;
struct model_s
{
	char		name[MAX_QPATH];

	int			registration_sequence;
	modtype_t	type;
	int			numframes;
	
	int			flags;

//
// volume occupied by the model graphics
//		
	Vector		mins, maxs;
	float		radius;

//
// solid volume for clipping 
//
	qboolean	clipbox;
	Vector		clipmins, clipmaxs;

//
// brush model
//
	int			firstmodelsurface, nummodelsurfaces;
	int			lightmap;		// only for submodels

	int			numsubmodels;
	mmodel_t	*submodels;

	int			numplanes;
	cplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	int			firstnode;
	mnode_t		*nodes;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numtexdata;
	mtexdata_t	*texdata;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	dvis_t		*vis;

	colorRGBExp32		*lightdata;
	char		*entities;

	// for alias models and skins
//	image_t		*skins[MAX_MD2SKINS];

	int			numworldlights;
	dworldlight_t *worldlights;

	int			numportals;
	mportal_t	*portals;

	int			numclusters;
	mcluster_t	*clusters;

	int			numportalverts;
	unsigned short *portalverts;

	int			numclusterportals;
	unsigned short *clusterportals;

	int			extradatasize;
	void		*extradata;
};

extern void Mod_LoadMap( void *buffer );
extern void R_SetupVis( Vector& origin, model_t *worldmodel );
extern int r_oldviewcluster, r_viewcluster, r_oldviewcluster2, r_viewcluster2, r_visframecount;

extern model_t *r_worldmodel;

#endif // GL_MODEL_H
