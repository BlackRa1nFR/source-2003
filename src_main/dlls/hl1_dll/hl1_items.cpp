//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "hl1_items.h"


void CHL1Item::Spawn( void )
{
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );
	
	// This will make them not collide with anything but the world.
	SetCollisionGroup( COLLISION_GROUP_NONE );
	UTIL_SetSize(this, Vector(-15, -15, 0), Vector(15, 15, 15));
	SetTouch(ItemTouch);

	//Create the object in the physics system
	if ( VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE | FSOLID_TRIGGER | FSOLID_NOT_SOLID, false ) == NULL )
	{
		AddSolidFlags( FSOLID_NOT_STANDABLE );
		AddSolidFlags( FSOLID_TRIGGER );

		// If it's not physical, drop it to the floor
		if (UTIL_DropToFloor(this, MASK_SOLID) == 0)
		{
			Warning( "Item %s fell out of level at %f,%f,%f\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
			UTIL_Remove( this );
			return;
		}
	}
	Relink();
}
