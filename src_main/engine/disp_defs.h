//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DISPINFO_DEFS_H
#define DISPINFO_DEFS_H
#ifdef _WIN32
#pragma once
#endif


#include "utlvector.h"
#include "materialsystem/imesh.h"
#include "convar.h"
#include "bspfile.h"
#include "decal_private.h"
#include "mempool.h"
#include "decal_clip.h"
#include "bitvec.h"


extern ConVar				r_DrawDisp;


class CDispInfo;
class CDispGroup;

#define MAX_STATIC_BUFFER_VERTS		(8*1024)
#define MAX_STATIC_BUFFER_INDICES	(8*1024)

#define MAX_DISP_DECALS		32

#define MAX_EXTRA_DEPENDENCIES	8
#define MAX_RETESSELATE		MAX_MAP_DISPINFO	// Max number of displacements that can be retesselated per frame.

#define MAX_TOTAL_DISP_DEPENDENCIES	(4*2 + 4*MAX_DISP_CORNER_NEIGHBORS + MAX_EXTRA_DEPENDENCIES)



// These hold the HDISPINFOARRAYs.
class CDispArray
{
public:
	unsigned short	m_CurTag;

	CDispInfo		*m_pDispInfos;
	int				m_nDispInfos;
};



// These classes store groups of meshes which can be drawn with one IMesh::Draw() call.
class CGroupMesh
{
public:
	IMesh					*m_pMesh;
	CUtlVector<CDispInfo*>	m_DispInfos;

	// This list is updated each frame to point at the visible LODs.
	CUtlVector<CDispInfo*>	m_VisibleDisps;
	CUtlVector<CPrimList>	m_Visible;
	int						m_nVisible;

	CDispGroup				*m_pGroup;
};


class CDispGroup
{
public:

	int						m_LightmapPageID;
	IMaterial				*m_pMaterial;
	CUtlVector<CGroupMesh*>	m_Meshes;
	CUtlVector<int>			m_DispInfos;
	int						m_nVisible;
};


// This represents the two neighboring verts that are used to calculate the
// error when removing a side vert of a node.
class CSideVertCorners
{
public:
	CFourVerts		m_Corners[2];
};


class CDispDecalBase
{
public:
	CDispDecalBase( int flags ) : m_Flags(flags) {}

	enum 
	{ 
		NODE_BITFIELD_COMPUTED = 0x1,
		DECAL_SHADOW = 0x2,
		NO_INTERSECTION = 0x4,
		FRAGMENTS_COMPUTED = 0x8,	// *non-shadow* fragments
	};

	// Precalculated flags on the nodes telling which nodes this decal can intersect.
	// See CPowerInfo::m_NodeIndexIncrements for a description of how the node tree is
	// walked using this bit vector.
	CBitVec<85>	m_NodeIntersect;	// The number of nodes on a 17x17 is 85.
									// Note: this must be larger if MAX_MAP_DISP_POWER gets larger.

	// Number of triangles + verts that got generated for this decal.
	unsigned char	m_Flags;
	unsigned short	m_nVerts;
	unsigned short	m_nTris;
};

//-----------------------------------------------------------------------------
// Types associated with normal decals
//-----------------------------------------------------------------------------

typedef unsigned short DispDecalFragmentHandle_t;

enum
{
	DISP_DECAL_FRAGMENT_HANDLE_INVALID = (DispDecalFragmentHandle_t)~0
};

class CDispDecal : public CDispDecalBase
{
public:
	CDispDecal() : CDispDecalBase(0) {}

	decal_t		*m_pDecal;
	float		m_DecalWorldScale[2];
	Vector		m_TextureSpaceBasis[3];
	DispDecalFragmentHandle_t	m_FirstFragment;
};

class CDispDecalFragment
{
public:
	enum				{ MAX_VERTS = 8 };

	decal_t				*m_pDecal;
	int					m_nVerts;
	CDecalVert			m_pVerts[MAX_VERTS];
};


typedef unsigned short DispShadowFragmentHandle_t;

enum
{
	DISP_SHADOW_FRAGMENT_HANDLE_INVALID = (DispShadowFragmentHandle_t)~0
};

class CDispShadowDecal : public CDispDecalBase
{
public:	
	CDispShadowDecal() : CDispDecalBase(DECAL_SHADOW) {}

	ShadowHandle_t	m_Shadow;
	DispShadowFragmentHandle_t	m_FirstFragment;
};


class CDispShadowFragment
{
public:
	// NOTE: This # is >8 because we have 6 clip planes, it overflowed..
	enum				{ MAX_VERTS = 12 };

	int	m_nVerts;
	Vector	m_Verts[MAX_VERTS];
	Vector	m_TCoords[MAX_VERTS];
};



class CDispRenderVert
{
public:
	Vector			m_vPos;
	Vector			m_vOriginalPos;	// position at map load time - used to limit the terrain mods

	Vector2D		m_vTexCoord;
	Vector2D		m_LMCoords;
	float			m_BumpSTexCoordOffset;

	// These are necessary for mat_normals to work
	// But since only mat_normals needs them, I figure I'll only add this
	// in debug builds for now, since it's a lot of memory
	Vector			m_vNormal;
	Vector			m_vSVector;
	Vector			m_vTVector;
};



// --------------------------------------------------------------------------------- //
// Global tables.
// --------------------------------------------------------------------------------- //

extern int					g_CoreDispNeighborOrientationMap[4][4];


// --------------------------------------------------------------------------------- //
// Global variables.
// --------------------------------------------------------------------------------- //
extern float						g_flDispToleranceSqr;
extern CUtlVector<unsigned char>	g_DispLMAlpha;
extern CUtlVector<unsigned char>	g_DispLightmapSamplePositions;
extern CUtlVector<CDispGroup*>		g_DispGroups;


// CVars.
extern ConVar				r_DispUpdateAll;
extern ConVar				r_DispFullRadius;
extern ConVar				r_DispRadius;
extern ConVar				r_DispTolerance;
extern ConVar				r_DispLockLOD;
extern ConVar				r_DispEnableLOD;
extern ConVar				r_DispSetLOD;
extern ConVar				r_DispWalkable;
extern ConVar				r_DispBuildable;
extern ConVar				r_DispUseStaticMeshes;

extern CMemoryPool			g_DecalFragmentPool;

// If this is true, then no backface removal is done.
extern bool					g_bDispOrthoRender;

#endif // DISPINFO_DEFS_H
