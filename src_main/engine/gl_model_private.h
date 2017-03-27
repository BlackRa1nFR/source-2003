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
// $NoKeywords: $
//=============================================================================

#ifndef GL_MODEL_PRIVATE_H
#define GL_MODEL_PRIVATE_H

#ifdef _WIN32
#pragma once
#endif

#include "vector4d.h"
#include "tier0/dbg.h"
#include "disp_leaflink.h"
#include "idispinfo.h"
#include "shadowmgr.h"
#include "vcollide.h"
#include "studio.h"
#include "qlimits.h"
#include "cache.h"
#include "host.h"
#include "cmodel.h"
#include "bspfile.h"
#include "Overlay.h"
#include "datamap.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

struct studiomeshdata_t;
struct decal_t;
struct msurface1_t;
struct msurface2_t;
struct msurfacelighting_t;
typedef decal_t *pDecal_t;
class ITexture;
class CEngineSprite;


// !!! if this is changed, it must be changed in asm_draw.h too !!!
struct mvertex_t
{
	Vector		position;
};

// !!! if this is changed, it must be changed in asm_draw.h too !!!
struct medge_t
{
	unsigned short	v[2];
//	unsigned int	cachededgeoffset;
};

class IMaterial;
class IMaterialVar;

struct mtexinfo_t
{
	Vector4D	textureVecsTexelsPerWorldUnits[2];	// [s/t] unit vectors in world space. 
							                        // [i][3] is the s/t offset relative to the origin.
	Vector4D	lightmapVecsLuxelsPerWorldUnits[2];
	float		luxelsPerWorldUnit;
	float		worldUnitsPerLuxel;
	int			flags;
	IMaterial	*material;

	mtexinfo_t( mtexinfo_t const& src )
	{
		// copy constructor needed since Vector4D has no copy constructor
		memcpy( this, &src, sizeof(mtexinfo_t) );
	}
};

struct mnode_t
{
	short		area;			// If all leaves below this node are in the same area, then
								// this is the area index. If not, this is -1.

// common with leaf
	int			contents;		// <0 to differentiate from leafs
								// -1 means check the node for visibility
								// -2 means don't check the node for visibility

	int			visframe;		// node needs to be traversed if current
	
	Vector		m_vecCenter;
	Vector		m_vecHalfDiagonal;

	mnode_t		*parent;

// node specific
	cplane_t	*plane;
	mnode_t		*children[2];	

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
};


struct mleaf_t
{
public:

	CDispIterator	GetDispIterator() { return CDispIterator( m_pDisplacements, CDispLeafLink::LIST_LEAF ); }

public:
	short		area;

// common with node
	int			contents;		// contents mask
	int			visframe;		// node needs to be traversed if current

	Vector		m_vecCenter;
	Vector		m_vecHalfDiagonal;

	mnode_t		*parent;

// leaf specific
	int			cluster;

	int			firstmarksurface;
	int			nummarksurfaces;

	// Head of a doubly linked list of displacements that touch the leaf.
	// These are a little nasty to iterate, so use CDispIterator to do so like this:
	//
	// for( CLeafDispIterator it = pLeaf->GetDispIterator(); it.IsValid(); )
	// { CDispLeafLink *pCur = it.Inc();   ...   }
	CDispLeafLink	*m_pDisplacements;

	int			leafWaterDataID;
	int			m_nReserved;
};


struct mleafwaterdata_t
{
	float	surfaceZ;
	float	minZ;
	short	surfaceTexInfoID;
};


struct mcubemapsample_t
{
	Vector origin;
	ITexture *pTexture;
	unsigned char size; // default (mat_envmaptgasize) if 0, 1<<(size-1) otherwise.
};


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


struct mmodel_t
{
	Vector		mins, maxs;
	Vector		origin;		// for sounds or lights
	float		radius;
	int			headnode;
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
};

struct mprimitive_t
{
	int	type;
	unsigned short	firstIndex;
	unsigned short	indexCount;
	unsigned short	firstVert;
	unsigned short	vertCount;
};

struct mprimvert_t
{
	Vector		pos;
	float		texCoord[2];
	float		lightCoord[2];
};

#include "model_types.h"

