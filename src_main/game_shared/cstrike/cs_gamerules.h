//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef CS_GAMERULES_H
#define CS_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif


#include "teamplay_gamerules.h"
#include "convar.h"
#include "cs_shareddefs.h"

#ifdef CLIENT_DLL
	#include "c_cs_player.h"
#else
	#include "cs_player.h"
#endif



// used for EndRoundMessage() logged messages
#define Target_Bombed							1
#define VIP_Escaped								2
#define VIP_Assassinated						3
#define Terrorists_Escaped						4
#define CTs_PreventEscape						5
#define Escaping_Terrorists_Neutralized			6
#define Bomb_Defused							7
#define CTs_Win									8
#define Terrorists_Win							9
#define Round_Draw								10
#define All_Hostages_Rescued					11
#define Target_Saved							12
#define Hostages_Not_Rescued					13
#define Terrorists_Not_Escaped					14
#define VIP_Not_Escaped							15


extern ConVar startmoney;
extern ConVar tkpunish;
extern ConVar allow_spectators;
extern ConVar fadetoblack;
extern ConVar c4timer;
extern ConVar buytime;


#ifdef CLIENT_DLL
	#define CCSGameRules C_CSGameRules
#endif


class CCSGameRules : public CTeamplayRules
{
public:
	DECLARE_CLASS( CCSGameRules, CTeamplayRules );
	DECLARE_NETWORKCLASS();


	// Stuff that is shared between client and server.
	bool IsFreezePeriod();


	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

	float TimeRemaining();
	float GetRoundStartTime();		// When this round started.
	float GetRoundElapsedTime();	// How much time has elapsed since the round started.

	bool IsVIPMap() const;
	bool IsBombDefuseMap() const;
	bool IsIntermission() const;


private:

	CNetworkVar( bool, m_bFreezePeriod );		// TRUE at beginning of round, set to FALSE when the period expires
	CNetworkVar( int, m_iRoundTimeSecs );		// (Current) round timer - set to m_iIntroRoundTime, then m_iRoundTime.
	CNetworkVar( float, m_fRoundCount );		


public:

#ifdef CLIENT_DLL


#else
	
	CCSGameRules();
	virtual ~CCSGameRules();

	CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );

	static void cc_EndRound();

	virtual void Think();

	// Called when game rules are created by CWorld
	virtual void LevelInit( void );
	// Called when game rules are destroyed by CWorld
	virtual void LevelShutdown( void );

	virtual bool ClientCommand( const char *pcmd, CBaseEntity *pEdict  );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );

	virtual bool PlayTextureSounds( void ) { return true; }
	// Let the game rules specify if fall death should fade screen to black
	virtual bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return FALSE; }

	virtual void UpdateClientData( CBasePlayer *pl );
	
	// Death notices
	virtual void		DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );

	virtual void			InitDefaultAIRelationships( void );

	virtual const char *GetGameDescription( void ) { return "TeamFortress 2"; }  // this is the game name that gets seen in the server browser
	virtual const char *AIClassText(int classType);

	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );

	virtual const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer );

	// Called after the map has finished loading.
	virtual void Activate();

	// Recreate all the map entities from the map data (preserving their indices),
	// then remove everything else except the players.
	// Also get rid of all world decals.
	void CleanUpMap();

	void CheckFreezePeriodExpired();
	void CheckRoundTimeExpired();

	void CheckWinConditions();
	void TerminateRound( float tmDelay, int iWinStatus );
	void RestartRound();
	void BalanceTeams();
	bool TeamFull( int team_id );
	bool TeamStacked( int newTeam_id, int curTeam_id  );
	bool FPlayerCanRespawn( CBasePlayer *pPlayer );
	void EndRoundMessage( const char* sentence, int event );
	void UpdateTeamScores();
	void CheckMapConditions();

	// Check various conditions to end the map.
	bool CheckGameOver();
	bool CheckTimeLimit();
	bool CheckMaxRounds();
	bool CheckWinLimit();

	void CheckLevelInitialized();
	void CheckRestartRound();
	

	// Checks if it still needs players to start a round, or if it has enough players to start rounds.
	// Starts a round and returns true if there are enough players.
	bool NeededPlayersCheck( bool &bNeededPlayers );

	// Setup counts for m_iNumTerrorist, m_iNumCT, m_iNumSpawnableTerrorist, m_iNumSpawnableCT, etc.
	void InitializePlayerCounts(
		int &NumAliveTerrorist,
		int &NumAliveCT,
		int &NumDeadTerrorist,
		int &NumDeadCT
		);

	// Check to see if the round is over for the various game types. Terminates the round
	// and returns true if the round should end.
	bool PrisonRoundEndCheck();
	bool BombRoundEndCheck( bool bNeededPlayers );
	bool HostageRescueRoundEndCheck( bool bNeededPlayers );

	// Check to see if the teams exterminated each other. Ends the round and returns true if so.
	bool TeamExterminationCheck(
		int NumAliveTerrorist,
		int NumAliveCT,
		int NumDeadTerrorist,
		int NumDeadCT,
		bool bNeededPlayers
		);

	void ReadMultiplayCvars();
	void SwapAllPlayers();



	// VIP FUNCTIONS
	bool VIPRoundEndCheck( bool bNeededPlayers );
	void PickNextVIP();


	// BOMB MAP FUNCTIONS
	void GiveC4( bool bDoSecondPass=true );
	bool IsThereABomber();
	bool IsThereABomb();


