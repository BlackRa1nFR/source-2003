//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef PHYSICS_IMPACT_DAMAGE_H
#define PHYSICS_IMPACT_DAMAGE_H
#ifdef _WIN32
#pragma once
#endif


struct impactentry_t
{
	float	impulse;
	float	damage;
};

struct impactdamagetable_t
{
	impactentry_t	*linearTable;
	impactentry_t	*angularTable;
	int			linearCount;	// array size of linearTable
	int			angularCount;	// array size of angularTable

	float		minSpeedSqr;	// minimum squared impact speed for damage
	float		minRotSpeedSqr;
	float		minMass;		// minimum mass to do damage

	// filter out reall small objects, set all to zero to disable
	float		smallMassMax;
	float		smallMassCap;
	float		smallMassMinSpeedSqr;

	// exaggerate the effects of really large objects, set all to 1 to disable
	float		largeMassMin;
	float		largeMassScale;
};



extern impactdamagetable_t gDefaultNPCImpactDamageTable;
extern impactdamagetable_t gDefaultPlayerImpactDamageTable;
extern impactdamagetable_t gDefaultPlayerVehicleImpactDamageTable;

// NOTE Default uses default NPC table
float CalculateDefaultPhysicsDamage( int index, gamevcollisionevent_t *pEvent, float energyScale, bool allowStaticDamage, int &damageTypeOut );

// use passes in the table
float CalculatePhysicsImpactDamage( int index, gamevcollisionevent_t *pEvent, const impactdamagetable_t &table, float energyScale, bool allowStaticDamage, int &damageTypeOut );

#endif // PHYSICS_IMPACT_DAMAGE_H
