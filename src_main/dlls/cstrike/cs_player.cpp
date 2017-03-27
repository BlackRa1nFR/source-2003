//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "cs_player.h"
#include "cs_gamerules.h"
#include "trains.h"
#include "vcollide_parse.h"
#include "in_buttons.h"
#include "igamemovement.h"
#include "ai_hull.h"
#include "hl2_shareddefs.h"
#include "ndebugoverlay.h"
#include "weapon_csbase.h"
#include "decals.h"
#include "ammodef.h"
#include "IEffects.h"
#include "cs_client.h"
#include "client.h"
#include "cs_shareddefs.h"
#include "shake.h"
#include "team.h"
#include "weapon_c4.h"
#include "weapon_parse.h"
#include "weapon_knife.h"


#pragma warning( disable : 4355 )


#define CS_PLAYER_MODEL			"models/player/urban.mdl"
#define CS_TERRORIST_MODEL		"models/player/terror.mdl"
#define ARBITRARY_RUN_SPEED		75.0f
#define CS_PLAYER_VIEW_OFFSET	Vector( 0, 0, 53.5 )


#define WEAPON_SLOT_RIFLE		0
#define WEAPON_SLOT_PISTOL		1
#define WEAPON_SLOT_KNIFE		2
#define WEAPON_SLOT_GRENADES	3
#define WEAPON_SLOT_C4			4


#define KEVLAR_PRICE			650
#define HELMET_PRICE			350
#define ASSAULTSUIT_PRICE		1000

ConVar cs_ShowStateTransitions( "cs_ShowStateTransitions", "-2", 0, "cs_ShowStateTransitions <ent index or -1 for all>. Show player state transitions." );


EHANDLE g_pLastCTSpawn;
EHANDLE g_pLastTerroristSpawn;


// -------------------------------------------------------------------------------- //
// Classes
// -------------------------------------------------------------------------------- //

class CPhysicsPlayerCallback : public IPhysicsPlayerControllerEvent
{
public:
	int ShouldMoveTo( IPhysicsObject *pObject, const Vector &position )
	{
		CCSPlayer *pPlayer = (CCSPlayer *)pObject->GetGameData();
		if ( pPlayer->TouchedPhysics() )
		{
			return 0;
		}
		return 1;
	}
};

static CPhysicsPlayerCallback playerCallback;


// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( player, CCSPlayer );
PRECACHE_REGISTER(player);

IMPLEMENT_SERVERCLASS_ST( CCSPlayer, DT_CSPlayer )
	SendPropExclude( "DT_BaseAnimating" , "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating" , "m_flPlaybackRate" ),

	SendPropInt( SENDINFO( m_iPlayerState ), Q_log2( NUM_PLAYER_STATES )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iAccount ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bInBombZone ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bInBuyZone ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iClass ), Q_log2( CS_NUM_CLASSES )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_ArmorValue ), 8 ),
	SendPropFloat( SENDINFO( m_flEyeAnglesPitch ), 7, 0, -90, 90 )
END_SEND_TABLE()

// -------------------------------------------------------------------------------- //

int SelectDefaultTeam()
{
	int team;
	CCSGameRules *mp = CSGameRules();

	// Choose the team that's lacking players
	if ( mp->m_iNumTerrorist < mp->m_iNumCT )
	{
		team = TEAM_TERRORIST;
	}
	else if ( mp->m_iNumTerrorist > mp->m_iNumCT )
	{
		team = TEAM_CT;
	}
	// Choose the team that's losing
	else if ( mp->m_iNumTerroristWins < mp->m_iNumCTWins )
	{
		team = TEAM_TERRORIST;
	}
	else if ( mp->m_iNumCTWins < mp->m_iNumTerroristWins )
	{
		team = TEAM_CT;
	}
	else
	{
		// Teams and scores are equal, pick a random team
		if ( random->RandomInt( 0, 1 ) == 0 )
		{
			team = TEAM_CT;
		}
		else
		{
			team = TEAM_TERRORIST;
		}
	}

	if ( mp->TeamFull( team ) )
	{
		// Pick the opposite team
		if ( team == TEAM_TERRORIST )
		{
			team = TEAM_CT;
		}
		else
		{
			team = TEAM_TERRORIST;
		}

		// No choices left
		if ( mp->TeamFull( team ) )
			return TEAM_UNASSIGNED;
	}

	return team;
}


// -------------------------------------------------------------------------------- //
// CCSPlayer implementation.
// -------------------------------------------------------------------------------- //

CCSPlayer::CCSPlayer() : m_PlayerAnimState( this )
{
	m_bEscaped = false;
	m_iAccount = 0;
	m_bNotKilled = true;
	m_bIsVIP = false;
	m_iClass = CS_CLASS_NONE;
	m_flEyeAnglesPitch = 0;
	
	// Start them off on the STATE_SHOWLTEXT state.
	m_pCurStateInfo = NULL;
	State_Enter( STATE_SHOWLTEXT );

	m_lifeState = LIFE_DEAD;			// Start "dead".
	m_bInBombZone = false;
	m_bInBuyZone = false;
	m_flBuyZonePulseTime = 0;
}


CCSPlayer::~CCSPlayer()
{
}


CCSPlayer *CCSPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CCSPlayer::s_PlayerEdict = ed;
	return (CCSPlayer*)CreateEntityByName( className );
}


void CCSPlayer::Precache()
{
	engine->PrecacheModel( CS_PLAYER_MODEL );
	engine->PrecacheModel( CS_TERRORIST_MODEL );
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Allow pre-frame adjustments on the player
//-----------------------------------------------------------------------------
void CCSPlayer::PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper )
{
	BaseClass::PlayerRunCommand( ucmd, moveHelper );
}


void CCSPlayer::Touch( CBaseEntity *pOther )
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


void CCSPlayer::Spawn()
{
	// Get rid of the progress bar...
	SetProgressBarTime( 0 );

	// Set their player model.
	if ( GetTeamNumber() == TEAM_TERRORIST )
	{
		SetModel( CS_TERRORIST_MODEL );
	}
	else
	{
		SetModel( CS_PLAYER_MODEL );
	}
    
	//MIKETODO: get rid of the dependency on g_ulModelIndexPlayer.
	g_ulModelIndexPlayer = GetModelIndex();

	// Preserve some things that CBasePlayer::Spawn messes with. If this gets too messy, we can
	// copy CBasePlayer's Spawn and remove the stuff we need.
	int armor = ArmorValue();
	BaseClass::Spawn();
	SetArmorValue( armor );

	// Override what CBasePlayer set for the view offset.
	SetViewOffset( CS_PLAYER_VIEW_OFFSET );

	//
	// Our player movement speed is set once here. This will override the cl_xxxx
	// cvars unless they are set to be lower than this.
	//
	SetMaxSpeed( 250 );

	SetFOV( 90 );

	// MIKETODO: get rid of this.
	//	CheatImpulseCommands( 101 );

	m_bIsDefusing = false;

	Relink();

	// Special-case here. A bunch of things happen in CBasePlayer::Spawn(), and we really want the
	// player states to control these things, so give whatever player state we're in a chance
	// to reinitialize itself.
	State_Transition( m_iPlayerState );
}


void CCSPlayer::GiveDefaultItems()
{
	// Always give the player the knife.
	GiveNamedItem( "weapon_knife" );
	
	if ( GetTeamNumber() == TEAM_CT )
	{
		GiveNamedItem( "weapon_usp" );
		GiveAmmo( 24, BULLET_PLAYER_45ACP );
	}
	else if ( GetTeamNumber() == TEAM_TERRORIST )
	{
		GiveNamedItem( "weapon_glock" );
		GiveAmmo( 40, BULLET_PLAYER_9MM );
	}
}


void CCSPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	SetArmorValue( 0 );
	
	// Do this because for some reason the base class's Event_Killed likes to reset the
	// model from a global variable.
	g_ulModelIndexPlayer = GetModelIndex();

	m_bIsDefusing = false;
	State_Transition( STATE_DEATH_ANIM );	// Transition into the dying state.
	BaseClass::Event_Killed( info );
	
	CSGameRules()->CheckWinConditions();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCSPlayer::InitVCollision()
{
	BaseClass::InitVCollision();

	// Setup the HL2 specific callback.
	GetPhysicsController()->SetEventHandler( &playerCallback );
}


bool CCSPlayer::IsCarryingBomb() const
{
	return Weapon_OwnsThisType( WEAPON_C4_CLASSNAME ) != NULL;
}


bool CCSPlayer::HasShield() const
{
	//MIKETODO
	return false;
}


bool CCSPlayer::IsShieldDrawn() const
{
	//MIKETODO
	return false;
}


void CCSPlayer::CheatImpulseCommands( int iImpulse )
{
	switch( iImpulse )
	{
		case 101:
		{
			extern int gEvilImpulse101;
			gEvilImpulse101 = true;
			
			//GiveNamedItem( "weapon_ak47" );
			//GiveNamedItem( "weapon_aug" );
			AddAccount( 16000 );

			GiveAmmo( 250, BULLET_PLAYER_50AE );
			GiveAmmo( 250, BULLET_PLAYER_762MM );
			GiveAmmo( 250, BULLET_PLAYER_338MAG );
			GiveAmmo( 250, BULLET_PLAYER_556MM );
			GiveAmmo( 250, BULLET_PLAYER_9MM );
			GiveAmmo( 250, BULLET_PLAYER_BUCKSHOT );
			GiveAmmo( 250, BULLET_PLAYER_45ACP );
			GiveAmmo( 250, BULLET_PLAYER_357SIG );
			GiveAmmo( 250, BULLET_PLAYER_57MM );

			gEvilImpulse101 = false;
		}
		break;

		default:
		{
			BaseClass::CheatImpulseCommands( iImpulse );
		}
	}
}


