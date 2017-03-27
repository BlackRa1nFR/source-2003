//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PHYSICS_SHADOW_H
#define PHYSICS_SHADOW_H
#pragma once

class CPhysicsObject;
class IPhysicsShadowController;
class IPhysicsPlayerController;

extern IPhysicsShadowController *CreateShadowController( CPhysicsObject *pObject, bool allowTranslation, bool allowRotation );
extern IPhysicsPlayerController *CreatePlayerController( CPhysicsObject *pObject );


extern void ComputeController( IVP_U_Float_Point &currentSpeed, const IVP_U_Float_Point &delta, const IVP_U_Float_Point &maxSpeed, float scaleDelta, float damping );

#include "ivp_physics.hxx"

struct shadowcontrol_params_t
{
	shadowcontrol_params_t() { lastPosition.set_to_zero(); }

	IVP_U_Point			targetPosition;
	IVP_U_Quat			targetRotation;
	IVP_U_Float_Point	maxSpeed;
	IVP_U_Float_Point	maxAngular;
	IVP_U_Point			lastPosition;		// estimate for where the object should be, used to compute error for teleport
	float				dampFactor;
	float				teleportDistance;
};


float ComputeShadowControllerHL( CPhysicsObject *pObject, const hlshadowcontrol_params_t &params, float secondsToArrival, float dt );
float ComputeShadowControllerIVP( IVP_Real_Object *pivp, shadowcontrol_params_t &params, float secondsToArrival, float dt );

#endif // PHYSICS_SHADOW_H
