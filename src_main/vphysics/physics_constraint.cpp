#include <stdio.h>
#include "vphysics_interface.h"
#include "physics_object.h"
#include "convert.h"
#include "mathlib.h"
#include "vcollide_parse.h"

#include "ivp_physics.hxx"
#include "ivp_listener_object.hxx"
#include "tier0/dbg.h"
#include "vphysics/constraints.h"
#include "isaverestore.h"
#include "physics_environment.h"
#include "vphysics_saverestore.h"
#include "utlvector.h"

// HACKHACK: Mathlib defines this too!
#undef clamp
#undef max
#undef min

// There's some constructor problems in the hk stuff...
// The classes inherit from other classes with private constructor
#pragma warning (disable : 4510 )
#pragma warning (disable : 4610 )

// new havana constraint class
#include "hk_physics/physics.h"
#include "hk_physics/constraint/constraint.h"

#include "hk_physics/constraint/breakable_constraint/breakable_constraint_bp.h"
#include "hk_physics/constraint/breakable_constraint/breakable_constraint.h"
#include "hk_physics/constraint/limited_ball_socket/limited_ball_socket_bp.h"
#include "hk_physics/constraint/limited_ball_socket/limited_ball_socket_constraint.h"
#include "hk_physics/constraint/fixed/fixed_bp.h"
#include "hk_physics/constraint/fixed/fixed_constraint.h"
#include "hk_physics/constraint/stiff_spring/stiff_spring_bp.h"
#include "hk_physics/constraint/stiff_spring/stiff_spring_constraint.h"

#include "hk_physics/constraint/ball_socket/ball_socket_bp.h"
#include "hk_physics/constraint/ball_socket/ball_socket_constraint.h"

#include "hk_physics/constraint/prismatic/prismatic_bp.h"
#include "hk_physics/constraint/prismatic/prismatic_constraint.h"

#include "hk_physics/constraint/ragdoll/ragdoll_constraint.h"
#include "hk_physics/constraint/ragdoll/ragdoll_constraint_bp.h"
#include "hk_physics/constraint/ragdoll/ragdoll_constraint_bp_builder.h"

#include "hk_physics/constraint/hinge/hinge_constraint.h"
#include "hk_physics/constraint/hinge/hinge_bp.h"
#include "hk_physics/constraint/hinge/hinge_bp_builder.h"

#include "hk_physics/constraint/pulley/pulley_constraint.h"
#include "hk_physics/constraint/pulley/pulley_bp.h"

#include "hk_physics/constraint/local_constraint_system/local_constraint_system.h"
#include "hk_physics/constraint/local_constraint_system/local_constraint_system_bp.h"

#include "ivp_cache_object.hxx"
#include "ivp_template_constraint.hxx"
#include "ivp_core.hxx"
extern void qh_srand( int seed);
#include "qhull_user.hxx"


hk_Vector3 TransformHLWorldToHavanaLocal( const Vector &hlWorld, IVP_Real_Object *pObject )
{
	IVP_U_Float_Point tmp;
	IVP_U_Point pointOut;
	ConvertPositionToIVP( hlWorld, tmp );

	TransformIVPToLocal( tmp, pointOut, pObject, true );
	return hk_Vector3( pointOut.k[0], pointOut.k[1], pointOut.k[2] );
}

Vector TransformHavanaLocalToHLWorld( const hk_Vector3 &input, IVP_Real_Object *pObject, bool translate )
{
	IVP_U_Float_Point ivpLocal( input.x, input.y, input.z );
	IVP_U_Float_Point ivpWorld;

	TransformLocalToIVP( ivpLocal, ivpWorld, pObject, translate );

	Vector hlOut;
	if ( translate )
	{
		ConvertPositionToHL( ivpWorld, hlOut );
	}
	else
	{
		ConvertDirectionToHL( ivpWorld, hlOut );
	}
	return hlOut;
}

inline hk_Vector3 vec( const IVP_U_Point &point )
{
	hk_Vector3 tmp(point.k[0], point.k[1], point.k[2] );
	return tmp;
}

// UNDONE: if vector were aligned we could simply cast these.
inline hk_Vector3 vec( const Vector &point )
{
	hk_Vector3 tmp(point.x, point.y, point.z );
	return tmp;
}

void ConvertHLLocalMatrixToHavanaLocal( const matrix3x4_t& hlMatrix, hk_Transform &out )
{
	IVP_U_Matrix ivpMatrix;
	ConvertMatrixToIVP( hlMatrix, ivpMatrix );
	ivpMatrix.get_4x4_column_major( (hk_real *)&out );
}

void set_4x4_column_major( IVP_U_Matrix &ivpMatrix, float *in4x4 )
{
	ivpMatrix.set_elem( 0, 0, in4x4[0] );
	ivpMatrix.set_elem( 1, 0, in4x4[1] );
	ivpMatrix.set_elem( 2, 0, in4x4[2] );

	ivpMatrix.set_elem( 0, 1, in4x4[4] );
	ivpMatrix.set_elem( 1, 1, in4x4[5] );
	ivpMatrix.set_elem( 2, 1, in4x4[6] );

	ivpMatrix.set_elem( 0, 2, in4x4[8] );
	ivpMatrix.set_elem( 1, 2, in4x4[9] );
	ivpMatrix.set_elem( 2, 2, in4x4[10] );

	ivpMatrix.vv.k[0] = in4x4[12];
	ivpMatrix.vv.k[1] = in4x4[13];
	ivpMatrix.vv.k[2] = in4x4[14];
}

inline void ConvertPositionToIVP( const Vector &point, hk_Vector3 &out )
{
	IVP_U_Float_Point ivp;
	ConvertPositionToIVP( point, ivp );
	out = vec( ivp );
}

inline void ConvertPositionToHL( const hk_Vector3 &point, Vector& out )
{
	float tmpY = IVP2HL(point.z);
	out.z = -IVP2HL(point.y);
	out.y = tmpY;
	out.x = IVP2HL(point.x);
}

inline void ConvertDirectionToHL( const hk_Vector3 &point, Vector& out )
{
	float tmpY = point.z;
	out.z = -point.y;
	out.y = tmpY;
	out.x = point.x;
}

void ConvertHavanaLocalMatrixToHL( hk_Transform &in, matrix3x4_t& hlMatrix, IVP_Real_Object *pObject )
{
	IVP_U_Matrix ivpMatrix;
	set_4x4_column_major( ivpMatrix, (hk_real *)&in );
	ConvertMatrixToHL( ivpMatrix, hlMatrix );
}

struct vphysics_save_cphysicsconstraintgroup_t
{
	bool isActive;
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_cphysicsconstraintgroup_t )
	DEFINE_FIELD( vphysics_save_cphysicsconstraintgroup_t,	isActive,	FIELD_BOOLEAN ),
END_DATADESC()

// a little list that holds active groups so they can activate after 
// the constraints are restored and added to the groups
static CUtlVector<CPhysicsConstraintGroup *> g_ConstraintGroupActivateList;

class CPhysicsConstraintGroup: public IPhysicsConstraintGroup 
{
public:
	hk_Local_Constraint_System *GetLCS() { return m_pLCS; }

	void WriteToTemplate( vphysics_save_cphysicsconstraintgroup_t &groupParams )
	{
		groupParams.isActive = m_pLCS->is_active();
	}

public:
	CPhysicsConstraintGroup( IVP_Environment *pEnvironment );
	~CPhysicsConstraintGroup();
	virtual void Activate();
private:
	hk_Local_Constraint_System *m_pLCS;
};

