//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "IEffects.h"
#include "collisionutils.h"

#include "ai_basenpc.h"
#include "ai_scriptconditions.h"

ConVar debugscriptconditions( "ai_debugscriptconditions", "0" );

#define ScrCondDbgMsg( msg ) \
	do \
	{ \
		if ( debugscriptconditions.GetBool() ) \
		{ \
			Msg msg; \
		} \
	} \
	while (0)


//=============================================================================
//
// CAI_ScriptConditions
//
//=============================================================================

LINK_ENTITY_TO_CLASS(ai_script_conditions, CAI_ScriptConditions);

BEGIN_DATADESC( CAI_ScriptConditions )

	DEFINE_FUNCTION( CAI_ScriptConditions, EvaluationThink ),
	
	DEFINE_OUTPUT( CAI_ScriptConditions, m_OnConditionsSatisfied, "OnConditionsSatisfied" ),
	DEFINE_OUTPUT( CAI_ScriptConditions, m_OnConditionsTimeout, "OnConditionsTimeout" ),

	//---------------------------------

	DEFINE_INPUTFUNC( CAI_ScriptConditions, FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( CAI_ScriptConditions, FIELD_VOID, "Disable", InputDisable ),

	//---------------------------------

	// Inputs
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_fDisabled, 					FIELD_BOOLEAN, 	"StartDisabled" 			),
	DEFINE_FIELD( CAI_ScriptConditions, m_hActor, FIELD_EHANDLE ),
	DEFINE_FIELD( CAI_ScriptConditions, m_hTarget, FIELD_EHANDLE ),

	DEFINE_KEYFIELD(CAI_ScriptConditions, m_flRequiredTime, 			FIELD_FLOAT, 	"RequiredTime" 				),
	DEFINE_EMBEDDED(CAI_ScriptConditions, m_Timer ),
	DEFINE_EMBEDDED(CAI_ScriptConditions, m_Timeout ),

	DEFINE_KEYFIELD(CAI_ScriptConditions, m_fMinState, 					FIELD_INTEGER,	"MinimumState" 				),
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_fMaxState, 					FIELD_INTEGER,	"MaximumState" 				),
	
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_fScriptStatus, 				FIELD_INTEGER,	"ScriptStatus" 				),
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_fActorSeePlayer,			FIELD_INTEGER,	"ActorSeePlayer"			),
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_Actor,						FIELD_STRING,	"Actor"						),
	
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_flPlayerActorProximity,		FIELD_FLOAT, 	"PlayerActorProximity" 		),
	DEFINE_EMBEDDED(CAI_ScriptConditions, m_PlayerActorProxTester),

	DEFINE_KEYFIELD(CAI_ScriptConditions, m_flPlayerActorFOV, 			FIELD_FLOAT, 	"PlayerActorFOV" 			),
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_bPlayerActorFOVTrueCone,	FIELD_BOOLEAN,	"PlayerActorFOVTrueCone"	),

	DEFINE_KEYFIELD(CAI_ScriptConditions, m_fPlayerActorLOS, 			FIELD_INTEGER, 	"PlayerActorLOS" 			),
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_fActorSeeTarget,			FIELD_INTEGER,	"ActorSeeTarget" 			),
	
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_flActorTargetProximity,		FIELD_FLOAT, 	"ActorTargetProximity" 		),
	DEFINE_EMBEDDED(CAI_ScriptConditions, m_ActorTargetProxTester),

	DEFINE_KEYFIELD(CAI_ScriptConditions, m_flPlayerTargetProximity, 	FIELD_FLOAT, 	"PlayerTargetProximity"		),
	DEFINE_EMBEDDED(CAI_ScriptConditions, m_PlayerTargetProxTester),

	DEFINE_KEYFIELD(CAI_ScriptConditions, m_flPlayerTargetFOV, 			FIELD_FLOAT,	"PlayerTargetFOV"			),
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_bPlayerTargetFOVTrueCone,	FIELD_BOOLEAN,	"PlayerTargetFOVTrueCone"	),

	DEFINE_KEYFIELD(CAI_ScriptConditions, m_fPlayerTargetLOS, 			FIELD_INTEGER,	"PlayerTargetLOS"			),
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_fPlayerBlockingActor,		FIELD_INTEGER,  "PlayerBlockingActor"		),

	DEFINE_KEYFIELD(CAI_ScriptConditions, m_flMinTimeout, 				FIELD_FLOAT,	"MinTimeout"				),
	DEFINE_KEYFIELD(CAI_ScriptConditions, m_flMaxTimeout, 				FIELD_FLOAT,	"MaxTimeout"				),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( CAI_ProxTester )
	DEFINE_FIELD( CAI_ProxTester, m_distSq, FIELD_FLOAT ),
	DEFINE_FIELD( CAI_ProxTester, m_fInside, FIELD_BOOLEAN ),
