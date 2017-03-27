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
#define	STOP_EPSILON		0.1
#define	MAX_CLIP_PLANES		5

#include "filesystem.h"
#include <stdarg.h>

extern IFileSystem *filesystem;

void COM_Log( char *pszFile, char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	FileHandle_t fp;
	char *pfilename;
	
	if ( !pszFile )
	{
		pfilename = "hllog.txt";
	}
	else
	{
		pfilename = pszFile;
	}
	va_start (argptr,fmt);
	Q_vsnprintf(string, sizeof( string ), fmt,argptr);
	va_end (argptr);

	fp = filesystem->Open( pfilename, "a+t");
	if (fp)
	{
		filesystem->FPrintf(fp, "%s", string);
		filesystem->Close(fp);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructs GameMovement interface
//-----------------------------------------------------------------------------
CGameMovement::CGameMovement( void )
{
	m_nOldWaterLevel	= WL_NotInWater;
	m_nOnLadder			= 0;

	mv					= NULL;

	// FIXME YWB:  Get these from a shared header
	m_vecMinsNormal = VEC_HULL_MIN;
	m_vecMaxsNormal = VEC_HULL_MAX;
	m_vecMinsDucked = VEC_DUCK_HULL_MIN;
	m_vecMaxsDucked = VEC_DUCK_HULL_MAX;

	m_vecViewOffsetNormal = VEC_VIEW;
	m_vecViewOffsetDucked = VEC_DUCK_VIEW;

	m_surfaceProps = 0;
	m_surfaceFriction = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameMovement::~CGameMovement( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ducked - 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector& CGameMovement::GetPlayerMins( bool ducked ) const
{
	return ducked ? m_vecMinsDucked : m_vecMinsNormal;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ducked - 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector& CGameMovement::GetPlayerMaxs( bool ducked ) const
{	
	return ducked ? m_vecMaxsDucked : m_vecMaxsNormal;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ducked - 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector& CGameMovement::GetPlayerViewOffset( bool ducked ) const
{
	return ducked ? m_vecViewOffsetDucked : m_vecViewOffsetNormal;
}


//-----------------------------------------------------------------------------
// Traces player movement + position
//-----------------------------------------------------------------------------
inline void CGameMovement::TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	VPROF( "CGameMovement::TracePlayerBBox" );

	Ray_t ray;
	ray.Init( start, end, GetPlayerMins( player->m_Local.m_bDucked ), GetPlayerMaxs( player->m_Local.m_bDucked ) );
	UTIL_TraceRay( ray, fMask, mv->m_nPlayerHandle.Get(), collisionGroup, &pm );
}

inline CBaseHandle CGameMovement::TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm )
{
	Ray_t ray;
	ray.Init( pos, pos, GetPlayerMins( player->m_Local.m_bDucked ), GetPlayerMaxs( player->m_Local.m_bDucked ) );
	UTIL_TraceRay( ray, MASK_PLAYERSOLID, mv->m_nPlayerHandle.Get(), collisionGroup, &pm );
	if ( (pm.contents & MASK_PLAYERSOLID) && pm.m_pEnt )
	{
		return pm.m_pEnt->GetRefEHandle();
	}
	else
	{	
		return INVALID_EHANDLE_INDEX;
	}
}


/*

// FIXME FIXME:  Does this need to be hooked up?
bool CGameMovement::IsWet() const
{
	return ((pev->flags & FL_INRAIN) != 0) || (m_WetTime >= gpGlobals->time);
}

//-----------------------------------------------------------------------------
// Plants player footprint decals
//-----------------------------------------------------------------------------

#define PLAYER_HALFWIDTH 12
void CGameMovement::PlantFootprint( surfacedata_t *psurface )
{
	// Can't plant footprints on fake materials (ladders, wading)
	if ( psurface->gameMaterial != 'X' )
	{
		int footprintDecal = -1;

		// Figure out which footprint type to plant...
		// Use the wet footprint if we're wet...
		if (IsWet())
		{
			footprintDecal = DECAL_FOOTPRINT_WET;
		}
		else
		{	   
			// FIXME: Activate this once we decide to pull the trigger on it.
			// NOTE: We could add in snow, mud, others here
//			switch(psurface->gameMaterial)
//			{
//			case 'D':
//				footprintDecal = DECAL_FOOTPRINT_DIRT;
//				break;
//			}
		}

		if (footprintDecal != -1)
		{
			Vector right;
			AngleVectors( pev->angles, 0, &right, 0 );

			// Figure out where the top of the stepping leg is 
			trace_t tr;
			Vector hipOrigin;
			VectorMA( pev->origin, 
				m_IsFootprintOnLeft ? -PLAYER_HALFWIDTH : PLAYER_HALFWIDTH,
				right, hipOrigin );

			// Find where that leg hits the ground
			UTIL_TraceLine( hipOrigin, hipOrigin + Vector(0, 0, -COORD_EXTENT * 1.74), 
							MASK_SOLID_BRUSHONLY, edict(), COLLISION_GROUP_NONE, &tr);

			unsigned char mType = TEXTURETYPE_Find( &tr );

			// Splat a decal
			CPVSFilter filter( tr.endpos );
			te->FootprintDecal( filter, 0.0f, &tr.endpos, &right, ENTINDEX(tr.u.ent), 
							   gDecals[footprintDecal].index, mType );

		}
	}

	// Switch feet for next time
	m_IsFootprintOnLeft = !m_IsFootprintOnLeft;
}

#define WET_TIME			    5.f	// how many seconds till we're completely wet/dry
#define DRY_TIME			   20.f	// how many seconds till we're completely wet/dry

void CBasePlayer::UpdateWetness()
{
	// BRJ 1/7/01
	// Check for whether we're in a rainy area....
	// Do this by tracing a line straight down with a size guaranteed to
	// be larger than the map
	// Update wetness based on whether we're in rain or not...

	trace_t tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector(0, 0, -COORD_EXTENT * 1.74), 
					MASK_SOLID_BRUSHONLY, edict(), COLLISION_GROUP_NONE, &tr);
	if (tr.surface.flags & SURF_WET)
	{
		if (! (pev->flags & FL_INRAIN) )
		{
			// Transition...
			// Figure out how wet we are now (we were drying off...)
			float wetness = (m_WetTime - gpGlobals->time) / DRY_TIME;
			if (wetness < 0.0f)
				wetness = 0.0f;

			// Here, wet time represents the time at which we get totally wet
			m_WetTime = gpGlobals->time + (1.0 - wetness) * WET_TIME; 

			pev->flags |= FL_INRAIN;
		}
	}
	else
	{
		if ((pev->flags & FL_INRAIN) != 0)
		{
			// Transition...
			// Figure out how wet we are now (we were getting more wet...)
			float wetness = 1.0f + (gpGlobals->time - m_WetTime) / WET_TIME;
			if (wetness > 1.0f)
				wetness = 1.0f;

			// Here, wet time represents the time at which we get totally dry
			m_WetTime = gpGlobals->time + wetness * DRY_TIME; 

			pev->flags &= ~FL_INRAIN;
		}
	}
}
*/


//-----------------------------------------------------------------------------
// Should the step sound play?
//-----------------------------------------------------------------------------
bool CGameMovement::ShouldPlayStepSound( surfacedata_t *psurface, float fvol )
{
	if ( gpGlobals->maxClients > 1 && ( !m_nOnLadder && Vector2DLength( mv->m_vecVelocity.AsVector2D() ) <= 195 ) )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : step - 
//			fvol - 
//			force - force sound to play
//-----------------------------------------------------------------------------
void CGameMovement::PlayStepSound( surfacedata_t *psurface, float fvol, bool force )
{
	if ( gpGlobals->maxClients > 1 && !sv_footsteps.GetFloat() )
		return;

	if ( !psurface )
		return;

	IPhysicsSurfaceProps *physprops = MoveHelper( )->GetSurfaceProps();

	if (!force && !ShouldPlayStepSound( psurface, fvol ))
		return;

// TODO:  See note above, should this be hooked up?
//	PlantFootprint( psurface );

	unsigned short count, start; 
	if ( player->m_Local.m_nStepside )
	{
		count = psurface->stepleftCount;
		start = psurface->stepleft;
	}
	else
	{
		count = psurface->steprightCount;
		start = psurface->stepright;
	}

	player->m_Local.m_nStepside = !player->m_Local.m_nStepside;

	if ( !count )
		return;

	if ( !mv->m_bFirstRunOfFunctions )
		return;

	const char *pSoundName = physprops->GetString( start, random->RandomInt( 0, count-1 ) );
	CSoundParameters params;
	if ( !CBaseEntity::GetParametersForSound( pSoundName, params ) )
		return;

	MoveHelper( )->StartSound( mv->m_vecOrigin, CHAN_BODY, params.soundname, fvol, params.soundlevel, 0, params.pitch );
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::CategorizeGroundSurface( void )
{
	//
	// Find the name of the material that lies beneath the player.
	//
	Vector start, end;
	VectorCopy( mv->m_vecOrigin, start );
	VectorCopy( mv->m_vecOrigin, end );

	// Straight down
	end[2] -= 64;

	// Fill in default values, just in case.
	trace_t	trace;
	TracePlayerBBox( start, end, MASK_PLAYERSOLID_BRUSHONLY, COLLISION_GROUP_PLAYER_MOVEMENT, trace ); 
	IPhysicsSurfaceProps *physprops = MoveHelper()->GetSurfaceProps();
	m_surfaceProps = trace.surface.surfaceProps;
	m_pSurfaceData = physprops->GetSurfaceData( m_surfaceProps );
	physprops->GetPhysicsProperties( m_surfaceProps, NULL, NULL, &m_surfaceFriction, NULL );
	
	// HACKHACK: Scale this to fudge the relationship between vphysics friction values and player friction values.
	// A value of 0.8f feels pretty normal for vphysics, whereas 1.0f is normal for players.
	// This scaling trivially makes them equivalent.  REVISIT if this affects low friction surfaces too much.
	m_surfaceFriction *= 1.25f;
	if ( m_surfaceFriction > 1.0f )
		m_surfaceFriction = 1.0f;
	m_chTextureType = m_pSurfaceData->gameMaterial;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::UpdateStepSound( void )
{
	int	fWalking;
	float fvol;
	Vector knee;
	Vector feet;
	Vector center;
	float height;
	float speed;
	float velrun;
	float velwalk;
	float flduck;
	int	fLadder;

	if ( player->m_flStepSoundTime > 0 )
		return;

	if ( player->GetFlags() & (FL_FROZEN|FL_ATCONTROLS))
		return;

	if ( player->GetMoveType() == MOVETYPE_NOCLIP )
		return;

	if ( !sv_footsteps.GetFloat() )
		return;

	IPhysicsSurfaceProps *physprops = MoveHelper( )->GetSurfaceProps();
	surfacedata_t *psurface = m_pSurfaceData;

	speed = VectorLength( mv->m_vecVelocity );

	// determine if we are on a ladder
	fLadder = ( player->GetMoveType() == MOVETYPE_LADDER );// IsOnLadder();

	// UNDONE: need defined numbers for run, walk, crouch, crouch run velocities!!!!	
	if ( ( player->GetFlags() & FL_DUCKING) || fLadder )
	{
		velwalk = 60;		// These constants should be based on cl_movespeedkey * cl_forwardspeed somehow
		velrun = 80;		// UNDONE: Move walking to server
		flduck = 100;
	}
	else
	{
// UNDONE TF2 specific code:
// TF2: Changed these speeds to make the dependant on class's maxspeed
//if (!m_pPlayerClass)
//	return;
//
//	velwalk = m_pPlayerClass->GetMaxWalkSpeed() / 2;	// Make walk sounds from halfspeed up
//	velrun = m_pPlayerClass->GetMaxWalkSpeed() + 1;		// Make run sounds while sprint is on
//
//
		velwalk = 120;
		velrun = 220;
		flduck = 0;
	}

	// If we're on a ladder or on the ground, and we're moving fast enough,
	//  play step sound.  Also, if player->m_flStepSoundTime is zero, get the new
	//  sound right away - we just started moving in new level.
	if ( (fLadder || ( player->GetGroundEntity() != NULL ) ) &&
		( VectorLength( mv->m_vecVelocity ) > 0.0 ) &&
		( speed >= velwalk || !player->m_flStepSoundTime ) )
	{
		MoveHelper()->PlayerSetAnimation( PLAYER_WALK );

		fWalking = speed < velrun;		

		VectorCopy( mv->m_vecOrigin, center );
		VectorCopy( mv->m_vecOrigin, knee );
		VectorCopy( mv->m_vecOrigin, feet );

		height = GetPlayerMaxs( player->m_Local.m_bDucked )[ 2 ] - GetPlayerMins( player->m_Local.m_bDucked )[ 2 ];

		knee[2] = mv->m_vecOrigin[2] + 0.2 * height;

		// find out what we're stepping in or on...
		if (fLadder)
		{
			psurface = physprops->GetSurfaceData( physprops->GetSurfaceIndex( "ladder" ) );
			fvol = 0.35;
			player->m_flStepSoundTime = 350;
		}
		else if ( enginetrace->GetPointContents( knee ) & MASK_WATER )
		{
			static int iSkipStep = 0;

			if ( iSkipStep == 0 )
			{
				iSkipStep++;
				return;
			}

			if ( iSkipStep++ == 3 )
			{
				iSkipStep = 0;
			}
			psurface = physprops->GetSurfaceData( physprops->GetSurfaceIndex( "wade" ) );
			fvol = 0.65;
			player->m_flStepSoundTime = 600;
		}
		else if ( enginetrace->GetPointContents( feet ) & MASK_WATER )
		{
			psurface = physprops->GetSurfaceData( physprops->GetSurfaceIndex( "water" ) );
			fvol = fWalking ? 0.2 : 0.5;
			player->m_flStepSoundTime = fWalking ? 400 : 300;		
		}
		else
		{
			player->m_flStepSoundTime = fWalking ? 400 : 300;
			switch ( m_chTextureType )
			{
			default:
			case CHAR_TEX_CONCRETE:						
				fvol = fWalking ? 0.2 : 0.5;
				break;

			case CHAR_TEX_METAL:	
				fvol = fWalking ? 0.2 : 0.5;
				break;

			case CHAR_TEX_DIRT:
				fvol = fWalking ? 0.25 : 0.55;
				break;

			case CHAR_TEX_VENT:	
				fvol = fWalking ? 0.4 : 0.7;
				break;

			case CHAR_TEX_GRATE:
				fvol = fWalking ? 0.2 : 0.5;
				break;

			case CHAR_TEX_TILE:	
				fvol = fWalking ? 0.2 : 0.5;
				break;

			case CHAR_TEX_SLOSH:
				fvol = fWalking ? 0.2 : 0.5;
				break;
			}
		}
		
		player->m_flStepSoundTime += flduck; // slower step time if ducking

		// play the sound
		// 35% volume if ducking
		if ( player->GetFlags() & FL_DUCKING )
		{
			fvol *= 0.35;
		}

		PlayStepSound( psurface, fvol, false );
	}
}

bool CGameMovement::IsDead( void ) const
{
	return ( player->m_iHealth <= 0 ) ? true : false;
}

float CGameMovement::GetSpeedCropFraction() const
{
	return 1;
}

//-----------------------------------------------------------------------------
// Figures out how the constraint should slow us down
//-----------------------------------------------------------------------------
float CGameMovement::ComputeConstraintSpeedFactor( void )
{
	// If we have a constraint, slow down because of that too.
	if ( !mv || mv->m_flConstraintRadius == 0.0f )
		return 1.0f;

	float flDistSq = mv->m_vecOrigin.DistToSqr( mv->m_vecConstraintCenter );

	float flOuterRadiusSq = mv->m_flConstraintRadius * mv->m_flConstraintRadius;
	float flInnerRadiusSq = mv->m_flConstraintRadius - mv->m_flConstraintWidth;
	flInnerRadiusSq *= flInnerRadiusSq;

	// Only slow us down if we're inside the constraint ring
	if ((flDistSq <= flInnerRadiusSq) || (flDistSq >= flOuterRadiusSq))
		return 1.0f;

	// Only slow us down if we're running away from the center
	Vector vecDesired;
	VectorMultiply( m_vecForward, mv->m_flForwardMove, vecDesired );
	VectorMA( vecDesired, mv->m_flSideMove, m_vecRight, vecDesired );
	VectorMA( vecDesired, mv->m_flUpMove, m_vecUp, vecDesired );

	Vector vecDelta;
	VectorSubtract( mv->m_vecOrigin, mv->m_vecConstraintCenter, vecDelta );
	VectorNormalize( vecDelta );
	VectorNormalize( vecDesired );
	if (DotProduct( vecDelta, vecDesired ) < 0.0f)
		return 1.0f;

	float flFrac = (sqrt(flDistSq) - (mv->m_flConstraintRadius - mv->m_flConstraintWidth)) / mv->m_flConstraintWidth;

	float flSpeedFactor = Lerp( flFrac, 1.0f, mv->m_flConstraintSpeedFactor ); 
	return flSpeedFactor;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::CheckParameters( void )
{
	QAngle	v_angle;

	if ( player->GetMoveType() != MOVETYPE_ISOMETRIC &&
		 player->GetMoveType() != MOVETYPE_NOCLIP )
	{
		float spd;
		float maxspeed;

		spd = ( mv->m_flForwardMove * mv->m_flForwardMove ) +
			  ( mv->m_flSideMove * mv->m_flSideMove ) +
			  ( mv->m_flUpMove * mv->m_flUpMove );
		spd = sqrt( spd );

		maxspeed = mv->m_flClientMaxSpeed;
		if ( maxspeed != 0.0 )
		{
			mv->m_flMaxSpeed = min( maxspeed, mv->m_flMaxSpeed );
		}

		// If we have a constraint, slow down because of that too.
		float flConstraintSpeedFactor = ComputeConstraintSpeedFactor();
		mv->m_flMaxSpeed *= flConstraintSpeedFactor;

		if ( ( spd != 0.0 ) &&
			 ( spd > mv->m_flMaxSpeed ) )
		{
			float fRatio = mv->m_flMaxSpeed / spd;
			mv->m_flForwardMove *= fRatio;
			mv->m_flSideMove    *= fRatio;
			mv->m_flUpMove      *= fRatio;
		}
	}

	if ( !m_bSpeedCropped && ( mv->m_nButtons & IN_SPEED ) && !(mv->m_nButtons & IN_DUCK))
	{
		//float frac = 0.47f;
		float frac = GetSpeedCropFraction();


		mv->m_flForwardMove *= frac;
		mv->m_flSideMove    *= frac;
		mv->m_flUpMove      *= frac;
		m_bSpeedCropped = true;
	}

	if ( player->GetFlags() & FL_FROZEN ||
		 player->GetFlags() & FL_ONTRAIN || 
		 IsDead() )
	{
		mv->m_flForwardMove = 0;
		mv->m_flSideMove    = 0;
		mv->m_flUpMove      = 0;
	}

	DecayPunchAngle();

	// Take angles from command.
	if ( !IsDead() )
	{
		v_angle = mv->m_vecAngles;
		v_angle = v_angle + player->m_Local.m_vecPunchAngle;

		// Now adjust roll angle
		if ( player->GetMoveType() != MOVETYPE_ISOMETRIC  &&
			 player->GetMoveType() != MOVETYPE_NOCLIP )
		{
			mv->m_vecAngles[ROLL]  = CalcRoll( v_angle, mv->m_vecVelocity, sv_rollangle.GetFloat(), sv_rollspeed.GetFloat() )*4;
		}
		else
		{
			mv->m_vecAngles[ROLL] = 0.0; // v_angle[ ROLL ];
		}
		mv->m_vecAngles[PITCH] = v_angle[PITCH];
		mv->m_vecAngles[YAW]   = v_angle[YAW];
	}
	else
	{
		mv->m_vecAngles = mv->m_vecOldAngles;
	}

	// Set dead player view_offset
	if ( IsDead() )
	{
		player->SetViewOffset( VEC_DEAD_VIEWHEIGHT );
	}

	// Adjust client view angles to match values used on server.
	if ( mv->m_vecAngles[YAW] > 180.0f )
	{
		mv->m_vecAngles[YAW] -= 360.0f;
	}
}

void CGameMovement::ReduceTimers( void )
{
	float frame_msec = 1000.0f * gpGlobals->frametime;

	if ( player->m_flStepSoundTime > 0 )
	{
		player->m_flStepSoundTime -= frame_msec;
		if ( player->m_flStepSoundTime < 0 )
		{
			player->m_flStepSoundTime = 0;
		}
	}
	if ( player->m_Local.m_flDucktime > 0 )
	{
		player->m_Local.m_flDucktime -= frame_msec;
		if ( player->m_Local.m_flDucktime < 0 )
		{
			player->m_Local.m_flDucktime = 0;
		}
	}
	if ( player->m_flSwimSoundTime > 0 )
	{
		player->m_flSwimSoundTime -= frame_msec;
		if ( player->m_flSwimSoundTime < 0 )
		{
			player->m_flSwimSoundTime = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMove - 
//-----------------------------------------------------------------------------
void CGameMovement::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove )
{
	// Cropping movement speed scales mv->m_fForwardSpeed etc. globally
	// Once we crop, we don't want to recursively crop again, so we set the crop
	//  flag globally here once per usercmd cycle.
	m_bSpeedCropped = false;

	// Run the recursive portion of the move
	_ProcessMovement( pPlayer, pMove );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *input - 
//			*output - 
//-----------------------------------------------------------------------------
void CGameMovement::_ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove )
{
	mv	= pMove;
	player = pPlayer;
	Assert( mv );
	Assert( player );

	// bisect time interval for very long commands
	if (gpGlobals->frametime > 0.05f )
	{
		float savet = gpGlobals->frametime;
		
		float t = gpGlobals->frametime * 0.5f;

		gpGlobals->frametime = t;
		
		_ProcessMovement( player, mv );

		// NOTE:  Only fire impulse on first time through
		mv->m_nImpulseCommand = 0;

		// Make sure frametime is valid
		gpGlobals->frametime = t;

		_ProcessMovement( player, mv );

		// Reset frametime so other functionas after this aren't hosed
		gpGlobals->frametime = savet;

		return;
	}

	mv->m_flMaxSpeed = sv_maxspeed.GetFloat();

	// Run the command.
	PlayerMove();

	FinishMove();
}

//-----------------------------------------------------------------------------
// Purpose: Sets ground entity
//-----------------------------------------------------------------------------
void CGameMovement::FinishMove( void )
{
	mv->m_nOldButtons = mv->m_nButtons;
	
	if( player->GetGroundEntity() != NULL )
	{
		player->AddFlag( FL_ONGROUND );
	}
	else
	{
		player->RemoveFlag( FL_ONGROUND );
	}
}

#define PUNCH_DAMPING		9.0f		// bigger number makes the response more damped, smaller is less damped
										// currently the system will overshoot, with larger damping values it won't
#define PUNCH_SPRING_CONSTANT	65.0f	// bigger number increases the speed at which the view corrects

//-----------------------------------------------------------------------------
// Purpose: Decays the punchangle toward 0,0,0.
//			Modelled as a damped spring
//-----------------------------------------------------------------------------
void CGameMovement::DecayPunchAngle( void )
{
	if ( player->m_Local.m_vecPunchAngle->LengthSqr() > 0.001 || player->m_Local.m_vecPunchAngleVel->LengthSqr() > 0.001 )
	{
		player->m_Local.m_vecPunchAngle += player->m_Local.m_vecPunchAngleVel * gpGlobals->frametime;
		float damping = 1 - (PUNCH_DAMPING * gpGlobals->frametime);
		
		if ( damping < 0 )
		{
			damping = 0;
		}
		player->m_Local.m_vecPunchAngleVel *= damping;
		 
		// torsional spring
		// UNDONE: Per-axis spring constant?
		float springForceMagnitude = PUNCH_SPRING_CONSTANT * gpGlobals->frametime;
		springForceMagnitude = clamp(springForceMagnitude, 0, 2 );
		player->m_Local.m_vecPunchAngleVel -= player->m_Local.m_vecPunchAngle * springForceMagnitude;

		// don't wrap around
		player->m_Local.m_vecPunchAngle.Init( 
			clamp(player->m_Local.m_vecPunchAngle->x, -89, 89 ), 
			clamp(player->m_Local.m_vecPunchAngle->y, -179, 179 ),
			clamp(player->m_Local.m_vecPunchAngle->z, -89, 89 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::StartGravity( void )
{
	float ent_gravity;
	
	if (player->GetGravity())
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity.GetFloat() * 0.5 * gpGlobals->frametime );
	mv->m_vecVelocity[2] += player->GetBaseVelocity()[2] * gpGlobals->frametime;

	Vector temp = player->GetBaseVelocity();
	temp[ 2 ] = 0;
	player->SetBaseVelocity( temp );

	CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::CheckWaterJump( void )
{
	Vector	flatforward;
	Vector forward;
	Vector	flatvelocity;
	float curspeed;

	AngleVectors( mv->m_vecViewAngles, &forward );  // Determine movement angles

	// Already water jumping.
	if (player->m_flWaterJumpTime)
		return;

	// Don't hop out if we just jumped in
	if (mv->m_vecVelocity[2] < -180)
		return; // only hop out if we are moving up

	// See if we are backing up
	flatvelocity[0] = mv->m_vecVelocity[0];
	flatvelocity[1] = mv->m_vecVelocity[1];
	flatvelocity[2] = 0;

	// Must be moving
	curspeed = VectorNormalize( flatvelocity );
	
	// see if near an edge
	flatforward[0] = forward[0];
	flatforward[1] = forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	// Are we backing into water from steps or something?  If so, don't pop forward
	if ( curspeed != 0.0 && ( DotProduct( flatvelocity, flatforward ) < 0.0 ) )
		return;

	Vector vecStart;
	// Start line trace at waist height (using the center of the player for this here)
	vecStart= mv->m_vecOrigin + (GetPlayerMins( player->m_Local.m_bDucked ) + GetPlayerMaxs( player->m_Local.m_bDucked )) * 0.5;
	// But raised to the predefined water jump height
	vecStart.z += WATERJUMP_HEIGHT; 

	Vector vecEnd;
	VectorMA( vecStart, 24, flatforward, vecEnd );
	
	trace_t tr;
	UTIL_TraceLine( vecStart, vecEnd, MASK_PLAYERSOLID_BRUSHONLY, mv->m_nPlayerHandle.Get(), COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction < 1.0 )		// solid at waist
	{
		vecStart.z += GetPlayerMaxs( player->m_Local.m_bDucked ).z - WATERJUMP_HEIGHT;
		VectorMA( vecStart, 24, flatforward, vecEnd );
		VectorMA( vec3_origin, -50, tr.plane.normal, player->m_vecWaterJumpVel );

		UTIL_TraceLine( vecStart, vecEnd, MASK_PLAYERSOLID_BRUSHONLY, mv->m_nPlayerHandle.Get(), COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction == 1.0 )		// open at eye level
		{
			mv->m_vecVelocity[2] = 260;			// Push up
			mv->m_nOldButtons |= IN_JUMP;		// Don't jump again until released
			player->AddFlag( FL_WATERJUMP );
			player->m_flWaterJumpTime = 2000;	// Do this for 2 seconds
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::WaterJump( void )
{
	if (player->m_flWaterJumpTime > 10000)
		player->m_flWaterJumpTime = 10000;

	if (!player->m_flWaterJumpTime)
		return;

	player->m_flWaterJumpTime -= 1000.0f * gpGlobals->frametime;

	if (player->m_flWaterJumpTime <= 0 || !player->GetWaterLevel())
	{
		player->m_flWaterJumpTime = 0;
		player->RemoveFlag( FL_WATERJUMP );
	}
	
	mv->m_vecVelocity[0] = player->m_vecWaterJumpVel[0];
	mv->m_vecVelocity[1] = player->m_vecWaterJumpVel[1];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::WaterMove( void )
{
	int		i;
	Vector	wishvel;
	float	wishspeed;
	Vector	wishdir;
	Vector	start, dest;
	Vector  temp;
	trace_t	pm;
	float speed, newspeed, addspeed, accelspeed;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	//
	// user intentions
	//
	for (i=0 ; i<3 ; i++)
		wishvel[i] = forward[i]*mv->m_flForwardMove + right[i]*mv->m_flSideMove;

	// if we have the jump key down, move us up as well
	if (mv->m_nButtons & IN_JUMP)
	{
		wishvel[2] += mv->m_flClientMaxSpeed;
	}
	// Sinking after no other movement occurs
	else if (!mv->m_flForwardMove && !mv->m_flSideMove && !mv->m_flUpMove)
	{
		wishvel[2] -= 60;		// drift towards bottom
	}
	else  // Go straight up by upmove amount.
	{
		wishvel[2] += mv->m_flUpMove;
	}

	// Copy it over and determine speed
	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	// Cap speed.
	if (wishspeed > mv->m_flMaxSpeed)
	{
		VectorScale (wishvel, mv->m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = mv->m_flMaxSpeed;
	}
	// Slow us down a bit.
	wishspeed *= 0.8;

	VectorAdd (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
	
	// Water friction
	VectorCopy(mv->m_vecVelocity, temp);
	speed = VectorNormalize(temp);
	if (speed)
	{
		newspeed = speed - gpGlobals->frametime * speed * sv_friction.GetFloat() * m_surfaceFriction;

		if (newspeed < 0)
			newspeed = 0;
		VectorScale (mv->m_vecVelocity, newspeed/speed, mv->m_vecVelocity);
	}
	else
	{
		newspeed = 0;
	}

	//
	// water acceleration
	//
	if (wishspeed < 0.1f)  // old !
		return;

	addspeed = wishspeed - newspeed;
	if (addspeed > 0)
	{
		VectorNormalize(wishvel);
		accelspeed = sv_accelerate.GetFloat() * wishspeed * gpGlobals->frametime * m_surfaceFriction;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		for (i = 0; i < 3; i++)
		{
			float deltaSpeed = accelspeed * wishvel[i];
			mv->m_vecVelocity[i] += deltaSpeed;
			mv->m_outWishVel[i] += deltaSpeed;
		}
	}

	// Now move
	// assume it is a stair or a slope, so press down from stepheight above
	VectorMA (mv->m_vecOrigin, gpGlobals->frametime, mv->m_vecVelocity, dest);
	VectorCopy (dest, start);
	if ( player->m_Local.m_bAllowAutoMovement )
	{
		start[2] += player->m_Local.m_flStepSize + 1;
	}
	
	TracePlayerBBox( start, dest, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );
	
	if (!pm.startsolid && !pm.allsolid)
	{	
		float stepDist = pm.endpos.z - mv->m_vecOrigin.z;
		mv->m_outStepHeight += stepDist;
		// walked up the step, so just keep result and exit
		VectorCopy (pm.endpos, mv->m_vecOrigin);
		return;
	}
	
	// Try moving straight along out normal path.
	TryPlayerMove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::Friction( void )
{
	float	speed, newspeed, control;
	float	friction;
	float	drop;
	Vector newvel;

	
	// If we are in water jump cycle, don't apply friction
	if (player->m_flWaterJumpTime)
		return;

	// Calculate speed
	speed = VectorLength( mv->m_vecVelocity );
	
	// If too slow, return
	if (speed < 0.1f)
	{
		return;
	}

	drop = 0;

	// apply ground friction
	if (player->GetGroundEntity() != NULL)  // On an entity that is the ground
	{
		Vector start, stop;
		trace_t pm;

		//
		// NOTE: added a "1.0f" to the player minimum (bbox) value so that the 
		//       trace starts just inside of the bounding box, this make sure
		//       that we don't get any collision epsilon (on surface) errors.
		//		 The significance of the 16 below is this is how many units out front we are checking
		//		 to see if the player box would fall.  The 49 is the number of units down that is required
		//		 to be considered a fall.  49 is derived from 1 (added 1 from above) + 48 the max fall 
		//		 distance a player can fall and still jump back up.
		//
		//		 UNDONE: In some cases there are still problems here.  Specifically, no collision check is
		//		 done so 16 units in front of the player could be inside a volume or past a collision point.
		start[0] = stop[0] = mv->m_vecOrigin[0] + (mv->m_vecVelocity[0]/speed)*16;
		start[1] = stop[1] = mv->m_vecOrigin[1] + (mv->m_vecVelocity[1]/speed)*16;
		start[2] = mv->m_vecOrigin[2] + ( GetPlayerMins(player->m_Local.m_bDucked)[2] + 1.0f );
		stop[2] = start[2] - 49;

		// This should *not* be a raytrace, it should be a box trace such that we can determine if the
		// player's box would fall off the ledge.  Ray tracing has problems associated with walking on rails
		// or on planks where a single ray would have the code believe the player is going to fall when in fact
		// they wouldn't.  The by product of this not working properly is that when a player gets to what
		// the code believes is an edge, the friction is bumped way up thus slowing the player down.
		// If not done properly, this kicks in way too often and forces big unintentional slowdowns.
		TracePlayerBBox( start, stop, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm ); 

		if (pm.fraction == 1.0)
			friction = sv_friction.GetFloat() * sv_edgefriction.GetFloat();
		else
			friction = sv_friction.GetFloat();

		// Grab friction value.
		friction *= m_surfaceFriction;  // player friction?

		// Bleed off some speed, but if we have less than the bleed
		//  threshhold, bleed the theshold amount.
		control = (speed < sv_stopspeed.GetFloat()) ?
			sv_stopspeed.GetFloat() : speed;

		// Add the amount to the drop amount.
		drop += control*friction*gpGlobals->frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	// Determine proportion of old speed we are using.
	newspeed /= speed;

	// Adjust velocity according to proportion.
	newvel[0] = mv->m_vecVelocity[0] * newspeed;
	newvel[1] = mv->m_vecVelocity[1] * newspeed;
	newvel[2] = mv->m_vecVelocity[2] * newspeed;

	VectorCopy( newvel, mv->m_vecVelocity );
 	mv->m_outWishVel -= (1.f-newspeed) * mv->m_vecVelocity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FinishGravity( void )
{
	float ent_gravity;

	if ( player->m_flWaterJumpTime )
		return;

	if ( player->GetGravity() )
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Get the correct velocity for the end of the dt 
  	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity.GetFloat() * gpGlobals->frametime * 0.5);

	CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wishdir - 
//			accel - 
//-----------------------------------------------------------------------------
void CGameMovement::AirAccelerate( Vector& wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;
	
	if (player->pl.deadflag)
		return;
	
	if (player->m_flWaterJumpTime)
		return;

	// Cap speed
	if (wishspd > 30)
		wishspd = 30;

	// Determine veer amount
	currentspeed = mv->m_vecVelocity.Dot(wishdir);

	// See how much to add
	addspeed = wishspd - currentspeed;

	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration
	accelspeed = accel * wishspeed * gpGlobals->frametime * m_surfaceFriction;

	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// Adjust pmove vel.
	for (i=0 ; i<3 ; i++)
	{
		mv->m_vecVelocity[i] += accelspeed * wishdir[i];
		mv->m_outWishVel[i] += accelspeed * wishdir[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::AirMove( void )
{
	int			i;
	Vector		wishvel;
	float		fmove, smove;
	Vector		wishdir;
	float		wishspeed;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles
	
	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;
	
	// Zero out z components of movement vectors
	forward[2] = 0;
	right[2]   = 0;
	VectorNormalize(forward);  // Normalize remainder of vectors
	VectorNormalize(right);    // 

	for (i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > mv->m_flMaxSpeed)
	{
		VectorScale (wishvel, mv->m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = mv->m_flMaxSpeed;
	}
	
	AirAccelerate( wishdir, wishspeed, sv_airaccelerate.GetFloat() );

	// Add in any base velocity to the current velocity.
	VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	TryPlayerMove();
}


bool CGameMovement::CanAccelerate()
{
	// Dead players don't accelerate.
	if (player->pl.deadflag)
		return false;

	// If waterjumping, don't accelerate
	if (player->m_flWaterJumpTime)
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wishdir - 
//			wishspeed - 
//			accel - 
//-----------------------------------------------------------------------------
void CGameMovement::Accelerate( Vector& wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed;

	// This gets overridden because some games (CSPort) want to allow dead (observer) players
	// to be able to move around.
	if ( !CanAccelerate() )
		return;

	// See if we are changing direction a bit
	currentspeed = mv->m_vecVelocity.Dot(wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of accleration.
	accelspeed = accel * gpGlobals->frametime * wishspeed * m_surfaceFriction;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// Adjust velocity.
	for (i=0 ; i<3 ; i++)
	{
		mv->m_vecVelocity[i] += accelspeed * wishdir[i];	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::WalkMove( void )
{
	int clip;
	int i;

	Vector wishvel;
	float spd;
	float fmove, smove;
	Vector wishdir;
	float wishspeed;

	Vector dest, start;
	Vector original, originalvel;
	Vector down, downvel;
	float downdist, updist;
	trace_t pm;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	CHandle< CBaseEntity > oldground;
	oldground = player->GetGroundEntity();
	
	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;
	
	// Zero out z components of movement vectors
	forward[2] = 0;
	right[2]   = 0;
	
	VectorNormalize (forward);  // Normalize remainder of vectors.
	VectorNormalize (right);    // 

	for (i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	
	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//
	if (wishspeed > mv->m_flMaxSpeed)
	{
		VectorScale (wishvel, mv->m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = mv->m_flMaxSpeed;
	}

	// Set pmove velocity
	mv->m_vecVelocity[2] = 0;
	Accelerate ( wishdir, wishspeed, sv_accelerate.GetFloat() );
	mv->m_vecVelocity[2] = 0;

	// Add in any base velocity to the current velocity.
	VectorAdd (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	spd = VectorLength( mv->m_vecVelocity );

	if (spd < 1.0f)
	{
		mv->m_vecVelocity.Init();
		return;
	}

	// first try just moving to the destination	
	dest[0] = mv->m_vecOrigin[0] + mv->m_vecVelocity[0]*gpGlobals->frametime;
	dest[1] = mv->m_vecOrigin[1] + mv->m_vecVelocity[1]*gpGlobals->frametime;	
	dest[2] = mv->m_vecOrigin[2];

	// first try moving directly to the next spot
	VectorCopy (dest, start);

	TracePlayerBBox( mv->m_vecOrigin, dest, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );

	// If we made it all the way, then copy trace end
	//  as new player position.

	mv->m_outWishVel += wishdir * wishspeed;

	if (pm.fraction == 1)
	{
		VectorCopy( pm.endpos, mv->m_vecOrigin );
		return;
	}

	if (oldground == NULL &&   // Don't walk up stairs if not on ground.
		player->GetWaterLevel()  == 0)
		return;

	if (player->m_flWaterJumpTime)         // If we are jumping out of water, don't do anything more.
		return;

	// Try sliding forward both on ground and up 16 pixels
	//  take the move that goes farthest
	VectorCopy (mv->m_vecOrigin, original);       // Save out original pos &
	VectorCopy (mv->m_vecVelocity, originalvel);  //  velocity.

	// Slide move
	clip = TryPlayerMove();

	// Copy the results out
	VectorCopy (mv->m_vecOrigin  , down);
	VectorCopy (mv->m_vecVelocity, downvel);

	// Reset original values.
	VectorCopy (original, mv->m_vecOrigin);
	VectorCopy (originalvel, mv->m_vecVelocity);

	// Start out up one stair height
	VectorCopy (mv->m_vecOrigin, dest);
	if ( player->m_Local.m_bAllowAutoMovement )
	{
		dest[2] += player->m_Local.m_flStepSize;
	}

	TracePlayerBBox( mv->m_vecOrigin, dest, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );

	// If we started okay and made it part of the way at least,
	//  copy the results to the movement start position and then
	//  run another move try.
	if (!pm.startsolid && !pm.allsolid)
	{
		VectorCopy (pm.endpos, mv->m_vecOrigin);
	}

	// slide move the rest of the way.
	clip = TryPlayerMove();

	// Now try going back down from the end point
	//  press down the stepheight
	VectorCopy (mv->m_vecOrigin, dest);
	if ( player->m_Local.m_bAllowAutoMovement )
	{
		dest[2] -= player->m_Local.m_flStepSize;
	}

	TracePlayerBBox( mv->m_vecOrigin, dest, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );

	// If we are not on the ground any more then
	//  use the original movement attempt
	if ( pm.plane.normal[2] < 0.7)
		goto usedown;
	// If the trace ended up in empty space, copy the end
	//  over to the origin.
	if (!pm.startsolid && !pm.allsolid)
	{
		VectorCopy (pm.endpos, mv->m_vecOrigin);
	}
	// Copy this origion to up.
	VectorCopy (mv->m_vecOrigin, up);

	// decide which one went farther
	downdist = (down[0] - original[0])*(down[0] - original[0])
		     + (down[1] - original[1])*(down[1] - original[1]);
	updist   = (up[0]   - original[0])*(up[0]   - original[0])
		     + (up[1]   - original[1])*(up[1]   - original[1]);

	if (downdist > updist)
	{
usedown:
	;
		VectorCopy (down   , mv->m_vecOrigin);
		VectorCopy (downvel, mv->m_vecVelocity);
	}
	else // copy z value from slide move
		mv->m_vecVelocity[2] = downvel[2];

	float stepDist = mv->m_vecOrigin.z - original.z;
	if ( stepDist > 0 )
	{
		mv->m_outStepHeight += stepDist;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bOnLadder - 
//-----------------------------------------------------------------------------
void CGameMovement::FullWalkMove( const bool bOnLadder )
{
	if ( !CheckWater() ) 
	{
		StartGravity();
	}

	// If we are leaping out of the water, just update the counters.
	if (player->m_flWaterJumpTime)
	{
		WaterJump();
		TryPlayerMove();
		// See if we are still in water?
		CheckWater();
		return;
	}

	// If we are swimming in the water, see if we are nudging against a place we can jump up out
	//  of, and, if so, start out jump.  Otherwise, if we are not moving up, then reset jump timer to 0
	if ( player->GetWaterLevel() >= WL_Waist ) 
	{
		if ( player->GetWaterLevel() == WL_Waist )
		{
			CheckWaterJump();
		}

			// If we are falling again, then we must not trying to jump out of water any more.
		if ( mv->m_vecVelocity[2] < 0 && 
			 player->m_flWaterJumpTime )
		{
			player->m_flWaterJumpTime = 0;
		}

		// Was jump button pressed?
		if (mv->m_nButtons & IN_JUMP)
		{
			CheckJumpButton();
		}
		else
		{
			mv->m_nOldButtons &= ~IN_JUMP;
		}

		// Perform regular water movement
		WaterMove();
		VectorSubtract (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

		// Redetermine position vars
		CategorizePosition();
	}
	else
	// Not fully underwater
	{
		// Was jump button pressed?
		if (mv->m_nButtons & IN_JUMP)
		{
			if (!bOnLadder)
			{
				CheckJumpButton();
			}
		}
		else
		{
			mv->m_nOldButtons &= ~IN_JUMP;
		}

		// Fricion is handled before we add in any base velocity. That way, if we are on a conveyor, 
		//  we don't slow when standing still, relative to the conveyor.
		if (player->GetGroundEntity() != NULL)
		{
			mv->m_vecVelocity[2] = 0.0;
			Friction();
		}

		// Make sure velocity is valid.
		CheckVelocity();

		if (player->GetGroundEntity() != NULL)
		{
			WalkMove();
		}
		else
		{
			AirMove();  // Take into account movement when in air.
		}

		// Set final flags.
		CategorizePosition();

		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like
		//  a conveyor (or maybe another monster?)
		VectorSubtract (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

		// Make sure velocity is valid.
		CheckVelocity();

		// Add any remaining gravitational component.
		if ( !CheckWater() )
		{
			FinishGravity();
		}

		// If we are on ground, no downward velocity.
		if ( player->GetGroundEntity() != NULL )
		{
			mv->m_vecVelocity[2] = 0;
		}
		CheckFalling();
	}

	if  ( ( m_nOldWaterLevel == WL_NotInWater && player->GetWaterLevel() != WL_NotInWater ) ||
		  ( m_nOldWaterLevel != WL_NotInWater && player->GetWaterLevel() == WL_NotInWater ) )
	{
		PlaySwimSound();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FullObserverMove( void )
{
	int mode = player->GetObserverMode();
	CBaseEntity * target = player->GetObserverTarget();

	if ( mode <= OBS_MODE_FIXED || mode == OBS_MODE_ROAMING || !target)
	{
		return; //don't move
	}

	mv->m_vecOrigin = target->EyePosition();
	mv->m_vecViewAngles = target->EyeAngles();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FullNoClipMove( void )
{
	int i;
	Vector wishvel;
	float fmove, smove;
	Vector forward, right, up;
	Vector wishdir;
	float wishspeed;
	float spd;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles
	
	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;
	
	VectorNormalize (forward);  // Normalize remainder of vectors
	VectorNormalize (right);    // 

	for (i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] += mv->m_flUpMove;

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//
	if (wishspeed > sv_noclipspeed.GetFloat() )
	{
		VectorScale (wishvel, sv_noclipspeed.GetFloat()/wishspeed, wishvel);
		wishspeed = sv_noclipspeed.GetFloat();
	}

	if ( sv_noclipaccelerate.GetFloat() > 0.0 )
	{
		// Set pmove velocity
		Accelerate ( wishdir, wishspeed, sv_noclipaccelerate.GetFloat() );

		spd = VectorLength( mv->m_vecVelocity );
		if (spd < 1.0f)
		{
			mv->m_vecVelocity.Init();
			return;
		}
		
		if ( 1 )
		{
			float friction;
			float newspeed;
			float control;
			float drop = 0.0;

			friction = sv_friction.GetFloat() * m_surfaceFriction;
					
			// Bleed off some speed, but if we have less than the bleed
			//  threshhold, bleed the theshold amount.
			control = (spd < sv_noclipspeed.GetFloat()/4.0) ? sv_noclipspeed.GetFloat()/4.0 : spd;
			
			// Add the amount to the drop amount.
			drop = control * friction * gpGlobals->frametime;

			// scale the velocity
			newspeed = spd - drop;
			if (newspeed < 0)
				newspeed = 0;

			// Determine proportion of old speed we are using.
			newspeed /= spd;

			VectorScale( mv->m_vecVelocity, newspeed, mv->m_vecVelocity );
		}
	}
	else
	{
		VectorCopy( wishvel, mv->m_vecVelocity );
	}

	// Just move ( don't clip or anything )
	VectorMA( mv->m_vecOrigin, gpGlobals->frametime, mv->m_vecVelocity, mv->m_vecOrigin );

	// Zero out velocity if in noaccel mode
	if ( sv_noclipaccelerate.GetFloat() < 0.0 )
	{
		mv->m_vecVelocity.Init();
	}
}


//-----------------------------------------------------------------------------
// Checks to see if we should actually jump 
//-----------------------------------------------------------------------------
void CGameMovement::PlaySwimSound()
{
	MoveHelper()->StartSound( mv->m_vecOrigin, "Player.Swim" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::CheckJumpButton( void )
{
	if (player->pl.deadflag)
	{
		mv->m_nOldButtons |= IN_JUMP ;	// don't jump again until released
		return;
	}

	// See if we are waterjumping.  If so, decrement count and return.
	if (player->m_flWaterJumpTime)
	{
		player->m_flWaterJumpTime -= gpGlobals->frametime;
		if (player->m_flWaterJumpTime < 0)
			player->m_flWaterJumpTime = 0;
		
		return;
	}

	// If we are in the water most of the way...
	if ( player->GetWaterLevel() >= 2 )
	{	
		// swimming, not jumping
		player->SetGroundEntity( (CBaseEntity *)NULL );

		if(player->GetWaterType() == CONTENTS_WATER)    // We move up a certain amount
			mv->m_vecVelocity[2] = 100;
		else if (player->GetWaterType() == CONTENTS_SLIME)
			mv->m_vecVelocity[2] = 80;
		
		// play swiming sound
		if ( player->m_flSwimSoundTime <= 0 )
		{
			// Don't play sound again for 1 second
			player->m_flSwimSoundTime = 1000;
			PlaySwimSound();
		}

		return;
	}

	// No more effect
 	if (player->GetGroundEntity() == NULL)
	{
		mv->m_nOldButtons |= IN_JUMP;
		return;		// in air, so no effect
	}

	if ( mv->m_nOldButtons & IN_JUMP )
		return;		// don't pogo stick

	// In the air now.
    player->SetGroundEntity( (CBaseEntity *)NULL );
	
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
	if ( (  player->m_Local.m_bDucking ) || (  player->GetFlags() & FL_DUCKING ) )
	{
		// d = 0.5 * g * t^2		- distance traveled with linear accel
		// t = sqrt(2.0 * 45 / g)	- how long to fall 45 units
		// v = g * t				- velocity at the end (just invert it to jump up that high)
		// v = g * sqrt(2.0 * 45 / g )
		// v^2 = g * g * 2.0 * 45 / g
		// v = sqrt( g * 2.0 * 45 )
		mv->m_vecVelocity[2] = flGroundFactor * sqrt(2 * 800 * 45.0);  // 2 * gravity * height
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


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bOnLadder - 
//-----------------------------------------------------------------------------
void CGameMovement::FullLadderMove( bool bOnLadder )
{
	CheckWater();

	// Was jump button pressed?
	// If so, set velocity to 270 away from ladder.  This is currently wrong.
	// Also, set MOVE_TYPE to walk, too.
	if(mv->m_nButtons & IN_JUMP)
	{
		if(!bOnLadder)
		{
			CheckJumpButton();
		}
	}
	else
	{
		mv->m_nOldButtons &= ~IN_JUMP;
	}
	
	// Perform the move accounting for any base velocity.
	VectorAdd (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
	TryPlayerMove();
	VectorSubtract (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CGameMovement::TryPlayerMove( void )
{
	int			bumpcount, numbumps;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[MAX_CLIP_PLANES];
	Vector		primal_velocity, original_velocity;
	Vector      new_velocity;
	int			i, j;
	trace_t	pm;
	Vector		end;
	float		time_left, allFraction;
	int			blocked;		
	
	numbumps  = 4;           // Bump up to four times
	
	blocked   = 0;           // Assume not blocked
	numplanes = 0;           //  and not sliding along any planes

	VectorCopy (mv->m_vecVelocity, original_velocity);  // Store original velocity
	VectorCopy (mv->m_vecVelocity, primal_velocity);
	
	allFraction = 0;
	time_left = gpGlobals->frametime;   // Total time for this movement operation.

	new_velocity.Init();

	for (bumpcount=0 ; bumpcount < numbumps; bumpcount++)
	{
		if ( mv->m_vecVelocity.Length() == 0.0 )
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		for (i=0 ; i<3 ; i++)
			end[i] = mv->m_vecOrigin[i] + time_left * mv->m_vecVelocity[i];

		// See if we can make it from origin to end point.
		TracePlayerBBox( mv->m_vecOrigin, end, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );

		allFraction += pm.fraction;

		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if (pm.allsolid)
		{	
			// entity is trapped in another solid
			VectorCopy (vec3_origin, mv->m_vecVelocity);
			return 4;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove.origin and 
		//  zero the plane counter.
		if( pm.fraction > 0 )
		{	
			// actually covered some distance
			VectorCopy (pm.endpos, mv->m_vecOrigin);
			VectorCopy (mv->m_vecVelocity, original_velocity);
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if (pm.fraction == 1)
		{
			 break;		// moved the entire distance
		}

		// Save entity that blocked us (since fraction was < 1.0)
		//  for contact
		// Add it if it's not already in the list!!!
		MoveHelper( )->AddToTouched( pm, mv->m_vecVelocity );

		// If the plane we hit has a high z component in the normal, then
		//  it's probably a floor
		if (pm.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
		}
		// If the plane has a zero z component in the normal, then it's a 
		//  step or wall
		if (!pm.plane.normal[2])
		{
			blocked |= 2;		// step / wall
		}

		// Reduce amount of m_flFrameTime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * pm.fraction;

		// Did we run out of planes to clip against?
		if (numplanes >= MAX_CLIP_PLANES)
		{	
			// this shouldn't really happen
			//  Stop our movement if so.
			VectorCopy (vec3_origin, mv->m_vecVelocity);
			//Con_DPrintf("Too many planes 4\n");

			break;
		}

		// Set up next clipping plane
		VectorCopy (pm.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		//

		// relfect player velocity 
		// Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
		//  and pressing forward and nobody was really using this bounce/reflection feature anyway...
		if ( numplanes == 1 &&
			player->GetMoveType() == MOVETYPE_WALK &&
			player->GetGroundEntity() == NULL )	
		{
			for ( i = 0; i < numplanes; i++ )
			{
				if ( planes[i][2] > 0.7  )
				{
					// floor or slope
					ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
					VectorCopy( new_velocity, original_velocity );
				}
				else
				{
					ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + sv_bounce.GetFloat() * (1 - m_surfaceFriction) );
				}
			}

			VectorCopy( new_velocity, mv->m_vecVelocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
			for (i=0 ; i < numplanes ; i++)
			{
				ClipVelocity (
					original_velocity,
					planes[i],
					mv->m_vecVelocity,
					1);

				for (j=0 ; j<numplanes ; j++)
					if (j != i)
					{
						// Are we now moving against this plane?
						if (mv->m_vecVelocity.Dot(planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)  // Didn't have to clip, so we're ok
					break;
			}
			
			// Did we go all the way through plane set
			if (i != numplanes)
			{	// go along this plane
				// pmove.velocity is set in clipping call, no need to set again.
				;  
			}
			else
			{	// go along the crease
				if (numplanes != 2)
				{
					VectorCopy (vec3_origin, mv->m_vecVelocity);
					break;
				}
				CrossProduct (planes[0], planes[1], dir);
				d = dir.Dot(mv->m_vecVelocity);
				VectorScale (dir, d, mv->m_vecVelocity );
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			d = mv->m_vecVelocity.Dot(primal_velocity);
			if (d <= 0)
			{
				//Con_DPrintf("Back\n");
				VectorCopy (vec3_origin, mv->m_vecVelocity);
				break;
			}
		}
	}

	if ( allFraction == 0 )
	{
		VectorCopy (vec3_origin, mv->m_vecVelocity);
	}

	return blocked;
}


//-----------------------------------------------------------------------------
// Purpose: Determine whether or not the player is on a ladder (physprop or world).
//-----------------------------------------------------------------------------
inline bool CGameMovement::OnLadder( trace_t &trace )
{
	if ( trace.contents & CONTENTS_LADDER )
		return true;

	IPhysicsSurfaceProps *pPhysProps = MoveHelper( )->GetSurfaceProps();
	if ( pPhysProps )
	{
		surfacedata_t *pSurfaceData = pPhysProps->GetSurfaceData( trace.surface.surfaceProps );
		if ( pSurfaceData )
		{
			if ( pSurfaceData->climbable != 0.0f )
				return true;
		}
	}

	return false;
}

// For some reason, if optimizations are on, then after this part:
	// decompose velocity into ladder plane
	// normal = DotProduct( velocity, trace.plane.normal );
// the value of normal gets trashed.
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameMovement::LadderMove( void )
{
	trace_t pm;
	bool onFloor;
	Vector floor;
	Vector wishdir;
	Vector end;

	if ( player->GetMoveType() == MOVETYPE_NOCLIP )
		return false;

	// If I'm already moving on a ladder, use the previous ladder direction
	if ( player->GetMoveType() == MOVETYPE_LADDER )
	{
		wishdir = -player->m_vecLadderNormal;
	}
	else
	{
		// otherwise, use the direction player is attempting to move
		if ( mv->m_flForwardMove || mv->m_flSideMove )
		{
			for (int i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
				wishdir[i] = m_vecForward[i]*mv->m_flForwardMove + m_vecRight[i]*mv->m_flSideMove;

			VectorNormalize(wishdir);
		}
		else
		{
			// Player is not attempting to move, no ladder behavior
			return false;
		}
	}

	// wishdir points toward the ladder if any exists
	VectorMA( mv->m_vecOrigin, 2, wishdir, end );
	TracePlayerBBox( mv->m_vecOrigin, end, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );

	// no ladder in that direction, return
	if ( pm.fraction == 1.0f || !OnLadder( pm ) )
		return false;
//	if ( pm.fraction == 1.0 || !(pm.contents & CONTENTS_LADDER ) )
//		return false;

	player->SetMoveType( MOVETYPE_LADDER );
	player->SetMoveCollide( MOVECOLLIDE_DEFAULT );

	player->m_vecLadderNormal = pm.plane.normal;

	// On ladder, convert movement to be relative to the ladder

	VectorCopy( mv->m_vecOrigin, floor );
	floor[2] += GetPlayerMins( player->m_Local.m_bDucked )[2] - 1;

	if( enginetrace->GetPointContents( floor ) == CONTENTS_SOLID )
	{
		onFloor = true;
	}
	else
	{
		onFloor = false;
	}

	player->SetGravity( 0 );
	
	float forwardSpeed = 0, rightSpeed = 0;
	if ( mv->m_nButtons & IN_BACK )
		forwardSpeed -= MAX_CLIMB_SPEED;
	
	if ( mv->m_nButtons & IN_FORWARD )
		forwardSpeed += MAX_CLIMB_SPEED;
	
	if ( mv->m_nButtons & IN_MOVELEFT )
		rightSpeed -= MAX_CLIMB_SPEED;
	
	if ( mv->m_nButtons & IN_MOVERIGHT )
		rightSpeed += MAX_CLIMB_SPEED;

	if ( mv->m_nButtons & IN_JUMP )
	{
		player->SetMoveType( MOVETYPE_WALK );
		player->SetMoveCollide( MOVECOLLIDE_DEFAULT );

		VectorScale( pm.plane.normal, 270, mv->m_vecVelocity );
	}
	else
	{
		if ( forwardSpeed != 0 || rightSpeed != 0 )
		{
			Vector velocity, perp, cross, lateral, tmp;

			//ALERT(at_console, "pev %.2f %.2f %.2f - ",
			//	pev->velocity.x, pev->velocity.y, pev->velocity.z);
			// Calculate player's intended velocity
			//Vector velocity = (forward * gpGlobals->v_forward) + (right * gpGlobals->v_right);
			VectorScale( m_vecForward, forwardSpeed, velocity );
			VectorMA( velocity, rightSpeed, m_vecRight, velocity );

			// Perpendicular in the ladder plane
			VectorCopy( vec3_origin, tmp );
			tmp[2] = 1;
			CrossProduct( tmp, pm.plane.normal, perp );
			VectorNormalize( perp );

			// decompose velocity into ladder plane
			float normal = DotProduct( velocity, pm.plane.normal );

			// This is the velocity into the face of the ladder
			VectorScale( pm.plane.normal, normal, cross );

			// This is the player's additional velocity
			VectorSubtract( velocity, cross, lateral );

			// This turns the velocity into the face of the ladder into velocity that
			// is roughly vertically perpendicular to the face of the ladder.
			// NOTE: It IS possible to face up and move down or face down and move up
			// because the velocity is a sum of the directional velocity and the converted
			// velocity through the face of the ladder -- by design.
			CrossProduct( pm.plane.normal, perp, tmp );
			VectorMA( lateral, -normal, tmp, mv->m_vecVelocity );

			if ( onFloor && normal > 0 )	// On ground moving away from the ladder
			{
				VectorMA( mv->m_vecVelocity, MAX_CLIMB_SPEED, pm.plane.normal, mv->m_vecVelocity );
			}
			//pev->velocity = lateral - (CrossProduct( trace.vecPlaneNormal, perp ) * normal);
		}
		else
		{
			mv->m_vecVelocity.Init();
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : axis - 
// Output : const char
//-----------------------------------------------------------------------------
const char *DescribeAxis( int axis )
{
	static char sz[ 32 ];

	switch ( axis )
	{
	case 0:
		strcpy( sz, "X" );
		break;
	case 1:
		strcpy( sz, "Y" );
		break;
	case 2:
	default:
		strcpy( sz, "Z" );
		break;
	}

	return sz;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::CheckVelocity( void )
{
	int i;

	//
	// bound velocity
	//
	for (i=0; i < 3; i++)
	{
		// See if it's bogus.
		if (IS_NAN(mv->m_vecVelocity[i]))
		{
			DevMsg( 1, "PM  Got a NaN velocity %s\n", DescribeAxis( i ) );
			mv->m_vecVelocity[i] = 0;
		}
		if (IS_NAN(mv->m_vecOrigin[i]))
		{
			DevMsg( 1, "PM  Got a NaN origin on %s\n", DescribeAxis( i ) );
			mv->m_vecOrigin[i] = 0;
		}

		// Bound it.
		if (mv->m_vecVelocity[i] > sv_maxvelocity.GetFloat()) 
		{
			DevMsg( 1, "PM  Got a velocity too high on %s\n", DescribeAxis( i ) );
			mv->m_vecVelocity[i] = sv_maxvelocity.GetFloat();
		}
		else if (mv->m_vecVelocity[i] < -sv_maxvelocity.GetFloat())
		{
			DevMsg( 1, "PM  Got a velocity too low on %s\n", DescribeAxis( i ) );
			mv->m_vecVelocity[i] = -sv_maxvelocity.GetFloat();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::AddGravity( void )
{
	float ent_gravity;

	if ( player->m_flWaterJumpTime )
		return;

	if (player->GetGravity())
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Add gravity incorrectly
	mv->m_vecVelocity[2] -= (ent_gravity * sv_gravity.GetFloat() * gpGlobals->frametime);
	mv->m_vecVelocity[2] += player->GetBaseVelocity()[2] * gpGlobals->frametime;
	Vector temp = player->GetBaseVelocity();
	temp[2] = 0;
	player->SetBaseVelocity( temp );
	
	CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : push - 
// Output : trace_t
//-----------------------------------------------------------------------------
trace_t CGameMovement::PushEntity( Vector& push )
{
	trace_t pm;
	Vector	end;
		
	VectorAdd (mv->m_vecOrigin, push, end);
	TracePlayerBBox( mv->m_vecOrigin, end, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );
	VectorCopy (pm.endpos, mv->m_vecOrigin);

	// So we can run impact function afterwards.
	// If
	if (pm.fraction < 1.0 && !pm.allsolid)
	{
		MoveHelper( )->AddToTouched( pm,mv->m_vecVelocity );
	}

	return pm;
}	

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : in - 
//			normal - 
//			out - 
//			overbounce - 
// Output : int
//-----------------------------------------------------------------------------
int CGameMovement::ClipVelocity( Vector& in, Vector& normal, Vector& out, float overbounce )
{
	float	backoff;
	float	change;
	float angle;
	int		i, blocked;
	
	angle = normal[ 2 ];

	blocked = 0x00;         // Assume unblocked.
	if (angle > 0)			// If the plane that is blocking us has a positive z component, then assume it's a floor.
		blocked |= 0x01;	// 
	if (!angle)				// If the plane has no Z, it is vertical (wall/step)
		blocked |= 0x02;	// 
	

	// Determine how far along plane to slide based on incoming direction.
	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change; 
		// If out velocity is too small, zero it out.
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
	
#if 0
	// slight adjustment - hopefully to adjust for displacement surfaces
	float adjust = DotProduct( out, normal );
	if( adjust < 0.0f )
	{
		out += ( normal * -adjust );
//		Msg( "Adjustment = %lf\n", adjust );
	}
#endif

	// Return blocking flags.
	return blocked;
}

//-----------------------------------------------------------------------------
// Purpose: Computes roll angle for a certain movement direction and velocity
// Input  : angles - 
//			velocity - 
//			rollangle - 
//			rollspeed - 
// Output : float 
//-----------------------------------------------------------------------------
float CGameMovement::CalcRoll ( const QAngle &angles, const Vector &velocity, float rollangle, float rollspeed )
{
	float   sign;
	float   side;
	float   value;
	Vector  forward, right, up;
	
	AngleVectors (angles, &forward, &right, &up);
	
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);
	value = rollangle;
	if (side < rollspeed)
	{
		side = side * value / rollspeed;
	}
	else
	{
		side = value;
	}
	return side*sign;
}

#define CHECKSTUCK_MINTIME 0.05  // Don't check again too quickly.

static Vector rgv3tStuckTable[54];
static int rgStuckLast[32][2];

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CreateStuckTable( void )
{
	float x, y, z;
	int idx;
	int i;
	float zi[3];
	static int firsttime = 1;

	if ( !firsttime )
		return;

	firsttime = 0;

	memset(rgv3tStuckTable, 0, 54 * sizeof(Vector));

	idx = 0;
	// Little Moves.
	x = y = 0;
	// Z moves
	for (z = -0.125 ; z <= 0.125 ; z += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	x = z = 0;
	// Y moves
	for (y = -0.125 ; y <= 0.125 ; y += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -0.125 ; x <= 0.125 ; x += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for ( x = - 0.125; x <= 0.125; x += 0.250 )
	{
		for ( y = - 0.125; y <= 0.125; y += 0.250 )
		{
			for ( z = - 0.125; z <= 0.125; z += 0.250 )
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}

	// Big Moves.
	x = y = 0;
	zi[0] = 0.0f;
	zi[1] = 1.0f;
	zi[2] = 6.0f;

	for (i = 0; i < 3; i++)
	{
		// Z moves
		z = zi[i];
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	x = z = 0;

	// Y moves
	for (y = -2.0f ; y <= 2.0f ; y += 2.0)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -2.0f ; x <= 2.0f ; x += 2.0f)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for (i = 0 ; i < 3; i++)
	{
		z = zi[i];
		
		for (x = -2.0f ; x <= 2.0f ; x += 2.0f)
		{
			for (y = -2.0f ; y <= 2.0f ; y += 2.0)
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//			server - 
//			offset - 
// Output : int
//-----------------------------------------------------------------------------
int GetRandomStuckOffsets(int nIndex, int server, Vector& offset)
{
 // Last time we did a full
	int idx;
	idx = rgStuckLast[nIndex][server]++;

	VectorCopy(rgv3tStuckTable[idx % 54], offset);

	return (idx % 54);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//			server - 
//-----------------------------------------------------------------------------
void ResetStuckOffsets(int nIndex, int server)
{
	rgStuckLast[nIndex][server] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
// Output : int
//-----------------------------------------------------------------------------
int CGameMovement::CheckStuck( void )
{
	Vector base;
	Vector offset;
	Vector test;
	EntityHandle_t hitent;
	int idx;
	float fTime;
	int i;
	trace_t traceresult;

	static float rgStuckCheckTime[32][2]; // Last time we did a full

	CreateStuckTable();

	hitent = TestPlayerPosition( mv->m_vecOrigin, COLLISION_GROUP_PLAYER_MOVEMENT, traceresult );
	if ( hitent == INVALID_ENTITY_HANDLE )
	{
		ResetStuckOffsets( player->entindex(), player->IsServer() );
		return 0;
	}

	// Deal with stuckness...

	MoveHelper()->Con_NPrintf( player->IsServer() ? 1 : 0, "%s stuck on object %i/%s", 
		player->IsServer() ? "server" : "client",
		hitent,
		MoveHelper()->GetName(hitent) );

	VectorCopy( mv->m_vecOrigin, base );

	// 
	// Deal with precision error in network.
	// 
	// World or BSP model
	if ( !player->IsServer() )
	{
		if ( MoveHelper()->IsWorldEntity( hitent ) )
		{
			int nReps = 0;
			ResetStuckOffsets(player->entindex(), player->IsServer());
			do 
			{
				i = GetRandomStuckOffsets(player->entindex(), player->IsServer(), offset);

				VectorAdd(base, offset, test);
				
				if (TestPlayerPosition( test, COLLISION_GROUP_PLAYER_MOVEMENT, traceresult ) == INVALID_ENTITY_HANDLE)
				{
					ResetStuckOffsets(player->entindex(), player->IsServer());
					VectorCopy(test, mv->m_vecOrigin);
					return 0;
				}
				nReps++;
			} while (nReps < 54);
		}
	}

	// Only an issue on the client.
	idx = player->IsServer() ? 0 : 1;

	fTime = engine->Time();
	// Too soon?
	if ( rgStuckCheckTime[player->entindex()][idx] >=  fTime - CHECKSTUCK_MINTIME )
	{
		return 1;
	}
	rgStuckCheckTime[player->entindex()][idx] = fTime;

	MoveHelper( )->AddToTouched( traceresult, mv->m_vecVelocity );

	i = GetRandomStuckOffsets(player->entindex(), player->IsServer(), offset);

	VectorAdd(base, offset, test);

	if (TestPlayerPosition( test, COLLISION_GROUP_PLAYER_MOVEMENT, traceresult ) == INVALID_ENTITY_HANDLE)
	{
		ResetStuckOffsets(player->entindex(), player->IsServer());

		if ( i >= 27 )
		{
			VectorCopy(test, mv->m_vecOrigin);
		}
		return 0;
	}

	/*
	// If player is flailing while stuck in another player ( should never happen ), then see
	//  if we can't "unstick" them forceably.
	if ( mv->m_nButtons & ( IN_JUMP | IN_DUCK | IN_ATTACK ) && ( pmv->physents[ hitent ].player != 0 ) )
	{
		float x, y, z;
		float xystep = 8.0;
		float zstep = 18.0;
		float xyminmax = xystep;
		float zminmax = 4 * zstep;
		
		for ( z = 0; z <= zminmax; z += zstep )
		{
			for ( x = -xyminmax; x <= xyminmax; x += xystep )
			{
				for ( y = -xyminmax; y <= xyminmax; y += xystep )
				{
					VectorCopy( base, test );
					test[0] += x;
					test[1] += y;
					test[2] += z;

					if ( pmv->TestPosition ( test, NULL ) == -1 )
					{
						VectorCopy( test, mv->m_vecOrigin );
						return 0;
					}
				}
			}
		}
	}
	*/
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : bool
//-----------------------------------------------------------------------------
bool CGameMovement::InWater( void )
{
	return ( player->GetWaterLevel() > WL_Feet );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
// Output : bool
//-----------------------------------------------------------------------------
bool CGameMovement::CheckWater( void )
{
	Vector	point;
	int		cont;

	// Pick a spot just above the players feet.
	point[0] = mv->m_vecOrigin[0] + (GetPlayerMins( player->m_Local.m_bDucked )[0] + GetPlayerMaxs( player->m_Local.m_bDucked )[0]) * 0.5;
	point[1] = mv->m_vecOrigin[1] + (GetPlayerMins( player->m_Local.m_bDucked )[1] + GetPlayerMaxs( player->m_Local.m_bDucked )[1]) * 0.5;
	point[2] = mv->m_vecOrigin[2] + GetPlayerMins( player->m_Local.m_bDucked )[2] + 1;
	
	// Assume that we are not in water at all.
	player->SetWaterLevel( WL_NotInWater );
	player->SetWaterType( CONTENTS_EMPTY );

	// Grab point contents.
	cont = enginetrace->GetPointContents ( point );
	
	// Are we under water? (not solid and not empty?)
	if ( cont & MASK_WATER )
	{
		// Set water type
		player->SetWaterType( cont );

		// We are at least at level one
		player->SetWaterLevel( WL_Feet );

		// Now check a point that is at the player hull midpoint.
		point[2] = mv->m_vecOrigin[2] + (GetPlayerMins( player->m_Local.m_bDucked )[2] + GetPlayerMaxs( player->m_Local.m_bDucked )[2])*0.5;
		cont = enginetrace->GetPointContents( point );
		// If that point is also under water...
		if ( cont & MASK_WATER )
		{
			// Set a higher water level.
			player->SetWaterLevel( WL_Waist );

			// Now check the eye position.  (view_ofs is relative to the origin)
			point[2] = mv->m_vecOrigin[2] + player->GetViewOffset()[2];
			cont = enginetrace->GetPointContents( point );
			if ( cont & MASK_WATER )
				player->SetWaterLevel( WL_Eyes );  // In over our eyes
		}

		// Adjust velocity based on water current, if any.
		if ( cont & MASK_CURRENT )
		{
			Vector v;
			VectorClear(v);
			if ( cont & CONTENTS_CURRENT_0 )
				v[0] += 1;
			if ( cont & CONTENTS_CURRENT_90 )
				v[1] += 1;
			if ( cont & CONTENTS_CURRENT_180 )
				v[0] -= 1;
			if ( cont & CONTENTS_CURRENT_270 )
				v[1] -= 1;
			if ( cont & CONTENTS_CURRENT_UP )
				v[2] += 1;
			if ( cont & CONTENTS_CURRENT_DOWN )
				v[2] -= 1;

			// BUGBUG -- this depends on the value of an unspecified enumerated type
			// The deeper we are, the stronger the current.
			Vector temp;
			VectorMA( player->GetBaseVelocity(), 50.0*player->GetWaterLevel(), v, temp );
			player->SetBaseVelocity( temp );
		}
	}

	return ( player->GetWaterLevel() > WL_Feet );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
//-----------------------------------------------------------------------------
void CGameMovement::CategorizePosition( void )
{
	Vector point;
	trace_t pm;

	
	// if the player hull point one unit down is solid, the player
	// is on ground
	
	// see if standing on something solid	

	// Doing this before we move may introduce a potential latency in water detection, but
	// doing it after can get us stuck on the bottom in water if the amount we move up
	// is less than the 1 pixel 'threshold' we're about to snap to.	Also, we'll call
	// this several times per frame, so we really need to avoid sticking to the bottom of
	// water on each call, and the converse case will correct itself if called twice.
	CheckWater();

	point[0] = mv->m_vecOrigin[0];
	point[1] = mv->m_vecOrigin[1];
//	point[2] = mv->m_vecOrigin[2] - 2;
	point[2] = mv->m_vecOrigin[2] - 2;		// move a total of 4 units to try and avoid some
	                                        // epsilon error

	Vector bumpOrigin;
	bumpOrigin = mv->m_vecOrigin;
	bumpOrigin.z += 2;

	if ( mv->m_vecVelocity[2] > 180)   // Shooting up really fast.  Definitely not on ground.
	{
		player->SetGroundEntity( (CBaseEntity *)NULL );
	}
	else
	{
		// Try and move down.
		TracePlayerBBox( bumpOrigin, point, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		
		// Moving up two units got us stuck in something, start tracing down exactly at our
		//  current origin (since CheckStuck allowed us to get here, that pos is valid)
		if ( pm.startsolid )
		{
			bumpOrigin = mv->m_vecOrigin;
			TracePlayerBBox( bumpOrigin, point, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		}

		// If we hit a steep plane, we are not on ground
		if ( pm.plane.normal[2] < 0.7)
		{
			player->SetGroundEntity( (CBaseEntity *)NULL );	// too steep
		}
		else
		{
			player->SetGroundEntity( pm.m_pEnt );  // Otherwise, point to index of ent under us.
		}

		// If we are on something...
		if (player->GetGroundEntity() != NULL)
		{
			// Then we are not in water jump sequence
			player->m_flWaterJumpTime = 0;
			
			// If we could make the move, drop us down that 1 pixel
			if ( player->GetWaterLevel() < WL_Waist && !pm.startsolid && !pm.allsolid )
			{
				// check distance we would like to move -- this is supposed to just keep up
				// "on the ground" surface not stap us back to earth (i.e. on move origin to
				// end position when the ground is within .5 units away) (2 units)
				if( pm.fraction )
//				if( pm.fraction < 0.5)
				{
					VectorCopy(pm.endpos, mv->m_vecOrigin);
				}
			}
		}

		// Standing on an entity other than the world
		if ( !pm.DidHitWorld() )   // So signal that we are touching something.
		{
			MoveHelper( )->AddToTouched( pm, mv->m_vecVelocity );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determine if the player has hit the ground while falling, apply
//			damage, and play the appropriate impact sound.
//-----------------------------------------------------------------------------
void CGameMovement::CheckFalling( void )
{
	if ( player->GetGroundEntity() != NULL &&
		 !IsDead() &&
		 player->m_Local.m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHOLD )
	{
		bool bAlive = true;
		float fvol = 0.5;

		if ( player->GetWaterLevel() > 0 )
		{
			// They landed in water.
		}
		else
		{
			//
			// They hit the ground.
			//
			if ( player->m_Local.m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED )
			{
				//
				// If they hit the ground going this fast they may take damage (and die).
				//
				bAlive = MoveHelper( )->PlayerFallingDamage();
				fvol = 1.0;
			}
			else if ( player->m_Local.m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED / 2 )
			{
				fvol = 0.85;
			}
			else if ( player->m_Local.m_flFallVelocity < PLAYER_MIN_BOUNCE_SPEED )
			{
				fvol = 0;
			}
		}

		if ( fvol > 0.0 )
		{
			//
			// Play landing sound right away.
			//
			player->m_flStepSoundTime = 0;
			UpdateStepSound();
			
			//
			// Play step sound for current texture.
			//
			PlayStepSound( m_pSurfaceData, fvol, true );

			//
			// Knock the screen around a little bit, temporary effect.
			//
			player->m_Local.m_vecPunchAngle.Set( ROLL, player->m_Local.m_flFallVelocity * 0.013 );

			if ( player->m_Local.m_vecPunchAngle[PITCH] > 8 )
			{
				player->m_Local.m_vecPunchAngle.Set( PITCH, 8 );
			}
		}

		if (bAlive)
		{
			MoveHelper( )->PlayerSetAnimation( PLAYER_WALK );
		}
	}

	//
	// Clear the fall velocity so the impact doesn't happen again.
	//
	if ( player->GetGroundEntity() != NULL ) 
	{		
		player->m_Local.m_flFallVelocity = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Use for ease-in, ease-out style interpolation (accel/decel)  Used by ducking code.
// Input  : value - 
//			scale - 
// Output : float
//-----------------------------------------------------------------------------
float CGameMovement::SplineFraction( float value, float scale )
{
	float valueSquared;

	value = scale * value;
	valueSquared = value * value;

	// Nice little ease-in, ease-out spline-like curve
	return 3 * valueSquared - 2 * valueSquared * value;
}

//-----------------------------------------------------------------------------
// Purpose: Determine if crouch/uncrouch caused player to get stuck in world
// Input  : direction - 
//-----------------------------------------------------------------------------
void CGameMovement::FixPlayerCrouchStuck( bool upward )
{
	EntityHandle_t hitent;
	int i;
	Vector test;
	trace_t dummy;

	int direction = upward ? 1 : 0;

	hitent = TestPlayerPosition( mv->m_vecOrigin, COLLISION_GROUP_PLAYER_MOVEMENT, dummy );
	if (hitent == INVALID_ENTITY_HANDLE )
		return;
	
	VectorCopy( mv->m_vecOrigin, test );	
	for ( i = 0; i < 36; i++ )
	{
		mv->m_vecOrigin[2] += direction;
		hitent = TestPlayerPosition( mv->m_vecOrigin, COLLISION_GROUP_PLAYER_MOVEMENT, dummy );
		if (hitent == INVALID_ENTITY_HANDLE )
			return;
	}

	VectorCopy( test, mv->m_vecOrigin ); // Failed
}

bool CGameMovement::CanUnduck()
{
	int i;
	trace_t trace;
	Vector newOrigin;

	VectorCopy( mv->m_vecOrigin, newOrigin );

	if ( player->GetGroundEntity() != NULL )
	{
		for ( i = 0; i < 3; i++ )
		{
			newOrigin[i] += ( m_vecMinsDucked[i] - m_vecMinsNormal[i] );
		}
	}
	else
	{
		// If in air an letting go of croush, make sure we can offset origin to make
		//  up for uncrouching
 		Vector hullSizeNormal = m_vecMaxsNormal - m_vecMinsNormal;
		Vector hullSizeCrouch = m_vecMaxsDucked - m_vecMinsDucked;

		Vector viewDelta = -0.5f * ( hullSizeNormal - hullSizeCrouch );

		VectorAdd( newOrigin, viewDelta, newOrigin );
	}

	bool saveducked = player->m_Local.m_bDucked;
	player->m_Local.m_bDucked = false;
	TracePlayerBBox( newOrigin, newOrigin, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	player->m_Local.m_bDucked = saveducked;

	if ( !trace.startsolid )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Stop ducking
//-----------------------------------------------------------------------------
void CGameMovement::FinishUnDuck( void )
{
	int i;
	trace_t trace;
	Vector newOrigin;

	VectorCopy( mv->m_vecOrigin, newOrigin );

	if ( player->GetGroundEntity() != NULL )
	{
		for ( i = 0; i < 3; i++ )
		{
			newOrigin[i] += ( m_vecMinsDucked[i] - m_vecMinsNormal[i] );
		}
	}
	else
	{
		// If in air an letting go of croush, make sure we can offset origin to make
		//  up for uncrouching
 		Vector hullSizeNormal = m_vecMaxsNormal - m_vecMinsNormal;
		Vector hullSizeCrouch = m_vecMaxsDucked - m_vecMinsDucked;

		Vector viewDelta = -0.5f * ( hullSizeNormal - hullSizeCrouch );

		VectorAdd( newOrigin, viewDelta, newOrigin );
	}

	player->m_Local.m_bDucked = false;

	player->RemoveFlag( FL_DUCKING );
	player->m_Local.m_bDucking  = false;
	player->SetViewOffset( GetPlayerViewOffset( false ) );
	player->m_Local.m_flDucktime = 0;
	
	VectorCopy( newOrigin, mv->m_vecOrigin );

	// Recategorize position since ducking can change origin
	CategorizePosition();
}

//-----------------------------------------------------------------------------
// Purpose: Finish ducking
//-----------------------------------------------------------------------------
void CGameMovement::FinishDuck( void )
{
	int i;

	Vector hullSizeNormal = m_vecMaxsNormal - m_vecMinsNormal;
	Vector hullSizeCrouch = m_vecMaxsDucked - m_vecMinsDucked;

	Vector viewDelta = 0.5f * ( hullSizeNormal - hullSizeCrouch );

	player->m_Local.m_bDucked = 1;
	player->SetViewOffset( GetPlayerViewOffset( true ) );
	player->AddFlag( FL_DUCKING );
	player->m_Local.m_bDucking = false;

	// HACKHACK - Fudge for collision bug - no time to fix this properly
	if ( player->GetGroundEntity() != NULL )
	{
		for ( i = 0; i < 3; i++ )
		{
			mv->m_vecOrigin[i] -= ( m_vecMinsDucked[i] - m_vecMinsNormal[i] );
		}
	}
	else
	{
		VectorAdd( mv->m_vecOrigin, viewDelta, mv->m_vecOrigin );
	}

	// See if we are stuck?
	FixPlayerCrouchStuck( true );

	// Recategorize position since ducking can change origin
	CategorizePosition();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : duckFraction - 
//-----------------------------------------------------------------------------
void CGameMovement::SetDuckedEyeOffset( float duckFraction )
{
	Vector vDuckHullMin = GetPlayerMins( true );
	Vector vStandHullMin = GetPlayerMins( false );

	float fMore = ( vDuckHullMin.z - vStandHullMin.z );

	Vector vecDuckViewOffset = GetPlayerViewOffset( true );
	Vector vecStandViewOffset = GetPlayerViewOffset( false );
	Vector temp = player->GetViewOffset();
	temp.z = ( ( vecDuckViewOffset.z - fMore ) * duckFraction ) +
				( vecStandViewOffset.z * ( 1 - duckFraction ) );
	player->SetViewOffset( temp );
}

//-----------------------------------------------------------------------------
// Purpose: See if duck button is pressed and do the appropriate things
//-----------------------------------------------------------------------------
void CGameMovement::Duck( void )
{
	int buttonsChanged	= ( mv->m_nOldButtons ^ mv->m_nButtons );	// These buttons have changed this frame
	int buttonsPressed	=  buttonsChanged & mv->m_nButtons;			// The changed ones still down are "pressed"
	int buttonsReleased	=  buttonsChanged & mv->m_nOldButtons;		// The changed ones which were previously down are "released"

	if ( mv->m_nButtons & IN_DUCK )
	{
		mv->m_nOldButtons |= IN_DUCK;
	}
	else
	{
		mv->m_nOldButtons &= ~IN_DUCK;
	}

	if ( IsDead() )
	{
		// Unduck
		if ( player->GetFlags() & FL_DUCKING )
		{
			FinishUnDuck();
		}
		return;
	}

	if ( !m_bSpeedCropped && ( player->GetFlags() & FL_DUCKING ) )
	{
		float frac = 0.33333333f;
		mv->m_flForwardMove	*= frac;
		mv->m_flSideMove	*= frac;
		mv->m_flUpMove		*= frac;
		m_bSpeedCropped		= true;
	}
	
	// Holding duck, in process of ducking or fully ducked?
	if ( ( mv->m_nButtons & IN_DUCK ) || ( player->m_Local.m_bDucking ) || ( player->GetFlags() & FL_DUCKING ) )
	{
		if ( mv->m_nButtons & IN_DUCK )
		{
			bool alreadyDucked = ( player->GetFlags() & FL_DUCKING ) ? true : false;

			if ( (buttonsPressed & IN_DUCK ) && !( player->GetFlags() & FL_DUCKING ) )
			{
				// Use 1 second so super long jump will work
				player->m_Local.m_flDucktime = 1000;
				player->m_Local.m_bDucking    = true;
			}

			float duckmilliseconds = max( 0.0f, 1000.0f - (float)player->m_Local.m_flDucktime );
			float duckseconds = duckmilliseconds / 1000.0f;

			//time = max( 0.0, ( 1.0 - (float)player->m_Local.m_flDucktime / 1000.0 ) );
			
			if ( player->m_Local.m_bDucking )
			{
				// Finish ducking immediately if duck time is over or not on ground
				if ( ( duckseconds > TIME_TO_DUCK ) || 
					( player->GetGroundEntity() == NULL ) ||
					alreadyDucked)
				{
					FinishDuck();
				}
				else
				{
					// Calc parametric time
					float duckFraction = SimpleSpline( duckseconds / TIME_TO_DUCK );
					SetDuckedEyeOffset( duckFraction );
				}
			}
		}
		else
		{
			// Try to unduck unless automovement is not allowed
			// NOTE: When not onground, you can always unduck
			if ( player->m_Local.m_bAllowAutoMovement || player->GetGroundEntity() == NULL )
			{
				if ( (buttonsReleased & IN_DUCK ) && ( player->GetFlags() & FL_DUCKING ) )
				{
					// Use 1 second so super long jump will work
					player->m_Local.m_flDucktime = 1000;
					player->m_Local.m_bDucking    = true;  // or unducking
				}

				float duckmilliseconds = max( 0.0f, 1000.0f - (float)player->m_Local.m_flDucktime );
				float duckseconds = duckmilliseconds / 1000.0f;

				if ( CanUnduck() )
				{
					if ( player->m_Local.m_bDucking || 
						 player->m_Local.m_bDucked ) // or unducking
					{
						// Finish ducking immediately if duck time is over or not on ground
						if ( ( duckseconds > TIME_TO_UNDUCK ) ||
							 ( player->GetGroundEntity() == NULL ) )
						{
							FinishUnDuck();
						}
						else
						{
							// Calc parametric time
							float duckFraction = SimpleSpline( 1.0f - ( duckseconds / TIME_TO_UNDUCK ) );
							SetDuckedEyeOffset( duckFraction );
						}
					}
				}
				else
				{
					// Still under something where we can't unduck, so make sure we reset this timer so
					//  that we'll unduck once we exit the tunnel, etc.
					player->m_Local.m_flDucktime	= 1000;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::PlayerMove( void )
{
	VPROF( "CGameMovement::PlayerMove" );

	EntityHandle_t ladder = INVALID_ENTITY_HANDLE;

	CheckParameters();
	
	// clear output applied velocity
	mv->m_outWishVel = vec3_origin;

	MoveHelper( )->ResetTouchList();                    // Assume we don't touch anything

	ReduceTimers();

	AngleVectors (mv->m_vecViewAngles, &m_vecForward, &m_vecRight, &m_vecUp );  // Determine movement angles

	/*
	// Special handling for spectator and observers. (iuser1 is set if the player's in observer mode)
	if ( pmv->spectator || pmv->iuser1 > 0 )
	{
		SpectatorMove();
		CategorizePosition();
		return;
	}
	*/

	// Always try and unstick us unless we are a couple of the movement modes
	if ( player->GetMoveType() != MOVETYPE_NOCLIP && 
		 player->GetMoveType() != MOVETYPE_NONE && 		 
		 player->GetMoveType() != MOVETYPE_ISOMETRIC && 
		 player->GetMoveType() != MOVETYPE_OBSERVER )
	{
		if ( CheckStuck() )
		{
			// Can't move, we're stuck
			return;  
		}
	}

	// Now that we are "unstuck", see where we are (player->GetWaterLevel() and type, player->GetGroundEntity()).
	CategorizePosition();

	// Store off the starting water level
	m_nOldWaterLevel = player->GetWaterLevel();

	// If we are not on ground, store off how fast we are moving down
	if ( player->GetGroundEntity() == NULL )
	{
		player->m_Local.m_flFallVelocity = -mv->m_vecVelocity[ 2 ];
	}

	m_nOnLadder = 0;

	UpdateStepSound();

	CategorizeGroundSurface();
	Duck();

	// Don't run ladder code if dead on on a train
	if ( !player->pl.deadflag && !(player->GetFlags() & FL_ONTRAIN) )
	{
		// FIXME THIS CODE IS A BIT DIFFERENT
		if ( LadderMove() )
		{
			;
		}
		else if ( player->GetMoveType() == MOVETYPE_LADDER )
		{
			// clear ladder stuff unless player is noclipping or being lifted by barnacle. 
			// it will be set immediately again next frame if necessary
			player->SetMoveType( MOVETYPE_WALK );
			player->SetMoveCollide( MOVECOLLIDE_DEFAULT );
		}
	}

	// Slow down, I'm pulling it! (a box maybe) but only when I'm standing on ground
	if ( (player->GetGroundEntity() != NULL) && (mv->m_nButtons & IN_USE) && !m_bSpeedCropped )
	{
		float frac = 1.0f / 3.0f;
		mv->m_flForwardMove *= frac;
		mv->m_flSideMove    *= frac;
		mv->m_flUpMove      *= frac;
		m_bSpeedCropped = true;
	}

	// Handle movement modes.
	switch (player->GetMoveType())
	{
		case MOVETYPE_NONE:
			break;

		case MOVETYPE_NOCLIP:
			FullNoClipMove();
			break;

		case MOVETYPE_FLY:
		case MOVETYPE_FLYGRAVITY:
			FullTossMove();
			break;

		case MOVETYPE_LADDER:
			FullLadderMove( (ladder != INVALID_ENTITY_HANDLE) );
			break;

		case MOVETYPE_WALK:
			FullWalkMove( (ladder != INVALID_ENTITY_HANDLE) );
			break;

		case MOVETYPE_ISOMETRIC:
			//IsometricMove();
			// Could also try:  FullTossMove();
			FullWalkMove( false );
			break;
			
		case MOVETYPE_OBSERVER:
			//IsometricMove();
			// Could also try:  FullTossMove();
			FullObserverMove();
			break;

		default:
			DevMsg( 1, "Bogus pmove player movetype %i on (%i) 0=cl 1=sv\n", player->GetMoveType(), player->IsServer());
			break;
	}
}


//-----------------------------------------------------------------------------
// Performs the collision resolution for fliers.
//-----------------------------------------------------------------------------
void CGameMovement::PerformFlyCollisionResolution( trace_t &pm, Vector &move )
{
	Vector base;
	float vel;
	float backoff;

	switch (player->GetMoveCollide())
	{
	case MOVECOLLIDE_FLY_CUSTOM:
		// Do nothing; the velocity should have been modified by touch
		// FIXME: It seems wrong for touch to modify velocity
		// given that it can be called in a number of places
		// where collision resolution do *not* in fact occur

		// Should this ever occur for players!?
		Assert(0);
		break;

	case MOVECOLLIDE_FLY_BOUNCE:	
	case MOVECOLLIDE_DEFAULT:
		{
			if (player->GetMoveCollide() == MOVECOLLIDE_FLY_BOUNCE)
				backoff = 2.0 - m_surfaceFriction;
			else
				backoff = 1;

			ClipVelocity (mv->m_vecVelocity, pm.plane.normal, mv->m_vecVelocity, backoff);
		}
		break;

	default:
		// Invalid collide type!
		Assert(0);
		break;
	}

	// stop if on ground
	if (pm.plane.normal[2] > 0.7)
	{		
		base.Init();
		if (mv->m_vecVelocity[2] < sv_gravity.GetFloat() * gpGlobals->frametime)
		{
			// we're rolling on the ground, add static friction.
			player->SetGroundEntity( pm.m_pEnt ); 
			mv->m_vecVelocity[2] = 0;
		}

		vel = DotProduct( mv->m_vecVelocity, mv->m_vecVelocity );

		// Con_DPrintf("%f %f: %.0f %.0f %.0f\n", vel, trace.fraction, ent->velocity[0], ent->velocity[1], ent->velocity[2] );

		if (vel < (30 * 30) || (player->GetMoveCollide() != MOVECOLLIDE_FLY_BOUNCE))
		{
			player->SetGroundEntity( pm.m_pEnt ); 
			mv->m_vecVelocity.Init();
		}
		else
		{
			VectorScale (mv->m_vecVelocity, (1.0 - pm.fraction) * gpGlobals->frametime * 0.9, move);
			pm = PushEntity( move );
		}
		VectorSubtract( mv->m_vecVelocity, base, mv->m_vecVelocity );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FullTossMove( void )
{
	trace_t pm;
	Vector move;
	
	CheckWater();

	// add velocity if player is moving 
	if ( (mv->m_flForwardMove != 0.0f) || (mv->m_flSideMove != 0.0f) || (mv->m_flUpMove != 0.0f))
	{
		Vector forward, right, up;
		float fmove, smove;
		Vector wishdir, wishvel;
		float wishspeed;
		int i;

		AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

		// Copy movement amounts
		fmove = mv->m_flForwardMove;
		smove = mv->m_flSideMove;

		VectorNormalize (forward);  // Normalize remainder of vectors.
		VectorNormalize (right);    // 
		
		for (i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
			wishvel[i] = forward[i]*fmove + right[i]*smove;

		wishvel[2] += mv->m_flUpMove;

		VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
		wishspeed = VectorNormalize(wishdir);

		//
		// Clamp to server defined max speed
		//
		if (wishspeed > mv->m_flMaxSpeed)
		{
			VectorScale (wishvel, mv->m_flMaxSpeed/wishspeed, wishvel);
			wishspeed = mv->m_flMaxSpeed;
		}

		// Set pmove velocity
		Accelerate ( wishdir, wishspeed, sv_accelerate.GetFloat() );
	}

	if ( mv->m_vecVelocity[2] > 0 )
	{
		player->SetGroundEntity( (CBaseEntity *)NULL );
	}

	// If on ground and not moving, return.
	if ( player->GetGroundEntity() != NULL )
	{
		if (VectorCompare(player->GetBaseVelocity(), vec3_origin) &&
		    VectorCompare(mv->m_vecVelocity, vec3_origin))
			return;
	}

	CheckVelocity();

	// add gravity
	if ( player->GetMoveType() == MOVETYPE_FLYGRAVITY )
	{
		AddGravity();
	}

	// move origin
	// Base velocity is not properly accounted for since this entity will move again after the bounce without
	// taking it into account
	VectorAdd (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
	
	CheckVelocity();

	VectorScale (mv->m_vecVelocity, gpGlobals->frametime, move);
	VectorSubtract (mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

	pm = PushEntity( move );	// Should this clear basevelocity

	CheckVelocity();

	if (pm.allsolid)
	{	
		// entity is trapped in another solid
		player->SetGroundEntity( pm.m_pEnt );
		mv->m_vecVelocity.Init();
		return;
	}
	
	if (pm.fraction != 1)
	{
		PerformFlyCollisionResolution( pm, move );
	}
	
	// check for in water
	CheckWater();
}

//-----------------------------------------------------------------------------
// Purpose: TF2 commander mode movement logic
//-----------------------------------------------------------------------------

#pragma warning (disable : 4701)

void CGameMovement::IsometricMove( void )
{
	int i;
	Vector wishvel;
	float fmove, smove;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles
	
	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;
	
	// No up / down movement
	forward[2] = 0;
	right[2] = 0;

	VectorNormalize (forward);  // Normalize remainder of vectors
	VectorNormalize (right);    // 

	for (i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	//wishvel[2] += mv->m_flUpMove;

	VectorMA (mv->m_vecOrigin, gpGlobals->frametime, wishvel, mv->m_vecOrigin);
	
	// Zero out the velocity so that we don't accumulate a huge downward velocity from
	//  gravity, etc.
	mv->m_vecVelocity.Init();
}

#pragma warning (default : 4701)
