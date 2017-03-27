//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEGRENADE_SHARED_H
#define BASEGRENADE_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#if defined( CLIENT_DLL )

#define CBaseGrenade C_BaseGrenade

#include "c_basecombatcharacter.h"

#else

#include "basecombatcharacter.h"

#endif

#define BASEGRENADE_EXPLOSION_VOLUME	1024

class CTakeDamageInfo;

class CBaseGrenade : public CBaseCombatCharacter
{
	DECLARE_CLASS( CBaseGrenade, CBaseCombatCharacter );
public:

	CBaseGrenade(void);
	~CBaseGrenade(void);

	DECLARE_PREDICTABLE();
	DECLARE_NETWORKCLASS();


#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	virtual void		Precache( void );

	virtual void		Explode( trace_t *pTrace, int bitsDamageType );
	void				Smoke( void );

	void				BounceTouch( CBaseEntity *pOther );
	void				SlideTouch( CBaseEntity *pOther );
	void				ExplodeTouch( CBaseEntity *pOther );
	void				DangerSoundThink( void );
	void				PreDetonate( void );
	virtual void		Detonate( void );
	void				DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void				TumbleThink( void );

	virtual Vector		GetBlastForce() { return vec3_origin; }

	virtual void		BounceSound( void );
	virtual int			BloodColor( void ) { return DONT_BLEED; }
	virtual void		Event_Killed( const CTakeDamageInfo &info );

	virtual float		GetShakeAmplitude( void ) { return 25.0; }
	virtual float		GetShakeRadius( void ) { return 750.0; }

	// Damage accessors.
	virtual float GetDamage()
	{
		return m_flDamage;
	}
	virtual float GetDamageRadius()
	{
		return m_DmgRadius;
	}

	virtual void SetDamage(float flDamage)
	{
		m_flDamage = flDamage;
	}

	virtual void SetDamageRadius(float flDamageRadius)
	{
		m_DmgRadius = flDamageRadius;
	}

	// Bounce sound accessors.
	void SetBounceSound( const char *pszBounceSound ) 
	{
		m_iszBounceSound = MAKE_STRING( pszBounceSound );
	}

	CBaseCombatCharacter *GetOwner( void );
	void				  SetOwner( CBaseCombatCharacter *pOwner );


public:
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_vecVelocity );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_fFlags );
	
	bool				m_fRegisteredSound;			// whether or not this grenade has issued its DANGER sound to the world sound list yet.
	CNetworkVar( bool, m_bIsLive );					// Is this grenade live, or can it be picked up?
	CNetworkVar( float, m_DmgRadius );				// How far do I do damage?
	float				m_flDetonateTime;			// Time at which to detonate.

protected:

	CNetworkVar( float, m_flDamage );		// Damage to inflict.
	string_t m_iszBounceSound;	// The sound to make on bouncing.  If not NULL, overrides the BounceSound() function.

private:
	CNetworkHandle( CBaseEntity, m_hOwner );					// Who set owned this grenade

	CBaseGrenade( const CBaseGrenade & ); // not defined, not accessible

};

#endif // BASEGRENADE_SHARED_H
