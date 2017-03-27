/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// GMan - misunderstood servant of the people
//=========================================================

#include	"cbase.h"
#include	"AI_Default.h"
#include	"AI_Task.h"
#include	"AI_Schedule.h"
#include	"AI_Node.h"
#include	"AI_Hull.h"
#include	"AI_Hint.h"
#include	"AI_Route.h"
#include	"AI_Motor.h"
#include	"soundent.h"
#include	"game.h"
#include	"NPCEvent.h"
#include	"EntityList.h"
#include	"activitylist.h"
#include	"animation.h"
#include	"IEffects.h"
#include	"vstdlib/random.h"
#include "ai_baseactor.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CNPC_GMan : public CAI_BaseActor
{
	DECLARE_CLASS( CNPC_GMan, CAI_BaseActor );
public:

	void Spawn( void );
	void Precache( void );
	float MaxYawSpeed( void ){ return 90.0f; }
	Class_T  Classify ( void );
	void HandleAnimEvent( animevent_t *pEvent );
	int GetSoundInterests ( void );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	int	 OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	void TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType);

	virtual int PlayScriptedSentence( const char *pszSentence, float duration, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener );
	
	EHANDLE m_hPlayer;
	EHANDLE m_hTalkTarget;
	float   m_flTalkTime;
};

LINK_ENTITY_TO_CLASS( monster_gman, CNPC_GMan );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
Class_T	CNPC_GMan :: Classify ( void )
{
	return	CLASS_NONE;
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_GMan :: HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// GetSoundInterests - generic monster can't hear.
//=========================================================
int CNPC_GMan :: GetSoundInterests ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CNPC_GMan::Spawn()
{
	Precache();

	BaseClass::Spawn();

	SetModel( "models/gman.mdl" );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_USE_WEAPONS | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD);

	NPCInit();

	Relink();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_GMan :: Precache()
{
	engine->PrecacheModel( "models/gman.mdl" );
}	


//=========================================================
// AI Schedules Specific to this monster
//=========================================================


void CNPC_GMan :: StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT:
		if (m_hPlayer == NULL)
		{
			m_hPlayer = gEntList.FindEntityByClassname( NULL, "player" );
		}
		break;
	}

	BaseClass::StartTask( pTask );
}

void CNPC_GMan :: RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT:
		// look at who I'm talking to
		if (m_flTalkTime > gpGlobals->curtime && m_hTalkTarget != NULL)
		{
			float yaw = VecToYaw(m_hTalkTarget->GetAbsOrigin() - GetAbsOrigin()) - GetAbsAngles().y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;

			// turn towards vector
			SetBoneController( 0, yaw );
		}
		// look at player, but only if playing a "safe" idle animation
		else if (m_hPlayer != NULL && GetSequence() == 0)
		{
			float yaw = VecToYaw(m_hPlayer->GetAbsOrigin() - GetAbsOrigin()) - GetAbsAngles().y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;

			// turn towards vector
			SetBoneController( 0, yaw );
		}
		else 
		{
			SetBoneController( 0, 0 );
		}
		BaseClass::RunTask( pTask );
		break;
	}

	SetBoneController( 0, 0 );
	BaseClass::RunTask( pTask );
}


//=========================================================
// Override all damage
//=========================================================
int CNPC_GMan :: OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	m_iHealth = m_iMaxHealth / 2; // always trigger the 50% damage aitrigger

	if ( inputInfo.GetDamage() > 0 )
		 SetCondition( COND_LIGHT_DAMAGE );
	
	if ( inputInfo.GetDamage() >= 20 )
		 SetCondition( COND_HEAVY_DAMAGE );
	
	return TRUE;
}


void CNPC_GMan::TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType)
{
	g_pEffects->Ricochet( ptr->endpos, ptr->plane.normal );
//	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

int CNPC_GMan::PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener )
{
	BaseClass::PlayScriptedSentence( pszSentence, delay, volume, soundlevel, bConcurrent, pListener );

	m_flTalkTime = gpGlobals->curtime + delay;
	m_hTalkTarget = pListener;

	return 1;
}
