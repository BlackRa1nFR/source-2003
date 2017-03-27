//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TAKEDAMAGEINFO_H
#define TAKEDAMAGEINFO_H
#ifdef _WIN32
#pragma once
#endif


#include "networkvar.h" // todo: change this when DECLARE_CLASS is moved into a better location.


class CBaseEntity;


class CTakeDamageInfo
{
public:
	DECLARE_CLASS_NOBASE( CTakeDamageInfo );

					CTakeDamageInfo();
					CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType = 0 );
					CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL );
	

	// Inflictor is the weapon or rocket (or player) that is dealing the damage.
	CBaseEntity*	GetInflictor() const;
	void			SetInflictor( CBaseEntity *pInflictor );

	// Attacker is the character who originated the attack (like a player or an AI).
	CBaseEntity*	GetAttacker() const;
	void			SetAttacker( CBaseEntity *pAttacker );

	float			GetDamage() const;
	void			SetDamage( float flDamage );
	float			GetMaxDamage() const;
	void			SetMaxDamage( float flMaxDamage );
	void			ScaleDamage( float flScaleAmount );

	Vector			GetDamageForce() const;
	void			SetDamageForce( const Vector &damageForce );

	Vector			GetDamagePosition() const;
	void			SetDamagePosition( const Vector &damagePosition );

	Vector			GetReportedPosition() const;
	void			SetReportedPosition( const Vector &reportedPosition );

	int				GetDamageType() const;
	void			SetDamageType( int bitsDamageType );
	void			AddDamageType( int bitsDamageType );

	int				GetCustomKill() const;
	void			SetCustomKill( int iKillType );

protected:
	void			Init( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillType );

	Vector			m_vecDamageForce;
	Vector			m_vecDamagePosition;
	Vector			m_vecReportedPosition;	// Position players are told damage is coming from
	EHANDLE			m_hInflictor;
	EHANDLE			m_hAttacker;
	float			m_flDamage;
	float			m_flMaxDamage;
	int				m_bitsDamageType;
	int				m_iCustomKillType;
};

//-----------------------------------------------------------------------------
// Purpose: Multi damage. Used to collect multiple damages in the same frame (i.e. shotgun pellets)
//-----------------------------------------------------------------------------
class CMultiDamage : public CTakeDamageInfo
{
	DECLARE_CLASS( CMultiDamage, CTakeDamageInfo );
public:
	CMultiDamage();

	bool			IsClear( void ) { return (m_hTarget == NULL); }
	CBaseEntity		*GetTarget() const;
	void			SetTarget( CBaseEntity *pTarget );

	void			Init( CBaseEntity *pTarget, CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillType );

protected:
	EHANDLE			m_hTarget;
};

extern CMultiDamage g_MultiDamage;

// Multidamage accessors
void ClearMultiDamage( void );
void ApplyMultiDamage( void );
void AddMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity );

//-----------------------------------------------------------------------------
// Purpose: Utility functions for physics damage force calculation 
//-----------------------------------------------------------------------------
float ImpulseScale( float flTargetMass, float flDesiredSpeed );
void CalculateExplosiveDamageForce( CTakeDamageInfo *info, const Vector &vecDir, const Vector &vecForceOrigin, float flScale = 1.0 );
void CalculateBulletDamageForce( CTakeDamageInfo *info, int iBulletType, const Vector &vecBulletDir, const Vector &vecForceOrigin, float flScale = 1.0 );
void CalculateMeleeDamageForce( CTakeDamageInfo *info, const Vector &vecMeleeDir, const Vector &vecForceOrigin, float flScale = 1.0 );
void GuessDamageForce( CTakeDamageInfo *info, const Vector &vecForceDir, const Vector &vecForceOrigin, float flScale = 1.0 );

// -------------------------------------------------------------------------------------------------- //
// Inlines.
// -------------------------------------------------------------------------------------------------- //

inline CBaseEntity* CTakeDamageInfo::GetInflictor() const
{
	return m_hInflictor;
}


inline void CTakeDamageInfo::SetInflictor( CBaseEntity *pInflictor )
{
	m_hInflictor = pInflictor;
}


inline CBaseEntity* CTakeDamageInfo::GetAttacker() const
{
	return m_hAttacker;
}


inline void CTakeDamageInfo::SetAttacker( CBaseEntity *pAttacker )
{
	m_hAttacker = pAttacker;
}


inline float CTakeDamageInfo::GetDamage() const
{
	return m_flDamage;
}


inline void CTakeDamageInfo::SetDamage( float flDamage )
{
	m_flDamage = flDamage;
}

inline float CTakeDamageInfo::GetMaxDamage() const
{
	return m_flMaxDamage;
}

inline void CTakeDamageInfo::SetMaxDamage( float flMaxDamage )
{
	m_flMaxDamage = flMaxDamage;
}


inline void CTakeDamageInfo::ScaleDamage( float flScaleAmount )
{
	m_flDamage *= flScaleAmount;
}


inline Vector CTakeDamageInfo::GetDamageForce() const
{
	return m_vecDamageForce;
}


inline void CTakeDamageInfo::SetDamageForce( const Vector &damageForce )
{
	m_vecDamageForce = damageForce;
}


inline Vector CTakeDamageInfo::GetDamagePosition() const
{
	return m_vecDamagePosition;
}


inline void CTakeDamageInfo::SetDamagePosition( const Vector &damagePosition )
{
	m_vecDamagePosition = damagePosition;
}

inline Vector CTakeDamageInfo::GetReportedPosition() const
{
	return m_vecReportedPosition;
}


inline void CTakeDamageInfo::SetReportedPosition( const Vector &reportedPosition )
{
	m_vecReportedPosition = reportedPosition;
}

inline int CTakeDamageInfo::GetDamageType() const
{
	return m_bitsDamageType;
}


inline void CTakeDamageInfo::SetDamageType( int bitsDamageType )
{
	m_bitsDamageType = bitsDamageType;
}

inline void	CTakeDamageInfo::AddDamageType( int bitsDamageType )
{
	m_bitsDamageType |= bitsDamageType;
}

inline int	CTakeDamageInfo::GetCustomKill() const
{
	return m_iCustomKillType;
}

inline void CTakeDamageInfo::SetCustomKill( int iKillType )
{
	m_iCustomKillType = iKillType;
}

// -------------------------------------------------------------------------------------------------- //
// Inlines.
// -------------------------------------------------------------------------------------------------- //
inline CBaseEntity *CMultiDamage::GetTarget() const
{
	return m_hTarget;
}

inline void CMultiDamage::SetTarget( CBaseEntity *pTarget )
{
	m_hTarget = pTarget;
}


#endif // TAKEDAMAGEINFO_H
