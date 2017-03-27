//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements breakables and pushables. func_breakable is a bmodel
//			that breaks into pieces after taking damage.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "filters.h"
#include "func_break.h"
#include "decals.h"
#include "explode.h"
#include "in_buttons.h"
#include "physics.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystemBase.h"
#include "globals.h"
#include "util.h"
#include "physics_impact_damage.h"
#include "props.h"

#ifdef HL1_DLL
extern void PlayerPickupObject( CBasePlayer *pPlayer, CBaseEntity *pObject );
#endif

extern Vector		g_vecAttackDir;

// Just add more items to the bottom of this array and they will automagically be supported
// This is done instead of just a classname in the FGD so we can control which entities can
// be spawned, and still remain fairly flexible
const char *CBreakable::pSpawnObjects[] =
{
	NULL,						// 0
	"item_battery",				// 1
	"item_healthkit",			// 2
	"item_box_srounds"			// 3
	"item_large_box_srounds"	// 4
	"item_box_mrounds"			// 5
	"item_large_box_mrounds"	// 6
	"item_box_lrounds"			// 7
	"item_large_box_lrounds"	// 8
	"item_box_buckshot"			// 9
	"item_flare_round"			// 10
	"item_box_flare_rounds"		// 11
	"item_ml_grenade"			// 12
	"item_ar2_grenade"			// 13
	"item_box_sniper_rounds"	// 14
	"weapon_iceaxe"				// 15
	"weapon_stunstick"			// 16
	"weapon_ar1"				// 17
	"weapon_ar2"				// 18
	"weapon_hmg1"				// 19
	"weapon_rpg"				// 20
	"weapon_smg1"				// 21
	"weapon_smg2"				// 22
	"weapon_slam"				// 23
	"weapon_shotgun"			// 24
	"weapon_molotov"			// 25
};

