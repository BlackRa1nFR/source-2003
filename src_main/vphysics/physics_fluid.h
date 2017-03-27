//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PHYSICS_FLUID_H
#define PHYSICS_FLUID_H
#pragma once

#include "vphysics_interface.h"

class IVP_Compact_Surface;
class IVP_Environment;
class IVP_Listener_Phantom;
class CBuoyancyAttacher;
class IVP_Liquid_Surface_Descriptor_Simple;
class CPhysicsObject;
class CPhysicsObject;

class CPhysicsFluidController : public IPhysicsFluidController
{
public:
	CPhysicsFluidController( CBuoyancyAttacher *pBuoy, IVP_Liquid_Surface_Descriptor_Simple *pLiquid, CPhysicsObject *pObject );
	~CPhysicsFluidController( void );

	void SetGameData( void *pGameData );
	void *GetGameData( void ) const;

	void GetSurfacePlane( Vector *pNormal, float *pDist );

	class IVP_Real_Object *GetIVPObject();
	float GetDensity();

private:
	CBuoyancyAttacher					*m_pBuoyancy;
	IVP_Liquid_Surface_Descriptor_Simple *m_pLiquidSurface;
	CPhysicsObject						*m_pObject;
	void *m_pGameData;
};

extern CPhysicsFluidController *CreateFluidController( IVP_Environment *pEnvironment, CPhysicsObject *pFluidObject, fluidparams_t *pParams );


#endif // PHYSICS_FLUID_H
