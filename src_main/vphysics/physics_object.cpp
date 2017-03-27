//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include <stdio.h>
#include "physics_object.h"
#include "ivp_physics.hxx"
#include "ivp_core.hxx"
#include "ivp_surman_polygon.hxx"
#include "ivp_templates.hxx"
#include "ivp_compact_ledge.hxx"
#include "ivp_compact_ledge_solver.hxx"
#include "ivp_mindist.hxx"
#include "ivp_friction.hxx"
#include "ivp_phantom.hxx"

#include "hk_mopp/ivp_surman_mopp.hxx"
#include "hk_mopp/ivp_compact_mopp.hxx"

#include "ivp_compact_surface.hxx"
#include "physics_material.h"
#include "physics_environment.h"
#include "physics_trace.h"
#include "physics_shadow.h"
#include "convert.h"
#include "tier0/dbg.h"

#include "mathlib.h"
#include "isaverestore.h"
#include "vphysics_internal.h"
#include "vphysics_saverestore.h"
#include "bspflags.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IPhysicsCollision *physcollision;

// This angular basis is the integral of each differential drag area's torque over the whole OBB
// For each axis, each face, the integral is (1/Iaxis) * (1/3 * w^2l^3 + 1/2 * w^4l + lw^2h^2)
// l,w, & h are half widths - where l is in the direction of the axis, w is in the plane (l/w plane) of the face, 
// and h is perpendicular to the face.  So for each axis, you sum up this integral over 2 pairs of faces 
// (this function returns the integral for one pair of opposite faces, not one face)
static float AngDragIntegral( float invInertia, float l, float w, float h )
{
	float w2 = w*w;
	float l2 = l*l;
	float h2 = h*h;

	return invInertia * ( (1.f/3.f)*w2*l*l2 + 0.5 * w2*w2*l + l*w2*h2 );
}


CPhysicsObject::CPhysicsObject( void )
{
#ifdef _WIN32
	void *pData = ((char *)this) + sizeof(void *); // offset beyond vtable
	int dataSize = sizeof(*this) - sizeof(void *);
	
	Assert( pData == &m_pGameData );
	
	memset( pData, 0, dataSize );
#elif _LINUX
	
	//!!HACK HACK - rework this if we ever change compiler versions (from gcc 3.2!!!)
	void *pData = ((char *)this) + sizeof(void *); // offset beyond vtable
	int dataSize = sizeof(*this) - sizeof(void *);
	
	Assert( pData == &m_pGameData );
	
	memset( pData, 0, dataSize );	
#else
#error
#endif

	// HACKHACK: init this as a sphere until someone attaches a surfacemanager
	m_collideType = COLLIDE_BALL;
	m_contentsMask = CONTENTS_SOLID;
}

void CPhysicsObject::Init( IVP_Real_Object *pObject, int materialIndex, float volume, float drag, float angDrag, const Vector *massCenterOverride )
{
	m_materialIndex = materialIndex;
	m_pObject = pObject;
	pObject->client_data = (void *)this;
	m_pGameData = NULL;
	m_gameFlags = 0;
	m_sleepState = OBJ_SLEEP;		// objects start asleep
	m_callbacks = CALLBACK_GLOBAL_COLLISION|CALLBACK_GLOBAL_FRICTION|CALLBACK_FLUID_TOUCH|CALLBACK_GLOBAL_TOUCH|CALLBACK_GLOBAL_COLLIDE_STATIC|CALLBACK_DO_FLUID_SIMULATION;
	m_activeIndex = 0xFFFF;
	m_pShadow = NULL;
	m_shadowTempGravityDisable = false;

	m_dragBasis = vec3_origin;
	m_angDragBasis = vec3_origin;
	if ( massCenterOverride )
	{
		m_massCenterOverride = *massCenterOverride;
	}

	if ( !IsStatic() && GetCollide() )
	{
		// Basically we are computing drag as an OBB.  Get OBB extents for projection
		// scale those extents by appropriate mass/inertia to compute velocity directly (not force)
		// in the controller
		// NOTE: Compute these even if drag coefficients are zero, because the drag coefficient could change later

		// Get an AABB for this object and use the area of each side as a basis for approximating cross-section area for drag
		Vector dragMins, dragMaxs;
		// NOTE: coordinates in/out of physcollision are in HL units, not IVP
		physcollision->CollideGetAABB( dragMins, dragMaxs, GetCollide(), vec3_origin, vec3_angle );

		Vector delta = dragMaxs - dragMins;
		ConvertPositionToIVP( delta.x, delta.y, delta.z );
		delta.x = fabsf(delta.x);
		delta.y = fabsf(delta.y);
		delta.z = fabsf(delta.z);
		// dragBasis is now the area of each side
		m_dragBasis.x = delta.y * delta.z;
		m_dragBasis.y = delta.x * delta.z;
		m_dragBasis.z = delta.x * delta.y;
		m_dragBasis *= GetInvMass();

		const IVP_U_Float_Point *pInvRI = m_pObject->get_core()->get_inv_rot_inertia();

		// This angular basis is the integral of each differential drag area's torque over the whole OBB
		// need half lengths for this integral
		delta *= 0.5;
		// rotation about the x axis
		m_angDragBasis.x = AngDragIntegral( pInvRI->k[0], delta.x, delta.y, delta.z ) + AngDragIntegral( pInvRI->k[0], delta.x, delta.z, delta.y );
		// rotation about the y axis
		m_angDragBasis.y = AngDragIntegral( pInvRI->k[1], delta.y, delta.x, delta.z ) + AngDragIntegral( pInvRI->k[1], delta.y, delta.z, delta.x );
		// rotation about the z axis
		m_angDragBasis.z = AngDragIntegral( pInvRI->k[2], delta.z, delta.x, delta.y ) + AngDragIntegral( pInvRI->k[2], delta.z, delta.y, delta.x );
	}
	else
	{
		drag = 0;
		angDrag = 0;
	}

	m_dragCoefficient = drag;
	m_angDragCoefficient = angDrag;

	SetVolume( volume );
}

CPhysicsObject::~CPhysicsObject( void )
{
	RemoveShadowController();

	if ( m_pObject )
	{
		// prevents callbacks to the game code / unlink from this object
		m_callbacks = 0;
		m_pGameData = 0;
		m_pObject->client_data = 0;

		IVP_Core *pCore = m_pObject->get_core();
		if ( pCore->physical_unmoveable == IVP_TRUE && pCore->controllers_of_core.n_elems )
		{
			// go ahead and notify them if this happens in the real world
			for(int i = pCore->controllers_of_core.len()-1; i >=0 ;i-- ) 
			{
				IVP_Controller *my_controller = pCore->controllers_of_core.element_at(i);
				my_controller->core_is_going_to_be_deleted_event(pCore);
				Assert(my_controller==pCore->environment->get_gravity_controller());
			}
		}

		// UNDONE: Don't free the surface manager here
		// UNDONE: Remove reference to it by calling something in physics_collide
		IVP_SurfaceManager *pSurman = GetSurfaceManager();

		IVP_Environment *pEnv = m_pObject->get_environment();
		CPhysicsEnvironment *pVEnv = (CPhysicsEnvironment *)pEnv->client_data;

		// BUGBUG: Sometimes IVP will call a "revive" on the object we're deleting!
		if ( pVEnv && pVEnv->ShouldQuickDelete() )
		{
			m_pObject->delete_silently();
		}
		else
		{
			m_pObject->delete_and_check_vicinity();
		}
		delete pSurman;
	}
}