public:

	virtual void	SetAllowWeaponSwitch( bool allow );
	virtual bool	GetAllowWeaponSwitch( void );

	// VARIABLES FOR ALL TYPES OF MAPS
	bool m_bLevelInitialized;
	int m_iRoundWinStatus;		// 1 == CT's won last round, 2 == Terrorists did, 3 == Draw, no winner
	int m_iTotalRoundsPlayed;
	int m_iUnBalancedRounds;	// keeps track of the # of consecutive rounds that have gone by where one team outnumbers the other team by more than 2
	bool m_bRoundTerminating;
	
	int m_iIntroRoundTime;		// (From mp_freezetime) - How many seconds long the intro round (when players are frozen) is.
	int m_iRoundTime;			// (From mp_roundtime) - How many seconds long this round is.
	float m_fIntroRoundCount;	// The global time when the intro round ends and the real one starts
								// wrote the original "m_flRoundTime" comment for this variable).
	float m_flIntermissionStartTime;

	float m_flResetTime;		// When the round started.
	float m_flTimeLimit;		// Time limit for the current round.

	float m_fTeamCount;			// m_flRestartRoundTime, the global time when the round is supposed to end, if this is not 0

	int m_iNumTerrorist;		// The number of terrorists on the team (this is generated at the end of a round)
	int m_iNumCT;				// The number of CTs on the team (this is generated at the end of a round)
	int m_iNumSpawnableTerrorist;
	int m_iNumSpawnableCT;

	float m_fMaxIdlePeriod;		//SupraFiend: For the idle kick functionality. This is tha max amount of time that the player has to be idle before being kicked

	bool m_bFirstConnected;
	bool m_bCompleteReset;		// Set to TRUE to have the scores reset next time round restarts

	int m_iAccountTerrorist;
	int m_iAccountCT;

	short m_iNumCTWins;
	short m_iNumTerroristWins;

	int m_iNumConsecutiveCTLoses;		//SupraFiend: the number of rounds the CTs have lost in a row.
	int m_iNumConsecutiveTerroristLoses;//SupraFiend: the number of rounds the Terrorists have lost in a row.

	int m_iSpawnPointCount_Terrorist;		// Number of Terrorist spawn points
	int m_iSpawnPointCount_CT;				// Number of CT spawn points

	bool m_bTCantBuy;			// Who can and can't buy.
	bool m_bCTCantBuy;
	bool m_bMapHasBuyZone;

	int m_iLoserBonus;			// SupraFiend: the amount of money the losing team gets. This scales up as they lose more rounds in a row
	float m_tmNextPeriodicThink;
	
	
	// HOSTAGE RESCUE VARIABLES
	bool m_bMapHasRescueZone;
	int m_iHostagesRescued;
	int m_iHostagesTouched;


	// PRISON ESCAPE VARIABLES
	int m_iHaveEscaped;
	bool m_bMapHasEscapeZone;
	int m_iNumEscapers;			
	int m_iNumEscapeRounds;		// keeps track of the # of consecutive rounds of escape played.. Teams will be swapped after 8 rounds

	
	// VIP VARIABLES
	int m_iMapHasVIPSafetyZone;	// 0 = uninitialized;   1 = has VIP safety zone;   2 = DOES not have VIP safetyzone
	CHandle<CCSPlayer> m_pVIP;
	int m_iConsecutiveVIP;


	// BOMB MAP VARIABLES
	bool m_bTargetBombed;	// whether or not the bomb has been bombed
	bool m_bBombDefused;	// whether or not the bomb has been defused

	bool m_bMapHasBombTarget;
	bool m_bMapHasBombZone;

	bool m_bBombDropped;
	int m_iC4Guy;			// The current Terrorist who has the C4.


private:

	// Don't allow switching weapons while gaining new technologies
	bool			m_bAllowWeaponSwitch;

#endif
};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CCSGameRules* CSGameRules()
{
	return static_cast<CCSGameRules*>(g_pGameRules);
}



//-----------------------------------------------------------------------------
// Purpose: Useful utility functions
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL

#else
	
	class CTFTeam;
	CTFTeam *GetOpposingTeam( CTeam *pTeam );
	bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround );

#endif

#endif // TF_GAMERULES_H
