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
// This file contains a little interface to deal with static prop collisions
//
//=============================================================================

#ifndef STATICPROPMGR_H
#define STATICPROPMGR_H

#include "vector.h"
#include "engine/IStaticPropMgr.h"

// FIXME: Remove! Only here for the test against old code
#include "enginetrace.h"


//-----------------------------------------------------------------------------
// foward declarations
//-----------------------------------------------------------------------------
class ICollideable;
FORWARD_DECLARE_HANDLE( LightCacheHandle_t );


//-----------------------------------------------------------------------------
// The engine's static prop manager
//-----------------------------------------------------------------------------
class IStaticPropMgrEngine
{
public:
	virtual bool Init() = 0;
	virtual void Shutdown() = 0;

	// Call at the beginning of the level, will unserialize all static
	// props and add them to the main collision tree
	virtual void LevelInit() = 0;

	// Call this when there's a client, *after* LevelInit, and after the world entity is loaded
	virtual void LevelInitClient() = 0;

	// Call this when there's a client, *before* LevelShutdown
	virtual void LevelShutdownClient() = 0;

	// Call at the end of the level, cleans up the static props
	virtual void LevelShutdown() = 0;

	// Call this to recompute static prop lighting when necessary
	virtual void RecomputeStaticLighting() = 0;

	// Check if a static prop is in a particular PVS.
	virtual bool IsPropInPVS( IHandleEntity *pHandleEntity, const byte *pVis ) const = 0;

	// returns a collideable interface to static props
	virtual ICollideable *GetStaticProp( IHandleEntity *pHandleEntity ) = 0;

	// returns the lightcache handle associated with a static prop
	virtual LightCacheHandle_t GetLightCacheHandleForStaticProp( IHandleEntity *pHandleEntity ) = 0;

	// Is a base handle a static prop?
	virtual bool IsStaticProp( IHandleEntity *pHandleEntity ) const = 0;
	virtual bool IsStaticProp( CBaseHandle handle ) const = 0;

	// Returns the static prop index (useful for networking)
	virtual int GetStaticPropIndex( IHandleEntity *pHandleEntity ) const = 0;
};


//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IStaticPropMgrEngine* StaticPropMgr();


#endif	// STATICPROPMGR_H
