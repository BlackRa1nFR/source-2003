#include "cbase.h"
#include "physics_impact_damage.h"

//==============================================================================================
// PLAYER PHYSICS DAMAGE TABLE
//==============================================================================================
static impactentry_t playerLinearTable[] =
{
	{ 150*150, 5 },
	{ 250*250, 10 },
	{ 450*450, 50 },
	{ 700*700, 100 },
	{ 1000*1000, 500 },
};

static impactentry_t playerAngularTable[] =
{
	{ 100*100, 10 },
	{ 150*150, 20 },
	{ 200*200, 50 },
	{ 300*300, 500 },
};

impactdamagetable_t gDefaultPlayerImpactDamageTable =
{
	playerLinearTable,
	playerAngularTable,

	ARRAYSIZE(playerLinearTable),
	ARRAYSIZE(playerAngularTable),

	24*24,		// minimum linear speed
	360*360,	// minimum angular speed
	2,			// can't take damage from anything under 2kg

	5,			// anything less than 5kg is "small"
	5,			// never take more than 5 pts of damage from anything under 5kg
	36*36,		// <5kg objects must go faster than 36 in/s to do damage

	0,			// large mass in kg (no large mass effects)
	1.0,		// large mass scale

};

//==============================================================================================
// PLAYER-IN-VEHICLE PHYSICS DAMAGE TABLE
//==============================================================================================
static impactentry_t playerVehicleLinearTable[] =
{
	{ 150*150, 2 },
	{ 250*250, 4 },
	{ 350*350, 8 },
	{ 450*450, 15 },
	{ 700*700, 25 },
	{ 1000*1000, 50 },
	{ 1500*1500, 100 },
	{ 2000*2000, 500 },
};

static impactentry_t playerVehicleAngularTable[] =
{
	{ 100*100, 10 },
	{ 150*150, 20 },
	{ 200*200, 50 },
	{ 300*300, 500 },
};

impactdamagetable_t gDefaultPlayerVehicleImpactDamageTable =
{
	playerVehicleLinearTable,
	playerVehicleAngularTable,

	ARRAYSIZE(playerVehicleLinearTable),
	ARRAYSIZE(playerVehicleAngularTable),

	24*24,		// minimum linear speed
	360*360,	// minimum angular speed
	80,			// can't take damage from anything under 80 kg

	150,		// anything less than 150kg is "small"
	5,			// never take more than 5 pts of damage from anything under 150kg
	36*36,		// <150kg objects must go faster than 36 in/s to do damage

	0,			// large mass in kg (no large mass effects)
	1.0,		// large mass scale

};


//==============================================================================================
// NPC PHYSICS DAMAGE TABLE
//==============================================================================================
static impactentry_t npcLinearTable[] =
{
	{ 150*150, 5 },
	{ 250*250, 10 },
	{ 350*350, 50 },
	{ 500*500, 100 },
	{ 1000*1000, 500 },
};

static impactentry_t npcAngularTable[] =
{
	{ 100*100, 10 },
	{ 150*150, 25 },
	{ 200*200, 50 },
	{ 250*250, 500 },
};

impactdamagetable_t gDefaultNPCImpactDamageTable =
{
	npcLinearTable,
	npcAngularTable,
	
	ARRAYSIZE(npcLinearTable),
	ARRAYSIZE(npcAngularTable),

	24*24,		// minimum linear speed squared
	360*360,	// minimum angular speed squared (360 deg/s to cause spin/slice damage)
	2,			// can't take damage from anything under 2kg

	5,			// anything less than 5kg is "small"
	5,			// never take more than 5 pts of damage from anything under 5kg
	36*36,		// <5kg objects must go faster than 36 in/s to do damage

	500,		// large mass in kg 
	4,			// large mass scale (anything over 500kg does 4X as much energy to read from damage table)
};