#define MODELFLAG_MATERIALPROXY	(1<<0)	// we've got a material proxy
#define MODELFLAG_TRANSLUCENT	(1<<1)	// we've got a translucent model
#define MODELFLAG_VERTEXLIT		(1<<2)	// we've got a vertex-lit model
#define MODELFLAG_TRANSLUCENT_TWOPASS	(1<<3)	// render opaque part in opaque pass

// only models with type "mod_brush" have this data
struct brushdata_t
{
	int			firstmodelsurface, nummodelsurfaces;

	int			numsubmodels;
	mmodel_t	*submodels;

	int			numplanes;
	cplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numleafwaterdata;
	mleafwaterdata_t *leafwaterdata;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numoccluders;
	doccluderdata_t *occluders;

	int			numoccluderpolys;
	doccluderpolydata_t *occluderpolys;

	int			numoccludervertindices;
	int			*occludervertindices;

	int				numvertnormalindices;	// These index vertnormals.
	unsigned short	*vertnormalindices;

	int			numvertnormals;
	Vector		*vertnormals;

	int			numnodes;
	int			firstnode;
	mnode_t		*nodes;
	unsigned short *m_LeafMinDistToWater;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numtexdata;
	csurface_t	*texdata;

    int         numDispInfos;
    HDISPINFOARRAY	hDispInfos;	// Use DispInfo_Index to get IDispInfos..

/*
    int         numOrigSurfaces;
    msurface_t  *pOrigSurfaces;
*/

	int			numsurfaces;
	msurface1_t	*surfaces1;
	msurface2_t	*surfaces2;
	msurfacelighting_t *surfacelighting;

	int			numvertindices;
	unsigned short *vertindices;

	int			nummarksurfaces;
	int			*marksurfaces; // garymctoptimize: this could an array of shorts?

	colorRGBExp32		*lightdata;

	int			numworldlights;
	dworldlight_t *worldlights;

	// non-polygon primitives (strips and lists)
	int			numprimitives;
	mprimitive_t *primitives;

	int			numprimverts;
	mprimvert_t *primverts;

	int			numprimindices;
	unsigned short *primindices;

	int				m_nAreas;
	darea_t			*m_pAreas;

	int				m_nAreaPortals;
	dareaportal_t	*m_pAreaPortals;

	int			m_nClipPortalVerts;
	Vector		*m_pClipPortalVerts;

	mcubemapsample_t  *m_pCubemapSamples;
	int				   m_nCubemapSamples;
	
#if 0
	int			numportals;
	mportal_t	*portals;

	int			numclusters;
	mcluster_t	*clusters;

	int			numportalverts;
	unsigned short *portalverts;

	int			numclusterportals;
	unsigned short *clusterportals;
#endif
};


// only models with type "mod_studio" have this data
struct studiodata_t
{
	// the vphysics.dll collision model
	vcollide_t	vcollisionData;

	studiohwdata_t hardwareData;
	
	// true if the studio mesh data has been loaded
	bool		studiomeshLoaded;

	// true if the collision data has been loaded
	bool		vcollisionLoaded;
};


// only models with type "mod_sprite" have this data
struct spritedata_t
{
	int			numframes;
	CEngineSprite		*sprite;
};

struct model_t
{
	char		name[MAX_QPATH];

	// UNDONE: Choose between these two methods
	//int			registration_sequence;
	int			needload;		// mark loaded/not loaded

	modtype_t	type;
	int			flags;

//
// volume occupied by the model graphics
//		
	Vector		mins, maxs;
	float		radius;

	int			extradatasize;
	cache_user_t	cache;

	union
	{
		brushdata_t		brush;
		studiodata_t	studio;
		spritedata_t	sprite;
	};
};


//-----------------------------------------------------------------------------
// Decals
//-----------------------------------------------------------------------------

struct decallist_t
{
	DECLARE_SIMPLE_DATADESC();

	Vector		position;
	char		name[ 128 ];
	short		entityIndex;
	byte		depth;
	byte		flags;

	// This is the surface plane that we hit so that we can move certain decals across
	//  transitions if they hit similar geometry
	Vector		impactPlaneNormal;
};

#define	MAXLIGHTMAPS	4

