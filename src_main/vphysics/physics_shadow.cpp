#include <stdio.h>

#include "vphysics_interface.h"
#include "physics_object.h"
#include "physics_shadow.h"
#include "physics_environment.h"
#include "physics_material.h"
#include "convert.h"
#include "isaverestore.h"
#include "vphysics_saverestore.h"

// ipion
#include "ivp_physics.hxx"

// IsInContact
#include "ivp_mindist.hxx"
#include "ivp_core.hxx"
#include "ivp_friction.hxx"

#include "mathlib.h"
#include "tier0/dbg.h"


struct vphysics_save_cshadowcontroller_t;
struct vphysics_save_shadowcontrolparams_t;


// UNDONE: Try this controller!
//damping is usually 1.0
//frequency is usually in the range 1..16
void ComputePDControllerCoefficients( float *coefficientsOut, const float frequency, const float damping, const float dt )
{
	const float ks = 9.0f * frequency * frequency;
	const float kd = 4.5f * frequency * damping;

	const float scale = 1.0f / ( 1.0f  + kd * dt + ks * dt * dt );

	coefficientsOut[0] = ks  *  scale;
	coefficientsOut[1] = ( kd + ks * dt ) * scale;

	// Use this controller like:
	// speed += (coefficientsOut[0] * (targetPos - currentPos) + coefficientsOut[1] * (targetSpeed - currentSpeed)) * dt
}

void ComputeController( IVP_U_Float_Point &currentSpeed, const IVP_U_Float_Point &delta, const IVP_U_Float_Point &maxSpeed, float scaleDelta, float damping )
{
	// scale by timestep
	IVP_U_Float_Point acceleration;
	acceleration.set_multiple( &delta, scaleDelta );
	
	if ( currentSpeed.quad_length() < 1e-6 )
	{
		currentSpeed.set_to_zero();
	}

	acceleration.add_multiple( &currentSpeed, -damping );

	for(int i=2; i>=0; i--)
	{
		if(IVP_Inline_Math::fabsd(acceleration.k[i]) < maxSpeed.k[i]) 
			continue;
		
		// clip force
		acceleration.k[i] = (acceleration.k[i] < 0) ? -maxSpeed.k[i] : maxSpeed.k[i];
	}

	currentSpeed.add( &acceleration );
}


class CPlayerController : public IVP_Controller_Independent, public IPhysicsPlayerController
{
public:
	CPlayerController( CPhysicsObject *pObject );
	~CPlayerController( void );

	// ipion interfaces
    void do_simulation_controller( IVP_Event_Sim *es,IVP_U_Vector<IVP_Core> *cores);
    virtual IVP_CONTROLLER_PRIORITY get_controller_priority() { return IVP_CP_MOTION; }

	void SetObject( IPhysicsObject *pObject );
	void SetEventHandler( IPhysicsPlayerControllerEvent *handler );
	void Update( const Vector& position, const Vector& velocity, bool onground, IPhysicsObject *ground );
	void MaxSpeed( const Vector &velocity );
	bool IsInContact( void );
	virtual void GetControlVelocity( Vector &outVel )
	{
		ConvertPositionToHL( m_currentSpeed, outVel );
	}
	int GetShadowPosition( Vector *position, QAngle *angles )
	{
		IVP_U_Matrix matrix;
		
		IVP_Environment *pEnv = m_pObject->GetObject()->get_environment();
		CPhysicsEnvironment *pVEnv = (CPhysicsEnvironment *)pEnv->client_data;

		double psi = pEnv->get_next_PSI_time().get_seconds();
		m_pObject->GetObject()->calc_at_matrix( psi, &matrix );
		if ( angles )
		{
			ConvertRotationToHL( matrix, *angles );
		}
		if ( position )
		{
			ConvertPositionToHL( matrix.vv, *position );
		}

		return pVEnv->GetSimulatedPSIs();
	}
	virtual void StepUp( float height );


private:
	void AttachObject( void );
	void DetachObject( void );
	int TryTeleportObject( void );

	bool				m_enable;
	bool				m_onground;
	CPhysicsObject		*m_pObject;
	IVP_U_Float_Point	m_saveRot;

	IPhysicsPlayerControllerEvent	*m_handler;
	float				m_maxDeltaPosition;
	float				m_dampFactor;
	IVP_U_Point			m_targetPosition;
	IVP_U_Float_Point	m_maxSpeed;
	IVP_U_Float_Point	m_currentSpeed;
};


