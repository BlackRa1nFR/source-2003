//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "cs_playeranimstate.h"

#ifdef CLIENT_DLL
	#include "c_cs_player.h"
#else
	#include "cs_player.h"
#endif


// Below this many degrees, slow down turning rate linearly
#define FADE_TURN_DEGREES	45.0f

// After this, need to start turning feet
#define MAX_TORSO_ANGLE		70.0f

// Below this amount, don't play a turning animation/perform IK
#define MIN_TURN_ANGLE_REQUIRING_TURN_ANIMATION		15.0f

static ConVar tf2_feetyawrate( 
	"tf2_feetyawrate", 
	"720", 
	FCVAR_REPLICATED, 
	"How many degrees per second that we can turn our feet or upper body." );

static ConVar tf2_facefronttime( 
	"tf2_facefronttime", 
	"5", 
	FCVAR_REPLICATED, 
	"After this amount of time of standing in place but aiming to one side, go ahead and move feet to face upper body." );

static ConVar tf2_ik( "tf2_ik", "1", FCVAR_REPLICATED, "Use IK on in-place turns." );


CPlayerAnimState::CPlayerAnimState( CCSPlayer *outer )
	: m_pOuter( outer )
{
	m_flGaitYaw = 0.0f;
	m_flGoalFeetYaw = 0.0f;
	m_flCurrentFeetYaw = 0.0f;
	m_flCurrentTorsoYaw = 0.0f;
	m_flLastYaw = 0.0f;
	m_flLastTurnTime = 0.0f;
};


