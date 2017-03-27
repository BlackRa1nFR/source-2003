//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================

#ifndef CS_PLAYER_H
#define CS_PLAYER_H
#pragma once


#include "player.h"
#include "server_class.h"
#include "cs_playeranimstate.h"
#include "cs_shareddefs.h"


class CWeaponCSBase;
class CMenu;


#define MENU_STRING_BUFFER_SIZE	1024
#define MENU_MSG_TEXTCHUNK_SIZE	50


// Function table for each player state.
class CCSPlayerStateInfo
{
public:
	CSPlayerState m_iPlayerState;
	const char *m_pStateName;
	
	void (CCSPlayer::*pfnEnterState)();	// Init and deinit the state.
	void (CCSPlayer::*pfnLeaveState)();

	void (CCSPlayer::*pfnPreThink)();	// Do a PreThink() in this state.
};



//=============================================================================
// >> CounterStrike player
//=============================================================================
class CCSPlayer : public CBasePlayer
{
	DECLARE_CLASS( CCSPlayer, CBasePlayer );
	DECLARE_SERVERCLASS();
public:

	CCSPlayer();
	~CCSPlayer();

	static CCSPlayer *CreatePlayer( const char *className, edict_t *ed );
	static CCSPlayer* Instance( int iEnt );

	virtual void		Precache();
	virtual void		Spawn();
	void PostSpawn();
	
	virtual void		Touch( CBaseEntity *pOther );
	virtual void		CheatImpulseCommands( int iImpulse );
	virtual bool		ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );
	virtual void		PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper );
	virtual void		SetAnimation( PLAYER_ANIM playerAnim );
	virtual void		PostThink();

	virtual int			OnTakeDamage( const CTakeDamageInfo &info );
	
	virtual Vector		FireBullets3( 
		Vector vecSrc, 
		const QAngle &shootAngles, 
		float vecSpread, 
		float flDistance, 
		int iPenetration, 
		int iBulletType, 
		int iDamage, 
		float flRangeModifier, 
		CBaseEntity *pevAttacker );
	
	virtual void		Event_Killed( const CTakeDamageInfo &info );
	virtual void		TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr );

	void GetBulletTypeParameters( 
		int iBulletType, 
		int &iPenetrationPower, 
		float &flPenetrationDistance,
		int &iSparksAmount, 
		int &iCurrentDamage );

	// Returns true if the player is allowed to move.
	bool CanMove() const;

	// from cbasecombatcharacter
	void				InitVCollision( void );

	bool IsCarryingBomb() const;
	bool HasShield() const;
	bool IsShieldDrawn() const;

	void GiveDefaultItems();

	// Reset account, get rid of shield, etc..
	void Reset();

	void RoundRespawn();

	// Add money to this player's account.
	void AddAccount( int amount, bool bTrackChange=true );
	void BlinkAccount( int nBlinks );

	bool HasC4() const;	// Is this player carrying a C4 bomb?
	void DropC4();	// Get rid of the C4 bomb.
	
	bool HasDefuser();	// Is this player carrying a bomb defuser?

	// Have this guy speak a message into his radio.
	void Radio( const char *msg_id, const char *msg_verbose );

	void ResetMaxSpeed();

	CWeaponCSBase* GetActiveCSWeapon();

	// Switch the player's team without killing him.
	void SwitchTeam();

	void PreThink();

	// This is the think function for the player when they first join the server and have to select a team
	void JoiningThink();

	bool ClientCommand( const char *cmd );
	bool HandleMenu_ChooseTeam( int slot );
	bool HandleMenu_ChooseClass( int iClass );
	void HandleCommand_JoinTeam( int slot );

	// Returns one of the CS_CLASS_ enums.
	int PlayerClass() const;

	void MoveToNextIntroCamera();

	// Go into observer mode (floating around, watching players, etc).
	virtual bool StartObserverMode( Vector vecPosition, QAngle vecViewAngle );

	virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex=0 );

	// Used to be GETINTOGAME state.
	void GetIntoGame();

	CBaseEntity* EntSelectSpawnPoint();

	void SetProgressBarTime( int barTime );
	virtual void PlayerDeathThink();

	void Weapon_Equip( CBaseCombatWeapon *pWeapon );
	virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon );


// ------------------------------------------------------------------------------------------------ //
// Player state management.
// ------------------------------------------------------------------------------------------------ //
public:

	void State_Transition( CSPlayerState newState );	// Cleanup the previous state and enter a new state.
	CSPlayerState State_Get() const;				// Get the current state.

