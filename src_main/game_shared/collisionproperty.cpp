//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "collisionproperty.h"

#ifdef CLIENT_DLL

#include "c_baseentity.h"
#include "recvproxy.h"

#else

#include "baseentity.h"
#include "sendproxy.h"

#endif

#include "predictable_entity.h"

//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------

#ifndef CLIENT_DLL

	BEGIN_DATADESC_NO_BASE( CCollisionProperty )
//		DEFINE_FIELD( CCollisionProperty, m_pOuter, FIELD_CLASSPTR ),
		DEFINE_GLOBAL_KEYFIELD( CCollisionProperty, m_vecMins, FIELD_VECTOR, "mins" ),
		DEFINE_GLOBAL_KEYFIELD( CCollisionProperty, m_vecMaxs, FIELD_VECTOR, "maxs" ),
		DEFINE_GLOBAL_KEYFIELD( CCollisionProperty, m_vecSize, FIELD_VECTOR, "size" ),
//		DEFINE_FIELD( CCollisionProperty, m_vecCenter, FIELD_VECTOR ),
		DEFINE_KEYFIELD( CCollisionProperty, m_Solid, FIELD_INTEGER, "solid" ),
		DEFINE_FIELD( CCollisionProperty, m_usSolidFlags, FIELD_SHORT ),
//		DEFINE_PHYSPTR( CCollisionProperty, m_pPhysicsObject ),
	END_DATADESC()

#endif


//-----------------------------------------------------------------------------
// Prediction
//-----------------------------------------------------------------------------
BEGIN_PREDICTION_DATA_NO_BASE( CCollisionProperty )

	DEFINE_PRED_FIELD( CCollisionProperty, m_Solid, FIELD_SHORT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CCollisionProperty, m_vecMins, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CCollisionProperty, m_vecMaxs, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL

static void RecvProxy_Solid( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((CCollisionProperty*)pStruct)->SetSolid( (SolidType_t)pData->m_Value.m_Int );
}

static void RecvProxy_SolidFlags( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((CCollisionProperty*)pStruct)->SetSolidFlags( pData->m_Value.m_Int );
}

#else

static void SendProxy_Solid( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	pOut->m_Int = ((CCollisionProperty*)pStruct)->GetSolid();
}

static void SendProxy_SolidFlags( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	pOut->m_Int = ((CCollisionProperty*)pStruct)->GetSolidFlags();
}

#endif

BEGIN_NETWORK_TABLE_NOBASE( CCollisionProperty, DT_CollisionProperty )

#ifdef CLIENT_DLL
	RecvPropVector( RECVINFO(m_vecMins) ),
	RecvPropVector( RECVINFO(m_vecMaxs) ),
	RecvPropInt( "solid", 0, SIZEOF_IGNORE, 0, RecvProxy_Solid ),
	RecvPropInt( "solidflags", 0, SIZEOF_IGNORE, 0, RecvProxy_SolidFlags ),
#else
	SendPropVector( SENDINFO(m_vecMins), 0, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecMaxs), 0, SPROP_COORD),
	SendPropInt("solid", 0, SIZEOF_IGNORE,       3, SPROP_UNSIGNED, SendProxy_Solid ),
	SendPropInt("solidflags", 0, SIZEOF_IGNORE,  FSOLID_MAX_BITS, SPROP_UNSIGNED, SendProxy_SolidFlags ),
#endif

END_NETWORK_TABLE()

																							
//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CCollisionProperty::CCollisionProperty() : m_pOuter(NULL)
{
	m_vecMins.GetForModify().Init();
	m_vecMaxs.GetForModify().Init();
//	m_vecCenter.GetForModify().Init();
//	m_vecSize.GetForModify().Init();
}

CCollisionProperty::~CCollisionProperty()
{
}


//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
void CCollisionProperty::Init( CBaseEntity *pEntity )
{
	m_pOuter = pEntity;
	m_vecMins.GetForModify().Init();
	m_vecMaxs.GetForModify().Init();
//	m_vecCenter.GetForModify().Init();
//	m_vecSize.GetForModify().Init();
}


//-----------------------------------------------------------------------------
// EntityHandle
//-----------------------------------------------------------------------------
IHandleEntity *CCollisionProperty::GetEntityHandle()
{
	return m_pOuter;
}


//-----------------------------------------------------------------------------
// Collision group
//-----------------------------------------------------------------------------
int CCollisionProperty::GetCollisionGroup()
{
	return m_pOuter->GetCollisionGroup();
}


//-----------------------------------------------------------------------------
// IClientUnknown
//-----------------------------------------------------------------------------
IClientUnknown* CCollisionProperty::GetIClientUnknown()
{
#ifdef CLIENT_DLL
	return m_pOuter->GetIClientUnknown();
#else
	return NULL;
#endif
}


