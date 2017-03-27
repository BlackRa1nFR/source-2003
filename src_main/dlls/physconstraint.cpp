//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Physics constraint entities
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "physics.h"
#include "entityoutput.h"
#include "engine/IEngineSound.h"
#include "vphysics/constraints.h"
#include "igamesystem.h"
#include "physics_saverestore.h"
#include "vcollide_parse.h"

#define SF_CONSTRAINT_DISABLE_COLLISION			0x0001
#define SF_SLIDE_LIMIT_ENDS						0x0002
#define SF_PULLEY_RIGID							0x0002
#define SF_LENGTH_RIGID							0x0002
#define SF_RAGDOLL_FREEMOVEMENT					0x0002
#define SF_CONSTRAINT_START_INACTIVE			0x0004

struct hl_constraint_info_t
{
	hl_constraint_info_t() 
	{ 
		pObjects[0] = pObjects[1] = NULL;
		anchorPosition[0].Init();
		anchorPosition[1].Init();
		swapped = false; 
	}
	IPhysicsObject	*pObjects[2];
	Vector			anchorPosition[2];
	bool			swapped;
};

struct constraint_anchor_t
{
	Vector		localOrigin;
	EHANDLE		hEntity;
	string_t	name;
};

class CAnchorList : public CAutoGameSystem
{
public:
	void LevelShutdownPostEntity() 
	{
		m_list.Purge();
	}

	void AddToList( string_t name, CBaseEntity *pEntity, const Vector &localCoordinate )
	{
		int index = m_list.AddToTail();
		constraint_anchor_t *pAnchor = &m_list[index];

		pAnchor->hEntity = pEntity;
		pAnchor->name = name;
		pAnchor->localOrigin = localCoordinate;
	}
	constraint_anchor_t *Find( string_t name )
	{
		for ( int i = m_list.Count()-1; i >=0; i-- )
		{
			if ( FStrEq( STRING(m_list[i].name), STRING(name) ) )
			{
				return &m_list[i];
			}
		}
		return NULL;
	}

private:
	CUtlVector<constraint_anchor_t>	m_list;
};

static CAnchorList g_AnchorList;

class CConstraintAnchor : public CPointEntity
{
	DECLARE_CLASS( CConstraintAnchor, CPointEntity );
public:
	void Spawn( void )
	{
		if ( GetParent() )
		{
			g_AnchorList.AddToList( GetEntityName(),  GetParent(), GetLocalOrigin() );
			UTIL_Remove( this );
		}
	}
};

LINK_ENTITY_TO_CLASS( info_constraint_anchor, CConstraintAnchor );

class CPhysConstraintSystem : public CLogicalEntity
{
	DECLARE_CLASS( CPhysConstraintSystem, CLogicalEntity );
public:

	void Spawn();
	IPhysicsConstraintGroup *GetVPhysicsGroup() { return m_pMachine; }

	DECLARE_DATADESC();
private:
	IPhysicsConstraintGroup *m_pMachine;
};

BEGIN_DATADESC( CPhysConstraintSystem )
	DEFINE_PHYSPTR( CPhysConstraintSystem, m_pMachine ),
END_DATADESC()


void CPhysConstraintSystem::Spawn()
{
	m_pMachine = physenv->CreateConstraintGroup();
}

LINK_ENTITY_TO_CLASS( phys_constraintsystem, CPhysConstraintSystem );

void PhysTeleportConstrainedEntity( CBaseEntity *pTeleportSource, IPhysicsObject *pObject0, IPhysicsObject *pObject1, const Vector &prevPosition, const QAngle &prevAngles, bool physicsRotate )
{
	// teleport the other object
	CBaseEntity *pEntity0 = static_cast<CBaseEntity *> (pObject0->GetGameData());
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *> (pObject1->GetGameData());
	if ( !pEntity0 || !pEntity1 )
		return;

	// figure out which entity needs to be fixed up (the one that isn't pTeleportSource)
	CBaseEntity *pFixup = pEntity1;
	// teleport the other object
	if ( pTeleportSource != pEntity0 )
	{
		if ( pTeleportSource != pEntity1 )
		{
			Msg("Bogus teleport notification!!\n");
			return;
		}
		pFixup = pEntity0;
	}

	// constraint doesn't move this entity
	if ( pFixup->GetMoveType() != MOVETYPE_VPHYSICS )
		return;

	QAngle oldAngles = prevAngles;

	if ( !physicsRotate )
	{
		oldAngles = pTeleportSource->GetAbsAngles();
	}

	matrix3x4_t startCoord, startInv, endCoord, xform;
	AngleMatrix( oldAngles, prevPosition, startCoord );
	MatrixInvert( startCoord, startInv );
	ConcatTransforms( pTeleportSource->EntityToWorldTransform(), startInv, xform );
	QAngle fixupAngles;
	Vector fixupPos;

	ConcatTransforms( xform, pFixup->EntityToWorldTransform(), endCoord );
	MatrixAngles( endCoord, fixupAngles, fixupPos );
	pFixup->Teleport( &fixupPos, &fixupAngles, NULL );
}