void CPhysicsConstraintGroup::Activate()
{
	if (m_pLCS)
	{
		m_pLCS->activate();
	}
}


CPhysicsConstraintGroup::~CPhysicsConstraintGroup()
{
	delete m_pLCS;
}


CPhysicsConstraintGroup::CPhysicsConstraintGroup( IVP_Environment *pEnvironment )
{
	hk_Local_Constraint_System_BP cs_bp;
	m_pLCS = new hk_Local_Constraint_System( static_cast<hk_Environment *>(pEnvironment), &cs_bp );
	m_pLCS->set_client_data( (void *)this );
}

enum vphysics_save_constrainttypes_t
{
	CONSTRAINT_UNKNOWN = 0,
	CONSTRAINT_RAGDOLL,
	CONSTRAINT_HINGE,
	CONSTRAINT_FIXED,
	CONSTRAINT_BALLSOCKET,
	CONSTRAINT_SLIDING,
	CONSTRAINT_PULLEY,
	CONSTRAINT_LENGTH,
};

struct vphysics_save_cphysicsconstraint_t
{
	int constraintType;
	CPhysicsConstraintGroup *pGroup;
	CPhysicsObject *pObjReference;
	CPhysicsObject *pObjAttached;
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_cphysicsconstraint_t )
	DEFINE_FIELD( vphysics_save_cphysicsconstraint_t,		constraintType,		FIELD_INTEGER ),
	DEFINE_VPHYSPTR( vphysics_save_cphysicsconstraint_t, pGroup ),
	DEFINE_VPHYSPTR( vphysics_save_cphysicsconstraint_t, pObjReference ),
	DEFINE_VPHYSPTR( vphysics_save_cphysicsconstraint_t, pObjAttached ),
END_DATADESC()

struct vphysics_save_constraintbreakable_t : public constraint_breakableparams_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_constraintbreakable_t )
	DEFINE_FIELD( vphysics_save_constraintbreakable_t,		strength,		FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintbreakable_t,		forceLimit,		FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintbreakable_t,		torqueLimit,		FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintbreakable_t,		isActive,		FIELD_BOOLEAN ),
END_DATADESC()

struct vphysics_save_constraintaxislimit_t : public constraint_axislimit_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_constraintaxislimit_t )
	DEFINE_FIELD( vphysics_save_constraintaxislimit_t,		minRotation,		FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintaxislimit_t,		maxRotation,		FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintaxislimit_t,		angularVelocity,		FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintaxislimit_t,		torque,		FIELD_FLOAT ),
END_DATADESC()

struct vphysics_save_constraintfixed_t : public constraint_fixedparams_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_constraintfixed_t )
	DEFINE_AUTO_ARRAY2D( vphysics_save_constraintfixed_t, attachedRefXform, FIELD_FLOAT ),
	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_constraintfixed_t, constraint, vphysics_save_constraintbreakable_t ),
END_DATADESC()

struct vphysics_save_constrainthinge_t : public constraint_hingeparams_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_constrainthinge_t )
	DEFINE_FIELD( vphysics_save_constrainthinge_t, worldPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( vphysics_save_constrainthinge_t, worldAxisDirection, FIELD_VECTOR ),
	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_constrainthinge_t, constraint, vphysics_save_constraintbreakable_t ),
	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_constrainthinge_t, hingeAxis, vphysics_save_constraintaxislimit_t ),
END_DATADESC()

struct vphysics_save_constraintsliding_t : public constraint_slidingparams_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_constraintsliding_t )
	DEFINE_AUTO_ARRAY2D( vphysics_save_constraintsliding_t, attachedRefXform, FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintsliding_t, slideAxisRef, FIELD_VECTOR ),
	DEFINE_FIELD( vphysics_save_constraintsliding_t, limitMin, FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintsliding_t, limitMax, FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintsliding_t, friction, FIELD_FLOAT ),
	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_constraintsliding_t, constraint, vphysics_save_constraintbreakable_t ),
END_DATADESC()

struct vphysics_save_constraintpulley_t : public constraint_pulleyparams_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_constraintpulley_t )
	DEFINE_AUTO_ARRAY( vphysics_save_constraintpulley_t, pulleyPosition, FIELD_POSITION_VECTOR ),
	DEFINE_AUTO_ARRAY( vphysics_save_constraintpulley_t, objectPosition, FIELD_VECTOR ),
	DEFINE_FIELD( vphysics_save_constraintpulley_t, totalLength, FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintpulley_t, gearRatio, FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintpulley_t, isRigid, FIELD_BOOLEAN ),
	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_constraintpulley_t, constraint, vphysics_save_constraintbreakable_t ),
END_DATADESC()

struct vphysics_save_constraintlength_t : public constraint_lengthparams_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_constraintlength_t )
	DEFINE_AUTO_ARRAY( vphysics_save_constraintlength_t, objectPosition, FIELD_VECTOR ),
	DEFINE_FIELD( vphysics_save_constraintlength_t, totalLength, FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_constraintlength_t, isRigid, FIELD_BOOLEAN ),
	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_constraintlength_t, constraint, vphysics_save_constraintbreakable_t ),
END_DATADESC()

struct vphysics_save_constraintballsocket_t : public constraint_ballsocketparams_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_constraintballsocket_t )
	DEFINE_AUTO_ARRAY( vphysics_save_constraintballsocket_t, constraintPosition, FIELD_VECTOR ),
	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_constraintballsocket_t, constraint, vphysics_save_constraintbreakable_t ),
END_DATADESC()

struct vphysics_save_constraintragdoll_t : public constraint_ragdollparams_t
{
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_constraintragdoll_t )
	DEFINE_AUTO_ARRAY2D( vphysics_save_constraintragdoll_t, constraintToReference, FIELD_FLOAT ),
	DEFINE_AUTO_ARRAY2D( vphysics_save_constraintragdoll_t, constraintToAttached, FIELD_FLOAT ),
	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_constraintragdoll_t, axes[0], vphysics_save_constraintaxislimit_t ),
	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_constraintragdoll_t, axes[1], vphysics_save_constraintaxislimit_t ),
	DEFINE_EMBEDDED_OVERRIDE( vphysics_save_constraintragdoll_t, axes[2], vphysics_save_constraintaxislimit_t ),
END_DATADESC()

struct vphysics_save_constraint_t
{
	vphysics_save_constraintfixed_t fixed;
	vphysics_save_constrainthinge_t hinge;
	vphysics_save_constraintsliding_t sliding;
	vphysics_save_constraintpulley_t pulley;
	vphysics_save_constraintlength_t length;
	vphysics_save_constraintballsocket_t ballsocket;
	vphysics_save_constraintragdoll_t ragdoll;
};

// UNDONE: May need to change interface to specify limits before construction
// UNDONE: Refactor constraints to contain a separate object for the various constraint types?
class CPhysicsConstraint: public IPhysicsConstraint, public IVP_Listener_Object
{
public:
	CPhysicsConstraint( CPhysicsObject *pReferenceObject, CPhysicsObject *pAttachedObject );
	~CPhysicsConstraint( void );

	// init as ragdoll constraint
	void InitRagdoll( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_ragdollparams_t &ragdoll );
	// init as hinge
	void InitHinge( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_hingeparams_t &hinge );
	// init as fixed (BUGBUG: This is broken)
	void InitFixed( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_fixedparams_t &fixed );
	// init as ballsocket
	void InitBallsocket( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_ballsocketparams_t &ballsocket );
	// init as sliding constraint
	void InitSliding( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_slidingparams_t &sliding );
	// init as pulley
	void InitPulley( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_pulleyparams_t &pulley );
	// init as stiff spring / length constraint
	void InitLength( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_lengthparams_t &length );

