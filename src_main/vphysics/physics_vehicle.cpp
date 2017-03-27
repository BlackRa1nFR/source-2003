//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifdef _WIN32
#pragma warning (disable:4127)
#pragma warning (disable:4244)
#endif

#include "ivp_physics.hxx"
#include "ivp_core.hxx"
#include "ivp_controller.hxx"
#include "ivp_cache_object.hxx"
#include "ivp_car_system.hxx"
#include "ivp_constraint_car.hxx"
#include "ivp_material.hxx"

#include "convert.h"
#include "vphysics_interface.h"
#include "physics_environment.h"
#include "vphysics/vehicles.h"
#include "physics_vehicle.h"
#include "physics_controller_raycast_vehicle.h"
#include "physics_airboat.h"
#include "physics_object.h"
#include "tier0/dbg.h"
#include "physics_material.h"
#include "isaverestore.h"
#include "ivp_car_system.hxx"
#include "ivp_listener_object.hxx"
#include "vphysics_saverestore.h"

#define THROTTLE_OPPOSING_FORCE_EPSILON		5.0f

// y in/s = x miles/hour * (5280 * 12 (in / mile)) * (1 / 3600 (hour / sec) )
#define MPH2INS(x)		( (x) * 5280.0f * 12.0f / 3600.0f )

#define FVEHICLE_THROTTLE_STOPPED		0x00000001
#define FVEHICLE_HANDBRAKE_ON			0x00000002

struct vphysics_save_cvehiclecontroller_t
{
	CPhysicsObject				*m_pCarBody;
	int							m_wheelCount;
	vehicleparams_t				m_vehicleData;
	vehicle_operatingparams_t	m_currentState;
	float						m_wheelRadius;
	float						m_bodyMass;
	float						m_totalWheelMass;
	float						m_gravityLength;
	CPhysicsObject				*m_pWheels[VEHICLE_MAX_WHEEL_COUNT];
	Vector						m_wheelPosition_Bs[VEHICLE_MAX_WHEEL_COUNT];
	Vector						m_tracePosition_Bs[VEHICLE_MAX_WHEEL_COUNT];
	int							m_vehicleFlags;
	bool						m_bSlipperyTires;
	unsigned int				m_nVehicleType;
	bool						m_bTraceData;
	bool						m_bOccupied;
	bool						m_bEngineDisable;

	DECLARE_SIMPLE_DATADESC();
};


