//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CMODEL_PRIVATE_H
#define CMODEL_PRIVATE_H
#pragma once

#include "assert.h"
#include "quakedef.h"
#include "bspfile.h"
#include "cmodel.h"
#include "dispcoll_common.h"
#include "utlvector.h"
#include "disp_leaflink.h"

extern Vector trace_start;
extern Vector trace_end;
extern Vector trace_mins;
extern Vector trace_maxs;
extern Vector trace_extents;

extern trace_t trace_trace;
extern trace_t g_StabTrace;

extern int trace_contents;
extern qboolean trace_ispoint;

#include "coordsize.h"

// JAYHL2: This used to be -1, but that caused lots of epsilon issues
// Under the quake2 code, traces needed to be larger than DIST_EPSILON in clip space
// (basically the distance they span each plane along it's normal) or there are problems.
// Perhaps Quake2 limited maps to a certain slope / angle on walkable ground, 
// or minimum trace length.  I can't see it in their source.
#ifndef NEVER_UPDATED
#define		NEVER_UPDATED		-99999
#endif

//=============================================================================
//
// Local CModel Structures (cmodel.cpp and cmodel_disp.cpp)
//

struct cbrushside_t
{
	cplane_t	*plane;
	csurface_t	*surface;
	int			bBevel;							// is the side a bevel plane?
};

typedef struct cbrush_s
{
	int				contents;
	int				numsides;
	int				firstbrushside;
	int				checkcount[2];			// to avoid repeated testings
	struct cbrush_s	*next;
} cbrush_t;

struct cleaf_t
{
	int			    contents;
	int			    cluster;
	int			    area;
	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
	CDispLeafLink	*m_pDisplacements;
};

struct carea_t
{
	int		numareaportals;
	int		firstareaportal;
	int		floodnum;							// if two areas have equal floodnums, they are connected
	int		floodvalid;
};


struct cnode_t
{
	cplane_t	*plane;
	int			children[2];		// negative numbers are leafs
};


// global collision checkcount
void BeginCheckCount();
int CurrentCheckCount();
int CurrentCheckCountDepth();
void EndCheckCount();


//-----------------------------------------------------------------------------
// We keep running into overflow errors here. This is to avoid that problem
//-----------------------------------------------------------------------------
template <class T>
class CRangeValidatedArray
{
public:
	void Attach( int nCount, T* pData );
	void Detach();

	T &operator[]( int i );
	const T &operator[]( int i ) const;

	T *Base();

private:
	T *m_pArray;

#ifdef _DEBUG
	int m_nCount;
#endif
};

template <class T>
T &CRangeValidatedArray<T>::operator[]( int i )
{
	Assert( (i >= 0) && (i < m_nCount) );
	return m_pArray[i];
}

template <class T>
const T &CRangeValidatedArray<T>::operator[]( int i ) const
{
	Assert( (i >= 0) && (i < m_nCount) );
	return m_pArray[i];
}

template <class T>
void CRangeValidatedArray<T>::Attach( int nCount, T* pData )
{
	m_pArray = pData;

#ifdef _DEBUG
	m_nCount = nCount;
#endif
}

template <class T>
void CRangeValidatedArray<T>::Detach()
{
	m_pArray = NULL;

#ifdef _DEBUG
	m_nCount = 0;
#endif
}

template <class T>
T *CRangeValidatedArray<T>::Base()
{
	return m_pArray;
}


//=============================================================================
//
// Collision BSP Data Class
//
class CCollisionBSPData
{
public:
	// This is sort of a hack, but it was a little too painful to do this any other way
	// The goal of this dude is to allow us to override the tree with some
	// other tree (or a subtree)
	cnode_t*					map_rootnode;

	char						map_name[MAX_QPATH];

	int									numbrushsides;
	CRangeValidatedArray<cbrushside_t>	map_brushsides;
	int									numplanes;
	CRangeValidatedArray<cplane_t>		map_planes;
	int									numnodes;
	CRangeValidatedArray<cnode_t>		map_nodes;
	int									numleafs;				// allow leaf funcs to be called without a map
	CRangeValidatedArray<cleaf_t>		map_leafs;
	int									emptyleaf, solidleaf;
	int									numleafbrushes;
	CRangeValidatedArray<unsigned short>	map_leafbrushes;
	int									numcmodels;
	CRangeValidatedArray<cmodel_t>		map_cmodels;
	int									numbrushes;
	CRangeValidatedArray<cbrush_t>		map_brushes;
	
