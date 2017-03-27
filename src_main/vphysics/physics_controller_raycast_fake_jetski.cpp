//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "tier0/dbg.h"
#include "physics_controller_raycast_fake_jetski.h"
#include "ivp_material.hxx"
#include "ivp_ray_solver.hxx"
#include "ivp_cache_object.hxx"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPhysics_System_Raycast_Fake_Jetski::CPhysics_System_Raycast_Fake_Jetski( IVP_Environment *pEnv, const IVP_Template_Car_System *pCarSystem )
																		: IVP_Controller_Raycast_Fake_Jetski( pEnv, pCarSystem )
{
	m_bLean = false;
	InitSystemFakeJetski( pCarSystem );
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
CPhysics_System_Raycast_Fake_Jetski::~CPhysics_System_Raycast_Fake_Jetski()
{
}

//-----------------------------------------------------------------------------
// Purpose: Setup the car system wheels.
//-----------------------------------------------------------------------------
void CPhysics_System_Raycast_Fake_Jetski::InitSystemFakeJetski( const IVP_Template_Car_System *pCarSystem )
{
	for ( int iWheel = 0; iWheel < pCarSystem->n_wheels; ++iWheel )
	{
		m_pWheels[iWheel] = pCarSystem->car_wheel[iWheel];
		m_pWheels[iWheel]->enable_collision_detection( IVP_FALSE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the raycast wheel.
//-----------------------------------------------------------------------------
IPhysicsObject *CPhysics_System_Raycast_Fake_Jetski::GetWheel( int index )
{
	Assert( index >= 0 );
	Assert( index < n_wheels );

	return ( IPhysicsObject* )m_pWheels[index]->client_data;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPhysics_System_Raycast_Fake_Jetski::SetWheelFriction( int iWheel, float flFriction )
{
	change_friction_of_wheel( IVP_POS_WHEEL( iWheel ), flFriction );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPhysics_System_Raycast_Fake_Jetski::SetLeanBack( bool bLean )
{
	m_bLean = bLean;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CPhysics_System_Raycast_Fake_Jetski::InLeanBack( void )
{
	return m_bLean;
}

//-----------------------------------------------------------------------------
// Purpose: Setup the car system wheels.
//-----------------------------------------------------------------------------
void CPhysics_System_Raycast_Fake_Jetski::do_raycasts( IVP_Event_Sim *es,
						                               int n_wheels,
						                               class IVP_Ray_Solver_Template *t_in,
						                               class IVP_Ray_Hit *hits_out,
						                               IVP_FLOAT *friction_of_object_out )
{
	IVP_ASSERT( n_wheels == 4 );

	IVP_Ray_Solver_Min ray_solver0(&t_in[0]);
	IVP_Ray_Solver_Min ray_solver1(&t_in[1]); 
	IVP_Ray_Solver_Min ray_solver2(&t_in[2]);
	IVP_Ray_Solver_Min ray_solver3(&t_in[3]);
    
    IVP_Ray_Solver_Min *solvers[4] = { &ray_solver0, &ray_solver1, &ray_solver2, &ray_solver3 };
    IVP_Ray_Solver_Group rs_group( n_wheels, (IVP_Ray_Solver **)solvers );

	// Debug!
#if 0
	{
		IVP_CarSystemDebugData_t carSystemDebugData;
		GetCarSystemDebugData( carSystemDebugData );
		carSystemDebugData.wheelRaycasts[0][0] = ray_solver0.ray_start_point;
		carSystemDebugData.wheelRaycasts[0][1] = ray_solver0.ray_end_point;
		carSystemDebugData.wheelRaycasts[1][0] = ray_solver1.ray_start_point;
		carSystemDebugData.wheelRaycasts[1][1] = ray_solver1.ray_end_point;
		carSystemDebugData.wheelRaycasts[2][0] = ray_solver2.ray_start_point;
		carSystemDebugData.wheelRaycasts[2][1] = ray_solver2.ray_end_point;
		carSystemDebugData.wheelRaycasts[3][0] = ray_solver3.ray_start_point;
		carSystemDebugData.wheelRaycasts[3][1] = ray_solver3.ray_end_point;
	}
#endif

    // check which objects are hit	    
    rs_group.check_ray_group_against_all_objects_in_sim( es->environment );

    for ( int i = 0; i < n_wheels; i++ )
	{
		IVP_Ray_Hit *hit = solvers[i]->get_ray_hit();
		if (hit)
		{
			hits_out[i] = *hit;
			friction_of_object_out[i] = hit->hit_real_object->l_default_material->get_friction_factor();

			// Debug!
#if 0
			{
				if ( solvers[i]->ray_length != 0.0f )
				{
					carSystemDebugData.wheelRaycastImpacts[i] = ( hit->hit_distance / solvers[i]->ray_length );
				}
				else
				{
					carSystemDebugData.wheelRaycastImpacts[i] = 0.0f;
				}
			}
#endif
		}
		else
		{
			memset( &hits_out[i], 0, sizeof(IVP_Ray_Hit) );
			friction_of_object_out[i] = 0;

			// Debug!
#if 0
			{
				carSystemDebugData.wheelRaycastImpacts[i] = 0.0f;
			}
#endif
		}
    }

	// Debug!
#if 0
	{	
		SetCarSystemDebugData( carSystemDebugData );
	}
#endif
}

// Not really necessary except for debug??
void CPhysics_System_Raycast_Fake_Jetski::update_wheel_positions( void )
{
	// Get the car body object.
    IVP_Cache_Object *pCacheObject = m_pJetskiBody->get_cache_object();

	// Get the core (vehicle) matrix.
    IVP_U_Matrix m_core_f_object;    
	m_pJetskiBody->calc_m_core_f_object( &m_core_f_object );
    
    for ( int iWheel = 0; iWheel < n_wheels; ++iWheel )
	{
		// Get the current raycast wheel.
		IVP_Raycast_Fake_Jetski_Wheel *pRaycastWheel = get_wheel( IVP_POS_WHEEL( iWheel ) );

		// Get the position of the wheel in vehicle core space.
		IVP_U_Float_Point hp_cs; 
		hp_cs.add_multiple( &pRaycastWheel->hp_cs, &pRaycastWheel->spring_direction_cs, pRaycastWheel->raycast_dist - pRaycastWheel->wheel_radius );

		// Get the position on vehicle object space (inverse transform).
		IVP_U_Float_Point hp_os; 
		m_core_f_object.vimult4( &hp_cs, &hp_os );

		// Transform the wheel position from object space into world space.
		IVP_U_Point hp_ws; 
		pCacheObject->transform_position_to_world_coords( &hp_os, &hp_ws );
		
		// Apply rotational component.
		IVP_U_Point wheel_cs( &pRaycastWheel->axis_direction_cs );
		IVP_U_Point wheel2_cs( 0 ,0 ,0 ); 
		wheel2_cs.k[index_y] = -1.0;
		wheel2_cs.rotate( IVP_COORDINATE_INDEX( index_x ), pRaycastWheel->angle_wheel );
		
		IVP_U_Matrix3 m_core_f_wheel;
		m_core_f_wheel.init_normized3_col( &wheel_cs, IVP_COORDINATE_INDEX( index_x ), &wheel2_cs );
		
		IVP_U_Matrix3 m_world_f_wheel;
		pCacheObject->m_world_f_object.mmult3( &m_core_f_wheel, &m_world_f_wheel ); // bid hack, assumes cs = os (for rotation);
		
		IVP_U_Quat rot_ws; 
		rot_ws.set_quaternion( &m_world_f_wheel );
		m_pWheels[iWheel]->beam_object_to_new_position( &rot_ws, &hp_ws );
    }

    pCacheObject->remove_reference();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPhysics_System_Raycast_Fake_Jetski::update_leanback_force( void )
{
	if ( m_bLean )
	{
		// Get the jetski core.
		IVP_Core *pJetskiCore = m_pJetskiBody->get_core();
	    const IVP_U_Matrix *matWorldFromCore = pJetskiCore->get_m_world_f_core_PSI();
		
		// Apply force to front ray-trace points only.
		for ( int iWheel = 0; iWheel < 2; ++iWheel )
		{
			// Get the current raycast wheel.
			IVP_Raycast_Fake_Jetski_Wheel *pRaycastWheel = get_wheel( IVP_POS_WHEEL( iWheel ) );
			IVP_U_Float_Point vecPoint;

			
		}
	}
}
