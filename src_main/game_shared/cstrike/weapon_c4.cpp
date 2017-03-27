//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_c4.h"
#include "in_buttons.h"
#include "cs_gamerules.h"
#include "decals.h"


#if defined( CLIENT_DLL )
	#include "c_cs_player.h"
#else
	#include "cs_player.h"
	#include "explode.h"
	#include "mapinfo.h"
	#include "team.h"
#endif


#define BLINK_INTERVAL 2.0
#define PLANTED_C4_MODEL "models/battery.mdl"

int g_sModelIndexC4Glow = -1;

#define WEAPON_C4_ARM_TIME	3.0


#ifdef CLIENT_DLL

#else

	LINK_ENTITY_TO_CLASS( planted_c4, CPlantedC4 );
	PRECACHE_REGISTER( planted_c4 );

	BEGIN_DATADESC( CPlantedC4 )
		DEFINE_FUNCTION( CPlantedC4, C4Think ),
		DEFINE_FUNCTION( CPlantedC4, Detonate2 )
	END_DATADESC()

	CUtlVector< CPlantedC4* > g_PlantedC4s;


	CPlantedC4::CPlantedC4()
	{
		g_PlantedC4s.AddToTail( this );
	}


	CPlantedC4::~CPlantedC4()
	{
		g_PlantedC4s.FindAndRemove( this );
	}


	void CPlantedC4::Precache()
	{
		g_sModelIndexC4Glow = engine->PrecacheModel( "sprites/ledglow.vmt" );
		engine->PrecacheModel( PLANTED_C4_MODEL );
	}


	CPlantedC4* CPlantedC4::ShootSatchelCharge( CCSPlayer *pevOwner, Vector vecStart, QAngle vecAngles )
	{
		CPlantedC4 *pGrenade = dynamic_cast< CPlantedC4* >( CreateEntityByName( "planted_c4" ) );
		if ( pGrenade )
		{
			pGrenade->Init( pevOwner, vecStart, vecAngles );
			return pGrenade;
		}
		else
		{
			Warning( "Can't create planted_c4 entity!" );
			return NULL;
		}
	}


	void CPlantedC4::Init( CCSPlayer *pevOwner, Vector vecStart, QAngle vecAngles )
	{
		SetMoveType( MOVETYPE_NONE );
		SetSolid( SOLID_BBOX );

		SetModel( PLANTED_C4_MODEL );	// Change this to c4 model

		SetCollisionBounds( Vector( 0, 0, 0 ), Vector( 8, 8, 8 ) );

		SetAbsOrigin( vecStart );
		SetAbsAngles( vecAngles );
		SetOwnerEntity( pevOwner );
		
		// Detonate in "time" seconds
		SetThink( &CPlantedC4::C4Think );
		//pGrenade->SetTouch( &CPlantedC4::C4Touch );

		SetNextThink( gpGlobals->curtime + 0.1f );
		
		m_flC4Blow = gpGlobals->curtime + c4timer.GetInt();

		m_flNextFreqInterval = c4timer.GetInt() / 4;
		m_flNextFreq = gpGlobals->curtime;
		m_flNextBeep = gpGlobals->curtime + 0.5;
		m_iCurWave = 0;
		m_sBeepName = NULL;

		m_flNextBlink = gpGlobals->curtime + BLINK_INTERVAL;
		m_flNextDefuse = 0;

		m_bStartDefuse = false;
		m_bJustBlew = false;
		SetFriction( 0.9 );

		Relink();
	}


	void CPlantedC4::C4Think()
	{
		if (!IsInWorld())
		{
			UTIL_Remove( this );
			return;
		}
		
		SetNextThink( gpGlobals->curtime + 0.12 );
		
		if ( gpGlobals->curtime >= m_flNextFreq )
		{	
			m_flNextFreq = gpGlobals->curtime + m_flNextFreqInterval;
			m_flNextFreqInterval *= 0.9;
			m_iCurWave = clamp( m_iCurWave+1, 0, 5 );
		}

		if ( gpGlobals->curtime >= m_flNextBeep )
		{	
			m_flNextBeep = gpGlobals->curtime + 1.4;

			// Play a beep sound.		
			char soundName[64];
			Q_snprintf( soundName, sizeof( soundName ), "c4.beep%d", m_iCurWave );

			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), soundName );

			// let the bots hear the bomb beeping
			//g_pBotControl->OnEvent( EVENT_BOMB_BEEP, this );
		}

		if(gpGlobals->curtime >= m_flNextBlink)//added by SupraFiend to improve Bomb visibility
		{
			m_flNextBlink = gpGlobals->curtime + BLINK_INTERVAL;

			Vector vPos = GetAbsOrigin();
			vPos.z += 5;
			CPVSFilter filter( GetAbsOrigin() );
			te->GlowSprite( filter, 0, &vPos, g_sModelIndexC4Glow, 1, 0.5, 255 );
		}
		
		// IF the timer has expired ! blow this bomb up!
		if (m_flC4Blow <= gpGlobals->curtime)
		{
			// let the bots hear the bomb exploding
			//g_pBotControl->OnEvent( EVENT_BOMB_EXPLODED );

			// Tell the bomb target brush to trigger its targets
			//MIKETODO: use entity IO?
			/*
			if ( m_pentCurBombTarget )
			{
				CBaseEntity* pBombTarget = CBaseEntity::Instance( m_pentCurBombTarget );
				
				if ( pBombTarget )
					pBombTarget->Use( CBaseEntity::Instance( pev->owner ), this, USE_TOGGLE, 0 );
			}
			*/

			// give the defuser credit for defusing the bomb
			CBasePlayer *pBombOwner = dynamic_cast< CBasePlayer* >( GetOwnerEntity() );
			if ( pBombOwner )
			{
				pBombOwner->IncrementFragCount( 3 );
			}

			CSGameRules()->m_bBombDropped = false;

			if (GetWaterLevel() == 0)
				SetThink ( &CPlantedC4::Detonate2 );
			else
				UTIL_Remove (this); // Get rid of this thing if it's underwater..
		}

		//if the defusing process has started
		if ((m_bStartDefuse == true) && (m_pBombDefuser != NULL))
		{
			//if the defusing process has not ended yet
			if ( m_flDefuseCountDown > gpGlobals->curtime)
			{
				int iOnGround = FBitSet( m_pBombDefuser->GetFlags(), FL_ONGROUND );

				//if the bomb defuser has stopped defusing the bomb
				if( m_fNextDefuse < gpGlobals->curtime || !iOnGround )
				{
					if ( !iOnGround )
						ClientPrint( m_pBombDefuser, HUD_PRINTCENTER, "#C4_Defuse_Must_Be_On_Ground");

					// release the player from being frozen
					m_pBombDefuser->ResetMaxSpeed();
					m_pBombDefuser->m_bIsDefusing = false;

					//cancel the progress bar
					m_pBombDefuser->SetProgressBarTime( 0 );
					m_pBombDefuser = NULL;
					m_bStartDefuse = false;
					m_flDefuseCountDown = 0;

					// tell the bots someone has aborted defusing
					//g_pBotControl->OnEvent( EVENT_BOMB_DEFUSE_ABORTED );
				}

				return;
			}
			//if the defuse process has ended, kill the c4
			else if ( !m_pBombDefuser->IsDead() )
			{
				// tell the bots the bomb is defused
				//g_pBotControl->OnEvent( EVENT_BOMB_DEFUSED );

				//MIKETODO: spectator
				/*
				// Broadcast to the entire server
				Broadcast( "BOMBDEF" );

				// send director message, that something important happened here
				MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
					WRITE_BYTE ( 9 );
					WRITE_BYTE ( DRC_CMD_EVENT );	// bomb defuse
					WRITE_SHORT( ENTINDEX( m_pBombDefuser->edict() ));	// index number of secondary entity
					WRITE_SHORT( 0 );	// index number of secondary entity
					WRITE_LONG( 15 | DRC_FLAG_DRAMATIC | DRC_FLAG_FINAL | DRC_FLAG_FACEPLAYER );   // eventflags (priority and flags)
				MESSAGE_END();
				*/

				CPASAttenuationFilter filter( this );
				EmitSound( filter, entindex(), "c4.disarmed" );
				
				UTIL_Remove(this);
				
				// The bomb has just been disarmed.. Check to see if the round should end now
				m_bJustBlew = true;

				// release the player from being frozen
				m_pBombDefuser->ResetMaxSpeed();
				m_pBombDefuser->m_bIsDefusing = false;

				CSGameRules()->m_bBombDefused = true;
				CSGameRules()->CheckWinConditions();

				// give the defuser credit for defusing the bomb
				m_pBombDefuser->IncrementFragCount( 3 );

				CSGameRules()->m_bBombDropped = false;
				
				// Clear their progress bar.
				m_pBombDefuser->SetProgressBarTime( 0 );

				m_pBombDefuser = NULL;
				m_bStartDefuse = false;

				return;
			}

			//if it gets here then the previouse defuser has taken off or been killed
			// release the player from being frozen
			m_pBombDefuser->ResetMaxSpeed();
			m_pBombDefuser->m_bIsDefusing = false;
			m_bStartDefuse = false;
			m_pBombDefuser = NULL;

			// tell the bots someone has aborted defusing
			//g_pBotControl->OnEvent( EVENT_BOMB_DEFUSE_ABORTED );
		}
	}


	void CPlantedC4::Detonate2()
	{
		trace_t tr;
		Vector		vecSpot;// trace starts here!

		vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
		UTIL_TraceLine( vecSpot, vecSpot + Vector ( 0, 0, -40 ), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

		Explode2( &tr, DMG_BLAST );
	}


	// Regular explosions
	void CPlantedC4::Explode2( trace_t *pTrace, int bitsDamageType )
	{
		float		flRndSound;// sound randomizer
		CCSGameRules *mp = CSGameRules();

		SetSolidFlags( FSOLID_NOT_SOLID );

		m_takedamage = DAMAGE_NO;

		// Shake the ground!!
		//UTIL_ScreenShake( pTrace->endpos, 25.0, 150.0, 1.0, 3000 );

		mp->m_bTargetBombed = true;

		// This variable is checked by the function CheckWinConditions()  to see if the bomb has just detonated.
		m_bJustBlew = true;
		// Check to see if the round is over after the bomb went off...
		mp->CheckWinConditions();


		// Pull out of the wall a bit
		if ( pTrace->fraction != 1.0 )
		{
			SetAbsOrigin( pTrace->endpos + (pTrace->plane.normal * 0.6) );
		}

		// Fireball sprite and sound!!
		{
			Vector fireballPos = GetAbsOrigin();
			CPVSFilter filter( fireballPos );
			te->Sprite( filter, 0, &fireballPos, g_sModelIndexFireball, 100, 150 );
		}

		{
			Vector fireballPos = GetAbsOrigin() + Vector( 
				random->RandomFloat( -512, 512 ), 
				random->RandomFloat( -512, 512 ),
				random->RandomFloat( -10, 10 ) );

			CPVSFilter filter( fireballPos );
			te->Sprite( filter, 0, &fireballPos, g_sModelIndexFireball, 100, 150 );
		}

		{
			Vector fireballPos = GetAbsOrigin() + Vector( 
				random->RandomFloat( -512, 512 ), 
				random->RandomFloat( -512, 512 ),
				random->RandomFloat( -10, 10 ) );

			CPVSFilter filter( fireballPos );
			te->Sprite( filter, 0, &fireballPos, g_sModelIndexFireball, 100, 150 );
		}

		// Emit a sound effect of the explosion that travels across the entire level!
		CPASAttenuationFilter filter( this );
		EmitSound( filter, entindex(), "c4.explode" );
		
		//EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/c4_explode1.wav", 1.0, 0.25);
		
		SetOwnerEntity( NULL ); // can't traceline attack owner if this is set

		float flBombRadius = 100;
		if ( g_pMapInfo )
			flBombRadius = g_pMapInfo->m_flBombRadius;
		CSGameRules()->RadiusDamage( 
			CTakeDamageInfo( this, GetOwnerEntity(), 1000, bitsDamageType ), GetAbsOrigin(), flBombRadius, CLASS_NONE );

		// send director message, that something important happed here
		/*
		MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
			WRITE_BYTE ( 9 );	// command length in bytes
			WRITE_BYTE ( DRC_CMD_EVENT );	// bomb explode
			WRITE_SHORT( ENTINDEX(this->edict()) );	// index number of primary entity
			WRITE_SHORT( 0 );	// index number of secondary entity
			WRITE_LONG( 15 | DRC_FLAG_FINAL );   // eventflags (priority and flags)
		MESSAGE_END();
		*/

		UTIL_DecalTrace( pTrace, "Scorch" );

		flRndSound = random->RandomFloat( 0 , 1 );


		CPASAttenuationFilter debrisFilter( this );
		EmitSound( debrisFilter, entindex(), "c4.debris" );

		m_fEffects |= EF_NODRAW;
		
		//SetThink( &CPlantedC4::Smoke2 );
		//SetNextThink( gpGlobals->curtime + 0.85 );
	}

	
	// For CTs to defuse the c4
	void CPlantedC4::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CCSPlayer *player = dynamic_cast< CCSPlayer* >( pActivator );

		if ( !player || player->GetTeamNumber() != TEAM_CT)
			return;

		if ( m_bStartDefuse )
		{
			m_fNextDefuse = gpGlobals->curtime + 0.5;
		}
		else
		{
			// freeze the player in place while diffusing
			player->SetMaxSpeed( 1 );

			// tell the bots someone has started defusing
			//g_pBotControl->OnEvent( EVENT_BOMB_DEFUSING, pActivator );

			int defuseTime;
			if ( player->HasDefuser() )
			{
				// Log this information
				//UTIL_LogPrintf("\"%s<%i><%s><CT>\" triggered \"Begin_Bomb_Defuse_With_Kit\"\n", 
				//	STRING( player->pev->netname ),
				//	GETPLAYERUSERID( player->edict() ),
				//	GETPLAYERAUTHID( player->edict() ) );

				ClientPrint( player, HUD_PRINTCENTER, "#Defusing_Bomb_With_Defuse_Kit" );
				defuseTime = 5;
			}
			else
			{
				// Log this information
				//UTIL_LogPrintf("\"%s<%i><%s><CT>\" triggered \"Begin_Bomb_Defuse_Without_Kit\"\n", 
				//	STRING( player->pev->netname ),
				//	GETPLAYERUSERID( player->edict() ),
				//	GETPLAYERAUTHID( player->edict() ) );

				ClientPrint( player, HUD_PRINTCENTER, "#Defusing_Bomb_Without_Defuse_Kit" );
				defuseTime = 10;
			}

			CPASAttenuationFilter filter( player );
			EmitSound( filter, entindex(), "c4.disarm" );

			m_fNextDefuse = gpGlobals->curtime + 0.5;
			m_pBombDefuser = player;
			m_bStartDefuse = TRUE;
			player->m_bIsDefusing = true;

			m_flDefuseCountDown = gpGlobals->curtime + defuseTime;
			
			//start the progress bar
			player->SetProgressBarTime( defuseTime );
		}
	}


