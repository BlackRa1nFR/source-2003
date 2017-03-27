//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"
#include "hl1_basecombatweapon_shared.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHL1CombatWeapon::SetObjectCollisionBox( void )
{
	ComputeSurroundingBox();
	SetAbsMins( GetAbsMins() + Vector( -24, -24, 0 ) );
	SetAbsMaxs( GetAbsMaxs() + Vector( 24, 24, 16 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHL1CombatWeapon::FallInit( void )
{
	SetModel( GetWorldModel() );
	SetSize( Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );	// Pointsize until it lands on the ground
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER );
	AddSolidFlags( FSOLID_NOT_SOLID );
	Relink();

	SetPickupTouch();
	
	SetThink( FallThink );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Items that have just spawned run this think to catch them when 
//			they hit the ground. Once we're sure that the object is grounded, 
//			we change its solid type to trigger and set it in a large box that 
//			helps the player get it.
//-----------------------------------------------------------------------------
void CBaseHL1CombatWeapon::FallThink ( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( CBaseEntity::GetFlags() & FL_ONGROUND )
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if ( GetOwnerEntity() )
		{
			EmitSound( "BaseCombatWeapon.WeaponDrop" );
		}

		// lie flat
		QAngle ang = GetAbsAngles();
		ang.x = 0;
		ang.z = 0;
		SetAbsAngles( ang );

		Materialize(); 
	}
}
