//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hl1_player.h"
#include "gamerules.h"
#include "trains.h"
#include "hl1_basecombatweapon_shared.h"
#include "vcollide_parse.h"
#include "in_buttons.h"
#include "igamemovement.h"
#include "ai_hull.h"
#include "hl2_shareddefs.h"
#include "func_monitor.h"
#include "Point_Camera.h"
#include "ndebugoverlay.h"
#include "globals.h"
#include "ai_interactions.h"
#include "engine/IEngineSound.h"


ConVar player_showpredictedposition( "player_showpredictedposition", "0" );
ConVar player_showpredictedposition_timestep( "player_showpredictedposition_timestep", "1.0" );


LINK_ENTITY_TO_CLASS( player, CHL1_Player );
PRECACHE_REGISTER(player);


BEGIN_DATADESC( CHL1_Player )
	DEFINE_FIELD( CHL1_Player, m_nControlClass, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CHL1_Player, m_vecMissPositions, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CHL1_Player, m_nNumMissPositions, FIELD_INTEGER ),

	// Suit power fields
	DEFINE_FIELD( CHL1_Player, m_bitsActiveDevices, FIELD_INTEGER ),
	DEFINE_FIELD( CHL1_Player, m_flSuitPowerLoad, FIELD_FLOAT ),

	DEFINE_FIELD( CHL1_Player, m_flIdleTime, FIELD_TIME ),
	DEFINE_FIELD( CHL1_Player, m_flMoveTime, FIELD_TIME ),
	DEFINE_FIELD( CHL1_Player, m_flLastDamageTime, FIELD_TIME ),
	DEFINE_FIELD( CHL1_Player, m_flTargetFindTime, FIELD_TIME ),
	DEFINE_FIELD( CHL1_Player, m_bHasLongJump, FIELD_BOOLEAN ),
END_DATADESC()


//
// SUIT POWER DEVICES
//
#define SUITPOWER_CHARGE_RATE	4.2
#define bits_SUIT_DEVICE_FLASHLIGHT	0x00000002

CSuitPowerDevice SuitDeviceFlashlight( bits_SUIT_DEVICE_FLASHLIGHT, 0.555 );	// 100 units in 3 minutes

IMPLEMENT_SERVERCLASS_ST( CHL1_Player, DT_HL1Player )
	SendPropInt( SENDINFO( m_bHasLongJump ), 1, SPROP_UNSIGNED ),
END_SEND_TABLE()

CHL1_Player::CHL1_Player()
{
	m_nNumMissPositions	= 0;
}

void CHL1_Player::Precache( void )
{
	BaseClass::Precache();
	enginesound->PrecacheSound(SOUND_FLASHLIGHT_ON);

}