void CPlayerAnimState::SetOuterPoseParameter( int iParam, float flValue )
{
	// Make sure to set all the history values too, otherwise the server can overwrite them.
	GetOuter()->SetPoseParameter( iParam, flValue );
#ifdef CLIENT_DLL
	GetOuter()->m_iv_flPoseParameter.SetHistoryValuesForItem( iParam, flValue );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	// Store these. All the calculations are based on them.
	m_flEyeYaw = eyeYaw;
	m_flEyePitch = eyePitch;

	
	// Start out facing the entity towards the eye direction.
	m_angRender[YAW] = m_flEyeYaw;
	m_angRender[PITCH] = m_angRender[ROLL] = 0;

	ComputePoseParam_MoveYaw();
	ComputePoseParam_BodyPitch();
	ComputePoseParam_BodyYaw();

	ComputePlaybackRate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerAnimState::ComputePlaybackRate()
{
	// Determine ideal playback rate
	Vector vel;
	GetOuterAbsVelocity( vel );

	float speed = vel.Length2D();

	bool isMoving = ( speed > 0.5f ) ? true : false;

	Activity currentActivity = 	GetOuter()->GetSequenceActivity( GetOuter()->GetSequence() );

	switch ( currentActivity )
	{
	case ACT_WALK:
	case ACT_RUN:
	case ACT_IDLE:
		{
			float maxspeed = GetOuter()->MaxSpeed();
			if ( isMoving && ( maxspeed > 1.0f ) )
			{
				float flFactor = 1.0f;

				// Note this gets set back to 1.0 if sequence changes due to ResetSequenceInfo below
				float flWantedRate = ( speed * flFactor ) / maxspeed;
				flWantedRate = clamp( flWantedRate, 0.1, 10 );	// don't go nuts here.
				GetOuter()->SetPlaybackRate( flWantedRate );

				// BUG BUG:
				// This stuff really should be m_flPlaybackRate = speed / m_flGroundSpeed
			}
			else
			{
				GetOuter()->SetPlaybackRate( 1.0f );
			}
		}
		break;
	default:
		{
			GetOuter()->SetPlaybackRate( 1.0f );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBasePlayer
//-----------------------------------------------------------------------------
CCSPlayer *CPlayerAnimState::GetOuter() const
{
	return m_pOuter;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void CPlayerAnimState::EstimateYaw( void )
{
	float dt = gpGlobals->frametime;

	if ( !dt )
	{
		return;
	}

	Vector est_velocity;
	GetOuterAbsVelocity( est_velocity );
	if ( est_velocity[1] == 0 && est_velocity[0] == 0 )
	{
		float flYawDiff = m_flEyeYaw - m_flGaitYaw;
		flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;
		if (flYawDiff > 180)
			flYawDiff -= 360;
		if (flYawDiff < -180)
			flYawDiff += 360;

		if (dt < 0.25)
			flYawDiff *= dt * 4;
		else
			flYawDiff *= dt;

		m_flGaitYaw += flYawDiff;
		m_flGaitYaw = m_flGaitYaw - (int)(m_flGaitYaw / 360) * 360;
	}
	else
	{
		m_flGaitYaw = (atan2(est_velocity[1], est_velocity[0]) * 180 / M_PI);

		if (m_flGaitYaw > 180)
			m_flGaitYaw = 180;
		else if (m_flGaitYaw < -180)
			m_flGaitYaw = -180;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override for backpeddling
// Input  : dt - 
//-----------------------------------------------------------------------------
void CPlayerAnimState::ComputePoseParam_MoveYaw()
{
	int iYaw = GetOuter()->LookupPoseParameter( "move_yaw" );
	if ( iYaw < 0 )
		return;

	// view direction relative to movement
	float flYaw;	 

	EstimateYaw();

	float ang = m_flEyeYaw;
	if ( ang > 180.0f )
	{
		ang -= 360.0f;
	}
	else if ( ang < -180.0f )
	{
		ang += 360.0f;
	}

	// calc side to side turning
	flYaw = ang - m_flGaitYaw;
	// Invert for mapping into 8way blend
	flYaw = -flYaw;
	flYaw = flYaw - (int)(flYaw / 360) * 360;

	if (flYaw < -180)
	{
		flYaw = flYaw + 360;
	}
	else if (flYaw > 180)
	{
		flYaw = flYaw - 360;
	}
	
	SetOuterPoseParameter( iYaw, flYaw );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerAnimState::ComputePoseParam_BodyPitch( void )
{
	// Get pitch from v_angle
	float flPitch = m_flEyePitch;
	if ( flPitch > 180.0f )
	{
		flPitch -= 360.0f;
	}
	flPitch = clamp( flPitch, -90, 90 );

	m_angRender[YAW] = m_flEyeYaw;
	m_angRender[PITCH] = m_angRender[ROLL] = 0;

	// See if we have a blender for pitch
	int pitch = GetOuter()->LookupPoseParameter( "body_pitch" );
	if ( pitch < 0 )
		return;

	SetOuterPoseParameter( pitch, flPitch );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : goal - 
//			maxrate - 
//			dt - 
//			current - 
// Output : int
//-----------------------------------------------------------------------------
int CPlayerAnimState::ConvergeAngles( float goal,float maxrate, float dt, float& current )
{
	int direction = TURN_NONE;

	float anglediff = goal - current;
	float anglediffabs = fabs( anglediff );

	anglediff = AngleNormalize( anglediff );

	float scale = 1.0f;
	if ( anglediffabs <= FADE_TURN_DEGREES )
	{
		scale = anglediffabs / FADE_TURN_DEGREES;
		// Always do at least a bit of the turn ( 1% )
		scale = clamp( scale, 0.01f, 1.0f );
	}

	float maxmove = maxrate * dt * scale;

	if ( fabs( anglediff ) < maxmove )
	{
		current = goal;
	}
	else
	{
		if ( anglediff > 0 )
		{
			current += maxmove;
			direction = TURN_LEFT;
		}
		else
		{
			current -= maxmove;
			direction = TURN_RIGHT;
		}
	}

	current = AngleNormalize( current );

	return direction;
}

void CPlayerAnimState::ComputePoseParam_BodyYaw()
{
	// See if we even have a blender for pitch
	int upper_body_yaw = GetOuter()->LookupPoseParameter( "body_yaw" );
	if ( upper_body_yaw < 0 )
	{
		return;
	}

	// Assume upper and lower bodies are aligned and that we're not turning
	float flGoalTorsoYaw = 0.0f;
	int turning = TURN_NONE;
	float turnrate = tf2_feetyawrate.GetFloat();

	Vector vel;
	GetOuterAbsVelocity( vel );

	bool isMoving = ( vel.Length() > 0.1f ) ? true : false;
	if ( isMoving )
	{
		m_flLastTurnTime = 0.0f;
		m_nTurningInPlace = TURN_NONE;
		m_flGoalFeetYaw = m_flEyeYaw;
		flGoalTorsoYaw = 0.0f;
		turning = ConvergeAngles( m_flGoalFeetYaw, turnrate, gpGlobals->frametime, m_flCurrentFeetYaw );
		m_flCurrentTorsoYaw = m_flEyeYaw - m_flCurrentFeetYaw;
	}
	else
	{
		// Just stopped moving, try and clamp feet
		if ( m_flLastTurnTime <= 0.0f )
		{
			m_flLastTurnTime	= gpGlobals->curtime;
			m_flLastYaw			= m_flEyeYaw;
			// Snap feet to be perfectly aligned with torso/eyes
			m_flGoalFeetYaw		= m_flEyeYaw;
			m_flCurrentFeetYaw	= m_flGoalFeetYaw;
			m_nTurningInPlace	= TURN_NONE;
		}

		// If rotating in place, update stasis timer
		if ( m_flLastYaw != m_flEyeYaw )
		{
			m_flLastTurnTime	= gpGlobals->curtime;
			m_flLastYaw			= m_flEyeYaw;
		}

		if ( m_flGoalFeetYaw != m_flCurrentFeetYaw )
		{
			m_flLastTurnTime	= gpGlobals->curtime;
		}

		turning = ConvergeAngles( m_flGoalFeetYaw, turnrate, gpGlobals->frametime, m_flCurrentFeetYaw );

		// See how far off current feetyaw is from true yaw
		float yawdelta = m_flEyeYaw - m_flCurrentFeetYaw;
		yawdelta = AngleNormalize( yawdelta );

		bool rotated_too_far = false;

		float yawmagnitude = fabs( yawdelta );
		// If too far, then need to turn in place
		if ( yawmagnitude > MAX_TORSO_ANGLE )
		{
			rotated_too_far = true;
		}

		// Standing still for a while, rotate feet around to face forward
		// Or rotated too far
		// FIXME:  Play an in place turning animation
		if ( rotated_too_far || 
			( gpGlobals->curtime > m_flLastTurnTime + tf2_facefronttime.GetFloat() ) )
		{
			m_flGoalFeetYaw		= m_flEyeYaw;
			m_flLastTurnTime	= gpGlobals->curtime;

			float yd = m_flCurrentFeetYaw - m_flGoalFeetYaw;
			if ( yd > 0 )
			{
				m_nTurningInPlace = TURN_RIGHT;
			}
			else if ( yd < 0 )
			{
				m_nTurningInPlace = TURN_LEFT;
			}
			else
			{
				m_nTurningInPlace = TURN_NONE;
			}

			turning = ConvergeAngles( m_flGoalFeetYaw, turnrate, gpGlobals->frametime, m_flCurrentFeetYaw );
			yawdelta = m_flEyeYaw - m_flCurrentFeetYaw;
		}

		// Snap upper body into position since the delta is already smoothed for the feet
		flGoalTorsoYaw = yawdelta;
		m_flCurrentTorsoYaw = flGoalTorsoYaw;
	}


	if ( turning == TURN_NONE )
	{
		m_nTurningInPlace = turning;
	}

	if ( m_nTurningInPlace != TURN_NONE )
	{
		// If we're close to finishing the turn, then turn off the turning animation
		if ( fabs( m_flCurrentFeetYaw - m_flGoalFeetYaw ) < MIN_TURN_ANGLE_REQUIRING_TURN_ANIMATION )
		{
			m_nTurningInPlace = TURN_NONE;
		}
	}

	// Counter rotate upper body as needed
	ConvergeAngles( flGoalTorsoYaw, turnrate, gpGlobals->frametime, m_flCurrentTorsoYaw );

	// Rotate entire body into position
	m_angRender[YAW] = m_flCurrentFeetYaw;
	m_angRender[PITCH] = m_angRender[ROLL] = 0;

	SetOuterPoseParameter( upper_body_yaw, clamp( m_flCurrentTorsoYaw, -90.0f, 90.0f ) );
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CPlayerAnimState::BodyYawTranslateActivity( Activity activity )
{
	// Not even standing still, sigh
	if ( activity != ACT_IDLE )
		return activity;

	// Not turning
	switch ( m_nTurningInPlace )
	{
	default:
	case TURN_NONE:
		return activity;
	/*
	case TURN_RIGHT:
		return ACT_TURNRIGHT45;
	case TURN_LEFT:
		return ACT_TURNLEFT45;
	*/
	case TURN_RIGHT:
	case TURN_LEFT:
		return tf2_ik.GetBool() ? ACT_TURN : activity;
	}

	Assert( 0 );
	return activity;
}

const QAngle& CPlayerAnimState::GetRenderAngles()
{
	return m_angRender;
}


void CPlayerAnimState::GetOuterAbsVelocity( Vector& vel )
{
#if defined( CLIENT_DLL )
	GetOuter()->EstimateAbsVelocity( vel );
#else
	vel = GetOuter()->GetAbsVelocity();
#endif
}