	void WriteToTemplate( vphysics_save_cphysicsconstraint_t &header, vphysics_save_constraint_t &constraintTemplate );

	void WriteRagdoll( constraint_ragdollparams_t &ragdoll );
	void WriteHinge( constraint_hingeparams_t &hinge );
	void WriteFixed( constraint_fixedparams_t &fixed );
	void WriteSliding( constraint_slidingparams_t &sliding );
	void WriteBallsocket( constraint_ballsocketparams_t &ballsocket );
	void WritePulley( constraint_pulleyparams_t &pulley );
	void WriteLength( constraint_lengthparams_t &length );
	CPhysicsConstraintGroup *GetConstraintGroup();

	hk_Constraint *CreateBreakableConstraint( hk_Constraint *pRealConstraint, hk_Local_Constraint_System *pLcs, const constraint_breakableparams_t &constraint )
	{
		m_isBreakable = true;
		hk_Breakable_Constraint_BP bp;
		bp.m_real_constraint = pRealConstraint;
		float forceLimit = ConvertDistanceToIVP( constraint.forceLimit );
		bp.m_linear_strength = forceLimit > 0 ? forceLimit : 1e12;
		bp.m_angular_strength = constraint.torqueLimit > 0 ? DEG2RAD(constraint.torqueLimit) : 1e12;
		return new hk_Breakable_Constraint( pLcs, &bp );
	}
	void ReadBreakableConstraint( constraint_breakableparams_t &params );

	hk_Constraint *GetRealConstraint()
	{
		if ( m_isBreakable )
		{
			hk_Breakable_Constraint_BP bp;
			((hk_Breakable_Constraint *)m_HkConstraint)->write_to_blueprint( &bp );
			return bp.m_real_constraint;
		}
		return m_HkConstraint;
	}

	void Activate( void );
	void Deactivate( void );
	void SetupRagdollAxis( int axis, const constraint_axislimit_t &axisData, hk_Limited_Ball_Socket_BP *ballsocketBP );
	void SetupRagdollAxisTorque( int axis, const constraint_axislimit_t &axisData, hk_Ragdoll_Constraint_BP *bp );

	// UNDONE: Implement includeStatic for havana

	void SetGameData( void *gameData ) { m_pGameData = gameData; }
	void *GetGameData( void ) { return m_pGameData; }
	IPhysicsObject *GetReferenceObject( void );
	IPhysicsObject *GetAttachedObject( void );

	void DetachListener();
	// Object listener
    virtual void event_object_deleted( IVP_Event_Object *);
    virtual void event_object_created( IVP_Event_Object *) {}
    virtual void event_object_revived( IVP_Event_Object *) {}
    virtual void event_object_frozen ( IVP_Event_Object *) {}

private:
	CPhysicsObject			*m_pObjReference;
	CPhysicsObject			*m_pObjAttached;

	// havana constraints
	hk_Constraint			*m_HkConstraint;
	hk_Local_Constraint_System *m_HkLCS;
	void					*m_pGameData;
	// these are used to crack the abstract pointers on save/load
	short					m_constraintType;
	short					m_isBreakable;
};

CPhysicsConstraint::CPhysicsConstraint( CPhysicsObject *pReferenceObject, CPhysicsObject *pAttachedObject )
{
	m_pGameData = NULL;
	m_HkConstraint = NULL;
	m_HkLCS = NULL;
	m_constraintType = CONSTRAINT_UNKNOWN;
	m_isBreakable = false;

	if ( pReferenceObject && pAttachedObject )
	{
		m_pObjReference = (CPhysicsObject *)pReferenceObject;
		m_pObjAttached = (CPhysicsObject *)pAttachedObject;
		if ( !(m_pObjReference->GetCallbackFlags() & CALLBACK_NEVER_DELETED) )
		{
			m_pObjReference->GetObject()->add_listener_object( this );
		}
		if ( !(m_pObjAttached->GetCallbackFlags() & CALLBACK_NEVER_DELETED) )
		{
			m_pObjAttached->GetObject()->add_listener_object( this );
		}
	}
	else
	{
		m_pObjReference = NULL;
		m_pObjAttached = NULL;
	}
}

// ragdoll constraint
void CPhysicsConstraint::InitRagdoll( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_ragdollparams_t &ragdoll )
{
	m_constraintType = CONSTRAINT_RAGDOLL;

	hk_Rigid_Body *ref = (hk_Rigid_Body*)m_pObjReference->GetObject();
	hk_Rigid_Body *att = (hk_Rigid_Body*)m_pObjAttached->GetObject();

	hk_Limited_Ball_Socket_BP ballsocketBP;
	ConvertHLLocalMatrixToHavanaLocal( ragdoll.constraintToReference, ballsocketBP.m_transform_os_ks[0] );
	ConvertHLLocalMatrixToHavanaLocal( ragdoll.constraintToAttached, ballsocketBP.m_transform_os_ks[1] );

	int i;
	for ( i = 0; i < 3; i++ )
	{
		SetupRagdollAxis( i, ragdoll.axes[i], &ballsocketBP );
	}
	ballsocketBP.m_constrainTranslation = ragdoll.onlyAngularLimits ? false : true;

	hk_Ragdoll_Constraint_BP_Builder r_builder;
	r_builder.initialize_from_limited_ball_socket_bp( &ballsocketBP, ref, att);
	hk_Ragdoll_Constraint_BP *bp = (hk_Ragdoll_Constraint_BP  *)r_builder.get_blueprint();  // get non const bp
	
	for ( i = 0; i < 3; i++ )
	{
		SetupRagdollAxisTorque( i, ragdoll.axes[i], bp );
	}

	hk_Local_Constraint_System *lcs = constraint_group ? constraint_group->GetLCS() : NULL;
	hk_Environment *hkEnvironment = static_cast<hk_Environment *>(pEnvironment);
	if ( !lcs )
	{
		hk_Local_Constraint_System_BP bp;
		lcs = new hk_Local_Constraint_System( hkEnvironment, &bp );
		m_HkLCS = lcs;
	}

	m_HkConstraint = new hk_Ragdoll_Constraint( lcs, bp, ref, att);

	if ( m_HkLCS && ragdoll.isActive )
	{
		m_HkLCS->activate();
	}
}

