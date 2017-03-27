//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Functions dealing with the player.
//
//=============================================================================

#include "cbase.h"
#include "const.h"
#include "baseplayer_shared.h"
#include "trains.h"
#include "soundent.h"
#include "gib.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "entityapi.h"
#include "entitylist.h"
#include "eventqueue.h"
#include "worldsize.h"
#include "isaverestore.h"
#include "globalstate.h"
#include "basecombatweapon.h"
#include "ai_basenpc.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_networkmanager.h"
#include "ammodef.h"
#include "mathlib.h"
#include "ndebugoverlay.h"
#include "baseviewmodel.h"
#include "in_buttons.h"
#include "client.h"
#include "team.h"
#include "particle_smokegrenade.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movehelper_server.h"
#include "igamemovement.h"
#include "saverestoretypes.h"
#include "iservervehicle.h"
#include "movevars_shared.h"
#include "vcollide_parse.h"
#include "player_command.h"
#include "vehicle_base.h"
#include "AI_Criteria.h"
#include "globals.h"
#include "usermessages.h"
#include "gamevars_shared.h"
#include "world.h"

static ConVar	sv_aim( "sv_aim", "1", FCVAR_ARCHIVE | FCVAR_SERVER, "Autoaim state" );
static ConVar dsp_explosion_effect_duration( "dsp_explosion_effect_duration", "4", 0, "How long to apply confusion/ear-ringing effect after taking damage from blast." );

// TIME BASED DAMAGE AMOUNT
// tweak these values based on gameplay feedback:
#define PARALYZE_DURATION	2		// number of 2 second intervals to take damage
#define PARALYZE_DAMAGE		1.0		// damage to take each 2 second interval

#define NERVEGAS_DURATION	2
#define NERVEGAS_DAMAGE		5.0

#define POISON_DURATION		5
#define POISON_DAMAGE		2.0

#define RADIATION_DURATION	2
#define RADIATION_DAMAGE	1.0

#define ACID_DURATION		2
#define ACID_DAMAGE			5.0

#define SLOWBURN_DURATION	2
#define SLOWBURN_DAMAGE		1.0

#define SLOWFREEZE_DURATION	2
#define SLOWFREEZE_DAMAGE	1.0


extern bool		g_fDrawLines;
int				gEvilImpulse101;

bool gInitHUD = true;

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);
int MapTextureTypeStepType(char chTextureType);
extern void	SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern void AddMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity );


#define CMD_MOSTRECENT 0

//#define	FLASH_DRAIN_TIME	 1.2 //100 units/3 minutes
//#define	FLASH_CHARGE_TIME	 0.2 // 100 units/20 seconds  (seconds per unit)


//#define PLAYER_MAX_SAFE_FALL_DIST	20// falling any farther than this many feet will inflict damage
//#define	PLAYER_FATAL_FALL_DIST		60// 100% damage inflicted if player falls this many feet
//#define	DAMAGE_PER_UNIT_FALLEN		(float)( 100 ) / ( ( PLAYER_FATAL_FALL_DIST - PLAYER_MAX_SAFE_FALL_DIST ) * 12 )
//#define MAX_SAFE_FALL_UNITS			( PLAYER_MAX_SAFE_FALL_DIST * 12 )

// player damage adjusters
ConVar	sk_player_head( "sk_player_head","2" );
ConVar	sk_player_chest( "sk_player_chest","1" );
ConVar	sk_player_stomach( "sk_player_stomach","1" );
ConVar	sk_player_arm( "sk_player_arm","1" );
ConVar	sk_player_leg( "sk_player_leg","1" );

// pl
BEGIN_SIMPLE_DATADESC( CPlayerState )
	DEFINE_FIELD( CPlayerState, netname, FIELD_STRING ),
	DEFINE_FIELD( CPlayerState, v_angle, FIELD_VECTOR ),
	DEFINE_FIELD( CPlayerState, deadflag, FIELD_BOOLEAN ),

	// this is always set to true on restore, don't bother saving it.
	// DEFINE_FIELD( CPlayerState, fixangle, FIELD_INTEGER ),
	// DEFINE_FIELD( CPlayerState, anglechangetotal, FIELD_FLOAT ),
	// DEFINE_FIELD( CPlayerState, anglechangefinal, FIELD_FLOAT ),
END_DATADESC()

// Global Savedata for player
BEGIN_DATADESC( CBasePlayer )

	DEFINE_EMBEDDED( CBasePlayer, m_Local ),
	DEFINE_EMBEDDED( CBasePlayer, pl ),

	DEFINE_FIELD( CBasePlayer, m_nButtons, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonReleased, FIELD_INTEGER ),

	//DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
	DEFINE_FIELD( CBasePlayer, m_iObserverMode, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iObserverLastMode, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_hObserverTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_bForcedObserverMode, FIELD_BOOLEAN ),
//	DEFINE_FIELD( CBasePlayer, m_hAutoAimTarget, FIELD_EHANDLE ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_szAnimExtension, FIELD_CHARACTER ),
//	DEFINE_FIELD( CBasePlayer, m_Activity, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_vecAdditionalPVSOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_vecCameraPVSOrigin, FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( CBasePlayer, m_hUseEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iRespawnFrames, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_hVehicle, FIELD_EHANDLE ),

	// recreate, don't restore
	// DEFINE_FIELD( CBasePlayer, m_CommandContext, CUtlVector < CCommandContext > ),
	//DEFINE_FIELD( CBasePlayer, m_pPhysicsController, FIELD_POINTER ),
	//DEFINE_FIELD( CBasePlayer, m_pShadowStand, FIELD_POINTER ),
	//DEFINE_FIELD( CBasePlayer, m_pShadowCrouch, FIELD_POINTER ),
	//DEFINE_FIELD( CBasePlayer, m_vphysicsCollisionState, FIELD_INTEGER ),
	
	DEFINE_FIELD( CBasePlayer, m_oldOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_vecSmoothedVelocity, FIELD_VECTOR ),
	//DEFINE_FIELD( CBasePlayer, m_touchedPhysObject, FIELD_BOOLEAN ),
	//DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
	DEFINE_FIELD( CBasePlayer, m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_rgItems, FIELD_INTEGER ),
	//DEFINE_FIELD( CBasePlayer, m_fNextSuicideTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_flTimeStepSound, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flSwimTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flDuckTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_flSuitUpdate, FIELD_TIME ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_lastDamageAmount, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_tbdPrev, FIELD_TIME ),

	//DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ), // Don't need to restore

	DEFINE_FIELD( CBasePlayer, m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_idrownrestored, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_nPoisonDmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_nPoisonRestored, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fInitHUD, FIELD_BOOLEAN ),
	//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_BOOLEAN ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ), // Don't need to restore, debug
	//DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
	//DEFINE_FIELD( CBasePlayer, m_lastx, FIELD_INTEGER ),
	//DEFINE_FIELD( CBasePlayer, m_lasty, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_iFrags, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iDeaths, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flNextDecalTime, FIELD_TIME ),
	//DEFINE_AUTO_ARRAY( CBasePlayer, m_szTeamName, FIELD_STRING ), // mp

	//DEFINE_FIELD( CBasePlayer, m_bConnected, FIELD_BOOLEAN ),
	// from edict_t
	DEFINE_FIELD( CBasePlayer, m_ArmorValue, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_DmgOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_DmgTake, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_DmgSave, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_AirFinished, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_PainFinished, FIELD_TIME ),
	
	DEFINE_FIELD( CBasePlayer, m_iPlayerLocked, FIELD_INTEGER ),

	DEFINE_AUTO_ARRAY( CBasePlayer, m_hViewModel, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_flMaxspeed, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flWaterJumpTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_vecWaterJumpVel, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_nImpulse, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flStepSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flSwimSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_vecLadderNormal, FIELD_VECTOR ),

	DEFINE_FIELD( CBasePlayer, m_flFlashTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_nDrownDmgRate, FIELD_INTEGER ),

	// NOT SAVED
	//DEFINE_FIELD( CBasePlayer, m_vForcedOrigin, FIELD_VECTOR ),
	//DEFINE_FIELD( CBasePlayer, m_bForceOrigin, FIELD_BOOLEAN ),
	//DEFINE_FIELD( CBasePlayer, m_nTickBase, FIELD_INTEGER ),
	//DEFINE_FIELD( CBasePlayer, m_LastCmd, FIELD_ ),
	// DEFINE_FIELD( CBasePlayer, m_pCurrentCommand, CUserCmd ),

	DEFINE_FIELD( CBasePlayer, m_bitsDamageType, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_rgbTimeBasedDamage, FIELD_CHARACTER ),
	DEFINE_FIELD( CBasePlayer, m_hLastWeapon, FIELD_EHANDLE ),

	// DEFINE_FIELD( CBasePlayer, m_SimulatedByThisPlayer, CUtlVector < CHandle < CBaseEntity > > ),
	DEFINE_FIELD( CBasePlayer, m_nExplosionFX, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flExplosionFXEndTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flOldPlayerZ, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_bPlayerUnderwater, FIELD_BOOLEAN ),

	DEFINE_FIELD( CBasePlayer, m_vecConstraintCenter, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_flConstraintRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flConstraintWidth, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flConstraintSpeedFactor, FIELD_FLOAT ),

	// Function Pointers
	DEFINE_FUNCTION( CBasePlayer, PlayerDeathThink ),

END_DATADESC()


// pl
BEGIN_PREDICTION_DATA_NO_BASE( CPlayerState )

	// DEFINE_FIELD( CPlayerState, netname, FIELD_STRING ),
	// DEFINE_FIELD( CPlayerState, v_angle, FIELD_VECTOR ),
	DEFINE_FIELD( CPlayerState, deadflag, FIELD_BOOLEAN ),
	// this is always set to true on restore, don't bother saving it.
	// DEFINE_FIELD( CPlayerState, fixangle, FIELD_INTEGER ),
	// DEFINE_FIELD( CPlayerState, anglechange, FIELD_FLOAT ),

END_PREDICTION_DATA()

