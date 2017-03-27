//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef COLLISIONPROPERTY_H
#define COLLISIONPROPERTY_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "engine/ICollideable.h"
#include "vector.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseEntity;
class IHandleEntity;
class QAngle;
class Vector;
struct Ray_t;
class IPhysicsObject;


//-----------------------------------------------------------------------------
// Encapsulates collision representation for an entity
//-----------------------------------------------------------------------------
class CCollisionProperty : public ICollideable
{
	DECLARE_CLASS_NOBASE( CCollisionProperty );
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CCollisionProperty();
	~CCollisionProperty();

	void Init( CBaseEntity *pEntity );

	// Methods of ICollideable
	virtual IHandleEntity	*GetEntityHandle();
	virtual const Vector&	WorldAlignMins( ) const;
	virtual const Vector&	WorldAlignMaxs( ) const;
 	virtual const Vector&	EntitySpaceMins( ) const;
	virtual const Vector&	EntitySpaceMaxs( ) const;
	virtual bool			IsBoundsDefinedInEntitySpace() const;
	virtual bool			TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	virtual bool			TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	virtual int				GetCollisionModelIndex();
	virtual const model_t*	GetCollisionModel();
	virtual const Vector&	GetCollisionOrigin();
	virtual const QAngle&	GetCollisionAngles();
	virtual SolidType_t		GetSolid() const;
	virtual int				GetSolidFlags() const;
	virtual IClientUnknown*	GetIClientUnknown();
	virtual int				GetCollisionGroup();

	// Sets the collision bounds + the size (OBB/AABB)
	void					SetCollisionBounds( const Vector& mins, const Vector &maxs );

	void					SetSolid( SolidType_t val );

	// Methods related to size
	const Vector&			WorldAlignSize( ) const;
	const Vector&			EntitySpaceSize( ) const;

	// Methods related to solid flags
	void					ClearSolidFlags( void );	
	void					RemoveSolidFlags( int flags );
	void					AddSolidFlags( int flags );
	bool					IsSolidFlagSet( int flagMask ) const;
	void				 	SetSolidFlags( int flags );
	bool					IsSolid() const;

private:
	CBaseEntity *GetOuter();

private:
	CBaseEntity *m_pOuter;

	CNetworkVector( m_vecMins );
	CNetworkVector( m_vecMaxs );
	Vector			m_vecSize;
	mutable Vector	m_vecCenter;

	// One of the SOLID_ defines. Use GetSolid/SetSolid.
	CNetworkVar( SolidType_t, m_Solid );			
	CNetworkVar( unsigned short, m_usSolidFlags );

	// pointer to the entity's physics object (vphysics.dll)
	IPhysicsObject	*m_pPhysicsObject;
	
	friend class CBaseEntity;
};


//-----------------------------------------------------------------------------
// For networking this bad boy
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_CollisionProperty );
#else
EXTERN_SEND_TABLE( DT_CollisionProperty );
#endif


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline CBaseEntity *CCollisionProperty::GetOuter()
{
	return m_pOuter;
}


//-----------------------------------------------------------------------------
// Methods related to size
//-----------------------------------------------------------------------------
inline const Vector& CCollisionProperty::WorldAlignSize( ) const
{
	Assert( !IsBoundsDefinedInEntitySpace() );
	return m_vecSize;
}

inline const Vector& CCollisionProperty::EntitySpaceSize( ) const
{
	Assert( IsBoundsDefinedInEntitySpace() );
	return m_vecSize;
}


//-----------------------------------------------------------------------------
// Methods relating to solid flags
//-----------------------------------------------------------------------------
inline bool CCollisionProperty::IsBoundsDefinedInEntitySpace() const
{
	return ( m_Solid == SOLID_BSP || m_Solid == SOLID_OBB || m_Solid == SOLID_VPHYSICS );
}

inline void CCollisionProperty::ClearSolidFlags( void )
{
	SetSolidFlags( 0 );
}

inline void CCollisionProperty::RemoveSolidFlags( int flags )
{
	SetSolidFlags( m_usSolidFlags & ~flags );
}

inline void CCollisionProperty::AddSolidFlags( int flags )
{
	SetSolidFlags( m_usSolidFlags | flags );
}

inline int CCollisionProperty::GetSolidFlags( void ) const
{
	return m_usSolidFlags;
}

inline bool CCollisionProperty::IsSolidFlagSet( int flagMask ) const
{
	return (m_usSolidFlags & flagMask) != 0;
}

inline bool CCollisionProperty::IsSolid() const
{
	return ::IsSolid( m_Solid, m_usSolidFlags );
}


#endif // COLLISIONPROPERTY_H