void CPhysicsObject::Wake( void )
{
	m_pObject->ensure_in_simulation();
}

// supported
void CPhysicsObject::Sleep( void )
{
	m_pObject->disable_simulation();
}


bool CPhysicsObject::IsAsleep( void )
{
	if ( m_sleepState == OBJ_AWAKE )
		return false;

	return true;
}

void CPhysicsObject::NotifySleep( void )
{
	if ( m_sleepState == OBJ_AWAKE )
	{
		m_sleepState = OBJ_STARTSLEEP;
	}
	else
	{
		// UNDONE: This fails sometimes and we get sleep calls for a sleeping object, debug?
		//Assert(m_sleepState==OBJ_STARTSLEEP);
		m_sleepState = OBJ_SLEEP;
	}
}


void CPhysicsObject::NotifyWake( void )
{
	m_sleepState = OBJ_AWAKE;
}


void CPhysicsObject::SetCallbackFlags( unsigned short callbackflags )
{
	m_callbacks = callbackflags;
}


unsigned short CPhysicsObject::GetCallbackFlags( void )
{
	return m_callbacks;
}


void CPhysicsObject::SetGameFlags( unsigned short userFlags )
{
	m_gameFlags = userFlags;
}

unsigned short CPhysicsObject::GetGameFlags( void ) const
{
	return m_gameFlags;
}

bool CPhysicsObject::IsStatic( void )
{
	if ( m_pObject->get_core()->physical_unmoveable )
		return true;

	return false;
}


void CPhysicsObject::EnableCollisions( bool enable )
{
	if ( enable )
	{
		m_callbacks |= CALLBACK_ENABLING_COLLISION;
		m_pObject->enable_collision_detection( IVP_TRUE );
		m_callbacks &= ~CALLBACK_ENABLING_COLLISION;
	}
	else
	{
		m_pObject->enable_collision_detection( IVP_FALSE );
	}
}

void CPhysicsObject::RecheckCollisionFilter()
{
	BEGIN_IVP_ALLOCATION();
	m_pObject->recheck_collision_filter();
	END_IVP_ALLOCATION();
}

CPhysicsEnvironment	*CPhysicsObject::GetVPhysicsEnvironment()
{
	return (CPhysicsEnvironment *) (m_pObject->get_core()->environment->client_data);
}


bool CPhysicsObject::IsControlling( IVP_Controller *pController )
{
	IVP_Core *pCore = m_pObject->get_core();
	for ( int i = 0; i < pCore->controllers_of_core.len(); i++ )
	{
		// already controlling this core?
		if ( pCore->controllers_of_core.element_at(i) == pController )
			return true;
	}

	return false;
}

bool CPhysicsObject::IsGravityEnabled()
{
	if ( !IsStatic() )
	{
		return IsControlling( m_pObject->get_core()->environment->get_gravity_controller() );
	}

	return false;
}

bool CPhysicsObject::IsDragEnabled()
{
	if ( !IsStatic() )
	{
		return IsControlling( GetVPhysicsEnvironment()->GetDragController() );
	}

	return false;
}


bool CPhysicsObject::IsMotionEnabled()
{
	return m_pObject->get_core()->pinned ? false : true;
}


bool CPhysicsObject::IsMoveable()
{
	if ( IsStatic() || !IsMotionEnabled() )
		return false;
	return true;
}


void CPhysicsObject::EnableGravity( bool enable )
{
	if ( IsStatic() )
		return;


	bool isEnabled = IsGravityEnabled();
	
	if ( enable == isEnabled )
		return;

	IVP_Controller *pGravity = m_pObject->get_core()->environment->get_gravity_controller();
	if ( enable )
	{
		m_pObject->get_core()->add_core_controller( pGravity );
	}
	else
	{
		m_pObject->get_core()->rem_core_controller( pGravity );
	}
}

void CPhysicsObject::EnableDrag( bool enable )
{
	if ( IsStatic() )
		return;

	bool isEnabled = IsDragEnabled();
	
	if ( enable == isEnabled )
		return;

	IVP_Controller *pDrag = GetVPhysicsEnvironment()->GetDragController();

	if ( enable )
	{
		m_pObject->get_core()->add_core_controller( pDrag );
	}
	else
	{
		m_pObject->get_core()->rem_core_controller( pDrag );
	}
}


void CPhysicsObject::SetDragCoefficient( float *pDrag, float *pAngularDrag )
{
	if ( pDrag )
	{
		m_dragCoefficient = *pDrag;
	}
	if ( pAngularDrag )
	{
		m_angDragCoefficient = *pAngularDrag;
	}
}


void CPhysicsObject::EnableMotion( bool enable )
{
	bool isMoveable = IsMotionEnabled();

	// no change
	if ( isMoveable == enable )
		return;

	BEGIN_IVP_ALLOCATION();
	m_pObject->set_pinned( enable ? IVP_FALSE : IVP_TRUE );
	END_IVP_ALLOCATION();

	RecheckCollisionFilter();
}


void CPhysicsObject::SetGameData( void *pGameData )
{
	m_pGameData = pGameData;
}

void *CPhysicsObject::GetGameData( void ) const
{
	return m_pGameData;
}

void CPhysicsObject::SetMass( float mass )
{
	Assert( mass > 0 );
	m_pObject->change_mass( mass );
	SetVolume( m_volume );
}

float CPhysicsObject::GetMass( void ) const
{
	return m_pObject->get_core()->get_mass();
}

float CPhysicsObject::GetInvMass( void ) const
{
	return m_pObject->get_core()->get_inv_mass();
}

Vector CPhysicsObject::GetInertia( void ) const
{
	const IVP_U_Float_Point *pRI = m_pObject->get_core()->get_rot_inertia();
	
	Vector hlInertia;
	ConvertDirectionToHL( *pRI, hlInertia );
	VectorAbs( hlInertia, hlInertia );
	return hlInertia;
}

Vector CPhysicsObject::GetInvInertia( void ) const
{
	const IVP_U_Float_Point *pRI = m_pObject->get_core()->get_inv_rot_inertia();

	Vector hlInvInertia;
	ConvertDirectionToHL( *pRI, hlInvInertia );
	VectorAbs( hlInvInertia, hlInvInertia );
	return hlInvInertia;
}


