//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef WEAPON_KNIFE_H
#define WEAPON_KNIFE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_csbase.h"


#if defined( CLIENT_DLL )

	#define CKnife C_Knife

#endif


// ----------------------------------------------------------------------------- //
// CKnife class definition.
// ----------------------------------------------------------------------------- //

class CKnife : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CKnife, CWeaponCSBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	#ifndef CLIENT_DLL
		DECLARE_DATADESC();
	#endif

	
	CKnife();

	// We say yes to this so the weapon system lets us switch to it.
	virtual bool HasPrimaryAmmo();
	virtual bool CanBeSelected();
	
	void Spawn();
	void SwingAgain();
	void Smack();
	void PrimaryAttack();
	void SecondaryAttack();
	void WeaponAnimation( int iAnimation );
	int Stab (int fFirst );
	int Swing( int fFirst );
	bool Deploy();
	void Holster( int skiplocal = 0 );
	bool CanDrop();

	void WeaponIdle();


public:
	
	trace_t m_trHit;
	CNetworkVar( int, m_iSwing );


private:
	CKnife( const CKnife & ) {}
};


#endif // WEAPON_KNIFE_H