	// this points to the whole block of memory for vis data, but it is used to
	// reference the header at the top of the block.
	int									numvisibility;
	dvis_t*								map_vis;

	int									numentitychars;
	CRangeValidatedArray<char>			map_entitystring;
	int									numareas;
	CRangeValidatedArray<carea_t>		map_areas;
	int									numareaportals;
	CRangeValidatedArray<dareaportal_t>	map_areaportals;
	int									numclusters;
	char								*map_nullname;
	int									numtextures;
	char*								map_texturenames;
	CRangeValidatedArray<csurface_t>	map_surfaces;
	int									floodvalid;
	int									numportalopen;
	CRangeValidatedArray<qboolean>		portalopen;
};



//=============================================================================
//
// physics collision
//
class IPhysicsSurfaceProps;
class IPhysicsCollision;

extern IPhysicsSurfaceProps	*physprop;
extern IPhysicsCollision	*physcollision;

//=============================================================================
//
// This might eventually become a class/interface when we want multiple instances
// etc.......for now....
//
extern csurface_t nullsurface;

extern bool bStartSolidDisp;

bool CollisionBSPData_Init( CCollisionBSPData *pBSPData );
void CollisionBSPData_Destroy( CCollisionBSPData *pBSPData );
void CollisionBSPData_LinkPhysics( void );

void CollisionBSPData_PreLoad( CCollisionBSPData *pBSPData );
bool  CollisionBSPData_Load( const char *pName, CCollisionBSPData *pBSPData );
void CollisionBSPData_PostLoad( void );

//-----------------------------------------------------------------------------
// Returns the collision tree associated with the ith displacement
//-----------------------------------------------------------------------------

CDispCollTree* CollisionBSPData_GetCollisionTree( int i );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline CCollisionBSPData *GetCollisionBSPData( void /*int ndxBSP*/ )
{
	extern CCollisionBSPData g_BSPData;								// the global collision bsp
	return &g_BSPData;
}

//=============================================================================
//
// Collision Model Counts
//
class CCollisionCounts
{
public:
	int		m_PointContents;
	int		m_Traces;
	int		m_BrushTraces;
	int		m_DispTraces;
	int		m_Stabs;
};

void CollisionCounts_Init( CCollisionCounts *pCounts );

extern CCollisionCounts g_CollisionCounts;


//=============================================================================
//
// Older Code That Should Live Here!!!!
// a shared file should contain all the CDispCollTree stuff
//

//=============================================================================
//
// Displacement Collision Functions and Data
//
extern Vector trace_StabDir;		// the direction to stab in
extern int trace_bDispHit;			// hit displacement surface last

extern int g_DispCollTreeCount;
extern CDispCollTree *g_pDispCollTrees;

extern csurface_t dispSurf;

// memory allocation/de-allocation
CDispCollTree **DispCollTrees_AllocLeafList( int size );
void DispCollTrees_FreeLeafList( CCollisionBSPData *pBSPData );

// setup
void CM_DispTreeLeafnum( CCollisionBSPData *pBSPData );

// collision
void CM_PreStab( cleaf_t *pLeaf, Vector &vStabDir, int &contents );
void CM_Stab( CCollisionBSPData *pBSPData, Vector const &start, Vector const &vStabDir, int contents );
void CM_PostStab( void );
void CM_TestInDispTree( CCollisionBSPData *pBSPData, cleaf_t *pLeaf, Vector const &traceStart, 
				Vector const &boxMin, Vector const &boxMax, int collisionMask, trace_t *pTrace );
void CM_TraceToDispTree( CDispCollTree *pDispTree, Vector &traceStart, Vector &traceEnd,
		    			 Vector &boxMin, Vector &boxMax, float startFrac, float endFrac, trace_t *pTrace, bool bRayCast );
void CM_PostTraceToDispTree( void );

//=============================================================================
//
// profiling purposes only -- remove when done!!!
//
void FASTCALL CM_ClipBoxToBrush ( CCollisionBSPData *pBSPData, const Vector& mins, const Vector& maxs, const Vector& p1, const Vector& p2,
								  trace_t *trace, cbrush_t *brush, BOOL bDispSurf );
void CM_TestBoxInBrush ( CCollisionBSPData *pBSPData, const Vector& mins, const Vector& maxs, const Vector& p1,
					  trace_t *trace, cbrush_t *brush, BOOL bDispSurf );
void FASTCALL CM_RecursiveHullCheck ( CCollisionBSPData *pBSPData, int num, float p1f, float p2f, const Vector& p1, const Vector& p2);


#endif // CMODEL_PRIVATE_H