void CPhysicsObject::SetInertia( const Vector &inertia )
{
	IVP_U_Float_Point ri;
	ConvertDirectionToIVP( inertia, ri );
	ri.k[0] = IVP_Inline_Math::fabsd(ri.k[0]);
	ri.k[1] = IVP_Inline_Math::fabsd(ri.k[1]);
	ri.k[2] = IVP_Inline_Math::fabsd(ri.k[2]);
	m_pObject->get_core()->set_rotation_inertia( &ri );
}


void CPhysicsObject::GetDamping( float *speed, float *rot )
{
	IVP_Core *pCore = m_pObject->get_core();
	if ( speed )
	{
		*speed = pCore->speed_damp_factor;
	}
	if ( rot )
	{
		*rot = pCore->rot_speed_damp_factor.k[0];
	}
}

void CPhysicsObject::SetDamping( const float *speed, const float *rot )
{
	IVP_Core *pCore = m_pObject->get_core();
	if ( speed )
	{
		pCore->speed_damp_factor = *speed;
	}
	if ( rot )
	{
		pCore->rot_speed_damp_factor.set( *rot, *rot, *rot );
	}
}

void CPhysicsObject::SetVolume( float volume )
{
	m_volume = volume;
	if ( volume != 0.f )
	{
		volume *= HL2IVP_FACTOR*HL2IVP_FACTOR*HL2IVP_FACTOR;
		float density = GetMass() / volume;
		float matDensity;
		physprops->GetPhysicsProperties( GetMaterialIndexInternal(), &matDensity, NULL, NULL, NULL );
		m_buoyancyRatio = density / matDensity;
	}
	else
	{
		m_buoyancyRatio = 1.0f;
	}
}

float CPhysicsObject::GetVolume()
{
	return m_volume;
}


void CPhysicsObject::SetBuoyancyRatio( float ratio )
{
	m_buoyancyRatio = ratio;
}

void CPhysicsObject::SetContents( unsigned int contents )
{
	m_contentsMask = contents;
}

// converts HL local units to HL world units
void CPhysicsObject::LocalToWorld( Vector &worldPosition, const Vector &localPosition )
{
	matrix3x4_t matrix;
	GetPositionMatrix( matrix );
	// copy in case the src == dest
	VectorTransform( Vector(localPosition), matrix, worldPosition );
}

// Converts world HL units to HL local/object units
void CPhysicsObject::WorldToLocal( Vector &localPosition, const Vector &worldPosition )
{
	matrix3x4_t matrix;
	GetPositionMatrix( matrix );
	// copy in case the src == dest
	VectorITransform( Vector(worldPosition), matrix, localPosition );
}

void CPhysicsObject::LocalToWorldVector( Vector &worldVector, const Vector &localVector )
{
	matrix3x4_t matrix;
	GetPositionMatrix( matrix );
	// copy in case the src == dest
	VectorRotate( Vector(localVector), matrix, worldVector );
}

void CPhysicsObject::WorldToLocalVector( Vector &localVector, const Vector &worldVector )
{
	matrix3x4_t matrix;
	GetPositionMatrix( matrix );
	// copy in case the src == dest
	VectorIRotate( Vector(worldVector), matrix, localVector );
}


// UNDONE: Expose & tune this number  (HL max velocity is 2000 in / s, limit physics objects to same speed)
// NOTE: clamp happens in ivp coordinates, so scale to meters
#define MAX_SPEED		(HL2IVP(2000.f))
#define MAX_ROT_SPEED	(DEG2RAD(360.f*10.f))	// 10 rev/s max rotation speed


// Apply force impulse (momentum) to the object
void CPhysicsObject::ApplyForceCenter( const Vector &forceVector )
{
//	Assert(IsMoveable());
	if ( !IsMoveable() )
		return;

	IVP_U_Float_Point tmp;

	ConvertForceImpulseToIVP( forceVector, tmp );
	IVP_Core *core = m_pObject->get_core();
	tmp.mult( core->get_inv_mass() );
	tmp.k[0] = clamp( tmp.k[0], -MAX_SPEED, MAX_SPEED );
	tmp.k[1] = clamp( tmp.k[1], -MAX_SPEED, MAX_SPEED );
	tmp.k[2] = clamp( tmp.k[2], -MAX_SPEED, MAX_SPEED );
	m_pObject->async_add_speed_object_ws( &tmp );
}

void CPhysicsObject::ApplyForceOffset( const Vector &forceVector, const Vector &worldPosition )
{
//	Assert(IsMoveable());
	if ( !IsMoveable() )
		return;

	IVP_U_Point pos;
	IVP_U_Float_Point force;

	ConvertForceImpulseToIVP( forceVector, force );
	ConvertPositionToIVP( worldPosition, pos );

	IVP_Core *core = m_pObject->get_core();
	core->async_push_core_ws( &pos, &force );
	core->speed_change.k[0] = clamp( core->speed_change.k[0], -MAX_SPEED, MAX_SPEED );
	core->speed_change.k[1] = clamp( core->speed_change.k[1], -MAX_SPEED, MAX_SPEED );
	core->speed_change.k[2] = clamp( core->speed_change.k[2], -MAX_SPEED, MAX_SPEED );

	core->rot_speed_change.k[0] = clamp( core->rot_speed_change.k[0], -MAX_ROT_SPEED, MAX_ROT_SPEED );
	core->rot_speed_change.k[1] = clamp( core->rot_speed_change.k[1], -MAX_ROT_SPEED, MAX_ROT_SPEED );
	core->rot_speed_change.k[2] = clamp( core->rot_speed_change.k[2], -MAX_ROT_SPEED, MAX_ROT_SPEED );
	Wake();
}

void CPhysicsObject::CalculateForceOffset( const Vector &forceVector, const Vector &worldPosition, Vector &centerForce, AngularImpulse &centerTorque )
{
	IVP_U_Point pos;
	IVP_U_Float_Point force;

	ConvertPositionToIVP( forceVector, force );
	ConvertPositionToIVP( worldPosition, pos );

	IVP_Core *core = m_pObject->get_core();

    const IVP_U_Matrix *m_world_f_core = core->get_m_world_f_core_PSI();

    IVP_U_Float_Point point_d_ws;
	point_d_ws.subtract(&pos, m_world_f_core->get_position());

    IVP_U_Float_Point cross_point_dir;

    cross_point_dir.calc_cross_product( &point_d_ws, &force);
    m_world_f_core->inline_vimult3( &cross_point_dir, &cross_point_dir);

	ConvertAngularImpulseToHL( cross_point_dir, centerTorque );
	ConvertForceImpulseToHL( force, centerForce );
}