class CPhysConstraint : public CLogicalEntity
{
	DECLARE_CLASS( CPhysConstraint, CLogicalEntity );
public:

	CPhysConstraint();
	~CPhysConstraint();

	DECLARE_DATADESC();

	void Spawn( void );
	void Precache( void );
	void Activate( void );

	void ClearStaticFlag( IPhysicsObject *pObj )
	{
		if ( !pObj )
			return;
		unsigned short gameFlags = pObj->GetGameFlags();
		gameFlags &= ~FVPHYSICS_CONSTRAINT_STATIC;
		pObj->SetGameFlags( gameFlags );

	}

	void Deactivate()
	{
		m_pConstraint->Deactivate();
		ClearStaticFlag( m_pConstraint->GetReferenceObject() );
		ClearStaticFlag( m_pConstraint->GetAttachedObject() );
		if ( m_spawnflags & SF_CONSTRAINT_DISABLE_COLLISION )
		{
			// constraint may be getting deactivated because an object got deleted, so check them here.
			IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
			IPhysicsObject *pAtt = m_pConstraint->GetAttachedObject();
			if ( pRef && pAtt )
			{
				physenv->EnableCollisions( pRef, pAtt );
				if ( !pRef->IsStatic() )
				{
					pRef->RecheckCollisionFilter();
				}
				if ( !pAtt->IsStatic() )
				{
					pAtt->RecheckCollisionFilter();
				}
			}
		}
	}

	void OnBreak( void )
	{
		Deactivate();
		if ( m_breakSound != NULL_STRING )
		{
			CPASAttenuationFilter filter( this, ATTN_STATIC );
			EmitSound( filter, entindex(), CHAN_STATIC, STRING(m_breakSound), VOL_NORM, ATTN_STATIC );
		}
		m_OnBreak.FireOutput( this, this );
		UTIL_Remove( this );
	}

	void InputBreak( inputdata_t &inputdata )
	{
		m_pConstraint->Deactivate();
		OnBreak();
	}

	void InputOnBreak( inputdata_t &inputdata )
	{
		OnBreak();
	}

	void InputTurnOn( inputdata_t &inputdata )
	{
		ActivateConstraint();
	}
	void InputTurnOff( inputdata_t &inputdata )
	{
		Deactivate();
	}

	void GetBreakParams( constraint_breakableparams_t &params )
	{
		params.Defaults();
		params.forceLimit = lbs2kg(m_forceLimit);
		params.torqueLimit = lbs2kg(m_torqueLimit);
	}

	void NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params );

protected:	
	void GetConstraintObjects( hl_constraint_info_t &info );
	bool ActivateConstraint( void );
	virtual IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info ) = 0;

	IPhysicsConstraint	*m_pConstraint;

	// These are "template" values used to construct the hinge
	string_t		m_nameAttach1;
	string_t		m_nameAttach2;
	string_t		m_breakSound;
	string_t		m_nameSystem;
	float			m_forceLimit;
	float			m_torqueLimit;
	unsigned int	m_teleportTick;

	COutputEvent	m_OnBreak;
};

BEGIN_DATADESC( CPhysConstraint )

	DEFINE_PHYSPTR( CPhysConstraint, m_pConstraint ),

	DEFINE_KEYFIELD( CPhysConstraint, m_nameSystem, FIELD_STRING, "constraintsystem" ),
	DEFINE_KEYFIELD( CPhysConstraint, m_nameAttach1, FIELD_STRING, "attach1" ),
	DEFINE_KEYFIELD( CPhysConstraint, m_nameAttach2, FIELD_STRING, "attach2" ),
	DEFINE_KEYFIELD( CPhysConstraint, m_breakSound, FIELD_SOUNDNAME, "breaksound" ),
	DEFINE_KEYFIELD( CPhysConstraint, m_forceLimit, FIELD_FLOAT, "forcelimit" ),
	DEFINE_KEYFIELD( CPhysConstraint, m_torqueLimit, FIELD_FLOAT, "torquelimit" ),
