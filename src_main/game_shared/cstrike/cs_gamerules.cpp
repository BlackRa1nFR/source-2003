//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "cs_gamerules.h"
#include "ammodef.h"
#include "weapon_csbase.h"
#include "cs_shareddefs.h"


#ifdef CLIENT_DLL


#else
	
	#include "utldict.h"
	#include "cs_player.h"
	#include "cs_team.h"
	#include "cs_gamerules.h"
	#include "voice_gamemgr.h"
	#include "igamesystem.h"
	#include "weapon_c4.h"
	#include "mapinfo.h"
	#include "shake.h"
	#include "mapentities.h"
	#include "game.h"

#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


LINK_ENTITY_TO_CLASS( cs_gamerules, CCSGameRules );

IMPLEMENT_NETWORKCLASS_ALIASED( CSGameRules, DT_CSGameRules )

BEGIN_NETWORK_TABLE( CCSGameRules, DT_CSGameRules )
	#ifdef CLIENT_DLL
		RecvPropInt( RECVINFO( m_bFreezePeriod ) ),
		RecvPropInt( RECVINFO( m_iRoundTimeSecs ) ),
		RecvPropFloat( RECVINFO( m_fRoundCount ) )
	#else
		SendPropInt( SENDINFO( m_bFreezePeriod ), 1, SPROP_UNSIGNED ),
		SendPropInt( SENDINFO( m_iRoundTimeSecs ), 16 ),
		SendPropFloat( SENDINFO( m_fRoundCount ), 32, SPROP_NOSCALE )
	#endif
END_NETWORK_TABLE()


ConVar ammo_50AE_max( "ammo_50AE_max", "35", FCVAR_REPLICATED );
ConVar ammo_762mm_max( "ammo_762mm_max", "90", FCVAR_REPLICATED );
ConVar ammo_556mm_max( "ammo_556mm_max", "90", FCVAR_REPLICATED );
ConVar ammo_338mag_max( "ammo_338mag_max", "30", FCVAR_REPLICATED );
ConVar ammo_9mm_max( "ammo_9mm_max", "120", FCVAR_REPLICATED );
ConVar ammo_buckshot_max( "ammo_buckshot_max", "32", FCVAR_REPLICATED );
ConVar ammo_45acp_max( "ammo_45acp_max", "100", FCVAR_REPLICATED );
ConVar ammo_357sig_max( "ammo_357sig_max", "52", FCVAR_REPLICATED );
ConVar ammo_57mm_max( "ammo_57mm_max", "100", FCVAR_REPLICATED );
ConVar ammo_hegrenade_max( "ammo_hegrenade_max", "1", FCVAR_REPLICATED );
ConVar ammo_flashbang_max( "ammo_flashbang_max", "1", FCVAR_REPLICATED );


#ifdef CLIENT_DLL