void CPhysicsObject::CalculateVelocityOffset( const Vector &forceVector, const Vector &worldPosition, Vector &centerVelocity, AngularImpulse &centerAngularVelocity )
{
	IVP_U_Point pos;
	IVP_U_Float_Point force;

	ConvertForceImpulseToIVP( forceVector, force );
	ConvertPositionToIVP( worldPosition, pos );

	IVP_Core *core = m_pObject->get_core();

    const IVP_U_Matrix *m_world_f_core = core->get_m_world_f_core_PSI();

    IVP_U_Float_Point point_d_ws;
	point_d_ws.subtract(&pos, m_world_f_core->get_position());

    IVP_U_Float_Point cross_point_dir;

    cross_point_dir.calc_cross_product( &point_d_ws, &force);
    m_world_f_core->inline_vimult3( &cross_point_dir, &cross_point_dir);

    cross_point_dir.set_pairwise_mult( &cross_point_dir, core->get_inv_rot_inertia());
	ConvertAngularImpulseToHL( cross_point_dir, centerAngularVelocity );
    force.set_multiple( &force, core->get_inv_mass() );
	ConvertForceImpulseToHL( force, centerVelocity );
}

void CPhysicsObject::ApplyTorqueCenter( const AngularImpulse &torqueImpulse )
{
	//Assert(IsMoveable());
	if ( !IsMoveable() )
		return;
	IVP_U_Float_Point ivpTorque;
	ConvertAngularImpulseToIVP( torqueImpulse, ivpTorque );
	IVP_Core *core = m_pObject->get_core();
	core->async_rot_push_core_multiple_ws( &ivpTorque, 1.0 );
	core->rot_speed_change.k[0] = clamp( core->rot_speed_change.k[0], -MAX_ROT_SPEED, MAX_ROT_SPEED );
	core->rot_speed_change.k[1] = clamp( core->rot_speed_change.k[1], -MAX_ROT_SPEED, MAX_ROT_SPEED );
	core->rot_speed_change.k[2] = clamp( core->rot_speed_change.k[2], -MAX_ROT_SPEED, MAX_ROT_SPEED );
	Wake();
}

void CPhysicsObject::GetPosition( Vector *worldPosition, QAngle *angles )
{
	IVP_U_Matrix matrix;
	m_pObject->get_m_world_f_object_AT( &matrix );
	if ( worldPosition )
	{
		ConvertPositionToHL( matrix.vv, *worldPosition );
	}
	
	if ( angles )
	{
		ConvertRotationToHL( matrix, *angles );
	}
}


void CPhysicsObject::GetPositionMatrix( matrix3x4_t& positionMatrix )
{
	IVP_U_Matrix matrix;
	m_pObject->get_m_world_f_object_AT( &matrix );
	ConvertMatrixToHL( matrix, positionMatrix );
}


void CPhysicsObject::GetVelocity( Vector *velocity, AngularImpulse *angularVelocity )
{
	if ( !velocity && !angularVelocity )
		return;

	IVP_Core *core = m_pObject->get_core();
	if ( velocity )
	{
		IVP_U_Float_Point speed;
		speed.add( &core->speed, &core->speed_change );
		ConvertPositionToHL( speed, *velocity );
	}

	if ( angularVelocity )
	{
		IVP_U_Float_Point rotSpeed;
		rotSpeed.add( &core->rot_speed, &core->rot_speed_change );
		// xform to HL space
		ConvertAngularImpulseToHL( rotSpeed, *angularVelocity );
	}
}

void CPhysicsObject::GetVelocityAtPoint( const Vector &worldPosition, Vector &velocity )
{
	IVP_Core *core = m_pObject->get_core();
	IVP_U_Point pos;
	ConvertPositionToIVP( worldPosition, pos );

	IVP_U_Float_Point rotSpeed;
	rotSpeed.add( &core->rot_speed, &core->rot_speed_change );

    IVP_U_Float_Point av_ws;
    core->get_m_world_f_core_PSI()->vmult3( &rotSpeed, &av_ws);

    IVP_U_Float_Point pos_rel; 	
	pos_rel.subtract( &pos, core->get_position_PSI());
    IVP_U_Float_Point cross;    
	cross.inline_calc_cross_product(&av_ws,&pos_rel);

	IVP_U_Float_Point speed;
    speed.add(&core->speed, &cross);
    speed.add(&core->speed_change);

	ConvertPositionToHL( speed, velocity );
}


// UNDONE: Limit these?
void CPhysicsObject::AddVelocity( const Vector *velocity, const AngularImpulse *angularVelocity )
{
	Assert(IsMoveable());
	if ( !IsMoveable() )
		return;
	IVP_Core *core = m_pObject->get_core();

	Wake();

	if ( velocity )
	{
		IVP_U_Float_Point ivpVelocity;
		ConvertPositionToIVP( *velocity, ivpVelocity );
		core->speed_change.add( &ivpVelocity );
	}

	if ( angularVelocity )
	{
		IVP_U_Float_Point ivpAngularVelocity;
		ConvertAngularImpulseToIVP( *angularVelocity, ivpAngularVelocity );

		core->rot_speed_change.add(&ivpAngularVelocity);
	}
}

void CPhysicsObject::SetPosition( const Vector &worldPosition, const QAngle &angles, bool isTeleport )
{
	IVP_U_Quat rot;
	IVP_U_Point pos;

	if ( m_pShadow )
	{
		UpdateShadow( worldPosition, angles, false, 0 );
	}
	ConvertPositionToIVP( worldPosition, pos );

	ConvertRotationToIVP( angles, rot );

	if ( m_pObject->is_collision_detection_enabled() && isTeleport )
	{
		EnableCollisions( false );
		m_pObject->beam_object_to_new_position( &rot, &pos, IVP_FALSE );
		EnableCollisions( true );
	}
	else
	{
		m_pObject->beam_object_to_new_position( &rot, &pos, IVP_FALSE );
	}
}

void CPhysicsObject::SetPositionMatrix( const matrix3x4_t& matrix, bool isTeleport )
{
	if ( m_pShadow )
	{
		Vector worldPosition;
		QAngle angles;
		MatrixAngles( matrix, angles );
		MatrixGetColumn( matrix, 3, worldPosition );
		UpdateShadow( worldPosition, angles, false, 0 );
	}

	IVP_U_Quat rot;
	IVP_U_Matrix mat;

	ConvertMatrixToIVP( matrix, mat );

	rot.set_quaternion( &mat );

	if ( m_pObject->is_collision_detection_enabled() && isTeleport )
	{
		EnableCollisions( false );
		m_pObject->beam_object_to_new_position( &rot, &mat.vv, IVP_FALSE );
		EnableCollisions( true );
	}
	else
	{
		m_pObject->beam_object_to_new_position( &rot, &mat.vv, IVP_FALSE );
	}
}

void CPhysicsObject::SetVelocity( const Vector *velocity, const AngularImpulse *angularVelocity )
{
	Assert(IsMoveable());
	if ( !IsMoveable() )
		return;
	IVP_Core *core = m_pObject->get_core();

	Wake();

	if ( velocity )
	{
		ConvertPositionToIVP( *velocity, core->speed_change );
		core->speed.set_to_zero();
	}

	if ( angularVelocity )
	{
		ConvertAngularImpulseToIVP( *angularVelocity, core->rot_speed_change );
		core->rot_speed.set_to_zero();
	}
}


