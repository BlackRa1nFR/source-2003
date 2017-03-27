//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		barnacle - stationary ceiling mounted 'fishing' monster	
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hl1_npc_barnacle.h"
#include "NPCEvent.h"
#include "gib.h"
#include "AI_Default.h"
#include "activitylist.h"
#include "hl2_player.h"
#include "vstdlib/random.h"
#include "physics_saverestore.h"
#include "vcollide_parse.h"
#include "engine/IEngineSound.h"

ConVar	sk_barnacle_health( "sk_barnacle_health","0");

//-----------------------------------------------------------------------------
// Private activities.
//-----------------------------------------------------------------------------
static int ACT_EAT = 0;

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionBarnacleVictimDangle	= 0;
int	g_interactionBarnacleVictimReleased	= 0;
int	g_interactionBarnacleVictimGrab		= 0;

LINK_ENTITY_TO_CLASS( monster_barnacle, CNPC_Barnacle );
IMPLEMENT_CUSTOM_AI( monster_barnacle, CNPC_Barnacle );

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Barnacle::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CNPC_Barnacle);

	ADD_CUSTOM_ACTIVITY(CNPC_Barnacle, ACT_EAT);

	g_interactionBarnacleVictimDangle	= CBaseCombatCharacter::GetInteractionID();
	g_interactionBarnacleVictimReleased	= CBaseCombatCharacter::GetInteractionID();
	g_interactionBarnacleVictimGrab		= CBaseCombatCharacter::GetInteractionID();	
}


BEGIN_DATADESC( CNPC_Barnacle )

	DEFINE_FIELD( CNPC_Barnacle, m_flAltitude, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_Barnacle, m_flKillVictimTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Barnacle, m_cGibs, FIELD_INTEGER ),// barnacle loads up on gibs each time it kills something.
	DEFINE_FIELD( CNPC_Barnacle, m_fTongueExtended, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Barnacle, m_fLiftingPrey, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Barnacle, m_flTongueAdj, FIELD_FLOAT ),

	// Function pointers
	DEFINE_THINKFUNC( CNPC_Barnacle, BarnacleThink ),
	DEFINE_THINKFUNC( CNPC_Barnacle, WaitTillDead ),
END_DATADESC()


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
Class_T	CNPC_Barnacle::Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_Barnacle::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNACLE_AE_PUKEGIB:
		CGib::SpawnRandomGibs( this, 1, GIB_HUMAN );	
		break;
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CNPC_Barnacle::Spawn()
{
	Precache( );

	SetModel( "models/barnacle.mdl" );
	UTIL_SetSize( this, Vector(-16, -16, -32), Vector(16, 16, 0) );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_NONE );
	SetBloodColor( BLOOD_COLOR_GREEN );
	m_iHealth			= sk_barnacle_health.GetFloat();
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	m_flKillVictimTime	= 0;
	m_cGibs				= 0;
	m_fLiftingPrey		= FALSE;
	m_takedamage		= DAMAGE_YES;

	InitBoneControllers();
	InitTonguePosition();

	// set eye position
	SetDefaultEyeOffset();

	SetActivity ( ACT_IDLE );

	SetThink ( BarnacleThink );
	SetNextThink( gpGlobals->curtime + 0.5f );

	Relink();

	//Do not have a shadow
	m_fEffects |= EF_NOSHADOW;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Barnacle::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;
	if ( info.GetDamageType() & DMG_CLUB )
	{
		info.SetDamage( m_iHealth );
	}

	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
