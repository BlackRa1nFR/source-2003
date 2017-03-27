//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "vphysics_interface.h"
#include "physics.h"
#include "vcollide_parse.h"
#include "entitylist.h"
#include "customentity.h"
#include "physobj.h"
#include "hierarchy.h"
#include "game.h"
#include "ndebugoverlay.h"
#include "engine/IEngineSound.h"
#include "model_types.h"
#include "props.h"
#include "physics_saverestore.h"
#include "saverestore_utlvector.h"
#include "vphysics/constraints.h"
#include "collisionutils.h"
#include "decals.h"
#include "bone_setup.h"

const char *GetMassEquivalent(float flMass);

// This is a physically simulated spring, used to join objects together and create spring forces
//
// NOTE: Springs are not physical in the sense that they only create force, they do not collide with
// anything or have any REAL constraints.  They can be stretched infinitely (though this will create
// and infinite force), they can penetrate any other object (or spring). They do not occupy any space.
// 

#define SF_SPRING_ONLYSTRETCH		0x0001

class CPhysicsSpring : public CBaseEntity
{
	DECLARE_CLASS( CPhysicsSpring, CBaseEntity );
public:
	CPhysicsSpring();
	~CPhysicsSpring();

	void	Spawn( void );
	void	Activate( void );

	// Inputs
	void InputSetSpringConstant( inputdata_t &inputdata );
	void InputSetSpringDamping( inputdata_t &inputdata );
	void InputSetSpringLength( inputdata_t &inputdata );

	// Debug
	int		DrawDebugTextOverlays(void);
	void	DrawDebugGeometryOverlays(void);

	void GetSpringObjectConnections( string_t nameStart, string_t nameEnd, IPhysicsObject **pStart, IPhysicsObject **pEnd );
	void NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params );

	DECLARE_DATADESC();

private:	
	IPhysicsSpring		*m_pSpring;
	bool			m_isLocal;

	// These are "template" values used to construct the spring.  After creation, they are not needed
	float			m_tempConstant;
	float			m_tempLength;	// This is the "ideal" length of the spring, not the length it is currently stretched to.
	float			m_tempDamping;
	float			m_tempRelativeDamping;

	string_t		m_nameAttachStart;
	string_t		m_nameAttachEnd;
	Vector			m_start;
	Vector			m_end;
	unsigned int 	m_teleportTick;
};

LINK_ENTITY_TO_CLASS( phys_spring, CPhysicsSpring );

BEGIN_DATADESC( CPhysicsSpring )

	DEFINE_PHYSPTR( CPhysicsSpring, m_pSpring ),

	DEFINE_KEYFIELD( CPhysicsSpring, m_tempConstant, FIELD_FLOAT, "constant" ),
	DEFINE_KEYFIELD( CPhysicsSpring, m_tempLength, FIELD_FLOAT, "length" ),
	DEFINE_KEYFIELD( CPhysicsSpring, m_tempDamping, FIELD_FLOAT, "damping" ),
	DEFINE_KEYFIELD( CPhysicsSpring, m_tempRelativeDamping, FIELD_FLOAT, "relativedamping" ),

	DEFINE_KEYFIELD( CPhysicsSpring, m_nameAttachStart, FIELD_STRING, "attach1" ),
	DEFINE_KEYFIELD( CPhysicsSpring, m_nameAttachEnd, FIELD_STRING, "attach2" ),

	DEFINE_FIELD( CPhysicsSpring, m_start, FIELD_POSITION_VECTOR ),
	DEFINE_KEYFIELD( CPhysicsSpring, m_end, FIELD_POSITION_VECTOR, "springaxis" ),
	DEFINE_FIELD( CPhysicsSpring, m_isLocal, FIELD_BOOLEAN ),

	// Not necessary to save... it's only there to make sure 
//	DEFINE_FIELD( CPhysicsSpring, m_teleportTick, FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC( CPhysicsSpring, FIELD_FLOAT, "SetSpringConstant", InputSetSpringConstant ),
	DEFINE_INPUTFUNC( CPhysicsSpring, FIELD_FLOAT, "SetSpringLength", InputSetSpringLength ),
	DEFINE_INPUTFUNC( CPhysicsSpring, FIELD_FLOAT, "SetSpringDamping", InputSetSpringDamping ),

END_DATADESC()


CPhysicsSpring::CPhysicsSpring( void )
{
#ifdef _DEBUG
	m_start.Init();
	m_end.Init();
#endif
	m_pSpring = NULL;
	m_tempConstant = 150;
	m_tempLength = 0;
	m_tempDamping = 2.0;
	m_tempRelativeDamping = 0.01;
	m_isLocal = false;
	m_teleportTick = 0xFFFFFFFF;
}