CPlayerController::CPlayerController( CPhysicsObject *pObject )
{
	m_pObject = pObject;
	m_handler = NULL;
	m_maxDeltaPosition = ConvertDistanceToIVP( 24 );
	m_dampFactor = 1.0f;
	AttachObject();
}

CPlayerController::~CPlayerController( void ) 
{
	DetachObject();
}

void CPlayerController::AttachObject( void )
{
	IVP_Real_Object *pivp = m_pObject->GetObject();
	IVP_Core *pCore = pivp->get_core();
	m_saveRot = pCore->rot_speed_damp_factor;
	pCore->rot_speed_damp_factor = IVP_U_Float_Point( 100, 100, 100 );
	pCore->calc_calc();
	pivp->get_environment()->get_controller_manager()->add_controller_to_core( this, pCore );
}

void CPlayerController::DetachObject( void )
{
	if ( !m_pObject )
		return;

	IVP_Real_Object *pivp = m_pObject->GetObject();
	IVP_Core *pCore = pivp->get_core();
	pCore->rot_speed_damp_factor = m_saveRot;
	pCore->calc_calc();
	m_pObject = NULL;
	pivp->get_environment()->get_controller_manager()->remove_controller_from_core( this, pCore );
}

void CPlayerController::SetObject( IPhysicsObject *pObject )
{
	CPhysicsObject *obj = (CPhysicsObject *)pObject;
	if ( obj == m_pObject )
		return;

	DetachObject();
	m_pObject = obj;
	AttachObject();
}

int CPlayerController::TryTeleportObject( void )
{
	if ( m_handler )
	{
		Vector hlPosition;
		ConvertPositionToHL( m_targetPosition, hlPosition );
		if ( !m_handler->ShouldMoveTo( m_pObject, hlPosition ) )
			return 0;
	}

	IVP_Real_Object *pivp = m_pObject->GetObject();
	IVP_U_Quat targetOrientation;
	IVP_U_Point outPosition;

	pivp->get_quat_world_f_object_AT( &targetOrientation, &outPosition );

	if ( pivp->is_collision_detection_enabled() )
	{
		m_pObject->EnableCollisions( false );
		pivp->beam_object_to_new_position( &targetOrientation, &m_targetPosition, IVP_TRUE );
		m_pObject->EnableCollisions( true );
	}
	else
	{
		pivp->beam_object_to_new_position( &targetOrientation, &m_targetPosition, IVP_TRUE );
	}

	return 1;
}

void CPlayerController::StepUp( float height )
{
	Vector step( 0, 0, height );

	IVP_Real_Object *pIVP = m_pObject->GetObject();
	IVP_U_Quat world_f_object;
	IVP_U_Point positionIVP, deltaIVP;
	ConvertPositionToIVP( step, deltaIVP );
	pIVP->get_quat_world_f_object_AT( &world_f_object, &positionIVP );
	positionIVP.add( &deltaIVP );
	pIVP->beam_object_to_new_position( &world_f_object, &positionIVP, IVP_TRUE );
}

void CPlayerController::do_simulation_controller( IVP_Event_Sim *es,IVP_U_Vector<IVP_Core> *)
{
	if ( !m_enable )
		return;

	IVP_Real_Object *pivp = m_pObject->GetObject();
	IVP_Environment *pIVPEnv = pivp->get_environment();
	CPhysicsEnvironment *pVEnv = (CPhysicsEnvironment *)pIVPEnv->client_data;

	float psiScale = pVEnv->GetInvPSIScale();
	if ( !psiScale )
		return;

	IVP_Core *pCore = pivp->get_core();
	// current situation
	const IVP_U_Matrix *m_world_f_core = pCore->get_m_world_f_core_PSI();
	const IVP_U_Point *cur_pos_ws = m_world_f_core->get_position();

	// ---------------------------------------------------------
	// Translation
	// ---------------------------------------------------------

	IVP_U_Float_Point delta_position;  delta_position.subtract( &m_targetPosition, cur_pos_ws);


	if (!pivp->flags.shift_core_f_object_is_zero)
	{
		IVP_U_Float_Point shift_cs_os_ws;
		m_world_f_core->vmult3( pivp->get_shift_core_f_object(), &shift_cs_os_ws);
		delta_position.subtract( &shift_cs_os_ws );
	}


	IVP_DOUBLE qdist = delta_position.quad_length();

	// UNDONE: This is totally bogus!  Measure error using last known estimate
	// not current position!
	if ( qdist > m_maxDeltaPosition * m_maxDeltaPosition )
	{
		if ( TryTeleportObject() )
			return;
	}

	// float to allow stepping
	if ( m_onground )
	{
		const IVP_U_Point *pgrav = es->environment->get_gravity();
		IVP_U_Float_Point gravSpeed;
		gravSpeed.set_multiple( pgrav, es->delta_time );
		pCore->speed.subtract( &gravSpeed );
	}

	ComputeController( pCore->speed, delta_position, m_maxSpeed, psiScale / es->delta_time, m_dampFactor );
}

