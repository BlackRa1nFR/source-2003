//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Shared util code between client and server.
//
//=============================================================================

#ifndef UTIL_SHARED_H
#define UTIL_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"
#include "cmodel.h"
#include "utlvector.h"
#include "networkvar.h"
#include "engine/IEngineTrace.h"
#include "engine/IStaticPropMgr.h"
#include "shared_classnames.h"

#ifdef CLIENT_DLL
#include "cdll_client_int.h"
#endif


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CGameTrace;
typedef CGameTrace trace_t;


//-----------------------------------------------------------------------------
// Pitch + yaw
//-----------------------------------------------------------------------------
float		UTIL_VecToYaw			(const Vector &vec);
float		UTIL_VecToPitch			(const Vector &vec);
float		UTIL_VecToYaw			(const matrix3x4_t& matrix, const Vector &vec);
float		UTIL_VecToPitch			(const matrix3x4_t& matrix, const Vector &vec);
Vector		UTIL_YawToVector		( float yaw );


//-----------------------------------------------------------------------------
// Standard collision filters...
//-----------------------------------------------------------------------------
bool PassServerEntityFilter( const IHandleEntity *pTouch, const IHandleEntity *pPass );
bool StandardFilterRules( IHandleEntity *pServerEntity, int fContentsMask );


//-----------------------------------------------------------------------------
// Converts an IHandleEntity to an CBaseEntity
//-----------------------------------------------------------------------------
inline const CBaseEntity *EntityFromEntityHandle( const IHandleEntity *pConstHandleEntity )
{
	IHandleEntity *pHandleEntity = const_cast<IHandleEntity*>(pConstHandleEntity);

#ifdef CLIENT_DLL
	IClientUnknown *pUnk = (IClientUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#else
	if ( staticpropmgr->IsStaticProp( pHandleEntity ) )
		return NULL;

	IServerNetworkable *pServerNetworkable = static_cast<IServerNetworkable*>(pHandleEntity);
	return pServerNetworkable->GetBaseEntity();
#endif
}

inline CBaseEntity *EntityFromEntityHandle( IHandleEntity *pHandleEntity )
{
#ifdef CLIENT_DLL
	IClientUnknown *pUnk = (IClientUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#else
	if ( staticpropmgr->IsStaticProp( pHandleEntity ) )
		return NULL;

	IServerNetworkable *pServerNetworkable = static_cast<IServerNetworkable*>(pHandleEntity);
	return pServerNetworkable->GetBaseEntity();
#endif
}


//-----------------------------------------------------------------------------
// traceline methods
//-----------------------------------------------------------------------------
class CTraceFilterSimple : public CTraceFilter
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterSimple );
	
	CTraceFilterSimple( const IHandleEntity *passentity, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );

private:
	const IHandleEntity *m_pPassEnt;
	int m_collisionGroup;
};

class CTraceFilterSimpleList : public CTraceFilterSimple
{
public:
	CTraceFilterSimpleList( int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );

	void	AddEntityToIgnore( IHandleEntity *pEntity );
private:
	CUtlVector<IHandleEntity*>	m_PassEntities;
};


inline void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 const IHandleEntity *ignore, int collisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );
	CTraceFilterSimple traceFilter( ignore, collisionGroup );
	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
}

inline void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 ITraceFilter *pFilter, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );
	enginetrace->TraceRay( ray, mask, pFilter, ptr );
}

inline void UTIL_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const IHandleEntity *ignore, 
					 int collisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );
	CTraceFilterSimple traceFilter( ignore, collisionGroup );
	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
}

inline void UTIL_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, ITraceFilter *pFilter, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );
	enginetrace->TraceRay( ray, mask, pFilter, ptr );
}

inline void UTIL_TraceRay( const Ray_t &ray, unsigned int mask, 
						  const IHandleEntity *ignore, int collisionGroup, trace_t *ptr )
{
	CTraceFilterSimple traceFilter( ignore, collisionGroup );
	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
}

// Sweeps a particular entity through the world
void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, unsigned int mask, trace_t *ptr );
void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, ITraceFilter *pFilter, trace_t *ptr );
void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, const IHandleEntity *ignore, int collisionGroup, trace_t *ptr );

inline int UTIL_PointContents( const Vector &vec )
{
	return enginetrace->GetPointContents( vec );
}

// Sweeps against a particular model, using collision rules 
void UTIL_TraceModel( const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, 
					  const Vector &hullMax, CBaseEntity *pentModel, int collisionGroup, trace_t *ptr );



#endif // UTIL_SHARED_H
