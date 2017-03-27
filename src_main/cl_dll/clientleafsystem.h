//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//
//=============================================================================

#if !defined( CLIENTLEAFSYSTEM_H )
#define CLIENTLEAFSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "IGameSystem.h"
#include "engine/IClientLeafSystem.h"
#include "cdll_int.h"


//-----------------------------------------------------------------------------
// Foward declarations
//-----------------------------------------------------------------------------
struct WorldListInfo_t;
class IClientRenderable;
class Vector;
class CGameTrace;
typedef CGameTrace trace_t;
struct Ray_t;
class Vector2D;
class CStaticProp;


//-----------------------------------------------------------------------------
// Handle to an renderable in the client leaf system
//-----------------------------------------------------------------------------
enum
{
	DETAIL_PROP_RENDER_HANDLE = (ClientRenderHandle_t)0xfffe
};


class CRenderList
{
public:
	enum
	{
		MAX_GROUP_ENTITIES = 4096
	};

	struct CEntry
	{
		IClientRenderable	*m_pRenderable;
		unsigned short		m_iWorldListInfoLeaf; // NOTE: this indexes WorldListInfo_t's leaf list.
		unsigned short		m_TwoPass;
		ClientRenderHandle_t m_RenderHandle;
	};

	// The leaves for the entries are in the order of the leaves you call CollateRenderablesInLeaf in.
	CEntry		m_RenderGroups[RENDER_GROUP_COUNT][MAX_GROUP_ENTITIES];
	int			m_RenderGroupCounts[RENDER_GROUP_COUNT];
};


//-----------------------------------------------------------------------------
// Used by CollateRenderablesInLeaf
//-----------------------------------------------------------------------------
struct SetupRenderInfo_t
{
	CRenderList *m_pRenderList;
	Vector m_vecRenderOrigin;
	int m_nRenderFrame;
	float m_flRenderDistSq;
	bool m_bDrawDetailObjects;
};


//-----------------------------------------------------------------------------
// A handle associated with shadows managed by the client leaf system
//-----------------------------------------------------------------------------
typedef unsigned short ClientLeafShadowHandle_t;
enum
{
	CLIENT_LEAF_SHADOW_INVALID_HANDLE = (ClientLeafShadowHandle_t)~0 
};


//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
class IClientLeafShadowEnum
{
public:
	// return false to stop iterating
	// The user ID is the id passed into CreateShadow
	virtual bool EnumShadow( unsigned short userId ) = 0;
};


//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
class IClientLeafSystem : public IClientLeafSystemEngine, public IGameSystem
{
public:
	// Adds and removes renderables from the leaf lists
	virtual ClientRenderHandle_t AddRenderable( IClientRenderable* pRenderable, RenderGroup_t group ) = 0;

	// Returns true if the renderable was put into any leaves.
	virtual bool IsRenderableVisible( ClientRenderHandle_t handle ) = 0;

	// Indicates which leaves detail objects are in
	virtual void SetDetailObjectsInLeaf( int leaf, int firstDetailObject, int detailObjectCount ) = 0;
	virtual void GetDetailObjectsInLeaf( int leaf, int& firstDetailObject, int& detailObjectCount ) = 0;

	// Indicates which leaves detail objects should be rendered from, returns the detais objects in the leaf
	virtual void DrawDetailObjectsInLeaf( int leaf, int frameNumber, int& firstDetailObject, int& detailObjectCount ) = 0;

	// Should we draw detail objects in this leaf?
	virtual bool ShouldDrawDetailObjectsInLeaf( int leaf, int frameNumber ) = 0;

	// Call this when a renderable moves
	virtual void RenderableMoved( ClientRenderHandle_t handle ) = 0;

	virtual bool IsRenderableUnderWater( WorldListInfo_t& info, 
									     CRenderList::CEntry *pRenderListEntry ) = 0;
	virtual bool IsRenderableAboveWater( WorldListInfo_t& info, 
									     CRenderList::CEntry *pRenderListEntry ) = 0;

	// Call each frame for changeable entities. This detects if the entity needs
	// to be readded to the leaves.
	virtual void DetectChanges( ClientRenderHandle_t handle, RenderGroup_t group ) = 0;

	// Comptes which leaf translucent objects should be rendered in
	virtual void ComputeTranslucentRenderLeaf( int count, int *pLeafList, int *pLeafFogVolumeList, int frameNumber ) = 0;

	// Put renderables in the leaf into their appropriate lists.
	virtual void CollateRenderablesInLeaf( int leaf, int worldListLeafIndex, SetupRenderInfo_t &info ) = 0;

	// Put renderables in the leaf into their appropriate lists.
	virtual void CollateRenderablesInViewModelRenderGroup( CUtlVector< IClientRenderable * >& list ) = 0;

	// Call this to deactivate static prop rendering..
	virtual void DrawStaticProps( bool enable ) = 0;

	// Call this to deactivate small object rendering
	virtual void DrawSmallEntities( bool enable ) = 0;

	// The following methods are related to shadows...
	virtual ClientLeafShadowHandle_t AddShadow( unsigned short userId ) = 0;
	virtual void RemoveShadow( ClientLeafShadowHandle_t h ) = 0;

	// Project a shadow
	virtual void ProjectShadow( ClientLeafShadowHandle_t handle, const Vector& origin, 
				const Vector& dir, const Vector2D& size, float maxDist ) = 0;

	// Find all shadow casters in a set of leaves
	virtual void EnumerateShadowsInLeaves( int leafCount, int* pLeaves, IClientLeafShadowEnum* pEnum ) = 0;
};


//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
extern IClientLeafSystem *g_pClientLeafSystem;
inline IClientLeafSystem* ClientLeafSystem()
{
	return g_pClientLeafSystem;
}


#endif	// CLIENTLEAFSYSTEM_H