// drawing surface flags
#define SURFDRAW_NOLIGHT		0x0001		// no lightmap
#define	SURFDRAW_NODE			0x0002		// This surface is on a node
#define	SURFDRAW_SKY			0x0004		// portal to sky
#define SURFDRAW_BUMPLIGHT		0x0008		// Has multiple lightmaps for bump-mapping
#define SURFDRAW_NODRAW			0x0010		// don't draw this surface, not really visible
#define SURFDRAW_TRANS			0x0020		// sort this surface from back to front
#define SURFDRAW_PLANEBACK		0x0040		// faces away from plane of the node that stores this face
#define SURFDRAW_DYNAMIC		0x0080		// Don't use a static buffer for this face
#define SURFDRAW_TANGENTSPACE	0x0100		// This surface needs a tangent space
#define SURFDRAW_NOCULL			0x0200		// Don't bother backface culling these

// Every surface is either:
//		1) a water surface (SURFDRAW_WATERSURFACE)
//		2) above water (SURFDRAW_ABOVEWATER)
//		3) below water (SURFDRAW_UNDERWATER)
// If a surface is a water surface, then it can't be either under or above water.
// A surface can be any combination of SURFDRAW_ABOVEWATER and SURFDRAW_UNDERWATER
#define SURFDRAW_ABOVEWATER		0x0400		// At least partially above water
#define SURFDRAW_UNDERWATER		0x0800		// At least partially underwater
#define SURFDRAW_WATERSURFACE	0x1000		// water surface
#define SURFDRAW_HASLIGHTSYTLES 0x2000		// has a lightstyle other than 0
#define SURFDRAW_HAS_DISP		0x4000		// has a dispinfo

// old fully general volumetric fog. .  unused.
//#define SURFDRAW_FOG			0x0040		// draw as fog

// UNDONE: Every vert has a normal now
//#define NO_SURF_VERTNORMALS			0xFFFF

struct msurface1_t
{
	int			firstvertindex;		// look up in model->vertindices[]

	// garymct - are these needed? - used by decal projection code
	short		textureMins[2];		// smallest unnormalized s/t position on the surface.
	short		textureExtents[2];	// ?? s/t texture size, 1..512 for all non-sky surfaces

	mtexinfo_t	*texinfo;

	unsigned short numPrims;
	unsigned short firstPrimID;			// index into primitive list if numPrims > 0

	unsigned short	firstvertnormal;

	// FIXME: Should I just point to the leaf here since it has this data?????????????
	short fogVolumeID;			// -1 if not in fog  
    IDispInfo   *pDispInfo;         // displacement map information
};

// This is a single cache line (32 bytes)
struct msurfacelighting_t
{
	// You read that minus sign right. See the comment below.
	colorRGBExp32 *AvgLightColor( int nLightStyleIndex ) { return m_pSamples - (nLightStyleIndex + 1); }

	// Lightmap info
	short m_pLightmapMins[2];
	short m_pLightmapExtents[2];
	short m_pOffsetIntoLightmapPage[2];

	int m_nLastComputedFrame;	// last frame the surface's lightmap was recomputed
	int m_fDLightBits;			// Indicates which dlights illuminates this surface.
	int m_nDLightFrame;			// Indicates the last frame in which dlights illuminated this surface

	unsigned char m_nStyles[MAXLIGHTMAPS];	// index into d_lightstylevalue[] for animated lights 
											// no one surface can be effected by more than 4 
											// animated lights.

	// NOTE: This is tricky. To get this to fit in a single cache line,
	// and to save the extra memory of not having to store average light colors for
	// lightstyles that are not used, I store between 0 and 4 average light colors +before+
	// the samples, depending on how many lightstyles there are. Naturally, accessing
	// an average color for an undefined lightstyle on the surface results in undefined results.

	// 0->4 avg light colors, *in reverse order from m_nStyles* + [numstyles*surfsize]
	colorRGBExp32 *m_pSamples;
};


#pragma pack(1)
struct msurface2_t
{
	int			flags;			// see SURFDRAW_ #defines
	int			visframe;		// should be drawn when node is crossed
	cplane_t	*plane;			// pointer to shared plane	
	int			texturechain; // surfID
	decal_t		*pdecals;
	int			vertCount;
	short		materialSortID;
	unsigned short	vertBufferIndex;
	ShadowDecalHandle_t	m_ShadowDecals; // unsigned short
	OverlayFragmentHandle_t m_nFirstOverlayFragment;	// First overlay fragment on the surface
};
#pragma pack()

#define ASSERT_SURF_VALID(surfID) Assert( ((surfID) >= 0) && ((surfID) < host_state.worldmodel->brush.numsurfaces) )
#define IS_SURF_VALID(surfID) ( surfID != -1 )

