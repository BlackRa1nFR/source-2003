//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Player for HL2.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hl2_player.h"
#include "gamerules.h"
#include "trains.h"
#include "basehlcombatweapon_shared.h"
#include "vcollide_parse.h"
#include "in_buttons.h"
#include "AI_Interactions.h"
#include "igamemovement.h"
#include "ai_hull.h"
#include "hl2_shareddefs.h"
#include "func_monitor.h"
#include "Point_Camera.h"
#include "engine/IEngineSound.h"
#include "ndebugoverlay.h"
#include "iServervehicle.h"
#include "ivehicle.h"
#include "ai_basenpc.h"		
#include "weapon_physcannon.h"
#include "globals.h"

extern ConVar weapon_showproficiency;
extern proficiencyinfo_t g_WeaponProficiencyTable[];

ConVar max_command_dist( "player_max_command_dist", "1024" );

// This switches between the single primary weapon, and multiple weapons with buckets approach (jdw)
#define	HL2_SINGLE_PRIMARY_WEAPON_MODE	0

// This switches between the single primary weapon, and multiple weapons with buckets approach (jdw)
#define	HL2_SINGLE_PRIMARY_WEAPON_MODE	0

extern int gEvilImpulse101;

ConVar sv_autojump( "sv_autojump", "0" );

ConVar hl2_walkspeed( "hl2_walkspeed", "100" );
ConVar hl2_normspeed( "hl2_normspeed", "190" );
ConVar hl2_sprintspeed( "hl2_sprintspeed", "320" );

ConVar player_showpredictedposition( "player_showpredictedposition", "0" );
ConVar player_showpredictedposition_timestep( "player_showpredictedposition_timestep", "1.0" );

LINK_ENTITY_TO_CLASS( player, CHL2_Player );
PRECACHE_REGISTER(player);

CBaseEntity *FindEntityForward( CBasePlayer *pMe );

// Global Savedata for HL2 player
BEGIN_DATADESC( CHL2_Player )

	DEFINE_EMBEDDED( CHL2_Player, m_HL2Local ),
	DEFINE_FIELD( CHL2_Player, m_nControlClass, FIELD_INTEGER ),
	DEFINE_FIELD( CHL2_Player, m_fIsSprinting, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHL2_Player, m_fIsWalking, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHL2_Player, m_fCommanderMode, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHL2_Player, m_iNumSelectedNPCs, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CHL2_Player, m_vecMissPositions, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CHL2_Player, m_nNumMissPositions, FIELD_INTEGER ),

	// Suit power fields
	DEFINE_FIELD( CHL2_Player, m_bitsActiveDevices, FIELD_INTEGER ),
	DEFINE_FIELD( CHL2_Player, m_flSuitPowerLoad, FIELD_FLOAT ),

	DEFINE_FIELD( CHL2_Player, m_flIdleTime, FIELD_TIME ),
	DEFINE_FIELD( CHL2_Player, m_flMoveTime, FIELD_TIME ),
	DEFINE_FIELD( CHL2_Player, m_flLastDamageTime, FIELD_TIME ),
	DEFINE_FIELD( CHL2_Player, m_flTargetFindTime, FIELD_TIME ),

END_DATADESC()

CHL2_Player::CHL2_Player()
{
	m_nNumMissPositions	= 0;
}

//
// SUIT POWER DEVICES
//
#define SUITPOWER_CHARGE_RATE	4.2

#define bits_SUIT_DEVICE_SPRINT		0x00000001
#define bits_SUIT_DEVICE_FLASHLIGHT	0x00000002

CSuitPowerDevice SuitDeviceSprint( bits_SUIT_DEVICE_SPRINT, 6.25 );				// 100 units in 16 seconds
CSuitPowerDevice SuitDeviceFlashlight( bits_SUIT_DEVICE_FLASHLIGHT, 0.555 );	// 100 units in 3 minutes


IMPLEMENT_SERVERCLASS_ST(CHL2_Player, DT_HL2_Player)
	SendPropDataTable(SENDINFO_DT(m_HL2Local), &REFERENCE_SEND_TABLE(DT_HL2Local), SendProxy_SendLocalDataTable),
END_SEND_TABLE()


