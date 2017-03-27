//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "gamemovement.h"
#include "in_buttons.h"
#include <stdarg.h>
#include "movevars_shared.h"
#include "engine/IEngineTrace.h"
#include "SoundEmitterSystemBase.h"
#include "decals.h"
#include "tier0/vprof.h"
#include "hl1_gamemovement.h"


// Expose our interface.
static CHL1GameMovement g_GameMovement;
IGameMovement *g_pGameMovement = ( IGameMovement * )&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL1GameMovement::CheckJumpButton( void )
{
	m_pHL1Player = ToHL1Player( player );

	Assert( m_pHL1Player );

	if (m_pHL1Player->pl.deadflag)
	{
		mv->m_nOldButtons |= IN_JUMP ;	// don't jump again until released
		return;
	}

	// See if we are waterjumping.  If so, decrement count and return.
	if (m_pHL1Player->m_flWaterJumpTime)
	{
		m_pHL1Player->m_flWaterJumpTime -= gpGlobals->frametime;
		if (m_pHL1Player->m_flWaterJumpTime < 0)
			m_pHL1Player->m_flWaterJumpTime = 0;
		
		return;
	}

	// If we are in the water most of the way...
	if ( m_pHL1Player->GetWaterLevel() >= 2 )
	{	
		// swimming, not jumping
		m_pHL1Player->SetGroundEntity( (CBaseEntity *)NULL );

		if(m_pHL1Player->GetWaterType() == CONTENTS_WATER)    // We move up a certain amount
			mv->m_vecVelocity[2] = 100;
		else if (m_pHL1Player->GetWaterType() == CONTENTS_SLIME)
			mv->m_vecVelocity[2] = 80;
		
		// play swiming sound
		if ( m_pHL1Player->m_flSwimSoundTime <= 0 )
		{
			// Don't play sound again for 1 second
			m_pHL1Player->m_flSwimSoundTime = 1000;
			PlaySwimSound();
		}

		return;
	}

	// No more effect
 	if (m_pHL1Player->GetGroundEntity() == NULL)
	{
		mv->m_nOldButtons |= IN_JUMP;
		return;		// in air, so no effect
	}

	if ( mv->m_nOldButtons & IN_JUMP )
		return;		// don't pogo stick

	// In the air now.
    m_pHL1Player->SetGroundEntity( (CBaseEntity *)NULL );
	
	PlayStepSound( m_pSurfaceData, 1.0, true );
	
	MoveHelper()->PlayerSetAnimation( PLAYER_JUMP );

	float flGroundFactor = 1.0f;
	if (m_pSurfaceData)
	{
		flGroundFactor = m_pSurfaceData->jumpFactor; 
	}

	// Acclerate upward
	// If we are ducking...
	float startz = mv->m_vecVelocity[2];
	if ( (  m_pHL1Player->m_Local.m_bDucking ) || (  m_pHL1Player->GetFlags() & FL_DUCKING ) )
	{
		// d = 0.5 * g * t^2		- distance traveled with linear accel
		// t = sqrt(2.0 * 45 / g)	- how long to fall 45 units
		// v = g * t				- velocity at the end (just invert it to jump up that high)
		// v = g * sqrt(2.0 * 45 / g )
		// v^2 = g * g * 2.0 * 45 / g
		// v = sqrt( g * 2.0 * 45 )
		
		// Adjust for super long jump module
		// UNDONE -- note this should be based on forward angles, not current velocity.
		if ( m_pHL1Player->m_bHasLongJump &&
			( mv->m_nButtons & IN_DUCK ) &&
			( m_pHL1Player->m_Local.m_flDucktime > 0 ) &&
			mv->m_vecVelocity.Length() > 50 )
		{
			m_pHL1Player->m_Local.m_vecPunchAngle.Set( PITCH, -5 );

			mv->m_vecVelocity = m_vecForward * PLAYER_LONGJUMP_SPEED * 1.6;
			mv->m_vecVelocity.z = sqrt(2 * 800 * 56.0);
		}
		else
		{
			mv->m_vecVelocity[2] = flGroundFactor * sqrt(2 * 800 * 45.0);  // 2 * gravity * height
		}
	}
	else
	{
		mv->m_vecVelocity[2] += flGroundFactor * sqrt(2 * 800 * 45.0);  // 2 * gravity * height
	}
	FinishGravity();

	mv->m_outWishVel.z += mv->m_vecVelocity[2] - startz;
	mv->m_outStepHeight += 0.1f;

	// Flag that we jumped.
	mv->m_nOldButtons |= IN_JUMP;	// don't jump again until released
}