//	DEFINE_FIELD( CPhysConstraint, m_teleportTick, FIELD_INTEGER ),

	DEFINE_OUTPUT( CPhysConstraint, m_OnBreak, "OnBreak" ),

	DEFINE_INPUTFUNC( CPhysConstraint, FIELD_VOID, "Break", InputBreak ),
	DEFINE_INPUTFUNC( CPhysConstraint, FIELD_VOID, "ConstraintBroken", InputOnBreak ),

	DEFINE_INPUTFUNC( CPhysConstraint, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CPhysConstraint, FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()


CPhysConstraint::CPhysConstraint( void )
{
	m_pConstraint = NULL;
	m_nameAttach1 = NULL_STRING;
	m_nameAttach2 = NULL_STRING;
	m_forceLimit = 0;
	m_torqueLimit = 0;
	m_teleportTick = 0xFFFFFFFF;
}

CPhysConstraint::~CPhysConstraint()
{
	physenv->DestroyConstraint( m_pConstraint );
}

void CPhysConstraint::Precache( void )
{
	if ( m_breakSound != NULL_STRING )
	{
		enginesound->PrecacheSound( STRING(m_breakSound) );
	}
}

void CPhysConstraint::Spawn( void )
{
	BaseClass::Spawn();

	Precache();
}

extern IPhysicsObject *FindPhysicsObject( const char *pName );

void FindPhysicsAnchor( string_t name, hl_constraint_info_t &info, int index )
{
	constraint_anchor_t *pAnchor = g_AnchorList.Find( name );
	if ( pAnchor )
	{
		CBaseEntity *pEntity = pAnchor->hEntity;
		if ( pEntity )
		{
			info.anchorPosition[index] = pAnchor->localOrigin;
			info.pObjects[index] = pAnchor->hEntity->VPhysicsGetObject();
		}
		else
		{
			pAnchor = NULL;
		}
	}
	if ( !pAnchor )
	{
		info.anchorPosition[index] = vec3_origin;
		info.pObjects[index] = FindPhysicsObject( STRING(name) );
	}
}

void CPhysConstraint::GetConstraintObjects( hl_constraint_info_t &info )
{
	FindPhysicsAnchor( m_nameAttach1, info, 0 );
	FindPhysicsAnchor( m_nameAttach2, info, 1 );

	// Missing one object, assume the world instead
	if ( info.pObjects[0] == NULL && info.pObjects[1] )
	{
		info.pObjects[0] = g_PhysWorldObject;
	}
	else if ( info.pObjects[0] && !info.pObjects[1] )
	{
		info.pObjects[1] = info.pObjects[0];
		info.pObjects[0] = g_PhysWorldObject;		// Try to make the world object consistently object0 for ease of implementation
		info.swapped = true;
	}
	else if ( info.pObjects[0] && info.pObjects[1] )
	{
		CBaseEntity *pEntity0 = (CBaseEntity *)info.pObjects[0]->GetGameData();
		g_pNotify->AddEntity( this, pEntity0 );

		CBaseEntity *pEntity1 = (CBaseEntity *)info.pObjects[1]->GetGameData();
		g_pNotify->AddEntity( this, pEntity1 );
	}
}

void CPhysConstraint::Activate( void )
{
	BaseClass::Activate();

	if ( !m_pConstraint )
	{
		if ( !ActivateConstraint() )
		{
			UTIL_Remove(this);
		}
	}
}

IPhysicsConstraintGroup *GetConstraintGroup( string_t systemName )
{
	CBaseEntity *pMachine = gEntList.FindEntityByName( NULL, systemName, NULL );

	if ( pMachine )
	{
		CPhysConstraintSystem *pGroup = dynamic_cast<CPhysConstraintSystem *>(pMachine);
		if ( pGroup )
		{
			return pGroup->GetVPhysicsGroup();
		}
	}
	return NULL;
}

bool CPhysConstraint::ActivateConstraint( void )
{
	// A constraint attaches two objects to each other.
	// The constraint is specified in the coordinate frame of the "reference" object
	// and constrains the "attached" object
	hl_constraint_info_t info;
	if ( m_pConstraint )
	{
		// already have a constraint, don't make a new one
		info.pObjects[0] = m_pConstraint->GetReferenceObject();
		info.pObjects[1] = m_pConstraint->GetAttachedObject();
		if ( m_spawnflags & SF_CONSTRAINT_DISABLE_COLLISION )
		{
			physenv->DisableCollisions( info.pObjects[0], info.pObjects[1] );
		}
		m_pConstraint->Activate();
	}
	else
	{
		GetConstraintObjects( info );

		if ( !info.pObjects[0] && !info.pObjects[1] )
		{
			return false;
		}

		IPhysicsConstraintGroup *pGroup = GetConstraintGroup( m_nameSystem );
		m_pConstraint = CreateConstraint( pGroup, info );
		if ( !m_pConstraint )
		{
			return false;
		}
		m_pConstraint->SetGameData( (void *)this );

		if ( pGroup )
		{
			pGroup->Activate();
		}
	}

	if ( m_spawnflags & SF_CONSTRAINT_DISABLE_COLLISION )
	{
		physenv->DisableCollisions( info.pObjects[0], info.pObjects[1] );
	}

	return true;
}

void CPhysConstraint::NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
	// don't recurse
	if ( eventType != NOTIFY_EVENT_TELEPORT || (unsigned int)gpGlobals->tickcount == m_teleportTick )
		return;

	m_teleportTick = gpGlobals->tickcount;

	PhysTeleportConstrainedEntity( pNotify, m_pConstraint->GetReferenceObject(), m_pConstraint->GetAttachedObject(), params.pTeleport->prevOrigin, params.pTeleport->prevAngles, params.pTeleport->physicsRotate );
}

