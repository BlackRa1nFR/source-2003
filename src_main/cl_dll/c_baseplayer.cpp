//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_baseplayer.h"
#include "weapon_selection.h"
#include "history_resource.h"
#include "iinput.h"
#include "view.h"
#include "iviewrender.h"
#include "dlight.h"
#include "iclientmode.h"
#include "iefx.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "c_soundscape.h"
#include "usercmd.h"
#include "c_playerresource.h"
#include "iclientvehicle.h"
#include "view_shared.h"
#include "C_VguiScreen.h"
#include "movevars_shared.h"
#include "prediction.h"
#include "tier0/vprof.h"

// Don't alias here
#if defined( CBasePlayer )
#undef CBasePlayer	
#endif

void IN_SetDead( bool dead );

#define FLASHLIGHT_DISTANCE		1000
#define MAX_VGUI_INPUT_MODE_SPEED 30
#define MAX_VGUI_INPUT_MODE_SPEED_SQ (MAX_VGUI_INPUT_MODE_SPEED*MAX_VGUI_INPUT_MODE_SPEED)

extern ConVar default_fov;
extern ConVar sensitivity;

ConVar zoom_sensitivity_ratio( "zoom_sensitivity_ratio", "1.0", 0, "Additional mouse sensitivity scale factor applied when FOV is zoomed in." );

extern void RecvProxy_FOV( const CRecvProxyData *pData, void *pStruct, void *pOut );

// -------------------------------------------------------------------------------- //
// RecvTable for CPlayerState.
// -------------------------------------------------------------------------------- //

	BEGIN_RECV_TABLE_NOBASE(CPlayerState, DT_PlayerState)
		RecvPropInt		(RECVINFO(deadflag)),
	END_RECV_TABLE()


BEGIN_RECV_TABLE_NOBASE( CPlayerLocalData, DT_Local )
	RecvPropArray(RecvPropInt(RECVINFO(m_chAreaBits[0])), m_chAreaBits),
	RecvPropInt(RECVINFO(m_iHideHUD)),

	// View
	RecvPropInt(RECVINFO(m_iFOV), 0, RecvProxy_FOV),
	RecvPropFloat(RECVINFO(m_flFOVRate)),

	RecvPropInt		(RECVINFO(m_bDucked)),
	RecvPropInt		(RECVINFO(m_bDucking)),
	RecvPropFloat	(RECVINFO(m_flDucktime)),
	RecvPropFloat	(RECVINFO(m_flFallVelocity)),
//	RecvPropInt		(RECVINFO(m_nOldButtons)),
	RecvPropVector	(RECVINFO(m_vecClientBaseVelocity)),
	RecvPropVector	(RECVINFO(m_vecPunchAngle)),
	RecvPropVector	(RECVINFO(m_vecPunchAngleVel)),
	RecvPropInt		(RECVINFO(m_bDrawViewmodel)),
	RecvPropInt		(RECVINFO(m_bWearingSuit)),
	RecvPropFloat	(RECVINFO(m_flStepSize)),
	RecvPropInt		(RECVINFO(m_bAllowAutoMovement)),

	// 3d skybox data
	RecvPropInt(RECVINFO(m_skybox3d.scale)),
	RecvPropVector(RECVINFO(m_skybox3d.origin)),
	RecvPropInt(RECVINFO(m_skybox3d.area)),

	// 3d skybox fog data
	RecvPropInt( RECVINFO( m_skybox3d.fog.enable ) ),
	RecvPropInt( RECVINFO( m_skybox3d.fog.blend ) ),
	RecvPropVector( RECVINFO( m_skybox3d.fog.dirPrimary ) ),
	RecvPropInt( RECVINFO( m_skybox3d.fog.colorPrimary ) ),
	RecvPropInt( RECVINFO( m_skybox3d.fog.colorSecondary ) ),
	RecvPropFloat( RECVINFO( m_skybox3d.fog.start ) ),
	RecvPropFloat( RECVINFO( m_skybox3d.fog.end ) ),

	// fog data
	RecvPropInt( RECVINFO( m_fog.enable ) ),
	RecvPropInt( RECVINFO( m_fog.blend ) ),
	RecvPropVector( RECVINFO( m_fog.dirPrimary ) ),
	RecvPropInt( RECVINFO( m_fog.colorPrimary ) ),
	RecvPropInt( RECVINFO( m_fog.colorSecondary ) ),
	RecvPropFloat( RECVINFO( m_fog.start ) ),
	RecvPropFloat( RECVINFO( m_fog.end ) ),
	RecvPropFloat( RECVINFO( m_fog.farz ) ),

	// audio data
	RecvPropVector( RECVINFO( m_audio.localSound[0] ) ),
	RecvPropVector( RECVINFO( m_audio.localSound[1] ) ),
	RecvPropVector( RECVINFO( m_audio.localSound[2] ) ),
	RecvPropVector( RECVINFO( m_audio.localSound[3] ) ),
	RecvPropInt( RECVINFO( m_audio.soundscapeIndex ) ),
	RecvPropInt( RECVINFO( m_audio.localBits ) ),
	RecvPropEHandle( RECVINFO( m_audio.ent ) ),
END_RECV_TABLE()