//-----------------------------------------------------------------------------
// Purpose: Allow pre-frame adjustments on the player
//-----------------------------------------------------------------------------
void CHL1_Player::PreThink(void)
{
	CheckExplosionEffects();
	if ( player_showpredictedposition.GetBool() )
	{
		Vector	predPos;

		UTIL_PredictedPosition( this, player_showpredictedposition_timestep.GetFloat(), &predPos );

		NDebugOverlay::Box( predPos, NAI_Hull::Mins( GetHullType() ), NAI_Hull::Maxs( GetHullType() ), 0, 255, 0, 0, 0.01f );
		NDebugOverlay::Line( GetAbsOrigin(), predPos, 0, 255, 0, 0, 0.01f );
	}

	int buttonsChanged;
	buttonsChanged = m_afButtonPressed | m_afButtonReleased;

	g_pGameRules->PlayerThink( this );

	if ( g_fGameOver || IsPlayerLockedInPlace() )
		return;         // intermission or finale

	ItemPreFrame( );
	WaterMove();

	if ( g_pGameRules && g_pGameRules->FAllowFlashlight() )
		m_Local.m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_Local.m_iHideHUD |= HIDEHUD_FLASHLIGHT;


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

	// THIS CODE DOESN'T SEEM TO DO ANYTHING!!!
	// WHY IS IT STILL HERE? (sjb)
	if (m_nButtons & IN_JUMP)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}

	// If trying to duck, already ducked, or in the process of ducking
	if ((m_nButtons & IN_DUCK) || (GetFlags() & FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) )
		Duck();

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

	//Find targets for NPC to shoot if they decide to miss us
	FindMissTargets();
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Class_T  CHL1_Player::Classify ( void )
{
	// If player controlling another entity?  If so, return this class
	if (m_nControlClass != CLASS_NONE)
	{
		return m_nControlClass;
	}
	else
	{
		return CLASS_PLAYER;
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
bool CHL1_Player::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if ( interactionType == g_interactionBarnacleVictimDangle )
	{
		TakeDamage ( CTakeDamageInfo( sourceEnt, sourceEnt, m_iHealth + ArmorValue(), DMG_SLASH | DMG_ALWAYSGIB ) );
		return true;
	}
	
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


void CHL1_Player::PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper)
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

void CHL1_Player::Touch( CBaseEntity *pOther )
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
// Purpose: Sets HL1specific defaults.
//-----------------------------------------------------------------------------
void CHL1_Player::Spawn(void)
{
	SetModel( "models/player.mdl" );
	g_ulModelIndexPlayer = GetModelIndex();

	BaseClass::Spawn();

	//
	// Our player movement speed is set once here. This will override the cl_xxxx
	// cvars unless they are set to be lower than this.
	//
	SetMaxSpeed( 320 );

	SetFOV( 90 );
}


class CPhysicsPlayerCallback : public IPhysicsPlayerControllerEvent
{
public:
	int ShouldMoveTo( IPhysicsObject *pObject, const Vector &position )
	{
		CHL1_Player *pPlayer = (CHL1_Player *)pObject->GetGameData();
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
void CHL1_Player::InitVCollision( void )
{
	BaseClass::InitVCollision();

	// Setup the HL2 specific callback.
	GetPhysicsController()->SetEventHandler( &playerCallback );
}


CHL1_Player::~CHL1_Player( void )
{
}

extern int gEvilImpulse101;
void CHL1_Player::CheatImpulseCommands( int iImpulse )
{
	switch( iImpulse )
	{
	case 101:
		gEvilImpulse101 = true;
		
		GiveNamedItem( "item_suit" );
		GiveNamedItem( "weapon_physgun" );
		GiveNamedItem( "weapon_crowbar" );
		GiveNamedItem( "weapon_glock" );
		GiveNamedItem( "weapon_357" );
		GiveNamedItem( "weapon_shotgun" );
		GiveNamedItem( "weapon_mp5" );
		GiveNamedItem( "weapon_satchel" );
		GiveNamedItem( "weapon_rpg" );
		GiveNamedItem( "weapon_crossbow" );
		GiveNamedItem( "weapon_egon" );
		GiveNamedItem( "weapon_tripmine" );
		GiveNamedItem( "weapon_gauss" );
		GiveNamedItem( "weapon_handgrenade" );
		GiveNamedItem( "weapon_snark" );
		GiveNamedItem( "weapon_hornetgun" );
		GiveAmmo( 250,	"9mmRound" );
		GiveAmmo( 36,	"357Round" );
		GiveAmmo( 125,	"Buckshot" );
		GiveAmmo( 10,	"MP5_grenade" );
		GiveAmmo( 5,	"RPG_Rocket" );
		GiveAmmo( 50,	"XBowBolt" );
		GiveAmmo( 100,	"Uranium" );
		GiveAmmo( 10,	"Grenade" );
		GiveAmmo( 8,	"Hornet" );
		GiveAmmo( 15,	"Snark" );
		GiveAmmo( 5,	"TripMine" );
		GiveAmmo( 5,	"Satchel" );

		gEvilImpulse101 = false;
		break;

	case 0:
	default:
		BaseClass::CheatImpulseCommands( iImpulse );
	}
}

bool CHL1_Player::ShouldTransmit( const edict_t *recipient, 
								    const void *pvs, int clientArea )
{
	// Allow me to introduce myself to, er, myself.
	// I.e., always update the recipient player data even if it's nodraw (first person mode)
	if ( recipient == pev )
		return true;

	return BaseClass::ShouldTransmit( recipient, pvs, clientArea );
}


void CHL1_Player::SetupVisibility( unsigned char *pvs, unsigned char *pas )
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
// Purpose: 
//-----------------------------------------------------------------------------
int	CHL1_Player::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( info.GetDamage() > 0.0f )
	{
		m_flLastDamageTime = gpGlobals->curtime;
	}

	return BaseClass::OnTakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL1_Player::FindMissTargets( void )
{
	if ( m_flTargetFindTime > gpGlobals->curtime )
		return;

	m_flTargetFindTime	= gpGlobals->curtime + 1.0f;
	m_nNumMissPositions = 0;

	CBaseEntity *pEnts[256];
	Vector		radius( 80, 80, 80);

	int numEnts = UTIL_EntitiesInBox( pEnts, 256, GetAbsOrigin()-radius, GetAbsOrigin()+radius, 0 );

	for ( int i = 0; i < numEnts; i++ )
	{
		if ( pEnts[i] == NULL )
			continue;

		if ( m_nNumMissPositions >= 16 )
			return;

		//See if it's a good target candidate
		if ( FClassnameIs( pEnts[i], "prop_dynamic" ) || 
			 FClassnameIs( pEnts[i], "dynamic_prop" ) || 
			 FClassnameIs( pEnts[i], "prop_physics" ) || 
			 FClassnameIs( pEnts[i], "physics_prop" ) )
		{
			//NDebugOverlay::Cross3D( pEnts[i]->WorldSpaceCenter(), -Vector(4,4,4), Vector(4,4,4), 0, 255, 0, true, 1.0f );

			m_vecMissPositions[m_nNumMissPositions++] = pEnts[i]->WorldSpaceCenter();
			continue;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Good position to shoot at
//-----------------------------------------------------------------------------
bool CHL1_Player::GetMissPosition( Vector *position )
{
	if ( m_nNumMissPositions == 0 )
		return false;

	(*position) = m_vecMissPositions[ random->RandomInt( 0, m_nNumMissPositions-1 ) ];

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL1_Player::SuitPower_Update( void )
{
	if( SuitPower_ShouldRecharge() )
	{
		SuitPower_Charge( SUITPOWER_CHARGE_RATE * gpGlobals->frametime );
	}
	else if( m_bitsActiveDevices )
	{
		if( !SuitPower_Drain( m_flSuitPowerLoad * gpGlobals->frametime ) )
		{
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
void CHL1_Player::SuitPower_Initialize( void )
{
	m_bitsActiveDevices = 0x00000000;
	m_flSuitPower = 100.0;
	m_flSuitPowerLoad = 0.0;
}


//-----------------------------------------------------------------------------
// Purpose: Interface to drain power from the suit's power supply.
// Input:	Amount of charge to remove (expressed as percentage of full charge)
// Output:	Returns TRUE if successful, FALSE if not enough power available.
//-----------------------------------------------------------------------------
bool CHL1_Player::SuitPower_Drain( float flPower )
{
	m_flSuitPower -= flPower;

	if( m_flSuitPower < 0.0 )
	{
		// Power is depleted!
		// Clamp and fail
		m_flSuitPower = 0.0;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Interface to add power to the suit's power supply
// Input:	Amount of charge to add
//-----------------------------------------------------------------------------
void CHL1_Player::SuitPower_Charge( float flPower )
{
	m_flSuitPower += flPower;

	if( m_flSuitPower > 100.0 )
	{
		// Full charge, clamp.
		m_flSuitPower = 100.0;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CHL1_Player::SuitPower_AddDevice( const CSuitPowerDevice &device )
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
bool CHL1_Player::SuitPower_RemoveDevice( const CSuitPowerDevice &device )
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
int CHL1_Player::FlashlightIsOn( void )
{
	return FBitSet(m_fEffects, EF_DIMLIGHT);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL1_Player::FlashlightTurnOn( void )
{
	if ( IsSuitEquipped() )
	{
		m_fEffects |= EF_DIMLIGHT;
		CPASAttenuationFilter filter( this );
		enginesound->EmitSound( filter, entindex(), CHAN_VOICE, SOUND_FLASHLIGHT_ON, 1, ATTN_NORM );	
//		EmitSound( "HL2Player.FlashLightOn" );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL1_Player::FlashlightTurnOff( void )
{
	m_fEffects &= ~EF_DIMLIGHT;
	CPASAttenuationFilter filter( this );
	enginesound->EmitSound( filter, entindex(), CHAN_VOICE, SOUND_FLASHLIGHT_ON, 1, ATTN_NORM );	
//	EmitSound( "HL2Player.FlashLightOff" );
}




//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void MatrixOrthogonalize( matrix3x4_t &matrix, int column )
{
	Vector columns[3];
	int i;

	for ( i = 0; i < 3; i++ )
	{
		MatrixGetColumn( matrix, i, columns[i] );
	}

	int index0 = column;
	int index1 = (column+1)%3;
	int index2 = (column+2)%3;

	columns[index2] = CrossProduct( columns[index0], columns[index1] );
	columns[index1] = CrossProduct( columns[index2], columns[index0] );
	VectorNormalize( columns[index2] );
	VectorNormalize( columns[index1] );
	MatrixSetColumn( columns[index1], index1, matrix );
	MatrixSetColumn( columns[index2], index2, matrix );
}

#define SIGN(x) ( (x) < 0 ? -1 : 1 )

static QAngle AlignAngles( const QAngle &angles, float cosineAlignAngle )
{
	matrix3x4_t alignMatrix;
	AngleMatrix( angles, alignMatrix );

	for ( int j = 0; j < 3; j++ )
	{
		Vector vec;
		MatrixGetColumn( alignMatrix, j, vec );
		for ( int i = 0; i < 3; i++ )
		{
			if ( fabs(vec[i]) > cosineAlignAngle )
			{
				vec[i] = SIGN(vec[i]);
				vec[(i+1)%3] = 0;
				vec[(i+2)%3] = 0;
				MatrixSetColumn( vec, j, alignMatrix );
				MatrixOrthogonalize( alignMatrix, j );
				break;
			}
		}
	}

	QAngle out;
	MatrixAngles( alignMatrix, out );
	return out;
}


static void TraceCollideAgainstBBox( const CPhysCollide *pCollide, const Vector &start, const Vector &end, const QAngle &angles, const Vector &boxOrigin, const Vector &mins, const Vector &maxs, trace_t *ptr )
{
	physcollision->TraceBox( boxOrigin, boxOrigin + (start-end), mins, maxs, pCollide, start, angles, ptr );

	if ( ptr->DidHit() )
	{
		ptr->endpos = start * (1-ptr->fraction) + end * ptr->fraction;
		ptr->startpos = start;
		ptr->plane.dist = -ptr->plane.dist;
		ptr->plane.normal *= -1;
	}
}

//-----------------------------------------------------------------------------

CGrabController::CGrabController( void )
{
	m_shadow.dampFactor = 1;
	m_shadow.teleportDistance = 0;
	m_errorTime = 0;
	m_error = 0;
	// make this controller really stiff!
	m_shadow.maxSpeed.Init( 1e4, 1e4, 1e4 );
	m_shadow.maxAngular.Init( 1e4, 1e4, 1e4 );

	m_attachedEntity = NULL;
}

void CGrabController::SetMaxImpulse( const Vector &linear, const AngularImpulse &angular )
{
	m_shadow.maxSpeed = linear;
	m_shadow.maxAngular = angular;
}

CGrabController::~CGrabController( void )
{
	DetachEntity();
}

void CGrabController::AttachEntity( CBaseEntity *pEntity, IPhysicsObject *pPhys, const Vector &position, const QAngle &rotation )
{
	pPhys->GetDamping( NULL, &m_saveRotDamping );
	float damping = 10;
	pPhys->SetDamping( NULL, &damping );

	m_controller = physenv->CreateMotionController( this );
	m_controller->AttachObject( pPhys );
	m_controller->SetPriority( IPhysicsMotionController::HIGH_PRIORITY );

	pPhys->Wake();
	PhysSetGameFlags( pPhys, FVPHYSICS_PLAYER_HELD );
	SetTargetPosition( position, rotation );
	m_attachedEntity = pEntity;
	m_flLoadWeight = pPhys->GetMass();
}

void CGrabController::DetachEntity( void )
{
	CBaseEntity *pEntity = GetAttached();
	if ( pEntity )
	{
		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if ( pPhys )
		{
			// on the odd chance that it's gone to sleep while under anti-gravity
			pPhys->Wake();
			pPhys->SetDamping( NULL, &m_saveRotDamping );
			PhysClearGameFlags( pPhys, FVPHYSICS_PLAYER_HELD );
		}
	}
	m_attachedEntity = NULL;
	physenv->DestroyMotionController( m_controller );
	m_controller = NULL;
}

IMotionEvent::simresult_e CGrabController::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	m_timeToArrive = pObject->ComputeShadowControl( m_shadow, m_timeToArrive, deltaTime );
	linear.Init();
	angular.Init();
	m_errorTime += deltaTime;
	return SIM_LOCAL_ACCELERATION;
}

// when looking level, hold bottom of object 8 inches below eye level
#define PLAYER_HOLD_LEVEL_EYES	-8

// when looking down, hold bottom of object 0 inches from feet
#define PLAYER_HOLD_DOWN_FEET	2

// when looking up, hold bottom of object 24 inches above eye level
#define PLAYER_HOLD_UP_EYES		24

// use a +/-30 degree range for the entire range of motion of pitch
#define PLAYER_LOOK_PITCH_RANGE	30

// player can reach down 2ft below his feet (otherwise he'll hold the object above the bottom)
#define PLAYER_REACH_DOWN_DISTANCE	24

class CPlayerPickupController : public CBaseEntity
{
	DECLARE_CLASS( CPlayerPickupController, CBaseEntity );
public:
	void Init( CBasePlayer *pPlayer, CBaseEntity *pObject );
	void Shutdown();
	bool OnControls( CBaseEntity *pControls ) { return true; }
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void ComputePlayerMatrix( matrix3x4_t &out );
	void CheckObjectPosition( Vector &position, const QAngle &angles, const Vector &oldPosition );


private:
	CGrabController		m_grabController;
	CBasePlayer			*m_pPlayer;
	Vector				m_positionPlayerSpace;
	QAngle				m_anglesPlayerSpace;
};

LINK_ENTITY_TO_CLASS( player_pickup, CPlayerPickupController );


void CPlayerPickupController::Init( CBasePlayer *pPlayer, CBaseEntity *pObject )
{
	m_pPlayer = pPlayer;

	IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();
	Vector position;
	QAngle angles;
	pPhysics->GetPosition( &position, &angles );
	m_grabController.SetMaxImpulse( Vector(20*100,20*100,20*100), AngularImpulse(20*180,20*180,20*180) );
	m_grabController.AttachEntity( pObject, pPhysics, position, angles );
	// Holster player's weapon
	if ( m_pPlayer->GetActiveWeapon() )
	{
		if ( !m_pPlayer->GetActiveWeapon()->Holster() )
		{
			Shutdown();
			return;
		}
	}

	m_pPlayer->m_Local.m_iHideHUD |= HIDEHUD_WEAPONS;
	m_pPlayer->SetUseEntity( this );
	matrix3x4_t tmp;
	ComputePlayerMatrix( tmp );
	VectorITransform( position, tmp, m_positionPlayerSpace );

	// UNDONE: This algorithm needs a bit more thought.  REVISIT.
	// put the bottom of the object arms' length below eye level
	// get bottommost point of object
	Vector bottom = physcollision->CollideGetExtent( pPhysics->GetCollide(), vec3_origin, angles, Vector(0,0,-1) );

	// get the real eye origin
	Vector playerEye = pPlayer->EyePosition();

	// move target up so that bottom of object is at PLAYER_HOLD_LEVEL z in local space
//	float delta = PLAYER_HOLD_LEVEL_EYES - bottom.z - m_positionPlayerSpace.z;
	float delta = 0;

	// player can reach down 2ft below his feet
	float maxPickup = (playerEye.z + PLAYER_HOLD_LEVEL_EYES) - (pPlayer->GetAbsMins().z - PLAYER_REACH_DOWN_DISTANCE);

	delta = clamp( delta, pPlayer->WorldAlignMins().z, maxPickup );
	m_positionPlayerSpace.z += delta;
	m_anglesPlayerSpace = TransformAnglesToLocalSpace( angles, tmp );

	m_anglesPlayerSpace = AlignAngles( m_anglesPlayerSpace, DOT_30DEGREE );
	
	// re-transform and check
	angles = TransformAnglesToWorldSpace( m_anglesPlayerSpace, tmp );
	VectorTransform( m_positionPlayerSpace, tmp, position );
	// hackhack: Move up to eye position for the check
	float saveZ = position.z;
	position.z = playerEye.z;
	CheckObjectPosition( position, angles, position );
	
	// move back to original position
	position.z = saveZ;

	VectorITransform( position, tmp, m_positionPlayerSpace );
}

void CPlayerPickupController::Shutdown()
{
	m_pPlayer->SetUseEntity( NULL );
	m_grabController.DetachEntity();
	if ( m_pPlayer->GetActiveWeapon() )
	{
		m_pPlayer->GetActiveWeapon()->Deploy();
	}

	m_pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONS;
	Remove();
}

void CPlayerPickupController::ComputePlayerMatrix( matrix3x4_t &out )
{
	QAngle angles = m_pPlayer->EyeAngles();
	Vector origin = m_pPlayer->EyePosition();
	
	// 0-360 / -180-180
	angles.x = AngleDistance( angles.x, 0 );
	angles.x = clamp( angles.x, -PLAYER_LOOK_PITCH_RANGE, PLAYER_LOOK_PITCH_RANGE );

	float feet = m_pPlayer->GetAbsMins().z;
	float eyes = origin.z;
	float zoffset = 0;
	// moving up (negative pitch is up)
	if ( angles.x < 0 )
	{
		zoffset = RemapVal( angles.x, 0, -PLAYER_LOOK_PITCH_RANGE, PLAYER_HOLD_LEVEL_EYES, PLAYER_HOLD_UP_EYES );
	}
	else
	{
		zoffset = RemapVal( angles.x, 0, PLAYER_LOOK_PITCH_RANGE, PLAYER_HOLD_LEVEL_EYES, PLAYER_HOLD_DOWN_FEET + (feet - eyes) );
	}
//	origin.z += zoffset;
	angles.x = 0;
	AngleMatrix( angles, origin, out );
}

void CPlayerPickupController::CheckObjectPosition( Vector &position, const QAngle &angles, const Vector &oldPosition )
{
	CBaseEntity *pAttached = m_grabController.GetAttached();
	trace_t tr;
	// move radially away from the player and check for space
	Vector offsetDir = position - m_pPlayer->GetAbsOrigin();
	offsetDir.z = 0;
	VectorNormalize(offsetDir);
	Vector startSweep = position + offsetDir * pAttached->EntitySpaceSize().Length();

	TraceCollideAgainstBBox( pAttached->VPhysicsGetObject()->GetCollide(), startSweep, position, angles, 
			m_pPlayer->GetAbsOrigin(), m_pPlayer->WorldAlignMins(), m_pPlayer->WorldAlignMaxs(), &tr );

	if ( tr.fraction != 1.0 )
	{
		// if you hit, back off 4 inches and set that as the target
		// otherwise, you made it all the way and the position is fine
		if ( tr.startsolid )
		{
			position = oldPosition;
		}
		else
		{
			position = tr.endpos + offsetDir * 2;
		}
	}
}


void CPlayerPickupController::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( ToBasePlayer(pActivator) == m_pPlayer )
	{
		CBaseEntity *pAttached = m_grabController.GetAttached();

		if ( !pAttached || useType == USE_OFF || m_grabController.ComputeError() > 12 || (m_pPlayer->m_nButtons & (IN_ATTACK|IN_ATTACK2)) )
		{
			Shutdown();
			return;
		}
		else if ( useType == USE_SET )
		{
			// update position
			matrix3x4_t tmp;
			ComputePlayerMatrix( tmp );
			Vector position;
			QAngle angles;
			VectorTransform( m_positionPlayerSpace, tmp, position );
			angles = TransformAnglesToWorldSpace( m_anglesPlayerSpace, tmp );
			CheckObjectPosition( position, angles, pAttached->GetAbsOrigin() );

			// check to see if object is still sitting on something
			trace_t tr;
			Vector down = Vector( 0, 0, -6 );
			UTIL_TraceEntity( pAttached, position, position + down, MASK_SOLID, &tr );
			// if trace made it then there's nothing left to sit on, break the tie
			if ( tr.fraction == 1 )
			{
				Shutdown();
				return;
			}
			else
			{
				m_grabController.SetTargetPosition( position, angles );
			}
		}
	}
}


// custom trace filter so we can ignore the player, but still hit debris
class CTraceFilterPickup : public CTraceFilterEntitiesOnly
{
public:
	CTraceFilterPickup( CBaseEntity *pEntity ) { m_pIgnore = pEntity; }
	virtual bool ShouldHitEntity( IHandleEntity *pEntity, int contentsMask )
	{
		if ( m_pIgnore )
		{
			if ( pEntity == m_pIgnore )
				return false;
		}
		return true;
	}
private:
	IServerEntity		*m_pIgnore;
};

void PlayerPickupObject( CBasePlayer *pPlayer, CBaseEntity *pObject )
{
	Vector forward;
	trace_t tr;
	pPlayer->GetVectors( &forward, NULL, NULL );
	CTraceFilterPickup filter( pPlayer );
	Ray_t ray;
	ray.Init( pPlayer->EyePosition(), pPlayer->EyePosition() + forward * 180 );
	enginetrace->TraceRay( ray, MASK_SOLID, &filter, &tr );
	if ( !tr.DidHit() || tr.m_pEnt != pObject )
		return;

	CPlayerPickupController *pController = (CPlayerPickupController *)CBaseEntity::Create( "player_pickup", pObject->GetAbsOrigin(), vec3_angle, pPlayer );
	if ( !pController )
		return;

	pController->Init( pPlayer, pObject );
}