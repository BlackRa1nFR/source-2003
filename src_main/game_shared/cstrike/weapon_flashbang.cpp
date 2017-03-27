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
	
	#define CFlashbang C_Flashbang

#else

	#include "cs_player.h"
	#include "items.h"
	#include "flashbang_projectile.h"

#endif


#define GRENADE_TIMER	3.0f //Seconds


//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CFlashbang : public CBaseCSGrenade
{
public:
	DECLARE_CLASS( CFlashbang, CWeaponCSBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CFlashbang() {}

#ifdef CLIENT_DLL

#else
	DECLARE_DATADESC();

	virtual void ThrowGrenade();

#endif

	CFlashbang( const CFlashbang & ) {}
};


IMPLEMENT_NETWORKCLASS_ALIASED( Flashbang, DT_Flashbang )

BEGIN_NETWORK_TABLE(CFlashbang, DT_Flashbang)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CFlashbang )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_flashbang, CFlashbang );
PRECACHE_WEAPON_REGISTER( weapon_flashbang );


#ifndef CLIENT_DLL

	BEGIN_DATADESC( CFlashbang )
	END_DATADESC()

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *pPlayer - 
	//-----------------------------------------------------------------------------
	void CFlashbang::ThrowGrenade()
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
		CFlashbangProjectile::Create( 
			vecSrc, 
			vec3_angle, 
			vecThrow, 
			AngularImpulse(600,random->RandomInt(-1200,1200),0), 
			pPlayer, 
			GRENADE_TIMER );

		m_bRedraw = true;
	}

#endif