void CPlayerController::SetEventHandler( IPhysicsPlayerControllerEvent *handler ) 
{
	m_handler = handler;
}

void CPlayerController::Update( const Vector& position, const Vector& velocity, bool onground, IPhysicsObject *ground )
{
	IVP_U_Point	targetPositionIVP;
	IVP_U_Float_Point targetSpeedIVP;

	ConvertPositionToIVP( position, targetPositionIVP );
	ConvertPositionToIVP( velocity, targetSpeedIVP );
	
	// if the object hasn't moved, abort
	if ( targetSpeedIVP.quad_distance_to( &m_currentSpeed ) < 1e-6 )
	{
		if ( targetPositionIVP.quad_distance_to( &m_targetPosition ) < 1e-6 )
		{
			return;
		}
	}

	m_targetPosition = targetPositionIVP;
	m_currentSpeed = targetSpeedIVP;

	IVP_Real_Object *pivp = m_pObject->GetObject();
	IVP_Core *pCore = pivp->get_core();
	IVP_Environment *pEnv = pivp->get_environment();
	pEnv->get_controller_manager()->ensure_core_in_simulation(pCore);

	m_enable = true;
	// m_onground makes this object anti-grav
	// UNDONE: Re-evaluate this
	m_onground = false;//onground;

	if ( velocity.LengthSqr() <= 0.1f )
	{
		// no input velocity, just go where physics takes you.
		m_enable = false;
		ground = NULL;
	}
	else
	{
		MaxSpeed( velocity );
	}
}

void CPlayerController::MaxSpeed( const Vector &velocity )
{
	IVP_Core *pCore = m_pObject->GetObject()->get_core();
	IVP_U_Float_Point ivpVel;
	ConvertPositionToIVP( velocity, ivpVel );
	IVP_U_Float_Point available = ivpVel;
	
	// normalize and save length
	float length = ivpVel.real_length_plus_normize();

	float dot = ivpVel.dot_product( &pCore->speed );
	if ( dot > 0 )
	{
		ivpVel.mult( dot * length );
		available.subtract( &ivpVel );
	}

	IVP_Float_PointAbs( m_maxSpeed, available );
}


bool CPlayerController::IsInContact( void )
{
	IVP_Real_Object *pivp = m_pObject->GetObject();
	if ( !pivp->flags.collision_detection_enabled )
		return false;

	IVP_Synapse_Friction *pfriction = pivp->get_first_friction_synapse();
	while ( pfriction )
	{
		extern IVP_Real_Object *GetOppositeSynapseObject( IVP_Synapse_Friction *pfriction );

		IVP_Real_Object *pobj = GetOppositeSynapseObject( pfriction );
		if ( pobj->flags.collision_detection_enabled )
		{
			// skip if this is a static object
			if ( !pobj->get_core()->physical_unmoveable && !pobj->get_core()->pinned )
			{
				CPhysicsObject *pPhys = static_cast<CPhysicsObject *>(pobj->client_data);
				
				// skip if this is a shadow object
				// UNDONE: Is this a hack? need a better way to detect shadows?
				if ( !(pPhys->GetCallbackFlags() & CALLBACK_SHADOW_COLLISION) )
					return true;
			}
		}

		pfriction = pfriction->get_next();
	}

	return false;
}


IPhysicsPlayerController *CreatePlayerController( CPhysicsObject *pObject )
{
	return new CPlayerController( pObject );
}