CPhysicsSpring::~CPhysicsSpring( void )
{
	if ( m_pSpring )
	{
		physenv->DestroySpring( m_pSpring );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPhysicsSpring::InputSetSpringConstant( inputdata_t &inputdata )
{
	m_tempConstant = inputdata.value.Float();
	m_pSpring->SetSpringConstant(inputdata.value.Float());
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPhysicsSpring::InputSetSpringDamping( inputdata_t &inputdata )
{
	m_tempDamping = inputdata.value.Float();
	m_pSpring->SetSpringDamping(inputdata.value.Float());
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPhysicsSpring::InputSetSpringLength( inputdata_t &inputdata )
{
	m_tempLength = inputdata.value.Float();
	m_pSpring->SetSpringLength(inputdata.value.Float());
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CPhysicsSpring::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"Constant: %3.2f",m_tempConstant);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Length: %3.2f",m_tempLength);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Damping: %3.2f",m_tempDamping);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Override base class to add display of fly direction
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CPhysicsSpring::DrawDebugGeometryOverlays(void) 
{
	// ------------------------------
	// Draw if BBOX is on
	// ------------------------------
	if (m_debugOverlays & OVERLAY_BBOX_BIT)
	{
		Vector vStartPos;
		Vector vEndPos;
		m_pSpring->GetEndpoints( &vStartPos, &vEndPos );

		Vector vSpringDir = vEndPos - vStartPos;
		VectorNormalize(vSpringDir);

		Vector vLength = vStartPos + (vSpringDir*m_tempLength);

		NDebugOverlay::Line(vStartPos, vLength, 0,0,255, false, 0);
		NDebugOverlay::Line(vLength, vEndPos, 255,0,0, false, 0);
	}
	BaseClass::DrawDebugGeometryOverlays();
}

bool PointIsNearer( IPhysicsObject *pObject1, const Vector &point1, const Vector &point2 )
{
	Vector center;
	
	pObject1->GetPosition( &center, 0 );

	float dist1 = (center - point1).LengthSqr();
	float dist2 = (center - point2).LengthSqr();

	if ( dist1 < dist2 )
		return true;

	return false;
}

IPhysicsObject *FindPhysicsObject( const char *pName )
{
	if ( !pName || !strlen(pName) )
		return NULL;

	CBaseEntity *pEntity = NULL;

	while (1)
	{
		pEntity = gEntList.FindEntityByName( pEntity, pName, NULL );
		if ( !pEntity )
			break;
		IPhysicsObject *pObject = pEntity->VPhysicsGetObject();
		if ( pObject )
			return pObject;
	}
	return NULL;
}

void CPhysicsSpring::GetSpringObjectConnections( string_t nameStart, string_t nameEnd, IPhysicsObject **pStart, IPhysicsObject **pEnd )
{
	IPhysicsObject *pStartObject = FindPhysicsObject( STRING(nameStart) );
	IPhysicsObject *pEndObject = FindPhysicsObject( STRING(nameEnd) );

	// Assume the world for missing objects
	if ( !pStartObject )
	{
		pStartObject = g_PhysWorldObject;
	}
	else if ( !pEndObject )
	{
		// try to sort so that the world is always the start object
		pEndObject = pStartObject;
		pStartObject = g_PhysWorldObject;
	}
	else
	{
		CBaseEntity *pEntity0 = (CBaseEntity *) (pStartObject->GetGameData());
		g_pNotify->AddEntity( this, pEntity0 );

		CBaseEntity *pEntity1 = (CBaseEntity *) pEndObject->GetGameData();
		g_pNotify->AddEntity( this, pEntity1 );
	}

	*pStart = pStartObject;
	*pEnd = pEndObject;
}


void CPhysicsSpring::Activate( void )
{
	BaseClass::Activate();

	// UNDONE: save/restore all data, and only create the spring here

	if ( !m_pSpring )
	{
		IPhysicsObject *pStart, *pEnd;

		GetSpringObjectConnections( m_nameAttachStart, m_nameAttachEnd, &pStart, &pEnd );

		// Needs to connect to real, different objects
		if ( (!pStart || !pEnd) || (pStart == pEnd) )
		{
			DevMsg("ERROR: Can't init spring %s from \"%s\" to \"%s\"\n", GetDebugName(), STRING(m_nameAttachStart), STRING(m_nameAttachEnd) );
			UTIL_Remove( this );
			return;
		}

		// if m_end is not closer to pEnd than m_start, swap
		if ( !PointIsNearer( pEnd, m_end, m_start ) )
		{
			Vector tmpVec = m_start;
			m_start = m_end;
			m_end = tmpVec;
		}

		// create the spring
		springparams_t spring;
		spring.constant = m_tempConstant;
		spring.damping = m_tempDamping;
		spring.naturalLength = m_tempLength;
		spring.relativeDamping = m_tempRelativeDamping;
		spring.startPosition = m_start;
		spring.endPosition = m_end;
		spring.useLocalPositions = false;
		spring.onlyStretch = HasSpawnFlags( SF_SPRING_ONLYSTRETCH );
		m_pSpring = physenv->CreateSpring( pStart, pEnd, &spring );
	}
}


void CPhysicsSpring::Spawn( void )
{
	SetSolid( SOLID_NONE );
	m_start = GetAbsOrigin();
	if ( m_tempLength <= 0 )
	{
		m_tempLength = (m_end - m_start).Length();
	}
}

void CPhysicsSpring::NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
	// don't recurse
	if ( eventType != NOTIFY_EVENT_TELEPORT || (unsigned int)gpGlobals->tickcount == m_teleportTick )
		return;

	m_teleportTick = gpGlobals->tickcount;
	PhysTeleportConstrainedEntity( pNotify, m_pSpring->GetStartObject(), m_pSpring->GetEndObject(), params.pTeleport->prevOrigin, params.pTeleport->prevAngles, params.pTeleport->physicsRotate );
}


// ---------------------------------------------------------------------
//
// CPhysBox -- physically simulated brush rectangular solid
//
// ---------------------------------------------------------------------

// SendTable stuff.
IMPLEMENT_SERVERCLASS_ST(CPhysBox, DT_PhysBox)
END_SEND_TABLE()

// Physbox Spawnflags. Start at 0x01000 to avoid collision with CBreakable's
#define SF_PHYSBOX_ASLEEP			0x01000
#define SF_PHYSBOX_IGNOREUSE		0x02000
#define SF_PHYSBOX_DEBRIS			0x04000
#define SF_PHYSBOX_MOTIONDISABLED	0x08000
#define SF_PHYSBOX_USEPREFERRED		0x10000

LINK_ENTITY_TO_CLASS( func_physbox, CPhysBox );

BEGIN_DATADESC( CPhysBox )

	DEFINE_FIELD( CPhysBox, m_hCarryingPlayer, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( CPhysBox, m_massScale, FIELD_FLOAT, "massScale" ),
	DEFINE_KEYFIELD( CPhysBox, m_damageType, FIELD_INTEGER, "Damagetype" ),
	DEFINE_KEYFIELD( CPhysBox, m_iszOverrideScript, FIELD_STRING, "overridescript" ),
	DEFINE_KEYFIELD( CPhysBox, m_damageToEnableMotion, FIELD_INTEGER, "damagetoenablemotion" ),
	DEFINE_KEYFIELD( CPhysBox, m_angPreferredCarryAngles, FIELD_VECTOR, "preferredcarryangles" ),

	DEFINE_INPUTFUNC( CPhysBox, FIELD_VOID, "Wake", InputWake ),
	DEFINE_INPUTFUNC( CPhysBox, FIELD_VOID, "Sleep", InputSleep ),
	DEFINE_INPUTFUNC( CPhysBox, FIELD_VOID, "EnableMotion", InputEnableMotion ),
	DEFINE_INPUTFUNC( CPhysBox, FIELD_VOID, "DisableMotion", InputDisableMotion ),
	DEFINE_INPUTFUNC( CPhysBox, FIELD_VOID, "ForceDrop", InputForceDrop ),

	// Function pointers
	DEFINE_FUNCTION( CPhysBox, BreakTouch ),

	// Outputs
	DEFINE_OUTPUT( CPhysBox, m_OnDamaged, "OnDamaged" ),
	DEFINE_OUTPUT( CPhysBox, m_OnAwakened, "OnAwakened" ),
	DEFINE_OUTPUT( CPhysBox, m_OnPhysGunPickup, "OnPhysGunPickup" ),
	DEFINE_OUTPUT( CPhysBox, m_OnPhysGunDrop, "OnPhysGunDrop" ),

END_DATADESC()

// UNDONE: Save/Restore needs to take the physics object's properties into account
// UNDONE: Acceleration, velocity, angular velocity, etc. must be preserved
// UNDONE: Many of these quantities are relative to a coordinate frame
// UNDONE: Translate when going across transitions?
// UNDONE: Build transition transformation, and transform data in save/restore for IPhysicsObject
// UNDONE: Angles are saved in the entity, but not propagated back to the IPhysicsObject on restore

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysBox::Spawn( void )
{
	Precache();

	if ( HasSpawnFlags( SF_BREAK_TRIGGER_ONLY ) )
	{
		m_takedamage = DAMAGE_NO;
		AddSpawnFlags( SF_BREAK_DONT_TAKE_PHYSICS_DAMAGE );
	}
	else if ( m_iHealth == 0 )
	{
		m_takedamage = DAMAGE_EVENTS_ONLY;
		AddSpawnFlags( SF_BREAK_DONT_TAKE_PHYSICS_DAMAGE );
	}
	else
	{
		m_takedamage = DAMAGE_YES;
	}
  
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );

	SetAbsVelocity( vec3_origin );
	SetModel( STRING( GetModelName() ) );
	Relink();
	SetSolid( SOLID_VPHYSICS );
	if ( HasSpawnFlags( SF_PHYSBOX_DEBRIS ) )
	{
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	}

	CreateVPhysics();

	m_hCarryingPlayer = NULL;

	SetTouch( &CPhysBox::BreakTouch );
	if ( HasSpawnFlags( SF_BREAK_TRIGGER_ONLY ) )		// Only break on trigger
	{
		SetTouch( NULL );
	}

	if ( m_impactEnergyScale == 0 )
	{
		m_impactEnergyScale = 1.0;
	}
}

bool CPhysBox::CreateVPhysics()
{
	solid_t tmpSolid;
	PhysModelParseSolid( tmpSolid, this, GetModelIndex() );
	if ( m_massScale > 0 )
	{
		float mass = tmpSolid.params.mass * m_massScale;
		mass = clamp( mass, 0.5, 1e6 );
		tmpSolid.params.mass = mass;
	}

	PhysSolidOverride( tmpSolid, m_iszOverrideScript );
	IPhysicsObject *pPhysics = VPhysicsInitNormal( GetSolid(), GetSolidFlags(), true, &tmpSolid );

	if ( m_damageType == 1 )
	{
		PhysSetGameFlags( pPhysics, FVPHYSICS_DMG_SLICE );
	}

	// Wake it up if not asleep
	if ( !HasSpawnFlags(SF_PHYSBOX_ASLEEP) )
	{
		VPhysicsGetObject()->Wake();
	}

	if ( HasSpawnFlags(SF_PHYSBOX_MOTIONDISABLED) || m_damageToEnableMotion > 0  )
	{
		VPhysicsGetObject()->EnableMotion( false );
	}

	// only send data when physics moves this object
	NetworkStateManualMode( true );

	return true;
}


int CPhysBox::ObjectCaps() 
{ 
	int caps = BaseClass::ObjectCaps();
	if ( !HasSpawnFlags( SF_PHYSBOX_IGNOREUSE ) )
	{
		if ( CBasePlayer::CanPickupObject( this, 35, 128 ) )
		{
			caps |= FCAP_IMPULSE_USE;
		}
	}

	return caps;
}

void CPhysBox::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( pPlayer )
	{
		pPlayer->PickupObject( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CPhysBox::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		if (VPhysicsGetObject())
		{
			char tempstr[512];
			Q_snprintf(tempstr, sizeof(tempstr),"Mass: %.2f kg / %.2f lb (%s)", VPhysicsGetObject()->GetMass(), kg2lbs(VPhysicsGetObject()->GetMass()), GetMassEquivalent(VPhysicsGetObject()->GetMass()));
			NDebugOverlay::EntityText(entindex(), text_offset, tempstr, 0);
			text_offset++;
		}
	}

	return text_offset;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that breaks the physics object away from its parent
//			and starts it simulating.
//-----------------------------------------------------------------------------
void CPhysBox::InputWake( inputdata_t &inputdata )
{
	VPhysicsGetObject()->Wake();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that breaks the physics object away from its parent
//			and stops it simulating.
//-----------------------------------------------------------------------------
void CPhysBox::InputSleep( inputdata_t &inputdata )
{
	VPhysicsGetObject()->Sleep();
}

//-----------------------------------------------------------------------------
// Purpose: Enable physics motion and collision response (on by default)
//-----------------------------------------------------------------------------
void CPhysBox::InputEnableMotion( inputdata_t &inputdata )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->EnableMotion( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Disable any physics motion or collision response
//-----------------------------------------------------------------------------
void CPhysBox::InputDisableMotion( inputdata_t &inputdata )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->EnableMotion( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: If we're being held by the player's hand/physgun, force it to drop us
//-----------------------------------------------------------------------------
void CPhysBox::InputForceDrop( inputdata_t &inputdata )
{
	if ( m_hCarryingPlayer )
	{
		m_hCarryingPlayer->ForceDropOfCarriedPhysObjects();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysBox::Move( const Vector &direction )
{
	VPhysicsGetObject()->ApplyForceCenter( direction );
}

// Update the visible representation of the physic system's representation of this object
void CPhysBox::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	NetworkStateChanged();
	BaseClass::VPhysicsUpdate( pPhysics );

	// if this is the first time we have moved, fire our target
	if ( HasSpawnFlags( SF_PHYSBOX_ASLEEP ) )
	{
		if ( !pPhysics->IsAsleep() )
		{
			m_OnAwakened.FireOutput(this, this);
			FireTargets( STRING(m_target), this, this, USE_TOGGLE, 0 );
			m_spawnflags &= ~SF_PHYSBOX_ASLEEP;
		}
	}
}

void CPhysBox::OnPhysGunPickup( CBasePlayer *pPhysGunUser )
{
	m_hCarryingPlayer = pPhysGunUser;
	m_OnPhysGunPickup.FireOutput( pPhysGunUser, this );
}

void CPhysBox::OnPhysGunDrop( CBasePlayer *pPhysGunUser, bool wasLaunched )
{
	m_hCarryingPlayer = NULL;
	m_OnPhysGunDrop.FireOutput( pPhysGunUser, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPhysBox::OnTakeDamage( const CTakeDamageInfo &info )
{
	// note: if motion is disabled, OnTakeDamage can't apply physics force
	int ret = BaseClass::OnTakeDamage( info );

	// Check our health against the threshold:
	if( m_damageToEnableMotion > 0 && GetHealth() < m_damageToEnableMotion )
	{
		// only do this once
		m_damageToEnableMotion = 0;

		IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
		if ( pPhysicsObject != NULL )
		{
			pPhysicsObject->Wake();
			pPhysicsObject->EnableMotion( true );
			
			VPhysicsTakeDamage( info );
		}
	}

	if ( info.GetInflictor() )
	{
		m_OnDamaged.FireOutput( info.GetAttacker(), this );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this physbox has preferred carry angles
//-----------------------------------------------------------------------------
bool CPhysBox::HasPreferredCarryAngles( void )
{
	return HasSpawnFlags( SF_PHYSBOX_USEPREFERRED );
}


// ---------------------------------------------------------------------
//
// CPhysExplosion -- physically simulated explosion
//
// ---------------------------------------------------------------------
#define SF_PHYSEXPLOSION_NODAMAGE	0x0001
LINK_ENTITY_TO_CLASS( env_physexplosion, CPhysExplosion );

BEGIN_DATADESC( CPhysExplosion )

	DEFINE_KEYFIELD( CPhysExplosion, m_damage, FIELD_FLOAT, "magnitude" ),
	DEFINE_KEYFIELD( CPhysExplosion, m_radius, FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( CPhysExplosion, m_targetEntityName, FIELD_STRING, "targetentityname" ),

	// Inputs
	DEFINE_INPUTFUNC( CPhysExplosion, FIELD_VOID, "Explode", InputExplode ),

END_DATADESC()


void CPhysExplosion::Spawn( void )
{
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	SetModelName( NULL_STRING );
}


CBaseEntity *CPhysExplosion::FindEntity( CBaseEntity *pEntity, CBaseEntity *pActivator )
{
	if ( m_targetEntityName != NULL_STRING )
	{
		return gEntList.FindEntityByName( pEntity, m_targetEntityName, pActivator );
	}
	else
	{
		float radius = m_radius;
		if ( radius <= 0 )
		{
			// Use the same radius as combat
			radius = m_damage * 2.5;
		}
		return gEntList.FindEntityInSphere( pEntity, GetAbsOrigin(), radius );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysExplosion::InputExplode( inputdata_t &inputdata )
{
	Explode( inputdata.pActivator );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysExplosion::Explode( CBaseEntity *pActivator )
{
	CBaseEntity *pEntity = NULL;
	float		adjustedDamage, falloff, flDist;
	Vector		vecSpot;

	falloff = 1.0 / 2.5;

	// iterate on all entities in the vicinity.
	// I've removed the traceline heuristic from phys explosions. SO right now they will
	// affect entities through walls. (sjb)
	// UNDONE: Try tracing world-only?
	while ((pEntity = FindEntity(pEntity, pActivator)) != NULL)
	{
		// UNDONE: Ask the object if it should get force if it's not MOVETYPE_VPHYSICS?
		if ( pEntity->m_takedamage != DAMAGE_NO && (pEntity->GetMoveType() == MOVETYPE_VPHYSICS || (pEntity->VPhysicsGetObject() && !pEntity->IsPlayer())) )
		{
			vecSpot = pEntity->BodyTarget( GetAbsOrigin() );
			
			// decrease damage for an ent that's farther from the bomb.
			flDist = ( GetAbsOrigin() - vecSpot ).Length();

			if( m_radius == 0 || flDist <= m_radius )
			{
				adjustedDamage =  flDist * falloff;
				adjustedDamage = m_damage - adjustedDamage;
		
				if ( adjustedDamage < 0 )
				{
					adjustedDamage = 0;
				}

				CTakeDamageInfo info( this, this, adjustedDamage, DMG_BLAST );
				CalculateExplosiveDamageForce( &info, (vecSpot - GetAbsOrigin()), GetAbsOrigin() );

				if ( HasSpawnFlags( SF_PHYSEXPLOSION_NODAMAGE ) )
				{
					pEntity->VPhysicsTakeDamage( info );
				}
				else
				{
					pEntity->TakeDamage( info );
				}
			}
		}
	}
}


//==================================================
// CPhysImpact
//==================================================

#define	bitsPHYSIMPACT_NOFALLOFF		0x00000001
#define	bitsPHYSIMPACT_INFINITE_LENGTH	0x00000002
#define	bitsPHYSIMPACT_IGNORE_MASS		0x00000004

#define	DEFAULT_EXPLODE_DISTANCE	256
LINK_ENTITY_TO_CLASS( env_physimpact, CPhysImpact );

BEGIN_DATADESC( CPhysImpact )

	DEFINE_KEYFIELD( CPhysImpact, m_damage,				FIELD_FLOAT,	"magnitude" ),
	DEFINE_KEYFIELD( CPhysImpact, m_distance,			FIELD_FLOAT,	"distance" ),
	DEFINE_KEYFIELD( CPhysImpact, m_directionEntityName,FIELD_STRING,	"directionentityname" ),

	// Function pointers
	DEFINE_FUNCTION( CPhysImpact, PointAtEntity ),

	DEFINE_INPUTFUNC( CPhysImpact, FIELD_VOID, "Impact", InputImpact ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysImpact::Activate( void )
{
	BaseClass::Activate();
	
	//If we have a direction target, setup to point at it
	if ( m_directionEntityName != NULL_STRING )
	{
		PointAtEntity();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysImpact::Spawn( void )
{
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	SetModelName( NULL_STRING );

	//If not targetted, and no distance is set, give it a default value
	if ( m_distance == 0 )
	{
		m_distance = DEFAULT_EXPLODE_DISTANCE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysImpact::PointAtEntity( void )
{
	//If we're not targetted at anything, don't bother
	if ( m_directionEntityName == NULL_STRING )
		return;

	UTIL_PointAtNamedEntity( this, m_directionEntityName );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CPhysImpact::InputImpact( inputdata_t &inputdata )
{
	Vector	dir;
	trace_t	trace;

	AngleVectors( GetAbsAngles(), &dir );
	
	//Setup our trace information
	float	dist	= HasSpawnFlags( bitsPHYSIMPACT_INFINITE_LENGTH ) ? MAX_TRACE_LENGTH : m_distance;
	Vector	start	= GetAbsOrigin();
	Vector	end		= start + ( dir * dist );

	//Trace out
	UTIL_TraceLine( start, end, MASK_SHOT, this, 0, &trace );

	if ( trace.fraction != 1.0 )
	{
		CBaseEntity	*pEnt = trace.m_pEnt;
	
		IPhysicsObject *pPhysics = pEnt->VPhysicsGetObject();
		//If the entity is valid, hit it
		if ( ( pEnt != NULL  ) && ( pPhysics != NULL ) )
		{
			//Damage falls off unless specified or the ray's length is infinite
			float	damage = HasSpawnFlags( bitsPHYSIMPACT_NOFALLOFF | bitsPHYSIMPACT_INFINITE_LENGTH ) ? 
								m_damage : (m_damage * (1.0f-trace.fraction));
			
			if ( HasSpawnFlags( bitsPHYSIMPACT_IGNORE_MASS ) )
			{
				damage *= pPhysics->GetMass();
			}

			pPhysics->ApplyForceOffset( -damage * trace.plane.normal * phys_pushscale.GetFloat(), trace.endpos );
		}
	}
}


class CSimplePhysicsBrush : public CBaseEntity
{
	DECLARE_CLASS( CSimplePhysicsBrush, CBaseEntity );
public:
	void Spawn()
	{
		SetModel( STRING( GetModelName() ) );
		SetMoveType( MOVETYPE_VPHYSICS );
		SetSolid( SOLID_VPHYSICS );
		m_takedamage = DAMAGE_EVENTS_ONLY;
		NetworkStateManualMode( true );
	}
	void VPhysicsUpdate( IPhysicsObject *pPhysics )
	{
		NetworkStateChanged();
		BaseClass::VPhysicsUpdate( pPhysics );
	}
};

LINK_ENTITY_TO_CLASS( simple_physics_brush, CSimplePhysicsBrush );

class CSimplePhysicsProp : public CBaseProp
{
	DECLARE_CLASS( CSimplePhysicsProp, CBaseProp );

public:
	void Spawn()
	{
		BaseClass::Spawn();
		SetMoveType( MOVETYPE_VPHYSICS );
		SetSolid( SOLID_VPHYSICS );
		m_takedamage = DAMAGE_EVENTS_ONLY;
		NetworkStateManualMode( true );
	}
	void VPhysicsUpdate( IPhysicsObject *pPhysics )
	{
		NetworkStateChanged();
		BaseClass::VPhysicsUpdate( pPhysics );
	}
};

LINK_ENTITY_TO_CLASS( simple_physics_prop, CSimplePhysicsProp );

// UNDONE: Is this worth it?, just recreate the object instead? (that happens when this returns false anyway)
// recreating works, but is more expensive and won't inherit properties (velocity, constraints, etc)
bool TransferPhysicsObject( CBaseEntity *pFrom, CBaseEntity *pTo )
{
	IPhysicsObject *pVPhysics = pFrom->VPhysicsGetObject();
	if ( !pVPhysics || pVPhysics->IsStatic() )
		return false;

	// clear out the pointer so it won't get deleted
	pFrom->VPhysicsSwapObject( NULL );
	// remove any AI behavior bound to it
	pVPhysics->RemoveShadowController();
	// transfer to the new owner
	pTo->VPhysicsSetObject( pVPhysics );
	pVPhysics->SetGameData( (void *)pTo );
	pTo->VPhysicsUpdate( pVPhysics );
	
	// may have been temporarily disabled by the old object
	pVPhysics->EnableMotion( true );
	pVPhysics->EnableGravity( true );
	
	// Update for the new entity solid type
	pVPhysics->RecheckCollisionFilter();

	return true;
}

// UNDONE: Move/rename this function
static CBaseEntity *CreateSimplePhysicsObject( CBaseEntity *pEntity, bool createAsleep )
{
	CBaseEntity *pPhysEntity = NULL;
	int modelindex = pEntity->GetModelIndex();
	const model_t *model = modelinfo->GetModel( modelindex );
	if ( model && modelinfo->GetModelType(model) == mod_brush )
	{
		pPhysEntity = CreateEntityByName( "simple_physics_brush" );
	}
	else
	{
		pPhysEntity = CreateEntityByName( "simple_physics_prop" );
	}

	pPhysEntity->KeyValue( "model", STRING(pEntity->GetModelName()) );
	pPhysEntity->SetAbsOrigin( pEntity->GetAbsOrigin() );
	pPhysEntity->SetAbsAngles( pEntity->GetAbsAngles() );
	pPhysEntity->Spawn();
	if ( !TransferPhysicsObject( pEntity, pPhysEntity ) )
	{
		pPhysEntity->VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
	}
	if ( !createAsleep )
	{
		pPhysEntity->VPhysicsGetObject()->Wake();
	}
	return pPhysEntity;
}

#define SF_CONVERT_ASLEEP		0x0001

class CPhysConvert : public CLogicalEntity
{
	DECLARE_CLASS( CPhysConvert, CLogicalEntity );

public:
	COutputEvent m_OnConvert;	

	// Input handlers
	void InputConvertTarget( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	string_t		m_swapModel;
};

LINK_ENTITY_TO_CLASS( phys_convert, CPhysConvert );

BEGIN_DATADESC( CPhysConvert )

	DEFINE_KEYFIELD( CPhysConvert, m_swapModel, FIELD_STRING, "swapmodel" ),
	
	// Inputs
	DEFINE_INPUTFUNC( CPhysConvert, FIELD_VOID, "ConvertTarget", InputConvertTarget ),

	// Outputs
	DEFINE_OUTPUT( CPhysConvert, m_OnConvert, "OnConvert"),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Input handler that converts our target to a physics object.
//-----------------------------------------------------------------------------
void CPhysConvert::InputConvertTarget( inputdata_t &inputdata )
{
	bool createAsleep = HasSpawnFlags(SF_CONVERT_ASLEEP);
	// Fire output
	m_OnConvert.FireOutput( inputdata.pActivator, this );

	CBaseEntity *entlist[512];
	CBaseEntity *pSwap = gEntList.FindEntityByName( NULL, m_swapModel, inputdata.pActivator );
	CBaseEntity *pEntity = NULL;
	
	int count = 0;
	while ( (pEntity = gEntList.FindEntityByName( pEntity, m_target, inputdata.pActivator )) != NULL )
	{
		entlist[count++] = pEntity;
		if ( count >= ARRAYSIZE(entlist) )
			break;
	}

	// if we're swapping to model out, don't loop over more than one object
	// multiple objects with the same brush model will render, but the dynamic lights
	// and decals will be shared between the two instances...
	if ( pSwap && count > 0 )
	{
		count = 1;
	}

	for ( int i = 0; i < count; i++ )
	{
		pEntity = entlist[i];

		// don't convert something that is already physics based
		if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
		{
			Msg( "ERROR phys_convert %s ! Already MOVETYPE_VPHYSICS\n", STRING(pEntity->m_iClassname) );
			continue;
		}

		UnlinkFromParent( pEntity );

		if ( pSwap )
		{
			// we can't reuse this physics object, so kill it
			pEntity->VPhysicsDestroyObject();
			pEntity->SetModel( STRING(pSwap->GetModelName()) );
		}

		CBaseEntity *pPhys = CreateSimplePhysicsObject( pEntity, createAsleep );
		
		// created phys object, now move hierarchy over
		if ( pPhys )
		{
			pPhys->SetName( pEntity->GetEntityName() );
			TransferChildren( pEntity, pPhys );
			pEntity->AddSolidFlags( FSOLID_NOT_SOLID );
			pEntity->m_fEffects |= EF_NODRAW;
			UTIL_Relink( pEntity );
			UTIL_Remove( pEntity );
		}
	}
}

//============================================================================================================
// PHYS MAGNET
//============================================================================================================
#define SF_MAGNET_ASLEEP			0x0001
#define SF_MAGNET_MOTIONDISABLED	0x0002
#define SF_MAGNET_SUCK				0x0004
#define SF_MAGNET_ALLOWROTATION		0x0008

LINK_ENTITY_TO_CLASS( phys_magnet, CPhysMagnet );

// BUGBUG: This won't work!  Right now you can't save physics pointers inside an embedded type!
BEGIN_SIMPLE_DATADESC( magnetted_objects_t )

	DEFINE_PHYSPTR( magnetted_objects_t, pConstraint ),
	DEFINE_FIELD( magnetted_objects_t,	 hEntity,	FIELD_EHANDLE	),

END_DATADESC()

BEGIN_DATADESC( CPhysMagnet )
	// Outputs
	DEFINE_OUTPUT( CPhysMagnet, m_OnMagnetAttach, "OnAttach" ),
	DEFINE_OUTPUT( CPhysMagnet, m_OnMagnetDetach, "OnDetach" ),

	// Keys
	DEFINE_KEYFIELD( CPhysMagnet, m_massScale, FIELD_FLOAT, "massScale" ),
	DEFINE_KEYFIELD( CPhysMagnet, m_iszOverrideScript, FIELD_STRING, "overridescript" ),
	DEFINE_KEYFIELD( CPhysMagnet, m_iMaxObjectsAttached, FIELD_INTEGER, "maxobjects" ),
	DEFINE_KEYFIELD( CPhysMagnet, m_forceLimit, FIELD_FLOAT, "forcelimit" ),
	DEFINE_KEYFIELD( CPhysMagnet, m_torqueLimit, FIELD_FLOAT, "torquelimit" ),

	DEFINE_UTLVECTOR( CPhysMagnet, m_MagnettedEntities, FIELD_EMBEDDED ),
	DEFINE_PHYSPTR( CPhysMagnet, m_pConstraintGroup ),

	DEFINE_FIELD( CPhysMagnet, m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPhysMagnet, m_bHasHitSomething, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPhysMagnet, m_flTotalMass, FIELD_FLOAT ),
	DEFINE_FIELD( CPhysMagnet, m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CPhysMagnet, m_flNextSuckTime, FIELD_FLOAT ),

	// Inputs
	DEFINE_INPUTFUNC( CPhysMagnet, FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( CPhysMagnet, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CPhysMagnet, FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPhysMagnet::CPhysMagnet( void )
{
	m_forceLimit = 0;
	m_torqueLimit = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPhysMagnet::~CPhysMagnet( void )
{
	DetachAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::Spawn( void )
{
	Precache();

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );
	SetModel( STRING( GetModelName() ) );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	solid_t tmpSolid;
	PhysModelParseSolid( tmpSolid, this, GetModelIndex() );
	if ( m_massScale > 0 )
	{
		float mass = tmpSolid.params.mass * m_massScale;
		mass = clamp( mass, 0.5, 1e6 );
		tmpSolid.params.mass = mass;
	}
	PhysSolidOverride( tmpSolid, m_iszOverrideScript );
	VPhysicsInitNormal( GetSolid(), GetSolidFlags(), true, &tmpSolid );

	// Wake it up if not asleep
	if ( !HasSpawnFlags(SF_MAGNET_ASLEEP) )
	{
		VPhysicsGetObject()->Wake();
	}

	if ( HasSpawnFlags(SF_MAGNET_MOTIONDISABLED) )
	{
		VPhysicsGetObject()->EnableMotion( false );
	}

	m_bActive = true;
	m_pConstraintGroup = NULL;
	m_flTotalMass = 0;
	m_flNextSuckTime = 0;

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::Precache( void )
{
	PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::Touch( CBaseEntity *pOther )
{
	// Ignore triggers
	if ( pOther->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		return;

	m_bHasHitSomething = true;

	// Don't pickup if we're not active
	if ( !m_bActive )
		return;

	// Hit our maximum?
	if ( m_iMaxObjectsAttached && m_iMaxObjectsAttached <= GetNumAttachedObjects() )
		return;

	// Make sure it's made of metal
	trace_t tr = GetTouchTrace();
	char cTexType = TEXTURETYPE_Find( &tr );
	if ( cTexType != CHAR_TEX_METAL && cTexType != CHAR_TEX_COMPUTER )
	{
		// See if the model is set to be metal
		if ( Q_strncmp( Studio_GetDefaultSurfaceProps( GetModelPtr() ), "metal", 5 ) )
			return;
	}

	IPhysicsObject *pPhysics = pOther->VPhysicsGetObject();
	if ( pPhysics && pOther->GetMoveType() == MOVETYPE_VPHYSICS && pPhysics->IsMoveable() )
	{
		// Make sure we haven't already got this sucker on the magnet
		int iCount = m_MagnettedEntities.Count();
		for ( int i = 0; i < iCount; i++ )
		{
			if ( m_MagnettedEntities[i].hEntity == pOther )
				return;
		}

		// We want to cast a long way to ensure our shadow shows up
		pOther->SetShadowCastDistance( 2048 );

		// Create a constraint between the magnet and this sucker
		IPhysicsObject *pMagnetPhysObject = VPhysicsGetObject();
		Assert( pMagnetPhysObject );

		magnetted_objects_t newEntityOnMagnet;
		newEntityOnMagnet.hEntity = pOther;

		// Use the right constraint
		if ( HasSpawnFlags( SF_MAGNET_ALLOWROTATION ) )
		{
			constraint_ballsocketparams_t ballsocket;
			ballsocket.Defaults();
			ballsocket.constraint.Defaults();
			ballsocket.constraint.forceLimit = lbs2kg(m_forceLimit);
			ballsocket.constraint.torqueLimit = lbs2kg(m_torqueLimit);

			pMagnetPhysObject->WorldToLocal( ballsocket.constraintPosition[0], tr.endpos );
			pPhysics->WorldToLocal( ballsocket.constraintPosition[1], tr.endpos );

			//newEntityOnMagnet.pConstraint = physenv->CreateBallsocketConstraint( pMagnetPhysObject, pPhysics, m_pConstraintGroup, ballsocket );
			newEntityOnMagnet.pConstraint = physenv->CreateBallsocketConstraint( pMagnetPhysObject, pPhysics, NULL, ballsocket );
		}
		else
		{
			constraint_fixedparams_t fixed;
			fixed.Defaults();
			fixed.InitWithCurrentObjectState( pMagnetPhysObject, pPhysics );
			fixed.constraint.Defaults();
			fixed.constraint.forceLimit = lbs2kg(m_forceLimit);
			fixed.constraint.torqueLimit = lbs2kg(m_torqueLimit);

			// FIXME: Use the magnet's constraint group.
			//newEntityOnMagnet.pConstraint = physenv->CreateFixedConstraint( pMagnetPhysObject, pPhysics, m_pConstraintGroup, fixed );
			newEntityOnMagnet.pConstraint = physenv->CreateFixedConstraint( pMagnetPhysObject, pPhysics, NULL, fixed );
		}

		newEntityOnMagnet.pConstraint->SetGameData( (void *) this );
		m_MagnettedEntities.AddToTail( newEntityOnMagnet );

		m_flTotalMass += pPhysics->GetMass();
	}

	DoMagnetSuck( pOther );

	m_OnMagnetAttach.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	m_bHasHitSomething = true;
	DoMagnetSuck( pEvent->pEntities[!index] );

	BaseClass::VPhysicsCollision( index, pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::DoMagnetSuck( CBaseEntity *pOther )
{
	if ( !HasSpawnFlags( SF_MAGNET_SUCK ) )
		return;

	if ( !m_bActive )
		return;

	// Don't repeatedly suck
	if ( m_flNextSuckTime > gpGlobals->curtime )
		return;
	
	// Look for physics objects underneath the magnet and suck them onto it
	Vector vecCheckPos, vecSuckPoint;
	VectorTransform( Vector(0,0,-96), EntityToWorldTransform(), vecCheckPos );
	VectorTransform( Vector(0,0,-64), EntityToWorldTransform(), vecSuckPoint );

	CBaseEntity *pEntities[20];
	int iNumEntities = UTIL_EntitiesInSphere( pEntities, 20, vecCheckPos, 80.0, 0 );
	for ( int i = 0; i < iNumEntities; i++ )
	{
		CBaseEntity *pEntity = pEntities[i];
		if ( !pEntity || pEntity == pOther )
			continue;

		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if ( pPhys && pEntity->GetMoveType() == MOVETYPE_VPHYSICS && pPhys->GetMass() < 5000 )
		{
			// Do we have line of sight to it?
			trace_t tr;
			UTIL_TraceLine( GetAbsOrigin(), pEntity->GetAbsOrigin(), MASK_SHOT, this, 0, &tr );
			if ( tr.fraction == 1.0 || tr.m_pEnt == pEntity )
			{
				// Pull it towards the magnet
				Vector vecVelocity = (vecSuckPoint - pEntity->GetAbsOrigin());
				VectorNormalize(vecVelocity);
				vecVelocity *= 5 * pPhys->GetMass();
				pPhys->AddVelocity( &vecVelocity, NULL );
			}
		}
	}

	m_flNextSuckTime = gpGlobals->curtime + 2.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::SetConstraintGroup( IPhysicsConstraintGroup *pGroup )
{
	m_pConstraintGroup = pGroup;
}

//-----------------------------------------------------------------------------
// Purpose: Make the magnet active
//-----------------------------------------------------------------------------
void CPhysMagnet::InputTurnOn( inputdata_t &inputdata )
{
	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: Make the magnet inactive. Drop everything it's got hooked on.
//-----------------------------------------------------------------------------
void CPhysMagnet::InputTurnOff( inputdata_t &inputdata )
{
	m_bActive = false;
	DetachAll();
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the magnet's active state
//-----------------------------------------------------------------------------
void CPhysMagnet::InputToggle( inputdata_t &inputdata )
{
	if ( m_bActive )
	{
		InputTurnOff( inputdata );
	}
	else
	{
		InputTurnOn( inputdata );
	}
}

//-----------------------------------------------------------------------------
// Purpose: One of our magnet constraints broke
//-----------------------------------------------------------------------------
void CPhysMagnet::ConstraintBroken( IPhysicsConstraint *pConstraint )
{
	// Find the entity that was constrained and release it
	int iCount = m_MagnettedEntities.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		if ( m_MagnettedEntities[i].pConstraint == pConstraint )
		{
			IPhysicsObject *pPhysObject = m_MagnettedEntities[i].hEntity->VPhysicsGetObject();
			m_flTotalMass -= pPhysObject->GetMass();

			m_MagnettedEntities.Remove(i);
			break;
		}
	}

	m_OnMagnetDetach.FireOutput( this, this );

	physenv->DestroyConstraint( pConstraint  );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::DetachAll( void )
{
	// Make sure we haven't already got this sucker on the magnet
	int iCount = m_MagnettedEntities.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		/*
		// It looks bad when the shadow immediately vanishes from a high-up object on releasing it.
		// We could delay the shadow removing for a few seconds?
		if ( m_MagnettedEntities[i].hEntity )
		{
			m_MagnettedEntities[i].hEntity->SetShadowCastDistance( 0 );
		}
		*/
		physenv->DestroyConstraint( m_MagnettedEntities[i].pConstraint  );
	}

	m_MagnettedEntities.Purge();
	m_flTotalMass = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPhysMagnet::GetNumAttachedObjects( void )
{
	return m_MagnettedEntities.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CPhysMagnet::GetTotalMassAttachedObjects( void )
{
	return m_flTotalMass;
}