void GetWorldCoordFromSynapse( IVP_Synapse_Friction *pfriction, IVP_U_Point &world )
{
#if 0
	IVP_Contact_Point *pContact = pfriction->get_contact_point();
	IVP_Contact_Situation *pContactSituation = pContact->get_lt();

	// how do we know if this is out of date?
	if ( pContactSituation )
	{
		world = pContactSituation->contact_point_ws;
	}
	else
	{
		const IVP_Compact_Ledge *pledge = pfriction->edge->get_compact_ledge();
		const IVP_Compact_Poly_Point *pPoint = pfriction->edge->get_start_point( pledge );
		IVP_Cache_Object *cache = pfriction->get_object()->get_cache_object_no_lock();

		cache->transform_position_to_world_coords( pPoint, &world );
	}
#endif

	world.set(pfriction->get_contact_point()->get_contact_point_ws());
}


bool CPhysicsObject::GetContactPoint( Vector *contactPoint, IPhysicsObject **contactObject )
{
	IVP_Synapse_Friction *pfriction = m_pObject->get_first_friction_synapse();
	if ( !pfriction )
		return false;

	if ( contactPoint )
	{
		IVP_U_Point world;
		GetWorldCoordFromSynapse( pfriction, world );
		ConvertPositionToHL( world, *contactPoint );
	}
	if ( contactObject )
	{
		IVP_Real_Object *pivp = GetOppositeSynapseObject( pfriction );
		*contactObject = static_cast<IPhysicsObject *>(pivp->client_data);
	}
	return true;
}

void CPhysicsObject::SetShadow( const Vector &maxVelocity, const AngularImpulse &maxAngularVelocity, bool allowPhysicsMovement, bool allowPhysicsRotation )
{
	if ( m_pShadow )
	{
		m_pShadow->MaxSpeed( maxVelocity, maxAngularVelocity );
	}
	else
	{
	//	m_pObject->get_core()->rot_speed_damp_factor = IVP_U_Float_Point( 1e5f, 1e5f, 1e5f );
		m_shadowTempGravityDisable = false;

		unsigned int flags = GetCallbackFlags() | CALLBACK_SHADOW_COLLISION;
		flags &= ~CALLBACK_GLOBAL_FRICTION;
		flags &= ~CALLBACK_GLOBAL_COLLIDE_STATIC;
		SetCallbackFlags( flags );

		IVP_Environment *pEnv = m_pObject->get_environment();
		CPhysicsEnvironment *pVEnv = (CPhysicsEnvironment *)pEnv->client_data;
		m_pShadow = pVEnv->CreateShadowController( this, allowPhysicsMovement, allowPhysicsRotation );
		m_pShadow->MaxSpeed( maxVelocity, maxAngularVelocity );
		SetRollingDrag( 0 );
		EnableDrag( false );
		if ( !allowPhysicsMovement )
		{
			EnableGravity( false );
		}
	}
}

void CPhysicsObject::UpdateShadow( const Vector &targetPosition, const QAngle &targetAngles, bool tempDisableGravity, float timeOffset )
{
	if ( tempDisableGravity != m_shadowTempGravityDisable )
	{
		m_shadowTempGravityDisable = tempDisableGravity;
		if ( !m_pShadow || m_pShadow->AllowsTranslation() )
		{
			EnableGravity( !m_shadowTempGravityDisable );
		}
	}
	if ( m_pShadow )
	{
		m_pShadow->Update( targetPosition, targetAngles, timeOffset );
	}
}


void CPhysicsObject::RemoveShadowController()
{
	if ( m_pShadow )
	{
		IVP_Environment *pEnv = m_pObject->get_environment();
		CPhysicsEnvironment *pVEnv = (CPhysicsEnvironment *)pEnv->client_data;
		pVEnv->DestroyShadowController( m_pShadow );
		m_pShadow = NULL;
	}
}

// Back door to allow save/restore of backlink between shadow controller and physics object
void CPhysicsObject::RestoreShadowController( IPhysicsShadowController *pShadowController )
{
	Assert( !m_pShadow );
	m_pShadow = pShadowController;
}

