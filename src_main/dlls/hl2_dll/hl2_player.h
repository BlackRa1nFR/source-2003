//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Player for HL2.
//
// $NoKeywords: $
//=============================================================================

#ifndef HL2_PLAYER_H
#define HL2_PLAYER_H
#pragma once


#include "player.h"
#include "hl2_playerlocaldata.h"

extern int TrainSpeed(int iSpeed, int iMax);
extern void CopyToBodyQue( CBaseAnimating *pCorpse );

enum HL2PlayerPhysFlag_e
{
	PFLAG_ONBARNACLE	= ( 1<<6 )		// player is hangning from the barnalce
};

class IPhysicsPlayerController;

//----------------------------------------------------
// Definitions for weapon slots
//----------------------------------------------------
#define	WEAPON_MELEE_SLOT			0
#define	WEAPON_SECONDARY_SLOT		1
#define	WEAPON_PRIMARY_SLOT			2
#define	WEAPON_EXPLOSIVE_SLOT		3
#define	WEAPON_TOOL_SLOT			4

//=============================================================================
//=============================================================================
class CSuitPowerDevice
{
public:
	CSuitPowerDevice( int bitsID, float flDrainRate ) { m_bitsDeviceID = bitsID; m_flDrainRate = flDrainRate; }
private:
	int		m_bitsDeviceID;	// tells what the device is. DEVICE_SPRINT, DEVICE_FLASHLIGHT, etc. BITMASK!!!!!
	float	m_flDrainRate;	// how quickly does this device deplete suit power? ( percent per second )

public:
	int		GetDeviceID( void ) const { return m_bitsDeviceID; }
	float	GetDeviceDrainRate( void ) const { return m_flDrainRate; }
};


//=============================================================================
// >> HL2_PLAYER
//=============================================================================
class CHL2_Player : public CBasePlayer
{
public:
	DECLARE_CLASS( CHL2_Player, CBasePlayer );

	CHL2_Player();
	~CHL2_Player( void );
	
	static CHL2_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CHL2_Player::s_PlayerEdict = ed;
		return (CHL2_Player*)CreateEntityByName( className );
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void		CreateCorpse( void ) { CopyToBodyQue( this ); };

	virtual void		Precache( void );
	virtual void		Spawn(void);
	virtual void		Touch( CBaseEntity *pOther );
	virtual void		CheatImpulseCommands( int iImpulse );
	virtual bool		ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );
	virtual void		PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper);
	virtual void		ItemPostFrame( void );
	virtual void		PlayerUse ( void );
	virtual void		UpdateClientData( void );

	virtual Vector		EyeDirection2D( void );
	virtual Vector		EyeDirection3D( void );

	virtual void		CommanderMode();

	virtual bool		ClientCommand(const char *cmd);

	virtual void		GetInVehicle( IServerVehicle *pVehicle, int nRole );

	// from cbasecombatcharacter
	void				InitVCollision( void );
	int					CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );

	Class_T				Classify ( void );

	// from CBasePlayer
	void				SetupVisibility( unsigned char *pvs, unsigned char *pas );

	// Suit Power Interface
	virtual void		SuitPower_Update( void );
	virtual bool		SuitPower_Drain( float flPower ); // consume some of the suit's power.
	virtual void		SuitPower_Charge( float flPower ); // add suit power.
	virtual void		SuitPower_SetCharge( float flPower ) { m_HL2Local.m_flSuitPower = flPower; }
	virtual void		SuitPower_Initialize( void );
	virtual bool		SuitPower_AddDevice( const CSuitPowerDevice &device );
	virtual bool		SuitPower_RemoveDevice( const CSuitPowerDevice &device );
	virtual bool		SuitPower_ShouldRecharge( void ) { return m_bitsActiveDevices == 0x00000000 && m_HL2Local.m_flSuitPower < 100; }

	// Commander Mode for controller NPCs
	bool SelectNPC( CAI_BaseNPC *pNPC );
	int	 SelectAllAllies();
	bool IsInCommanderMode() { return m_fCommanderMode; }
	void SetCommanderMode( bool set );
	void CancelCommanderMode() { UnselectAllNPCs(); SetCommanderMode( false ); }
	void UnselectAllNPCs();
	void CommanderExecute();


	// Sprint Device
	virtual	void		StartSprinting( void );
	virtual	void		StopSprinting( void );
	virtual void		InitSprinting( void );
	virtual bool		IsSprinting( void ) { return m_fIsSprinting; }

	// Walking
	virtual	void		StartWalking( void );
	virtual	void		StopWalking( void );
	virtual bool		IsWalking( void ) { return m_fIsWalking; }

	// Aiming heuristics accessors
	virtual float		GetIdleTime( void ) const { return ( m_flIdleTime - m_flMoveTime ); }
	virtual float		GetMoveTime( void ) const { return ( m_flMoveTime - m_flIdleTime ); }
	virtual float		GetLastDamageTime( void ) const { return m_flLastDamageTime; }
	virtual bool		IsDucking( void ) const { return !!( GetFlags() & FL_DUCKING ); }

	virtual int			OnTakeDamage( const CTakeDamageInfo &info );
	bool				ShouldShootMissTarget( CBaseCombatCharacter *pAttacker );


	virtual int			GiveAmmo( int nCount, int nAmmoIndex, bool bSuppressSound);
	virtual bool		BumpWeapon( CBaseCombatWeapon *pWeapon );
	
	virtual void		Weapon_Equip( CBaseCombatWeapon *pWeapon );
	virtual bool		Weapon_Lower( void );
	virtual bool		Weapon_Ready( void );
	virtual bool		Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );

	// Flashlight Device
	int					FlashlightIsOn( void );
	void				FlashlightTurnOn( void );
	void				FlashlightTurnOff( void );

	// physics interactions
	virtual void		PickupObject( CBaseEntity *pObject );
	virtual void		ForceDropOfCarriedPhysObjects( void );

	Class_T				m_nControlClass;			// Class when player is controlling another entity
	// This player's HL2 specific data that should only be replicated to 
	//  the player and not to other players.
	CNetworkVarEmbedded( CHL2PlayerLocalData, m_HL2Local );

	virtual bool		IsFollowingPhysics( void ) { return (m_afPhysicsFlags & PFLAG_ONBARNACLE) > 0; }

protected:
	virtual void		PreThink( void );
	virtual bool		HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
	virtual void		UpdateWeaponPosture( void );

private:
	bool				CommanderExecuteOne( CAI_BaseNPC *pNpc, const trace_t &trace );

	bool				m_fIsSprinting;
	bool				m_fIsWalking;
	bool				m_fCommanderMode;
	int					m_iNumSelectedNPCs;

	Vector				m_vecMissPositions[16];
	int					m_nNumMissPositions;

	// Suit power fields
	int							m_bitsActiveDevices;
	float						m_flSuitPowerLoad;	// net suit power drain (total of all device's drainrates)

	// Aiming heuristics code
	float						m_flIdleTime;		//Amount of time we've been motionless
	float						m_flMoveTime;		//Amount of time we've been in motion
	float						m_flLastDamageTime;	//Last time we took damage
	float						m_flTargetFindTime;
};

#endif	//HL2_PLAYER_H


