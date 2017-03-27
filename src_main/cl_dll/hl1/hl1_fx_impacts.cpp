//========= Copyright © 1996-2003, Valve Corporation, All rights reserved. ============
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================
#include "cbase.h"
#include "fx_impact.h"


//-----------------------------------------------------------------------------
// Purpose: Handle weapon impacts
//-----------------------------------------------------------------------------
void ImpactCallback( const CEffectData &data )
{
	trace_t tr;
	Vector vecOrigin, vecStart, vecShotDir;
	int iMaterial, iDamageType, iHitbox, iEntIndex;
	ParseImpactData( data, &vecOrigin, &vecStart, &vecShotDir, &iMaterial, &iDamageType, &iHitbox, &iEntIndex );

	// If we hit, perform our custom effects and play the sound
	if ( Impact( vecOrigin, vecStart, iMaterial, iDamageType, iHitbox, iEntIndex, tr ) )
	{
		// Check for custom effects based on the Decal index
		PerformCustomEffects( vecOrigin, tr, vecShotDir, iMaterial, 1.0 );
		PlayImpactSound( tr );
	}
}

DECLARE_CLIENT_EFFECT( "Impact", ImpactCallback );
