//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef PHYSICS_CONTROLLER_RAYCAST_FAKE_JETSKI_H
#define PHYSICS_CONTROLLER_RAYCAST_FAKE_JETSKI_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"
#include "ivp_physics.hxx"
#include "ivp_controller.hxx"
#include "ivp_car_system.hxx"
#include "ivp_controller_fake_jetski.h"

class IPhysicsObject;

//=============================================================================
//
// Raycast Fake Jetski System
//
class CPhysics_System_Raycast_Fake_Jetski : public IVP_Controller_Raycast_Fake_Jetski 
{

public:

    CPhysics_System_Raycast_Fake_Jetski( IVP_Environment *env, const IVP_Template_Car_System *t );
    virtual ~CPhysics_System_Raycast_Fake_Jetski();

    virtual void do_raycasts( IVP_Event_Sim *, int n_wheels, IVP_Ray_Solver_Template *t_in,
			                  IVP_Ray_Hit *hits_out, IVP_FLOAT *friction_of_object_out );

    void update_wheel_positions( void );
	void update_leanback_force( void );

	void SetWheelFriction( int iWheel, float flFriction );
	void SetLeanBack( bool bLean );
	bool InLeanBack( void );

	IPhysicsObject *GetWheel( int index );

protected:

	void InitSystemFakeJetski( const IVP_Template_Car_System *pCarSystem );

    IVP_Real_Object *m_pWheels[IVP_RAYCAST_FAKE_JETSKI_MAX_WHEELS];
	bool			 m_bLean;
};

#endif // PHYSICS_CONTROLLER_RAYCAST_FAKE_JETSKI_H