class CPhysHinge : public CPhysConstraint
{
	DECLARE_CLASS( CPhysHinge, CPhysConstraint );

public:
	void Spawn( void );
	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
	{
		if ( m_hinge.worldAxisDirection == vec3_origin )
		{
			DevMsg("ERROR: Hinge with bad data!!!\n" );
			return NULL;
		}
		// BUGBUG: These numbers are very hard to edit
		// Scale by 1000 to make things easier
		// CONSIDER: Unify the units of torque around something other 
		// than HL units (kg * in^2 / s ^2)
		m_hinge.hingeAxis.SetAxisFriction( 0, 0, m_hingeFriction * 1000 );

		return physenv->CreateHingeConstraint( info.pObjects[0], info.pObjects[1], pGroup, m_hinge );
	}

	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			NDebugOverlay::Line(m_hinge.worldPosition, m_hinge.worldPosition + 48 * m_hinge.worldAxisDirection, 0, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	DECLARE_DATADESC();

private:
	constraint_hingeparams_t m_hinge;
	float m_hingeFriction;
};

BEGIN_DATADESC( CPhysHinge )

// Quiet down classcheck
//	DEFINE_FIELD( CPhysHinge, m_hinge, FIELD_??? ),

	DEFINE_KEYFIELD( CPhysHinge, m_hingeFriction, FIELD_FLOAT, "hingefriction" ),
	DEFINE_FIELD( CPhysHinge, m_hinge.worldPosition, FIELD_POSITION_VECTOR ),
	DEFINE_KEYFIELD( CPhysHinge, m_hinge.worldAxisDirection, FIELD_VECTOR, "hingeaxis" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_hinge, CPhysHinge );


void CPhysHinge::Spawn( void )
{
	m_hinge.worldPosition = GetLocalOrigin();
	m_hinge.worldAxisDirection -= GetLocalOrigin();
	VectorNormalize(m_hinge.worldAxisDirection);
	UTIL_SnapDirectionToAxis( m_hinge.worldAxisDirection );

	m_hinge.hingeAxis.SetAxisFriction( 0, 0, 0 );
	GetBreakParams( m_hinge.constraint );
	m_hinge.constraint.strength = 1.0;
}


class CPhysBallSocket : public CPhysConstraint
{
public:
	DECLARE_CLASS( CPhysBallSocket, CPhysConstraint );

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
	{
		constraint_ballsocketparams_t ballsocket;
	
		ballsocket.Defaults();
		
		for ( int i = 0; i < 2; i++ )
		{
			info.pObjects[i]->WorldToLocal( ballsocket.constraintPosition[i], GetAbsOrigin() );
		}
		GetBreakParams( ballsocket.constraint );
		ballsocket.constraint.torqueLimit = 0;

		return physenv->CreateBallsocketConstraint( info.pObjects[0], info.pObjects[1], pGroup, ballsocket );
	}
};

LINK_ENTITY_TO_CLASS( phys_ballsocket, CPhysBallSocket );

class CPhysSlideConstraint : public CPhysConstraint
{
public:
	DECLARE_CLASS( CPhysSlideConstraint, CPhysConstraint );