inline int& MSurf_Flags( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces2[surfID].flags;
}

inline bool SurfaceHasDispInfo( int surfID, model_t *pModel = host_state.worldmodel )
{
	return ( MSurf_Flags( surfID, pModel ) & SURFDRAW_HAS_DISP ) ? true : false;
}

inline int& MSurf_VisFrame( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces2[surfID].visframe;
}


/*
inline int& MSurf_DLightFrame( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfacelighting[surfID].dlightframe;
}
*/

inline int& MSurf_DLightBits( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfacelighting[surfID].m_fDLightBits;
}

inline cplane_t& MSurf_Plane( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return *pModel->brush.surfaces2[surfID].plane;
}

inline int& MSurf_FirstVertIndex( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces1[surfID].firstvertindex;
}

inline int& MSurf_VertCount( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces2[surfID].vertCount;
}

inline short *MSurf_TextureMins( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces1[surfID].textureMins;
}

inline short *MSurf_TextureExtents( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces1[surfID].textureExtents;
}

inline short *MSurf_LightmapMins( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfacelighting[surfID].m_pLightmapMins;
}

inline short *MSurf_LightmapExtents( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfacelighting[surfID].m_pLightmapExtents;
}

inline short MSurf_MaxLightmapSizeWithBorder( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	return SurfaceHasDispInfo( surfID, pModel ) ? MAX_DISP_LIGHTMAP_DIM_INCLUDING_BORDER : MAX_BRUSH_LIGHTMAP_DIM_INCLUDING_BORDER;
}

inline short MSurf_MaxLightmapSizeWithoutBorder( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	return SurfaceHasDispInfo( surfID, pModel ) ? MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER : MAX_BRUSH_LIGHTMAP_DIM_WITHOUT_BORDER;
}

inline int& MSurf_TextureChain( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces2[surfID].texturechain;
}

inline mtexinfo_t *MSurf_TexInfo( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces1[surfID].texinfo;
}


inline pDecal_t& MSurf_Decals( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces2[surfID].pdecals;
}

inline ShadowDecalHandle_t& MSurf_ShadowDecals( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces2[surfID].m_ShadowDecals;
}


inline colorRGBExp32 *MSurf_AvgLightColor( int surfID, int nIndex, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfacelighting[surfID].AvgLightColor(nIndex);
}

inline byte *MSurf_Styles( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfacelighting[surfID].m_nStyles;
}

/*
inline int *MSurf_CachedLight( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfacelighting[surfID].cached_light;
}

inline short& MSurf_CachedDLight( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfacelighting[surfID].cached_dlight;
}
*/

inline unsigned short& MSurf_NumPrims( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces1[surfID].numPrims;
}

inline unsigned short& MSurf_FirstPrimID( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces1[surfID].firstPrimID;
}

inline colorRGBExp32 *MSurf_Samples( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfacelighting[surfID].m_pSamples;
}


inline IDispInfo *MSurf_DispInfo( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces1[surfID].pDispInfo;
}

inline unsigned short &MSurf_FirstVertNormal( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces1[surfID].firstvertnormal;
}

inline unsigned short &MSurf_VertBufferIndex( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces2[surfID].vertBufferIndex;
}

inline short &MSurf_FogVolumeID( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces1[surfID].fogVolumeID;
}

inline short& MSurf_MaterialSortID( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces2[surfID].materialSortID;
}

inline short *MSurf_OffsetIntoLightmapPage( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfacelighting[surfID].m_pOffsetIntoLightmapPage;
}

inline VPlane MSurf_GetForwardFacingPlane( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return VPlane( MSurf_Plane( surfID).normal, MSurf_Plane( surfID ).dist );
}


inline OverlayFragmentHandle_t &MSurf_OverlayFragmentList( int surfID, model_t *pModel = host_state.worldmodel )
{
//	ASSERT_SURF_VALID( surfID );
	Assert( pModel );
	return pModel->brush.surfaces2[surfID].m_nFirstOverlayFragment;
}

inline msurfacelighting_t *SurfaceLighting( int surfID, model_t *pModel = host_state.worldmodel )
{
	Assert( pModel );
	return &pModel->brush.surfacelighting[surfID];
}

#endif // GL_MODEL_PRIVATE_H