#endif



// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED( C4, DT_WeaponC4 )

BEGIN_NETWORK_TABLE( CC4, DT_WeaponC4 )
	#ifdef CLIENT_DLL
		RecvPropInt( RECVINFO( m_bStartedArming ) )
	#else
		SendPropInt( SENDINFO( m_bStartedArming ), 1, SPROP_UNSIGNED )
	#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CC4 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_c4, CC4 );
PRECACHE_WEAPON_REGISTER( weapon_c4 );



// -------------------------------------------------------------------------------- //
// Globals.
// -------------------------------------------------------------------------------- //

CUtlVector< CC4* > g_C4s;



// -------------------------------------------------------------------------------- //
// CC4 implementation.
// -------------------------------------------------------------------------------- //

CC4::CC4()
{
	g_C4s.AddToTail( this );
}


CC4::~CC4()
{
	g_C4s.FindAndRemove( this );
}


void CC4::ItemPostFrame()
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	// Disable all the firing code.. the C4 grenade is all custom.
	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		PrimaryAttack();
	}
	else
	{
		WeaponIdle();
		
		#ifndef CLIENT_DLL
			pPlayer->ResetMaxSpeed();
		#endif
	}
}


#ifndef CLIENT_DLL

	bool CC4::Holster( CBaseCombatWeapon *pSwitchingTo )
	{
		CCSPlayer *pPlayer = GetPlayerOwner();
		if ( pPlayer )
			pPlayer->SetProgressBarTime( 0 );
		
		return BaseClass::Holster( pSwitchingTo );
	}


	bool CC4::ShouldRemoveOnRoundRestart()
	{
		// Doesn't matter if we have an owner or not.. always remove the C4 when the round restarts.
		// The gamerules will give another C4 to some lucky player.
		return true;
	}

