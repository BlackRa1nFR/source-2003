//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "tier0/dbg.h"
#include "physics_airboat.h"
#include "physics_material.h"
#include "ivp_material.hxx"
#include "ivp_ray_solver.hxx"
#include "ivp_cache_object.hxx"
#include "cmodel.h"
#include "convert.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPhysics_Airboat::CPhysics_Airboat( IVP_Environment *pEnv, const IVP_Template_Car_System *pCarSystem,
								    IPhysicsGameTrace *pGameTrace )
									: IVP_Controller_Raycast_Airboat( pEnv, pCarSystem )
{
	InitAirboat( pCarSystem );
	m_pGameTrace = pGameTrace;
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
CPhysics_Airboat::~CPhysics_Airboat()
{
}

//-----------------------------------------------------------------------------
// Purpose: Setup the car system wheels.
//-----------------------------------------------------------------------------
void CPhysics_Airboat::InitAirboat( const IVP_Template_Car_System *pCarSystem )
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
IPhysicsObject *CPhysics_Airboat::GetWheel( int index )
{
	Assert( index >= 0 );
	Assert( index < n_wheels );

	return ( IPhysicsObject* )m_pWheels[index]->client_data;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPhysics_Airboat::SetWheelFriction( int iWheel, float flFriction )
{
	change_friction_of_wheel( IVP_POS_WHEEL( iWheel ), flFriction );
}

//-----------------------------------------------------------------------------
// Purpose:: Convert data to HL2 measurements, and test direction of raycast.
//-----------------------------------------------------------------------------
void CPhysics_Airboat::pre_raycasts_gameside( int nRaycastCount, IVP_Ray_Solver_Template *pRays,
											  Ray_t *pGameRays, IVP_Raycast_Airboat_Impact *pImpacts )
{
	for ( int iRaycast = 0; iRaycast < nRaycastCount; ++iRaycast )
	{
		// Setup the ray.
		Vector vecStart;
		ConvertPositionToHL( pRays[iRaycast].ray_start_point, vecStart );

		Vector vecEnd;
		Vector vecDirection;
		ConvertDirectionToHL( pRays[iRaycast].ray_normized_direction, vecDirection );
		float flRayLength = IVP2HL( pRays[iRaycast].ray_length );

		// Check to see if that point is in water.
		pImpacts[iRaycast].bInWater = IVP_FALSE;
		if ( m_pGameTrace->VehiclePointInWater( vecStart ) )
		{
			vecDirection.Negate();
			pImpacts[iRaycast].bInWater = IVP_TRUE;
		}

		vecEnd = vecStart + ( vecDirection * flRayLength );

		// Shorten the trace.
		if ( m_pGameTrace->VehiclePointInWater( vecEnd ) )
		{
			pRays[iRaycast].ray_length = AIRBOAT_RAYCAST_DIST_WATER;
			flRayLength = IVP2HL( pRays[iRaycast].ray_length );
			vecEnd = vecStart + ( vecDirection * flRayLength );
		}

		Vector vecZero( 0.0f, 0.0f, 0.0f );
		pGameRays[iRaycast].Init( vecStart, vecEnd, vecZero, vecZero );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPhysics_Airboat::do_raycasts_gameside( int nRaycastCount, IVP_Ray_Solver_Template *pRays,
										     IVP_Raycast_Airboat_Impact *pImpacts )
{
	Assert( nRaycastCount >= 0 );
	Assert( nRaycastCount <= IVP_RAYCAST_AIRBOAT_MAX_WHEELS );

	Ray_t gameRays[IVP_RAYCAST_AIRBOAT_MAX_WHEELS];
	pre_raycasts_gameside( nRaycastCount, pRays, gameRays, pImpacts );

	// Do the raycasts and set impact data.
	trace_t trace;
	for ( int iRaycast = 0; iRaycast < nRaycastCount; ++iRaycast )
	{
		// Trace.
		if ( pImpacts[iRaycast].bInWater )
		{
			IPhysicsObject *pPhysAirboat = static_cast<IPhysicsObject*>( m_pAirboatBody->client_data );
			m_pGameTrace->VehicleTraceRay( gameRays[iRaycast], pPhysAirboat->GetGameData(), &trace ); 
		}
		else
		{
			IPhysicsObject *pPhysAirboat = static_cast<IPhysicsObject*>( m_pAirboatBody->client_data );
			m_pGameTrace->VehicleTraceRayWithWater( gameRays[iRaycast], pPhysAirboat->GetGameData(), &trace ); 
		}
		
		// Set impact data.
		if ( trace.fraction == 1.0f )
		{
			pImpacts[iRaycast].bImpact = IVP_FALSE;
		}
		else
		{
			pImpacts[iRaycast].bImpact = IVP_TRUE;

			// Set water surface flag.
			pImpacts[iRaycast].bImpactWater = IVP_FALSE;
			if ( trace.contents & MASK_WATER )
			{
				pImpacts[iRaycast].bImpactWater = IVP_TRUE;
			}

			// Save impact surface data.
			ConvertPositionToIVP( trace.endpos, pImpacts[iRaycast].vecImpactPointWS );
			ConvertDirectionToIVP( trace.plane.normal, pImpacts[iRaycast].vecImpactNormalWS );

			// Save surface properties.
			surfacedata_t *pSurfaceData = physprops->GetSurfaceData( trace.surface.surfaceProps );
			pImpacts[iRaycast].flDampening = pSurfaceData->dampening;
			pImpacts[iRaycast].flFriction = pSurfaceData->friction;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPhysics_Airboat::update_wheel_positions( void )
{
	return;
}