bool CCSPlayer::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
{
	// Allow me to introduce myself to, er, myself.
	// I.e., always update the recipient player data even if it's nodraw (first person mode)
	if ( recipient == pev )
		return true;

	return BaseClass::ShouldTransmit( recipient, pvs, clientArea );
}


void CCSPlayer::PostThink()
{
	BaseClass::PostThink();

	
	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_flEyeAnglesPitch = EyeAngles()[PITCH];
	if ( m_flEyeAnglesPitch > 180.0f )
	{
		m_flEyeAnglesPitch -= 360.0f;
	}
	m_flEyeAnglesPitch = clamp( m_flEyeAnglesPitch, -90, 90 );


	// Update the animation state.
	m_PlayerAnimState.Update( GetAbsAngles()[YAW], m_flEyeAnglesPitch );

	// Reset this.. it gets reset each frame that we're in a bomb zone.
	m_bInBombZone = false;
	
	// Clear the "in buy zone" status every second. It has to stay on for a while so the
	// value gets sent to the client.
	if ( m_bInBuyZone && gpGlobals->curtime - m_flBuyZonePulseTime > 1.0f )
	{
		m_bInBuyZone = false;
	}
}


int CCSPlayer::OnTakeDamage( const CTakeDamageInfo &info )
{
	CBaseEntity *pInflictor = info.GetInflictor();
	if ( !pInflictor )
		return 0;

	// Teammates can't hurt each other.
	if ( pInflictor->GetTeamNumber() != GetTeamNumber() ||
		pInflictor == this ||
		info.GetAttacker() == this )
	{
		return BaseClass::OnTakeDamage( info );
	}
	else
	{
		return 0;
	}
}


void CCSPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	Activity idealActivity = GetActivity();
	int animDesired = GetSequence();

	// Figure out stuff about the current state.
	float speed = GetAbsVelocity().Length2D();
	bool isMoving = ( speed != 0.0f ) ? true : false;
	bool isDucked = ( GetFlags() & FL_DUCKING ) ? true : false;
	bool isStillJumping = !( GetFlags() & FL_ONGROUND ) && ( GetActivity() == ACT_HOP );
	bool isRunning = false;

	if ( speed > ARBITRARY_RUN_SPEED )
	{
		isRunning = true;
	}

	// Now figure out what to do based on the current state and the new state.
	switch ( playerAnim )
	{
	default:
	case PLAYER_RELOAD:
	case PLAYER_ATTACK1:
	case PLAYER_IDLE:
	case PLAYER_WALK:
		// Are we still jumping?
		// If so, keep playing the jump animation.
		if ( !isStillJumping )
		{
			idealActivity = ACT_WALK;

			if ( isDucked )
			{
				idealActivity = !isMoving ? ACT_CROUCHIDLE : ACT_RUN_CROUCH;
			}
			else
			{
				if ( isRunning )
				{
					idealActivity = ACT_RUN;
				}
				else
				{
					idealActivity = isMoving ? ACT_WALK : ACT_IDLE;
				}
			}

			// Allow body yaw to override for standing and turning in place
			idealActivity = m_PlayerAnimState.BodyYawTranslateActivity( idealActivity );
		}
		break;

	case PLAYER_JUMP:
		idealActivity = ACT_HOP;
		break;

	case PLAYER_DIE:
		// Uses Ragdoll now???
		idealActivity = ACT_DIESIMPLE;
		break;

	// FIXME:  Use overlays for reload, start/leave aiming, attacking
	case PLAYER_START_AIMING:
	case PLAYER_LEAVE_AIMING:
		idealActivity = ACT_WALK;
		break;
	}

	// No change requested?
	if ( ( GetActivity() == idealActivity ) && ( GetSequence() != -1 ) )
		return;

	animDesired = SelectWeightedSequence( idealActivity );

	SetActivity( idealActivity );

	// Already using the desired animation?
	if ( GetSequence() == animDesired )
		return;

	ResetSequence( animDesired );
}


//MIKETODO: this probably should let the shield model catch the trace attacks.
bool CCSPlayer::IsHittingShield( const Vector &vecDirection, trace_t *ptr )
{
	/*
	if ( HasShield() == false )
		 return false;

	if ( ptr->hitgroup == HITGROUP_SHIELD )
		 return true;

	if ( m_bShieldDrawn == false )
		 return false;
	
	Vector2D	vec2LOS;
	float		flDot;

	Vector vForward;
	AngleVectors( GetAbsAngles(), &vForward );
	
	vec2LOS = vecDirection.Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct ( vec2LOS , vForward.Make2D() );

	if ( flDot < -0.87f )
		 return true;
	*/
	return false;
}


void CCSPlayer::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	bool bShouldBleed = true;
	bool bShouldSpark = false;
	bool bHitShield = IsHittingShield( vecDir, ptr );

	CBasePlayer *pAttacker = (CBasePlayer*)ToBasePlayer( info.GetAttacker() );

	if ( ( GetTeamNumber() == pAttacker->GetTeamNumber() ) )
		bShouldBleed = false;
	
	if ( m_takedamage != DAMAGE_YES )
		return;

	m_LastHitGroup = ptr->hitgroup;
	float flDamage = info.GetDamage();

	if ( bHitShield )
	{
		/*
		flDamage = 0;
		bShouldBleed = false;

		if ( RANDOM_LONG( 0, 1 ) )
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/ric_metal-1.wav", 1, ATTN_NORM );
		else
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/ric_metal-2.wav", 1, ATTN_NORM );

		UTIL_Sparks( ptr->vecEndPos );

		pev->punchangle.x = flDamage * RANDOM_FLOAT ( -0.15, 0.15 );
		pev->punchangle.z = flDamage * RANDOM_FLOAT ( -0.15, 0.15 );

		if (pev->punchangle.x < 4)
			pev->punchangle.x = -4;

		if (pev->punchangle.z < -5)
			pev->punchangle.z = -5;
		else if (pev->punchangle.z > 5)
			pev->punchangle.z = 5;
		*/
	}
	else
	{
		switch ( ptr->hitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		
		case HITGROUP_HEAD:

			if ( m_iKevlar == 2 )
			{
				bShouldBleed = false;
				bShouldSpark = true;
			}
			
			flDamage *= 4;

			if ( bShouldBleed == true )
			{
				QAngle punchAngle = GetPunchAngle();
				punchAngle.x = flDamage * -0.5;
			
				if ( punchAngle.x < -12 )
					punchAngle.x = -12;
			
				punchAngle.z = flDamage * random->RandomFloat(-1,1);
			
				if ( punchAngle.z < -9 )
					punchAngle.z = -9;
			
				else if ( punchAngle.z > 9 )
					punchAngle.z = 9;
			
				SetPunchAngle( punchAngle );
			}
			break;

		case HITGROUP_CHEST:

			if ( m_iKevlar != 0 )
				 bShouldBleed = false;

			flDamage *= 1;

			if ( bShouldBleed == true )
			{
				QAngle punchAngle = GetPunchAngle();
				punchAngle.x = flDamage * -0.1;
			
				if ( punchAngle.x < -4 )
					punchAngle.x = -4;

				SetPunchAngle( punchAngle );
			}
			break;

		case HITGROUP_STOMACH:

			if ( m_iKevlar != 0 )
				 bShouldBleed = false;

			flDamage *= 1.25;

			if ( bShouldBleed == true )
			{
				QAngle punchAngle = GetPunchAngle();
				punchAngle.x = flDamage * -0.1;
			
				if ( punchAngle.x < -4 )
					punchAngle.x = -4;

				SetPunchAngle( punchAngle );
			}

			break;

		case HITGROUP_LEFTARM:

			if ( m_iKevlar != 0 )
				 bShouldBleed = false;

			break;
		case HITGROUP_RIGHTARM:
			if ( m_iKevlar != 0 )
				 bShouldBleed = false;
			break;

		case HITGROUP_LEFTLEG:
			flDamage *= 0.75;
			break;

		case HITGROUP_RIGHTLEG:
			flDamage *= 0.75;
			break;

		default:
			break;
		}
	}

	if ( bShouldBleed == true )
	{
		UTIL_BloodDecalTrace( ptr, BLOOD_COLOR_RED );
		SpawnBlood( ptr->endpos, BloodColor(), info.GetDamage() ); // a little surface blood.
		TraceBleed( info.GetDamage(), vecDir, ptr, info.GetDamageType() );
	}
	else
	{
		if ( ptr->hitgroup == HITGROUP_HEAD && bShouldSpark == true ) // they hit a helmet
		{
			Vector dir = ptr->plane.normal;
			g_pEffects->Sparks( ptr->endpos, 1, 1, &dir );
		}
	}

	CTakeDamageInfo subInfo = info;
	subInfo.SetDamage( flDamage );
	AddMultiDamage( subInfo, this );
}


void CCSPlayer::Reset()
{
	ResetFragCount();
	ResetDeathCount();
	m_iAccount = 0;
	AddAccount( -16000, false );
	m_bNotKilled = false;

	//RemoveShield();

	AddAccount( startmoney.GetInt(), true );
}


