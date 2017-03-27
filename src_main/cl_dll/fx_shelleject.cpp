//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "c_te_legacytempents.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ShellEjectCallback( const CEffectData &data )
{
	int iEntIndex = data.m_nEntIndex;

	// Use the gun angles to orient the shell
	C_BaseEntity *pEnt = ClientEntityList().GetEnt( iEntIndex );
	if ( pEnt )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pEnt->GetAbsAngles(), 0 );
	}
}

DECLARE_CLIENT_EFFECT( "ShellEject", ShellEjectCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RifleShellEjectCallback( const CEffectData &data )
{
	int iEntIndex = data.m_nEntIndex;

	// Use the gun angles to orient the shell
	C_BaseEntity *pEnt = ClientEntityList().GetEnt( iEntIndex );
	if ( pEnt )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pEnt->GetAbsAngles(), 1 );
	}
}

DECLARE_CLIENT_EFFECT( "RifleShellEject", RifleShellEjectCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ShotgunShellEjectCallback( const CEffectData &data )
{
	int iEntIndex = data.m_nEntIndex;

	// Use the gun angles to orient the shell
	C_BaseEntity *pEnt = ClientEntityList().GetEnt( iEntIndex );
	if ( pEnt )
	{
		tempents->EjectBrass( data.m_vOrigin, data.m_vAngles, pEnt->GetAbsAngles(), 2 );
	}
}

DECLARE_CLIENT_EFFECT( "ShotgunShellEject", ShotgunShellEjectCallback );