int CPhysicsObject::GetShadowPosition( Vector *position, QAngle *angles )
{
	IVP_U_Matrix matrix;
	
	IVP_Environment *pEnv = m_pObject->get_environment();
	CPhysicsEnvironment *pVEnv = (CPhysicsEnvironment *)pEnv->client_data;

	double psi = pEnv->get_next_PSI_time().get_seconds();
	m_pObject->calc_at_matrix( psi, &matrix );
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


IPhysicsShadowController *CPhysicsObject::GetShadowController( void ) const
{
	return m_pShadow;
}

const CPhysCollide *CPhysicsObject::GetCollide( void ) const
{
	switch( m_collideType )
	{
	case COLLIDE_POLY:
		{
			IVP_SurfaceManager_Polygon *pSurman = (IVP_SurfaceManager_Polygon *)m_pObject->get_surface_manager();
			return (const CPhysCollide *)pSurman->get_compact_surface();
		}
	case COLLIDE_BALL:
		return NULL;
#if ENABLE_IVP_MOPP
	case COLLIDE_MOPP:
		{
			IVP_SurfaceManager_Mopp *pSurman = (IVP_SurfaceManager_Mopp *)m_pObject->get_surface_manager();
			return (const CPhysCollide *)pSurman->get_compact_mopp();
		}
#endif
	}
	return NULL;
}


IVP_SurfaceManager *CPhysicsObject::GetSurfaceManager( void ) const
{
	if ( m_collideType != COLLIDE_BALL )
	{
		return m_pObject->get_surface_manager();
	}
	return NULL;
}


float CPhysicsObject::GetDragInDirection( const IVP_U_Float_Point &velocity ) const
{
	IVP_U_Float_Point local;

    const IVP_U_Matrix *m_world_f_core = m_pObject->get_core()->get_m_world_f_core_PSI();
    m_world_f_core->vimult3( &velocity, &local );

	return m_dragCoefficient * IVP_Inline_Math::fabsd( local.k[0] * m_dragBasis.x ) + 
		IVP_Inline_Math::fabsd( local.k[1] * m_dragBasis.y ) + 
		IVP_Inline_Math::fabsd( local.k[2] * m_dragBasis.z );
}

float CPhysicsObject::GetAngularDragInDirection( const IVP_U_Float_Point &angVelocity ) const
{
	return m_angDragCoefficient * IVP_Inline_Math::fabsd( angVelocity.k[0] * m_angDragBasis.x ) + 
		IVP_Inline_Math::fabsd( angVelocity.k[1] * m_angDragBasis.y ) + 
		IVP_Inline_Math::fabsd( angVelocity.k[2] * m_angDragBasis.z );
}

const char *CPhysicsObject::GetName()
{
	return m_pObject->get_name();
}

void CPhysicsObject::SetMaterialIndex( int materialIndex )
{
	if ( m_materialIndex == materialIndex )
		return;

	m_materialIndex = materialIndex;
	IVP_Material *pMaterial = physprops->GetIVPMaterial( materialIndex );
	m_pObject->l_default_material = pMaterial;
	m_pObject->recompile_material_changed();
}

// convert square velocity magnitude from IVP to HL
float CPhysicsObject::GetEnergy()
{
	IVP_Core *pCore = m_pObject->get_core();
	IVP_FLOAT energy = 0.0f;
	IVP_U_Float_Point tmp;

	energy = 0.5f * pCore->get_mass() * pCore->speed.dot_product(&pCore->speed); // 1/2mvv
	tmp.set_pairwise_mult(&pCore->rot_speed, pCore->get_rot_inertia()); // wI
	energy += 0.5f * tmp.dot_product(&pCore->rot_speed); // 1/2mvv + 1/2wIw
	
	return ConvertEnergyToHL( energy );
}

float CPhysicsObject::ComputeShadowControl( const hlshadowcontrol_params_t &params, float secondsToArrival, float dt )
{
	return ComputeShadowControllerHL( this, params, secondsToArrival, dt );
}

float CPhysicsObject::GetSphereRadius()
{
	if ( m_collideType != COLLIDE_BALL )
		return 0;

	return ConvertDistanceToHL( m_pObject->to_ball()->get_radius() );
}

float CPhysicsObject::CalculateLinearDrag( const Vector &unitDirection ) const
{
	IVP_U_Float_Point ivpDir;
	ConvertDirectionToIVP( unitDirection, ivpDir );

	return GetDragInDirection( ivpDir );
}

float CPhysicsObject::CalculateAngularDrag( const Vector &objectSpaceRotationAxis ) const
{
	IVP_U_Float_Point ivpAxis;
	ConvertDirectionToIVP( objectSpaceRotationAxis, ivpAxis );

	// drag factor is per-radian, convert to per-degree
	return GetAngularDragInDirection( ivpAxis ) * DEG2RAD(1.0);
}


void CPhysicsObject::BecomeTrigger()
{
	if ( IsTrigger() )
		return;

	EnableDrag( false );
	EnableGravity( false );

	// UNDONE: Use defaults here?  Do we want object sets by default?
	IVP_Template_Phantom trigger;
    trigger.manage_intruding_cores = IVP_TRUE; // manage a list of intruded objects
	trigger.dont_check_for_unmoveables = IVP_TRUE;
    trigger.exit_policy_extra_radius = 0.1f; // relatively strict exit check [m]

	m_pObject->convert_to_phantom( &trigger );

	// hook up events
	IVP_Environment *pEnv = m_pObject->get_environment();
	CPhysicsEnvironment *pVEnv = (CPhysicsEnvironment *)pEnv->client_data;
	pVEnv->PhantomAdd( this );
}


void CPhysicsObject::RemoveTrigger()
{
    IVP_Controller_Phantom *pController = m_pObject->get_controller_phantom();
	
	// NOTE: This will remove the back-link in the object
	delete pController;
}


bool CPhysicsObject::IsTrigger()
{
	return m_pObject->get_controller_phantom() != NULL ? true : false;
}

bool CPhysicsObject::IsFluid()
{
    IVP_Controller_Phantom *pController = m_pObject->get_controller_phantom();
	if ( pController )
	{
		// UNDONE: Make a base class for triggers?  IPhysicsTrigger?
		//	and derive fluids and any other triggers from that class
		//	then you can ask that class what to do here.
		if ( pController->client_data )
			return true;
	}

	return false;
}

static void InitObjectTemplate( IVP_Template_Real_Object &objectTemplate, int materialIndex, objectparams_t *pParams, bool isStatic )
{
	objectTemplate.mass = pParams->mass;
	
	if ( materialIndex >= 0 )
	{
		objectTemplate.material = physprops->GetIVPMaterial( materialIndex );
	}
	else
	{
		materialIndex = physprops->GetSurfaceIndex( "default" );
		objectTemplate.material = physprops->GetIVPMaterial( materialIndex );
	}

	// HACKHACK: Do something with this name?
	BEGIN_IVP_ALLOCATION();
	objectTemplate.set_name(pParams->pName);
	END_IVP_ALLOCATION();

	objectTemplate.set_nocoll_group_ident( NULL );

	objectTemplate.physical_unmoveable = isStatic ? IVP_TRUE : IVP_FALSE;
	objectTemplate.rot_inertia_is_factor = IVP_TRUE;

	float inertia = pParams->inertia;
	
	// don't allow <=0 inertia!!!!
	if ( inertia <= 0 )
		inertia = 1.0;

	objectTemplate.rot_inertia.set(inertia, inertia, inertia);
	objectTemplate.rot_speed_damp_factor.set(pParams->rotdamping, pParams->rotdamping, pParams->rotdamping);
	objectTemplate.speed_damp_factor = pParams->damping;
	objectTemplate.auto_check_rot_inertia = pParams->rotInertiaLimit;
}

CPhysicsObject *CreatePhysicsObject( CPhysicsEnvironment *pEnvironment, const CPhysCollide *pCollisionModel, int materialIndex, const Vector &position, const QAngle& angles, objectparams_t *pParams, bool isStatic )
{
	IVP_Template_Real_Object objectTemplate;
	IVP_U_Quat rotation;
	IVP_U_Point pos;

	ConvertRotationToIVP( angles, rotation );
	ConvertPositionToIVP( position, pos );

	InitObjectTemplate( objectTemplate, materialIndex, pParams, isStatic );

    IVP_U_Matrix massCenterMatrix;
    massCenterMatrix.init();
	if ( pParams->massCenterOverride )
	{
		IVP_U_Point center;
		ConvertPositionToIVP( *pParams->massCenterOverride, center );
		massCenterMatrix.shift_os( &center );
		objectTemplate.mass_center_override = &massCenterMatrix;
	}

	CPhysicsObject *pObject = new CPhysicsObject();
	IVP_SurfaceManager *pSurman = CreateSurfaceManager( pCollisionModel, pObject->m_collideType );

	BEGIN_IVP_ALLOCATION();

	IVP_Polygon *realObject = pEnvironment->GetIVPEnvironment()->create_polygon(pSurman, &objectTemplate, &rotation, &pos);

	pObject->Init( realObject, materialIndex, pParams->volume, pParams->dragCoefficient, pParams->dragCoefficient, pParams->massCenterOverride );
	pObject->SetGameData( pParams->pGameData );

	if ( pParams->enableCollisions )
	{
		pObject->EnableCollisions( true );
	}
	if ( !isStatic && pParams->dragCoefficient != 0.0f )
	{
		pObject->EnableDrag( true );
	}
	pObject->SetRollingDrag( pParams->rollingDrag );

	END_IVP_ALLOCATION();

	return pObject;
}

CPhysicsObject *CreatePhysicsSphere( CPhysicsEnvironment *pEnvironment, float radius, int materialIndex, const Vector &position, const QAngle &angles, objectparams_t *pParams, bool isStatic )
{
	IVP_U_Quat rotation;
	IVP_U_Point pos;

	ConvertRotationToIVP( angles, rotation );
	ConvertPositionToIVP( position, pos );

	IVP_Template_Real_Object objectTemplate;
	InitObjectTemplate( objectTemplate, materialIndex, pParams, isStatic );

	IVP_Template_Ball ballTemplate;
	ballTemplate.radius = ConvertDistanceToIVP( radius );
	IVP_Ball *realObject = pEnvironment->GetIVPEnvironment()->create_ball( &ballTemplate, &objectTemplate, &rotation, &pos );

	float volume = pParams->volume;
	if ( volume <= 0 )
	{
		volume = 4.0f * radius * radius * radius * M_PI / 3.0f;
	}
	CPhysicsObject *pObject = new CPhysicsObject();
	pObject->Init( realObject, materialIndex, volume, 0, 0, NULL ); //, pParams->dragCoefficient, pParams->dragCoefficient
	pObject->SetGameData( pParams->pGameData );

	if ( pParams->enableCollisions )
	{
		pObject->EnableCollisions( true );
	}
	// drag is not supported on spheres
	//pObject->EnableDrag( false );
	// no rolling drag by default
	pObject->SetRollingDrag( 0 );

	return pObject;
}

class CMaterialIndexOps : public CDefSaveRestoreOps
{
public:
	// save data type interface
	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave ) 
	{
		int materialIndex = *((int *)fieldInfo.pField);
		const char *pMaterialName = physprops->GetPropName( materialIndex );
		if ( !pMaterialName )
		{
			pMaterialName = physprops->GetPropName( 0 );
		}
		int len = strlen(pMaterialName) + 1;
		pSave->WriteInt( &len );
		pSave->WriteString( pMaterialName );
	}

	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore ) 
	{
		char nameBuf[1024];
		int nameLen = pRestore->ReadInt();
		pRestore->ReadString( nameBuf, sizeof(nameBuf), nameLen );
		int *pMaterialIndex = (int *)fieldInfo.pField;
		*pMaterialIndex = physprops->GetSurfaceIndex( nameBuf );
		if ( *pMaterialIndex < 0 )
		{
			*pMaterialIndex = 0;
		}
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo ) 
	{ 
		int *pMaterialIndex = (int *)fieldInfo.pField;
		return (*pMaterialIndex == 0);
	}

	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo ) 
	{
		int *pMaterialIndex = (int *)fieldInfo.pField;
		*pMaterialIndex = 0;
	}
};