void CCSPlayer::AddAccount( int amount, bool bTrackChange )
{
	m_iAccount += amount;

	if ( m_iAccount < 0 )
		m_iAccount = 0;
	else if ( m_iAccount > 16000 )
		m_iAccount = 16000;

	// MIKETODO: the client can detect the change in a recv proxy if it wants.
	/*
	// Send money update to HUD
	UserMessageBegin (MSG_ONE, "Money" );
		WRITE_LONG( (int32)m_iAccount );
		WRITE_BYTE( bTrackChange );
	MessageEnd();
	*/
}


void CCSPlayer::BlinkAccount( int nBlinks )
{
}


CCSPlayer* CCSPlayer::Instance( int iEnt )
{
	return dynamic_cast< CCSPlayer* >( CBaseEntity::Instance( INDEXENT( iEnt ) ) );
}


void CCSPlayer::DropC4()
{
}


bool CCSPlayer::HasDefuser()
{
	//MIKETODO: defuser
	return false;
}


void CCSPlayer::RoundRespawn()
{
	// teamkill punishment.. 
	if ( (m_bJustKilledTeammate == true) && tkpunish.GetInt() )
	{
		m_bJustKilledTeammate = false;
		ClientKill( pev );
		m_bPunishedForTK = true;
	}

	//MIKETODO: menus
	//if ( m_iMenu != Menu_ChooseAppearance )
	{
		// Put them back into the game.
		State_Transition( STATE_JOINED );
		respawn( this, false );
		m_nButtons = 0;
		SetNextThink( TICK_NEVER_THINK );
		StopObserverMode();
	}
}


void CCSPlayer::Radio( const char *msg_id, const char *msg_verbose )
{
	// Spectators don't say radio messages.
	if ( IsObserver() )
		return;

	// Neither do dead guys.
	if ( m_lifeState == LIFE_DEAD )
		return;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		bool bSend = false;

		CCSPlayer *pPlayer = CCSPlayer::Instance( i );
		if ( pPlayer && !FNullEnt( pPlayer->pev ) )
		{
			// are we a regular player? (not spectator)
			if ( !pPlayer->IsObserver() )
			{
				// is this player on our team? (even dead players hear our radio calls)
				if ( pPlayer->GetTeamNumber() == GetTeamNumber() )
					bSend = true;
			}
			// this means we're a spectator
			else
			{
				//MIKETODO: do this when spectator mode is in
				/*
				if ( ( pPlayer->pev->iuser1 == OBS_CHASE_LOCKED || pPlayer->pev->iuser1 == OBS_CHASE_FREE || pPlayer->pev->iuser1 == OBS_IN_EYE ) ) 
				{
					if ( pPlayer->m_hObserverTarget )
					{
						CBasePlayer *pTarget = (CBasePlayer *)CBasePlayer::Instance( pPlayer->m_hObserverTarget->pev );

						if ( pTarget && pTarget->m_iTeam == m_iTeam )
						{
							bSend = true;
						}
					}
				}
				*/
			}

			if ( bSend )
			{
				//MIKETODO: ignorerad command
				//if ( !pPlayer->m_bIgnoreRadio )
				{
					CPASAttenuationFilter filter( this );
					EmitSound( filter, entindex(), msg_id );
					
					//MIKETODO: radio message icon
					/*
					if ( msg_verbose != NULL )
					{
						ClientPrint( pEntity->pev, HUD_PRINTTALK, "#Game_radio", STRING( pev->netname ), msg_verbose );

						// put an icon over this guys head to show that he used the radio
						MessageBegin( MSG_ONE, SVC_TEMPENTITY, NULL, pEntity->pev );
							WRITE_BYTE( TE_PLAYERATTACHMENT );
							WRITE_BYTE( ENTINDEX( edict() ) );		// byte	(entity index of player)
							WRITE_COORD( 35 );						// coord (vertical offset) ( attachment origin.z = player origin.z + vertical offset)
							WRITE_SHORT( g_sModelIndexRadio );		// short (model index) of tempent
							WRITE_SHORT( 15 );						// short (life * 10 ) e.g. 40 = 4 seconds
						MessageEnd();
					}
					*/
				}
			}
		}
	}
}

//=========================================================
// ResetMaxSpeed
//
// Reset this player's max speed via the active item
//=========================================================
void CCSPlayer::ResetMaxSpeed()
{
	float speed;

	CWeaponCSBase *pWeapon = GetActiveCSWeapon();
	if ( IsObserver() )
	{
		// Player gets speed bonus in observer mode
		speed = 900;
	}
	else if ( CSGameRules()->IsFreezePeriod() )
	{
		// Player should not move during the freeze period
		speed = 1;
	}
	else if ( m_bIsVIP == true )  // VIP is slow due to the armour he's wearing
	{
		speed = 227;
	}
	else if ( pWeapon )
	{
		// Get player speed from selected weapon
		speed = pWeapon->GetMaxSpeed();
	}
	else
	{
		// No active item, set the player's speed to default
		speed = 240;
	}

	SetMaxSpeed( speed );
}

CWeaponCSBase* CCSPlayer::GetActiveCSWeapon()
{
	return dynamic_cast< CWeaponCSBase* >( GetActiveWeapon() );
}


void CCSPlayer::SwitchTeam()
{
	if ( GetTeamNumber() == TEAM_CT )
	{
		ChangeTeam( TEAM_TERRORIST );
	}
	else if ( GetTeamNumber() == TEAM_TERRORIST )
	{
		ChangeTeam( TEAM_CT );
	}
}


void CCSPlayer::PreThink()
{
	BaseClass::PreThink();

	if ( g_fGameOver )
		return;

	ResetMaxSpeed();
	State_PreThink();
}


void CCSPlayer::MoveToNextIntroCamera()
{
	m_pIntroCamera = gEntList.FindEntityByClassname(NULL, "trigger_camera");

	// find the target
	CBaseEntity *Target = NULL;
	if(m_pIntroCamera)
		Target = gEntList.FindEntityByTarget( NULL, STRING(m_pIntroCamera->m_target) );

	if( !m_pIntroCamera || !Target ) //if there are no cameras(or the camera has no target, find a spawn point and black out the screen
	{
		CSGameRules()->GetPlayerSpawnSpot( this );
		SetAbsAngles( QAngle( 0, 0, 0 ) );
	}
	else
	{
		Vector vCamera = Target->GetAbsOrigin() - m_pIntroCamera->GetAbsOrigin();
		VectorNormalize( vCamera );
		
		QAngle CamAngles;
		VectorAngles( vCamera, CamAngles );
		CamAngles.x = -CamAngles.x;

		SetAbsOrigin( m_pIntroCamera->GetAbsOrigin() );
		SetAbsAngles( CamAngles );
		m_fIntroCamTime = gpGlobals->curtime + 6;
	}
}


void CCSPlayer::ShowVGUIMenu( int MenuType )
{
	CSingleUserRecipientFilter filter( this );

	UserMessageBegin( filter, "VGUIMenu" );
		WRITE_BYTE( MenuType );
	MessageEnd();
}


void CCSPlayer::CSWeaponDrop( CBaseCombatWeapon *pWeapon )
{
	if ( pWeapon )
	{
		Vector vForward;
		GetVectors( &vForward, NULL, NULL );
		Vector vTossPos = WorldSpaceCenter() + vForward * 150;

		Weapon_Drop( pWeapon, &vTossPos, NULL );
		pWeapon->AddSolidFlags( FSOLID_NOT_SOLID );
	}
}


void CCSPlayer::DropRifle()
{
	CBaseCombatWeapon *pWeapon = Weapon_GetSlot( WEAPON_SLOT_RIFLE );
	if ( pWeapon )
		CSWeaponDrop( pWeapon );
}


void CCSPlayer::DropPistol()
{
	CBaseCombatWeapon *pWeapon = Weapon_GetSlot( WEAPON_SLOT_PISTOL );
	if ( pWeapon )
		CSWeaponDrop( pWeapon );
}


bool CCSPlayer::IsInBuyZone()
{
	return m_bInBuyZone;
}


bool CCSPlayer::CanPlayerBuy( bool display )
{
	// is the player alive?
	if ( m_lifeState != LIFE_ALIVE )
	{
		return false;
	}

	// is the player in a buy zone?
	if ( !IsInBuyZone() )
	{
		return false;
	}

	CCSGameRules* mp = CSGameRules();

	int buyTime = (int)(buytime.GetFloat() * 60);

	if ( mp->GetRoundElapsedTime() > buyTime )
	{
		if ( display == true )
		{
			char strBuyTime[16];
			Q_snprintf( strBuyTime, sizeof( strBuyTime ), "%d", buyTime );
			ClientPrint( this, HUD_PRINTCENTER, "#Cant_buy", strBuyTime );
		}

		return false;
	}

	if ( m_bIsVIP )
	{
		if ( display == true )
			ClientPrint( this, HUD_PRINTCENTER, "#VIP_cant_buy" );

		return false;
	}

	if ( ( mp->m_bCTCantBuy == TRUE ) && ( GetTeamNumber() == TEAM_CT ) )
	{
		if ( display == true )
			ClientPrint( this, HUD_PRINTCENTER, "#CT_cant_buy" );

		return false;
	}

	if ( ( mp->m_bTCantBuy == TRUE ) && ( GetTeamNumber() == TEAM_TERRORIST ) )
	{
		if ( display == true )
			ClientPrint( this, HUD_PRINTCENTER, "#Terrorist_cant_buy" );

		return false;
	}

	return true;
}