//-----------------------------------------------------------------------------
// Sets the solid type
//-----------------------------------------------------------------------------
void CCollisionProperty::SetSolid( SolidType_t val )
{
#ifndef CLIENT_DLL
	if ( m_Solid != val )
	{
		// OBB is not yet implemented
		Assert( val != SOLID_OBB );
		if ( val == SOLID_BSP )
		{
			if ( m_pOuter->GetParent() )
			{
				// must be SOLID_VPHYSICS because parent might rotate
				val = SOLID_VPHYSICS;
			}
		}
		m_Solid = val;

		// ivp maintains state based on recent return values from the collision filter, so anything
		// that can change the state that a collision filter will return (like m_Solid) needs to call RecheckCollisionFilter.
		IPhysicsObject *pObj = m_pOuter->VPhysicsGetObject();
		if ( pObj )
		{
			pObj->RecheckCollisionFilter();
		}

		m_pOuter->Relink();
	}
#else
	m_Solid = val;
#endif
}

SolidType_t CCollisionProperty::GetSolid() const
{
	return m_Solid;
}

//-----------------------------------------------------------------------------
// Sets the solid flags
//-----------------------------------------------------------------------------
void CCollisionProperty::SetSolidFlags( int flags )
{
#ifndef CLIENT_DLL
	int oldFlags = m_usSolidFlags;
	m_usSolidFlags = (unsigned short)(flags & 0xFFFF);

	if ( (oldFlags & FSOLID_NOT_SOLID) != (m_usSolidFlags & FSOLID_NOT_SOLID) )
	{
		// ivp maintains state based on recent return values from the collision filter, so anything
		// that can change the state that a collision filter will return (like m_Solid) needs to call RecheckCollisionFilter.
		IPhysicsObject *pObj = m_pOuter->VPhysicsGetObject();
		if ( pObj )
		{
			pObj->RecheckCollisionFilter();
		}

		m_pOuter->Relink();
	}
#else
	m_usSolidFlags = (flags & 0xFFFF);
#endif
}


//-----------------------------------------------------------------------------
// Coordinate system of the collision model
//-----------------------------------------------------------------------------
const Vector& CCollisionProperty::GetCollisionOrigin()
{
	return m_pOuter->GetAbsOrigin();
}

const QAngle& CCollisionProperty::GetCollisionAngles()
{
	if ( !IsBoundsDefinedInEntitySpace() )
	{
		return vec3_angle;
	}

	return m_pOuter->GetAbsAngles();
}


//-----------------------------------------------------------------------------
// Sets the collision bounds + the size
//-----------------------------------------------------------------------------
void CCollisionProperty::SetCollisionBounds( const Vector& mins, const Vector &maxs )
{
	m_vecMins = mins;
	m_vecMaxs = maxs;

#ifndef CLIENT_DLL
	VectorSubtract( maxs, mins, m_vecSize );
#endif
}


//-----------------------------------------------------------------------------
// Bounding representation (OBB/AABB)
//-----------------------------------------------------------------------------
const Vector& CCollisionProperty::WorldAlignMins( ) const
{
	Assert( !IsBoundsDefinedInEntitySpace() );
	return m_vecMins.Get();
}

const Vector& CCollisionProperty::WorldAlignMaxs( ) const
{
	Assert( !IsBoundsDefinedInEntitySpace() );
	return m_vecMaxs.Get();
}

const Vector& CCollisionProperty::EntitySpaceMins( ) const
{
	Assert( IsBoundsDefinedInEntitySpace() );

	// FIXME! Not changing behavior at first
	return m_vecMins.Get();
}

const Vector& CCollisionProperty::EntitySpaceMaxs( ) const
{
	Assert( IsBoundsDefinedInEntitySpace() );

	// FIXME! Not changing behavior at first
	return m_vecMaxs.Get();
}


//-----------------------------------------------------------------------------
// Collision model (BSP)
//-----------------------------------------------------------------------------
int CCollisionProperty::GetCollisionModelIndex()
{
	return m_pOuter->GetModelIndex();
}

const model_t* CCollisionProperty::GetCollisionModel()
{
	return m_pOuter->GetModel();
}


//-----------------------------------------------------------------------------
// Collision methods implemented in the entity
// FIXME: This shouldn't happen there!!
//-----------------------------------------------------------------------------
bool CCollisionProperty::TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	return m_pOuter->TestCollision( ray, fContentsMask, tr );
}

bool CCollisionProperty::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	return m_pOuter->TestHitboxes( ray, fContentsMask, tr );
}