END_DATADESC()


//-----------------------------------------------------------------------------

#define EVALUATOR( name ) { &CAI_ScriptConditions::Eval##name, #name }

CAI_ScriptConditions::EvaluatorInfo_t CAI_ScriptConditions::gm_Evaluators[] =
{
	EVALUATOR( ActorSeePlayer ),
	EVALUATOR( State ),
	EVALUATOR( PlayerActorProximity ),
	EVALUATOR( PlayerTargetProximity ),
	EVALUATOR( ActorTargetProximity ),
	EVALUATOR( PlayerBlockingActor ),
	EVALUATOR( PlayerActorLook ),
	EVALUATOR( PlayerTargetLook ),
	EVALUATOR( ActorSeeTarget),
	EVALUATOR( PlayerActorLOS ),
	EVALUATOR( PlayerTargetLOS ),
};

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalState( const EvalArgs_t &args )
{
	if ( !args.pActor )
		return true;

	CAI_BaseNPC *pNpc = args.pActor->MyNPCPointer();
	
	// !!!LATER - fix this code, we shouldn't need the table anymore
	// now that we've placed the NPC state defs in a logical order (sjb)
	static int stateVals[] = 
	{
		-1, // NPC_STATE_NONE
		 0, // NPC_STATE_IDLE
		 1, // NPC_STATE_ALERT
		 2, // NPC_STATE_COMBAT
		-1, // NPC_STATE_SCRIPT
		-1, // NPC_STATE_PLAYDEAD
		-1, // NPC_STATE_PRONE
		-1, // NPC_STATE_DEAD
	};
	
	int valState = stateVals[pNpc->m_NPCState];
	
	if ( valState < 0 )
	{
		if ( pNpc->m_NPCState == NPC_STATE_SCRIPT && m_fScriptStatus >= TS_TRUE )
			return true;
		
		return false;
	}
		
	const int valLow  = stateVals[m_fMinState];
	const int valHigh = stateVals[m_fMaxState];

	if ( valLow > valHigh )
	{
		Msg( "Script condition warning: Invalid setting for Maximum/Minimum state\n" );
		Disable();
		return false;
	}
	
	return ( valState >= valLow && valState <= valHigh );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalActorSeePlayer( const EvalArgs_t &args )
{
	if( m_fActorSeePlayer == TS_NONE )
	{
		// Don't care, so don't do any work.
		return true;
	}

	if ( !args.pActor )
		return true;

	bool fCanSeePlayer = args.pActor->MyNPCPointer()->HasCondition( COND_SEE_PLAYER );
	return ( (int)m_fActorSeePlayer == (int)fCanSeePlayer );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalActorSeeTarget( const EvalArgs_t &args )
{
	if( m_fActorSeeTarget == TS_NONE )
	{
		// Don't care, so don't do any work.
		return true;
	}

	if ( args.pTarget )
	{
		if ( !args.pActor )
			return true;

		CAI_BaseNPC *pNPCActor = args.pActor->MyNPCPointer();
		bool fSee = pNPCActor->FInViewCone( args.pTarget ) && pNPCActor->FVisible( args.pTarget );

		if( fSee )
		{
			if( m_fActorSeeTarget == TS_TRUE )
			{
				return true;
			}

			return false;
		}
		else
		{
			if( m_fActorSeeTarget == TS_FALSE )
			{
				return true;
			}

			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerActorProximity( const EvalArgs_t &args )
{
	return m_PlayerActorProxTester.Check( args.pPlayer, args.pActor );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerTargetProximity( const EvalArgs_t &args )
{
	return ( !args.pTarget || 
			 m_PlayerTargetProxTester.Check( args.pPlayer, args.pTarget ) );
}


//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalActorTargetProximity( const EvalArgs_t &args )
{
	return ( !args.pTarget || 
			 m_ActorTargetProxTester.Check( args.pActor, args.pTarget ) );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerActorLook( const EvalArgs_t &args )
{
	return IsInFOV( args.pPlayer, args.pActor, m_flPlayerActorFOV, m_bPlayerActorFOVTrueCone );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerTargetLook( const EvalArgs_t &args )
{
	return ( !args.pTarget || IsInFOV( args.pPlayer, args.pTarget, m_flPlayerTargetFOV, m_bPlayerTargetFOVTrueCone ) );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerActorLOS( const EvalArgs_t &args )
{
	if( m_fPlayerActorLOS == TS_NONE )
	{
		// Don't execute expensive code if we don't care.
		return true;
	}

	return PlayerHasLineOfSight( args.pPlayer, args.pActor, m_fPlayerActorLOS == TS_FALSE );
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerTargetLOS( const EvalArgs_t &args )
{
	if( m_fPlayerTargetLOS == TS_NONE )
	{
		// Don't execute expensive code if we don't care.
		return true;
	}

	return ( !args.pTarget || PlayerHasLineOfSight( args.pPlayer, args.pTarget, m_fPlayerTargetLOS == TS_FALSE ) );
}


//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::EvalPlayerBlockingActor( const EvalArgs_t &args )
{
	if ( m_fPlayerBlockingActor == TS_NONE )
		return true;

#if 0
	CAI_BaseNPC *pNpc = args.pActor->MyNPCPointer();

	const float testDist = 30.0;
		
	Vector origin = ( args.pActor->GetAbsMins() + args.pActor->GetAbsMaxs() ) * 0.5;
	Vector delta  = UTIL_YawToVector( args.pActor->GetAngles().y ) * testDist;
	
	bool intersect = IsBoxIntersectingRay(args.pActor->GetAbsMins(), args.pActor->GetAbsMaxs(), origin, delta );
#endif

	if ( m_fPlayerBlockingActor == TS_FALSE )
		return true;

	return false; // for now, never say player is blocking
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::Spawn()
{
	Assert( ( m_fMinState == NPC_STATE_IDLE || m_fMinState == NPC_STATE_COMBAT || m_fMinState == NPC_STATE_ALERT ) &&
			( m_fMaxState == NPC_STATE_IDLE || m_fMaxState == NPC_STATE_COMBAT || m_fMaxState == NPC_STATE_ALERT ) );

	m_PlayerActorProxTester.Init( m_flPlayerActorProximity );
	m_PlayerTargetProxTester.Init( m_flPlayerTargetProximity );
	m_ActorTargetProxTester.Init( m_flActorTargetProximity );

	m_Timer.Set( m_flRequiredTime );

}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::Activate()
{
	BaseClass::Activate();

	// When we spawn, m_fDisabled is initial state as given by worldcraft.
	// following that, we keep it updated and it reflects current state.
	if( !m_fDisabled )
		Enable();
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::EvaluationThink()
{
	AssertMsg( !m_fDisabled, ("Violated invariant between CAI_ScriptConditions disabled state and think func setting") );

	if ( m_Actor != NULL_STRING && !m_hActor.Get() )
	{
		Msg( "Warning: Active AI script conditions associated with an non-existant or destroyed NPC\n" );
		Disable();
		return;
	}

	if( m_flMinTimeout > 0 && m_Timeout.Expired() )
	{
		ScrCondDbgMsg( ( "%s firing output OnConditionsTimeout (%f seconds)\n", STRING( GetEntityName() ), m_Timer.GetInterval() ) );

		Disable();
		m_OnConditionsTimeout.FireOutput( this, this );
		
		return;
	}
	
	bool      result = true;
	const int nEvaluators = sizeof( gm_Evaluators ) / sizeof( gm_Evaluators[0] );

	EvalArgs_t args =
	{
		GetActor(),
		GetPlayer(),
		m_hTarget.Get()
	};

	for ( int i = 0; i < nEvaluators; ++i )
	{
		if ( !(this->*gm_Evaluators[i].pfnEvaluator)( args ) )
		{
			m_Timer.Reset();
			result = false;

			ScrCondDbgMsg( ( "%s failed on: %s\n", STRING( GetEntityName() ), gm_Evaluators[ i ].pszName ) );

			break;
		}
	}

	if ( result )
	{
		ScrCondDbgMsg( ( "%s waiting... %f\n", STRING( GetEntityName() ), m_Timer.GetRemaining() ) );
	}

	if ( result && m_Timer.Expired() )
	{
		ScrCondDbgMsg( ( "%s firing output OnConditionsSatisfied\n", STRING( GetEntityName() ) ) );

		// Default behavior for now, provide worldcraft option later.
		Disable();
		m_OnConditionsSatisfied.FireOutput( this, this );
	}
	
	SetThinkTime();
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::Enable( void )
{
	m_hActor = gEntList.FindEntityByName( NULL, m_Actor, NULL );
	m_hTarget = gEntList.FindEntityByName( NULL, m_target, NULL );
	
	if ( m_Actor != NULL_STRING && !m_hActor.Get() )
	{
		Msg( "Warning: Spawning AI script conditions associated with an non-existant NPC\n" );
		Disable();
		return;
	}

	if ( m_hActor && !m_hActor->MyNPCPointer() )
	{
		Warning( "Script condition warning: warning actor is not an NPC\n" );
		Disable();
		return;
	}

	SetThink( &CAI_ScriptConditions::EvaluationThink );
	SetThinkTime();

	if( m_flMaxTimeout > 0 )
	{
		m_Timeout.Set( random->RandomFloat( m_flMinTimeout, m_flMaxTimeout ), false );
	}
	else
	{
		m_Timeout.Set( m_flMinTimeout, false );
	}

	m_fDisabled = false;
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::Disable( void )
{
	SetThink( NULL );

	m_fDisabled = true;
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::InputEnable( inputdata_t &inputdata )
{
	Enable();
}

//-----------------------------------------------------------------------------

void CAI_ScriptConditions::InputDisable( inputdata_t &inputdata )
{
	Disable();
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::IsInFOV( CBaseEntity *pViewer, CBaseEntity *pViewed, float fov, bool bTrueCone )
{
	CBaseCombatCharacter *pCombatantViewer = pViewer->MyCombatCharacterPointer();

	if ( fov < 360 && pCombatantViewer )
	{
		Vector vLookDir;
		Vector vActorDir;

		// Halve the fov. As expressed here, fov is the full size of the viewcone.
		float flFovDotResult;
		flFovDotResult = cos( DEG2RAD( fov / 2 ) );

		if( bTrueCone )
		{
			// 3D Check
			vLookDir = pCombatantViewer->EyeDirection3D( );
			vActorDir = pViewed->EyePosition() - pViewer->EyePosition();
		}
		else
		{
			// 2D Check
			vLookDir = pCombatantViewer->EyeDirection2D( );
			vActorDir = pViewed->EyePosition() - pViewer->EyePosition();
			vActorDir.z = 0.0;
		}

		VectorNormalize(vActorDir);

		float fDotPr = DotProduct(vLookDir, vActorDir);

		if ( fDotPr < flFovDotResult )
		{
			if( fov < 0 )
			{
				// Designer has requested that the player
				// NOT be looking at this place.
				return true;
			}

			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_ScriptConditions::PlayerHasLineOfSight( CBaseEntity *pViewer, CBaseEntity *pViewed, bool fNot )
{
	CBaseCombatCharacter *pCombatantViewer = pViewer->MyCombatCharacterPointer();

	if( pCombatantViewer )
	{
		Vector vSrc;
		Vector vDest;
		trace_t tr;

		vSrc = pCombatantViewer->EyePosition();
		vDest = pViewed->EyePosition();

		AI_TraceLine( vSrc, vDest, MASK_PLAYERSOLID_BRUSHONLY, pViewer, COLLISION_GROUP_NONE, &tr );

		if( tr.fraction == 1.0 )
		{
			// Line of sight exists.
			if( fNot )
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			// No line of sight.
			if( fNot )
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}

//=============================================================================
