/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== h_battery.cpp ========================================================

  battery-related code

*/

#include "cbase.h"
#include "gamerules.h"
#include "player.h"
#include "engine/IEngineSound.h"

ConVar	sk_suitcharger( "sk_suitcharger","0" );

class CRecharge : public CBaseToggle
{
public:
	DECLARE_CLASS( CRecharge, CBaseToggle );

	void Spawn( );
	bool CreateVPhysics();
	void Off(void);
	void Recharge(void);
	bool KeyValue( const char *szKeyName, const char *szValue );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return (BaseClass :: ObjectCaps() | FCAP_CONTINUOUS_USE); }

	DECLARE_DATADESC();

	float m_flNextCharge; 
	int		m_iReactivate ; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;
	
	COutputFloat m_OutRemainingCharge;
};

BEGIN_DATADESC( CRecharge )

	DEFINE_FIELD( CRecharge, m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( CRecharge, m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD( CRecharge, m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD( CRecharge, m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( CRecharge, m_flSoundTime, FIELD_TIME ),

	// Function Pointers
	DEFINE_FUNCTION( CRecharge, Off ),
	DEFINE_FUNCTION( CRecharge, Recharge ),

	DEFINE_OUTPUT(CRecharge, m_OutRemainingCharge, "OutRemainingCharge"),

END_DATADESC()


LINK_ENTITY_TO_CLASS(func_recharge, CRecharge);


bool CRecharge::KeyValue( const char *szKeyName, const char *szValue )
{
	if (	FStrEq(szKeyName, "style") ||
				FStrEq(szKeyName, "height") ||
				FStrEq(szKeyName, "value1") ||
				FStrEq(szKeyName, "value2") ||
				FStrEq(szKeyName, "value3"))
	{
	}
	else if (FStrEq(szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(szValue);
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

void CRecharge::Spawn()
{
	Precache( );

	SetSolid( SOLID_BSP );
	SetMoveType( MOVETYPE_PUSH );

	Relink();
	UTIL_SetSize(this, EntitySpaceMins(), EntitySpaceMaxs());
	SetModel( STRING( GetModelName() ) );
	m_iJuice = sk_suitcharger.GetFloat();
	SetTextureFrameIndex( 0 );

	CreateVPhysics();
}

bool CRecharge::CreateVPhysics()
{
	VPhysicsInitStatic();
	return true;
}

void CRecharge::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// if it's not a player, ignore
	if ( !pActivator->IsPlayer() )
		return;

	// if there is no juice left, turn it off
	if (m_iJuice <= 0)
	{
		SetTextureFrameIndex( 1 );
		Off();
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if ( m_iJuice <= 0 )
	{
		if (m_flSoundTime <= gpGlobals->curtime)
		{
			m_flSoundTime = gpGlobals->curtime + 0.62;
			EmitSound( "SuitRecharge.Deny" );
		}
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.25 );
	SetThink(Off);

	// Time to recharge yet?

	if (m_flNextCharge >= gpGlobals->curtime)
		return;

	// Make sure that we have a caller
	if (!pActivator)
		return;

	m_hActivator = pActivator;

	//only recharge the player

	if (!m_hActivator->IsPlayer() )
		return;
	
	// Play the on sound or the looping charging sound
	if (!m_iOn)
	{
		m_iOn++;
		EmitSound( "SuitRecharge.Start" );
		m_flSoundTime = 0.56 + gpGlobals->curtime;
	}
	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->curtime))
	{
		m_iOn++;
		CPASAttenuationFilter filter( this, "SuitRecharge.ChargingLoop" );
		filter.MakeReliable();
		EmitSound( filter, entindex(), "SuitRecharge.ChargingLoop" );
	}

	CBasePlayer *pl = (CBasePlayer *) m_hActivator.Get();

	// charge the player
	if (pl->ArmorValue() < 100)
	{
		m_iJuice--;
		pl->IncrementArmorValue( 1, 100 );
	}

	// Send the output.
	float flRemaining = m_iJuice / sk_suitcharger.GetFloat();
	m_OutRemainingCharge.Set(flRemaining, pActivator, this);

	// govern the rate of charge
	m_flNextCharge = gpGlobals->curtime + 0.1;
}

void CRecharge::Recharge(void)
{
	m_iJuice = sk_suitcharger.GetFloat();
	SetTextureFrameIndex( 0 );
	SetThink( SUB_DoNothing );
}

void CRecharge::Off(void)
{
	// Stop looping sound.
	if (m_iOn > 1)
	{
		StopSound( "SuitRecharge.ChargingLoop" );
	}

	m_iOn = 0;

	if ((!m_iJuice) &&  ( ( m_iReactivate = g_pGameRules->FlHEVChargerRechargeTime() ) > 0) )
	{
		SetNextThink( gpGlobals->curtime + m_iReactivate );
		SetThink(Recharge);
	}
	else
		SetThink( NULL );
}