BEGIN_PREDICTION_DATA( CBasePlayer )

	DEFINE_EMBEDDED( CBasePlayer, m_Local ),
	DEFINE_PRED_TYPEDESCRIPTION( CBasePlayer, pl, CPlayerState ),

	// UNDONE: Should this be FIELD_STRING or FIELD_CHARACTER?
	DEFINE_FIELD( CBasePlayer, m_szAnimExtension, FIELD_STRING ),
	DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonReleased, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_rgItems, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fNextSuicideTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flTimeStepSound, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flSwimTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flDuckTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flSuitUpdate, FIELD_FLOAT ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_lastDamageAmount, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_tbdPrev, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ),
	DEFINE_FIELD( CBasePlayer, m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_idrownrestored, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fInitHUD, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),
	// DEFINE_FIELD( CBasePlayer, m_hUseEntity, EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_iObserverMode, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iObserverLastMode, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_hObserverTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_lastx, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_lasty, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iFrags, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iDeaths, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iRespawnFrames, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flNextDecalTime, FIELD_FLOAT ),
	// UNDONE: Should this be FIELD_STRING or auto array of FIELD_CHARACTER?
	DEFINE_FIELD( CBasePlayer, m_szTeamName, FIELD_STRING ),
	DEFINE_FIELD( CBasePlayer, m_bConnected, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_vecAdditionalPVSOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_vecCameraPVSOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_ArmorValue, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_DmgOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_DmgTake, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_DmgSave, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_AirFinished, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_PainFinished, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iPlayerLocked, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( CBasePlayer, m_hViewModel, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_flMaxspeed, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_nButtons, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flWaterJumpTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_nImpulse, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flStepSoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flSwimSoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_vecLadderNormal, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_flFlashTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_nDrownDmgRate, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_vForcedOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_bForceOrigin, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_nTickBase, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayer, m_LastCmd, CUserCmd ),

END_PREDICTION_DATA()

int giPrecacheGrunt = 0;

edict_t *CBasePlayer::s_PlayerEdict = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseViewModel
//-----------------------------------------------------------------------------
CBaseViewModel *CBasePlayer::GetViewModel( int index /*= 0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );
	return m_hViewModel[ index ].Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CBaseViewModel *vm = ( CBaseViewModel * )CreateEntityByName( "viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this );
		m_hViewModel.Set( index, vm );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::DestroyViewModels( void )
{
	int i;
	for ( i = MAX_VIEWMODELS - 1; i >= 0; i-- )
	{
		CBaseViewModel *vm = GetViewModel( i );
		if ( !vm )
			continue;

		UTIL_Remove( vm );
		m_hViewModel.Set( i, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Static member function to create a player of the specified class
// Input  : *className - 
//			*ed - 
// Output : CBasePlayer
//-----------------------------------------------------------------------------
CBasePlayer *CBasePlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CBasePlayer *player;
	CBasePlayer::s_PlayerEdict = ed;
	player = ( CBasePlayer * )CreateEntityByName( className );
	return player;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
CBasePlayer::CBasePlayer( )
{
	CONSTRUCT_PREDICTABLE( CBasePlayer );

#ifdef _DEBUG
	m_vecAutoAim.Init();
	m_vecAdditionalPVSOrigin.Init();
	m_vecCameraPVSOrigin.Init();
	m_DmgOrigin.Init();
	m_vecLadderNormal.Init();

	m_oldOrigin.Init();
	m_vecSmoothedVelocity.Init();
#endif

	if ( s_PlayerEdict )
	{
		// take the assigned edict_t and attach it
		Assert( s_PlayerEdict != NULL );
		CBaseEntity::AttachEdict( s_PlayerEdict );
		s_PlayerEdict = NULL;
	}

	m_flFlashTime = -1;

	m_iHealth = 0;
	Weapon_SetLast( NULL );
	m_bitsDamageType = 0;

	m_bForceOrigin = false;
	m_hVehicle = NULL;
	m_pCurrentCommand = NULL;

	ResetObserverMode();
}

CBasePlayer::~CBasePlayer( )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CBasePlayer::UpdateOnRemove( void )
{
	VPhysicsDestroyObject();

	// Remove him from his current team
	if ( GetTeam() )
	{
		Warning("BUG: REMOVING PLAYER FROM TEAM IN UpdateOnRemove!\n");
		GetTeam()->RemovePlayer( this );
	}

	// Chain at end to mimic destructor unwind order
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : **pvs - 
//			**pas - 
//-----------------------------------------------------------------------------
void CBasePlayer::SetupVisibility( unsigned char *pvs, unsigned char *pas )
{
	Vector org;
	org = EyePosition();

	engine->AddOriginToPVS( org );

	// FIXME:  Re-enable later?
	//	*pas = NULL;
}


//-----------------------------------------------------------------------------
// Sets the view angles
//-----------------------------------------------------------------------------
void CBasePlayer::SnapEyeAngles( const QAngle &viewAngles )
{
	pl.v_angle = viewAngles;
	pl.fixangle = 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSpeed - 
//			iMax - 
// Output : int
//-----------------------------------------------------------------------------
int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed/fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer::DeathSound( void )
{
	// temporarily using pain sounds for death sounds

	// Did we die from falling?
	if ( m_bitsDamageType & DMG_FALL )
	{
		// They died in the fall. Play a splat sound.
		EmitSound( "Player.FallGib" );
	}
	else
	{
		EmitSound( "Player.Death" );
	}

	// play one of the suit death alarms
	UTIL_EmitGroupnameSuit(edict(), "HEV_DEAD");
}

// override takehealth
// bitsDamageType indicates type of damage healed. 

int CBasePlayer::TakeHealth( float flHealth, int bitsDamageType )
{
	// clear out any damage types we healed.
	// UNDONE: generic health should not heal any
	// UNDONE: time-based damage
	if (m_takedamage)
	{
		m_bitsDamageType &= ~(bitsDamageType & ~DMG_TIMEBASED);
	}

	return BaseClass::TakeHealth (flHealth, bitsDamageType);
}


//-----------------------------------------------------------------------------
// Purpose: Draw all overlays (should be implemented in cascade by subclass to add
//			any additional non-text overlays)
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
void CBasePlayer::DrawDebugGeometryOverlays(void) 
{
	// --------------------------------------------------------
	// If in buddha mode and dead draw lines to indicate death
	// --------------------------------------------------------
	if ((m_debugOverlays & OVERLAY_BUDDHA_MODE) && m_iHealth == 1)
	{
		Vector vBodyDir = BodyDirection2D( );
		Vector eyePos	= EyePosition() + vBodyDir*10.0;
		Vector vUp		= Vector(0,0,8);
		Vector vSide;
		CrossProduct( vBodyDir, vUp, vSide);
		NDebugOverlay::Line(eyePos+vSide+vUp, eyePos-vSide-vUp, 255,0,0, false, 0);
		NDebugOverlay::Line(eyePos+vSide-vUp, eyePos-vSide+vUp, 255,0,0, false, 0);
	}
	BaseClass::DrawDebugGeometryOverlays();
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	if ( m_takedamage )
	{
		CTakeDamageInfo info = inputInfo;

		// --------------------------------------------------
		//  If an NPC check if friendly fire is disallowed
		// --------------------------------------------------
		CAI_BaseNPC *pNPC = info.GetAttacker()->MyNPCPointer();
		if ( pNPC && (pNPC->CapabilitiesGet() & bits_CAP_NO_HIT_PLAYER) && pNPC->IRelationType( this ) != D_HT )
		{
			return;
		}

		// Prevent team damage here so blood doesn't appear
		if ( info.GetAttacker()->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer*)info.GetAttacker();
			if ( InSameTeam(pPlayer) )
				return;
		}

		SetLastHitGroup( ptr->hitgroup );

		
		switch ( ptr->hitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			info.ScaleDamage( sk_player_head.GetFloat() );
			break;
		case HITGROUP_CHEST:
			info.ScaleDamage( sk_player_chest.GetFloat() );
			break;
		case HITGROUP_STOMACH:
			info.ScaleDamage( sk_player_stomach.GetFloat() );
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			info.ScaleDamage( sk_player_arm.GetFloat() );
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			info.ScaleDamage( sk_player_leg.GetFloat() );
			break;
		default:
			break;
		}

		SpawnBlood(ptr->endpos, BloodColor(), info.GetDamage());// a little surface blood.
		TraceBleed( info.GetDamage(), vecDir, ptr, info.GetDamageType() );
		AddMultiDamage( info, this );
	}
}

//------------------------------------------------------------------------------
// Purpose : Do some kind of damage effect for the type of damage
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBasePlayer::DamageEffect(float flDamage, int fDamageType)
{
	if (fDamageType & DMG_SLASH)
	{
		// If slash damage shoot some blood
		SpawnBlood(EyePosition(), BloodColor(), flDamage);
	}
	else if (fDamageType & DMG_PLASMA)
	{
		// Blue screen fade
		color32 blue = {0,0,255,100};
		UTIL_ScreenFade( this, blue, 0.2, 0.4, FFADE_MODULATE );

		// Very small screen shake
		ViewPunch(QAngle(random->RandomInt(-0.1,0.1), random->RandomInt(-0.1,0.1), random->RandomInt(-0.1,0.1)));

		// Burn sound 
		EmitSound( "Player.PlasmaDamage" );
	}
	else if (fDamageType & DMG_SONIC)
	{
		// Sonic damage sound 
		EmitSound( "Player.SonicDamage" );
	}
}


/*
	Take some damage.  
	NOTE: each call to OnTakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to OnTakeDamage using DMG_GENERIC.
*/

#define ARMOR_RATIO	 0.2	// Armor Takes 80% of the damage
#define ARMOR_BONUS  0.5	// Each Point of Armor is work 1/x points of health


int CBasePlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = inputInfo.GetDamageType();
	int ffound = true;
	int fmajor;
	int fcritical;
	int fTookDamage;
	int ftrivial;
	float flRatio;
	float flBonus;
	float flHealthPrev = m_iHealth;

	CTakeDamageInfo info = inputInfo;

	IServerVehicle *pVehicle = GetVehicle();
	if ( pVehicle )
		info.ScaleDamage(pVehicle->DamageModifier(info));

	if ( GetFlags() & FL_GODMODE )
	{
		return 0;
	}

	if (m_debugOverlays & OVERLAY_BUDDHA_MODE) 
		if ((m_iHealth - info.GetDamage()) <= 0)
	{
		m_iHealth = 1;
		return 0;
	}

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	if ( ( info.GetDamageType() & DMG_BLAST ) && g_pGameRules->IsMultiplayer() )
	{
		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead
	if ( !IsAlive() )
		return 0;
	// go take the damage first

	
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, info.GetAttacker() ) )
	{
		// Refuse the damage
		return 0;
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = info.GetDamage();

	// Armor. 
	if (m_ArmorValue && !(info.GetDamageType() & (DMG_FALL | DMG_DROWN)) )// armor doesn't protect against fall or drown damage!
	{
		float flNew = info.GetDamage() * flRatio;

		float flArmor;

		flArmor = (info.GetDamage() - flNew) * flBonus;

		// Does this use more armor than we have?
		if (flArmor > m_ArmorValue)
		{
			flArmor = m_ArmorValue;
			flArmor *= (1/flBonus);
			flNew = info.GetDamage() - flArmor;
			m_ArmorValue = 0;
		}
		else
			m_ArmorValue -= flArmor;
		
		info.SetDamage( flNew );
	}

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	info.SetDamage( (int)info.GetDamage() );
	fTookDamage = BaseClass::OnTakeDamage( info );

	if ( fTookDamage )
	{
		// add to the damage total for clients, which will be sent as a single
		// message at the end of the frame
		// todo: remove after combining shotgun blasts?
		if ( info.GetInflictor() && info.GetInflictor()->edict() )
			m_DmgOrigin = info.GetInflictor()->GetAbsOrigin();

		m_DmgTake += (int)info.GetDamage();
	}

	// reset damage time countdown for each type of time based damage player just sustained

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
			if (info.GetDamageType() & (DMG_PARALYZE << i))
				m_rgbTimeBasedDamage[i] = 0;
	}

	// Display any effect associate with this damage type
	DamageEffect(info.GetDamage(),bitsDamage);

	// how bad is it, doc?
	ftrivial = (m_iHealth > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (m_iHealth < 30);

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

		// DMG_BURN	
		// DMG_FREEZE
		// DMG_BLAST
		// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get resent

	while (fTookDamage && (!ftrivial || (bitsDamage & DMG_TIMEBASED)) && ffound && bitsDamage)
	{
		ffound = false;

		if (bitsDamage & DMG_CLUB)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC);	// minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = true;
		}
		if (bitsDamage & (DMG_FALL | DMG_CRUSH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", false, SUIT_NEXT_IN_30SEC);	// major fracture
			else
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC);	// minor fracture
	
			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = true;
		}
		
		if (bitsDamage & DMG_BULLET)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", false, SUIT_NEXT_IN_30SEC);	// blood loss detected
			//else
			//	SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC);	// minor laceration
			
			bitsDamage &= ~DMG_BULLET;
			ffound = true;
		}

		if (bitsDamage & DMG_SLASH)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", false, SUIT_NEXT_IN_30SEC);	// major laceration
			else
				SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC);	// minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = true;
		}
		
		if (bitsDamage & DMG_SONIC)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", false, SUIT_NEXT_IN_1MIN);	// internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = true;
		}

		if (bitsDamage & (DMG_POISON | DMG_PARALYZE))
		{
			if (bitsDamage & DMG_POISON)
			{
				m_nPoisonDmg += info.GetDamage();
				m_tbdPrev = gpGlobals->curtime;
				m_rgbTimeBasedDamage[itbd_PoisonRecover] = 0;
			}

			SetSuitUpdate("!HEV_DMG3", false, SUIT_NEXT_IN_1MIN);	// blood toxins detected
			bitsDamage &= ~( DMG_POISON | DMG_PARALYZE );
			ffound = true;
		}

		if (bitsDamage & DMG_ACID)
		{
			SetSuitUpdate("!HEV_DET1", false, SUIT_NEXT_IN_1MIN);	// hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = true;
		}

		if (bitsDamage & DMG_NERVEGAS)
		{
			SetSuitUpdate("!HEV_DET0", false, SUIT_NEXT_IN_1MIN);	// biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = true;
		}

		if (bitsDamage & DMG_RADIATION)
		{
			SetSuitUpdate("!HEV_DET2", false, SUIT_NEXT_IN_1MIN);	// radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = true;
		}
		if (bitsDamage & DMG_SHOCK)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = true;
		}
	}

	m_Local.m_vecPunchAngle.SetX( -2 );

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75) 
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate("!HEV_MED1", false, SUIT_NEXT_IN_30MIN);	// automedic on

		// give morphine shot if not given recently
		SetSuitUpdate("!HEV_HEAL7", false, SUIT_NEXT_IN_30MIN);	// morphine shot
	}
	
	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{

		// already took major damage, now it's critical...
		if (m_iHealth < 6)
			SetSuitUpdate("!HEV_HLTH3", false, SUIT_NEXT_IN_10MIN);	// near death
		else if (m_iHealth < 20)
			SetSuitUpdate("!HEV_HLTH2", false, SUIT_NEXT_IN_10MIN);	// health critical
	
		// give critical health warnings
		if (!random->RandomInt(0,3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN); //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if (fTookDamage && (info.GetDamageType() & DMG_TIMEBASED) && flHealthPrev < 75)
		{
			if (flHealthPrev < 50)
			{
				if (!random->RandomInt(0,3))
					SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN); //seek medical attention
			}
			else
				SetSuitUpdate("!HEV_HLTH1", false, SUIT_NEXT_IN_10MIN);	// health dropping
		}

	// Do special explosion damage effect
	if ( bitsDamage & DMG_BLAST )
	{
		OnDamagedByExplosion( info );
	}

	return fTookDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			damageAmount - 
//-----------------------------------------------------------------------------
#define MIN_SHOCK_AND_CONFUSION_DAMAGE	30.0f
#define MIN_EAR_RINGING_DISTANCE		240.0f  // 20 feet

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void CBasePlayer::OnDamagedByExplosion( const CTakeDamageInfo &info )
{
	// Let first effect wear off first
	if ( GetExplosionEffects() != FEXPLOSION_NONE )
		return;

	float lastDamage = info.GetDamage();

	float distanceFromPlayer = 9999.0f;

	CBaseEntity *inflictor = info.GetInflictor();
	if ( inflictor )
	{
		Vector delta = GetAbsOrigin() - inflictor->GetAbsOrigin();
		distanceFromPlayer = delta.Length();
	}

	bool ear_ringing = distanceFromPlayer < MIN_EAR_RINGING_DISTANCE ? true : false;
	bool shock = lastDamage >= MIN_SHOCK_AND_CONFUSION_DAMAGE;

	//Msg( "expl dist %f damage %f\n",
	//	distanceFromPlayer, lastDamage );

	if ( !shock && !ear_ringing )
		return;

	if ( shock )
	{
		m_nExplosionFX = FEXPLOSION_SHOCK;
	}
	else
	{
		m_nExplosionFX = FEXPLOSION_EAR_RINGING;
	}

	m_flExplosionFXEndTime = gpGlobals->curtime + dsp_explosion_effect_duration.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::CheckExplosionEffects( void )
{
	if ( !m_nExplosionFX )
		return;

	if ( gpGlobals->curtime < m_flExplosionFXEndTime )
		return;

	ClearExplosionEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::ClearExplosionEffects( void )
{
	m_nExplosionFX = FEXPLOSION_NONE;
	m_flExplosionFXEndTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayer::GetExplosionEffects( void ) const
{
	return m_nExplosionFX;
}


//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems( void )
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBaseCombatWeapon *rgpPackWeapons[ 20 ];// 20 hardcoded for now. How to determine exactly how many weapons we have?
	int iPackAmmo[ MAX_AMMO_SLOTS + 1];
	int iPW = 0;// index into packweapons array
	int iPA = 0;// index into packammo array

	memset(rgpPackWeapons, NULL, sizeof(rgpPackWeapons) );
	memset(iPackAmmo, -1, sizeof(iPackAmmo) );

	// get the game rules 
	iWeaponRules = g_pGameRules->DeadPlayerWeapons( this );
 	iAmmoRules = g_pGameRules->DeadPlayerAmmo( this );

	if ( iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO )
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems( true );
		return;
	}

// go through all of the weapons and make a list of the ones to pack
	for ( i = 0 ; i < WeaponCount() ; i++ )
	{
		// there's a weapon here. Should I pack it?
		CBaseCombatWeapon *pPlayerItem = GetWeapon( i );
		if ( pPlayerItem )
		{
			switch( iWeaponRules )
			{
			case GR_PLR_DROP_GUN_ACTIVE:
				if ( GetActiveWeapon() && pPlayerItem == GetActiveWeapon() )
				{
					// this is the active item. Pack it.
					rgpPackWeapons[ iPW++ ] = pPlayerItem;
				}
				break;

			case GR_PLR_DROP_GUN_ALL:
				rgpPackWeapons[ iPW++ ] = pPlayerItem;
				break;

			default:
				break;
			}
		}
	}

// now go through ammo and make a list of which types to pack.
	if ( iAmmoRules != GR_PLR_DROP_AMMO_NO )
	{
		for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
		{
			if ( GetAmmoCount( i ) > 0 )
			{
				// player has some ammo of this type.
				switch ( iAmmoRules )
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[ iPA++ ] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					// WEAPONTODO: Make this work
					/*
					if ( GetActiveWeapon() && i == GetActiveWeapon()->m_iPrimaryAmmoType ) 
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[ iPA++ ] = i;
					}
					else if ( GetActiveWeapon() && i == GetActiveWeapon()->m_iSecondaryAmmoType ) 
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[ iPA++ ] = i;
					}
					*/
					break;

				default:
					break;
				}
			}
		}
	}

	RemoveAllItems( true );// now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllItems( bool removeSuit )
{
	if (GetActiveWeapon())
	{
		ResetAutoaim( );
		GetActiveWeapon()->Holster( );
	}

	Weapon_SetLast( NULL );
	RemoveAllWeapons();
 	RemoveAllAmmo();

	if ( removeSuit )
	{
		m_Local.m_bWearingSuit = false;
	}

	UpdateClientData();
}

bool CBasePlayer::IsDead() const
{
	return m_lifeState == LIFE_DEAD;
}

static float DamageForce( const Vector &size, float damage )
{ 
	float force = damage * ((32 * 32 * 72.0) / (size.x * size.y * size.z)) * 5;
	
	if ( force > 1000.0) 
	{
		force = 1000.0;
	}

	return force;
}


int CBasePlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// set damage type sustained
	m_bitsDamageType |= info.GetDamageType();

	if ( !BaseClass::OnTakeDamage_Alive( info ) )
		return 0;

	Vector vecDir = vec3_origin;
	if ( info.GetInflictor() )
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
		VectorNormalize( vecDir );
	}

	if ( info.GetInflictor() && (GetMoveType() == MOVETYPE_WALK) && 
		(!info.GetAttacker() || !info.GetAttacker()->IsSolidFlagSet(FSOLID_TRIGGER)) )
	{
		ApplyAbsVelocityImpulse( vecDir * -DamageForce( WorldAlignSize(), info.GetDamage() ) );
	}

	return 1;
}


void CBasePlayer::Event_Killed( const CTakeDamageInfo &info )
{
	CSound *pSound;

	g_pGameRules->PlayerKilled( this, info );


	ClearUseEntity();
	
	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	{
		if ( pSound )
		{
			pSound->Reset();
		}
	}

	// don't let the status bar glitch for players.with <0 health.
	if (m_iHealth < -99)
	{
		m_iHealth = 0;
	}

	// holster the current weapon
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->Holster();
	}

	SetAnimation( PLAYER_DIE );
	
	SetModelIndex( g_ulModelIndexPlayer );    // don't use eyes

	SetViewOffset( VEC_DEAD_VIEWHEIGHT );
	m_lifeState		= LIFE_DYING;

	pl.deadflag = true;
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_FLYGRAVITY );
	Relink();
	RemoveFlag( FL_ONGROUND );
	if (GetAbsVelocity().z < 10)
	{
		Vector vecImpulse( 0, 0, random->RandomFloat(0,300) );
		ApplyAbsVelocityImpulse( vecImpulse );
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, false, 0);

	// reset FOV
	SetFOV( 0 );

	BaseClass::Event_Killed( info );
}

void CBasePlayer::Event_Dying()
{
	// NOT GIBBED, RUN THIS CODE

	DeathSound();

	// The dead body rolls out of the vehicle.
	if ( IsInAVehicle() )
	{
		LeaveVehicle();
	}

	QAngle angles = GetLocalAngles();

	angles.x = 0;
	angles.z = 0;
	
	SetLocalAngles( angles );

	SetThink(&CBasePlayer::PlayerDeathThink);
	SetNextThink( gpGlobals->curtime + 0.1f );
	BaseClass::Event_Dying();
}


// Set the activity based on an event or current state
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	char szAnim[64];

	float speed;

	speed = GetAbsVelocity().Length2D();

	if (GetFlags() & (FL_FROZEN|FL_ATCONTROLS))
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	Activity idealActivity = ACT_WALK;// TEMP!!!!!

	// This could stand to be redone. Why is playerAnim abstracted from activity? (sjb)
	if (playerAnim == PLAYER_JUMP)
	{
		idealActivity = ACT_HOP;
	}
	else if (playerAnim == PLAYER_SUPERJUMP)
	{
		idealActivity = ACT_LEAP;
	}
	else if (playerAnim == PLAYER_DIE)
	{
		if ( m_lifeState == LIFE_ALIVE )
		{
			idealActivity = GetDeathActivity();
		}
	}
	else if (playerAnim == PLAYER_ATTACK1)
	{
		if ( m_Activity == ACT_HOVER	|| 
			 m_Activity == ACT_SWIM		||
			 m_Activity == ACT_HOP		||
			 m_Activity == ACT_LEAP		||
			 m_Activity == ACT_DIESIMPLE )
		{
			idealActivity = m_Activity;
		}
		else
		{
			idealActivity = ACT_RANGE_ATTACK1;
		}
	}
	else if (playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK)
	{
		if ( !( GetFlags() & FL_ONGROUND ) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP) )	// Still jumping
		{
			idealActivity = m_Activity;
		}
		else if ( GetWaterLevel() > 1 )
		{
			if ( speed == 0 )
				idealActivity = ACT_HOVER;
			else
				idealActivity = ACT_SWIM;
		}
		else
		{
			idealActivity = ACT_WALK;
		}
	}

	
	if (idealActivity == ACT_RANGE_ATTACK1)
	{
		if ( GetFlags() & FL_DUCKING )	// crouching
		{
			Q_strncpy( szAnim, "crouch_shoot_" ,sizeof(szAnim));
		}
		else
		{
			Q_strncpy( szAnim, "ref_shoot_" ,sizeof(szAnim));
		}
		strcat( szAnim, m_szAnimExtension );
		animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;

		if ( GetSequence() != animDesired || !SequenceLoops() )
		{
			m_flCycle = 0;
		}

		if (!SequenceLoops())
		{
			m_fEffects |= EF_NOINTERP;
		}

		SetActivity( idealActivity );
		ResetSequence( animDesired );
	}
	else if (idealActivity == ACT_WALK)
	{
		if (GetActivity() != ACT_RANGE_ATTACK1 || IsActivityFinished())
		{
			if ( GetFlags() & FL_DUCKING )	// crouching
			{
				Q_strncpy( szAnim, "crouch_aim_" ,sizeof(szAnim));
			}
			else
			{
				Q_strncpy( szAnim, "ref_aim_" ,sizeof(szAnim));
			}
			strcat( szAnim, m_szAnimExtension );
			animDesired = LookupSequence( szAnim );
			if (animDesired == -1)
				animDesired = 0;
			SetActivity( ACT_WALK );
		}
		else
		{
			animDesired = GetSequence();
		}
	}
	else
	{
		if ( GetActivity() == idealActivity)
			return;
	
		SetActivity( idealActivity );

		animDesired = SelectWeightedSequence( m_Activity );

		// Already using the desired animation?
		if (GetSequence() == animDesired)
			return;

		ResetSequence( animDesired );
		m_flCycle = 0;
		return;
	}

	// Already using the desired animation?
	if (GetSequence() == animDesired)
		return;

	//Msg( "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	ResetSequence( animDesired );
	m_flCycle		= 0;
}