static CMaterialIndexOps g_MaterialIndexDataOps;

ISaveRestoreOps* MaterialIndexDataOps()
{
	return &g_MaterialIndexDataOps;
}

struct vphysics_save_cphysicsobject_t
{
	const CPhysCollide *pCollide;
	const char *pName;
	float	sphereRadius;

	bool	isStatic;
	bool	collisionEnabled;
	bool	gravityEnabled;
	bool	dragEnabled;
	bool	motionEnabled;
	bool	isAsleep;
	bool	isTrigger;
	short	collideType;
	int		materialIndex;
	float	mass;
	Vector	rotInertia;
	float	speedDamping;
	float	rotSpeedDamping;
	Vector	massCenterOverride;
    
	unsigned int	callbacks;
	unsigned int	gameFlags;

	unsigned int	contentsMask;
	
	float			volume;
	float			dragCoefficient;
	float			angDragCoefficient;
	bool			hasShadowController;
	IPhysicsShadowController	*pShadow;
	//bool			m_shadowTempGravityDisable;

	Vector			origin;
	QAngle			angles;
	Vector			velocity;
	AngularImpulse	angVelocity;

	DECLARE_SIMPLE_DATADESC();
};


BEGIN_SIMPLE_DATADESC( vphysics_save_cphysicsobject_t )
//	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		pCollide,			FIELD_???	),	// don't save this
//	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		pName,				FIELD_???	),	// don't save this
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		sphereRadius,		FIELD_FLOAT	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		isStatic,			FIELD_BOOLEAN	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		collisionEnabled,	FIELD_BOOLEAN	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		gravityEnabled,		FIELD_BOOLEAN	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		dragEnabled,		FIELD_BOOLEAN	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		motionEnabled,		FIELD_BOOLEAN	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		isAsleep,			FIELD_BOOLEAN	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		isTrigger,			FIELD_BOOLEAN	),
	DEFINE_CUSTOM_FIELD( vphysics_save_cphysicsobject_t, materialIndex, &g_MaterialIndexDataOps ),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		mass,				FIELD_FLOAT	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		rotInertia,			FIELD_VECTOR ),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		speedDamping,		FIELD_FLOAT	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		rotSpeedDamping,	FIELD_FLOAT	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		massCenterOverride,	FIELD_VECTOR ),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		callbacks,			FIELD_INTEGER	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		gameFlags,			FIELD_INTEGER	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		contentsMask,		FIELD_INTEGER	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		volume,				FIELD_FLOAT	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		dragCoefficient,	FIELD_FLOAT	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		angDragCoefficient,	FIELD_FLOAT	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		hasShadowController,FIELD_BOOLEAN	),
	//DEFINE_VPHYSPTR( vphysics_save_cphysicsobject_t, pShadow ),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		origin,				FIELD_POSITION_VECTOR	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		angles,				FIELD_VECTOR	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		velocity,			FIELD_VECTOR	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		angVelocity,		FIELD_VECTOR	),
	DEFINE_FIELD( vphysics_save_cphysicsobject_t,		collideType,		FIELD_SHORT	),
END_DATADESC()

bool CPhysicsObject::IsCollisionEnabled()
{
	return GetObject()->is_collision_detection_enabled() ? true : false;
}