// -------------------------------------------------------------------------------- //
// This data only gets sent to clients that ARE this player entity.
// -------------------------------------------------------------------------------- //

	BEGIN_RECV_TABLE_NOBASE( C_BasePlayer, DT_LocalPlayerExclusive )

		RecvPropDataTable	( RECVINFO_DT(m_Local),0, &REFERENCE_RECV_TABLE(DT_Local) ),

		RecvPropFloat		( RECVINFO(m_vecViewOffset[0]) ),
		RecvPropFloat		( RECVINFO(m_vecViewOffset[1]) ),
		RecvPropFloat		( RECVINFO(m_vecViewOffset[2]) ),
		RecvPropFloat		( RECVINFO(m_flFriction) ),

		RecvPropArray(
			RecvPropInt( RECVINFO(m_iAmmo[0])),
			m_iAmmo),

		RecvPropInt			( RECVINFO(m_fFlags) ),

		RecvPropInt			( RECVINFO(m_fOnTarget) ),

		RecvPropInt			( RECVINFO( m_nTickBase ) ),
		RecvPropInt			( RECVINFO( m_nNextThinkTick ) ),

		RecvPropInt			( RECVINFO( m_lifeState ) ),

		RecvPropEHandle		( RECVINFO( m_hGroundEntity ) ),
		RecvPropEHandle		( RECVINFO( m_hLastWeapon ) ),

		RecvPropInt			( RECVINFO( m_nExplosionFX ) ),

 		RecvPropFloat		( RECVINFO(m_vecVelocity[0]) ),
 		RecvPropFloat		( RECVINFO(m_vecVelocity[1]) ),
 		RecvPropFloat		( RECVINFO(m_vecVelocity[2]) ),

		RecvPropArray		( RecvPropEHandle( RECVINFO( m_hViewModel[0] ) ), m_hViewModel ),

		RecvPropEHandle		( RECVINFO( m_hConstraintEntity)),
		RecvPropVector		( RECVINFO( m_vecConstraintCenter) ),
		RecvPropFloat		( RECVINFO( m_flConstraintRadius )),
		RecvPropFloat		( RECVINFO( m_flConstraintWidth )),
		RecvPropFloat		( RECVINFO( m_flConstraintSpeedFactor )),

		RecvPropInt			( RECVINFO( m_iObserverMode) ),
		RecvPropEHandle		( RECVINFO( m_hObserverTarget) ),

		RecvPropInt			( RECVINFO( m_nWaterLevel ) ),

	END_RECV_TABLE()

	
