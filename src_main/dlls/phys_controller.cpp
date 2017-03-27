//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entitylist.h"
#include "physics.h"
#include "vphysics/constraints.h"
#include "physics_saverestore.h"
#include "phys_controller.h"

#define SF_THRUST_STARTACTIVE		0x0001
#define SF_THRUST_FORCE				0x0002
#define SF_THRUST_TORQUE			0x0004
#define SF_THRUST_LOCAL_ORIENTATION	0x0008
#define SF_THRUST_MASS_INDEPENDENT	0x0010
#define SF_THRUST_IGNORE_POS		0x0020

class CPhysThruster;

//-----------------------------------------------------------------------------
// Purpose: This class only implements the IMotionEvent-specific behavior
//			It keeps track of the forces so they can be integrated
//-----------------------------------------------------------------------------
class CConstantForceController : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:
	void Init( IMotionEvent::simresult_e controlType ) 
	{ 
		m_controlType = controlType;
	}

	void SetConstantForce( const Vector &linear, const AngularImpulse &angular );
	void ScaleConstantForce( float scale );
	
	IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );
	IMotionEvent::simresult_e	m_controlType;
	Vector			m_linear;
	AngularImpulse	m_angular;
	Vector			m_linearSave;
	AngularImpulse	m_angularSave;
};

BEGIN_SIMPLE_DATADESC( CConstantForceController )
	DEFINE_FIELD( CConstantForceController,	m_controlType,	FIELD_INTEGER ),
	DEFINE_FIELD( CConstantForceController,	m_linear,		FIELD_VECTOR ),
	DEFINE_FIELD( CConstantForceController,	m_angular,		FIELD_VECTOR ),
	DEFINE_FIELD( CConstantForceController,	m_linearSave,	FIELD_VECTOR ),
	DEFINE_FIELD( CConstantForceController,	m_angularSave,	FIELD_VECTOR ),
END_DATADESC()


void CConstantForceController::SetConstantForce( const Vector &linear, const AngularImpulse &angular )
{
	m_linear = linear;
	m_angular = angular;
	// cache these for scaling later
	m_linearSave = linear;
	m_angularSave = angular;
}

void CConstantForceController::ScaleConstantForce( float scale )
{
	m_linear = m_linearSave * scale;
	m_angular = m_angularSave * scale;
}


IMotionEvent::simresult_e CConstantForceController::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	linear = m_linear;
	angular = m_angular;
	
	return m_controlType;
}

// UNDONE: Make these logical entities
//-----------------------------------------------------------------------------
// Purpose: This is a general entity that has a force/motion controller that 
//			simply integrates a constant linear/angular acceleration
//-----------------------------------------------------------------------------
class CPhysForce : public CPointEntity
{
public:
	DECLARE_CLASS( CPhysForce, CPointEntity );

	CPhysForce();
	~CPhysForce();

	DECLARE_DATADESC();

	virtual void OnRestore( );
	void Spawn( void );
	void Activate( void );

	void ForceOn( void );
	void ForceOff( void );
	void ActivateForce( void );

	// Input handlers
	void InputActivate( inputdata_t &inputdata );
	void InputDeactivate( inputdata_t &inputdata );
	void InputForceScale( inputdata_t &inputdata );

	void SaveForce( void );
	void ScaleForce( float scale );

	// MUST IMPLEMENT THIS IN DERIVED CLASS
	virtual void SetupForces( IPhysicsObject *pPhys, Vector &linear, AngularImpulse &angular ) = 0;

	// optional
	virtual void OnActivate( void ) {}

protected:	
	IPhysicsMotionController	*m_pController;

	string_t		m_nameAttach;
	float			m_force;
	float			m_forceTime;
	EHANDLE			m_attachedObject;

	CConstantForceController m_integrator;
};