void QuaternionDiff( const IVP_U_Quat &p, const IVP_U_Quat &q, IVP_U_Quat &qt )
{
	IVP_U_Quat q2;

	// decide if one of the quaternions is backwards
	q2.set_invert_unit_quat( &q );
	qt.set_mult_quat( &q2, &p );
	qt.normize_quat();
}


void QuaternionAxisAngle( const IVP_U_Quat &q, Vector &axis, float &angle )
{
	angle = 2 * acos(q.w);
	if ( angle > M_PI )
	{
		angle -= 2*M_PI;
	}

	axis.Init( q.x, q.y, q.z );
	VectorNormalize( axis );
}


void GetObjectPosition_IVP( IVP_U_Point &origin, IVP_Real_Object *pivp )
{
	const IVP_U_Matrix *m_world_f_core = pivp->get_core()->get_m_world_f_core_PSI();
	origin.set( m_world_f_core->get_position() );
	if (!pivp->flags.shift_core_f_object_is_zero)
	{
		IVP_U_Float_Point shift_cs_os_ws;
		m_world_f_core->vmult3( pivp->get_shift_core_f_object(), &shift_cs_os_ws );
		origin.add(&shift_cs_os_ws);
	}
}


bool IsZeroVector( const IVP_U_Point &vec )
{
	return (vec.k[0] == 0.0 && vec.k[1] == 0.0 && vec.k[2] == 0.0 ) ? true : false;
}

float ComputeShadowControllerIVP( IVP_Real_Object *pivp, shadowcontrol_params_t &params, float secondsToArrival, float dt )
{
	// resample fraction
	// This allows us to arrive at the target at the requested time
	float fraction = 1.0;
	if ( secondsToArrival > 0 )
	{
		fraction *= dt / secondsToArrival;
		if ( fraction > 1 )
		{
			fraction = 1;
		}
	}

	secondsToArrival -= dt;
	if ( secondsToArrival < 0 )
	{
		secondsToArrival = 0;
	}

	if ( fraction <= 0 )
		return secondsToArrival;

	// ---------------------------------------------------------
	// Translation
	// ---------------------------------------------------------

	IVP_U_Point positionIVP;
	GetObjectPosition_IVP( positionIVP, pivp );

	IVP_U_Float_Point delta_position;
	delta_position.subtract( &params.targetPosition, &positionIVP);

	// BUGBUG: Save off velocities and estimate final positions
	// measure error against these final sets
	// also, damp out 100% saved velocity, use max additional impulses
	// to correct error and damp out error velocity
	// extrapolate position
	if ( params.teleportDistance > 0 )
	{
		IVP_DOUBLE qdist;
		if ( !IsZeroVector(params.lastPosition) )
		{
			IVP_U_Float_Point tmpDelta;
			tmpDelta.subtract( &positionIVP, &params.lastPosition );
			qdist = tmpDelta.quad_length();
		}
		else
		{
			// UNDONE: This is totally bogus!  Measure error using last known estimate
			// not current position!
			qdist = delta_position.quad_length();
		}

		if ( qdist > params.teleportDistance * params.teleportDistance )
		{
			if ( pivp->is_collision_detection_enabled() )
			{
				pivp->enable_collision_detection( IVP_FALSE );
				pivp->beam_object_to_new_position( &params.targetRotation, &params.targetPosition, IVP_TRUE );
				pivp->enable_collision_detection( IVP_TRUE );
			}
			else
			{
				pivp->beam_object_to_new_position( &params.targetRotation, &params.targetPosition, IVP_TRUE );
			}
		}
	}

	float invDt = 1.0f / dt;
	IVP_Core *pCore = pivp->get_core();
	ComputeController( pCore->speed, delta_position, params.maxSpeed, fraction * invDt, params.dampFactor );

	params.lastPosition.add_multiple( &positionIVP, &pCore->speed, dt );

	IVP_U_Float_Point deltaAngles;
	// compute rotation offset
	IVP_U_Quat deltaRotation;
	QuaternionDiff( params.targetRotation, pCore->q_world_f_core_next_psi, deltaRotation );

	// convert offset to angular impulse
	Vector axis;
	float angle;
	QuaternionAxisAngle( deltaRotation, axis, angle );
	VectorNormalize(axis);

	deltaAngles.k[0] = axis.x * angle;
	deltaAngles.k[1] = axis.y * angle;
	deltaAngles.k[2] = axis.z * angle;

	ComputeController( pCore->rot_speed, deltaAngles, params.maxAngular, fraction * invDt, params.dampFactor );

	return secondsToArrival;
}

