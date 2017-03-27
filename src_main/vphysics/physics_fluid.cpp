#include "vphysics_interface.h"
#include "physics_fluid.h"
#include "physics_object.h"
#include "physics_material.h"
#include "convert.h"

#include <stdio.h>

#include "ivp_physics.hxx"
#include "ivp_core.hxx"
#include "ivp_compact_surface.hxx"
#include "ivp_surman_polygon.hxx"
#include "ivp_templates.hxx"
#include "ivp_phantom.hxx"
#include "ivp_controller_buoyancy.hxx"
#include "ivp_liquid_surface_descript.hxx"

// NOTE: This is auto-deleted by the phantom controller
class CBuoyancyAttacher : public IVP_Attacher_To_Cores_Buoyancy
{
public:
    virtual IVP_Template_Buoyancy *get_parameters_per_core( IVP_Core *pCore );
    CBuoyancyAttacher(IVP_Template_Buoyancy &templ, IVP_U_Set_Active<IVP_Core> *set_of_cores_, IVP_Liquid_Surface_Descriptor *liquid_surface_descriptor_);

	float m_density;
};

CPhysicsFluidController::CPhysicsFluidController( CBuoyancyAttacher *pBuoy, IVP_Liquid_Surface_Descriptor_Simple *pLiquid, CPhysicsObject *pObject )
{
	m_pBuoyancy = pBuoy;
	m_pLiquidSurface = pLiquid;
	m_pObject = pObject;
}

CPhysicsFluidController::~CPhysicsFluidController( void )
{
	delete m_pLiquidSurface;
}

void CPhysicsFluidController::SetGameData( void *pGameData ) 
{ 
	m_pGameData = pGameData; 
}

void *CPhysicsFluidController::GetGameData( void ) const 
{ 
	return m_pGameData; 
}

void CPhysicsFluidController::GetSurfacePlane( Vector *pNormal, float *pDist )
{
	ConvertPlaneToHL( m_pLiquidSurface->surface, pNormal, pDist );
	if ( pNormal )
	{
		*pNormal *= -1;
	}
	if ( pDist )
	{
		*pDist *= -1;
	}
}

IVP_Real_Object *CPhysicsFluidController::GetIVPObject() 
{ 
	return m_pObject->GetObject();
}

float CPhysicsFluidController::GetDensity()
{
	return m_pBuoyancy->m_density;
}


IVP_Template_Buoyancy *CBuoyancyAttacher::get_parameters_per_core( IVP_Core *pCore )
{
	if ( pCore )
	{
		IVP_Real_Object *pivp = pCore->objects.element_at(0);
		CPhysicsObject *pPhys = static_cast<CPhysicsObject *>(pivp->client_data);

		// This ratio is for objects whose mass / (collision model) volume is not equal to their density.
		// Keep the fluid pressure/friction solution for the volume, but scale the buoyant force calculations
		// to be in line with the object's real density.  This is accompilshed by changing the fluid's density
		// on a per-object basis.
		float ratio = pPhys->GetBuoyancyRatio();

		if ( pPhys->GetShadowController() || !(pPhys->GetCallbackFlags() & CALLBACK_DO_FLUID_SIMULATION) )
		{
			// NOTE: don't do buoyancy on these guys for now!
			template_buoyancy.medium_density = 0;
		}
		else
		{
			template_buoyancy.medium_density = m_density * ratio;
		}
	}
	else
	{
		template_buoyancy.medium_density = m_density;
	}

	return &template_buoyancy;
}

CBuoyancyAttacher::CBuoyancyAttacher(IVP_Template_Buoyancy &templ, IVP_U_Set_Active<IVP_Core> *set_of_cores_, IVP_Liquid_Surface_Descriptor *liquid_surface_descriptor_)
	:IVP_Attacher_To_Cores_Buoyancy(templ, set_of_cores_, liquid_surface_descriptor_)
{
	m_density = templ.medium_density;
}


CPhysicsFluidController *CreateFluidController( IVP_Environment *pEnvironment, CPhysicsObject *pFluidObject, fluidparams_t *pParams )
{
	pFluidObject->BecomeTrigger();

	IVP_Controller_Phantom *pPhantom = pFluidObject->GetObject()->get_controller_phantom();
	if ( !pPhantom )
		return NULL;

    // ------------------------------------
    // first initialize the water's surface
    // ------------------------------------
    // note that this surface is an infinite plane!
	for ( int i = 0; i < 4; i++ )
	{
		pParams->surfacePlane[i] = -pParams->surfacePlane[i];
	}

    IVP_U_Float_Hesse water_surface;
	ConvertPlaneToIVP( pParams->surfacePlane.AsVector3D(), pParams->surfacePlane[3], water_surface );
	
    IVP_U_Float_Point abs_speed_of_current_ws;
	ConvertPositionToIVP( pParams->currentVelocity, abs_speed_of_current_ws );

    IVP_Liquid_Surface_Descriptor_Simple *lsd = new IVP_Liquid_Surface_Descriptor_Simple(&water_surface, &abs_speed_of_current_ws);

    // ---------------------------------------------
    // create parameter template for Buoyancy_Solver
    // ---------------------------------------------
	// UNDONE: Expose these other parameters
    IVP_Template_Buoyancy buoyancy_input;
    buoyancy_input.medium_density			= ConvertDensityToIVP(pParams->density); // density of water (unit: kg/m^3)
    buoyancy_input.pressure_damp_factor     = pParams->damping;
    buoyancy_input.viscosity_factor       = 0.0f;
    buoyancy_input.torque_factor          = 0.01f;
    buoyancy_input.viscosity_input_factor = 0.1f;
    // -------------------------------------------------------------------------------
    // create "water" (i.e. buoyancy solver) and attach a dynamic list of object cores
    // -------------------------------------------------------------------------------
    CBuoyancyAttacher *attacher_to_cores_buoyancy = new CBuoyancyAttacher( buoyancy_input, pPhantom->get_intruding_cores(), lsd );

	CPhysicsFluidController *pFluid = new CPhysicsFluidController( attacher_to_cores_buoyancy, lsd, pFluidObject );
	pFluid->SetGameData( pParams->pGameData );
	pPhantom->client_data = static_cast<void *>(pFluid);

	return pFluid;
}



bool SavePhysicsFluidController( const physsaveparams_t &params, CPhysicsFluidController *pFluidObject )
{
	return false;
}

bool RestorePhysicsFluidController( const physrestoreparams_t &params, CPhysicsFluidController **ppFluidObject )
{
	return false;
}

