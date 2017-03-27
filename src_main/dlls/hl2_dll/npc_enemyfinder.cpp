//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Bullseyes act as targets for other NPC's to attack and to trigger
//			events 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "decals.h"
#include "ieffects.h"


#define SF_ENEMY_FINDER_CHECK_VIS (1 << 16)


class CNPC_EnemyFinder : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_EnemyFinder, CAI_BaseNPC );

public:
	int		m_nStartOn;
	float	m_flMinSearchDist;
	float	m_flMaxSearchDist;

	void	Precache( void );
	void	Spawn( void );
	void	StartNPC ( void );
	bool	IsValidEnemy( CBaseEntity *pTarget );
	Class_T Classify( void );

	virtual int	SelectSchedule( void );

	// Input handlers.
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};


LINK_ENTITY_TO_CLASS( npc_enemyfinder, CNPC_EnemyFinder );


//-----------------------------------------------------------------------------
// Custom schedules.
//-----------------------------------------------------------------------------
enum
{
	SCHED_EFINDER_SEARCH = LAST_SHARED_SCHEDULE,
};

IMPLEMENT_CUSTOM_AI( npc_enemyfinder, CNPC_EnemyFinder );

BEGIN_DATADESC( CNPC_EnemyFinder )

	// Inputs
	DEFINE_INPUT( CNPC_EnemyFinder, m_nStartOn,			FIELD_INTEGER,	"StartOn" ),
	DEFINE_INPUT( CNPC_EnemyFinder, m_flFieldOfView,	FIELD_FLOAT,	"FieldOfView" ),
	DEFINE_INPUT( CNPC_EnemyFinder, m_flMinSearchDist,	FIELD_FLOAT,	"MinSearchDist" ),
	DEFINE_INPUT( CNPC_EnemyFinder, m_flMaxSearchDist,	FIELD_FLOAT,	"MaxSearchDist" ),

	DEFINE_INPUTFUNC( CNPC_EnemyFinder, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CNPC_EnemyFinder, FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::InitCustomSchedules( void )
{
	INIT_CUSTOM_AI( CNPC_EnemyFinder );

	ADD_CUSTOM_SCHEDULE( CNPC_EnemyFinder, SCHED_EFINDER_SEARCH );
	AI_LOAD_SCHEDULE( CNPC_EnemyFinder, SCHED_EFINDER_SEARCH );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning the enemy finder on.
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::InputTurnOn( inputdata_t &inputdata )
{
	SetThink(CallNPCThink);
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning the enemy finder off.
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::InputTurnOff( inputdata_t &inputdata )
{
	SetThink(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::Spawn( void )
{
	Precache();

	// This is a dummy model that is never used!
	SetModel( "models/player.mdl" );

	UTIL_SetSize(this, vec3_origin, vec3_origin);

	SetMoveType( MOVETYPE_FLY );
	SetBloodColor( DONT_BLEED );
	SetGravity( 0.0 );
	m_iHealth			= 1;
	
	AddFlag( FL_NPC );

	SetSolid( SOLID_NONE );

	// Relink for solid type.
	Relink();

	if (m_flFieldOfView < -1.0)
	{
		Msg("ERROR: EnemyFinder field of view must be between -1.0 and 1.0\n");
		m_flFieldOfView		= 0.5;
	}
	else if (m_flFieldOfView > 1.0)
	{
		Msg("ERROR: EnemyFinder field of view must be between -1.0 and 1.0\n");
		m_flFieldOfView		= 1.0;
	}
	CapabilitiesAdd	( bits_CAP_SQUAD );

	NPCInit();

	// Set this after NPCInit()
	m_takedamage	= DAMAGE_NO;
	m_fEffects		|= EF_NODRAW;
	m_NPCState		= NPC_STATE_ALERT;	// always alert

	SetViewOffset( vec3_origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
int CNPC_EnemyFinder::SelectSchedule( void )
{
	return SCHED_EFINDER_SEARCH;
}

//------------------------------------------------------------------------------
// Purpose : Override base class to check range and visibility
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_EnemyFinder::IsValidEnemy( CBaseEntity *pTarget )
{
	// ---------------------------------
	//  Check range
	// ---------------------------------
	float flTargetDist = (GetAbsOrigin() - pTarget->GetAbsOrigin()).Length();
	if (flTargetDist < m_flMinSearchDist)
	{
		return false;
	}
	if (flTargetDist > m_flMaxSearchDist)
	{
		return false;
	}

	// ------------------------------------------------------
	// Make sure I can see the target from my position
	// ------------------------------------------------------
	if (FBitSet ( m_spawnflags, SF_ENEMY_FINDER_CHECK_VIS))
	{
		trace_t tr;

		// Trace from launch position to target position.  
		// Use position above actual barral based on vertical launch speed
		Vector vStartPos = GetAbsOrigin();
		Vector vEndPos	 = pTarget->EyePosition();
		AI_TraceLine( vStartPos, vEndPos, MASK_SHOT, pTarget, COLLISION_GROUP_NONE, &tr );

		if (tr.fraction == 1.0)
		{
			return true;
		}
		return false;
	}
	else
	{
		return true;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_EnemyFinder::StartNPC ( void )
{
	BaseClass::StartNPC();

	if (!m_nStartOn)
	{
		SetThink(NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_EnemyFinder::Classify( void )
{
	return	CLASS_NONE;
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
// > SCHED_EFINDER_SEARCH
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_EFINDER_SEARCH,

	"	Tasks"
	"		TASK_WAIT_RANDOM			0.5		"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
);