BEGIN_SIMPLE_DATADESC( vphysics_save_cvehiclecontroller_t )
	DEFINE_VPHYSPTR( vphysics_save_cvehiclecontroller_t,m_pCarBody ),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_wheelCount,		FIELD_INTEGER ),
	DEFINE_EMBEDDED( vphysics_save_cvehiclecontroller_t,m_vehicleData ),
	DEFINE_EMBEDDED( vphysics_save_cvehiclecontroller_t,m_currentState ),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_wheelCount,		FIELD_INTEGER ),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_bodyMass,			FIELD_FLOAT	),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_totalWheelMass,	FIELD_FLOAT	),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_gravityLength,	FIELD_FLOAT	),
	DEFINE_VPHYSPTR_ARRAY( vphysics_save_cvehiclecontroller_t,	m_pWheels, VEHICLE_MAX_WHEEL_COUNT ),
	DEFINE_ARRAY( vphysics_save_cvehiclecontroller_t,	m_wheelPosition_Bs,	FIELD_VECTOR, VEHICLE_MAX_WHEEL_COUNT ),
	DEFINE_ARRAY( vphysics_save_cvehiclecontroller_t,	m_tracePosition_Bs,	FIELD_VECTOR, VEHICLE_MAX_WHEEL_COUNT ),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_vehicleFlags,		FIELD_INTEGER ),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_bSlipperyTires,	FIELD_BOOLEAN ),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_nVehicleType,		FIELD_INTEGER ),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_bTraceData,		FIELD_BOOLEAN ),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_bOccupied,		FIELD_BOOLEAN ),
	DEFINE_FIELD( vphysics_save_cvehiclecontroller_t,	m_bEngineDisable,	FIELD_BOOLEAN ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( vehicle_operatingparams_t )
	DEFINE_FIELD( vehicle_operatingparams_t,	speed,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_operatingparams_t,	engineRPM,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_operatingparams_t,	gear,			FIELD_INTEGER ),
	DEFINE_FIELD( vehicle_operatingparams_t,	boostDelay,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_operatingparams_t,	boostTimeLeft,	FIELD_INTEGER ),
	DEFINE_FIELD( vehicle_operatingparams_t,	skidding,		FIELD_BOOLEAN ),
	DEFINE_ARRAY( vehicle_operatingparams_t,	wheelSkidValue,	FIELD_FLOAT, VEHICLE_MAX_WHEEL_COUNT ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( vehicle_bodyparams_t )
	DEFINE_FIELD( vehicle_bodyparams_t,	massCenterOverride,		FIELD_VECTOR ),
	DEFINE_FIELD( vehicle_bodyparams_t,	massOverride,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_bodyparams_t,	addGravity,				FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_bodyparams_t,	tiltForce,				FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_bodyparams_t,	tiltForceHeight,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_bodyparams_t,	counterTorqueFactor,	FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_bodyparams_t,	keepUprightTorque,		FIELD_FLOAT ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( vehicle_wheelparams_t )
	DEFINE_FIELD( vehicle_wheelparams_t,	radius,				FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_wheelparams_t,	mass,				FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_wheelparams_t,	inertia,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_wheelparams_t,	damping,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_wheelparams_t,	rotdamping,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_wheelparams_t,	frictionScale,		FIELD_FLOAT ),
	DEFINE_CUSTOM_FIELD( vehicle_wheelparams_t,	materialIndex,		MaterialIndexDataOps() ),
	DEFINE_CUSTOM_FIELD( vehicle_wheelparams_t,	skidMaterialIndex,	MaterialIndexDataOps() ),
	DEFINE_FIELD( vehicle_wheelparams_t,	springAdditionalLength,	FIELD_FLOAT ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( vehicle_suspensionparams_t )
	DEFINE_FIELD( vehicle_suspensionparams_t,	springConstant,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_suspensionparams_t,	springDamping,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_suspensionparams_t,	stabilizerConstant,	FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_suspensionparams_t,	springDampingCompression,	FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_suspensionparams_t,	maxBodyForce,		FIELD_FLOAT ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( vehicle_axleparams_t )
	DEFINE_FIELD( vehicle_axleparams_t,	offset,					FIELD_VECTOR ),
	DEFINE_FIELD( vehicle_axleparams_t,	wheelOffset,			FIELD_VECTOR ),
	DEFINE_FIELD( vehicle_axleparams_t,	raytraceCenterOffset,	FIELD_VECTOR ),
	DEFINE_FIELD( vehicle_axleparams_t,	raytraceOffset,			FIELD_VECTOR ),
	DEFINE_EMBEDDED( vehicle_axleparams_t,wheels ),
	DEFINE_EMBEDDED( vehicle_axleparams_t,suspension ),
	DEFINE_FIELD( vehicle_axleparams_t,	torqueFactor,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_axleparams_t,	brakeFactor,			FIELD_FLOAT ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( vehicle_steeringparams_t )
	DEFINE_FIELD( vehicle_steeringparams_t,	degrees,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_steeringparams_t,	steeringRateSlow,	FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_steeringparams_t,	steeringRateFast,	FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_steeringparams_t,	steeringRestFactor,	FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_steeringparams_t,	speedSlow,			FIELD_FLOAT ),			
	DEFINE_FIELD( vehicle_steeringparams_t,	speedFast,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_steeringparams_t,	isSkidAllowed,		FIELD_BOOLEAN ),
	DEFINE_FIELD( vehicle_steeringparams_t,	dustCloud,			FIELD_BOOLEAN ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( vehicle_engineparams_t )
	DEFINE_FIELD( vehicle_engineparams_t,	horsepower,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	maxSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	maxRevSpeed,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	maxRPM,				FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	axleRatio,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	throttleTime,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	maxRPM,				FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	isAutoTransmission,	FIELD_BOOLEAN ),
	DEFINE_FIELD( vehicle_engineparams_t,	gearCount,			FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( vehicle_engineparams_t,	gearRatio,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	shiftUpRPM,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	shiftDownRPM,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	boostForce,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	boostDuration,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	boostDelay,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	boostMaxSpeed,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_engineparams_t,	torqueBoost,		FIELD_BOOLEAN ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( vehicleparams_t )
	DEFINE_FIELD( vehicleparams_t,	axleCount,					FIELD_INTEGER ),
	DEFINE_FIELD( vehicleparams_t,	wheelsPerAxle,				FIELD_INTEGER ),
	DEFINE_EMBEDDED( vehicleparams_t,body ),
	DEFINE_EMBEDDED_AUTO_ARRAY( vehicleparams_t,axles ),
	DEFINE_EMBEDDED( vehicleparams_t,engine ),
	DEFINE_EMBEDDED( vehicleparams_t,steering ),
END_DATADESC()


bool IsVehicleWheel( IVP_Real_Object *pivp )
{
	CPhysicsObject *pObject = static_cast<CPhysicsObject *>(pivp->client_data);

	// FIXME: Check why this is null! It occurs when jumping the ravine in seafloor
	if (!pObject)
		return false;

	return (pObject->GetCallbackFlags() & CALLBACK_IS_VEHICLE_WHEEL) ? true : false;
}


class CVehicleController : public IPhysicsVehicleController, public IVP_Listener_Object
{
public:
	CVehicleController() {}
	CVehicleController( const vehicleparams_t &params, CPhysicsEnvironment *pEnv, unsigned int nVehicleType, IPhysicsGameTrace *pGameTrace );
	~CVehicleController();

	// CVehicleController
	void InitCarSystem( CPhysicsObject *pBodyObject );

	// IPhysicsVehicleController
	void Update( float dt, vehicle_controlparams_t &controls );
	float UpdateBooster( float dt );
	void SetSpringLength(int wheelIndex, float length);
	const vehicle_operatingparams_t &GetOperatingParams()	{ return m_currentState; }
	const vehicleparams_t &GetVehicleParams()				{ return m_vehicleData; }
	vehicleparams_t &GetVehicleParamsForChange()			{ return m_vehicleData; }
	int GetWheelCount(void)									{ return m_wheelCount; };
	IPhysicsObject* GetWheel(int index);
	virtual void GetWheelContactPoint( int index, Vector &contact );
	virtual float GetWheelSkidDeltaTime( int index );
	void SetWheelFriction(int wheelIndex, float friction);

	void SetEngineDisabled( bool bDisable )					{ m_bEngineDisable = bDisable; }
	bool IsEngineDisabled( void )							{ return m_bEngineDisable; }

	// Save/load
	void WriteToTemplate( vphysics_save_cvehiclecontroller_t &controllerTemplate );
	void InitFromTemplate( CPhysicsEnvironment *pEnv, void *pGameData, const vphysics_save_cvehiclecontroller_t &controllerTemplate );

	// Debug
	void GetCarSystemDebugData( vehicle_debugcarsystem_t &debugCarSystem );

	// IVP_Listener_Object
	// Object listener, only hook delete
    virtual void event_object_deleted( IVP_Event_Object *);
    virtual void event_object_created( IVP_Event_Object *) {}
    virtual void event_object_revived( IVP_Event_Object *) {}
    virtual void event_object_frozen ( IVP_Event_Object *) {}

	// Entry/Exit 
	void OnVehicleEnter( void )								{ m_bOccupied = true; }
	void OnVehicleExit( void )								{ m_bOccupied = false; }

protected:
	float CreateIVPObjects( );
	void ShutdownCarSystem();

	void InitVehicleData( const vehicleparams_t &params );
	void InitCarSystemBody( IVP_Template_Car_System &ivpVehicleData );
	void InitCarSystemWheels( IVP_Template_Car_System &ivpVehicleData, float &totalTorqueDistribution );

	void AttachListener();
	void DetachListener();

	IVP_Real_Object *CreateWheel( int wheelIndex, vehicle_axleparams_t &axle );
	void CreateTraceData( int wheelIndex, vehicle_axleparams_t &axle );

	// Update.
	void UpdateSteering( vehicle_controlparams_t &controls, float flDeltaTime );
	void UpdatePowerslide( bool bPowerslide );
	void UpdateEngine( vehicle_controlparams_t &controls, float flDeltaTime, float flThrottle, float flBrake, bool bHandbrake, bool bPowerslide );
	void UpdateExtraForces( void );
	void UpdateWheelPositions( void );
	float CalcSteering( float dt, float steering );
	void CalcEngine( float throttle, float brake_val, bool handbrake, float steeringVal, bool torqueBoost );

private:
	IVP_Car_System				*m_pCarSystem;
	CPhysicsObject				*m_pCarBody;
	CPhysicsEnvironment			*m_pEnv;
	IPhysicsGameTrace			*m_pGameTrace;
	int							m_wheelCount;
	vehicleparams_t				m_vehicleData;
	vehicle_operatingparams_t	m_currentState;
	float						m_wheelRadius;
	float						m_bodyMass;
	float						m_totalWheelMass;
	float						m_gravityLength;
	CPhysicsObject				*m_pWheels[VEHICLE_MAX_WHEEL_COUNT];
	IVP_U_Float_Point			m_wheelPosition_Bs[VEHICLE_MAX_WHEEL_COUNT];
	IVP_U_Float_Point			m_tracePosition_Bs[VEHICLE_MAX_WHEEL_COUNT];
	int							m_vehicleFlags;
	bool						m_bSlipperyTires;
	unsigned int				m_nVehicleType;
	bool						m_bTraceData;
	bool						m_bOccupied;
	bool						m_bEngineDisable;
};

CVehicleController::CVehicleController( const vehicleparams_t &params, CPhysicsEnvironment *pEnv, unsigned int nVehicleType, IPhysicsGameTrace *pGameTrace )
{
	m_pEnv = pEnv;
	m_pGameTrace = pGameTrace;
	m_pCarSystem = NULL;
	m_nVehicleType = nVehicleType;
	InitVehicleData( params );
	for ( int i = 0; i < VEHICLE_MAX_WHEEL_COUNT; i++ )
	{
		m_pWheels[i] = NULL;
	}
	m_pCarBody = NULL;
	m_wheelCount = 0;
	m_wheelRadius = 0;
	memset( &m_currentState, 0, sizeof(m_currentState) );
	m_bodyMass = 0;
	m_vehicleFlags = 0;
	memset( m_wheelPosition_Bs, 0, sizeof(m_wheelPosition_Bs) );
	memset( m_tracePosition_Bs, 0, sizeof(m_tracePosition_Bs) );

	m_bTraceData = false;
	if ( m_nVehicleType == VEHICLE_TYPE_AIRBOAT_RAYCAST )
	{
		m_bTraceData = true;
	}

	m_bOccupied = false;
	m_bEngineDisable = false;
}

CVehicleController::~CVehicleController()
{
	ShutdownCarSystem();
}

IPhysicsObject* CVehicleController::GetWheel( int index ) 
{ 
	// TODO: This is getting messy.
	if ( m_nVehicleType == VEHICLE_TYPE_CAR_WHEELS )
	{
		return m_pWheels[index]; 
	}
	else if ( m_nVehicleType == VEHICLE_TYPE_CAR_RAYCAST && m_pCarSystem )
	{
		return static_cast<CPhysics_Car_System_Raycast_Wheels*>( m_pCarSystem )->GetWheel( index );
	}
	else if ( m_nVehicleType == VEHICLE_TYPE_AIRBOAT_RAYCAST && m_pCarSystem )
	{
		return static_cast<CPhysics_Airboat*>( m_pCarSystem )->GetWheel( index );
	}

	return NULL;
}

void CVehicleController::SetWheelFriction(int wheelIndex, float friction)
{
	CPhysics_Airboat *pAirboat = static_cast<CPhysics_Airboat*>( m_pCarSystem );
	if ( !pAirboat )
		return;

	pAirboat->SetWheelFriction( wheelIndex, friction );
}

void CVehicleController::GetWheelContactPoint( int index, Vector &contact )
{
	IVP_Constraint_Car_Object *pWheel = m_pWheels[index]->GetObject()->get_core()->car_wheel;
	if ( pWheel )
	{
		ConvertPositionToHL( pWheel->last_contact_position_ws, contact );
	}
	else
	{
		contact.Init();
	}
}

float CVehicleController::GetWheelSkidDeltaTime( int index )
{
	IVP_Real_Object *pObject = m_pWheels[index]->GetObject();
	IVP_Constraint_Car_Object *pWheel = pObject->get_core()->car_wheel;
	return pObject->get_environment()->get_current_time() - pWheel->last_skid_time;
}

void CVehicleController::AttachListener()
{
	m_pCarBody->GetObject()->add_listener_object( this );
}

void CVehicleController::event_object_deleted( IVP_Event_Object * )
{
	// the car system's constraint solver is going to delete itself now, so NULL the car system.

	// BUGBUG: This leaks - fix the car system destructor and add a message to the car system for this!
	m_pCarSystem = NULL;
}

IVP_Real_Object *CVehicleController::CreateWheel( int wheelIndex, vehicle_axleparams_t &axle )
{
	if ( wheelIndex >= VEHICLE_MAX_WHEEL_COUNT )
		return NULL;

	// HACKHACK: In Save/load, the wheel was reloaded, so pretend to create it
	if ( GetWheel( wheelIndex ) )
	{
		CPhysicsObject *pWheelObject = static_cast<CPhysicsObject *>(GetWheel(wheelIndex));
		return pWheelObject->GetObject();
	}

	objectparams_t params;
	memset( &params, 0, sizeof(params) );

	Vector bodyPosition;
	QAngle bodyAngles;
	m_pCarBody->GetPosition( &bodyPosition, &bodyAngles );
	matrix3x4_t matrix;
	AngleMatrix( bodyAngles, bodyPosition, matrix );

	Vector position = axle.offset;

	// BUGBUG: This only works with 2 wheels per axle
	if ( wheelIndex & 1 )
	{
		position += axle.wheelOffset;
	}
	else
	{
		position -= axle.wheelOffset;
	}

	QAngle angles = vec3_angle;
	Vector wheelPositionHL;
	VectorTransform( position, matrix, wheelPositionHL );

	params.damping = axle.wheels.damping;
	params.dragCoefficient = 0;
	params.enableCollisions = false;
	params.inertia = axle.wheels.inertia;
	params.mass = axle.wheels.mass;
	params.pGameData = m_pCarBody->GetGameData();
	params.pName = "VehicleWheel";
	params.rotdamping = axle.wheels.rotdamping;
	params.rotInertiaLimit = 0;
	params.massCenterOverride = NULL;
	// needs to be in HL units because we're calling through the "outer" interface to create
	// the wheels
	float radius = ConvertDistanceToHL( axle.wheels.radius );
	float r3 = radius * radius * radius;
	params.volume = (4 / 3) * M_PI * r3;

	CPhysicsObject *pWheel = (CPhysicsObject *)m_pEnv->CreateSphereObject( radius, axle.wheels.materialIndex, wheelPositionHL, angles, &params, false );
    pWheel->Wake();

	// UNDONE: only mask off some of these flags?
	unsigned int flags = pWheel->GetCallbackFlags();
	flags = 0;
	pWheel->SetCallbackFlags( flags );

	// cache the wheel object pointer
	m_pWheels[wheelIndex] = pWheel;

	IVP_U_Point wheelPositionIVP, wheelPositionBs;
	ConvertPositionToIVP( wheelPositionHL, wheelPositionIVP );
	TransformIVPToLocal( wheelPositionIVP, wheelPositionBs, m_pCarBody->GetObject(), true );
	m_wheelPosition_Bs[wheelIndex].set_to_zero();
	m_wheelPosition_Bs[wheelIndex].set( &wheelPositionBs );

	pWheel->SetCallbackFlags( pWheel->GetCallbackFlags() | CALLBACK_IS_VEHICLE_WHEEL );

	return pWheel->GetObject();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVehicleController::CreateTraceData( int wheelIndex, vehicle_axleparams_t &axle )
{
	if ( wheelIndex >= VEHICLE_MAX_WHEEL_COUNT )
		return;

	objectparams_t params;
	memset( &params, 0, sizeof( params ) );

	Vector bodyPosition;
	QAngle bodyAngles;
	matrix3x4_t matrix;
	m_pCarBody->GetPosition( &bodyPosition, &bodyAngles );
	AngleMatrix( bodyAngles, bodyPosition, matrix );

	Vector tracePosition = axle.raytraceCenterOffset;
	// BUGBUG: This only works with 2 wheels per axle
	if ( wheelIndex & 1 )
	{
		tracePosition += axle.raytraceOffset;
	}
	else
	{
		tracePosition -= axle.raytraceOffset;
	}

	Vector tracePositionHL;
	VectorTransform( tracePosition, matrix, tracePositionHL );

	IVP_U_Point tracePositionIVP, tracePositionBs;
	ConvertPositionToIVP( tracePositionHL, tracePositionIVP );
	TransformIVPToLocal( tracePositionIVP, tracePositionBs, m_pCarBody->GetObject(), true );
	m_tracePosition_Bs[wheelIndex].set_to_zero();
	m_tracePosition_Bs[wheelIndex].set( &tracePositionBs );
}

float CVehicleController::CreateIVPObjects( )
{
	// Initialize the car system (body and wheels).
	IVP_Template_Car_System ivpVehicleData( m_wheelCount, m_vehicleData.axleCount );
	InitCarSystemBody( ivpVehicleData );
	float totalTorqueDistribution;
	InitCarSystemWheels( ivpVehicleData, totalTorqueDistribution );

	// Raycast Car
	switch ( m_nVehicleType )
	{
	case VEHICLE_TYPE_CAR_WHEELS: { m_pCarSystem = new IVP_Car_System_Real_Wheels( m_pEnv->GetIVPEnvironment(), &ivpVehicleData ); break; }
	case VEHICLE_TYPE_CAR_RAYCAST: { m_pCarSystem = new CPhysics_Car_System_Raycast_Wheels( m_pEnv->GetIVPEnvironment(), &ivpVehicleData ); break; }
	case VEHICLE_TYPE_AIRBOAT_RAYCAST: { m_pCarSystem = new CPhysics_Airboat( m_pEnv->GetIVPEnvironment(), &ivpVehicleData, m_pGameTrace ); break; }
	}

	AttachListener();

	return totalTorqueDistribution;
}


void CVehicleController::InitCarSystem( CPhysicsObject *pBodyObject )
{
	if ( m_pCarSystem )
	{
		ShutdownCarSystem();
	}

	// Car body.
	m_pCarBody = pBodyObject;
	m_bodyMass = m_pCarBody->GetMass();
	m_gravityLength = m_pEnv->GetIVPEnvironment()->get_gravity()->real_length();
	m_vehicleData.body.tiltForce = m_gravityLength * m_vehicleData.body.tiltForce * m_bodyMass;
	// float keepUprightTorque;						// This is unused for 4-wheel vehicles.
	// IVP_FLOAT fast_turn_factor					// unmodified

	// Setup axle/wheel counts.
	m_wheelCount = m_vehicleData.axleCount * m_vehicleData.wheelsPerAxle;

	float totalTorqueDistribution = CreateIVPObjects();

	IVP_U_Float_Point tmp;
	ConvertPositionToIVP( m_vehicleData.body.massCenterOverride, tmp );
    //float mass_center_shift_z = tmp.k[2];

    // Note: Next formula looks reverse, but they are correct !!

	// normalize the torque distribution
	float torqueScale = 1;
	if ( totalTorqueDistribution > 0 )
	{
		torqueScale /= totalTorqueDistribution;
	}

	for ( int i = 0; i < m_vehicleData.axleCount; i++ )
	{
		m_vehicleData.axles[i].torqueFactor *= torqueScale;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup the body parameters.
//-----------------------------------------------------------------------------
void CVehicleController::InitCarSystemBody( IVP_Template_Car_System &ivpVehicleData )
{
	ivpVehicleData.car_body = m_pCarBody->GetObject();

	ivpVehicleData.index_x = IVP_INDEX_X;
	ivpVehicleData.index_y = IVP_INDEX_Y;
	ivpVehicleData.index_z = IVP_INDEX_Z;

	ivpVehicleData.body_counter_torque_factor = m_vehicleData.body.counterTorqueFactor;
	ivpVehicleData.body_down_force_vertical_offset = m_vehicleData.body.tiltForceHeight;
	ivpVehicleData.extra_gravity_force_value = m_vehicleData.body.addGravity * m_gravityLength * m_bodyMass;
	ivpVehicleData.extra_gravity_height_offset = 0;

#if 0
	// HACKHACK: match example
	ivpVehicleData.extra_gravity_force_value = 1.2;
	ivpVehicleData.body_down_force_vertical_offset = 2;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Setup the wheel paramters.
//-----------------------------------------------------------------------------
void CVehicleController::InitCarSystemWheels( IVP_Template_Car_System &ivpVehicleData, float &totalTorqueDistribution )
{
	int	  wheelIndex = 0;

	m_wheelRadius = 0;
	m_totalWheelMass = 0;

	// Clear to accumulation.
	totalTorqueDistribution = 0.0f;

	int i;
	for ( i = 0; i < m_vehicleData.axleCount; i++ )
	{
		for ( int w = 0; w < m_vehicleData.wheelsPerAxle; w++, wheelIndex++ )
		{
			IVP_Real_Object *pWheel = CreateWheel( wheelIndex, m_vehicleData.axles[i] );
			if ( pWheel )
			{
				// Create ray trace data for wheel.
				if ( m_bTraceData )
				{
					CreateTraceData( wheelIndex, m_vehicleData.axles[i] );
				}

				ivpVehicleData.car_wheel[wheelIndex] = pWheel;
				ivpVehicleData.wheel_radius[wheelIndex] = pWheel->get_core()->upper_limit_radius;
				ivpVehicleData.wheel_reversed_sign[wheelIndex] = 1.0;
				// only for raycast car
				ivpVehicleData.friction_of_wheel[wheelIndex] = m_vehicleData.axles[i].wheels.frictionScale;
				ivpVehicleData.spring_constant[wheelIndex] = m_vehicleData.axles[i].suspension.springConstant * m_bodyMass;
				ivpVehicleData.spring_dampening[wheelIndex] = m_vehicleData.axles[i].suspension.springDamping * m_bodyMass;
				ivpVehicleData.spring_dampening_compression[wheelIndex] = m_vehicleData.axles[i].suspension.springDampingCompression * m_bodyMass;
				ivpVehicleData.max_body_force[wheelIndex] = m_vehicleData.axles[i].suspension.maxBodyForce * m_bodyMass;
				ivpVehicleData.spring_pre_tension[wheelIndex] = -m_vehicleData.axles[i].wheels.springAdditionalLength;
				
				ivpVehicleData.wheel_pos_Bos[wheelIndex] = m_wheelPosition_Bs[wheelIndex];
				if ( m_bTraceData )
				{
					ivpVehicleData.trace_pos_Bos[wheelIndex] = m_tracePosition_Bs[wheelIndex];
				}

				m_totalWheelMass += m_vehicleData.axles[i].wheels.mass;
			}
		}

		ivpVehicleData.stabilizer_constant[i] = m_vehicleData.axles[i].suspension.stabilizerConstant * m_bodyMass;
		// this should output in radians per second
		ivpVehicleData.wheel_max_rotation_speed[i] = m_vehicleData.engine.maxSpeed / m_vehicleData.axles[i].wheels.radius;
		totalTorqueDistribution += m_vehicleData.axles[i].torqueFactor;
		if ( m_vehicleData.axles[i].wheels.radius > m_wheelRadius )
		{
			m_wheelRadius = m_vehicleData.axles[i].wheels.radius;
		}
	}

	// Set up wheel collisions
	for ( i = 0; i < m_wheelCount; i++ )
	{
		m_pEnv->DisableCollisions( m_pCarBody, m_pWheels[i] );
		for ( int w = 0; w < i; w++ )
		{
			m_pEnv->DisableCollisions( m_pWheels[i], m_pWheels[w] );
		}
	}
	for ( i = 0; i < m_wheelCount; i++ )
	{
		m_pWheels[i]->EnableCollisions( true );
	}
}

void CVehicleController::ShutdownCarSystem()
{
	delete m_pCarSystem;
	m_pCarSystem = NULL;
	for ( int i = 0; i < m_wheelCount; i++ )
	{
		m_pEnv->DestroyObject( m_pWheels[i] );
		m_pWheels[i] = NULL;
	}
}


void CVehicleController::InitVehicleData( const vehicleparams_t &params )
{
	m_vehicleData = params;
	// input speed is in miles/hour.  Convert to in/s
	m_vehicleData.engine.maxSpeed = MPH2INS(m_vehicleData.engine.maxSpeed);
	m_vehicleData.engine.maxRevSpeed = MPH2INS(m_vehicleData.engine.maxRevSpeed);
	m_vehicleData.engine.boostMaxSpeed = MPH2INS(m_vehicleData.engine.boostMaxSpeed);
	m_vehicleData.body.tiltForceHeight = ConvertDistanceToIVP( m_vehicleData.body.tiltForceHeight );
	for ( int i = 0; i < m_vehicleData.axleCount; i++ )
	{
		m_vehicleData.axles[i].wheels.radius = ConvertDistanceToIVP( m_vehicleData.axles[i].wheels.radius );
		m_vehicleData.axles[i].wheels.springAdditionalLength = ConvertDistanceToIVP( m_vehicleData.axles[i].wheels.springAdditionalLength );
	}
}

void CVehicleController::SetSpringLength(int wheelIndex, float length)
{
	m_pCarSystem->change_spring_length((IVP_POS_WHEEL)wheelIndex, length);
}

//-----------------------------------------------------------------------------
// Purpose: Allows booster timer to run, 
// Returns: true if time still exists
//			false if timer has run out (i.e. can use boost again)
//-----------------------------------------------------------------------------
float CVehicleController::UpdateBooster( float dt )
{
	m_pCarSystem->update_booster( dt );
	m_currentState.boostDelay = m_pCarSystem->get_booster_delay();
	return m_currentState.boostDelay;
}

//-----------------------------------------------------------------------------
// Purpose: Update the vehicle controller.
//-----------------------------------------------------------------------------
void CVehicleController::Update( float dt, vehicle_controlparams_t &controls )
{
	// Calculate the throttle and brake values.
	float flThrottle = controls.throttle;
	bool bHandbrake = controls.handbrake;
	float flBrake = controls.brake;
	bool bPowerslide = bHandbrake;

	if ( bPowerslide )
	{
		flThrottle = 0.0f;
	}

	if ( flThrottle == 0.0f && flBrake == 0.0f && !bHandbrake )
	{
		flBrake = 0.1f;
	}

	// Update steering.
	UpdateSteering( controls, dt );

	// Update powerslide.
	UpdatePowerslide( bPowerslide );

	// Update engine.
	UpdateEngine( controls, dt, flThrottle, flBrake, bHandbrake, bPowerslide );

	// Apply the extra forces to the car (downward, counter-torque, etc.)
	UpdateExtraForces();

	// Update the physical position of the wheels for raycast vehicles.
	UpdateWheelPositions();
}

//-----------------------------------------------------------------------------
// Purpose: Update the steering on the vehicle.
//-----------------------------------------------------------------------------
void CVehicleController::UpdateSteering( vehicle_controlparams_t &controls, float flDeltaTime )
{
   // Steering - IVP steering is in radians.
    float flSteeringAngle = CalcSteering( flDeltaTime, controls.steering );
    m_pCarSystem->do_steering( DEG2RAD( flSteeringAngle ) );
}

//-----------------------------------------------------------------------------
// Purpose: Update the powerslide state (wheel materials).
//-----------------------------------------------------------------------------
void CVehicleController::UpdatePowerslide( bool bPowerslide )
{
	// Only allow skidding if it is allowed by the vehicle type.
	if ( !m_vehicleData.steering.isSkidAllowed )
		return;

	// Check to see if the vehicle is occupied.
	if ( !m_bOccupied )
		return;

	// Check to see if the wheels are already appropriate for the powerslide state.
	if ( bPowerslide == m_bSlipperyTires )
		return;

	int iWheel = 0;
	for ( int iAxle = 0; iAxle < m_vehicleData.axleCount; ++iAxle )
	{
		for ( int iAxleWheel = 0; iAxleWheel < m_vehicleData.wheelsPerAxle; ++iAxleWheel, ++iWheel )
		{
			// Change to slippery tires - if we can.
			if ( bPowerslide && ( m_vehicleData.axles[iAxle].wheels.skidMaterialIndex != - 1 ) )
			{
 				m_pWheels[iWheel]->SetMaterialIndex( m_vehicleData.axles[iAxle].wheels.skidMaterialIndex );
			}
			// Change back to normal tires.
			else
			{
				m_pWheels[iWheel]->SetMaterialIndex( m_vehicleData.axles[iAxle].wheels.materialIndex );
			}
		}
	}

	m_bSlipperyTires = bPowerslide;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVehicleController::UpdateEngine( vehicle_controlparams_t &controls, float flDeltaTime, 
									   float flThrottle, float flBrake, bool bHandbrake, bool bPowerslide )
{
	// Get the current vehicle speed.
    m_currentState.speed = ConvertDistanceToHL( m_pCarSystem->get_body_speed() );
	if ( !bPowerslide )
	{
		// HACK! Allowing you to overcome gravity at low throttle.
		if ( ( flThrottle < 0.0f && m_currentState.speed > THROTTLE_OPPOSING_FORCE_EPSILON ) ||
			 ( flThrottle > 0.0f && m_currentState.speed < -THROTTLE_OPPOSING_FORCE_EPSILON ) )
		{	
			bHandbrake = true;
		}
	}

	// only activate booster if vehicle uses force type boost, otherwise, apply torqueboost in CalcEngine()
	bool torqueBoost = false;
	if ( controls.boost > 0 )
	{
		if ( m_vehicleData.engine.torqueBoost )
		{
			torqueBoost = true;
		}
		else
		{
			m_pCarSystem->activate_booster( m_vehicleData.engine.boostForce * controls.boost, m_vehicleData.engine.boostDuration, m_vehicleData.engine.boostDelay );
		}
	}

	m_pCarSystem->update_booster( flDeltaTime );
	m_currentState.boostDelay = m_pCarSystem->get_booster_delay();

	CalcEngine( flThrottle, flBrake, bHandbrake, controls.steering, torqueBoost );
	m_pCarSystem->update_throttle( flThrottle );

	if ( m_vehicleData.engine.boostDuration + m_vehicleData.engine.boostDelay > 0 )	// watch out for div by zero
	{
		if ( m_currentState.boostDelay > 0 )
		{
			m_currentState.boostTimeLeft = 100 - 100 * ( m_currentState.boostDelay / ( m_vehicleData.engine.boostDuration +  m_vehicleData.engine.boostDelay ) );
		}
		else
		{
			m_currentState.boostTimeLeft = 100;		// ready to go any time
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Apply extra forces to the vehicle.  The downward force, counter-
//          torque etc.
//-----------------------------------------------------------------------------
void CVehicleController::UpdateExtraForces( void )
{
	// Extra downward force.
	IVP_Cache_Object *co = m_pCarBody->GetObject()->get_cache_object();
	float y_val = co->m_world_f_object.get_elem( IVP_INDEX_Y, IVP_INDEX_Y );
	if ( fabs( y_val ) < 0.05 )
	{
		m_pCarSystem->change_body_downforce( m_vehicleData.body.tiltForce );
	}
	else
	{
		m_pCarSystem->change_body_downforce( 0.0 );
	}
	co->remove_reference();

	// Counter-torque.
	if ( m_nVehicleType == VEHICLE_TYPE_CAR_WHEELS )
	{
	    m_pCarSystem->update_body_countertorque();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the physical position of the wheels for raycast vehicles.
//	  NOTE: Raycast boat doesn't have wheels.
//-----------------------------------------------------------------------------
void CVehicleController::UpdateWheelPositions( void )
{
	if ( m_nVehicleType == VEHICLE_TYPE_CAR_RAYCAST )
	{
		m_pCarSystem->update_wheel_positions();
	}
}

float CVehicleController::CalcSteering( float dt, float steering )
{
    steering *= m_vehicleData.steering.degrees;
	return steering;
}

void CVehicleController::CalcEngine( float throttle, float brake_val, bool handbrake, float steeringVal, bool torqueBoost )
{
    // throttle goes forward and backward, [-1, 1]
    // brake_val [0..1]

    // estimate rot speed based on current speed and the front wheels radius
    float absSpeed = fabs( m_currentState.speed );

	if ( m_vehicleData.engine.isAutoTransmission )
	{
		// estimate the engine RPM
		float avgRotSpeed = 0.0;

		bool skid = false;
		for( int i = 0; i < m_wheelCount; i++ )
		{
			float rot_speed = fabs(m_pCarSystem->get_wheel_angular_velocity(IVP_POS_WHEEL(i)));
			avgRotSpeed += rot_speed;
			if ( fabs(rot_speed) > absSpeed + 0.1 )		// allow for some slop
			{
				skid = true;
			}
		}
		// check for locked wheels also
		if ( handbrake && fabs(m_currentState.speed) > 30 )	// wheels locked and car still moving?
		{
			skid = true;					// then we're skidding
		}
		m_currentState.skidding = skid & m_vehicleData.steering.isSkidAllowed;		

		avgRotSpeed *= 0.5f / (float)IVP_PI / m_wheelCount;

		float estEngineRPM = avgRotSpeed * m_vehicleData.engine.axleRatio * m_vehicleData.engine.gearRatio[ m_currentState.gear ] * 60;

		// only shift up when going forward
		if ( throttle > 0 )
		{
			// check for higher gear, top gear is gearcount-1 (0 based)
			while ( estEngineRPM > m_vehicleData.engine.shiftUpRPM && m_currentState.gear < m_vehicleData.engine.gearCount-1 )
			{
				m_currentState.gear++;
				estEngineRPM = avgRotSpeed * m_vehicleData.engine.axleRatio * m_vehicleData.engine.gearRatio[ m_currentState.gear ] * 60;
			}
		}

		// check for lower gear
		while ( estEngineRPM < m_vehicleData.engine.shiftDownRPM && m_currentState.gear > 0 )
		{
			m_currentState.gear--;
			estEngineRPM = avgRotSpeed * m_vehicleData.engine.axleRatio * m_vehicleData.engine.gearRatio[ m_currentState.gear ] * 60;
		}
		m_currentState.engineRPM = avgRotSpeed * m_vehicleData.engine.axleRatio * m_vehicleData.engine.gearRatio[ m_currentState.gear ] * 60;
	}
	
	// speed governor
#if 0
	if ( fabs(throttle) > 0 && ( (!torqueBoost && absSpeed > m_vehicleData.engine.maxSpeed ) || 
		 ( torqueBoost && absSpeed > m_vehicleData.engine.boostMaxSpeed) ) )
	{
		throttle = 0.0;
    }
#endif
	if ( ( throttle > 0 ) && ( ( !torqueBoost && absSpeed > m_vehicleData.engine.maxSpeed ) || 
		 ( torqueBoost && absSpeed > m_vehicleData.engine.boostMaxSpeed) ) )
	{
		throttle = 0.0;
    }

	// Check for reverse - both of these "governors" or horrible and need to be redone before we ship!
	if ( ( throttle < 0 ) && ( !torqueBoost && ( absSpeed > m_vehicleData.engine.maxRevSpeed ) ) )
	{
		throttle = 0.0f;
	}

    if ( throttle != 0.0 )
	{
		m_vehicleFlags &= ~FVEHICLE_THROTTLE_STOPPED;
		// calculate the force that propels the car
		const float watt_per_hp = 745.0f;
		const float seconds_per_minute = 60.0f;

		float wheel_force_by_throttle = throttle * 
			m_vehicleData.engine.horsepower * (watt_per_hp * seconds_per_minute) * 
			m_vehicleData.engine.gearRatio[m_currentState.gear]  * m_vehicleData.engine.axleRatio / 
			(m_vehicleData.engine.maxRPM * m_wheelRadius * (2 * IVP_PI));

		if ( m_currentState.engineRPM >= m_vehicleData.engine.maxRPM )
		{
			wheel_force_by_throttle = 0;
		}

		int wheelIndex = 0;
		for ( int i = 0; i < m_vehicleData.axleCount; i++ )
		{
			float axleFactor = m_vehicleData.axles[i].torqueFactor;

			float boostFactor = 0.5f;
			if ( torqueBoost )
			{
				boostFactor = 1.0f;
			}
			float axleTorque = boostFactor * wheel_force_by_throttle * axleFactor * m_vehicleData.axles[i].wheels.radius;

			for ( int w = 0; w < m_vehicleData.wheelsPerAxle; w++, wheelIndex++ )
			{
				float torqueVal = axleTorque;
				m_pCarSystem->change_wheel_torque((IVP_POS_WHEEL)wheelIndex, torqueVal);
			}
		}
	}
	else if ( brake_val != 0 )
	{
		m_vehicleFlags &= ~FVEHICLE_THROTTLE_STOPPED;

		// Brake to slow down the wheel.
		float wheel_force_by_brake = brake_val * m_gravityLength * ( m_bodyMass + m_totalWheelMass );
		
		float sign = m_currentState.speed >= 0.0f ? -1.0f : 1.0f;
		int wheelIndex = 0;
		for ( int i = 0; i < m_vehicleData.axleCount; i++ )
		{
			float torque_val = 0.5 * sign * wheel_force_by_brake * m_vehicleData.axles[i].brakeFactor * m_vehicleData.axles[i].wheels.radius;
			for ( int w = 0; w < m_vehicleData.wheelsPerAxle; w++, wheelIndex++ )
			{
				m_pCarSystem->change_wheel_torque( ( IVP_POS_WHEEL )wheelIndex, torque_val );
			}
		}
	}
	else if ( !(m_vehicleFlags & FVEHICLE_THROTTLE_STOPPED) )
	{
		m_vehicleFlags |= FVEHICLE_THROTTLE_STOPPED;

		for ( int w = 0; w < m_wheelCount; w++ )
		{
			m_pCarSystem->change_wheel_torque((IVP_POS_WHEEL)w, 0);
		}
	}

	bool currentHandbrake = (m_vehicleFlags & FVEHICLE_HANDBRAKE_ON) ? true : false;
	if ( handbrake != currentHandbrake )
	{
		if ( handbrake )
		{
			m_vehicleFlags |= FVEHICLE_HANDBRAKE_ON;
		}
		else
		{
			m_vehicleFlags &= ~FVEHICLE_HANDBRAKE_ON;
		}

		for ( int w = 0; w < m_wheelCount; w++ )
		{
			m_pCarSystem->fix_wheel( (IVP_POS_WHEEL)w, handbrake ? IVP_TRUE : IVP_FALSE );
		}
	}

	if ( m_nVehicleType == VEHICLE_TYPE_CAR_WHEELS )
	{
		for ( int w = 0; w < m_wheelCount; w++ )
		{
			m_currentState.wheelSkidValue[w] = m_pWheels[w]->GetObject()->get_core()->car_wheel->last_skid_value;
		}

//	    m_pCarSystem->update_body_countertorque();
	}

    // motor sound effects
    // ...
}

//-----------------------------------------------------------------------------
// Purpose: Get debug rendering data from the ipion physics system.
//-----------------------------------------------------------------------------
void CVehicleController::GetCarSystemDebugData( vehicle_debugcarsystem_t &debugCarSystem )
{
	IVP_CarSystemDebugData_t carSystemDebugData;
	m_pCarSystem->GetCarSystemDebugData( carSystemDebugData );

	// Raycast car wheel trace data.
	for ( int iWheel = 0; iWheel < VEHICLE_DEBUGRENDERDATA_MAX_WHEELS; ++iWheel )
	{
		debugCarSystem.vecWheelRaycasts[iWheel][0].x = carSystemDebugData.wheelRaycasts[iWheel][0].k[0];
		debugCarSystem.vecWheelRaycasts[iWheel][0].y = carSystemDebugData.wheelRaycasts[iWheel][0].k[1];
		debugCarSystem.vecWheelRaycasts[iWheel][0].z = carSystemDebugData.wheelRaycasts[iWheel][0].k[2];

		debugCarSystem.vecWheelRaycasts[iWheel][1].x = carSystemDebugData.wheelRaycasts[iWheel][1].k[0];
		debugCarSystem.vecWheelRaycasts[iWheel][1].y = carSystemDebugData.wheelRaycasts[iWheel][1].k[1];
		debugCarSystem.vecWheelRaycasts[iWheel][1].z = carSystemDebugData.wheelRaycasts[iWheel][1].k[2];

		debugCarSystem.vecWheelRaycastImpacts[iWheel] = debugCarSystem.vecWheelRaycasts[iWheel][0] + ( carSystemDebugData.wheelRaycastImpacts[iWheel] *
			                                            ( debugCarSystem.vecWheelRaycasts[iWheel][1] - debugCarSystem.vecWheelRaycasts[iWheel][0] ) );
	}
}


//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
void CVehicleController::WriteToTemplate( vphysics_save_cvehiclecontroller_t &controllerTemplate )
{
	// Get rid of the handbrake flag.  The car keeps the flag and will reset it fixing wheels, 
	// else the system thinks it already fixed the wheels on load and the car roles.
	m_vehicleFlags &= ~FVEHICLE_HANDBRAKE_ON;

	controllerTemplate.m_pCarBody = m_pCarBody;
	controllerTemplate.m_wheelCount = m_wheelCount;
	controllerTemplate.m_wheelRadius = m_wheelRadius;
	controllerTemplate.m_bodyMass = m_bodyMass;
	controllerTemplate.m_totalWheelMass = m_totalWheelMass;
	controllerTemplate.m_gravityLength = m_gravityLength;
	controllerTemplate.m_vehicleFlags = m_vehicleFlags;
	controllerTemplate.m_bSlipperyTires = m_bSlipperyTires;
	controllerTemplate.m_nVehicleType = m_nVehicleType;
	controllerTemplate.m_bTraceData = m_bTraceData;
	controllerTemplate.m_bOccupied = m_bOccupied;
	controllerTemplate.m_bEngineDisable = m_bEngineDisable;
	memcpy( &controllerTemplate.m_currentState, &m_currentState, sizeof(m_currentState) );
	memcpy( &controllerTemplate.m_vehicleData, &m_vehicleData, sizeof(m_vehicleData) );
	for (int i = 0; i < VEHICLE_MAX_WHEEL_COUNT; ++i )
	{
		controllerTemplate.m_pWheels[i] = m_pWheels[i];
		ConvertPositionToHL( m_wheelPosition_Bs[i], controllerTemplate.m_wheelPosition_Bs[i] );
		ConvertPositionToHL( m_tracePosition_Bs[i], controllerTemplate.m_tracePosition_Bs[i] );
	}
}


void CVehicleController::InitFromTemplate( CPhysicsEnvironment *pEnv, void *pGameData, const vphysics_save_cvehiclecontroller_t &controllerTemplate )
{
	m_pEnv = pEnv;
	m_pCarBody = controllerTemplate.m_pCarBody;
	m_wheelCount = controllerTemplate.m_wheelCount;
	m_wheelRadius = controllerTemplate.m_wheelRadius;
	m_bodyMass = controllerTemplate.m_bodyMass;
	m_totalWheelMass = controllerTemplate.m_totalWheelMass;
	m_gravityLength = controllerTemplate.m_gravityLength;
	m_vehicleFlags = controllerTemplate.m_vehicleFlags;
	m_bSlipperyTires = controllerTemplate.m_bSlipperyTires;
	m_nVehicleType = controllerTemplate.m_nVehicleType;
	m_bTraceData = controllerTemplate.m_bTraceData;
	m_bOccupied = controllerTemplate.m_bOccupied;
	m_bEngineDisable = controllerTemplate.m_bEngineDisable;
	memcpy( &m_currentState, &controllerTemplate.m_currentState, sizeof(m_currentState) );
	memcpy( &m_vehicleData, &controllerTemplate.m_vehicleData, sizeof(m_vehicleData) );

	for (int i = 0; i < VEHICLE_MAX_WHEEL_COUNT; ++i )
	{
		m_pWheels[i] = controllerTemplate.m_pWheels[i];
		ConvertPositionToIVP( controllerTemplate.m_wheelPosition_Bs[i], m_wheelPosition_Bs[i] );
		ConvertPositionToIVP( controllerTemplate.m_tracePosition_Bs[i], m_tracePosition_Bs[i] );
	}

	CreateIVPObjects( );
}

//-----------------------------------------------------------------------------
// Class factory
//-----------------------------------------------------------------------------
IPhysicsVehicleController *CreateVehicleController( CPhysicsEnvironment *pEnv, CPhysicsObject *pBodyObject, const vehicleparams_t &params, unsigned int nVehicleType,  IPhysicsGameTrace *pGameTrace )
{
	CVehicleController *pController = new CVehicleController( params, pEnv, nVehicleType, pGameTrace );
	pController->InitCarSystem( pBodyObject );

	return pController;
}

bool SavePhysicsVehicleController( const physsaveparams_t &params, CVehicleController *pVehicleController )
{
	vphysics_save_cvehiclecontroller_t controllerTemplate;
	memset( &controllerTemplate, 0, sizeof(controllerTemplate) );

	pVehicleController->WriteToTemplate( controllerTemplate );
	params.pSave->WriteAll( &controllerTemplate );

	return true;
}

bool RestorePhysicsVehicleController( const physrestoreparams_t &params, CVehicleController **ppVehicleController )
{
	*ppVehicleController = new CVehicleController();
	
	vphysics_save_cvehiclecontroller_t controllerTemplate;
	memset( &controllerTemplate, 0, sizeof(controllerTemplate) );
	params.pRestore->ReadAll( &controllerTemplate );

	(*ppVehicleController)->InitFromTemplate( static_cast<CPhysicsEnvironment *>(params.pEnvironment), params.pGameData, controllerTemplate );

	return true;
}


