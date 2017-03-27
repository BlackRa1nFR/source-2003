//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef WEAPON_C4_H
#define WEAPON_C4_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_csbase.h"
#include "utlvector.h"


#if defined( CLIENT_DLL )

	#define CC4 C_C4

#else

	// ------------------------------------------------------------------------------------------ //
	// CPlantedC4 class.
	// ------------------------------------------------------------------------------------------ //

	class CPlantedC4 : public CBaseAnimating
	{
	public:
		DECLARE_CLASS( CPlantedC4, CBaseAnimating );
		DECLARE_DATADESC();

		CPlantedC4();
		virtual ~CPlantedC4();
		
		static CPlantedC4* ShootSatchelCharge( CCSPlayer *pevOwner, Vector vecStart, QAngle vecAngles );
		virtual void Precache();
		
		// Set these flags so CTs can use the C4 to disarm it.
		virtual int	ObjectCaps() { return BaseClass::ObjectCaps() | (FCAP_CONTINUOUS_USE | FCAP_USE_IN_RADIUS); }


	public:

		bool m_bJustBlew;
		float m_flC4Blow;


	private:

		void Init( CCSPlayer *pevOwner, Vector vecStart, QAngle vecAngles );
		void C4Think();
		
		// This becomes the think function when the timer has expired and it is about to explode.
		void Detonate2();
		void Explode2( trace_t *pTrace, int bitsDamageType );

		void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );


	private:

		float m_flNextDefuse;
		bool m_bStartDefuse;

		// Info for blinking.
		float m_flNextBlink;

		// Info for the beeping.
		float m_flNextFreqInterval;
		float m_flNextFreq;
		float m_flNextBeep;
		int m_iCurWave;				// Which sound we're playing.
		const char *m_sBeepName;

		// Info for defusing.
		CHandle<CCSPlayer> m_pBombDefuser;
		float m_flDefuseCountDown;
		float m_fNextDefuse;
	};

	extern CUtlVector< CPlantedC4* > g_PlantedC4s;

#endif


#define WEAPON_C4_CLASSNAME "weapon_c4"


class CC4 : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CC4, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CC4();
	virtual ~CC4();

	bool IsPistol() const;
	void ItemPostFrame();
	virtual void PrimaryAttack();
	virtual void WeaponIdle();

	#ifdef CLIENT_DLL

	#else
		
		virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );
		virtual bool ShouldRemoveOnRoundRestart();
	
	#endif


	CNetworkVar( bool, m_bStartedArming );
	float m_fArmedTime;
	bool m_bBombPlacedAnimation;


private:
	
	CC4( const CC4 & );
};


// All the currently-active C4 grenades.
extern CUtlVector< CC4* > g_C4s;


#endif // WEAPON_C4_H
