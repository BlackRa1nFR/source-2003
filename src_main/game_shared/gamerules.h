//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef GAMERULES_H
#define GAMERULES_H
#ifdef _WIN32
#pragma once
#endif


#ifdef CLIENT_DLL

	#include "c_baseentity.h"
	#define CGameRules C_GameRules

#else
	
	#include "baseentity.h"

#endif


//#include "items.h"
class CBaseCombatWeapon;
class CBaseCombatCharacter;
class CBasePlayer;
class CItem;
class CAmmoDef;

// weapon respawning return codes
enum
{	
	GR_NONE = 0,
	
	GR_WEAPON_RESPAWN_YES,
	GR_WEAPON_RESPAWN_NO,
	
	GR_AMMO_RESPAWN_YES,
	GR_AMMO_RESPAWN_NO,
	
	GR_ITEM_RESPAWN_YES,
	GR_ITEM_RESPAWN_NO,

	GR_PLR_DROP_GUN_ALL,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_NO,

	GR_PLR_DROP_AMMO_ALL,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_NO,
};

// Player relationship return codes
enum
{
	GR_NOTTEAMMATE = 0,
	GR_TEAMMATE,
	GR_ENEMY,
	GR_ALLY,
	GR_NEUTRAL,
};

class CGameRules : public CBaseEntity
{
public:
	DECLARE_CLASS( CGameRules, CBaseEntity );
	DECLARE_NETWORKCLASS();


	// Stuff shared between client and server.

	CGameRules(void);
	virtual ~CGameRules( void );

// Ammo Definitions
	//CAmmoDef* GetAmmoDef();

	virtual bool SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon ); // Switch to the next best weapon
	virtual CBaseCombatWeapon *GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon ); // I can't use this weapon anymore, get me the next best one.
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

#ifdef CLIENT_DLL

#else


// CBaseEntity overrides.
public:

	// ALWAYS transmit to all clients.
	virtual bool ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );
	
	// UNDONE: Is this correct, Mike?
	// Don't carry these across a transition, they are recreated.
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

// Setup
	
	// Called when game rules are created by CWorld
	virtual void LevelInit( void ) { return; };
	
	// Called when game rules are destroyed by CWorld
	virtual void LevelShutdown( void ) { return; };

	virtual void Precache( void ) { return; };

	virtual void RefreshSkillData( bool forceUpdate );// fill skill data struct with proper values
	virtual void Think( void ) = 0;// GR_Think - runs every server frame, should handle any timer tasks, periodic events, etc.
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity ) = 0;  // Can this item spawn (eg NPCs don't spawn in deathmatch).

	// Called at the end of GameFrame (i.e. after all game logic has run this frame)
	virtual void EndGameFrame( void );

	virtual bool FAllowFlashlight( void ) = 0;// Are players allowed to switch on their flashlight?
	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon ) = 0;// should the player switch to this weapon?

// Functions to verify the single/multiplayer status of a game
	virtual bool IsMultiplayer( void ) = 0;// is this a multiplayer game? (either coop or deathmatch)
	virtual bool IsDeathmatch( void ) = 0;//is this a deathmatch game?
	virtual bool IsTeamplay( void ) { return FALSE; };// is this deathmatch game being played with team rules?
	virtual bool IsCoOp( void ) = 0;// is this a coop game?
	virtual const char *GetGameDescription( void ) { return "Half-Life 2"; }  // this is the game name that gets seen in the server browser
	
// Client connection/disconnection
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] ) = 0;// a client just connected to the server (player hasn't spawned yet)
	virtual void InitHUD( CBasePlayer *pl ) = 0;		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient ) = 0;// a client just disconnected from the server
	virtual void UpdateGameMode( CBasePlayer *pPlayer ) {}  // the client needs to be informed of the current game mode

// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer ) = 0;// this client just hit the ground after a fall. How much damage?
	virtual bool  FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker ) {return TRUE;};// can this player take damage from this attacker?
	virtual bool ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) { return TRUE; }
	virtual void  RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore );
	// Let the game rules specify if fall death should fade screen to black
	virtual bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return TRUE; }

// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer ) = 0;// called by CBasePlayer::Spawn just before releasing player into the game
	virtual void PlayerThink( CBasePlayer *pPlayer ) = 0; // called by CBasePlayer::PreThink every frame, before physics are run and after keys are accepted
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer ) = 0;// is this player allowed to respawn now?
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer ) = 0;// When in the future will this player be able to spawn?
	virtual CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );// Place this player on their spawnspot and face them the proper direction.

	virtual bool AllowAutoTargetCrosshair( void ) { return TRUE; };
	virtual bool ClientCommand( const char *pcmd, CBaseEntity *pEdict );  // handles the user commands;  returns TRUE if command handled properly
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer ) {}		// the player has changed userinfo;  can change it now

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled ) = 0;// how many points do I award whoever kills this player?
	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info ) = 0;// Called each time a player dies
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )=  0;// Call this from within a GameRules class to report an obituary.
	virtual const char *GetCustomKillString( const CTakeDamageInfo &info ) { return NULL; }
// Weapon retrieval
	virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );// The player is touching an CBaseCombatWeapon, do I give it to him?

// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon ) = 0;// should this weapon respawn?
	virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon ) = 0;// when may this weapon respawn?
	virtual float FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon ) = 0; // can i respawn now,  and if not, when should i try again?
	virtual Vector VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon ) = 0;// where in the world should this weapon respawn?

// Item retrieval
	virtual bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;// is this player allowed to take this item?
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;// call each time a player picks up an item (battery, healthkit)

// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem ) = 0;// Should this item respawn?
	virtual float FlItemRespawnTime( CItem *pItem ) = 0;// when may this item respawn?
	virtual Vector VecItemRespawnSpot( CItem *pItem ) = 0;// where in the world should this item respawn?

// Ammo retrieval
	virtual bool CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex ); // can this player take more of this ammo?
	virtual bool CanHaveAmmo( CBaseCombatCharacter *pPlayer, const char *szName );
	virtual void PlayerGotAmmo( CBaseCombatCharacter *pPlayer, char *szName, int iCount ) = 0;// called each time a player picks up some ammo in the world

// AI Definitions
	virtual void			InitDefaultAIRelationships( void ) { return; }
	virtual const char*		AIClassText(int classType) { return NULL; }

// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void ) = 0;// how long until a depleted HealthCharger recharges itself?
	virtual float FlHEVChargerRechargeTime( void ) { return 0; }// how long until a depleted HealthCharger recharges itself?

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer ) = 0;// what do I do with a player's weapons when he's killed?

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer ) = 0;// Do I drop ammo when the player dies? How much?

// Teamplay stuff
	virtual const char *GetTeamID( CBaseEntity *pEntity ) = 0;// what team is this entity on?
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget ) = 0;// What is the player's relationship with this entity?
	virtual bool PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker ) = 0;

	virtual int GetTeamIndex( const char *pTeamName ) { return -1; }
	virtual const char *GetIndexedTeamName( int teamIndex ) { return ""; }
	virtual bool IsValidTeam( const char *pTeamName ) { return true; }
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib ) {}
	virtual const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer ) { return ""; }
	virtual void UpdateClientData( CBasePlayer *pPlayer ) { };

// Sounds
	virtual bool PlayTextureSounds( void ) { return TRUE; }
	virtual bool PlayFootstepSounds( CBasePlayer *pl ) { return TRUE; }

// NPCs
	virtual bool FAllowNPCs( void ) = 0;//are NPCs allowed

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) {}
				    
	// trace line rules
	virtual float WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd, unsigned int mask, trace_t *ptr );
#endif
};


#ifndef CLIENT_DLL
	CGameRules *InstallGameRules( void );
	
	// Create user messages for game here, calls into static player class creation functions
	void RegisterUserMessages( void );
#endif



//-----------------------------------------------------------------------------
// Gets us at the game rules
//-----------------------------------------------------------------------------

extern CGameRules *g_pGameRules;

inline CGameRules* GameRules()
{
	return g_pGameRules;
}

#endif // GAMERULES_H