// -------------------------------------------------------------------------------- //
// DT_BasePlayer datatable.
// -------------------------------------------------------------------------------- //
	IMPLEMENT_CLIENTCLASS_DT(C_BasePlayer, DT_BasePlayer, CBasePlayer)
		// We have both the local and nonlocal data in here, but the server proxies
		// only send one.
		RecvPropDataTable( "localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalPlayerExclusive) ),

		RecvPropDataTable(RECVINFO_DT(pl), 0, &REFERENCE_RECV_TABLE(DT_PlayerState), DataTableRecvProxy_StaticDataTable),

		RecvPropEHandle( RECVINFO(m_hVehicle) ),

		RecvPropInt		(RECVINFO(m_iHealth)),

		RecvPropFloat		(RECVINFO(m_flMaxspeed)),

	END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( CPlayerState )

	DEFINE_PRED_FIELD( CPlayerState, deadflag, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	// DEFINE_FIELD( CPlayerState, netname, string_t ),
	// DEFINE_FIELD( CPlayerState, fixangle, FIELD_INTEGER ),
	// DEFINE_FIELD( CPlayerState, anglechangetotal, FIELD_FLOAT ),
	// DEFINE_FIELD( CPlayerState, anglechangefinal, FIELD_FLOAT ),
	// DEFINE_FIELD( CPlayerState, v_angle, FIELD_VECTOR ),

END_PREDICTION_DATA()	

BEGIN_PREDICTION_DATA_NO_BASE( CPlayerLocalData )

	// DEFINE_PRED_TYPEDESCRIPTION( CPlayerLocalData, m_skybox3d, sky3dparams_t ),
	// DEFINE_PRED_TYPEDESCRIPTION( CPlayerLocalData, m_fog, fogparams_t ),
	// DEFINE_PRED_TYPEDESCRIPTION( CPlayerLocalData, m_audio, audioparams_t ),
	DEFINE_FIELD( CPlayerLocalData, m_nStepside, FIELD_INTEGER ),

	DEFINE_PRED_FIELD( CPlayerLocalData, m_iHideHUD, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CPlayerLocalData, m_iFOV, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( CPlayerLocalData, m_vecPunchAngle, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.125f ),
	DEFINE_PRED_FIELD_TOL( CPlayerLocalData, m_vecPunchAngleVel, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.125f ),
	DEFINE_PRED_FIELD( CPlayerLocalData, m_bDrawViewmodel, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CPlayerLocalData, m_bWearingSuit, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CPlayerLocalData, m_bAllowAutoMovement, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( CPlayerLocalData, m_bDucked, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CPlayerLocalData, m_bDucking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CPlayerLocalData, m_flDucktime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( CPlayerLocalData, m_flFallVelocity, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.5f ),
//	DEFINE_PRED_FIELD( CPlayerLocalData, m_nOldButtons, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( CPlayerLocalData, m_nOldButtons, FIELD_INTEGER ),
	DEFINE_PRED_FIELD( CPlayerLocalData, m_vecClientBaseVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CPlayerLocalData, m_flStepSize, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()	

BEGIN_PREDICTION_DATA( C_BasePlayer )

	DEFINE_PRED_TYPEDESCRIPTION( C_BasePlayer, m_Local, CPlayerLocalData ),
	DEFINE_PRED_TYPEDESCRIPTION( C_BasePlayer, pl, CPlayerState ),

	DEFINE_PRED_FIELD( C_BasePlayer, m_hVehicle, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( C_BasePlayer, m_flMaxspeed, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.5f ),
	DEFINE_PRED_FIELD( C_BasePlayer, m_iHealth, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( C_BasePlayer, m_fOnTarget, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BasePlayer, m_nNextThinkTick, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BasePlayer, m_lifeState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( C_BasePlayer, m_nWaterLevel, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD( C_BasePlayer, m_nButtons, FIELD_INTEGER ),
	DEFINE_FIELD( C_BasePlayer, m_flWaterJumpTime, FIELD_FLOAT ),
	DEFINE_FIELD( C_BasePlayer, m_nImpulse, FIELD_INTEGER ),
	DEFINE_FIELD( C_BasePlayer, m_flStepSoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( C_BasePlayer, m_flSwimSoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( C_BasePlayer, m_vecLadderNormal, FIELD_VECTOR ),
	DEFINE_FIELD( C_BasePlayer, m_flPhysics, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( C_BasePlayer, m_szAnimExtension, FIELD_CHARACTER ),
	DEFINE_FIELD( C_BasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( C_BasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( C_BasePlayer, m_afButtonReleased, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BasePlayer, m_vecOldViewAngles, FIELD_VECTOR ),

	// DEFINE_ARRAY( C_BasePlayer, m_iOldAmmo, FIELD_INTEGER,  MAX_AMMO_TYPES ),

	//DEFINE_FIELD( C_BasePlayer, m_hOldVehicle, FIELD_EHANDLE ),
	// DEFINE_FIELD( C_BasePlayer, m_pModelLight, dlight_t* ),
	// DEFINE_FIELD( C_BasePlayer, m_pEnvironmentLight, dlight_t* ),
	// DEFINE_FIELD( C_BasePlayer, m_pBrightLight, dlight_t* ),
	DEFINE_FIELD( C_BasePlayer, m_hLastWeapon, FIELD_EHANDLE ),

	DEFINE_PRED_FIELD( C_BasePlayer, m_nTickBase, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( C_BasePlayer, m_hGroundEntity, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( C_BasePlayer, m_nExplosionFX, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_ARRAY( C_BasePlayer, m_hViewModel, FIELD_EHANDLE, MAX_VIEWMODELS, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( player, C_BasePlayer );

// -------------------------------------------------------------------------------- //
// Functions.
// -------------------------------------------------------------------------------- //
C_BasePlayer::C_BasePlayer()
{
	CONSTRUCT_PREDICTABLE( C_BasePlayer );

	AddVar( &m_vecViewOffset, &m_iv_vecViewOffset, LATCH_SIMULATION_VAR );

#ifdef _DEBUG																
	m_vecLadderNormal.Init();
	m_vecOldViewAngles.Init();
#endif

	m_pModelLight = 0;
	m_pEnvironmentLight = 0;
	m_pBrightLight = 0;
	m_pCurrentVguiScreen = NULL;
	m_pCurrentCommand = NULL;

	ResetObserverMode();
}

static C_BasePlayer *g_pLocalPlayer = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BasePlayer::~C_BasePlayer()
{
	DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
	if ( this == g_pLocalPlayer )
	{
		if ( m_nExplosionFX != FEXPLOSION_NONE )
		{
			ConVar *dsp_player = (ConVar *)cvar->FindVar( "dsp_player" );
			Assert( dsp_player );
			if ( dsp_player )
			{
				dsp_player->SetValue( 0 );
			}
		}
		g_pLocalPlayer = NULL;
	}
}

void C_BasePlayer::Spawn( void )
{
	pl.deadflag	= false;
	m_Local.m_bDrawViewmodel = true;
	m_Local.m_flStepSize = sv_stepsize.GetFloat();
	m_Local.m_bAllowAutoMovement = true;

	m_iHealth			= 100;
	m_takedamage		= DAMAGE_YES;
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_WALK );
	m_flMaxspeed		= 0.0f;

	// Clear all flags except for FL_FULLEDICT
	ClearFlags();
	AddFlag( FL_CLIENT );
	AddFlag( FL_AIMTARGET );

	SetFriction( 1.0f );

	m_fEffects		&= EF_NOSHADOW;	// only preserve the shadow flag
	m_lifeState		= LIFE_ALIVE;
	m_Local.m_iFOV	= 0;	// init field of view.
	m_nRenderFX = kRenderFxNone;

	m_flNextAttack	= gpGlobals->curtime;

	// dont let uninitialized value here hurt the player
	m_Local.m_flFallVelocity = 0;

    SetModel( "models/player.mdl" );
	SetSequence( SelectWeightedSequence( ACT_IDLE ) );

	if ( GetFlags() & FL_DUCKING ) 
		SetSize(VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		SetSize(VEC_HULL_MIN, VEC_HULL_MAX);

	Precache();

	SetThink(NULL);

	// SetCollisionGroup( COLLISION_GROUP_PLAYER );

	// g_pGameRules->PlayerSpawn( this );
}

//-----------------------------------------------------------------------------
// Purpose: Gets a pointer to the local player, if it exists yet.
// Output : C_BasePlayer
//-----------------------------------------------------------------------------
C_BasePlayer *C_BasePlayer::GetLocalPlayer( void )
{
	return g_pLocalPlayer;
}


CBaseEntity	* C_BasePlayer::GetObserverTarget()	// returns players targer or NULL
{
	return m_hObserverTarget;
}

bool C_BasePlayer::ViewModel_IsTransparent( void )
{
	return IsTransparent();
}

//-----------------------------------------------------------------------------
// Used by prediction, sets the view angles for the player
//-----------------------------------------------------------------------------
void C_BasePlayer::SetLocalViewAngles( const QAngle &viewAngles )
{
	pl.v_angle = viewAngles;
}


//-----------------------------------------------------------------------------
// returns the player name
//-----------------------------------------------------------------------------
const char * C_BasePlayer::GetPlayerName()
{
	return g_PR ? g_PR->Get_Name(entindex()) : "";
}

//-----------------------------------------------------------------------------
// Is the player dead?
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsPlayerDead()
{
	return pl.deadflag == true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_BasePlayer::SetVehicleRole( int nRole )
{
	if ( !IsInAVehicle() )
		return;

	// HL2 has only a player in a vehicle.
	if ( nRole > VEHICLE_DRIVER )
		return;

	char szCmd[64];
	sprintf( szCmd, "vehicleRole %i\n", nRole );
	engine->ServerCmd( szCmd );
}

//-----------------------------------------------------------------------------
// Purpose: Store original ammo data to see what has changed
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BasePlayer::OnPreDataChanged( DataUpdateType_t updateType )
{
	for (int i = 0; i < MAX_AMMO_TYPES; ++i)
	{
		m_iOldAmmo[i] = GetAmmoCount(i);
	}
	m_nOldExplosionFX = m_nExplosionFX;

	BaseClass::OnPreDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_BasePlayer::PostDataUpdate( DataUpdateType_t updateType )
{
	// This has to occur here as opposed to OnDataChanged so that EHandles to the player created
	//  on this same frame are not stomped because prediction thinks there
	//  isn't a local player yet!!!
	if ( IsLocalPlayer() )
	{
		// Make sure g_pLocalPlayer is correct
		g_pLocalPlayer = this;
		SetSimulatedEveryTick( true );
	}
	else
	{
		SetSimulatedEveryTick( false );
	}

	BaseClass::PostDataUpdate( updateType );

	// Only care about this for local player
	if ( IsLocalPlayer() )
	{
		//Update our FOV, including any zooms going on
		int iDefaultFOV = default_fov.GetInt();
		
		if ( m_Local.m_iFOV == 0 )
		{
			// Use player's preferred fov
			m_Local.m_iFOV = iDefaultFOV;
		}
		
		// Clamp FOV in MP
		int min_fov = GetMinFOV();

		// Don't let it go too low
		m_Local.m_iFOV = max( min_fov, m_Local.m_iFOV );

		if ( gHUD.m_flMouseSensitivityFactor )
		{
			gHUD.m_flMouseSensitivity = sensitivity.GetFloat() * gHUD.m_flMouseSensitivityFactor;
		}
		else
		{
			// No override, don't use huge sensitivity
			if ( m_Local.m_iFOV == iDefaultFOV )
			{  
				// reset to saved sensitivity
				gHUD.m_flMouseSensitivity = 0;
			}
			else
			{  
				// Set a new sensitivity that is proportional to the change from the FOV default and scaled
				//  by a separate compensating factor
				gHUD.m_flMouseSensitivity = 
					sensitivity.GetFloat() * // regular sensitivity
					((float)m_Local.m_iFOV / (float)iDefaultFOV) * // linear fov downscale
					zoom_sensitivity_ratio.GetFloat(); // sensitivity scale factor
			}
		}
	}
	else
	{
// FIXME: BUG BUG: HACK HACK:  okay, you get the point
// when we finally fix the delta encoding relative to multiple
// players and the baseline, we can remove this
#if defined( CLIENT_DLL )
		for ( int i = 0 ; i < MAX_VIEWMODELS; i++ )
		{
			m_hViewModel[ i ] = NULL;
		}
#endif
	}

	if ( m_Local.m_iv_vecPunchAngle.GetCurrent() != m_Local.m_vecPunchAngle )
	{
		m_Local.m_iv_vecPunchAngle.NoteChanged( this, LATCH_SIMULATION_VAR );
	}
}

void C_BasePlayer::OnRestore()
{
	BaseClass::OnRestore();

	// For ammo history icons to current value so they don't flash on level transtions
	for ( int i = 0; i < MAX_AMMO_TYPES; i++ )
	{
		m_iOldAmmo[i] = GetAmmoCount(i);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Process incoming data
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BasePlayer::OnDataChanged( DataUpdateType_t updateType )
{
	if ( IsLocalPlayer() )
	{
		SetPredictionEligible( true );
	}

	BaseClass::OnDataChanged( updateType );

	// Only care about this for local player
	if ( IsLocalPlayer() )
	{
		// Reset engine areabits pointer
		( *render->GetAreaBits() ) = m_Local.m_chAreaBits;

		// Check for Ammo pickups.
		for ( int i = 0; i < MAX_AMMO_TYPES; i++ )
		{
			if ( GetAmmoCount(i) > m_iOldAmmo[i] )
			{
				// Don't add to ammo pickup if the ammo doesn't do it
				const FileWeaponInfo_t *pWeaponData = gWR.GetWeaponFromAmmo(i);

				if ( !pWeaponData || !( pWeaponData->iFlags & ITEM_FLAG_NOAMMOPICKUPS ) )
				{
					// We got more ammo for this ammo index. Add it to the ammo history
					CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );
					pHudHR->AddToHistory( HISTSLOT_AMMO, i, abs(GetAmmoCount(i) - m_iOldAmmo[i]) );
				}
			}
		}

		IN_SetDead( GetHealth() <= 0 ? true : false );
		Soundscape_Update( m_Local.m_audio );

		if ( m_nOldExplosionFX != m_nExplosionFX )
		{
			ConVar *dsp_player = (ConVar *)cvar->FindVar( "dsp_player" );
			Assert( dsp_player );
			if ( dsp_player )
			{
				if ( m_nExplosionFX != FEXPLOSION_NONE )
				{
					bool shock = m_nExplosionFX == FEXPLOSION_SHOCK ? true : false;

					int effect = shock ? random->RandomInt( 35, 37 ) : random->RandomInt( 32, 34 );

					dsp_player->SetValue( effect );
				}
				else
				{
					dsp_player->SetValue( 0 );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Did we just enter a vehicle this frame?
//-----------------------------------------------------------------------------
bool C_BasePlayer::JustEnteredVehicle()
{
	if ( !IsInAVehicle() )
		return false;

	return ( m_hOldVehicle == m_hVehicle );
}

//-----------------------------------------------------------------------------
// Are we in VGUI input mode?.
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsInVGuiInputMode() const
{
	return (m_pCurrentVguiScreen.Get() != NULL);
}


//-----------------------------------------------------------------------------
// Check to see if we're in vgui input mode...
//-----------------------------------------------------------------------------
void C_BasePlayer::DetermineVguiInputMode( CUserCmd *pCmd )
{
	// If we're dead, close down and abort!
	if ( !IsAlive() )
	{
		DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
		m_pCurrentVguiScreen.Set( NULL );
		return;
	}

	// If we're in vgui mode *and* we're holding down mouse buttons,
	// stay in vgui mode even if we're outside the screen bounds
	if (m_pCurrentVguiScreen.Get() && (pCmd->buttons & (IN_ATTACK | IN_ATTACK2)) )
	{
		SetVGuiScreenButtonState( m_pCurrentVguiScreen.Get(), pCmd->buttons );

		// Kill all attack inputs if we're in vgui screen mode
		pCmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);
		return;
	}

	// We're not in vgui input mode if we're moving, or have hit a key
	// that will make us move...

	// Not in vgui mode if we're moving too quickly
	// ROBIN: Disabled movement preventing VGUI screen usage
	//if (GetVelocity().LengthSqr() > MAX_VGUI_INPUT_MODE_SPEED_SQ)
	if ( 0 )
	{
		DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
		m_pCurrentVguiScreen.Set( NULL );
		return;
	}

	// Don't enter vgui mode if we've got combat buttons held down
	bool bAttacking = false;
	if ( ((pCmd->buttons & IN_ATTACK) || (pCmd->buttons & IN_ATTACK2)) && !m_pCurrentVguiScreen.Get() )
	{
		bAttacking = true;
	}

	// Not in vgui mode if we're pushing any movement key at all
	// Not in vgui mode if we're in a vehicle...
	// ROBIN: Disabled movement preventing VGUI screen usage
	//if ((pCmd->forwardmove > MAX_VGUI_INPUT_MODE_SPEED) ||
	//	(pCmd->sidemove > MAX_VGUI_INPUT_MODE_SPEED) ||
	//	(pCmd->upmove > MAX_VGUI_INPUT_MODE_SPEED) ||
	//	(pCmd->buttons & IN_JUMP) ||
	//	(bAttacking) )
	if ( bAttacking || IsInAVehicle() )
	{ 
		DeactivateVguiScreen( m_pCurrentVguiScreen.Get() );
		m_pCurrentVguiScreen.Set( NULL );
		return;
	}

	// Not in vgui mode if there are no nearby screens
	C_BaseEntity *pOldScreen = m_pCurrentVguiScreen.Get();

	m_pCurrentVguiScreen = FindNearbyVguiScreen( EyePosition(), pCmd->viewangles, GetTeamNumber() );

	if (pOldScreen != m_pCurrentVguiScreen)
	{
		DeactivateVguiScreen( pOldScreen );
		ActivateVguiScreen( m_pCurrentVguiScreen.Get() );
	}

	if (m_pCurrentVguiScreen.Get())
	{
		SetVGuiScreenButtonState( m_pCurrentVguiScreen.Get(), pCmd->buttons );

		// Kill all attack inputs if we're in vgui screen mode
		pCmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handling
//-----------------------------------------------------------------------------
void C_BasePlayer::CreateMove( float flFrameTime, float flInputSampleTime, CUserCmd *pCmd )
{
	static bool bWasFrozen = false;

	// Allow the vehicle to clamp the view angles
	if ( IsInAVehicle() )
	{
		IClientVehicle *pVehicle = m_hVehicle.Get()->GetClientVehicle();
		if ( pVehicle )
		{
			pVehicle->UpdateViewAngles( this, pCmd );
			engine->SetViewAngles( pCmd->viewangles );
		}
	}

	// If the frozen flag is set, prevent view movement (server prevents the rest of the movement)
	if ( GetFlags() & FL_FROZEN )
	{
		// Don't stomp the first time we get frozen
		if ( bWasFrozen )
		{
			// Stomp the new viewangles with old ones
			pCmd->viewangles = m_vecOldViewAngles;
			engine->SetViewAngles( pCmd->viewangles );
		}
	}

	m_vecOldViewAngles = pCmd->viewangles;
	bWasFrozen = (GetFlags() & FL_FROZEN);

	// Check to see if we're in vgui input mode...
	DetermineVguiInputMode( pCmd );
}


//-----------------------------------------------------------------------------
// Purpose: Player has changed to a new team
//-----------------------------------------------------------------------------
void C_BasePlayer::TeamChange( int iNewTeam )
{
	// Base class does nothing
}


//-----------------------------------------------------------------------------
// Purpose: Do the flashlight
//-----------------------------------------------------------------------------
void C_BasePlayer::UpdateFlashlight()
{
	// JAY: Flashlight effect.  Currently just a light that floats in from of 
	// the player a few feet. Using 3 lights is better looking, but SLOW.  
	// A conical light projection might look better and be more efficient.

	// BRJ: Modified the flashlight to use a true spotlight for model illumination
	// and uses Jay's old method of making a spherical light that only illuminates
	// the lightmaps at the intersection point.

	// The Dim light is the flash light
	if ( IsEffectActive(EF_DIMLIGHT) )
	{
		if ( !m_pModelLight || ( m_pModelLight->key != index ))
		{
			// Set up the model light
			m_pModelLight = effects->CL_AllocDlight (index);
			m_pModelLight->flags = DLIGHT_NO_WORLD_ILLUMINATION;
			m_pModelLight->radius = FLASHLIGHT_DISTANCE;
			m_pModelLight->m_InnerAngle = 6;
			m_pModelLight->m_OuterAngle = 10;
			m_pModelLight->color.r = m_pModelLight->color.g = 
				m_pModelLight->color.b = 255;
			m_pModelLight->color.exponent = 5;

			// FEH: Need this here because the environment light's CL_AllocDLight
			// will kill it right away if we don't
			m_pModelLight->die = gpGlobals->curtime + 0.2f;
		}

		if ( !m_pEnvironmentLight || ( m_pEnvironmentLight->key != -index ))
		{
			// Set up the environment light
			m_pEnvironmentLight = effects->CL_AllocDlight (-index);
			m_pEnvironmentLight->flags = DLIGHT_NO_MODEL_ILLUMINATION;
			m_pEnvironmentLight->radius = 80;
		}

		VectorCopy (EyePosition(),  m_pModelLight->origin);

		Vector vecForward;
		EyeVectors( &vecForward );
		VectorCopy (vecForward,  m_pEnvironmentLight->m_Direction);

		// For bumped lighting
		VectorCopy (vecForward,  m_pModelLight->m_Direction);

		Vector end;
		end = m_pModelLight->origin + FLASHLIGHT_DISTANCE * vecForward;
		
		// Trace a line outward, don't use hitboxes (too slow)
		trace_t		pm;
		UTIL_TraceLine( m_pModelLight->origin, end, MASK_NPCWORLDSTATIC, NULL, COLLISION_GROUP_NONE, &pm );
		VectorCopy( pm.endpos, m_pEnvironmentLight->origin );

		float falloff = pm.fraction* FLASHLIGHT_DISTANCE;

		if ( falloff < 500 )
			falloff = 1.0;
		else
			falloff = 500.0 / falloff;

		falloff *= falloff;
		
		m_pEnvironmentLight->radius = 80;
		m_pEnvironmentLight->color.r = m_pEnvironmentLight->color.g = 
			m_pEnvironmentLight->color.b = 255 * falloff;
		m_pEnvironmentLight->color.exponent = 0;

		// Make it live for a bit
		m_pModelLight->die = gpGlobals->curtime + 0.2f;
		m_pEnvironmentLight->die = gpGlobals->curtime + 0.2f;

		// Update list of surfaces we influence
		render->TouchLight( m_pEnvironmentLight );
	}
	else
	{
		// Clear out the light
		if ( m_pModelLight && ( m_pModelLight->key == index ))
		{
			m_pModelLight->die = gpGlobals->curtime;
			m_pModelLight = 0;
		}
		if ( m_pEnvironmentLight && ( m_pEnvironmentLight->key == -index ) )
		{
			m_pEnvironmentLight->die = gpGlobals->curtime;
			m_pEnvironmentLight = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Creates player flashlight if it's ative
//-----------------------------------------------------------------------------
void C_BasePlayer::Flashlight( void )
{
	UpdateFlashlight();

	if ( IsEffectActive( EF_BRIGHTLIGHT ) )
	{
		if ( !m_pBrightLight || ( m_pBrightLight->key != LIGHT_INDEX_PLAYER_BRIGHT + index ))
		{
			// Set up the bright light
			m_pBrightLight = effects->CL_AllocDlight (LIGHT_INDEX_PLAYER_BRIGHT + index);
			m_pBrightLight->color.r = m_pBrightLight->color.g = 
				m_pBrightLight->color.b = 250;
			m_pBrightLight->color.exponent = 5;
		}
		VectorCopy (GetAbsOrigin(),  m_pBrightLight->origin);
		m_pBrightLight->origin[2] += 16;
		m_pBrightLight->die = gpGlobals->curtime + 0.2f;
	}
	else
	{
		if ( m_pBrightLight && ( m_pBrightLight->key == (LIGHT_INDEX_PLAYER_BRIGHT + index) ))
		{
			m_pBrightLight->die = gpGlobals->curtime;
			m_pBrightLight = 0;
		}
	}

	// Checdk for muzzle flash and apply to view model
	C_BaseEntity *ve = cl_entitylist->GetEnt( render->GetViewEntity() );
	
	// Add muzzle flash to gun model if player is flashing
	if ( ve && ve->IsEffectActive( EF_MUZZLEFLASH) )		
	{
		if ( this == GetLocalPlayer() )
		{
			for ( int i = 0; i < MAX_VIEWMODELS; i++ )
			{
				C_BaseViewModel *vm = GetViewModel( i );
				if ( !vm )
					continue;

				vm->ActivateEffect( EF_MUZZLEFLASH, true );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Engine is asking whether to add this player to the visible entities list
//-----------------------------------------------------------------------------
void C_BasePlayer::AddEntity( void )
{
	// FIXME/UNDONE:  Should the local player say yes to adding itself now 
	// and then, when it ges time to render and it shouldn't still do the render with
	// STUDIO_EVENTS set so that its attachment points will get updated even if not
	// in third person?


	// If set to invisible, skip. Do this before resetting the entity pointer so it has 
	// valid data to decide whether it's visible.
	if ( !ShouldDraw() || !g_pClientMode->ShouldDrawLocalPlayer( this ) )
	{
		return;
	}

	// Server says don't interpolate this frame, so set previous info to new info.
	if ( IsEffectActive(EF_NOINTERP) || 
		Teleported() )
	{
		ResetLatched();
	}

	// Add in lighting effects
	CreateLightEffects();
}


//-----------------------------------------------------------------------------
// Called when not in tactical mode. Allows view to be overriden for things like driving a tank.
//-----------------------------------------------------------------------------
void C_BasePlayer::OverrideView( CViewSetup *pSetup )
{
}


bool C_BasePlayer::ShouldDraw()
{
	return !IsDormant() && (!IsLocalPlayer() || input->CAM_IsThirdPerson()) && BaseClass::ShouldDraw();
}

void C_BasePlayer::GetChaseCamView(Vector& eyeOrigin, QAngle& eyeAngles)
{
	CBaseEntity	* target = GetObserverTarget();

	if ( !target ) 
	{
		// just copy a save in-map position
		VectorCopy( EyePosition(), eyeOrigin );
		VectorCopy( EyeAngles(), eyeAngles );
		return;
	};

	QAngle viewangles;

	trace_t tr;

	Vector forward, viewpoint;

	Vector origin = target->EyePosition(); // target is a player with eyes

	engine->GetViewAngles( viewangles );

	AngleVectors( viewangles, &forward );

	VectorNormalize( forward );

	VectorMA(origin, -96.0f, forward, viewpoint );
	
	UTIL_TraceLine( origin, viewpoint, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	if (tr.fraction < 1.0)
	{
		// step 4 units away from wall to avoid clipping flaws
		VectorMA(origin, -(96.0f * tr.fraction - 4.0), forward, viewpoint );
	}

	VectorCopy( viewangles, eyeAngles );
	VectorCopy( viewpoint, eyeOrigin );
}

void C_BasePlayer::GetInEyeCamView(Vector& eyeOrigin, QAngle& eyeAngles)
{
	CBaseEntity	* target = GetObserverTarget();

	if ( !target ) 
	{
		// just copy a save in-map position
		VectorCopy( EyePosition(), eyeOrigin );
		VectorCopy( EyeAngles(), eyeAngles );
		return;
	};

	VectorCopy( target->GetRenderAngles(), eyeAngles );
	VectorCopy( target->EyePosition(), eyeOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: Return the weapon to have open the weapon selection on, based upon our currently active weapon
//			Base class just uses the weapon that's currently active.
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *C_BasePlayer::GetActiveWeaponForSelection( void )
{
	return GetActiveWeapon();
}

C_BaseAnimating* C_BasePlayer::GetRenderedWeaponModel()
{
	// Attach to either their weapon model or their view model.
	if ( input->CAM_IsThirdPerson() || this != C_BasePlayer::GetLocalPlayer() )
	{
		return GetActiveWeapon();
	}
	else
	{
		return GetViewModel();
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsLocalPlayer( void )
{
	return ( index == engine->GetPlayer() ? true : false );
}


// For weapon prediction
void C_BasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	// FIXME
}

void C_BasePlayer::UpdateClientData( void )
{
	// Update all the items
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		if ( GetWeapon(i) )  // each item updates it's successors
			GetWeapon(i)->UpdateClientData( this );
	}
}

// Prediction stuff
void C_BasePlayer::PreThink( void )
{
	ItemPreFrame();

	UpdateClientData();

	if (m_lifeState >= LIFE_DYING)
		return;

	//
	// If we're not on the ground, we're falling. Update our falling velocity.
	//
	if ( !( GetFlags() & FL_ONGROUND ) )
	{
		m_Local.m_flFallVelocity = -GetAbsVelocity().z;
	}
}

void C_BasePlayer::PostThink( void )
{
	if ( IsAlive())
	{
		// do weapon stuff
		ItemPostFrame();

		if ( GetFlags() & FL_ONGROUND )
		{		
			m_Local.m_flFallVelocity = 0;
		}

		// Don't allow bogus sequence on player
		if ( GetSequence() == -1 )
		{
			SetSequence( 0 );
		}

		StudioFrameAdvance();
	}

	// Even if dead simulate entities
	SimulatePlayerSimulatedEntities();
}

//-----------------------------------------------------------------------------
// Purpose: Simulate the player for this frame
//-----------------------------------------------------------------------------
void C_BasePlayer::Simulate()
{
	//Frame updates
	if ( this == C_BasePlayer::GetLocalPlayer() )
	{
		//Update the flashlight
		Flashlight();

		//Update FOV zooming
		UpdateFOV();
	}

	BaseClass::Simulate();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseViewModel
//-----------------------------------------------------------------------------
C_BaseViewModel *C_BasePlayer::GetViewModel( int index /*= 0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	C_BaseViewModel *vm = m_hViewModel[ index ];
	Assert( !vm || !vm->GetOwnerEntity() || ( vm->GetOwnerEntity() == this  ) );
	return vm;
}

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector C_BasePlayer::GetAutoaimVector( float flDelta )
{
	// Never autoaim a predicted weapon (for now)
	Vector	forward;
	AngleVectors( GetAbsAngles() + m_Local.m_vecPunchAngle, &forward );
	return	forward;
}

void C_BasePlayer::FireBullets( int cShots, const Vector &vecSrc, const Vector &vecDirShooting, 
	const Vector &vecSpread, float flDistance, int iBulletType, int iTracerFreq /*= 4*/, 
	int firingEntID /*= -1*/, int attachmentID /*= -1*/,int iDamage /*= 0*/, C_BaseEntity *pAttacker /*= NULL*/ )
{
	// FIXME
	//	Msg( "predicted C_BasePlayer::FireBullets\n" );
}


// Stuff for prediction
void C_BasePlayer::SetSuitUpdate(char *name, int fgroup, int iNoRepeat)
{
	// FIXME:  Do something here?
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BasePlayer::ResetAutoaim( void )
{
#if 0
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = QAngle( 0, 0, 0 );
		engine->CrosshairAngle( edict(), 0, 0 );
	}
#endif
	m_fOnTarget = false;
}

bool C_BasePlayer::ShouldPredict( void )
{
	// Do this before calling into baseclass so prediction data block gets allocated
	if ( IsLocalPlayer() )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Special processing for player simulation
// NOTE: Don't chain to BaseClass!!!!
//-----------------------------------------------------------------------------
void C_BasePlayer::PhysicsSimulate( void )
{
	VPROF( "C_BasePlayer::PhysicsSimulate" );
	// If we've got a moveparent, we must simulate that first.
	CBaseEntity *pMoveParent = GetMoveParent();
	if (pMoveParent)
	{
		pMoveParent->PhysicsSimulate();
	}

	// Make sure not to simulate this guy twice per frame
	if (m_nSimulationTick == gpGlobals->tickcount)
		return;

	m_nSimulationTick = gpGlobals->tickcount;

	Assert( g_pLocalPlayer == this );
	C_CommandContext *ctx = GetCommandContext();
	Assert( ctx );
	Assert( ctx->needsprocessing );
	if ( !ctx->needsprocessing )
		return;

	ctx->needsprocessing = false;

	// Handle FL_FROZEN.
	if(GetFlags() & FL_FROZEN)
	{
		ctx->cmd.frametime = 0.0;
		ctx->cmd.forwardmove = 0;
		ctx->cmd.sidemove = 0;
		ctx->cmd.upmove = 0;
		ctx->cmd.buttons = 0;
		ctx->cmd.impulse = 0;
		//VectorCopy ( pl.v_angle, ctx->cmd.viewangles );
	}

	// Run the next command
	prediction->RunCommand( 
		this, 
		&ctx->cmd, 
		MoveHelper() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BasePlayer::GetExplosionEffects( void ) const
{
	return m_nExplosionFX;
}


const QAngle& C_BasePlayer::GetPunchAngle()
{
	return m_Local.m_vecPunchAngle.Get();
}


void C_BasePlayer::SetPunchAngle( const QAngle &angle )
{
	m_Local.m_vecPunchAngle = angle;
}


float C_BasePlayer::GetWaterJumpTime() const
{
	return m_flWaterJumpTime;
}


//-----------------------------------------------------------------------------
// Purpose: Return true if this object can be +used by the player
//-----------------------------------------------------------------------------
bool C_BasePlayer::IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float C_BasePlayer::CurrentFOV( void )
{
	float fFOV = m_Local.m_iFOV;

	//See if we need to lerp the values
	if ( ( m_Local.m_iFOV != m_Local.m_iFOVStart ) && ( m_Local.m_flFOVRate > 0.0f ) )
	{
		float deltaTime = (float)( gpGlobals->curtime - m_Local.m_flFOVTime ) / m_Local.m_flFOVRate;

		if ( deltaTime >= 1.0f )
		{
			//If we're past the zoom time, just take the new value and stop lerping
			fFOV = m_Local.m_iFOVStart = m_Local.m_iFOV;
		}
		else
		{
			fFOV = SimpleSplineRemapVal( deltaTime, 0.0f, 1.0f, m_Local.m_iFOVStart, m_Local.m_iFOV );
		}
	}

	return fFOV;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BasePlayer::UpdateFOV( void )
{
	float	currentFOV = CurrentFOV();

	// Set for rendering
	engine->SetFieldOfView( currentFOV );
}


float C_BasePlayer::GetFOV() const
{
	return m_Local.m_iFOV;
}


//-----------------------------------------------------------------------------
// Purpose: Called when the FOV changes from the server-side
// Input  : *pData - 
//			*pStruct - 
//			*pOut - 
//-----------------------------------------------------------------------------
void RecvProxy_FOV( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CPlayerLocalData *pPlayerData = (CPlayerLocalData *) pStruct;

	//Hold onto the old FOV as our starting point
	int iNewFov = pData->m_Value.m_Int;
	if ( pPlayerData->m_iFOV != iNewFov )
	{
		pPlayerData->m_iFOVStart = pPlayerData->m_iFOV;

		//Get our start time for the zoom
		pPlayerData->m_flFOVTime = gpGlobals->curtime;
	}

	pPlayerData->m_iFOV = iNewFov;
}

//-----------------------------------------------------------------------------
// Purpose: Remove this player from a vehicle
//-----------------------------------------------------------------------------
void C_BasePlayer::LeaveVehicle( void )
{
	if ( NULL == m_hVehicle.Get() )
		return;

// Let server do this for now
#if 0
	IClientVehicle *pVehicle = GetVehicle();
	Assert( pVehicle );

	int nRole = pVehicle->GetPassengerRole( this );
	Assert( nRole >= 0 );

	SetParent( NULL );

	// Find the first non-blocked exit point:
	Vector vNewPos = GetAbsOrigin();
	QAngle qAngles = GetAbsAngles();
	pVehicle->GetPassengerExitPoint( nRole, &vNewPos, &qAngles );
	OnVehicleEnd( vNewPos );
	SetAbsOrigin( vNewPos );
	SetAbsAngles( qAngles );

	m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONS;
	m_fEffects &= ~EF_NODRAW;

	SetMoveType( MOVETYPE_WALK );
	SetCollisionGroup( COLLISION_GROUP_PLAYER );
	Relink();

	qAngles[ROLL] = 0;
	SnapEyeAngles( qAngles );

	m_hVehicle = NULL;
	pVehicle->SetPassenger(nRole, NULL);

	Weapon_Switch( m_hLastWeapon );
#endif
}


float C_BasePlayer::GetMinFOV()	const
{
	if ( engine->GetMaxClients() == 1 )
	{
		// Let them do whatever they want, more or less, in single player
		return 5;
	}
	else
	{
		return 75;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called by networking code when an entity is new to the PVS or comes down with the EF_NOINTERP flag set.
//  The position history data is flushed out right after this call, so we need to store off the current data
//  in the latched fields so we try to interpolate
// Input  : *ent - 
//			full_reset - 
//-----------------------------------------------------------------------------
void C_BasePlayer::ResetLatched( void )
{
	m_Local.m_iv_vecPunchAngle.Reset();
	BaseClass::ResetLatched();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BasePlayer::Interpolate( float currentTime )
{
	if ( !BaseClass::Interpolate( currentTime ) )
		return false;

	if ( GetPredictable() || IsClientCreated() )
		return true;

	m_Local.m_iv_vecPunchAngle.Interpolate( this, currentTime );
	return true;
}

float C_BasePlayer::GetFinalPredictedTime() const
{
	return ( m_nFinalPredictedTick * TICK_RATE );
}