void ConvertShadowControllerToIVP( const hlshadowcontrol_params_t &in, shadowcontrol_params_t &out )
{
	ConvertPositionToIVP( in.targetPosition, out.targetPosition );
	ConvertRotationToIVP( in.targetRotation, out.targetRotation );
	out.teleportDistance = ConvertDistanceToIVP( in.teleportDistance );

	ConvertForceImpulseToIVP( in.maxSpeed, out.maxSpeed);
	IVP_Float_PointAbs( out.maxSpeed, out.maxSpeed );
	ConvertAngularImpulseToIVP( in.maxAngular, out.maxAngular );
	IVP_Float_PointAbs( out.maxAngular, out.maxAngular );
	out.dampFactor = in.dampFactor;
}

void ConvertShadowControllerToHL( const shadowcontrol_params_t &in, hlshadowcontrol_params_t &out )
{
	ConvertPositionToHL( in.targetPosition, out.targetPosition );
	ConvertRotationToHL( in.targetRotation, out.targetRotation );
	out.teleportDistance = ConvertDistanceToHL( in.teleportDistance );

	ConvertForceImpulseToHL( in.maxSpeed, out.maxSpeed);
	VectorAbs( out.maxSpeed, out.maxSpeed );
	ConvertAngularImpulseToHL( in.maxAngular, out.maxAngular );
	VectorAbs( out.maxAngular, out.maxAngular );
	out.dampFactor = in.dampFactor;
}

float ComputeShadowControllerHL( CPhysicsObject *pObject, const hlshadowcontrol_params_t &params, float secondsToArrival, float dt )
{
	shadowcontrol_params_t ivpParams;
	ConvertShadowControllerToIVP( params, ivpParams );
	return ComputeShadowControllerIVP( pObject->GetObject(), ivpParams, secondsToArrival, dt );
}


class CShadowController : public IVP_Controller_Independent, public IPhysicsShadowController
{
public:
	CShadowController() {}
	CShadowController( CPhysicsObject *pObject, bool allowTranslation, bool allowRotation );
	~CShadowController( void );

	// ipion interfaces
    void do_simulation_controller( IVP_Event_Sim *es,IVP_U_Vector<IVP_Core> *cores);
    virtual IVP_CONTROLLER_PRIORITY get_controller_priority() { return IVP_CP_MOTION; }

	void SetObject( IPhysicsObject *pObject );
	void Update( const Vector &position, const QAngle &angles, float secondsToArrival );
	void MaxSpeed( const Vector &velocity, const AngularImpulse &angularVelocity );
	virtual void GetControlVelocity( Vector &outVel )
	{
		ConvertPositionToHL( m_currentSpeed, outVel );
	}
	virtual void StepUp( float height );
	virtual void SetTeleportDistance( float teleportDistance );
	virtual bool AllowsTranslation() { return m_allowPhysicsMovement; }
	virtual bool AllowsRotation() { return m_allowPhysicsRotation; }

	void WriteToTemplate( vphysics_save_cshadowcontroller_t &controllerTemplate );
	void InitFromTemplate( const vphysics_save_cshadowcontroller_t &controllerTemplate );

private:
	void AttachObject( void );
	void DetachObject( void );

	CPhysicsObject		*m_pObject;
	float				m_secondsToArrival;
	IVP_U_Float_Point	m_saveRot;
	IVP_U_Float_Point	m_savedRI;
	IVP_U_Float_Point	m_currentSpeed;
	float				m_savedMass;
	int					m_savedMaterialIndex;
	bool				m_enable;
	bool				m_allowPhysicsMovement;
	bool				m_allowPhysicsRotation;
	shadowcontrol_params_t	m_shadow;
};


CShadowController::CShadowController( CPhysicsObject *pObject, bool allowTranslation, bool allowRotation )
{
	m_pObject = pObject;
	m_shadow.dampFactor = 1.0f;
	m_shadow.teleportDistance = 0;

	m_allowPhysicsMovement = allowTranslation;
	m_allowPhysicsRotation = allowRotation;
	AttachObject();
}

