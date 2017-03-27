//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "ai_moveshoot.h"
#include "ai_basenpc.h"
#include "ai_navigator.h"
#include "ai_memory.h"

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_MoveAndShootOverlay )
	DEFINE_FIELD( CAI_MoveAndShootOverlay, m_bMovingAndShooting, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_MoveAndShootOverlay, m_nMoveShots, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_MoveAndShootOverlay, m_flNextMoveShootTime, FIELD_FLOAT ),
	DEFINE_FIELD( CAI_MoveAndShootOverlay, m_minBurst, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_MoveAndShootOverlay, m_maxBurst, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_MoveAndShootOverlay, m_minPause, FIELD_FLOAT ),
	DEFINE_FIELD( CAI_MoveAndShootOverlay, m_maxPause, FIELD_FLOAT ),
END_DATADESC()

//-------------------------------------

void CAI_MoveAndShootOverlay::NoShootWhileMove()
{
	m_flNextMoveShootTime = FLT_MAX;
}

//-------------------------------------

void CAI_MoveAndShootOverlay::StartShootWhileMove( float minPause, float maxPause )
{
	if ( GetOuter()->GetState() == NPC_STATE_SCRIPT || 
		 !GetOuter()->GetActiveWeapon() ||
		 !GetOuter()->HaveSequenceForActivity( GetOuter()->TranslateActivity( ACT_WALK_AIM ) ) ||
		 !GetOuter()->HaveSequenceForActivity( GetOuter()->TranslateActivity( ACT_RUN_AIM ) ) )
	{
		NoShootWhileMove();
		return;
	}
	
	m_minBurst = GetOuter()->GetActiveWeapon()->GetMinBurst();
	m_maxBurst = GetOuter()->GetActiveWeapon()->GetMaxBurst();
	m_minPause = minPause;
	m_maxPause = maxPause;

	m_nMoveShots = GetOuter()->GetActiveWeapon()->GetRandomBurst();
	m_flNextMoveShootTime = 0;
}

//-------------------------------------

bool CAI_MoveAndShootOverlay::CanAimAtEnemy()
{
	CAI_BaseNPC *pOuter = GetOuter();
	bool result = false;
	bool resetConditions = false;
	CAI_ScheduleBits savedConditions;

	if ( !GetOuter()->ConditionsGathered() )
	{
		savedConditions = GetOuter()->AccessConditionBits();
		GetOuter()->GatherEnemyConditions( GetOuter()->GetEnemy() );
	}
	
	if ( pOuter->HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
		result = true;
	}
	else if ( !pOuter->HasCondition( COND_ENEMY_DEAD ) &&
			  !pOuter->HasCondition( COND_TOO_FAR_TO_ATTACK ) &&
			  !pOuter->HasCondition( COND_ENEMY_TOO_FAR ) &&
			  !pOuter->HasCondition( COND_ENEMY_OCCLUDED ) &&
			  !pOuter->HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		result = true;
	}

	if ( resetConditions )
	{
		GetOuter()->AccessConditionBits() = savedConditions;
	}
		
	return result;
}

//-------------------------------------

void CAI_MoveAndShootOverlay::UpdateMoveShootActivity( bool bMoveAimAtEnemy )
{
	// FIXME: should be able to query that transition/state is happening
	// FIXME: needs to not try to shoot if the movement type isn't understood
	Activity curActivity = GetOuter()->GetNavigator()->GetMovementActivity();
	Activity newActivity = curActivity;

	if (bMoveAimAtEnemy)
	{
		switch( curActivity )
		{
		case ACT_WALK:
			newActivity = ACT_WALK_AIM;
			break;
		case ACT_RUN:
			newActivity = ACT_RUN_AIM;
			break;
		}
	}
	else
	{
		switch( curActivity )
		{
		case ACT_WALK_AIM:
			newActivity = ACT_WALK;
			break;
		case ACT_RUN_AIM:
			newActivity = ACT_RUN;
			break;
		}
	}

	if ( curActivity != newActivity )
	{
		// transitioning, wait a bit
		m_flNextMoveShootTime = gpGlobals->curtime + 0.3;
		GetOuter()->GetNavigator()->SetMovementActivity( newActivity );
	}
}

//-------------------------------------

void CAI_MoveAndShootOverlay::RunShootWhileMove()
{
	if ( m_flNextMoveShootTime == FLT_MAX )
		return;

	CAI_BaseNPC *pOuter = GetOuter();

	// keep enemy if dead but try to look for a new one
	if (!pOuter->GetEnemy() || !pOuter->GetEnemy()->IsAlive())
	{
		CBaseEntity *pNewEnemy = pOuter->BestEnemy();

		if( pNewEnemy != NULL )
		{
			//New enemy! Clear the timers and set conditions.
			pOuter->SetCondition( COND_NEW_ENEMY );
			pOuter->SetEnemy( pNewEnemy );
			pOuter->SetState( NPC_STATE_COMBAT );
		}
		else
		{
			pOuter->ClearAttackConditions();
		}
		// SetEnemy( NULL );
	}

	if ( GetEnemy() == NULL || !pOuter->GetNavigator()->IsGoalActive() )
		return;

	bool bMoveAimAtEnemy = CanAimAtEnemy();

	UpdateMoveShootActivity( bMoveAimAtEnemy );

	if (bMoveAimAtEnemy)
	{
		Assert( pOuter->GetActiveWeapon() ); // This should have been caught at task start

		// time to fire?
		if ( pOuter->HasCondition( COND_CAN_RANGE_ATTACK1 ) && gpGlobals->curtime >= m_flNextMoveShootTime )
		{
			if ( m_bMovingAndShooting || GetOuter()->OnBeginMoveAndShoot() )
			{
				m_bMovingAndShooting = true;
				Activity activity = pOuter->TranslateActivity( ACT_GESTURE_RANGE_ATTACK1 );

				Assert( activity != ACT_INVALID );

				if (--m_nMoveShots > 0)
				{
					pOuter->SetLastAttackTime( gpGlobals->curtime );
					pOuter->AddGesture( activity );
					// FIXME: this seems a bit wacked
					pOuter->Weapon_SetActivity( pOuter->Weapon_TranslateActivity( ACT_RANGE_ATTACK1 ), 0 );
					m_flNextMoveShootTime = gpGlobals->curtime + pOuter->GetActiveWeapon()->GetFireRate() - 0.1;
				}
				else
				{
					m_nMoveShots = random->RandomInt( m_minBurst, m_maxBurst );
					m_flNextMoveShootTime = gpGlobals->curtime + random->RandomFloat( m_minPause, m_maxPause );
					m_bMovingAndShooting = false;
					GetOuter()->OnEndMoveAndShoot();
				}
			}
		}

		// try to keep facing towards the last known position of the enemy
		Vector vecEnemyLKP = pOuter->GetEnemyLKP();
		pOuter->AddFacingTarget( pOuter->GetEnemy(), vecEnemyLKP, 1.0, 0.8 );
	}
	else
	{
		if ( m_bMovingAndShooting )
		{
			m_bMovingAndShooting = false;
			GetOuter()->OnEndMoveAndShoot();
		}
	}
}

//-------------------------------------

void CAI_MoveAndShootOverlay::EndShootWhileMove()
{
	if ( m_bMovingAndShooting )
	{
		m_bMovingAndShooting = false;
		GetOuter()->OnEndMoveAndShoot();
	}
}

//-----------------------------------------------------------------------------
