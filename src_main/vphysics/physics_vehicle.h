//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PHYSICS_VEHICLE_H
#define PHYSICS_VEHICLE_H
#ifdef _WIN32
#pragma once
#endif


struct vehicleparams_t;
class IPhysicsVehicleController;
class CPhysicsObject;
class CPhysicsEnvironment;
class IVP_Real_Object;

bool IsVehicleWheel( IVP_Real_Object *pivp );

IPhysicsVehicleController *CreateVehicleController( CPhysicsEnvironment *pEnv, CPhysicsObject *pBodyObject, const vehicleparams_t &params, unsigned int nVehicleType, IPhysicsGameTrace *pGameTrace );

#endif // PHYSICS_VEHICLE_H
