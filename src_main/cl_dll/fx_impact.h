//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef FX_IMPACT_H
#define FX_IMPACT_H
#ifdef _WIN32
#pragma once
#endif

#include "c_te_effect_dispatch.h"
#include "istudiorender.h"

#define	CLIENT_SHOT_FORCE	4000

// Parse the impact data from the server's data block
void ParseImpactData( const CEffectData &data, Vector *vecOrigin, Vector *vecStart, Vector *vecShotDir, int *iMaterial, int *iDamageType, int *iHitbox, int *iEntIndex );

// Get the decal name to use based on an impact with the specified entity, surface material, and damage type
char const *GetImpactDecal( C_BaseEntity *pEntity, int iMaterial, int iDamageType );

// Try and hit clientside ragdolls along this shot's path
bool AffectRagdolls( Vector vecOrigin, Vector vecStart, int iDamageType );

// Create a leak effect if the material specifies one
void LeakEffect( trace_t &tr );

// Basic decal handling
// Returns true if it hit something
bool Impact( Vector &vecOrigin, Vector &vecStart, int iMaterial, int iDamageType, int iHitbox, int iEntIndex, trace_t &tr, bool bDecal = true, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );

// Do spiffy things according to the material hit
void PerformCustomEffects( Vector &vecOrigin, trace_t &tr, Vector &shotDir, int iMaterial, int iScale );

// Play the correct impact sound according to the material hit
void PlayImpactSound( trace_t &tr );

//-----------------------------------------------------------------------------
// Purpose: Enumerator class for ragdolls being affected by bullet forces
//-----------------------------------------------------------------------------
class CRagdollEnumerator : public IPartitionEnumerator
{
public:
	// Forced constructor
	CRagdollEnumerator( Ray_t& shot, float force, int iDamageType );

	// Actual work code
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );

	bool Hit( void ) const { return m_bHit; }

private:
	Ray_t			m_rayShot;
	float			m_flForce;
	int				m_iDamageType;
	bool			m_bHit;
};

#endif // FX_IMPACT_H
