//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================

#ifndef HL1_PLAYER_H
#define HL1_PLAYER_H
#pragma once


#include "player.h"

#define	SOUND_FLASHLIGHT_ON		"items/flashlight1.wav"
#define	SOUND_FLASHLIGHT_OFF	"items/flashlight1.wav"

extern int TrainSpeed(int iSpeed, int iMax);
extern void CopyToBodyQue( CBaseAnimating *pCorpse );

enum HL1PlayerPhysFlag_e
{
	PFLAG_ONBARNACLE	= ( 1<<6 )		// player is hangning from the barnalce
};

class IPhysicsPlayerController;


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
// >> HL1_PLAYER
//=============================================================================
class CHL1_Player : public CBasePlayer
{
	DECLARE_CLASS( CHL1_Player, CBasePlayer );
	DECLARE_SERVERCLASS();
public:

	DECLARE_DATADESC();

	CHL1_Player();
	~CHL1_Player( void );

	static CHL1_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CHL1_Player::s_PlayerEdict = ed;
		return (CHL1_Player*)CreateEntityByName( className );
	}

	virtual void		CreateCorpse( void ) { CopyToBodyQue( this ); };

	virtual void		Precache( void );
	virtual void		Spawn(void);
	virtual void		Touch( CBaseEntity *pOther );
	virtual void		CheatImpulseCommands( int iImpulse );
	virtual bool		ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea );
	virtual void		PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper );
	
	// from cbasecombatcharacter
	void				InitVCollision( void );

	Class_T				Classify ( void );
	Class_T				m_nControlClass;			// Class when player is controlling another entity

	// from CBasePlayer
	void				SetupVisibility( unsigned char *pvs, unsigned char *pas );

	// Suit Power Interface
	virtual void		SuitPower_Update( void );
	virtual bool		SuitPower_Drain( float flPower ); // consume some of the suit's power.
	virtual void		SuitPower_Charge( float flPower ); // add suit power.
	virtual void		SuitPower_SetCharge( float flPower ) { m_flSuitPower = flPower; }
	virtual void		SuitPower_Initialize( void );
	virtual bool		SuitPower_AddDevice( const CSuitPowerDevice &device );
	virtual bool		SuitPower_RemoveDevice( const CSuitPowerDevice &device );
	virtual bool		SuitPower_ShouldRecharge( void ) { return m_flSuitPower < 100; }

	// Aiming heuristics accessors
	virtual float		GetIdleTime( void ) const { return ( m_flIdleTime - m_flMoveTime ); }
	virtual float		GetMoveTime( void ) const { return ( m_flMoveTime - m_flIdleTime ); }
	virtual float		GetLastDamageTime( void ) const { return m_flLastDamageTime; }
	virtual bool		IsDucking( void ) const { return !!( GetFlags() & FL_DUCKING ); }

	virtual int			OnTakeDamage( const CTakeDamageInfo &info );
	virtual void		FindMissTargets( void );
	virtual bool		GetMissPosition( Vector *position );

	virtual void		OnDamagedByExplosion( const CTakeDamageInfo &info ) { };
	void				PlayerPickupObject( CBasePlayer *pPlayer, CBaseEntity *pObject );
protected:
	virtual void		PreThink( void );
	virtual bool		HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

private:
	Vector				m_vecMissPositions[16];
	int					m_nNumMissPositions;

	// Aiming heuristics code
	float						m_flIdleTime;		//Amount of time we've been motionless
	float						m_flMoveTime;		//Amount of time we've been in motion
	float						m_flLastDamageTime;	//Last time we took damage
	float						m_flTargetFindTime;

public:

	// Flashlight Device
	int					FlashlightIsOn( void );
	void				FlashlightTurnOn( void );
	void				FlashlightTurnOff( void );
	float				m_flSuitPower;
	int					m_bitsActiveDevices;
	float				m_flSuitPowerLoad;	// net suit power drain (total of all device's drainrates)

	// For gauss weapon
	float m_flStartCharge;
	float m_flAmmoStartCharge;
	float m_flPlayAftershock;
	float m_flNextAmmoBurn;// while charging, when to absorb another unit of player's ammo?

	CNetworkVar( bool, m_bHasLongJump );
};


//-----------------------------------------------------------------------------
// Converts an entity to a HL1 player
//-----------------------------------------------------------------------------
inline CHL1_Player *ToHL1Player( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;
#if _DEBUG
	return dynamic_cast<CHL1_Player *>( pEntity );
#else
	return static_cast<CHL1_Player *>( pEntity );
#endif
}

class CGrabController : public IMotionEvent
{
public:
	CGrabController( void );
	~CGrabController( void );
	void AttachEntity( CBaseEntity *pEntity, IPhysicsObject *pPhys, const Vector &position, const QAngle &rotation );
	void DetachEntity( void );
	void SetTargetPosition( const Vector &target, const QAngle &targetOrientation )
	{
		m_shadow.targetPosition = target;
		m_shadow.targetRotation = targetOrientation;
		
		m_timeToArrive = gpGlobals->frametime;

		CBaseEntity *pAttached = GetAttached();
		if ( pAttached )
		{
			IPhysicsObject *pObj = pAttached->VPhysicsGetObject();
			
			if ( pObj != NULL )
			{
				pObj->Wake();
			}
			else
			{
				DetachEntity();
			}
		}
	}

	float ComputeError()
	{
		if ( m_errorTime <= 0 )
			return 0;

		CBaseEntity *pAttached = GetAttached();
		if ( pAttached )
		{
			Vector pos;
			IPhysicsObject *pObj = pAttached->VPhysicsGetObject();
			pObj->GetShadowPosition( &pos, NULL );
			float error = (m_shadow.targetPosition - pos).Length();
			m_errorTime = clamp(m_errorTime, 0, 1);
			m_error = (1-m_errorTime) * m_error + m_errorTime * error;
		}
		
		m_errorTime = 0;

		return m_error;
	}
	float GetLoadWeight( void ) const { return m_flLoadWeight; }

	CBaseEntity *GetAttached() { return (CBaseEntity *)m_attachedEntity; }

	IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );
	void SetMaxImpulse( const Vector &linear, const AngularImpulse &angular );
	
private:
	hlshadowcontrol_params_t	m_shadow;
	float			m_timeToArrive;
	float			m_errorTime;
	float			m_error;

	float			m_saveRotDamping;
	float			m_flLoadWeight;
	EHANDLE			m_attachedEntity;

	IPhysicsMotionController *m_controller;
};

#endif	//HL1_PLAYER_H