// Purpose: Initialize tongue position when first spawned
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Barnacle::InitTonguePosition( void )
{
	CBaseEntity *pTouchEnt;
	float flLength;

	pTouchEnt = TongueTouchEnt( &flLength );
	m_flAltitude = flLength;

	Vector origin;
	QAngle angle;

	GetAttachment( "TongueEnd", origin, angle );

	m_flTongueAdj = origin.z - GetAbsOrigin().z;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::BarnacleThink ( void )
{
	CBaseEntity *pTouchEnt;
	float flLength;

	SetNextThink( gpGlobals->curtime + 0.1f );

	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI)
	{
		// AI Disabled, don't do anything
	}
	else if ( GetEnemy() != NULL )
	{
// barnacle has prey.

		if ( !GetEnemy()->IsAlive() )
		{
			// someone (maybe even the barnacle) killed the prey. Reset barnacle.
			m_fLiftingPrey = FALSE;// indicate that we're not lifting prey.
			SetEnemy( NULL );
			return;
		}

		CBaseCombatCharacter* pVictim = GetEnemyCombatCharacterPointer();
		Assert( pVictim );

		if ( m_fLiftingPrey )
		{	

			if ( GetEnemy() != NULL && pVictim->m_lifeState == LIFE_DEAD )
			{
				// crap, someone killed the prey on the way up.
				SetEnemy( NULL );
				m_fLiftingPrey = FALSE;
				return;
			}

	// still pulling prey.
			Vector vecNewEnemyOrigin = GetEnemy()->GetLocalOrigin();
			vecNewEnemyOrigin.x = GetLocalOrigin().x;
			vecNewEnemyOrigin.y = GetLocalOrigin().y;

			// guess as to where their neck is
			// FIXME: remove, ask victim where their neck is
			vecNewEnemyOrigin.x -= 6 * cos(GetEnemy()->GetLocalAngles().y * M_PI/180.0);	
			vecNewEnemyOrigin.y -= 6 * sin(GetEnemy()->GetLocalAngles().y * M_PI/180.0);

			m_flAltitude -= BARNACLE_PULL_SPEED;
			vecNewEnemyOrigin.z += BARNACLE_PULL_SPEED;

			if ( fabs( GetLocalOrigin().z - ( vecNewEnemyOrigin.z + GetEnemy()->GetViewOffset().z - 12 ) ) < BARNACLE_BODY_HEIGHT )
			{
		// prey has just been lifted into position ( if the victim origin + eye height + 8 is higher than the bottom of the barnacle, it is assumed that the head is within barnacle's body )
				m_fLiftingPrey = FALSE;

				CPASAttenuationFilter filter( this );
				EmitSound( filter, entindex(), CHAN_WEAPON, "barnacle/bcl_bite3.wav", 1, ATTN_NORM );

				// Take a while to kill the player
				m_flKillVictimTime = gpGlobals->curtime + 10;
						
				if ( pVictim )
				{
					pVictim->HandleInteraction( g_interactionBarnacleVictimDangle, NULL, this );
					SetActivity ( (Activity)ACT_EAT );
				}
			}

			UTIL_SetOrigin ( GetEnemy(), vecNewEnemyOrigin );
		}
		else
		{
	// prey is lifted fully into feeding position and is dangling there.

			if ( m_flKillVictimTime != -1 && gpGlobals->curtime > m_flKillVictimTime )
			{
				// kill!
				if ( pVictim )
				{
					// DMG_CRUSH added so no physics force is generated
					pVictim->TakeDamage( CTakeDamageInfo( this, this, pVictim->m_iHealth, DMG_SLASH | DMG_ALWAYSGIB | DMG_CRUSH ) );
					m_cGibs = 3;
				}

				return;
			}

			// bite prey every once in a while
			if ( pVictim && ( random->RandomInt( 0, 49 ) == 0 ) )
			{
				CPASAttenuationFilter filter( this );
				
				
				switch ( random->RandomInt(0,2) )
				{
					case 0:	EmitSound( filter, entindex(), CHAN_WEAPON, "barnacle/bcl_chew1.wav", 1, ATTN_NORM ); break;
					case 1:	EmitSound( filter, entindex(), CHAN_WEAPON, "barnacle/bcl_chew2.wav", 1, ATTN_NORM );	break;
					case 2:	EmitSound( filter, entindex(), CHAN_WEAPON, "barnacle/bcl_chew3.wav", 1, ATTN_NORM );	break;
				}

				if ( pVictim )
				{
					pVictim->HandleInteraction( g_interactionBarnacleVictimDangle, NULL, this );
				}
			}
		}
	}
	else
	{
// barnacle has no prey right now, so just idle and check to see if anything is touching the tongue.

		// If idle and no nearby client, don't think so often
		if ( !UTIL_FindClientInPVS( edict() ) )
			SetNextThink( random->RandomFloat(1,1.5) );	// Stagger a bit to keep barnacles from thinking on the same frame

		if ( IsActivityFinished() )
		{// this is done so barnacle will fidget.
			SetActivity ( ACT_IDLE );
		}

		if ( m_cGibs && random->RandomInt(0,99) == 1 )
		{
			// cough up a gib.
			CGib::SpawnRandomGibs( this, 1, GIB_HUMAN );
			m_cGibs--;

			CPASAttenuationFilter filter( this );

			switch ( random->RandomInt(0,2) )
			{
				case 0:	EmitSound( filter, entindex(), CHAN_WEAPON, "barnacle/bcl_chew1.wav", 1, ATTN_NORM ); break;
				case 1:	EmitSound( filter, entindex(), CHAN_WEAPON, "barnacle/bcl_chew2.wav", 1, ATTN_NORM );	break;
				case 2:	EmitSound( filter, entindex(), CHAN_WEAPON, "barnacle/bcl_chew3.wav", 1, ATTN_NORM );	break;
			}
		}

		pTouchEnt = TongueTouchEnt( &flLength );

		if ( pTouchEnt != NULL && m_fTongueExtended )
		{
			// tongue is fully extended, and is touching someone.
			CBaseCombatCharacter* pBCC = (CBaseCombatCharacter *)pTouchEnt;

			// FIXME: humans should return neck position
			Vector vecGrabPos = pTouchEnt->GetAbsOrigin();

			if ( pBCC && pBCC->HandleInteraction( g_interactionBarnacleVictimGrab, &vecGrabPos, this ) )
			{
				CPASAttenuationFilter filter( this );
				EmitSound( filter, entindex(), CHAN_WEAPON, "barnacle/bcl_alert2.wav", 1, ATTN_NORM );

				SetSequenceByName ( "attack1" );

				SetEnemy( pTouchEnt );

				pTouchEnt->SetMoveType( MOVETYPE_FLY );
				pTouchEnt->SetAbsVelocity( vec3_origin );
				pTouchEnt->SetBaseVelocity( vec3_origin );
				Vector origin = GetAbsOrigin();
				origin.z = pTouchEnt->GetAbsOrigin().z;
				pTouchEnt->SetLocalOrigin( origin );
				
				m_fLiftingPrey = TRUE;// indicate that we should be lifting prey.
				m_flKillVictimTime = -1;// set this to a bogus time while the victim is lifted.

				m_flAltitude = (GetAbsOrigin().z - vecGrabPos.z);
			}
		}
		else
		{
			// calculate a new length for the tongue to be clear of anything else that moves under it. 
			if ( m_flAltitude < flLength )
			{
				// if tongue is higher than is should be, lower it kind of slowly.
				m_flAltitude += BARNACLE_PULL_SPEED;
				m_fTongueExtended = FALSE;
			}
			else
			{
				m_flAltitude = flLength;
				m_fTongueExtended = TRUE;
			}

		}

	}

	// ALERT( at_console, "tounge %f\n", m_flAltitude + m_flTongueAdj );
	// NDebugOverlay::Box( GetAbsOrigin() - Vector( 0, 0, m_flAltitude ), Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 255,255,255, 0, 0.1 );

	//SetBoneController( 0, 0 );//-(m_flAltitude + m_flTongueAdj) );
	SetBoneController( 0, -(m_flAltitude + m_flTongueAdj) );
	StudioFrameAdvance();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::Event_Killed( const CTakeDamageInfo &info )
{
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage		= DAMAGE_NO;

	Relink();

	if ( GetEnemy() != NULL )
	{
		CBaseCombatCharacter *pVictim = GetEnemyCombatCharacterPointer();

		if ( pVictim )
		{
			pVictim->HandleInteraction( g_interactionBarnacleVictimReleased, NULL, this );
		}
	}

	CGib::SpawnRandomGibs( this, 4, GIB_HUMAN );

	//EmitSound( "NPC_Barnacle.Die" );

	CPASAttenuationFilter filter( this );
				
	switch ( random->RandomInt ( 0, 1 ) )
	{
		case 0:	EmitSound( filter, entindex(), CHAN_WEAPON, "barnacle/bcl_die1.wav", 1, ATTN_NORM );	break;
		case 1:	EmitSound( filter, entindex(), CHAN_WEAPON, "barnacle/bcl_die3.wav", 1, ATTN_NORM );	break;
	}

	SetActivity ( ACT_DIESIMPLE );
	SetBoneController( 0, 0 );

	StudioFrameAdvance();

	SetNextThink( gpGlobals->curtime + 0.1f );
	SetThink ( WaitTillDead );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::WaitTillDead ( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	StudioFrameAdvance();
	DispatchAnimEvents ( this );

	if ( IsActivityFinished() )
	{
		// death anim finished. 
		StopAnimation();
		SetThink ( NULL );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Barnacle::Precache()
{
	engine->PrecacheModel("models/barnacle.mdl");

	enginesound->PrecacheSound("barnacle/bcl_alert2.wav");//happy, lifting food up
	enginesound->PrecacheSound("barnacle/bcl_bite3.wav");//just got food to mouth
	enginesound->PrecacheSound("barnacle/bcl_chew1.wav");
	enginesound->PrecacheSound("barnacle/bcl_chew2.wav");
	enginesound->PrecacheSound("barnacle/bcl_chew3.wav");
	enginesound->PrecacheSound("barnacle/bcl_die1.wav" );
	enginesound->PrecacheSound("barnacle/bcl_die3.wav" );

	BaseClass::Precache();
}	

//=========================================================
// TongueTouchEnt - does a trace along the barnacle's tongue
// to see if any entity is touching it. Also stores the length
// of the trace in the int pointer provided.
//=========================================================
#define BARNACLE_CHECK_SPACING	8
CBaseEntity *CNPC_Barnacle::TongueTouchEnt ( float *pflLength )
{
	trace_t		tr;
	float		length;

	// trace once to hit architecture and see if the tongue needs to change position.
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() - Vector ( 0 , 0 , 2048 ), 
		MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	
	length = fabs( GetAbsOrigin().z - tr.endpos.z );
	// Pull it up a tad
	length -= 16;
	if ( pflLength )
	{
		*pflLength = length;
	}

	Vector delta = Vector( BARNACLE_CHECK_SPACING, BARNACLE_CHECK_SPACING, 0 );
	Vector mins = GetAbsOrigin() - delta;
	Vector maxs = GetAbsOrigin() + delta;
	maxs.z = GetAbsOrigin().z;
	mins.z -= length;

	CBaseEntity *pList[10];
	int count = UTIL_EntitiesInBox( pList, 10, mins, maxs, (FL_CLIENT|FL_NPC) );
	if ( count )
	{
		for ( int i = 0; i < count; i++ )
		{
			CBaseCombatCharacter *pVictim = ToBaseCombatCharacter( pList[ i ] );

			bool bCanHurt = false;

			if ( IRelationType( pList[i] ) == D_HT || IRelationType( pList[i] ) == D_FR )
				 bCanHurt = true;

			if ( pList[i] != this && bCanHurt == true && pVictim->m_lifeState == LIFE_ALIVE )
			{
				return pList[i];
			}
		}
	}

	return NULL;
}
