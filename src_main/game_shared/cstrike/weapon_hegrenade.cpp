//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbase.h"
#include "gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "weapon_basecsgrenade.h"


#ifdef CLIENT_DLL
	
	#define CHEGrenade C_HEGrenade

#else

	#include "cs_player.h"
	#include "items.h"
	#include "hegrenade_projectile.h"

#endif


#define GRENADE_TIMER	3.0f //Seconds


//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CHEGrenade : public CBaseCSGrenade
{
public:
	DECLARE_CLASS( CHEGrenade, CWeaponCSBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CHEGrenade() {}

#ifdef CLIENT_DLL

#else
	DECLARE_DATADESC();

	virtual void ThrowGrenade();

#endif

	CHEGrenade( const CHEGrenade & ) {}
};


IMPLEMENT_NETWORKCLASS_ALIASED( HEGrenade, DT_HEGrenade )

BEGIN_NETWORK_TABLE(CHEGrenade, DT_HEGrenade)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CHEGrenade )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_hegrenade, CHEGrenade );
PRECACHE_WEAPON_REGISTER( weapon_hegrenade );


#ifndef CLIENT_DLL

	BEGIN_DATADESC( CHEGrenade )
	END_DATADESC()

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *pPlayer - 
	//-----------------------------------------------------------------------------
	void CHEGrenade::ThrowGrenade()
	{
		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
		if ( !pPlayer )
		{
			Assert( false );
			return;
		}

		Vector	vecSrc = pPlayer->EyePosition();
		Vector	vForward, vRight;

		pPlayer->EyeVectors( &vForward, &vRight, NULL );
		vecSrc += vForward * 18.0f + vRight * 8.0f;

		vForward[2] += 0.1f;

		Vector vecThrow = vForward * 1200 + pPlayer->GetAbsVelocity();
		Msg( "Threw a grenade!\n" );
		CHEGrenadeProjectile::Create( vecSrc, vec3_angle, vecThrow, AngularImpulse(600,random->RandomInt(-1200,1200),0), pPlayer, GRENADE_TIMER );

		m_bRedraw = true;
	}

#endif

