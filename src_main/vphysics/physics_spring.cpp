#include <stdio.h>
#include "physics_object.h"
#include "physics_environment.h"
#include "convert.h"

#include "ivp_physics.hxx"
#include "ivp_core.hxx"
#include "ivp_templates.hxx"
#include "ivp_actuator.hxx"
#include "ivp_actuator_spring.hxx"
#include "ivp_listener_object.hxx"
#include "isaverestore.h"
#include "vphysics_saverestore.h"

struct vphysics_save_cphysicsspring_t : public springparams_t
{
	CPhysicsObject *pObjStart;
	CPhysicsObject *pObjEnd;
	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( vphysics_save_cphysicsspring_t )
	DEFINE_FIELD( vphysics_save_cphysicsspring_t,		constant,		FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_cphysicsspring_t,		naturalLength,		FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_cphysicsspring_t,		damping,		FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_cphysicsspring_t,		relativeDamping,		FIELD_FLOAT ),
	DEFINE_FIELD( vphysics_save_cphysicsspring_t,		startPosition,		FIELD_POSITION_VECTOR	),
	DEFINE_FIELD( vphysics_save_cphysicsspring_t,		endPosition,		FIELD_POSITION_VECTOR	),
	DEFINE_FIELD( vphysics_save_cphysicsspring_t,		useLocalPositions,	FIELD_BOOLEAN	),
	DEFINE_FIELD( vphysics_save_cphysicsspring_t,		onlyStretch,	FIELD_BOOLEAN	),

	DEFINE_VPHYSPTR( vphysics_save_cphysicsspring_t, pObjStart ),
	DEFINE_VPHYSPTR( vphysics_save_cphysicsspring_t, pObjEnd ),
END_DATADESC()


// BUGBUG: No way to delete a spring because springs get auto-deleted without notification
// when an object they are attached to is deleted.
// So there is no way to tell if the pointer is valid.
class CPhysicsSpring : public IPhysicsSpring, public IVP_Listener_Object
{
public:
	CPhysicsSpring( CPhysicsObject *pObjectStart, CPhysicsObject *pObjectEnd, IVP_Actuator_Spring *pSpring );
	~CPhysicsSpring();

	// IPhysicsSpring
	void GetEndpoints( Vector* worldPositionStart, Vector* worldPositionEnd );
	void SetSpringConstant( float flSpringContant);
	void SetSpringDamping( float flSpringDamping);
	void SetSpringLength( float flSpringLength);
	IPhysicsObject *GetStartObject( void ) { return m_pObjStart; }
	IPhysicsObject *GetEndObject( void ) { return m_pObjEnd; }

	void AttachListener();
	void DetachListener();

	// Object listener
    virtual void event_object_deleted( IVP_Event_Object *);
    virtual void event_object_created( IVP_Event_Object *) {}
    virtual void event_object_revived( IVP_Event_Object *) {}
    virtual void event_object_frozen ( IVP_Event_Object *) {}
	void WriteToTemplate( vphysics_save_cphysicsspring_t &params );

private:

	CPhysicsSpring( void );
	IVP_Actuator_Spring		*m_pSpring;
	CPhysicsObject			*m_pObjStart;
	CPhysicsObject			*m_pObjEnd;
};

CPhysicsSpring::CPhysicsSpring( CPhysicsObject *pObjectStart, CPhysicsObject *pObjectEnd, IVP_Actuator_Spring *pSpring ) : m_pSpring(pSpring)
{
	m_pObjStart = pObjectStart;
	m_pObjEnd = pObjectEnd;
	AttachListener();
}

void CPhysicsSpring::AttachListener()
{
	if ( !(m_pObjStart->GetCallbackFlags() & CALLBACK_NEVER_DELETED) )
	{
		m_pObjStart->GetObject()->add_listener_object( this );
	}

	if ( !(m_pObjEnd->GetCallbackFlags() & CALLBACK_NEVER_DELETED) )
	{
		m_pObjEnd->GetObject()->add_listener_object( this );
	}
}

CPhysicsSpring::~CPhysicsSpring()
{
	if ( m_pSpring )
	{
		DetachListener();
		delete m_pSpring;
	}
}

void CPhysicsSpring::DetachListener()
{
	if ( !(m_pObjStart->GetCallbackFlags() & CALLBACK_NEVER_DELETED) )
	{
		m_pObjStart->GetObject()->remove_listener_object( this );
	}

	if ( !(m_pObjEnd->GetCallbackFlags() & CALLBACK_NEVER_DELETED) )
	{
		m_pObjEnd->GetObject()->remove_listener_object( this );
	}

	m_pObjStart = NULL;
	m_pObjEnd = NULL;
	m_pSpring = NULL;
}

void CPhysicsSpring::event_object_deleted( IVP_Event_Object * )
{
	// the spring is going to delete itself now, so NULL it.
	DetachListener();
}

void CPhysicsSpring::GetEndpoints( Vector* worldPositionStart, Vector* worldPositionEnd )
{
	IVP_U_Point world_coords;

	if ( worldPositionStart )
	{
		const IVP_Anchor *anchor = m_pSpring->get_actuator_anchor(0);
		TransformLocalToIVP( anchor->object_pos, world_coords, m_pObjStart->GetObject(), true );
		ConvertPositionToHL( world_coords, *worldPositionStart );
	}

	if ( worldPositionEnd )
	{
		const IVP_Anchor *anchor = m_pSpring->get_actuator_anchor(1);
		TransformLocalToIVP( anchor->object_pos, world_coords, m_pObjEnd->GetObject(), true );
		ConvertPositionToHL( world_coords, *worldPositionEnd );
	}
}

void CPhysicsSpring::SetSpringConstant( float flSpringConstant )
{
	if ( m_pSpring )
	{
		float currentConstant = m_pSpring->get_constant();
		if ( currentConstant != flSpringConstant )
		{
			m_pSpring->set_constant(flSpringConstant);
		}
	}
}

void CPhysicsSpring::SetSpringDamping( float flSpringDamping )
{
	if ( m_pSpring )
	{
		m_pSpring->set_damp(flSpringDamping);
	}
}



void CPhysicsSpring::SetSpringLength( float flSpringLength )
{
	if ( m_pSpring )
	{
		float currentLengthIVP = m_pSpring->get_spring_length_zero_force();
		float desiredLengthIVP = ConvertDistanceToIVP(flSpringLength);

		// must change enough, or skip to keep objects sleeping
		const float SPRING_LENGTH_EPSILON = 1e-3f;
		if ( fabs(desiredLengthIVP-currentLengthIVP) < ConvertDistanceToIVP(SPRING_LENGTH_EPSILON)  )
			return;

		m_pSpring->set_len( desiredLengthIVP );
	}
}

void CPhysicsSpring::WriteToTemplate( vphysics_save_cphysicsspring_t &params )
{
	params.constant = m_pSpring->get_constant();
	params.naturalLength = ConvertDistanceToHL( m_pSpring->get_spring_length_zero_force() );
	params.damping = m_pSpring->get_damp_factor();
	params.relativeDamping = m_pSpring->get_rel_pos_damp();

	const IVP_Anchor *anchor0 = m_pSpring->get_actuator_anchor(0);
	ConvertPositionToHL( anchor0->object_pos, params.startPosition );
	const IVP_Anchor *anchor1 = m_pSpring->get_actuator_anchor(1);
	ConvertPositionToHL( anchor1->object_pos, params.endPosition );
	params.useLocalPositions = true;

	params.onlyStretch = m_pSpring->get_only_stretch() ? true : false;
	params.pObjStart = m_pObjStart;
	params.pObjEnd = m_pObjEnd;
}


IPhysicsSpring *CreateSpring( IVP_Environment *pEnvironment, CPhysicsObject *pObjectStart, CPhysicsObject *pObjectEnd, springparams_t *pParams )
{
	// fill in template
	IVP_Template_Spring spring_template;
	
	spring_template.spring_values_are_relative=IVP_FALSE;
	spring_template.spring_constant		= pParams->constant;
	spring_template.spring_len			= ConvertDistanceToIVP( pParams->naturalLength );
	spring_template.spring_damp			= pParams->damping;
	spring_template.rel_pos_damp		= pParams->relativeDamping;

	spring_template.spring_force_only_on_stretch = pParams->onlyStretch ? IVP_TRUE : IVP_FALSE;

	// Create anchors for the objects
	IVP_Template_Anchor anchorTemplateObjectStart, anchorTemplateObjectEnd;
	// create spring

	IVP_U_Float_Point ivpPosStart;
	IVP_U_Float_Point ivpPosEnd;

	if ( !pParams->useLocalPositions )
	{
		Vector local;
		pObjectStart->WorldToLocal( local, pParams->startPosition );
		ConvertPositionToIVP( local, ivpPosStart );

		pObjectEnd->WorldToLocal( local, pParams->endPosition );
		ConvertPositionToIVP( local, ivpPosEnd );
	}
	else
	{
		ConvertPositionToIVP( pParams->startPosition, ivpPosStart );
		ConvertPositionToIVP( pParams->endPosition, ivpPosEnd );
	}

	anchorTemplateObjectStart.set_anchor_position_os( pObjectStart->GetObject(), &ivpPosStart );
	anchorTemplateObjectEnd.set_anchor_position_os( pObjectEnd->GetObject(), &ivpPosEnd );

	spring_template.anchors[0] = &anchorTemplateObjectStart;
	spring_template.anchors[1] = &anchorTemplateObjectEnd;
	IVP_Actuator_Spring *pSpring = pEnvironment->create_spring( &spring_template );

	return new CPhysicsSpring( pObjectStart, pObjectEnd, pSpring );
}


bool SavePhysicsSpring( const physsaveparams_t &params, CPhysicsSpring *pSpring )
{
	vphysics_save_cphysicsspring_t springTemplate;
	memset( &springTemplate, 0, sizeof(springTemplate) );

	pSpring->WriteToTemplate( springTemplate );
	params.pSave->WriteAll( &springTemplate );
	return true;
}

bool RestorePhysicsSpring( const physrestoreparams_t &params, CPhysicsSpring **ppSpring )
{
	vphysics_save_cphysicsspring_t springTemplate;
	memset( &springTemplate, 0, sizeof(springTemplate) );
	params.pRestore->ReadAll( &springTemplate );
	CPhysicsEnvironment *pEnvironment = (CPhysicsEnvironment *)params.pEnvironment;
	*ppSpring = (CPhysicsSpring *)pEnvironment->CreateSpring( springTemplate.pObjStart, springTemplate.pObjEnd, &springTemplate );

	return true;
}