	DECLARE_DATADESC();
	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

	Vector	m_axisEnd;
	float	m_slideFriction;
};

LINK_ENTITY_TO_CLASS( phys_slideconstraint, CPhysSlideConstraint );

BEGIN_DATADESC( CPhysSlideConstraint )

	DEFINE_KEYFIELD( CPhysSlideConstraint, m_axisEnd, FIELD_POSITION_VECTOR, "slideaxis" ),
	DEFINE_KEYFIELD( CPhysSlideConstraint, m_slideFriction, FIELD_FLOAT, "slidefriction" ),

END_DATADESC()



IPhysicsConstraint *CPhysSlideConstraint::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_slidingparams_t sliding;
	sliding.Defaults();
	GetBreakParams( sliding.constraint );
	sliding.constraint.strength = 1.0;

	Vector axisDirection = m_axisEnd - GetAbsOrigin();
	VectorNormalize( axisDirection );
	UTIL_SnapDirectionToAxis( axisDirection );

	sliding.InitWithCurrentObjectState( info.pObjects[0], info.pObjects[1], axisDirection );
	sliding.friction = m_slideFriction;
	if ( m_spawnflags & SF_SLIDE_LIMIT_ENDS )
	{
		Vector position;
		info.pObjects[1]->GetPosition( &position, NULL );

		sliding.limitMin = DotProduct( axisDirection, GetAbsOrigin() );
		sliding.limitMax = DotProduct( axisDirection, m_axisEnd );
		if ( sliding.limitMax < sliding.limitMin )
		{
			swap( sliding.limitMin, sliding.limitMax );
		}

		// expand limits to make initial position of the attached object valid
		float limit = DotProduct( position, axisDirection );
		if ( limit < sliding.limitMin )
		{
			sliding.limitMin = limit;
		}
		else if ( limit > sliding.limitMax )
		{
			sliding.limitMax = limit;
		}
		// offset so that the current position is 0
		sliding.limitMin -= limit;
		sliding.limitMax -= limit;
	}

	return physenv->CreateSlidingConstraint( info.pObjects[0], info.pObjects[1], pGroup, sliding );
}

//-----------------------------------------------------------------------------
// Purpose: Fixed breakable constraint
//-----------------------------------------------------------------------------
class CPhysFixed : public CPhysConstraint
{
	DECLARE_CLASS( CPhysFixed, CPhysConstraint );
public:
	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );
};

LINK_ENTITY_TO_CLASS( phys_constraint, CPhysFixed );


//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CPhysFixed::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_fixedparams_t fixed;
	fixed.Defaults();
	fixed.InitWithCurrentObjectState( info.pObjects[0], info.pObjects[1] );
	GetBreakParams( fixed.constraint );

	// constraining to the world means object 1 is fixed
	if ( info.pObjects[0] == g_PhysWorldObject )
	{
		unsigned short gameFlags = info.pObjects[1]->GetGameFlags();
		gameFlags |= FVPHYSICS_CONSTRAINT_STATIC;
		info.pObjects[1]->SetGameFlags( gameFlags );
	}

	return physenv->CreateFixedConstraint( info.pObjects[0], info.pObjects[1], pGroup, fixed );
}