BEGIN_DATADESC( CPhysForce )

	DEFINE_PHYSPTR( CPhysForce, m_pController ),
	DEFINE_KEYFIELD( CPhysForce, m_nameAttach, FIELD_STRING, "attach1" ),
	DEFINE_KEYFIELD( CPhysForce, m_force, FIELD_FLOAT, "force" ),
	DEFINE_KEYFIELD( CPhysForce, m_forceTime, FIELD_FLOAT, "forcetime" ),

	DEFINE_FIELD( CPhysForce, m_attachedObject, FIELD_EHANDLE ),
	DEFINE_EMBEDDED( CPhysForce, m_integrator ),
	
	DEFINE_INPUTFUNC( CPhysForce, FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC( CPhysForce, FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC( CPhysForce, FIELD_FLOAT, "scale", InputForceScale ),
	
	// Function Pointers
	DEFINE_FUNCTION( CPhysForce, ForceOff ),

END_DATADESC()


CPhysForce::CPhysForce( void )
{
	m_pController = NULL;
}

CPhysForce::~CPhysForce()
{
	if ( m_pController )
	{
		physenv->DestroyMotionController( m_pController );
	}
}

void CPhysForce::Spawn( void )
{
	if ( m_spawnflags & SF_THRUST_LOCAL_ORIENTATION )
	{
		m_integrator.Init( IMotionEvent::SIM_LOCAL_ACCELERATION );
	}
	else
	{
		m_integrator.Init( IMotionEvent::SIM_GLOBAL_ACCELERATION );
	}

	if ( m_spawnflags & SF_THRUST_STARTACTIVE )
	{
		ForceOn();
	}
}

void CPhysForce::OnRestore( )
{
	BaseClass::OnRestore();

	if ( m_pController )
	{
		m_pController->SetEventHandler( &m_integrator );
	}
}

void CPhysForce::Activate( void )
{
	BaseClass::Activate();
	
	if ( m_attachedObject == NULL )
	{
		m_attachedObject = gEntList.FindEntityByName( NULL, m_nameAttach, NULL );
	}
	
	// Let the derived class set up before we throw the switch
	OnActivate();

	if ( m_pController )
	{
		ActivateForce();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysForce::InputActivate( inputdata_t &inputdata )
{
	ForceOn();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysForce::InputDeactivate( inputdata_t &inputdata )
{
	ForceOff();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysForce::InputForceScale( inputdata_t &inputdata )
{
	ScaleForce( inputdata.value.Float() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysForce::ForceOn( void )
{
	if ( m_pController )
		return;

	ActivateForce();
	if ( m_forceTime )
	{
		SetNextThink( gpGlobals->curtime + m_forceTime );
		SetThink( &CPhysForce::ForceOff );
	}
}


void CPhysForce::ActivateForce( void )
{
	BaseClass::Activate();
	
	IPhysicsObject *pPhys = NULL;
	if ( m_attachedObject )
	{
		pPhys = m_attachedObject->VPhysicsGetObject();
	}
	
	if ( !pPhys )
		return;

	Vector linear;
	AngularImpulse angular;

	SetupForces( pPhys, linear, angular );

	m_integrator.SetConstantForce( linear, angular );
	m_pController = physenv->CreateMotionController( &m_integrator );
	m_pController->AttachObject( pPhys );
	// Make sure the object is simulated
	pPhys->Wake();
}


void CPhysForce::ForceOff( void )
{
	if ( !m_pController )
		return;

	physenv->DestroyMotionController( m_pController );
	m_pController = NULL;
	SetThink( NULL );
	SetNextThink( TICK_NEVER_THINK );
	IPhysicsObject *pPhys = NULL;
	if ( m_attachedObject )
	{
		pPhys = m_attachedObject->VPhysicsGetObject();
		if ( pPhys )
		{
			pPhys->Wake();
		}
	}
}


void CPhysForce::ScaleForce( float scale )
{
	if ( !m_pController )
		ForceOn();

	m_integrator.ScaleConstantForce( scale );
	m_pController->WakeObjects();
}


//-----------------------------------------------------------------------------
// Purpose: A rocket-engine/thruster based on the force controller above
//			Calculate the force (and optional torque) that the engine would create
//-----------------------------------------------------------------------------
class CPhysThruster : public CPhysForce
{
	DECLARE_CLASS( CPhysThruster, CPhysForce );
public:
	DECLARE_DATADESC();

	virtual void OnActivate( void );
	virtual void SetupForces( IPhysicsObject *pPhys, Vector &linear, AngularImpulse &angular );

private:	
	Vector			m_localOrigin;
};

LINK_ENTITY_TO_CLASS( phys_thruster, CPhysThruster );

BEGIN_DATADESC( CPhysThruster )

	DEFINE_FIELD( CPhysThruster, m_localOrigin, FIELD_VECTOR ),

END_DATADESC()



void CPhysThruster::OnActivate( void )
{
	if ( m_attachedObject != NULL )
	{
		// maintain the local relationship with this entity
		// it may move before the thruster is activated
		if ( m_spawnflags & SF_THRUST_IGNORE_POS )
		{
			m_localOrigin = vec3_origin;
		}
		else
		{
			m_localOrigin = GetAbsOrigin() - m_attachedObject->GetAbsOrigin();
		}
	}
}

void CPhysThruster::SetupForces( IPhysicsObject *pPhys, Vector &linear, AngularImpulse &angular )
{
	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );
	forward = forward * m_force;

	// multiply the force by mass (it's actually just an acceleration)
	if ( m_spawnflags & SF_THRUST_MASS_INDEPENDENT )
	{
		forward = forward * pPhys->GetMass();
	}
	
	EntityMatrix xform;
	xform.InitFromEntity( m_attachedObject );
	Vector origin = xform.LocalToWorld( m_localOrigin );
	forward = xform.LocalToWorldRotation( forward );

	// Adjust for the position of the thruster -- apply proper torque)
	pPhys->CalculateVelocityOffset( forward, origin, linear, angular );

	// Stay in local space always if this flag is set
	if ( m_spawnflags & SF_THRUST_LOCAL_ORIENTATION )
	{
		linear = xform.WorldToLocalRotation( linear );
	}

	if ( !(m_spawnflags & SF_THRUST_FORCE) )
	{
		// clear out force
		linear.Init();
	}

	if ( !(m_spawnflags & SF_THRUST_TORQUE) )
	{
		// clear out torque
		angular.Init();
	}
}


//-----------------------------------------------------------------------------
// Purpose: A controllable motor - exerts torque
//-----------------------------------------------------------------------------
class CPhysTorque : public CPhysForce
{
	DECLARE_CLASS( CPhysTorque, CPhysForce );
public:
	DECLARE_DATADESC();
	void Spawn( void );
	virtual void SetupForces( IPhysicsObject *pPhys, Vector &linear, AngularImpulse &angular );
private:	
	Vector m_axis;
};

BEGIN_DATADESC( CPhysTorque )

	DEFINE_KEYFIELD( CPhysTorque, m_axis, FIELD_VECTOR, "axis" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( phys_torque, CPhysTorque );

void CPhysTorque::Spawn( void )
{
	// force spawnflags to agree with implementation of this class
	m_spawnflags |= SF_THRUST_TORQUE | SF_THRUST_MASS_INDEPENDENT;
	m_spawnflags &= ~SF_THRUST_FORCE;

	m_axis -= GetAbsOrigin();
	VectorNormalize(m_axis);
	UTIL_SnapDirectionToAxis( m_axis );
	BaseClass::Spawn();
}

void CPhysTorque::SetupForces( IPhysicsObject *pPhys, Vector &linear, AngularImpulse &angular )
{
	// clear out force
	linear.Init();

	matrix3x4_t matrix;
	pPhys->GetPositionMatrix( matrix );

	// transform motor axis to local space
	Vector axis_ls;
	VectorIRotate( m_axis, matrix, axis_ls );

	// Set torque to be around selected axis
	angular = axis_ls * m_force;
}



//-----------------------------------------------------------------------------
// Purpose: This class only implements the IMotionEvent-specific behavior
//			It keeps track of the forces so they can be integrated
//-----------------------------------------------------------------------------
class CMotorController : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:
	IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );
	float		m_speed;
	float		m_maxTorque;
	Vector		m_axis;
	float		m_inertiaFactor;

	float		m_lastSpeed;
	float		m_lastAcceleration;
	float		m_lastForce;
	float		m_restistanceDamping;
};

BEGIN_SIMPLE_DATADESC( CMotorController )

	DEFINE_FIELD( CMotorController,	m_speed,				FIELD_FLOAT ),
	DEFINE_FIELD( CMotorController,	m_maxTorque,			FIELD_FLOAT ),
	DEFINE_KEYFIELD( CMotorController, m_axis,				FIELD_VECTOR, "axis" ),
	DEFINE_KEYFIELD( CMotorController, m_inertiaFactor,		FIELD_FLOAT, "inertiafactor" ),
	DEFINE_FIELD( CMotorController,	m_lastSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( CMotorController,	m_lastAcceleration,		FIELD_FLOAT ),
	DEFINE_FIELD( CMotorController,	m_lastForce,			FIELD_FLOAT ),
	DEFINE_FIELD( CMotorController,	m_restistanceDamping,	FIELD_FLOAT ),

END_DATADESC()


IMotionEvent::simresult_e CMotorController::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	linear = vec3_origin;
	angular = vec3_origin;

	if ( m_speed == 0 )
		return SIM_NOTHING;

	matrix3x4_t matrix;
	pObject->GetPositionMatrix( matrix );
	AngularImpulse currentRotAxis;
	
	// currentRotAxis is in local space
	pObject->GetVelocity( NULL, &currentRotAxis );
	// transform motor axis to local space
	Vector motorAxis_ls;
	VectorIRotate( m_axis, matrix, motorAxis_ls );
	float currentSpeed = DotProduct( currentRotAxis, motorAxis_ls );
	
	float inertia = DotProductAbs( pObject->GetInertia(), motorAxis_ls );

	// compute absolute acceleration, don't integrate over the timestep
	float accel = m_speed - currentSpeed;
	float rotForce = accel * inertia * m_inertiaFactor;

	// BUGBUG: This heuristic is a little flaky
	// UNDONE: Make a better heuristic for speed control
	if ( fabsf(m_lastAcceleration) > 0 )
	{
		float deltaSpeed = currentSpeed - m_lastSpeed;
		// make sure they are going the same way
		if ( deltaSpeed * accel > 0 )
		{
			float factor = deltaSpeed / m_lastAcceleration;
			factor = 1 - clamp( factor, 0, 1 );
			rotForce += m_lastForce * factor * m_restistanceDamping;
		}
		else 
		{
			if ( currentSpeed != 0 )
			{
				// have we reached a steady state that isn't our target?
				float increase = deltaSpeed / m_lastAcceleration;
				if ( fabsf(increase) < 0.05 )
				{
					rotForce += m_lastForce * m_restistanceDamping;
				}
			}
		}
	}
	// -------------------------------------------------------


	if ( m_maxTorque != 0 )
	{
		if ( rotForce > m_maxTorque )
		{
			rotForce = m_maxTorque;
		}
		else if ( rotForce < -m_maxTorque )
		{
			rotForce = -m_maxTorque;
		}
	}

	m_lastForce = rotForce;
	m_lastAcceleration = (rotForce / inertia);
	m_lastSpeed = currentSpeed;

	// this is in local space
	angular = motorAxis_ls * rotForce;
	
	return SIM_LOCAL_FORCE;
}

#define SF_MOTOR_START_ON		0x0001				// starts on by default
#define SF_MOTOR_NOCOLLIDE		0x0002				// don't collide with world geometry
#define SF_MOTOR_HINGE			0x0004				// motor also acts as a hinge constraining the object to this axis
// NOTE: THIS DOESN'T WORK YET
#define SF_MOTOR_LOCAL			0x0008				// Maintain local relationship with the attached object

class CPhysMotor : public CLogicalEntity
{
	DECLARE_CLASS( CPhysMotor, CLogicalEntity );
public:
	~CPhysMotor();
	DECLARE_DATADESC();
	void Spawn( void );
	void Activate( void );
	void Think( void );

	void TurnOn( void );
	void TargetSpeedChanged( void );
	
	void InputSetTargetSpeed( inputdata_t &inputdata );
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void CalculateAcceleration();

	string_t	m_nameAttach;
	EHANDLE		m_attachedObject;
	float		m_spinUp;
	float		m_additionalAcceleration;
	float		m_targetSpeed;
	float		m_angularAcceleration;
	float		m_lastTime;

	IPhysicsConstraint	*m_pHinge;
	IPhysicsMotionController *m_pController;
	CMotorController m_motor;
};


BEGIN_DATADESC( CPhysMotor )

	DEFINE_KEYFIELD( CPhysMotor, m_nameAttach, FIELD_STRING, "attach1" ),
	DEFINE_FIELD( CPhysMotor, m_attachedObject, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( CPhysMotor, m_spinUp, FIELD_FLOAT, "spinup" ),
	DEFINE_KEYFIELD( CPhysMotor, m_additionalAcceleration, FIELD_FLOAT, "addangaccel" ),
	DEFINE_KEYFIELD( CPhysMotor, m_targetSpeed, FIELD_FLOAT, "speed" ),
	DEFINE_FIELD( CPhysMotor, m_angularAcceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CPhysMotor, m_lastTime, FIELD_TIME ),
	DEFINE_PHYSPTR( CPhysMotor, m_pHinge ),
	DEFINE_PHYSPTR( CPhysMotor, m_pController ),
	
	DEFINE_INPUTFUNC( CPhysMotor, FIELD_FLOAT, "SetSpeed", InputSetTargetSpeed ),
	DEFINE_INPUTFUNC( CPhysMotor, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CPhysMotor, FIELD_VOID, "TurnOff", InputTurnOff ),

	DEFINE_EMBEDDED( CPhysMotor, m_motor ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( phys_motor, CPhysMotor );


void CPhysMotor::CalculateAcceleration()
{
	if ( m_spinUp )
	{
		m_angularAcceleration = fabsf(m_targetSpeed / m_spinUp);
	}
	else
	{
		m_angularAcceleration = fabsf(m_targetSpeed);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that sets a speed to spin up or down to.
//-----------------------------------------------------------------------------
void CPhysMotor::InputSetTargetSpeed( inputdata_t &inputdata )
{
	if ( m_targetSpeed == inputdata.value.Float() )
		return;

	m_targetSpeed = inputdata.value.Float();
	TargetSpeedChanged();
	CalculateAcceleration();
}


void CPhysMotor::TargetSpeedChanged( void )
{
	SetNextThink( gpGlobals->curtime );
	m_lastTime = gpGlobals->curtime;
	m_pController->WakeObjects();
}


//------------------------------------------------------------------------------
// Purpose: Input handler that turns the motor on.
//------------------------------------------------------------------------------
void CPhysMotor::InputTurnOn( inputdata_t &inputdata )
{
	TurnOn();
}


//------------------------------------------------------------------------------
// Purpose: Input handler that turns the motor off.
//------------------------------------------------------------------------------
void CPhysMotor::InputTurnOff( inputdata_t &inputdata )
{
	m_motor.m_speed = 0;
	SetNextThink( TICK_NEVER_THINK );
}


CPhysMotor::~CPhysMotor()
{
	physenv->DestroyConstraint( m_pHinge );
	physenv->DestroyMotionController( m_pController );
}


void CPhysMotor::Spawn( void )
{
	m_motor.m_axis -= GetLocalOrigin();
	VectorNormalize(m_motor.m_axis);
	UTIL_SnapDirectionToAxis( m_motor.m_axis );
}


void CPhysMotor::TurnOn( void )
{
	CBaseEntity *pAttached = m_attachedObject;
	if ( !pAttached )
		return;

	IPhysicsObject *pPhys = pAttached->VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->Wake();
		SetNextThink( gpGlobals->curtime );
	}
}


void CPhysMotor::Activate( void )
{
	BaseClass::Activate();
	
	// This gets called after all objects spawn and after all objects restore
	if ( m_attachedObject == NULL )
	{
		CBaseEntity *pAttach = gEntList.FindEntityByName( NULL, m_nameAttach, NULL );
		if ( pAttach && pAttach->GetMoveType() == MOVETYPE_VPHYSICS )
		{
			m_attachedObject = pAttach;
			IPhysicsObject *pPhys = m_attachedObject->VPhysicsGetObject();
			CalculateAcceleration();
			matrix3x4_t matrix;
			pPhys->GetPositionMatrix( matrix );
			Vector motorAxis_ls;
			VectorIRotate( m_motor.m_axis, matrix, motorAxis_ls );
			float inertia = DotProductAbs( pPhys->GetInertia(), motorAxis_ls );
			m_motor.m_maxTorque = inertia * m_motor.m_inertiaFactor * (m_angularAcceleration + m_additionalAcceleration);
			m_motor.m_restistanceDamping = 1.0f;
		}
	}

	if ( m_attachedObject )
	{
		IPhysicsObject *pPhys = m_attachedObject->VPhysicsGetObject();

		// create a hinge constraint for this object?
		if ( m_spawnflags & SF_MOTOR_HINGE )
		{
			// UNDONE: Don't do this on restore?
			if ( !m_pHinge )
			{
				constraint_hingeparams_t hingeParams;
				hingeParams.Defaults();
				hingeParams.worldAxisDirection = m_motor.m_axis;
				hingeParams.worldPosition = GetLocalOrigin();

				m_pHinge = physenv->CreateHingeConstraint( g_PhysWorldObject, pPhys, NULL, hingeParams );
				m_pHinge->SetGameData( (void *)this );
			}

			if ( m_spawnflags & SF_MOTOR_NOCOLLIDE )
			{
				physenv->DisableCollisions( pPhys, g_PhysWorldObject );
			}
		}
		else
		{
			m_pHinge = NULL;
		}

		// NOTE: On restore, this path isn't run because m_pController will not be NULL
		if ( !m_pController )
		{
			m_pController = physenv->CreateMotionController( &m_motor );
			m_pController->AttachObject( m_attachedObject->VPhysicsGetObject() );

			if ( m_spawnflags & SF_MOTOR_START_ON )
			{
				TurnOn();
			}
		}
	}

	// Need to do this on restore since there's no good way to save this
	if ( m_pController )
	{
		m_pController->SetEventHandler( &m_motor );
	}
}


void CPhysMotor::Think( void )
{
	// angular acceleration is always positive - it should be treated as a magnitude - the controller 
	// will apply it in the proper direction
	Assert(m_angularAcceleration>=0);

	m_motor.m_speed = UTIL_Approach( m_targetSpeed, m_motor.m_speed, m_angularAcceleration*(gpGlobals->curtime-m_lastTime) );
	m_lastTime = gpGlobals->curtime;
	if ( m_motor.m_speed != m_targetSpeed )
	{
		SetNextThink( gpGlobals->curtime );
	}
}

//======================================================================================
// KEEPUPRIGHT CONTROLLER
//======================================================================================
#define SF_KEEPUPRIGHT_START_INACTIVE		0x0001

LINK_ENTITY_TO_CLASS( phys_keepupright, CKeepUpright );

BEGIN_DATADESC( CKeepUpright )

	DEFINE_FIELD( CKeepUpright, m_worldGoalAxis, FIELD_VECTOR ),
	DEFINE_FIELD( CKeepUpright, m_localTestAxis, FIELD_VECTOR ),
	DEFINE_PHYSPTR( CKeepUpright, m_pController ),
	DEFINE_KEYFIELD( CKeepUpright, m_nameAttach, FIELD_STRING, "attach1" ),
	DEFINE_FIELD( CKeepUpright, m_attachedObject, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( CKeepUpright, m_angularLimit, FIELD_FLOAT, "angularlimit" ),
	DEFINE_FIELD( CKeepUpright, m_bActive, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( CKeepUpright, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CKeepUpright, FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()

CKeepUpright::CKeepUpright()
{
	// by default, recover from up to 15 degrees / sec angular velocity
	m_angularLimit = 15;
	m_attachedObject = NULL;
}

CKeepUpright::~CKeepUpright()
{
	if ( m_pController )
	{
		physenv->DestroyMotionController( m_pController );
		m_pController = NULL;
	}
}

void CKeepUpright::Spawn()
{
	// align the object's local Z axis
	m_localTestAxis.Init( 0, 0, 1 );
	// Use our Up axis so mapmakers can orient us arbitrarily
	GetVectors( NULL, NULL, &m_worldGoalAxis );

	SetMoveType( MOVETYPE_NONE );

	if ( m_spawnflags & SF_KEEPUPRIGHT_START_INACTIVE )
	{
		m_bActive = false;
	}
	else
	{
		m_bActive = true;
	}
}

void CKeepUpright::Activate()
{
	BaseClass::Activate();
	
	if ( !m_pController )
	{
		// This case occurs when spawning
		IPhysicsObject *pPhys;
		if ( m_attachedObject )
		{
			pPhys = m_attachedObject->VPhysicsGetObject();
		}
		else
		{
			extern IPhysicsObject *FindPhysicsObject( const char *pName );
			pPhys = FindPhysicsObject( STRING(m_nameAttach) );
		}

		if ( !pPhys )
		{
			UTIL_Remove(this);
			return;
		}

		m_pController = physenv->CreateMotionController( (IMotionEvent *)this );
		m_pController->AttachObject( pPhys );
	}
	else
	{
		// This case occurs when restoring
		m_pController->SetEventHandler( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Use this to spawn a keepupright controller via code instead of map-placed
//-----------------------------------------------------------------------------
CKeepUpright *CKeepUpright::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, float flAngularLimit, bool bActive )
{
	CKeepUpright *pKeepUpright = (CKeepUpright*)CBaseEntity::Create( "phys_keepupright", vecOrigin, vecAngles, pOwner );
	if ( pKeepUpright )
	{
		pKeepUpright->m_attachedObject = pOwner;
		pKeepUpright->m_angularLimit = flAngularLimit;
		if ( !bActive )
		{
			pKeepUpright->AddSpawnFlags( SF_KEEPUPRIGHT_START_INACTIVE );
		}
		pKeepUpright->Spawn();
		pKeepUpright->Activate();
	}

	return pKeepUpright;
}

IMotionEvent::simresult_e CKeepUpright::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	if ( !m_bActive )
		return SIM_NOTHING;

	Vector currentAxis;
	matrix3x4_t matrix;

	// get the object's local to world transform
	pObject->GetPositionMatrix( matrix );

	// Get the object's local test axis in world space
	VectorRotate( m_localTestAxis, matrix, currentAxis );
	Vector rotationAxis = CrossProduct( currentAxis, m_worldGoalAxis );

	// atan2() is well defined, so do a Dot & Cross instead of asin(Cross)
	float cosine = DotProduct( currentAxis, m_worldGoalAxis );
	float sine = VectorNormalize( rotationAxis );
	float angle = atan2( sine, cosine );

	angle = RAD2DEG(angle);
	rotationAxis *= angle;
	VectorIRotate( rotationAxis, matrix, angular );
	float invDeltaTime = (1/deltaTime);
	angular *= invDeltaTime * invDeltaTime;

	AngularImpulse angVel;
	pObject->GetVelocity( NULL, &angVel );
	angular -= angVel;

	float len = VectorNormalize( angular );
	if ( len > m_angularLimit * invDeltaTime )
		len = m_angularLimit * invDeltaTime;
	angular *= len;

	linear.Init();

	return SIM_LOCAL_ACCELERATION;
}