void CPhysicsObject::WriteToTemplate( vphysics_save_cphysicsobject_t &objectTemplate )
{
	if ( m_collideType == COLLIDE_BALL )
	{
		objectTemplate.pCollide = NULL;
		objectTemplate.sphereRadius = GetSphereRadius();
	}
	else
	{
		objectTemplate.pCollide = GetCollide();
		objectTemplate.sphereRadius = 0;
	}
	objectTemplate.isStatic = IsStatic();
	objectTemplate.collisionEnabled = IsCollisionEnabled();
	objectTemplate.gravityEnabled = IsGravityEnabled();
	objectTemplate.dragEnabled = IsDragEnabled();
	objectTemplate.motionEnabled = IsMotionEnabled();
	objectTemplate.isAsleep = IsAsleep();
	objectTemplate.isTrigger = IsTrigger();
	objectTemplate.materialIndex = m_materialIndex;
	objectTemplate.mass = GetMass();

	objectTemplate.rotInertia = GetInertia();
	GetDamping( &objectTemplate.speedDamping, &objectTemplate.rotSpeedDamping );
	objectTemplate.massCenterOverride = m_massCenterOverride;
    
	objectTemplate.callbacks = m_callbacks;
	objectTemplate.gameFlags = m_gameFlags;
	objectTemplate.volume = GetVolume();
	objectTemplate.dragCoefficient = m_dragCoefficient;
	objectTemplate.angDragCoefficient = m_angDragCoefficient;
	objectTemplate.pShadow = m_pShadow;
	objectTemplate.hasShadowController = (m_pShadow != NULL) ? true : false;
	//bool			m_shadowTempGravityDisable;
	objectTemplate.collideType = m_collideType;
	objectTemplate.contentsMask = m_contentsMask;
	GetPosition( &objectTemplate.origin, &objectTemplate.angles );
	GetVelocity( &objectTemplate.velocity, &objectTemplate.angVelocity );
}

void CPhysicsObject::InitFromTemplate( CPhysicsEnvironment *pEnvironment, void *pGameData, const vphysics_save_cphysicsobject_t &objectTemplate )
{
	m_collideType = objectTemplate.collideType;

	IVP_Template_Real_Object ivpObjectTemplate;
	IVP_U_Quat rotation;
	IVP_U_Point pos;

	ConvertRotationToIVP( objectTemplate.angles, rotation );
	ConvertPositionToIVP( objectTemplate.origin, pos );

	ivpObjectTemplate.mass = objectTemplate.mass;
	
	if ( objectTemplate.materialIndex >= 0 )
	{
		ivpObjectTemplate.material = physprops->GetIVPMaterial( objectTemplate.materialIndex );
	}
	else
	{
		ivpObjectTemplate.material = physprops->GetIVPMaterial( physprops->GetSurfaceIndex( "default" ) );
	}

	Assert( ivpObjectTemplate.material );
	// HACKHACK: Pass this name in for debug
	ivpObjectTemplate.set_name(objectTemplate.pName);
	ivpObjectTemplate.set_nocoll_group_ident( NULL );

	ivpObjectTemplate.physical_unmoveable = objectTemplate.isStatic ? IVP_TRUE : IVP_FALSE;
	ivpObjectTemplate.rot_inertia_is_factor = IVP_TRUE;

	ivpObjectTemplate.rot_inertia.set( 1,1,1 );
	ivpObjectTemplate.rot_speed_damp_factor.set( objectTemplate.rotSpeedDamping, objectTemplate.rotSpeedDamping, objectTemplate.rotSpeedDamping );
	ivpObjectTemplate.speed_damp_factor = objectTemplate.speedDamping;

    IVP_U_Matrix massCenterMatrix;
    massCenterMatrix.init();
	const Vector *massOverride = 0;
	if ( objectTemplate.massCenterOverride != vec3_origin )
	{
		IVP_U_Point center;
		ConvertPositionToIVP( objectTemplate.massCenterOverride, center );
		massCenterMatrix.shift_os( &center );
		ivpObjectTemplate.mass_center_override = &massCenterMatrix;
		massOverride = &objectTemplate.massCenterOverride;
	}

	IVP_Real_Object *realObject = NULL;
	if ( m_collideType == COLLIDE_BALL )
	{
		IVP_Template_Ball ballTemplate;
		ballTemplate.radius = ConvertDistanceToIVP( objectTemplate.sphereRadius );

		realObject = pEnvironment->GetIVPEnvironment()->create_ball( &ballTemplate, &ivpObjectTemplate, &rotation, &pos );
	}
	else
	{
		const IVP_Compact_Surface *pSurface = (const IVP_Compact_Surface *)objectTemplate.pCollide;
		IVP_SurfaceManager_Polygon *surman = new IVP_SurfaceManager_Polygon( pSurface );
		realObject = pEnvironment->GetIVPEnvironment()->create_polygon(surman, &ivpObjectTemplate, &rotation, &pos);
	}


	Init( realObject, objectTemplate.materialIndex, objectTemplate.volume, objectTemplate.dragCoefficient, objectTemplate.dragCoefficient, massOverride );

	SetInertia( objectTemplate.rotInertia );
	
	// will wake up the object
	if ( objectTemplate.velocity.LengthSqr() != 0 || objectTemplate.angVelocity.LengthSqr() != 0 )
	{
		Assert( !objectTemplate.isAsleep );
		SetVelocity( &objectTemplate.velocity, &objectTemplate.angVelocity );
	}

	SetCallbackFlags( (unsigned short) objectTemplate.callbacks );
	SetGameFlags( (unsigned short) objectTemplate.gameFlags );

	SetGameData( pGameData );
	SetContents( objectTemplate.contentsMask );

	if ( objectTemplate.dragEnabled )
	{
		Assert( !objectTemplate.isStatic );
		EnableDrag( true );
	}

	if ( !objectTemplate.motionEnabled )
	{
		Assert( !objectTemplate.isStatic );
		EnableMotion( false );
	}

	if ( objectTemplate.isTrigger )
	{
		BecomeTrigger();
	}

	if ( !objectTemplate.gravityEnabled )
	{
		EnableGravity( false );
	}

	if ( objectTemplate.collisionEnabled )
	{
		EnableCollisions( true );
	}

	if ( !objectTemplate.isAsleep )
	{
		Assert( !objectTemplate.isStatic );
		Wake();
	}
	m_pShadow = NULL;
}


bool SavePhysicsObject( const physsaveparams_t &params, CPhysicsObject *pObject )
{
	vphysics_save_cphysicsobject_t objectTemplate;
	memset( &objectTemplate, 0, sizeof(objectTemplate) );

	pObject->WriteToTemplate( objectTemplate );
	params.pSave->WriteAll( &objectTemplate );

	if ( objectTemplate.hasShadowController )
	{
		return SavePhysicsShadowController( params, objectTemplate.pShadow );
	}
	return true;
}

bool RestorePhysicsObject( const physrestoreparams_t &params, CPhysicsObject **ppObject )
{
	vphysics_save_cphysicsobject_t objectTemplate;
	memset( &objectTemplate, 0, sizeof(objectTemplate) );
	params.pRestore->ReadAll( &objectTemplate );
	objectTemplate.pCollide = params.pCollisionModel;
	objectTemplate.pName = params.pName;
	*ppObject = new CPhysicsObject();
	(*ppObject)->InitFromTemplate( static_cast<CPhysicsEnvironment *>(params.pEnvironment), params.pGameData, objectTemplate );

	if ( objectTemplate.hasShadowController )
	{
		bool restored = RestorePhysicsShadowControllerInternal( params, &objectTemplate.pShadow, *ppObject );
		(*ppObject)->RestoreShadowController( objectTemplate.pShadow );
		return restored;
	}

	return true;
}