float ReadDamageTable( impactentry_t *pTable, int tableCount, float impulse )
{
	if ( pTable )
	{
		int i;
		for ( i = 0; i < tableCount; i++ )
		{
			if ( impulse < pTable[i].impulse )
				break;
		}
		if ( i > 0 )
		{
			i--;
			//Msg("Damge %.0f, energy %.0f\n", pTable[i].damage, FastSqrt(impulse) );
			return pTable[i].damage;
		}
	}
	return 0;
}

float CalculatePhysicsImpactDamage( int index, gamevcollisionevent_t *pEvent, const impactdamagetable_t &table, float energyScale, bool allowStaticDamage, int &damageType )
{
	damageType = DMG_CRUSH;
	int otherIndex = !index;

	// this is a non-moving object due to a constraint, or being held by the player - no damage
	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & (FVPHYSICS_CONSTRAINT_STATIC|FVPHYSICS_PLAYER_HELD) )
		return 0;

	float otherSpeedSqr = pEvent->preVelocity[otherIndex].LengthSqr();
	float otherAngSqr = 0;
	
	// factor in angular for sharp objects
	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_DMG_SLICE )
	{
		otherAngSqr = pEvent->preAngularVelocity[otherIndex].LengthSqr();
	}

	// UNDONE: What about crushing damage?
	float otherMass = pEvent->pObjects[otherIndex]->GetMass();
	
	// UNDONE: allowStaticDamage is a hack - work out some method for 
	// breakable props to impact the world and break!!
	if ( !allowStaticDamage )
	{
		if ( otherMass < table.minMass )
			return 0;
		// check to see if the object is small
		if ( otherMass < table.smallMassMax && otherSpeedSqr < table.smallMassMinSpeedSqr )
			return 0;
		
		if ( otherSpeedSqr < table.minSpeedSqr && otherAngSqr < table.minRotSpeedSqr )
			return 0;
	}

	if ( energyScale <= 0 )
	{
		energyScale = 1.0;
	}

	float damage = 0;

	// don't ever take spin damage from slowly spinning objects
	if ( otherAngSqr > table.minRotSpeedSqr )
	{
		Vector otherInertia = pEvent->pObjects[otherIndex]->GetInertia();
		float angularMom = DotProductAbs( otherInertia, pEvent->preAngularVelocity[otherIndex] );
		damage = ReadDamageTable( table.angularTable, table.angularCount, angularMom * energyScale );
		if ( damage > 0 )
		{
//			Msg("Spin : %.1f, Damage %.0f\n", FastSqrt(angularMom), damage );
			damageType |= DMG_SLASH;
		}
	}
	
	float eliminatedEnergy = (pEvent->postVelocity[index] - pEvent->preVelocity[index]).LengthSqr() * pEvent->pObjects[index]->GetMass();
	float otherEliminatedEnergy = (pEvent->postVelocity[otherIndex] - pEvent->preVelocity[otherIndex]).LengthSqr() * otherMass;
	
	// exaggerate the effects of really large objects
	if ( otherMass >= table.largeMassMin )
	{
		otherEliminatedEnergy *= table.largeMassScale;
	}

	eliminatedEnergy += otherEliminatedEnergy;

	// now in units of this character's speed squared
	float invMass = pEvent->pObjects[index]->GetInvMass();
	if ( !pEvent->pObjects[index]->IsMoveable() )
	{
		// inv mass is zero, but impact damage is enabled on this
		// prop, so recompute:
		invMass = 1.0f / pEvent->pObjects[index]->GetMass();
	}
	eliminatedEnergy *= invMass * energyScale;
	
	damage += ReadDamageTable( table.linearTable, table.linearCount, eliminatedEnergy );

	// allow table to limit the effect of small objects
	if ( otherMass < table.smallMassMax && table.smallMassCap > 0 )
	{
		damage = clamp( damage, 0, table.smallMassCap );
	}

	return damage;
}

float CalculateDefaultPhysicsDamage( int index, gamevcollisionevent_t *pEvent, float energyScale, bool allowStaticDamage, int &damageType )
{
	return CalculatePhysicsImpactDamage( index, pEvent, gDefaultNPCImpactDamageTable, energyScale, allowStaticDamage, damageType );
}