void CCSPlayer::AttemptToBuyVest()
{
	if ( ArmorValue() >= 100 )
	{
		ClientPrint( this, HUD_PRINTCENTER, "#Already_Have_Kevlar" );
	}
	else if ( m_iAccount < KEVLAR_PRICE )
	{
		ClientPrint( this, HUD_PRINTCENTER, "#Not_Enough_Money" );
		BlinkAccount( 2 );
	}
	else
	{
		if ( m_iKevlar == 2 ) // m_iKevlar == 2 means the player has a helmet
		{
			ClientPrint( this, HUD_PRINTCENTER, "#Already_Have_Helmet_Bought_Kevlar" );
		}

		GiveNamedItem( "item_kevlar" );
		AddAccount( -KEVLAR_PRICE ); 
	}
}


void CCSPlayer::AttemptToBuyAssaultSuit()
{
	int fullArmor = ArmorValue() >= 100 ? 1 : 0;
	int helmet    = m_iKevlar == 2 ? 1 : 0;

	int price = 0, enoughMoney = 0;
	
	if ( fullArmor && helmet )
	{
		ClientPrint( this, HUD_PRINTCENTER, "#Already_Have_Kevlar_Helmet" );
		return;
	}
	else if ( fullArmor && !helmet && m_iAccount >= HELMET_PRICE )
	{
		enoughMoney = 1;
		price = HELMET_PRICE;
		ClientPrint( this, HUD_PRINTCENTER, "#Already_Have_Kevlar_Bought_Helmet" );
	}
	else if ( !fullArmor && helmet && m_iAccount >= KEVLAR_PRICE )
	{
		enoughMoney = 1;
		price = KEVLAR_PRICE;
		ClientPrint( this, HUD_PRINTCENTER, "#Already_Have_Helmet_Bought_Kevlar" );
	}
	else if ( m_iAccount >= ASSAULTSUIT_PRICE )
	{			
		enoughMoney = 1;
		price = ASSAULTSUIT_PRICE;
	}

	// process the result
	if ( !enoughMoney )
	{			
		ClientPrint( this, HUD_PRINTCENTER, "#Not_Enough_Money" );
		BlinkAccount( 2 );
	}
	else
	{
		GiveNamedItem( "item_assaultsuit" );
		AddAccount( -price ); 
	}
}


// Handles the special "buy" alias commands we're creating to accommodate the buy
// scripts players use (now that we've rearranged the buy menus and broken the scripts)
// ** Returns TRUE if we've handled the command **
bool CCSPlayer::HandleBuyAliasCommands( const char *pszCommand )
{
	// Let them buy it if it's got a weapon data file.
	char wpnName[128];
	Q_snprintf( wpnName, sizeof( wpnName ), "weapon_%s", pszCommand );
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( wpnName );
	if ( hWpnInfo == GetInvalidWeaponInfoHandle() )
	{
		if ( Q_stricmp( pszCommand, "vest" ) == 0 )
		{
			AttemptToBuyVest();
			return true;
		}
		else if ( Q_stricmp( pszCommand, "vesthelm" ) == 0 )
		{
			AttemptToBuyAssaultSuit();
			return true;
		}
	
		return false;
	}
	else
	{
		// Ok, we have weapon info.
		CCSWeaponInfo *pWeaponInfo = dynamic_cast< CCSWeaponInfo* >( GetFileWeaponInfoFromHandle( hWpnInfo ) );
		if ( !pWeaponInfo )
		{
			Assert( false );
			return true;
		}

		// MIKETODO: assasination maps have a specific set of weapons that can be used in them.
		if ( pWeaponInfo->m_iTeam != TEAM_UNASSIGNED && GetTeamNumber() != pWeaponInfo->m_iTeam )
		{
			if ( pWeaponInfo->m_WrongTeamMsg[0] != 0 )
			{
				ClientPrint( this, HUD_PRINTCENTER, "#Alias_Not_Avail", pWeaponInfo->m_WrongTeamMsg );
			}
		}
		// are they allowed to buy this weapon?
		else if ( CanPlayerBuy( true ) )
		{
			// do they have enough money?
			if ( m_iAccount >= pWeaponInfo->m_iWeaponPrice ) 
			{
				if ( pWeaponInfo->m_WeaponType == WEAPONTYPE_PISTOL )
				{
					DropPistol();
				}
				else if ( pWeaponInfo->m_WeaponType == WEAPONTYPE_RIFLE )
				{
					DropRifle();
				}
				else if ( pWeaponInfo->m_WeaponType == WEAPONTYPE_GRENADE )
				{
					// Only one type of each grenade allowed.
					if ( Weapon_OwnsThisType( wpnName ) )
					{
						ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Carry_Anymore" );
					}
				}

				// Switch to the pimp new weapon we just bought.
				CBaseEntity *pItem = GiveNamedItem( wpnName );
				CBaseCombatWeapon *pWeapon = dynamic_cast< CBaseCombatWeapon* >( pItem );
				if ( pWeapon )
					Weapon_Switch( pWeapon );

				AddAccount( -pWeaponInfo->m_iWeaponPrice ); 
			}
			else
			{
				ClientPrint( this, HUD_PRINTCENTER, "#Not_Enough_Money" );
				//BlinkAccount( pPlayer, 2 );
			}
		}
		
		return true;
	}

/*
	// rifles
	if ( FStrEq( pszCommand, "galil" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_TERRORIST )
		{
			BuyRifle( pPlayer, 1 );
		}
		else // fail gracefully
		{
			pszFailItem = "#Galil";
		}
	}
	else if ( FStrEq( pszCommand, "ak47" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_TERRORIST )
		{
			BuyRifle( pPlayer, 2 );
		}
		else // fail gracefully
		{
			pszFailItem = "#AK47";
		}
	}
	else if ( FStrEq( pszCommand, "scout" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_TERRORIST )
		{
			BuyRifle( pPlayer, 3 );
		}
		else
		{
			BuyRifle( pPlayer, 2 );
		}
	}
	else if ( FStrEq( pszCommand, "sg552" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_TERRORIST )
		{
			BuyRifle( pPlayer, 4 );
		}
		else // fail gracefully
		{
			pszFailItem = "#SG552";
		}
	}
	else if ( FStrEq( pszCommand, "awp" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_TERRORIST )
		{
			BuyRifle( pPlayer, 5 );
		}
		else
		{
			BuyRifle( pPlayer, 6 );
		}
	}
	else if ( FStrEq( pszCommand, "g3sg1" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_TERRORIST )
		{
			BuyRifle( pPlayer, 6 );
		}
		else // fail gracefully
		{
			pszFailItem = "#G3SG1";
		}
	}
	else if ( FStrEq( pszCommand, "famas" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_CT )
		{
			BuyRifle( pPlayer, 1 );
		}
		else // fail gracefully
		{
			pszFailItem = "#Famas";
		}	
	}
	else if ( FStrEq( pszCommand, "m4a1" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_CT )
		{
			BuyRifle( pPlayer, 3 );
		}
		else // fail gracefully
		{
			pszFailItem = "#M4A1";
		}
	}
	else if ( FStrEq( pszCommand, "aug" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_CT )
		{
			BuyRifle( pPlayer, 4 );
		}
		else // fail gracefully
		{
			pszFailItem = "#Aug";
		}
	}
	else if ( FStrEq( pszCommand, "sg550" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_CT )
		{
			BuyRifle( pPlayer, 5 );
		}
		else // fail gracefully
		{
			pszFailItem = "#SG550";
		}
	}
	// pistols
	else if ( FStrEq( pszCommand, "glock" ) )
	{
		bRetVal = TRUE;
		BuyPistol( pPlayer, 1 );
	}
	else if ( FStrEq( pszCommand, "usp" ) )
	{
		bRetVal = TRUE;
		BuyPistol( pPlayer, 2 );
	}
	else if ( FStrEq( pszCommand, "p228" ) )
	{
		bRetVal = TRUE;
		BuyPistol( pPlayer, 3 );
	}
	else if ( FStrEq( pszCommand, "deagle" ) )
	{
		bRetVal = TRUE;
		BuyPistol( pPlayer, 4 );
	}
	else if ( FStrEq( pszCommand, "elites" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_TERRORIST )
		{
			BuyPistol( pPlayer, 5 );
		}
		else // fail gracefully
		{
			pszFailItem = "#Beretta96G";
		}
	}
	else if ( FStrEq( pszCommand, "fn57" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_CT )
		{
			BuyPistol( pPlayer, 5 );
		}
		else // fail gracefully
		{
			pszFailItem = "#FiveSeven";
		}
	}
	// shotguns
	else if ( FStrEq( pszCommand, "m3" ) )
	{
		bRetVal = TRUE;
		BuyShotgun( pPlayer, 1 );
	}
	else if ( FStrEq( pszCommand, "xm1014" ) )
	{
		bRetVal = TRUE;
		BuyShotgun( pPlayer, 2 );
	}
	// submachine guns
	else if ( FStrEq( pszCommand, "mac10" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_TERRORIST )
		{
			BuySubMachineGun( pPlayer, 1 );
		}
		else // fail gracefully
		{
			pszFailItem = "#Mac10";
		}
	}
	else if ( FStrEq( pszCommand, "tmp" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_CT )
		{
			BuySubMachineGun( pPlayer, 1 );
		}
		else // fail gracefully
		{
			pszFailItem = "#tmp";
		}
	}
	else if ( FStrEq( pszCommand, "mp5" ) )
	{
		bRetVal = TRUE;
		BuySubMachineGun( pPlayer, 2 );
	}
	else if ( FStrEq( pszCommand, "ump45" ) )
	{
		bRetVal = TRUE;
		BuySubMachineGun( pPlayer, 3 );
	}
	else if ( FStrEq( pszCommand, "p90" ) )
	{
		bRetVal = TRUE;
		BuySubMachineGun( pPlayer, 4 );
	}
	// machinegun
	else if ( FStrEq( pszCommand, "m249" ) )
	{
		bRetVal = TRUE;
		BuyMachineGun( pPlayer, 1 );
	}
	// primary ammo
	else if ( FStrEq( pszCommand, "primammo" ) )
	{
		bRetVal = TRUE;
		// Buy as much primary ammo as possible
		// Blink money only if player doesn't have enough for the
		// first clip
		if ( BuyAmmo( pPlayer, 1, TRUE ) )
		{
			while ( BuyAmmo( pPlayer, 1, FALSE ) );
		}
	}
	// secondary ammo
	else if ( FStrEq( pszCommand, "secammo" ) )
	{
		bRetVal = TRUE;
		// Buy as much secondary ammo as possible
		// Blink money only if player doesn't have enough for the
		// first clip
		if ( BuyAmmo( pPlayer, 2, TRUE ) )
		{
			while ( BuyAmmo( pPlayer, 2, FALSE ) );
		}
	}
	// equipment
	else if ( FStrEq( pszCommand, "vest" ) )
	{
		bRetVal = TRUE;
		BuyItem( pPlayer, 1 );
	}
	else if ( FStrEq( pszCommand, "vesthelm" ) )
	{
		bRetVal = TRUE;
		BuyItem( pPlayer, 2 );	
	}
	else if ( FStrEq( pszCommand, "flash" ) )
	{
		bRetVal = TRUE;
		BuyItem( pPlayer, 3 );
	}
	else if ( FStrEq( pszCommand, "hegren" ) )
	{
		bRetVal = TRUE;
		BuyItem( pPlayer, 4 );
	}
	else if ( FStrEq( pszCommand, "sgren" ) )
	{
		bRetVal = TRUE;
		BuyItem( pPlayer, 5 );
	}
	else if ( FStrEq( pszCommand, "nvgs" ) )
	{
		bRetVal = TRUE;
		BuyItem( pPlayer, 6 );
	}
	else if ( FStrEq( pszCommand, "defuser" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_CT )
		{
			BuyItem( pPlayer, 7 );
		}
		else // fail gracefully
		{
			pszFailItem = "#Bomb_Defusal_Kit";
		}
	}
	else if ( FStrEq( pszCommand, "shield" ) )
	{
		bRetVal = TRUE;
		if ( pPlayer->GetTeamNumber() == TEAM_CT )
		{
			BuyItem( pPlayer, 8 );
		}
		else // fail gracefully
		{
			pszFailItem = "#TactShield_Desc";
		}
	}
*/
}


