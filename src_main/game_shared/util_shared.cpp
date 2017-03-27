//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "mathlib.h"
#include "util_shared.h"
#include "model_types.h"


float UTIL_VecToYaw( const Vector &vec )
{
	if (vec.y == 0 && vec.x == 0)
		return 0;
	
	float yaw = atan2( vec.y, vec.x );

	yaw = RAD2DEG(yaw);

	if (yaw < 0)
		yaw += 360;

	return yaw;
}


float UTIL_VecToPitch( const Vector &vec )
{
	if (vec.y == 0 && vec.x == 0)
	{
		if (vec.z < 0)
			return 180.0;
		else
			return -180.0;
	}

	float dist = vec.Length2D();
	float pitch = atan2( -vec.z, dist );

	pitch = RAD2DEG(pitch);

	return pitch;
}

float UTIL_VecToYaw( const matrix3x4_t &matrix, const Vector &vec )
{
	Vector tmp = vec;
	VectorNormalize( tmp );

	float x = matrix[0][0] * tmp.x + matrix[1][0] * tmp.y + matrix[2][0] * tmp.z;
	float y = matrix[0][1] * tmp.x + matrix[1][1] * tmp.y + matrix[2][1] * tmp.z;

	if (x == 0.0f && y == 0.0f)
		return 0.0f;
	
	float yaw = atan2( -y, x );

	yaw = RAD2DEG(yaw);

	if (yaw < 0)
		yaw += 360;

	return yaw;
}


float UTIL_VecToPitch( const matrix3x4_t &matrix, const Vector &vec )
{
	Vector tmp = vec;
	VectorNormalize( tmp );

	float x = matrix[0][0] * tmp.x + matrix[1][0] * tmp.y + matrix[2][0] * tmp.z;
	float z = matrix[0][2] * tmp.x + matrix[1][2] * tmp.y + matrix[2][2] * tmp.z;

	if (x == 0.0f && z == 0.0f)
		return 0.0f;
	
	float pitch = atan2( z, x );

	pitch = RAD2DEG(pitch);

	if (pitch < 0)
		pitch += 360;

	return pitch;
}

Vector UTIL_YawToVector( float yaw )
{
	Vector ret;
	
	ret.z = 0;
	float angle = DEG2RAD( yaw );
	SinCos( angle, &ret.y, &ret.x );

	return ret;
}


//-----------------------------------------------------------------------------
//
// Shared client/server trace filter code
//
//-----------------------------------------------------------------------------
bool PassServerEntityFilter( const IHandleEntity *pTouch, const IHandleEntity *pPass ) 
{
	if ( !pPass )
		return true;

	if ( pTouch == pPass )
		return false;

	const CBaseEntity *pEntTouch = EntityFromEntityHandle( pTouch );
	const CBaseEntity *pEntPass = EntityFromEntityHandle( pPass );
	if ( !pEntTouch || !pEntPass )
		return true;

	// don't clip against own missiles
	if ( pEntTouch->GetOwnerEntity() == pEntPass )
		return false;
	
	// don't clip against owner
	if ( pEntPass->GetOwnerEntity() == pEntTouch )
		return false;	


	return true;
}


//-----------------------------------------------------------------------------
// A standard filter to be applied to just about everything.
//-----------------------------------------------------------------------------
bool StandardFilterRules( IHandleEntity *pHandleEntity, int fContentsMask )
{
	CBaseEntity *pCollide = EntityFromEntityHandle( pHandleEntity );

	// Static prop case...
	if ( !pCollide )
		return true;

	if ( pCollide->GetSolidFlags() & FSOLID_VOLUME_CONTENTS )
		return false;

	// This code is used to cull out tests against moving entities
	SolidType_t solid = pCollide->GetSolid();
	const model_t *pModel = pCollide->GetModel();

	if ( ( modelinfo->GetModelType( pModel ) != mod_brush ) || (solid != SOLID_BSP && solid != SOLID_VPHYSICS) )
	{
		if ( (fContentsMask & CONTENTS_MONSTER) == 0 )
			return false;
	}

	// This code is used to cull out tests against see-thru entities
	if ( !(fContentsMask & CONTENTS_WINDOW) && pCollide->IsTransparent() )
		return false;

	// FIXME: this is to skip BSP models that are entities that can be 
	// potentially moved/deleted, similar to a monster but doors don't seem to 
	// be flagged as monsters
	// FIXME: the FL_WORLDBRUSH looked promising, but it needs to be set on 
	// everything that's actually a worldbrush and it currently isn't
	if ( !(fContentsMask & CONTENTS_MOVEABLE) && (pCollide->GetMoveType() == MOVETYPE_PUSH))// !(touch->flags & FL_WORLDBRUSH) )
		return false;

	return true;
}