#endif


void CC4::PrimaryAttack()
{
	bool	PlaceBomb = false;
	CCSPlayer *pPlayer = GetPlayerOwner();


	int onGround = FBitSet( pPlayer->GetFlags(), FL_ONGROUND );

	if( m_bStartedArming == false )
	{
		if( pPlayer->m_bInBombZone && onGround )
		{
			m_bStartedArming = true;
			m_fArmedTime = gpGlobals->curtime + WEAPON_C4_ARM_TIME;
			m_bBombPlacedAnimation = false;
			//SendWeaponAnim( C4_ARM, UseDecrement() ? 1: 0 );

#if !defined( CLIENT_DLL )			
			// freeze the player in place while planting
			pPlayer->SetMaxSpeed( 1 );

			// player "arming bomb" animation
			pPlayer->SetAnimation( PLAYER_ATTACK1 );

			pPlayer->SetProgressBarTime( 3 );	
#endif
		}
		else
		{
			if ( !pPlayer->m_bInBombZone )
			{
				ClientPrint( pPlayer, HUD_PRINTCENTER, "#C4_Plant_At_Bomb_Spot");
			}
			else
			{
				ClientPrint( pPlayer, HUD_PRINTCENTER, "#C4_Plant_Must_Be_On_Ground");
			}

			m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
			return;
		}
	}
	else
	{
		if ( !onGround || !pPlayer->m_bInBombZone )
		{
			if( !pPlayer->m_bInBombZone )
			{
				ClientPrint( pPlayer, HUD_PRINTCENTER, "#C4_Arming_Cancelled" );
			}
			else
			{
				ClientPrint( pPlayer, HUD_PRINTCENTER, "#C4_Plant_Must_Be_On_Ground" );
			}

			m_flNextPrimaryAttack = gpGlobals->curtime + 1.5;
			m_bStartedArming = false;

#if !defined( CLIENT_DLL )
			// release the player from being frozen, we've somehow left the bomb zone
			pPlayer->ResetMaxSpeed();

			pPlayer->SetProgressBarTime( 0 );

			//pPlayer->SetAnimation( PLAYER_HOLDBOMB );

#endif
			/*
			if(m_bBombPlacedAnimation == true) //this means the placement animation is canceled
			{
				SendWeaponAnim( C4_DRAW, UseDecrement() ? 1: 0 );
			}
			else
			{
				SendWeaponAnim( C4_IDLE1, UseDecrement() ? 1: 0 );
			}
			*/
			
			return;
		}
		else
		{
			if( gpGlobals->curtime >= m_fArmedTime ) //the c4 is ready to be armed
			{
				//check to make sure the player is still in the bomb target area
				PlaceBomb = true;
			}
			else if( ( gpGlobals->curtime >= (m_fArmedTime - 0.75) ) && ( !m_bBombPlacedAnimation ) )
			{
				//call the c4 Placement animation 
				m_bBombPlacedAnimation = true;
				//SendWeaponAnim( C4_DROP, UseDecrement() ? 1: 0 );
				
#if !defined( CLIENT_DLL )
				// player "place" animation
				//pPlayer->SetAnimation( PLAYER_HOLDBOMB );
#endif
			}
		}
	}

	if ( PlaceBomb && m_bStartedArming )
	{
		m_bStartedArming = false;
		m_fArmedTime = 0;
		
		if( pPlayer->m_bInBombZone )
		{
#if !defined( CLIENT_DLL )
			//Broadcast("BOMBPL");

			CPlantedC4::ShootSatchelCharge( pPlayer, pPlayer->GetAbsOrigin(), QAngle(0,0,0) );

			// send director message, that something important happened here
			/*
			MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
				WRITE_BYTE ( 9 );	// command length in bytes
				WRITE_BYTE ( DRC_CMD_EVENT ); // bomb placed
				WRITE_SHORT( ENTINDEX(pPlayer->edict()) );
				WRITE_SHORT( 0 );
				WRITE_LONG( 11 | DRC_FLAG_FACEPLAYER );   // eventflags (priority and flags)
			MESSAGE_END();
			*/

			// tell the Ts the bomb has been planted (on radar)
			CTeam *pTeam = GetGlobalTeam( TEAM_TERRORIST );
			for ( int iPlayer=0; iPlayer < pTeam->GetNumPlayers(); iPlayer++ )
			{
				CBasePlayer *pTempPlayer = pTeam->GetPlayer( iPlayer );
				
				if ( pTempPlayer->m_lifeState != LIFE_DEAD )
				{
					/*
					MESSAGE_BEGIN( MSG_ONE, gmsgBombDrop, NULL, pTempPlayer->pev );
						WRITE_COORD( pBomb->pev->origin.x );
						WRITE_COORD( pBomb->pev->origin.y );
						WRITE_COORD( pBomb->pev->origin.z );
						WRITE_BYTE( 1 ); // bomb was planted
					MESSAGE_END();
					*/
				}			
			}

			UTIL_ClientPrintAll( HUD_PRINTCENTER,"#Bomb_Planted" );
			pPlayer->SetProgressBarTime( 0 );

			// tell bots the bomb has been planted
			//g_pBotControl->OnEvent( EVENT_BOMB_PLANTED, pPlayer );

			CSGameRules()->m_bBombDropped = false;

			// Play the plant sound.
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), "c4.plant" );

			// release the player from being frozen
			pPlayer->ResetMaxSpeed();

			// Remove the C4 icon from the HUD
			//pPlayer->SetBombIcon();

			// No more c4!
			pPlayer->Weapon_Drop( this, NULL, NULL );
			UTIL_Remove( this );
#endif

			return;
		}
		else
		{
			ClientPrint( pPlayer, HUD_PRINTCENTER, "#C4_Activated_At_Bomb_Spot" );

#if !defined( CLIENT_DLL )
			//pPlayer->SetAnimation( PLAYER_HOLDBOMB );

			// release the player from being frozen
			pPlayer->ResetMaxSpeed();
#endif

			m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
			return;
		}
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
	m_flTimeWeaponIdle = gpGlobals->curtime + random->RandomFloat ( 10, 15 );
}

void CC4::WeaponIdle()
{
	if ( m_bStartedArming )
	{
		m_bStartedArming = false; //if the player releases the attack button cancel the arming sequence

		#if !defined( CLIENT_DLL )
			CCSPlayer *pPlayer = GetPlayerOwner();

			// release the player from being frozen
			pPlayer->ResetMaxSpeed();

			m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;

			pPlayer->SetProgressBarTime( 0 );
		#endif 

		/*
		if(m_bBombPlacedAnimation == true) //this means the placement animation is canceled
			SendWeaponAnim( C4_DRAW, UseDecrement() ? 1: 0);
		else
			SendWeaponAnim( C4_IDLE1, UseDecrement() ? 1: 0);
		*/
	}

	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	//UTIL_Remove( this );
}