#else

	// longest the intermission can last, in seconds
	#define MAX_INTERMISSION_TIME 120

   
	// --------------------------------------------------------------------------------------------------- //
	// Voice helper
	// --------------------------------------------------------------------------------------------------- //

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker )
		{
			// Dead players can only be heard by other dead team mates
			if ( pTalker->IsAlive() == false )
			{
				if ( pListener->IsAlive() == false )
					return ( pListener->InSameTeam( pTalker ) );

				return false;
			}

			return ( pListener->InSameTeam( pTalker ) );
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;



	// --------------------------------------------------------------------------------------------------- //
	// Globals.
	// --------------------------------------------------------------------------------------------------- //

	// NOTE: the indices here must match TEAM_TERRORIST, TEAM_CT, TEAM_SPECTATOR, etc.
	char *sTeamNames[] =
	{
		"",				// This one gets skipped over to account for the iTeam-1 shift in GetGlobalTeam..
		"Unassigned",
		"Terrorist",
		"Counter-Terrorist",
		"Spectator"
	};


	ConVar autoteambalance( "mp_autoteambalance", "1" );

	ConVar maxrounds( 
		"mp_maxrounds", 
		"0", 
		0, 
		"max number of rounds to play before server changes maps",
		true, 0, 
		false, 0 );

	ConVar winlimit( 
		"mp_winlimit", 
		"0", 
		0, 
		"max number of rounds one team can win before server changes maps",
		true, 0,
		false, 0 );

	ConVar startmoney( 
		"mp_startmoney", 
		"800", 
		0,
		"amount of money each player gets when they reset",
		true, 800,
		true, 16000 );	

	ConVar roundtime( 
		"mp_roundtime",
		"5",
		0,
		"How many minutes each round takes.",
		true, 1,	// min value
		true, 9		// max value
		);

	ConVar freezetime( 
		"mp_freezetime",
		"6",
		0,
		"how many seconds to keep players frozen when the round starts",
		true, 0,	// min value
		true, 60	// max value
		);

	ConVar c4timer( 
		"mp_c4timer", 
		"45", 
		0,
		"how long from when the C4 is armed until it blows",
		true, 10,	// min value
		true, 90	// max value
		);

	ConVar limitteams( 
		"mp_limitteams", 
		"2", 
		0,
		"Max # of players 1 team can have over another",
		true, 1,	// min value
		true, 20	// max value
		);

	ConVar tkpunish( 
		"mp_tkpunish", 
		"0", 
		0,
		"Will a TK'er be punished in the next round?  {0=no,  1=yes}" );

	ConVar buytime( 
		"mp_buytime", 
		"1.5",
		0,
		"How many minutes after round start players can buy items for.",
		true, 0.25,
		false, 0 );

	ConVar fadetoblack(
		"mp_fadetoblack",
		"0",
		FCVAR_REPLICATED,
		"fade a player's screen to black when he dies" );

	ConVar mp_chattime(
		"mp_chattime", 
		"10", 
		0,
		"amount of time players can chat after the round is over",
		true, 1,
		true, MAX_INTERMISSION_TIME
		);

	ConVar restartround( 
		"sv_restartround", 
		"0",
		0,
		"If non-zero, round will restart in the specified number of seconds" );

	ConVar sv_restart(
		"sv_restart", 
		"0", 
		0,
		"If non-zero, round will restart in the specified number of seconds" );

	
	ConCommand cc_EndRound( "EndRound", &CCSGameRules::cc_EndRound, "End the current CounterStrike round.", FCVAR_CHEAT );


	// --------------------------------------------------------------------------------------------------- //
	// Global helper functions.
	// --------------------------------------------------------------------------------------------------- //

	// Helper function to parse arguments to player commands.
	const char* FindEngineArg( const char *pName )
	{
		int nArgs = engine->Cmd_Argc();
		for ( int i=1; i < nArgs; i++ )
		{
			if ( stricmp( engine->Cmd_Argv(i), pName ) == 0 )
				return (i+1) < nArgs ? engine->Cmd_Argv(i+1) : "";
		}
		return 0;
	}


	int FindEngineArgInt( const char *pName, int defaultVal )
	{
		const char *pVal = FindEngineArg( pName );
		if ( pVal )
			return atoi( pVal );
		else
			return defaultVal;
	}


	void InitBodyQue(void)
	{
		// FIXME: Make this work
	}


	bool IsSpaceEmpty( CBaseEntity *pMainEnt, const Vector &vMin, const Vector &vMax )
	{
		Vector vHalfDims = ( vMax - vMin ) * 0.5f;
		Vector vCenter = vMin + vHalfDims;

		trace_t trace;
		UTIL_TraceHull( vCenter, vCenter, -vHalfDims, vHalfDims, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );

		bool bClear = ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) );
		return bClear;
	}


	Vector MaybeDropToGround( 
		CBaseEntity *pMainEnt, 
		bool bDropToGround, 
		const Vector &vPos, 
		const Vector &vMins, 
		const Vector &vMaxs )
	{
		if ( bDropToGround )
		{
			trace_t trace;
			UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
			return trace.endpos;
		}
		else
		{
			return vPos;
		}
	}


	//-----------------------------------------------------------------------------
	// Purpose: This function can be used to find a valid placement location for an entity.
	//			Given an origin to start looking from and a minimum radius to place the entity at,
	//			it will sweep out a circle around vOrigin and try to find a valid spot (on the ground)
	//			where mins and maxs will fit.
	// Input  : *pMainEnt - Entity to place
	//			&vOrigin - Point to search around
	//			fRadius - Radius to search within
	//			nTries - Number of tries to attempt
	//			&mins - mins of the Entity
	//			&maxs - maxs of the Entity
	//			&outPos - Return point
	// Output : Returns true and fills in outPos if it found a spot.
	//-----------------------------------------------------------------------------
	bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround )
	{
		// This function moves the box out in each dimension in each step trying to find empty space like this:
		//
		//											  X  
		//							   X			  X  
		// Step 1:   X     Step 2:    XXX   Step 3: XXXXX
		//							   X 			  X  
		//											  X  
		//
			 
		Vector mins, maxs;
		pMainEnt->WorldSpaceAABB( &mins, &maxs );
		mins -= pMainEnt->GetAbsOrigin();
		maxs -= pMainEnt->GetAbsOrigin();

		// Put some padding on their bbox.
		float flPadSize = 5;
		Vector vTestMins = mins - Vector( flPadSize, flPadSize, flPadSize );
		Vector vTestMaxs = maxs + Vector( flPadSize, flPadSize, flPadSize );

		// First test the starting origin.
		if ( IsSpaceEmpty( pMainEnt, vOrigin + vTestMins, vOrigin + vTestMaxs ) )
		{
			outPos = MaybeDropToGround( pMainEnt, bDropToGround, vOrigin, vTestMins, vTestMaxs );
			return true;
		}

		Vector vDims = vTestMaxs - vTestMins;

		// Keep branching out until we get too far.
		int iCurIteration = 0;
		int nMaxIterations = 15;
		
		int offset = 0;
		do
		{
			for ( int iDim=0; iDim < 3; iDim++ )
			{
				float flCurOffset = offset * vDims[iDim];

				for ( int iSign=0; iSign < 2; iSign++ )
				{
					Vector vBase = vOrigin;
					vBase[iDim] += (iSign*2-1) * flCurOffset;
				
					if ( IsSpaceEmpty( pMainEnt, vBase + vTestMins, vBase + vTestMaxs ) )
					{
						// Ensure that there is a clear line of sight from the spawnpoint entity to the actual spawn point.
						// (Useful for keeping things from spawning behind walls near a spawn point)
						trace_t tr;
						UTIL_TraceLine( vOrigin, vBase, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &tr );

						if ( tr.fraction != 1.0 )
						{
							continue;
						}

						outPos = MaybeDropToGround( pMainEnt, bDropToGround, vBase, vTestMins, vTestMaxs );
						return true;
					}
				}
			}

			++offset;
		} while ( iCurIteration++ < nMaxIterations );

	//	Warning( "EntityPlacementTest for ent %d:%s failed!\n", pMainEnt->entindex(), pMainEnt->GetClassname() );
		return false;
	}


	void Broadcast( const char* sentence )
	{
		char text[256];

		if ( !sentence )
			return;

		Q_strncpy( text, "%!MRAD_", sizeof( text ) );
		Q_strncat( text, UTIL_VarArgs (("%s"), sentence), sizeof( text ) );

		/*
		CBroadcastRecipientFilter filter;
		UserMessageBegin ( filter, "SendAudio" );
			WRITE_BYTE( 0 );
			WRITE_STRING( text );
		MessageEnd();
		*/
	}


	// Reset the dropped bomb on everyone's radar
	void ResetBombPickup()
	{
		//CBroadcastRecipientFilter filter;
		//UserMessageBegin( filter, "BombPickup" );
		//MessageEnd();
	}


	int UTIL_HumansInGame( bool ignoreSpectators = false )
	{
		int iCount = 0;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CCSPlayer *entity = CCSPlayer::Instance( i );

			if ( entity && !FNullEnt( entity->pev ) )
			{
				if ( FStrEq( STRING( entity->pl.netname ), "" ) )
					continue;

				if ( FBitSet( entity->GetFlags(), FL_FAKECLIENT ) )
					continue;

				if ( ignoreSpectators && entity->GetTeamNumber() != TEAM_TERRORIST && entity->GetTeamNumber() != TEAM_CT )
					continue;

				iCount++;
			}
		}

		return iCount;
	}



	// --------------------------------------------------------------------------------------------------- //
	// CCSGameRules implementation.
	// --------------------------------------------------------------------------------------------------- //

	CCSGameRules::CCSGameRules()
	{
		m_bAllowWeaponSwitch = true;
		m_bFreezePeriod = true;
		m_iRoundWinStatus = 0;
		m_iNumTerrorist = m_iNumCT = 0;	// number of players per team
		m_fTeamCount = 0.0;				// RestartRoundTime
		m_bRoundTerminating = 0;
		m_iNumSpawnableTerrorist = m_iNumSpawnableCT = 0;
		m_bFirstConnected = false;
		m_bCompleteReset = false;
		m_iAccountTerrorist = m_iAccountCT = 0;
		m_iNumCTWins = 0;
		m_iNumTerroristWins = 0;
		m_bTargetBombed = false;
		m_bBombDefused = false;
		m_iTotalRoundsPlayed = 0;
		m_iUnBalancedRounds = 0;
		m_flResetTime = 0;
		m_flTimeLimit = 0;

		ReadMultiplayCvars();

		//This makes the round timer function as the intro timer on the client side for the intro round only
		m_iIntroRoundTime += 2;
		m_iRoundTimeSecs = m_iIntroRoundTime;

		// Create the team managers
		for ( int i = 1; i < ARRAYSIZE( sTeamNames ); i++ )
		{
			CTeam *pTeam = (CTeam*)CreateEntityByName( "cs_team_manager" );
			pTeam->Init( sTeamNames[i], i );

			g_Teams.AddToTail( pTeam );
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	CCSGameRules::~CCSGameRules()
	{
		// Note, don't delete each team since they are in the gEntList and will 
		// automatically be deleted from there, instead.
		g_Teams.Purge();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	void CCSGameRules::UpdateClientData( CBasePlayer *player )
	{
	}

	//-----------------------------------------------------------------------------
	// Purpose: TF2 Specific Client Commands
	// Input  :
	// Output :
	//-----------------------------------------------------------------------------
	bool CCSGameRules::ClientCommand(const char *pcmd, CBaseEntity *pEdict )
	{
		CCSPlayer *pPlayer = ToCSPlayer( pEdict );

		if ( FStrEq( pcmd, "changeteam" ) )
		{
			return true;
		}
		else if( pPlayer->ClientCommand( pcmd ) )
		{
			return true;
		}
		else if( BaseClass::ClientCommand( pcmd, pEdict ) )
		{
			return true;
		}

		return false;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Player has just spawned. Equip them.
	//-----------------------------------------------------------------------------
	void CCSGameRules::PlayerSpawn( CBasePlayer *pBasePlayer )
	{
		CCSPlayer *pPlayer = ToCSPlayer( pBasePlayer );
		if ( !pPlayer )
			Error( "PlayerSpawn" );

		// This is tied to the joining state (m_iJoiningState).. add it when the joining state is there.
		//if ( pPlayer->m_bJustConnected )
		//	return;

		pPlayer->EquipSuit();
		
		bool addDefault = true;

		CBaseEntity	*pWeaponEntity = NULL;
		while ( pWeaponEntity = gEntList.FindEntityByClassname( pWeaponEntity, "game_player_equip" ))
		{
			pWeaponEntity->Touch( pPlayer );
			addDefault = false;
		}

		if ( pPlayer->m_bNotKilled == true )
			addDefault = false;

		if ( addDefault || pPlayer->m_bIsVIP )
			pPlayer->GiveDefaultItems();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *pVictim - 
	//			*pKiller - 
	//			*pInflictor - 
	//-----------------------------------------------------------------------------
	void CCSGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
	{
		// BaseClass::DeathNotice( pVictim, info ); // CS doesn't know DeathMsg
	}

	void CCSGameRules::InitDefaultAIRelationships()
	{
		//  Allocate memory for default relationships
		CBaseCombatCharacter::AllocateDefaultRelationships();

		// --------------------------------------------------------------
		// First initialize table so we can report missing relationships
		// --------------------------------------------------------------
		int i, j;
		for (i=0;i<NUM_AI_CLASSES;i++)
		{
			for (j=0;j<NUM_AI_CLASSES;j++)
			{
				// By default all relationships are neutral of priority zero
				CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
			}
		}
	}

	//------------------------------------------------------------------------------
	// Purpose : Return classify text for classify type
	//------------------------------------------------------------------------------
	const char *CCSGameRules::AIClassText(int classType)
	{
		switch (classType)
		{
			case CLASS_NONE:			return "CLASS_NONE";
			case CLASS_PLAYER:			return "CLASS_PLAYER";
			default:					return "MISSING CLASS in ClassifyText()";
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: When gaining new technologies in TF, prevent auto switching if we
	//  receive a weapon during the switch
	// Input  : *pPlayer - 
	//			*pWeapon - 
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool CCSGameRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
	{
		if ( !GetAllowWeaponSwitch() )
			return false;

		return BaseClass::FShouldSwitchWeapon( pPlayer, pWeapon );
	}


	void CCSGameRules::TerminateRound( float tmDelay, int iWinStatus )
	{
		m_iRoundWinStatus	= iWinStatus;
		m_fTeamCount		= gpGlobals->curtime + tmDelay;
		m_bRoundTerminating	= 1;
	}


	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : allow - 
	//-----------------------------------------------------------------------------
	void CCSGameRules::SetAllowWeaponSwitch( bool allow )
	{
		m_bAllowWeaponSwitch = allow;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool CCSGameRules::GetAllowWeaponSwitch()
	{
		return m_bAllowWeaponSwitch;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *pPlayer - 
	// Output : const char
	//-----------------------------------------------------------------------------
	const char *CCSGameRules::SetDefaultPlayerTeam( CBasePlayer *pPlayer )
	{
		Assert( pPlayer );
		int clientIndex = pPlayer->entindex();
		engine->SetClientKeyValue( clientIndex, engine->GetInfoKeyBuffer( pPlayer->edict() ), "model", "" );
		return BaseClass::SetDefaultPlayerTeam( pPlayer );
	}


	void CCSGameRules::Activate()
	{
		BaseClass::Activate();

		// Figure out from the entities in the map what kind of map this is (bomb run, prison escape, etc).
		CheckMapConditions();
	}


	// Called when game rules are created by CWorld
	void CCSGameRules::LevelInit()
	{ 
		BaseClass::LevelInit();
	}

	// Called when game rules are destroyed by CWorld
	void CCSGameRules::LevelShutdown()
	{
		BaseClass::LevelShutdown();
	}

	void CCSGameRules::CheckWinConditions()
	{
		// If a winner has already been determined.. then get the heck out of here
		if (m_iRoundWinStatus != 0)
			return;

		// Initialize the player counts..
		int NumDeadCT, NumDeadTerrorist, NumAliveTerrorist, NumAliveCT;
		InitializePlayerCounts( NumAliveTerrorist, NumAliveCT, NumDeadTerrorist, NumDeadCT );


		/***************************** OTHER PLAYER's CHECK *********************************************************/
		bool bNeededPlayers = false;
		if ( NeededPlayersCheck( bNeededPlayers ) )
			return;

		/****************************** ASSASINATION/VIP SCENARIO CHECK *******************************************************/
		if ( VIPRoundEndCheck( bNeededPlayers ) )
			return;

		/****************************** PRISON ESCAPE CHECK *******************************************************/
		if ( PrisonRoundEndCheck() )
			return;


		/****************************** BOMB CHECK ********************************************************/
		if ( BombRoundEndCheck( bNeededPlayers ) )
			return;


		/***************************** TEAM EXTERMINATION CHECK!! *********************************************************/
		// CounterTerrorists won by virture of elimination
		if ( TeamExterminationCheck( NumAliveTerrorist, NumAliveCT, NumDeadTerrorist, NumDeadCT, bNeededPlayers ) )
			return;

		
		/******************************** HOSTAGE RESCUE CHECK ******************************************************/
		if ( HostageRescueRoundEndCheck( bNeededPlayers ) )
			return;
	}


	bool CCSGameRules::NeededPlayersCheck( bool &bNeededPlayers )
	{
		// We needed players to start scoring
		// Do we have them now?
		if( !m_iNumSpawnableTerrorist || !m_iNumSpawnableCT )
		{
			UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#Game_scoring" );
			bNeededPlayers = true;
		}

		if ( !m_bFirstConnected && m_iNumSpawnableTerrorist && m_iNumSpawnableCT )
		{
			// Start the round immediately when the first person joins
			UTIL_LogPrintf( "World triggered \"Game_Commencing\"\n" );

			m_bFreezePeriod  = false; //Make sure we are not on the FreezePeriod.
			m_bCompleteReset = true;

			EndRoundMessage( "#Game_Commencing", Round_Draw );

			TerminateRound( 3, 3 );
			m_bFirstConnected = true;
			return true;
		}

		return false;
	}


	void CCSGameRules::InitializePlayerCounts(
		int &NumAliveTerrorist,
		int &NumAliveCT,
		int &NumDeadTerrorist,
		int &NumDeadCT
		)
	{
		NumAliveTerrorist = NumAliveCT = NumDeadCT = NumDeadTerrorist = 0;
		m_iNumTerrorist = m_iNumCT = m_iNumSpawnableTerrorist = m_iNumSpawnableCT = 0;
		m_iHaveEscaped = 0;

		// Count how many dead players there are on each team.
		for ( int iTeam=1; iTeam <= GetNumberOfTeams(); iTeam++ )
		{
			CTeam *pTeam = GetGlobalTeam( iTeam );

			for ( int iPlayer=0; iPlayer < pTeam->GetNumPlayers(); iPlayer++ )
			{
				CCSPlayer *pPlayer = ToCSPlayer( pTeam->GetPlayer( iPlayer ) );
				Assert( pPlayer );
				if ( !pPlayer )
					continue;

				Assert( pPlayer->GetTeamNumber() == pTeam->GetTeamNumber() );

				switch ( pTeam->GetTeamNumber() )
				{
				case TEAM_CT:
					m_iNumCT++;

					//if ( pPlayer->m_iMenu != Menu_ChooseAppearance )
						m_iNumSpawnableCT++;

					if ( pPlayer->m_lifeState != LIFE_ALIVE )
						NumDeadCT++;
					else
						NumAliveCT++;

					break;

				case TEAM_TERRORIST:
					m_iNumTerrorist++;

					//if ( pPlayer->m_iMenu != Menu_ChooseAppearance )
						m_iNumSpawnableTerrorist++;

					if ( pPlayer->m_lifeState != LIFE_ALIVE )
						NumDeadTerrorist++;
					else
						NumAliveTerrorist++;

					// Check to see if this guy escaped.
					if ( pPlayer->m_bEscaped == true )
						m_iHaveEscaped++;

					break;
				}
			}
		}
	}


	bool CCSGameRules::HostageRescueRoundEndCheck( bool bNeededPlayers )
	{
		//MIKETODO: reenable this when implementing hostage rescue.
		/*
		// Check to see if 50% of the hostages have been rescued.
		CBaseEntity* hostage = NULL;
		int iHostages = 0;
		bool bHostageAlive = false;  // Assume that all hostages are either rescued or dead..

		hostage = gEntList.FindEntityByClassname ( hostage, "hostage_entity" );
		while (hostage != NULL)
		{
			iHostages++;
			if (hostage->pev->takedamage == DAMAGE_YES) // We've found a live hostage. don't end the round
				bHostageAlive = true;

			hostage = gEntList.FindEntityByClassname ( hostage, "hostage_entity" );
		}

		// There are no hostages alive.. check to see if the CTs have rescued atleast 50% of them.
		if ( (bHostageAlive == false) &&	(iHostages > 0))
		{
			if ( m_iHostagesRescued >= (iHostages * 0.5)	)
			{
				Broadcast ("ctwin");

				m_iAccountCT += 2500;

				if ( !bNeededPlayers )
				{
					m_iNumCTWins ++;
					// Update the clients team score
					UpdateTeamScores();
				}
				EndRoundMessage( "#All_Hostages_Rescued", All_Hostages_Rescued );
				TerminateRound( 5, 1 );
				return true;
			}
		}
		*/
		return false;
	}


	bool CCSGameRules::PrisonRoundEndCheck()
	{
		//MIKETODO: get this working when working on prison escape
		/*
		if (m_bMapHasEscapeZone == true)
		{
			float flEscapeRatio;

			flEscapeRatio = (float) m_iHaveEscaped / (float) m_iNumEscapers;

			if (flEscapeRatio >= m_flRequiredEscapeRatio)
			{
				Broadcast ("terwin");
				m_iAccountTerrorist += 3150;

				if ( !bNeededPlayers )
				{
					m_iNumTerroristWins ++;
					// Update the clients team score
					UpdateTeamScores();
				}
				EndRoundMessage( "#Terrorists_Escaped", Terrorists_Escaped );
				TerminateRound( 5,2 );
				return;
			}
			else if ( NumAliveTerrorist == 0 && flEscapeRatio < m_flRequiredEscapeRatio)
			{
				Broadcast("ctwin");
				m_iAccountCT += (1 - flEscapeRatio) * 3500; // CTs are rewarded based on how many terrorists have escaped...
				
				if ( !bNeededPlayers )
				{
					m_iNumCTWins++;
					// Update the clients team score
					UpdateTeamScores();
				}
				EndRoundMessage( "#CTs_PreventEscape", CTs_PreventEscape );
				TerminateRound( 5, 1 );
				return;
			}

			else if ( NumAliveTerrorist == 0 && NumDeadTerrorist != 0 && m_iNumSpawnableCT > 0 )
			{
				Broadcast ("ctwin");
				m_iAccountCT += (1 - flEscapeRatio) * 3250; // CTs are rewarded based on how many terrorists have escaped...
				
				if ( !bNeededPlayers )
				{
					m_iNumCTWins++;
					// Update the clients team score
					UpdateTeamScores();
				}
				EndRoundMessage( "#Escaping_Terrorists_Neutralized", Escaping_Terrorists_Neutralized );
				TerminateRound( 5, 1 );
				return;
			}
			// else return;    
		}
		*/

		return false;
	}


	bool CCSGameRules::VIPRoundEndCheck( bool bNeededPlayers )
	{
		if (m_iMapHasVIPSafetyZone != 1)
			return false;

		if (m_pVIP == NULL)
			return false;

		if (m_pVIP->m_bEscaped == true)
		{
			Broadcast ("ctwin");
			m_iAccountCT += 3500;

			if ( !bNeededPlayers )
			{
				m_iNumCTWins ++;
				// Update the clients team score
				UpdateTeamScores();
			}

			//MIKETODO: get this working when working on VIP scenarios
			/*
			MessageBegin( MSG_SPEC, SVC_DIRECTOR );
				WRITE_BYTE ( 9 );	// command length in bytes
				WRITE_BYTE ( DRC_CMD_EVENT );	// VIP rescued
				WRITE_SHORT( ENTINDEX(m_pVIP->edict()) );	// index number of primary entity
				WRITE_SHORT( 0 );	// index number of secondary entity
				WRITE_LONG( 15 | DRC_FLAG_FINAL);   // eventflags (priority and flags)
			MessageEnd();
			*/

			EndRoundMessage( "#VIP_Escaped", VIP_Escaped );
			TerminateRound(5,1);
			return true;
		}
		else if ( m_pVIP->m_lifeState == LIFE_DEAD )   // The VIP is dead
		{
			Broadcast ( "terwin" );
			m_iAccountTerrorist += 3250;

			if ( !bNeededPlayers )
			{
				m_iNumTerroristWins ++;
				// Update the clients team score
				UpdateTeamScores();
			}
			EndRoundMessage( "#VIP_Assassinated", VIP_Assassinated );
			TerminateRound( 5,2 );
			return true;
		}

		return false;
	}


	bool CCSGameRules::BombRoundEndCheck( bool bNeededPlayers )
	{
		// Check to see if the bomb target was hit or the bomb defused.. if so, then let's end the round!
		if ( ( m_bTargetBombed == true ) && ( m_bMapHasBombTarget == true ) )
		{
			Broadcast ( "terwin" );

			m_iAccountTerrorist += 3500;

			if ( !bNeededPlayers )
			{
				m_iNumTerroristWins ++;
				// Update the clients team score
				UpdateTeamScores();
			}

			EndRoundMessage( "#Target_Bombed", Target_Bombed );
			TerminateRound( 5, 2 );
			return true;
		}
		else
		if ( ( m_bBombDefused == true ) && ( m_bMapHasBombTarget == true ) )
		{
			Broadcast ( "ctwin" );

			m_iAccountCT += 3250;

			if ( !bNeededPlayers )
			{
				m_iNumCTWins++;
				// Update the clients team score
				UpdateTeamScores();
			}

			EndRoundMessage( "#Bomb_Defused", Bomb_Defused );
			TerminateRound( 5, 1 );
			return true;
		}

		return false;
	}


	bool CCSGameRules::TeamExterminationCheck(
		int NumAliveTerrorist,
		int NumAliveCT,
		int NumDeadTerrorist,
		int NumDeadCT,
		bool bNeededPlayers
	)
	{
		if ( ( m_iNumCT > 0 && m_iNumSpawnableCT > 0 ) && ( m_iNumTerrorist > 0 && m_iNumSpawnableTerrorist > 0 ) )
		{
			if ( NumAliveTerrorist == 0 && NumDeadTerrorist != 0 && m_iNumSpawnableCT > 0 )
			{
				bool nowin = false;
					
				for ( int iGrenade=0; iGrenade < g_PlantedC4s.Count(); iGrenade++ )
				{
					CPlantedC4 *C4 = g_PlantedC4s[iGrenade];

					if ( ( C4->m_bJustBlew == false )	)
						nowin = true;
				}

				if ( !nowin )
				{
					Broadcast ("ctwin");

					if ( m_bMapHasBombTarget )
						m_iAccountCT += 3250;
					else
						m_iAccountCT += 3000;

					if ( !bNeededPlayers )
					{
						m_iNumCTWins++;
						// Update the clients team score
						UpdateTeamScores();
					}
					EndRoundMessage( "#CTs_Win", CTs_Win );	
					TerminateRound( 5, 1 );
				}

				return true;
			}
		
			// Terrorists WON
			if ( NumAliveCT == 0 && NumDeadCT != 0 && m_iNumSpawnableTerrorist > 0 )
			{
				Broadcast ( "terwin" );
				
				if ( m_bMapHasBombTarget )
					m_iAccountTerrorist += 3250;
				else
					m_iAccountTerrorist += 3000;

				if ( !bNeededPlayers )
				{
					m_iNumTerroristWins++;
					// Update the clients team score
					UpdateTeamScores();
				}
				EndRoundMessage( "#Terrorists_Win", Terrorists_Win );
				TerminateRound( 5, 2 );
				return true;
			}
		}
		else if ( NumAliveCT == 0 && NumAliveTerrorist == 0 )
		{
			EndRoundMessage( "#Round_Draw", Round_Draw );
			Broadcast( "rounddraw" );
			TerminateRound( 5, 3 );
			return true;
		}

		return false;
	}


	void CCSGameRules::PickNextVIP()
	{
		// MIKETODO: work on this when getting VIP maps running.
		/*
		if (IsVIPQueueEmpty() != true)
		{
			// Remove the current VIP from his VIP status and make him a regular CT.
			if (m_pVIP != NULL)
				ResetCurrentVIP();

			for (int i = 0; i <= 4; i++)
			{
				if (VIPQueue[i] != NULL)
				{
					m_pVIP = VIPQueue[i];
					m_pVIP->MakeVIP();

					VIPQueue[i] = NULL;		// remove this player from the VIP queue
					StackVIPQueue();		// and re-organize the queue
					m_iConsecutiveVIP = 0;
					return;
				}
			}
		}
		else if (m_iConsecutiveVIP >= 3)	// If it's been the same VIP for 3 rounds already.. then randomly pick a new one
		{
			m_iLastPick++;

			if (m_iLastPick > m_iNumCT)
				m_iLastPick = 1;

			int iCount = 1;

			CBaseEntity* pPlayer = NULL;
			CBasePlayer* player = NULL;
			CBasePlayer* pLastPlayer = NULL;

			pPlayer = UTIL_FindEntityByClassname ( pPlayer, "player" );
			while (		(pPlayer != NULL) && (!FNullEnt(pPlayer->edict()))	)
			{
				if (	!(pPlayer->pev->flags & FL_DORMANT)	)
				{
					player = GetClassPtr((CBasePlayer *)pPlayer->pev);
					
					if (	(player->m_iTeam == CT) && (iCount == m_iLastPick)	)
					{
						if (	(player == m_pVIP) && (pLastPlayer != NULL)	)
							player = pLastPlayer;

						// Remove the current VIP from his VIP status and make him a regular CT.
						if (m_pVIP != NULL)
							ResetCurrentVIP();

						player->MakeVIP();
						m_iConsecutiveVIP = 0;

						return;
					}
					else if ( player->m_iTeam == CT )
						iCount++;

					if ( player->m_iTeam != SPECTATOR )
						pLastPlayer = player;
				}
				pPlayer = UTIL_FindEntityByClassname ( pPlayer, "player" );
			}
		}
		else if (m_pVIP == NULL)  // There is no VIP and there is no one waiting to be the VIP.. therefore just pick the first CT player we can find.
		{
			CBaseEntity* pPlayer = NULL;
			CBasePlayer* player = NULL;

			pPlayer = UTIL_FindEntityByClassname ( pPlayer, "player" );
			while (		(pPlayer != NULL) && (!FNullEnt(pPlayer->edict()))	)
			{
				if ( pPlayer->pev->flags != FL_DORMANT	)
				{
					player = GetClassPtr((CBasePlayer *)pPlayer->pev);
		
					if ( player->m_iTeam == CT )
					{
						player->MakeVIP();
						m_iConsecutiveVIP = 0;
						return;
					}
				}
				pPlayer = UTIL_FindEntityByClassname ( pPlayer, "player" );
			}
		}
		*/
	}


	void CCSGameRules::ReadMultiplayCvars()
	{
		m_iRoundTime = roundtime.GetInt() * 60;
		m_iIntroRoundTime = freezetime.GetInt();
	}


	void CCSGameRules::RestartRound()
	{
		m_iTotalRoundsPlayed++;
		
		//ClearBodyQue();

		// Hardlock the player accelaration to 5.0
		//CVAR_SET_FLOAT( "sv_accelerate", 5.0 );
		//CVAR_SET_FLOAT( "sv_friction", 4.0 );
		//CVAR_SET_FLOAT( "sv_stopspeed", 75 );

		// Tabulate the number of players on each team.
		m_iNumCT = GetGlobalTeam( TEAM_CT )->GetNumPlayers();
		m_iNumTerrorist = GetGlobalTeam( TEAM_TERRORIST )->GetNumPlayers();
		
		// reset the dropped bomb on everyone's radar
		if ( m_bMapHasBombTarget )
		{
			ResetBombPickup();
		}

		m_bBombDropped = false;
		
		//MIKETODO: revisit HLTV support later
		/*
		// reset all players health for HLTV
		MessageBegin( MSG_SPEC, gmsgHLTV );
			WRITE_BYTE( 0 );	// 0 = all players
			WRITE_BYTE( 100 | 128 );	// 100 health + msg flag
		MessageEnd();

		// reset all players FOV for HLTV
		MessageBegin( MSG_SPEC, gmsgHLTV );
			WRITE_BYTE( 0 );	// all players
			WRITE_BYTE( 0 );	// to default FOV value
		MessageEnd();
		*/


		/*************** AUTO-BALANCE CODE *************/
		if ( autoteambalance.GetInt() != 0 &&
			(m_iUnBalancedRounds >= 1) )
		{
			BalanceTeams();
		}

		if ( ((m_iNumCT - m_iNumTerrorist) >= 2) ||
			((m_iNumTerrorist - m_iNumCT) >= 2)	)
		{
			m_iUnBalancedRounds++;
		}
		else
		{
			m_iUnBalancedRounds = 0;
		}

		// Warn the players of an impending auto-balance next round...
		if ( autoteambalance.GetInt() != 0 &&
			(m_iUnBalancedRounds == 1)	)
		{
			UTIL_ClientPrintAll( HUD_PRINTCENTER,"#Auto_Team_Balance_Next_Round");
		}

		/*************** AUTO-BALANCE CODE *************/

		if ( m_bCompleteReset )
		{
			// bounds check
			if ( timelimit.GetInt() < 0 )
			{
				timelimit.SetValue( 0 );
			}

			m_flResetTime = gpGlobals->curtime;

			// Reset timelimit
			if ( timelimit.GetInt() != 0 )
				m_flTimeLimit = gpGlobals->curtime + ( timelimit.GetInt() * 60 );

			// Reset total # of rounds played
			m_iTotalRoundsPlayed = 0;

			// Reset score info
			m_iNumTerroristWins				= 0;
			m_iNumCTWins					= 0;
			m_iNumConsecutiveTerroristLoses	= 0;
			m_iNumConsecutiveCTLoses		= 0;


			// Reset team scores
			UpdateTeamScores();


			// Reset the player stats
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CCSPlayer *pPlayer = CCSPlayer::Instance( i );

				if ( pPlayer && !FNullEnt( pPlayer->pev ) )
					pPlayer->Reset();
			}

		}

		m_bFreezePeriod = true;
		m_bRoundTerminating = 0;

		ReadMultiplayCvars();

		// set the idlekick max time (in seconds)
		m_fMaxIdlePeriod = m_iRoundTime * 2;

		//*******new functionality by SupraFiend (in seconds)
		//This makes the round timer function as the intro timer on the client side
		m_iRoundTimeSecs = m_iIntroRoundTime;

		// Check to see if there's a mapping info paramater entity
		CMapInfo *mi = g_pMapInfo;
		if (mi != NULL)
		{
			switch ( mi->m_iBuyingStatus )
			{
				case 0: 
					m_bCTCantBuy = false; 
					m_bTCantBuy = false; 
					Msg( "EVERYONE CAN BUY!\n" );
					break;
				
				case 1: 
					m_bCTCantBuy = false; 
					m_bTCantBuy = true; 
					Msg( "Only CT's can buy!!\n" );
					break;

				case 2: 
					m_bCTCantBuy = true; 
					m_bTCantBuy = false; 
					Msg( "Only T's can buy!!\n" );
					break;
				
				case 3: 
					m_bCTCantBuy = true; 
					m_bTCantBuy = true; 
					Msg( "No one can buy!!\n" );
					break;

				default: 
					m_bCTCantBuy = false; 
					m_bTCantBuy = false; 
					break;
			}
		}
		
		
		// Check to see if this map has a bomb target in it

		if ( gEntList.FindEntityByClassname( NULL, "func_bomb_target" ) )
		{
			m_bMapHasBombTarget		= true;
			m_bMapHasBombZone		= true;
		}
		else if ( gEntList.FindEntityByClassname( NULL, "info_bomb_target" ) )
		{
			m_bMapHasBombTarget		= true;
			m_bMapHasBombZone		= false;
		}
		else
		{
			m_bMapHasBombTarget		= false;
			m_bMapHasBombZone		= false;
		}

		// Check to see if this map has hostage rescue zones

		if ( gEntList.FindEntityByClassname( NULL, "func_hostage_rescue" ) )
			m_bMapHasRescueZone = true;
		else
			m_bMapHasRescueZone = false;


		// See if the map has func_buyzone entities
		// Used by CBasePlayer::HandleSignals() to support maps without these entities
		
		if ( gEntList.FindEntityByClassname( NULL, "func_buyzone" ) )
			m_bMapHasBuyZone = true;
		else
			m_bMapHasBuyZone = false;


		// GOOSEMAN : See if this map has func_escapezone entities
		if ( gEntList.FindEntityByClassname( NULL, "func_escapezone" ) )
		{
			m_bMapHasEscapeZone = true;
			m_iHaveEscaped = 0;
			m_iNumEscapers = 0; // Will increase this later when we count how many Ts are starting
			if (m_iNumEscapeRounds >= 3)
			{
				SwapAllPlayers();
				m_iNumEscapeRounds = 0;
			}

			m_iNumEscapeRounds++;  // Increment the number of rounds played... After 8 rounds, the players will do a whole sale switch..
		}
		else
			m_bMapHasEscapeZone = false;

		// Check to see if this map has VIP safety zones
		if ( gEntList.FindEntityByClassname( NULL, "func_vip_safetyzone" ) )
		{
			PickNextVIP();
			m_iConsecutiveVIP++;
			m_iMapHasVIPSafetyZone = 1;
		}
		else
			m_iMapHasVIPSafetyZone = 2;



		// Update accounts based on number of hostages remaining.. SUB_Remove these hostage entities.

		//MIKETODO: commented out this code for now until I get hostages working.
		int acct_tmp = 0;
		/*
		for ( int iHostage=0; iHostage < g_Hostages.Count(); iHostage++ )
		{
			CHostage *pHostage = g_Hostages[iHostage];

			if ( pHostage->GetSolidFlags() & FSOLID_NOT_SOLID )
			{
				acct_tmp += 150;

				if ( pHostage->m_lifeState == LIFE_DEAD )
					hostage->pev->deadflag = DEAD_RESPAWNABLE;
			}
			
			temp->RePosition();
			hostage = (CBaseMonster*) gEntList.FindEntityByClassname(hostage, "hostage_entity");
		
			if ( acct_tmp >= 2000 )
				break;
		}
		*/

		//*******Catch up code by SupraFiend. Scale up the loser bonus when teams fall into losing streaks
		if (m_iRoundWinStatus == 2) // terrorists won
		{
			//check to see if they just broke a losing streak
			if(m_iNumConsecutiveTerroristLoses > 1)
				m_iLoserBonus = 1500;//this is the default losing bonus

			m_iNumConsecutiveTerroristLoses = 0;//starting fresh
			m_iNumConsecutiveCTLoses++;//increment the number of wins the CTs have had
		}
		else if (m_iRoundWinStatus == 1) // CT Won
		{
			//check to see if they just broke a losing streak
			if(m_iNumConsecutiveCTLoses > 1)
				m_iLoserBonus = 1500;//this is the default losing bonus

			m_iNumConsecutiveCTLoses = 0;//starting fresh
			m_iNumConsecutiveTerroristLoses++;//increment the number of wins the Terrorists have had
		}

		//check if the losing team is in a losing streak & that the loser bonus hasen't maxed out.
		if((m_iNumConsecutiveTerroristLoses > 1) && (m_iLoserBonus < 3000))
			m_iLoserBonus += 500;//help out the team in the losing streak
		else
		if((m_iNumConsecutiveCTLoses > 1) && (m_iLoserBonus < 3000))
			m_iLoserBonus += 500;//help out the team in the losing streak

		// assign the wining and losing bonuses
		if (m_iRoundWinStatus == 2) // terrorists won
		{
			m_iAccountTerrorist += acct_tmp;
			m_iAccountCT += m_iLoserBonus;
		}
		else if (m_iRoundWinStatus == 1) // CT Won
		{
			m_iAccountCT += acct_tmp;
			if (m_bMapHasEscapeZone == false)	// only give them the bonus if this isn't an escape map
				m_iAccountTerrorist += m_iLoserBonus;
		}
		

		//Update CT account based on number of hostages rescued
		m_iAccountCT += m_iHostagesRescued * 750;


		// Update individual players accounts and respawn players

		//**********new code by SupraFiend
		m_fIntroRoundCount = m_fRoundCount = gpGlobals->curtime;//the round time stamp must be set before players are spawned
		
		//Adrian - No cash for anyone at first rounds! ( well, only the default. )
		if ( m_bCompleteReset )
		{
			m_iAccountTerrorist = m_iAccountCT = 0; //No extra cash!.

			//We are starting fresh. So it's like no one has ever won or lost.
			m_iNumTerroristWins				= 0; 
			m_iNumCTWins					= 0;
			m_iNumConsecutiveTerroristLoses	= 0;
			m_iNumConsecutiveCTLoses		= 0;
			m_iLoserBonus					= 1400;
		}

		// tell bots that the round is restarting
		//g_pBotControl->RestartRound();

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CCSPlayer *pPlayer = CCSPlayer::Instance( i );

			if ( pPlayer && !FNullEnt( pPlayer->edict() ) )
			{
				pPlayer->m_iNumSpawns	= 0;
				pPlayer->m_bTeamChanged	= false;

				if ( pPlayer->GetTeamNumber() == TEAM_CT )
				{
					pPlayer->AddAccount( m_iAccountCT );
				}
				else if ( pPlayer->GetTeamNumber() == TEAM_TERRORIST )
				{
					m_iNumEscapers++;	// Add another potential escaper to the mix!
					pPlayer->AddAccount( m_iAccountTerrorist );

					if ( m_bMapHasEscapeZone == true )  // If it's a prison scenario then remove the Ts guns
						pPlayer->m_bNotKilled = false;   // this will cause them to be reset with default weapons, armor, and items
				}			

				if ( pPlayer->GetTeamNumber() != TEAM_UNASSIGNED && pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
				{
					if(pPlayer->HasC4() == true)//drop the c4 if the player is carrying it
						pPlayer->DropC4();

					pPlayer->RoundRespawn();
				}

				// Gooseman : The following code fixes the HUD icon bug
				// by removing the C4 and DEFUSER icons from the HUD regardless
				// for EVERY player (regardless of what team they're on)
			}
		}

		// Respawn entities (glass, doors, etc..)
		CleanUpMap();

		// Give C4 to the terrorists
		if (m_bMapHasBombTarget == true	)
			GiveC4();

		// Reset game variables
		m_flIntermissionEndTime = 0;
		m_flIntermissionStartTime = 0;
		m_fTeamCount = 0.0;
		m_iAccountTerrorist = m_iAccountCT = 0;
		m_iHostagesRescued = 0;
		m_iHostagesTouched = 0;
		m_iRoundWinStatus = 0;
		m_bTargetBombed = m_bBombDefused = false;
		m_bLevelInitialized = false;
		m_bCompleteReset = false;
	}


	void CCSGameRules::GiveC4( bool bDoSecondPass )
	{
		int iTeamCount;
		int iTemp = 0;

		//Modified by SupraFiend
		iTeamCount = m_iNumTerrorist;
		m_iC4Guy++;

		// if we've looped past the last Terrorist.. then give the C4 to the first one.
		if ( m_iC4Guy > iTeamCount )
	 		 m_iC4Guy = 1;

		// Give the C4 to the specified T player..
		for ( int iPlayer=1; iPlayer < gpGlobals->maxClients; iPlayer++ )
		{
			CCSPlayer *pPlayer = ToCSPlayer( UTIL_PlayerByIndex( iPlayer ) );
			if ( pPlayer && !FNullEnt( pPlayer->pev ) )
			{
				if ( (pPlayer->m_lifeState != LIFE_DEAD) && (pPlayer->GetTeamNumber() == TEAM_TERRORIST) )
				{
					iTemp++;
					if ( iTemp == m_iC4Guy )
					{
						pPlayer->GiveNamedItem( WEAPON_C4_CLASSNAME );
						//pPlayer->SetBombIcon();
						//pPlayer->pev->body = 1;
						
						//pPlayer->m_flDisplayHistory |= DHF_BOMB_RETRIEVED;
						//pPlayer->HintMessage( "#Hint_you_have_the_bomb", false, true );
		
						// Log this information
						//UTIL_LogPrintf("\"%s<%i><%s><TERRORIST>\" triggered \"Spawned_With_The_Bomb\"\n", 
						//	STRING( pPlayer->pl.netname ),
						//	GETPLAYERUSERID( pPlayer->edict() ),
						//	GETPLAYERAUTHID( pPlayer->edict() ) );

						m_bBombDropped = false;
						return;
					}
				}	
			}		
		}

		// m_iC4Guy went past the end of the player list.. reset it.
		if ( bDoSecondPass )
		{
			m_iC4Guy = -1; //Start again next time.
			GiveC4( false );
		}
	}


	void CCSGameRules::Think()
	{
		BaseClass::Think();

		
		for ( int i = 1; i <= GetNumberOfTeams(); i++ )
		{
			GetGlobalTeam( i )->Think();
		}


		if ( m_fRoundCount == 0 )
			m_fIntroRoundCount = m_fRoundCount = gpGlobals->curtime;//intialize the timer time stamps, this happens once only

		///// Check game rules /////
		if ( CheckGameOver() )
			return;	

		// have we hit the timelimit?
		if ( CheckTimeLimit() )
			return;
		
		// have we hit the max rounds?
		if ( CheckMaxRounds() )
			return;

		if ( CheckWinLimit() )
			return;

		
		// Check for the end of the round.
		if ( IsFreezePeriod() )
		{
			CheckFreezePeriodExpired();
		}
		else 
		{
			CheckRoundTimeExpired();
		}

		
		if ( m_fTeamCount != 0.0 && m_fTeamCount <= gpGlobals->curtime )
			RestartRound();
		

		CheckLevelInitialized();


		if ( gpGlobals->curtime > m_tmNextPeriodicThink )
		{
			CheckRestartRound();
			m_tmNextPeriodicThink = gpGlobals->curtime + 1.0;
		}
	}


	bool CCSGameRules::CheckGameOver()
	{
		if ( g_fGameOver )   // someone else quit the game already
		{
			// bounds check
			m_flIntermissionEndTime = m_flIntermissionStartTime + mp_chattime.GetInt();

			// check to see if we should change levels now
			if ( m_flIntermissionEndTime < gpGlobals->curtime )
			{
				if (
					/* PHINEAS BOT */
					( UTIL_HumansInGame() == 0 ) || // if only bots, just change immediately
					/* END PHINEAS BOT */
					
					m_iEndIntermissionButtonHit || // check that someone has pressed a key, or the max intermission time is over
					( ( m_flIntermissionStartTime + MAX_INTERMISSION_TIME ) < gpGlobals->curtime) ) 
					ChangeLevel(); // intermission is over
			}

			return true;
		}

		return false;
	}


	bool CCSGameRules::CheckTimeLimit()
	{
		float fTimeLimit = timelimit.GetInt();
		if ( fTimeLimit > 0 )
		{
			m_flTimeLimit = ( m_flResetTime + ( fTimeLimit * 60 ) );

			if ( gpGlobals->curtime >= m_flTimeLimit )
			{
				Msg( "Changing maps because time limit has been met\n" );
				GoToIntermission();
				return true;
			}
		}

		return false;
	}


	bool CCSGameRules::CheckMaxRounds()
	{
		if ( maxrounds.GetInt() != 0 )
		{
			if ( m_iTotalRoundsPlayed >= maxrounds.GetInt() )
			{
				Msg( "Changing maps due to maximum rounds have been met\n" );
				GoToIntermission();
				return true;
			}
		}
		
		return false;
	}


	bool CCSGameRules::CheckWinLimit()
	{
		// has one team won the specified number of rounds?
		if ( winlimit.GetInt() != 0 )
		{
			if ( ( m_iNumCTWins >= winlimit.GetInt() ) || ( m_iNumTerroristWins >= winlimit.GetInt() ) )
			{
				Msg( "Changing maps...one team has won the specified number of rounds\n" );
				GoToIntermission();
				return true;
			}
		}

		return false;
	}


	void CCSGameRules::CheckFreezePeriodExpired()
	{
		if ( TimeRemaining() > 0 )
			return;

		// Log this information
		UTIL_LogPrintf("World triggered \"Round_Start\"\n");

		// Freeze period expired: kill the flag
		m_bFreezePeriod = false;

		char CT_sentence[40];
		char T_sentence[40];
		
		switch ( random->RandomInt( 0, 3 ) )
		{
		case 0:
			Q_strncpy(CT_sentence,"radio.moveout", sizeof( CT_sentence ) ); 
			Q_strncpy(T_sentence ,"radio.moveout", sizeof( T_sentence ) ); 
			break;

		case 1:
			Q_strncpy(CT_sentence, "radio.letsgo", sizeof( CT_sentence ) ); 
			Q_strncpy(T_sentence , "radio.letsgo", sizeof( T_sentence ) ); 
			break;

		case 2:
			Q_strncpy(CT_sentence , "radio.locknload", sizeof( CT_sentence ) );
			Q_strncpy(T_sentence , "radio.locknload", sizeof( T_sentence ) );
			break;

		default:
			Q_strncpy(CT_sentence , "radio.go", sizeof( CT_sentence ) );
			Q_strncpy(T_sentence , "radio.go", sizeof( T_sentence ) );
			break;
		}

		// More specific radio commands for the new scenarios : Prison & Assasination
		if (m_bMapHasEscapeZone == TRUE)
		{
			Q_strncpy(CT_sentence , "radio.elim", sizeof( CT_sentence ) );
			Q_strncpy(T_sentence , "radio.getout", sizeof( T_sentence ) );
		}
		else if (m_iMapHasVIPSafetyZone == 1)
		{
			Q_strncpy(CT_sentence , "radio.vip", sizeof( CT_sentence ) );
			Q_strncpy(T_sentence , "radio.locknload", sizeof( T_sentence ) );
		}

		// Reset the round time
		m_fRoundCount = gpGlobals->curtime;

		// in seconds
		m_iRoundTimeSecs = m_iRoundTime;

		// Update the timers for all clients and play a sound
		bool bCTPlayed = false;
		bool bTPlayed = false;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CCSPlayer *pPlayer = CCSPlayer::Instance( i );
			if ( pPlayer && !FNullEnt( pPlayer->pev ) )
			{
				if ( pPlayer->State_Get() == STATE_JOINED )
				{
					if ( (pPlayer->GetTeamNumber() == TEAM_CT) && !bCTPlayed )
					{
						pPlayer->Radio( CT_sentence, NULL );
						bCTPlayed = true;
					}
					else if ( (pPlayer->GetTeamNumber() == TEAM_TERRORIST) && !bTPlayed )
					{
						pPlayer->Radio( T_sentence, NULL );
						bTPlayed = true;
					}

					if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
					{
						pPlayer->ResetMaxSpeed();
					}
				}
				
				//pPlayer->SyncRoundTimer();
			}
		}
	}


	void CCSGameRules::CheckRoundTimeExpired()
	{
		if ( TimeRemaining() <= 0 && m_iRoundWinStatus == 0 ) //We haven't completed other objectives, so go for this!.
		{
			// Round time expired
			float flEndRoundTime;

			// Check to see if there's still a live C4 hanging around.. if so, wait until this C4 blows before ending the round
			if ( g_PlantedC4s.Count() > 0 )
			{
				CPlantedC4 *C4 = g_PlantedC4s[0];
				if ( C4->m_bJustBlew == false)
					flEndRoundTime = C4->m_flC4Blow;
				else
					flEndRoundTime = gpGlobals->curtime + 5.0;
			}
			else
				flEndRoundTime = gpGlobals->curtime + 5.0;

			// New code to get rid of round draws!!

			if ( m_bMapHasBombTarget == true )
			{
				Broadcast( "ctwin" );
				m_iAccountCT += 2000;
				
				m_iNumCTWins++;
				EndRoundMessage( "#Target_Saved", Target_Saved );
				TerminateRound( 5,1 );
				UpdateTeamScores();
			}
			else if ( gEntList.FindEntityByClassname( NULL, "hostage_entity" ) != NULL )
			{
				Broadcast( "terwin" );
				m_iAccountTerrorist += 2000; 
				
				m_iNumTerroristWins++;
				EndRoundMessage( "#Hostages_Not_Rescued", Hostages_Not_Rescued );
				TerminateRound( 5,2 );
				UpdateTeamScores();
			}
			else if ( m_bMapHasEscapeZone == true )
			{
				Broadcast( "ctwin" );
				
				m_iNumCTWins++;
				EndRoundMessage( "#Terrorists_Not_Escaped", Terrorists_Not_Escaped );
				TerminateRound( 5,1 );
				UpdateTeamScores();
			}
			else if ( m_iMapHasVIPSafetyZone == 1 )
			{
				Broadcast( "terwin" );
				m_iAccountTerrorist += 2000;
				
				m_iNumTerroristWins++;
				EndRoundMessage( "#VIP_Not_Escaped", VIP_Not_Escaped );

				TerminateRound( 5,2 );
				UpdateTeamScores();
			}


			// This is done so that the portion of code has enough time to do it's thing.
			m_fRoundCount = gpGlobals->curtime + 60;
		}
	}


	void CCSGameRules::CheckLevelInitialized()
	{
		if ( !m_bLevelInitialized )
		{
			// Count the number of spawn points for each team
			// This determines the maximum number of players allowed on each

			CBaseEntity* ent; 
			
			m_iSpawnPointCount_Terrorist	= 0;
			m_iSpawnPointCount_CT			= 0;

			ent = NULL;

			while ( ent = gEntList.FindEntityByClassname( ent, "info_player_deathmatch" ) )
				m_iSpawnPointCount_Terrorist++;

			while ( ent = gEntList.FindEntityByClassname( ent, "info_player_start" ) )
				m_iSpawnPointCount_CT++;


			m_bLevelInitialized = true;
		}
	}


	void CCSGameRules::CheckRestartRound()
	{
		// Restart the round if specified by the server
		int iRestartDelay = restartround.GetInt();
		if ( !iRestartDelay )
			iRestartDelay = sv_restart.GetInt();

		if ( iRestartDelay > 0 )
		{
			if ( iRestartDelay > 10 )
				iRestartDelay = 10;

			// log the restart
			UTIL_LogPrintf( "World triggered \"Restart_Round_(%i_%s)\"\n", iRestartDelay, iRestartDelay == 1 ? "second" : "seconds" );

			UTIL_LogPrintf( "Team \"CT\" scored \"%i\" with \"%i\" players\n", m_iNumCTWins, m_iNumCT );
			UTIL_LogPrintf( "Team \"TERRORIST\" scored \"%i\" with \"%i\" players\n", m_iNumTerroristWins, m_iNumTerrorist );

			// let the players know
			char strRestartDelay[64];
			Q_snprintf( strRestartDelay, sizeof( strRestartDelay ), "%d", iRestartDelay );
			UTIL_ClientPrintAll( HUD_PRINTCENTER, "#Game_will_restart_in", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS" );
			UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#Game_will_restart_in", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS" );

			m_fTeamCount = gpGlobals->curtime + iRestartDelay;
			m_bCompleteReset = true;
			restartround.SetValue( 0 );
			sv_restart.SetValue( 0 );
		}
	}


	void CCSGameRules::BalanceTeams()
	{
		int iTeamToSwap;
		int iNumToSwap;

		if (m_iMapHasVIPSafetyZone == 1) // The ratio for teams is different for Assasination maps
		{
			int iDesiredNumCT, iDesiredNumTerrorist;
			
			if ( (m_iNumCT + m_iNumTerrorist)%2 != 0)	// uneven number of players
				iDesiredNumCT			= (int)((m_iNumCT + m_iNumTerrorist) * 0.55) + 1;
			else
				iDesiredNumCT			= (int)((m_iNumCT + m_iNumTerrorist)/2);
			iDesiredNumTerrorist	= (m_iNumCT + m_iNumTerrorist) - iDesiredNumCT;

			if ( m_iNumCT < iDesiredNumCT )
			{
				iTeamToSwap = TEAM_TERRORIST;
				iNumToSwap = iDesiredNumCT - m_iNumCT;
			}
			else if ( m_iNumTerrorist < iDesiredNumTerrorist )
			{
				iTeamToSwap = TEAM_CT;
				iNumToSwap = iDesiredNumTerrorist - m_iNumTerrorist;
			}
			else
				return;
		}
		else
		{
			if (m_iNumCT > m_iNumTerrorist)
			{
				iTeamToSwap = TEAM_CT;
				iNumToSwap = (m_iNumCT - m_iNumTerrorist)/2;
			}
			else if (m_iNumTerrorist > m_iNumCT)
			{
				iTeamToSwap = TEAM_TERRORIST;
				iNumToSwap = (m_iNumTerrorist - m_iNumCT)/2;
			}
			else
			{
				return;	// Teams are even.. Get out of here.
			}
		}

		if (iNumToSwap > 4) // Don't swap more than 4 players at a time.. This is a naive method of avoiding infinite loops.
			iNumToSwap = 4;


		// last person to join the server
		int iHighestUserID   = 0;

		CCSPlayer *toSwap  = NULL;
		for (int i = 1; i <= iNumToSwap; i++)
		{
			iHighestUserID = 0;
			toSwap = NULL;

			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CCSPlayer *pPlayer = CCSPlayer::Instance( i );
				if ( pPlayer && !FNullEnt( pPlayer->pev ) )
				{
					if ( ( pPlayer->GetTeamNumber() == iTeamToSwap ) && 
						( engine->GetPlayerUserId( pPlayer->edict() ) > iHighestUserID ) && 
						( m_pVIP != pPlayer ) )
					{
						iHighestUserID = engine->GetPlayerUserId( pPlayer->edict() );
						toSwap = pPlayer;
					}
				}
			}

			if ( toSwap != NULL )
			{
				toSwap->SwitchTeam();
			}
		}
	}


	bool CCSGameRules::TeamFull( int team_id )
	{
		switch ( team_id )
		{
		case TEAM_TERRORIST:
			return m_iNumTerrorist >= m_iSpawnPointCount_Terrorist;

		case TEAM_CT:
			return m_iNumCT >= m_iSpawnPointCount_CT;
		}

		return false;
	}

	//checks to see if the desired team is stacked, returns true if it is
	bool CCSGameRules::TeamStacked( int newTeam_id, int curTeam_id  )
	{
		//players are allowed to change to their own team
		if(newTeam_id == curTeam_id)
			return(false);

		switch ( newTeam_id )
		{
		case TEAM_TERRORIST:
			if(curTeam_id != TEAM_UNASSIGNED && curTeam_id != TEAM_SPECTATOR)
			{
				if((m_iNumTerrorist + 1) > (m_iNumCT + limitteams.GetInt() - 1))
					return(true);
				else
					return(false);
			}
			else
			{
				if((m_iNumTerrorist + 1) > (m_iNumCT + limitteams.GetInt()))
					return(true);
				else
					return(false);
			}
			break;
		case TEAM_CT:
			if(curTeam_id != TEAM_UNASSIGNED && curTeam_id != TEAM_SPECTATOR)
			{
				if((m_iNumCT + 1) > (m_iNumTerrorist + limitteams.GetInt() - 1))
					return(true);
				else
					return(false);
			}
			else
			{
				if((m_iNumCT + 1) > (m_iNumTerrorist + limitteams.GetInt()))
					return(true);
				else
					return(false);
			}
			break;
		}

		return false;
	}


	//=========================================================
	//=========================================================
	bool CCSGameRules::FPlayerCanRespawn( CBasePlayer *pBasePlayer )
	{
		CCSPlayer *pPlayer = ToCSPlayer( pBasePlayer );
		if ( !pPlayer )
			Error( "FPlayerCanRespawn: pPlayer=0" );

		// Player cannot respawn twice in a round
		if ( pPlayer->m_iNumSpawns > 0 )
			return false;

		// If they're dead after the map has ended, and it's about to start the next round,
		// wait for the round restart to respawn them.
		if ( gpGlobals->curtime < m_fTeamCount )
			return false;

		// Player cannot respawn until next round if more than 20 seconds in

		// Tabulate the number of players on each team.
		m_iNumCT = GetGlobalTeam( TEAM_CT )->GetNumPlayers();
		m_iNumTerrorist = GetGlobalTeam( TEAM_TERRORIST )->GetNumPlayers();

		if ( m_iNumTerrorist > 0 && m_iNumCT > 0 )
		{
			if ( gpGlobals->curtime > m_fRoundCount + 20 )
			{
				//If this player just connected and fadetoblack is on, then maybe
				//the server admin doesn't want him peeking around.
				color32_s clr = {0,0,0,0};
				if ( fadetoblack.GetInt() != 0 )
					UTIL_ScreenFade( pPlayer, clr, 3, 3, FFADE_OUT | FFADE_STAYOUT );

				return false;
			}
		}

		// Player cannot respawn while in the Choose Appearance menu
		//if ( pPlayer->m_iMenu == Menu_ChooseAppearance )
		//	return false;

		return true;
	}


	void CCSGameRules::EndRoundMessage( const char* sentence, int event )
	{
		char * team = NULL;
		const char * message = &(sentence[1]);
		int teamTriggered = 1;
			
		UTIL_ClientPrintAll( HUD_PRINTCENTER, sentence );

		switch ( event )
		{
		case Target_Bombed:
		case VIP_Assassinated:
		case Terrorists_Escaped:
		case Terrorists_Win:
		case Hostages_Not_Rescued:
		case VIP_Not_Escaped:
			team = "TERRORIST";
			// tell bots the terrorists won the round
			//g_pBotControl->OnEvent( EVENT_TERRORISTS_WIN );
			break;
		case VIP_Escaped:
		case CTs_PreventEscape:
		case Escaping_Terrorists_Neutralized:
		case Bomb_Defused:
		case CTs_Win:
		case All_Hostages_Rescued:
		case Target_Saved:
		case Terrorists_Not_Escaped:
			team = "CT";
			// tell bots the CTs won the round
			//g_pBotControl->OnEvent( EVENT_CTS_WIN );
			break;
		case Round_Draw:
		default:
			teamTriggered = 0;
			// tell bots the round was a draw
			//g_pBotControl->OnEvent( EVENT_ROUND_DRAW );
			break;
		}

		if ( teamTriggered == 1 )
			UTIL_LogPrintf( "Team \"%s\" triggered \"%s\" (CT \"%i\") (T \"%i\")\n", team, message, m_iNumCTWins, m_iNumTerroristWins );
		else
			UTIL_LogPrintf( "World triggered \"%s\" (CT \"%i\") (T \"%i\")\n", message, m_iNumCTWins, m_iNumTerroristWins );

		UTIL_LogPrintf("World triggered \"Round_End\"\n");
	}


	void CCSGameRules::UpdateTeamScores()
	{
		// MIKETODO: put this data into the team entities.
		//Assert( false );
		/*
		MessageBegin( MSG_ALL, gmsgTeamScore );
			WRITE_STRING( "CT" );
			WRITE_SHORT( m_iNumCTWins );
		MessageEnd();

		MessageBegin( MSG_ALL, gmsgTeamScore );
			WRITE_STRING( "TERRORIST" );
			WRITE_SHORT( m_iNumTerroristWins );
		MessageEnd();
		*/
	}


	void CCSGameRules::CheckMapConditions()
	{
		// Check to see if this map has a bomb target in it
		if ( gEntList.FindEntityByClassname( NULL, "func_bomb_target" ) )
		{
			m_bMapHasBombTarget		= true;
			m_bMapHasBombZone		= true;
		}
		else if ( gEntList.FindEntityByClassname( NULL, "info_bomb_target" ) )
		{
			m_bMapHasBombTarget		= true;
			m_bMapHasBombZone		= false;
		}
		else
		{
			m_bMapHasBombTarget		= false;
			m_bMapHasBombZone		= false;
		}

		// See if the map has func_buyzone entities
		// Used by CBasePlayer::HandleSignals() to support maps without these entities
		if ( gEntList.FindEntityByClassname( NULL, "func_buyzone" ) )
		{
			m_bMapHasBuyZone = true;
		}
		else
		{
			m_bMapHasBuyZone = false;
		}

		// Check to see if this map has hostage rescue zones
		if ( gEntList.FindEntityByClassname( NULL, "func_hostage_rescue" ) )
		{
			m_bMapHasRescueZone = true;
		}
		else
		{
			m_bMapHasRescueZone = false;
		}

		// GOOSEMAN : See if this map has func_escapezone entities
		if ( gEntList.FindEntityByClassname( NULL, "func_escapezone" ) )
		{
			m_bMapHasEscapeZone = true;
		}
		else
		{
			m_bMapHasEscapeZone = false;
		}

		// Check to see if this map has VIP safety zones
		if ( gEntList.FindEntityByClassname( NULL, "func_vip_safetyzone" ) )
		{
			m_iMapHasVIPSafetyZone = 1;
		}
		else
		{
			m_iMapHasVIPSafetyZone = 2;
		}
	}


	void CCSGameRules::SwapAllPlayers()
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CCSPlayer *pPlayer = CCSPlayer::Instance( i );
			if ( pPlayer && !FNullEnt( pPlayer->pev ) )
				pPlayer->SwitchTeam();
		}

		// Swap Team victories
		int iTemp;

		iTemp = m_iNumCTWins;
		m_iNumCTWins = m_iNumTerroristWins;
		m_iNumTerroristWins = iTemp;
		
		// Update the clients team score
		UpdateTeamScores();
	}


	bool FindInList( const char **pStrings, int nStrings, const char *pToFind )
	{
		for ( int i=0; i < nStrings; i++ )
			if ( Q_stricmp( pStrings[i], pToFind ) == 0 )
				return true;

		return false;
	}


	void CCSGameRules::CleanUpMap()
	{
		// Recreate all the map entities from the map data (preserving their indices),
		// then remove everything else except the players.

		
		// These entities are preserved. The rest are removed and recreated.
		const char *preserveEnts[] =
		{
			"player",
			"cs_gamerules",
			"viewmodel",
			"worldspawn",
			"ai_network",
			"soundent"
		};


		// Get rid of all entities except players.
		CBaseEntity *pCur = gEntList.FirstEnt();
		while ( pCur )
		{
			if ( !FindInList( preserveEnts, ARRAYSIZE( preserveEnts ), pCur->GetClassname() ) )
			{
				// Weapons with owners don't want to be removed..
				CWeaponCSBase *pWeapon = dynamic_cast< CWeaponCSBase* >( pCur );
				if ( !pWeapon || pWeapon->ShouldRemoveOnRoundRestart() )
				{
					UTIL_Remove( pCur );
				}
			}

			pCur = gEntList.NextEnt( pCur );
		}
		gEntList.CleanupDeleteList();



		// Now reload the map entities.
		class CCSMapEntityFilter : public IMapEntityFilter
		{
		public:
			virtual bool ShouldCreateEntity( const char *pClassname )
			{
				// Don't recreate the worldspawn entity.
				if ( Q_stricmp( pClassname, "worldspawn" ) == 0 )
					return false;
				else
					return true;
			}
		};
		CCSMapEntityFilter filter;
		MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );


		// Get rid of all world decals.



		/*
		CBaseEntity* torestart = NULL;
		CBaseEntity* toremove = NULL;

		torestart = UTIL_FindEntityByClassname(NULL, "light");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "light");
		}

		torestart = UTIL_FindEntityByClassname(NULL, "func_breakable");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "func_breakable");
		}

		torestart = UTIL_FindEntityByClassname(NULL, "func_door");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "func_door");
		}

		torestart = UTIL_FindEntityByClassname(NULL, "func_water");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "func_water");
		}

		torestart = UTIL_FindEntityByClassname(NULL, "func_door_rotating");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "func_door_rotating");
		}

		torestart = UTIL_FindEntityByClassname(NULL, "func_tracktrain");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "func_tracktrain");
		}

		torestart = UTIL_FindEntityByClassname(NULL, "func_vehicle");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "func_vehicle");
		}

		torestart = UTIL_FindEntityByClassname(NULL, "func_train");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "func_train");
		}

		torestart = UTIL_FindEntityByClassname(NULL, "armoury_entity");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "armoury_entity");
		}

		torestart = UTIL_FindEntityByClassname(NULL, "ambient_generic");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "ambient_generic");
		}
		
		torestart = UTIL_FindEntityByClassname(NULL, "env_sprite");
		while (torestart != NULL)
		{
			torestart->Restart();
			torestart = UTIL_FindEntityByClassname(torestart, "env_sprite");
		}

		// Remove grenades and C4
		int icount = 0;
		toremove = UTIL_FindEntityByClassname(NULL, "grenade");
		while (	( toremove != NULL ) && ( icount < 20 ) )
		{
			UTIL_Remove(toremove);
			toremove = UTIL_FindEntityByClassname(toremove, "grenade");
			icount++;
		}

		//Remove defuse kit
		//Old code only removed 4 kits and stopped.
		toremove = UTIL_FindEntityByClassname(NULL, "item_thighpack");
		while (	(toremove != NULL) )
		{
			UTIL_Remove(toremove);
			toremove = UTIL_FindEntityByClassname(toremove, "item_thighpack");
		}

		RemoveGuns ();

		PLAYBACK_EVENT( FEV_GLOBAL | FEV_RELIABLE, 0, m_usResetDecals );
		*/
	}


	bool CCSGameRules::IsThereABomber()
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CCSPlayer *pPlayer = CCSPlayer::Instance( i );

			if ( pPlayer && !FNullEnt( pPlayer->pev ) )
			{
				if ( pPlayer->GetTeamNumber() == TEAM_CT )
					continue;

				if ( pPlayer->HasC4() )
					 return true; //There you are.
			}
		}

		//Didn't find a bomber.
		return false;
	}


	void CCSGameRules::cc_EndRound()
	{
		CSGameRules()->m_iRoundTimeSecs = 1;
	}


	CBaseEntity *CCSGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
	{
		CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();

		// Make sure the spawn spot isn't blocked...
		Vector vecTestOrg = pSpawnSpot->GetAbsOrigin();

		vecTestOrg.z += (pPlayer->GetAbsMaxs().z - pPlayer->GetAbsMins().z) * 0.5;
		Vector origin;
		EntityPlacementTest( pPlayer, vecTestOrg, origin, true );
		
		// Move the player to the place it said.
		pPlayer->Teleport( &origin, NULL, NULL );
		UTIL_Relink( pPlayer );

		pPlayer->SetAbsVelocity( vec3_origin );
		pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
		pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
		pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );
		
		return pSpawnSpot;
	}


	bool CCSGameRules::IsThereABomb()
	{
		return gEntList.FindEntityByClassname( NULL, WEAPON_C4_CLASSNAME ) != 0;
	}


#endif


float CCSGameRules::TimeRemaining()
{
	return (float)m_iRoundTimeSecs - gpGlobals->curtime + m_fRoundCount; 
}


float CCSGameRules::GetRoundStartTime()
{
	return m_fRoundCount;
}


float CCSGameRules::GetRoundElapsedTime()
{
	return gpGlobals->curtime - m_fRoundCount;
}


bool CCSGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
	}

	// Ignore base class HL2 definition for COLLISION_GROUP_WEAPON, change to COLLISION_GROUP_NONE
	if ( collisionGroup0 == COLLISION_GROUP_WEAPON )
	{
		collisionGroup0 = COLLISION_GROUP_NONE;
	}
	if ( collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		collisionGroup1 = COLLISION_GROUP_NONE;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}