// hinge constraint
void CPhysicsConstraint::InitHinge( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_hingeparams_t &hinge )
{
	m_constraintType = CONSTRAINT_HINGE;
	hk_Environment *hkEnvironment = static_cast<hk_Environment *>(pEnvironment);

	bool breakable = false;
	if ( hinge.constraint.forceLimit != 0 || hinge.constraint.torqueLimit != 0 )
	{
		// UNDONE: Can't support groups and breaking at the same time!
		// should we assert here?
		constraint_group = NULL;
		breakable = true;
	}

	hk_Hinge_BP_Builder builder;

	IVP_U_Point axisIVP_ws, axisStartIVP_ws, axisStartIVP_os;
	
	ConvertDirectionToIVP( hinge.worldAxisDirection, axisIVP_ws );
	builder.set_axis_ws( (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject(), vec(axisIVP_ws) );

	builder.set_position_os( 0, TransformHLWorldToHavanaLocal( hinge.worldPosition, m_pObjReference->GetObject() ) );
	builder.set_position_os( 1, TransformHLWorldToHavanaLocal( hinge.worldPosition, m_pObjAttached->GetObject() ) );

	builder.set_tau( hinge.constraint.strength );
	// torque is an impulse radians/sec * inertia
	if ( hinge.hingeAxis.torque != 0 )
	{
		builder.set_angular_motor( DEG2RAD(hinge.hingeAxis.angularVelocity), DEG2RAD(hinge.hingeAxis.torque) );
	}
	if ( hinge.hingeAxis.minRotation != hinge.hingeAxis.maxRotation )
	{
		builder.set_angular_limits( DEG2RAD(hinge.hingeAxis.minRotation), DEG2RAD(hinge.hingeAxis.maxRotation) );
	}
	hk_Local_Constraint_System *lcs = constraint_group ? constraint_group->GetLCS() : NULL;
	if ( !lcs )
	{
		hk_Local_Constraint_System_BP bp;
		lcs = new hk_Local_Constraint_System( hkEnvironment, &bp );
		m_HkLCS = lcs;
	}

	if ( breakable )
	{
		hk_Hinge_Constraint *pHinge = new hk_Hinge_Constraint( hkEnvironment, builder.get_blueprint(), (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );
		m_HkConstraint = CreateBreakableConstraint( pHinge, lcs, hinge.constraint );
	}
	else
	{
		m_HkConstraint = new hk_Hinge_Constraint( lcs, builder.get_blueprint(), (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );
	}
	if ( m_HkLCS && hinge.constraint.isActive )
	{
		m_HkLCS->activate();
	}
	m_HkConstraint->set_client_data( (void *)this );
}


// fixed constraint
void CPhysicsConstraint::InitFixed( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_fixedparams_t &fixed )
{
	m_constraintType = CONSTRAINT_FIXED;
	hk_Environment *hkEnvironment = static_cast<hk_Environment *>(pEnvironment);

	bool breakable = false;
	if ( fixed.constraint.forceLimit != 0 || fixed.constraint.torqueLimit != 0 )
	{
		// UNDONE: Can't support groups and breaking at the same time!
		// should we assert here?
		constraint_group = NULL;
		breakable = true;
	}

	hk_Fixed_BP fixed_bp;
	ConvertHLLocalMatrixToHavanaLocal( fixed.attachedRefXform, fixed_bp.m_transform_os_ks );

	fixed_bp.m_tau = fixed.constraint.strength;
	
	hk_Local_Constraint_System *lcs = constraint_group ? constraint_group->GetLCS() : NULL;
	if ( !lcs )
	{
		hk_Local_Constraint_System_BP bp;
		lcs = new hk_Local_Constraint_System( hkEnvironment, &bp );
		m_HkLCS = lcs;
	}

	if ( breakable )
	{
		hk_Constraint *pFixed = new hk_Fixed_Constraint( hkEnvironment, &fixed_bp, (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );

		m_HkConstraint = CreateBreakableConstraint( pFixed, m_HkLCS, fixed.constraint );
	}
	else
	{
		m_HkConstraint = new hk_Fixed_Constraint( lcs, &fixed_bp, (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );
	}

	if ( m_HkLCS && fixed.constraint.isActive )
	{
		m_HkLCS->activate();
	}
	m_HkConstraint->set_client_data( (void *)this );
}

void CPhysicsConstraint::InitBallsocket( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_ballsocketparams_t &ballsocket )
{
	m_constraintType = CONSTRAINT_BALLSOCKET;

	hk_Environment *hkEnvironment = static_cast<hk_Environment *>(pEnvironment);

	bool breakable = false;
	if ( ballsocket.constraint.forceLimit != 0 || ballsocket.constraint.torqueLimit != 0 )
	{
		// UNDONE: Can't support groups and breaking at the same time!
		// should we assert here?
		constraint_group = NULL;
		breakable = true;
	}

	hk_Ball_Socket_BP builder;

	for ( int i = 0; i < 2; i++ )
	{
		hk_Vector3 hkConstraintLocal;
		ConvertPositionToIVP( ballsocket.constraintPosition[i], hkConstraintLocal );
		builder.set_position_os( i, hkConstraintLocal );
	}

	builder.m_strength = ballsocket.constraint.strength;
	hk_Local_Constraint_System *lcs = constraint_group ? constraint_group->GetLCS() : NULL;
	if ( !lcs )
	{
		hk_Local_Constraint_System_BP bp;
		lcs = new hk_Local_Constraint_System( hkEnvironment, &bp );
		m_HkLCS = lcs;
	}

	if ( breakable )
	{
		hk_Ball_Socket_Constraint *pConstraint = new hk_Ball_Socket_Constraint( hkEnvironment, &builder, (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );
		m_HkConstraint = CreateBreakableConstraint( pConstraint, lcs, ballsocket.constraint );
	}
	else
	{
		m_HkConstraint = new hk_Ball_Socket_Constraint( lcs, &builder, (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );
	}

	if ( m_HkLCS && ballsocket.constraint.isActive )
	{
		m_HkLCS->activate();
	}
	m_HkConstraint->set_client_data( (void *)this );
}

void CPhysicsConstraint::InitSliding( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_slidingparams_t &sliding )
{
	m_constraintType = CONSTRAINT_SLIDING;
	hk_Environment *hkEnvironment = static_cast<hk_Environment *>(pEnvironment);

	bool breakable = false;
	if ( sliding.constraint.forceLimit != 0 || sliding.constraint.torqueLimit != 0 )
	{
		// UNDONE: Can't support groups and breaking at the same time!
		// should we assert here?
		constraint_group = NULL;
		breakable = true;
	}

	hk_Prismatic_BP prismatic_bp;
	hk_Transform t;
	ConvertHLLocalMatrixToHavanaLocal( sliding.attachedRefXform, t );
	prismatic_bp.m_transform_Ros_Aos.m_translation = t.get_translation();
	prismatic_bp.m_transform_Ros_Aos.m_rotation.set( t );

	IVP_U_Float_Point refAxisDir;
	ConvertDirectionToIVP( sliding.slideAxisRef, refAxisDir );
	prismatic_bp.m_axis_Ros = vec(refAxisDir);
	prismatic_bp.m_tau = sliding.constraint.strength;
	
	hk_Constraint_Limit_BP bp;

	if ( sliding.limitMin != sliding.limitMax )
	{
		bp.set_limits( ConvertDistanceToIVP(sliding.limitMin), ConvertDistanceToIVP(sliding.limitMax) );
	}
	if ( sliding.friction )
	{
		bp.set_friction( ConvertDistanceToIVP(sliding.friction) );
	}
	prismatic_bp.m_limit.init_limit( bp, 1.0 );

	hk_Local_Constraint_System *lcs = constraint_group ? constraint_group->GetLCS() : NULL;
	if ( !lcs )
	{
		hk_Local_Constraint_System_BP bp;
		lcs = new hk_Local_Constraint_System( hkEnvironment, &bp );
		m_HkLCS = lcs;
	}

	if ( breakable )
	{
		hk_Constraint *pFixed = new hk_Prismatic_Constraint( hkEnvironment, &prismatic_bp, (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );

		m_HkConstraint = CreateBreakableConstraint( pFixed, m_HkLCS, sliding.constraint );
	}
	else
	{
		m_HkConstraint = new hk_Prismatic_Constraint( lcs, &prismatic_bp, (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );
	}

	if ( m_HkLCS && sliding.constraint.isActive )
	{
		m_HkLCS->activate();
	}
	m_HkConstraint->set_client_data( (void *)this );
}

void CPhysicsConstraint::InitPulley( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_pulleyparams_t &pulley )
{
	m_constraintType = CONSTRAINT_PULLEY;

	hk_Environment *hkEnvironment = static_cast<hk_Environment *>(pEnvironment);

	bool breakable = false;
	if ( pulley.constraint.forceLimit != 0 || pulley.constraint.torqueLimit != 0 )
	{
		// UNDONE: Can't support groups and breaking at the same time!
		// should we assert here?
		constraint_group = NULL;
		breakable = true;
	}

	hk_Pulley_BP pulley_bp;
	pulley_bp.m_tau = pulley.constraint.strength;
	//pulley_bp.m_strength = pulley.constraint.strength;
	pulley_bp.m_gearing = pulley.gearRatio;
	pulley_bp.m_is_rigid = pulley.isRigid;

	// Get the current length of rope
	pulley_bp.m_length = ConvertDistanceToIVP( pulley.totalLength );

	// set the anchor positions in object space
	ConvertPositionToIVP( pulley.objectPosition[0], pulley_bp.m_translation_os_ks[0] );
	ConvertPositionToIVP( pulley.objectPosition[1], pulley_bp.m_translation_os_ks[1] );

	// set the pully positions in world space
	ConvertPositionToIVP( pulley.pulleyPosition[0], pulley_bp.m_worldspace_point[0] );
	ConvertPositionToIVP( pulley.pulleyPosition[1], pulley_bp.m_worldspace_point[1] );

	hk_Local_Constraint_System *lcs = constraint_group ? constraint_group->GetLCS() : NULL;
	if ( !lcs )
	{
		hk_Local_Constraint_System_BP bp;
		lcs = new hk_Local_Constraint_System( hkEnvironment, &bp );
		m_HkLCS = lcs;
	}

	if ( breakable )
	{
		hk_Constraint *pPulley = new hk_Pulley_Constraint( hkEnvironment, &pulley_bp, (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );

		m_HkConstraint = CreateBreakableConstraint( pPulley, m_HkLCS, pulley.constraint );
	}
	else
	{
		m_HkConstraint = new hk_Pulley_Constraint( lcs, &pulley_bp, (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );
	}

	if ( m_HkLCS && pulley.constraint.isActive )
	{
		m_HkLCS->activate();
	}
	m_HkConstraint->set_client_data( (void *)this );
}


void CPhysicsConstraint::InitLength( IVP_Environment *pEnvironment, CPhysicsConstraintGroup *constraint_group, const constraint_lengthparams_t &length )
{
	m_constraintType = CONSTRAINT_LENGTH;

	hk_Environment *hkEnvironment = static_cast<hk_Environment *>(pEnvironment);

	bool breakable = false;
	if ( length.constraint.forceLimit != 0 || length.constraint.torqueLimit != 0 )
	{
		// UNDONE: Can't support groups and breaking at the same time!
		// should we assert here?
		constraint_group = NULL;
		breakable = true;
	}

	hk_Stiff_Spring_BP stiff_bp;
	stiff_bp.m_tau = length.constraint.strength;
	//stiff_bp.m_strength = length.constraint.strength;
	stiff_bp.m_is_rigid = length.isRigid;

	// Get the current length of rope
	stiff_bp.m_length = ConvertDistanceToIVP( length.totalLength );

	// set the anchor positions in object space
	ConvertPositionToIVP( length.objectPosition[0], stiff_bp.m_translation_os_ks[0] );
	ConvertPositionToIVP( length.objectPosition[1], stiff_bp.m_translation_os_ks[1] );

	hk_Local_Constraint_System *lcs = constraint_group ? constraint_group->GetLCS() : NULL;
	if ( !lcs )
	{
		hk_Local_Constraint_System_BP bp;
		lcs = new hk_Local_Constraint_System( hkEnvironment, &bp );
		m_HkLCS = lcs;
	}

	if ( breakable )
	{
		hk_Constraint *pLength = new hk_Stiff_Spring_Constraint( hkEnvironment, &stiff_bp, (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );

		m_HkConstraint = CreateBreakableConstraint( pLength, m_HkLCS, length.constraint );
	}
	else
	{
		m_HkConstraint = new hk_Stiff_Spring_Constraint( lcs, &stiff_bp, (hk_Rigid_Body*)m_pObjReference->GetObject(), (hk_Rigid_Body*)m_pObjAttached->GetObject() );
	}

	if ( m_HkLCS && length.constraint.isActive )
	{
		m_HkLCS->activate();
	}
	m_HkConstraint->set_client_data( (void *)this );
}

// Serialization: Write out a description for this constraint
void CPhysicsConstraint::WriteToTemplate( vphysics_save_cphysicsconstraint_t &header, vphysics_save_constraint_t &constraintTemplate )
{
	header.constraintType = m_constraintType;
	
	// this constraint is inert due to one of it's objects getting deleted
	if ( !m_HkConstraint )
		return;

	// BUGBUG: STORE THIS SOMEHOW!!!
	header.pGroup = GetConstraintGroup();

	header.pObjReference = m_pObjReference;
	header.pObjAttached = m_pObjAttached;

	switch( header.constraintType )
	{
	case CONSTRAINT_UNKNOWN:
		Assert(0);
		break;
	case CONSTRAINT_HINGE:
		WriteHinge( constraintTemplate.hinge );
		break;
	case CONSTRAINT_FIXED:
		WriteFixed( constraintTemplate.fixed );
		break;
	case CONSTRAINT_SLIDING:
		WriteSliding( constraintTemplate.sliding );
		break;
	case CONSTRAINT_PULLEY:
		WritePulley( constraintTemplate.pulley );
		break;
	case CONSTRAINT_LENGTH:
		WriteLength( constraintTemplate.length );
		break;
	case CONSTRAINT_BALLSOCKET:
		WriteBallsocket( constraintTemplate.ballsocket );
		break;
	case CONSTRAINT_RAGDOLL:
		WriteRagdoll( constraintTemplate.ragdoll );
		break;
	}
}

CPhysicsConstraintGroup *CPhysicsConstraint::GetConstraintGroup()
{
	if ( !m_HkConstraint )
		return NULL;

	hk_Local_Constraint_System *plcs = m_HkConstraint->get_constraint_system();
	Assert(plcs);
	return (CPhysicsConstraintGroup *)plcs->get_client_data();
}

void CPhysicsConstraint::ReadBreakableConstraint( constraint_breakableparams_t &params )
{
	if ( m_isBreakable )
	{
		hk_Breakable_Constraint_BP bp;
		((hk_Breakable_Constraint *)m_HkConstraint)->write_to_blueprint( &bp );

		params.forceLimit = ConvertDistanceToHL( bp.m_linear_strength );
		params.torqueLimit = RAD2DEG( bp.m_angular_strength );
		params.strength = 1.0;
		Assert( m_HkLCS != NULL );
	}
	else
	{
		params.Defaults();
	}

	if ( m_HkLCS )
	{
		params.isActive = m_HkLCS->is_active();
	}
}


void CPhysicsConstraint::WriteFixed( constraint_fixedparams_t &fixed )
{
	hk_Fixed_Constraint *pConstraint = (hk_Fixed_Constraint *)GetRealConstraint();
	ReadBreakableConstraint( fixed.constraint );

	hk_Fixed_BP fixed_bp;
	pConstraint->write_to_blueprint( &fixed_bp );
	// save fixed bp into params
	ConvertHavanaLocalMatrixToHL( fixed_bp.m_transform_os_ks, fixed.attachedRefXform, m_pObjReference->GetObject() );
}

void CPhysicsConstraint::WriteRagdoll( constraint_ragdollparams_t &ragdoll )
{
	int axisMap[] = {0,2,1};

	hk_Ragdoll_Constraint *pConstraint = (hk_Ragdoll_Constraint *)GetRealConstraint();
	hk_Ragdoll_Constraint_BP ragdoll_bp;
	// BUGBUG: Write this and figure out how to recreate
	// or change init func to setup this bp directly
	pConstraint->write_to_blueprint( &ragdoll_bp );

	ConvertHavanaLocalMatrixToHL( ragdoll_bp.m_transform_os_ks[0], ragdoll.constraintToReference, m_pObjReference->GetObject() );
	ConvertHavanaLocalMatrixToHL( ragdoll_bp.m_transform_os_ks[1], ragdoll.constraintToAttached, m_pObjAttached->GetObject() );
	for ( int i = 0; i < 3; i ++ )
	{
		constraint_axislimit_t &ragdollAxis = ragdoll.axes[i];
		hk_Constraint_Limit_BP &bpAxis = ragdoll_bp.m_limits[ axisMap[i] ];

		ragdollAxis.angularVelocity = RAD2DEG(bpAxis.m_desired_velocity);
		ragdollAxis.torque = bpAxis.m_joint_friction * m_pObjReference->GetInvMass();
		// X&Y
		if ( i != 2 )
		{
			ragdollAxis.minRotation = -RAD2DEG(bpAxis.m_limit_max);
			ragdollAxis.maxRotation = -RAD2DEG(bpAxis.m_limit_min);
		}
		// Z
		else
		{
			ragdollAxis.minRotation = RAD2DEG(bpAxis.m_limit_min);
			ragdollAxis.maxRotation = RAD2DEG(bpAxis.m_limit_max);
		}
	}
	ragdoll.childIndex = -1;
	ragdoll.parentIndex = -1;
}

void CPhysicsConstraint::WriteHinge( constraint_hingeparams_t &hinge )
{
	hk_Hinge_Constraint *pConstraint = (hk_Hinge_Constraint *)GetRealConstraint();
	ReadBreakableConstraint( hinge.constraint );

	hk_Hinge_BP hinge_bp;
	pConstraint->write_to_blueprint( &hinge_bp );
	// save hinge bp into params
	hinge.worldPosition = TransformHavanaLocalToHLWorld( hinge_bp.m_axis_os[0].get_origin(), m_pObjReference->GetObject(), true );
	hinge.worldAxisDirection = TransformHavanaLocalToHLWorld( hinge_bp.m_axis_os[0].get_direction(), m_pObjReference->GetObject(), false );
	hinge.hingeAxis.SetAxisFriction( 0,0,0 );

	if ( hinge_bp.m_limit.m_limit_is_enabled )
	{
		hinge.hingeAxis.minRotation = RAD2DEG(hinge_bp.m_limit.m_limit_min);
		hinge.hingeAxis.maxRotation = RAD2DEG(hinge_bp.m_limit.m_limit_max);
	}
	if ( hinge_bp.m_limit.m_friction_is_enabled )
	{
		hinge.hingeAxis.angularVelocity = RAD2DEG(hinge_bp.m_limit.m_desired_velocity);
		hinge.hingeAxis.torque = RAD2DEG(hinge_bp.m_limit.m_joint_friction);
	}
}

void CPhysicsConstraint::WriteSliding( constraint_slidingparams_t &sliding )
{
	sliding.Defaults();
	hk_Prismatic_Constraint *pConstraint = (hk_Prismatic_Constraint *)GetRealConstraint();
	ReadBreakableConstraint( sliding.constraint );

	hk_Prismatic_BP prismatic_bp;
	pConstraint->write_to_blueprint( &prismatic_bp );
	// save bp into params

	hk_Transform t;
	t.set_translation( prismatic_bp.m_transform_Ros_Aos.m_translation );
	t.set_rotation( prismatic_bp.m_transform_Ros_Aos.m_rotation );
	ConvertHavanaLocalMatrixToHL( t, sliding.attachedRefXform, m_pObjReference->GetObject() );
	if ( prismatic_bp.m_limit.m_friction_is_enabled )
	{
		sliding.friction = ConvertDistanceToHL( prismatic_bp.m_limit.m_joint_friction );
	}
	if ( prismatic_bp.m_limit.m_limit_is_enabled )
	{
		sliding.limitMin = ConvertDistanceToHL( prismatic_bp.m_limit.m_limit_min );
		sliding.limitMax = ConvertDistanceToHL( prismatic_bp.m_limit.m_limit_max );
	}
	ConvertDirectionToHL( prismatic_bp.m_axis_Ros, sliding.slideAxisRef );
}


void CPhysicsConstraint::WritePulley( constraint_pulleyparams_t &pulley )
{
	pulley.Defaults();
	hk_Pulley_Constraint *pConstraint = (hk_Pulley_Constraint *)GetRealConstraint();
	ReadBreakableConstraint( pulley.constraint );

	hk_Pulley_BP pulley_bp;
	pConstraint->write_to_blueprint( &pulley_bp );

	// save bp into params
	for ( int i = 0; i < 2; i++ )
	{
		ConvertPositionToHL( pulley_bp.m_worldspace_point[i], pulley.pulleyPosition[i] );
		ConvertPositionToHL( pulley_bp.m_translation_os_ks[i], pulley.objectPosition[i] );
	}
	pulley.gearRatio = pulley_bp.m_gearing;

	pulley.totalLength = ConvertDistanceToHL(pulley_bp.m_length);
	pulley.isRigid = pulley_bp.m_is_rigid;
}


void CPhysicsConstraint::WriteLength( constraint_lengthparams_t &length )
{
	length.Defaults();
	hk_Stiff_Spring_Constraint *pConstraint = (hk_Stiff_Spring_Constraint *)GetRealConstraint();
	ReadBreakableConstraint( length.constraint );

	hk_Stiff_Spring_BP stiff_bp;
	pConstraint->write_to_blueprint( &stiff_bp );

	// save bp into params
	for ( int i = 0; i < 2; i++ )
	{
		ConvertPositionToHL( stiff_bp.m_translation_os_ks[i], length.objectPosition[i] );
	}

	length.totalLength = ConvertDistanceToHL(stiff_bp.m_length);
	length.isRigid = stiff_bp.m_is_rigid;
}


void CPhysicsConstraint::WriteBallsocket( constraint_ballsocketparams_t &ballsocket )
{
	ballsocket.Defaults();
	hk_Ball_Socket_Constraint *pConstraint = (hk_Ball_Socket_Constraint *)GetRealConstraint();
	ReadBreakableConstraint( ballsocket.constraint );

	hk_Ball_Socket_BP ballsocket_bp;
	pConstraint->write_to_blueprint( &ballsocket_bp );

	// save bp into params
	for ( int i = 0; i < 2; i++ )
	{
		ConvertPositionToHL( ballsocket_bp.m_translation_os_ks[i], ballsocket.constraintPosition[i] );
	}
}


void CPhysicsConstraint::DetachListener()
{
	if ( !(m_pObjReference->GetCallbackFlags() & CALLBACK_NEVER_DELETED) )
	{
		m_pObjReference->GetObject()->remove_listener_object( this );
	}

	if ( !(m_pObjAttached->GetCallbackFlags() & CALLBACK_NEVER_DELETED) )
	{
		m_pObjAttached->GetObject()->remove_listener_object( this );
	}

	m_pObjReference = NULL;
	m_pObjAttached = NULL;
}


void CPhysicsConstraint::event_object_deleted( IVP_Event_Object * )
{
	// the constraint (or local constraint system) is going to delete itself now, so NULL it.
	DetachListener();
	m_HkConstraint = NULL;
	m_HkLCS = NULL;
}


CPhysicsConstraint::~CPhysicsConstraint( void )
{
	// arg.  There should be a better way to do this
	if ( m_HkConstraint || m_HkLCS )
	{
		DetachListener();
		if ( m_HkLCS )
		{
			delete m_HkLCS;
		}
		else
		{
			delete m_HkConstraint;
		}
	}
}

void CPhysicsConstraint::Activate( void )
{
	if ( m_HkLCS )
	{
		m_HkLCS->activate();
	}
}


void CPhysicsConstraint::Deactivate( void )
{
	if ( m_HkLCS )
	{
		m_HkLCS->deactivate();
	}
}


void CPhysicsConstraint::SetupRagdollAxisTorque( int axis, const constraint_axislimit_t &axisData, hk_Ragdoll_Constraint_BP *bp )
{
	static int axisMap[] = {0,2,1};
	bp->m_limits[axisMap[axis]].set_motor( DEG2RAD(axisData.angularVelocity), axisData.torque * m_pObjReference->GetMass() );
	bp->m_tau = 1.0f;
}

void CPhysicsConstraint::SetupRagdollAxis( int axis, const constraint_axislimit_t &axisData, hk_Limited_Ball_Socket_BP *ballsocketBP )
{
	static int axisMap[] = {0,2,1};
	
	// X & Y
	if ( axis != 2 )
	{
		ballsocketBP->m_angular_limits[ axisMap[axis] ].set( DEG2RAD(-axisData.maxRotation), DEG2RAD(-axisData.minRotation));
	}
	// Z
	else
	{
		ballsocketBP->m_angular_limits[ axisMap[axis] ].set( DEG2RAD(axisData.minRotation), DEG2RAD(axisData.maxRotation));
	}
}


// UNDONE: Keep this around to clip "includeStatic" code
#if 0
void CPhysicsConstraint::SetBreakLimit( float breakLimitForce, float breakLimitTorque, bool includeStatic )
{
	float factor = ConvertDistanceToIVP( 1.0f );
	
	// convert to ivp
	IVP_Environment *pEnvironment = m_pConstraint->get_associated_controlled_cores()->element_at(0)->environment;
	float gravity = pEnvironment->get_gravity()->real_length();
	breakLimitTorque = breakLimitTorque * gravity * factor;	// proportional to distance
	breakLimitForce = breakLimitForce * gravity;
	
	if ( breakLimitForce != 0 )
	{
		if ( includeStatic )
		{
			breakLimitForce += m_pObjAttached->GetMass() * gravity * pEnvironment->get_delta_PSI_time();
		}

		m_pConstraint->change_max_translation_impulse( IVP_CFE_BREAK, breakLimitForce );
	}
	else
	{
		m_pConstraint->change_max_translation_impulse( IVP_CFE_NONE, 0 );
	}

	if ( breakLimitTorque != 0 )
	{
		if ( includeStatic )
		{
			const IVP_U_Point *massCenter = m_pObjAttached->GetObject()->get_core()->get_position_PSI();

			IVP_U_Point tmp;
			tmp.set( massCenter );
			tmp.subtract( &m_constraintOrigin );
			float dist = tmp.real_length();
			breakLimitTorque += (m_pObjAttached->GetMass() * gravity * dist * pEnvironment->get_delta_PSI_time());
		}
		m_pConstraint->change_max_rotation_impulse( IVP_CFE_BREAK, breakLimitTorque );
	}
	else
	{
		m_pConstraint->change_max_rotation_impulse( IVP_CFE_NONE, 0 );
	}
}
#endif


IPhysicsObject *CPhysicsConstraint::GetReferenceObject( void )
{
	return m_pObjReference;
}


IPhysicsObject *CPhysicsConstraint::GetAttachedObject( void )
{
	return m_pObjAttached;
}

void SeedRandomGenerators()
{
	ivp_srand(1);
	hk_Math::srand01('h'+'a'+'v'+'o'+'k');
	qh_RANDOMseed_(1);
}

extern int ivp_srand_read(void);
void ReadRandom( int buffer[4] )
{
	buffer[0] = (int)hk_Math::hk_random_seed;
	buffer[1] = ivp_srand_read();
}

void WriteRandom( int buffer[4] )
{
	hk_Math::srand01((unsigned int)buffer[0]);
	ivp_srand(buffer[1]);
}


IPhysicsConstraint *GetClientDataForHkConstraint( class hk_Breakable_Constraint *pHkConstraint )
{
	return static_cast<CPhysicsConstraint *>( pHkConstraint->get_client_data() );
}

// Create a container for a group of constraints
IPhysicsConstraintGroup *CreatePhysicsConstraintGroup( IVP_Environment *pEnvironment )
{
	return new CPhysicsConstraintGroup( pEnvironment );
}

IPhysicsConstraint *CreateRagdollConstraint( IVP_Environment *pEnvironment, CPhysicsObject *pReferenceObject, CPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_ragdollparams_t &ragdoll )
{
	CPhysicsConstraint *pConstraint = new CPhysicsConstraint( pReferenceObject, pAttachedObject );
	pConstraint->InitRagdoll( pEnvironment, (CPhysicsConstraintGroup *)pGroup, ragdoll );
	return pConstraint;
}

IPhysicsConstraint *CreateHingeConstraint( IVP_Environment *pEnvironment, CPhysicsObject *pReferenceObject, CPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_hingeparams_t &hinge )
{
	CPhysicsConstraint *pConstraint = new CPhysicsConstraint( pReferenceObject, pAttachedObject );
	pConstraint->InitHinge( pEnvironment, (CPhysicsConstraintGroup *)pGroup, hinge );
	return pConstraint;
}

IPhysicsConstraint *CreateFixedConstraint( IVP_Environment *pEnvironment, CPhysicsObject *pReferenceObject, CPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_fixedparams_t &fixed )
{
	CPhysicsConstraint *pConstraint = new CPhysicsConstraint( pReferenceObject, pAttachedObject );
	pConstraint->InitFixed( pEnvironment, (CPhysicsConstraintGroup *)pGroup, fixed );
	return pConstraint;
}

IPhysicsConstraint *CreateSlidingConstraint( IVP_Environment *pEnvironment, CPhysicsObject *pReferenceObject, CPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_slidingparams_t &sliding )
{
	CPhysicsConstraint *pConstraint = new CPhysicsConstraint( pReferenceObject, pAttachedObject );
	pConstraint->InitSliding( pEnvironment, (CPhysicsConstraintGroup *)pGroup, sliding );
	return pConstraint;
}

IPhysicsConstraint *CreateBallsocketConstraint( IVP_Environment *pEnvironment, CPhysicsObject *pReferenceObject, CPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_ballsocketparams_t &ballsocket )
{
	CPhysicsConstraint *pConstraint = new CPhysicsConstraint( pReferenceObject, pAttachedObject );
	pConstraint->InitBallsocket( pEnvironment, (CPhysicsConstraintGroup *)pGroup, ballsocket );
	return pConstraint;
}

IPhysicsConstraint *CreatePulleyConstraint( IVP_Environment *pEnvironment, CPhysicsObject *pReferenceObject, CPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_pulleyparams_t &pulley )
{
	CPhysicsConstraint *pConstraint = new CPhysicsConstraint( pReferenceObject, pAttachedObject );
	pConstraint->InitPulley( pEnvironment, (CPhysicsConstraintGroup *)pGroup, pulley );
	return pConstraint;
}

IPhysicsConstraint *CreateLengthConstraint( IVP_Environment *pEnvironment, CPhysicsObject *pReferenceObject, CPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_lengthparams_t &length )
{
	CPhysicsConstraint *pConstraint = new CPhysicsConstraint( pReferenceObject, pAttachedObject );
	pConstraint->InitLength( pEnvironment, (CPhysicsConstraintGroup *)pGroup, length );
	return pConstraint;
}

// header.pGroup is optionally NULL, all other fields must be valid!
bool IsValidConstraint( const vphysics_save_cphysicsconstraint_t &header )
{
	if ( header.constraintType != CONSTRAINT_UNKNOWN && header.pObjAttached && header.pObjReference )
		return true;

	return false;
}

bool SavePhysicsConstraint( const physsaveparams_t &params, CPhysicsConstraint *pConstraint )
{
	vphysics_save_cphysicsconstraint_t header;
	vphysics_save_constraint_t constraintTemplate;
	memset( &header, 0, sizeof(header) );
	memset( &constraintTemplate, 0, sizeof(constraintTemplate) );

	pConstraint->WriteToTemplate( header, constraintTemplate );
	
	params.pSave->WriteAll( &header );
	if ( IsValidConstraint( header ) )
	{
		switch ( header.constraintType )
		{
		case CONSTRAINT_UNKNOWN:
			Assert(0);
			break;
		case CONSTRAINT_HINGE:
			params.pSave->WriteAll( &constraintTemplate.hinge );
			break;
		case CONSTRAINT_FIXED:
			params.pSave->WriteAll( &constraintTemplate.fixed );
			break;
		case CONSTRAINT_SLIDING:
			params.pSave->WriteAll( &constraintTemplate.sliding );
			break;
		case CONSTRAINT_PULLEY:
			params.pSave->WriteAll( &constraintTemplate.pulley );
			break;
		case CONSTRAINT_LENGTH:
			params.pSave->WriteAll( &constraintTemplate.length );
			break;
		case CONSTRAINT_BALLSOCKET:
			params.pSave->WriteAll( &constraintTemplate.ballsocket );
			break;
		case CONSTRAINT_RAGDOLL:
			params.pSave->WriteAll( &constraintTemplate.ragdoll );
			break;
		}
		return true;
	}
	// inert constraint, just save header
	return true;
}

bool RestorePhysicsConstraint( const physrestoreparams_t &params, CPhysicsConstraint **ppConstraint )
{
	vphysics_save_cphysicsconstraint_t header;
	memset( &header, 0, sizeof(header) );
	
	params.pRestore->ReadAll( &header );
	if ( IsValidConstraint( header ) )
	{
		switch ( header.constraintType )
		{
		case CONSTRAINT_UNKNOWN:
			Assert(0);
			break;
		case CONSTRAINT_HINGE:
			{
				vphysics_save_constrainthinge_t hinge;
				memset( &hinge, 0, sizeof(hinge) );
				params.pRestore->ReadAll( &hinge );
				CPhysicsEnvironment *pEnvironment = (CPhysicsEnvironment *)params.pEnvironment;
				*ppConstraint = (CPhysicsConstraint *)pEnvironment->CreateHingeConstraint( header.pObjReference, header.pObjAttached, header.pGroup, hinge );
			}
			break;
		case CONSTRAINT_FIXED:
			{
				vphysics_save_constraintfixed_t fixed;
				memset( &fixed, 0, sizeof(fixed) );
				params.pRestore->ReadAll( &fixed );
				CPhysicsEnvironment *pEnvironment = (CPhysicsEnvironment *)params.pEnvironment;
				*ppConstraint = (CPhysicsConstraint *)pEnvironment->CreateFixedConstraint( header.pObjReference, header.pObjAttached, header.pGroup, fixed );
			}
			break;
		case CONSTRAINT_SLIDING:
			{
				vphysics_save_constraintsliding_t sliding;
				memset( &sliding, 0, sizeof(sliding) );
				params.pRestore->ReadAll( &sliding );
				CPhysicsEnvironment *pEnvironment = (CPhysicsEnvironment *)params.pEnvironment;
				*ppConstraint = (CPhysicsConstraint *)pEnvironment->CreateSlidingConstraint( header.pObjReference, header.pObjAttached, header.pGroup, sliding );
			}
			break;
		case CONSTRAINT_PULLEY:
			{
				vphysics_save_constraintpulley_t pulley;
				memset( &pulley, 0, sizeof(pulley) );
				params.pRestore->ReadAll( &pulley );
				CPhysicsEnvironment *pEnvironment = (CPhysicsEnvironment *)params.pEnvironment;
				*ppConstraint = (CPhysicsConstraint *)pEnvironment->CreatePulleyConstraint( header.pObjReference, header.pObjAttached, header.pGroup, pulley );
			}
			break;
		case CONSTRAINT_LENGTH:
			{
				vphysics_save_constraintlength_t length;
				memset( &length, 0, sizeof(length) );
				params.pRestore->ReadAll( &length );
				CPhysicsEnvironment *pEnvironment = (CPhysicsEnvironment *)params.pEnvironment;
				*ppConstraint = (CPhysicsConstraint *)pEnvironment->CreateLengthConstraint( header.pObjReference, header.pObjAttached, header.pGroup, length );
			}
			break;
		case CONSTRAINT_BALLSOCKET:
			{
				vphysics_save_constraintballsocket_t ballsocket;
				memset( &ballsocket, 0, sizeof(ballsocket) );
				params.pRestore->ReadAll( &ballsocket );
				CPhysicsEnvironment *pEnvironment = (CPhysicsEnvironment *)params.pEnvironment;
				*ppConstraint = (CPhysicsConstraint *)pEnvironment->CreateBallsocketConstraint( header.pObjReference, header.pObjAttached, header.pGroup, ballsocket );
			}
			break;
		case CONSTRAINT_RAGDOLL:
			{
				vphysics_save_constraintragdoll_t ragdoll;
				memset( &ragdoll, 0, sizeof(ragdoll) );
				params.pRestore->ReadAll( &ragdoll );
				CPhysicsEnvironment *pEnvironment = (CPhysicsEnvironment *)params.pEnvironment;
				*ppConstraint = (CPhysicsConstraint *)pEnvironment->CreateRagdollConstraint( header.pObjReference, header.pObjAttached, header.pGroup, ragdoll );
			}
			break;
		}

		return true;
	}

	// inert constraint, create an empty shell
	*ppConstraint = new CPhysicsConstraint( NULL, NULL );
	return true;
}


bool SavePhysicsConstraintGroup( const physsaveparams_t &params, CPhysicsConstraintGroup *pConstraintGroup )
{
	vphysics_save_cphysicsconstraintgroup_t groupTemplate;
	memset( &groupTemplate, 0, sizeof(groupTemplate) );

	pConstraintGroup->WriteToTemplate( groupTemplate );
	params.pSave->WriteAll( &groupTemplate );
	return true;
}

bool RestorePhysicsConstraintGroup( const physrestoreparams_t &params, CPhysicsConstraintGroup **ppConstraintGroup )
{
	vphysics_save_cphysicsconstraintgroup_t groupTemplate;
	params.pRestore->ReadAll( &groupTemplate );
	CPhysicsEnvironment *pEnvironment = (CPhysicsEnvironment *)params.pEnvironment;
	*ppConstraintGroup = (CPhysicsConstraintGroup *)pEnvironment->CreateConstraintGroup();
	if ( *ppConstraintGroup && groupTemplate.isActive )
	{
		g_ConstraintGroupActivateList.AddToTail( *ppConstraintGroup );
	}
	return true;
}


void PostRestorePhysicsConstraintGroup()
{
	for ( int i = 0; i < g_ConstraintGroupActivateList.Count(); i++ )
	{
		g_ConstraintGroupActivateList[i]->Activate();
	}
	g_ConstraintGroupActivateList.Purge();
}
