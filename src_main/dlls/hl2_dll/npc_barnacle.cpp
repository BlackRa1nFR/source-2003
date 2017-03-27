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
#include "physics_prop_ragdoll.h"
#include "npc_barnacle.h"
#include "NPCEvent.h"
#include "gib.h"
#include "AI_Default.h"
#include "activitylist.h"
#include "hl2_player.h"
#include "vstdlib/random.h"
#include "physics_saverestore.h"
#include "vcollide_parse.h"
#include "vphysics/constraints.h"
#include "studio.h"
#include "bone_setup.h"

ConVar	sk_barnacle_health( "sk_barnacle_health","0");

//-----------------------------------------------------------------------------
// Private activities.
//-----------------------------------------------------------------------------
int ACT_BARNACLE_SLURP;			// Pulling the tongue up with prey on the end
int ACT_BARNACLE_BITE_HUMAN;	// Biting the head of a humanoid
int ACT_BARNACLE_CHEW_HUMAN;	// Slowly swallowing the humanoid
int ACT_BARNACLE_BARF_HUMAN;	// Spitting out human legs & gibs
int ACT_BARNACLE_TONGUE_WRAP;	// Wrapping the tongue around a target

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionBarnacleVictimDangle	= 0;
int	g_interactionBarnacleVictimReleased	= 0;
int	g_interactionBarnacleVictimGrab		= 0;

LINK_ENTITY_TO_CLASS( npc_barnacle, CNPC_Barnacle );