CShadowController::~CShadowController( void ) 
{
	DetachObject();
}

void CShadowController::AttachObject( void )
{
	IVP_Real_Object *pivp = m_pObject->GetObject();
	IVP_Core *pCore = pivp->get_core();
	m_saveRot = pCore->rot_speed_damp_factor;
	m_savedRI = *pCore->get_rot_inertia();
	m_savedMass = pCore->get_mass();
	m_savedMaterialIndex = m_pObject->GetMaterialIndexInternal();
	
	m_pObject->SetMaterialIndex( MATERIAL_INDEX_SHADOW );

	pCore->rot_speed_damp_factor = IVP_U_Float_Point( 100, 100, 100 );

	if ( !m_allowPhysicsRotation )
	{
		IVP_U_Float_Point ri( 1e15f, 1e15f, 1e15f );
		pCore->set_rotation_inertia( &ri );
	}
	if ( !m_allowPhysicsMovement )
	{
		m_pObject->SetMass( 1e6f );
		//pCore->inv_rot_inertia.hesse_val = 0.0f;
		m_pObject->EnableGravity( false );
	}

	pCore->calc_calc();
	pivp->get_environment()->get_controller_manager()->add_controller_to_core( this, pCore );
	m_shadow.lastPosition.set_to_zero();
}

void CShadowController::DetachObject( void )
{
	IVP_Real_Object *pivp = m_pObject->GetObject();
	IVP_Core *pCore = pivp->get_core();
	pCore->rot_speed_damp_factor = m_saveRot;
	pCore->set_mass( m_savedMass );
	pCore->set_rotation_inertia( &m_savedRI ); // this calls calc_calc()
	m_pObject = NULL;
	pivp->get_environment()->get_controller_manager()->remove_controller_from_core( this, pCore );
}

void CShadowController::SetObject( IPhysicsObject *pObject )
{
	CPhysicsObject *obj = (CPhysicsObject *)pObject;
	if ( obj == m_pObject )
		return;

	DetachObject();
	m_pObject = obj;
	AttachObject();
}


void CShadowController::StepUp( float height )
{
	Vector step( 0, 0, height );

	IVP_Real_Object *pIVP = m_pObject->GetObject();
	IVP_U_Quat world_f_object;
	IVP_U_Point positionIVP, deltaIVP;
	ConvertPositionToIVP( step, deltaIVP );
	pIVP->get_quat_world_f_object_AT( &world_f_object, &positionIVP );
	positionIVP.add( &deltaIVP );
	pIVP->beam_object_to_new_position( &world_f_object, &positionIVP, IVP_TRUE );
}

void CShadowController::SetTeleportDistance( float teleportDistance )
{
	m_shadow.teleportDistance = ConvertDistanceToIVP( teleportDistance );
}

void CShadowController::do_simulation_controller( IVP_Event_Sim *es,IVP_U_Vector<IVP_Core> *)
{
	if ( m_enable )
	{
		IVP_Real_Object *pivp = m_pObject->GetObject();

		ComputeShadowControllerIVP( pivp, m_shadow, m_secondsToArrival, es->delta_time );

		// if we have time left, subtract it off
		m_secondsToArrival -= es->delta_time;
		if ( m_secondsToArrival < 0 )
		{
			m_secondsToArrival = 0;
		}
	}
	else
	{
		m_shadow.lastPosition.set_to_zero();
	}
}

// NOTE: This isn't a test for equivalent orientations, it's a test for calling update
// with EXACTLY the same data repeatedly
static bool IsEqual( const IVP_U_Point &pt0, const IVP_U_Point &pt1 )
{
	return pt0.quad_distance_to( &pt1 ) < 1e-8f ? true : false;
}

// NOTE: This isn't a test for equivalent orientations, it's a test for calling update
// with EXACTLY the same data repeatedly
static bool IsEqual( const IVP_U_Quat &pt0, const IVP_U_Quat &pt1 )
{
	float delta = fabs(pt0.x - pt1.x);
	delta += fabs(pt0.y - pt1.y);
	delta += fabs(pt0.z - pt1.z);
	delta += fabs(pt0.w - pt1.w);
	return delta < 1e-8f ? true : false;
}

