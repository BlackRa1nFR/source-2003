//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "sceneentity.h"
#include "ai_baseactor.h"
#include "ai_navigator.h"

ConVar flex_minplayertime( "flex_minplayertime", "5" );
ConVar flex_maxplayertime( "flex_maxplayertime", "7" );
ConVar flex_minawaytime( "flex_minawaytime", "0.5" );
ConVar flex_maxawaytime( "flex_maxawaytime", "1.0" );

BEGIN_DATADESC( CAI_BaseActor )

	DEFINE_FIELD( CAI_BaseActor, m_fLatchedPositions, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_latchedEyeOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CAI_BaseActor, m_latchedEyeDirection, FIELD_VECTOR ),
	DEFINE_FIELD( CAI_BaseActor, m_latchedHeadDirection, FIELD_VECTOR ),
	DEFINE_FIELD( CAI_BaseActor, m_goalHeadDirection, FIELD_VECTOR ),
	DEFINE_FIELD( CAI_BaseActor, m_flBlinktime, FIELD_FLOAT ),
	DEFINE_FIELD( CAI_BaseActor, m_hLookTarget, FIELD_EHANDLE ),
	// TODO					m_lookQueue					
	DEFINE_FIELD( CAI_BaseActor, m_flNextRandomLookTime, FIELD_FLOAT ),
	DEFINE_FIELD( CAI_BaseActor, m_iszExpressionScene, FIELD_STRING ),
	DEFINE_FIELD( CAI_BaseActor, m_hExpressionSceneEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( CAI_BaseActor, m_iszIdleExpression, FIELD_STRING ),
	DEFINE_FIELD( CAI_BaseActor, m_iszAlertExpression, FIELD_STRING ),
	DEFINE_FIELD( CAI_BaseActor, m_iszCombatExpression, FIELD_STRING ),
	DEFINE_FIELD( CAI_BaseActor, m_iszDeathExpression, FIELD_STRING ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterBodyTransY, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterBodyTransX, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterBodyLift, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterBodyYaw, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterBodyPitch, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterBodyRoll, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterSpineYaw, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterSpinePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterSpineRoll, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterNeckTrans, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterHeadYaw, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterHeadPitch, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_ParameterHeadRoll, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightMoveRightLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightMoveForwardBack, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightMoveUpDown, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightBodyRightLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightBodyUpDown, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightBodyTilt, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightChestRightLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightChestUpDown, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightChestTilt, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightHeadForwardBack, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightHeadRightLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightHeadUpDown, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_BaseActor, m_FlexweightHeadTilt, FIELD_INTEGER ),

	DEFINE_KEYFIELD( CAI_BaseActor, m_iszExpressionOverride, FIELD_STRING, "ExpressionOverride" ),

	DEFINE_INPUTFUNC( CAI_BaseActor,	FIELD_STRING,	"SetExpressionOverride",	InputSetExpressionOverride ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: clear out latched state
//-----------------------------------------------------------------------------
void CAI_BaseActor::StudioFrameAdvance ()
{
	// clear out head and eye latched values
	m_fLatchedPositions &= ~(HUMANOID_LATCHED_HEAD | HUMANOID_LATCHED_EYE);

	BaseClass::StudioFrameAdvance();
}




void CAI_BaseActor::SetModel( const char *szModelName )
{
	BaseClass::SetModel( szModelName );

	Init( m_ParameterBodyTransY, "body_trans_Y" );
	Init( m_ParameterBodyTransX, "body_trans_X" );
	Init( m_ParameterBodyLift, "body_lift" );
	Init( m_ParameterBodyYaw, "body_yaw" );
	Init( m_ParameterBodyPitch, "body_pitch" );
	Init( m_ParameterBodyRoll, "body_roll" );
	Init( m_ParameterSpineYaw, "spine_yaw" );
	Init( m_ParameterSpinePitch, "spine_pitch" );
	Init( m_ParameterSpineRoll, "spine_roll" );
	Init( m_ParameterNeckTrans, "neck_trans" );
	Init( m_ParameterHeadYaw, "head_yaw" );
	Init( m_ParameterHeadPitch, "head_pitch" );
	Init( m_ParameterHeadRoll, "head_roll" );

	Init( m_FlexweightMoveRightLeft, "move_rightleft" );
	Init( m_FlexweightMoveForwardBack, "move_forwardback" );
	Init( m_FlexweightMoveUpDown, "move_updown" );
	Init( m_FlexweightBodyRightLeft, "body_rightleft" );
	Init( m_FlexweightBodyUpDown, "body_updown" );
	Init( m_FlexweightBodyTilt, "body_tilt" );
	Init( m_FlexweightChestRightLeft, "chest_rightleft" );
	Init( m_FlexweightChestUpDown, "chest_updown" );
	Init( m_FlexweightChestTilt, "chest_tilt" );
	Init( m_FlexweightHeadForwardBack, "head_forwardback" );
	Init( m_FlexweightHeadRightLeft, "head_rightleft" );
	Init( m_FlexweightHeadUpDown, "head_updown" );
	Init( m_FlexweightHeadTilt, "head_tilt" );
}


//-----------------------------------------------------------------------------
// Purpose: clear out latched state
//-----------------------------------------------------------------------------
void CAI_BaseActor::SetViewtarget( const Vector &viewtarget )
{
	// clear out eye latch
	m_fLatchedPositions &= ~HUMANOID_LATCHED_EYE;

	// NDebugOverlay::Line(m_latchedEyeOrigin + Vector( 0,0,-32), viewtarget + Vector( 0,0,-32), 0,0,255, false, 0.1);

	BaseClass::SetViewtarget( viewtarget );
}


//-----------------------------------------------------------------------------
// Purpose: Returns true position of the eyeballs
//-----------------------------------------------------------------------------
void CAI_BaseActor::UpdateLatchedValues( ) 
{ 
	if (!(m_fLatchedPositions & HUMANOID_LATCHED_HEAD))
	{
		// set head latch
		m_fLatchedPositions |= HUMANOID_LATCHED_HEAD;

		QAngle angle;

		if (GetAttachment( "eyes", m_latchedEyeOrigin, angle ))
		{
			AngleVectors( angle, &m_latchedHeadDirection );
		}
		else
		{
			m_latchedEyeOrigin = BaseClass::EyePosition( );
			AngleVectors( GetLocalAngles(), &m_latchedHeadDirection );
		}
		// clear out eye latch
		m_fLatchedPositions &= ~(HUMANOID_LATCHED_EYE);
		// Msg( "eyeball %4f %4f %4f  : %3f %3f %3f\n", origin.x, origin.y, origin.z, angles.x, angles.y, angles.z );
	}

	if (!(m_fLatchedPositions & HUMANOID_LATCHED_EYE))
	{
		m_fLatchedPositions |= HUMANOID_LATCHED_EYE;

		if ( CapabilitiesGet() & bits_CAP_ANIMATEDFACE )
		{
			m_latchedEyeDirection = GetViewtarget() - m_latchedEyeOrigin; 
			VectorNormalize( m_latchedEyeDirection );
		}
		else
		{
			m_latchedEyeDirection = m_latchedHeadDirection;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true position of the eyeballs
//-----------------------------------------------------------------------------
Vector CAI_BaseActor::EyePosition( )
{ 
	UpdateLatchedValues();

	return m_latchedEyeOrigin;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if target is in legal range of eye movement for the current head position
//-----------------------------------------------------------------------------
bool CAI_BaseActor::ValidEyeTarget(const Vector &lookTargetPos)
{
	Vector vHeadDir = HeadDirection3D( );
	Vector lookTargetDir	= lookTargetPos - EyePosition();
	VectorNormalize(lookTargetDir);

	// Only look if it doesn't crank my eyeballs too far
	float dotPr = DotProduct(lookTargetDir, vHeadDir);
	// Msg( "ValidEyeTarget( %4f %4f %4f )  %3f\n", lookTargetPos.x, lookTargetPos.y, lookTargetPos.z, dotPr );

	if (dotPr > 0.5) // +- 60 degrees
	// if (dotPr > 0.86) // +- 30 degrees
	{
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if target is in legal range of possible head movements
//-----------------------------------------------------------------------------
bool CAI_BaseActor::ValidHeadTarget(const Vector &lookTargetPos)
{
	Vector vFacing = BodyDirection3D();
	Vector lookTargetDir = lookTargetPos - EyePosition();
	VectorNormalize(lookTargetDir);

	// Only look if it doesn't crank my head too far
	float dotPr = DotProduct(lookTargetDir, vFacing);
	if (dotPr > -0.5 && fabs( lookTargetDir.z ) < 0.7) // +- 120 degrees side to side, +- 45 up/down
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Integrate head turn over time
//-----------------------------------------------------------------------------
void CAI_BaseActor::SetHeadDirection( const Vector &vTargetPos, float flInterval)
{
	float flDiff;

	if (!(CapabilitiesGet() & bits_CAP_TURN_HEAD))
	{
		return;
	}

	Vector vEyePosition;
	QAngle vEyeAngles;
	GetAttachment( "eyes", vEyePosition, vEyeAngles );

	// Msg( "yaw %f pitch %f\n", vEyeAngles.y, vEyeAngles.x );

	//--------------------------------------
	// Set head yaw
	//--------------------------------------
	float flDesiredYaw = VecToYaw(vTargetPos - vEyePosition) + Get( m_FlexweightHeadRightLeft );

	flDiff = UTIL_AngleDiff( flDesiredYaw, vEyeAngles.y);

	Set( m_ParameterHeadYaw, Get( m_ParameterHeadYaw ) + flDiff );

	/*
// HACK HACK: ywb:  during .vcd playback, if we animate the slider for "head left/right" that ends up
//  moving the "head_yaw" poseparameter above, which if you the head does an extreme left right ends 
//  up feeding a huge body_yaw into here to compensate for the head motion.
// This needs to be fixed! so it's disabled for now.
	float flHead = GetPoseParameter( iPose );

	iPose = LookupPoseParameter( "body_yaw" );
	if (fabs( GetPoseParameter( iPose ) + flHead / 8.0) < 45)
	{
		SetPoseParameter( iPose, GetPoseParameter( iPose ) + flHead / 8.0 );
	}
	*/

	//--------------------------------------
	// Set head pitch
	//--------------------------------------
	float	flDesiredPitch	= UTIL_VecToPitch(vTargetPos - vEyePosition) + Get( m_FlexweightHeadUpDown );
	flDiff = UTIL_AngleDiff( flDesiredPitch, vEyeAngles.x );

	// Msg("rightleft %.1f updown %.1f\n", GetFlexWeight( "head_rightleft" ), GetFlexWeight( "head_updown" ) );
	Set( m_ParameterHeadPitch, Get( m_ParameterHeadPitch) + flDiff );
	//iPose = LookupPoseParameter( "body_pitch" );
	//SetPoseParameter( iPose, GetPoseParameter( iPose ) + flDiff / 8.0 );

	//--------------------------------------
	// Set head roll
	//--------------------------------------
	float	flDesiredRoll	= Get( m_FlexweightHeadTilt);
	flDiff = UTIL_AngleDiff( flDesiredRoll, vEyeAngles.z );

	// Msg("pitch %.1f yaw %.1f roll %.1f\n", GetPoseParameter( "head_pitch" ), GetPoseParameter( "head_yaw" ), GetPoseParameter( "head_roll" ) );

	if (fabs(flDiff) > 10.0)
		flDiff = flDiff;

	Set( m_ParameterHeadRoll, Get( m_ParameterHeadRoll) + flDiff );

	// FIXME: only during idle, or in response to accel/decel
	Set( m_ParameterBodyTransY, Get( m_FlexweightMoveRightLeft ) );
	Set( m_ParameterBodyTransX, Get( m_FlexweightMoveForwardBack ) );
	Set( m_ParameterBodyLift, Get( m_FlexweightMoveUpDown ) );
	Set( m_ParameterBodyYaw, Get( m_FlexweightBodyRightLeft ) );
	Set( m_ParameterBodyPitch, Get( m_FlexweightBodyUpDown ) );
	Set( m_ParameterBodyRoll, Get( m_FlexweightBodyTilt ) );
	Set( m_ParameterSpineYaw, Get( m_FlexweightChestRightLeft ) );
	Set( m_ParameterSpinePitch, Get( m_FlexweightChestUpDown ) );
	Set( m_ParameterSpineRoll, Get( m_FlexweightChestTilt ) );
	Set( m_ParameterNeckTrans, Get( m_FlexweightHeadForwardBack ) );

	InvalidateBoneCache( );
	// clear out head and eye latched values
	m_fLatchedPositions &= ~(HUMANOID_LATCHED_HEAD | HUMANOID_LATCHED_EYE);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_BaseActor::ClearHeadAdjustment( void )
{
	Set( m_ParameterHeadYaw, 0 );
	Set( m_ParameterHeadPitch, 0 );
	Set( m_ParameterHeadRoll, 0 );

	InvalidateBoneCache( );
	// clear out head and eye latched values
	m_fLatchedPositions &= ~(HUMANOID_LATCHED_HEAD | HUMANOID_LATCHED_EYE);
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's eye direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::EyeDirection2D( void )
{
	Vector vEyeDirection = EyeDirection3D( );
	vEyeDirection.z = 0;

	VectorNormalize( vEyeDirection );

	return vEyeDirection;
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's eye direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::EyeDirection3D( void )
{
	UpdateLatchedValues( );

	return m_latchedEyeDirection;
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's head direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::HeadDirection2D( void )
{	
	Vector vHeadDirection = HeadDirection3D();
	vHeadDirection.z = 0;
	VectorNormalize( vHeadDirection );
	return vHeadDirection;
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's head direction in 3D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::HeadDirection3D( )
{	
	UpdateLatchedValues( );

	return m_latchedHeadDirection;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CAI_BaseActor::HasActiveLookTargets( void )
{
	return m_lookQueue.Count() != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Look at other NPCs and clients from time to time
//-----------------------------------------------------------------------------
float CAI_BaseActor::PickRandomLookTarget( bool bExcludePlayers, float minTime, float maxTime )
{
	float			flDuration = random->RandomFloat( minTime, maxTime );
	float			flInfluence = random->RandomFloat( 0.3, 0.5 );
	float			flRamp = random->RandomFloat( 0.2, 0.4 );
	
	CBaseEntity *pEnemy = GetEnemy();

	if (pEnemy != NULL)
	{
		if ( ( FVisible( pEnemy ) || random->RandomInt(0, 3) == 0 ) && ValidHeadTarget(pEnemy->EyePosition()))
		{
			// look at enemy closer
			flInfluence = random->RandomFloat( 0.7, 1.0 );
			flRamp = 0;
			AddLookTarget( pEnemy, flInfluence, flDuration, flRamp );
			return flDuration;
		}
		else
		{
			// look at something else for a shorter time
			flDuration = random->RandomFloat( 0.5, 0.8 );
			// move head faster
			flRamp = 0.2;
		}
	}
	
	bool bIsNavigating = ( GetNavigator()->IsGoalActive() && GetNavigator()->IsGoalSet() );
	
	if ( bIsNavigating && random->RandomInt(1, 10) <= 3 )
	{
		Vector navLookPoint;
		Vector delta;
		if ( GetNavigator()->GetPointAlongPath( &navLookPoint, 12 * 12 ) && (delta = navLookPoint - GetLocalOrigin()).Length() > 8.0 * 12.0 )
		{
			if ( random->RandomInt(1, 10) <= 5 )
			{
				AddLookTarget( navLookPoint, flInfluence, random->RandomFloat( 0.2, 0.4 ), 0 );
			}
			else
			{
				AddLookTarget( this, flInfluence, random->RandomFloat( 1.0, 2.0 ), 0 );
			}
			return 0;
		}
	}

	if ( GetState() == NPC_STATE_COMBAT && random->RandomInt(1, 10) <= 8 )
	{
		AddLookTarget( this, flInfluence, flDuration, flRamp );
		return flDuration;
	}

	CBaseEntity *pBestEntity = NULL;
	CBaseEntity *pEntity = NULL;
	int iHighestImportance = 0;
	for ( CEntitySphereQuery sphere( GetAbsOrigin(), 1024, 0 ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		if (pEntity == this)
		{
			continue;
		}

		if ( bExcludePlayers && pEntity->GetFlags() & FL_CLIENT )
		{
			// Don't look at any players.
			continue;
		}

		// CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if ((pEntity->GetFlags() & FL_CLIENT) && (pEntity->IsMoving() || random->RandomInt( 0, 2) == 0))
		{
			if (FVisible( pEntity ) && ValidHeadTarget(pEntity->EyePosition()))
			{
				pBestEntity = pEntity;
				break;
			}
		}
	
		if (pEntity->IsViewable())
		{
			// fTestDist = (EyePosition() - pEntity->EyePosition()).Length();
			Vector delta = (pEntity->EyePosition() - EyePosition());
			VectorNormalize( delta );

			int iImportance;
#if 0
			// consider things in front to be more important than things to the sides
			iImportance = (DotProduct( delta, HeadDirection3D() );
#else
			// No, for now, give all targets random priority (as long as they're in front)
			iImportance = random->RandomInt( 1, 100 );
			
#endif
			// make other npcs, and moving npc's far more important
			if (pEntity->MyNPCPointer())
			{
				iImportance *= 10;
				if (pEntity->IsMoving())
				{
					iImportance *= 10;
				}
			}

			if ( iImportance > iHighestImportance )
			{
				if (FVisible( pEntity ) && ValidHeadTarget(pEntity->EyePosition()))
				{
					iHighestImportance = iImportance;
					pBestEntity	= pEntity; 
					// NDebugOverlay::Line( EyePosition(), pEntity->EyePosition(), 0,0,255, false, 0.5);
				}
			}
		}
	}

	if (pBestEntity)
	{
		// Msg("looking at %s\n", pBestEntity->GetClassname() );
		AddLookTarget( pBestEntity, flInfluence, flDuration, flRamp );
		return flDuration;
	}
	return 0;
	// Msg("nothing to see\n" );
}

//-----------------------------------------------------------------------------
// Purpose: Set direction that the NPC is looking
//-----------------------------------------------------------------------------
void CAI_BaseActor::AddLookTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp )
{
	m_lookQueue.Add( pTarget, flImportance, flDuration, flRamp );
}


void CAI_BaseActor::AddLookTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	m_lookQueue.Add( vecPosition, flImportance, flDuration, flRamp );
}

//-----------------------------------------------------------------------------
// Purpose: Maintain eye, head, body postures, etc.
//-----------------------------------------------------------------------------
void CAI_BaseActor::MaintainLookTargets ( float flInterval )
{
	int i;

	if ( m_iszExpressionScene != NULL_STRING && m_hExpressionSceneEnt == NULL )
	{
		InstancedScriptedScene( this, STRING(m_iszExpressionScene), &m_hExpressionSceneEnt );
	}

	// ARRGGHHH, this needs to be moved!!!!
	ProcessExpressions( );

	// cached versions of the current eye position
	Vector vEyePosition = EyePosition( );

	// clear out head turning for turn checks are based on a neutral position
	ClearHeadAdjustment( );

	// initialize goal head direction to be current direction - this frames animation layering/pose parameters -  
	// but with the head controlls removed.
	Vector vHead = HeadDirection3D( );

	// NDebugOverlay::Line( vEyePosition, vEyePosition + vHead * 16, 0,0,255, false, 0.1);

	// clean up look targets
	for (i = 0; i < m_lookQueue.Count();)
	{
		if (!m_lookQueue[i].IsActive())
		{
			m_lookQueue.Remove( i );
		}
		else
		{
			i++;
		}
	}

	// RIGHT NOW cyclers do this! (SJB) (E3 2003) This gives us a sort of 'gaze-away' for the E3 tech demo.
	bool fIsCycler = FClassnameIs( this, "cycler_actor" );
	if ( m_flNextRandomLookTime < gpGlobals->curtime)
	{
		if (!HasActiveLookTargets() && ( GetState() != NPC_STATE_SCRIPT || fIsCycler ) )
		{
			// If my look queue is empty, fill it up a bit!
			// stagger player and random targets.

			if ( fIsCycler )
			{
				// Look at player (blink as you look back at them)!
				AddLookTarget( UTIL_PlayerByIndex( 1 ), 0.5, random->RandomFloat( flex_minplayertime.GetFloat(), flex_maxplayertime.GetFloat() ) );
			}
			else
			{
				// Look at whatever!
				m_flNextRandomLookTime = gpGlobals->curtime + PickRandomLookTarget() - 0.2;
			}
		}
		else
		{
			m_flNextRandomLookTime = gpGlobals->curtime + 1.0;
		}
	}

	// figure out ideal head yaw
	bool bValidHeadTarget = false;
	for (i = 0; i < m_lookQueue.Count();)
	{
		if (m_lookQueue[i].IsThis( this ))
		{
			// Msg( "head (%d) %s : %.1f : %.1f\n", i, STRING( pTarget->GetEntityName() ), m_lookQueue[i].flInterest, m_lookQueue[i].flTime - gpGlobals->curtime );
			// look directly ahead....
			i++;
		}
		else 
		{
			float flInterest = m_lookQueue[i].Interest( );
			// Msg( "head (%d) %.2f : %.1f %.1f %.1f\n", i, flInterest, m_lookQueue[i].GetPosition().x, m_lookQueue[i].GetPosition().y, m_lookQueue[i].GetPosition().z );

			Vector tmp = m_lookQueue[i].GetPosition() - vEyePosition;
			VectorNormalize( tmp );
			// FIXME: assumes head will turn 360!
			if (DotProduct( tmp, HeadDirection3D() ) >= -0.5)
			{
				vHead = vHead * (1 - flInterest) + tmp * flInterest;
				// Msg( "turning %f\n", m_lookQueue[i].flInterest );
				bValidHeadTarget = true;
				// NDebugOverlay::Line( vEyePosition, m_lookQueue[i].GetPosition(), 0,255,0, false, 0.1);
			}
			else
			{
				// NDebugOverlay::Line( vEyePosition, m_lookQueue[i].GetPosition(), 255,0,0, false, 0.1);
			}
			i++;
		}
	}

	// turn head toward target
	if (bValidHeadTarget)
	{
		SetHeadDirection( vEyePosition + vHead * 100, flInterval );
		m_goalHeadDirection = vHead;
	}
	else
	{
		// no target, decay all head control direction
		m_goalHeadDirection = m_goalHeadDirection * 0.8 + vHead * 0.2;
		VectorNormalize( m_goalHeadDirection );
		SetHeadDirection( vEyePosition + m_goalHeadDirection * 100, flInterval );
		// NDebugOverlay::Line( vEyePosition, vEyePosition + m_goalHeadDirection * 100, 255,0,0, false, 0.1);
	}

	// Msg( "%.1f %.1f ", GetPoseParameter( "head_pitch" ), GetPoseParameter( "head_roll" ) );

	// figure out eye target
	// eyes need to look directly at a target, even if the head doesn't quite aim there yet.
	bool bFoundTarget = false;
	for (i = m_lookQueue.Count() - 1; i >= 0; i--)
	{
		if (m_lookQueue[i].IsThis( this ))
		{
			// Msg( "eyes (%d) %s\n", i, STRING( pTarget->GetEntityName() ) );
			bFoundTarget = true;
			SetViewtarget( vEyePosition + vHead * 100 );
			break;
		}
		else
		{
			// E3 Hack
			if (ValidEyeTarget(m_lookQueue[i].GetPosition()))
			{
				// Msg( "eyes (%d) %s\n", i, STRING( pTarget->GetEntityName() ) );
#if 0
				// FIXME: add blink when changing targets
				if (m_hLookTarget != pTarget)
				{
					m_flBlinktime -= 0.5;
				}
#endif

				bFoundTarget = true;
				// m_hLookTarget = pTarget;
				SetViewtarget( m_lookQueue[i].GetPosition() );
				break;
			}
		}
	}

	// this should take into acount where it will try to be....
	if (!bFoundTarget && !ValidEyeTarget( GetViewtarget() ))
	{
		Vector right, up;
		VectorVectors( HeadDirection3D(), right, up );
		// Msg("random view\n");
		SetViewtarget( EyePosition() + HeadDirection3D() * 128 + right * random->RandomFloat(-32,32) + up * random->RandomFloat(-16,16) );
	}

	// Msg("pitch %.1f yaw %.1f\n", GetFlexWeight( "eyes_updown" ), GetFlexWeight( "eyes_rightleft" ) );

	// blink
	if (m_flBlinktime < gpGlobals->curtime)
	{
		m_blinktoggle = !m_blinktoggle;
		m_flBlinktime = gpGlobals->curtime + random->RandomFloat( 1.5, 4.5 );
	}
}


//-----------------------------------------------------------------------------

const char *CAI_BaseActor::SelectExpressionForState( NPC_STATE state )
{
	if ( m_iszExpressionOverride != NULL_STRING && state != NPC_STATE_DEAD )
	{
		return STRING(m_iszExpressionOverride);
	}

	switch ( state )
	{
	case NPC_STATE_IDLE:
		if ( m_iszIdleExpression != NULL_STRING )
			return STRING(m_iszIdleExpression);
		break;

	case NPC_STATE_COMBAT:
		if ( m_iszCombatExpression != NULL_STRING )
			return STRING(m_iszCombatExpression);
		break;

	case NPC_STATE_ALERT:
		if ( m_iszAlertExpression != NULL_STRING )
			return STRING(m_iszAlertExpression);
		break;

	case NPC_STATE_PLAYDEAD:
	case NPC_STATE_DEAD:
		if ( m_iszDeathExpression != NULL_STRING )
			return STRING(m_iszDeathExpression);
		break;
	}

	return NULL;
}

//-----------------------------------------------------------------------------

void CAI_BaseActor::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	SetExpression( SelectExpressionForState( NewState ) );

	BaseClass::OnStateChange( OldState, NewState );
}

//-----------------------------------------------------------------------------

void CAI_BaseActor::SetExpression( const char *pszExpressionScene )
{
	if ( !pszExpressionScene || !*pszExpressionScene )
	{
		ClearExpression();
		return;
	}

	if ( m_iszExpressionScene != NULL_STRING && stricmp( STRING(m_iszExpressionScene), pszExpressionScene ) == 0 )
	{
		return;
	}

	if ( m_hExpressionSceneEnt != NULL )
	{
		StopInstancedScriptedScene( this, m_hExpressionSceneEnt );
	}

	m_iszExpressionScene = NULL_STRING;
	if ( pszExpressionScene )
	{
		InstancedScriptedScene( this, pszExpressionScene, &m_hExpressionSceneEnt  );

		if ( m_hExpressionSceneEnt != NULL )
		{
			m_iszExpressionScene = AllocPooledString( pszExpressionScene );
		}
	}
}

//-----------------------------------------------------------------------------

void CAI_BaseActor::ClearExpression()
{
	if ( m_hExpressionSceneEnt != NULL )
	{
		StopInstancedScriptedScene( this, m_hExpressionSceneEnt );
	}
	m_iszExpressionScene = NULL_STRING;
}

//-----------------------------------------------------------------------------

const char *CAI_BaseActor::GetExpression()
{
	return STRING(m_iszExpressionScene);
}

//-----------------------------------------------------------------------------

void CAI_BaseActor::InputSetExpressionOverride( inputdata_t &inputdata )
{
	bool fHadOverride = ( m_iszExpressionOverride != NULL_STRING );
	m_iszExpressionOverride = inputdata.value.StringID();
	if (  m_iszExpressionOverride != NULL_STRING )
	{
		SetExpression( STRING(m_iszExpressionOverride) );
	}
	else if ( fHadOverride )
	{
		SetExpression( SelectExpressionForState( GetState() ) );
	}
}

//-----------------------------------------------------------------------------