/*
===========
WaterMove
============
*/
#define AIRTIME	12		// lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if (GetMoveType() == MOVETYPE_NOCLIP)
	{
		m_AirFinished = gpGlobals->curtime + AIRTIME;
		return;
	}

	if (m_iHealth < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (GetWaterLevel() != 3) 
	{
		// not underwater
		
		// play 'up for air' sound
		
		if (m_AirFinished < gpGlobals->curtime)
		{
			EmitSound( "Player.DrownStart" );
		}
		else if (m_AirFinished < gpGlobals->curtime + 9)
		{
			EmitSound( "Player.DrownContinue" );
		}

		m_AirFinished = gpGlobals->curtime + AIRTIME;
		m_nDrownDmgRate = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.
			
			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}

	}
	else
	{	// fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (m_AirFinished < gpGlobals->curtime)		// drown!
		{
			if (m_PainFinished < gpGlobals->curtime)
			{
				// take drowning damage
				m_nDrownDmgRate += 1;
				if (m_nDrownDmgRate > 5)
				{
					m_nDrownDmgRate = 5;
				}

				OnTakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), m_nDrownDmgRate, DMG_DROWN ) );
				m_PainFinished = gpGlobals->curtime + 1;
				
				// track drowning damage, give it back when
				// player finally takes a breath
				m_idrowndmg += m_nDrownDmgRate;
			} 
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if ( GetWaterLevel() < 2 )
	{
		if ( GetWaterLevel() == 0 )
		{
			if ( GetFlags() & FL_INWATER )
			{       
				EmitSound( "Player.Wade" );
				StopSound( "Player.AmbientUnderWater" );
				m_bPlayerUnderwater = false;

				RemoveFlag( FL_INWATER );
			}
			return;
		}
		else
		{
			StopSound( "Player.AmbientUnderWater" );
			m_bPlayerUnderwater = false;
			return;
		}
	}
	else if ( GetWaterLevel() > 2 )
	{
		if ( m_bPlayerUnderwater == false )
		{
			EmitSound( "Player.AmbientUnderWater" );

			m_bPlayerUnderwater = true;
		}
		return;
	}
	
	// make bubbles

	air = (int)( m_AirFinished - gpGlobals->curtime );
	
#if 0
	if (GetWaterType() == CONTENT_LAVA)		// do damage
	{
		if (m_flDamageTime < gpGlobals->curtime)
		{
			OnTakeDamage( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 10 * GetWaterLevel(), DMG_BURN);
		}
	}
	else if (GetWaterType() == CONTENT_SLIME)		// do damage
	{
		m_flDamageTime = gpGlobals->curtime + 1;
		OnTakeDamage(GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 4 * GetWaterLevel(), DMG_ACID);
	}
#endif
	
	if (!(GetFlags() & FL_INWATER))
	{
		// player enter water sound
		if (GetWaterType() == CONTENTS_WATER)
		{
			EmitSound( "Player.Wade" );
		}
	
		AddFlag( FL_INWATER );
	}
}


// true if the player is attached to a ladder
bool CBasePlayer::IsOnLadder( void )
{ 
	return (GetMoveType() == MOVETYPE_LADDER);
}


float CBasePlayer::GetWaterJumpTime() const
{
	return m_flWaterJumpTime;
}


void CBasePlayer::PlayerDeathThink(void)
{
	float flForward;

	SetNextThink( gpGlobals->curtime + 0.1f );

	if (GetFlags() & FL_ONGROUND)
	{
		flForward = GetAbsVelocity().Length() - 20;
		if (flForward <= 0)
		{
			SetAbsVelocity( vec3_origin );
		}
		else
		{
			Vector vecNewVelocity = GetAbsVelocity();
			VectorNormalize( vecNewVelocity );
			vecNewVelocity *= flForward;
			SetAbsVelocity( vecNewVelocity );
		}
	}

	if ( HasWeapons() )
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}

	if (GetModelIndex() && (!IsSequenceFinished()) && (m_lifeState == LIFE_DYING))
	{
		StudioFrameAdvance( );

		m_iRespawnFrames++;
		if ( m_iRespawnFrames < 60 )  // animations should be no longer than this
			return;
	}

	if (m_lifeState == LIFE_DYING)
		m_lifeState = LIFE_DEAD;
	
	StopAnimation();

	m_fEffects |= EF_NOINTERP;
	m_flPlaybackRate = 0.0;

	int fAnyButtonDown = (m_nButtons & ~IN_SCORE);
	
	// wait for all buttons released
	if (m_lifeState == LIFE_DEAD)
	{
		if (fAnyButtonDown)
			return;

		if ( g_pGameRules->FPlayerCanRespawn( this ) )
		{
			m_fDeadTime = gpGlobals->curtime;
			m_lifeState = LIFE_RESPAWNABLE;
		}
		
		return;
	}

// if the player has been dead for one second longer than allowed by forcerespawn, 
// forcerespawn isn't on. Send the player off to an intermission camera until they 
// choose to respawn.
	if ( g_pGameRules->IsMultiplayer() && ( gpGlobals->curtime > (m_fDeadTime + 6) ) && !IsObserver() )
	{
		// go to dead camera. 
		StartDeathCam();
	}
	
// wait for any button down,  or mp_forcerespawn is set and the respawn time is up
	if (!fAnyButtonDown 
		&& !( g_pGameRules->IsMultiplayer() && forcerespawn.GetInt() > 0 && (gpGlobals->curtime > (m_fDeadTime + 5))) )
		return;

	m_nButtons = 0;
	m_iRespawnFrames = 0;

	//Msg( "Respawn\n");

	respawn( this, !IsObserver() );// don't copy a corpse if we're in deathcam.
	SetNextThink( TICK_NEVER_THINK );
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam( void )
{
	CBaseEntity *pSpot, *pNewSpot;
	int iRand;

	if ( GetViewOffset() == vec3_origin )
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = gEntList.FindEntityByClassname( NULL, "info_intermission");	

	if ( pSpot )
	{
		// at least one intermission spot in the world.
		iRand = random->RandomInt( 0, 3 );

		while ( iRand > 0 )
		{
			pNewSpot = gEntList.FindEntityByClassname( pSpot, "info_intermission");
			
			if ( pNewSpot )
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		CreateCorpse();
		StartObserverMode( pSpot->GetAbsOrigin(), pSpot->GetAbsAngles() );
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		trace_t tr;

		CreateCorpse();

		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, 128 ), 
			MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		QAngle angles;
		VectorAngles( GetAbsOrigin() - tr.endpos, angles );
		StartObserverMode( tr.endpos, angles );
		return;
	}
}

void CBasePlayer::StopObserverMode()
{
	m_bForcedObserverMode = false;
	m_iObserverMode = OBS_MODE_NONE;
	m_afPhysicsFlags &= ~PFLAG_OBSERVER;
}

bool CBasePlayer::StartObserverMode( Vector vecPosition, QAngle vecViewAngle )
{
	m_afPhysicsFlags |= PFLAG_OBSERVER;

	// Holster weapon immediately, to allow it to cleanup
    if ( GetActiveWeapon() )
		GetActiveWeapon()->Holster();

	// clear out the suit message cache so we don't keep chattering
    SetSuitUpdate(NULL, FALSE, 0);

	// set position and viewangle
	SetViewOffset( vec3_origin );
	SetAbsOrigin( vecPosition );
	SetAbsAngles( vecViewAngle );
	SetPunchAngle( vecViewAngle );
	SnapEyeAngles( vecViewAngle );

	SetMoveType( MOVETYPE_OBSERVER );
	// SetModelIndex( 0 );

	RemoveFlag( FL_DUCKING );
    AddSolidFlags( FSOLID_NOT_SOLID );
	Relink();

	SetObserverMode( m_iObserverLastMode );

	// Setup flags
    m_Local.m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
	m_takedamage = DAMAGE_NO;		
	m_fEffects |= EF_NODRAW;			
	m_iHealth = 1;
	m_lifeState = LIFE_DEAD; // Can't be dead, otherwise movement doesn't work right.
	pl.deadflag = true;

	return true;
}

bool CBasePlayer::SetObserverMode(int mode)
{
	if ( mode < OBS_MODE_NONE || mode > OBS_MODE_ROAMING )
		return false;

	if ( mode == m_iObserverMode )
		return true;	// we already are in this mode

	// check forcecamera settings for dead players
	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		switch ( deadspecmode.GetInt() )
		{
			case OBS_ALLOW_ALL	:	break;
			case OBS_ALLOW_TEAM :	mode = OBS_MODE_IN_EYE;	break;
			case OBS_ALLOW_NONE :	mode = OBS_MODE_FIXED; break;
		}
	}

	m_iObserverLastMode = m_iObserverMode;

	m_iObserverMode = mode;
	
	switch ( mode )
	{
		case OBS_MODE_NONE:
		case OBS_MODE_FIXED :
			SetMoveType( MOVETYPE_NONE );
			break;

		case OBS_MODE_IN_EYE :	
		case OBS_MODE_CHASE :
			SetMoveType( MOVETYPE_OBSERVER );
			break;

		case OBS_MODE_ROAMING : 
			SetMoveType( MOVETYPE_NOCLIP );
			break;

	}

	CheckObserverSettings();

	return true;	
}

int CBasePlayer::GetObserverMode()
{
	return m_iObserverMode;
}

void CBasePlayer::ForceObserverMode(int mode)
{
	int tempMode = OBS_MODE_ROAMING;

	if ( m_iObserverMode == mode )
		return;

	// don't change last mode if already in forced mode

	if ( m_bForcedObserverMode )
	{
		tempMode = m_iObserverLastMode;
	}
	
	SetObserverMode( mode );

	if ( m_bForcedObserverMode )
	{
		m_iObserverLastMode = tempMode;
	}

	m_bForcedObserverMode = true;
}

void CBasePlayer::CheckObserverSettings()
{
	// check if we are in forced mode an may go back

	if ( m_bForcedObserverMode )
	{
		CBaseEntity * target = FindNextObserverTarget( false );
		if ( target )
		{
				// we found a valid target
				SetObserverTarget( target );
				SetObserverMode( m_iObserverLastMode );
				m_bForcedObserverMode = false;
				// TODO check for HUD icons
				return;
		}
	}

	// check if our spectating target is still a valid one
	
	if (  m_iObserverMode == OBS_MODE_IN_EYE || m_iObserverMode == OBS_MODE_CHASE )
	{
		if ( !IsValidObserverTarget( m_hObserverTarget ) )
		{
			// our traget is not valid, try to find new target
			CBaseEntity * target = FindNextObserverTarget( false );
			if ( target )
			{
				// switch to new target
				SetObserverTarget( target );	
			}
			else
			{
				// couldn't find new taget, switch to temporary mode
				ForceObserverMode( OBS_MODE_ROAMING );
			}
		}
	}
	else if ( m_iObserverMode == OBS_MODE_ROAMING )
	{
		// is always ok
	}
}

CBaseEntity * CBasePlayer::GetObserverTarget()
{
	return m_hObserverTarget.Get();
}

bool CBasePlayer::SetObserverTarget(CBaseEntity * target)
{
	if ( !IsValidObserverTarget( target ) )
		return false;
	
	m_hObserverTarget.Set( target );

	return true;
}

bool CBasePlayer::IsValidObserverTarget(CBaseEntity * target)
{
	if ( target == NULL )
		return false;

	// MOD AUTHORS: Add checks on target here or in derived methode

	if ( !target->IsPlayer() )	// only track players
		return false;

	CBasePlayer * player = ToBasePlayer( target );

	if ( player == this )	// don't what myself
		return false;
	
	// Don't spec observers or players who haven't picked a class yet
	if ( player->IsObserver() )
		return NULL;

	if ( player->m_fEffects & EF_NODRAW ) // don't watch invisible players
		return false;

	if ( player->m_lifeState == LIFE_RESPAWNABLE ) // target is dead, waiting for respawn
		return false;

	if ( player->m_lifeState == LIFE_DEAD )
	{
		if ( (player->m_fDeadTime + 2.0f ) < gpGlobals->curtime )
		{
			return false;	// allow watching until 2 seconds after death to see death animation
		}
	}
		
	switch ( deadspecmode.GetInt() )	// check forcecamera settings
	{
		case OBS_ALLOW_ALL	:	break;
		case OBS_ALLOW_TEAM :	if ( (this->GetTeamNumber() != target->GetTeamNumber()) &&
									 (this->GetTeamNumber() != TEAM_SPECTATOR) )
								  return false;
								break;
		case OBS_ALLOW_NONE :	return false;
	}
	
	return true;	// passed all test
}

CBaseEntity * CBasePlayer::FindNextObserverTarget(bool bReverse)
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.