void CShadowController::Update( const Vector &position, const QAngle &angles, float secondsToArrival )
{
	IVP_U_Point targetPosition = m_shadow.targetPosition;
	IVP_U_Quat targetRotation = m_shadow.targetRotation;

	ConvertPositionToIVP( position, m_shadow.targetPosition );
	m_secondsToArrival = secondsToArrival < 0 ? 0 : secondsToArrival;
	ConvertRotationToIVP( angles, m_shadow.targetRotation );

	m_enable = true;

	if ( IsEqual( targetPosition, m_shadow.targetPosition ) && IsEqual( targetRotation, m_shadow.targetRotation ) )
		return;

	m_pObject->Wake();
}


void CShadowController::MaxSpeed( const Vector &velocity, const AngularImpulse &angularVelocity )
{
	IVP_Core *pCore = m_pObject->GetObject()->get_core();

	// limit additional velocity to that which is not amplifying the current velocity
	IVP_U_Float_Point ivpVel;
	ConvertPositionToIVP( velocity, ivpVel );
	IVP_U_Float_Point available = ivpVel;

	m_currentSpeed = ivpVel;
	
	// normalize and save length
	float length = ivpVel.real_length_plus_normize();

	float dot = ivpVel.dot_product( &pCore->speed );
	if ( dot > 0 )
	{
		ivpVel.mult( dot * length );
		available.subtract( &ivpVel );
	}
	IVP_Float_PointAbs( m_shadow.maxSpeed, available );

	// same for angular, project on to current and remove redundant (amplifying) input
	IVP_U_Float_Point ivpAng;
	ConvertAngularImpulseToIVP( angularVelocity, ivpAng );
	IVP_U_Float_Point availableAng = ivpAng;
	float lengthAng = ivpAng.real_length_plus_normize();

	float dotAng = ivpAng.dot_product( &pCore->rot_speed );
	if ( dotAng > 0 )
	{
		ivpAng.mult( dotAng * lengthAng );
		availableAng.subtract( &ivpAng );
	}

	IVP_Float_PointAbs( m_shadow.maxAngular, availableAng );
}

struct vphysics_save_shadowcontrolparams_t : public hlshadowcontrol_params_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_shadowcontrolparams_t )
	DEFINE_FIELD( vphysics_save_shadowcontrolparams_t,		targetPosition,		FIELD_POSITION_VECTOR	),
	DEFINE_FIELD( vphysics_save_shadowcontrolparams_t,		targetRotation,		FIELD_VECTOR	),
	DEFINE_FIELD( vphysics_save_shadowcontrolparams_t,		maxSpeed,			FIELD_VECTOR	),
	DEFINE_FIELD( vphysics_save_shadowcontrolparams_t,		maxAngular,			FIELD_VECTOR	),
	DEFINE_FIELD( vphysics_save_shadowcontrolparams_t,		dampFactor,			FIELD_FLOAT	),
	DEFINE_FIELD( vphysics_save_shadowcontrolparams_t,		teleportDistance,	FIELD_FLOAT	),
END_DATADESC()

struct vphysics_save_cshadowcontroller_t
{
	CPhysicsObject		*pObject;
	float				secondsToArrival;
	IVP_U_Float_Point	saveRot;
	IVP_U_Float_Point	savedRI;
	IVP_U_Float_Point	currentSpeed;

	float				savedMass;
	int					savedMaterial;
	bool				enable;
	bool				allowPhysicsMovement;
	bool				allowPhysicsRotation;

	hlshadowcontrol_params_t	shadowParams;

	DECLARE_SIMPLE_DATADESC();
};


BEGIN_SIMPLE_DATADESC( vphysics_save_cshadowcontroller_t )
	//DEFINE_VPHYSPTR( vphysics_save_cshadowcontroller_t,	pObject ),
	DEFINE_FIELD( vphysics_save_cshadowcontroller_t,		secondsToArrival,		FIELD_FLOAT	),
	DEFINE_ARRAY( vphysics_save_cshadowcontroller_t,		saveRot.k,				FIELD_FLOAT, 3 ),
	DEFINE_ARRAY( vphysics_save_cshadowcontroller_t,		savedRI.k,				FIELD_FLOAT, 3 ),
	DEFINE_ARRAY( vphysics_save_cshadowcontroller_t,		currentSpeed.k,			FIELD_FLOAT, 3 ),
	DEFINE_FIELD( vphysics_save_cshadowcontroller_t,		savedMass,				FIELD_FLOAT	),
	DEFINE_CUSTOM_FIELD( vphysics_save_cshadowcontroller_t, savedMaterial, MaterialIndexDataOps() ),
	DEFINE_FIELD( vphysics_save_cshadowcontroller_t,		enable,						FIELD_BOOLEAN	),
	DEFINE_FIELD( vphysics_save_cshadowcontroller_t,		allowPhysicsMovement,		FIELD_BOOLEAN	),
	DEFINE_FIELD( vphysics_save_cshadowcontroller_t,		allowPhysicsRotation,		FIELD_BOOLEAN	),

	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_cshadowcontroller_t, shadowParams, vphysics_save_shadowcontrolparams_t ),
