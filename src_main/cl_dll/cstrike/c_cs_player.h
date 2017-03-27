//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef C_CS_PLAYER_H
#define C_CS_PLAYER_H
#ifdef _WIN32
#pragma once
#endif


#include "cs_playeranimstate.h"
#include "c_baseplayer.h"
#include "cs_shareddefs.h"


class C_CSPlayer : public C_BasePlayer
{
public:
	DECLARE_CLASS( C_CSPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();

	C_CSPlayer();

	virtual int DrawModel( int flags );
	virtual void AddEntity();

	bool IsShieldDrawn() const;
	bool HasShield() const;
	bool HasC4() const;	// Is this player carrying a C4 bomb?
	
	static C_CSPlayer* GetLocalCSPlayer();
	CSPlayerState State_Get() const;

	virtual float GetMinFOV() const;
	
	// Get how much $$$ this guy has.
	int GetAccount() const;

	// Returns one of the CS_CLASS_ enums.
	int PlayerClass() const;

	// Returns true if we're allowed to show the buy menu right now.
	bool CanShowBuyMenu() const;
	bool CanShowTeamMenu() const;

	// Returns true if the player is allowed to move.
	bool CanMove() const;

	// Get the amount of armor the player has.
	int ArmorValue() const;

	virtual const QAngle& GetRenderAngles();

	
// Implemented in shared code.
public:	

	Vector FireBullets3( 
		Vector vecSrc, 
		const QAngle &shootAngles, 
		float vecSpread, 
		float flDistance, 
		int iPenetration, 
		int iBulletType, 
		int iDamage, 
		float flRangeModifier, 
		CBaseEntity *pevAttacker );

	void GetBulletTypeParameters( 
		int iBulletType, 
		int &iPenetrationPower, 
		float &flPenetrationDistance,
		int &iSparksAmount, 
		int &iCurrentDamage );


public:
	CPlayerAnimState m_PlayerAnimState;

	// Predicted variables.
	bool m_bResumeZoom;
	int m_iLastZoom;			// after firing a shot, set the FOV to 90, and after showing the animation, bring the FOV back to last zoom level.
	float m_flEjectBrass;		// time to eject a shell.. this is used for guns that do not eject shells right after being shot.
	CNetworkVar( CSPlayerState, m_iPlayerState );	// SupraFiend: this gives the current state in the joining process, the states are listed above
	bool m_bIsDefusing;			// tracks whether this player is currently defusing a bomb
	CNetworkVar( bool, m_bInBombZone );
	CNetworkVar( bool, m_bInBuyZone );	
	
	// How long the progress bar takes to get to the end. If this is 0, then the progress bar
	// should not be drawn.
	short m_ProgressBarTime;
	
	// When the progress bar should start.
	float m_ProgressBarStartTime;


private:

	CNetworkVar( int, m_iAccount );	
	CNetworkVar( int, m_iClass );
	CNetworkVar( int, m_ArmorValue );
	CNetworkVar( float, m_flEyeAnglesPitch );


private:
	C_CSPlayer( const C_CSPlayer & );
};


inline CCSPlayer *ToCSPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CCSPlayer*>( pEntity );
}


#endif // C_CS_PLAYER_H