//-----------------------------------------------------------------------------
// Purpose: Breakable pulley w/ropes constraint
//-----------------------------------------------------------------------------
class CPhysPulley : public CPhysConstraint
{
	DECLARE_CLASS( CPhysPulley, CPhysConstraint );
public:
	DECLARE_DATADESC();

	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			Vector origin = GetAbsOrigin();
			Vector refPos = origin, attachPos = origin;
			IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
			if ( pRef )
			{
				matrix3x4_t matrix;
				pRef->GetPositionMatrix( matrix );
				VectorTransform( m_offset[0], matrix, refPos );
			}
			IPhysicsObject *pAttach = m_pConstraint->GetAttachedObject();
			if ( pAttach )
			{
				matrix3x4_t matrix;
				pAttach->GetPositionMatrix( matrix );
				VectorTransform( m_offset[1], matrix, attachPos );
			}
			NDebugOverlay::Line( refPos, origin, 0, 255, 0, false, 0 );
			NDebugOverlay::Line( origin, m_position2, 0, 255, 0, false, 0 );
			NDebugOverlay::Line( m_position2, attachPos, 0, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

private:
	Vector		m_position2;
	Vector		m_offset[2];
	float		m_addLength;
	float		m_gearRatio;
};

BEGIN_DATADESC( CPhysPulley )

	DEFINE_KEYFIELD( CPhysPulley, m_position2, FIELD_POSITION_VECTOR, "position2" ),
	DEFINE_AUTO_ARRAY( CPhysPulley, m_offset, FIELD_VECTOR ),
	DEFINE_KEYFIELD( CPhysPulley, m_addLength, FIELD_FLOAT, "addlength" ),
	DEFINE_KEYFIELD( CPhysPulley, m_gearRatio, FIELD_FLOAT, "gearratio" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_pulleyconstraint, CPhysPulley );


//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CPhysPulley::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_pulleyparams_t pulley;
	pulley.Defaults();
	pulley.pulleyPosition[0] = GetAbsOrigin();
	pulley.pulleyPosition[1] = m_position2;

	matrix3x4_t matrix;
	Vector world[2];

	info.pObjects[0]->GetPositionMatrix( matrix );
	VectorTransform( info.anchorPosition[0], matrix, world[0] );
	info.pObjects[1]->GetPositionMatrix( matrix );
	VectorTransform( info.anchorPosition[1], matrix, world[1] );

	for ( int i = 0; i < 2; i++ )
	{
		pulley.objectPosition[i] = info.anchorPosition[i];
		m_offset[i] = info.anchorPosition[i];
	}
	
	pulley.totalLength = m_addLength + 
		(world[0] - pulley.pulleyPosition[0]).Length() + 
		((world[1] - pulley.pulleyPosition[1]).Length() * m_gearRatio);

	if ( m_gearRatio != 0 )
	{
		pulley.gearRatio = m_gearRatio;
	}
	GetBreakParams( pulley.constraint );
	if ( m_spawnflags & SF_PULLEY_RIGID )
	{
		pulley.isRigid = true;
	}

	return physenv->CreatePulleyConstraint( info.pObjects[0], info.pObjects[1], pGroup, pulley );
}

//-----------------------------------------------------------------------------
// Purpose: Breakable rope/length constraint
//-----------------------------------------------------------------------------
class CPhysLength : public CPhysConstraint
{
	DECLARE_CLASS( CPhysLength, CPhysConstraint );
public:
	DECLARE_DATADESC();

	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			Vector origin = GetAbsOrigin();
			Vector refPos = origin, attachPos = origin;
			IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
			if ( pRef )
			{
				matrix3x4_t matrix;
				pRef->GetPositionMatrix( matrix );
				VectorTransform( m_offset[0], matrix, refPos );
			}
			IPhysicsObject *pAttach = m_pConstraint->GetAttachedObject();
			if ( pAttach )
			{
				matrix3x4_t matrix;
				pAttach->GetPositionMatrix( matrix );
				VectorTransform( m_offset[1], matrix, attachPos );
			}
			NDebugOverlay::Line( refPos, attachPos, 0, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

private:
	Vector		m_offset[2];
	Vector		m_vecAttach;
	float		m_addLength;
};

BEGIN_DATADESC( CPhysLength )

	DEFINE_AUTO_ARRAY( CPhysLength, m_offset, FIELD_VECTOR ),
	DEFINE_KEYFIELD( CPhysLength, m_addLength, FIELD_FLOAT, "addlength" ),
	DEFINE_KEYFIELD( CPhysLength, m_vecAttach, FIELD_POSITION_VECTOR, "attachpoint" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_lengthconstraint, CPhysLength );


//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CPhysLength::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_lengthparams_t length;
	length.Defaults();
	Vector position[2];
	position[0] = GetAbsOrigin();
	position[1] = m_vecAttach;
	int index = info.swapped ? 1 : 0;
	length.InitWorldspace( info.pObjects[0], info.pObjects[1], position[index], position[!index] );
	length.totalLength += m_addLength;