//-----------------------------------------------------------------------------
// Simple trace filter
//-----------------------------------------------------------------------------
CTraceFilterSimple::CTraceFilterSimple( const IHandleEntity *passedict, int collisionGroup )
{
	m_pPassEnt = passedict;
	m_collisionGroup = collisionGroup;
}


//-----------------------------------------------------------------------------
// The trace filter!
//-----------------------------------------------------------------------------
bool CTraceFilterSimple::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
		return false;

	if ( m_pPassEnt )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
		{
			return false;
		}
	}

	// Don't test if the game code tells us we should ignore this collision...
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
		return false;
	if ( pEntity && !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Trace filter that can take a list of entities to ignore
//-----------------------------------------------------------------------------
CTraceFilterSimpleList::CTraceFilterSimpleList( int collisionGroup ) :
	CTraceFilterSimple( NULL, collisionGroup )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTraceFilterSimpleList::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( m_PassEntities.Find(pHandleEntity) != m_PassEntities.InvalidIndex() )
		return false;

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}


//-----------------------------------------------------------------------------
// Purpose: Add an entity to my list of entities to ignore in the trace
//-----------------------------------------------------------------------------
void CTraceFilterSimpleList::AddEntityToIgnore( IHandleEntity *pEntity )
{
	m_PassEntities.AddToTail( pEntity );
}


//-----------------------------------------------------------------------------
// Sweeps against a particular model, using collision rules 
//-----------------------------------------------------------------------------
void UTIL_TraceModel( const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, 
					  const Vector &hullMax, CBaseEntity *pentModel, int collisionGroup, trace_t *ptr )
{
	// Cull it....
	if ( pentModel && pentModel->ShouldCollide( collisionGroup, MASK_ALL ) )
	{
		Ray_t ray;
		ray.Init( vecStart, vecEnd, hullMin, hullMax );
		enginetrace->ClipRayToEntity( ray, MASK_ALL, pentModel, ptr ); 
	}
	else
	{
		memset( ptr, 0, sizeof(trace_t) );
		ptr->fraction = 1.0f;
	}
}


//-----------------------------------------------------------------------------
// Sweep an entity from the starting to the ending position 
//-----------------------------------------------------------------------------
class CTraceFilterEntity : public CTraceFilterSimple
{
	DECLARE_CLASS( CTraceFilterEntity, CTraceFilterSimple );

public:
	CTraceFilterEntity( CBaseEntity *pEntity, int nCollisionGroup ) : CTraceFilterSimple( pEntity, nCollisionGroup )
	{
		if (!pEntity->GetMoveParent())
		{
			m_pTopParent = NULL;
		}
		else
		{
			m_pTopParent = HighestParent( pEntity );
		}
	}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		Assert( dynamic_cast<CBaseEntity*>(pHandleEntity) );
		CBaseEntity *pTestEntity = static_cast<CBaseEntity*>(pHandleEntity);

		// Check parents against each other
		if ( m_pTopParent || pTestEntity->GetMoveParent() )
		{
			CBaseEntity *pParent1 = HighestParent( pTestEntity );
			
			// NOTE: Don't let siblings/parents collide.
			if ( m_pTopParent == pParent1 )
				return false;
		}

		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}

private:
	CBaseEntity *HighestParent( CBaseEntity *pEntity )
	{
		if (!pEntity)
			return NULL;

		while ( pEntity->GetMoveParent() )
		{
			pEntity = pEntity->GetMoveParent();
		}

		return pEntity;
	}

	CBaseEntity *m_pTopParent;
};

class CTraceFilterEntityIgnoreOther : public CTraceFilterEntity
{
public:
	CTraceFilterEntityIgnoreOther( CBaseEntity *pEntity, const IHandleEntity *pIgnore, int nCollisionGroup ) : 
		CTraceFilterEntity( pEntity, nCollisionGroup ), m_pIgnoreOther( pIgnore )
	{
	}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( pHandleEntity == m_pIgnoreOther )
			return false;

		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}

private:
	const IHandleEntity *m_pIgnoreOther;
};

//-----------------------------------------------------------------------------
// Sweeps a particular entity through the world 
//-----------------------------------------------------------------------------
void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, unsigned int mask, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, pEntity->WorldAlignMins(), pEntity->WorldAlignMaxs() );
	CTraceFilterEntity traceFilter( pEntity, pEntity->GetCollisionGroup() );
	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
}

void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, const IHandleEntity *pIgnore, int nCollisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, pEntity->WorldAlignMins(), pEntity->WorldAlignMaxs() );
	CTraceFilterEntityIgnoreOther traceFilter( pEntity, pIgnore, nCollisionGroup );
	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
}

void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, ITraceFilter *pFilter, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, pEntity->WorldAlignMins(), pEntity->WorldAlignMaxs() );
	enginetrace->TraceRay( ray, mask, pFilter, ptr );
}