void CCSPlayer::HandleCommand_JoinTeam( int slot )
{
	if ( !HandleMenu_ChooseTeam( slot ) )
	{
		// Re-display the menu
		ShowVGUIMenu( MENU_TEAM );
	}
	else
	{
		if ( GetTeamNumber() == TEAM_CT )
			ShowVGUIMenu( MENU_CT );
		else if ( GetTeamNumber() == TEAM_TERRORIST )
			ShowVGUIMenu( MENU_TERRORIST );
	}
}


bool CCSPlayer::BuyGunAmmo( CBaseCombatWeapon *pWeapon, bool bBlinkMoney )
{
	if ( !CanPlayerBuy( false ) )
	{
		return false;
	}

	// Ensure that the weapon uses ammo
	int nAmmo = pWeapon->GetPrimaryAmmoType();
	if ( nAmmo == -1 )
	{
		return false;
	}

	// Can only buy if the player does not already have full ammo
	if ( GetAmmoCount( nAmmo ) >= GetAmmoDef()->MaxCarry( nAmmo ) )
	{
		return false;
	}

	//MIKETODO: put this in an ammo definition file.
	struct
	{
		const char *pAmmoName;
		int cost;
		int nBullets;
	} costs[] =
	{
		{ BULLET_PLAYER_50AE, 40, 7 },
		{ BULLET_PLAYER_762MM, 80, 30 },
		{ BULLET_PLAYER_556MM, 60, 30 },
		{ BULLET_PLAYER_338MAG, 125, 10 },
		{ BULLET_PLAYER_9MM, 20, 30 },
		{ BULLET_PLAYER_BUCKSHOT, 65, 8 },
		{ BULLET_PLAYER_45ACP, 25, 12 },
		{ BULLET_PLAYER_357SIG, 50, 13 },
		{ BULLET_PLAYER_57MM, 50, 50 }
	};

	int nCosts = sizeof( costs ) / sizeof( costs[0] );
	int iCost;
	for ( iCost=0; iCost < nCosts; iCost++ )
	{
		if ( IsAmmoType( nAmmo, costs[iCost].pAmmoName ) )
			break;
	}
	if ( iCost == nCosts )
	{
		Warning( "invalid ammo type\n" );
		return false;
	}

	// Purchase the ammo if the player has enough money
	if ( m_iAccount >= costs[iCost].cost )
	{
		GiveAmmo( costs[iCost].nBullets, nAmmo );
		AddAccount( -costs[iCost].cost );
		return true;
	}

	if ( bBlinkMoney )
	{
		// Not enough money.. let the player know
		ClientPrint( this, HUD_PRINTCENTER, "#Not_Enough_Money" );
		//BlinkAccount( &player, 2 ); 
	}

	return false;
}


bool CCSPlayer::BuyAmmo( int nSlot, bool bBlinkMoney )
{
	if ( !CanPlayerBuy( false ) )
	{
		return false;
	}

	if ( nSlot < 0 || nSlot > 1 )
	{
		return false;
	}

	// Buy one ammo clip for all weapons in the given slot
	//
	//  nSlot == 1 : Primary weapons
	//  nSlot == 2 : Secondary weapons
	
	CBaseCombatWeapon *pSlot = Weapon_GetSlot( nSlot );
	if ( !pSlot )
		return false;
	
	//MIKETODO: shield.
	//if ( player->HasShield() && player->m_rgpPlayerItems[2] )
	//	 pItem = player->m_rgpPlayerItems[2];

	if ( !BuyGunAmmo( pSlot, bBlinkMoney ) )
		return false;

	return true;
}


bool CCSPlayer::ClientCommand( const char *pcmd )
{
	if ( FStrEq( pcmd, "jointeam" ) ) 
	{
		if ( engine->Cmd_Argc() < 2 )
		{
			Warning( "Player sent bad jointeam syntax\n" );
		}

		int slot = atoi( engine->Cmd_Argv(1) );
		HandleCommand_JoinTeam( slot );
		return true;
	}
	else if ( FStrEq( pcmd, "spectate" ) ) // overrides BasePlayer command
	{
		HandleMenu_ChooseTeam( 6 );	// short cut
		return true;
	}
	else if ( FStrEq( pcmd, "joinclass" ) ) 
	{
		if ( engine->Cmd_Argc() < 2 )
		{
			Warning( "Player sent bad joinclass syntax\n" );
		}

		int slot = atoi( engine->Cmd_Argv(1) );
		HandleMenu_ChooseClass( slot );
		return true;
	}
	else if ( FStrEq( pcmd, "primammo" ) || FStrEq( pcmd, "buyammo1" ) )
	{
		if ( BuyAmmo( 0, true ) )
		{
			while ( BuyAmmo( 0, false ) );
		}
		return true;
	}
	else if ( FStrEq( pcmd, "secammo" ) || FStrEq( pcmd, "buyammo2" ) )
	{
		if ( BuyAmmo( 1, true ) )
		{
			while ( BuyAmmo( 1, false ) );
		}
		return true;
	}
	else if ( FStrEq( pcmd, "drop" ) )
	{
		if ( GetActiveWeapon() )
			CSWeaponDrop( GetActiveWeapon() );

		return true;
	}
	else if ( HandleBuyAliasCommands( pcmd ) )
	{
		return true;
	}

	return BaseClass::ClientCommand( pcmd );
}