// Tongue Spring constants
#define BARNACLE_TONGUE_SPRING_CONSTANT_HANGING			10000
#define BARNACLE_TONGUE_SPRING_CONSTANT_LIFTING			10000
#define BARNACLE_TONGUE_SPRING_CONSTANT_LOWERING		7000
#define BARNACLE_TONGUE_SPRING_DAMPING					20

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------	
CNPC_Barnacle::CNPC_Barnacle(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_Barnacle::~CNPC_Barnacle( void )
{
	// Destroy the ragdoll->tongue tip constraint
  	if ( m_pConstraint )
  	{
  		physenv->DestroyConstraint( m_pConstraint );
  		m_pConstraint = NULL;
  	}
}

BEGIN_DATADESC( CNPC_Barnacle )

	DEFINE_FIELD( CNPC_Barnacle, m_flAltitude, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_Barnacle, m_cGibs, FIELD_INTEGER ),// barnacle loads up on gibs each time it kills something.
	DEFINE_FIELD( CNPC_Barnacle, m_fTongueExtended, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Barnacle, m_bLiftingPrey, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Barnacle, m_bSwallowingPrey, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Barnacle, m_flDigestFinish, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Barnacle, m_bPlayedPullSound, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Barnacle, m_flVictimHeight, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_Barnacle, m_iGrabbedBoneIndex, FIELD_INTEGER ),

	DEFINE_FIELD( CNPC_Barnacle, m_vecRoot, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CNPC_Barnacle, m_vecTip, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CNPC_Barnacle, m_hTongueRoot, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_Barnacle, m_hTongueTip, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_Barnacle, m_hRagdoll, FIELD_EHANDLE ),
	DEFINE_AUTO_ARRAY( CNPC_Barnacle, m_pRagdollBones, FIELD_MATRIX3X4_WORLDSPACE ),
	DEFINE_PHYSPTR( CNPC_Barnacle, m_pConstraint ),

	// Function pointers
	DEFINE_THINKFUNC( CNPC_Barnacle, BarnacleThink ),
	DEFINE_THINKFUNC( CNPC_Barnacle, WaitTillDead ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CNPC_Barnacle, DT_Barnacle )
	SendPropFloat(  SENDINFO( m_flAltitude ), 0, SPROP_NOSCALE),
	SendPropVector( SENDINFO( m_vecRoot ), 0, SPROP_COORD ),
	SendPropVector( SENDINFO( m_vecTip ), 0, SPROP_COORD ),
END_SEND_TABLE()


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
Class_T	CNPC_Barnacle::Classify ( void )
{
	return	CLASS_BARNACLE;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize absmin & absmax to the appropriate box
//-----------------------------------------------------------------------------
void CNPC_Barnacle::SetObjectCollisionBox( void )
{
	BaseClass::SetObjectCollisionBox();

	// Extend our bounding box downwards the length of the tongue
	Vector vecAbsMins = GetAbsMins();
	vecAbsMins.z -= m_flAltitude;
	SetAbsMins( vecAbsMins );
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
	case BARNACLE_AE_BITE:
		BitePrey();
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
	m_cGibs				= 0;
	m_bLiftingPrey		= false;
	m_bSwallowingPrey	= false;
	m_flDigestFinish	= 0;
	m_takedamage		= DAMAGE_YES;
	m_pConstraint		= NULL;

	InitBoneControllers();
	InitTonguePosition();

	// set eye position
	SetDefaultEyeOffset();

	SetActivity( ACT_IDLE );

	SetThink ( BarnacleThink );
	SetNextThink( gpGlobals->curtime + 0.5f );

	Relink();

	//Do not have a shadow
	m_fEffects |= EF_NOSHADOW;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::Activate( void )
{
	BaseClass::Activate();

	// Create our tongue tips
	m_hTongueRoot = CBarnacleTongueTip::CreateTongueRoot( m_vecRoot, QAngle(90,0,0) );
	m_hTongueTip = CBarnacleTongueTip::CreateTongueTip( m_hTongueRoot, m_vecTip, QAngle(0,0,0) );
	Assert( m_hTongueRoot && m_hTongueTip );
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

	float flTongueAdj = origin.z - GetAbsOrigin().z;
	m_vecRoot = origin - Vector(0,0,flTongueAdj);
	m_vecTip.Set( m_vecRoot.Get() - Vector(0,0,(float)m_flAltitude) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::BarnacleThink ( void )
{
	CBaseEntity *pTouchEnt;
	float flLength;

	SetNextThink( gpGlobals->curtime + 0.1f );

	UpdateTongue();

	// AI Disabled, don't do anything?
	if ( CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI )
		return;
	
	// Do we have an enemy?
	if ( GetEnemy() )
	{
		if ( m_bLiftingPrey )
		{	
			LiftPrey();
		}
	}
	else if ( m_hRagdoll )
	{
		// Slowly swallowing the ragdoll?
		if ( m_bSwallowingPrey )
		{
			SwallowPrey();
		}
		// Stay bloated as we digest
		else if ( m_flDigestFinish )
		{
			// Still digesting him>
			if ( m_flDigestFinish > gpGlobals->curtime )
			{
				if ( IsActivityFinished() )
				{
					SetActivity( ACT_IDLE );
				}

				// bite prey every once in a while
				if ( random->RandomInt(0,49) == 0 )
				{
					EmitSound( "NPC_Barnacle.Digest" );
				}
			}
			else
			{
				// Finished digesting
				LostPrey( true ); // Remove all evidence
				m_flDigestFinish = 0;
			}
		}
	}
	else
	{
		// Were we lifting prey?
		if ( m_bSwallowingPrey || m_bLiftingPrey ) 
		{
			// Something removed our prey.
			LostPrey( false );
		}

		// barnacle has no prey right now, so just idle and check to see if anything is touching the tongue.

		// If idle and no nearby client, don't think so often
		if ( !UTIL_FindClientInPVS( edict() ) )
		{
			SetNextThink( gpGlobals->curtime + random->RandomFloat(1,1.5) );	// Stagger a bit to keep barnacles from thinking on the same frame
		}

		if ( IsActivityFinished() )
		{
			// this is done so barnacle will fidget.
			SetActivity( ACT_IDLE );
		}

		if ( m_cGibs && random->RandomInt(0,99) == 1 )
		{
			// cough up a gib.
			CGib::SpawnRandomGibs( this, 1, GIB_HUMAN );
			m_cGibs--;

			EmitSound( "NPC_Barnacle.Digest" );
		}

		pTouchEnt = TongueTouchEnt( &flLength );

		if ( pTouchEnt != NULL && m_fTongueExtended )
		{
			// tongue is fully extended, and is touching someone.
			CBaseCombatCharacter *pBCC = dynamic_cast<CBaseCombatCharacter *>(pTouchEnt);

			// FIXME: humans should return neck position
			Vector vecGrabPos = pTouchEnt->EyePosition();
			if ( pBCC && pBCC->HandleInteraction( g_interactionBarnacleVictimGrab, &vecGrabPos, this ) )
			{
				AttachTongueToTarget( pTouchEnt, vecGrabPos );
			}
		}
		else
		{
			// calculate a new length for the tongue to be clear of anything else that moves under it. 
			if ( m_flAltitude < flLength )
			{
				// if tongue is higher than is should be, lower it kind of slowly.
				m_flAltitude += BARNACLE_PULL_SPEED;
				m_fTongueExtended = false;
			}
			else
			{
				// Restore the hanging spring constant 
				m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_HANGING );

				m_flAltitude = flLength;
				m_fTongueExtended = true;
			}

		}

	}

	// NDebugOverlay::Box( GetAbsOrigin() - Vector( 0, 0, m_flAltitude ), Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 255,255,255, 0, 0.1 );

	StudioFrameAdvance();
	DispatchAnimEvents( this );
}

//-----------------------------------------------------------------------------
// Purpose: Lift the prety stuck to our tongue up towards our mouth
//-----------------------------------------------------------------------------
void CNPC_Barnacle::LiftPrey( void )
{
	CBaseCombatCharacter *pVictim = GetEnemyCombatCharacterPointer();
	Assert( pVictim );

	// Drop the prey if it's been obscured by something
	trace_t tr;
	AI_TraceLine( GetAbsOrigin(), pVictim->GetAbsOrigin(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction < 1.0 && tr.m_pEnt != pVictim && tr.m_pEnt != m_hRagdoll )
	{
		LostPrey( true );
		return;
	}

	// We're still pulling the prey into position. Get the current position of the victim.
	Vector vecVictimPos = pVictim->GetAbsOrigin();
	Vector vecCheckPos = vecVictimPos;
	if ( pVictim->IsPlayer() )
	{
		// Use different detection for the player, so it bites his head nicely
		vecCheckPos.z = pVictim->EyePosition().z;
	}
	else if ( m_hRagdoll )
	{
		// Get the distance to the bone we've grabbed
		if ( m_hRagdoll )
		{
			QAngle vecBoneAngles;
			m_hRagdoll->GetBonePosition( m_iGrabbedBoneIndex, vecCheckPos, vecBoneAngles );
		}
		else
		{
			vecCheckPos.z = pVictim->GetAbsOrigin().z;
		}
	}

	//NDebugOverlay::Box( vecCheckPos, -Vector(10,10,10), Vector(10,10,10), 255,255,255, 0, 0.1 );

	// Height from the barnacle's origin to the point at which it bites
	float flBiteZOffset = 60.0;

	// Play a scream when we're almost within bite range
	if ( !m_bPlayedPullSound && m_flAltitude < (flBiteZOffset + 100) )
	{
		EmitSound( "NPC_Barnacle.Scream" );
		m_bPlayedPullSound = true;
	}

	// Figure out when the prey has reached our bite range
	if ( m_flAltitude < flBiteZOffset )
	{
		// If we've got a ragdoll, wait until the bone is down below the mouth.
		if ( m_hRagdoll )
		{
			// Stop sucking while we wait for the ragdoll to settle
			SetActivity( ACT_IDLE );

			Vector vecVelocity;
			AngularImpulse angVel;
			float flDelta = 4.0;

			// Only bite if the target bone is in the right position.
			Vector vecBitePoint = GetAbsOrigin();
			vecBitePoint.z -= flBiteZOffset;
			//NDebugOverlay::Box( vecBitePoint, -Vector(10,10,10), Vector(10,10,10), 0,255,0, 0, 0.1 );
			if ( (vecBitePoint.x - vecCheckPos.x) > flDelta || (vecBitePoint.y - vecCheckPos.y) > flDelta )
				return;
			// Right height?
			if ( (vecBitePoint.z - vecCheckPos.z) > flDelta )
			{
				// Slowly raise / lower the target into the right position
				if ( vecBitePoint.z > vecCheckPos.z )
				{
					// Pull the victim towards the mouth
					m_flAltitude -= 1;
					vecVictimPos.z += 1;
				}
				else
				{
					// We pulled 'em up too far, so lower them a little
					m_flAltitude += 1;
					vecVictimPos.z -= 1;
				}
				UTIL_SetOrigin ( GetEnemy(), vecVictimPos );
				return;
			}

			// Get the velocity of the bone we've grabbed, and only bite when it's not moving much
			studiohdr_t *pStudioHdr = m_hRagdoll->GetModelPtr();
			mstudiobone_t *pBone = pStudioHdr->pBone( m_iGrabbedBoneIndex );
			int iBoneIndex = pBone->physicsbone;
			ragdoll_t *pRagdoll = m_hRagdoll->GetRagdoll();
			IPhysicsObject *pRagdollPhys = pRagdoll->list[iBoneIndex].pObject;
			pRagdollPhys->GetVelocity( &vecVelocity, &angVel );
			if ( vecVelocity.Length() > 5 )
				return;
		}

		m_bLiftingPrey = false;

		// Start the bite animation. The anim event in it will finish the job.
		SetActivity( (Activity)ACT_BARNACLE_BITE_HUMAN );
	}
	else
	{
		// Pull the victim towards the mouth
		float flPull = fabs(sin( gpGlobals->curtime * 5 ));
		flPull *= BARNACLE_PULL_SPEED;
		m_flAltitude -= flPull;
		vecVictimPos.z += flPull;

		UTIL_SetOrigin ( GetEnemy(), vecVictimPos );

		// Apply forces to the attached ragdoll based upon the animations of the enemy, if the enemy is still alive.
		if ( m_hRagdoll && GetEnemy() && GetEnemy()->IsAlive() )
		{
			CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>( GetEnemy() );

			// Get the current bone matrix
			/*
			Vector pos[MAXSTUDIOBONES];
			Quaternion q[MAXSTUDIOBONES];
			matrix3x4_t pBoneToWorld[MAXSTUDIOBONES];
			CalcPoseSingle( pStudioHdr, pos, q, pAnimating->GetSequence(), pAnimating->m_flCycle, pAnimating->GetPoseParameterArray(), BONE_USED_BY_ANYTHING );
			Studio_BuildMatrices( pStudioHdr, vec3_angle, vec3_origin, pos, q, -1, pBoneToWorld, BONE_USED_BY_ANYTHING );


			// Apply the forces to the ragdoll
			RagdollApplyAnimationAsVelocity( *(m_hRagdoll->GetRagdoll()), pBoneToWorld );
			*/

			// Get the current bone matrix
			matrix3x4_t pBoneToWorld[MAXSTUDIOBONES];
			pAnimating->SetupBones( pBoneToWorld, BONE_USED_BY_ANYTHING );

			// Apply the forces to the ragdoll
			RagdollApplyAnimationAsVelocity( *(m_hRagdoll->GetRagdoll()), m_pRagdollBones, pBoneToWorld, 0.2 );

			// Store off the current bone matrix for next time
			pAnimating->SetupBones( m_pRagdollBones, BONE_USED_BY_ANYTHING );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Grab the specified target with our tongue
//-----------------------------------------------------------------------------
void CNPC_Barnacle::AttachTongueToTarget( CBaseEntity *pTouchEnt, Vector vecGrabPos )
{
	if ( RandomFloat(0,1) > 0.5 )
	{
		EmitSound( "NPC_Barnacle.PullPant" );
	}
	else
	{
		EmitSound( "NPC_Barnacle.TongueStretch" );
	}

	SetActivity( (Activity)ACT_BARNACLE_SLURP );

	SetEnemy( pTouchEnt );

	Vector origin = GetAbsOrigin();
	origin.z = pTouchEnt->GetAbsOrigin().z;
	pTouchEnt->SetLocalOrigin( origin );

	m_bLiftingPrey = true;// indicate that we should be lifting prey.
	m_flAltitude = (GetAbsOrigin().z - vecGrabPos.z);
	m_bPlayedPullSound  = false;

	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>(pTouchEnt);
	if ( pAnimating )
	{
		if ( pTouchEnt->IsPlayer() )
		{
			// The player doesn't ragdoll, so just grab him and pull him up manually
			IPhysicsObject *pPlayerPhys = pTouchEnt->VPhysicsGetObject();
			IPhysicsObject *pTonguePhys = m_hTongueTip->VPhysicsGetObject();

			Vector vecGrabPos = pTouchEnt->EyePosition();
			m_hTongueTip->Teleport( &vecGrabPos, NULL, NULL );

			constraint_fixedparams_t fixed;
			fixed.Defaults();
			fixed.InitWithCurrentObjectState( pTonguePhys, pPlayerPhys );
			fixed.constraint.Defaults();
			m_pConstraint = physenv->CreateFixedConstraint( pTonguePhys, pPlayerPhys, NULL, fixed );

			// Increase the tongue's spring constant while lifting 
			m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_LIFTING );
			UpdateTongue();
		}
		else
		{
			// Make a ragdoll for the guy, and hide him.
			pTouchEnt->AddSolidFlags( FSOLID_NOT_SOLID );
			pTouchEnt->Relink();

			// Find his head bone
			m_iGrabbedBoneIndex = -1;
			Vector vecNeckOffset = (pTouchEnt->EyePosition() - m_hTongueTip->GetAbsOrigin());
			studiohdr_t *pHdr = pAnimating->GetModelPtr();
			if ( pHdr )
			{
				int set = pAnimating->GetHitboxSet();
				for( int i = 0; i < pHdr->iHitboxCount(set); i++ )
				{
					mstudiobbox_t *pBox = pHdr->pHitbox( i, set );
					if ( !pBox )
						continue;
					
					if ( pBox->group == HITGROUP_HEAD )
					{
						m_iGrabbedBoneIndex = pBox->bone;
						break;
					}
				}
			}
			
			// HACK: Until we have correctly assigned hitgroups on our models, lookup the bones
			//		 for the models that we know are in the barnacle maps.
			//m_iGrabbedBoneIndex = pAnimating->LookupBone( "Bip01 L Foot" );
			if ( m_iGrabbedBoneIndex == -1 )
			{
				// Citizens, Conscripts
				m_iGrabbedBoneIndex = pAnimating->LookupBone( "Bip01 Head" );
			}
			if ( m_iGrabbedBoneIndex == -1 )
			{
				// Metrocops, Combine soldiers
				m_iGrabbedBoneIndex = pAnimating->LookupBone( "ValveBiped.Bip01_Head1" );
			}
			if ( m_iGrabbedBoneIndex == -1 )
			{
				// Vortigaunts
				m_iGrabbedBoneIndex = pAnimating->LookupBone( "ValveBiped.head" );
			}
			if ( m_iGrabbedBoneIndex == -1 )
			{
				// Bullsquids
				m_iGrabbedBoneIndex = pAnimating->LookupBone( "Bullsquid.Head_Bone1" );
			}

			if ( m_iGrabbedBoneIndex == -1 )
			{
				Warning("Couldn't find a bone to grab in the barnacle's target.\n" );

				// Just use the first bone
				m_iGrabbedBoneIndex = 0;
			}

			// Move the tip to the bone
			Vector vecBonePos;
			QAngle vecBoneAngles;
			pAnimating->GetBonePosition( m_iGrabbedBoneIndex, vecBonePos, vecBoneAngles );
			m_hTongueTip->Teleport( &vecBonePos, NULL, NULL );

			//NDebugOverlay::Box( vecBonePos, -Vector(5,5,5), Vector(5,5,5), 255,255,255, 0, 10.0 );

			// Create the ragdoll attached to tongue
			IPhysicsObject *pTonguePhysObject = m_hTongueTip->VPhysicsGetObject();
			m_hRagdoll = CreateServerRagdollAttached( pAnimating, vec3_origin, -1, COLLISION_GROUP_NONE, pTonguePhysObject, m_hTongueTip, 0, vecBonePos, m_iGrabbedBoneIndex, vec3_origin );
			m_hRagdoll->RemoveSolidFlags( FSOLID_NOT_SOLID );
			m_hRagdoll->Relink();
			m_hRagdoll->SetThink( NULL );
			m_hRagdoll->SetDamageEntity( pAnimating );
			m_hRagdoll->RecheckCollisionFilter();

			// Apply the target's current velocity to each of the ragdoll's bones
			Vector vecVelocity = pAnimating->GetGroundSpeedVelocity() * 0.5;
			ragdoll_t *pRagdoll = m_hRagdoll->GetRagdoll();
			for ( int i = 0; i < pRagdoll->listCount; i++ )
			{
				pRagdoll->list[i].pObject->AddVelocity( &vecVelocity, NULL );
			}

			// Now hide the actual enemy
			pTouchEnt->m_fEffects |= EF_NODRAW;

			// Increase the tongue's spring constant while lifting 
			m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_LIFTING );
			UpdateTongue();

			// Store off the current bone matrix so we have it next frame
			pAnimating->SetupBones( m_pRagdollBones, BONE_USED_BY_ANYTHING );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Prey is in position, bite them and start swallowing them
//-----------------------------------------------------------------------------
void CNPC_Barnacle::BitePrey( void )
{
	CBaseCombatCharacter *pVictim = GetEnemyCombatCharacterPointer();
	Assert( pVictim );

	EmitSound( "NPC_Barnacle.FinalBite" );

	m_flVictimHeight = GetEnemy()->WorldAlignSize().z;

	// Kill the victim instantly
	int iDamageType = DMG_SLASH | DMG_ALWAYSGIB; 
	if ( m_hRagdoll )
	{
		// We've got a ragdoll, so prevent this creating another one
		iDamageType |= DMG_REMOVENORAGDOLL;
		m_hRagdoll->SetDamageEntity( NULL );
	}
	// DMG_CRUSH because we don't wan't to impart physics forces
	pVictim->TakeDamage( CTakeDamageInfo( this, this, pVictim->m_iHealth, iDamageType | DMG_CRUSH ) );
	m_cGibs = 3;

	// Players are never swallowed, nor is anything we don't have a ragdoll for
	if ( !m_hRagdoll || pVictim->IsPlayer() )
	{
		LostPrey( false );
		return;
	}

	// Stop the ragdoll moving and start to pull the sucker up into our mouth
	m_bSwallowingPrey = true;
	IPhysicsObject *pTonguePhys = m_hTongueTip->VPhysicsGetObject();

	// Make it nonsolid to the world so we can pull it through the roof
	m_hRagdoll->DisableCollisions( g_PhysWorldObject );

	// Stop the tongue's spring getting in the way of swallowing
	m_hTongueTip->m_pSpring->SetSpringConstant( 0 );

	// Switch the tongue tip to shadow and drag it up
	pTonguePhys->SetShadow( Vector(1e4,1e4,1e4), AngularImpulse(1e4,1e4,1e4), false, false );
	pTonguePhys->UpdateShadow( m_hTongueTip->GetAbsOrigin(), m_hTongueTip->GetAbsAngles(), false, 0 );
	m_hTongueTip->SetMoveType( MOVETYPE_NOCLIP );
	m_hTongueTip->SetAbsVelocity( Vector(0,0,32) );
	m_hTongueTip->Relink();

	m_flAltitude = (GetAbsOrigin().z - m_hTongueTip->GetAbsOrigin().z);
}

//-----------------------------------------------------------------------------
// Purpose: Slowly swallow the prey whole. Only used on humanoids.
//-----------------------------------------------------------------------------
void CNPC_Barnacle::SwallowPrey( void )
{
	if ( IsActivityFinished() )
	{
		SetActivity( (Activity)ACT_BARNACLE_CHEW_HUMAN );
	}

	// Move the body up slowly
	Vector vecSwallowPos = m_hTongueTip->GetAbsOrigin();
	vecSwallowPos.z -= m_flVictimHeight;
	//NDebugOverlay::Box( vecSwallowPos, -Vector(5,5,5), Vector(5,5,5), 255,255,255, 0, 0.1 );

	// bite prey every once in a while
	if ( random->RandomInt(0,49) == 0 )
	{
		EmitSound( "NPC_Barnacle.Digest" );
	}

	// Fully swallowed it?
	float flDistanceToGo = GetAbsOrigin().z - vecSwallowPos.z;
	if ( flDistanceToGo <= 0 )
	{
		// He's dead jim
		m_bSwallowingPrey = false;
		m_hTongueTip->SetAbsVelocity( vec3_origin );
		m_flDigestFinish = gpGlobals->curtime + 10.0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove the fake ragdoll and bring the actual enemy back in view
//-----------------------------------------------------------------------------
void CNPC_Barnacle::RemoveRagdoll( bool bDestroyRagdoll )
{
	// Destroy the tongue tip constraint
  	if ( m_pConstraint )
  	{
  		physenv->DestroyConstraint( m_pConstraint );
  		m_pConstraint = NULL;
  	}

	// Remove the ragdoll
	if ( m_hRagdoll )
	{
		// Only destroy the ragdoll if told to. We might be just dropping
		// the ragdoll because the target was killed on the way up.
		m_hRagdoll->SetDamageEntity( NULL );
		DetachAttachedRagdoll( m_hRagdoll );
		if ( bDestroyRagdoll )
		{
			UTIL_Remove( m_hRagdoll );
		}
		m_hRagdoll = NULL;

		// Reduce the spring constant while we lower
		m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_LOWERING );

		// Unhide the enemy
		if ( GetEnemy() )
		{
			GetEnemy()->m_fEffects &= ~EF_NODRAW;
			GetEnemy()->RemoveSolidFlags( FSOLID_NOT_SOLID );
			GetEnemy()->Relink();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: For some reason (he was killed, etc) we lost the prey we were dragging towards our mouth.
//-----------------------------------------------------------------------------
void CNPC_Barnacle::LostPrey( bool bRemoveRagdoll )
{
	if ( GetEnemy() )
	{
		CBaseCombatCharacter *pVictim = GetEnemyCombatCharacterPointer();
		if ( pVictim )
		{
			pVictim->HandleInteraction( g_interactionBarnacleVictimReleased, NULL, this );
			pVictim->RemoveFlag( FL_ONGROUND );
		}
	}

	RemoveRagdoll( bRemoveRagdoll );
	m_bLiftingPrey = false;
	m_bSwallowingPrey = false;
	SetEnemy( NULL );

	// Remove our tongue's shadow object, in case we just finished swallowing something
	IPhysicsObject *pPhysicsObject = m_hTongueTip->VPhysicsGetObject();
	if ( pPhysicsObject && pPhysicsObject->GetShadowController() )
	{
		Vector vecCenter = WorldSpaceCenter();
		m_hTongueTip->Teleport( &vecCenter, NULL, &vec3_origin );

		// Reduce the spring constant while we lower
		m_hTongueTip->m_pSpring->SetSpringConstant( BARNACLE_TONGUE_SPRING_CONSTANT_LOWERING );

		// Start colliding with the world again
		pPhysicsObject->RemoveShadowController();
		m_hTongueTip->SetMoveType( MOVETYPE_VPHYSICS );
		pPhysicsObject->EnableMotion( true );
		pPhysicsObject->EnableGravity( true );
		pPhysicsObject->RecheckCollisionFilter();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the positions of the tongue points
//-----------------------------------------------------------------------------
void CNPC_Barnacle::UpdateTongue( void )
{
	// Set the spring's length to that of the tongue's extension
	m_hTongueTip->m_pSpring->SetSpringLength( m_flAltitude );

	// Update the tip's position
	m_vecTip = m_hTongueTip->GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barnacle::Event_Killed( const CTakeDamageInfo &info )
{
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;
	m_lifeState	= LIFE_DYING;

	Relink();

	// Are we lifting prey?
	if ( GetEnemy() )
	{
		// Cleanup
		LostPrey( true );
	}
	else if ( m_bSwallowingPrey && m_hRagdoll )
	{
		// We're swallowing a body. Make it stick inside us.
		m_hTongueTip->SetAbsVelocity( vec3_origin );

		m_hRagdoll->StopFollowingEntity();
		m_hRagdoll->SetMoveType( MOVETYPE_VPHYSICS );
		m_hRagdoll->SetAbsOrigin( m_hTongueTip->GetAbsOrigin() );
		m_hRagdoll->RemoveSolidFlags( FSOLID_NOT_SOLID );
		m_hRagdoll->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		m_hRagdoll->RecheckCollisionFilter();
		m_hRagdoll->Relink();
	}
	else
	{
		// Destroy the ragdoll->tongue tip constraint
  		if ( m_pConstraint )
  		{
  			physenv->DestroyConstraint( m_pConstraint );
  			m_pConstraint = NULL;
  		}
		// Remove our tongue pieces
		UTIL_Remove( m_hTongueTip );
		LostPrey( true );
	}

	UTIL_Remove( m_hTongueRoot );

	CGib::SpawnRandomGibs( this, 4, GIB_HUMAN );

	EmitSound( "NPC_Barnacle.Die" );

	SetActivity( ACT_DIESIMPLE );

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
		m_lifeState	= LIFE_DEAD;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Barnacle::Precache()
{
	engine->PrecacheModel("models/barnacle.mdl");
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
	AI_TraceLine ( GetAbsOrigin(), GetAbsOrigin() - Vector ( 0 , 0 , 2048 ), 
		MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	
	length = fabs( GetAbsOrigin().z - tr.endpos.z );
	// Pull it up a tad
	length = max(8, length - 16);
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

			// only clients and monsters
			if (	pList[i]					!= this				&& 
					IRelationType( pList[i] )	== D_HT				&& 
					pVictim->m_lifeState		!= LIFE_DEAD		&&
					pVictim->m_lifeState		!= LIFE_DYING		&&
					!( pVictim->GetFlags() & FL_NOTARGET )	)	
			{
				return pList[i];
			}
		}
	}

	return NULL;
}

//===============================================================================================================================
// BARNACLE TONGUE TIP
//===============================================================================================================================
// Crane tip
LINK_ENTITY_TO_CLASS( npc_barnacle_tongue_tip, CBarnacleTongueTip );

BEGIN_DATADESC( CBarnacleTongueTip )

	DEFINE_PHYSPTR( CBarnacleTongueTip, m_pSpring ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: To by usable by vphysics, this needs to have a phys model.
//-----------------------------------------------------------------------------
void CBarnacleTongueTip::Spawn( void )
{
	Precache();
	SetModel( "models/props_junk/rock001a.mdl" );
	m_fEffects |= EF_NODRAW;

	// We don't want this to be solid, because we don't want it to collide with the barnacle.
	SetSolid( SOLID_VPHYSICS );
	AddSolidFlags( FSOLID_NOT_SOLID );
	BaseClass::Spawn();

	m_pSpring = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBarnacleTongueTip::Precache( void )
{
	PrecacheModel( "models/props_junk/rock001a.mdl" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Activate/create the spring
//-----------------------------------------------------------------------------
bool CBarnacleTongueTip::CreateSpring( CBaseAnimating *pTongueRoot )
{
	IPhysicsObject *pPhysObject = VPhysicsGetObject();
	IPhysicsObject *pRootPhysObject = pTongueRoot->VPhysicsGetObject();
	Assert( pRootPhysObject );
	Assert( pPhysObject );

	// Root has huge mass, tip has little
	pRootPhysObject->SetMass( 1e6 );
	pPhysObject->SetMass( 100 );
	float damping = 3;
	pPhysObject->SetDamping( &damping, &damping );

	springparams_t spring;
	spring.constant = BARNACLE_TONGUE_SPRING_CONSTANT_HANGING;
	spring.damping = BARNACLE_TONGUE_SPRING_DAMPING;
	spring.naturalLength = (GetAbsOrigin() - pTongueRoot->GetAbsOrigin()).Length();
	spring.relativeDamping = 10;
	spring.startPosition = GetAbsOrigin();
	spring.endPosition = pTongueRoot->GetAbsOrigin();
	spring.useLocalPositions = false;
	m_pSpring = physenv->CreateSpring( pPhysObject, pRootPhysObject, &spring );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Create a barnacle tongue tip at the bottom of the tongue
//-----------------------------------------------------------------------------
CBarnacleTongueTip *CBarnacleTongueTip::CreateTongueTip( CBaseAnimating *pTongueRoot, const Vector &vecOrigin, const QAngle &vecAngles )
{
	CBarnacleTongueTip *pTip = (CBarnacleTongueTip *)CBaseEntity::Create( "npc_barnacle_tongue_tip", vecOrigin, vecAngles );
	if ( !pTip )
		return NULL;

	pTip->VPhysicsInitNormal( pTip->GetSolid(), pTip->GetSolidFlags(), false );
	if ( !pTip->CreateSpring( pTongueRoot ) )
		return NULL;

	// Don't collide with the world
	IPhysicsObject *pTipPhys = pTip->VPhysicsGetObject();

	// turn off all floating / fluid simulation
	pTipPhys->SetCallbackFlags( pTipPhys->GetCallbackFlags() & (~CALLBACK_DO_FLUID_SIMULATION) );
	
	return pTip;
}

//-----------------------------------------------------------------------------
// Purpose: Create a barnacle tongue tip at the root (i.e. inside the barnacle)
//-----------------------------------------------------------------------------
CBarnacleTongueTip *CBarnacleTongueTip::CreateTongueRoot( const Vector &vecOrigin, const QAngle &vecAngles )
{
	CBarnacleTongueTip *pTip = (CBarnacleTongueTip *)CBaseEntity::Create( "npc_barnacle_tongue_tip", vecOrigin, vecAngles );
	if ( !pTip )
		return NULL;

	pTip->AddSolidFlags( FSOLID_NOT_SOLID );

	// Disable movement on the root, we'll move this thing manually.
	pTip->VPhysicsInitShadow( false, false );
	pTip->SetMoveType( MOVETYPE_NONE );
	return pTip;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_barnacle, CNPC_Barnacle )

	// Register our interactions
	DECLARE_INTERACTION( g_interactionBarnacleVictimDangle )
	DECLARE_INTERACTION( g_interactionBarnacleVictimReleased )
	DECLARE_INTERACTION( g_interactionBarnacleVictimGrab )

	// Conditions
		
	// Tasks

	// Activities
	DECLARE_ACTIVITY( ACT_BARNACLE_SLURP )			// Pulling the tongue up with prey on the end
	DECLARE_ACTIVITY( ACT_BARNACLE_BITE_HUMAN )		// Biting the head of a humanoid
	DECLARE_ACTIVITY( ACT_BARNACLE_CHEW_HUMAN )		// Slowly swallowing the humanoid
	DECLARE_ACTIVITY( ACT_BARNACLE_BARF_HUMAN )		// Spitting out human legs & gibs
	DECLARE_ACTIVITY( ACT_BARNACLE_TONGUE_WRAP )	// Wrapping the tongue around a target

	// Schedules

AI_END_CUSTOM_NPC()
