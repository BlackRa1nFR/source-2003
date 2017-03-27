//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ISERVERENTITY_H
#define ISERVERENTITY_H
#ifdef _WIN32
#pragma once
#endif


#include "cmodel.h"
#include "iservernetworkable.h"


struct Ray_t;
class ServerClass;
class ICollideable;


// This class is how the engine talks to entities in the game DLL.
// CBaseEntity implements this interface.
class IServerEntity : public IServerNetworkable
{
public:
	virtual					~IServerEntity() {}

	// Gets the interface to the collideable representation of the entity
	virtual ICollideable	*GetCollideable() = 0;
	
	// Tell the entity to recompute it's absmins/absmaxs
	virtual void			SetObjectCollisionBox() = 0;
	virtual void			CalcAbsolutePosition() = 0;

// Previously in pev
	// Indicate that the physics system should check this entity for untouching at the end of the
	//  current simulation frame
	virtual void			SetCheckUntouch( bool check ) = 0;

	// See if entity is currently touching any other entity
	virtual bool			IsCurrentlyTouching( void ) const = 0;

	// Cleared every frame and set to true any time this entity is sent to a client. 
	// A rough approximation of "can anyone see me?"
	virtual void			SetSentLastFrame( bool wassent ) = 0;

	virtual const Vector&	GetAbsOrigin( void ) const = 0;
 	virtual const QAngle&	GetAbsAngles( void ) const = 0;

	virtual const Vector&	GetAbsMins( void ) const = 0;
	virtual const Vector&	GetAbsMaxs( void ) const = 0;

	virtual int				GetModelIndex( void ) const = 0;
 	virtual string_t		GetModelName( void ) const = 0;

	virtual void			SetModelIndex( int index ) = 0;
};


#endif // ISERVERENTITY_H