// returns true if the selection has been handled and the player's menu 
// can be closed...false if the menu should be displayed again
bool CCSPlayer::HandleMenu_ChooseTeam( int slot )
{
	CCSGameRules *mp = CSGameRules();

	// If this player is a VIP, don't allow him to switch teams/appearances unless the following conditions are met :
	// a) There is another TEAM_CT player who is in the queue to be a VIP
	// b) This player is dead

	//MIKETODO: handle this when doing VIP mode
	/*
	if ( m_bIsVIP == true )
	{
		if ( !IsDead() )
		{	
			ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Switch_From_VIP" );	
			MenuReset();
			return true;
		}
		else if ( mp->IsVIPQueueEmpty() == true )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Switch_From_VIP" );
			MenuReset();
			return true;
		}			
	}
	*/

	int team = TEAM_UNASSIGNED;
	switch ( slot )
	{
	case 1:
		team = TEAM_TERRORIST;
		
		// Don't switch to a team we're already on..
		if ( team == GetTeamNumber() )
			return false;

		break;

	case 2:
		team = TEAM_CT;

		// Don't switch to a team we're already on..
		if ( team == GetTeamNumber() )
			return false;

		break;

	//MIKETODO: VIP mode
	/*
	case 3:
		if ( ( mp->m_iMapHasVIPSafetyZone == 1 ) && ( m_iTeam == TEAM_CT ) )
		{
			mp->AddToVIPQueue( player );
			MenuReset();
			return true;
		}
		else
		{
			return false;
		}
		break;
	*/

	case 5:
		// Attempt to auto-select a team
		team = SelectDefaultTeam();
		if ( team == TEAM_UNASSIGNED )
		{
			// kick a bot to allow human to join
			//MIKETODO: bots
			/*
			if (cv_bot_auto_vacate.value > 0 && !IsBot())
			{
				team = (RANDOM_LONG( 0, 1 ) == 0) ? TEAM_TERRORIST : TEAM_CT;
				if (UTIL_KickBotFromTeam( team ) == false)
				{
					// no bots on that team, try the other
					team = (team == TEAM_CT) ? TEAM_TERRORIST : TEAM_CT;
					if (UTIL_KickBotFromTeam( team ) == false)
					{
						// couldn't kick any bots, fail
						team = TEAM_UNASSIGNED;
					}
				}
			}
			*/
			
			if (team == TEAM_UNASSIGNED)
			{
				ClientPrint( this, HUD_PRINTCENTER, "#All_Teams_Full" );
				return false;
			}
		}
		break;

	case 6:

		// Prevent this is the cvar is set
		
		//MIKETODO: spectator proxy
		//if ( allow_spectators.GetInt() || ( GetFlags() & FL_PROXY ) )
		if ( allow_spectators.GetInt() )
		{
			// are we already a spectator?
			if ( GetTeamNumber() == TEAM_SPECTATOR )
			{
				ClientPrint( this, HUD_PRINTCENTER, "#Already_Is_Spectator" );
				return true;
			}

			//Only spectate if we are in the freeze period or dead.
			//This is done here just in case.
			if ( mp->IsFreezePeriod() || IsDead() )
			{
				if ( GetTeamNumber() != TEAM_UNASSIGNED && !IsDead() )
				{
					ClientKill( edict() );

					// add 1 to frags to balance out the 1 subtracted for killing yourself
					IncrementFragCount( 1 );
				}

				RemoveAllItems( true );

				ChangeTeam( TEAM_SPECTATOR );

				// Reset money
				m_iAccount = 0;
			
				// MIKETODO
				CBaseEntity *pentSpawnSpot = mp->GetPlayerSpawnSpot( this );
				StartObserverMode( pentSpawnSpot->GetAbsOrigin(), pentSpawnSpot->GetAbsAngles() );

				// do we have fadetoblack on? (need to fade their screen back in)
				if ( fadetoblack.GetInt() )
				{
					color32_s clr = { 0,0,0,0 };
					UTIL_ScreenFade( this, clr, 0.001, 0, FFADE_IN );
				}

				return true;
			}
			else
			{
				ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
				return false;
			}
		}
		else
		{
			ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
			return false;
		}
			
		break;


	default:
		return false;
	}
	

	// If the code gets this far, the team is not TEAM_UNASSIGNED


	// Player is switching to a new team (It is possible to switch to the
	// same team just to choose a new appearance)

	//MIKETODO: bots
	/*
	if ( mp->TeamFull( team ) )
	{
		// The specified team is full

		// attempt to kick a bot to make room for this player
		bool madeRoom = false;
		if (cv_bot_auto_vacate.value > 0 && !IsBot())
		{
			if (UTIL_KickBotFromTeam( team ))
				madeRoom = true;
		}

		if (!madeRoom)
		{
			ClientPrint( 
				pev,
				HUD_PRINTCENTER,
				( team == TEAM_TERRORIST ) ? "#Terrorists_Full" : "#CTs_Full" );

			return false;
		}
	}
	*/

	if (mp->TeamStacked( team, GetTeamNumber() ))//players are allowed to change to their own team so they can just change their model
	{
		// The specified team is full
		ClientPrint( 
			this,
			HUD_PRINTCENTER,
			( team == TEAM_TERRORIST ) ?	"#Too_Many_Terrorists" : "#Too_Many_CTs" );

		return false;
	}

	if ( GetTeamNumber() == TEAM_SPECTATOR && team != TEAM_SPECTATOR )
	{
		// If they're switching into spectator, setup spectator properties..
		m_bNotKilled = true;
	
		m_iAccount = startmoney.GetInt(); // all players start with "mp_startmoney" bucks
	
		AddSolidFlags( FSOLID_NOT_SOLID );
		Relink();
		SetMoveType( MOVETYPE_NOCLIP );
		m_fEffects = EF_NODRAW | EF_NOINTERP;
		m_takedamage = DAMAGE_NO;
		m_lifeState = LIFE_DEAD;
		SetLocalVelocity( vec3_origin );
		SetPunchAngle( QAngle( 0, 0, 0 ) );
				
		//m_bHasNightVision		 = false;
		//m_iHostagesKilled		 = 0;
		//m_fDeadTime				 = 0.0f;
		//has_disconnected		 = false;
		GetIntoGame();
		
		//g_engfuncs.pfnSetClientMaxspeed( ENT( pev ), 1 );
		//SET_MODEL( ENT(pev), "models/player.mdl" );
	}
	
	// Show the appropriate Choose Appearance menu
	// This must come before ClientKill() for CheckWinConditions() to function properly

	if ( !IsDead() )
	{
		ClientKill( edict() );
	}
	else
	{
		m_bTeamChanged = true;
	}

	// Switch their actual team...
	ChangeTeam( team );

	// Notify others that this player has joined a new team
	UTIL_ClientPrintAll( 
		HUD_PRINTNOTIFY,
		( team == TEAM_TERRORIST ) ?	"#Game_join_terrorist" : "#Game_join_ct", 
        ( STRING( pl.netname ) && STRING(pl.netname)[0] != 0 ) ? STRING(pl.netname) : "<unconnected>" );

	// Put up the class selection menu.
	State_Transition( STATE_PICKINGCLASS );
	return true;
}


bool CCSPlayer::HandleMenu_ChooseClass( int iClass )
{
	// Reset the player's state
	if ( State_Get() == STATE_JOINED )
	{
		CSGameRules()->CheckWinConditions();
	}
	
	Assert( iClass != CS_CLASS_NONE );
	m_iClass = iClass;
	GetIntoGame();
	return true;
}


bool CCSPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	if ( BaseClass::Weapon_Switch( pWeapon, viewmodelindex ) )
	{
		ResetMaxSpeed();
		return true;
	}
	else
	{
		return false;
	}
}


void CCSPlayer::GetIntoGame()
{
	// Set their model and if they're allowed to spawn right now, put them into the world.
	//SetPlayerModel( iClass );

	m_bNotKilled = false;
	SetFOV( 90 );
	ResetMaxSpeed();

	
	CCSGameRules *MPRules = CSGameRules();

	/* MIKETODO: hostage rescue
	if ( ( MPRules->m_bMapHasEscapeZone == true ) && ( m_iTeam == TEAM_CT ) )
	{
		m_iAccount = 0;

		CheckStartMoney();
		AddAccount( (int)startmoney.value, true );
	}
	*/

	
	//****************New Code by SupraFiend************
	if ( !MPRules->FPlayerCanRespawn( this ) )
	{
		// is joining in the middle of a round. If they are put them directly into observer mode
		//pev->deadflag		= DEAD_RESPAWNABLE;
		//pev->classname		= MAKE_STRING("player");
		//pev->flags		   &= ( FL_PROXY | FL_FAKECLIENT );	// clear flags, but keep proxy and bot flags that might already be set
		//pev->flags		   |= FL_CLIENT | FL_SPECTATOR;
		//SetThink(PlayerDeathThink);
		CBaseEntity *pentSpawnSpot = MPRules->GetPlayerSpawnSpot( this );
		StartObserverMode( pentSpawnSpot->GetAbsOrigin(), pentSpawnSpot->GetAbsAngles() );
		
		MPRules->CheckWinConditions();
	}
	else// else spawn them right in
	{
		Spawn();
		MPRules->CheckWinConditions();
		
		if( MPRules->m_fTeamCount == 0 )
		{
			//Bomb target, no bomber and no bomb lying around.
			if( MPRules->m_bMapHasBombTarget && !MPRules->IsThereABomber() && !MPRules->IsThereABomb() )
				MPRules->GiveC4(); //Checks for terrorists.
		}

		// If a new terrorist is entering the fray, then up the # of potential escapers.
		if ( GetTeamNumber() == TEAM_TERRORIST )
			MPRules->m_iNumEscapers++;

		State_Transition( STATE_JOINED );
	}
}


bool CCSPlayer::StartObserverMode( Vector vecPosition, QAngle vecViewAngle )
{
	BaseClass::StartObserverMode( vecPosition, vecViewAngle );

	State_Transition( STATE_OBSERVER_MODE );

	return true;
}


int CCSPlayer::PlayerClass() const
{
	return m_iClass;
}