/*	if ( m_flNextFollowTime && m_flNextFollowTime > gpGlobals->time )
	{
		return;
	} 

	m_flNextFollowTime = gpGlobals->time + 0.25;
	*/	// TODO move outside this function

	int		startIndex;
	
	if ( m_hObserverTarget )
	{
		// start using last followed player
		startIndex = m_hObserverTarget->entindex();	
	}
	else
	{
		// start using own player index
		startIndex = this->entindex();
	}

	int	currentIndex = startIndex;
	int iDir = bReverse ? -1 : 1; 
	
	do
	{
		currentIndex += iDir;

		// Loop through the clients
  		if (currentIndex > gpGlobals->maxClients)
  			currentIndex = 1;
		else if (currentIndex < 1)
  			currentIndex = gpGlobals->maxClients;

			CBaseEntity * nextTarget = UTIL_PlayerByIndex( currentIndex );

		if ( IsValidObserverTarget( nextTarget ) )
		{
			return nextTarget;	// found next valid player
		}
	} while ( currentIndex != startIndex );
		
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this object can be +used by the player
//-----------------------------------------------------------------------------
bool CBasePlayer::IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps )
{
	if ( pEntity )
	{
		int caps = pEntity->ObjectCaps();
		if ( caps & (FCAP_IMPULSE_USE|FCAP_CONTINUOUS_USE|FCAP_ONOFF_USE|FCAP_DIRECTIONAL_USE) )
		{
			if ( (caps & requiredCaps) == requiredCaps )
				return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBasePlayer::CanPickupObject( CBaseEntity *pObject, float massLimit, float sizeLimit )
{
	// UNDONE: Make this virtual and move to HL2 player
#ifdef HL2_DLL
	//Must be valid
	if ( pObject == NULL )
		return false;

	//Must move with physics
	if ( pObject->GetMoveType() != MOVETYPE_VPHYSICS )
		return false;

	IPhysicsObject *pPhys = pObject->VPhysicsGetObject();

	//Must have a physics object
	if ( pPhys == NULL )
		return false;

	//Msg( "Target mass: %f\n", pPhys->GetMass() );

	//Must be under our threshold weight
	if ( massLimit > 0 && pPhys->GetMass() > massLimit )
		return false;

	if ( !pPhys->IsMoveable() )
	{
		// Allow pickup of phys props that are motion enabled on player pickup
		CPhysicsProp *pProp = dynamic_cast<CPhysicsProp*>(pObject);
		if ( !pProp || !(pProp->HasSpawnFlags( SF_PHYSPROP_ENABLE_ON_PHYSCANNON )) )
			return false;
	}

	if ( sizeLimit > 0 )
	{
		const Vector &size = pObject->EntitySpaceSize();
		if ( size.x > sizeLimit || size.y > sizeLimit || size.z > sizeLimit )
			return false;
	}

	return true;
#else
	return false;
#endif
}


//-----------------------------------------------------------------------------
// Purpose:	Server side of jumping rules.  Most jumping logic is already
//			handled in shared gamemovement code.  Put stuff here that should
//			only be done server side.
//-----------------------------------------------------------------------------
void CBasePlayer::Jump()
{
}


void CBasePlayer::Duck( )
{
	if (m_nButtons & IN_DUCK) 
	{
		if ( m_Activity != ACT_LEAP )
		{
			SetAnimation( PLAYER_WALK );
		}
	}
}

//
// ID's player as such.
//
Class_T  CBasePlayer::Classify ( void )
{
	return CLASS_PLAYER;
}


void CBasePlayer::AddPoints( int score, bool bAllowNegativeScore )
{
	// Positive score always adds
	if ( score < 0 )
	{
		if ( !bAllowNegativeScore )
		{
			if ( m_iFrags < 0 )		// Can't go more negative
				return;
			
			if ( -score > m_iFrags )	// Will this go negative?
			{
				score = -m_iFrags;		// Sum will be 0
			}
		}
	}

	m_iFrags += score;
}


void CBasePlayer::AddPointsToTeam( int score, bool bAllowNegativeScore )
{
	int index = entindex();

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && i != index )
		{
			if ( g_pGameRules->PlayerRelationship( this, pPlayer ) == GR_TEAMMATE )
			{
				pPlayer->AddPoints( score, bAllowNegativeScore );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayer::GetCommandContextCount( void ) const
{
	return m_CommandContext.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : CCommandContext
//-----------------------------------------------------------------------------
CCommandContext *CBasePlayer::GetCommandContext( int index )
{
	if ( index < 0 || index >= m_CommandContext.Count() )
		return NULL;

	return &m_CommandContext[ index ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCommandContext	*CBasePlayer::AllocCommandContext( void )
{
	int idx = m_CommandContext.AddToTail();
	return &m_CommandContext[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void CBasePlayer::RemoveCommandContext( int index )
{
	m_CommandContext.Remove( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::RemoveAllCommandContexts()
{
	m_CommandContext.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Determine how much time we will be running this frame
// Output : float
//-----------------------------------------------------------------------------
int CBasePlayer::DetermineSimulationTicks( void )
{
	int command_context_count = GetCommandContextCount();

	int context_number;

	int simulation_ticks = 0;

	// Determine how much time we will be running this frame and fixup player clock as needed
	for ( context_number = 0; context_number < command_context_count; context_number++ )
	{
		CCommandContext const *ctx = GetCommandContext( context_number );
		Assert( ctx );

		int numbackup = ctx->totalcmds - ctx->numcmds;
		
		// Determine how many packets we'll be executing below
		int packets_to_run = ctx->numcmds + min( ctx->dropped_packets, numbackup );

		// Determine how long it will take to run those packets
		simulation_ticks += packets_to_run;
	}

	return simulation_ticks;
}

// 100 msec ahead or behind current clock means we need to fix clock on client
#define TARGET_CLOCK_CORRECTION_REQUIRED (ROUND_TO_TICKS(0.075f))

//-----------------------------------------------------------------------------
// Purpose: Based upon amount of time in simulation time, adjust m_nTickBase so that
//  we just end at the end of the current frame (so the player is basically on clock
//  with the server)
// Input  : simulation_ticks - 
//-----------------------------------------------------------------------------
void CBasePlayer::AdjustPlayerTimeBase( int simulation_ticks )
{
	Assert( simulation_ticks >= 0 );
	if ( simulation_ticks < 0 )
		return;

	// Start in the past so that we get to the sv.time that we'll hit at the end of the
	//  frame, just as we process the final command
	int end_of_frame_ticks =  gpGlobals->tickcount;

	float end_of_frame = TICK_RATE * end_of_frame_ticks;

	// If client gets ahead of this, we'll need to correct
	float too_fast_limit = end_of_frame + TARGET_CLOCK_CORRECTION_REQUIRED;
	// If client falls behind this, we'll also need to correct
	float too_slow_limit = end_of_frame - TARGET_CLOCK_CORRECTION_REQUIRED;

	// If we simply execute the commands in the packet, this is where the client's clock will
	//  end up
	float estimated_end_time = ( m_nTickBase + simulation_ticks ) * TICK_RATE;

	if ( gpGlobals->maxClients == 1 )
	{
		m_nTickBase = end_of_frame_ticks - simulation_ticks + 1;
	}
	else
	{
		// See if we are too fast
		if ( estimated_end_time > too_fast_limit )
		{
			Msg( "client too fast by %f msec\n", 1000.0f * ( estimated_end_time - end_of_frame ) );
			m_nTickBase = end_of_frame_ticks - simulation_ticks + 1;
		}
		// Or to slow
		else if ( estimated_end_time < too_slow_limit )
		{
			Msg( "client too slow by %f msec\n", 1000.0f * ( end_of_frame - estimated_end_time ) );
			m_nTickBase = end_of_frame_ticks - simulation_ticks + 1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Note, don't chain to BaseClass::PhysicsSimulate
//-----------------------------------------------------------------------------
void CBasePlayer::PhysicsSimulate( void )
{
	VPROF( "CBasePlayer::PhysicsSimulate" );
	// If we've got a moveparent, we must simulate that first.
	CBaseEntity *pMoveParent = GetMoveParent();
	if (pMoveParent)
	{
		pMoveParent->PhysicsSimulate();
	}

	// Make sure not to simulate this guy twice per frame
	if (m_nSimulationTick == gpGlobals->tickcount )
	{
		return;
	}

	m_nSimulationTick = gpGlobals->tickcount;

	// See how much time has queued up for running
	int simulation_ticks = DetermineSimulationTicks();

	// If some time will elapse, make sure our clock (m_nTickBase) starts at the correct time
	if ( simulation_ticks > 0 )
	{
		AdjustPlayerTimeBase( simulation_ticks );
	}

	// Store off true server timestamps
	float savetime		= gpGlobals->curtime;
	float saveframetime = gpGlobals->frametime;

	int command_context_count = GetCommandContextCount();
	for ( int context_number = 0; context_number < command_context_count; context_number++ )
	{
		// Get oldest ( newer are added to tail )
		CCommandContext *ctx = GetCommandContext( context_number );
		Assert( ctx );

		int i;
		int numbackup = ctx->totalcmds - ctx->numcmds;

		// If the server is paused, zero out motion,buttons,view changes
		if ( ctx->paused )
		{
			for ( i = 0; i < ctx->numcmds; i++ )
			{
				ctx->cmds[ i ].frametime = 0.0;
				ctx->cmds[ i ].forwardmove = 0;
				ctx->cmds[ i ].sidemove = 0;
				ctx->cmds[ i ].upmove = 0;
				ctx->cmds[ i ].buttons = 0;

				VectorCopy ( pl.v_angle, ctx->cmds[ i ].viewangles );
			}
			
			ctx->dropped_packets = 0;
		}
		else
		{
			// FIXME:  Should this really reset the pl.v_angle for every command based on the last command, 
			//  seems wrong
			// Copy view angle directly from final command.
			if ( !pl.fixangle )
			{
				VectorCopy ( ctx->cmds[ CMD_MOSTRECENT ].viewangles, pl.v_angle );
			}
		}

		// Save off the last good command in case we drop > numbackup packets and need to rerun them
		//  we'll use this to "guess" at what was in the missing packets
		m_LastCmd = ctx->cmds[ CMD_MOSTRECENT ];

		int starttick = m_nTickBase;

		MoveHelperServer()->SetHost( this );

		// Suppress predicted events, etc.
		if ( IsPredictingWeapons() )
		{
			IPredictionSystem::SuppressHostEvents( this );
		}

		// If we haven't dropped too many packets, then run some commands
		if ( ctx->dropped_packets < 24 )                
		{
			if ( ctx->dropped_packets > numbackup )
			{
				// Msg( "lost %i cmds\n", dropped_packets - numbackup );
			}

			// Rerun the last valid command a bunch of times.
			while ( ctx->dropped_packets > numbackup )           
			{
				PlayerRunCommand( &m_LastCmd, MoveHelperServer()  );
				// For this scenario packets, just use the start time over and over again
				m_nTickBase = starttick;
				ctx->dropped_packets--;
			}

			// Now run the "history" commands if we still have dropped packets
			while ( ctx->dropped_packets > 0 )
			{
				int cmdnum = ctx->dropped_packets + ctx->numcmds - 1;
				PlayerRunCommand( &ctx->cmds[ cmdnum ], MoveHelperServer()  );
				ctx->dropped_packets--;
			}
		}

		// Now run any new command(s).  Go backward because the most recent command is at index 0.
		for ( i = ctx->numcmds - 1; i >= 0; i-- )
		{
			PlayerRunCommand( &ctx->cmds[ i ], MoveHelperServer() );
		}

		// Always reset after running commands
		IPredictionSystem::SuppressHostEvents( NULL );
	}

	// Clear all contexts
	RemoveAllCommandContexts();

	// Restore the true server clock
	// FIXME:  Should this occur after simulation of children so
	//  that they are in the timespace of the player?
	gpGlobals->curtime		= savetime;
	gpGlobals->frametime	= saveframetime;	
}

unsigned int CBasePlayer::PhysicsSolidMaskForEntity() const
{
	return MASK_PLAYERSOLID;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buf - 
//			totalcmds - 
//			dropped_packets - 
//			ignore - 
//			paused - 
// Output : float -- Time in seconds of last movement command
//-----------------------------------------------------------------------------
void CBasePlayer::ProcessUsercmds( CUserCmd *cmds, int numcmds, int totalcmds,
	int dropped_packets, bool paused )
{
	CCommandContext *ctx = AllocCommandContext();
	Assert( ctx );

	int i;
	for ( i = totalcmds - 1; i >= 0; i-- )
	{
		ctx->cmds[ i ]		= cmds[ i ];
	}
	ctx->numcmds			= numcmds;
	ctx->totalcmds			= totalcmds,
	ctx->dropped_packets	= dropped_packets;
	ctx->paused				= paused;

	if ( paused )
	{
		m_nSimulationTick = -1;
		// Just run the commands right away if paused
		PhysicsSimulate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ucmd - 
//			*moveHelper - 
//-----------------------------------------------------------------------------
void CBasePlayer::PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	m_touchedPhysObject = false;

	// Handle FL_FROZEN.
	if(GetFlags() & FL_FROZEN)
	{
		ucmd->frametime = 0.0;
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
		ucmd->upmove = 0;
		ucmd->buttons = 0;
		ucmd->impulse = 0;
		VectorCopy ( pl.v_angle, ucmd->viewangles );
	}

	PlayerMove()->RunCommand(this, ucmd, moveHelper);
}


void CBasePlayer::HandleFuncTrain(void)
{
	if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
		AddFlag( FL_ONTRAIN );
	else 
		RemoveFlag( FL_ONTRAIN );

	// Train speed control
	if (( m_afPhysicsFlags & PFLAG_DIROVERRIDE ) == 0)
	{
		if (m_iTrain & TRAIN_ACTIVE)
		{
			m_iTrain = TRAIN_NEW; // turn off train
		}
		return;
	}

	CBaseEntity *pTrain = GetGroundEntity();
	float vel;

	if ( pTrain )
	{
		if ( !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) )
			pTrain = NULL;
	}
	
	if ( !pTrain )
	{
		if ( GetActiveWeapon()->ObjectCaps() & FCAP_DIRECTIONAL_USE )
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


void CBasePlayer::PreThink(void)
{							 
	if ( g_fGameOver || m_iPlayerLocked )
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
	CheckExplosionEffects();

	CheckSuitUpdate();

	if (m_lifeState >= LIFE_DYING)
		return;

	HandleFuncTrain();

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

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?
}


/* Time based Damage works as follows: 
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON			(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID			(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven
		#define DMG_SLOWFREEZE		(1 << 21)	// in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each NPC has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time 
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.  
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.
		
	
*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION	30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION	16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE		2.0

//#define RADIATION_DURATION	50
//#define RADIATION_DAMAGE	1.0

//#define ACID_DURATION		10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION	2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION	1.0
//#define SLOWFREEZE_DAMAGE	3.0

/* */


void CBasePlayer::CheckTimeBasedDamage() 
{
	int i;
	byte bDuration = 0;

	static float gtbdPrev = 0.0;

	if (!(m_bitsDamageType & DMG_TIMEBASED))
		return;

	// only check for time based damage approx. every 2 seconds
	if (abs(gpGlobals->curtime - m_tbdPrev) < 2.0)
		return;
	
	m_tbdPrev = gpGlobals->curtime;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// make sure bit is set for damage type
		if (m_bitsDamageType & (DMG_PARALYZE << i))
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
//				OnTakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);	
				bDuration = NERVEGAS_DURATION;
				break;
//			case itbd_Poison:
//				OnTakeDamage( CTakeDamageInfo( this, this, POISON_DAMAGE, DMG_GENERIC ) );
//				bDuration = POISON_DURATION;
//				break;
			case itbd_Radiation:
//				OnTakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = min(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4;	// get up to 5*10 = 50 points back
				break;

			case itbd_PoisonRecover:
			{
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been poisoned.
				if (m_nPoisonDmg > m_nPoisonRestored)
				{
					int nDif = min(m_nPoisonDmg - m_nPoisonRestored, 10);
					TakeHealth(nDif, DMG_GENERIC);
					m_nPoisonRestored += nDif;
				}
				bDuration = 9;	// get up to 10*10 = 100 points back
				break;
			}

			case itbd_Acid:
//				OnTakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
//				OnTakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
//				OnTakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				// decrement damage duration, detect when done.
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);	
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation. 
Some functions are automatic, some require power. 
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit 
		will come on and the battery will drain while the player stays in the area. 
		After the battery is dead, the player starts to take damage. 
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge. 

Notification (via the HUD)

x	Health
x	Ammo  
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged. 
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD 
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor. 

Augmentation 

	Reanimation (w/adrenaline)
		Causes the player to come back to life after he has been dead for 3 seconds. 
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA	
		Used automatically after picked up and after player enters the water. 
		Works for N seconds. Single use.	
	
Things powered by the battery

	Armor		
		Uses N watts for every M units of damage.
	Heat/Cool	
		Uses N watts for every second in hot/cold area.
	Long Jump	
		Uses N watts for every jump.
	Alien Cloak	
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield	
		Augments armor. Reduces Armor drain by one half
 
*/

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25

void CBasePlayer::UpdateGeigerCounter( void )
{
	byte range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->curtime < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->curtime + GEIGERDELAY;
		
	// send range to radition source to client

	range = (byte) (m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		CSingleUserRecipientFilter user( this );
		user.MakeReliable();
		UserMessageBegin( user, "Geiger" );
			WRITE_BYTE( range );
		MessageEnd();
	}

	// reset counter and semaphore
	if (!random->RandomInt(0,3))
		m_flgeigerRange = 1000;

}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME	3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate()
{
	int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;
	
	// Ignore suit updates if no suit
	if ( !IsSuitEquipped() )
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if ( g_pGameRules->IsMultiplayer() )
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if ( gpGlobals->curtime >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; i++)
			{
			if (isentence = m_rgSuitPlayList[isearch])
				break;
			
			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
			}

		if (isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)
			{
				// play sentence number

				char sentence[512];
				Q_strncpy( sentence, "!" ,sizeof(sentence));
				strcat( sentence, engine->SentenceNameFromIndex( isentence ) );
				UTIL_EmitSoundSuit( edict(), sentence );
			}
			else
			{
				// play sentence group
				UTIL_EmitGroupIDSuit(edict(), -isentence);
			}
		m_flSuitUpdate = gpGlobals->curtime + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check 
			m_flSuitUpdate = 0;
	}
}
 
// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate(char *name, int fgroup, int iNoRepeatTime)
{
	int i;
	int isentence;
	int iempty = -1;
	
	
	// Ignore suit updates if no suit
	if ( !IsSuitEquipped() )
		return;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name);	// Lookup sentence index (not group) by name
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(name);		// Lookup group index by name

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
			{
			// this sentence or group is already in 
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->curtime)
				{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
				}
			else
				{
				// don't play, still marked as norepeat
				return;
				}
			}
		// keep track of empty slot
		if (!m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = random->RandomInt(0, CSUITNOREPEAT-1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->curtime;
	}

	// find empty spot in queue, or overwrite last spot
	
	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->curtime)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->curtime + SUITFIRSTUPDATETIME;
		else 
			m_flSuitUpdate = gpGlobals->curtime + SUITUPDATETIME; 
	}

}

//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer::UpdatePlayerSound ( void )
{
	int iBodyVolume;
	int iVolume;
	CSound *pSound;

	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );

	if ( !pSound )
	{
		Msg( "Client lost reserved sound!\n" );
		return;
	}

	if (GetFlags() & FL_NOTARGET)
	{
		pSound->m_iVolume = 0;
		return;
	}

	// now figure out how loud the player's movement is.
	if ( GetFlags() & FL_ONGROUND )
	{	
		iBodyVolume = GetAbsVelocity().Length(); 

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast. 
		// NOTE: 512 units is a pretty large radius for a sound made by the player's body.
		// then again, I think some materials are pretty loud.
		if ( iBodyVolume > 512 )
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if ( m_nButtons & IN_JUMP )
	{
		// Jumping is a little louder.
		iBodyVolume += 100;
	}

	m_iTargetVolume = iBodyVolume;

	// if target volume is greater than the player sound's current volume, we paste the new volume in 
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives NPCs a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->Volume();

	if ( m_iTargetVolume > iVolume )
	{
		iVolume = m_iTargetVolume;
	}
	else if ( iVolume > m_iTargetVolume )
	{
		iVolume -= 250 * gpGlobals->frametime;

		if ( iVolume < m_iTargetVolume )
		{
			iVolume = 0;
		}
	}

	if ( m_fNoPlayerSound )
	{
		// debugging flag, lets players move around and shoot without NPCs hearing.
		iVolume = 0;
	}

	if ( pSound )
	{
		pSound->SetSoundOrigin( GetAbsOrigin() );
		pSound->m_iType = SOUND_PLAYER;
		pSound->m_iVolume = iVolume;
	}

	// Below are a couple of useful little bits that make it easier to visualize just how much noise the 
	// player is making. 
	//Vector forward = UTIL_YawToVector( pl.v_angle.y );
	//UTIL_Sparks( GetAbsOrigin() + forward * iVolume );
	//Msg( "%d/%d\n", iVolume, m_iTargetVolume );
}

// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck( CBasePlayer *pPlayer )
{
	trace_t trace;

	// Move up as many as 18 pixels if the player is stuck.
	for ( int i = 0; i < 18; i++ )
	{
		UTIL_TraceHull( pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), 
			VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &trace );
		if ( trace.startsolid )
		{
			Vector origin = pPlayer->GetAbsOrigin();
			origin.z += 1.0f;
			pPlayer->SetLocalOrigin( origin );
		}
		else
			break;
	}
}

#define SMOOTHING_FACTOR 0.9
extern CMoveData *g_pMoveData;

//-----------------------------------------------------------------------------
// For debugging...
//-----------------------------------------------------------------------------
void CBasePlayer::ForceOrigin( const Vector &vecOrigin )
{
	m_bForceOrigin = true;
	m_vForcedOrigin = vecOrigin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::PostThink()
{
	m_vecSmoothedVelocity = m_vecSmoothedVelocity * SMOOTHING_FACTOR + GetAbsVelocity() * ( 1 - SMOOTHING_FACTOR );

	if ( !g_fGameOver && !m_iPlayerLocked && IsAlive() )
	{
		// set correct collision bounds (may have changed in player movement code)
		if ( GetFlags() & FL_DUCKING )
		{
			SetCollisionBounds( VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
		}
		else
		{
			SetCollisionBounds( VEC_HULL_MIN, VEC_HULL_MAX );
		}

		// Handle controlling an entity
		if ( m_hUseEntity != NULL )
		{ 
			// if they've moved too far from the gun, or deployed another weapon, unuse the gun
			if ( m_hUseEntity->OnControls( this ) && 
				( !GetActiveWeapon() || FBitSet( GetActiveWeapon()->m_fEffects, EF_NODRAW ) ) )
			{  
				m_hUseEntity->Use( this, this, USE_SET, 2 );	// try fire the gun
			}
			else
			{
				// they've moved off the controls
				ClearUseEntity();
			}
		}

		// do weapon stuff
		ItemPostFrame();

		if ( GetFlags() & FL_ONGROUND )
		{		
			if (m_Local.m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
			{
				CSoundEnt::InsertSound ( SOUND_PLAYER, GetAbsOrigin(), m_Local.m_flFallVelocity, 0.2, this );
				// Msg( "fall %f\n", m_Local.m_flFallVelocity );
			}
			m_Local.m_flFallVelocity = 0;
		}

		// select the proper animation for the player character	
		if ( IsAlive() )
		{
			// If he's in a vehicle, sit down
			if ( IsInAVehicle() )
				SetAnimation( PLAYER_IN_VEHICLE );
			else if (!GetAbsVelocity().x && !GetAbsVelocity().y)
				SetAnimation( PLAYER_IDLE );
			else if ((GetAbsVelocity().x || GetAbsVelocity().y) && ( GetFlags() & FL_ONGROUND ))
				SetAnimation( PLAYER_WALK );
			else if (GetWaterLevel() > 1)
				SetAnimation( PLAYER_WALK );
		}

		// Don't allow bogus sequence on player
		if ( GetSequence() == -1 )
		{
			SetSequence( 0 );
		}

		StudioFrameAdvance();

		DispatchAnimEvents( this );

		SetSimulationTime( gpGlobals->curtime );

		//Let the weapon update as well
		Weapon_FrameUpdate();

		UpdatePlayerSound();

		if ( m_bForceOrigin )
		{
			SetLocalOrigin( m_vForcedOrigin );
			SetLocalAngles( m_Local.m_vecPunchAngle );
			m_Local.m_vecPunchAngle = RandomAngle( -25, 25 );
			m_Local.m_vecPunchAngleVel.Init();
		}

		PostThinkVPhysics();
	}

	// Even if dead simulate entities
	SimulatePlayerSimulatedEntities();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::PostThinkVPhysics( void )
{
	// Check to see if things are initialized!
	if ( !m_pPhysicsController )
		return;

	Vector newPosition = GetAbsOrigin();
	float frametime = gpGlobals->frametime;
	if ( frametime <= 0 || frametime > 0.1f )
		frametime = 0.1f;
	CBaseEntity	*groundentity = GetGroundEntity();
	IPhysicsObject *pPhysGround = NULL;

	if ( groundentity && groundentity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		pPhysGround = groundentity->VPhysicsGetObject();
		if ( pPhysGround && !pPhysGround->IsMoveable() )
			pPhysGround = NULL;
	}

	if ( !pPhysGround && m_touchedPhysObject && g_pMoveData->m_outStepHeight <= 0.f )
	{
		newPosition = m_oldOrigin + frametime * g_pMoveData->m_outWishVel;
		newPosition = (GetAbsOrigin() * 0.5f) + (newPosition * 0.5f);
	}

	int collisionState = VPHYS_WALK;
	if ( GetMoveType() == MOVETYPE_NOCLIP )
	{
		collisionState = VPHYS_NOCLIP;
	}
	else if ( GetFlags() & FL_DUCKING )
	{
		collisionState = VPHYS_CROUCH;
	}

	if ( collisionState != m_vphysicsCollisionState )
	{
		SetVCollisionState( collisionState );
	}

	if ( !(TouchedPhysics() || pPhysGround) )
	{
		g_pMoveData->m_outWishVel = Vector(m_flMaxspeed, m_flMaxspeed, m_flMaxspeed);
	}

	// teleport the physics object up by stepheight (game code does this - reflect in the physics)
	if ( g_pMoveData->m_outStepHeight > 0.1 )
	{
		m_pPhysicsController->StepUp( g_pMoveData->m_outStepHeight );
	}
	g_pMoveData->m_outStepHeight = 0;
	UpdateVPhysicsPosition( newPosition, g_pMoveData->m_outWishVel );

	m_oldOrigin = GetAbsOrigin();
}

void CBasePlayer::UpdateVPhysicsPosition( const Vector &position, const Vector &velocity )
{
	CBaseEntity	*groundentity = GetGroundEntity();
	IPhysicsObject *pPhysGround = NULL;

	bool onground = (GetFlags() & FL_ONGROUND) ? true : false;
	if ( groundentity && groundentity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		pPhysGround = groundentity->VPhysicsGetObject();
		if ( pPhysGround && !pPhysGround->IsMoveable() )
			pPhysGround = NULL;
	}
	m_pPhysicsController->Update( position, velocity, onground, pPhysGround );
}

void CBasePlayer::UpdatePhysicsShadowToCurrentPosition()
{
	UpdateVPhysicsPosition( GetAbsOrigin(), vec3_origin );
}

// checks if the spot is clear of players
bool IsSpawnPointValid( CBaseEntity *pPlayer, CBaseEntity *pSpot )
{
	CBaseEntity *ent = NULL;

	if ( !pSpot->IsTriggered( pPlayer ) )
	{
		return false;
	}

	for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); ent = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		// if ent is a client, don't spawn on 'em
		if ( ent->IsPlayer() && ent != pPlayer )
			return false;
	}

	return true;
}


CBaseEntity	*g_pLastSpawn = NULL;


//-----------------------------------------------------------------------------
// Purpose: Finds a player start entity of the given classname. If any entity of
//			of the given classname has the SF_PLAYER_START_MASTER flag set, that
//			is the entity that will be returned. Otherwise, the first entity of
//			the given classname is returned.
// Input  : pszClassName - should be "info_player_start", "info_player_coop", or
//			"info_player_deathmatch"
//-----------------------------------------------------------------------------
CBaseEntity *FindPlayerStart(const char *pszClassName)
{
	#define SF_PLAYER_START_MASTER	1
	
	CBaseEntity *pStart = gEntList.FindEntityByClassname(NULL, pszClassName);
	CBaseEntity *pStartFirst = pStart;
	while (pStart != NULL)
	{
		if (pStart->HasSpawnFlags(SF_PLAYER_START_MASTER))
		{
			return pStart;
		}

		pStart = gEntList.FindEntityByClassname(pStart, pszClassName);
	}

	return pStartFirst;
}


/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
CBaseEntity *CBasePlayer::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot;
	edict_t		*player;

	player = edict();

// choose a info_player_deathmatch point
	if (g_pGameRules->IsCoOp())
	{
		pSpot = gEntList.FindEntityByClassname( g_pLastSpawn, "info_player_coop");
		if ( pSpot )
			goto ReturnSpot;
		pSpot = gEntList.FindEntityByClassname( g_pLastSpawn, "info_player_start");
		if ( pSpot ) 
			goto ReturnSpot;
	}
	else if ( g_pGameRules->IsDeathmatch() )
	{
		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for ( int i = random->RandomInt(1,5); i > 0; i-- )
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
		if ( !pSpot )  // skip over the null point
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );

		CBaseEntity *pFirstSpot = pSpot;

		do 
		{
			if ( pSpot )
			{
				// check if pSpot is valid
				if ( IsSpawnPointValid( this, pSpot ) )
				{
					if ( pSpot->GetLocalOrigin() == vec3_origin )
					{
						pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
		} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if ( pSpot )
		{
			CBaseEntity *ent = NULL;
			for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); ent = sphere.GetCurrentEntity(); sphere.NextEntity() )
			{
				// if ent is a client, kill em (unless they are ourselves)
				if ( ent->IsPlayer() && !(ent->edict() == player) )
					ent->TakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC ) );
			}
			goto ReturnSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if ( !gpGlobals->startspot || !strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = FindPlayerStart( "info_player_start" );
		if ( pSpot )
			goto ReturnSpot;
	}
	else
	{
		pSpot = gEntList.FindEntityByName( NULL, gpGlobals->startspot, NULL );
		if ( pSpot )
			goto ReturnSpot;
	}

ReturnSpot:
	if ( !pSpot  )
	{
		Warning( "PutClientInServer: no info_player_start on level\n");
		return CBaseEntity::Instance( INDEXENT( 0 ) );
	}

	g_pLastSpawn = pSpot;
	return pSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Called the first time the player's created
//-----------------------------------------------------------------------------
void CBasePlayer::InitialSpawn( void )
{
	m_bConnected = true;
}

//-----------------------------------------------------------------------------
// Purpose: Called everytime the player respawns
//-----------------------------------------------------------------------------
void CBasePlayer::Spawn( void )
{
	SetClassname( "player" );
	
	SetSimulatedEveryTick( true );
	SetAnimatedEveryTick( true );

	pl.deadflag	= false;
	m_Local.m_bDrawViewmodel = true;
	m_Local.m_flStepSize = sv_stepsize.GetFloat();
	m_Local.m_bAllowAutoMovement = true;

	m_iHealth			= 100;
	m_ArmorValue		= 0;
	m_takedamage		= DAMAGE_YES;
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_WALK );
	m_iMaxHealth		= m_iHealth;
	m_flMaxspeed		= 0.0f;

	// Clear all flags except for FL_FULLEDICT
	if ( GetFlags() & FL_FAKECLIENT )
	{
		ClearFlags();
		AddFlag( FL_CLIENT | FL_FAKECLIENT );
	}
	else
	{
		ClearFlags();
		AddFlag( FL_CLIENT );
	}

	AddFlag( FL_AIMTARGET );

	SetFriction(1.0f);

	m_AirFinished	= gpGlobals->curtime + AIRTIME;
	m_nDrownDmgRate	= 2;
	m_fEffects		&= EF_NOSHADOW;	// only preserve the shadow flag
	m_lifeState		= LIFE_ALIVE;
	m_DmgTake		= 0;
	m_DmgSave		= 0;
	m_bitsHUDDamage		= -1;
	m_bitsDamageType	= 0;
	m_afPhysicsFlags	= 0;

	SetFOV( 0 );

	m_nRenderFX = kRenderFxNone;

	m_flNextDecalTime	= 0;// let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->curtime + 2.0;	// wait a few seconds until user-defined message registrations
												// are recieved by all clients
	
	m_flTimeStepSound	= 0;
	m_iStepLeft = 0;
	m_flFieldOfView		= 0.5;// some NPCs use this to determine whether or not the player is looking at them.

	m_bloodColor	= BLOOD_COLOR_RED;
	m_flNextAttack	= gpGlobals->curtime;

	m_vecAdditionalPVSOrigin = vec3_origin;
	m_vecCameraPVSOrigin = vec3_origin;

// dont let uninitialized value here hurt the player
	m_Local.m_flFallVelocity = 0;

	if ( !m_fGameHUDInitialized )
		g_pGameRules->SetDefaultPlayerTeam( this );

	SetSequence( SelectWeightedSequence( ACT_IDLE ) );

	if ( GetFlags() & FL_DUCKING ) 
		UTIL_SetSize(this, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);

	g_pGameRules->GetPlayerSpawnSpot( this );

    SetViewOffset( VEC_VIEW );
	Precache();
	m_HackedGunPos		= Vector( 0, 32, 0 );

	if ( m_iPlayerSound == SOUNDLIST_EMPTY )
	{
		Msg( "Couldn't alloc player sound slot!\n" );
	}

	m_fNoPlayerSound = false;// normal sound behavior.

	SetThink(NULL);
	m_fInitHUD = true;
	m_fWeapon = false;
	m_iClientBattery = -1;

	m_lastx = m_lasty = 0;
	
	ClearExplosionEffects();

	CreateViewModel();

	SetCollisionGroup( COLLISION_GROUP_PLAYER );

	// if the player is locked, make sure he stays locked
	if ( m_iPlayerLocked )
	{
		m_iPlayerLocked = false;
		LockPlayerInPlace();
	}

	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		StopObserverMode();
	}

	// Clear any screenfade
	color32 nothing = {0,0,0,0};
	UTIL_ScreenFade( this, nothing, 0, 0, FFADE_OUT );

	g_pGameRules->PlayerSpawn( this );

	m_vecSmoothedVelocity = vec3_origin;
	InitVCollision();
}

void CBasePlayer::Precache( void )
{
	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.
	
	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	/* todo - put in better spot and use new ainetowrk stuff
	if ( WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet )
	{
		if ( !WorldGraph.FSetGraphPointers() )
		{
			Msg( "**Graph pointers were not set!\n");
		}
		else
		{
			Msg( "**Graph Pointers Set!\n" );
		} 
	}
	*/

	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;
	m_bPlayerUnderwater = false;

	m_iClientBattery = -1;

	m_iTrain = TRAIN_NEW;

	m_iUpdateTime = 5;  // won't update for 1/2 a second

	if ( gInitHUD )
		m_fInitHUD = true;
}



int CBasePlayer::Save( ISave &save )
{
	if ( !BaseClass::Save(save) )
		return 0;

	return 1;
}


int CBasePlayer::Restore( IRestore &restore )
{
	int status = BaseClass::Restore(restore);
	if ( !status )
		return 0;

	CSaveRestoreData *pSaveData = gpGlobals->pSaveData;
	// landmark isn't present.
	if ( !pSaveData->levelInfo.fUseLandmark )
	{
		Msg( "No Landmark:%s\n", pSaveData->levelInfo.szLandmarkName );

		// default to normal spawn
		CBaseEntity *pSpawnSpot = EntSelectSpawnPoint();
		SetLocalOrigin( pSpawnSpot->GetLocalOrigin() + Vector(0,0,1) );
		SetLocalAngles( pSpawnSpot->GetLocalAngles() );
	}

	QAngle newViewAngles = pl.v_angle;
	newViewAngles.z = 0;	// Clear out roll
	SetLocalAngles( newViewAngles );
	SnapEyeAngles( newViewAngles );

	// Copied from spawn() for now
	SetBloodColor( BLOOD_COLOR_RED );

    g_ulModelIndexPlayer = GetModelIndex();

	if ( GetFlags() & FL_DUCKING ) 
	{
		// Use the crouch HACK
		FixPlayerCrouchStuck( this );
		UTIL_SetSize(this, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
		m_Local.m_bDucked = true;
	}
	else
	{
		m_Local.m_bDucked = false;
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	InitVCollision();

	// success
	return 1;
}

void CBasePlayer::SetTeamName( const char *pTeamName )
{
	Q_strncpy( m_szTeamName, pTeamName, TEAM_NAME_LENGTH );
}

void CBasePlayer::SetArmorValue( int value )
{
	m_ArmorValue = value;
}

void CBasePlayer::IncrementArmorValue( int nCount, int nMaxValue )
{ 
	m_ArmorValue += nCount;
	if (nMaxValue > 0)
	{
		if (m_ArmorValue > nMaxValue)
			m_ArmorValue = nMaxValue;
	}
}

// Only used by the physics gun... is there a better interface?
void CBasePlayer::SetPhysicsFlag( int nFlag, bool bSet )
{
	if (bSet)
		m_afPhysicsFlags |= nFlag;
	else
		m_afPhysicsFlags &= ~nFlag;
}


void CBasePlayer::NotifyNearbyRadiationSource( float flRange )
{
	// if player's current geiger counter range is larger
	// than range to this trigger hurt, reset player's
	// geiger counter range 

	if (m_flgeigerRange >= flRange)
		m_flgeigerRange = flRange;
}

void CBasePlayer::AllowImmediateDecalPainting()
{
	m_flNextDecalTime = gpGlobals->curtime;
}

// Suicide...
void CBasePlayer::CommitSuicide()
{
	// prevent suiciding too often
	if ( m_fNextSuicideTime > gpGlobals->curtime )
		return;

	// don't let them suicide for 5 seconds after suiciding
	m_fNextSuicideTime = gpGlobals->curtime + 5;  

	// have the player kill themself
	m_iHealth = 0;
	Event_Killed( CTakeDamageInfo( this, this, 0, DMG_NEVERGIB ) );
	Event_Dying();
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
bool CBasePlayer::HasWeapons( void )
{
	int i;

	for ( i = 0 ; i < WeaponCount() ; i++ )
	{
		if ( GetWeapon(i) )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecForce - 
//-----------------------------------------------------------------------------
void CBasePlayer::VelocityPunch( const Vector &vecForce )
{
	// Clear onground and add velocity.
	RemoveFlag( FL_ONGROUND );
	ApplyAbsVelocityImpulse(vecForce );
}


//--------------------------------------------------------------------------------------------------------------
// VEHICLES
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Put this player in a vehicle 
//-----------------------------------------------------------------------------
void CBasePlayer::GetInVehicle( IServerVehicle *pVehicle, int nRole )
{
	Assert( NULL == m_hVehicle.Get() );
	Assert( nRole >= 0 );

	if ( pVehicle->GetPassenger( nRole ) )
		return;

	CBaseEntity *pEnt = pVehicle->GetVehicleEnt();
	Assert( pEnt );

	if (!pVehicle->IsPassengerUsingStandardWeapons( nRole ))
	{
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();

		Weapon_SetLast( pWeapon );

		//Must be able to stow our weapon
		if ( ( pWeapon != NULL ) && ( pWeapon->Holster( NULL ) == false ) )
			return;

		m_Local.m_iHideHUD |= HIDEHUD_WEAPONS;
	}

	if ( !pVehicle->IsPassengerVisible( nRole ) )
	{
		m_fEffects |= EF_NODRAW;
	}

	ViewPunchReset();

	// Setting the velocity to 0 will cause the IDLE animation to play
	SetAbsVelocity( vec3_origin );
	SetMoveType( MOVETYPE_NOCLIP );

	// Choose the entry point of the vehicle,
	// By default, just stay at the point you started at...
	// NOTE: we have to set this first so that when the parent is set
	// the local position just works
	Vector vNewPos = GetAbsOrigin();
	QAngle qAngles = GetAbsAngles();
	pVehicle->GetPassengerStartPoint( nRole, &vNewPos, &qAngles );
	SetAbsOrigin( vNewPos );
	SetAbsAngles( qAngles );
	SetParent( pEnt );

	SetCollisionGroup( COLLISION_GROUP_IN_VEHICLE );
	Relink();
	
	pVehicle->SetPassenger( nRole, this );

	m_hVehicle = pEnt;

	// Throw an event indicating that the player entered the vehicle.
	g_pNotify->ReportNamedEvent( this, "PlayerEnteredVehicle" );

	OnVehicleStart();
}


//-----------------------------------------------------------------------------
// Purpose: Remove this player from a vehicle
//-----------------------------------------------------------------------------
void CBasePlayer::LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles )
{
	if ( NULL == m_hVehicle.Get() )
		return;

	IServerVehicle *pVehicle = GetVehicle();
	Assert( pVehicle );

	int nRole = pVehicle->GetPassengerRole( this );
	Assert( nRole >= 0 );

	SetParent( NULL );

	// Find the first non-blocked exit point:
	Vector vNewPos = GetAbsOrigin();
	QAngle qAngles = GetAbsAngles();
	if ( vecExitPoint == vec3_origin )
	{
		pVehicle->GetPassengerExitPoint( nRole, &vNewPos, &qAngles );
	}
	else
	{
		vNewPos = vecExitPoint;
		qAngles = vecExitAngles;
	}
	OnVehicleEnd( vNewPos );
	SetAbsOrigin( vNewPos );
	SetAbsAngles( qAngles );
	SnapEyeAngles( qAngles );

	m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONS;
	m_fEffects &= ~EF_NODRAW;

	SetMoveType( MOVETYPE_WALK );
	SetCollisionGroup( COLLISION_GROUP_PLAYER );
	Relink();

	qAngles[ROLL] = 0;
	SnapEyeAngles( qAngles );

	m_hVehicle = NULL;
	pVehicle->SetPassenger(nRole, NULL);

	Weapon_Switch( m_hLastWeapon.Get() );
}


//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CPointEntity
{
public:
	DECLARE_CLASS( CSprayCan, CPointEntity );

	void	Spawn ( CBasePlayer *pOwner );
	void	Think( void );

	virtual int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn ( CBasePlayer *pOwner )
{
	SetLocalOrigin( pOwner->WorldSpaceCenter() + Vector ( 0 , 0 , 32 ) );
	SetLocalAngles( pOwner->EyeAngles() );
	SetOwnerEntity( pOwner );
	SetNextThink( gpGlobals->curtime );
	EmitSound( "SprayCan.Paint" );
}

void CSprayCan::Think( void )
{
	trace_t	tr;	
	int playernum;
	int nFrames;
	CBasePlayer *pPlayer;
	
	pPlayer = ToBasePlayer( GetOwnerEntity() );

	nFrames = 1; // FIXME, look up from material

	playernum = GetOwnerEntity()->entindex();
	
	// Msg( "Spray by player %i, %i of %i\n", playernum, (int)(m_flFrame + 1), nFrames);

	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + forward * 128, 
		MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, & tr);

	UTIL_PlayerDecalTrace( &tr, playernum );
	
	// Just painted last custom frame.
	UTIL_Remove( this );
}

class	CBloodSplat : public CPointEntity
{
public:
	DECLARE_CLASS( CBloodSplat, CPointEntity );

	void	Spawn ( CBaseEntity *pOwner );
	void	Think ( void );
};

void CBloodSplat::Spawn ( CBaseEntity *pOwner )
{
	SetLocalOrigin( pOwner->WorldSpaceCenter() + Vector ( 0 , 0 , 32 ) );
	SetLocalAngles( pOwner->GetLocalAngles() );
	SetOwnerEntity( pOwner );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CBloodSplat::Think( void )
{
	trace_t	tr;	
	
	if ( g_Language != LANGUAGE_GERMAN )
	{
		CBasePlayer *pPlayer;
		pPlayer = ToBasePlayer( GetOwnerEntity() );

		Vector forward;
		AngleVectors( GetAbsAngles(), &forward );
		UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + forward * 128, 
			MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, & tr);

		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
	}
	UTIL_Remove( this );
}

//==============================================

//-----------------------------------------------------------------------------
// Purpose: Create and give the named item to the player. Then return it.
//-----------------------------------------------------------------------------
CBaseEntity	*CBasePlayer::GiveNamedItem( const char *pszName, int iSubType )
{
	// If I already own this type don't create one
	if ( Weapon_OwnsThisType(pszName, iSubType) )
		return NULL;

	// Msg( "giving %s\n", pszName );

	EHANDLE pent;

	pent = CreateEntityByName(pszName);
	if ( pent == NULL )
	{
		Msg( "NULL Ent in GiveNamedItem!\n" );
		return NULL;
	}

	pent->SetLocalOrigin( GetLocalOrigin() );
	pent->AddSpawnFlags( SF_NORESPAWN );

	if ( iSubType )
	{
		CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( (CBaseEntity*)pent );
		if ( pWeapon )
		{
			pWeapon->SetSubType( iSubType );
		}
	}

	DispatchSpawn( pent );

	if ( pent != NULL && !(pent->IsMarkedForDeletion()) ) 
	{
		pent->Touch( this );
	}

	return pent;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the nearest COLLIBALE entity in front of the player
//			that has a clear line of sight with the given classname
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *FindEntityClassForward( CBasePlayer *pMe, char *classname )
{
	trace_t tr;

	Vector forward;
	pMe->EyeVectors( &forward );
	UTIL_TraceLine(pMe->EyePosition(),
		pMe->EyePosition() + forward * MAX_COORD_RANGE,
		MASK_SOLID, pMe, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pHit = tr.m_pEnt;
		if (FClassnameIs( pHit,classname ) )
		{
			return pHit;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the nearest COLLIBALE entity in front of the player
//			that has a clear line of sight
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *FindEntityForward( CBasePlayer *pMe )
{
	trace_t tr;

	Vector forward;
	pMe->EyeVectors( &forward );
	UTIL_TraceLine(pMe->EyePosition(),
		pMe->EyePosition() + forward * MAX_COORD_RANGE,
		MASK_SOLID, pMe, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pHit = tr.m_pEnt;
		return pHit;
	}
	return NULL;

}

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity in front of the player of the given
//			classname, preferring collidable entities, but allows selection of 
//			enities that are on the other side of walls or objects
//
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *FindPickerEntityClass( CBasePlayer *pPlayer, char *classname )
{
	// First try to trace a hull to an entity
	CBaseEntity *pEntity = FindEntityClassForward( pPlayer, classname );

	// If that fails just look for the nearest facing entity
	if (!pEntity) 
	{
		Vector forward;
		Vector origin;
		pPlayer->EyeVectors( &forward );
		origin = pPlayer->WorldSpaceCenter();		
		pEntity = gEntList.FindEntityClassNearestFacing( origin, forward,0.95,classname);
	}
	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity in front of the player, preferring
//			collidable entities, but allows selection of enities that are
//			on the other side of walls or objects
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer )
{
	// First try to trace a hull to an entity
	CBaseEntity *pEntity = FindEntityForward( pPlayer );

	// If that fails just look for the nearest facing entity
	if (!pEntity) 
	{
		Vector forward;
		Vector origin;
		pPlayer->EyeVectors( &forward );
		origin = pPlayer->WorldSpaceCenter();		
		pEntity = gEntList.FindEntityNearestFacing( origin, forward,0.95);
	}
	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest node in front of the player
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Node *FindPickerAINode( CBasePlayer *pPlayer, NodeType_e nNodeType )
{
	Vector forward;
	Vector origin;

	pPlayer->EyeVectors( &forward );
	origin = pPlayer->EyePosition();	
	return g_pAINetworkManager->GetEditOps()->FindAINodeNearestFacing( origin, forward,0.90, nNodeType);
}

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest link in front of the player
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Link *FindPickerAILink( CBasePlayer* pPlayer )
{
	Vector forward;
	Vector origin;

	pPlayer->EyeVectors( &forward );
	origin = pPlayer->EyePosition();	
	return g_pAINetworkManager->GetEditOps()->FindAILinkNearestFacing( origin, forward,0.90);
}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer::ForceClientDllUpdate( void )
{
	m_iClientBattery = -1;
	m_iTrain |= TRAIN_NEW;  // Force new train message.
	m_fWeapon = false;          // Force weapon send

	// Force all HUD data to be resent to client
	m_fInitHUD = true;

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}

/*
============
ImpulseCommands
============
*/
extern int g_iWeaponCheat;

void CBasePlayer::ImpulseCommands( )
{
	trace_t	tr;

	// Handle use events
	PlayerUse();
		
	int iImpulse = (int)m_nImpulse;
	switch (iImpulse)
	{
	case 100:
        // temporary flashlight for level designers
        if ( FlashlightIsOn() )
		{
			FlashlightTurnOff();
		}
        else 
		{
			FlashlightTurnOn();
		}
		break;

	case 200:
		{
			CBaseCombatWeapon *pWeapon;

			pWeapon = GetActiveWeapon();
			
			if( pWeapon->m_fEffects & EF_NODRAW )
			{
				pWeapon->Deploy();
			}
			else
			{
				pWeapon->Holster();
			}
		}
		break;

	case	201:// paint decal
		
		if ( gpGlobals->curtime < m_flNextDecalTime )
		{
			// too early!
			break;

		}

		{
			Vector forward;
			EyeVectors( &forward );
			UTIL_TraceLine ( EyePosition(), 
				EyePosition() + forward * 128, 
				MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, & tr);
		}

		if ( tr.fraction != 1.0 )
		{// line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->curtime + decalfrequency.GetFloat();
			CSprayCan *pCan = CREATE_UNSAVED_ENTITY( CSprayCan, "spraycan" );
			pCan->Spawn( this );
		}

		break;
	case    DEMO_RESTART_HUD_IMPULSE:  //  Demo recording, update client dll specific data again. (204)
		ForceClientDllUpdate(); 
		break;

	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands( iImpulse );
		break;
	}
	
	m_nImpulse = 0;
}


//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands( int iImpulse )
{
#if !defined( HLDEMO_BUILD )
	if ( g_iWeaponCheat == 0 )
	{
		return;
	}

	CBaseEntity *pEntity;
	trace_t tr;

	switch ( iImpulse )
	{
	case 76:
		{
			if (!giPrecacheGrunt)
			{
				giPrecacheGrunt = 1;
				Msg( "You must now restart to use Grunt-o-matic.\n");
			}
			else
			{
				Vector forward = UTIL_YawToVector( EyeAngles().y );
				Create("NPC_human_grunt", GetLocalOrigin() + forward * 128, GetLocalAngles());
			}
			break;
		}

	case 81:

		GiveNamedItem( "weapon_cubemap" );
		break;

	case 82:
		//Msg("Use ch_createjeep instead.\n");
		{
			// Cheat to create a jeep in front of the player
			Vector vecForward;
			AngleVectors( EyeAngles(), &vecForward );
			CBaseEntity *pJeep = (CBaseEntity *)CreateEntityByName( "prop_vehicle_jeep" );
			if ( pJeep )
			{
				Vector vecOrigin = GetAbsOrigin() + vecForward * 256 + Vector(0,0,64);
				QAngle vecAngles( 0, GetAbsAngles().y - 90, 0 );
				pJeep->SetAbsOrigin( vecOrigin );
				pJeep->SetAbsAngles( vecAngles );
				pJeep->KeyValue( "model", "models/buggy.mdl" );
				pJeep->KeyValue( "solid", "6" );
				pJeep->KeyValue( "targetname", "jeep" );
				pJeep->KeyValue( "vehiclescript", "scripts/vehicles/jeep_test.txt" );
				pJeep->Spawn();
				pJeep->Activate();
				pJeep->Relink();
				pJeep->Teleport( &vecOrigin, &vecAngles, NULL );
			}
		}
		break;

	case 101:
		gEvilImpulse101 = true;

		EquipSuit();

		// Give the player everything!
		GiveAmmo( 255,	"SmallRound");
		GiveAmmo( 255,	"MediumRound");
		GiveAmmo( 255,	"LargeRound");
		GiveAmmo( 255,	"Buckshot");
		GiveAmmo( 3,	"ar2_grenade");
		GiveAmmo( 3,	"ml_grenade");
		GiveAmmo( 5,	"grenade");
		GiveAmmo( 150,	"GaussEnergy");
		
		GiveNamedItem( "weapon_frag" );
		GiveNamedItem( "weapon_crowbar" );
		GiveNamedItem( "weapon_pistol" );
		GiveNamedItem( "weapon_ar2" );
		GiveNamedItem( "weapon_shotgun" );
		GiveNamedItem( "weapon_smg1" );
		GiveNamedItem( "weapon_gauss" );
		GiveNamedItem( "weapon_physcannon" );
		GiveNamedItem( "weapon_rpg" );
		if ( GetHealth() < 100 )
		{
			TakeHealth( 25, DMG_GENERIC );
		}
		
		gEvilImpulse101		= false;

		break;

	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs( this, 1, GIB_HUMAN );
		break;

	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
			if ( pNPC )
				pNPC->ReportAIState();
		}
		break;

	case	105:// player makes no sound for NPCs to hear.
		{
			if ( m_fNoPlayerSound )
			{
				Msg( "Player is audible\n" );
				m_fNoPlayerSound = false;
			}
			else
			{
				Msg( "Player is silent\n" );
				m_fNoPlayerSound = true;
			}
			break;
		}

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			Msg( "Classname: %s", pEntity->GetClassname() );
			
			if ( pEntity->GetEntityName() != NULL_STRING )
			{
				Msg( " - Name: %s\n", STRING( pEntity->GetEntityName() ) );
			}
			else
			{
				Msg( " - Name: No Targetname\n" );
			}

			if ( pEntity->m_iParent != NULL_STRING )
				Msg( "Parent: %s\n", STRING(pEntity->m_iParent) );

			Msg( "Model: %s\n", STRING( pEntity->GetModelName() ) );
			if ( pEntity->m_iGlobalname != NULL_STRING )
				Msg( "Globalname: %s\n", STRING(pEntity->m_iGlobalname) );
		}
		break;

	case 107:
		{
			trace_t tr;

			edict_t		*pWorld = engine->PEntityOfEntIndex( 0 );

			Vector start = EyePosition();
			Vector forward;
			EyeVectors( &forward );
			Vector end = start + forward * 1024;
			UTIL_TraceLine( start, end, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.m_pEnt )
				pWorld = tr.m_pEnt->edict();

			const char *pTextureName = tr.surface.name;

			if ( pTextureName )
				Msg( "Texture: %s\n", pTextureName );
		}
		break;

	//
	// Sets the debug NPC to be the NPC under the crosshair.
	//
	case 108:
	{
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
			if ( pNPC != NULL )
			{
				Msg( "Debugging %s (0x%x)\n", pNPC->GetClassname(), pNPC );
				CAI_BaseNPC::SetDebugNPC( pNPC );
			}
		}
		break;
	}

	case	195:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_fly", GetLocalOrigin(), GetLocalAngles());
		}
		break;
	case	196:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_large", GetLocalOrigin(), GetLocalAngles());
		}
		break;
	case	197:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_human", GetLocalOrigin(), GetLocalAngles());
		}
		break;
	case	202:// Random blood splatter
		{
			Vector forward;
			EyeVectors( &forward );
			UTIL_TraceLine ( EyePosition(), 
				EyePosition() + forward * 128, 
				MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, & tr);

			if ( tr.fraction != 1.0 )
			{// line hit something, so paint a decal
				CBloodSplat *pBlood = CREATE_UNSAVED_ENTITY( CBloodSplat, "bloodsplat" );
				pBlood->Spawn( this );
			}
		}
		break;
	case	203:// remove creature.
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			UTIL_Remove( pEntity );
//			if ( pEntity->m_takedamage )
//				pEntity->SetThink(SUB_Remove);
		}
		break;
	}
#endif	// HLDEMO_BUILD
}


bool CBasePlayer::ClientCommand(const char *cmd)
{
	if( stricmp( cmd, "SmokeGrenade" ) == 0 )
	{
		ParticleSmokeGrenade *pSmoke = dynamic_cast<ParticleSmokeGrenade*>( CreateEntityByName(PARTICLESMOKEGRENADE_ENTITYNAME) );
		if ( pSmoke )
		{
			Vector vForward;
			AngleVectors( GetLocalAngles(), &vForward );
			vForward.z = 0;
			VectorNormalize( vForward );

			pSmoke->SetLocalOrigin( GetLocalOrigin() + vForward * 100 );
			pSmoke->SetFadeTime(25, 30);	// Fade out between 25 seconds and 30 seconds.
			pSmoke->Activate();
			pSmoke->SetLifetime(30);
			pSmoke->FillVolume();

			return true;
		}
	}
	else if( stricmp( cmd, "vehicleRole" ) == 0 )
	{
		// Get the vehicle role value.
		if ( engine->Cmd_Argc() == 2 )
		{
			// Check to see if a player is in a vehicle.
			if ( IsInAVehicle() )
			{
				int nRole = atoi( engine->Cmd_Argv( 1 ) );
				IServerVehicle *pVehicle = GetVehicle();
				if ( pVehicle )
				{
					// Only switch roles if role is empty!
					if ( !pVehicle->GetPassenger( nRole ) )
					{
						LeaveVehicle();
						GetInVehicle( pVehicle, nRole );
					}
				}			
			}

			return true;
		}
	}
	else if ( stricmp( cmd, "spectate" ) == 0 ) // join spectator team & start observer mode
	{
		if ( GetTeamNumber() == TEAM_SPECTATOR )
			return true;

		if ( !IsDead() )
		{
			ClientKill( edict() );	// kill player

			// add 1 to frags to balance out the 1 subtracted for killing yourself
			IncrementFragCount( 1 );
		}

		RemoveAllItems( true );

		ChangeTeam( TEAM_SPECTATOR );

		StartObserverMode( GetAbsOrigin(), GetAbsAngles() );
		return true;
	}
	else if ( stricmp( cmd, "specmode" ) == 0 ) // new observer mode
	{
		// check for parameters.
		if ( engine->Cmd_Argc() >= 2 )
		{
			int mode = atoi( engine->Cmd_Argv(1) );

			if ( IsObserver() && mode > OBS_MODE_NONE)
			{
				// set new spectator mode, don't allow OBS_MODE_NONE
				if ( !SetObserverMode( mode ) )
					ClientPrint( this, HUD_PRINTCONSOLE, "#Spectator_Mode_Unkown");
			}
			else
			{
				// remember spectator mode for later use
				m_iObserverLastMode = mode;
			}
		}
		else
		{
			// sitch to next spec mode if no parameter give
			int nextMode = m_iObserverMode + 1;
			
			if ( nextMode > OBS_MODE_ROAMING )
				nextMode = OBS_MODE_IN_EYE;

			SetObserverMode( nextMode );

		}
		return true;
	}
	else if ( stricmp( cmd, "specnext" ) == 0 ) // chase next player
	{
		if ( IsObserver() )
		{
			// set new spectator mode
			CBaseEntity * target = FindNextObserverTarget( false );
			if ( target )
				SetObserverTarget( target );
		}
		
		return true;
	}
	else if ( stricmp( cmd, "specprev" ) == 0 ) // chase prevoius player
	{
		if ( IsObserver() )
		{
			// set new spectator mode
			CBaseEntity * target = FindNextObserverTarget( true );
			if ( target )
				SetObserverTarget( target );
		}
		
		return true;
	}


	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_CH_CreateJeep( void )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	// Cheat to create a jeep in front of the player
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	CBaseEntity *pJeep = (CBaseEntity *)CreateEntityByName( "prop_vehicle_jeep" );
	if ( pJeep )
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector(0,0,64);
		QAngle vecAngles( 0, pPlayer->GetAbsAngles().y - 90, 0 );
		pJeep->SetAbsOrigin( vecOrigin );
		pJeep->SetAbsAngles( vecAngles );
		pJeep->KeyValue( "model", "models/buggy.mdl" );
		pJeep->KeyValue( "solid", "6" );
		pJeep->KeyValue( "targetname", "jeep" );
		pJeep->KeyValue( "vehiclescript", "scripts/vehicles/jeep_test.txt" );
		pJeep->Spawn();
		pJeep->Activate();
		pJeep->Relink();
		pJeep->Teleport( &vecOrigin, &vecAngles, NULL );
	}
}

static ConCommand ch_createjeep("ch_createjeep", CC_CH_CreateJeep, "Spawn jeep in front of the player.", FCVAR_CHEAT);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_CH_CreateAirboat( void )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	// Cheat to create a jeep in front of the player
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	CBaseEntity *pJeep = ( CBaseEntity* )CreateEntityByName( "prop_vehicle_airboat" );
	if ( pJeep )
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector( 0,0,64 );
		QAngle vecAngles( 0, pPlayer->GetAbsAngles().y - 90, 0 );
		pJeep->SetAbsOrigin( vecOrigin );
		pJeep->SetAbsAngles( vecAngles );
		pJeep->KeyValue( "model", "models/airboat.mdl" );
		pJeep->KeyValue( "solid", "6" );
		pJeep->KeyValue( "targetname", "airboat" );
		pJeep->KeyValue( "vehiclescript", "scripts/vehicles/airboat.txt" );
		pJeep->Spawn();
		pJeep->Activate();
		pJeep->Relink();
		pJeep->Teleport( &vecOrigin, &vecAngles, NULL );
	}
}

static ConCommand ch_createairboat( "ch_createairboat", CC_CH_CreateAirboat, "Spawn airboat in front of the player.", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CBasePlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
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
		if( Weapon_EquipAmmoOnly( pWeapon ) )
		{
			// Only remove me if I have no ammo left
			// Can't just check HasAnyAmmo because if I don't use clips, I want to be removed, 
			if ( pWeapon->UsesClipsForAmmo1() && pWeapon->HasPrimaryAmmo() )
				return false;

			UTIL_Remove( pWeapon );
			return true;
		}
		else
		{
			return false;
		}
	}
	// -------------------------
	// Otherwise take the weapon
	// -------------------------
	else 
	{
		pWeapon->CheckRespawn();

		pWeapon->AddSolidFlags( FSOLID_NOT_SOLID );
		pWeapon->m_fEffects	|= EF_NODRAW;

		Weapon_Equip( pWeapon );
		return true;
	}
}


bool CBasePlayer::RemovePlayerItem( CBaseCombatWeapon *pItem )
{
	if (GetActiveWeapon() == pItem)
	{
		ResetAutoaim( );
		pItem->Holster( );
		pItem->SetNextThink( TICK_NEVER_THINK );; // crowbar may be trying to swing again, etc
		pItem->SetThink( NULL );
	}

	if ( m_hLastWeapon.Get() == pItem )
	{
		Weapon_SetLast( NULL );
	}

	return Weapon_Detach( pItem );
}


//-----------------------------------------------------------------------------
// Purpose: Hides or shows the player's view model. The "r_drawviewmodel" cvar
//			can still hide the viewmodel even if this is set to true.
// Input  : bShow - true to show, false to hide the view model.
//-----------------------------------------------------------------------------
void CBasePlayer::ShowViewModel(bool bShow)
{
	m_Local.m_bDrawViewmodel = bShow ? 1 : 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
QAngle CBasePlayer::BodyAngles()
{
	return EyeAngles();
}

//------------------------------------------------------------------------------
// Purpose : Add noise to BodyTarget() to give enemy a better chance of
//			 getting a clear shot when the player is peeking above a hole
//			 or behind a ladder (eventually the randomly-picked point 
//			 along the spine will be one that is exposed above the hole or 
//			 between rungs of a ladder.)
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CBasePlayer::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{ 
	if ( IsInAVehicle() )
	{
		return GetVehicle()->GetVehicleEnt()->BodyTarget( posSrc, bNoisy );
	}
	if (bNoisy)
	{
		return GetAbsOrigin() + (GetViewOffset() * random->RandomFloat( 0.7, 1.0 )); 
	}
	else
	{
		return EyePosition(); 
	}
};		

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer::UpdateClientData( void )
{
	CSingleUserRecipientFilter user( this );
	user.MakeReliable();

	if (m_fInitHUD)
	{
		m_fInitHUD = false;
		gInitHUD = false;

		UserMessageBegin( user, "ResetHUD" );
			WRITE_BYTE( 0 );
		MessageEnd();

		if ( !m_fGameHUDInitialized )
		{
			UserMessageBegin( user, "InitHUD" );
			MessageEnd();

			g_pGameRules->InitHUD( this );
			InitHUD();
			m_fGameHUDInitialized = true;
			if ( g_pGameRules->IsMultiplayer() )
			{
				FireTargets( "game_playerjoin", this, this, USE_TOGGLE, 0 );

				// dvsents2: uncomment when removing FireTargets everywhere
				//variant_t value;
				//g_EventQueue.AddEvent( "game_playerjoin", "Use", value, 0, this, this );
			}
		}
		FireTargets( "game_playerspawn", this, this, USE_TOGGLE, 0 );

		// dvsents2: uncomment when removing FireTargets everywhere
		//variant_t value;
		//g_EventQueue.AddEvent( "game_playerspawn", "Use", value, 0, this, this );
	}

	// HACKHACK -- send the message to display the game title
	CWorld *world = GetWorldEntity();
	if ( world && world->GetDisplayTitle() )
	{
		UserMessageBegin( user, "GameTitle" );
			WRITE_BYTE( world->GetStartDark() ? 1 : 0 );
		MessageEnd();
		world->SetDisplayTitle( false );
	}

	if (m_ArmorValue != m_iClientBattery)
	{
		m_iClientBattery = m_ArmorValue;

		// send "battery" update message
		if ( usermessages->LookupUserMessage( "Battery" ) != -1 )
		{
			UserMessageBegin( user, "Battery" );
				WRITE_SHORT( (int)m_ArmorValue);
			MessageEnd();
		}
	}

#if 0 // BYE BYE!!
	// Update Flashlight
	if ((m_flFlashLightTime) && (m_flFlashLightTime <= gpGlobals->curtime))
	{
		if (FlashlightIsOn())
		{
			if (m_iFlashBattery)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->curtime;
				m_iFlashBattery--;
				
				if (!m_iFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_iFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->curtime;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}
	}
#endif 

	if (m_iTrain & TRAIN_NEW)
	{
		// send "Train" update message
		UserMessageBegin( user, "Train" );
			WRITE_BYTE(m_iTrain & 0xF);
		MessageEnd();

		m_iTrain &= ~TRAIN_NEW;
	}

	// Update all the items
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		if ( GetWeapon(i) )  // each item updates it's successors
			GetWeapon(i)->UpdateClientData( this );
	}

	// Let other entities who care get a crack at this
	CBaseEntity::PerformUpdateClientData( this );

	// Let any global rules update the HUD, too
	g_pGameRules->UpdateClientData( this );
}


void CBasePlayer::EnableControl(bool fControl)
{
	if (!fControl)
		AddFlag( FL_FROZEN );
	else
		RemoveFlag( FL_FROZEN );

}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the player should autoaim or not
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::ShouldAutoaim( void )
{
	//Must have autoaiming selected
	if ( sv_aim.GetInt() == 0 )
		return false;

	//Cannot autoaim on hard
	if ( g_iSkillLevel == SKILL_HARD )
		return false;

	//Cannot be in multiplayer
	if ( gpGlobals->maxClients > 1 )
		return false;

	return true;
}

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer::GetAutoaimVector( float flDelta )
{
	if ( ShouldAutoaim() == false )
	{
		Vector	forward;
		AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle, &forward );
		
		return	forward;
	}

	Vector vecSrc	= Weapon_ShootPosition( );
	float flDist	= MAX_COORD_RANGE;

	m_vecAutoAim.Init( 0.0f, 0.0f, 0.0f );

	QAngle angles = AutoaimDeflection( vecSrc, flDist, flDelta );

	// update ontarget if changed
	if ( !g_pGameRules->AllowAutoTargetCrosshair() )
		m_fOnTarget = 0;

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;


	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if ( g_iSkillLevel == SKILL_EASY )
	{
		m_vecAutoAim = angles * 0.9f;
	}
	else
	{
		m_vecAutoAim = m_vecAutoAim * 0.3f + angles * 0.7f;
	}

	// Don't send across network if sv_aim is 0
	//FIXME: Crosshair offset drawing is broken, don't move it for now
	/*
	if ( ShouldAutoaim()  )
	{
		if ( m_vecAutoAim.x != m_lastx ||  m_vecAutoAim.y != m_lasty )
		{
			engine->CrosshairAngle( edict(), m_vecAutoAim.x, m_vecAutoAim.y );
			
			m_lastx = m_vecAutoAim.x;
			m_lasty = m_vecAutoAim.y;
		}
	}
	*/

	// Msg( "%f %f\n", angles.x, angles.y );

	Vector	forward;
	AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle + m_vecAutoAim, &forward );
	
	return forward;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecSrc - 
//			flDist - 
//			flDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
QAngle CBasePlayer::AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  )
{
	float		bestdot;
	Vector		bestdir;
	CBaseEntity	*bestent;
	trace_t		tr;
	Vector		v_forward, v_right, v_up;

	if ( ShouldAutoaim() == false )
	{
		m_fOnTarget = false;
		return vec3_angle;
	}

	AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle + m_vecAutoAim, &v_forward, &v_right, &v_up );

	// try all possible entities
	bestdir = v_forward;
	bestdot = flDelta; // +- 10 degrees
	bestent = NULL;

	//Reset this data
	m_fOnTarget			= false;
	m_hAutoAimTarget	= NULL;

	UTIL_TraceLine( vecSrc, vecSrc + bestdir * flDist, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	CBaseEntity *pEntHit = tr.m_pEnt;

	if ( pEntHit && pEntHit->m_takedamage != DAMAGE_NO)
	{
		m_hAutoAimTarget = pEntHit;

		// don't look through water
		if (!((GetWaterLevel() != 3 && pEntHit->GetWaterLevel() == 3) || (GetWaterLevel() == 3 && pEntHit->GetWaterLevel() == 0)))
		{
			if ( pEntHit->GetFlags() & FL_AIMTARGET )
			{
				m_fOnTarget = true;
			}

			//Already on target, don't autoaim
			return vec3_angle;
		}
	}

	int count = AimTarget_ListCount();
	if ( count )
	{
		CBaseEntity **pList = (CBaseEntity **)stackalloc( sizeof(CBaseEntity *) * count );
		AimTarget_ListCopy( pList, count );

		for ( int i = 0; i < count; i++ )
		{
			Vector center;
			Vector dir;
			float dot;
			CBaseEntity *pEntity = pList[i];

			if (!pEntity->IsAlive() || !pEntity->edict() )
				continue;

			if ( !g_pGameRules->ShouldAutoAim( this, pEntity->edict() ) )
				continue;

			// don't look through water
			if ((GetWaterLevel() != 3 && pEntity->GetWaterLevel() == 3) || (GetWaterLevel() == 3 && pEntity->GetWaterLevel() == 0))
				continue;

			// Only shoot enemies!
			if ( IRelationType( pEntity ) != D_HT )
			{
				if ( !pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch())
					// Msg( "friend\n");
					continue;
			}

			center = pEntity->BodyTarget( vecSrc );

			dir = (center - vecSrc);
			VectorNormalize( dir );

			// make sure it's in front of the player
			if (DotProduct (dir, v_forward ) < 0)
				continue;

			dot = fabs( DotProduct (dir, v_right ) ) 
				+ fabs( DotProduct (dir, v_up ) ) * 0.5;

			// tweak for distance
			dot *= 1.0 + 0.2 * ((center - vecSrc).Length() / flDist);

			if (dot > bestdot)
				continue;	// to far to turn

			UTIL_TraceLine( vecSrc, center, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			if (tr.fraction != 1.0 && tr.m_pEnt != pEntity )
			{
				// Msg( "hit %s, can't see %s\n", STRING( tr.u.ent->classname ), STRING( pEdict->classname ) );
				continue;
			}

			// can shoot at this one
			bestdot = dot;
			bestent = pEntity;
			bestdir = dir;
		}
		if ( bestent )
		{
			QAngle bestang;
			VectorAngles( bestdir, bestang );

			bestang -= EyeAngles() - m_Local.m_vecPunchAngle;

			m_hAutoAimTarget = bestent;

			if ( bestent->GetFlags() & FL_AIMTARGET )
			{
				m_fOnTarget = true;
			}

			return bestang;
		}
	}

	return QAngle( 0, 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::ResetAutoaim( void )
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = QAngle( 0, 0, 0 );
		engine->CrosshairAngle( edict(), 0, 0 );
	}
	m_fOnTarget = false;
}

// ==========================================================================
//	> Weapon stuff
// ==========================================================================

//-----------------------------------------------------------------------------
// Purpose: Override base class, player can always use weapon
// Input  : A weapon
// Output :	true or false
//-----------------------------------------------------------------------------
bool CBasePlayer::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	return true;
}



//-----------------------------------------------------------------------------
// Purpose: Override to clear dropped weapon from the hud
//-----------------------------------------------------------------------------
void CBasePlayer::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget /* = NULL */, const Vector *pVelocity /* = NULL */ )
{
	bool bWasActiveWeapon = false;
	if ( pWeapon == GetActiveWeapon() )
	{
		bWasActiveWeapon = true;
	}

	if ( pWeapon )
	{
		if ( bWasActiveWeapon )
		{
			pWeapon->SendWeaponAnim( ACT_VM_IDLE );
		}
	}

	BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );

	if ( bWasActiveWeapon )
	{
		if (!SwitchToNextBestWeapon( NULL ))
		{
			CBaseViewModel *vm = GetViewModel();
			if ( vm )
			{
				vm->m_fEffects |= EF_NODRAW;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : weaponSlot - 
//-----------------------------------------------------------------------------
void CBasePlayer::Weapon_DropSlot( int weaponSlot )
{
	CBaseCombatWeapon *pWeapon;

	// Check for that slot being occupied already
	for ( int i=0; i < MAX_WEAPONS; i++ )
	{
		pWeapon = GetWeapon( i );
		
		if ( pWeapon != NULL )
		{
			// If the slots match, it's already occupied
			if ( pWeapon->GetSlot() == weaponSlot )
			{
				Weapon_Drop( pWeapon, NULL, NULL );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override to add weapon to the hud
//-----------------------------------------------------------------------------
void CBasePlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );

	// should we switch to this item?
	if ( g_pGameRules->FShouldSwitchWeapon( this, pWeapon ) )
	{
		Weapon_Switch( pWeapon );
	}
}


//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
CBaseEntity *CBasePlayer::HasNamedPlayerItem( const char *pszItemName )
{
	for ( int i = 0 ; i < WeaponCount() ; i++ )
	{
		if ( !GetWeapon(i) )
			continue;

		if ( FStrEq( pszItemName, GetWeapon(i)->GetClassname() ) )
		{
			return GetWeapon(i);
		}
	}

	return NULL;
}



//================================================================================
// TEAM HANDLING
//================================================================================
//-----------------------------------------------------------------------------
// Purpose: Put the player in the specified team
//-----------------------------------------------------------------------------
void CBasePlayer::ChangeTeam( int iTeamNum )
{
	// if this is our current team, just abort
	if ( iTeamNum == GetTeamNumber() )
		return;

	// Immediately tell the client that he's changing team. This has to be done
	// first, so that all user messages that follow as a result of the team change
	// come after this one, allowing the client to be prepared for them.
	CSingleUserRecipientFilter user( this );
	user.MakeReliable();
	UserMessageBegin( user, "TeamChange" );
		WRITE_BYTE( iTeamNum );
	MessageEnd();

	// Remove him from his current team
	if ( GetTeam() )
	{
		GetTeam()->RemovePlayer( this );
	}

	// Are we being added to a team?
	if ( iTeamNum )
	{
		GetGlobalTeam( iTeamNum )->AddPlayer( this );
	}

	BaseClass::ChangeTeam( iTeamNum );
}

//-----------------------------------------------------------------------------
// Purpose: Locks a player to the spot; they can't move, shoot, or be hurt
//-----------------------------------------------------------------------------
void CBasePlayer::LockPlayerInPlace( void )
{
	if ( m_iPlayerLocked )
		return;

	AddFlag( FL_GODMODE | FL_FROZEN );
	SetMoveType( MOVETYPE_NONE );
	m_iPlayerLocked = true;

	// force a client data update, so that anything that has been done to
	// this player previously this frame won't get delayed in being sent
	UpdateClientData();
}

//-----------------------------------------------------------------------------
// Purpose: Unlocks a previously locked player
//-----------------------------------------------------------------------------
void CBasePlayer::UnlockPlayer( void )
{
	if ( !m_iPlayerLocked )
		return;

	RemoveFlag( FL_GODMODE | FL_FROZEN );
	SetMoveType( MOVETYPE_WALK );
	m_iPlayerLocked = false;
}

bool CBasePlayer::ClearUseEntity()
{
	if ( m_hUseEntity != NULL )
	{
		// Stop controlling the train/object
		// TODO: Send HUD Update
		m_hUseEntity->Use( this, this, USE_OFF, 0 );
		m_hUseEntity = NULL;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Pick a vector for the player to shoot along vector aim( missilespeed )
// Input  : speed - missile speed
//			result - resulting aim vector
//-----------------------------------------------------------------------------
void CBasePlayer::GetAimVector( float speed, float aimfactor, Vector& result )
{
	CBaseEntity	*check, *bestent = NULL;
	Vector	start, dir, end, bestdir;
	int		i, j;
	trace_t	tr;
	float	dist, bestdist;
	
	start = EyePosition();

	Vector	forward;
	EyeVectors( &forward );

	// try sending a trace straight
	dir = forward;
	
	end = start + ( MAX_COORD_INTEGER * 0.25 ) * dir;
	UTIL_TraceLine( start, end, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	CBaseEntity *ent = tr.m_pEnt;

	if ( ent && ( ent->GetFlags() & FL_AIMTARGET ) )
	{
		result = forward;
		return;
	}

	// try all possible entities
	bestdir		= dir;
	bestdist	= aimfactor;
	
	for (i=1 ; i < engine->GetEntityCount() ; i++ )
	{
		check = GetContainingEntity( engine->PEntityOfEntIndex( i ) );
		if ( !check )
			continue;

		if ( !( check->GetFlags() & FL_AIMTARGET ) || (check->GetFlags() & FL_STATICPROP) )
			continue;

		if ( check == this )
			continue;

		for (j=0 ; j<3 ; j++)
		{
			end[j] = check->GetAbsOrigin()[j] + 0.75 *(check->WorldAlignMins()[j] + check->WorldAlignMaxs()[j]) + 0.0 * GetViewOffset()[j];
		}
		
		dir = end - start;
		VectorNormalize( dir );
		
		dist = dir.Dot( forward );
		if ( dist < bestdist )
		{
			// to far to turn
			continue;	
		}

		UTIL_TraceLine( start, end, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.m_pEnt == check )
		{	// can shoot at this one
			bestdist	= dist;
			bestent		= check;
			bestdir		= dir;
		}
	}

	result = bestdir;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::HideViewModels( void )
{
	for ( int i = 0 ; i < MAX_VIEWMODELS; i++ )
	{
		CBaseViewModel *vm = GetViewModel( i );
		if ( !vm )
			continue;

		vm->SetWeaponModel( NULL, NULL );
	}
}

class CStripWeapons : public CPointEntity
{
	DECLARE_CLASS( CStripWeapons, CPointEntity );
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
};

LINK_ENTITY_TO_CLASS( player_weaponstrip, CStripWeapons );

void CStripWeapons::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = NULL;

	if ( pActivator && pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = (CBasePlayer *)CBaseEntity::Instance( engine->PEntityOfEntIndex( 1 ) );
	}

	if ( pPlayer )
		pPlayer->RemoveAllItems( false );
}


class CRevertSaved : public CPointEntity
{
	DECLARE_CLASS( CRevertSaved, CPointEntity );
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	LoadThink( void );

	DECLARE_DATADESC();

	inline	float	Duration( void ) { return m_Duration; }
	inline	float	HoldTime( void ) { return m_HoldTime; }
	inline	float	LoadTime( void ) { return m_loadTime; }

	inline	void	SetDuration( float duration ) { m_Duration = duration; }
	inline	void	SetHoldTime( float hold ) { m_HoldTime = hold; }
	inline	void	SetLoadTime( float time ) { m_loadTime = time; }

	//Inputs
	void InputReload(inputdata_t &data);

private:

	float	m_loadTime;
	float	m_Duration;
	float	m_HoldTime;
};

LINK_ENTITY_TO_CLASS( player_loadsaved, CRevertSaved );

BEGIN_DATADESC( CRevertSaved )

	DEFINE_KEYFIELD( CRevertSaved, m_loadTime, FIELD_FLOAT, "loadtime" ),
	DEFINE_KEYFIELD( CRevertSaved, m_Duration, FIELD_FLOAT, "duration" ),
	DEFINE_KEYFIELD( CRevertSaved, m_HoldTime, FIELD_FLOAT, "holdtime" ),

	DEFINE_INPUTFUNC( CRevertSaved, FIELD_VOID, "Reload", InputReload ),


	// Function Pointers
	DEFINE_FUNCTION( CRevertSaved, LoadThink ),

	// Inputs
	DEFINE_INPUTFUNC( CRevertSaved, FIELD_VOID, "Reload", InputReload ),

END_DATADESC()


void CRevertSaved::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	UTIL_ScreenFadeAll( m_clrRender, Duration(), HoldTime(), FFADE_OUT );
	SetNextThink( gpGlobals->curtime + LoadTime() );
	SetThink( &CRevertSaved::LoadThink );
}

void CRevertSaved::InputReload( inputdata_t &inputdata )
{
	UTIL_ScreenFadeAll( m_clrRender, Duration(), HoldTime(), FFADE_OUT );
	SetNextThink( gpGlobals->curtime + LoadTime() );
	SetThink( &CRevertSaved::LoadThink );
}

void CRevertSaved::LoadThink( void )
{
	if ( !gpGlobals->deathmatch )
	{
		engine->ServerCommand("reload\n");
	}
}


void SendProxy_CropFlagsToPlayerFlagBitsLength( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID)
{
	int mask = (1<<PLAYER_FLAG_BITS) - 1;
	int data = *(int *)pVarData;

	pOut->m_Int = ( data & mask );
}
// -------------------------------------------------------------------------------- //
// SendTable for CPlayerState.
// -------------------------------------------------------------------------------- //

	BEGIN_SEND_TABLE_NOBASE(CPlayerState, DT_PlayerState)
		SendPropInt		(SENDINFO(deadflag),	1, SPROP_UNSIGNED ),
	END_SEND_TABLE()

// -------------------------------------------------------------------------------- //
// This data only gets sent to clients that ARE this player entity.
// -------------------------------------------------------------------------------- //

	BEGIN_SEND_TABLE_NOBASE( CBasePlayer, DT_LocalPlayerExclusive )

		SendPropDataTable	( SENDINFO_DT(m_Local), &REFERENCE_SEND_TABLE(DT_Local) ),
		
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 0), 8, 0,	-32.0, 32.0f),
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 1), 8, 0,	-32.0, 32.0f),
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 2), 10, 0,	0.0f, 128.0f),
		SendPropFloat		( SENDINFO(m_flFriction),		8,	SPROP_ROUNDDOWN,	0.0f,	4.0f),

		SendPropArray(
			SendPropInt( SENDINFO_ARRAY(m_iAmmo), 10, SPROP_UNSIGNED ),
			m_iAmmo),

		SendPropInt			( SENDINFO(m_fFlags), PLAYER_FLAG_BITS, SPROP_UNSIGNED, SendProxy_CropFlagsToPlayerFlagBitsLength ),
		SendPropInt			( SENDINFO( m_fOnTarget ), 2, SPROP_UNSIGNED ),

		SendPropInt			( SENDINFO( m_nTickBase ) ),
		SendPropInt			( SENDINFO( m_nNextThinkTick ) ),

		SendPropInt			( SENDINFO( m_lifeState ), 3, SPROP_UNSIGNED ),

		SendPropEHandle		( SENDINFO( m_hGroundEntity ) ),
		SendPropEHandle		( SENDINFO( m_hLastWeapon ) ),

		SendPropInt			( SENDINFO( m_nExplosionFX ), 2, SPROP_UNSIGNED ),

 		SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 0), 20, 0, -2048.0f, 2048.0f  ),
 		SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 1), 20, 0, -2048.0f, 2048.0f  ),
 		SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 2), 16, 0, -2048.0f, 2048.0f  ),

		SendPropArray		( SendPropEHandle( SENDINFO_ARRAY( m_hViewModel ) ), m_hViewModel ),

		SendPropEHandle		( SENDINFO( m_hConstraintEntity)),
		SendPropVector		( SENDINFO( m_vecConstraintCenter), 0, SPROP_NOSCALE ),
		SendPropFloat		( SENDINFO( m_flConstraintRadius ), 0, SPROP_NOSCALE ),
		SendPropFloat		( SENDINFO( m_flConstraintWidth ), 0, SPROP_NOSCALE ),
		SendPropFloat		( SENDINFO( m_flConstraintSpeedFactor ), 0, SPROP_NOSCALE ),

		SendPropInt			( SENDINFO( m_iObserverMode ), 3, SPROP_UNSIGNED ),
		SendPropEHandle		( SENDINFO( m_hObserverTarget ) ),

		SendPropInt			( SENDINFO( m_nWaterLevel ), 2, SPROP_UNSIGNED ),

	END_SEND_TABLE()


// -------------------------------------------------------------------------------- //
// DT_BasePlayer sendtable.
// -------------------------------------------------------------------------------- //

	void SendProxy_Origin( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)pStruct;
		Assert( pPlayer && pPlayer->pev );

		pOut->m_Vector[0] = pPlayer->GetLocalOrigin()[0];
		pOut->m_Vector[1] = pPlayer->GetLocalOrigin()[1];
		pOut->m_Vector[2] = pPlayer->GetLocalOrigin()[2];
	}


	IMPLEMENT_SERVERCLASS_ST( CBasePlayer, DT_BasePlayer )

		SendPropDataTable(SENDINFO_DT(pl), &REFERENCE_SEND_TABLE(DT_PlayerState), SendProxy_DataTableToDataTable),
		SendPropEHandle(SENDINFO(m_hVehicle)),
		
		SendPropInt		(SENDINFO(m_iHealth), 10 ),

		SendPropFloat		(SENDINFO(m_flMaxspeed), 12, SPROP_ROUNDDOWN, 0.0f, 2048.0f ),  // CL

		// Data that only gets sent to the local player.
		SendPropDataTable( "localdata", 0, &REFERENCE_SEND_TABLE(DT_LocalPlayerExclusive), SendProxy_SendLocalDataTable ),

	END_SEND_TABLE()

//=============================================================================
//
// Player Physics Shadow Code
//

void CBasePlayer::SetupVPhysicsShadow( CPhysCollide *pStandModel, const char *pStandHullName, CPhysCollide *pCrouchModel, const char *pCrouchHullName )
{
	solid_t solid;
	solid.params = g_PhysDefaultObjectParams;
	solid.params.mass = 85.0f;
	solid.params.inertia = 1e24f;
	solid.params.enableCollisions = false;
	//disable drag
	solid.params.dragCoefficient = 0;
	// create standing hull
	m_pShadowStand = PhysModelCreateCustom( this, pStandModel, GetLocalOrigin(), GetLocalAngles(), pStandHullName, false, &solid );
	m_pShadowStand->SetCallbackFlags( CALLBACK_GLOBAL_COLLISION | CALLBACK_SHADOW_COLLISION );

	// create crouchig hull
	m_pShadowCrouch = PhysModelCreateCustom( this, pCrouchModel, GetLocalOrigin(), GetLocalAngles(), pCrouchHullName, false, &solid );
	m_pShadowCrouch->SetCallbackFlags( CALLBACK_GLOBAL_COLLISION | CALLBACK_SHADOW_COLLISION );

	// default to stand
	VPhysicsSetObject( m_pShadowStand );

	// tell physics lists I'm a shadow controller object
	PhysAddShadow( this );	
	m_pPhysicsController = physenv->CreatePlayerController( m_pShadowStand );

	// init state
	if ( GetFlags() & FL_DUCKING )
	{
		SetVCollisionState( VPHYS_CROUCH );
	}
	else
	{
		SetVCollisionState( VPHYS_WALK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Empty, just want to keep the baseentity version from being called
//          current so we don't kick up dust, etc.
//-----------------------------------------------------------------------------
void CBasePlayer::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::VPhysicsShadowUpdate( IPhysicsObject *pPhysics )
{
	Vector newPosition;

	bool physicsUpdated = m_pPhysicsController->GetShadowPosition( &newPosition, NULL ) > 0 ? true : false;

	if ( m_pPhysicsController->IsInContact() )
	{
		m_touchedPhysObject = true;
	}

	if ( IsFollowingPhysics() )
	{
		m_touchedPhysObject = true;
	}

	if ( GetMoveType() == MOVETYPE_NOCLIP )
	{
		m_oldOrigin = GetAbsOrigin();
		return;
	}

	if ( !physicsUpdated )
		return;

	CBaseEntity	*groundentity = GetGroundEntity();
	IPhysicsObject *pPhysGround = NULL;

	if ( groundentity && groundentity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		pPhysGround = groundentity->VPhysicsGetObject();
		if ( pPhysGround && !pPhysGround->IsMoveable() )
			pPhysGround = NULL;
	}

	Vector newVelocity;
	pPhysics->GetPosition( &newPosition, 0 );
	pPhysics->GetVelocity( &newVelocity, NULL );
	//NDebugOverlay::Box( newPosition, WorldAlignMins(), WorldAlignMaxs(), 0,0,255, 24, 0.01f);

	Vector tmp = GetAbsOrigin() - newPosition;
	if ( !m_touchedPhysObject && !(GetFlags() & FL_ONGROUND) )
	{
		tmp.z *= 0.5f;	// don't care about z delta as much
	}
	float dist = tmp.LengthSqr();
	float deltaV = (newVelocity - GetAbsVelocity()).LengthSqr();

	if ( dist >= VPHYS_MAX_DISTSQR || deltaV >= VPHYS_MAX_VELSQR || (pPhysGround && !m_touchedPhysObject) )
	{
		if ( m_touchedPhysObject || pPhysGround )
		{
			if ( deltaV >= VPHYS_MAX_VELSQR )
			{
				Vector dir = GetAbsVelocity();
				float len = VectorNormalize(dir);
				float dot = DotProduct( newVelocity, dir );
				if ( dot > len )
					dot = len;
				else if ( dot < -len )
					dot = -len;

				dir *= dot;
				newVelocity -= dir;

				ApplyAbsVelocityImpulse( newVelocity );
			}

			trace_t trace;
			UTIL_TraceEntity( this, newPosition, newPosition, 
				MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
			if ( !trace.allsolid && !trace.startsolid )
			{
				SetAbsOrigin( newPosition );
				Relink();
			}
		}
		else
		{
			trace_t trace;
			UTIL_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin(), 
				MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
			
			// current position is not ok, fixup
			if ( trace.allsolid || trace.startsolid )
			{
				// STUCK!?!?!
				//Warning( "Stuck on %s!!\n", STRING(trace.u.ent->classname) );
				SetAbsOrigin( newPosition );
				Relink();
			}
		}
	}
	else
	{
		if ( m_touchedPhysObject )
		{
			// check my position (physics object could have simulated into my position
			// physics is not very far away, check my position
			trace_t trace;
			UTIL_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin(),
				MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
			
			// is current position ok?
			if ( trace.allsolid || trace.startsolid )
			{
				// stuck????!?!?
				SetAbsOrigin( newPosition );
				Relink();
			}
		}
	}
	m_oldOrigin = GetAbsOrigin();
	// UNDONE: Force physics object to be at player position when not touching phys???
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::VPhysicsGetShadowVelocity( IPhysicsObject *pPhysics, Vector &outVel )
{
	m_pPhysicsController->GetControlVelocity( outVel );
}

// recreate physics on save/load, don't try to save the state!
bool CBasePlayer::ShouldSavePhysics()
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::InitVCollision( void )
{
	// Cleanup any old vphysics stuff.
	VPhysicsDestroyObject();

	
	CPhysCollide *pModel = PhysCreateBbox( WorldAlignMins(), WorldAlignMaxs() );
	CPhysCollide *pCrouchModel = PhysCreateBbox( VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );

	SetupVPhysicsShadow( pModel, "player_stand", pCrouchModel, "player_crouch" );
}


void CBasePlayer::VPhysicsDestroyObject()
{
	// Since CBasePlayer aliases its pointer to the physics object, tell CBaseEntity to 
	// clear out its physics object pointer so we don't wind up deleting one of
	// the aliased objects twice.
	VPhysicsSetObject( NULL );

	PhysRemoveShadow( this );
	
	if ( m_pPhysicsController )
	{
		physenv->DestroyPlayerController( m_pPhysicsController );
		m_pPhysicsController = NULL;
	}

	if ( m_pShadowStand )
	{
		PhysDestroyObject( m_pShadowStand );
		m_pShadowStand = NULL;
	}
	if ( m_pShadowCrouch )
	{
		PhysDestroyObject( m_pShadowCrouch );
		m_pShadowCrouch = NULL;
	}

	BaseClass::VPhysicsDestroyObject();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::SetVCollisionState( int collisionState )
{
	m_vphysicsCollisionState = collisionState;
	switch( collisionState )
	{
	case VPHYS_WALK:
		m_pShadowStand->SetPosition( GetAbsOrigin(), vec3_angle, true );
		m_pShadowCrouch->EnableCollisions( false );
		m_pShadowStand->EnableCollisions( true );
		m_pPhysicsController->SetObject( m_pShadowStand );
		VPhysicsSwapObject( m_pShadowStand );
		break;

	case VPHYS_CROUCH:
		m_pShadowCrouch->SetPosition( GetAbsOrigin(), vec3_angle, true );
		m_pShadowStand->EnableCollisions( false );
		m_pShadowCrouch->EnableCollisions( true );
		m_pPhysicsController->SetObject( m_pShadowCrouch );
		VPhysicsSwapObject( m_pShadowCrouch );
		break;
	
	case VPHYS_NOCLIP:
		m_pShadowCrouch->EnableCollisions( false );
		m_pShadowStand->EnableCollisions( false );
		break;
	}
}


int CBasePlayer::GetFOV() const
{
	return m_Local.m_iFOV;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the FOV for the player
// Input  : FOV - target FOV
//			zoomRate - amount of time to interpolate between current FOV to target (default is instantly)
//-----------------------------------------------------------------------------
void CBasePlayer::SetFOV( int FOV, float zoomRate )
{
	m_Local.m_iFOV		= FOV;
	m_Local.m_flFOVRate	= zoomRate;
}


//-----------------------------------------------------------------------------
// Purpose: // static func
// Input  : set - 
//-----------------------------------------------------------------------------
void CBasePlayer::ModifyOrAppendPlayerCriteria( AI_CriteriaSet& set )
{
	CBasePlayer *player = ( CBasePlayer * )CBaseEntity::Instance( engine->PEntityOfEntIndex( 1 ) );
	if ( !player )
		return;

	// Append our health
	set.AppendCriteria( "playerhealth", UTIL_VarArgs( "%i", player->GetHealth() ) );
	float healthfrac = 0.0f;
	if ( player->GetMaxHealth() > 0 )
	{
		healthfrac = (float)player->GetHealth() / (float)player->GetMaxHealth();
	}

	set.AppendCriteria( "playerhealthfrac", UTIL_VarArgs( "%.3f", healthfrac ) );

	CBaseCombatWeapon *weapon = player->GetActiveWeapon();
	if ( weapon )
	{
		set.AppendCriteria( "playerweapon", weapon->GetClassname() );
	}
	else
	{
		set.AppendCriteria( "playerweapon", "none" );
	}

	// Append current activity name
	set.AppendCriteria( "playeractivity", CAI_BaseNPC::GetActivityName( player->GetActivity() ) );

	set.AppendCriteria( "playerspeed", UTIL_VarArgs( "%.3f", player->GetAbsVelocity().Length() ) );

	player->AppendContextToCriteria( set, "player" );
}


const QAngle& CBasePlayer::GetPunchAngle()
{
	return m_Local.m_vecPunchAngle.Get();
}


void CBasePlayer::SetPunchAngle( const QAngle &punchAngle )
{
	m_Local.m_vecPunchAngle = punchAngle;
}

//-----------------------------------------------------------------------------
// Purpose: Apply a movement constraint to the player
//-----------------------------------------------------------------------------
void CBasePlayer::ActivateMovementConstraint( CBaseEntity *pEntity, const Vector &vecCenter, float flRadius, float flConstraintWidth, float flSpeedFactor )
{
	m_hConstraintEntity = pEntity;
	m_vecConstraintCenter = vecCenter;
	m_flConstraintRadius = flRadius;
	m_flConstraintWidth = flConstraintWidth;
	m_flConstraintSpeedFactor = flSpeedFactor;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::DeactivateMovementConstraint( )
{
	m_hConstraintEntity = NULL;
	m_flConstraintRadius = 0.0f;
	m_vecConstraintCenter = vec3_origin;
}