	for ( int i = 0; i < 2; i++ )
	{
		m_offset[i] = length.objectPosition[i];
	}
	GetBreakParams( length.constraint );
	if ( m_spawnflags & SF_LENGTH_RIGID )
	{
		length.isRigid = true;
	}

	return physenv->CreateLengthConstraint( info.pObjects[0], info.pObjects[1], pGroup, length );
}

//-----------------------------------------------------------------------------
// Purpose: Limited ballsocket constraint with toggle-able translation constraints
//-----------------------------------------------------------------------------
class CRagdollConstraint : public CPhysConstraint
{
	DECLARE_CLASS( CRagdollConstraint, CPhysConstraint );
public:
	DECLARE_DATADESC();
#if 0
	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			NDebugOverlay::Line( refPos, attachPos, 0, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}
#endif

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

private:
	float		m_xmin;	// constraint limits in degrees
	float		m_xmax;
	float		m_ymin;
	float		m_ymax;
	float		m_zmin;
	float		m_zmax;
};

BEGIN_DATADESC( CRagdollConstraint )

	DEFINE_KEYFIELD( CRagdollConstraint, m_xmin, FIELD_FLOAT, "xmin" ),
	DEFINE_KEYFIELD( CRagdollConstraint, m_xmax, FIELD_FLOAT, "xmax" ),
	DEFINE_KEYFIELD( CRagdollConstraint, m_ymin, FIELD_FLOAT, "ymin" ),
	DEFINE_KEYFIELD( CRagdollConstraint, m_ymax, FIELD_FLOAT, "ymax" ),
	DEFINE_KEYFIELD( CRagdollConstraint, m_zmin, FIELD_FLOAT, "zmin" ),
	DEFINE_KEYFIELD( CRagdollConstraint, m_zmax, FIELD_FLOAT, "zmax" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_ragdollconstraint, CRagdollConstraint );

//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CRagdollConstraint::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_ragdollparams_t ragdoll;
	ragdoll.Defaults();

	matrix3x4_t entityToWorld, worldToEntity;
	info.pObjects[0]->GetPositionMatrix( entityToWorld );
	MatrixInvert( entityToWorld, worldToEntity );
	ConcatTransforms( worldToEntity, EntityToWorldTransform(), ragdoll.constraintToReference );

	info.pObjects[1]->GetPositionMatrix( entityToWorld );
	MatrixInvert( entityToWorld, worldToEntity );
	ConcatTransforms( worldToEntity, EntityToWorldTransform(), ragdoll.constraintToAttached );

	ragdoll.onlyAngularLimits = HasSpawnFlags( SF_RAGDOLL_FREEMOVEMENT ) ? true : false;
	ragdoll.axes[0].SetAxisFriction( m_xmin, m_xmax, 0 );
	ragdoll.axes[1].SetAxisFriction( m_ymin, m_ymax, 0 );
	ragdoll.axes[2].SetAxisFriction( m_zmin, m_zmax, 0 );

	if ( HasSpawnFlags( SF_CONSTRAINT_START_INACTIVE ) )
	{
		ragdoll.isActive = false;
	}

	return physenv->CreateRagdollConstraint( info.pObjects[0], info.pObjects[1], pGroup, ragdoll );
}



class CPhysConstraintEvents : public IPhysicsConstraintEvent
{
	void ConstraintBroken( IPhysicsConstraint *pConstraint )
	{
		CBaseEntity *pEntity = (CBaseEntity *)pConstraint->GetGameData();
		if ( pEntity )
		{
			IPhysicsConstraintEvent *pConstraintEvent = dynamic_cast<IPhysicsConstraintEvent*>( pEntity );
			//Msg("Constraint broken %s\n", pEntity->GetDebugName() );
			if ( pConstraintEvent )
			{
				pConstraintEvent->ConstraintBroken( pConstraint );
			}
			else
			{
				variant_t emptyVariant;
				pEntity->AcceptInput( "ConstraintBroken", NULL, NULL, emptyVariant, 0 );
			}
		}
	}
};

static CPhysConstraintEvents constraintevents;
// registered in physics.cpp
IPhysicsConstraintEvent *g_pConstraintEvents = &constraintevents;