bool CCSPlayer::SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot )
{
	// Find the next spawn spot.
	pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	if ( pSpot == NULL ) // skip over the null point
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

	CBaseEntity *pFirstSpot = pSpot;
	do 
	{
		if ( pSpot )
		{
			// check if pSpot is valid
			if ( IsSpawnPointValid( this, pSpot ) )
			{
				if ( pSpot->GetAbsOrigin() == Vector( 0, 0, 0 ) )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
					continue;
				}

				// if so, go to pSpot
				return true;
			}
		}
		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if ( pSpot != NULL )
	{
		CBaseEntity *ent = gEntList.FindEntityInSphere( NULL, pSpot->GetAbsOrigin(), 64 );
		while (ent)
		{
			// if ent is a client, kill em (unless they are ourselves)
			if ( ent->IsPlayer() && ent != this )
				ClientKill( ent->edict() );
			
			//check for more
			ent = gEntList.FindEntityInSphere( ent, pSpot->GetAbsOrigin(), 64 );
		}
		
		return true;
	}

	return false;
}


CBaseEntity* CCSPlayer::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot;

	/* MIKETODO: VIP
		// VIP spawn point *************
		if ( ( g_pGameRules->IsDeathmatch() ) && ( ((CBasePlayer*)pPlayer)->m_bIsVIP == TRUE) )
		{
			//ALERT (at_console,"Looking for a VIP spawn point\n");
			// Randomize the start spot
			//for ( int i = RANDOM_LONG(1,5); i > 0; i-- )
			pSpot = UTIL_FindEntityByClassname( NULL, "info_vip_start" );
			if ( !FNullEnt( pSpot ) )  // skip over the null point
				goto ReturnSpot;
			else
				goto CTSpawn;
		}
		
		//
		// the counter-terrorist spawns at "info_player_start"
		else
	*/ 
	
	if ( GetTeamNumber() == TEAM_CT )
	{
		pSpot = g_pLastCTSpawn;
		if ( SelectSpawnSpot( "info_player_start", pSpot ) )
			goto ReturnSpot;
	}

	/*********************************************************/
	// The terrorist spawn points
	else if ( GetTeamNumber() == TEAM_TERRORIST )
	{
		pSpot = g_pLastTerroristSpawn;
		if ( SelectSpawnSpot( "info_player_deathmatch", pSpot ) )
			goto ReturnSpot;
	}


	// If startspot is set, (re)spawn there.
	if ( !gpGlobals->startspot || !strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = gEntList.FindEntityByClassname(NULL, "info_player_deathmatch");
		if ( pSpot )
			goto ReturnSpot;
	}
	else
	{
		pSpot = gEntList.FindEntityByTarget( NULL, STRING(gpGlobals->startspot) );
		if ( pSpot )
			goto ReturnSpot;
	}

ReturnSpot:
	if ( !pSpot )
	{
		Warning( "PutClientInServer: no info_player_start on level" );
		return CBaseEntity::Instance( INDEXENT(0) );
	}

	if ( GetTeamNumber() == TEAM_TERRORIST )
		g_pLastTerroristSpawn = pSpot;
	else
		g_pLastCTSpawn = pSpot;

	return pSpot;
} 


void CCSPlayer::SetProgressBarTime( int barTime )
{
	// send message to player
	CSingleUserRecipientFilter filter( this );
	UserMessageBegin( filter, "BarTime" );
		WRITE_BYTE( barTime );
	MessageEnd();
}

/*
void CBasePlayer::PreThink()
{
	int buttonsChanged = (m_afButtonLast ^ pev->button);	// These buttons have changed this frame
	
	if ( buttonsChanged )	//this means the player has pressed or released a key
		m_fLastMovement = gpGlobals->time;

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed =  buttonsChanged & pev->button;		// The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button);	// The ones not down are "released"

	
	// Hint messages should be updated even if the game is over
	m_hintMessageQueue.Update( *this );


	g_pGameRules->PlayerThink( this );

	if ( g_fGameOver )
		return;         // intermission or finale

	if ( m_iJoiningState != STATE_JOINED )
		JoiningThink();

	//SupraFiend: Mission Briefing text, remove it when the player hits an important button
	if(m_bMissionBriefing == true)
	{
		if ( m_afButtonPressed & ( IN_ATTACK | IN_ATTACK2 )) 
		{
			m_afButtonPressed &= ~( IN_ATTACK | IN_ATTACK2 );
			RemoveLevelText();
			m_bMissionBriefing = false;
		}
	}


	UTIL_MakeVectors(pev->v_angle);             // is this still used?
	
	ItemPreFrame( );
	WaterMove();

	if ( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		// GOOSEMAN : Slow down the player based on the velocity modifier
		if ( m_flVelocityModifier < 1.0 )
		{
			m_flVelocityModifier += 0.01;
			pev->velocity = pev->velocity * m_flVelocityModifier;
		}
		if (m_flVelocityModifier > 1.0)
			m_flVelocityModifier = 1.0;
	}

	if (	(m_flIdleCheckTime <= gpGlobals->time) || (m_flIdleCheckTime == 0.0)	)	
	{
		m_flIdleCheckTime = gpGlobals->time + 5;	// check every 5 seconds

		//check if this player has been inactive for 2 rounds straight
		if( (gpGlobals->time - m_fLastMovement) > ((CHalfLifeMultiplay*)g_pGameRules)->m_fMaxIdlePeriod )
		{
			if ( CVAR_GET_FLOAT( "mp_autokick" ) )
			{
				// Log the kick
				UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Game_idle_kick\" (auto)\n", 
					STRING( pev->netname ),
					GETPLAYERUSERID( edict() ),
					GETPLAYERAUTHID( edict() ),
					GetTeam( m_iTeam ) );

				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#Game_idle_kick", STRING(pev->netname) );
				SERVER_COMMAND(  UTIL_VarArgs( "kick %s\n", STRING( pev->netname ) )  );
				m_fLastMovement = gpGlobals->time;
			}
		}
	}


	if ( g_pGameRules && g_pGameRules->FAllowFlashlight() )
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;


	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();
	
	CheckTimeBasedDamage();

	CheckSuitUpdate();

	// So the correct flags get sent to client asap.
	//
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
		pev->flags |= FL_ONTRAIN;
	else 
		pev->flags &= ~FL_ONTRAIN;

	// Observer Button Handling
	if ( IsObserver() &&  (m_afPhysicsFlags & PFLAG_OBSERVER) )
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		Observer_CheckProperties();
		return;
	}

	if ( pev->deadflag >= DEAD_DYING && pev->deadflag != DEAD_RESPAWNABLE )
	{
		PlayerDeathThink();
		return;
	}

	//GOOSEMAN : new code to determine if a player is on a train or not
	CBaseEntity *pGroundEntity = CBaseEntity::Instance( pev->groundentity);
	if (pGroundEntity && (pGroundEntity->Classify() == CLASS_VEHICLE))
	{
		pev->iuser4 = 1;
	}
	else
	{
		pev->iuser4 = 0;
	}


	// Train speed control
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
	{
		CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );
		float vel;
		
		if ( !pTrain )
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine( pev->origin, pev->origin + Vector(0,0,-38), ignore_monsters, ENT(pev), &trainTrace );

			// HACKHACK - Just look for the func_tracktrain classname
			if ( trainTrace.flFraction != 1.0 && trainTrace.pHit )
			pTrain = CBaseEntity::Instance( trainTrace.pHit );


			if ( !pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(pev) )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				((CFuncVehicle*)pTrain)->m_pDriver = NULL;
				return;
			}
		}
		else if ( !FBitSet( pev->flags, FL_ONGROUND ) || FBitSet( pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL ) ) //|| (pev->button & (IN_MOVELEFT|IN_MOVERIGHT) ) )
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW|TRAIN_OFF;
			((CFuncVehicle*)pTrain)->m_pDriver = NULL;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;
		if (pTrain->Classify() == CLASS_VEHICLE)
		{
			if ( pev->button & IN_FORWARD )
			{
				vel = 1;
				pTrain->Use( this, this, USE_SET, (float)vel );
			}
			if ( pev->button & IN_BACK )
			{
				vel = -1;
				pTrain->Use( this, this, USE_SET, (float)vel );
			}
			if ( pev->button & IN_MOVELEFT)
			{
				vel = 20;
				pTrain->Use( this, this, USE_SET, (float)vel );
			}
			if ( pev->button & IN_MOVERIGHT)
			{
				vel = 30;
				pTrain->Use( this, this, USE_SET, (float)vel );
			}
		}
		else
		{
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
		}

		if (vel)
		{
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE|TRAIN_NEW;
		}

	}
	else if (m_iTrain & TRAIN_ACTIVE)
		m_iTrain = TRAIN_NEW; // turn off train

	if (pev->button & IN_JUMP)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}

	// If trying to duck, already ducked, or in the process of ducking
	if ((pev->button & IN_DUCK) || FBitSet(pev->flags,FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) )
		Duck();

	if ( !FBitSet ( pev->flags, FL_ONGROUND ) )
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = NULL;

	if ( m_afPhysicsFlags & PFLAG_ONBARNACLE )
	{
		pev->velocity = g_vecZero;
	}

	if ( !( m_flDisplayHistory & DHF_ROUND_STARTED ) && (CanPlayerBuy(false) == TRUE)	)
	{
		HintMessage( "#Hint_press_buy_to_purchase", FALSE );
		m_flDisplayHistory |= DHF_ROUND_STARTED;
	}
}
*/

void CCSPlayer::PlayerDeathThink()
{
}