END_DATADESC()

void CShadowController::WriteToTemplate( vphysics_save_cshadowcontroller_t &controllerTemplate )
{
	controllerTemplate.pObject = m_pObject;
	controllerTemplate.secondsToArrival = m_secondsToArrival;
	controllerTemplate.saveRot = m_saveRot;
	controllerTemplate.savedRI = m_savedRI;
	controllerTemplate.currentSpeed = m_currentSpeed;
	controllerTemplate.savedMass = m_savedMass;
	controllerTemplate.savedMaterial = m_savedMaterialIndex;
	controllerTemplate.enable = m_enable;
	controllerTemplate.allowPhysicsMovement = m_allowPhysicsMovement;
	controllerTemplate.allowPhysicsRotation = m_allowPhysicsRotation;
	
	ConvertShadowControllerToHL( m_shadow, controllerTemplate.shadowParams );
}

void CShadowController::InitFromTemplate( const vphysics_save_cshadowcontroller_t &controllerTemplate )
{
	m_pObject = controllerTemplate.pObject;

	m_secondsToArrival = controllerTemplate.secondsToArrival;
	m_saveRot = controllerTemplate.saveRot;
	m_savedRI = controllerTemplate.savedRI;
	m_currentSpeed = controllerTemplate.currentSpeed;
	m_savedMass = controllerTemplate.savedMass;
	m_savedMaterialIndex = controllerTemplate.savedMaterial;
	m_enable = controllerTemplate.enable;
	m_allowPhysicsMovement = controllerTemplate.allowPhysicsMovement;
	m_allowPhysicsRotation = controllerTemplate.allowPhysicsRotation;
	
	ConvertShadowControllerToIVP( controllerTemplate.shadowParams, m_shadow );
	m_pObject->GetObject()->get_environment()->get_controller_manager()->add_controller_to_core( this, m_pObject->GetObject()->get_core() );
}


IPhysicsShadowController *CreateShadowController( CPhysicsObject *pObject, bool allowTranslation, bool allowRotation )
{
	return new CShadowController( pObject, allowTranslation, allowRotation );
}



bool SavePhysicsShadowController( const physsaveparams_t &params, IPhysicsShadowController *pIShadow )
{
	vphysics_save_cshadowcontroller_t controllerTemplate;
	memset( &controllerTemplate, 0, sizeof(controllerTemplate) );

	CShadowController *pShadowController = (CShadowController *)pIShadow;
	pShadowController->WriteToTemplate( controllerTemplate );
	params.pSave->WriteAll( &controllerTemplate );
	return true;
}

bool RestorePhysicsShadowController( const physrestoreparams_t &params, IPhysicsShadowController **ppShadowController )
{
	return false;
}

bool RestorePhysicsShadowControllerInternal( const physrestoreparams_t &params, IPhysicsShadowController **ppShadowController, CPhysicsObject *pObject )
{
	vphysics_save_cshadowcontroller_t controllerTemplate;

	memset( &controllerTemplate, 0, sizeof(controllerTemplate) );
	params.pRestore->ReadAll( &controllerTemplate );
	
	// HACKHACK: pass this in
	controllerTemplate.pObject = pObject;

	CShadowController *pShadow = new CShadowController();
	pShadow->InitFromTemplate( controllerTemplate );

	*ppShadowController = pShadow;
	return true;
}

bool SavePhysicsPlayerController( const physsaveparams_t &params, CPlayerController *pPlayerController )
{
	return false;
}

bool RestorePhysicsPlayerController( const physrestoreparams_t &params, CPlayerController **ppPlayerController )
{
	return false;
}

