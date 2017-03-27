//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		SLAM 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	WEAPONSLAM_H
#define	WEAPONSLAM_H

#include "basegrenade_shared.h"
#include "basehlcombatweapon.h"

enum SlamState_t
{
	SLAM_TRIPMINE_READY,
	SLAM_SATCHEL_THROW,
	SLAM_SATCHEL_ATTACH,
};

class CWeapon_SLAM : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeapon_SLAM, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

	SlamState_t			m_tSlamState;
	bool				m_bDetonatorArmed;
	bool				m_bNeedDetonatorDraw;
	bool				m_bNeedDetonatorHolster;
	bool				m_bNeedReload;
	bool				m_bClearReload;
	bool				m_bThrowSatchel;
	bool				m_bAttachSatchel;
	bool				m_bAttachTripmine;
	float				m_flWallSwitchTime;

	void				Spawn( void );
	void				Precache( void );

	int					CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	void				PrimaryAttack( void );
	void				SecondaryAttack( void );
	void				WeaponIdle( void );
	void				WeaponSwitch( void );
	void				SLAMThink( void );
	
//	void				SetPickupTouch( void );
//	void				SlamTouch( CBaseEntity *pOther );	// default weapon touch
	void				ItemPostFrame( void );	
	bool				Reload( void );
	void				SetSlamState( SlamState_t newState );
	bool				CanAttachSLAM(void);		// In position where can attach SLAM?
	bool				AnyUndetonatedCharges(void);
	void				StartTripmineAttach( void );
	void				TripmineAttach( void );

	void				StartSatchelDetonate( void );
	void				SatchelDetonate( void );
	void				StartSatchelThrow( void );
	void				StartSatchelAttach( void );
	void				SatchelThrow( void );
	void				SatchelAttach( void );
	bool				Deploy( void );
	bool				Holster( CBaseCombatWeapon *pSwitchingTo = NULL );


	CWeapon_SLAM();

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
};


#endif	//WEAPONSLAM_H