bool CCSGameRules::IsFreezePeriod()
{
	return m_bFreezePeriod;
}


bool CCSGameRules::IsVIPMap() const
{
	//MIKETODO: VIP mode
	return false;
}


bool CCSGameRules::IsBombDefuseMap() const
{
	return true;
}


bool CCSGameRules::IsIntermission() const
{
	//MIKETODO: intermission mode
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Init TF2 ammo definitions
//-----------------------------------------------------------------------------

CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;
		
		def.AddAmmoType( BULLET_PLAYER_50AE, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_50AE_max", 0 );
		def.AddAmmoType( BULLET_PLAYER_762MM, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_762mm_max", 0 );
		def.AddAmmoType( BULLET_PLAYER_556MM, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_556mm_max", 0 );
		def.AddAmmoType( BULLET_PLAYER_338MAG, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_338mag_max", 0 );
		def.AddAmmoType( BULLET_PLAYER_9MM, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_9mm_max", 0 );
		def.AddAmmoType( BULLET_PLAYER_BUCKSHOT, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_buckshot_max", 0 );
		def.AddAmmoType( BULLET_PLAYER_45ACP, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_45acp_max", 0 );
		def.AddAmmoType( BULLET_PLAYER_357SIG, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_357sig_max", 0 );
		def.AddAmmoType( BULLET_PLAYER_57MM, DMG_BULLET, TRACER_LINE, 0, 0, "ammo_57mm_max", 0 );
		def.AddAmmoType( AMMO_TYPE_HEGRENADE, DMG_BLAST, TRACER_LINE, 0, 0, "ammo_hegrenade_max", 0 );
		def.AddAmmoType( AMMO_TYPE_FLASHBANG, 0, TRACER_LINE, 0, 0, "ammo_flashbang_max", 0 );
	}

	return &def;
}