void CHL2_Player::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Allow pre-frame adjustments on the player
//-----------------------------------------------------------------------------
void CHL2_Player::PreThink(void)
{
	CheckExplosionEffects();
	if ( player_showpredictedposition.GetBool() )
	{
		Vector	predPos;

		UTIL_PredictedPosition( this, player_showpredictedposition_timestep.GetFloat(), &predPos );

		NDebugOverlay::Box( predPos, NAI_Hull::Mins( GetHullType() ), NAI_Hull::Maxs( GetHullType() ), 0, 255, 0, 0, 0.01f );
		NDebugOverlay::Line( GetAbsOrigin(), predPos, 0, 255, 0, 0, 0.01f );
	}

	// Riding a vehicle?
	if ( IsInAVehicle() )	
	{
		// make sure we update the client, check for timed damage and update suit even if we are in a vehicle
		UpdateClientData();		
		CheckTimeBasedDamage();
		CheckSuitUpdate();
		WaterMove();	
		return;
	}

	// This is an experiment of mine- autojumping! 
	// only affects you if sv_autojump is nonzero.
	if( (GetFlags() & FL_ONGROUND) && sv_autojump.GetFloat() != 0 )
	{
		// check autojump
		Vector vecCheckDir;

		vecCheckDir = GetAbsVelocity();

		float flVelocity = VectorNormalize( vecCheckDir );

		if( flVelocity > 200 )
		{
			// Going fast enough to autojump
			vecCheckDir = WorldSpaceCenter() + vecCheckDir * 34 - Vector( 0, 0, 16 );

			trace_t tr;

			UTIL_TraceHull( WorldSpaceCenter() - Vector( 0, 0, 16 ), vecCheckDir, NAI_Hull::Mins(HULL_TINY_CENTERED),NAI_Hull::Maxs(HULL_TINY_CENTERED), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER, &tr );
			
			//NDebugOverlay::Line( tr.startpos, tr.endpos, 0,255,0, true, 10 );

			if( tr.fraction == 1.0 && !tr.startsolid )
			{
				// Now trace down!
				UTIL_TraceLine( vecCheckDir, vecCheckDir - Vector( 0, 0, 64 ), MASK_PLAYERSOLID, this, COLLISION_GROUP_NONE, &tr );

				//NDebugOverlay::Line( tr.startpos, tr.endpos, 0,255,0, true, 10 );

				if( tr.fraction == 1.0 && !tr.startsolid )
				{
					// !!!HACKHACK
					// I KNOW, I KNOW, this is definitely not the right way to do this,
					// but I'm prototyping! (sjb)
					Vector vecNewVelocity = GetAbsVelocity();
					vecNewVelocity.z += 250;
					SetAbsVelocity( vecNewVelocity );
				}
			}
		}
	}

	int buttonsChanged;

	buttonsChanged = m_afButtonPressed | m_afButtonReleased;

	if( buttonsChanged & IN_SPEED )
	{
		// The state of the sprint button has changed. 
		if( IsSprinting() && !(m_afButtonPressed & IN_SPEED) )
		{
			StopSprinting();
		}
		else if( !IsSprinting() && !IsWalking() && (m_afButtonPressed & IN_SPEED) && !(m_nButtons & IN_DUCK) )
		{
			StartSprinting();
		}
	}
	else if( buttonsChanged & IN_WALK )
	{
		// The state of the WALK button has changed. 
		if( IsWalking() && !(m_afButtonPressed & IN_WALK) )
		{
			StopWalking();
		}
		else if( !IsWalking() && !IsSprinting() && (m_afButtonPressed & IN_WALK) && !(m_nButtons & IN_DUCK) )
		{
			StartWalking();
		}
	}



	if ( g_fGameOver || IsPlayerLockedInPlace() )
		return;         // finale

	ItemPreFrame( );
	WaterMove();

	if ( g_pGameRules && g_pGameRules->FAllowFlashlight() )
		m_Local.m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_Local.m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	// Operate suit accessories and manage power consumption/charge
	SuitPower_Update();

	// checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();
	
	CheckTimeBasedDamage();

	CheckSuitUpdate();

	if (m_lifeState >= LIFE_DYING)
	{
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
		AddFlag( FL_ONTRAIN );
	else 
		RemoveFlag( FL_ONTRAIN );

	// Train speed control
	if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
	{
		CBaseEntity *pTrain = GetGroundEntity();
		float vel;

		if ( pTrain )
		{
			if ( !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) )
				pTrain = NULL;
		}
		
		if ( !pTrain )
		{
			if ( GetActiveWeapon() && (GetActiveWeapon()->ObjectCaps() & FCAP_DIRECTIONAL_USE) )
			{
				m_iTrain = TRAIN_ACTIVE | TRAIN_NEW;

				if ( m_nButtons & IN_FORWARD )
				{
					m_iTrain |= TRAIN_FAST;
				}
				else if ( m_nButtons & IN_BACK )
				{
					m_iTrain |= TRAIN_BACK;
				}
				else
				{
					m_iTrain |= TRAIN_NEUTRAL;
				}
				return;
			}
			else
			{
				trace_t trainTrace;
				// Maybe this is on the other side of a level transition
				UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,-38), 
					MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trainTrace );

				if ( trainTrace.fraction != 1.0 && trainTrace.m_pEnt )
					pTrain = trainTrace.m_pEnt;


				if ( !pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(GetContainingEntity(pev)) )
				{
//					Warning( "In train mode with no train!\n" );
					m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
					m_iTrain = TRAIN_NEW|TRAIN_OFF;
					return;
				}
			}
		}
		else if ( !( GetFlags() & FL_ONGROUND ) || pTrain->HasSpawnFlags( SF_TRACKTRAIN_NOCONTROL ) || (m_nButtons & (IN_MOVELEFT|IN_MOVERIGHT) ) )
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
			m_iTrain = TRAIN_NEW|TRAIN_OFF;
			return;
		}

		SetAbsVelocity( vec3_origin );
		vel = 0;
		if ( m_afButtonPressed & IN_FORWARD )
		{
			vel = 1;
			pTrain->Use( this, this, USE_SET, (float)vel );
		}
		else if ( m_afButtonPressed & IN_BACK )
		{
			vel = -1;
			pTrain->Use( this, this, USE_SET, (float)vel );
		}

		if (vel)
		{
			m_iTrain = TrainSpeed(pTrain->m_flSpeed, ((CFuncTrackTrain*)pTrain)->GetMaxSpeed());
			m_iTrain |= TRAIN_ACTIVE|TRAIN_NEW;
		}
	} 
	else if (m_iTrain & TRAIN_ACTIVE)
	{
		m_iTrain = TRAIN_NEW; // turn off train
	}


	//
	// If we're not on the ground, we're falling. Update our falling velocity.
	//
	if ( !( GetFlags() & FL_ONGROUND ) )
	{
		m_Local.m_flFallVelocity = -GetAbsVelocity().z;
	}

	if ( m_afPhysicsFlags & PFLAG_ONBARNACLE )
	{
		SetAbsVelocity( vec3_origin );
	}
	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	//Update weapon's ready status
	UpdateWeaponPosture();
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Class_T  CHL2_Player::Classify ( void )
{
	// If player controlling another entity?  If so, return this class
	if (m_nControlClass != CLASS_NONE)
	{
		return m_nControlClass;
	}
	else
	{
		if(IsInAVehicle())
		{
			IServerVehicle *pVehicle = GetVehicle();
			return pVehicle->ClassifyPassenger( this, CLASS_PLAYER );
		}
		else
		{
			return CLASS_PLAYER;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CHL2_Player::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if ( interactionType == g_interactionBarnacleVictimDangle )
		return false;
	
	if (interactionType ==	g_interactionBarnacleVictimReleased)
	{
		m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
		SetMoveType( MOVETYPE_WALK );
		return true;
	}
	else if (interactionType ==	g_interactionBarnacleVictimGrab)
	{
		m_afPhysicsFlags |= PFLAG_ONBARNACLE;
		ClearUseEntity();
		return true;
	}
	return false;
}


void CHL2_Player::PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	// Handle FL_FROZEN.
	if ( m_afPhysicsFlags & PFLAG_ONBARNACLE )
	{
		ucmd->frametime = 0.0;
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
		ucmd->upmove = 0;
	}

	//Update our movement information
	if ( ( ucmd->forwardmove != 0 ) || ( ucmd->sidemove != 0 ) || ( ucmd->upmove != 0 ) )
	{
		m_flIdleTime -= ucmd->frametime * 2.0f;
		
		if ( m_flIdleTime < 0.0f )
		{
			m_flIdleTime = 0.0f;
		}

		m_flMoveTime += ucmd->frametime;

		if ( m_flMoveTime > 4.0f )
		{
			m_flMoveTime = 4.0f;
		}
	}
	else
	{
		m_flIdleTime += ucmd->frametime;
		
		if ( m_flIdleTime > 4.0f )
		{
			m_flIdleTime = 4.0f;
		}

		m_flMoveTime -= ucmd->frametime * 2.0f;
		
		if ( m_flMoveTime < 0.0f )
		{
			m_flMoveTime = 0.0f;
		}
	}

	//Msg("Player time: [ACTIVE: %f]\t[IDLE: %f]\n", m_flMoveTime, m_flIdleTime );

	BaseClass::PlayerRunCommand( ucmd, moveHelper );
}

void CHL2_Player::Touch( CBaseEntity *pOther )
{
	if ( pOther == GetGroundEntity() )
		return;

	if ( pOther->GetMoveType() != MOVETYPE_VPHYSICS || pOther->GetSolid() != SOLID_VPHYSICS )
		return;

	IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
	if ( !pPhys || !pPhys->IsMoveable() )
		return;

	SetTouchedPhysics( true );
}

//-----------------------------------------------------------------------------
// Purpose: Sets HL2 specific defaults.
//-----------------------------------------------------------------------------
void CHL2_Player::Spawn(void)
{
	SetModel( "models/player.mdl" );
    g_ulModelIndexPlayer = GetModelIndex();

	BaseClass::Spawn();

	//
	// Our player movement speed is set once here. This will override the cl_xxxx
	// cvars unless they are set to be lower than this.
	//
	//m_flMaxspeed = 320;

	InitSprinting();

	SuitPower_SetCharge( 100 );

	m_iNumSelectedNPCs = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::InitSprinting( void )
{
	StopSprinting();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::StartSprinting( void )
{
	if( m_HL2Local.m_flSuitPower < 33 )
	{
		// Don't sprint unless there's a reasonable
		// amount of suit power.
		EmitSound(  "HL2Player.SprintNoPower" );
		return;
	}

	if( !SuitPower_AddDevice( SuitDeviceSprint ) )
		return;

	EmitSound( "HL2Player.SprintStart" );

	SetMaxSpeed( hl2_sprintspeed.GetFloat() );
	m_fIsSprinting = true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::StopSprinting( void )
{
	if ( m_bitsActiveDevices & SuitDeviceSprint.GetDeviceID() )
	{
		SuitPower_RemoveDevice( SuitDeviceSprint );
	}
	SetMaxSpeed( hl2_normspeed.GetFloat() );
	m_fIsSprinting = false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::StartWalking( void )
{
	SetMaxSpeed( hl2_walkspeed.GetFloat() );
	m_fIsWalking = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::StopWalking( void )
{
	SetMaxSpeed( hl2_normspeed.GetFloat() );
	m_fIsWalking = false;
}

class CPhysicsPlayerCallback : public IPhysicsPlayerControllerEvent
{
public:
	int ShouldMoveTo( IPhysicsObject *pObject, const Vector &position )
	{
		CHL2_Player *pPlayer = (CHL2_Player *)pObject->GetGameData();
		if ( pPlayer->TouchedPhysics() )
		{
			return 0;
		}
		return 1;
	}
};

static CPhysicsPlayerCallback playerCallback;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2_Player::InitVCollision( void )
{
	BaseClass::InitVCollision();

	// Setup the HL2 specific callback.
	IPhysicsPlayerController *pPlayerController = GetPhysicsController();
	if ( pPlayerController )
	{
		pPlayerController->SetEventHandler( &playerCallback );
	}
}


CHL2_Player::~CHL2_Player( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Add this NPC to the NPC's that are about to receive an order.
// Input  : *pNPC - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2_Player::SelectNPC( CAI_BaseNPC *pNPC )
{
	if ( pNPC->IsSelected() )
	{
		// Unselect
		pNPC->PlayerSelect( false );
		m_iNumSelectedNPCs--;
		return false;
	}

	if( !pNPC->IsPlayerAlly() || !pNPC->CanBeSelected() )
	{
		return false;
	}

	// Select
	pNPC->PlayerSelect( true );
	m_iNumSelectedNPCs++;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
//-----------------------------------------------------------------------------
void CHL2_Player::SetCommanderMode( bool set )
{
	m_fCommanderMode = set;

	if( set )
	{
		ShowViewModel(false);
	}
	else
	{
		ShowViewModel(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// bHandled - indicates whether to continue delivering this order to
// all allies. Allows us to stop delivering certain types of orders once we find
// a suitable candidate. (like picking up a single weapon. We don't wish for
// all allies to respond and try to pick up one weapon).
//----------------------------------------------------------------------------- 
bool CHL2_Player::CommanderExecuteOne( CAI_BaseNPC *pNpc, const trace_t &trace )
{
	if ( trace.m_pEnt && !trace.DidHitWorld() )
	{
		return pNpc->TargetOrder( trace.m_pEnt );
	}
	else if ( pNpc->IsSelected() )
	{
		pNpc->MoveOrder( trace.endpos );
	}
	
	return true;
}

void CHL2_Player::CommanderExecute()
{
	int i;
	trace_t tr;
	Vector vecTarget;

	Vector forward;
	EyeVectors( &forward );
	
	//---------------------------------
	
	UTIL_TraceLine( EyePosition(), EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	
	if ( tr.DidHitWorld() )
	{
		vecTarget = tr.endpos;

		// Back up from whatever we hit so that there's enough space at the 
		// target location for a bounding box.
		vecTarget -= forward * 48;

		// Now trace down. 
		UTIL_TraceLine( vecTarget, vecTarget - Vector( 0, 0, 512 ), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

		if( tr.fraction == 1.0 )
		{
			CancelCommanderMode();
			return;
		}
	}

	//---------------------------------
	// If the trace hits an NPC, send all ally NPCs a "target" order. Always
	// goes to targeted one first
	CAI_BaseNPC * pTargetNpc = (tr.m_pEnt) ? tr.m_pEnt->MyNPCPointer() : NULL;
	
	bool bHandled = false;
	if( pTargetNpc )
	{
		bHandled = !CommanderExecuteOne( pTargetNpc, tr );
	}
	
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();

	for ( i = 0; !bHandled && i < nAIs; i++ )
	{
		if ( ppAIs[i] != pTargetNpc && ppAIs[i]->IsPlayerAlly() )
		{
			bHandled = !CommanderExecuteOne( ppAIs[i], tr );
		}
		Assert( nAIs == g_AI_Manager.NumAIs() ); // not coded to support mutating set of NPCs
	}

	//---------------------------------
	for ( i = 0; i < nAIs; i++ )
	{
		if ( ppAIs[i]->IsPlayerAlly() )
		{
			ppAIs[i]->PlayerSelect( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2_Player::ItemPostFrame( void )
{
	if( IsInCommanderMode() )
	{
		if( m_nButtons & IN_ATTACK )
		{
			// Debounce the button
			m_nButtons &= ~IN_ATTACK;

			// Prevent weapon firing for a bit
			m_flNextAttack = gpGlobals->curtime + 0.2;

			CommanderExecute();

			SetCommanderMode( false );
		}

		if( m_nButtons & IN_ATTACK2 )
		{
			// Debounce the button
			m_nButtons &= ~IN_ATTACK2;

			// Prevent weapon firing for a bit
			CBaseCombatWeapon *pWeapon = GetActiveWeapon();
			if( pWeapon )
			{
				pWeapon->m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
				pWeapon->m_flNextSecondaryAttack = gpGlobals->curtime + 0.2;
			}

			CancelCommanderMode();
			SetCommanderMode( false );
		}
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2_Player::UnselectAllNPCs()
{
	CBaseEntity *pEnt = gEntList.FirstEnt();

	while( pEnt )
	{
		CAI_BaseNPC *pNPC;
		pNPC = pEnt->MyNPCPointer();

		if( pNPC && pNPC->IsSelected() )
		{
			pNPC->PlayerSelect( false );
		}

		pEnt = gEntList.NextEnt( pEnt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Select everyone in the map (right now) who claims to be an ally.
//			so long as they're visible and fairly nearby.
//-----------------------------------------------------------------------------
int CHL2_Player::SelectAllAllies()
{
	CBaseEntity *pEnt = gEntList.FirstEnt();
	int count = 0;

	while( pEnt )
	{
		CAI_BaseNPC *pNPC;

		pNPC = pEnt->MyNPCPointer();

		if( pNPC && pNPC->IsAlive() && pNPC->IsPlayerAlly() )
		{
#if 0
			float flDist = ( GetAbsOrigin() - pNPC->GetAbsOrigin() ).Length();

			if( flDist <= max_command_dist.GetFloat() )
			{
				SelectNPC( pNPC );
				count++;
			}
#endif
			// No distance checks right now.
			SelectNPC( pNPC );
			count++;
		}

		pEnt = gEntList.NextEnt( pEnt );
	}

	return count;
}

//-----------------------------------------------------------------------------
// Enter/exit commander mode, manage ally selection.
//-----------------------------------------------------------------------------
void CHL2_Player::CommanderMode()
{
	if( IsInCommanderMode() )
	{
		// select or deselect an NPC
		CBaseEntity *pEntity = FindEntityForward( this );
		CAI_BaseNPC *pNPC;

		if( pEntity )
		{
			pNPC = pEntity->MyNPCPointer();
		}
		else
		{
#if 0
			// Command all selected ents to follow player.
			CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
			int nAIs = g_AI_Manager.NumAIs();

			for ( int i = 0; i < nAIs; i++ )
			{
				if ( ppAIs[i]->IsPlayerAlly() )
				{
					ppAIs[i]->TargetOrder( this );
					ppAIs[i]->PlayerSelect( false );
				}
				Assert( nAIs == g_AI_Manager.NumAIs() );
			}
			CancelCommanderMode();
#endif
			return;
		}

		if( pNPC )
		{
			float flDist;
			flDist = ( GetAbsOrigin() - pNPC->GetAbsOrigin() ).Length();

			if( flDist <= max_command_dist.GetFloat() )
			{
				SelectNPC( pNPC );
			}
		}

		if( m_iNumSelectedNPCs < 1 )
		{
			// Drop out of command mode if we unselect our last guy.
			SetCommanderMode( false );
		}
	}
	else
	{
		SetCommanderMode( true );

		// Select a single NPC if we're pointed at him. 
		// Otherwise, select all of my allies.
		CBaseEntity *pEntity = FindEntityForward( this );
		CAI_BaseNPC *pNPC;

		if( pEntity )
		{
			// Select a single NPC.
			pNPC = pEntity->MyNPCPointer();
			if( pNPC )
			{
/*
				float flDist;
				flDist = ( GetAbsOrigin() - pNPC->GetAbsOrigin() ).Length();
				if( flDist <= max_command_dist.GetFloat() )
				{
					// We know this guy's visible, cause the trace hit him.
					// but make sure he's not too far away.
					SelectNPC( pNPC );
				}
				else
				{
					SetCommanderMode( false );
				}
*/
				SelectNPC( pNPC );
				return;
			}
		}

		// Select all allies
		if( !SelectAllAllies() )
		{
			SetCommanderMode( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iImpulse - 
//-----------------------------------------------------------------------------
void CHL2_Player::CheatImpulseCommands( int iImpulse )
{
	switch( iImpulse )
	{
	case 50:
	{
		CommanderMode();
		break;
	}

	default:
		BaseClass::CheatImpulseCommands( iImpulse );
	}
}

bool CHL2_Player::ShouldTransmit( const edict_t *recipient, 
								    const void *pvs, int clientArea )
{
	// Allow me to introduce myself to, er, myself.
	// I.e., always update the recipient player data even if it's nodraw (first person mode)
	if ( recipient == pev )
		return true;

	return BaseClass::ShouldTransmit( recipient, pvs, clientArea );
}


void CHL2_Player::SetupVisibility( unsigned char *pvs, unsigned char *pas )
{
	BaseClass::SetupVisibility( pvs, pas );
	
	CPointCamera *pCameraEnt = NULL;
	while( pCameraEnt = gEntList.NextEntByClass( pCameraEnt ) )
	{
		pCameraEnt->SetActive( false );
	}
	
	// FIXME: cache previously seen monitor as a first guess?  Probably doesn't make sense to
	// optimize for the case where we are seeing a monitor.
	CFuncMonitor *pMonitorEnt = NULL;
	while( pMonitorEnt = gEntList.NextEntByClass( pMonitorEnt ) )
	{
		if( pMonitorEnt->IsOn() && pMonitorEnt->IsInPVS( edict(), pvs ) )
		{
			CPointCamera *pCameraEnt = pMonitorEnt->GetCamera();
			if( pCameraEnt )
			{
				engine->AddOriginToPVS( pCameraEnt->GetAbsOrigin() );
				pCameraEnt->SetActive( true );
			}
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::SuitPower_Update( void )
{
	if( SuitPower_ShouldRecharge() )
	{
		SuitPower_Charge( SUITPOWER_CHARGE_RATE * gpGlobals->frametime );
	}
	else if( m_bitsActiveDevices )
	{
		if( !SuitPower_Drain( m_flSuitPowerLoad * gpGlobals->frametime ) )
		{
			// TURN OFF ALL DEVICES!!
			if( IsSprinting() )
			{
				StopSprinting();
			}

			if( FlashlightIsOn() )
			{
				FlashlightTurnOff();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Charge battery fully, turn off all devices.
//-----------------------------------------------------------------------------
void CHL2_Player::SuitPower_Initialize( void )
{
	m_bitsActiveDevices = 0x00000000;
	m_HL2Local.m_flSuitPower = 100.0;
	m_flSuitPowerLoad = 0.0;
}


//-----------------------------------------------------------------------------
// Purpose: Interface to drain power from the suit's power supply.
// Input:	Amount of charge to remove (expressed as percentage of full charge)
// Output:	Returns TRUE if successful, FALSE if not enough power available.
//-----------------------------------------------------------------------------
bool CHL2_Player::SuitPower_Drain( float flPower )
{
	m_HL2Local.m_flSuitPower -= flPower;

	if( m_HL2Local.m_flSuitPower < 0.0 )
	{
		// Power is depleted!
		// Clamp and fail
		m_HL2Local.m_flSuitPower = 0.0;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Interface to add power to the suit's power supply
// Input:	Amount of charge to add
//-----------------------------------------------------------------------------
void CHL2_Player::SuitPower_Charge( float flPower )
{
	m_HL2Local.m_flSuitPower += flPower;

	if( m_HL2Local.m_flSuitPower > 100.0 )
	{
		// Full charge, clamp.
		m_HL2Local.m_flSuitPower = 100.0;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CHL2_Player::SuitPower_AddDevice( const CSuitPowerDevice &device )
{
	// Make sure this device is NOT active!!
	if( m_bitsActiveDevices & device.GetDeviceID() )
		return false;

	if( !IsSuitEquipped() )
		return false;

	m_bitsActiveDevices |= device.GetDeviceID();
	m_flSuitPowerLoad += device.GetDeviceDrainRate();
	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CHL2_Player::SuitPower_RemoveDevice( const CSuitPowerDevice &device )
{
	// Make sure this device is active!!
	if( ! (m_bitsActiveDevices & device.GetDeviceID()) )
		return false;

	if( !IsSuitEquipped() )
		return false;

	m_bitsActiveDevices &= ~device.GetDeviceID();
	m_flSuitPowerLoad -= device.GetDeviceDrainRate();

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CHL2_Player::FlashlightIsOn( void )
{
	return FBitSet(m_fEffects, EF_DIMLIGHT);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::FlashlightTurnOn( void )
{
	if( !SuitPower_AddDevice( SuitDeviceFlashlight ) )
		return;

	m_fEffects |= EF_DIMLIGHT;
	EmitSound( "HL2Player.FlashLightOn" );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::FlashlightTurnOff( void )
{
	if( !SuitPower_RemoveDevice( SuitDeviceFlashlight ) )
		return;

	m_fEffects &= ~EF_DIMLIGHT;
	EmitSound( "HL2Player.FlashLightOff" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CHL2_Player::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( info.GetDamage() > 0.0f )
	{
		m_flLastDamageTime = gpGlobals->curtime;
	}

	return BaseClass::OnTakeDamage( info );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CHL2_Player::ShouldShootMissTarget( CBaseCombatCharacter *pAttacker )
{
	if( gpGlobals->curtime > m_flTargetFindTime )
	{
		// Put this off into the future again.
		m_flTargetFindTime = gpGlobals->curtime + random->RandomFloat( 3, 5 );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iCount - 
//			iAmmoIndex - 
//			bSuppressSound - 
// Output : int
//-----------------------------------------------------------------------------
int CHL2_Player::GiveAmmo( int nCount, int nAmmoIndex, bool bSuppressSound)
{
	bool bCheckAutoSwitch = false;
	if (!GetAmmoCount(nAmmoIndex))
	{
		bCheckAutoSwitch = true;
	}

	int nAdd = BaseClass::GiveAmmo(nCount, nAmmoIndex, bSuppressSound);

	//
	// If I was dry on ammo for my best weapon and justed picked up ammo for it,
	// autoswitch to my best weapon now.
	//
	if (bCheckAutoSwitch)
	{
		CBaseCombatWeapon *pWeapon = g_pGameRules->GetNextBestWeapon(this, GetActiveWeapon());
		if (pWeapon && pWeapon->GetPrimaryAmmoType() == nAmmoIndex)
		{
			SwitchToNextBestWeapon(GetActiveWeapon());
		}
	}

	return nAdd;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pWeapon - 
//-----------------------------------------------------------------------------
void CHL2_Player::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
#if	HL2_SINGLE_PRIMARY_WEAPON_MODE

	if ( pWeapon->GetSlot() == WEAPON_PRIMARY_SLOT )
	{
		Weapon_DropSlot( WEAPON_PRIMARY_SLOT );
	}

#endif

	BaseClass::Weapon_Equip( pWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CHL2_Player::BumpWeapon( CBaseCombatWeapon *pWeapon )
{

#if	HL2_SINGLE_PRIMARY_WEAPON_MODE

	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	// ----------------------------------------
	// If I already have it just take the ammo
	// ----------------------------------------
	if (Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType())) 
	{
		//Only remove the weapon if we attained ammo from it
		if ( Weapon_EquipAmmoOnly( pWeapon ) == false )
			return false;

		// Only remove me if I have no ammo left
		// Can't just check HasAnyAmmo because if I don't use clips, I want to be removed, 
		if ( pWeapon->UsesClipsForAmmo1() && pWeapon->HasPrimaryAmmo() )
			return false;

		UTIL_Remove( pWeapon );
		return false;
	}
	// -------------------------
	// Otherwise take the weapon
	// -------------------------
	else 
	{
		//Make sure we're not trying to take a new weapon type we already have
		if ( Weapon_SlotOccupied( pWeapon ) )
		{
			CBaseCombatWeapon *pActiveWeapon = Weapon_GetSlot( WEAPON_PRIMARY_SLOT );

			if ( pActiveWeapon != NULL && pActiveWeapon->HasAnyAmmo() == false && Weapon_CanSwitchTo( pWeapon ) )
			{
				Weapon_Equip( pWeapon );
				return true;
			}

			//Attempt to take ammo if this is the gun we're holding already
			if ( Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType() ) )
			{
				Weapon_EquipAmmoOnly( pWeapon );
			}

			return false;
		}

		pWeapon->CheckRespawn();

		pWeapon->AddSolidFlags( FSOLID_NOT_SOLID );
		pWeapon->m_fEffects	|= EF_NODRAW;

		Weapon_Equip( pWeapon );

		EmitSound( "HL2Player.PickupWeapon" );
		
		return true;
	}
#else

	return BaseClass::BumpWeapon( pWeapon );

#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cmd - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2_Player::ClientCommand(const char *cmd)
{
#if	HL2_SINGLE_PRIMARY_WEAPON_MODE

	//Drop primary weapon
	if( stricmp( cmd, "DropPrimary" ) == 0 )
	{
		Weapon_DropSlot( WEAPON_PRIMARY_SLOT );
		return true;
	}

#endif

	if ( !stricmp( cmd, "emit" ) )
	{
		CSingleUserRecipientFilter filter( this );
		if ( engine->Cmd_Argc() > 1 )
		{
			EmitSound( filter, entindex(), engine->Cmd_Argv( 1 ) );
		}
		else
		{
			EmitSound( filter, entindex(), "Test.Sound" );
		}
		return true;
	}

	return BaseClass::ClientCommand( cmd );
}

void CHL2_Player::GetInVehicle( IServerVehicle *pVehicle, int nRole )
{
	BaseClass::GetInVehicle( pVehicle, nRole );

	// Orient the player to face forward! While we're in hierarchy,
	// the player's view angles are *relative*
	QAngle qRelativeAngles = GetLocalAngles();
	qRelativeAngles[ROLL] = 0;
	SnapEyeAngles( qRelativeAngles );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : void CBasePlayer::PlayerUse
//-----------------------------------------------------------------------------
void CHL2_Player::PlayerUse ( void )
{
	// Was use pressed or released?
	if ( ! ((m_nButtons | m_afButtonPressed | m_afButtonReleased) & IN_USE) )
		return;

	if ( m_afButtonPressed & IN_USE )
	{
		// Currently using a latched entity?
		if ( ClearUseEntity() )
		{
			return;
		}
		else
		{
			if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
			{
				m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
			else
			{	// Start controlling the train!
				CBaseEntity *pTrain = GetGroundEntity();
				if ( pTrain && !(m_nButtons & IN_JUMP) && (GetFlags() & FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pTrain->OnControls(this) )
				{
					m_afPhysicsFlags |= PFLAG_DIROVERRIDE;
					m_iTrain = TrainSpeed(pTrain->m_flSpeed, ((CFuncTrackTrain*)pTrain)->GetMaxSpeed());
					m_iTrain |= TRAIN_NEW;
					EmitSound( "HL2Player.TrainUse" );
					return;
				}
			}
		}
	}

	CBaseEntity *pUseEntity = FindUseEntity();

	// Found an object
	if ( pUseEntity )
	{
		//!!!UNDONE: traceline here to prevent +USEing buttons through walls			
		int caps = pUseEntity->ObjectCaps();
		variant_t emptyVariant;

		if ( m_afButtonPressed & IN_USE )
		{
			if ( pUseEntity->MyNPCPointer() )
			{
				EmitSound( "HL2Player.UseNPC" );
			}
			else
			{
				EmitSound( "HL2Player.Use" );
			}
		}

		if ( ( (m_nButtons & IN_USE) && (caps & FCAP_CONTINUOUS_USE) ) ||
			 ( (m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE|FCAP_ONOFF_USE)) ) )
		{
			if ( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ( (m_afButtonReleased & IN_USE) && (pUseEntity->ObjectCaps() & FCAP_ONOFF_USE) )	// BUGBUG This is an "off" use
		{
			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );
		}

#if	HL2_SINGLE_PRIMARY_WEAPON_MODE

		//Check for weapon pick-up
		if ( m_afButtonPressed & IN_USE )
		{
			CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon *>(pUseEntity);

			if ( ( pWeapon != NULL ) && ( Weapon_CanSwitchTo( pWeapon ) ) )
			{
				//Try to take ammo or swap the weapon
				if ( Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType() ) )
				{
					Weapon_EquipAmmoOnly( pWeapon );
				}
				else
				{
					Weapon_DropSlot( pWeapon->GetSlot() );
					Weapon_Equip( pWeapon );
				}
			}
		}
#endif
	}
	else if ( m_afButtonPressed & IN_USE )
	{
		EmitSound( "HL2Player.UseDeny" );
	}
}

ConVar	sv_show_crosshair_target( "sv_show_crosshair_target", "0" );

//-----------------------------------------------------------------------------
// Purpose: Updates the posture of the weapon from lowered to ready
//-----------------------------------------------------------------------------
void CHL2_Player::UpdateWeaponPosture( void )
{
//FIXME: Reimplement this when movements blends are working again
#if 0

	//Setup our viewmodel's movement speed
	CBaseViewModel *pVM = GetViewModel();

	//Send the poseparameter to the viewmodel
	if ( ( pVM != NULL ) && ( pVM->GetModelPtr() != NULL ) )
	{
		//Player's velocity ramped from slowest to fastest
		float moveBlend = RemapVal( GetAbsVelocity().Length(), 0.0f, MaxSpeed(), 0.0f, 1.0f );
		pVM->SetPoseParameter( "movement", moveBlend );
	}

#endif

	//FIXME: Don't do this twice!
	GetAutoaimVector( AUTOAIM_10DEGREES );

	//If we're over something
	if ( m_hAutoAimTarget != NULL )
	{
		Disposition_t dis = IRelationType( m_hAutoAimTarget );

		//Debug info for seeing what an object "cons" as
		if ( sv_show_crosshair_target.GetBool() )
		{
			int text_offset = BaseClass::DrawDebugTextOverlays();

			char tempstr[255];	

			switch ( dis )
			{
			case D_LI:
				Q_snprintf( tempstr, sizeof(tempstr), "Disposition: Like" );
				break;

			case D_HT:
				Q_snprintf( tempstr, sizeof(tempstr), "Disposition: Hate" );
				break;
			
			case D_FR:
				Q_snprintf( tempstr, sizeof(tempstr), "Disposition: Fear" );
				break;
			
			case D_NU:
				Q_snprintf( tempstr, sizeof(tempstr), "Disposition: Neutral" );
				break;

			default:
			case D_ER:
				Q_snprintf( tempstr, sizeof(tempstr), "Disposition: !!!ERROR!!!" );
				break;
			}

			//Draw the text
			NDebugOverlay::EntityText( m_hAutoAimTarget->entindex(), text_offset, tempstr, 0 );
		}

		//See if we hates it
		if ( dis == D_LI )
		{
			//We're over a friendly, drop our weapon
			if ( Weapon_Lower() == false )
			{
				//FIXME: We couldn't lower our weapon!
			}
		}
		else if ( dis == D_HT )
		{
			if ( Weapon_Ready() == false )
			{
				//FIXME: We couldn't raise our weapon!
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Lowers the weapon posture (for hovering over friendlies)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2_Player::Weapon_Lower( void )
{
	CBaseHLCombatWeapon *pWeapon = dynamic_cast<CBaseHLCombatWeapon *>(GetActiveWeapon());

	if ( pWeapon == NULL )
		return false;

	return pWeapon->Lower();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the weapon posture to normal
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2_Player::Weapon_Ready( void )
{
	CBaseHLCombatWeapon *pWeapon = dynamic_cast<CBaseHLCombatWeapon *>(GetActiveWeapon());

	if ( pWeapon == NULL )
		return false;

	return pWeapon->Ready();
}


void CHL2_Player::PickupObject( CBaseEntity *pObject )
{
	PlayerPickupObject( this, pObject );
}

//-----------------------------------------------------------------------------
// Purpose: Force the player to drop any physics objects he's carrying
//-----------------------------------------------------------------------------
void CHL2_Player::ForceDropOfCarriedPhysObjects( void )
{
	// Drop any objects being handheld.
	ClearUseEntity();

	// Then force the physcannon to drop anything it's holding, if it's our active weapon
	CWeaponPhysCannon *pCannon = dynamic_cast<CWeaponPhysCannon *>(GetActiveWeapon());
	if ( pCannon )
	{
		pCannon->ForceDrop();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2_Player::UpdateClientData( void )
{
	if (m_DmgTake || m_DmgSave || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = GetLocalOrigin();
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		damageOrigin = m_DmgOrigin;

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		m_DmgTake = clamp( m_DmgTake, 0, 255 );
		m_DmgSave = clamp( m_DmgSave, 0, 255 );

		CSingleUserRecipientFilter user( this );
		user.MakeReliable();
		UserMessageBegin( user, "Damage" );
			WRITE_BYTE( m_DmgSave );
			WRITE_BYTE( m_DmgTake );
			WRITE_LONG( visibleDamageBits );
			WRITE_FLOAT( damageOrigin.x );	//BUG: Should be fixed point (to hud) not floats
			WRITE_FLOAT( damageOrigin.y );	//BUG: However, the HUD does _not_ implement bitfield messages (yet)
			WRITE_FLOAT( damageOrigin.z );	//BUG: We use WRITE_VEC3COORD for everything else
		MessageEnd();
	
		m_DmgTake = 0;
		m_DmgSave = 0;
		m_bitsHUDDamage = m_bitsDamageType;
		
		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	BaseClass::UpdateClientData();
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CHL2_Player::EyeDirection2D( void )
{
	Vector vecReturn = EyeDirection3D();
	vecReturn.z = 0;

	return vecReturn;
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CHL2_Player::EyeDirection3D( void )
{
	Vector vecForward;

	AngleVectors( pl.v_angle, &vecForward );

	return vecForward;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CHL2_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	// Recalculate proficiency!
	SetCurrentWeaponProficiency( CalcWeaponProficiency( pWeapon ) );

	return BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CHL2_Player::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	int proficiency;

	proficiency = WEAPON_PROFICIENCY_PERFECT;

	if( weapon_showproficiency.GetBool() != 0 )
	{
		Msg("Player switched to %s, proficiency is %s\n", pWeapon->GetClassname(), g_WeaponProficiencyTable[ proficiency ].pszName );
	}

	return proficiency;
}

