//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "usercmd.h"
#include "bitbuf.h"

// TF2 specific, need enough space for OBJ_LAST items from tf_shareddefs.h
#define WEAPON_SUBTYPE_BITS	6

//-----------------------------------------------------------------------------
// Purpose: Write a delta compressed user command.
// Input  : *buf - 
//			*to - 
//			*from - 
// Output : static
//-----------------------------------------------------------------------------
void WriteUsercmd( bf_write *buf, CUserCmd *to, CUserCmd *from )
{
	if ( to->command_number != ( from->command_number + 1 ) )
	{
		buf->WriteOneBit( 1 );
		buf->WriteUBitLong( to->command_number, 32 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->viewangles[ 0 ] != from->viewangles[ 0 ] )
	{
		buf->WriteOneBit( 1 );
		buf->WriteBitAngle( to->viewangles[ 0 ], 16 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->viewangles[ 1 ] != from->viewangles[ 1 ] )
	{
		buf->WriteOneBit( 1 );
		buf->WriteBitAngle( to->viewangles[ 1 ], 16 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->viewangles[ 2 ] != from->viewangles[ 2 ] )
	{
		buf->WriteOneBit( 1 );
		buf->WriteBitAngle( to->viewangles[ 2 ], 8 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->forwardmove != from->forwardmove )
	{
		buf->WriteOneBit( 1 );
		buf->WriteSBitLong( to->forwardmove, 16 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->sidemove != from->sidemove )
	{
		buf->WriteOneBit( 1 );
		buf->WriteSBitLong( to->sidemove, 16 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->upmove != from->upmove )
	{
		buf->WriteOneBit( 1 );
		buf->WriteSBitLong( to->upmove, 16 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->buttons != from->buttons )
	{
		buf->WriteOneBit( 1 );
	  	buf->WriteUBitLong( to->buttons, 32 );
 	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->impulse != from->impulse )
	{
		buf->WriteOneBit( 1 );
	    buf->WriteUBitLong( to->impulse, 8 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	// Write msec, frametime will be reconstructed on server
	buf->WriteUBitLong( to->msec, FRAMETIME_BITPRECISION );

	buf->WriteOneBit( to->predict_weapons );
	buf->WriteOneBit( to->lag_compensation );

	if ( to->lerp_msec != from->lerp_msec )
	{
		buf->WriteOneBit( 1 );
		buf->WriteUBitLong( (int)to->lerp_msec, 8 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->commandrate != from->commandrate )
	{
		buf->WriteOneBit( 1 );
		buf->WriteUBitLong( to->commandrate, 8 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}


	if ( to->updaterate != from->updaterate )
	{
		buf->WriteOneBit( 1 );
		buf->WriteUBitLong( to->updaterate, 8 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->weaponselect != from->weaponselect )
	{
		buf->WriteOneBit( 1 );
		buf->WriteUBitLong( to->weaponselect, MAX_EDICT_BITS );

		if ( to->weaponsubtype != from->weaponsubtype )
		{
			buf->WriteOneBit( 1 );
			buf->WriteUBitLong( to->weaponsubtype, WEAPON_SUBTYPE_BITS );
		}
		else
		{
			buf->WriteOneBit( 0 );
		}
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->random_seed != from->random_seed )
	{
		buf->WriteOneBit( 1 );
		buf->WriteUBitLong( to->random_seed, 32 );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	// TODO: Can probably get away with fewer bits.
	if ( to->mousedx != from->mousedx )
	{
		buf->WriteOneBit( 1 );
		buf->WriteShort( to->mousedx );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}

	if ( to->mousedy != from->mousedy )
	{
		buf->WriteOneBit( 1 );
		buf->WriteShort( to->mousedy );
	}
	else
	{
		buf->WriteOneBit( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Read in a delta compressed usercommand.
// Input  : *buf - 
//			*move - 
//			*from - 
// Output : static void ReadUsercmd
//-----------------------------------------------------------------------------
void ReadUsercmd( bf_read *buf, CUserCmd *move, CUserCmd *from )
{
	// Assume no change
	*move = *from;

	if ( buf->ReadOneBit() )
	{
		move->command_number = buf->ReadUBitLong( 32 );
	}
	else
	{
		// Assume steady increment
		move->command_number = from->command_number + 1;
	}

	// Read direction
	if ( buf->ReadOneBit() )
	{
		move->viewangles[0] = buf->ReadBitAngle( 16 );
	}

	if ( buf->ReadOneBit() )
	{
		move->viewangles[1] = buf->ReadBitAngle( 16 );
	}

	if ( buf->ReadOneBit() )
	{
		move->viewangles[2] = buf->ReadBitAngle( 8 );
	}

	// Read movement
	if ( buf->ReadOneBit() )
	{
		move->forwardmove = buf->ReadSBitLong( 16 );
	}
	
	if ( buf->ReadOneBit() )
	{
		move->sidemove = buf->ReadSBitLong( 16 );
	}

	if ( buf->ReadOneBit() )
	{
		move->upmove = buf->ReadSBitLong( 16 );
	}

	// read buttons
	if ( buf->ReadOneBit() )
	{
		move->buttons = buf->ReadUBitLong( 32 );
	}

	if ( buf->ReadOneBit() )
	{
		move->impulse = buf->ReadUBitLong( 8 );
	}

	// read time to run command
	move->msec = buf->ReadUBitLong( FRAMETIME_BITPRECISION );
	move->msec = clamp( move->msec, 0, (int)( MAX_USERCMD_FRAMETIME * FRAMETIME_ROUNDFACTOR ) );

	move->frametime = (float)move->msec / FRAMETIME_ROUNDFACTOR;
	move->frametime = clamp( move->frametime, 0.0f, MAX_USERCMD_FRAMETIME );

	move->predict_weapons	= buf->ReadOneBit() ? true : false;
	move->lag_compensation	= buf->ReadOneBit() ? true : false;

	if ( buf->ReadOneBit() )
	{
		move->lerp_msec = (float)buf->ReadUBitLong( 8 );
	}

	if ( buf->ReadOneBit() )
	{
		move->commandrate = buf->ReadUBitLong( 8 );
	}

	if ( buf->ReadOneBit() )
	{
		move->updaterate = buf->ReadUBitLong( 8 );
	}

	if ( buf->ReadOneBit() )
	{
		move->weaponselect = buf->ReadUBitLong( MAX_EDICT_BITS );
		if ( buf->ReadOneBit() )
		{
			move->weaponsubtype = buf->ReadUBitLong( WEAPON_SUBTYPE_BITS );
		}
	}

	if ( buf->ReadOneBit() )
	{
		move->random_seed = buf->ReadUBitLong( 32 );
	}

	if ( buf->ReadOneBit() )
	{
		move->mousedx = buf->ReadShort();
	}

	if ( buf->ReadOneBit() )
	{
		move->mousedy = buf->ReadShort();
	}
}