void CCSPlayer::State_Transition( CSPlayerState newState )
{
	State_Leave();
	State_Enter( newState );
}


void CCSPlayer::State_Enter( CSPlayerState newState )
{
	m_iPlayerState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	if ( cs_ShowStateTransitions.GetInt() == -1 || cs_ShowStateTransitions.GetInt() == entindex() )
	{
		if ( m_pCurStateInfo )
			Msg( "ShowStateTransitions: entering '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			Msg( "ShowStateTransitions: entering #%d\n", newState );
	}
	
	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
		(this->*m_pCurStateInfo->pfnEnterState)();
}


void CCSPlayer::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}


void CCSPlayer::State_PreThink()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnPreThink )
	{
		(this->*m_pCurStateInfo->pfnPreThink)();
	}
}


CCSPlayerStateInfo* CCSPlayer::State_LookupInfo( CSPlayerState state )
{
	// This table MUST match the 
	static CCSPlayerStateInfo playerStateInfos[] =
	{
		{ STATE_JOINED,			"STATE_JOINED",			&CCSPlayer::State_Enter_JOINED,		NULL,						&CCSPlayer::State_PreThink_JOINED },
		{ STATE_SHOWLTEXT,		"STATE_SHOWLTEXT",		&CCSPlayer::State_Enter_SHOWLTEXT,		NULL,						&CCSPlayer::State_PreThink_SHOWLTEXT },
		{ STATE_PICKINGTEAM,	"STATE_PICKINGTEAM",	&CCSPlayer::State_Enter_PICKINGTEAM,	&CCSPlayer::State_Leave_PICKINGTEAM,	&CCSPlayer::State_PreThink_ShowIntroCameras },
		{ STATE_PICKINGCLASS,	"STATE_PICKINGCLASS",	&CCSPlayer::State_Enter_PICKINGCLASS,	NULL,						&CCSPlayer::State_PreThink_ShowIntroCameras },
		{ STATE_DEATH_ANIM,		"STATE_DEATH_ANIM",		&CCSPlayer::State_Enter_DEATH_ANIM,	NULL,						&CCSPlayer::State_PreThink_DEATH_ANIM },
		{ STATE_DEATH_WAIT_FOR_KEY,	"STATE_DEATH_WAIT_FOR_KEY",	&CCSPlayer::State_Enter_DEATH_WAIT_FOR_KEY,	NULL,						&CCSPlayer::State_PreThink_DEATH_WAIT_FOR_KEY },
		{ STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CCSPlayer::State_Enter_OBSERVER_MODE,	NULL,						&CCSPlayer::State_PreThink_OBSERVER_MODE }
	};

	for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
	{
		if ( playerStateInfos[i].m_iPlayerState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}


void CCSPlayer::State_Enter_SHOWLTEXT()
{
	SetMoveType( MOVETYPE_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );
}


void CCSPlayer::State_PreThink_SHOWLTEXT()
{
	// Verify some state.
	Assert( GetMoveType() == MOVETYPE_NONE );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );
	Assert( GetAbsVelocity().Length() == 0 );

	// Update whatever intro camera it's at.
	State_PreThink_ShowIntroCameras();
}


void CCSPlayer::State_Enter_PICKINGTEAM()
{
}


void CCSPlayer::State_Enter_OBSERVER_MODE()
{
//    SetMoveType( MOVETYPE_FLY );	// in CS use movetype fly
	
	// Set the player's speed
	ResetMaxSpeed();
}


void CCSPlayer::State_PreThink_OBSERVER_MODE()
{
	// Make sure nobody has changed any of our state.
//	Assert( GetMoveType() == MOVETYPE_FLY );
	Assert( m_takedamage == DAMAGE_NO );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );
	Assert( (m_fEffects & EF_NODRAW) != 0 );
	
	// Must be dead.
	Assert( m_lifeState == LIFE_DEAD );
	Assert( pl.deadflag );
}


void CCSPlayer::State_Enter_DEATH_ANIM()
{
	if ( HasWeapons() )
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}

	// Used for a timer.
	m_fDeadTime = gpGlobals->curtime;
}


void CCSPlayer::State_PreThink_DEATH_ANIM()
{
	// If the anim is done playing, go to the next state (waiting for a keypress to 
	// either respawn the guy or put him into observer mode).
	if ( GetFlags() & FL_ONGROUND )
	{
		float flForward = GetAbsVelocity().Length() - 20;
		if (flForward <= 0)
		{
			SetAbsVelocity( vec3_origin );
		}
		else
		{
			Vector vAbsVel = GetAbsVelocity();
			VectorNormalize( vAbsVel );
			vAbsVel *= flForward;
			SetAbsVelocity( vAbsVel );
		}
	}

	// Finish the death animation.
	
	// MIKETODO: death anims
	// How we wait for it to end depends on how we want guys to die.
	// If they ragdoll, we can wait for the ragdoll to settle.
	// If they play an animation, we can wait for the animation to end.
	// Right now, it just uses a 1 second timer.

	//if ( !m_nModelIndex || IsActivityFinished() )
	if ( gpGlobals->curtime >= (m_fDeadTime+1) )
	{
		State_Transition( STATE_DEATH_WAIT_FOR_KEY );
	}
}


void CCSPlayer::State_Enter_DEATH_WAIT_FOR_KEY()
{
	// Remember when we died, so we can automatically put them into observer mode
	// if they don't hit a key soon enough.
	m_fDeadTime = gpGlobals->curtime;
	m_lifeState = LIFE_DEAD;
		
	StopAnimation();

	m_fEffects |= EF_NOINTERP;
}


void CCSPlayer::State_PreThink_DEATH_WAIT_FOR_KEY()
{
	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	if ( GetMoveType() != MOVETYPE_NONE && (GetFlags() & FL_ONGROUND) )
		SetMoveType( MOVETYPE_NONE );
	
	// if the player has been dead for one second longer than allowed by forcerespawn, 
	// forcerespawn isn't on. Send the player off to an intermission camera until they 
	// choose to respawn.
	if ( ( gpGlobals->curtime > (m_fDeadTime + 3) ) && 
	  !( m_afPhysicsFlags & PFLAG_OBSERVER) )
	{
		//Send message to everybody to spawn a corpse.
		//SpawnClientSideCorpse();

		// go to dead camera. 
		StartDeathCam();
		return;
	}

	// If it hasn't timed out yet, let them hit a key 
	if ( m_lifeState == LIFE_DEAD )
	{
		bool fAnyButtonDown = !!(m_nButtons & ~IN_SCORE);
		if ( fAnyButtonDown )
			return;

		if ( g_pGameRules->FPlayerCanRespawn( this ) )
		{
			m_lifeState = LIFE_RESPAWNABLE;
			CSGameRules()->CheckWinConditions();
		}
	}
}


void CCSPlayer::State_Leave_PICKINGTEAM()
{
	// If we're just entering the server and it was showing intro cameras, stop that.
	m_pIntroCamera = NULL;
}


void CCSPlayer::State_Enter_PICKINGCLASS()
{
}


void CCSPlayer::State_PreThink_ShowIntroCameras()
{
	if( m_pIntroCamera && (gpGlobals->curtime >= m_fIntroCamTime) )
	{
		MoveToNextIntroCamera();
	}
}


void CCSPlayer::State_Enter_JOINED()
{
	SetMoveType( MOVETYPE_WALK );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
    m_Local.m_iHideHUD = 0;
}


void CCSPlayer::State_PreThink_JOINED()
{
	// We only allow noclip here only because noclip is useful for debugging.
	// It would be nice if the noclip command set some flag so we could tell that they
	// did it intentionally.
	if ( IsEFlagSet( EFL_NOCLIP_ACTIVE ) )
	{
		Assert( GetMoveType() == MOVETYPE_NOCLIP );
	}
	else
	{
//		Assert( GetMoveType() == MOVETYPE_WALK );
	}

	Assert( !IsSolidFlagSet( FSOLID_NOT_SOLID ) );
}


void CCSPlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	CWeaponCSBase *pCSWeapon = dynamic_cast< CWeaponCSBase* >( pWeapon );
	if ( pCSWeapon )
	{
		// For rifles, pistols, or the knife, drop our old weapon in this slot.
		if ( pCSWeapon->GetSlot() == WEAPON_SLOT_RIFLE || 
			pCSWeapon->GetSlot() == WEAPON_SLOT_PISTOL || 
			pCSWeapon->GetSlot() == WEAPON_SLOT_KNIFE )
		{
			CBaseCombatWeapon *pDropWeapon = Weapon_GetSlot( pCSWeapon->GetSlot() );
			if ( pDropWeapon )
			{
				CSWeaponDrop( pDropWeapon );
				pDropWeapon->AddSolidFlags( FSOLID_NOT_SOLID );
			}
		}
	}

	BaseClass::Weapon_Equip( pWeapon );
}


bool CCSPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		extern int gEvilImpulse101;
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	// ----------------------------------------
	// If I already have it just take the ammo
	// ----------------------------------------
	if ( Weapon_SlotOccupied( pWeapon ) )
	{
		Weapon_EquipAmmoOnly( pWeapon );
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
		pWeapon->CheckRespawn();

		pWeapon->AddSolidFlags( FSOLID_NOT_SOLID );
		pWeapon->m_fEffects	|= EF_NODRAW;

		Weapon_Equip( pWeapon );
		return true;
	}
}