private:
	void State_Enter( CSPlayerState newState );		// Initialize the new state.
	void State_Leave();								// Cleanup the previous state.
	void State_PreThink();							// Update the current state.
	
	// Find the state info for the specified state.
	static CCSPlayerStateInfo* State_LookupInfo( CSPlayerState state );

	// This tells us which state the player is currently in (joining, observer, dying, etc).
	// Each state has a well-defined set of parameters that go with it (ie: observer is movetype_noclip, nonsolid,
	// invisible, etc).
	CNetworkVar( CSPlayerState, m_iPlayerState );

	CCSPlayerStateInfo *m_pCurStateInfo;			// This can be NULL if no state info is defined for m_iPlayerState.


	// Specific state handler functions.
	void State_Enter_SHOWLTEXT();
	void State_PreThink_SHOWLTEXT();

	void State_Enter_PICKINGTEAM();
	void State_Leave_PICKINGTEAM();

	void State_Enter_PICKINGCLASS();
	void State_PreThink_ShowIntroCameras();

	void State_Enter_JOINED();
	void State_PreThink_JOINED();

	void State_Enter_OBSERVER_MODE();
	void State_PreThink_OBSERVER_MODE();

	void State_Enter_DEATH_WAIT_FOR_KEY();
	void State_PreThink_DEATH_WAIT_FOR_KEY();

	void State_Enter_DEATH_ANIM();
	void State_PreThink_DEATH_ANIM();



public:
	
	// Predicted variables.
	bool m_bResumeZoom;
	int m_iLastZoom;			// after firing a shot, set the FOV to 90, and after showing the animation, bring the FOV back to last zoom level.
	float m_flEjectBrass;		// time to eject a shell.. this is used for guns that do not eject shells right after being shot.
	bool m_bIsDefusing;			// tracks whether this player is currently defusing a bomb
	int m_LastHitGroup;			// the last body region that took damage
	int m_iKevlar;				// What type of armour does this player have (0 = none)
	bool m_bEscaped;			// Has this terrorist escaped yet?
	
	
	// Other variables.
	bool m_bNotKilled;			// Has the player been killed or has he just respawned.
	bool m_bIsVIP;				// Are we the VIP?
	int m_iNumSpawns;			// Number of times player has spawned this round
	bool m_bTeamChanged;		// SupraFiend: this is set to false every death. Players are restricted to one team change per death (when dead)
	CNetworkVar( int, m_iAccount );	// How much cash this player has.
	
	bool m_bJustKilledTeammate;
	bool m_bPunishedForTK;

	// Backup copy of the menu text so the player can change this and the menu knows when to update.
	char	m_MenuStringBuffer[MENU_STRING_BUFFER_SIZE];

	CNetworkVar( int, m_iClass ); // One of the CS_CLASS_ enums.

	// When the player joins, it cycles their view between trigger_camera entities.
	// This is the current camera, and the time that we'll switch to the next one.
	EHANDLE m_pIntroCamera;
	float m_fIntroCamTime;

	// If nonzero, the client will draw the progress bar.
	CNetworkVar( int, m_ProgressBarTime );
	
	// Set to true each frame while in a bomb zone.
	// Reset after prediction (in PostThink).
	CNetworkVar( bool, m_bInBombZone );

	CNetworkVar( bool, m_bInBuyZone );
	float m_flBuyZonePulseTime;			// Last time we set m_bInBuyZone to true.

	// Make sure to register changes for armor.
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_ArmorValue );


protected:

	bool IsHittingShield( const Vector &vecDirection, trace_t *ptr );


private:

	bool SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot );
	void ShowVGUIMenu( int MenuType );

	void CSWeaponDrop( CBaseCombatWeapon *pWeapon );
	void DropRifle();
	void DropPistol();

	void AttemptToBuyVest();
	void AttemptToBuyAssaultSuit();
	
	bool HandleBuyAliasCommands( const char *pszCommand );
	bool IsInBuyZone();
	bool CanPlayerBuy( bool display );

	bool BuyAmmo( int nSlot, bool bBlinkMoney );
	bool BuyGunAmmo( CBaseCombatWeapon *pWeapon, bool bBlinkMoney );


private:

	// Aiming heuristics code
	float						m_flIdleTime;		//Amount of time we've been motionless
	float						m_flMoveTime;		//Amount of time we've been in motion
	float						m_flLastDamageTime;	//Last time we took damage
	float						m_flTargetFindTime;

	// Copyed from EyeAngles() so we can send it to the client.
	CNetworkVar( float, m_flEyeAnglesPitch );

	bool m_bVCollisionInitted;

	CPlayerAnimState m_PlayerAnimState;
};


inline CSPlayerState CCSPlayer::State_Get() const
{
	return m_iPlayerState;
}

inline CCSPlayer *ToCSPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CCSPlayer*>( pEntity );
}


#endif	//CS_PLAYER_H
