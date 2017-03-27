//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ENGINE_ICOLLIDEABLE_H
#define ENGINE_ICOLLIDEABLE_H
#ifdef _WIN32
#pragma once
#endif


enum SolidType_t;
class IHandleEntity;
struct Ray_t;
struct model_t;
class Vector;
class QAngle;
class CGameTrace;
typedef CGameTrace trace_t;
class IClientUnknown;


class ICollideable
{
public:
	// Gets at the entity handle associated with the collideable
	virtual IHandleEntity	*GetEntityHandle() = 0;

	// These methods return a *world-aligned* box relative to the absorigin of the entity.
	virtual const Vector&	WorldAlignMins( ) const = 0;
	virtual const Vector&	WorldAlignMaxs( ) const = 0;

	// These methods return a box defined in the space of the entity
 	virtual const Vector&	EntitySpaceMins( ) const = 0;
	virtual const Vector&	EntitySpaceMaxs( ) const = 0;

	// Returns the center of the entity
	virtual bool			IsBoundsDefinedInEntitySpace() const = 0;

	// custom collision test
	virtual bool			TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr ) = 0;

	// Perform hitbox test, returns true *if hitboxes were tested at all*!!
	virtual bool			TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr ) = 0;

	// Returns the BRUSH model index if this is a brush model. Otherwise, returns -1.
	virtual int				GetCollisionModelIndex() = 0;

	// Return the model, if it's a studio model.
	virtual const model_t*	GetCollisionModel() = 0;

	// Get angles and origin.
	virtual const Vector&	GetCollisionOrigin() = 0;
	virtual const QAngle&	GetCollisionAngles() = 0;

	// Return a SOLID_ define.
	virtual SolidType_t		GetSolid() const = 0;
	virtual int				GetSolidFlags() const = 0;

	// Gets at the containing class...
	virtual IClientUnknown*	GetIClientUnknown() = 0;
	
	// We can filter out collisions based on collision group
	virtual int				GetCollisionGroup() = 0;
};


#endif // ENGINE_ICOLLIDEABLE_H