LINK_ENTITY_TO_CLASS( func_breakable, CBreakable );
BEGIN_DATADESC( CBreakable )

	DEFINE_FIELD( CBreakable, m_Material, FIELD_INTEGER ),
	DEFINE_FIELD( CBreakable, m_Explosion, FIELD_INTEGER ),
	DEFINE_FIELD( CBreakable, m_hBreaker, FIELD_EHANDLE ),

	// Don't need to save/restore these because we precache after restore
	//DEFINE_FIELD( CBreakable, m_idShard, FIELD_INTEGER ),
	DEFINE_FIELD( CBreakable, m_angle, FIELD_FLOAT ),
	DEFINE_FIELD( CBreakable, m_iszGibModel, FIELD_STRING ),
	DEFINE_FIELD( CBreakable, m_iszSpawnObject, FIELD_STRING ),
	DEFINE_FIELD( CBreakable, m_ExplosionMagnitude, FIELD_INTEGER ),
	DEFINE_KEYFIELD( CBreakable, m_flPressureDelay, FIELD_FLOAT, "PressureDelay" ),
	DEFINE_KEYFIELD( CBreakable, m_iMinHealthDmg, FIELD_INTEGER, "minhealthdmg" ),
	DEFINE_FIELD( CBreakable, m_bTookPhysicsDamage, FIELD_BOOLEAN ),
	DEFINE_INPUT( CBreakable, m_impactEnergyScale, FIELD_FLOAT, "physdamagescale" ),

	DEFINE_INPUTFUNC( CBreakable, FIELD_VOID, "Break", InputBreak ),
	DEFINE_INPUTFUNC( CBreakable, FIELD_INTEGER, "SetHealth", InputSetHealth ),
	DEFINE_INPUTFUNC( CBreakable, FIELD_INTEGER, "AddHealth", InputAddHealth ),
	DEFINE_INPUTFUNC( CBreakable, FIELD_INTEGER, "RemoveHealth", InputRemoveHealth ),

	// Function Pointers
	DEFINE_FUNCTION( CBreakable, BreakTouch ),
	DEFINE_FUNCTION( CBreakable, Die ),

	// Outputs
	DEFINE_OUTPUT(CBreakable, m_OnBreak, "OnBreak"),
	DEFINE_OUTPUT(CBreakable, m_OnHealthChanged, "OnHealthChanged"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBreakable::KeyValue( const char *szKeyName, const char *szValue )
{
	// UNDONE_WC: explicitly ignoring these fields, but they shouldn't be in the map file!
	if (FStrEq(szKeyName, "explosion"))
	{
		if (!stricmp(szValue, "directed"))
			m_Explosion = expDirected;
		else if (!stricmp(szValue, "random"))
			m_Explosion = expRandom;
		else
			m_Explosion = expRandom;
	}
	else if (FStrEq(szKeyName, "material"))
	{
		int i = atoi( szValue);

		// 0:glass, 1:metal, 2:flesh, 3:wood

		if ((i < 0) || (i >= matLastMaterial))
			m_Material = matWood;
		else
			m_Material = (Materials)i;
	}
	else if (FStrEq(szKeyName, "deadmodel"))
	{
	}
	else if (FStrEq(szKeyName, "shards"))
	{
//			m_iShards = atof(szValue);
	}
	else if (FStrEq(szKeyName, "gibmodel") )
	{
		m_iszGibModel = AllocPooledString(szValue);
	}
	else if (FStrEq(szKeyName, "spawnobject") )
	{
		int object = atoi( szValue );
		if ( object > 0 && object < ARRAYSIZE(pSpawnObjects) )
			m_iszSpawnObject = MAKE_STRING( pSpawnObjects[object] );
	}
	else if (FStrEq(szKeyName, "explodemagnitude") )
	{
		ExplosionSetMagnitude( atoi( szValue ) );
	}
	else if (FStrEq(szKeyName, "lip") )
	{
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBreakable::Spawn( void )
{
    Precache( );    

	if ( !m_iHealth || FBitSet( m_spawnflags, SF_BREAK_TRIGGER_ONLY ) )
	{
		m_takedamage = DAMAGE_NO;
	}
	else
	{
		m_takedamage = DAMAGE_YES;
	}
  
	SetSolid( SOLID_BSP );
    SetMoveType( MOVETYPE_PUSH );
	
	// this is a hack to shoot the gibs in a specific yaw/direction
	m_angle = GetLocalAngles().y;
	SetLocalAngles( vec3_angle );
	
	SetModel( STRING( GetModelName() ) );//set size and link into world.

	SetTouch( &CBreakable::BreakTouch );
	if ( FBitSet( m_spawnflags, SF_BREAK_TRIGGER_ONLY ) )		// Only break on trigger
	{
		SetTouch( NULL );
	}

	// Flag unbreakable glass as "worldbrush" so it will block ALL tracelines
	if ( !IsBreakable() && m_nRenderMode != kRenderNormal )
		AddFlag( FL_WORLDBRUSH );

	if ( m_impactEnergyScale == 0 )
	{
		m_impactEnergyScale = 1.0;
	}

	CreateVPhysics();
}

//-----------------------------------------------------------------------------
bool CBreakable::CreateVPhysics( void )
{
	VPhysicsInitStatic();
	return true;
}


//-----------------------------------------------------------------------------

const char *CBreakable::MaterialSound( Materials precacheMaterial )
{
    switch ( precacheMaterial ) 
	{
	case matWood:
		return "Breakable.MatWood";
	case matFlesh:
		return "Breakable.MatFlesh";
	case matComputer:
	case matUnbreakableGlass:
	case matGlass:
		return "Breakable.MatGlass";
	case matMetal:
		return "Breakable.MatMetal";
	case matCinderBlock:
	case matRocks:
		return "Breakable.MatConcrete";
	case matCeilingTile:
	case matNone:
	default:
		break;
	}

	return NULL;
}

void CBreakable::MaterialSoundRandom( int entindex, Materials soundMaterial, float volume )
{
	const char	*soundname;
	soundname = MaterialSound( soundMaterial );
	if ( !soundname )
		return;

	CSoundParameters params;
	if ( !GetParametersForSound( soundname, params ) )
		return;

	CPASAttenuationFilter filter( CBaseEntity::Instance( entindex ), params.soundlevel );
	EmitSound( filter, entindex, params.channel, params.soundname, volume, params.soundlevel );
}


void CBreakable::Precache( void )
{
	const char *pGibName = "models/gibs/woodgibs.mdl";

    switch (m_Material) 
	{
	case matWood:
		pGibName = "models/gibs/woodgibs.mdl";
		break;
	case matFlesh:
		pGibName = "models/fleshgibs.mdl";
		break;

	case matUnbreakableGlass:
	case matGlass:
		pGibName = "models/gibs/glass_shard.mdl";
		break;
	case matComputer:
		pGibName = "models/computergibs.mdl";
		break;
	case matMetal:
		pGibName = "models/gibs/metalgibs.mdl";
		break;
	case matCinderBlock:
		pGibName = "models/gibs/concgibs.mdl";
		break;
	case matRocks:
		pGibName = "models/rockgibs.mdl";
		break;
	case matCeilingTile:
		pGibName = "models/ceilinggibs.mdl";
		break;
	}

	if ( m_iszGibModel != NULL_STRING )
	{
		pGibName = STRING(m_iszGibModel);
	}

	m_idShard = engine->PrecacheModel( pGibName );

	// Precache the spawn item's data
	if ( m_iszSpawnObject != NULL_STRING )
	{
		UTIL_PrecacheOther( STRING( m_iszSpawnObject ) );
	}
}

// play shard sound when func_breakable takes damage.
// the more damage, the louder the shard sound.


void CBreakable::DamageSound( void )
{
	int pitch;
	float fvol;
	char *soundname = NULL;
	int material = m_Material;

//	if (random->RandomInt(0,1))
//		return;

	if (random->RandomInt(0,2))
	{
		pitch = PITCH_NORM;
	}
	else
	{
		pitch = 95 + random->RandomInt(0,34);
	}

	fvol = random->RandomFloat(0.75, 1.0);

	if (material == matComputer && random->RandomInt(0,1))
	{
		material = matMetal;
	}

	switch (material)
	{
	case matComputer:
	case matGlass:
	case matUnbreakableGlass:
		soundname = "Breakable.MatGlass";
		break;

	case matWood:
		soundname = "Breakable.MatWood";
		break;

	case matMetal:
		soundname = "Breakable.MatMetal";
		break;

	case matFlesh:
		soundname = "Breakable.MatFlesh";
		break;

	case matRocks:
	case matCinderBlock:
		soundname = "Breakable.MatConcrete";
		break;

	case matCeilingTile:
		break;

	default:
		break;
	}

	if ( soundname )
	{
		CSoundParameters params;
		if ( GetParametersForSound( soundname, params ) )
		{
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), params.channel, params.soundname, fvol, params.soundlevel, 0, pitch );
		}
	}
}

void CBreakable::BreakTouch( CBaseEntity *pOther )
{
	float flDamage;
	
	// only players can break these right now
	if ( !pOther->IsPlayer() || !IsBreakable() )
	{
        return;
	}

	// can I be broken when run into?
	if ( HasSpawnFlags( SF_BREAK_TOUCH ) )
	{
		flDamage = pOther->GetAbsVelocity().Length() * 0.01;

		if (flDamage >= m_iHealth)
		{
			SetTouch( NULL );
			OnTakeDamage( CTakeDamageInfo( pOther, pOther, flDamage, DMG_CRUSH ) );

			// do a little damage to player if we broke glass or computer
			CTakeDamageInfo info( pOther, pOther, flDamage/4, DMG_SLASH );
			CalculateMeleeDamageForce( &info, (pOther->GetAbsOrigin() - GetAbsOrigin()), GetAbsOrigin() );
			pOther->TakeDamage( info );
		}
	}

	// can I be broken when stood upon?
	if ( HasSpawnFlags( SF_BREAK_PRESSURE ) && pOther->GetGroundEntity() == this )
	{
		// play creaking sound here.
		DamageSound();

		m_hBreaker = pOther;

		SetThink ( &CBreakable::Die );
		SetTouch( NULL );
		
		// Add optional delay 
		SetNextThink( gpGlobals->curtime + m_flPressureDelay );

	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for adding to the breakable's health.
// Input  : Integer health points to add.
//-----------------------------------------------------------------------------
void CBreakable::InputAddHealth( inputdata_t &inputdata )
{
	m_iHealth += inputdata.value.Int();
	m_OnHealthChanged.Set( m_iHealth, inputdata.pActivator, this );

	if ( m_iHealth <= 0 )
	{
		Break( inputdata.pActivator );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for breaking the breakable immediately.
//-----------------------------------------------------------------------------
void CBreakable::InputBreak( inputdata_t &inputdata )
{
	Break( inputdata.pActivator );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for removing health from the breakable.
// Input  : Integer health points to remove.
//-----------------------------------------------------------------------------
void CBreakable::InputRemoveHealth( inputdata_t &inputdata )
{
	m_iHealth -= inputdata.value.Int();
	m_OnHealthChanged.Set( m_iHealth, inputdata.pActivator, this );

	if ( m_iHealth <= 0 )
	{
		Break( inputdata.pActivator );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the breakable's health.
//-----------------------------------------------------------------------------
void CBreakable::InputSetHealth( inputdata_t &inputdata )
{
	m_iHealth = inputdata.value.Int();
	m_OnHealthChanged.Set( m_iHealth, inputdata.pActivator, this );

	if ( m_iHealth <= 0 )
	{
		Break( inputdata.pActivator );
	}
	else
	{
		if ( FBitSet( m_spawnflags, SF_BREAK_TRIGGER_ONLY ) )
		{
			m_takedamage = DAMAGE_NO;
		}
		else
		{
			m_takedamage = DAMAGE_YES;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Breaks the breakable if it can be broken.
// Input  : pBreaker - The entity that caused us to break, either via an input,
//				by shooting us, or by touching us.
//-----------------------------------------------------------------------------
void CBreakable::Break( CBaseEntity *pBreaker )
{
	if ( IsBreakable() )
	{
		QAngle angles = GetLocalAngles();
		angles.y = m_angle;
		SetLocalAngles( angles );
		AngleVectors( GetLocalAngles(), &g_vecAttackDir );
		m_hBreaker = pBreaker;
		Die();
	}
}


void CBreakable::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	// random spark if this is a 'computer' object
	if (random->RandomInt(0,1) )
	{
		switch( m_Material )
		{
			case matComputer:
			{
				g_pEffects->Sparks( ptr->endpos );

				EmitSound( "Breakable.Computer" );
			}
			break;
			
			case matUnbreakableGlass:
				g_pEffects->Ricochet( ptr->endpos, (vecDir*-1.0f) );
			break;
		}
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}


//-----------------------------------------------------------------------------
// Purpose: Allows us to take damage from physics objects
//-----------------------------------------------------------------------------
void CBreakable::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	Vector damagePos;
	pEvent->pInternalData->GetContactPoint( damagePos );
	Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
	if ( damageForce == vec3_origin )
	{
		// This can happen if this entity is a func_breakable, and can't move.
		// Use the velocity of the entity that hit us instead.
		damageForce = pEvent->postVelocity[!index] * pEvent->pObjects[!index]->GetMass();
	}

	// If we're supposed to explode on collision, do so
	if ( HasSpawnFlags( SF_BREAK_PHYSICS_BREAK_IMMEDIATELY ) )
	{
		// We're toast
		m_bTookPhysicsDamage = true;
		CBaseEntity *pHitEntity = pEvent->pEntities[!index];

		CTakeDamageInfo dmgInfo( pHitEntity, pHitEntity, damageForce, damagePos, (m_iHealth + 1), DMG_CRUSH );
		PhysCallbackDamage( this, dmgInfo, *pEvent, index );
	}
	else if ( !HasSpawnFlags( SF_BREAK_DONT_TAKE_PHYSICS_DAMAGE ) )
	{
		int otherIndex = !index;
		CBaseEntity *pOther = pEvent->pEntities[otherIndex];

		// We're to take normal damage from this
		int damageType;
		float damage = CalculateDefaultPhysicsDamage( index, pEvent, m_impactEnergyScale, true, damageType );
		if ( damage > 0 )
		{
			CTakeDamageInfo dmgInfo( pOther, pOther, damageForce, damagePos, damage, damageType );
			PhysCallbackDamage( this, dmgInfo, *pEvent, index );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows us to make damage exceptions that are breakable-specific.
//-----------------------------------------------------------------------------
int CBreakable::OnTakeDamage( const CTakeDamageInfo &info )
{
	Vector	vecTemp;

	CTakeDamageInfo subInfo = info;

	// If attacker can't do at least the min required damage to us, don't take any damage from them
	if ( m_takedamage == DAMAGE_NO || info.GetDamage() < m_iMinHealthDmg )
		return 0;

	// Check our damage filter
	if ( !PassesDamageFilter(subInfo.GetAttacker()) )
	{
		m_bTookPhysicsDamage = false;
		return 1;
	}

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin). 
	if ( subInfo.GetAttacker() == subInfo.GetInflictor() )	
	{
		vecTemp = subInfo.GetInflictor()->GetAbsOrigin() - ( GetAbsMins() + ( EntitySpaceSize() * 0.5 ) );
	}
	else
	{
		// an actual missile was involved.
		vecTemp = subInfo.GetInflictor()->GetAbsOrigin() - ( GetAbsMins() + ( EntitySpaceSize() * 0.5 ) );
	}
	
	if (!IsBreakable())
		return 0;

	float flPropDamage = GetBreakableDamage( subInfo );
	subInfo.SetDamage( flPropDamage );

	// this global is still used for glass and other non-NPC killables, along with decals.
	g_vecAttackDir = -vecTemp;
	VectorNormalize( g_vecAttackDir );
	
	int iPrevHealth = m_iHealth;
	BaseClass::OnTakeDamage( subInfo );

	// If we took damage, fire the output
	if ( iPrevHealth != m_iHealth )
	{
		m_OnHealthChanged.Set( m_iHealth, subInfo.GetAttacker(), this );

		if (m_iHealth <= 0)
		{
			m_hBreaker = subInfo.GetAttacker();
			Die();
			return 0;
		}
	}

	// Make a shard noise each time func breakable is hit, if it's capable of taking damage
	if ( m_takedamage == DAMAGE_YES )
	{
		// Don't play shard noise if being burned.
		// Don't play shard noise if cbreakable actually died.
		if ( ( subInfo.GetDamageType() & DMG_BURN ) == false )
		{
			DamageSound();
		}
	}

	return 1;
}

//------------------------------------------------------------------------------
// Purpose : Reset the OnGround flags for any entities that may have been
//			 resting on me
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBreakable::ResetOnGroundFlags(void)
{
	float size = EntitySpaceSize().x;
	if ( size < EntitySpaceSize().y )
		size = EntitySpaceSize().y;
	if ( size < EntitySpaceSize().z )
		size = EntitySpaceSize().z;

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 9 inch high sheet
	Vector mins = GetAbsMins();
	Vector maxs = GetAbsMaxs();
	mins.z = GetAbsMaxs().z-1;
	maxs.z = GetAbsMaxs().z+8;

	// BUGBUG -- can only find 256 entities on a breakable -- should be enough
	CBaseEntity *pList[256];
	int count = UTIL_EntitiesInBox( pList, 256, mins, maxs, FL_ONGROUND );
	if ( count )
	{
		for ( int i = 0; i < count; i++ )
		{
			pList[i]->RemoveFlag( FL_ONGROUND );
			pList[i]->SetGroundEntity( (CBaseEntity *)NULL );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Breaks the breakable. m_hBreaker is the entity that caused us to break.
//-----------------------------------------------------------------------------
void CBreakable::Die( void )
{
	Vector vecSpot;// shard origin
	Vector vecVelocity;// shard velocity
	char cFlag = 0;
	int pitch;
	float fvol;
	
	pitch = 95 + random->RandomInt(0,29);

	if (pitch > 97 && pitch < 103)
	{
		pitch = 100;
	}

	// The more negative m_iHealth, the louder
	// the sound should be.

	fvol = random->RandomFloat(0.85, 1.0) + (abs(m_iHealth) / 100.0);
	if (fvol > 1.0)
	{
		fvol = 1.0;
	}

	const char *soundname = NULL;

	switch (m_Material)
	{
	default:
		break;

	case matGlass:
		soundname = "Breakable.Glass";
		cFlag = BREAK_GLASS;
		break;

	case matWood:
		soundname = "Breakable.Crate";
		cFlag = BREAK_WOOD;
		break;

	case matComputer:
	case matMetal:
		soundname = "Breakable.Metal";
		cFlag = BREAK_METAL;
		break;

	case matFlesh:
		soundname = "Breakable.Flesh";
		cFlag = BREAK_FLESH;
		break;

	case matRocks:
	case matCinderBlock:
		soundname = "Breakable.Concrete";
		cFlag = BREAK_CONCRETE;
		break;

	case matCeilingTile:
		soundname = "Breakable.Ceiling";
		break;
	}
    
	if ( soundname )
	{
		CSoundParameters params;
		if ( GetParametersForSound( soundname, params ) )
		{
			CPASAttenuationFilter filter( this );
			EmitSound( filter, entindex(), params.channel, params.soundname, fvol, params.soundlevel, 0, pitch );	
		}
	}
		
	if (m_Explosion == expDirected)
		vecVelocity = g_vecAttackDir * 200;
	else
	{
		vecVelocity.x = 0;
		vecVelocity.y = 0;
		vecVelocity.z = 0;
	}

	vecSpot = (GetAbsMins() + GetAbsMaxs()) * 0.5;

	CPVSFilter filter2( vecSpot );

	te->BreakModel( filter2, 0.0,
		&vecSpot, &EntitySpaceSize(), &vecVelocity, m_idShard, 100,
			0, 2.5, cFlag );

	ResetOnGroundFlags();

	// Don't fire something that could fire myself
	SetName( NULL_STRING );

	AddSolidFlags( FSOLID_NOT_SOLID );
	Relink();
	
	// Fire targets on break
	m_OnBreak.FireOutput( m_hBreaker, this );

	VPhysicsDestroyObject();
	SetThink( &CBreakable::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );
	if ( m_iszSpawnObject != NULL_STRING )
		CBaseEntity::Create( STRING(m_iszSpawnObject), WorldSpaceCenter(), GetAbsAngles(), this );

	if ( Explodable() )
	{
		ExplosionCreate( WorldSpaceCenter(), GetAbsAngles(), this, ExplosionMagnitude(), 0, true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether this object can be broken.
//-----------------------------------------------------------------------------
bool CBreakable::IsBreakable( void ) 
{ 
	return m_Material != matUnbreakableGlass;
}


char const *CBreakable::DamageDecal( int bitsDamageType, int gameMaterial )
{
	if ( m_Material == matGlass  )
		return "GlassBreak";

	if ( m_Material == matUnbreakableGlass )
		return "BulletProof";

	return BaseClass::DamageDecal( bitsDamageType, gameMaterial );
}

class CPushable : public CBreakable
{
public:
	DECLARE_CLASS( CPushable, CBreakable );

	void	Spawn ( void );
	bool	CreateVPhysics( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_ONOFF_USE; }

	// breakables use an overridden takedamage
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	unsigned int PhysicsSolidMaskForEntity( void ) const { return MASK_PLAYERSOLID; }
};


LINK_ENTITY_TO_CLASS( func_pushable, CPushable );


void CPushable::Spawn( void )
{
	if ( HasSpawnFlags( SF_PUSH_BREAKABLE ) )
	{
		BaseClass::Spawn();
	}
	else
	{
		Precache();

		SetSolid( SOLID_VPHYSICS );

		SetMoveType( MOVETYPE_PUSH );
		SetModel( STRING( GetModelName() ) );

		CreateVPhysics();
	}
}


bool CPushable::CreateVPhysics( void )
{
	VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
	IPhysicsObject *pPhysObj = VPhysicsGetObject();
	if ( pPhysObj )
	{
		pPhysObj->SetMass( 30 );
//		Vector vecInertia = Vector(800, 800, 800);
//		pPhysObj->SetInertia( vecInertia );
	}

	return true;
}

// Pull the func_pushable
void CPushable::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
#ifdef HL1_DLL
	// Allow pushables to be dragged by player
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( pPlayer )
	{
		if ( useType == USE_ON )
		{
			PlayerPickupObject( pPlayer, this );
		}
	}
#else
	BaseClass::Use( pActivator, pCaller, useType, value );
#endif
}


int CPushable::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( m_spawnflags & SF_PUSH_BREAKABLE )
		return BaseClass::OnTakeDamage( info );

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Allows us to take damage from physics objects
//-----------------------------------------------------------------------------
void CPushable::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];
	if ( pOther->IsPlayer() )
	{
		// Pushables don't take damage from impacts with the player
		// We call all the way back to the baseclass to get the physics effects.
		CBaseEntity::VPhysicsCollision( index, pEvent );
		return;
	}

	BaseClass::VPhysicsCollision( index, pEvent );
}

