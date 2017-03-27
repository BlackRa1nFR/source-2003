//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef PROPS_H
#define PROPS_H
#ifdef _WIN32
#pragma once
#endif

#include "baseanimating.h"

// Phys prop spawnflags
#define SF_PHYSPROP_START_ASLEEP				0x0001
#define SF_PHYSPROP_DONT_TAKE_PHYSICS_DAMAGE	0x0002		// this prop can't be damaged by physics collisions
#define SF_PHYSPROP_DEBRIS						0x0004
#define SF_PHYSPROP_MOTIONDISABLED				0x0008		// motion disabled at startup (flag only valid in spawn - motion can be enabled via input)
#define	SF_PHYSPROP_TOUCH						0x0010		// can be 'crashed through' by running player (plate glass)
#define SF_PHYSPROP_PRESSURE					0x0020		// can be broken by a player standing on it
#define SF_PHYSPROP_ENABLE_ON_PHYSCANNON		0x0040		// enable motion only if the player grabs it with the physcannon

// ParsePropData returns
enum
{
	PARSE_SUCCEEDED,
	PARSE_FAILED_NO_DATA,
	PARSE_FAILED_BAD_DATA,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseProp : public CBaseAnimating
{
	DECLARE_CLASS( CBaseProp, CBaseAnimating );
public:

	void Spawn( void );
	void Precache( void );
	void Activate( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	int  ParsePropData( void );
	
	void DrawDebugGeometryOverlays( void );

	// Don't treat as a live target
	virtual bool IsAlive( void ) { return false; }
	virtual bool OverridePropdata() { return true; }

public:
	string_t	m_iszBasePropData;	// Used for debugging
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBreakableProp : public CBaseProp
{
	typedef CBaseProp BaseClass;
public:
	DECLARE_CLASS( CBreakableProp, CBaseProp );
	DECLARE_SERVERCLASS();

	virtual void Spawn();
	virtual void Precache();

public:

	void BreakablePropTouch( CBaseEntity *pOther );

	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );

	void Break( CBaseEntity *pBreaker, const CTakeDamageInfo *pDamageInfo );
	void FadeOut( void );
	void BreakThink( void );
	void StartFadeOut( float delay );

	void InputBreak( inputdata_t &inputdata );
	void InputAddHealth( inputdata_t &inputdata );
	void InputRemoveHealth( inputdata_t &inputdata );
	void InputSetHealth( inputdata_t &inputdata );

	virtual bool OverridePropdata() { return false; }

	DECLARE_DATADESC();

	COutputEvent	m_OnBreak;
	COutputInt		m_OnHealthChanged;

	float			m_impactEnergyScale;

	float			m_explodeDamage;
	float			m_explodeRadius;
	int				m_iMinHealthDmg;

	// Damage modifiers
	void			SetDmgModBullet( float flDmgMod ) { m_flDmgModBullet = flDmgMod; }
	void			SetDmgModClub( float flDmgMod ) { m_flDmgModClub = flDmgMod; }
	void			SetDmgModExplosive( float flDmgMod ) { m_flDmgModExplosive = flDmgMod; }
	float			GetDmgModBullet( void ) { return m_flDmgModBullet; }
	float			GetDmgModClub( void ) { return m_flDmgModClub; }
	float			GetDmgModExplosive( void ) { return m_flDmgModExplosive; }

protected:
	unsigned int	m_createTick;
	float			m_flPressureDelay;
	EHANDLE			m_hBreaker;

	// Damage modifiers
	float			m_flDmgModBullet;
	float			m_flDmgModClub;
	float			m_flDmgModExplosive;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDynamicProp : public CBreakableProp
{
	DECLARE_CLASS( CDynamicProp, CBreakableProp );

public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

			CDynamicProp();
	void	Spawn( void );
	bool	CreateVPhysics( void );
	void	AnimThink( void );
	void	PropSetSequence( int nSequence );

	// Input handlers
	void InputSetAnimation( inputdata_t &inputdata );
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );

	COutputEvent		m_pOutputAnimBegun;
	COutputEvent		m_pOutputAnimOver;

	// Random animations
	bool				m_bRandomAnimator;
	float				m_flNextRandAnim;
	float				m_flMinRandAnimTime;
	float				m_flMaxRandAnimTime;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPhysicsProp : public CBreakableProp
{
	DECLARE_CLASS( CPhysicsProp, CBreakableProp );
	DECLARE_SERVERCLASS();

public:
	CPhysicsProp( void ) 
	{
		NetworkStateManualMode( true );
	}

	void Spawn( void );
	bool CreateVPhysics( void );

	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics )
	{
		BaseClass::VPhysicsUpdate( pPhysics );
		NetworkStateChanged();
	}

	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	void InputWake( inputdata_t &inputdata );
	void InputSleep( inputdata_t &inputdata );
	void InputEnableMotion( inputdata_t &inputdata );
	void InputDisableMotion( inputdata_t &inputdata );

	void EnableMotion( void );
	void OnPhysGunPickup( CBasePlayer *pPhysGunUser );

	int ObjectCaps() 
	{ 
		int caps = BaseClass::ObjectCaps();
		if ( CBasePlayer::CanPickupObject( this, 35, 128 ) )
		{
			caps |= FCAP_IMPULSE_USE;
		}
		return caps;
	}
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pActivator );
		if ( pPlayer )
		{
			pPlayer->PickupObject( this );
		}
	}

	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	int DrawDebugTextOverlays(void);

	DECLARE_DATADESC();

	COutputEvent m_MotionEnabled;

	float		m_massScale;
	float		m_inertiaScale;
	int			m_damageType;
	string_t	m_iszOverrideScript;
	int			m_damageToEnableMotion;
	float		m_flForceToEnableMotion;

protected:
	CNetworkVar( float, m_fadeMinDist );
	CNetworkVar( float, m_fadeMaxDist );

};

struct breakablepropparams_t
{
	breakablepropparams_t( const Vector &_origin, const QAngle &_angles, const Vector &_velocity, const Vector &_angularVelocity )
		: origin(_origin), angles(_angles), velocity(_velocity), angularVelocity(_angularVelocity)
	{
		impactEnergyScale = 0;
		defBurstScale = 0;
		defCollisionGroup = COLLISION_GROUP_NONE;
	}
	const Vector &origin;
	const QAngle &angles;
	const Vector &velocity;
	const AngularImpulse &angularVelocity;
	float impactEnergyScale;
	float defBurstScale;
	int defCollisionGroup;
};

float GetBreakableDamage( const CTakeDamageInfo &inputInfo, CBreakableProp *pProp = NULL );
void PropBreakablePrecacheAll( string_t modelName );
void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const breakablepropparams_t &params );
void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const Vector &origin, const QAngle &angles, const Vector &velocity, const AngularImpulse &angularVelocity, float impactEnergyScale, float burstScale, int collisionGroup );

#endif // PROPS_H
